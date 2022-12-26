// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "tizen_surface_view.h"

#include <Elementary.h>
#include <Evas.h>
#include <app_common.h>
#include <flutter/standard_method_codec.h>
#include <flutter_texture_registrar.h>
#include <tbm_surface.h>

#include "buffer_pool.h"
#include "log.h"
#include "tizen_surface_view_factory.h"

namespace {

typedef flutter::MethodCall<flutter::EncodableValue> FlMethodCall;
typedef flutter::MethodResult<flutter::EncodableValue> FlMethodResult;
typedef flutter::MethodChannel<flutter::EncodableValue> FlMethodChannel;

constexpr size_t kBufferPoolSize = 5;

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

}  // namespace

FlutterDesktopPixelBuffer* TizenSurfaceView::CopyPixelBuffer(size_t width,
                                                             size_t height) {
  mutex_.lock();
  // pixel_buffer_.reset(new FlutterDesktopPixelBuffer());
  if (!ee_) {
    mutex_.unlock();
    return nullptr;
  }
  pixels_ = (unsigned char*)ecore_evas_buffer_pixels_get(ee_);
  if (!pixels_) {
    mutex_.unlock();
    return nullptr;
  }

  // Color convert
  int t = 0;
  for (int i = 0; i < width * height * 4; i += 4) {
    t = pixels_[i];
    pixels_[i] = pixels_[i + 2];
    pixels_[i + 2] = t;
  }

  pixel_buffer_->width = width;
  pixel_buffer_->height = height;
  pixel_buffer_->buffer = pixels_;

  mutex_.unlock();
  return pixel_buffer_.get();
}

void TizenSurfaceView::_frame_update_cb(void* data, Evas* e, void* event_info) {
  LOG_ERROR("CJS UPdate");
}

TizenSurfaceView::TizenSurfaceView(flutter::PluginRegistrar* registrar,
                                   int view_id,
                                   flutter::TextureRegistrar* texture_registrar,
                                   double width, double height,
                                   const flutter::EncodableValue& params,
                                   void* win, void* surface)
    : PlatformView(registrar, view_id, nullptr),
      texture_registrar_(texture_registrar),
      width_(width),
      height_(height),
      win_(win),
      surface_(static_cast<Evas_Object*>(surface)) {
  LOG_ERROR("CJS win_ : %p  surface_ : %p view_id: %d width : %f height_ : %f",
            win_, surface_, view_id, width_, height_);
  w_ = width_;
  h_ = height_;

  pixel_buffer_.reset(new FlutterDesktopPixelBuffer());
  pixel_buffer_->width = 0;
  pixel_buffer_->height = 0;

  ee_ = ecore_evas_ecore_evas_get(evas_object_evas_get((Evas_Object*)surface_));

  if (!ee_) LOG_ERROR("CJS EE is null : %p ", surface_);
  int x, y, w, h;
  ecore_evas_geometry_get(ee_, &x, &y, &w, &h);

  pixels_ = (unsigned char*)ecore_evas_buffer_pixels_get(ee_);
  LOG_ERROR("CJS surface_src : %p pixels : %p (%d %d)", surface_, pixels_, w,
            h);

  texture_variant_ =
      std::make_unique<flutter::TextureVariant>(flutter::PixelBufferTexture(
          [this](size_t width,
                 size_t height) -> const FlutterDesktopPixelBuffer* {
            LOG_ERROR("CJS width : %d height_ : %d", width, height);
            return this->CopyPixelBuffer(width, height);
          }));
  LOG_ERROR("CJS CHECK");
  // texture_id_ = registrar_->RegisterTexture(texture_variant_.get());
  SetTextureId(texture_registrar_->RegisterTexture(texture_variant_.get()));
  LOG_ERROR("CJS CHECK");

  InitTizenSurfaceView();
  LOG_ERROR("CJS CHECK");
  channel_ = std::make_unique<FlMethodChannel>(
      GetPluginRegistrar()->messenger(), GetChannelName(),
      &flutter::StandardMethodCodec::GetInstance());
  channel_->SetMethodCallHandler(
      [tizen_surface_view = this](const auto& call, auto result) {
        tizen_surface_view->HandleMethodCall(call, std::move(result));
      });

  LOG_ERROR("CJS CHECK");
  TizenSurfaceView* tizen_surface_view = static_cast<TizenSurfaceView*>(this);
  LOG_ERROR("CJS width : %f height : %f", width, height);

  // evas_object_image_pixels_get_callback_set(
  // surface_, _frame_update_cb, nullptr);

  evas_event_callback_add(evas_object_evas_get(surface_),
                          EVAS_CALLBACK_RENDER_POST, _frame_update_cb, ee_);

  tizen_surface_view->texture_registrar_->MarkTextureFrameAvailable(
      tizen_surface_view->GetTextureId());
}

TizenSurfaceView::~TizenSurfaceView() { Dispose(); }

std::string TizenSurfaceView::GetChannelName() {
  return "plugins.flutter.io/tizen_surface_view";  //_" +
                                                   // std::to_string(GetViewId());
}

void TizenSurfaceView::Dispose() {
  texture_registrar_->UnregisterTexture(GetTextureId());
}

void TizenSurfaceView::Resize(double width, double height) {
  width_ = width;
  height_ = height;

  if (candidate_surface_) {
    candidate_surface_ = nullptr;
  }

  tbm_pool_->Prepare(width_, height_);
  evas_object_resize(tizen_surface_view_instance_, width_, height_);
}

void TizenSurfaceView::Touch(int type, int button, double x, double y,
                             double dx, double dy) {
  if (type == 0) {  // down event
    Evas_Event_Mouse_Down event;
    event.canvas.x = (int)x;
    event.canvas.y = (int)y;
    event.timestamp = time(nullptr);
    event.button = 1;
    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_MOUSE_DOWN);
    LOG_ERROR("CJS Touch %d %d (%f %f)", type, event.timestamp, x, y);
  } else if (type == 1) {  // move event
    // mouse_event_type = EWK_TOUCH_MOVE;

  } else if (type == 2) {  // up event
    Evas_Event_Mouse_Up event;
    event.canvas.x = (int)x;
    event.canvas.y = (int)y;
    event.timestamp = time(nullptr) + 1;
    event.button = 2;
    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_MOUSE_UP);
    // evas_event_feed_mouse_down();
    LOG_ERROR("CJS Touch %d %d (%f %f)", type, event.timestamp, x, y);
  } else {
    // TODO: Not implemented
  }

  // Eina_List* pointList = 0;
  // // Ewk_Touch_Point* point = new Ewk_Touch_Point;
  // point->id = 0;
  // point->x = x;
  // point->y = y;
  // point->state = state;
  // pointList = eina_list_append(pointList, point);

  // EwkInternalApiBinding::GetInstance().view.FeedTouchEvent(
  //     tizen_surface_view_instance_, mouse_event_type, pointList, 0);
  // eina_list_free(pointList);
  /*TizenSurfaceView* tizen_surface_view = static_cast<TizenSurfaceView*>(this);
  tizen_surface_view->texture_registrar_->MarkTextureFrameAvailable(
      tizen_surface_view->GetTextureId());*/
}

bool TizenSurfaceView::SendKey(const char* key, const char* string,
                               const char* compose, uint32_t modifiers,
                               uint32_t scan_code, bool is_down) {
  if (!IsFocused()) {
    return false;
  }
  // TizenSurfaceView* tizen_surface_view =
  // static_cast<TizenSurfaceView*>(this);
  // tizen_surface_view->texture_registrar_->MarkTextureFrameAvailable(
  // tizen_surface_view->GetTextureId());
  if (is_down) {
    LOG_ERROR("CJS KEY Down");
    /*Evas_Event_Key_Down downEvent;
    memset(&downEvent, 0, sizeof(Evas_Event_Key_Down));
    downEvent.key = key;
    downEvent.string = string;
    void* evasKeyEvent = static_cast<void*>(&downEvent);*/
    // EwkInternalApiBinding::GetInstance().view.SendKeyEvent(
    //     tizen_surface_view_instance_, evasKeyEvent, is_down);
    Evas_Event_Key_Down event;
    event.key = key;
    event.string = string;
    event.compose = compose;
    // event.modifiers = modifiers;
    event.keycode = scan_code;
    event.timestamp = time(nullptr);
    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_KEY_DOWN);
    return true;
  } else {
    LOG_ERROR("CJS KEY Up");
    Evas_Event_Key_Up event;
    event.key = key;
    event.string = string;
    event.compose = compose;
    // event.modifiers = modifiers;
    event.keycode = scan_code;
    event.timestamp = time(nullptr);
    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_KEY_UP);
    /*Evas_Event_Key_Up upEvent;
    memset(&upEvent, 0, sizeof(Evas_Event_Key_Up));
    upEvent.key = key;
    upEvent.string = string;
    void* evasKeyEvent = static_cast<void*>(&upEvent);*/
    // EwkInternalApiBinding::GetInstance().view.SendKeyEvent(
    //     tizen_surface_view_instance_, evasKeyEvent, is_down);
    return true;
  }

  return false;
}

void TizenSurfaceView::Resume() {
  // ewk_view_resume(tizen_surface_view_instance_);
}

void TizenSurfaceView::Stop() {
  // ewk_view_stop(tizen_surface_view_instance_);
}

void TizenSurfaceView::SetDirection(int direction) {
  // TODO: Implement if necessary.
}

void TizenSurfaceView::InitTizenSurfaceView() {
  // char* chromium_argv[] = {
  //     const_cast<char*>("--disable-pinch"),
  //     const_cast<char*>("--js-flags=--expose-gc"),
  //     const_cast<char*>("--single-process"),
  //     const_cast<char*>("--no-zygote"),
  // };
  // int chromium_argc = sizeof(chromium_argv) / sizeof(chromium_argv[0]);
  // EwkInternalApiBinding::GetInstance().main.SetArguments(chromium_argc,
  //                                                        chromium_argv);

  // ewk_init();
  // Ecore_Evas* evas = ecore_evas_new("wayland_egl", 0, 0, 1, 1, 0);

  // tizen_surface_view_instance_ = ewk_view_add(ecore_evas_get(evas));
  // ecore_evas_focus_set(evas, true);
  // ewk_view_focus_set(tizen_surface_view_instance_, true);
  // EwkInternalApiBinding::GetInstance().view.OffscreenRenderingEnabledSet(
  //     tizen_surface_view_instance_, true);

  // Ewk_Settings* settings =
  // ewk_view_settings_get(tizen_surface_view_instance_);

  // Ewk_Context* context = ewk_view_context_get(tizen_surface_view_instance_);
  // Ewk_Cookie_Manager* manager = ewk_context_cookie_manager_get(context);
  // if (manager) {
  //   ewk_cookie_manager_accept_policy_set(
  //       manager, EWK_COOKIE_ACCEPT_POLICY_NO_THIRD_PARTY);
  // }
  // ewk_settings_viewport_meta_tag_set(settings, false);
  // EwkInternalApiBinding::GetInstance().settings.ImePanelEnabledSet(settings,
  //                                                                  true);
  // ewk_settings_javascript_enabled_set(settings, true);

  // EwkInternalApiBinding::GetInstance().view.ImeWindowSet(
  //     tizen_surface_view_instance_, win_);
  // EwkInternalApiBinding::GetInstance().view.KeyEventsEnabledSet(
  //     tizen_surface_view_instance_, true);
  // ewk_context_cache_model_set(context, EWK_CACHE_MODEL_PRIMARY_WEBBROWSER);

  // evas_object_smart_callback_add(tizen_surface_view_instance_,
  //                                "offscreen,frame,rendered",
  //                                &TizenSurfaceView::OnFrameRendered, this);
  // evas_object_smart_callback_add(tizen_surface_view_instance_,
  // "load,started",
  //                                &TizenSurfaceView::OnLoadStarted, this);
  // evas_object_smart_callback_add(tizen_surface_view_instance_,
  // "load,finished",
  //                                &TizenSurfaceView::OnLoadFinished, this);
  // evas_object_smart_callback_add(tizen_surface_view_instance_, "load,error",
  //                                &TizenSurfaceView::OnLoadError, this);
  // evas_object_smart_callback_add(tizen_surface_view_instance_,
  //                                "console,message",
  //                                &TizenSurfaceView::OnConsoleMessage, this);
  // evas_object_smart_callback_add(tizen_surface_view_instance_,
  //                                "policy,navigation,decide",
  //                                &TizenSurfaceView::OnNavigationPolicy,
  //                                this);
  // Resize(width_, height_);
  // evas_object_show(tizen_surface_view_instance_);

  // evas_object_data_set(tizen_surface_view_instance_, kEwkInstance, this);
}

void TizenSurfaceView::HandleMethodCall(
    const FlMethodCall& method_call, std::unique_ptr<FlMethodResult> result) {
  if (!tizen_surface_view_instance_) {
    result->Error("Invalid operation",
                  "The tizen_surface_view instance has not been initialized.");
    return;
  }

  const std::string& method_name = method_call.method_name();
  const flutter::EncodableValue* arguments = method_call.arguments();

  result->NotImplemented();
}

FlutterDesktopGpuSurfaceDescriptor* TizenSurfaceView::ObtainGpuSurface(
    size_t width, size_t height) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!candidate_surface_) {
    if (rendered_surface_) {
      LOG_ERROR("CJS CHECK %d %d", width, height);
      return rendered_surface_->GpuSurface();
    }
    LOG_ERROR("CJS CHECK");
    return nullptr;
  }
  LOG_ERROR("CJS CHECK");
  if (rendered_surface_ && rendered_surface_->IsUsed()) {
    LOG_ERROR("CJS CHECK");
    tbm_pool_->Release(rendered_surface_);
  }
  rendered_surface_ = candidate_surface_;
  candidate_surface_ = nullptr;
  LOG_ERROR("CJS CHECK");
  return rendered_surface_->GpuSurface();
}

void TizenSurfaceView::OnFrameRendered(void* data, Evas_Object* obj,
                                       void* event_info) {
  if (event_info) {
    TizenSurfaceView* tizen_surface_view = static_cast<TizenSurfaceView*>(data);

    std::lock_guard<std::mutex> lock(tizen_surface_view->mutex_);
    if (!tizen_surface_view->working_surface_) {
      tizen_surface_view->working_surface_ =
          tizen_surface_view->tbm_pool_->GetAvailableBuffer();
      tizen_surface_view->working_surface_->UseExternalBuffer();
    }
    tizen_surface_view->working_surface_->SetExternalBuffer(
        static_cast<tbm_surface_h>(event_info));

    if (tizen_surface_view->candidate_surface_) {
      tizen_surface_view->tbm_pool_->Release(
          tizen_surface_view->candidate_surface_);
      tizen_surface_view->candidate_surface_ = nullptr;
    }
    tizen_surface_view->candidate_surface_ =
        tizen_surface_view->working_surface_;
    tizen_surface_view->working_surface_ = nullptr;
    tizen_surface_view->texture_registrar_->MarkTextureFrameAvailable(
        tizen_surface_view->GetTextureId());
  }
}
