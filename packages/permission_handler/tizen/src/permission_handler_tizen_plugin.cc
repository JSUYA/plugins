// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "permission_handler_tizen_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "app_settings_manager.h"
#include "permission_manager.h"
#include "permissions.h"
#include "service_manager.h"

namespace {

std::string PermissionToPrivilege(Permission permission) {
  switch (permission) {
    case Permission::kCalendar:
      return "http://tizen.org/privilege/calendar.read";
    case Permission::kCamera:
      return "http://tizen.org/privilege/camera";
    case Permission::kContacts:
      return "http://tizen.org/privilege/contact.read";
    case Permission::kLocation:
    case Permission::kLocationAlways:
    case Permission::kLocationWhenInUse:
      return "http://tizen.org/privilege/location";
    case Permission::kMicrophone:
      return "http://tizen.org/privilege/recorder";
    case Permission::kPhone:
      return "http://tizen.org/privilege/call";
    case Permission::kSensors:
    case Permission::kSensorsAlways:
      return "http://tizen.org/privilege/healthinfo";
    case Permission::kSMS:
      return "http://tizen.org/privilege/message.read";
    case Permission::kStorage:
      return "http://tizen.org/privilege/externalstorage";
    case Permission::kMediaLibrary:
      return "http://tizen.org/privilege/mediastorage";
    default:
      return "";
  }
}

class PermissionHandlerTizenPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar *registrar) {
    auto channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "flutter.baseflow.com/permissions/methods",
            &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<PermissionHandlerTizenPlugin>();

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result) {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  }

  PermissionHandlerTizenPlugin() {}

  virtual ~PermissionHandlerTizenPlugin() {}

 private:
  using FlMethodResult = flutter::MethodResult<flutter::EncodableValue>;

  struct PermissionRequest {
    flutter::EncodableList permissions;
    flutter::EncodableMap results;
    size_t index = 0;
    std::shared_ptr<FlMethodResult> result;
  };

  static void RequestNextPermission(
      const std::shared_ptr<PermissionRequest> &request) {
    while (request->index < request->permissions.size()) {
      flutter::EncodableValue argument = request->permissions[request->index++];
      Permission permission = Permission(std::get<int32_t>(argument));
      std::string privilege = PermissionToPrivilege(permission);

      if (privilege.empty()) {
        request->results[argument] = flutter::EncodableValue(
            static_cast<int32_t>(PermissionStatus::kGranted));
        continue;
      }

      PermissionManager::RequestPermission(
          privilege, [request, argument](PermissionStatus status) {
            if (status == PermissionStatus::kError) {
              request->result->Error("Operation failed",
                                     "Failed to request permission.");
              return;
            }
            request->results[argument] =
                flutter::EncodableValue(static_cast<int32_t>(status));
            RequestNextPermission(request);
          });
      return;
    }

    request->result->Success(flutter::EncodableValue(request->results));
  }

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
    const std::string &method_name = method_call.method_name();
    const flutter::EncodableValue *arguments = method_call.arguments();

    if (method_name == "checkServiceStatus") {
      if (std::holds_alternative<int32_t>(*arguments)) {
        Permission permission = Permission(std::get<int32_t>(*arguments));
        ServiceStatus status = service_manager_.CheckServiceStatus(permission);
        result->Success(flutter::EncodableValue(static_cast<int32_t>(status)));
      } else {
        result->Error("Invalid argument", "The argument must be an integer.");
      }
    } else if (method_name == "checkPermissionStatus") {
      if (std::holds_alternative<int32_t>(*arguments)) {
        Permission permission = Permission(std::get<int32_t>(*arguments));
        std::string privilege = PermissionToPrivilege(permission);

        PermissionStatus status;
        if (privilege.empty()) {
          status = PermissionStatus::kGranted;
        } else {
          status = permission_manager_.CheckPermission(privilege);
        }

        if (status != PermissionStatus::kError) {
          result->Success(
              flutter::EncodableValue(static_cast<int32_t>(status)));
        } else {
          result->Error("Operation failed", "Failed to check permission.");
        }
      } else {
        result->Error("Invalid argument", "The argument must be an integer.");
      }
    } else if (method_name == "requestPermissions") {
      if (std::holds_alternative<flutter::EncodableList>(*arguments)) {
        auto request = std::make_shared<PermissionRequest>();
        request->permissions = std::get<flutter::EncodableList>(*arguments);
        request->result = std::shared_ptr<FlMethodResult>(std::move(result));
        RequestNextPermission(request);
      } else {
        result->Error("Invalid argument", "The argument must be a List.");
      }
    } else if (method_name == "openAppSettings") {
      bool opened = settings_manager_.OpenAppSettings();
      result->Success(flutter::EncodableValue(opened));
    } else {
      result->NotImplemented();
    }
  }

  ServiceManager service_manager_;
  PermissionManager permission_manager_;
  AppSettingsManager settings_manager_;
};

}  // namespace

void PermissionHandlerTizenPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  PermissionHandlerTizenPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
