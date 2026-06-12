// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_blue_plus_tizen_plugin.h"

#include <flutter/event_channel.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <utility>

#include "bluetooth_manager.h"
#include "bt_constants.h"

namespace flutter_blue_plus_tizen {

namespace {

using FlEventChannel = flutter::EventChannel<EncodableValue>;
using FlEventSink = flutter::EventSink<EncodableValue>;
using FlMethodChannel = flutter::MethodChannel<EncodableValue>;
using FlStreamHandlerError = flutter::StreamHandlerError<EncodableValue>;

class FlutterBluePlusTizenPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar) {
    auto plugin = std::make_unique<FlutterBluePlusTizenPlugin>();
    auto* raw = plugin.get();

    plugin->method_channel_ = std::make_unique<FlMethodChannel>(
        registrar->messenger(), kMethodChannelName,
        &flutter::StandardMethodCodec::GetInstance());
    plugin->method_channel_->SetMethodCallHandler(
        [raw](const auto& call, auto result) {
          raw->manager_.HandleMethodCall(call, std::move(result));
        });

    plugin->event_channel_ = std::make_unique<FlEventChannel>(
        registrar->messenger(), kEventChannelName,
        &flutter::StandardMethodCodec::GetInstance());
    plugin->event_channel_->SetStreamHandler(
        std::make_unique<flutter::StreamHandlerFunctions<EncodableValue>>(
            [raw](const EncodableValue*, std::unique_ptr<FlEventSink>&& events)
                -> std::unique_ptr<FlStreamHandlerError> {
              raw->manager_.events().SetSink(std::move(events));
              return nullptr;
            },
            [raw](const EncodableValue*)
                -> std::unique_ptr<FlStreamHandlerError> {
              raw->manager_.events().ResetSink();
              return nullptr;
            }));

    registrar->AddPlugin(std::move(plugin));
  }

  FlutterBluePlusTizenPlugin() = default;
  ~FlutterBluePlusTizenPlugin() override = default;

  FlutterBluePlusTizenPlugin(const FlutterBluePlusTizenPlugin&) = delete;
  FlutterBluePlusTizenPlugin& operator=(const FlutterBluePlusTizenPlugin&) =
      delete;

 private:
  BluetoothManager manager_;
  std::unique_ptr<FlMethodChannel> method_channel_;
  std::unique_ptr<FlEventChannel> event_channel_;
};

}  // namespace

}  // namespace flutter_blue_plus_tizen

void FlutterBluePlusTizenPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  flutter_blue_plus_tizen::FlutterBluePlusTizenPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
