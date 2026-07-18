// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "permission_manager.h"

#ifndef TV_PROFILE
#include <privacy_privilege_manager.h>
#include <tizen.h>

#include <memory>
#endif

#include "log.h"

PermissionStatus PermissionManager::CheckPermission(
    const std::string &privilege) {
#ifdef TV_PROFILE
  return PermissionStatus::kAlways;
#else
  ppm_check_result_e result;
  int ret = ppm_check_permission(privilege.c_str(), &result);
  if (ret != PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE) {
    LOG_ERROR("Permission check failed [%s]: %s", privilege.c_str(),
              get_error_message(ret));
    return PermissionStatus::kError;
  }

  switch (result) {
    case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
      return PermissionStatus::kAlways;
    case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
    case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
    default:
      return PermissionStatus::kDenied;
  }
#endif
}

void PermissionManager::RequestPermission(const std::string &privilege,
                                          PermissionResultCallback callback) {
#ifdef TV_PROFILE
  callback(PermissionStatus::kAlways);
#else
  auto request =
      std::make_unique<PermissionResultCallback>(std::move(callback));
  PermissionResultCallback *request_ptr = request.release();

  int ret = ppm_request_permission(
      privilege.c_str(),
      [](ppm_call_cause_e cause, ppm_request_result_e result,
         const char *privilege, void *user_data) {
        std::unique_ptr<PermissionResultCallback> callback(
            static_cast<PermissionResultCallback *>(user_data));

        if (cause == PRIVACY_PRIVILEGE_MANAGER_CALL_CAUSE_ERROR) {
          LOG_ERROR("Received an error response [%s].", privilege);
          (*callback)(PermissionStatus::kError);
          return;
        }

        switch (result) {
          case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER:
            (*callback)(PermissionStatus::kAlways);
            break;
          case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_FOREVER:
            (*callback)(PermissionStatus::kDeniedForever);
            break;
          case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_ONCE:
          default:
            (*callback)(PermissionStatus::kDenied);
            break;
        }
      },
      request_ptr);

  if (ret != PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE) {
    request.reset(request_ptr);
    LOG_ERROR("Permission request failed [%s]: %s", privilege.c_str(),
              get_error_message(ret));
    (*request)(PermissionStatus::kError);
  }
#endif  // TV_PROFILE
}
