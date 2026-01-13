#include "tizen_view_plugin.h"

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <flutter_tizen.h>
#include <system_info.h>

#include <memory>
#include <string>

#include "log.h"
#include "tizenview_factory.h"
// #include "message.h"
#include "tizen_view.h"

namespace {

constexpr char kViewType[] = "plugins.flutter.io/tizenview";

class TizenViewPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar) {
    auto plugin = std::make_unique<TizenViewPlugin>();
    registrar->AddPlugin(std::move(plugin));
  }

  TizenViewPlugin() {}

  virtual ~TizenViewPlugin() {}
};

}  // namespace

void TizenViewPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef core_registrar) {
  flutter::PluginRegistrar* registrar =
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(core_registrar);
  FlutterDesktopRegisterViewFactory(
      core_registrar, kViewType, std::make_unique<TizenViewFactory>(registrar));
  TizenViewPlugin::RegisterWithRegistrar(registrar);
}
