// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_PAYLOAD_BUILDERS_H_
#define FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_PAYLOAD_BUILDERS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "encodable_utils.h"
#include "gatt_models.h"
#include "scan_data.h"

namespace flutter_blue_plus_tizen {

// Pure helpers that build the EncodableMap shapes expected by the Dart side.
// Keeping them outside the manager class makes them easy to unit-test and
// keeps the manager free of payload-shape concerns.

EncodableMap MakeAdapterStateMap(int adapter_state);
EncodableMap MakePhySupportMap();
EncodableMap MakeBluetoothDeviceMap(const std::string& remote_id,
                                    const std::string& platform_name);
EncodableValue MakeDevicesListValue(const EncodableList& devices);
EncodableMap MakeBondStateMap(const std::string& remote_id, int bond_state,
                              int prev_state);
EncodableMap MakeConnectionStateMap(const std::string& remote_id,
                                    int connection_state,
                                    int disconnect_reason_code,
                                    const std::string& disconnect_reason);
EncodableMap MakeServiceMap(const std::string& remote_id,
                            const std::string& service_uuid,
                            const EncodableList& characteristics);
EncodableMap MakeCharacteristicMap(const std::string& remote_id,
                                   const std::string& service_uuid,
                                   const std::string& characteristic_uuid,
                                   int instance_id,
                                   const EncodableList& descriptors,
                                   const CharacteristicProperties& properties);
EncodableMap MakeDescriptorMap(const std::string& remote_id,
                               const std::string& service_uuid,
                               const std::string& characteristic_uuid,
                               int instance_id,
                               const std::string& descriptor_uuid);
EncodableMap MakeCharacteristicDataMap(const std::string& remote_id,
                                       const std::string& service_uuid,
                                       const std::string& characteristic_uuid,
                                       int instance_id,
                                       const std::vector<uint8_t>& value,
                                       bool success, int error_code,
                                       const std::string& error_string);
EncodableMap MakeDescriptorDataMap(
    const std::string& remote_id, const std::string& service_uuid,
    const std::string& characteristic_uuid, int instance_id,
    const std::string& descriptor_uuid, const std::vector<uint8_t>& value,
    bool success, int error_code, const std::string& error_string);
EncodableMap MakeDiscoverServicesMap(const std::string& remote_id,
                                     const EncodableList& services,
                                     bool success, int error_code,
                                     const std::string& error_string);
EncodableMap MakeMtuChangedMap(const std::string& remote_id, unsigned int mtu,
                               bool success, int error_code,
                               const std::string& error_string);
EncodableMap MakeReadRssiMap(const std::string& remote_id, int rssi,
                             bool success, int error_code,
                             const std::string& error_string);
EncodableMap MakeScanResponseMap(const EncodableList& advertisements,
                                 bool success, int error_code,
                                 const std::string& error_string);
EncodableMap MakeScanAdvertisementMap(
    const ScanAdvertisementData& advertisement,
    const std::string& platform_name);
EncodableMap MakeCharacteristicPropertiesMap(
    const CharacteristicProperties& properties);

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_PAYLOAD_BUILDERS_H_
