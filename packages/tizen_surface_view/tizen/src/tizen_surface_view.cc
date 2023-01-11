// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "tizen_surface_view.h"

#include <Ecore_Input_Evas.h>
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

    for (int i = 0; i < width_ * height_ * 4; i += 4) {
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

  pixel_converted = false;
  mutex_.unlock();
  return pixel_buffer_.get();
}

void TizenSurfaceView::frame_update_cb(void* data, Evas* e, void* event_info) {
  TizenSurfaceView* tizen_surface_view = static_cast<TizenSurfaceView*>(data);

  if (tizen_surface_view && tizen_surface_view->pixel_buffer_ &&
      tizen_surface_view->texture_registrar_) {
    // tizen_surface_view->pixel_converted = false;
    tizen_surface_view->pixel_buffer_.reset(new FlutterDesktopPixelBuffer());
    // tizen_surface_view->pixels_ =
    //     (unsigned
    //     char*)ecore_evas_buffer_pixels_get(tizen_surface_view->ee_);

    if (!tizen_surface_view->pixel_converted) {
      unsigned char t = 0;

      for (int i = 0;
           i < tizen_surface_view->width_ * tizen_surface_view->height_ * 4;
           i += 4) {
        tizen_surface_view->temporay_pixel_buffer_[i] =
            tizen_surface_view->pixels_[i + 2];
        tizen_surface_view->temporay_pixel_buffer_[i + 1] =
            tizen_surface_view->pixels_[i + 1];
        tizen_surface_view->temporay_pixel_buffer_[i + 2] =
            tizen_surface_view->pixels_[i];
        tizen_surface_view->temporay_pixel_buffer_[i + 3] =
            tizen_surface_view->pixels_[i + 3];
      }
      // Color convert
      /*for (int i = 0; i < width * height * 4; i += 4) {
        t = pixels_[i];
        pixels_[i] = pixels_[i + 2];
        pixels_[i + 2] = t;
      }*/

      tizen_surface_view->pixel_converted = true;
    }

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
  LOG_ERROR("CJS CHECK");

  InitTizenSurfaceView();

  texture_variant_ =
      std::make_unique<flutter::TextureVariant>(flutter::PixelBufferTexture(
          [this](size_t width,
                 size_t height) -> const FlutterDesktopPixelBuffer* {
            return this->CopyPixelBuffer(width, height);
          }));
  SetTextureId(texture_registrar_->RegisterTexture(texture_variant_.get()));

  channel_ = std::make_unique<FlMethodChannel>(
      GetPluginRegistrar()->messenger(), GetChannelName(),
      &flutter::StandardMethodCodec::GetInstance());
  channel_->SetMethodCallHandler(
      [tizen_surface_view = this](const auto& call, auto result) {
        tizen_surface_view->HandleMethodCall(call, std::move(result));
      });

  texture_registrar_->MarkTextureFrameAvailable(GetTextureId());
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
  // need? (In internal, called menual render function.)
  pixels_ = (unsigned char*)ecore_evas_buffer_pixels_get(ee_);
}

void TizenSurfaceView::Touch(int type, int button, double x, double y,
                             double dx, double dy) {
  bool for_test = true;
  if (!surface_) return;
  if (type == 0) {  // down event
    if (for_test) {
      // FIXME : An internal mouse grab is required before calling re-feed mouse
      // down. To activate this externally, we must call re-feed mouse move
      // before mouse down.
      Evas_Event_Mouse_Move event;
      event.cur.canvas.x = static_cast<int>(x);
      event.cur.canvas.y = static_cast<int>(y);
      event.timestamp =
          (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                         0xffffffff);
      event.buttons = 1;
      event.event_flags = EVAS_EVENT_FLAG_ON_HOLD;

      evas_event_refeed_event(evas_object_evas_get(surface_), &event,
                              EVAS_CALLBACK_MOUSE_MOVE);
      //
    }

    Evas_Event_Mouse_Down event_down;
    event_down.canvas.x = static_cast<int>(x);
    event_down.canvas.y = static_cast<int>(y);
    event_down.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event_down.button = 1;
    event_down.event_flags = EVAS_EVENT_FLAG_ON_HOLD;
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
    event_up.timestamp =
        (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                       0xffffffff);
    event_up.button = 1;
    event_up.event_flags = EVAS_EVENT_FLAG_ON_HOLD;
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
    Evas_Object* focused = elm_object_focused_object_get(surface_);
    if (focused) {
      evas_event_feed_key_down(
          evas_object_evas_get(focused), (char*)key, key, string, compose,
          (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                         0xffffffff),
          nullptr);
    } else {
      // FIXME: Reset to ecore focus for focus-in events.
      if (ecore_evas_focus_get(ee_)) {
        ecore_evas_focus_set(ee_, EINA_FALSE);
        ecore_evas_focus_set(ee_, EINA_TRUE);
      }
      evas_object_focus_set(surface_, EINA_TRUE);
      evas_event_feed_key_down(
          evas_object_evas_get(surface_), (char*)key, key, string, compose,
          (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) 
                         0xffffffff),
          nullptr);
    }

    return true;
  } else {
    Evas_Object* focused = elm_object_focused_object_get(surface_);
    if (focused) {
      evas_event_feed_key_up_with_keycode(
          evas_object_evas_get(focused), (char*)key, key, string, compose,
          (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                         0xffffffff),
          nullptr, scan_code);
    } else {
      evas_event_feed_key_up_with_keycode(
          evas_object_evas_get(surface_), (char*)key, key, string, compose,
          (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) &
                         0xffffffff),
          nullptr, scan_code);
    }
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
  if (ee_) {
    ecore_evas_resize(ee_, width_, height_);
    // ecore_evas_manual_render(ee_);
    pixels_ = (unsigned char*)ecore_evas_buffer_pixels_get(ee_);

    evas_event_callback_add(evas_object_evas_get(surface_),
                            EVAS_CALLBACK_RENDER_POST, frame_update_cb, this);

    // FIXME: Is this necessary?
    // for (const std::string& key : kBindableSystemKeys) {
    //   eext_win_keygrab_set(surface_, key.c_str());
    // }
  } else {
    LOG_ERROR("CJS EE is null");
  }
}

void TizenSurfaceView::HandleMethodCall(
    const FlMethodCall& method_call, std::unique_ptr<FlMethodResult> result) {
  // const std::string& method_name = method_call.method_name();
  // const flutter::EncodableValue* arguments = method_call.arguments();

  result->NotImplemented();
}
