// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_TIZEN_VIEW_H_
#define FLUTTER_PLUGIN_TIZEN_VIEW_H_

#include <Elementary.h>
#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter_platform_view.h>

#include <mutex>

class BufferPool;
class BufferUnit;

class TizenView : public PlatformView {
 public:
  TizenView(flutter::PluginRegistrar* registrar, int view_id,
            flutter::TextureRegistrar* texture_registrar, double width,
            double height, const flutter::EncodableValue& params);
  ~TizenView();

  virtual void Dispose() override;

  virtual void Resize(double width, double height) override;
  virtual void Touch(int type, int button, double x, double y, double dx,
                     double dy) override;
  virtual void SetDirection(int direction) override;

  virtual void ClearFocus() override {}

  virtual bool SendKey(const char* key, const char* string, const char* compose,
                       uint32_t modifiers, uint32_t scan_code,
                       bool is_down) override;

  // LWE::WebContainer* GetWebViewInstance() { return webview_instance_; }
  // Evas_Object* GetNativeHandle() { return native_handle_; }

  FlutterDesktopGpuSurfaceDescriptor* ObtainGpuSurface(size_t width,
                                                       size_t height);

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  std::string GetChannelName();

  void InitTizenView();

  // LWE::WebContainer* webview_instance_ = nullptr;
  Evas_Object* native_handle_ = nullptr;
  flutter::TextureRegistrar* texture_registrar_;
  double width_;
  double height_;
  BufferUnit* working_surface_ = nullptr;
  BufferUnit* candidate_surface_ = nullptr;
  BufferUnit* rendered_surface_ = nullptr;
  bool is_mouse_lbutton_down_ = false;
  bool has_navigation_delegate_ = false;
  bool has_progress_tracking_ = false;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
  std::unique_ptr<flutter::TextureVariant> texture_variant_;
  std::mutex mutex_;
  std::unique_ptr<BufferPool> tbm_pool_;
  bool use_sw_backend_;
};

#endif  // FLUTTER_PLUGIN_TIZEN_VIEW_H_
