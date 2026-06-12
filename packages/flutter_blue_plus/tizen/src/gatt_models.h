// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_GATT_MODELS_H_
#define FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_GATT_MODELS_H_

#include <network/bluetooth.h>

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace flutter_blue_plus_tizen {

struct CharacteristicProperties {
  bool broadcast = false;
  bool read = false;
  bool write_without_response = false;
  bool write = false;
  bool notify = false;
  bool indicate = false;
  bool authenticated_signed_writes = false;
  bool extended_properties = false;
};

struct DeviceInfoSnapshot {
  std::string remote_id;
  std::string name;
  bool is_connected = false;
  std::set<std::string> service_uuids;
};

struct CharacteristicMeta {
  std::string remote_id;
  std::string service_uuid;
  std::string characteristic_uuid;
  int instance_id = 0;
  CharacteristicProperties properties;
  bt_gatt_h handle = nullptr;
};

struct DescriptorMeta {
  std::string remote_id;
  std::string service_uuid;
  std::string characteristic_uuid;
  int instance_id = 0;
  std::string descriptor_uuid;
  bt_gatt_h handle = nullptr;
};

enum class PendingGattKind {
  kReadCharacteristic,
  kWriteCharacteristic,
  kReadDescriptor,
  kWriteDescriptor,
};

struct PendingGattRequest {
  PendingGattKind kind;
  std::string remote_id;
  std::string service_uuid;
  std::string characteristic_uuid;
  int instance_id = 0;
  std::string descriptor_uuid;
  std::vector<uint8_t> value;
  uintptr_t handle_key = 0;
};

struct PendingMtuRequest {
  std::string remote_id;
  unsigned int requested_mtu = 0;
};

// Composite key uniquely identifying a discovered characteristic for a given
// remote device. Encoded as a string so it can be used as a std::map key
// without a custom hasher.
std::string MakeCharacteristicKey(const std::string& remote_id,
                                  const std::string& service_uuid,
                                  const std::string& characteristic_uuid,
                                  int instance_id);

std::string MakeDescriptorKey(const std::string& remote_id,
                              const std::string& service_uuid,
                              const std::string& characteristic_uuid,
                              int instance_id,
                              const std::string& descriptor_uuid);

CharacteristicProperties ReadCharacteristicProperties(bt_gatt_h handle);

DeviceInfoSnapshot SnapshotDeviceInfo(const bt_device_info_s& info);

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_GATT_MODELS_H_
