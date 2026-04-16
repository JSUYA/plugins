// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_SCAN_DATA_H_
#define FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_SCAN_DATA_H_

#include <network/bluetooth.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "encodable_utils.h"

namespace flutter_blue_plus_tizen {

struct MsdFilterData {
  int manufacturer_id = 0;
  std::vector<uint8_t> data;
  std::vector<uint8_t> mask;
};

struct ServiceDataFilterData {
  std::string service_uuid;
  std::vector<uint8_t> data;
  std::vector<uint8_t> mask;
};

struct ScanSettingsData {
  std::vector<std::string> with_services;
  std::vector<std::string> with_remote_ids;
  std::vector<std::string> with_names;
  std::vector<std::string> with_keywords;
  std::vector<MsdFilterData> with_msd;
  std::vector<ServiceDataFilterData> with_service_data;
  bool continuous_updates = false;
  int continuous_divisor = 1;

  bool HasFilters() const {
    return !with_services.empty() || !with_remote_ids.empty() ||
           !with_names.empty() || !with_keywords.empty() || !with_msd.empty() ||
           !with_service_data.empty();
  }
};

struct ScanAdvertisementData {
  std::string remote_id;
  std::string adv_name;
  int rssi = 0;
  bool connectable = true;
  int tx_power_level = 0;
  int appearance = 0;
  bool has_tx_power_level = false;
  bool has_appearance = false;
  std::map<int, std::vector<uint8_t>> manufacturer_data;
  std::map<std::string, std::vector<uint8_t>> service_data;
  std::set<std::string> service_uuids;
};

ScanSettingsData ParseScanSettings(const EncodableMap* arguments);

// Reads name, manufacturer data, service data, service UUIDs, tx power level
// and appearance from a Tizen scan-result info struct.
void ReadAdvertisementData(bt_adapter_le_device_scan_result_info_s* info,
                           ScanAdvertisementData& advertisement);

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_SCAN_DATA_H_
