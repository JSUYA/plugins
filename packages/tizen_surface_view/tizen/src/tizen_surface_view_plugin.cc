// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_surface_view_plugin.h"

#include <flutter/plugin_registrar.h>
#include <flutter_tizen.h>

#include <memory>

#include "tizen_surface_view_factory.h"

namespace {

constexpr char kViewType[] = "plugins.flutter.io/tizen_surface_view";

class TizenSurfaceViewPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar) {
    auto plugin = std::make_unique<TizenSurfaceViewPlugin>();
    registrar->AddPlugin(std::move(plugin));
  }

  TizenSurfaceViewPlugin() {}

  virtual ~TizenSurfaceViewPlugin() {}
};

}  // namespace

void TizenSurfaceViewPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef core_registrar) {
  flutter::PluginRegistrar* registrar =
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(core_registrar);
  FlutterDesktopViewRef view =
      FlutterDesktopPluginRegistrarGetView(core_registrar);
  FlutterDesktopRegisterViewFactory(
      core_registrar, kViewType,
      std::make_unique<TizenSurfaceViewFactory>(
          registrar, FlutterDesktopViewGetNativeHandle(view),
          FlutterDesktopViewGetPlatformViewSurface(view)));
  TizenSurfaceViewPlugin::RegisterWithRegistrar(registrar);
}
