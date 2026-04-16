// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bt_utils.h"

#include <Ecore.h>
#include <network/bluetooth.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>
#include <utility>

namespace flutter_blue_plus_tizen {

namespace {

struct MainThreadTask {
  std::function<void()> fn;
};

}  // namespace

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return value;
}

std::string ToUpper(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return value;
}

std::string NormalizeRemoteId(const std::string& remote_id) {
  return ToUpper(remote_id);
}

std::string NormalizeUuid(std::string uuid) {
  if (uuid.empty()) {
    return uuid;
  }
  uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());
  uuid = ToLower(uuid);
  if (uuid.size() == 4) {
    uuid = "0000" + uuid + "00001000800000805f9b34fb";
  } else if (uuid.size() == 8) {
    uuid = uuid + "00001000800000805f9b34fb";
  }
  if (uuid.size() != 32) {
    return uuid;
  }
  return uuid.substr(0, 8) + "-" + uuid.substr(8, 4) + "-" +
         uuid.substr(12, 4) + "-" + uuid.substr(16, 4) + "-" +
         uuid.substr(20, 12);
}

std::string DescribeBtError(int result) {
  const char* message = get_error_message(result);
  if (message == nullptr || *message == '\0') {
    return "Bluetooth error " + std::to_string(result);
  }
  return std::string(message) + " (" + std::to_string(result) + ")";
}

std::vector<uint8_t> CopyBytes(const char* data, int length) {
  if (data == nullptr || length <= 0) {
    return {};
  }
  const auto* bytes = reinterpret_cast<const uint8_t*>(data);
  return std::vector<uint8_t>(bytes, bytes + length);
}

void DispatchToMain(std::function<void()> fn) {
  auto* task = new MainThreadTask{std::move(fn)};
  ecore_main_loop_thread_safe_call_async(
      [](void* data) {
        std::unique_ptr<MainThreadTask> owned(
            static_cast<MainThreadTask*>(data));
        owned->fn();
      },
      task);
}

}  // namespace flutter_blue_plus_tizen
