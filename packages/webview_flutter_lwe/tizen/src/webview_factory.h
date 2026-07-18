// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_WEBVIEW_FACTORY_H_
#define FLUTTER_PLUGIN_WEBVIEW_FACTORY_H_

#include <flutter/encodable_value.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <flutter/texture_registrar.h>
#include <flutter_platform_view.h>

#include <memory>

class WebViewFactory : public PlatformViewFactory {
 public:
  WebViewFactory(flutter::PluginRegistrar* registrar);

  virtual PlatformView* Create(int view_id, double width, double height,
                               const ByteMessage& params) override;

  virtual void Dispose() override;

 private:
  void HandleCookieMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  flutter::TextureRegistrar* texture_registrar_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      cookie_channel_;
};

#endif  // FLUTTER_PLUGIN_WEBVIEW_FACTORY_H_
