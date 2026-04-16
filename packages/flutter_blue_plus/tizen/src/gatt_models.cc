// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gatt_models.h"

#include "bt_utils.h"

namespace flutter_blue_plus_tizen {

std::string MakeCharacteristicKey(const std::string& remote_id,
                                  const std::string& service_uuid,
                                  const std::string& characteristic_uuid,
                                  int instance_id) {
  return remote_id + "|" + NormalizeUuid(service_uuid) + "|" +
         NormalizeUuid(characteristic_uuid) + "|" + std::to_string(instance_id);
}

std::string MakeDescriptorKey(const std::string& remote_id,
                              const std::string& service_uuid,
                              const std::string& characteristic_uuid,
                              int instance_id,
                              const std::string& descriptor_uuid) {
  return MakeCharacteristicKey(remote_id, service_uuid, characteristic_uuid,
                               instance_id) +
         "|" + NormalizeUuid(descriptor_uuid);
}

CharacteristicProperties ReadCharacteristicProperties(bt_gatt_h handle) {
  int properties = 0;
  if (bt_gatt_characteristic_get_properties(handle, &properties) !=
      BT_ERROR_NONE) {
    properties = 0;
  }

  CharacteristicProperties data;
  data.broadcast = (properties & BT_GATT_PROPERTY_BROADCAST) != 0;
  data.read = (properties & BT_GATT_PROPERTY_READ) != 0;
  data.write_without_response =
      (properties & BT_GATT_PROPERTY_WRITE_WITHOUT_RESPONSE) != 0;
  data.write = (properties & BT_GATT_PROPERTY_WRITE) != 0;
  data.notify = (properties & BT_GATT_PROPERTY_NOTIFY) != 0;
  data.indicate = (properties & BT_GATT_PROPERTY_INDICATE) != 0;
  data.authenticated_signed_writes =
      (properties & BT_GATT_PROPERTY_AUTHENTICATED_SIGNED_WRITES) != 0;
  data.extended_properties =
      (properties & BT_GATT_PROPERTY_EXTENDED_PROPERTIES) != 0;
  return data;
}

DeviceInfoSnapshot SnapshotDeviceInfo(const bt_device_info_s& info) {
  DeviceInfoSnapshot snapshot;
  if (info.remote_address != nullptr) {
    snapshot.remote_id = NormalizeRemoteId(info.remote_address);
  }
  if (info.remote_name != nullptr) {
    snapshot.name = info.remote_name;
  }
  snapshot.is_connected = info.is_connected;
  for (int i = 0; i < info.service_count; ++i) {
    if (info.service_uuid != nullptr && info.service_uuid[i] != nullptr) {
      snapshot.service_uuids.insert(NormalizeUuid(info.service_uuid[i]));
    }
  }
  return snapshot;
}

}  // namespace flutter_blue_plus_tizen
