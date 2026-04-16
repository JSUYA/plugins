// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "scan_data.h"

#include <cstdlib>
#include <utility>

#include "bt_utils.h"

namespace flutter_blue_plus_tizen {

namespace {

std::string ReadScanDeviceName(bt_adapter_le_device_scan_result_info_s* info,
                               bt_adapter_le_packet_type_e packet_type) {
  char* name = nullptr;
  const int result =
      bt_adapter_le_get_scan_result_device_name(info, packet_type, &name);
  std::string device_name;
  if (result == BT_ERROR_NONE && name != nullptr) {
    device_name = name;
  }
  if (name != nullptr) {
    free(name);
  }
  return device_name;
}

std::set<std::string> ReadServiceUuids(
    bt_adapter_le_device_scan_result_info_s* info,
    bt_adapter_le_packet_type_e packet_type) {
  char** uuids = nullptr;
  int count = 0;
  const int result = bt_adapter_le_get_scan_result_service_uuids(
      info, packet_type, &uuids, &count);
  std::set<std::string> values;
  if (result == BT_ERROR_NONE && uuids != nullptr && count > 0) {
    for (int i = 0; i < count; ++i) {
      if (uuids[i] != nullptr) {
        values.insert(NormalizeUuid(uuids[i]));
        free(uuids[i]);
      }
    }
    free(uuids);
  }
  return values;
}

std::map<std::string, std::vector<uint8_t>> ReadServiceData(
    bt_adapter_le_device_scan_result_info_s* info,
    bt_adapter_le_packet_type_e packet_type) {
  bt_adapter_le_service_data_s* service_data_list = nullptr;
  int count = 0;
  const int result = bt_adapter_le_get_scan_result_service_data_list(
      info, packet_type, &service_data_list, &count);
  std::map<std::string, std::vector<uint8_t>> values;
  if (result == BT_ERROR_NONE && service_data_list != nullptr && count > 0) {
    for (int i = 0; i < count; ++i) {
      if (service_data_list[i].service_uuid != nullptr) {
        values[NormalizeUuid(service_data_list[i].service_uuid)] =
            CopyBytes(service_data_list[i].service_data,
                      service_data_list[i].service_data_len);
      }
    }
    bt_adapter_le_free_service_data_list(service_data_list, count);
  }
  return values;
}

std::map<int, std::vector<uint8_t>> ReadManufacturerData(
    bt_adapter_le_device_scan_result_info_s* info,
    bt_adapter_le_packet_type_e packet_type) {
  int manufacturer_id = 0;
  char* manufacturer_data = nullptr;
  int manufacturer_data_len = 0;
  const int result = bt_adapter_le_get_scan_result_manufacturer_data(
      info, packet_type, &manufacturer_id, &manufacturer_data,
      &manufacturer_data_len);
  std::map<int, std::vector<uint8_t>> values;
  if (result == BT_ERROR_NONE && manufacturer_data != nullptr &&
      manufacturer_data_len > 0) {
    values[manufacturer_id] =
        CopyBytes(manufacturer_data, manufacturer_data_len);
  }
  if (manufacturer_data != nullptr) {
    free(manufacturer_data);
  }
  return values;
}

bool ReadTxPowerLevel(bt_adapter_le_device_scan_result_info_s* info,
                      bt_adapter_le_packet_type_e packet_type,
                      int& tx_power_level) {
  int value = 0;
  const int result =
      bt_adapter_le_get_scan_result_tx_power_level(info, packet_type, &value);
  if (result != BT_ERROR_NONE) {
    return false;
  }
  tx_power_level = value;
  return true;
}

bool ReadAppearance(bt_adapter_le_device_scan_result_info_s* info,
                    bt_adapter_le_packet_type_e packet_type, int& appearance) {
  int value = 0;
  const int result =
      bt_adapter_le_get_scan_result_appearance(info, packet_type, &value);
  if (result != BT_ERROR_NONE) {
    return false;
  }
  appearance = value;
  return true;
}

}  // namespace

ScanSettingsData ParseScanSettings(const EncodableMap* arguments) {
  ScanSettingsData settings;
  for (const auto& uuid : GetStringList(arguments, "with_services")) {
    settings.with_services.push_back(NormalizeUuid(uuid));
  }
  for (const auto& remote_id : GetStringList(arguments, "with_remote_ids")) {
    settings.with_remote_ids.push_back(NormalizeRemoteId(remote_id));
  }
  settings.with_names = GetStringList(arguments, "with_names");
  settings.with_keywords = GetStringList(arguments, "with_keywords");
  GetBoolLike(arguments, "continuous_updates", settings.continuous_updates);
  GetIntLike(arguments, "continuous_divisor", settings.continuous_divisor);
  if (settings.continuous_divisor < 1) {
    settings.continuous_divisor = 1;
  }

  for (const auto& filter_map : GetMapList(arguments, "with_msd")) {
    MsdFilterData filter;
    GetIntLike(&filter_map, "manufacturer_id", filter.manufacturer_id);
    filter.data = GetBytesOrEmpty(&filter_map, "data");
    filter.mask = GetBytesOrEmpty(&filter_map, "mask");
    settings.with_msd.push_back(std::move(filter));
  }

  for (const auto& filter_map : GetMapList(arguments, "with_service_data")) {
    ServiceDataFilterData filter;
    std::string service_uuid;
    GetValueFromEncodableMap(&filter_map, "service", service_uuid);
    filter.service_uuid = NormalizeUuid(service_uuid);
    filter.data = GetBytesOrEmpty(&filter_map, "data");
    filter.mask = GetBytesOrEmpty(&filter_map, "mask");
    settings.with_service_data.push_back(std::move(filter));
  }
  return settings;
}

void ReadAdvertisementData(bt_adapter_le_device_scan_result_info_s* info,
                           ScanAdvertisementData& advertisement) {
  advertisement.adv_name =
      ReadScanDeviceName(info, BT_ADAPTER_LE_PACKET_SCAN_RESPONSE);
  if (advertisement.adv_name.empty()) {
    advertisement.adv_name =
        ReadScanDeviceName(info, BT_ADAPTER_LE_PACKET_ADVERTISING);
  }

  auto advertising_manufacturer =
      ReadManufacturerData(info, BT_ADAPTER_LE_PACKET_ADVERTISING);
  advertisement.manufacturer_data.insert(advertising_manufacturer.begin(),
                                         advertising_manufacturer.end());
  auto scan_response_manufacturer =
      ReadManufacturerData(info, BT_ADAPTER_LE_PACKET_SCAN_RESPONSE);
  advertisement.manufacturer_data.insert(scan_response_manufacturer.begin(),
                                         scan_response_manufacturer.end());

  auto advertising_service =
      ReadServiceData(info, BT_ADAPTER_LE_PACKET_ADVERTISING);
  advertisement.service_data.insert(advertising_service.begin(),
                                    advertising_service.end());
  auto scan_response_service =
      ReadServiceData(info, BT_ADAPTER_LE_PACKET_SCAN_RESPONSE);
  advertisement.service_data.insert(scan_response_service.begin(),
                                    scan_response_service.end());

  auto advertising_uuids =
      ReadServiceUuids(info, BT_ADAPTER_LE_PACKET_ADVERTISING);
  advertisement.service_uuids.insert(advertising_uuids.begin(),
                                     advertising_uuids.end());
  auto scan_response_uuids =
      ReadServiceUuids(info, BT_ADAPTER_LE_PACKET_SCAN_RESPONSE);
  advertisement.service_uuids.insert(scan_response_uuids.begin(),
                                     scan_response_uuids.end());

  int tx_power_level = 0;
  if (ReadTxPowerLevel(info, BT_ADAPTER_LE_PACKET_SCAN_RESPONSE,
                       tx_power_level) ||
      ReadTxPowerLevel(info, BT_ADAPTER_LE_PACKET_ADVERTISING,
                       tx_power_level)) {
    advertisement.tx_power_level = tx_power_level;
    advertisement.has_tx_power_level = true;
  }

  int appearance = 0;
  if (ReadAppearance(info, BT_ADAPTER_LE_PACKET_SCAN_RESPONSE, appearance) ||
      ReadAppearance(info, BT_ADAPTER_LE_PACKET_ADVERTISING, appearance)) {
    advertisement.appearance = appearance;
    advertisement.has_appearance = true;
  }
}

}  // namespace flutter_blue_plus_tizen
