// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "speech_to_text_tizen_plugin.h"

#include <Ecore.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <stt.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "log.h"
#include "permission_manager.h"

namespace {

constexpr char kChannelName[] = "plugin.csdcorp.com/speech_to_text";
constexpr char kMethodHasPermission[] = "has_permission";
constexpr char kMethodInitialize[] = "initialize";
constexpr char kMethodListen[] = "listen";
constexpr char kMethodStop[] = "stop";
constexpr char kMethodCancel[] = "cancel";
constexpr char kMethodLocales[] = "locales";

constexpr char kCallbackTextRecognition[] = "textRecognition";
constexpr char kCallbackNotifyError[] = "notifyError";
constexpr char kCallbackNotifyStatus[] = "notifyStatus";
constexpr char kCallbackSoundLevelChange[] = "soundLevelChange";

constexpr char kStatusListening[] = "listening";
constexpr char kStatusNotListening[] = "notListening";
constexpr char kStatusDone[] = "done";
constexpr char kStatusDoneNoResult[] = "doneNoResult";

constexpr char kPrivilegeRecorder[] = "http://tizen.org/privilege/recorder";
constexpr char kRecognitionTypeFree[] = STT_RECOGNITION_TYPE_FREE;
constexpr char kRecognitionTypeFreePartial[] =
    STT_RECOGNITION_TYPE_FREE_PARTIAL;
constexpr char kRecognitionTypeSearch[] = STT_RECOGNITION_TYPE_SEARCH;

constexpr double kSoundLevelIntervalSeconds = 0.1;

using EncodableValue = flutter::EncodableValue;
using EncodableMap = flutter::EncodableMap;
using FlMethodCall = flutter::MethodCall<EncodableValue>;
using FlMethodChannel = flutter::MethodChannel<EncodableValue>;
using FlMethodResult = flutter::MethodResult<EncodableValue>;

template <typename T>
bool GetValueFromEncodableMap(const EncodableMap* map, const char* key,
                              T& out) {
  if (map == nullptr) {
    return false;
  }
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

std::string EscapeJsonString(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char ch : value) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  return escaped;
}

std::string BuildResultJson(const std::vector<std::string>& phrases,
                            bool final_result) {
  std::string json = "{\"alternates\":[";
  for (size_t index = 0; index < phrases.size(); ++index) {
    if (index > 0) {
      json += ",";
    }
    json += "{\"recognizedWords\":\"";
    json += EscapeJsonString(phrases[index]);
    json += "\",\"confidence\":-1.0}";
  }
  json += "],\"resultType\":";
  json += final_result ? "2" : "0";
  json += "}";
  return json;
}

std::string BuildErrorJson(const std::string& error_msg, bool permanent) {
  return "{\"errorMsg\":\"" + EscapeJsonString(error_msg) +
         "\",\"permanent\":" + (permanent ? "true" : "false") + "}";
}

std::pair<std::string, bool> MapErrorCode(int error) {
  switch (error) {
    case STT_ERROR_PERMISSION_DENIED:
      return {"error_permission", true};
    case STT_ERROR_INVALID_LANGUAGE:
      return {"error_language_not_supported", true};
    case STT_ERROR_OUT_OF_NETWORK:
      return {"error_network", false};
    case STT_ERROR_TIMED_OUT:
    case STT_ERROR_RECORDING_TIMED_OUT:
      return {"error_speech_timeout", false};
    case STT_ERROR_NO_SPEECH:
      return {"error_no_match", false};
    case STT_ERROR_RECORDER_BUSY:
      return {"error_busy", false};
    case STT_ERROR_NOT_SUPPORTED:
    case STT_ERROR_NOT_SUPPORTED_FEATURE:
    case STT_ERROR_ENGINE_NOT_FOUND:
      return {"error_server", true};
    default:
      return {"error_unknown", false};
  }
}

std::pair<std::string, bool> MapResultMessage(const std::string& message) {
  if (message == STT_RESULT_MESSAGE_ERROR_TOO_SHORT ||
      message == STT_RESULT_MESSAGE_ERROR_TOO_QUIET ||
      message == STT_RESULT_MESSAGE_ERROR_TOO_FAST ||
      message == STT_RESULT_MESSAGE_ERROR_TOO_SOON) {
    return {"error_no_match", false};
  }
  if (message == STT_RESULT_MESSAGE_ERROR_TOO_LONG) {
    return {"error_speech_timeout", false};
  }
  return {"error_unknown", false};
}

const char* ResolveRecognitionType(int listen_mode, bool partial_results) {
  if (listen_mode == 2) {
    return kRecognitionTypeSearch;
  }
  if (partial_results) {
    return kRecognitionTypeFreePartial;
  }
  return kRecognitionTypeFree;
}

struct LanguageCollectionContext {
  std::vector<std::string>* locales;
  std::string default_language;
};

bool CollectLanguage(stt_h stt, const char* language, void* user_data) {
  auto* context = static_cast<LanguageCollectionContext*>(user_data);
  if (language == nullptr) {
    return true;
  }

  std::string locale(language);
  if (!context->default_language.empty() &&
      locale == context->default_language) {
    return true;
  }

  context->locales->push_back(locale + ":" + locale);
  return true;
}

class SpeechToTextTizenPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar) {
    auto plugin = std::make_unique<SpeechToTextTizenPlugin>(registrar);
    registrar->AddPlugin(std::move(plugin));
  }

  explicit SpeechToTextTizenPlugin(flutter::PluginRegistrar* registrar) {
    channel_ = std::make_unique<FlMethodChannel>(
        registrar->messenger(), kChannelName,
        &flutter::StandardMethodCodec::GetInstance());
    channel_->SetMethodCallHandler([this](const auto& call, auto result) {
      HandleMethodCall(call, std::move(result));
    });
  }

  ~SpeechToTextTizenPlugin() override {
    StopSoundLevelUpdates();

    if (stt_ != nullptr) {
      stt_unset_speech_status_cb(stt_);
      stt_unset_error_cb(stt_);
      stt_unset_state_changed_cb(stt_);
      stt_unset_recognition_result_cb(stt_);

      stt_state_e state = STT_STATE_CREATED;
      if (stt_get_state(stt_, &state) == STT_ERROR_NONE) {
        if (state == STT_STATE_RECORDING || state == STT_STATE_PROCESSING) {
          stt_cancel(stt_);
        }
        if (state != STT_STATE_CREATED) {
          stt_unprepare(stt_);
        }
      }
      stt_destroy(stt_);
      stt_ = nullptr;
    }
  }

 private:
  void HandleMethodCall(const FlMethodCall& method_call,
                        std::unique_ptr<FlMethodResult> result) {
    const std::string& method_name = method_call.method_name();

    if (method_name == kMethodHasPermission) {
      HandleHasPermission(std::move(result));
      return;
    }
    if (method_name == kMethodInitialize) {
      const auto* arguments =
          std::get_if<EncodableMap>(method_call.arguments());
      HandleInitialize(arguments, std::move(result));
      return;
    }
    if (method_name == kMethodListen) {
      const auto* arguments =
          std::get_if<EncodableMap>(method_call.arguments());
      HandleListen(arguments, std::move(result));
      return;
    }
    if (method_name == kMethodStop) {
      HandleStop(std::move(result));
      return;
    }
    if (method_name == kMethodCancel) {
      HandleCancel(std::move(result));
      return;
    }
    if (method_name == kMethodLocales) {
      HandleLocales(std::move(result));
      return;
    }

    result->NotImplemented();
  }

  void HandleHasPermission(std::unique_ptr<FlMethodResult> result) {
    const PermissionStatus status =
        permission_manager_.CheckPermission(kPrivilegeRecorder);
    result->Success(EncodableValue(status == PermissionStatus::kGranted));
  }

  void HandleInitialize(const EncodableMap* arguments,
                        std::unique_ptr<FlMethodResult> result) {
    GetValueFromEncodableMap(arguments, "debugLogging", debug_logging_);

    PermissionStatus permission_status =
        permission_manager_.CheckPermission(kPrivilegeRecorder);
    if (permission_status != PermissionStatus::kGranted) {
      permission_status =
          permission_manager_.RequestPermission(kPrivilegeRecorder);
    }

    if (permission_status != PermissionStatus::kGranted) {
      result->Success(EncodableValue(false));
      return;
    }

    if (!EnsurePrepared()) {
      result->Success(EncodableValue(false));
      return;
    }

    initialized_ = true;
    result->Success(EncodableValue(true));
  }

  void HandleListen(const EncodableMap* arguments,
                    std::unique_ptr<FlMethodResult> result) {
    if (!initialized_ && !EnsurePrepared()) {
      result->Success(EncodableValue(false));
      return;
    }

    std::string locale_id;
    GetValueFromEncodableMap(arguments, "localeId", locale_id);

    bool partial_results = true;
    GetValueFromEncodableMap(arguments, "partialResults", partial_results);

    int32_t listen_mode = 0;
    GetValueFromEncodableMap(arguments, "listenMode", listen_mode);

    std::string selected_language = ResolveLanguage(locale_id);
    if (selected_language.empty()) {
      result->Success(EncodableValue(false));
      return;
    }
    const char* requested_type =
        ResolveRecognitionType(listen_mode, partial_results);
    const char* recognition_type =
        ResolveSupportedRecognitionType(requested_type);

    result_sent_ = false;
    const int ret =
        stt_start(stt_, selected_language.c_str(), recognition_type);
    if (ret != STT_ERROR_NONE) {
      LOG_ERROR("stt_start failed: %d", ret);
      const auto [error_msg, permanent] = MapErrorCode(ret);
      InvokeStringMethod(kCallbackNotifyError,
                         BuildErrorJson(error_msg, permanent));
      result->Success(EncodableValue(false));
      return;
    }

    result->Success(EncodableValue(true));
  }

  void HandleStop(std::unique_ptr<FlMethodResult> result) {
    if (stt_ == nullptr) {
      result->Success();
      return;
    }

    stt_state_e state = STT_STATE_CREATED;
    if (stt_get_state(stt_, &state) != STT_ERROR_NONE ||
        state != STT_STATE_RECORDING) {
      result->Success();
      return;
    }

    const int ret = stt_stop(stt_);
    if (ret != STT_ERROR_NONE) {
      LOG_ERROR("stt_stop failed: %d", ret);
      result->Error("stop_failed", "Failed to stop speech recognition.");
      return;
    }
    result->Success();
  }

  void HandleCancel(std::unique_ptr<FlMethodResult> result) {
    if (stt_ == nullptr) {
      result->Success();
      return;
    }

    stt_state_e state = STT_STATE_CREATED;
    if (stt_get_state(stt_, &state) != STT_ERROR_NONE ||
        (state != STT_STATE_RECORDING && state != STT_STATE_PROCESSING)) {
      result->Success();
      return;
    }

    const int ret = stt_cancel(stt_);
    if (ret != STT_ERROR_NONE) {
      LOG_ERROR("stt_cancel failed: %d", ret);
      result->Error("cancel_failed", "Failed to cancel speech recognition.");
      return;
    }
    result->Success();
  }

  void HandleLocales(std::unique_ptr<FlMethodResult> result) {
    if (!initialized_ && !EnsurePrepared()) {
      result->Success(EncodableValue(flutter::EncodableList()));
      return;
    }

    flutter::EncodableList locales;
    std::string default_language = ResolveLanguage(std::string());
    if (!default_language.empty()) {
      locales.emplace_back(default_language + ":" + default_language);
    }

    std::vector<std::string> others;
    LanguageCollectionContext context{&others, default_language};
    const int ret =
        stt_foreach_supported_languages(stt_, CollectLanguage, &context);
    if (ret != STT_ERROR_NONE) {
      LOG_ERROR("stt_foreach_supported_languages failed: %d", ret);
      result->Success(EncodableValue(locales));
      return;
    }

    for (const auto& locale : others) {
      locales.emplace_back(locale);
    }
    result->Success(EncodableValue(locales));
  }

  bool EnsurePrepared() {
    if (!EnsureCreated()) {
      return false;
    }

    stt_state_e state = STT_STATE_CREATED;
    const int state_ret = stt_get_state(stt_, &state);
    if (state_ret != STT_ERROR_NONE) {
      LOG_ERROR("stt_get_state failed: %d", state_ret);
      return false;
    }

    if (state == STT_STATE_READY || state == STT_STATE_RECORDING ||
        state == STT_STATE_PROCESSING) {
      return true;
    }

    const int ret = stt_prepare(stt_);
    if (ret != STT_ERROR_NONE) {
      LOG_ERROR("stt_prepare failed: %d", ret);
      return false;
    }

    stt_set_silence_detection(stt_, STT_OPTION_SILENCE_DETECTION_AUTO);
    return true;
  }

  bool EnsureCreated() {
    if (stt_ != nullptr) {
      return true;
    }

    const int ret = stt_create(&stt_);
    if (ret != STT_ERROR_NONE || stt_ == nullptr) {
      LOG_ERROR("stt_create failed: %d", ret);
      return false;
    }

    stt_set_recognition_result_cb(stt_, OnRecognitionResult, this);
    stt_set_state_changed_cb(stt_, OnStateChanged, this);
    stt_set_error_cb(stt_, OnError, this);
    stt_set_speech_status_cb(stt_, OnSpeechStatusChanged, this);
    return true;
  }

  std::string ResolveLanguage(const std::string& requested_language) const {
    if (!requested_language.empty()) {
      return requested_language;
    }

    if (stt_ == nullptr) {
      return std::string();
    }

    char* default_language = nullptr;
    const int ret = stt_get_default_language(stt_, &default_language);
    if (ret != STT_ERROR_NONE || default_language == nullptr) {
      LOG_WARN("stt_get_default_language failed: %d", ret);
      return std::string();
    }

    std::string language(default_language);
    free(default_language);
    return language;
  }

  const char* ResolveSupportedRecognitionType(
      const char* requested_type) const {
    if (stt_ == nullptr || requested_type != kRecognitionTypeFreePartial) {
      return requested_type;
    }

    bool supported = false;
    const int ret =
        stt_is_recognition_type_supported(stt_, requested_type, &supported);
    if (ret == STT_ERROR_NONE && supported) {
      return requested_type;
    }
    return kRecognitionTypeFree;
  }

  void StartSoundLevelUpdates() {
    if (sound_level_timer_ != nullptr) {
      return;
    }

    sound_level_timer_ =
        ecore_timer_add(kSoundLevelIntervalSeconds, OnSoundLevelTimer, this);
    if (sound_level_timer_ == nullptr) {
      LOG_WARN("Failed to create sound level timer.");
    }
  }

  void StopSoundLevelUpdates() {
    if (sound_level_timer_ != nullptr) {
      ecore_timer_del(sound_level_timer_);
      sound_level_timer_ = nullptr;
    }
  }

  static Eina_Bool OnSoundLevelTimer(void* data) {
    auto* plugin = static_cast<SpeechToTextTizenPlugin*>(data);
    if (plugin->stt_ == nullptr) {
      plugin->sound_level_timer_ = nullptr;
      return ECORE_CALLBACK_CANCEL;
    }

    stt_state_e state = STT_STATE_CREATED;
    if (stt_get_state(plugin->stt_, &state) != STT_ERROR_NONE ||
        state != STT_STATE_RECORDING) {
      plugin->sound_level_timer_ = nullptr;
      return ECORE_CALLBACK_CANCEL;
    }

    float volume = 0.0f;
    const int ret = stt_get_recording_volume(plugin->stt_, &volume);
    if (ret == STT_ERROR_NONE) {
      plugin->InvokeDoubleMethod(kCallbackSoundLevelChange,
                                 static_cast<double>(volume));
    }
    return ECORE_CALLBACK_RENEW;
  }

  void PostTask(std::function<void()> task) {
    auto* heap_task = new std::function<void()>(std::move(task));
    ecore_main_loop_thread_safe_call_async(
        [](void* data) {
          auto* task = static_cast<std::function<void()>*>(data);
          (*task)();
          delete task;
        },
        heap_task);
  }

  void InvokeStringMethod(const std::string& method, const std::string& value) {
    PostTask([this, method, value]() {
      if (channel_) {
        channel_->InvokeMethod(method, std::make_unique<EncodableValue>(value));
      }
    });
  }

  void InvokeDoubleMethod(const std::string& method, double value) {
    PostTask([this, method, value]() {
      if (channel_) {
        channel_->InvokeMethod(method, std::make_unique<EncodableValue>(value));
      }
    });
  }

  void HandleStateChanged(stt_state_e previous, stt_state_e current) {
    if (debug_logging_) {
      LOG_INFO("STT state changed: %d -> %d", previous, current);
    }

    if (current == STT_STATE_RECORDING) {
      StartSoundLevelUpdates();
      InvokeStringMethod(kCallbackNotifyStatus, kStatusListening);
      return;
    }

    if (previous == STT_STATE_RECORDING && current != STT_STATE_RECORDING) {
      StopSoundLevelUpdates();
      InvokeStringMethod(kCallbackNotifyStatus, kStatusNotListening);
      InvokeStringMethod(kCallbackNotifyStatus,
                         result_sent_ ? kStatusDone : kStatusDoneNoResult);
      return;
    }

    if (current != STT_STATE_RECORDING) {
      StopSoundLevelUpdates();
    }
  }

  void HandleRecognitionResult(stt_result_event_e event,
                               const std::vector<std::string>& phrases,
                               const std::string& msg) {
    if (event == STT_RESULT_EVENT_ERROR) {
      const auto [error_msg, permanent] = MapResultMessage(msg);
      InvokeStringMethod(kCallbackNotifyError,
                         BuildErrorJson(error_msg, permanent));
      return;
    }

    if (phrases.empty()) {
      return;
    }

    result_sent_ = true;
    const bool final_result = event == STT_RESULT_EVENT_FINAL_RESULT;
    InvokeStringMethod(kCallbackTextRecognition,
                       BuildResultJson(phrases, final_result));
  }

  void HandleError(stt_error_e error) {
    const auto [error_msg, permanent] = MapErrorCode(error);
    InvokeStringMethod(kCallbackNotifyError,
                       BuildErrorJson(error_msg, permanent));
  }

  static void OnRecognitionResult(stt_h stt, stt_result_event_e event,
                                  const char** data, int data_count,
                                  const char* msg, void* user_data) {
    auto* plugin = static_cast<SpeechToTextTizenPlugin*>(user_data);
    plugin->PostTask(
        [plugin, event, data_copy = CopyStrings(data, data_count),
         msg_copy = msg == nullptr ? std::string() : std::string(msg)]() {
          plugin->HandleRecognitionResult(event, data_copy, msg_copy);
        });
  }

  static void OnStateChanged(stt_h stt, stt_state_e previous,
                             stt_state_e current, void* user_data) {
    auto* plugin = static_cast<SpeechToTextTizenPlugin*>(user_data);
    plugin->PostTask([plugin, previous, current]() {
      plugin->HandleStateChanged(previous, current);
    });
  }

  static void OnError(stt_h stt, stt_error_e error, void* user_data) {
    auto* plugin = static_cast<SpeechToTextTizenPlugin*>(user_data);
    plugin->PostTask([plugin, error]() { plugin->HandleError(error); });
  }

  static void OnSpeechStatusChanged(stt_h stt, stt_speech_status_e status,
                                    void* user_data) {
    auto* plugin = static_cast<SpeechToTextTizenPlugin*>(user_data);
    if (plugin->debug_logging_) {
      plugin->PostTask(
          [status]() { LOG_INFO("Speech status changed: %d", status); });
    }
  }

  static std::vector<std::string> CopyStrings(const char** data, int count) {
    std::vector<std::string> values;
    if (data == nullptr || count <= 0) {
      return values;
    }

    values.reserve(count);
    for (int index = 0; index < count; ++index) {
      values.emplace_back(data[index] == nullptr ? "" : data[index]);
    }
    return values;
  }

  std::unique_ptr<FlMethodChannel> channel_;
  PermissionManager permission_manager_;
  stt_h stt_ = nullptr;
  Ecore_Timer* sound_level_timer_ = nullptr;
  bool initialized_ = false;
  bool debug_logging_ = false;
  bool result_sent_ = false;
};

}  // namespace

void SpeechToTextTizenPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  SpeechToTextTizenPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
