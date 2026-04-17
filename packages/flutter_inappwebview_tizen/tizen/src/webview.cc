// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webview.h"

#include <Ecore_Evas.h>
#include <app_common.h>
#include <flutter/standard_method_codec.h>
#include <flutter_texture_registrar.h>
#include <tbm_surface.h>

#include "buffer_pool.h"
#include "log.h"
#include "webview_factory.h"

namespace {

constexpr size_t kBufferPoolSize = 5;
constexpr char kEwkInstance[] = "ewk_instance";
constexpr char kInAppWebViewChannelName[] =
    "com.pichillilorenzo/flutter_inappwebview_";
constexpr char kInAppWebViewCookieManagerChannelName[] =
    "com.pichillilorenzo/flutter_inappwebview_cookiemanager";
constexpr int kConsoleMessageTip = 0;
constexpr int kConsoleMessageLog = 1;
constexpr int kConsoleMessageWarning = 2;
constexpr int kConsoleMessageError = 3;
constexpr int kConsoleMessageDebug = 4;
constexpr int kWebResourceErrorUnknown = -1;

int ConvertLogLevel(Ewk_Console_Message_Level level) {
  switch (level) {
    case EWK_CONSOLE_MESSAGE_LEVEL_NULL:
    case EWK_CONSOLE_MESSAGE_LEVEL_LOG:
      return kConsoleMessageLog;
    case EWK_CONSOLE_MESSAGE_LEVEL_WARNING:
      return kConsoleMessageWarning;
    case EWK_CONSOLE_MESSAGE_LEVEL_ERROR:
      return kConsoleMessageError;
    case EWK_CONSOLE_MESSAGE_LEVEL_DEBUG:
      return kConsoleMessageDebug;
    case EWK_CONSOLE_MESSAGE_LEVEL_INFO:
      return kConsoleMessageTip;
    default:
      return kConsoleMessageLog;
  }
}

class NavigationRequestResult : public FlMethodResult {
 public:
  NavigationRequestResult(WebView* webview) : webview_(webview) {}

  void SuccessInternal(const flutter::EncodableValue* should_load) override {
    if (std::holds_alternative<bool>(*should_load) &&
        std::get<bool>(*should_load)) {
      webview_->Resume();
      return;
    }
    if (std::holds_alternative<int32_t>(*should_load) &&
        std::get<int32_t>(*should_load) == 1) {
      webview_->Resume();
      return;
    }
    webview_->Stop();
  }

  void ErrorInternal(const std::string& error_code,
                     const std::string& error_message,
                     const flutter::EncodableValue* error_details) override {
    LOG_ERROR("The request unexpectedly completed with an error.");
    webview_->Stop();
  }

  void NotImplementedInternal() override {
    LOG_ERROR("The target method was unexpectedly unimplemented.");
    webview_->Stop();
  }

 private:
  WebView* webview_;
};

template <typename T>
bool GetValueFromEncodableMap(const flutter::EncodableValue* arguments,
                              std::string key, T* out) {
  if (auto* map = std::get_if<flutter::EncodableMap>(arguments)) {
    auto iter = map->find(flutter::EncodableValue(key));
    if (iter != map->end() && !iter->second.IsNull()) {
      if (auto* value = std::get_if<T>(&iter->second)) {
        *out = *value;
        return true;
      }
    }
  }
  return false;
}

template <typename T>
bool GetValueFromEncodableMap(const flutter::EncodableMap& arguments,
                              std::string key, T* out) {
  auto iter = arguments.find(flutter::EncodableValue(key));
  if (iter != arguments.end() && !iter->second.IsNull()) {
    if (auto* value = std::get_if<T>(&iter->second)) {
      *out = *value;
      return true;
    }
  }
  return false;
}

flutter::EncodableMap CreateRequestMap(const std::string& url,
                                       const std::string& method = "GET") {
  flutter::EncodableMap map;
  map[flutter::EncodableValue("url")] = flutter::EncodableValue(url);
  map[flutter::EncodableValue("headers")] =
      flutter::EncodableValue(flutter::EncodableMap());
  map[flutter::EncodableValue("method")] = flutter::EncodableValue(method);
  map[flutter::EncodableValue("hasGesture")] = flutter::EncodableValue(false);
  map[flutter::EncodableValue("isForMainFrame")] =
      flutter::EncodableValue(true);
  map[flutter::EncodableValue("isRedirect")] = flutter::EncodableValue(false);
  return map;
}

flutter::EncodableMap CreateNavigationActionMap(const std::string& url) {
  flutter::EncodableMap map;
  map[flutter::EncodableValue("hasGesture")] = flutter::EncodableValue(false);
  map[flutter::EncodableValue("isForMainFrame")] =
      flutter::EncodableValue(true);
  map[flutter::EncodableValue("isRedirect")] = flutter::EncodableValue(false);
  map[flutter::EncodableValue("navigationType")] = flutter::EncodableValue();
  map[flutter::EncodableValue("request")] =
      flutter::EncodableValue(CreateRequestMap(url));
  map[flutter::EncodableValue("shouldPerformDownload")] =
      flutter::EncodableValue(false);
  map[flutter::EncodableValue("sourceFrame")] = flutter::EncodableValue();
  map[flutter::EncodableValue("targetFrame")] = flutter::EncodableValue();
  return map;
}

flutter::EncodableMap CreateErrorMap(
    const std::string& description, int error_type = kWebResourceErrorUnknown) {
  flutter::EncodableMap map;
  map[flutter::EncodableValue("description")] =
      flutter::EncodableValue(description);
  map[flutter::EncodableValue("type")] = flutter::EncodableValue(error_type);
  return map;
}

}  // namespace

std::set<WebView*> WebView::instances_;
std::mutex WebView::instances_mutex_;
std::vector<WebView::RetiredNativeView> WebView::retired_native_views_;
std::mutex WebView::retired_native_views_mutex_;

WebView::WebView(flutter::PluginRegistrar* registrar, int view_id,
                 flutter::TextureRegistrar* texture_registrar, double width,
                 double height, const flutter::EncodableValue& params,
                 void* window)
    : PlatformView(registrar, view_id, nullptr),
      texture_registrar_(texture_registrar),
      width_(width),
      height_(height),
      window_(window) {
  if (!EwkInternalApiBinding::GetInstance().Initialize()) {
    LOG_ERROR("Failed to initialize EWK internal APIs.");
    return;
  }

  tbm_pool_ = std::make_unique<SingleBufferPool>(width, height);

  texture_variant_ =
      std::make_unique<flutter::TextureVariant>(flutter::GpuSurfaceTexture(
          kFlutterDesktopGpuSurfaceTypeNone,
          [this](size_t width,
                 size_t height) -> const FlutterDesktopGpuSurfaceDescriptor* {
            return ObtainGpuSurface(width, height);
          }));
  SetTextureId(texture_registrar_->RegisterTexture(texture_variant_.get()));

  const auto channel_name = GetWebViewChannelName();

  webview_channel_ = std::make_unique<FlMethodChannel>(
      GetPluginRegistrar()->messenger(), channel_name,
      &flutter::StandardMethodCodec::GetInstance());

  webview_channel_->SetMethodCallHandler(
      [webview = this](const auto& call, auto result) {
        webview->HandleWebViewMethodCall(call, std::move(result));
      });

  webview_controller_channel_ = std::make_unique<FlMethodChannel>(
      GetPluginRegistrar()->messenger(), channel_name,
      &flutter::StandardMethodCodec::GetInstance());

  navigation_delegate_channel_ = std::make_unique<FlMethodChannel>(
      GetPluginRegistrar()->messenger(), channel_name,
      &flutter::StandardMethodCodec::GetInstance());

  cookie_channel_ = std::make_unique<FlMethodChannel>(
      GetPluginRegistrar()->messenger(), kInAppWebViewCookieManagerChannelName,
      &flutter::StandardMethodCodec::GetInstance());
  cookie_channel_->SetMethodCallHandler(
      [webview = this](const auto& call, auto result) {
        webview->HandleCookieMethodCall(call, std::move(result));
      });

  {
    std::lock_guard<std::mutex> lock(instances_mutex_);
    for (auto* instance : instances_) {
      if (!instance || instance == this) {
        continue;
      }
      instance->SetFocus(false);
      instance->ClearFocus();
    }
  }

  if (!InitWebView()) {
    LOG_ERROR("Failed to initialize EWK webview instance.");
    return;
  }

  {
    std::lock_guard<std::mutex> lock(instances_mutex_);
    instances_.insert(this);
  }

  ApplyInitialParams(params);
}

/**
 * Added as a JavaScript interface to the WebView for any JavaScript channel
 * that the Dart code sets up.
 *
 * Exposes a single method named `postMessage` to JavaScript, which sends a
 * message over a method channel to the Dart code.
 */
void WebView::RegisterJavaScriptChannelName(const std::string& name) {
  ewk_view_javascript_message_handler_add(
      webview_instance_, &WebView::OnJavaScriptMessage, name.c_str());
}

WebView::~WebView() { Dispose(); }

std::string WebView::GetWebViewChannelName() {
  return std::string(kInAppWebViewChannelName) + std::to_string(GetViewId());
}

std::string WebView::GetWebViewControllerChannelName() {
  return GetWebViewChannelName();
}

std::string WebView::GetNavigationDelegateChannelName() {
  return GetWebViewChannelName();
}

void WebView::ClearAllCacheAll() {
  std::lock_guard<std::mutex> lock(instances_mutex_);
  for (auto* instance : instances_) {
    if (!instance || !instance->webview_instance_) {
      continue;
    }
    Ewk_Context* context = ewk_view_context_get(instance->webview_instance_);
    if (context) {
      ewk_context_resource_cache_clear(context);
    }
  }
}

std::string WebView::GetDefaultUserAgent() {
  std::lock_guard<std::mutex> lock(instances_mutex_);
  for (auto* instance : instances_) {
    if (!instance || !instance->webview_instance_) {
      continue;
    }
    const char* user_agent =
        ewk_view_user_agent_get(instance->webview_instance_);
    if (user_agent) {
      return std::string(user_agent);
    }
  }
  return std::string();
}

WebView::RetiredNativeView WebView::TakeRetiredNativeView() {
  std::lock_guard<std::mutex> lock(retired_native_views_mutex_);
  if (retired_native_views_.empty()) {
    return {};
  }

  auto native_view = retired_native_views_.back();
  retired_native_views_.pop_back();
  return native_view;
}

void WebView::StoreRetiredNativeView(Evas_Object* webview_instance,
                                     Ecore_Evas* ecore_evas) {
  if (!webview_instance || !ecore_evas) {
    return;
  }

  std::lock_guard<std::mutex> lock(retired_native_views_mutex_);
  retired_native_views_.push_back({webview_instance, ecore_evas});
}

void WebView::DestroyRetiredNativeViews() {
  std::vector<RetiredNativeView> retired_native_views;
  {
    std::lock_guard<std::mutex> lock(retired_native_views_mutex_);
    retired_native_views.swap(retired_native_views_);
  }

  for (auto& native_view : retired_native_views) {
    if (native_view.webview_instance) {
      evas_object_del(native_view.webview_instance);
      native_view.webview_instance = nullptr;
    }
    if (native_view.ecore_evas) {
      ecore_evas_free(native_view.ecore_evas);
      native_view.ecore_evas = nullptr;
    }
  }
}

void WebView::Dispose() {
  if (disposed_) {
    return;
  }
  disposed_ = true;

  {
    std::lock_guard<std::mutex> lock(instances_mutex_);
    instances_.erase(this);
  }

  texture_registrar_->UnregisterTexture(GetTextureId(), nullptr);

  if (webview_instance_) {
    DetachInputFocus();
    evas_object_data_del(webview_instance_, kEwkInstance);
    evas_object_smart_callback_del(webview_instance_,
                                   "offscreen,frame,rendered",
                                   &WebView::OnFrameRendered);
    evas_object_smart_callback_del(webview_instance_, "load,started",
                                   &WebView::OnLoadStarted);
    evas_object_smart_callback_del(webview_instance_, "load,finished",
                                   &WebView::OnLoadFinished);
    evas_object_smart_callback_del(webview_instance_, "load,progress",
                                   &WebView::OnProgress);
    evas_object_smart_callback_del(webview_instance_, "load,error",
                                   &WebView::OnLoadError);
    evas_object_smart_callback_del(webview_instance_, "console,message",
                                   &WebView::OnConsoleMessage);
    evas_object_smart_callback_del(webview_instance_,
                                   "policy,navigation,decide",
                                   &WebView::OnNavigationPolicy);
    evas_object_smart_callback_del(webview_instance_, "url,changed",
                                   &WebView::OnUrlChange);
    ewk_view_stop(webview_instance_);
    ewk_view_suspend(webview_instance_);
    evas_object_move(webview_instance_, -10000, -10000);
    evas_object_resize(webview_instance_, 1, 1);
    evas_object_hide(webview_instance_);
    StoreRetiredNativeView(webview_instance_, ecore_evas_);
    webview_instance_ = nullptr;
  }
  ecore_evas_ = nullptr;

  // ewk_shutdown();
}

void WebView::Offset(double left, double top) {
  if (disposed_ || !webview_instance_) {
    return;
  }
  left_ = left;
  top_ = top;

  evas_object_move(webview_instance_, static_cast<int>(left_),
                   static_cast<int>(top_));
}

void WebView::Resize(double width, double height) {
  if (disposed_ || !webview_instance_) {
    return;
  }
  width_ = width;
  height_ = height;

  if (candidate_surface_) {
    candidate_surface_ = nullptr;
  }

  tbm_pool_->Prepare(width_, height_);
  evas_object_resize(webview_instance_, width_, height_);
}

void WebView::Touch(int event_type, int button_type, double x, double y,
                    double dx, double dy) {
  if (disposed_ || !webview_instance_) {
    return;
  }

  if (event_type == 0) {
    EnsureInputFocus();
  }

#ifdef WEBVIEW_TIZEN_TOUCH_EVENTS_ENABLED
  SendTouchEvent(event_type, x, y);
#else
  SendMouseEvent(event_type, button_type, x, y, dx, dy);
#endif
}

void WebView::SendTouchEvent(int event_type, double x, double y) {
  Ewk_Touch_Event_Type mouse_event_type = EWK_TOUCH_START;
  Evas_Touch_Point_State state = EVAS_TOUCH_POINT_DOWN;
  if (event_type == 0) {  // down event
    mouse_event_type = EWK_TOUCH_START;
    state = EVAS_TOUCH_POINT_DOWN;
  } else if (event_type == 1) {  // move event
    mouse_event_type = EWK_TOUCH_MOVE;
    state = EVAS_TOUCH_POINT_MOVE;
  } else if (event_type == 2) {  // up event
    mouse_event_type = EWK_TOUCH_END;
    state = EVAS_TOUCH_POINT_UP;
  } else {
    LOG_WARN("Unknown touch event type: %d", event_type);
  }

  Eina_List* points = 0;
  Ewk_Touch_Point* point = new Ewk_Touch_Point;
  point->id = 0;
  point->x = x + left_;
  point->y = y + top_;
  point->state = state;
  points = eina_list_append(points, point);

  EwkInternalApiBinding::GetInstance().view.FeedTouchEvent(
      webview_instance_, mouse_event_type, points, 0);
  eina_list_free(points);
}

void WebView::SendMouseEvent(int event_type, int button_type, double x,
                             double y, double dx, double dy) {
  Ewk_Mouse_Button_Type mouse_button_type = (Ewk_Mouse_Button_Type)0;
  switch (button_type) {
    case 1:
      mouse_button_type = EWK_MOUSE_BUTTON_LEFT;
      break;
    case 2:
      mouse_button_type = EWK_MOUSE_BUTTON_RIGHT;
      break;
    case 4:
      mouse_button_type = EWK_MOUSE_BUTTON_MIDDLE;
      break;
  }

  int px = x + left_;
  int py = y + top_;

  if (event_type == 0) {  // down event
    mouse_button_type_ = mouse_button_type;
    EwkInternalApiBinding::GetInstance().view.FeedMouseDown(
        webview_instance_, mouse_button_type_, px, py);
  } else if (event_type == 1) {
    if (dy != 0) {
      EwkInternalApiBinding::GetInstance().view.FeedMouseWheel(
          webview_instance_, true, dy > 0 ? 1 : -1, px, py);
    }
  } else if (event_type == 2) {  // up event
    EwkInternalApiBinding::GetInstance().view.FeedMouseUp(
        webview_instance_, mouse_button_type_, px, py);
    mouse_button_type_ = mouse_button_type;
  } else {
    LOG_WARN("Unknown mouse event type: %d", event_type);
  }
}

bool WebView::SendKey(const char* key, const char* string, const char* compose,
                      uint32_t modifiers, uint32_t scan_code, bool is_down) {
  if (!IsFocused()) {
    return false;
  }

  if (strcmp(key, "XF86Exit") == 0 && !is_down) {
    return false;
  }

  if (strcmp(key, "XF86Back") == 0 && !is_down) {
    if (ewk_view_back_possible(webview_instance_)) {
      ewk_view_back(webview_instance_);
      return true;
    }
    return false;
  }

  if (is_down) {
    // TODO(swift-kim): Deal with other members of the structure.
    Evas_Event_Key_Down down_event = {};
    down_event.key = key;
    down_event.string = string;
    EwkInternalApiBinding::GetInstance().view.SendKeyEvent(
        webview_instance_, &down_event, is_down);
  } else {
    Evas_Event_Key_Up up_event = {};
    up_event.key = key;
    up_event.string = string;
    EwkInternalApiBinding::GetInstance().view.SendKeyEvent(webview_instance_,
                                                           &up_event, is_down);
  }
  return true;
}

void WebView::Resume() { ewk_view_resume(webview_instance_); }

void WebView::Stop() { ewk_view_stop(webview_instance_); }

void WebView::SetDirection(int direction) {
  // TODO: Implement if necessary.
}

void WebView::ClearFocus() { DetachInputFocus(); }

void WebView::EnsureInputFocus() {
  if (disposed_ || !webview_instance_) {
    return;
  }

  if (!ime_attached_) {
    EwkInternalApiBinding::GetInstance().settings.ImePanelEnabledSet(
        ewk_view_settings_get(webview_instance_), true);
    EwkInternalApiBinding::GetInstance().view.ImeWindowSet(webview_instance_,
                                                           window_);
    ime_attached_ = true;
  }

  if (ecore_evas_) {
    ecore_evas_focus_set(ecore_evas_, true);
  }
  ewk_view_focus_set(webview_instance_, true);
}

void WebView::DetachInputFocus() {
  if (!webview_instance_) {
    return;
  }

  ewk_view_focus_set(webview_instance_, false);
  if (ime_attached_) {
    EwkInternalApiBinding::GetInstance().view.ImeWindowSet(webview_instance_,
                                                           nullptr);
    EwkInternalApiBinding::GetInstance().settings.ImePanelEnabledSet(
        ewk_view_settings_get(webview_instance_), false);
    ime_attached_ = false;
  }
  if (ecore_evas_) {
    ecore_evas_focus_set(ecore_evas_, false);
  }
}

bool WebView::InitWebView() {
  if (engine_policy_) {
    LOG_INFO("Upgrade web engine used.");
    EwkInternalApiBinding::GetInstance().main.SetVersionPolicy(1);
  }

  char* chromium_argv[] = {
      const_cast<char*>("--disable-pinch"),
      const_cast<char*>("--js-flags=--expose-gc"),
      const_cast<char*>("--single-process"),
      const_cast<char*>("--no-zygote"),
  };
  int chromium_argc = sizeof(chromium_argv) / sizeof(chromium_argv[0]);
  EwkInternalApiBinding::GetInstance().main.SetArguments(chromium_argc,
                                                         chromium_argv);

  // TODO(jsuya): ewk_init() and ewk_shutdown() are designed to be called only
  // once in a process.(If ewk_init() is called after ewk_shutdown() is
  // called, SIGTRAP is called internally.) ewk_init() initializes the efl
  // modules and web engine's arguments data. The efl modules are initialized
  // by default in OS, and arguments data is also initialized through
  // SetArguments() API, so calling ewk_init() is not necessary. Therefore,
  // temporarily comment out ewk_init() and ewk_shutdown(). It can be reverted
  // depending on updates to chromium-efl.
  // ewk_init();
  auto retired_native_view = TakeRetiredNativeView();
  webview_instance_ = retired_native_view.webview_instance;
  ecore_evas_ = retired_native_view.ecore_evas;

  if (!webview_instance_ || !ecore_evas_) {
    ecore_evas_ = ecore_evas_new("wayland_egl", 0, 0, 1, 1, 0);
    if (!ecore_evas_) {
      return false;
    }

    webview_instance_ = ewk_view_add(ecore_evas_get(ecore_evas_));
    if (!webview_instance_) {
      ecore_evas_free(ecore_evas_);
      ecore_evas_ = nullptr;
      return false;
    }
  } else {
    ewk_view_resume(webview_instance_);
  }
  EwkInternalApiBinding::GetInstance().view.OffscreenRenderingEnabledSet(
      webview_instance_, true);

  Ewk_Context* context = ewk_view_context_get(webview_instance_);
  Ewk_Cookie_Manager* cookie_manager = ewk_context_cookie_manager_get(context);
  if (cookie_manager) {
    ewk_cookie_manager_accept_policy_set(
        cookie_manager, EWK_COOKIE_ACCEPT_POLICY_NO_THIRD_PARTY);
  }
  ewk_context_cache_model_set(context, EWK_CACHE_MODEL_PRIMARY_WEBBROWSER);

  EwkInternalApiBinding::GetInstance().settings.ForceZoomSet(
      ewk_view_settings_get(webview_instance_), true);
  EwkInternalApiBinding::GetInstance().view.KeyEventsEnabledSet(
      webview_instance_, true);
#ifdef WEBVIEW_TIZEN_TOUCH_EVENTS_ENABLED
  EwkInternalApiBinding::GetInstance().view.TouchEventsEnabledSet(
      webview_instance_, true);
  EwkInternalApiBinding::GetInstance().view.MouseEventsEnabledSet(
      webview_instance_, false);
#else
  EwkInternalApiBinding::GetInstance().view.TouchEventsEnabledSet(
      webview_instance_, false);
  EwkInternalApiBinding::GetInstance().view.MouseEventsEnabledSet(
      webview_instance_, true);
#endif

  EwkInternalApiBinding::GetInstance().view.OnJavaScriptAlert(
      webview_instance_, &WebView::OnJavaScriptAlertDialog, this);
  EwkInternalApiBinding::GetInstance().view.OnJavaScriptConfirm(
      webview_instance_, &WebView::OnJavaScriptConfirmDialog, this);
  EwkInternalApiBinding::GetInstance().view.OnJavaScriptPrompt(
      webview_instance_, &WebView::OnJavaScriptPromptDialog, this);

#ifdef TV_PROFILE
  EwkInternalApiBinding::GetInstance().view.SupportVideoHoleSet(
      webview_instance_, window_, true, false);
#endif

  evas_object_smart_callback_add(webview_instance_, "offscreen,frame,rendered",
                                 &WebView::OnFrameRendered, this);
  evas_object_smart_callback_add(webview_instance_, "load,started",
                                 &WebView::OnLoadStarted, this);
  evas_object_smart_callback_add(webview_instance_, "load,finished",
                                 &WebView::OnLoadFinished, this);
  evas_object_smart_callback_add(webview_instance_, "load,progress",
                                 &WebView::OnProgress, this);
  evas_object_smart_callback_add(webview_instance_, "load,error",
                                 &WebView::OnLoadError, this);
  evas_object_smart_callback_add(webview_instance_, "console,message",
                                 &WebView::OnConsoleMessage, this);
  evas_object_smart_callback_add(webview_instance_, "policy,navigation,decide",
                                 &WebView::OnNavigationPolicy, this);
  evas_object_smart_callback_add(webview_instance_, "url,changed",
                                 &WebView::OnUrlChange, this);

  Resize(width_, height_);
  evas_object_show(webview_instance_);

  evas_object_data_set(webview_instance_, kEwkInstance, this);

  return true;
}

template <typename T>
void WebView::SetBackgroundColor(const T& color) {
  EwkInternalApiBinding::GetInstance().view.SetBackgroundColor(
      webview_instance_, color >> 16 & 0xff, color >> 8 & 0xff, color & 0xff,
      color >> 24 & 0xff);
}

void WebView::ApplySettings(const flutter::EncodableMap& settings) {
  settings_ = settings;

  bool bool_value = false;
  if (GetValueFromEncodableMap(settings, "javaScriptEnabled", &bool_value)) {
    ewk_settings_javascript_enabled_set(
        ewk_view_settings_get(webview_instance_), bool_value);
  }

  if (GetValueFromEncodableMap(settings, "supportZoom", &bool_value)) {
    EwkInternalApiBinding::GetInstance().settings.ForceZoomSet(
        ewk_view_settings_get(webview_instance_), bool_value);
  }

  std::string user_agent;
  if (GetValueFromEncodableMap(settings, "userAgent", &user_agent) &&
      !user_agent.empty()) {
    ewk_view_user_agent_set(webview_instance_, user_agent.c_str());
  }

  if (GetValueFromEncodableMap(settings, "transparentBackground",
                               &bool_value) &&
      bool_value) {
    SetBackgroundColor(static_cast<int32_t>(0x00000000));
  }
}

void WebView::ApplyInitialParams(const flutter::EncodableValue& params) {
  const auto* creation_params = std::get_if<flutter::EncodableMap>(&params);
  if (!creation_params) {
    return;
  }

  flutter::EncodableMap initial_settings;
  if (GetValueFromEncodableMap(*creation_params, "initialSettings",
                               &initial_settings)) {
    ApplySettings(initial_settings);
  }

  std::string initial_file;
  if (GetValueFromEncodableMap(*creation_params, "initialFile",
                               &initial_file) &&
      !initial_file.empty()) {
    char* res_path = app_get_resource_path();
    if (res_path) {
      std::string url =
          std::string("file://") + res_path + "flutter_assets/" + initial_file;
      free(res_path);
      ewk_view_url_set(webview_instance_, url.c_str());
      return;
    }
  }

  flutter::EncodableMap initial_data;
  if (GetValueFromEncodableMap(*creation_params, "initialData",
                               &initial_data)) {
    std::string data;
    std::string base_url = "about:blank";
    if (GetValueFromEncodableMap(initial_data, "data", &data)) {
      GetValueFromEncodableMap(initial_data, "baseUrl", &base_url);
      ewk_view_html_string_load(webview_instance_, data.c_str(),
                                base_url.c_str(), nullptr);
      return;
    }
  }

  flutter::EncodableMap url_request;
  if (GetValueFromEncodableMap(*creation_params, "initialUrlRequest",
                               &url_request)) {
    std::string url;
    if (GetValueFromEncodableMap(url_request, "url", &url) && !url.empty()) {
      ewk_view_url_set(webview_instance_, url.c_str());
    }
  }
}

void WebView::EmitScrollChanged() {
  if (!webview_channel_) {
    return;
  }

  int32_t x = 0;
  int32_t y = 0;
  ewk_view_scroll_pos_get(webview_instance_, &x, &y);
  flutter::EncodableMap args = {
      {flutter::EncodableValue("x"), flutter::EncodableValue(x)},
      {flutter::EncodableValue("y"), flutter::EncodableValue(y)},
  };
  webview_channel_->InvokeMethod(
      "onScrollChanged", std::make_unique<flutter::EncodableValue>(args));
}

void WebView::HandleWebViewMethodCall(const FlMethodCall& method_call,
                                      std::unique_ptr<FlMethodResult> result) {
  const std::string& method_name = method_call.method_name();
  const flutter::EncodableValue* arguments = method_call.arguments();

  if (method_name == "setEnginePolicy") {
    const auto* engine_policy = std::get_if<bool>(arguments);
    if (engine_policy) {
      engine_policy_ = *engine_policy;
    }
    result->Success();
    return;
  }

  if (!webview_instance_) {
    if (!InitWebView()) {
      result->Error("Invalid operation",
                    "The webview instance initialize failed.");
      return;
    }
  }

  if (method_name == "javaScriptMode") {
    const auto* mode = std::get_if<int32_t>(arguments);
    if (mode) {
      bool enabled = (*mode == 1);
      ewk_settings_javascript_enabled_set(
          ewk_view_settings_get(webview_instance_), enabled);
      settings_[flutter::EncodableValue("javaScriptEnabled")] =
          flutter::EncodableValue(enabled);
    }
    result->Success();
  } else if (method_name == "hasNavigationDelegate") {
    const auto* has_navigation_delegate = std::get_if<bool>(arguments);
    if (has_navigation_delegate) {
      has_navigation_delegate_ = *has_navigation_delegate;
    }
    result->Success();
  } else if (method_name == "loadUrl" || method_name == "loadRequest" ||
             method_name == "loadRequestWithParams") {
    flutter::EncodableMap url_request;
    if (!GetValueFromEncodableMap(arguments, "urlRequest", &url_request)) {
      if (auto* map = std::get_if<flutter::EncodableMap>(arguments)) {
        url_request = *map;
      }
    }

    std::string url;
    if (!GetValueFromEncodableMap(url_request, "url", &url)) {
      result->Error("Invalid argument", "No url provided.");
      return;
    }

    std::string method = "GET";
    GetValueFromEncodableMap(url_request, "method", &method);
    flutter::EncodableMap headers;
    GetValueFromEncodableMap(url_request, "headers", &headers);
    std::vector<uint8_t> body;
    GetValueFromEncodableMap(url_request, "body", &body);

    if (method == "POST" || !headers.empty() || !body.empty()) {
      Eina_Hash* ewk_headers = eina_hash_new(
          [](const void* key) -> unsigned int {
            return key ? strlen(static_cast<const char*>(key)) + 1 : 0;
          },
          [](const void* key1, int key1_length, const void* key2,
             int key2_length) -> int {
            return strcmp(static_cast<const char*>(key1),
                          static_cast<const char*>(key2));
          },
          EINA_KEY_HASH(eina_hash_superfast), [](void* data) { free(data); },
          10);

      for (const auto& header : headers) {
        auto key = std::get_if<std::string>(&header.first);
        auto value = std::get_if<std::string>(&header.second);
        if (key && value) {
          eina_hash_add(ewk_headers, key->c_str(), strdup(value->c_str()));
        }
      }

      if (!body.empty()) {
        body.push_back('\0');
      }

      const auto ewk_method =
          method == "POST" ? EWK_HTTP_METHOD_POST : EWK_HTTP_METHOD_GET;
      bool ret = ewk_view_url_request_set(
          webview_instance_, url.c_str(), ewk_method, ewk_headers,
          body.empty() ? nullptr : reinterpret_cast<const char*>(body.data()));
      eina_hash_free(ewk_headers);
      if (!ret) {
        result->Error("Operation failed",
                      "Failed to load request with parameters.");
        return;
      }
    } else {
      ewk_view_url_set(webview_instance_, url.c_str());
    }
    result->Success();
  } else if (method_name == "postUrl") {
    std::string url;
    std::vector<uint8_t> body;
    if (!GetValueFromEncodableMap(arguments, "url", &url)) {
      result->Error("Invalid argument", "No url provided.");
      return;
    }
    GetValueFromEncodableMap(arguments, "postData", &body);
    if (!body.empty()) {
      body.push_back('\0');
    }
    const bool ret = ewk_view_url_request_set(
        webview_instance_, url.c_str(), EWK_HTTP_METHOD_POST, nullptr,
        body.empty() ? nullptr : reinterpret_cast<const char*>(body.data()));
    if (ret) {
      result->Success();
    } else {
      result->Error("Operation failed", "Failed to submit POST request.");
    }
  } else if (method_name == "canGoBack") {
    result->Success(flutter::EncodableValue(
        static_cast<bool>(ewk_view_back_possible(webview_instance_))));
  } else if (method_name == "canGoForward") {
    result->Success(flutter::EncodableValue(
        static_cast<bool>(ewk_view_forward_possible(webview_instance_))));
  } else if (method_name == "canGoBackOrForward") {
    int32_t steps = 0;
    GetValueFromEncodableMap(arguments, "steps", &steps);
    if (steps < -1 || steps > 1) {
      result->Error("Unsupported operation",
                    "goBackOrForward only supports single-step navigation.");
    } else if (steps < 0) {
      result->Success(flutter::EncodableValue(
          static_cast<bool>(ewk_view_back_possible(webview_instance_))));
    } else if (steps > 0) {
      result->Success(flutter::EncodableValue(
          static_cast<bool>(ewk_view_forward_possible(webview_instance_))));
    } else {
      result->Success(flutter::EncodableValue(true));
    }
  } else if (method_name == "goBack") {
    ewk_view_back(webview_instance_);
    result->Success();
  } else if (method_name == "goForward") {
    ewk_view_forward(webview_instance_);
    result->Success();
  } else if (method_name == "goBackOrForward") {
    int32_t steps = 0;
    GetValueFromEncodableMap(arguments, "steps", &steps);
    if (steps < -1 || steps > 1) {
      result->Error("Unsupported operation",
                    "goBackOrForward only supports single-step navigation.");
      return;
    } else if (steps < 0) {
      ewk_view_back(webview_instance_);
    } else if (steps > 0) {
      ewk_view_forward(webview_instance_);
    }
    result->Success();
  } else if (method_name == "reload" || method_name == "reloadFromOrigin") {
    ewk_view_reload(webview_instance_);
    result->Success();
  } else if (method_name == "getUrl" || method_name == "currentUrl") {
    result->Success(
        flutter::EncodableValue(ewk_view_url_get(webview_instance_)));
  } else if (method_name == "getOriginalUrl") {
    Ewk_Back_Forward_List* list =
        ewk_view_back_forward_list_get(webview_instance_);
    auto* item = list ? ewk_back_forward_list_current_item_get(list) : nullptr;
    const char* original_url =
        item ? ewk_back_forward_list_item_original_url_get(item) : nullptr;
    result->Success(original_url ? flutter::EncodableValue(original_url)
                                 : flutter::EncodableValue());
  } else if (method_name == "getProgress") {
    current_progress_ =
        static_cast<int>(ewk_view_load_progress_get(webview_instance_));
    result->Success(flutter::EncodableValue(current_progress_));
  } else if (method_name == "isLoading") {
    result->Success(flutter::EncodableValue(is_loading_));
  } else if (method_name == "stopLoading") {
    ewk_view_stop(webview_instance_);
    result->Success();
  } else if (method_name == "evaluateJavaScript" ||
             method_name == "evaluateJavascript" ||
             method_name == "runJavaScriptReturningResult" ||
             method_name == "runJavaScript") {
    std::string javascript;
    if (!GetValueFromEncodableMap(arguments, "source", &javascript)) {
      const auto* source = std::get_if<std::string>(arguments);
      if (source) {
        javascript = *source;
      }
    }

    if (!javascript.empty()) {
      ewk_view_script_execute(webview_instance_, javascript.c_str(),
                              &WebView::OnEvaluateJavaScript, result.release());
    } else {
      result->Error("Invalid argument", "The argument must be a string.");
    }
  } else if (method_name == "addJavaScriptChannel") {
    const auto* channel = std::get_if<std::string>(arguments);
    if (channel) {
      RegisterJavaScriptChannelName(*channel);
      result->Success();
    } else {
      result->Error("Invalid argument", "The argument must be a string.");
    }
  } else if (method_name == "clearCache" || method_name == "clearAllCache") {
    Ewk_Context* context = ewk_view_context_get(webview_instance_);
    ewk_context_resource_cache_clear(context);
    result->Success();
  } else if (method_name == "getSettings") {
    result->Success(flutter::EncodableValue(settings_));
  } else if (method_name == "setSettings") {
    flutter::EncodableMap settings;
    if (GetValueFromEncodableMap(arguments, "settings", &settings)) {
      ApplySettings(settings);
      result->Success();
    } else {
      result->Error("Invalid argument", "No settings provided.");
    }
  } else if (method_name == "getTitle") {
    const char* title = ewk_view_title_get(webview_instance_);
    result->Success(title ? flutter::EncodableValue(std::string(title))
                          : flutter::EncodableValue());
  } else if (method_name == "scrollTo") {
    int32_t x = 0, y = 0;
    if (GetValueFromEncodableMap(arguments, "x", &x) &&
        GetValueFromEncodableMap(arguments, "y", &y)) {
      ewk_view_scroll_set(webview_instance_, x, y);
      EmitScrollChanged();
      result->Success();
    } else {
      result->Error("Invalid argument", "No x or y provided.");
    }
  } else if (method_name == "scrollBy") {
    int32_t x = 0, y = 0;
    if (GetValueFromEncodableMap(arguments, "x", &x) &&
        GetValueFromEncodableMap(arguments, "y", &y)) {
      ewk_view_scroll_by(webview_instance_, x, y);
      EmitScrollChanged();
      result->Success();
    } else {
      result->Error("Invalid argument", "No x or y provided.");
    }
  } else if (method_name == "getScrollPosition" ||
             method_name == "getScrollX" || method_name == "getScrollY") {
    int32_t x = 0, y = 0;
    ewk_view_scroll_pos_get(webview_instance_, &x, &y);
    if (method_name == "getScrollX") {
      result->Success(flutter::EncodableValue(x));
    } else if (method_name == "getScrollY") {
      result->Success(flutter::EncodableValue(y));
    } else {
      flutter::EncodableMap args = {
          {flutter::EncodableValue("x"), flutter::EncodableValue(x)},
          {flutter::EncodableValue("y"), flutter::EncodableValue(y)}};
      result->Success(flutter::EncodableValue(args));
    }
  } else if (method_name == "loadFlutterAsset") {
    const auto* key = std::get_if<std::string>(arguments);
    if (key) {
      char* res_path = app_get_resource_path();
      if (res_path) {
        std::string url =
            std::string("file://") + res_path + "flutter_assets/" + *key;
        free(res_path);
        ewk_view_url_set(webview_instance_, url.c_str());
        result->Success();
      } else {
        result->Error("Operation failed",
                      "Could not get the flutter_assets path.");
      }
    } else {
      result->Error("Invalid argument", "The argument must be a string.");
    }
  } else if (method_name == "loadData" || method_name == "loadHtmlString") {
    std::string html, base_url;
    if (!GetValueFromEncodableMap(arguments, "data", &html) &&
        !GetValueFromEncodableMap(arguments, "html", &html)) {
      result->Error("Invalid argument", "No html provided.");
      return;
    }
    GetValueFromEncodableMap(arguments, "baseUrl", &base_url);
    ewk_view_html_string_load(webview_instance_, html.c_str(), base_url.c_str(),
                              nullptr);
    result->Success();
  } else if (method_name == "loadFile") {
    std::string file_path;
    if (GetValueFromEncodableMap(arguments, "assetFilePath", &file_path) ||
        (arguments != nullptr &&
         std::holds_alternative<std::string>(*arguments) &&
         (file_path = std::get<std::string>(*arguments), true))) {
      std::string url;
      if (!file_path.empty() && file_path[0] == '/') {
        url = std::string("file://") + file_path;
      } else {
        char* res_path = app_get_resource_path();
        if (!res_path) {
          result->Error("Operation failed", "Could not get app resource path.");
          return;
        }
        url = std::string("file://") + res_path + "flutter_assets/" + file_path;
        free(res_path);
      }
      ewk_view_url_set(webview_instance_, url.c_str());
      result->Success();
    } else {
      result->Error("Invalid argument", "The argument must be a string.");
    }
  } else if (method_name == "backgroundColor") {
    if (std::holds_alternative<int32_t>(*arguments)) {
      SetBackgroundColor(std::get<int32_t>(*arguments));
      result->Success();
    } else if (std::holds_alternative<int64_t>(*arguments)) {
      SetBackgroundColor(std::get<int64_t>(*arguments));
      result->Success();
    } else {
      result->Error("Invalid argument",
                    "The argument must be a int32_t or int64_t.");
    }
  } else if (method_name == "setUserAgent") {
    const auto* user_agent = std::get_if<std::string>(arguments);
    if (user_agent) {
      ewk_view_user_agent_set(webview_instance_, user_agent->c_str());
    }
    result->Success();
  } else if (method_name == "getUserAgent") {
    result->Success(flutter::EncodableValue(
        std::string(ewk_view_user_agent_get(webview_instance_))));
  } else if (method_name == "getContentHeight" ||
             method_name == "getContentWidth") {
    result->Success(flutter::EncodableValue(0));
  } else if (method_name == "zoomBy") {
    double zoom_factor = 1.0;
    if (!GetValueFromEncodableMap(arguments, "zoomFactor", &zoom_factor)) {
      result->Error("Invalid argument", "No zoomFactor provided.");
      return;
    }
    const double old_scale = ewk_view_scale_get(webview_instance_);
    const double new_scale = old_scale * zoom_factor;
    ewk_view_scale_set(webview_instance_, new_scale, 0, 0);
    flutter::EncodableMap args = {
        {flutter::EncodableValue("oldScale"),
         flutter::EncodableValue(old_scale)},
        {flutter::EncodableValue("newScale"),
         flutter::EncodableValue(new_scale)},
    };
    webview_channel_->InvokeMethod(
        "onZoomScaleChanged", std::make_unique<flutter::EncodableValue>(args));
    result->Success();
  } else if (method_name == "getZoomScale" || method_name == "getScale") {
    result->Success(
        flutter::EncodableValue(ewk_view_scale_get(webview_instance_)));
  } else if (method_name == "findAll") {
    result->Error("Unsupported operation",
                  "findAll is not implemented on flutter_inappwebview_tizen.");
  } else if (method_name == "findNext" || method_name == "clearMatches") {
    result->Error(
        "Unsupported operation",
        (method_name + " is not implemented on flutter_inappwebview_tizen.")
            .c_str());
  } else if (method_name == "injectJavascriptFileFromUrl" ||
             method_name == "injectCSSCode" ||
             method_name == "injectCSSFileFromUrl" ||
             method_name == "getSelectedText" ||
             method_name == "callAsyncJavaScript" ||
             method_name == "saveWebArchive" ||
             method_name == "isSecureContext" ||
             method_name == "createWebMessageChannel" ||
             method_name == "postWebMessage" ||
             method_name == "addWebMessageListener" ||
             method_name == "canScrollVertically" ||
             method_name == "canScrollHorizontally" ||
             method_name == "pauseTimers" || method_name == "resumeTimers" ||
             method_name == "takeScreenshot" || method_name == "createPdf" ||
             method_name == "createWebArchiveData" ||
             method_name == "hasOnlySecureContent" ||
             method_name == "pauseAllMediaPlayback" ||
             method_name == "setAllMediaPlaybackSuspended" ||
             method_name == "closeAllMediaPresentations" ||
             method_name == "requestMediaPlaybackState" ||
             method_name == "isInFullscreen" ||
             method_name == "getCameraCaptureState" ||
             method_name == "setCameraCaptureState" ||
             method_name == "getMicrophoneCaptureState" ||
             method_name == "setMicrophoneCaptureState") {
    result->Error(
        "Unsupported operation",
        (method_name + " is not implemented on flutter_inappwebview_tizen.")
            .c_str());
  } else if (method_name == "enableZoom") {
    const auto* support = std::get_if<bool>(arguments);
    if (!support) {
      result->Error("Invalid argument", "The argument must be a bool.");
      return;
    }

    EwkInternalApiBinding::GetInstance().settings.ForceZoomSet(
        ewk_view_settings_get(webview_instance_), *support);
    result->Success();
  } else if (method_name == "javaScriptAlertReply") {
    EwkInternalApiBinding::GetInstance().view.JavaScriptAlertReply(
        webview_instance_);
    result->Success();
  } else if (method_name == "javaScriptConfirmReply") {
    const auto* value = std::get_if<bool>(arguments);
    if (value) {
      EwkInternalApiBinding::GetInstance().view.JavaScriptConfirmReply(
          webview_instance_, *value);
      result->Success();
    } else {
      result->Error("Invalid argument", "The argument must be a bool.");
    }
  } else if (method_name == "javaScriptPromptReply") {
    const auto* value = std::get_if<std::string>(arguments);
    if (value) {
      EwkInternalApiBinding::GetInstance().view.JavaScriptPromptReply(
          webview_instance_, (*value).c_str());
      result->Success();
    } else {
      result->Error("Invalid argument", "The argument must be a string.");
    }
  } else if (method_name == "pause") {
    ewk_view_suspend(webview_instance_);
    result->Success();
  } else if (method_name == "resume") {
    ewk_view_resume(webview_instance_);
    result->Success();
  } else {
    result->NotImplemented();
  }
}

void WebView::HandleCookieMethodCall(const FlMethodCall& method_call,
                                     std::unique_ptr<FlMethodResult> result) {
  if (!webview_instance_) {
    result->Error("Invalid operation",
                  "The webview instance has not been initialized.");
    return;
  }

  const std::string& method_name = method_call.method_name();

  if (method_name == "clearCookies" || method_name == "deleteAllCookies") {
    Ewk_Context* context = ewk_view_context_get(webview_instance_);
    Ewk_Cookie_Manager* cookie_manager =
        ewk_context_cookie_manager_get(context);
    if (cookie_manager) {
      ewk_cookie_manager_cookies_clear(cookie_manager);
      result->Success(flutter::EncodableValue(true));
    } else {
      result->Error("Operation failed", "Failed to get cookie manager");
    }
  } else {
    result->NotImplemented();
  }
}

FlutterDesktopGpuSurfaceDescriptor* WebView::ObtainGpuSurface(size_t width,
                                                              size_t height) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!candidate_surface_) {
    if (rendered_surface_) {
      return rendered_surface_->GpuSurface();
    }
    return nullptr;
  }
  if (rendered_surface_ && rendered_surface_->IsUsed()) {
    tbm_pool_->Release(rendered_surface_);
  }
  rendered_surface_ = candidate_surface_;
  candidate_surface_ = nullptr;
  return rendered_surface_->GpuSurface();
}

void WebView::OnFrameRendered(void* data, Evas_Object* obj, void* event_info) {
  if (event_info) {
    WebView* webview = static_cast<WebView*>(data);
    if (!webview || webview->disposed_ || !webview->webview_instance_) {
      return;
    }

    std::lock_guard<std::mutex> lock(webview->mutex_);
    if (!webview->working_surface_) {
      webview->working_surface_ = webview->tbm_pool_->GetAvailableBuffer();
      webview->working_surface_->UseExternalBuffer();
    }
    webview->working_surface_->SetExternalBuffer(
        static_cast<tbm_surface_h>(event_info));

    if (webview->candidate_surface_) {
      webview->tbm_pool_->Release(webview->candidate_surface_);
      webview->candidate_surface_ = nullptr;
    }
    webview->candidate_surface_ = webview->working_surface_;
    webview->working_surface_ = nullptr;
    webview->texture_registrar_->MarkTextureFrameAvailable(
        webview->GetTextureId());
  }
}

void WebView::OnLoadStarted(void* data, Evas_Object* obj, void* event_info) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return;
  }
  webview->is_loading_ = true;
  std::string url = std::string(ewk_view_url_get(webview->webview_instance_));
  flutter::EncodableMap args = {
      {flutter::EncodableValue("url"), flutter::EncodableValue(url)}};
  webview->webview_channel_->InvokeMethod(
      "onLoadStart", std::make_unique<flutter::EncodableValue>(args));
}

void WebView::OnLoadFinished(void* data, Evas_Object* obj, void* event_info) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return;
  }
  webview->is_loading_ = false;
  webview->current_progress_ = 100;
  std::string url = std::string(ewk_view_url_get(webview->webview_instance_));
  flutter::EncodableMap args = {
      {flutter::EncodableValue("url"), flutter::EncodableValue(url)}};
  webview->webview_channel_->InvokeMethod(
      "onLoadStop", std::make_unique<flutter::EncodableValue>(args));

  const char* title = ewk_view_title_get(webview->webview_instance_);
  if (title) {
    flutter::EncodableMap title_args = {
        {flutter::EncodableValue("title"), flutter::EncodableValue(title)}};
    webview->webview_channel_->InvokeMethod(
        "onTitleChanged",
        std::make_unique<flutter::EncodableValue>(title_args));
  }
}

void WebView::OnProgress(void* data, Evas_Object* obj, void* event_info) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return;
  }
  int32_t progress = 0;
  if (event_info) {
    progress = static_cast<int32_t>((*static_cast<double*>(event_info)) * 100);
  }
  webview->current_progress_ = progress;
  flutter::EncodableMap args = {
      {flutter::EncodableValue("progress"), flutter::EncodableValue(progress)}};
  webview->webview_channel_->InvokeMethod(
      "onProgressChanged", std::make_unique<flutter::EncodableValue>(args));
}

void WebView::OnLoadError(void* data, Evas_Object* obj, void* event_info) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return;
  }
  webview->is_loading_ = false;
  Ewk_Error* error = static_cast<Ewk_Error*>(event_info);
  std::string url = ewk_error_url_get(error);

  flutter::EncodableMap args = {
      {flutter::EncodableValue("request"),
       flutter::EncodableValue(CreateRequestMap(url))},
      {flutter::EncodableValue("error"),
       flutter::EncodableValue(
           CreateErrorMap(ewk_error_description_get(error)))},
  };
  webview->webview_channel_->InvokeMethod(
      "onReceivedError", std::make_unique<flutter::EncodableValue>(args));
}

void WebView::OnConsoleMessage(void* data, Evas_Object* obj, void* event_info) {
  Ewk_Console_Message* message = static_cast<Ewk_Console_Message*>(event_info);
  Ewk_Console_Message_Level log_level =
      EwkInternalApiBinding::GetInstance().console_message.LevelGet(message);
  std::string text =
      EwkInternalApiBinding::GetInstance().console_message.TextGet(message);
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return;
  }
  if (webview->webview_controller_channel_) {
    flutter::EncodableMap args = {
        {flutter::EncodableValue("messageLevel"),
         flutter::EncodableValue(ConvertLogLevel(log_level))},
        {flutter::EncodableValue("message"), flutter::EncodableValue(text)},
    };
    webview->webview_channel_->InvokeMethod(
        "onConsoleMessage", std::make_unique<flutter::EncodableValue>(args));
  }
}

void WebView::OnNavigationPolicy(void* data, Evas_Object* obj,
                                 void* event_info) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return;
  }
  Ewk_Policy_Decision* policy_decision =
      static_cast<Ewk_Policy_Decision*>(event_info);
  ewk_policy_decision_use(policy_decision);
  if (!webview->has_navigation_delegate_) {
    return;
  }
  ewk_view_suspend(webview->webview_instance_);

  const char* url = ewk_policy_decision_url_get(policy_decision);
  flutter::EncodableMap args = CreateNavigationActionMap(url);
  auto result = std::make_unique<NavigationRequestResult>(webview);
  webview->webview_channel_->InvokeMethod(
      "shouldOverrideUrlLoading",
      std::make_unique<flutter::EncodableValue>(args), std::move(result));
}

void WebView::OnUrlChange(void* data, Evas_Object* obj, void* event_info) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return;
  }
  std::string url = std::string(ewk_view_url_get(webview->webview_instance_));
  flutter::EncodableMap args = {
      {flutter::EncodableValue("url"), flutter::EncodableValue(url)},
      {flutter::EncodableValue("isReload"), flutter::EncodableValue(false)}};
  webview->webview_channel_->InvokeMethod(
      "onUpdateVisitedHistory",
      std::make_unique<flutter::EncodableValue>(args));
}

void WebView::OnEvaluateJavaScript(Evas_Object* obj, const char* result_value,
                                   void* user_data) {
  FlMethodResult* result = static_cast<FlMethodResult*>(user_data);
  if (result_value) {
    result->Success(flutter::EncodableValue(result_value));
  } else {
    result->Success();
  }
  delete result;
}

void WebView::OnJavaScriptMessage(Evas_Object* obj,
                                  Ewk_Script_Message message) {
  if (obj) {
    WebView* webview =
        static_cast<WebView*>(evas_object_data_get(obj, kEwkInstance));
    if (!webview || webview->disposed_ || !webview->webview_instance_) {
      return;
    }
    if (webview->webview_channel_) {
      std::string channel_name(message.name);
      std::string message_body(static_cast<char*>(message.body));

      flutter::EncodableMap args = {
          {flutter::EncodableValue("channel"),
           flutter::EncodableValue(channel_name)},
          {flutter::EncodableValue("message"),
           flutter::EncodableValue(message_body)},
      };
      webview->webview_channel_->InvokeMethod(
          "javaScriptChannelMessage",
          std::make_unique<flutter::EncodableValue>(args));
    }
  }
}

Eina_Bool WebView::OnJavaScriptAlertDialog(Evas_Object* o, const char* message,
                                           void* data) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return true;
  }

  flutter::EncodableMap args = {
      {flutter::EncodableValue("message"), flutter::EncodableValue(message)},
      {flutter::EncodableValue("url"),
       flutter::EncodableValue(ewk_view_url_get(webview->webview_instance_))},
      {flutter::EncodableValue("isMainFrame"), flutter::EncodableValue(true)}};
  webview->webview_channel_->InvokeMethod(
      "onJsAlert", std::make_unique<flutter::EncodableValue>(args));
  EwkInternalApiBinding::GetInstance().view.JavaScriptAlertReply(
      webview->webview_instance_);

  return true;
}

Eina_Bool WebView::OnJavaScriptConfirmDialog(Evas_Object* o,
                                             const char* message, void* data) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return true;
  }
  flutter::EncodableMap args = {
      {flutter::EncodableValue("message"), flutter::EncodableValue(message)},
      {flutter::EncodableValue("url"),
       flutter::EncodableValue(ewk_view_url_get(webview->webview_instance_))},
      {flutter::EncodableValue("isMainFrame"), flutter::EncodableValue(true)}};
  webview->webview_channel_->InvokeMethod(
      "onJsConfirm", std::make_unique<flutter::EncodableValue>(args));
  EwkInternalApiBinding::GetInstance().view.JavaScriptConfirmReply(
      webview->webview_instance_, true);

  return true;
}

Eina_Bool WebView::OnJavaScriptPromptDialog(Evas_Object* o, const char* message,
                                            const char* default_text,
                                            void* data) {
  WebView* webview = static_cast<WebView*>(data);
  if (!webview || webview->disposed_ || !webview->webview_instance_) {
    return true;
  }
  flutter::EncodableMap args = {
      {flutter::EncodableValue("message"), flutter::EncodableValue(message)},
      {flutter::EncodableValue("url"),
       flutter::EncodableValue(ewk_view_url_get(webview->webview_instance_))},
      {flutter::EncodableValue("defaultValue"),
       flutter::EncodableValue(default_text)},
      {flutter::EncodableValue("isMainFrame"), flutter::EncodableValue(true)}};
  webview->webview_channel_->InvokeMethod(
      "onJsPrompt", std::make_unique<flutter::EncodableValue>(args));
  EwkInternalApiBinding::GetInstance().view.JavaScriptPromptReply(
      webview->webview_instance_, default_text);
  return true;
}
