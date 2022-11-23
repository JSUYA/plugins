// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_TIZEN_SURFACE_VIEW_H_
#define FLUTTER_PLUGIN_TIZEN_SURFACE_VIEW_H_

#include <Elementary.h>
#include <Evas.h>
#include <flutter/encodable_value.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_message_codec.h>
#include <flutter/texture_registrar.h>
#include <flutter_platform_view.h>

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

class BufferPool;
class BufferUnit;

class TizenSurfaceView : public PlatformView {
 public:
  TizenSurfaceView(flutter::PluginRegistrar* registrar, int view_id,
                   flutter::TextureRegistrar* texture_registrar, double width,
                   double height, const flutter::EncodableValue& params,
                   void* win, void* surface);
  ~TizenSurfaceView();

  virtual void Dispose() override;

  virtual void Resize(double width, double height) override;
  virtual void Touch(int type, int button, double x, double y, double dx,
                     double dy) override;
  virtual void SetDirection(int direction) override;

  virtual void ClearFocus() override {}

  virtual bool SendKey(const char* key, const char* string, const char* compose,
                       uint32_t modifiers, uint32_t scan_code,
                       bool is_down) override;

  FlutterDesktopPixelBuffer* CopyPixelBuffer(size_t width, size_t height);
  static void frame_update_cb(void* data, Evas* e, void* event_info);

  void Resume();

  void Stop();

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  std::string GetChannelName();

  void InitTizenSurfaceView();

  void* win_ = nullptr;
  Evas_Object* surface_ = nullptr;
  flutter::TextureRegistrar* texture_registrar_;
  double width_ = 0.0;
  double height_ = 0.0;

  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
  std::unique_ptr<flutter::TextureVariant> texture_variant_;
  std::mutex mutex_;
  std::shared_ptr<FlutterDesktopPixelBuffer> pixel_buffer_;

  unsigned char* pixels_;
  unsigned char temporay_pixel_buffer_[1000 * 1000];
  Ecore_Evas* ee_;
  bool pixel_converted = false;
};

#endif  // FLUTTER_PLUGIN_TIZEN_SURFACE_VIEW_H_
