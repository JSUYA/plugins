// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_inappwebview_lwe_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <flutter_tizen.h>

#include <memory>

#include "webview.h"
#include "webview_factory.h"

namespace {

constexpr char kViewType[] = "com.pichillilorenzo/flutter_inappwebview_lwe";
constexpr char kStaticChannelName[] =
    "com.pichillilorenzo/flutter_inappwebview_manager";

class FlutterInappwebviewLwePlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar) {
    auto channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), kStaticChannelName,
            &flutter::StandardMethodCodec::GetInstance());
    auto plugin =
        std::make_unique<FlutterInappwebviewLwePlugin>(std::move(channel));
    registrar->AddPlugin(std::move(plugin));
  }

  explicit FlutterInappwebviewLwePlugin(
      std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel)
      : channel_(std::move(channel)) {
    channel_->SetMethodCallHandler([this](const auto& call, auto result) {
      HandleMethodCall(call, std::move(result));
    });
  }

  virtual ~FlutterInappwebviewLwePlugin() {}

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
    const std::string& method_name = method_call.method_name();
    const auto* arguments = method_call.arguments();

    if (method_name == "clearAllCache") {
      WebView::ClearAllCacheAll();
      result->Success();
    } else if (method_name == "disposeKeepAlive" || method_name == "dispose") {
      result->Success();
    } else if (method_name == "handlesURLScheme") {
      std::string url_scheme;
      if (auto* map = std::get_if<flutter::EncodableMap>(arguments)) {
        auto it = map->find(flutter::EncodableValue("urlScheme"));
        if (it != map->end()) {
          if (auto* value = std::get_if<std::string>(&it->second)) {
            url_scheme = *value;
          }
        }
      }
      const bool is_supported = url_scheme == "http" || url_scheme == "https" ||
                                url_scheme == "file" || url_scheme == "data" ||
                                url_scheme == "about" ||
                                url_scheme == "javascript";
      result->Success(flutter::EncodableValue(is_supported));
    } else if (method_name == "getDefaultUserAgent") {
      const auto user_agent = WebView::GetDefaultUserAgent();
      if (user_agent.empty()) {
        result->Error(
            "Unavailable",
            "No active WebView instance is available to derive a user agent.");
      } else {
        result->Success(flutter::EncodableValue(user_agent));
      }
    } else if (method_name == "createInAppWebView") {
      result->Error("Unsupported operation",
                    "Custom platform view is not implemented on "
                    "flutter_inappwebview_lwe.");
    } else {
      result->NotImplemented();
    }
  }

  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
};

}  // namespace

void FlutterInappwebviewLwePluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef core_registrar) {
  flutter::PluginRegistrar* registrar =
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(core_registrar);
  FlutterDesktopRegisterViewFactory(
      core_registrar, kViewType, std::make_unique<WebViewFactory>(registrar));
  FlutterInappwebviewLwePlugin::RegisterWithRegistrar(registrar);
}
