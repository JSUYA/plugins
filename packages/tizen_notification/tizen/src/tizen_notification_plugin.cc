// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_notification_plugin.h"

#include <app_common.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <notification.h>
#include <notification_error.h>

#include <filesystem>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "log.h"

namespace {

template <typename T>
static bool GetValueFromEncodableMap(const flutter::EncodableMap *map,
                                     const char *key, T &out) {
  auto iter = map->find(flutter::EncodableValue(key));
  if (iter != map->end() && !iter->second.IsNull()) {
    if (auto *value = std::get_if<T>(&iter->second)) {
      out = *value;
      return true;
    }
  }
  return false;
}

class TizenNotificationPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar *registrar) {
    auto channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "tizen/notification",
            &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<TizenNotificationPlugin>();

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result) {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  }

  TizenNotificationPlugin() {}

  virtual ~TizenNotificationPlugin() {}

 private:
  void FreeNotification(notification_h handle) {
    int ret = notification_free(handle);
    if (ret != NOTIFICATION_ERROR_NONE) {
      LOG_ERROR("notification_free failed: %s", get_error_message(ret));
    }
  }

  void DestroyAppControl(app_control_h app_control) {
    int ret = app_control_destroy(app_control);
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG_ERROR("app_control_destroy failed: %s", get_error_message(ret));
    }
  }

  notification_image_type_e StringToImageType(const std::string &type) {
    if (type == "icon") {
      return NOTIFICATION_IMAGE_TYPE_ICON;
    } else if (type == "iconForIndicator") {
      return NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR;
    } else if (type == "iconForLock") {
      return NOTIFICATION_IMAGE_TYPE_ICON_FOR_LOCK;
    }
    return NOTIFICATION_IMAGE_TYPE_ICON;
  }

  notification_sound_type_e StringToSoundType(const std::string &type) {
    if (type == "none") {
      return NOTIFICATION_SOUND_TYPE_NONE;
    } else if (type == "builtIn") {
      return NOTIFICATION_SOUND_TYPE_DEFAULT;
    } else if (type == "userData") {
      return NOTIFICATION_SOUND_TYPE_USER_DATA;
    }
    return NOTIFICATION_SOUND_TYPE_NONE;
  }

  notification_vibration_type_e StringToVibrationType(const std::string &type) {
    if (type == "none") {
      return NOTIFICATION_VIBRATION_TYPE_NONE;
    } else if (type == "builtIn") {
      return NOTIFICATION_VIBRATION_TYPE_DEFAULT;
    } else if (type == "userData") {
      return NOTIFICATION_VIBRATION_TYPE_USER_DATA;
    }
    return NOTIFICATION_VIBRATION_TYPE_NONE;
  }

  bool ToAbsolutePath(std::string &path) {
    std::filesystem::path filesystem_path(path);
    if (filesystem_path.is_absolute()) {
      return true;
    }
    char *res_path = app_get_shared_resource_path();
    if (!res_path) {
      return false;
    }
    path = std::string(res_path) + path;
    free(res_path);
    return true;
  }

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
    const auto &method_name = method_call.method_name();

    if (method_name == "show") {
      const auto *arguments =
          std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (!arguments) {
        result->Error("Invalid argument", "The argument must be a map.");
        return;
      }

      std::string id;
      if (!GetValueFromEncodableMap(arguments, "id", id)) {
        result->Error("Invalid argument", "No id provided.");
        return;
      }

      notification_h handle = notification_load_by_tag(id.c_str());
      if (handle) {
        // The notification with the ID already exists. Delete it.
        int ret = notification_delete(handle);
        FreeNotification(handle);
        handle = nullptr;
        if (ret != NOTIFICATION_ERROR_NONE) {
          result->Error("notification_delete failed", get_error_message(ret));
          return;
        }
      }

      bool ongoing = false;
      GetValueFromEncodableMap(arguments, "ongoing", ongoing);
      if (ongoing) {
        handle = notification_create(NOTIFICATION_TYPE_ONGOING);
        if (!handle) {
          result->Error("notification_create failed",
                        "Failed to create an ongoing notification.");
          return;
        }
        int ret =
            notification_set_layout(handle, NOTIFICATION_LY_ONGOING_EVENT);
        if (ret != NOTIFICATION_ERROR_NONE) {
          FreeNotification(handle);
          result->Error("notification_set_layout failed",
                        get_error_message(ret));
          return;
        }
      } else {
        handle = notification_create(NOTIFICATION_TYPE_NOTI);
        if (!handle) {
          result->Error("notification_create failed",
                        "Failed to create a notification.");
          return;
        }
      }

      int ret = notification_set_tag(handle, id.c_str());
      if (ret != NOTIFICATION_ERROR_NONE) {
        FreeNotification(handle);
        result->Error("notification_set_tag failed", get_error_message(ret));
        return;
      }

      std::string title;
      if (GetValueFromEncodableMap(arguments, "title", title)) {
        ret = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_TITLE,
                                    title.c_str(), nullptr,
                                    NOTIFICATION_VARIABLE_TYPE_NONE);
        if (ret != NOTIFICATION_ERROR_NONE) {
          FreeNotification(handle);
          result->Error("notification_set_text failed", get_error_message(ret));
          return;
        }
      }

      std::string body;
      if (GetValueFromEncodableMap(arguments, "body", body)) {
        ret = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_CONTENT,
                                    body.c_str(), nullptr,
                                    NOTIFICATION_VARIABLE_TYPE_NONE);
        if (ret != NOTIFICATION_ERROR_NONE) {
          FreeNotification(handle);
          result->Error("notification_set_text failed", get_error_message(ret));
          return;
        }
      }

      flutter::EncodableMap images;
      if (GetValueFromEncodableMap(arguments, "image", images)) {
        for (const auto &image : images) {
          if (!std::holds_alternative<std::string>(image.first)) {
            FreeNotification(handle);
            result->Error("Invalid argument",
                          "Notification image keys must be strings.");
            return;
          }
          const std::string &type = std::get<std::string>(image.first);
          if (image.second.IsNull()) {
            continue;
          }
          if (!std::holds_alternative<std::string>(image.second)) {
            FreeNotification(handle);
            result->Error("Invalid argument",
                          "Notification image paths must be strings.");
            return;
          }
          std::string path = std::get<std::string>(image.second);
          if (!ToAbsolutePath(path)) {
            FreeNotification(handle);
            result->Error("storage_error",
                          "Failed to get the shared resource path.");
            return;
          }
          ret = notification_set_image(handle, StringToImageType(type),
                                       path.c_str());
          if (ret != NOTIFICATION_ERROR_NONE) {
            FreeNotification(handle);
            result->Error("notification_set_image failed",
                          get_error_message(ret));
            return;
          }
        }
      }

      int32_t display_applist = 0;
      if (GetValueFromEncodableMap(arguments, "displayApplist",
                                   display_applist)) {
        ret = notification_set_display_applist(handle, display_applist);
        if (ret != NOTIFICATION_ERROR_NONE) {
          FreeNotification(handle);
          result->Error("notification_set_display_applist failed",
                        get_error_message(ret));
          return;
        }
      }

      int32_t properties = 0;
      if (GetValueFromEncodableMap(arguments, "properties", properties)) {
        ret = notification_set_property(handle, properties);
        if (ret != NOTIFICATION_ERROR_NONE) {
          FreeNotification(handle);
          result->Error("notification_set_property failed",
                        get_error_message(ret));
          return;
        }
      }

      flutter::EncodableMap sound;
      if (GetValueFromEncodableMap(arguments, "sound", sound)) {
        std::string type;
        GetValueFromEncodableMap(&sound, "type", type);
        std::string path;
        if (GetValueFromEncodableMap(&sound, "path", path)) {
          if (!ToAbsolutePath(path)) {
            FreeNotification(handle);
            result->Error("storage_error",
                          "Failed to get the shared resource path.");
            return;
          }
        }
        ret = notification_set_sound(handle, StringToSoundType(type),
                                     path.c_str());
        if (ret != NOTIFICATION_ERROR_NONE) {
          FreeNotification(handle);
          result->Error("notification_set_sound failed",
                        get_error_message(ret));
          return;
        }
      }

      flutter::EncodableMap vibration;
      if (GetValueFromEncodableMap(arguments, "vibration", vibration)) {
        std::string type;
        GetValueFromEncodableMap(&vibration, "type", type);
        std::string path;
        if (GetValueFromEncodableMap(&vibration, "path", path)) {
          if (!ToAbsolutePath(path)) {
            FreeNotification(handle);
            result->Error("storage_error",
                          "Failed to get the shared resource path.");
            return;
          }
        }
        ret = notification_set_vibration(handle, StringToVibrationType(type),
                                         path.c_str());
        if (ret != NOTIFICATION_ERROR_NONE) {
          FreeNotification(handle);
          result->Error("notification_set_vibration failed",
                        get_error_message(ret));
          return;
        }
      }

      flutter::EncodableMap app_control_data;
      if (GetValueFromEncodableMap(arguments, "appControl", app_control_data)) {
        app_control_h app_control = nullptr;
        ret = app_control_create(&app_control);
        if (ret != APP_CONTROL_ERROR_NONE) {
          FreeNotification(handle);
          result->Error("app_control_create failed", get_error_message(ret));
          return;
        }

        std::string app_id;
        if (GetValueFromEncodableMap(&app_control_data, "appId", app_id)) {
          ret = app_control_set_app_id(app_control, app_id.c_str());
          if (ret != APP_CONTROL_ERROR_NONE) {
            DestroyAppControl(app_control);
            FreeNotification(handle);
            result->Error("app_control_set_app_id failed",
                          get_error_message(ret));
            return;
          }
        }

        std::string operation;
        if (GetValueFromEncodableMap(&app_control_data, "operation",
                                     operation)) {
          ret = app_control_set_operation(app_control, operation.c_str());
          if (ret != APP_CONTROL_ERROR_NONE) {
            DestroyAppControl(app_control);
            FreeNotification(handle);
            result->Error("app_control_set_operation failed",
                          get_error_message(ret));
            return;
          }
        }

        std::string uri;
        if (GetValueFromEncodableMap(&app_control_data, "uri", uri)) {
          ret = app_control_set_uri(app_control, uri.c_str());
          if (ret != APP_CONTROL_ERROR_NONE) {
            DestroyAppControl(app_control);
            FreeNotification(handle);
            result->Error("app_control_set_uri failed", get_error_message(ret));
            return;
          }
        }

        std::string category;
        if (GetValueFromEncodableMap(&app_control_data, "category", category)) {
          ret = app_control_set_category(app_control, category.c_str());
          if (ret != APP_CONTROL_ERROR_NONE) {
            DestroyAppControl(app_control);
            FreeNotification(handle);
            result->Error("app_control_set_category failed",
                          get_error_message(ret));
            return;
          }
        }

        std::string mime;
        if (GetValueFromEncodableMap(&app_control_data, "mime", mime)) {
          ret = app_control_set_mime(app_control, mime.c_str());
          if (ret != APP_CONTROL_ERROR_NONE) {
            DestroyAppControl(app_control);
            FreeNotification(handle);
            result->Error("app_control_set_mime failed",
                          get_error_message(ret));
            return;
          }
        }

        flutter::EncodableMap extras;
        if (GetValueFromEncodableMap(&app_control_data, "extraData", extras)) {
          for (const auto &extra : extras) {
            if (!std::holds_alternative<std::string>(extra.first)) {
              DestroyAppControl(app_control);
              FreeNotification(handle);
              result->Error("Invalid argument",
                            "App control extra data keys must be strings.");
              return;
            }
            const std::string &key = std::get<std::string>(extra.first);
            int extra_result = APP_CONTROL_ERROR_NONE;
            if (const auto *value = std::get_if<std::string>(&extra.second)) {
              extra_result = app_control_add_extra_data(
                  app_control, key.c_str(), value->c_str());
            } else if (const auto *value_list =
                           std::get_if<flutter::EncodableList>(&extra.second)) {
              if (value_list->empty()) {
                DestroyAppControl(app_control);
                FreeNotification(handle);
                result->Error(
                    "Invalid argument",
                    "App control extra data lists must not be empty.");
                return;
              }
              std::vector<std::string> string_values;
              for (const flutter::EncodableValue &value : *value_list) {
                const auto *string_value = std::get_if<std::string>(&value);
                if (!string_value) {
                  DestroyAppControl(app_control);
                  FreeNotification(handle);
                  result->Error(
                      "Invalid argument",
                      "App control extra data values must be strings.");
                  return;
                }
                string_values.push_back(*string_value);
              }
              std::vector<const char *> values;
              for (const std::string &value : string_values) {
                values.push_back(value.c_str());
              }
              extra_result = app_control_add_extra_data_array(
                  app_control, key.c_str(), values.data(), values.size());
            } else {
              DestroyAppControl(app_control);
              FreeNotification(handle);
              result->Error(
                  "Invalid argument",
                  "App control extra data must be a string or string list.");
              return;
            }
            if (extra_result != APP_CONTROL_ERROR_NONE) {
              DestroyAppControl(app_control);
              FreeNotification(handle);
              result->Error("app_control_add_extra_data failed",
                            get_error_message(extra_result));
              return;
            }
          }
        }

        ret = notification_set_launch_option(
            handle, NOTIFICATION_LAUNCH_OPTION_APP_CONTROL, app_control);
        if (ret != NOTIFICATION_ERROR_NONE) {
          FreeNotification(handle);
          DestroyAppControl(app_control);
          result->Error("notification_set_launch_option failed",
                        get_error_message(ret));
          return;
        }
        DestroyAppControl(app_control);
      }

      ret = notification_post(handle);
      if (ret != NOTIFICATION_ERROR_NONE) {
        FreeNotification(handle);
        result->Error("notification_post failed", get_error_message(ret));
        return;
      }
      FreeNotification(handle);

      result->Success();
    } else if (method_name == "cancel") {
      const auto *id = std::get_if<std::string>(method_call.arguments());
      if (!id) {
        result->Error("Invalid argument", "The argument must be a string.");
        return;
      }

      notification_h handle = notification_load_by_tag(id->c_str());
      if (!handle) {
        result->Error("Invalid argument",
                      "No notification found with the given ID.");
        return;
      }

      int ret = notification_delete(handle);
      FreeNotification(handle);
      if (ret != NOTIFICATION_ERROR_NONE) {
        result->Error(std::to_string(ret), get_error_message(ret));
        return;
      }
      result->Success();
    } else if (method_name == "cancelAll") {
      int ret = notification_delete_all(NOTIFICATION_TYPE_NOTI);
      if (ret != NOTIFICATION_ERROR_NONE) {
        result->Error(std::to_string(ret), get_error_message(ret));
        return;
      }

      ret = notification_delete_all(NOTIFICATION_TYPE_ONGOING);
      if (ret != NOTIFICATION_ERROR_NONE) {
        result->Error(std::to_string(ret), get_error_message(ret));
        return;
      }
      result->Success();
    } else {
      result->NotImplemented();
    }
  }
};

}  // namespace

void TizenNotificationPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  TizenNotificationPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
