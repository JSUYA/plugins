// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "tizen_surface_view.h"

#include <Elementary.h>
#include <Evas.h>
#include <app_common.h>
#include <efl_extension.h>
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

  if (!pixels_ || width == 0 || height == 0) {
    LOG_ERROR("CJS CHECK ???????????");
    mutex_.unlock();
    return nullptr;
  }
  LOG_ERROR("CJS CHECK %");
  if (!pixel_converted) {
    LOG_ERROR("CJS CHECK %");
    unsigned char t = 0;
    // LOG_ERROR("CJS FIRST PIXEL %d %d %d %d", pixels_[0], pixels_[1],
    // pixels_[2],
    //           pixels_[3]);
    // Color convert
    /*for (int i = 0; i < width * height * 4; i += 4) {
      t = pixels_[i];
      pixels_[i] = pixels_[i + 2];
      pixels_[i + 2] = t;
    }*/
    for (int i = 0; i < width * height * 4; i += 4) {
      pixels2_[i] = pixels_[i + 2];
      pixels2_[i + 1] = pixels_[i + 1];
      pixels2_[i + 2] = pixels_[i];
      pixels2_[i + 3] = pixels_[i + 3];
    }

    pixel_converted = true;
  }
  LOG_ERROR("CJS CHECK %");
  pixel_buffer_->width = width;
  pixel_buffer_->height = height;
  pixel_buffer_->buffer = pixels2_;
  mutex_.unlock();
  return pixel_buffer_.get();
}

void TizenSurfaceView::frame_update_cb(void* data, Evas* e, void* event_info) {
  TizenSurfaceView* tizen_surface_view = static_cast<TizenSurfaceView*>(data);
  LOG_ERROR("CJS CHECK %");
  tizen_surface_view->mutex_.lock();
  if (tizen_surface_view && tizen_surface_view->pixel_buffer_ &&
      tizen_surface_view->texture_registrar_) {
    //  if (tizen_surface_view->pixel_converted) {
    for (int i = 0; i < tizen_surface_view->w_ * tizen_surface_view->h_ * 4;
         i++) {
      tizen_surface_view->pixels2_[i] = 0;
    }

    LOG_ERROR("CJS CHECK %");
    tizen_surface_view->pixel_converted = false;

    tizen_surface_view->pixel_buffer_.reset(new FlutterDesktopPixelBuffer());

    tizen_surface_view->texture_registrar_->MarkTextureFrameAvailable(
        tizen_surface_view->GetTextureId());
  }
  tizen_surface_view->mutex_.unlock();
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
  if (!win) LOG_ERROR("CJS win_ is null");
  if (!surface_) LOG_ERROR("CJS surface_ is null");
  LOG_ERROR(
      "CJS win_ : %p  surface_ : %p view_id: %d width : %f height_ : "
      "%f",
      win_, surface_, view_id, width_, height_);

  w_ = width_;
  h_ = height_;

  if (view_id != 0) {
    // pixel_converted = true;
  }
  texture_variant_ =
      std::make_unique<flutter::TextureVariant>(flutter::PixelBufferTexture(
          [this](size_t width,
                 size_t height) -> const FlutterDesktopPixelBuffer* {
            // LOG_ERROR("CJS width : %d height_ : %d", width, height);
            return this->CopyPixelBuffer(width, height);
          }));
  SetTextureId(texture_registrar_->RegisterTexture(texture_variant_.get()));

  InitTizenSurfaceView();

  channel_ = std::make_unique<FlMethodChannel>(
      GetPluginRegistrar()->messenger(), GetChannelName(),
      &flutter::StandardMethodCodec::GetInstance());
  channel_->SetMethodCallHandler(
      [tizen_surface_view = this](const auto& call, auto result) {
        tizen_surface_view->HandleMethodCall(call, std::move(result));
      });
}

TizenSurfaceView::~TizenSurfaceView() { Dispose(); }

std::string TizenSurfaceView::GetChannelName() {
  return "plugins.flutter.io/tizen_surface_view";  //_" +
                                                   // std::to_string(GetViewId());
}

void TizenSurfaceView::Dispose() {
  LOG_ERROR("CJS Dispose");
  evas_event_callback_del(evas_object_evas_get(surface_),
                          EVAS_CALLBACK_RENDER_POST, frame_update_cb);
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
  if (!surface_) return;
  if (type == 0) {  // down event

    // FIXME
    Evas_Event_Mouse_Move event;
    event.cur.output.x = static_cast<int>(x);
    event.cur.output.y = static_cast<int>(y);
    event.cur.canvas.x = static_cast<int>(x);
    event.cur.canvas.y = static_cast<int>(y);
    event.prev.canvas.x = static_cast<int>(dx);
    event.prev.canvas.y = static_cast<int>(dy);
    event.prev.output.x = static_cast<int>(dx);
    event.prev.output.y = static_cast<int>(dy);
    event.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event.buttons = 1;

    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_MOUSE_MOVE);
    //

    Evas_Event_Mouse_Down event_down;
    event_down.output.x = static_cast<int>(x);
    event_down.output.y = static_cast<int>(y);
    event_down.canvas.x = static_cast<int>(x);
    event_down.canvas.y = static_cast<int>(y);
    // event_down.flags = EVAS_BUTTON_NONE;
    event_down.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event_down.button = 1;
    if (surface_)
      evas_event_refeed_event(evas_object_evas_get(surface_), &event_down,
                              EVAS_CALLBACK_MOUSE_DOWN);
    LOG_ERROR("CJS Down %p(%p)%d %d (%f %f)", surface_,
              evas_object_evas_get(surface_), type, event_down.timestamp, x, y);

  } else if (type == 1) {  // move event
    Evas_Event_Mouse_Move event;
    event.cur.canvas.x = static_cast<int>(x);
    event.cur.canvas.y = static_cast<int>(y);
    event.cur.output.x = static_cast<int>(x);
    event.cur.output.y = static_cast<int>(y);
    event.prev.canvas.x = static_cast<int>(dx);
    event.prev.canvas.y = static_cast<int>(dy);
    event.prev.output.x = static_cast<int>(dx);
    event.prev.output.y = static_cast<int>(dy);
    event.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event.buttons = 1;
    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_MOUSE_MOVE);
    LOG_ERROR("CJS Move %p(%p)%d %d (%f %f)", surface_,
              evas_object_evas_get(surface_), type, event.timestamp, x, y);
  } else if (type == 2) {  // up event

    // FIXME
    Evas_Event_Mouse_Move event;
    event.cur.canvas.x = static_cast<int>(x);
    event.cur.canvas.y = static_cast<int>(y);
    event.cur.output.x = static_cast<int>(x);
    event.cur.output.y = static_cast<int>(y);
    event.prev.canvas.x = static_cast<int>(dx);
    event.prev.canvas.y = static_cast<int>(dy);
    event.prev.output.x = static_cast<int>(dx);
    event.prev.output.y = static_cast<int>(dy);
    event.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event.buttons = 1;
    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_MOUSE_MOVE);
    //

    Evas_Event_Mouse_Up event_up;
    event_up.output.x = static_cast<int>(x);
    event_up.output.y = static_cast<int>(y);
    event_up.canvas.x = static_cast<int>(x);
    event_up.canvas.y = static_cast<int>(y);
    // event_up.flags = EVAS_BUTTON_NONE;
    event_up.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event_up.button = 1;
    evas_event_refeed_event(evas_object_evas_get(surface_), &event_up,
                            EVAS_CALLBACK_MOUSE_UP);
    LOG_ERROR("CJS Up %p(%p)%d %d (%f %f)", surface_,
              evas_object_evas_get(surface_), type, event_up.timestamp, x, y);
  } else {
    // TODO: Not implemented
  }
}

bool TizenSurfaceView::SendKey(const char* key, const char* string,
                               const char* compose, uint32_t modifiers,
                               uint32_t scan_code, bool is_down) {
  // mutex_.lock();
  if (!IsFocused()) {
    return false;
  }
  LOG_ERROR("CJS KEY %p %p %p", key, string, compose);
  if (is_down) {
    // LOG_ERROR("CJS KEY Down %p %p / %s %s %s %d %d %d", surface_,
    //           evas_object_evas_get(surface_), key, string, compose,
    //           modifiers, scan_code, is_down);
    // evas_event_thaw(evas_object_evas_get(surface_));
    Evas_Event_Key_Down event;
    event.keyname = (char*)key;
    event.key = key;
    event.string = string;
    event.compose = compose;
    event.keycode = scan_code;
    event.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    evas_object_key_grab(surface_, key, 0, 0, EINA_TRUE);

    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_KEY_DOWN);

    // evas_event_feed_key_down(
    //     evas_object_evas_get(surface_), key, key, string, compose,
    //     (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
    //                    0xffffffff),
    // nullptr);

    /*Evas_Event_Key_Up event_up;
    event_up.key = key;
    event_up.string = key;
    event_up.compose = key;
    event_up.keycode = scan_code;
    event_up.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    evas_event_refeed_event(evas_object_evas_get(surface_), &event_up,
                            EVAS_CALLBACK_KEY_UP);*/
    texture_registrar_->MarkTextureFrameAvailable(GetTextureId());
    // mutex_.unlock();
    return true;
  } else {
    // LOG_ERROR("CJS KEY Up %p %p", surface_, evas_object_evas_get(surface_));
    Evas_Event_Key_Up event;
    event.keyname = (char*)key;
    event.key = key;
    event.string = string;
    event.compose = compose;
    event.keycode = scan_code;
    event.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_KEY_UP);
    texture_registrar_->MarkTextureFrameAvailable(GetTextureId());
    // mutex_.unlock();
    return true;
  }
  // mutex_.unlock();
  return false;
}

void TizenSurfaceView::Resume() {
  // ewk_view_resume(tizen_surface_view_instance_);
  LOG_ERROR("CJS Resume");
}

void TizenSurfaceView::Stop() {
  // ewk_view_stop(tizen_surface_view_instance_);
  LOG_ERROR("CJS STOP");
}

void TizenSurfaceView::SetDirection(int direction) {
  // TODO: Implement if necessary.
}

void TizenSurfaceView::InitTizenSurfaceView() {
  pixel_buffer_.reset(new FlutterDesktopPixelBuffer());
  pixel_buffer_->width = 0;
  pixel_buffer_->height = 0;

  ee_ = ecore_evas_ecore_evas_get(evas_object_evas_get((Evas_Object*)surface_));
  pixels_ = (unsigned char*)ecore_evas_buffer_pixels_get(ee_);

  evas_event_callback_add(evas_object_evas_get(surface_),
                          EVAS_CALLBACK_RENDER_POST, frame_update_cb, this);

  for (const std::string& key : kBindableSystemKeys) {
    // eext_win_keygrab_set(surface_, key.c_str());
    evas_object_key_grab((surface_), key.c_str(), 0, 0, EINA_TRUE);
  }
  // evas_object_key_grab

  texture_registrar_->MarkTextureFrameAvailable(GetTextureId());
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
