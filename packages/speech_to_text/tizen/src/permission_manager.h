// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_SPEECH_TO_TEXT_PERMISSION_MANAGER_H_
#define FLUTTER_PLUGIN_SPEECH_TO_TEXT_PERMISSION_MANAGER_H_

#include <functional>
#include <string>

enum class PermissionStatus {
  kDenied = 0,
  kGranted = 1,
  kRestricted = 2,
  kLimited = 3,
  kPermanentlyDenied = 4,
  kProvisional = 5,
  kError = 6
};

class PermissionManager {
 public:
  PermissionStatus CheckPermission(const std::string& privilege);

  static void RequestPermission(
      const std::string& privilege,
      std::function<void(PermissionStatus)> on_complete);
};

#endif  // FLUTTER_PLUGIN_SPEECH_TO_TEXT_PERMISSION_MANAGER_H_
