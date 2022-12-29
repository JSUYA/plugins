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

  Evas_Object* GetTizenSurfaceViewInstance() {
    return tizen_surface_view_instance_;
  }
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

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  std::string GetChannelName();

  void InitTizenSurfaceView();

  Evas_Object* tizen_surface_view_instance_ = nullptr;
  void* win_ = nullptr;
  Evas_Object* surface_ = nullptr;
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
  std::shared_ptr<FlutterDesktopPixelBuffer> pixel_buffer_;

  unsigned char* pixels_;
  unsigned char pixels2_[2000 * 2000];
  Ecore_Evas* ee_;
  Evas_Object* image_;
  int w_;
  int h_;
  bool pixel_converted = false;
};

#endif  // FLUTTER_PLUGIN_TIZEN_SURFACE_VIEW_H_
