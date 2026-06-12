// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BLUETOOTH_MANAGER_H_
#define FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BLUETOOTH_MANAGER_H_

#include <Ecore.h>
#include <flutter/encodable_value.h>
#include <flutter/method_call.h>
#include <flutter/method_result.h>
#include <network/bluetooth.h>

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "encodable_utils.h"
#include "event_dispatcher.h"
#include "gatt_models.h"
#include "scan_data.h"

namespace flutter_blue_plus_tizen {

// Owns all Tizen Bluetooth state used by the plugin and is the only place
// that talks to the bt_* C API. All state must be touched from the platform
// main thread; bt_* callbacks re-enter through DispatchToMain().
class BluetoothManager {
 public:
  using FlMethodCall = flutter::MethodCall<EncodableValue>;
  using FlMethodResult = flutter::MethodResult<EncodableValue>;

  BluetoothManager();
  ~BluetoothManager();

  BluetoothManager(const BluetoothManager&) = delete;
  BluetoothManager& operator=(const BluetoothManager&) = delete;

  EventDispatcher& events() { return events_; }

  void HandleMethodCall(const FlMethodCall& method_call,
                        std::unique_ptr<FlMethodResult> result);

 private:
  void EnsureInitialized();
  bool RegisterGlobalCallbacks();
  void BestEffortCleanupGlobalCallbacks(bool deinitialize);
  void CleanupBluetooth();
  int ReadAdapterState();

  void HandleGetAdapterName(std::unique_ptr<FlMethodResult> result);
  void HandleGetAdapterState(std::unique_ptr<FlMethodResult> result);
  void HandleSetLogLevel(const EncodableMap* arguments,
                         std::unique_ptr<FlMethodResult> result);
  void HandleConnect(const EncodableMap* arguments,
                     std::unique_ptr<FlMethodResult> result);
  void HandleDisconnect(const EncodableMap* arguments,
                        std::unique_ptr<FlMethodResult> result);
  void HandleDiscoverServices(const EncodableMap* arguments,
                              std::unique_ptr<FlMethodResult> result);
  void HandleReadCharacteristic(const EncodableMap* arguments,
                                std::unique_ptr<FlMethodResult> result);
  void HandleWriteCharacteristic(const EncodableMap* arguments,
                                 std::unique_ptr<FlMethodResult> result);
  void HandleReadDescriptor(const EncodableMap* arguments,
                            std::unique_ptr<FlMethodResult> result);
  void HandleWriteDescriptor(const EncodableMap* arguments,
                             std::unique_ptr<FlMethodResult> result);
  void HandleSetNotifyValue(const EncodableMap* arguments,
                            std::unique_ptr<FlMethodResult> result);
  void HandleStartScan(const EncodableMap* arguments,
                       std::unique_ptr<FlMethodResult> result);
  void HandleStopScan(std::unique_ptr<FlMethodResult> result);
  void HandleReadRssi(const EncodableMap* arguments,
                      std::unique_ptr<FlMethodResult> result);
  void HandleRequestMtu(const EncodableMap* arguments,
                        std::unique_ptr<FlMethodResult> result);
  void HandleRequestConnectionPriority(const EncodableMap* arguments,
                                       std::unique_ptr<FlMethodResult> result);
  void HandleCreateBond(const EncodableMap* arguments,
                        std::unique_ptr<FlMethodResult> result);
  void HandleRemoveBond(const EncodableMap* arguments,
                        std::unique_ptr<FlMethodResult> result);
  void HandleGetBondState(const EncodableMap* arguments,
                          std::unique_ptr<FlMethodResult> result);
  void HandleGetBondedDevices(std::unique_ptr<FlMethodResult> result);
  void HandleGetSystemDevices(const EncodableMap* arguments,
                              std::unique_ptr<FlMethodResult> result);
  void HandleTurnOn(std::unique_ptr<FlMethodResult> result);
  void HandleTurnOff(std::unique_ptr<FlMethodResult> result);

  bool EnsureClient(const std::string& remote_id, std::string& error_message);
  void RegisterClientCallbacks(bt_gatt_client_h client);
  std::string GetRemoteIdFromClient(bt_gatt_client_h client);
  void DestroyClient(const std::string& remote_id);
  void ClearCachedGattDatabase(const std::string& remote_id);
  EncodableList BuildServices(const std::string& remote_id,
                              bt_gatt_client_h client,
                              std::string* error_message);

  std::vector<DeviceInfoSnapshot> EnumerateBondedDevices();
  int GetBondStateValue(const std::string& remote_id);

  std::string GetGattUuid(bt_gatt_h handle);
  int SetGattValue(bt_gatt_h handle, const std::vector<uint8_t>& value);
  std::vector<uint8_t> GetGattValueBytes(bt_gatt_h handle);

  bool BuildCharacteristicRequest(const EncodableMap* arguments,
                                  PendingGattKind kind,
                                  PendingGattRequest& info);
  bool BuildDescriptorRequest(const EncodableMap* arguments,
                              PendingGattKind kind, PendingGattRequest& info);

  void EmitConnectionState(const std::string& remote_id, int connection_state,
                           int disconnect_reason_code,
                           const std::string& disconnect_reason);
  void EmitBondState(const std::string& remote_id, int bond_state,
                     int prev_state);
  void EmitDiscoverServicesFailure(const std::string& remote_id, int error_code,
                                   const std::string& error_string);
  void EmitCharacteristicData(const std::string& remote_id,
                              const std::string& service_uuid,
                              const std::string& characteristic_uuid,
                              int instance_id,
                              const std::vector<uint8_t>& value, bool success,
                              int error_code, const std::string& error_string,
                              bool written);
  void EmitDescriptorData(const std::string& remote_id,
                          const std::string& service_uuid,
                          const std::string& characteristic_uuid,
                          int instance_id, const std::string& descriptor_uuid,
                          const std::vector<uint8_t>& value, bool success,
                          int error_code, const std::string& error_string,
                          bool written);
  void EmitCccdWrite(const std::string& remote_id,
                     const std::string& service_uuid,
                     const std::string& characteristic_uuid, int instance_id,
                     const std::vector<uint8_t>& value, bool success,
                     int error_code, const std::string& error_string);
  void EmitMtuChanged(const std::string& remote_id, unsigned int mtu,
                      bool success, int error_code,
                      const std::string& error_string);
  void EmitReadRssi(const std::string& remote_id, int rssi, bool success,
                    int error_code, const std::string& error_string);
  void EmitScanFailure(int error_code, const std::string& error_string);

  bool MatchesScanRequest(const ScanAdvertisementData& advertisement) const;
  void UpdateDeviceName(const std::string& remote_id, const std::string& name);
  void EmitCurrentMtu(const std::string& remote_id);
  void FailPendingRequestsForRemoteId(const std::string& remote_id,
                                      int error_code,
                                      const std::string& error_string);

  // Main-thread handlers fed by the static bt_* callbacks below.
  void OnAdapterStateChangedOnMain(int result, bt_adapter_state_e state);
  void OnAdapterNameChangedOnMain(std::string name);
  void OnConnectionStateChangedOnMain(int result, bool connected,
                                      std::string remote_id);
  void OnBondCreatedOnMain(int result, DeviceInfoSnapshot device_info);
  void OnBondDestroyedOnMain(int result, std::string remote_id);
  void OnMtuChangedOnMain(uintptr_t client_key, std::string remote_id,
                          unsigned int mtu, unsigned int status);
  void OnServiceChangedOnMain(uintptr_t client_key, int change_type,
                              std::string service_uuid);
  void OnGattRequestCompletedOnMain(PendingGattRequest info, int result,
                                    bt_gatt_h request_handle);
  void OnCharacteristicValueChangedOnMain(uintptr_t handle_key,
                                          std::vector<uint8_t> value);
  void OnScanResultOnMain(int result, ScanAdvertisementData advertisement);
  Eina_Bool TickDiscoverServices(const std::string& remote_id,
                                 int& remaining_attempts);

  static void OnAdapterStateChangedCb(int result, bt_adapter_state_e state,
                                      void* user_data);
  static void OnAdapterNameChangedCb(char* device_name, void* user_data);
  static void OnConnectionStateChangedCb(int result, bool connected,
                                         const char* remote_address,
                                         void* user_data);
  static void OnBondCreatedCb(int result, bt_device_info_s* device_info,
                              void* user_data);
  static void OnBondDestroyedCb(int result, char* remote_address,
                                void* user_data);
  static void OnMtuChangedCb(bt_gatt_client_h client,
                             const bt_gatt_client_att_mtu_info_s* mtu_info,
                             void* user_data);
  static void OnServiceChangedCb(
      bt_gatt_client_h client, bt_gatt_client_service_change_type_e change_type,
      const char* service_uuid, void* user_data);
  static void OnGattRequestCompletedCb(int result, bt_gatt_h request_handle,
                                       void* user_data);
  static void OnCharacteristicValueChangedCb(bt_gatt_h characteristic,
                                             char* value, int len,
                                             void* user_data);
  static void OnScanResultCb(int result,
                             bt_adapter_le_device_scan_result_info_s* info,
                             void* user_data);
  static Eina_Bool OnDiscoverServicesTimerCb(void* data);

  EventDispatcher events_;
  bool initialized_ = false;
  bool supported_ = true;
  std::string unsupported_reason_;
  std::string adapter_name_;
  int adapter_state_;
  bool scanning_ = false;

  std::unique_ptr<ScanSettingsData> active_scan_settings_;
  std::set<std::string> auto_connect_remote_ids_;
  std::set<std::string> connected_remote_ids_;
  std::set<std::string> pending_connects_;
  std::map<std::string, std::string> device_names_;
  std::map<std::string, int> last_scan_rssi_;
  std::map<std::string, int> scan_event_counts_;
  std::map<std::string, int> pending_bond_states_;
  std::map<std::string, std::set<std::string>> service_uuids_by_remote_id_;
  std::map<std::string, bt_gatt_client_h> clients_by_remote_id_;
  std::map<uintptr_t, std::string> remote_id_by_client_handle_;
  std::map<std::string, CharacteristicMeta> characteristics_;
  std::map<std::string, DescriptorMeta> descriptors_;
  std::map<uintptr_t, std::string> notify_characteristics_by_handle_;
  std::map<uintptr_t, PendingGattRequest> pending_gatt_requests_;
  std::map<uintptr_t, PendingMtuRequest> pending_mtu_requests_;
  std::map<std::string, Ecore_Timer*> discover_service_timers_;
};

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_BLUETOOTH_MANAGER_H_
