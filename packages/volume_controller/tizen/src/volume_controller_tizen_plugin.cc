// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "volume_controller_tizen_plugin.h"

#include <flutter/event_channel.h>
#include <flutter/event_sink.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <sound_manager.h>

#include <exception>
#include <memory>
#include <optional>
#include <string>

#include "log.h"

namespace {

constexpr char kMethodChannelName[] =
    "com.kurenai7968.volume_controller.method";
constexpr char kEventChannelName[] =
    "com.kurenai7968.volume_controller.volume_listener_event";
constexpr char kMethodGetVolume[] = "getVolume";
constexpr char kMethodSetVolume[] = "setVolume";
constexpr char kMethodIsMuted[] = "isMuted";
constexpr char kMethodSetMute[] = "setMute";
constexpr char kArgVolume[] = "volume";
constexpr char kArgMuted[] = "isMute";
constexpr char kArgFetchInitialVolume[] = "fetchInitialVolume";

using EncodableValue = flutter::EncodableValue;
using EncodableMap = flutter::EncodableMap;
using FlEventChannel = flutter::EventChannel<EncodableValue>;
using FlEventSink = flutter::EventSink<EncodableValue>;
using FlMethodCall = flutter::MethodCall<EncodableValue>;
using FlMethodChannel = flutter::MethodChannel<EncodableValue>;
using FlMethodResult = flutter::MethodResult<EncodableValue>;
using FlStreamHandler = flutter::StreamHandler<EncodableValue>;
using FlStreamHandlerError = flutter::StreamHandlerError<EncodableValue>;

template <typename T>
bool GetValueFromEncodableMap(const EncodableMap* map, const char* key,
                              T& out) {
  auto iter = map->find(EncodableValue(key));
  if (iter == map->end() || iter->second.IsNull()) {
    return false;
  }
  if (auto* value = std::get_if<T>(&iter->second)) {
    out = *value;
    return true;
  }
  return false;
}

class VolumeControllerStreamHandler : public FlStreamHandler {
 public:
  explicit VolumeControllerStreamHandler(std::optional<int>* previous_volume)
      : previous_volume_(previous_volume) {}

  ~VolumeControllerStreamHandler() override { StopListening(); }

 protected:
  std::unique_ptr<FlStreamHandlerError> OnListenInternal(
      const EncodableValue* arguments,
      std::unique_ptr<FlEventSink>&& events) override {
    events_ = std::move(events);

    bool fetch_initial_volume = true;
    if (const auto* map = std::get_if<EncodableMap>(arguments)) {
      GetValueFromEncodableMap(map, kArgFetchInitialVolume,
                               fetch_initial_volume);
    }

    const int ret = sound_manager_add_volume_changed_cb(
        [](sound_type_e type, unsigned int volume, void* user_data) {
          try {
            auto* handler =
                static_cast<VolumeControllerStreamHandler*>(user_data);
            if (type != SOUND_TYPE_MEDIA || handler->events_ == nullptr) {
              return;
            }
            if (volume > 0) {
              *handler->previous_volume_ = static_cast<int>(volume);
            }
            const auto normalized = handler->NormalizeVolume(volume);
            if (normalized.has_value()) {
              handler->events_->Success(EncodableValue(normalized.value()));
            }
          } catch (const std::exception& error) {
            LOG_ERROR("Volume changed callback failed: %s", error.what());
          } catch (...) {
            LOG_ERROR("Volume changed callback failed.");
          }
        },
        this, &callback_id_);
    if (ret != SOUND_MANAGER_ERROR_NONE) {
      LOG_ERROR("Failed to register volume callback: %d", ret);
      events_.reset();
      return std::make_unique<FlStreamHandlerError>(
          std::to_string(ret), "Failed to register volume callback", nullptr);
    }
    callback_registered_ = true;

    if (fetch_initial_volume) {
      const auto initial = GetNormalizedVolume();
      if (initial.has_value()) {
        events_->Success(EncodableValue(initial.value()));
      }
    }
    return nullptr;
  }

  std::unique_ptr<FlStreamHandlerError> OnCancelInternal(
      const EncodableValue* arguments) override {
    StopListening();
    return nullptr;
  }

 private:
  void StopListening() {
    if (callback_registered_) {
      sound_manager_remove_volume_changed_cb(callback_id_);
      callback_registered_ = false;
    }
    events_.reset();
  }

  std::optional<double> NormalizeVolume(unsigned int volume) const {
    int max_volume = 0;
    const int ret = sound_manager_get_max_volume(SOUND_TYPE_MEDIA, &max_volume);
    if (ret != SOUND_MANAGER_ERROR_NONE || max_volume <= 0) {
      LOG_ERROR("Failed to get max volume: %d", ret);
      return std::nullopt;
    }
    return static_cast<double>(volume) / static_cast<double>(max_volume);
  }

  std::optional<double> GetNormalizedVolume() const {
    int volume = 0;
    const int ret = sound_manager_get_volume(SOUND_TYPE_MEDIA, &volume);
    if (ret != SOUND_MANAGER_ERROR_NONE) {
      LOG_ERROR("Failed to get volume: %d", ret);
      return std::nullopt;
    }
    return NormalizeVolume(volume);
  }

  std::optional<int>* previous_volume_;
  std::unique_ptr<FlEventSink> events_;
  int callback_id_ = 0;
  bool callback_registered_ = false;
};

class VolumeControllerTizenPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar) {
    auto plugin = std::make_unique<VolumeControllerTizenPlugin>();

    auto method_channel = std::make_unique<FlMethodChannel>(
        registrar->messenger(), kMethodChannelName,
        &flutter::StandardMethodCodec::GetInstance());
    method_channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto& call, auto result) {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    plugin->event_channel_ = std::make_unique<FlEventChannel>(
        registrar->messenger(), kEventChannelName,
        &flutter::StandardMethodCodec::GetInstance());
    plugin->event_channel_->SetStreamHandler(
        std::make_unique<VolumeControllerStreamHandler>(
            &plugin->previous_non_zero_volume_));

    plugin->InitializePreviousVolume();
    registrar->AddPlugin(std::move(plugin));
  }

  ~VolumeControllerTizenPlugin() override {
    if (event_channel_) {
      event_channel_->SetStreamHandler(nullptr);
    }
  }

 private:
  void InitializePreviousVolume() {
    int volume = 0;
    if (sound_manager_get_volume(SOUND_TYPE_MEDIA, &volume) ==
            SOUND_MANAGER_ERROR_NONE &&
        volume > 0) {
      previous_non_zero_volume_ = volume;
    }
  }

  void HandleMethodCall(const FlMethodCall& method_call,
                        std::unique_ptr<FlMethodResult> result) {
    const std::string& method_name = method_call.method_name();

    if (method_name == kMethodGetVolume) {
      HandleGetVolume(std::move(result));
      return;
    }
    if (method_name == kMethodSetVolume) {
      const auto* arguments =
          std::get_if<EncodableMap>(method_call.arguments());
      if (arguments == nullptr) {
        result->Error("invalid_arguments", "Arguments must be a map.");
        return;
      }
      HandleSetVolume(arguments, std::move(result));
      return;
    }
    if (method_name == kMethodIsMuted) {
      HandleIsMuted(std::move(result));
      return;
    }
    if (method_name == kMethodSetMute) {
      const auto* arguments =
          std::get_if<EncodableMap>(method_call.arguments());
      if (arguments == nullptr) {
        result->Error("invalid_arguments", "Arguments must be a map.");
        return;
      }
      HandleSetMute(arguments, std::move(result));
      return;
    }

    result->NotImplemented();
  }

  void HandleGetVolume(std::unique_ptr<FlMethodResult> result) {
    int volume = 0;
    int max_volume = 0;
    const int get_ret = sound_manager_get_volume(SOUND_TYPE_MEDIA, &volume);
    const int max_ret =
        sound_manager_get_max_volume(SOUND_TYPE_MEDIA, &max_volume);
    if (get_ret != SOUND_MANAGER_ERROR_NONE ||
        max_ret != SOUND_MANAGER_ERROR_NONE || max_volume <= 0) {
      result->Error("volume_error", "Failed to read the Tizen media volume.");
      return;
    }
    if (volume > 0) {
      previous_non_zero_volume_ = volume;
    }
    result->Success(EncodableValue(static_cast<double>(volume) /
                                   static_cast<double>(max_volume)));
  }

  void HandleSetVolume(const EncodableMap* arguments,
                       std::unique_ptr<FlMethodResult> result) {
    double normalized = 0.0;
    if (!GetValueFromEncodableMap(arguments, kArgVolume, normalized)) {
      result->Error("invalid_arguments", "Missing volume.");
      return;
    }

    if (normalized < 0.0) {
      normalized = 0.0;
    } else if (normalized > 1.0) {
      normalized = 1.0;
    }

    int max_volume = 0;
    const int max_ret =
        sound_manager_get_max_volume(SOUND_TYPE_MEDIA, &max_volume);
    if (max_ret != SOUND_MANAGER_ERROR_NONE || max_volume <= 0) {
      result->Error("volume_error",
                    "Failed to read the Tizen media max volume.");
      return;
    }

    const int volume =
        static_cast<int>(normalized * static_cast<double>(max_volume) + 0.5);
    const int ret = sound_manager_set_volume(SOUND_TYPE_MEDIA, volume);
    if (ret != SOUND_MANAGER_ERROR_NONE) {
      result->Error("volume_error", "Failed to set the Tizen media volume.");
      return;
    }

    if (volume > 0) {
      previous_non_zero_volume_ = volume;
    }
    result->Success();
  }

  void HandleIsMuted(std::unique_ptr<FlMethodResult> result) {
    int volume = 0;
    const int ret = sound_manager_get_volume(SOUND_TYPE_MEDIA, &volume);
    if (ret != SOUND_MANAGER_ERROR_NONE) {
      result->Error("volume_error", "Failed to read the Tizen media volume.");
      return;
    }
    result->Success(EncodableValue(volume == 0));
  }

  void HandleSetMute(const EncodableMap* arguments,
                     std::unique_ptr<FlMethodResult> result) {
    bool muted = false;
    if (!GetValueFromEncodableMap(arguments, kArgMuted, muted)) {
      result->Error("invalid_arguments", "Missing isMute.");
      return;
    }

    int current_volume = 0;
    int max_volume = 0;
    const int get_ret =
        sound_manager_get_volume(SOUND_TYPE_MEDIA, &current_volume);
    const int max_ret =
        sound_manager_get_max_volume(SOUND_TYPE_MEDIA, &max_volume);
    if (get_ret != SOUND_MANAGER_ERROR_NONE ||
        max_ret != SOUND_MANAGER_ERROR_NONE || max_volume <= 0) {
      result->Error("volume_error", "Failed to read Tizen media volume.");
      return;
    }

    int target_volume = current_volume;
    if (muted) {
      if (current_volume > 0) {
        previous_non_zero_volume_ = current_volume;
      }
      target_volume = 0;
    } else if (current_volume == 0) {
      target_volume = previous_non_zero_volume_.value_or(1);
      if (target_volume > max_volume) {
        target_volume = max_volume;
      }
      if (target_volume <= 0) {
        target_volume = 1;
      }
    }

    const int ret = sound_manager_set_volume(SOUND_TYPE_MEDIA, target_volume);
    if (ret != SOUND_MANAGER_ERROR_NONE) {
      result->Error("volume_error", "Failed to set the Tizen media volume.");
      return;
    }

    if (target_volume > 0) {
      previous_non_zero_volume_ = target_volume;
    }
    result->Success();
  }

  std::optional<int> previous_non_zero_volume_;
  std::unique_ptr<FlEventChannel> event_channel_;
};

}  // namespace

void VolumeControllerTizenPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  VolumeControllerTizenPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
