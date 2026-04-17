// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webview.h"

#include <app_common.h>
#include <flutter/standard_method_codec.h>
#include <flutter_texture_registrar.h>
#include <system_info.h>
#include <tbm_surface.h>

#include <stdexcept>
#include <variant>

#include "buffer_pool.h"
#include "log.h"
#include "lwe/LWEWebView.h"
#include "lwe/PlatformIntegrationData.h"
#include "webview_factory.h"

namespace {

constexpr size_t kBufferPoolSize = 5;
constexpr char kInAppWebViewChannelName[] =
    "com.pichillilorenzo/flutter_inappwebview_";
constexpr char kInAppWebViewCookieManagerChannelName[] =
    "com.pichillilorenzo/flutter_inappwebview_cookiemanager";
constexpr int kWebResourceErrorUnknown = -1;

extern "C" size_t LWE_EXPORT createWebViewInstance(
    unsigned x, unsigned y, unsigned width, unsigned height,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID,
    const std::function<::LWE::WebContainer::ExternalImageInfo(void)>&
        prepareImageCb,
    const std::function<void(::LWE::WebContainer*, bool needsFlush)>& flushCb,
    bool useSWBackend);

class NavigationRequestResult : public FlMethodResult {
 public:
  NavigationRequestResult(std::string url,
                          std::weak_ptr<WebViewCallbackContext> context)
      : url_(url), context_(std::move(context)) {}

  void SuccessInternal(const flutter::EncodableValue* should_load) override {
    if (std::holds_alternative<bool>(*should_load) &&
        std::get<bool>(*should_load)) {
      LoadUrl();
      return;
    }
    if (std::holds_alternative<int32_t>(*should_load) &&
        std::get<int32_t>(*should_load) == 1) {
      LoadUrl();
    }
  }

  void ErrorInternal(const std::string& error_code,
                     const std::string& error_message,
                     const flutter::EncodableValue* error_details) override {
    LOG_ERROR("The request unexpectedly completed with an error.");
  }

  void NotImplementedInternal() override {
    LOG_ERROR("The target method was unexpectedly unimplemented.");
  }

 private:
  void LoadUrl() {
    auto context = context_.lock();
    if (context && context->owner && context->owner->GetWebViewInstance()) {
      context->owner->GetWebViewInstance()->LoadURL(url_);
    }
  }

  std::string url_;
  std::weak_ptr<WebViewCallbackContext> context_;
};

template <typename T>
static bool GetValueFromEncodableMap(const flutter::EncodableValue* arguments,
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
static bool GetValueFromEncodableMap(const flutter::EncodableMap& arguments,
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

static flutter::EncodableMap CreateRequestMap(
    const std::string& url, const std::string& method = "GET") {
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

static flutter::EncodableMap CreateNavigationActionMap(const std::string& url) {
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

static flutter::EncodableMap CreateErrorMap(
    const std::string& description, int error_type = kWebResourceErrorUnknown) {
  flutter::EncodableMap map;
  map[flutter::EncodableValue("description")] =
      flutter::EncodableValue(description);
  map[flutter::EncodableValue("type")] = flutter::EncodableValue(error_type);
  return map;
}

static bool IsRunningOnEmulator() {
  bool result = false;
  char* value = nullptr;
  int ret = system_info_get_platform_string(
      "http://tizen.org/system/model_name", &value);
  if (ret == SYSTEM_INFO_ERROR_NONE && strcmp(value, "Emulator") == 0) {
    result = true;
  }
  if (value) {
    free(value);
  }
  return result;
}

}  // namespace

std::set<WebView*> WebView::instances_;
std::mutex WebView::instances_mutex_;

WebView::WebView(flutter::PluginRegistrar* registrar, int view_id,
                 flutter::TextureRegistrar* texture_registrar, double width,
                 double height, const flutter::EncodableValue& params)
    : PlatformView(registrar, view_id, nullptr),
      texture_registrar_(texture_registrar),
      width_(width),
      height_(height) {
  callback_context_ = std::make_shared<WebViewCallbackContext>();
  callback_context_->owner = this;

  use_sw_backend_ = IsRunningOnEmulator();
  if (use_sw_backend_) {
    tbm_pool_ = std::make_unique<SingleBufferPool>(width, height);
  } else {
    tbm_pool_ = std::make_unique<BufferPool>(width, height, kBufferPoolSize);
  }

  texture_variant_ =
      std::make_unique<flutter::TextureVariant>(flutter::GpuSurfaceTexture(
          kFlutterDesktopGpuSurfaceTypeNone,
          [this](size_t width,
                 size_t height) -> const FlutterDesktopGpuSurfaceDescriptor* {
            return ObtainGpuSurface(width, height);
          }));
  SetTextureId(texture_registrar_->RegisterTexture(texture_variant_.get()));

  InitWebView();

  dispatcher_ = std::make_unique<MessageDispatcher>();

  const auto channel_name = GetWebViewChannelName();

  webview_channel_ = std::make_unique<FlMethodChannel>(
      GetPluginRegistrar()->messenger(), channel_name,
      &flutter::StandardMethodCodec::GetInstance());
  webview_channel_->SetMethodCallHandler(
      [webview = this](const auto& call, auto result) {
        webview->HandleWebViewMethodCall(call, std::move(result));
      });

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
    instances_.insert(this);
  }

  ApplyInitialParams(params);

  auto weak_context = std::weak_ptr<WebViewCallbackContext>(callback_context_);

  webview_instance_->RegisterOnPageStartedHandler(
      [weak_context](LWE::WebContainer* container, const std::string& url) {
        auto context = weak_context.lock();
        auto* webview = context ? context->owner : nullptr;
        if (!webview || webview->disposed_ || !webview->webview_instance_) {
          return;
        }
        webview->is_loading_ = true;
        flutter::EncodableMap args = {
            {flutter::EncodableValue("url"), flutter::EncodableValue(url)}};

        webview->dispatcher_->dispatchTaskOnMainThread([weak_context, args]() {
          auto context = weak_context.lock();
          auto* webview = context ? context->owner : nullptr;
          if (!webview || webview->disposed_ || !webview->webview_channel_) {
            return;
          }
          webview->webview_channel_->InvokeMethod(
              "onLoadStart", std::make_unique<flutter::EncodableValue>(args));
        });
      });
  webview_instance_->RegisterOnPageLoadedHandler(
      [weak_context](LWE::WebContainer* container, const std::string& url) {
        auto context = weak_context.lock();
        auto* webview = context ? context->owner : nullptr;
        if (!webview || webview->disposed_ || !webview->webview_instance_) {
          return;
        }
        webview->is_loading_ = false;
        webview->current_progress_ = 100;
        flutter::EncodableMap args = {
            {flutter::EncodableValue("url"), flutter::EncodableValue(url)}};

        webview->dispatcher_->dispatchTaskOnMainThread([weak_context, args]() {
          auto context = weak_context.lock();
          auto* webview = context ? context->owner : nullptr;
          if (!webview || webview->disposed_ || !webview->webview_channel_ ||
              !webview->webview_instance_) {
            return;
          }
          webview->webview_channel_->InvokeMethod(
              "onLoadStop", std::make_unique<flutter::EncodableValue>(args));
          flutter::EncodableMap title_args = {
              {flutter::EncodableValue("title"),
               flutter::EncodableValue(webview->webview_instance_->GetTitle())},
          };
          webview->webview_channel_->InvokeMethod(
              "onTitleChanged",
              std::make_unique<flutter::EncodableValue>(title_args));
        });
      });
  webview_instance_->RegisterOnProgressChangedHandler(
      [weak_context](LWE::WebContainer* container, int progress) {
        auto context = weak_context.lock();
        auto* webview = context ? context->owner : nullptr;
        if (!webview || webview->disposed_ || !webview->webview_instance_) {
          return;
        }
        webview->current_progress_ = progress;
        flutter::EncodableMap args = {{flutter::EncodableValue("progress"),
                                       flutter::EncodableValue(progress)}};

        webview->dispatcher_->dispatchTaskOnMainThread([weak_context, args]() {
          auto context = weak_context.lock();
          auto* webview = context ? context->owner : nullptr;
          if (!webview || webview->disposed_ || !webview->webview_channel_) {
            return;
          }
          webview->webview_channel_->InvokeMethod(
              "onProgressChanged",
              std::make_unique<flutter::EncodableValue>(args));
        });
      });
  webview_instance_->RegisterOnReceivedErrorHandler(
      [weak_context](LWE::WebContainer* container, LWE::ResourceError error) {
        auto context = weak_context.lock();
        auto* webview = context ? context->owner : nullptr;
        if (!webview || webview->disposed_ || !webview->webview_instance_) {
          return;
        }
        webview->is_loading_ = false;
        flutter::EncodableMap args = {
            {flutter::EncodableValue("request"),
             flutter::EncodableValue(CreateRequestMap(error.GetUrl()))},
            {flutter::EncodableValue("error"),
             flutter::EncodableValue(CreateErrorMap(error.GetDescription()))},
        };
        webview->dispatcher_->dispatchTaskOnMainThread([weak_context, args]() {
          auto context = weak_context.lock();
          auto* webview = context ? context->owner : nullptr;
          if (!webview || webview->disposed_ || !webview->webview_channel_) {
            return;
          }
          webview->webview_channel_->InvokeMethod(
              "onReceivedError",
              std::make_unique<flutter::EncodableValue>(args));
        });
      });
  webview_instance_->RegisterShouldOverrideUrlLoadingHandler(
      [weak_context](LWE::WebContainer* view, const std::string& url) -> bool {
        auto context = weak_context.lock();
        auto* webview = context ? context->owner : nullptr;
        if (!webview || webview->disposed_ || !webview->webview_instance_ ||
            !webview->has_navigation_delegate_) {
          return false;
        }
        flutter::EncodableMap args = CreateNavigationActionMap(url);

        webview->dispatcher_->dispatchTaskOnMainThread(
            [weak_context, args, url]() {
              auto context = weak_context.lock();
              auto* webview = context ? context->owner : nullptr;
              if (!webview || webview->disposed_ || !webview->webview_channel_) {
                return;
              }
              auto result =
                  std::make_unique<NavigationRequestResult>(url, weak_context);
              webview->webview_channel_->InvokeMethod(
              "shouldOverrideUrlLoading",
              std::make_unique<flutter::EncodableValue>(args),
              std::move(result));
            });
        return true;
      });
}

/**
 * Added as a JavaScript interface to the WebView for any JavaScript channel
 * that the Dart code sets up.
 *
 * Exposes a single method named `postMessage` to JavaScript, which sends a
 * message over a method channel to the Dart code.
 */
void WebView::RegisterJavaScriptChannelName(const std::string& name) {
  if (disposed_ || !webview_instance_) {
    return;
  }
  auto weak_context = std::weak_ptr<WebViewCallbackContext>(callback_context_);
  auto on_message = [weak_context, name](const std::string& message)
                        -> std::string {
    auto context = weak_context.lock();
    auto* webview = context ? context->owner : nullptr;
    if (!webview || webview->disposed_ || !webview->webview_channel_ ||
        !webview->dispatcher_) {
      return "disposed";
    }
    flutter::EncodableMap args = {
        {flutter::EncodableValue("channel"), flutter::EncodableValue(name)},
        {flutter::EncodableValue("message"), flutter::EncodableValue(message)},
    };

    webview->dispatcher_->dispatchTaskOnMainThread([weak_context, args]() {
      auto context = weak_context.lock();
      auto* webview = context ? context->owner : nullptr;
      if (!webview || webview->disposed_ || !webview->webview_channel_) {
        return;
      }
      webview->webview_channel_->InvokeMethod(
          "javaScriptChannelMessage",
          std::make_unique<flutter::EncodableValue>(args));
    });
    return "success";
  };
  webview_instance_->AddJavaScriptInterface(name, "postMessage", on_message);
}

void WebView::UnregisterJavaScriptChannelName(const std::string& name) {
  if (disposed_ || !webview_instance_) {
    return;
  }
  webview_instance_->RemoveJavascriptInterface(name, "postMessage");
}

WebView::~WebView() { Dispose(); }

std::string WebView::GetWebViewChannelName() {
  return std::string(kInAppWebViewChannelName) + std::to_string(GetViewId());
}

std::string WebView::GetNavigationDelegateChannelName() {
  return GetWebViewChannelName();
}

void WebView::ClearAllCacheAll() {
  std::lock_guard<std::mutex> lock(instances_mutex_);
  for (auto* instance : instances_) {
    if (instance && instance->webview_instance_) {
      instance->webview_instance_->ClearCache();
    }
  }
}

std::string WebView::GetDefaultUserAgent() {
  std::lock_guard<std::mutex> lock(instances_mutex_);
  for (auto* instance : instances_) {
    if (instance && instance->webview_instance_) {
      return instance->webview_instance_->GetSettings().GetDefaultUserAgent();
    }
  }
  return std::string();
}

void WebView::Dispose() {
  if (disposed_) {
    return;
  }
  disposed_ = true;

  if (callback_context_) {
    callback_context_->owner = nullptr;
    callback_context_.reset();
  }

  {
    std::lock_guard<std::mutex> lock(instances_mutex_);
    instances_.erase(this);
  }

  texture_registrar_->UnregisterTexture(GetTextureId(), nullptr);

  if (webview_instance_) {
    ClearFocus();
    webview_instance_->Pause();
    webview_instance_->Destroy();
    webview_instance_ = nullptr;
  }

  working_surface_ = nullptr;
  candidate_surface_ = nullptr;
  rendered_surface_ = nullptr;
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
  webview_instance_->ResizeTo(width_, height_);
}

void WebView::Touch(int type, int button, double x, double y, double dx,
                    double dy) {
  if (disposed_ || !webview_instance_) {
    return;
  }
  if (type == 0) {  // down event
    EnsureInputFocus();
    webview_instance_->DispatchMouseDownEvent(
        LWE::MouseButtonValue::LeftButton,
        LWE::MouseButtonsValue::LeftButtonDown, x, y);
    is_mouse_lbutton_down_ = true;
  } else if (type == 1) {  // move event
    webview_instance_->DispatchMouseMoveEvent(
        is_mouse_lbutton_down_ ? LWE::MouseButtonValue::LeftButton
                               : LWE::MouseButtonValue::NoButton,
        is_mouse_lbutton_down_ ? LWE::MouseButtonsValue::LeftButtonDown
                               : LWE::MouseButtonsValue::NoButtonDown,
        x, y);
  } else if (type == 2) {  // up event
    webview_instance_->DispatchMouseUpEvent(
        LWE::MouseButtonValue::NoButton, LWE::MouseButtonsValue::NoButtonDown,
        x, y);
    is_mouse_lbutton_down_ = false;
  } else {
    LOG_WARN("Unknown touch event type: %d", type);
  }
}

void WebView::ClearFocus() {
  if (!webview_instance_) {
    return;
  }
  SetFocus(false);
  webview_instance_->Blur();
}

void WebView::EnsureInputFocus() {
  if (disposed_ || !webview_instance_) {
    return;
  }
  SetFocus(true);
  webview_instance_->Focus();
}

static LWE::KeyValue KeyToKeyValue(const std::string& key,
                                   bool is_shift_pressed) {
  if (key == "Left") {
    return LWE::KeyValue::ArrowLeftKey;
  } else if (key == "Right") {
    return LWE::KeyValue::ArrowRightKey;
  } else if (key == "Up") {
    return LWE::KeyValue::ArrowUpKey;
  } else if (key == "Down") {
    return LWE::KeyValue::ArrowDownKey;
  } else if (key == "space") {
    return LWE::KeyValue::SpaceKey;
  } else if (key == "Select") {
    return LWE::KeyValue::EnterKey;
  } else if (key == "Return") {
    return LWE::KeyValue::EnterKey;
  } else if (key == "Tab") {
    return LWE::KeyValue::TabKey;
  } else if (key == "BackSpace") {
    return LWE::KeyValue::BackspaceKey;
  } else if (key == "Escape") {
    return LWE::KeyValue::EscapeKey;
  } else if (key == "Delete") {
    return LWE::KeyValue::DeleteKey;
  } else if (key == "at") {
    return LWE::KeyValue::AtMarkKey;
  } else if (key == "minus") {
    if (is_shift_pressed) {
      return LWE::KeyValue::UnderScoreMarkKey;
    } else {
      return LWE::KeyValue::MinusMarkKey;
    }
  } else if (key == "equal") {
    if (is_shift_pressed) {
      return LWE::KeyValue::PlusMarkKey;
    } else {
      return LWE::KeyValue::EqualitySignKey;
    }
  } else if (key == "bracketleft") {
    if (is_shift_pressed) {
      return LWE::KeyValue::LeftCurlyBracketMarkKey;
    } else {
      return LWE::KeyValue::LeftSquareBracketKey;
    }
  } else if (key == "bracketright") {
    if (is_shift_pressed) {
      return LWE::KeyValue::RightCurlyBracketMarkKey;
    } else {
      return LWE::KeyValue::RightSquareBracketKey;
    }
  } else if (key == "semicolon") {
    if (is_shift_pressed) {
      return LWE::KeyValue::ColonMarkKey;
    } else {
      return LWE::KeyValue::SemiColonMarkKey;
    }
  } else if (key == "apostrophe") {
    if (is_shift_pressed) {
      return LWE::KeyValue::DoubleQuoteMarkKey;
    } else {
      return LWE::KeyValue::SingleQuoteMarkKey;
    }
  } else if (key == "comma") {
    if (is_shift_pressed) {
      return LWE::KeyValue::LessThanMarkKey;
    } else {
      return LWE::KeyValue::CommaMarkKey;
    }
  } else if (key == "period") {
    if (is_shift_pressed) {
      return LWE::KeyValue::GreaterThanSignKey;
    } else {
      return LWE::KeyValue::PeriodKey;
    }
  } else if (key == "slash") {
    if (is_shift_pressed) {
      return LWE::KeyValue::QuestionMarkKey;
    } else {
      return LWE::KeyValue::SlashKey;
    }
  } else if (key.length() == 1) {
    const char ch = key.at(0);
    if (ch >= '0' && ch <= '9') {
      if (is_shift_pressed) {
        switch (ch) {
          case '1':
            return LWE::KeyValue::ExclamationMarkKey;
          case '2':
            return LWE::KeyValue::AtMarkKey;
          case '3':
            return LWE::KeyValue::SharpMarkKey;
          case '4':
            return LWE::KeyValue::DollarMarkKey;
          case '5':
            return LWE::KeyValue::PercentMarkKey;
          case '6':
            return LWE::KeyValue::CaretMarkKey;
          case '7':
            return LWE::KeyValue::AmpersandMarkKey;
          case '8':
            return LWE::KeyValue::AsteriskMarkKey;
          case '9':
            return LWE::KeyValue::LeftParenthesisMarkKey;
          case '0':
            return LWE::KeyValue::RightParenthesisMarkKey;
        }
      }
      return LWE::KeyValue(LWE::KeyValue::Digit0Key + ch - '0');
    } else if (ch >= 'a' && ch <= 'z') {
      if (is_shift_pressed) {
        return LWE::KeyValue(LWE::KeyValue::LowerAKey + ch - 'a' - 32);
      } else {
        return LWE::KeyValue(LWE::KeyValue::LowerAKey + ch - 'a');
      }
    } else if (ch >= 'A' && ch <= 'Z') {
      if (is_shift_pressed) {
        return LWE::KeyValue(LWE::KeyValue::AKey + ch - 'A' + 32);
      } else {
        return LWE::KeyValue(LWE::KeyValue::AKey + ch - 'A');
      }
    }
  } else if (key == "XF86AudioRaiseVolume") {
    return LWE::KeyValue::TVVolumeUpKey;
  } else if (key == "XF86AudioLowerVolume") {
    return LWE::KeyValue::TVVolumeDownKey;
  } else if (key == "XF86AudioMute") {
    return LWE::KeyValue::TVMuteKey;
  } else if (key == "XF86RaiseChannel") {
    return LWE::KeyValue::TVChannelUpKey;
  } else if (key == "XF86LowerChannel") {
    return LWE::KeyValue::TVChannelDownKey;
  } else if (key == "XF86AudioRewind") {
    return LWE::KeyValue::MediaTrackPreviousKey;
  } else if (key == "XF86AudioNext") {
    return LWE::KeyValue::MediaTrackNextKey;
  } else if (key == "XF86AudioPause") {
    return LWE::KeyValue::MediaPauseKey;
  } else if (key == "XF86AudioRecord") {
    return LWE::KeyValue::MediaRecordKey;
  } else if (key == "XF86AudioPlay") {
    return LWE::KeyValue::MediaPlayKey;
  } else if (key == "XF86AudioStop") {
    return LWE::KeyValue::MediaStopKey;
  } else if (key == "XF86Info") {
    return LWE::KeyValue::TVInfoKey;
  } else if (key == "XF86Back") {
    return LWE::KeyValue::TVReturnKey;
  } else if (key == "XF86Red") {
    return LWE::KeyValue::TVRedKey;
  } else if (key == "XF86Green") {
    return LWE::KeyValue::TVGreenKey;
  } else if (key == "XF86Yellow") {
    return LWE::KeyValue::TVYellowKey;
  } else if (key == "XF86Blue") {
    return LWE::KeyValue::TVBlueKey;
  } else if (key == "XF86SysMenu") {
    return LWE::KeyValue::TVMenuKey;
  } else if (key == "XF86Home") {
    return LWE::KeyValue::TVHomeKey;
  } else if (key == "XF86Exit") {
    return LWE::KeyValue::TVExitKey;
  } else if (key == "XF86PreviousChannel") {
    return LWE::KeyValue::TVPreviousChannel;
  } else if (key == "XF86ChannelList") {
    return LWE::KeyValue::TVChannelList;
  } else if (key == "XF86ChannelGuide") {
    return LWE::KeyValue::TVChannelGuide;
  } else if (key == "XF86SimpleMenu") {
    return LWE::KeyValue::TVSimpleMenu;
  } else if (key == "XF86EManual") {
    return LWE::KeyValue::TVEManual;
  } else if (key == "XF86ExtraApp") {
    return LWE::KeyValue::TVExtraApp;
  } else if (key == "XF86Search") {
    return LWE::KeyValue::TVSearch;
  } else if (key == "XF86PictureSize") {
    return LWE::KeyValue::TVPictureSize;
  } else if (key == "XF86Sleep") {
    return LWE::KeyValue::TVSleep;
  } else if (key == "XF86Caption") {
    return LWE::KeyValue::TVCaption;
  } else if (key == "XF86More") {
    return LWE::KeyValue::TVMore;
  } else if (key == "XF86BTVoice") {
    return LWE::KeyValue::TVBTVoice;
  } else if (key == "XF86Color") {
    return LWE::KeyValue::TVColor;
  } else if (key == "XF86PlayBack") {
    return LWE::KeyValue::TVPlayBack;
  }
  LOG_WARN("Unknown key name: %s", key.c_str());
  return LWE::KeyValue::UnidentifiedKey;
}

bool WebView::SendKey(const char* key, const char* string, const char* compose,
                      uint32_t modifiers, uint32_t scan_code, bool is_down) {
  if (disposed_ || !webview_instance_ || !IsFocused()) {
    return false;
  }

  bool is_shift_pressed = modifiers & 1;

  struct Param {
    LWE::WebContainer* webview_instance;
    LWE::KeyValue key_value;
    bool is_down;
  };

  Param* param = new Param();
  param->webview_instance = webview_instance_;
  param->key_value = KeyToKeyValue(key, is_shift_pressed);
  param->is_down = is_down;

  if (param->key_value == LWE::KeyValue::TVReturnKey &&
      webview_instance_->CanGoBack()) {
    webview_instance_->GoBack();
    return true;
  }

  webview_instance_->AddIdleCallback(
      [](void* data) {
        Param* param = reinterpret_cast<Param*>(data);
        if (param->is_down) {
          param->webview_instance->DispatchKeyDownEvent(param->key_value);
          param->webview_instance->DispatchKeyPressEvent(param->key_value);
        } else {
          param->webview_instance->DispatchKeyUpEvent(param->key_value);
        }
        delete param;
      },
      param);

  return false;
}

void WebView::SetDirection(int direction) {
  // TODO: Implement if necessary.
}

void WebView::InitWebView() {
  if (webview_instance_) {
    ClearFocus();
    webview_instance_->Destroy();
    webview_instance_ = nullptr;
  }

  float pixel_ratio = 1.0;

  auto on_prepare_image = [this]() -> LWE::WebContainer::ExternalImageInfo {
    std::lock_guard<std::mutex> lock(mutex_);
    LWE::WebContainer::ExternalImageInfo result;
    if (!working_surface_) {
      working_surface_ = tbm_pool_->GetAvailableBuffer();
    }
    if (working_surface_) {
      result.imageAddress = working_surface_->Surface();
    } else {
      result.imageAddress = nullptr;
    }
    return result;
  };
  auto on_flush = [this](LWE::WebContainer* container, bool is_rendered) {
    if (is_rendered) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (candidate_surface_) {
        tbm_pool_->Release(candidate_surface_);
        candidate_surface_ = nullptr;
      }
      candidate_surface_ = working_surface_;
      working_surface_ = nullptr;
      texture_registrar_->MarkTextureFrameAvailable(GetTextureId());
    }
  };

  webview_instance_ =
      reinterpret_cast<LWE::WebContainer*>(createWebViewInstance(
          0, 0, width_, height_, pixel_ratio, "SamsungOneUI", "ko-KR",
          "Asia/Seoul", on_prepare_image, on_flush, use_sw_backend_));

#ifndef TV_PROFILE
  LWE::Settings settings = webview_instance_->GetSettings();
  settings.SetUserAgentString(
      "Mozilla/5.0 (like Gecko/54.0 Firefox/54.0) Mobile");
  webview_instance_->SetSettings(settings);
#endif
}

template <typename T>
void WebView::SetBackgroundColor(const T& color) {
  LWE::Settings settings = webview_instance_->GetSettings();
  settings.SetBaseBackgroundColor(color >> 16 & 0xff, color >> 8 & 0xff,
                                  color & 0xff, color >> 24 & 0xff);
  webview_instance_->SetSettings(settings);
}

void WebView::ApplySettings(const flutter::EncodableMap& settings) {
  settings_ = settings;

  std::string user_agent;
  if (GetValueFromEncodableMap(settings, "userAgent", &user_agent) &&
      !user_agent.empty()) {
    LWE::Settings lwe_settings = webview_instance_->GetSettings();
    lwe_settings.SetUserAgentString(user_agent);
    webview_instance_->SetSettings(lwe_settings);
  }

  bool transparent = false;
  if (GetValueFromEncodableMap(settings, "transparentBackground",
                               &transparent) &&
      transparent) {
    SetBackgroundColor(static_cast<int32_t>(0x00000000));
  }

  bool hide_scrollbar = false;
  if (GetValueFromEncodableMap(settings, "disableVerticalScroll",
                               &hide_scrollbar) &&
      hide_scrollbar) {
    LWE::Settings lwe_settings = webview_instance_->GetSettings();
    lwe_settings.SetScrollbarVisible(false);
    webview_instance_->SetSettings(lwe_settings);
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
      webview_instance_->LoadURL(url);
      return;
    }
  }

  flutter::EncodableMap initial_data;
  if (GetValueFromEncodableMap(*creation_params, "initialData",
                               &initial_data)) {
    std::string data;
    if (GetValueFromEncodableMap(initial_data, "data", &data)) {
      webview_instance_->LoadData(data);
      return;
    }
  }

  flutter::EncodableMap url_request;
  if (GetValueFromEncodableMap(*creation_params, "initialUrlRequest",
                               &url_request)) {
    std::string url;
    if (GetValueFromEncodableMap(url_request, "url", &url) && !url.empty()) {
      webview_instance_->LoadURL(url);
    }
  }
}

void WebView::EmitScrollChanged() {
  if (disposed_ || !webview_instance_ || !webview_channel_) {
    return;
  }

  flutter::EncodableMap args = {
      {flutter::EncodableValue("x"),
       flutter::EncodableValue(webview_instance_->GetScrollX())},
      {flutter::EncodableValue("y"),
       flutter::EncodableValue(webview_instance_->GetScrollY())},
  };
  webview_channel_->InvokeMethod(
      "onScrollChanged", std::make_unique<flutter::EncodableValue>(args));
}

void WebView::HandleWebViewMethodCall(const FlMethodCall& method_call,
                                      std::unique_ptr<FlMethodResult> result) {
  if (!webview_instance_) {
    result->Error("Invalid operation",
                  "The webview instance has not been initialized.");
    return;
  }

  const std::string& method_name = method_call.method_name();
  const flutter::EncodableValue* arguments = method_call.arguments();

  if (method_name == "javaScriptMode") {
    const auto* mode = std::get_if<int32_t>(arguments);
    if (mode && *mode == 0) {
      result->Error("Unsupported operation",
                    "Disabling JavaScript is not implemented on "
                    "flutter_inappwebview_lwe.");
    } else {
      result->Success();
    }
  } else if (method_name == "hasNavigationDelegate") {
    const auto* has_navigation_delegate = std::get_if<bool>(arguments);
    if (has_navigation_delegate) {
      has_navigation_delegate_ = *has_navigation_delegate;
    }
    result->Success();
  } else if (method_name == "loadUrl" || method_name == "loadRequest") {
    flutter::EncodableMap url_request;
    if (!GetValueFromEncodableMap(arguments, "urlRequest", &url_request)) {
      if (auto* map = std::get_if<flutter::EncodableMap>(arguments)) {
        url_request = *map;
      }
    }

    std::string url;
    std::string method = "GET";
    flutter::EncodableMap headers;
    std::vector<uint8_t> body;
    GetValueFromEncodableMap(url_request, "method", &method);
    GetValueFromEncodableMap(url_request, "headers", &headers);
    GetValueFromEncodableMap(url_request, "body", &body);

    if (method != "GET" || !headers.empty() || !body.empty()) {
      result->Error("Unsupported operation",
                    "loadRequest only supports plain GET requests on "
                    "flutter_inappwebview_lwe.");
      return;
    }

    if (GetValueFromEncodableMap(url_request, "url", &url) && !url.empty()) {
      webview_instance_->LoadURL(url);
      result->Success();
    } else {
      result->Error("Invalid argument", "No url provided.");
    }
  } else if (method_name == "postUrl") {
    result->Error("Unsupported operation",
                  "postUrl is not implemented on flutter_inappwebview_lwe.");
  } else if (method_name == "canGoBack") {
    result->Success(flutter::EncodableValue(webview_instance_->CanGoBack()));
  } else if (method_name == "canGoForward") {
    result->Success(flutter::EncodableValue(webview_instance_->CanGoForward()));
  } else if (method_name == "canGoBackOrForward") {
    int32_t steps = 0;
    GetValueFromEncodableMap(arguments, "steps", &steps);
    if (steps < -1 || steps > 1) {
      result->Error("Unsupported operation",
                    "goBackOrForward only supports single-step navigation.");
    } else if (steps < 0) {
      result->Success(flutter::EncodableValue(webview_instance_->CanGoBack()));
    } else if (steps > 0) {
      result->Success(
          flutter::EncodableValue(webview_instance_->CanGoForward()));
    } else {
      result->Success(flutter::EncodableValue(true));
    }
  } else if (method_name == "goBack") {
    webview_instance_->GoBack();
    result->Success();
  } else if (method_name == "goForward") {
    webview_instance_->GoForward();
    result->Success();
  } else if (method_name == "goBackOrForward") {
    int32_t steps = 0;
    GetValueFromEncodableMap(arguments, "steps", &steps);
    if (steps < -1 || steps > 1) {
      result->Error("Unsupported operation",
                    "goBackOrForward only supports single-step navigation.");
      return;
    } else if (steps < 0) {
      webview_instance_->GoBack();
    } else if (steps > 0) {
      webview_instance_->GoForward();
    }
    result->Success();
  } else if (method_name == "reload" || method_name == "reloadFromOrigin") {
    webview_instance_->Reload();
    result->Success();
  } else if (method_name == "getUrl" || method_name == "currentUrl") {
    result->Success(flutter::EncodableValue(webview_instance_->GetURL()));
  } else if (method_name == "getOriginalUrl") {
    result->Error(
        "Unsupported operation",
        "getOriginalUrl is not implemented on flutter_inappwebview_lwe.");
  } else if (method_name == "getProgress") {
    result->Success(flutter::EncodableValue(current_progress_));
  } else if (method_name == "isLoading") {
    result->Success(flutter::EncodableValue(is_loading_));
  } else if (method_name == "stopLoading") {
    webview_instance_->StopLoading();
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
      bool should_return = method_name != "runJavaScript";
      auto on_result = [result = result.release(),
                        should_return](std::string value) {
        if (result) {
          if (should_return) {
            result->Success(flutter::EncodableValue(value));
          } else {
            result->Success();
          }
          delete result;
        }
      };

      webview_instance_->EvaluateJavaScript(javascript, on_result);

    } else {
      result->Error("Invalid argument", "The argument must be a string.");
    }
  } else if (method_name == "addJavaScriptChannel") {
    const auto* channel = std::get_if<std::string>(arguments);
    if (channel) {
      RegisterJavaScriptChannelName(*channel);
    }
    result->Success();
  } else if (method_name == "removeJavaScriptChannel") {
    const auto* channel = std::get_if<std::string>(arguments);
    if (channel) {
      UnregisterJavaScriptChannelName(*channel);
    }
    result->Success();
  } else if (method_name == "clearCache" || method_name == "clearAllCache") {
    webview_instance_->ClearCache();
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
    result->Success(flutter::EncodableValue(webview_instance_->GetTitle()));
  } else if (method_name == "scrollTo") {
    int x = 0, y = 0;
    if (GetValueFromEncodableMap(arguments, "x", &x) &&
        GetValueFromEncodableMap(arguments, "y", &y)) {
      webview_instance_->ScrollTo(x, y);
      EmitScrollChanged();
      result->Success();
    } else {
      result->Error("Invalid argument", "No x or y provided.");
    }
  } else if (method_name == "scrollBy") {
    int x = 0, y = 0;
    if (GetValueFromEncodableMap(arguments, "x", &x) &&
        GetValueFromEncodableMap(arguments, "y", &y)) {
      webview_instance_->ScrollBy(x, y);
      EmitScrollChanged();
      result->Success();
    } else {
      result->Error("Invalid argument", "No x or y provided.");
    }
  } else if (method_name == "getScrollPosition" ||
             method_name == "getScrollX" || method_name == "getScrollY") {
    int x = webview_instance_->GetScrollX();
    int y = webview_instance_->GetScrollY();
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
        webview_instance_->LoadURL(url);
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
    if (GetValueFromEncodableMap(arguments, "baseUrl", &base_url) &&
        !base_url.empty()) {
      result->Error("Unsupported operation",
                    "loadData with baseUrl is not implemented on "
                    "flutter_inappwebview_lwe.");
      return;
    }
    webview_instance_->LoadData(html);
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
      webview_instance_->LoadURL(url);
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
      LWE::Settings settings = webview_instance_->GetSettings();
      settings.SetUserAgentString(*user_agent);
      webview_instance_->SetSettings(settings);
    }
    result->Success();
  } else if (method_name == "getUserAgent") {
    LWE::Settings settings = webview_instance_->GetSettings();
    result->Success(flutter::EncodableValue(settings.GetUserAgentString()));
  } else if (method_name == "getContentHeight" ||
             method_name == "getContentWidth") {
    result->Success(flutter::EncodableValue(0));
  } else if (method_name == "zoomBy" || method_name == "getZoomScale" ||
             method_name == "getScale" || method_name == "findAll" ||
             method_name == "findNext" || method_name == "clearMatches") {
    result->Error(
        "Unsupported operation",
        method_name + " is not implemented on flutter_inappwebview_lwe.");
  } else if (method_name == "pause") {
    webview_instance_->Pause();
    result->Success();
  } else if (method_name == "resume") {
    webview_instance_->Resume();
    result->Success();
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
        method_name + " is not implemented on flutter_inappwebview_lwe.");
  } else if (method_name == "setCookie") {
    result->NotImplemented();
  } else if (method_name == "setVerticalScrollBarEnabled" ||
             method_name == "setHorizontalScrollBarEnabled") {
    const auto* value = std::get_if<bool>(arguments);
    if (value) {
      LWE::Settings settings = webview_instance_->GetSettings();
      settings.SetScrollbarVisible(*value);
      webview_instance_->SetSettings(settings);
    }
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
    LWE::CookieManager* cookie = LWE::CookieManager::GetInstance();
    cookie->ClearCookies();
    result->Success(flutter::EncodableValue(true));
  } else {
    result->NotImplemented();
  }
}

FlutterDesktopGpuSurfaceDescriptor* WebView::ObtainGpuSurface(size_t width,
                                                              size_t height) {
  if (disposed_) {
    return nullptr;
  }
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
