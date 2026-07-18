// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_package_manager.h"

#include <memory>
#include <utility>

#include "log.h"

TizenPackageManager::TizenPackageManager() {
  int ret = package_manager_create(&package_manager_);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_manager_create failed: %s", get_error_message(ret));
    last_error_ = ret;
    return;
  }

  ret = package_manager_set_event_status(
      package_manager_, PACKAGE_MANAGER_STATUS_TYPE_INSTALL |
                            PACKAGE_MANAGER_STATUS_TYPE_UNINSTALL |
                            PACKAGE_MANAGER_STATUS_TYPE_UPGRADE);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_manager_set_event_status failed: %s",
              get_error_message(ret));
    last_error_ = ret;
    return;
  }

  ret = package_manager_set_event_cb(
      package_manager_,
      [](const char *type, const char *package,
         package_manager_event_type_e event_type,
         package_manager_event_state_e event_state, int progress,
         package_manager_error_e error, void *user_data) {
        auto *self = static_cast<TizenPackageManager *>(user_data);

        PacakgeEventState state;
        switch (event_state) {
          case PACKAGE_MANAGER_EVENT_STATE_STARTED:
            state = PacakgeEventState::kStarted;
            break;
          case PACKAGE_MANAGER_EVENT_STATE_PROCESSING:
            state = PacakgeEventState::kProcessing;
            break;
          case PACKAGE_MANAGER_EVENT_STATE_FAILED:
            state = PacakgeEventState::kFailed;
            break;
          case PACKAGE_MANAGER_EVENT_STATE_COMPLETED:
          default:
            state = PacakgeEventState::kCompleted;
            break;
        }

        if (event_type == PACKAGE_MANAGER_EVENT_TYPE_INSTALL) {
          if (self->install_callback_) {
            self->install_callback_(package, type, state, progress);
          }
        } else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL) {
          if (self->uninstall_callback_) {
            self->uninstall_callback_(package, type, state, progress);
          }
        } else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UPDATE) {
          if (self->update_callback_) {
            self->update_callback_(package, type, state, progress);
          }
        }
      },
      this);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_manager_set_event_cb failed: %s",
              get_error_message(ret));
    last_error_ = ret;
  }
}

TizenPackageManager::~TizenPackageManager() {
  if (package_manager_) {
    package_manager_unset_event_cb(package_manager_);
    package_manager_destroy(package_manager_);
  }
}

std::optional<PackageInfo> TizenPackageManager::GetPackageData(
    package_info_h handle, int &error) {
  PackageInfo result = {};

  char *name = nullptr;
  int ret = package_info_get_package(handle, &name);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_get_package failed: %s", get_error_message(ret));
    error = ret;
    return std::nullopt;
  }
  result.package_id = name;
  free(name);

  char *label = nullptr;
  ret = package_info_get_label(handle, &label);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_get_label failed: %s", get_error_message(ret));
    error = ret;
    return std::nullopt;
  }
  result.label = label;
  free(label);

  char *type = nullptr;
  ret = package_info_get_type(handle, &type);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_get_type failed: %s", get_error_message(ret));
    error = ret;
    return std::nullopt;
  }
  result.type = type;
  free(type);

  // The icon path is optional: a package may have no icon, so a failure here is
  // not treated as fatal. Only use the value when the call succeeds and the
  // path is non-empty.
  char *icon_path = nullptr;
  ret = package_info_get_icon(handle, &icon_path);
  if (ret == PACKAGE_MANAGER_ERROR_NONE && icon_path && icon_path[0] != '\0') {
    result.icon_path = icon_path;
  }
  if (icon_path) {
    free(icon_path);
  }

  char *version = nullptr;
  ret = package_info_get_version(handle, &version);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_get_version failed: %s", get_error_message(ret));
    error = ret;
    return std::nullopt;
  }
  result.version = version;
  free(version);

  package_info_installed_storage_type_e storage_type =
      PACKAGE_INFO_INTERNAL_STORAGE;
  ret = package_info_get_installed_storage(handle, &storage_type);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_get_installed_storage failed: %s",
              get_error_message(ret));
    error = ret;
    return std::nullopt;
  }
  if (storage_type == PACKAGE_INFO_EXTERNAL_STORAGE) {
    result.installed_storage_type = "external";
  } else {
    result.installed_storage_type = "internal";
  }

  ret = package_info_is_system_package(handle, &result.is_system);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_is_system_package failed: %s",
              get_error_message(ret));
    error = ret;
    return std::nullopt;
  }

  ret = package_info_is_preload_package(handle, &result.is_preloaded);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_is_preload_package failed: %s",
              get_error_message(ret));
    error = ret;
    return std::nullopt;
  }

  ret = package_info_is_removable_package(handle, &result.is_removable);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_is_removable_package failed: %s",
              get_error_message(ret));
    error = ret;
    return std::nullopt;
  }

  return result;
}

std::optional<PackageInfo> TizenPackageManager::GetPackageInfo(
    const std::string &package_id) {
  package_info_h handle = nullptr;
  int ret = package_info_create(package_id.c_str(), &handle);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_info_create failed: %s", get_error_message(ret));
    last_error_ = ret;
    return std::nullopt;
  }
  int error = PACKAGE_MANAGER_ERROR_NONE;
  std::optional<PackageInfo> package = GetPackageData(handle, error);
  package_info_destroy(handle);
  if (!package.has_value()) {
    last_error_ = error;
  }
  return package;
}

std::optional<std::vector<PackageInfo>> TizenPackageManager::GetAllPackagesInfo(
    int &error) {
  struct EnumerationContext {
    std::vector<PackageInfo> packages;
    int error = PACKAGE_MANAGER_ERROR_NONE;
  } context;

  int ret = package_manager_foreach_package_info(
      [](package_info_h handle, void *user_data) -> bool {
        auto *context = static_cast<EnumerationContext *>(user_data);
        if (handle) {
          std::optional<PackageInfo> package =
              TizenPackageManager::GetPackageData(handle, context->error);
          if (package.has_value()) {
            context->packages.push_back(package.value());
          } else {
            return false;
          }
        }
        return true;
      },
      &context);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_manager_foreach_package_info failed: %s",
              get_error_message(ret));
    error = ret;
    return std::nullopt;
  }

  if (context.error != PACKAGE_MANAGER_ERROR_NONE) {
    // GetPackageData() failed during the iteration.
    error = context.error;
    return std::nullopt;
  }
  error = PACKAGE_MANAGER_ERROR_NONE;
  return std::move(context.packages);
}

void TizenPackageManager::GetPackageSizeInfo(
    const std::string &package_id, OnPackageSizeEvent on_package_size_result) {
  auto request =
      std::make_unique<OnPackageSizeEvent>(std::move(on_package_size_result));
  OnPackageSizeEvent *request_ptr = request.release();

  int ret = package_manager_get_package_size_info(
      package_id.c_str(),
      [](const char *, const package_size_info_h size_info, void *user_data) {
        std::unique_ptr<OnPackageSizeEvent> request(
            static_cast<OnPackageSizeEvent *>(user_data));
        PackageSizeInfo package_size_info;

        if (!size_info) {
          (*request)(package_size_info, false,
                     PACKAGE_MANAGER_ERROR_SYSTEM_ERROR);
          return;
        }

        int error = PACKAGE_MANAGER_ERROR_NONE;
        if ((error = package_size_info_get_data_size(
                 size_info, &package_size_info.data_size)) !=
                PACKAGE_MANAGER_ERROR_NONE ||
            (error = package_size_info_get_cache_size(
                 size_info, &package_size_info.cache_size)) !=
                PACKAGE_MANAGER_ERROR_NONE ||
            (error = package_size_info_get_app_size(
                 size_info, &package_size_info.app_size)) !=
                PACKAGE_MANAGER_ERROR_NONE ||
            (error = package_size_info_get_external_data_size(
                 size_info, &package_size_info.external_data_size)) !=
                PACKAGE_MANAGER_ERROR_NONE ||
            (error = package_size_info_get_external_cache_size(
                 size_info, &package_size_info.external_cache_size)) !=
                PACKAGE_MANAGER_ERROR_NONE ||
            (error = package_size_info_get_external_app_size(
                 size_info, &package_size_info.external_app_size)) !=
                PACKAGE_MANAGER_ERROR_NONE) {
          (*request)(package_size_info, false, error);
          return;
        }

        (*request)(package_size_info, true, PACKAGE_MANAGER_ERROR_NONE);
      },
      request_ptr);

  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    request.reset(request_ptr);
    LOG_ERROR("package_manager_get_package_size_info failed: %s",
              get_error_message(ret));
    (*request)(PackageSizeInfo(), false, ret);
    return;
  }
}

bool TizenPackageManager::Install(const std::string &package_path) {
  package_manager_request_h request = nullptr;
  int ret = package_manager_request_create(&request);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_manager_request_create failed: %s",
              get_error_message(ret));
    last_error_ = ret;
    return false;
  }

  int request_id = 0;
  ret = package_manager_request_install(request, package_path.c_str(),
                                        &request_id);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    package_manager_request_destroy(request);
    LOG_ERROR("package_manager_request_install failed: %s",
              get_error_message(ret));
    last_error_ = ret;
    return false;
  }
  package_manager_request_destroy(request);

  return true;
}

bool TizenPackageManager::Uninstall(const std::string &package_id) {
  package_manager_request_h request = nullptr;
  int ret = package_manager_request_create(&request);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_manager_request_create failed: %s",
              get_error_message(ret));
    last_error_ = ret;
    return false;
  }

  ret = package_manager_request_set_type(request, "unknown");
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    package_manager_request_destroy(request);
    LOG_ERROR("package_manager_request_set_type failed: %s",
              get_error_message(ret));
    last_error_ = ret;
    return false;
  }

  int request_id = 0;
  ret = package_manager_request_uninstall(request, package_id.c_str(),
                                          &request_id);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("package_manager_request_uninstall failed: %s",
              get_error_message(ret));
    package_manager_request_destroy(request);
    last_error_ = ret;
    return false;
  }
  package_manager_request_destroy(request);

  return true;
}
