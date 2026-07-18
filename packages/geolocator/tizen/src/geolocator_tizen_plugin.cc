// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "geolocator_tizen_plugin.h"

#include <flutter/event_channel.h>
#include <flutter/event_sink.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <string>

#include "app_settings_manager.h"
#include "location_manager.h"
#include "permission_manager.h"

namespace {

typedef flutter::MethodChannel<flutter::EncodableValue> FlMethodChannel;
typedef flutter::EventChannel<flutter::EncodableValue> FlEventChannel;
typedef flutter::MethodCall<flutter::EncodableValue> FlMethodCall;
typedef flutter::EventSink<flutter::EncodableValue> FlEventSink;
typedef flutter::MethodResult<flutter::EncodableValue> FlMethodResult;
typedef flutter::StreamHandler<flutter::EncodableValue> FlStreamHandler;
typedef flutter::StreamHandlerError<flutter::EncodableValue>
    FlStreamHandlerError;

constexpr char kPrivilegeLocation[] = "http://tizen.org/privilege/location";

class LocationStreamHandler : public FlStreamHandler {
 protected:
  std::unique_ptr<FlStreamHandlerError> OnListenInternal(
      const flutter::EncodableValue *arguments,
      std::unique_ptr<FlEventSink> &&events) override {
    events_ = std::move(events);

    LocationCallback callback = [this](Position position) -> void {
      events_->Success(position.ToEncodableValue());
    };
    try {
      location_manager_.StartListenLocationUpdate(callback);
    } catch (const LocationManagerError &error) {
      return std::make_unique<FlStreamHandlerError>(
          std::to_string(error.GetErrorCode()), error.GetErrorString(),
          nullptr);
    }
    return nullptr;
  }

  std::unique_ptr<FlStreamHandlerError> OnCancelInternal(
      const flutter::EncodableValue *arguments) override {
    try {
      location_manager_.StopListenLocationUpdate();
    } catch (const LocationManagerError &error) {
      return std::make_unique<FlStreamHandlerError>(
          std::to_string(error.GetErrorCode()), error.GetErrorString(),
          nullptr);
    }
    events_.reset();
    return nullptr;
  }

 private:
  LocationManager location_manager_;
  std::unique_ptr<FlEventSink> events_;
};

class ServiceStatusStreamHandler : public FlStreamHandler {
 protected:
  std::unique_ptr<FlStreamHandlerError> OnListenInternal(
      const flutter::EncodableValue *arguments,
      std::unique_ptr<FlEventSink> &&events) override {
    events_ = std::move(events);

    ServiceStatusCallback callback = [this](ServiceStatus status) -> void {
      events_->Success(flutter::EncodableValue(static_cast<int>(status)));
    };
    try {
      location_manager_.StartListenServiceStatusUpdate(callback);
    } catch (const LocationManagerError &error) {
      return std::make_unique<FlStreamHandlerError>(
          std::to_string(error.GetErrorCode()), error.GetErrorString(),
          nullptr);
    }
    return nullptr;
  }

  std::unique_ptr<FlStreamHandlerError> OnCancelInternal(
      const flutter::EncodableValue *arguments) override {
    try {
      location_manager_.StopListenServiceStatusUpdate();
    } catch (const LocationManagerError &error) {
      return std::make_unique<FlStreamHandlerError>(
          std::to_string(error.GetErrorCode()), error.GetErrorString(),
          nullptr);
    }
    events_.reset();
    return nullptr;
  }

 private:
  LocationManager location_manager_;
  std::unique_ptr<FlEventSink> events_;
};

class GeolocatorTizenPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar *registrar) {
    auto channel = std::make_unique<FlMethodChannel>(
        registrar->messenger(), "flutter.baseflow.com/geolocator",
        &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<GeolocatorTizenPlugin>();

    plugin->service_updates_channel_ = std::make_unique<FlEventChannel>(
        registrar->messenger(),
        "flutter.baseflow.com/geolocator_service_updates",
        &flutter::StandardMethodCodec::GetInstance());
    plugin->service_updates_channel_->SetStreamHandler(
        std::make_unique<ServiceStatusStreamHandler>());

    plugin->updates_channel_ = std::make_unique<FlEventChannel>(
        registrar->messenger(), "flutter.baseflow.com/geolocator_updates",
        &flutter::StandardMethodCodec::GetInstance());
    plugin->updates_channel_->SetStreamHandler(
        std::make_unique<LocationStreamHandler>());

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result) {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  }

  GeolocatorTizenPlugin()
      : permission_manager_(std::make_unique<PermissionManager>()),
        location_manager_(std::make_unique<LocationManager>()),
        app_settings_manager_(std::make_unique<AppSettingsManager>()),
        permission_request_pending_(std::make_shared<bool>(false)) {}

  virtual ~GeolocatorTizenPlugin() = default;

 private:
  void HandleMethodCall(const FlMethodCall &method_call,
                        std::unique_ptr<FlMethodResult> result) {
    std::string method_name = method_call.method_name();

    if (method_name == "checkPermission") {
      OnCheckPermission(std::move(result));
    } else if (method_name == "isLocationServiceEnabled") {
      OnIsLocationServiceEnabled(std::move(result));
    } else if (method_name == "requestPermission") {
      OnRequestPermission(std::move(result));
    } else if (method_name == "getLastKnownPosition") {
      OnGetLastKnownPosition(std::move(result));
    } else if (method_name == "getCurrentPosition") {
      OnGetCurrentPosition(std::move(result));
    } else if (method_name == "openAppSettings") {
      bool opened = app_settings_manager_->OpenAppSettings();
      result->Success(flutter::EncodableValue(opened));
    } else if (method_name == "openLocationSettings") {
      bool opened = app_settings_manager_->OpenLocationSettings();
      result->Success(flutter::EncodableValue(opened));
    } else {
      result->NotImplemented();
    }
  }

  void OnCheckPermission(std::unique_ptr<FlMethodResult> result) {
    PermissionStatus permission_status =
        permission_manager_->CheckPermission(kPrivilegeLocation);
    if (permission_status == PermissionStatus::kError) {
      result->Error("Operation failed", "Permission check failed.");
      return;
    }
    result->Success(
        flutter::EncodableValue(static_cast<int>(permission_status)));
  }

  void OnIsLocationServiceEnabled(std::unique_ptr<FlMethodResult> result) {
    try {
      bool is_enabled = location_manager_->IsLocationServiceEnabled();
      result->Success(flutter::EncodableValue(is_enabled));
    } catch (const LocationManagerError &error) {
      result->Error("Operation failed", error.GetErrorString());
    }
  }

  void OnRequestPermission(std::unique_ptr<FlMethodResult> result) {
    if (*permission_request_pending_) {
      result->Error("Operation failed",
                    "A permission request is already in progress.");
      return;
    }

    *permission_request_pending_ = true;
    std::weak_ptr<bool> permission_request_pending =
        permission_request_pending_;
    auto shared_result = std::shared_ptr<FlMethodResult>(std::move(result));
    permission_manager_->RequestPermission(
        kPrivilegeLocation, [permission_request_pending, shared_result](
                                PermissionStatus permission_status) {
          std::shared_ptr<bool> pending = permission_request_pending.lock();
          if (!pending) {
            return;
          }
          *pending = false;

          if (permission_status == PermissionStatus::kError) {
            shared_result->Error("Operation failed",
                                 "Permission request failed.");
          } else if (permission_status == PermissionStatus::kDeniedForever ||
                     permission_status == PermissionStatus::kDenied) {
            shared_result->Error("Permission denied",
                                 "Permission denied by user.");
          } else {
            shared_result->Success(
                flutter::EncodableValue(static_cast<int>(permission_status)));
          }
        });
  }

  void OnGetLastKnownPosition(std::unique_ptr<FlMethodResult> result) {
    try {
      Position position = location_manager_->GetLastKnownPosition();
      result->Success(position.ToEncodableValue());
    } catch (const LocationManagerError &error) {
      result->Error("Operation failed", error.GetErrorString());
    }
  }

  void OnGetCurrentPosition(std::unique_ptr<FlMethodResult> result) {
    if (current_position_result_) {
      result->Error("Operation failed",
                    "A position request is already in progress.");
      return;
    }

    current_position_result_ = std::move(result);
    location_manager_->GetCurrentPosition(
        [this](Position position) {
          if (current_position_result_) {
            current_position_result_->Success(position.ToEncodableValue());
            current_position_result_.reset();
          }
        },
        [this](LocationManagerError error) {
          if (current_position_result_) {
            current_position_result_->Error("Operation failed",
                                            error.GetErrorString());
            current_position_result_.reset();
          }
        });
  }

  std::unique_ptr<FlMethodResult> current_position_result_;
  std::unique_ptr<PermissionManager> permission_manager_;
  std::unique_ptr<LocationManager> location_manager_;
  std::unique_ptr<AppSettingsManager> app_settings_manager_;
  std::shared_ptr<bool> permission_request_pending_;
  std::unique_ptr<FlEventChannel> service_updates_channel_;
  std::unique_ptr<FlEventChannel> updates_channel_;
};

}  // namespace

void GeolocatorTizenPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  GeolocatorTizenPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
