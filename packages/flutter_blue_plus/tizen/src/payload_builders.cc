// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "payload_builders.h"

#include "bt_constants.h"

namespace flutter_blue_plus_tizen {

EncodableMap MakeAdapterStateMap(int adapter_state) {
  return EncodableMap{
      {EncodableValue("adapter_state"), EncodableValue(adapter_state)},
  };
}

EncodableMap MakeBluetoothDeviceMap(const std::string& remote_id,
                                    const std::string& platform_name) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("platform_name"), EncodableValue(platform_name)},
  };
}

EncodableValue MakeDevicesListValue(const EncodableList& devices) {
  return EncodableValue(
      EncodableMap{{EncodableValue("devices"), EncodableValue(devices)}});
}

EncodableMap MakeBondStateMap(const std::string& remote_id, int bond_state,
                              int prev_state) {
  EncodableMap map{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("bond_state"), EncodableValue(bond_state)},
  };
  if (prev_state >= 0) {
    map[EncodableValue("prev_state")] = EncodableValue(prev_state);
  }
  return map;
}

EncodableMap MakeConnectionStateMap(const std::string& remote_id,
                                    int connection_state,
                                    int disconnect_reason_code,
                                    const std::string& disconnect_reason) {
  EncodableMap map{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("connection_state"), EncodableValue(connection_state)},
  };
  if (disconnect_reason_code != kNoIntValue) {
    map[EncodableValue("disconnect_reason_code")] =
        EncodableValue(disconnect_reason_code);
    map[EncodableValue("disconnect_reason_string")] =
        EncodableValue(disconnect_reason);
  }
  return map;
}

EncodableMap MakeServiceMap(const std::string& remote_id,
                            const std::string& service_uuid,
                            const EncodableList& characteristics) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("service_uuid"), EncodableValue(service_uuid)},
      {EncodableValue("characteristics"), EncodableValue(characteristics)},
  };
}

EncodableMap MakeCharacteristicPropertiesMap(
    const CharacteristicProperties& properties) {
  return EncodableMap{
      {EncodableValue("broadcast"), IntFlag(properties.broadcast)},
      {EncodableValue("read"), IntFlag(properties.read)},
      {EncodableValue("write_without_response"),
       IntFlag(properties.write_without_response)},
      {EncodableValue("write"), IntFlag(properties.write)},
      {EncodableValue("notify"), IntFlag(properties.notify)},
      {EncodableValue("indicate"), IntFlag(properties.indicate)},
      {EncodableValue("authenticated_signed_writes"),
       IntFlag(properties.authenticated_signed_writes)},
      {EncodableValue("extended_properties"),
       IntFlag(properties.extended_properties)},
      {EncodableValue("notify_encryption_required"), IntFlag(false)},
      {EncodableValue("indicate_encryption_required"), IntFlag(false)},
  };
}

EncodableMap MakeCharacteristicMap(const std::string& remote_id,
                                   const std::string& service_uuid,
                                   const std::string& characteristic_uuid,
                                   int instance_id,
                                   const EncodableList& descriptors,
                                   const CharacteristicProperties& properties) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("service_uuid"), EncodableValue(service_uuid)},
      {EncodableValue("characteristic_uuid"),
       EncodableValue(characteristic_uuid)},
      {EncodableValue("instance_id"), EncodableValue(instance_id)},
      {EncodableValue("descriptors"), EncodableValue(descriptors)},
      {EncodableValue("properties"),
       EncodableValue(MakeCharacteristicPropertiesMap(properties))},
  };
}

EncodableMap MakeDescriptorMap(const std::string& remote_id,
                               const std::string& service_uuid,
                               const std::string& characteristic_uuid,
                               int instance_id,
                               const std::string& descriptor_uuid) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("service_uuid"), EncodableValue(service_uuid)},
      {EncodableValue("characteristic_uuid"),
       EncodableValue(characteristic_uuid)},
      {EncodableValue("instance_id"), EncodableValue(instance_id)},
      {EncodableValue("descriptor_uuid"), EncodableValue(descriptor_uuid)},
  };
}

EncodableMap MakeCharacteristicDataMap(const std::string& remote_id,
                                       const std::string& service_uuid,
                                       const std::string& characteristic_uuid,
                                       int instance_id,
                                       const std::vector<uint8_t>& value,
                                       bool success, int error_code,
                                       const std::string& error_string) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("service_uuid"), EncodableValue(service_uuid)},
      {EncodableValue("characteristic_uuid"),
       EncodableValue(characteristic_uuid)},
      {EncodableValue("instance_id"), EncodableValue(instance_id)},
      {EncodableValue("value"), EncodableValue(value)},
      {EncodableValue("success"), IntFlag(success)},
      {EncodableValue("error_code"), EncodableValue(error_code)},
      {EncodableValue("error_string"), EncodableValue(error_string)},
  };
}

EncodableMap MakeDescriptorDataMap(
    const std::string& remote_id, const std::string& service_uuid,
    const std::string& characteristic_uuid, int instance_id,
    const std::string& descriptor_uuid, const std::vector<uint8_t>& value,
    bool success, int error_code, const std::string& error_string) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("service_uuid"), EncodableValue(service_uuid)},
      {EncodableValue("characteristic_uuid"),
       EncodableValue(characteristic_uuid)},
      {EncodableValue("instance_id"), EncodableValue(instance_id)},
      {EncodableValue("descriptor_uuid"), EncodableValue(descriptor_uuid)},
      {EncodableValue("value"), EncodableValue(value)},
      {EncodableValue("success"), IntFlag(success)},
      {EncodableValue("error_code"), EncodableValue(error_code)},
      {EncodableValue("error_string"), EncodableValue(error_string)},
  };
}

EncodableMap MakeDiscoverServicesMap(const std::string& remote_id,
                                     const EncodableList& services,
                                     bool success, int error_code,
                                     const std::string& error_string) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("services"), EncodableValue(services)},
      {EncodableValue("success"), IntFlag(success)},
      {EncodableValue("error_code"), EncodableValue(error_code)},
      {EncodableValue("error_string"), EncodableValue(error_string)},
  };
}

EncodableMap MakeMtuChangedMap(const std::string& remote_id, unsigned int mtu,
                               bool success, int error_code,
                               const std::string& error_string) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("mtu"), EncodableValue(static_cast<int32_t>(mtu))},
      {EncodableValue("success"), IntFlag(success)},
      {EncodableValue("error_code"), EncodableValue(error_code)},
      {EncodableValue("error_string"), EncodableValue(error_string)},
  };
}

EncodableMap MakeReadRssiMap(const std::string& remote_id, int rssi,
                             bool success, int error_code,
                             const std::string& error_string) {
  return EncodableMap{
      {EncodableValue("remote_id"), EncodableValue(remote_id)},
      {EncodableValue("rssi"), EncodableValue(rssi)},
      {EncodableValue("success"), IntFlag(success)},
      {EncodableValue("error_code"), EncodableValue(error_code)},
      {EncodableValue("error_string"), EncodableValue(error_string)},
  };
}

EncodableMap MakeScanResponseMap(const EncodableList& advertisements,
                                 bool success, int error_code,
                                 const std::string& error_string) {
  return EncodableMap{
      {EncodableValue("advertisements"), EncodableValue(advertisements)},
      {EncodableValue("success"), IntFlag(success)},
      {EncodableValue("error_code"), EncodableValue(error_code)},
      {EncodableValue("error_string"), EncodableValue(error_string)},
  };
}

EncodableMap MakeScanAdvertisementMap(
    const ScanAdvertisementData& advertisement,
    const std::string& platform_name) {
  EncodableMap manufacturer_data;
  for (const auto& entry : advertisement.manufacturer_data) {
    manufacturer_data.emplace(EncodableValue(entry.first),
                              EncodableValue(entry.second));
  }

  EncodableMap service_data;
  for (const auto& entry : advertisement.service_data) {
    service_data.emplace(EncodableValue(entry.first),
                         EncodableValue(entry.second));
  }

  EncodableList service_uuids;
  for (const auto& uuid : advertisement.service_uuids) {
    service_uuids.push_back(EncodableValue(uuid));
  }

  EncodableMap map{
      {EncodableValue("remote_id"), EncodableValue(advertisement.remote_id)},
      {EncodableValue("platform_name"), EncodableValue(platform_name)},
      {EncodableValue("adv_name"), EncodableValue(advertisement.adv_name)},
      {EncodableValue("connectable"), IntFlag(advertisement.connectable)},
      {EncodableValue("manufacturer_data"), EncodableValue(manufacturer_data)},
      {EncodableValue("service_data"), EncodableValue(service_data)},
      {EncodableValue("service_uuids"), EncodableValue(service_uuids)},
      {EncodableValue("rssi"), EncodableValue(advertisement.rssi)},
  };
  if (advertisement.has_tx_power_level) {
    map[EncodableValue("tx_power_level")] =
        EncodableValue(advertisement.tx_power_level);
  }
  if (advertisement.has_appearance) {
    map[EncodableValue("appearance")] =
        EncodableValue(advertisement.appearance);
  }
  return map;
}

}  // namespace flutter_blue_plus_tizen
