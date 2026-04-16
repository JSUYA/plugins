// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BT_UTILS_H_
#define FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BT_UTILS_H_

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace flutter_blue_plus_tizen {

std::string ToLower(std::string value);
std::string ToUpper(std::string value);

// Upper-case canonical form of a Bluetooth MAC address.
std::string NormalizeRemoteId(const std::string& remote_id);

// Canonical lowercase 128-bit form (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx).
// 16- or 32-bit short UUIDs are expanded with the Bluetooth Base UUID.
// Strings that are not a recognised UUID length are returned untouched after
// hyphen-stripping and lowercasing so the caller can still match against
// platform-specific values.
std::string NormalizeUuid(std::string uuid);

// Wraps a Tizen Bluetooth error code into a human readable message that
// includes the numeric code (for cross-referencing the Dart-side enum).
std::string DescribeBtError(int result);

// Owning copy of `length` bytes from `data`. Returns an empty vector if data
// is null or length is non-positive.
std::vector<uint8_t> CopyBytes(const char* data, int length);

// Posts `fn` to the main (Ecore) loop. Safe to call from any thread.
void DispatchToMain(std::function<void()> fn);

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BT_UTILS_H_
