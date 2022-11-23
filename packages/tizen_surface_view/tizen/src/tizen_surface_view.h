// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_TIZEN_SURFACE_VIEW_H_
#define FLUTTER_PLUGIN_TIZEN_SURFACE_VIEW_H_

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
                   void* win);
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

  void Resume();

  void Stop();

  Evas_Object* GetTizenSurfaceViewInstance() {
    return tizen_surface_view_instance_;
  }

  FlutterDesktopGpuSurfaceDescriptor* ObtainGpuSurface(size_t width,
                                                       size_t height);

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  std::string GetChannelName();

  void InitTizenSurfaceView();

  static void OnFrameRendered(void* data, Evas_Object* obj, void* event_info);

  Evas_Object* tizen_surface_view_instance_ = nullptr;
  void* win_ = nullptr;
  flutter::TextureRegistrar* texture_registrar_;
  double width_ = 0.0;
  double height_ = 0.0;
  BufferUnit* working_surface_ = nullptr;
  BufferUnit* candidate_surface_ = nullptr;
  BufferUnit* rendered_surface_ = nullptr;
  bool has_navigation_delegate_ = false;
  bool has_progress_tracking_ = false;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
  std::unique_ptr<flutter::TextureVariant> texture_variant_;
  std::mutex mutex_;
  std::unique_ptr<BufferPool> tbm_pool_;
};

#endif  // FLUTTER_PLUGIN_TIZEN_SURFACE_VIEW_H_
