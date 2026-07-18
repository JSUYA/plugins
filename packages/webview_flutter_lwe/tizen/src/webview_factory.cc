// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webview_factory.h"

#include <app_common.h>
#include <flutter/encodable_value.h>
#include <flutter/message_codec.h>

#include <string>
#include <variant>

#include "log.h"
#include "lwe/LWEWebView.h"
#include "webview.h"

static std::string GetAppDataPath() {
  char* path = app_get_data_path();
  if (!path) {
    return "/tmp/";
  }
  std::string result = std::string(path);
  free(path);
  return result;
}

WebViewFactory::WebViewFactory(flutter::PluginRegistrar* registrar)
    : PlatformViewFactory(registrar) {
  texture_registrar_ = registrar->texture_registrar();

  std::string data_path = GetAppDataPath() + std::string("Starfish_storage");
  LWE::LWE::Initialize(data_path.c_str());

  cookie_channel_ =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "plugins.flutter.io/lwe_cookie_manager",
          &flutter::StandardMethodCodec::GetInstance());
  cookie_channel_->SetMethodCallHandler(
      [this](const auto& call, auto result) {
        HandleCookieMethodCall(call, std::move(result));
      });
}

PlatformView* WebViewFactory::Create(int view_id, double width, double height,
                                     const ByteMessage&) {
  auto webview = std::make_unique<WebView>(
      GetPluginRegistrar(), view_id, texture_registrar_, width, height);
  if (!webview->IsInitialized()) {
    return nullptr;
  }
  return webview.release();
}

void WebViewFactory::Dispose() {
  if (cookie_channel_) {
    cookie_channel_->SetMethodCallHandler(nullptr);
    cookie_channel_.reset();
  }
  LWE::LWE::Finalize();
}

void WebViewFactory::HandleCookieMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name() != "clearCookies") {
    result->NotImplemented();
    return;
  }

  LWE::CookieManager::GetInstance()->ClearCookies();
  result->Success(flutter::EncodableValue(true));
}
