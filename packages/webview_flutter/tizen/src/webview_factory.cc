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
#include "webview.h"

WebViewFactory::WebViewFactory(flutter::PluginRegistrar* registrar,
                               void* window)
    : PlatformViewFactory(registrar), window_(window) {
  texture_registrar_ = registrar->texture_registrar();

  cookie_channel_ =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "plugins.flutter.io/tizen_cookie_manager",
          &flutter::StandardMethodCodec::GetInstance());
  cookie_channel_->SetMethodCallHandler(
      [this](const auto& call, auto result) {
        HandleCookieMethodCall(call, std::move(result));
      });
}

PlatformView* WebViewFactory::Create(int view_id, double width, double height,
                                     const ByteMessage& params) {
  bool engine_policy = false;
  if (!params.empty()) {
    const std::unique_ptr<flutter::EncodableValue> decoded =
        GetCodec().DecodeMessage(params);
    if (decoded) {
      const bool* value = std::get_if<bool>(decoded.get());
      engine_policy = value && *value;
    }
  }

  auto webview = std::make_unique<WebView>(
      GetPluginRegistrar(), view_id, texture_registrar_, width, height,
      engine_policy, window_, this);
  if (!webview->IsInitialized()) {
    return nullptr;
  }
  RegisterWebView(webview.get());
  return webview.release();
}

void WebViewFactory::Dispose() {
  if (cookie_channel_) {
    cookie_channel_->SetMethodCallHandler(nullptr);
    cookie_channel_.reset();
  }
  webviews_.clear();
}

void WebViewFactory::RegisterWebView(WebView* webview) {
  webviews_.insert(webview);
}

void WebViewFactory::UnregisterWebView(WebView* webview) {
  webviews_.erase(webview);
}

void WebViewFactory::HandleCookieMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name() != "clearCookies") {
    result->NotImplemented();
    return;
  }

  WebView* webview = nullptr;
  for (WebView* candidate : webviews_) {
    if (candidate->IsInitialized()) {
      webview = candidate;
      break;
    }
  }
  if (!webview) {
    result->Error("Invalid operation", "No initialized webview is available.");
    return;
  }

  Ewk_Context* context = ewk_view_context_get(webview->GetWebViewInstance());
  Ewk_Cookie_Manager* cookie_manager =
      context ? ewk_context_cookie_manager_get(context) : nullptr;
  if (!cookie_manager) {
    result->Error("Operation failed", "Failed to get cookie manager.");
    return;
  }

  ewk_cookie_manager_cookies_clear(cookie_manager);
  result->Success(flutter::EncodableValue(true));
}
