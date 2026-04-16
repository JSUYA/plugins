// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BT_CONSTANTS_H_
#define FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BT_CONSTANTS_H_

#include <limits>

namespace flutter_blue_plus_tizen {

// Flutter channel names. Must match the Dart side in
// `lib/flutter_blue_plus_tizen.dart`.
inline constexpr char kMethodChannelName[] =
    "plugins.flutter.io/flutter_blue_plus_tizen/methods";
inline constexpr char kEventChannelName[] =
    "plugins.flutter.io/flutter_blue_plus_tizen/events";
inline constexpr char kLogTag[] = "FBP-Tizen";

// Adapter state values used by the BmBluetoothAdapterState enum on the Dart
// side. See `BmAdapterStateEnum` in flutter_blue_plus_platform_interface.
inline constexpr int kBmAdapterUnknown = 0;
inline constexpr int kBmAdapterUnavailable = 1;
inline constexpr int kBmAdapterOn = 4;
inline constexpr int kBmAdapterOff = 6;

// Connection state values used by BmConnectionStateEnum.
inline constexpr int kBmConnectionDisconnected = 0;
inline constexpr int kBmConnectionConnected = 1;

// Bond state values used by BmBondStateEnum.
inline constexpr int kBmBondNone = 0;
inline constexpr int kBmBondBonding = 1;
inline constexpr int kBmBondBonded = 2;

// Verbose level value used by BmLogLevel.
inline constexpr int kBmLogVerbose = 5;

// Sentinel for "no integer value" in optional fields (tx_power_level,
// appearance, disconnect_reason_code).
inline constexpr int kNoIntValue = std::numeric_limits<int>::min();

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BT_CONSTANTS_H_
