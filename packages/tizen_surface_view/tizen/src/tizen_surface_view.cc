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

const std::vector<std::string> kBindableSystemKeys = {
    "XF86Menu",           "XF86Back",         "XF86AudioPlay",
    "XF86AudioPause",     "XF86AudioStop",    "XF86AudioNext",
    "XF86AudioPrev",      "XF86AudioRewind",  "XF86AudioForward",
    "XF86AudioPlayPause", "XF86AudioRecord",  "XF86LowerChannel",
    "XF86RaiseChannel",   "XF86ChannelList",  "XF86PreviousChannel",
    "XF86SimpleMenu",     "XF86History",      "XF86Favorites",
    "XF86Info",           "XF86Red",          "XF86Green",
    "XF86Yellow",         "XF86Blue",         "XF86Subtitle",
    "XF86PlayBack",       "XF86ChannelGuide", "XF86Caption",
    "XF86Exit",
};

typedef flutter::MethodCall<flutter::EncodableValue> FlMethodCall;
typedef flutter::MethodResult<flutter::EncodableValue> FlMethodResult;
typedef flutter::MethodChannel<flutter::EncodableValue> FlMethodChannel;

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
    mutex_.unlock();
    return nullptr;
  }

  if (!pixel_converted) {
    unsigned char t = 0;

    for (int i = 0; i < width * height * 4; i += 4) {
      temporay_pixel_buffer_[i] = pixels_[i + 2];
      temporay_pixel_buffer_[i + 1] = pixels_[i + 1];
      temporay_pixel_buffer_[i + 2] = pixels_[i];
      temporay_pixel_buffer_[i + 3] = pixels_[i + 3];
    }
    // Color convert
    /*for (int i = 0; i < width * height * 4; i += 4) {
      t = pixels_[i];
      pixels_[i] = pixels_[i + 2];
      pixels_[i + 2] = t;
    }*/

    pixel_converted = true;
  }

  pixel_buffer_->width = width;
  pixel_buffer_->height = height;
  pixel_buffer_->buffer = temporay_pixel_buffer_;
  mutex_.unlock();
  return pixel_buffer_.get();
}

void TizenSurfaceView::frame_update_cb(void* data, Evas* e, void* event_info) {
  TizenSurfaceView* tizen_surface_view = static_cast<TizenSurfaceView*>(data);

  if (tizen_surface_view && tizen_surface_view->pixel_buffer_ &&
      tizen_surface_view->texture_registrar_) {
    tizen_surface_view->pixel_converted = false;
    tizen_surface_view->pixel_buffer_.reset(new FlutterDesktopPixelBuffer());
    ecore_evas_manual_render(tizen_surface_view->ee_);
    tizen_surface_view->texture_registrar_->MarkTextureFrameAvailable(
        tizen_surface_view->GetTextureId());
  }
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
  texture_variant_ =
      std::make_unique<flutter::TextureVariant>(flutter::PixelBufferTexture(
          [this](size_t width,
                 size_t height) -> const FlutterDesktopPixelBuffer* {
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
  ecore_evas_resize(ee_, width_, height_);
  // need?
  pixels_ = (unsigned char*)ecore_evas_buffer_pixels_get(ee_);
}

void TizenSurfaceView::Touch(int type, int button, double x, double y,
                             double dx, double dy) {
  bool move_before_event = true;
  if (!surface_) return;
  if (type == 0) {  // down event

    if (move_before_event) {
      // FIXME
      Evas_Event_Mouse_Move event;
      event.cur.canvas.x = static_cast<int>(x);
      event.cur.canvas.y = static_cast<int>(y);
      event.timestamp =
          (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                         0xffffffff);
      event.buttons = 1;

      evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                              EVAS_CALLBACK_MOUSE_MOVE);
      //
    }

    ecore_evas_focus_set(ee_, EINA_TRUE);

    Evas_Event_Mouse_Down event_down;
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

  } else if (type == 1) {  // move event
    Evas_Event_Mouse_Move event;
    event.cur.canvas.x = static_cast<int>(x);
    event.cur.canvas.y = static_cast<int>(y);
    event.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event.buttons = 1;
    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_MOUSE_MOVE);

  } else if (type == 2) {  // up event
    Evas_Event_Mouse_Up event_up;
    event_up.canvas.x = static_cast<int>(x);
    event_up.canvas.y = static_cast<int>(y);
    // event_up.flags = EVAS_BUTTON_NONE;
    event_up.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event_up.button = 1;
    evas_event_refeed_event(evas_object_evas_get(surface_), &event_up,
                            EVAS_CALLBACK_MOUSE_UP);

  } else {
    // TODO: Not implemented
  }
}

bool TizenSurfaceView::SendKey(const char* key, const char* string,
                               const char* compose, uint32_t modifiers,
                               uint32_t scan_code, bool is_down) {
  if (!IsFocused()) {
    return false;
  }

  if (is_down) {
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
    // eext_win_keygrab_set(surface_, key);

    evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                            EVAS_CALLBACK_KEY_DOWN);

    texture_registrar_->MarkTextureFrameAvailable(GetTextureId());
    ecore_evas_focus_set(ee_, EINA_TRUE);

    return true;
  } else {
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
    ecore_evas_focus_set(ee_, EINA_TRUE);

    return true;
  }
  return false;
}

void TizenSurfaceView::Resume() { LOG_ERROR("CJS Resume"); }

void TizenSurfaceView::Stop() { LOG_ERROR("CJS STOP"); }

void TizenSurfaceView::SetDirection(int direction) {
  // TODO: Implement if necessary.
}

void TizenSurfaceView::InitTizenSurfaceView() {
  pixel_buffer_.reset(new FlutterDesktopPixelBuffer());
  pixel_buffer_->width = 0;
  pixel_buffer_->height = 0;

  ee_ = ecore_evas_ecore_evas_get(evas_object_evas_get((Evas_Object*)surface_));
  ecore_evas_resize(ee_, width_, height_);
  ecore_evas_manual_render(ee_);
  pixels_ = (unsigned char*)ecore_evas_buffer_pixels_get(ee_);

  evas_event_callback_add(evas_object_evas_get(surface_),
                          EVAS_CALLBACK_RENDER_POST, frame_update_cb, this);
  LOG_ERROR("CJS ee_ : %p pixels_ : %p", ee_, pixels_);
  for (const std::string& key : kBindableSystemKeys) {
    // eext_win_keygrab_set(surface_, key.c_str());
    evas_object_key_grab((surface_), key.c_str(), 0, 0, EINA_TRUE);
  }
  texture_registrar_->MarkTextureFrameAvailable(GetTextureId());
}

void TizenSurfaceView::HandleMethodCall(
    const FlMethodCall& method_call, std::unique_ptr<FlMethodResult> result) {
  // const std::string& method_name = method_call.method_name();
  // const flutter::EncodableValue* arguments = method_call.arguments();

  result->NotImplemented();
}
