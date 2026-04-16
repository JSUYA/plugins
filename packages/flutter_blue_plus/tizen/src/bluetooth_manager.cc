// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth_manager.h"

#include <dlog.h>

#include <algorithm>
#include <cstdlib>
#include <utility>

#include "bt_constants.h"
#include "bt_utils.h"
#include "payload_builders.h"

namespace flutter_blue_plus_tizen {

namespace {

struct GattRequestCallbackContext {
  BluetoothManager* manager = nullptr;
  PendingGattRequest info;
};

struct DiscoverServicesContext {
  BluetoothManager* manager = nullptr;
  std::string remote_id;
  int remaining_attempts = 0;
};

// Drops every entry in `target` that belongs to `remote_id`, then inserts the
// new entries from `incoming`. Used to refresh the per-device cache on each
// discoverServices() round.
template <typename MapType>
void ReplaceForRemoteId(MapType& target, const MapType& incoming,
                        const std::string& remote_id) {
  for (auto it = target.begin(); it != target.end();) {
    if (it->second.remote_id == remote_id) {
      it = target.erase(it);
    } else {
      ++it;
    }
  }
  for (const auto& entry : incoming) {
    target[entry.first] = entry.second;
  }
}

template <typename InvokeFn>
std::vector<bt_gatt_h> CollectGattHandles(InvokeFn&& invoke,
                                          std::string* error_message) {
  struct Context {
    std::vector<bt_gatt_h> handles;
  } context;

  const int bt_result = invoke(
      [](int /*total*/, int /*index*/, bt_gatt_h handle,
         void* user_data) -> bool {
        static_cast<Context*>(user_data)->handles.push_back(handle);
        return true;
      },
      &context);

  if (bt_result != BT_ERROR_NONE && bt_result != BT_ERROR_NO_DATA) {
    if (error_message != nullptr) {
      *error_message = DescribeBtError(bt_result);
    }
    return {};
  }
  return context.handles;
}

bool MatchesMaskedBytes(const std::vector<uint8_t>& actual,
                        const std::vector<uint8_t>& expected,
                        const std::vector<uint8_t>& mask) {
  if (expected.empty()) {
    return true;
  }
  if (actual.size() < expected.size()) {
    return false;
  }
  if (!mask.empty() && mask.size() < expected.size()) {
    return false;
  }
  for (size_t i = 0; i < expected.size(); ++i) {
    const uint8_t expected_mask = mask.empty() ? 0xFF : mask[i];
    if ((actual[i] & expected_mask) != (expected[i] & expected_mask)) {
      return false;
    }
  }
  return true;
}

}  // namespace

BluetoothManager::BluetoothManager() : adapter_state_(kBmAdapterUnknown) {
  ecore_init();
}

BluetoothManager::~BluetoothManager() {
  for (auto& entry : discover_service_timers_) {
    // ecore_timer_del returns the user_data pointer; reclaim the context to
    // avoid leaking it when the timer is cancelled before firing.
    auto* context =
        static_cast<DiscoverServicesContext*>(ecore_timer_del(entry.second));
    delete context;
  }
  discover_service_timers_.clear();
  if (scanning_) {
    bt_adapter_le_stop_scan();
  }
  CleanupBluetooth();
  ecore_shutdown();
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

void BluetoothManager::HandleMethodCall(
    const FlMethodCall& method_call, std::unique_ptr<FlMethodResult> result) {
  const auto* arguments = std::get_if<EncodableMap>(method_call.arguments());
  const std::string& name = method_call.method_name();

  if (name == "getAdapterName") {
    HandleGetAdapterName(std::move(result));
  } else if (name == "getAdapterState") {
    HandleGetAdapterState(std::move(result));
  } else if (name == "getBondState") {
    HandleGetBondState(arguments, std::move(result));
  } else if (name == "getBondedDevices") {
    HandleGetBondedDevices(std::move(result));
  } else if (name == "getSystemDevices") {
    HandleGetSystemDevices(arguments, std::move(result));
  } else if (name == "getPhySupport") {
    result->Success(EncodableValue(MakePhySupportMap()));
  } else if (name == "isSupported") {
    EnsureInitialized();
    result->Success(EncodableValue(supported_));
  } else if (name == "setLogLevel") {
    HandleSetLogLevel(arguments, std::move(result));
  } else if (name == "setOptions") {
    result->Success(EncodableValue(true));
  } else if (name == "setPreferredPhy") {
    result->Success(EncodableValue(false));
  } else if (name == "connect") {
    HandleConnect(arguments, std::move(result));
  } else if (name == "disconnect") {
    HandleDisconnect(arguments, std::move(result));
  } else if (name == "discoverServices") {
    HandleDiscoverServices(arguments, std::move(result));
  } else if (name == "readCharacteristic") {
    HandleReadCharacteristic(arguments, std::move(result));
  } else if (name == "writeCharacteristic") {
    HandleWriteCharacteristic(arguments, std::move(result));
  } else if (name == "readDescriptor") {
    HandleReadDescriptor(arguments, std::move(result));
  } else if (name == "writeDescriptor") {
    HandleWriteDescriptor(arguments, std::move(result));
  } else if (name == "setNotifyValue") {
    HandleSetNotifyValue(arguments, std::move(result));
  } else if (name == "startScan") {
    HandleStartScan(arguments, std::move(result));
  } else if (name == "stopScan") {
    HandleStopScan(std::move(result));
  } else if (name == "readRssi") {
    HandleReadRssi(arguments, std::move(result));
  } else if (name == "requestMtu") {
    HandleRequestMtu(arguments, std::move(result));
  } else if (name == "requestConnectionPriority") {
    HandleRequestConnectionPriority(arguments, std::move(result));
  } else if (name == "createBond") {
    HandleCreateBond(arguments, std::move(result));
  } else if (name == "removeBond") {
    HandleRemoveBond(arguments, std::move(result));
  } else if (name == "clearGattCache") {
    result->Error("unsupported",
                  "clearGattCache() is not supported on Tizen 6.5.");
  } else if (name == "turnOn") {
    HandleTurnOn(std::move(result));
  } else if (name == "turnOff") {
    HandleTurnOff(std::move(result));
  } else {
    result->NotImplemented();
  }
}

// ---------------------------------------------------------------------------
// Adapter lifecycle
// ---------------------------------------------------------------------------

void BluetoothManager::EnsureInitialized() {
  if (initialized_) {
    return;
  }
  initialized_ = true;

  const int init_result = bt_initialize();
  if (init_result != BT_ERROR_NONE && init_result != BT_ERROR_ALREADY_DONE) {
    supported_ = false;
    unsupported_reason_ = DescribeBtError(init_result);
    adapter_state_ = kBmAdapterUnavailable;
    return;
  }

  if (!RegisterGlobalCallbacks()) {
    BestEffortCleanupGlobalCallbacks(/*deinitialize=*/true);
    supported_ = false;
    if (unsupported_reason_.empty()) {
      unsupported_reason_ = "Failed to register Bluetooth callbacks.";
    }
    adapter_state_ = kBmAdapterUnavailable;
    return;
  }
  adapter_state_ = ReadAdapterState();
}

bool BluetoothManager::RegisterGlobalCallbacks() {
  // Each callback is registered individually so the unsupported_reason_ field
  // identifies which one failed.
  int rc = bt_adapter_set_state_changed_cb(OnAdapterStateChangedCb, this);
  if (rc != BT_ERROR_NONE) {
    unsupported_reason_ =
        "bt_adapter_set_state_changed_cb failed: " + DescribeBtError(rc);
    return false;
  }
  rc = bt_adapter_set_name_changed_cb(OnAdapterNameChangedCb, this);
  if (rc != BT_ERROR_NONE) {
    unsupported_reason_ =
        "bt_adapter_set_name_changed_cb failed: " + DescribeBtError(rc);
    return false;
  }
  rc =
      bt_gatt_set_connection_state_changed_cb(OnConnectionStateChangedCb, this);
  if (rc != BT_ERROR_NONE) {
    unsupported_reason_ = "bt_gatt_set_connection_state_changed_cb failed: " +
                          DescribeBtError(rc);
    return false;
  }
  rc = bt_device_set_bond_created_cb(OnBondCreatedCb, this);
  if (rc != BT_ERROR_NONE) {
    unsupported_reason_ =
        "bt_device_set_bond_created_cb failed: " + DescribeBtError(rc);
    return false;
  }
  rc = bt_device_set_bond_destroyed_cb(OnBondDestroyedCb, this);
  if (rc != BT_ERROR_NONE) {
    unsupported_reason_ =
        "bt_device_set_bond_destroyed_cb failed: " + DescribeBtError(rc);
    return false;
  }
  return true;
}

void BluetoothManager::BestEffortCleanupGlobalCallbacks(bool deinitialize) {
  bt_device_unset_bond_destroyed_cb();
  bt_device_unset_bond_created_cb();
  bt_gatt_unset_connection_state_changed_cb();
  bt_adapter_unset_name_changed_cb();
  bt_adapter_unset_state_changed_cb();
  if (deinitialize) {
    bt_deinitialize();
  }
}

void BluetoothManager::CleanupBluetooth() {
  if (!initialized_) {
    return;
  }
  BestEffortCleanupGlobalCallbacks(/*deinitialize=*/false);
  std::vector<std::string> remote_ids;
  remote_ids.reserve(clients_by_remote_id_.size());
  for (const auto& entry : clients_by_remote_id_) {
    remote_ids.push_back(entry.first);
  }
  for (const auto& remote_id : remote_ids) {
    DestroyClient(remote_id);
  }
  bt_deinitialize();
}

int BluetoothManager::ReadAdapterState() {
  bt_adapter_state_e adapter_state = BT_ADAPTER_DISABLED;
  const int rc = bt_adapter_get_state(&adapter_state);
  if (rc != BT_ERROR_NONE) {
    return rc == BT_ERROR_NOT_SUPPORTED ? kBmAdapterUnavailable
                                        : kBmAdapterUnknown;
  }
  return adapter_state == BT_ADAPTER_ENABLED ? kBmAdapterOn : kBmAdapterOff;
}

// ---------------------------------------------------------------------------
// Method handlers
// ---------------------------------------------------------------------------

void BluetoothManager::HandleGetAdapterName(
    std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Success(EncodableValue(std::string()));
    return;
  }

  char* name = nullptr;
  const int rc = bt_adapter_get_name(&name);
  if (rc == BT_ERROR_NONE && name != nullptr) {
    adapter_name_ = name;
  }
  if (name != nullptr) {
    free(name);
  }
  result->Success(EncodableValue(adapter_name_));
}

void BluetoothManager::HandleGetAdapterState(
    std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Success(EncodableValue(MakeAdapterStateMap(kBmAdapterUnavailable)));
    return;
  }
  adapter_state_ = ReadAdapterState();
  result->Success(EncodableValue(MakeAdapterStateMap(adapter_state_)));
}

void BluetoothManager::HandleSetLogLevel(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  int log_level = 0;
  GetIntLike(arguments, "log_level", log_level);
  events_.SetLogLevel(log_level);
  result->Success(EncodableValue(true));
}

void BluetoothManager::HandleConnect(const EncodableMap* arguments,
                                     std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Error("unsupported", unsupported_reason_);
    return;
  }

  std::string remote_id;
  bool auto_connect = false;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id) ||
      !GetBoolLike(arguments, "auto_connect", auto_connect)) {
    result->Error("invalid_argument",
                  "connect requires remote_id and auto_connect.");
    return;
  }

  remote_id = NormalizeRemoteId(remote_id);
  if (connected_remote_ids_.count(remote_id) ||
      pending_connects_.count(remote_id)) {
    result->Success(EncodableValue(false));
    return;
  }

  const bool had_client = clients_by_remote_id_.count(remote_id) > 0;
  std::string error_message;
  if (!EnsureClient(remote_id, error_message)) {
    result->Error(std::to_string(BT_ERROR_OPERATION_FAILED), error_message);
    return;
  }

  if (auto_connect) {
    auto_connect_remote_ids_.insert(remote_id);
  } else {
    auto_connect_remote_ids_.erase(remote_id);
  }
  pending_connects_.insert(remote_id);

  const int rc = bt_gatt_connect(remote_id.c_str(), auto_connect);
  if (rc == BT_ERROR_NONE) {
    events_.LogVerbose("connect(" + remote_id + ") started");
    result->Success(EncodableValue(true));
    return;
  }

  pending_connects_.erase(remote_id);
  auto_connect_remote_ids_.erase(remote_id);
  if (!had_client) {
    DestroyClient(remote_id);
  }

  if (auto_connect) {
    result->Error(std::to_string(rc), DescribeBtError(rc));
    return;
  }
  // Surface the failure as a synthetic disconnected event so the Dart side
  // resolves its connectionState future, matching the Android FBP behavior.
  EmitConnectionState(remote_id, kBmConnectionDisconnected, rc,
                      DescribeBtError(rc));
  result->Success(EncodableValue(true));
}

void BluetoothManager::HandleDisconnect(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Error("unsupported", unsupported_reason_);
    return;
  }

  std::string remote_id;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id)) {
    result->Error("invalid_argument", "disconnect requires remote_id.");
    return;
  }
  remote_id = NormalizeRemoteId(remote_id);
  auto_connect_remote_ids_.erase(remote_id);
  if (!connected_remote_ids_.count(remote_id) &&
      !pending_connects_.count(remote_id)) {
    result->Success(EncodableValue(false));
    return;
  }

  const int rc = bt_gatt_disconnect(remote_id.c_str());
  if (rc != BT_ERROR_NONE) {
    result->Error(std::to_string(rc), DescribeBtError(rc));
    return;
  }
  events_.LogVerbose("disconnect(" + remote_id + ") started");
  result->Success(EncodableValue(true));
}

void BluetoothManager::HandleDiscoverServices(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  std::string remote_id;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id)) {
    result->Error("invalid_argument", "discoverServices requires remote_id.");
    return;
  }
  remote_id = NormalizeRemoteId(remote_id);

  if (!supported_) {
    EmitDiscoverServicesFailure(remote_id, BT_ERROR_NOT_SUPPORTED,
                                unsupported_reason_);
    result->Success(EncodableValue(false));
    return;
  }
  if (!connected_remote_ids_.count(remote_id)) {
    EmitDiscoverServicesFailure(remote_id, BT_ERROR_REMOTE_DEVICE_NOT_CONNECTED,
                                "Device " + remote_id + " is not connected.");
    result->Success(EncodableValue(false));
    return;
  }

  std::string error_message;
  if (!EnsureClient(remote_id, error_message)) {
    EmitDiscoverServicesFailure(remote_id, BT_ERROR_OPERATION_FAILED,
                                error_message);
    result->Success(EncodableValue(false));
    return;
  }

  auto existing = discover_service_timers_.find(remote_id);
  if (existing != discover_service_timers_.end()) {
    auto* old_context = static_cast<DiscoverServicesContext*>(
        ecore_timer_del(existing->second));
    delete old_context;
    discover_service_timers_.erase(existing);
  }

  // Tizen exposes the GATT database via a synchronous cache that is populated
  // asynchronously after connection. Poll it every 100 ms for up to ~3 s.
  auto* context = new DiscoverServicesContext{this, remote_id, 30};
  Ecore_Timer* timer = ecore_timer_add(0.1, OnDiscoverServicesTimerCb, context);
  discover_service_timers_[remote_id] = timer;
  result->Success(EncodableValue(true));
}

void BluetoothManager::HandleReadCharacteristic(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  PendingGattRequest info;
  if (!BuildCharacteristicRequest(arguments,
                                  PendingGattKind::kReadCharacteristic, info)) {
    result->Error("invalid_argument",
                  "readCharacteristic requires characteristic arguments.");
    return;
  }

  if (!supported_) {
    EmitCharacteristicData(info.remote_id, info.service_uuid,
                           info.characteristic_uuid, info.instance_id, {},
                           false, BT_ERROR_NOT_SUPPORTED, unsupported_reason_,
                           false);
    result->Success(EncodableValue(false));
    return;
  }

  auto characteristic_it = characteristics_.find(
      MakeCharacteristicKey(info.remote_id, info.service_uuid,
                            info.characteristic_uuid, info.instance_id));
  if (characteristic_it == characteristics_.end()) {
    EmitCharacteristicData(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, {}, false, BT_ERROR_NO_DATA,
        "Characteristic not found. Call discoverServices() first.", false);
    result->Success(EncodableValue(false));
    return;
  }

  info.handle_key =
      reinterpret_cast<uintptr_t>(characteristic_it->second.handle);
  if (pending_gatt_requests_.count(info.handle_key)) {
    EmitCharacteristicData(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, {}, false, BT_ERROR_NOW_IN_PROGRESS,
        "Another GATT operation is already in progress for this handle.",
        false);
    result->Success(EncodableValue(false));
    return;
  }

  auto* context = new GattRequestCallbackContext{this, info};
  pending_gatt_requests_[info.handle_key] = info;
  const int rc = bt_gatt_client_read_value(characteristic_it->second.handle,
                                           OnGattRequestCompletedCb, context);
  if (rc == BT_ERROR_NONE) {
    result->Success(EncodableValue(true));
    return;
  }

  pending_gatt_requests_.erase(info.handle_key);
  delete context;
  EmitCharacteristicData(info.remote_id, info.service_uuid,
                         info.characteristic_uuid, info.instance_id, {}, false,
                         rc, DescribeBtError(rc), false);
  result->Success(EncodableValue(false));
}

void BluetoothManager::HandleWriteCharacteristic(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  PendingGattRequest info;
  if (!BuildCharacteristicRequest(
          arguments, PendingGattKind::kWriteCharacteristic, info)) {
    result->Error("invalid_argument",
                  "writeCharacteristic requires characteristic arguments.");
    return;
  }
  info.value = GetBytesOrEmpty(arguments, "value");

  if (!supported_) {
    EmitCharacteristicData(info.remote_id, info.service_uuid,
                           info.characteristic_uuid, info.instance_id,
                           info.value, false, BT_ERROR_NOT_SUPPORTED,
                           unsupported_reason_, true);
    result->Success(EncodableValue(false));
    return;
  }

  auto characteristic_it = characteristics_.find(
      MakeCharacteristicKey(info.remote_id, info.service_uuid,
                            info.characteristic_uuid, info.instance_id));
  if (characteristic_it == characteristics_.end()) {
    EmitCharacteristicData(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, info.value, false, BT_ERROR_NO_DATA,
        "Characteristic not found. Call discoverServices() first.", true);
    result->Success(EncodableValue(false));
    return;
  }

  int write_type = 0;
  GetIntLike(arguments, "write_type", write_type);
  const bt_gatt_write_type_e native_write_type =
      write_type == 1 ? BT_GATT_WRITE_TYPE_WRITE_NO_RESPONSE
                      : BT_GATT_WRITE_TYPE_WRITE;
  int rc = bt_gatt_characteristic_set_write_type(
      characteristic_it->second.handle, native_write_type);
  if (rc != BT_ERROR_NONE) {
    EmitCharacteristicData(info.remote_id, info.service_uuid,
                           info.characteristic_uuid, info.instance_id,
                           info.value, false, rc, DescribeBtError(rc), true);
    result->Success(EncodableValue(false));
    return;
  }

  rc = SetGattValue(characteristic_it->second.handle, info.value);
  if (rc != BT_ERROR_NONE) {
    EmitCharacteristicData(info.remote_id, info.service_uuid,
                           info.characteristic_uuid, info.instance_id,
                           info.value, false, rc, DescribeBtError(rc), true);
    result->Success(EncodableValue(false));
    return;
  }

  info.handle_key =
      reinterpret_cast<uintptr_t>(characteristic_it->second.handle);
  if (pending_gatt_requests_.count(info.handle_key)) {
    EmitCharacteristicData(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, info.value, false, BT_ERROR_NOW_IN_PROGRESS,
        "Another GATT operation is already in progress for this handle.", true);
    result->Success(EncodableValue(false));
    return;
  }

  auto* context = new GattRequestCallbackContext{this, info};
  pending_gatt_requests_[info.handle_key] = info;
  rc = bt_gatt_client_write_value(characteristic_it->second.handle,
                                  OnGattRequestCompletedCb, context);
  if (rc == BT_ERROR_NONE) {
    result->Success(EncodableValue(true));
    return;
  }

  pending_gatt_requests_.erase(info.handle_key);
  delete context;
  EmitCharacteristicData(info.remote_id, info.service_uuid,
                         info.characteristic_uuid, info.instance_id, info.value,
                         false, rc, DescribeBtError(rc), true);
  result->Success(EncodableValue(false));
}

void BluetoothManager::HandleReadDescriptor(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  PendingGattRequest info;
  if (!BuildDescriptorRequest(arguments, PendingGattKind::kReadDescriptor,
                              info)) {
    result->Error("invalid_argument",
                  "readDescriptor requires descriptor arguments.");
    return;
  }

  if (!supported_) {
    EmitDescriptorData(info.remote_id, info.service_uuid,
                       info.characteristic_uuid, info.instance_id,
                       info.descriptor_uuid, {}, false, BT_ERROR_NOT_SUPPORTED,
                       unsupported_reason_, false);
    result->Success(EncodableValue(false));
    return;
  }

  auto descriptor_it = descriptors_.find(MakeDescriptorKey(
      info.remote_id, info.service_uuid, info.characteristic_uuid,
      info.instance_id, info.descriptor_uuid));
  if (descriptor_it == descriptors_.end()) {
    EmitDescriptorData(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, info.descriptor_uuid, {}, false, BT_ERROR_NO_DATA,
        "Descriptor not found. Call discoverServices() first.", false);
    result->Success(EncodableValue(false));
    return;
  }

  info.handle_key = reinterpret_cast<uintptr_t>(descriptor_it->second.handle);
  if (pending_gatt_requests_.count(info.handle_key)) {
    EmitDescriptorData(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, info.descriptor_uuid, {}, false,
        BT_ERROR_NOW_IN_PROGRESS,
        "Another GATT operation is already in progress for this handle.",
        false);
    result->Success(EncodableValue(false));
    return;
  }

  auto* context = new GattRequestCallbackContext{this, info};
  pending_gatt_requests_[info.handle_key] = info;
  const int rc = bt_gatt_client_read_value(descriptor_it->second.handle,
                                           OnGattRequestCompletedCb, context);
  if (rc == BT_ERROR_NONE) {
    result->Success(EncodableValue(true));
    return;
  }

  pending_gatt_requests_.erase(info.handle_key);
  delete context;
  EmitDescriptorData(info.remote_id, info.service_uuid,
                     info.characteristic_uuid, info.instance_id,
                     info.descriptor_uuid, {}, false, rc, DescribeBtError(rc),
                     false);
  result->Success(EncodableValue(false));
}

void BluetoothManager::HandleWriteDescriptor(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  PendingGattRequest info;
  if (!BuildDescriptorRequest(arguments, PendingGattKind::kWriteDescriptor,
                              info)) {
    result->Error("invalid_argument",
                  "writeDescriptor requires descriptor arguments.");
    return;
  }
  info.value = GetBytesOrEmpty(arguments, "value");

  if (!supported_) {
    EmitDescriptorData(info.remote_id, info.service_uuid,
                       info.characteristic_uuid, info.instance_id,
                       info.descriptor_uuid, info.value, false,
                       BT_ERROR_NOT_SUPPORTED, unsupported_reason_, true);
    result->Success(EncodableValue(false));
    return;
  }

  auto descriptor_it = descriptors_.find(MakeDescriptorKey(
      info.remote_id, info.service_uuid, info.characteristic_uuid,
      info.instance_id, info.descriptor_uuid));
  if (descriptor_it == descriptors_.end()) {
    EmitDescriptorData(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, info.descriptor_uuid, info.value, false,
        BT_ERROR_NO_DATA,
        "Descriptor not found. Call discoverServices() first.", true);
    result->Success(EncodableValue(false));
    return;
  }

  int rc = SetGattValue(descriptor_it->second.handle, info.value);
  if (rc != BT_ERROR_NONE) {
    EmitDescriptorData(info.remote_id, info.service_uuid,
                       info.characteristic_uuid, info.instance_id,
                       info.descriptor_uuid, info.value, false, rc,
                       DescribeBtError(rc), true);
    result->Success(EncodableValue(false));
    return;
  }

  info.handle_key = reinterpret_cast<uintptr_t>(descriptor_it->second.handle);
  if (pending_gatt_requests_.count(info.handle_key)) {
    EmitDescriptorData(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, info.descriptor_uuid, info.value, false,
        BT_ERROR_NOW_IN_PROGRESS,
        "Another GATT operation is already in progress for this handle.", true);
    result->Success(EncodableValue(false));
    return;
  }

  auto* context = new GattRequestCallbackContext{this, info};
  pending_gatt_requests_[info.handle_key] = info;
  rc = bt_gatt_client_write_value(descriptor_it->second.handle,
                                  OnGattRequestCompletedCb, context);
  if (rc == BT_ERROR_NONE) {
    result->Success(EncodableValue(true));
    return;
  }

  pending_gatt_requests_.erase(info.handle_key);
  delete context;
  EmitDescriptorData(info.remote_id, info.service_uuid,
                     info.characteristic_uuid, info.instance_id,
                     info.descriptor_uuid, info.value, false, rc,
                     DescribeBtError(rc), true);
  result->Success(EncodableValue(false));
}

void BluetoothManager::HandleSetNotifyValue(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();

  PendingGattRequest info;
  if (!BuildCharacteristicRequest(arguments, PendingGattKind::kWriteDescriptor,
                                  info)) {
    result->Error("invalid_argument",
                  "setNotifyValue requires characteristic arguments.");
    return;
  }

  if (!supported_) {
    EmitCccdWrite(info.remote_id, info.service_uuid, info.characteristic_uuid,
                  info.instance_id, {}, false, BT_ERROR_NOT_SUPPORTED,
                  unsupported_reason_);
    result->Success(EncodableValue(true));
    return;
  }

  auto characteristic_it = characteristics_.find(
      MakeCharacteristicKey(info.remote_id, info.service_uuid,
                            info.characteristic_uuid, info.instance_id));
  if (characteristic_it == characteristics_.end()) {
    EmitCccdWrite(info.remote_id, info.service_uuid, info.characteristic_uuid,
                  info.instance_id, {}, false, BT_ERROR_NO_DATA,
                  "Characteristic not found. Call discoverServices() first.");
    result->Success(EncodableValue(true));
    return;
  }

  bool enable = false;
  bool force_indications = false;
  GetBoolLike(arguments, "enable", enable);
  GetBoolLike(arguments, "force_indications", force_indications);

  const auto& properties = characteristic_it->second.properties;
  if (!properties.notify && !properties.indicate) {
    EmitCccdWrite(
        info.remote_id, info.service_uuid, info.characteristic_uuid,
        info.instance_id, {}, false, BT_ERROR_NOT_SUPPORTED,
        "Characteristic does not support notifications or indications.");
    result->Success(EncodableValue(true));
    return;
  }

  const bool use_indications = force_indications && properties.indicate;
  const uintptr_t handle_key =
      reinterpret_cast<uintptr_t>(characteristic_it->second.handle);

  int rc = BT_ERROR_NONE;
  if (enable) {
    notify_characteristics_by_handle_[handle_key] =
        MakeCharacteristicKey(info.remote_id, info.service_uuid,
                              info.characteristic_uuid, info.instance_id);
    rc = bt_gatt_client_set_characteristic_value_changed_cb(
        characteristic_it->second.handle, OnCharacteristicValueChangedCb, this);
  } else {
    notify_characteristics_by_handle_.erase(handle_key);
    rc = bt_gatt_client_unset_characteristic_value_changed_cb(
        characteristic_it->second.handle);
  }

  // The Dart side identifies a notify/indicate enable by the CCCD bytes:
  //   {0x01,0x00} = notifications, {0x02,0x00} = indications, {0x00,0x00}=off.
  // Tizen's bt_gatt_client_set_characteristic_value_changed_cb writes the
  // CCCD internally, so we just synthesize the value the Dart side expects.
  std::vector<uint8_t> cccd_value;
  if (enable) {
    cccd_value = use_indications ? std::vector<uint8_t>{0x02, 0x00}
                                 : std::vector<uint8_t>{0x01, 0x00};
  } else {
    cccd_value = {0x00, 0x00};
  }

  const bool success = rc == BT_ERROR_NONE || rc == BT_ERROR_ALREADY_DONE;
  EmitCccdWrite(info.remote_id, info.service_uuid, info.characteristic_uuid,
                info.instance_id, cccd_value, success, success ? 0 : rc,
                success ? "" : DescribeBtError(rc));
  result->Success(EncodableValue(true));
}

void BluetoothManager::HandleStartScan(const EncodableMap* arguments,
                                       std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    EmitScanFailure(BT_ERROR_NOT_SUPPORTED, unsupported_reason_);
    dlog_print(DLOG_ERROR, kLogTag, "startScan() rejected: %s",
               unsupported_reason_.c_str());
    result->Error("unsupported", unsupported_reason_);
    return;
  }
  if (scanning_) {
    result->Success(EncodableValue(false));
    return;
  }

  active_scan_settings_ =
      std::make_unique<ScanSettingsData>(ParseScanSettings(arguments));
  scan_event_counts_.clear();

  const int rc = bt_adapter_le_start_scan(OnScanResultCb, this);
  if (rc != BT_ERROR_NONE) {
    active_scan_settings_.reset();
    EmitScanFailure(rc, DescribeBtError(rc));
    dlog_print(DLOG_ERROR, kLogTag, "bt_adapter_le_start_scan failed: %s",
               DescribeBtError(rc).c_str());
    result->Error(std::to_string(rc), DescribeBtError(rc));
    return;
  }
  scanning_ = true;
  events_.LogVerbose("startScan() started");
  result->Success(EncodableValue(true));
}

void BluetoothManager::HandleStopScan(std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!scanning_) {
    result->Success(EncodableValue(false));
    return;
  }

  const int rc = bt_adapter_le_stop_scan();
  const bool stopped = rc == BT_ERROR_NONE || rc == BT_ERROR_NOT_IN_PROGRESS;
  if (stopped) {
    scanning_ = false;
    active_scan_settings_.reset();
  }
  result->Success(EncodableValue(stopped));
}

void BluetoothManager::HandleReadRssi(const EncodableMap* arguments,
                                      std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  std::string remote_id;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id)) {
    result->Error("invalid_argument", "readRssi requires remote_id.");
    return;
  }
  remote_id = NormalizeRemoteId(remote_id);

  auto rssi_it = last_scan_rssi_.find(remote_id);
  if (rssi_it != last_scan_rssi_.end()) {
    EmitReadRssi(remote_id, rssi_it->second, true, 0, "");
    result->Success(EncodableValue(true));
    return;
  }

  EmitReadRssi(remote_id, 0, false, BT_ERROR_NO_DATA,
               "No recent scan RSSI is available for " + remote_id + ".");
  result->Success(EncodableValue(false));
}

void BluetoothManager::HandleRequestMtu(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  std::string remote_id;
  int mtu = 0;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id) ||
      !GetIntLike(arguments, "mtu", mtu)) {
    result->Error("invalid_argument", "requestMtu requires remote_id and mtu.");
    return;
  }
  remote_id = NormalizeRemoteId(remote_id);

  if (!supported_) {
    EmitMtuChanged(remote_id, static_cast<unsigned int>(mtu), false,
                   BT_ERROR_NOT_SUPPORTED, unsupported_reason_);
    result->Success(EncodableValue(false));
    return;
  }
  if (!connected_remote_ids_.count(remote_id)) {
    EmitMtuChanged(remote_id, static_cast<unsigned int>(mtu), false,
                   BT_ERROR_REMOTE_DEVICE_NOT_CONNECTED,
                   "Device " + remote_id + " is not connected.");
    result->Success(EncodableValue(false));
    return;
  }

  std::string error_message;
  if (!EnsureClient(remote_id, error_message)) {
    EmitMtuChanged(remote_id, static_cast<unsigned int>(mtu), false,
                   BT_ERROR_OPERATION_FAILED, error_message);
    result->Success(EncodableValue(false));
    return;
  }

  bt_gatt_client_h client = clients_by_remote_id_[remote_id];
  const uintptr_t client_key = reinterpret_cast<uintptr_t>(client);
  if (pending_mtu_requests_.count(client_key)) {
    EmitMtuChanged(remote_id, static_cast<unsigned int>(mtu), false,
                   BT_ERROR_NOW_IN_PROGRESS,
                   "Another MTU request is already in progress.");
    result->Success(EncodableValue(false));
    return;
  }

  pending_mtu_requests_[client_key] =
      PendingMtuRequest{remote_id, static_cast<unsigned int>(mtu)};
  const int rc = bt_gatt_client_request_att_mtu_change(
      client, static_cast<unsigned int>(mtu));
  if (rc == BT_ERROR_NONE) {
    result->Success(EncodableValue(true));
    return;
  }

  pending_mtu_requests_.erase(client_key);
  EmitMtuChanged(remote_id, static_cast<unsigned int>(mtu), false, rc,
                 DescribeBtError(rc));
  result->Success(EncodableValue(false));
}

void BluetoothManager::HandleRequestConnectionPriority(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Success(EncodableValue(false));
    return;
  }

  std::string remote_id;
  int connection_priority = 0;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id) ||
      !GetIntLike(arguments, "connection_priority", connection_priority)) {
    result->Error("invalid_argument",
                  "requestConnectionPriority requires remote_id and "
                  "connection_priority.");
    return;
  }
  remote_id = NormalizeRemoteId(remote_id);

  bt_device_le_connection_mode_e mode = BT_DEVICE_LE_CONNECTION_MODE_BALANCED;
  if (connection_priority == 1) {
    mode = BT_DEVICE_LE_CONNECTION_MODE_LOW_LATENCY;
  } else if (connection_priority == 2) {
    mode = BT_DEVICE_LE_CONNECTION_MODE_LOW_ENERGY;
  }
  const int rc = bt_device_update_le_connection_mode(remote_id.c_str(), mode);
  result->Success(EncodableValue(rc == BT_ERROR_NONE));
}

void BluetoothManager::HandleCreateBond(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Error("unsupported", unsupported_reason_);
    return;
  }

  std::string remote_id;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id)) {
    result->Error("invalid_argument", "createBond requires remote_id.");
    return;
  }
  remote_id = NormalizeRemoteId(remote_id);
  if (!connected_remote_ids_.count(remote_id)) {
    result->Error("invalid_state",
                  "Device " + remote_id + " is not connected.");
    return;
  }

  const int current_bond_state = GetBondStateValue(remote_id);
  if (current_bond_state == kBmBondBonding) {
    result->Success(EncodableValue(true));
    return;
  }
  if (current_bond_state == kBmBondBonded) {
    result->Success(EncodableValue(false));
    return;
  }

  pending_bond_states_[remote_id] = kBmBondBonding;
  EmitBondState(remote_id, kBmBondBonding, current_bond_state);

  const int rc = bt_device_create_bond(remote_id.c_str());
  if (rc == BT_ERROR_NONE) {
    result->Success(EncodableValue(true));
    return;
  }
  if (rc == BT_ERROR_ALREADY_DONE) {
    pending_bond_states_.erase(remote_id);
    EmitBondState(remote_id, kBmBondBonded, kBmBondBonding);
    result->Success(EncodableValue(false));
    return;
  }

  pending_bond_states_.erase(remote_id);
  EmitBondState(remote_id, kBmBondNone, kBmBondBonding);
  result->Error(std::to_string(rc), DescribeBtError(rc));
}

void BluetoothManager::HandleRemoveBond(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Error("unsupported", unsupported_reason_);
    return;
  }

  std::string remote_id;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id)) {
    result->Error("invalid_argument", "removeBond requires remote_id.");
    return;
  }
  remote_id = NormalizeRemoteId(remote_id);
  const int current_bond_state = GetBondStateValue(remote_id);
  if (current_bond_state == kBmBondNone) {
    result->Success(EncodableValue(false));
    return;
  }

  const int rc = bt_device_destroy_bond(remote_id.c_str());
  if (rc == BT_ERROR_NONE) {
    result->Success(EncodableValue(true));
    return;
  }
  EmitBondState(remote_id, current_bond_state, -1);
  result->Error(std::to_string(rc), DescribeBtError(rc));
}

void BluetoothManager::HandleGetBondState(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  std::string remote_id;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id)) {
    result->Error("invalid_argument", "getBondState requires remote_id.");
    return;
  }
  remote_id = NormalizeRemoteId(remote_id);

  auto pending_it = pending_bond_states_.find(remote_id);
  if (pending_it != pending_bond_states_.end()) {
    result->Success(EncodableValue(MakeBondStateMap(
        remote_id, pending_it->second,
        pending_it->second == kBmBondBonding ? kBmBondNone : -1)));
    return;
  }
  const int bond_state = GetBondStateValue(remote_id);
  result->Success(EncodableValue(MakeBondStateMap(remote_id, bond_state, -1)));
}

void BluetoothManager::HandleGetBondedDevices(
    std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Success(MakeDevicesListValue({}));
    return;
  }

  EncodableList encoded_devices;
  for (const auto& device : EnumerateBondedDevices()) {
    encoded_devices.push_back(
        EncodableValue(MakeBluetoothDeviceMap(device.remote_id, device.name)));
  }
  result->Success(MakeDevicesListValue(encoded_devices));
}

void BluetoothManager::HandleGetSystemDevices(
    const EncodableMap* arguments, std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Success(MakeDevicesListValue({}));
    return;
  }

  std::set<std::string> requested_services;
  for (const auto& uuid : GetStringList(arguments, "with_services")) {
    requested_services.insert(NormalizeUuid(uuid));
  }

  auto service_matches = [&](const std::set<std::string>& uuids) {
    if (requested_services.empty()) {
      return true;
    }
    for (const auto& uuid : uuids) {
      if (requested_services.count(uuid)) {
        return true;
      }
    }
    return false;
  };

  std::map<std::string, EncodableMap> devices;
  for (const auto& info : EnumerateBondedDevices()) {
    if (!info.is_connected) {
      continue;
    }
    if (!service_matches(info.service_uuids)) {
      continue;
    }
    devices[info.remote_id] = MakeBluetoothDeviceMap(info.remote_id, info.name);
  }

  for (const auto& remote_id : connected_remote_ids_) {
    auto services_it = service_uuids_by_remote_id_.find(remote_id);
    const std::set<std::string>& uuids =
        services_it == service_uuids_by_remote_id_.end()
            ? std::set<std::string>()
            : services_it->second;
    if (!service_matches(uuids)) {
      continue;
    }
    if (!devices.count(remote_id)) {
      devices[remote_id] =
          MakeBluetoothDeviceMap(remote_id, device_names_[remote_id]);
    }
  }

  EncodableList encoded;
  for (const auto& entry : devices) {
    encoded.push_back(EncodableValue(entry.second));
  }
  result->Success(MakeDevicesListValue(encoded));
}

void BluetoothManager::HandleTurnOn(std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Error("unsupported", unsupported_reason_);
    return;
  }
  adapter_state_ = ReadAdapterState();
  if (adapter_state_ == kBmAdapterOn) {
    result->Success(EncodableValue(false));
    return;
  }
  result->Error("unsupported",
                "Turning Bluetooth on is not supported for third-party Tizen "
                "applications.");
}

void BluetoothManager::HandleTurnOff(std::unique_ptr<FlMethodResult> result) {
  EnsureInitialized();
  if (!supported_) {
    result->Error("unsupported", unsupported_reason_);
    return;
  }
  adapter_state_ = ReadAdapterState();
  if (adapter_state_ != kBmAdapterOn) {
    result->Success(EncodableValue(false));
    return;
  }
  result->Error("unsupported",
                "Turning Bluetooth off is not supported for third-party Tizen "
                "applications.");
}

// ---------------------------------------------------------------------------
// GATT client bookkeeping
// ---------------------------------------------------------------------------

bool BluetoothManager::EnsureClient(const std::string& remote_id,
                                    std::string& error_message) {
  auto existing = clients_by_remote_id_.find(remote_id);
  if (existing != clients_by_remote_id_.end() && existing->second != nullptr) {
    return true;
  }

  bt_gatt_client_h client = nullptr;
  const int rc = bt_gatt_client_create(remote_id.c_str(), &client);
  if (rc != BT_ERROR_NONE) {
    error_message = DescribeBtError(rc);
    return false;
  }

  clients_by_remote_id_[remote_id] = client;
  remote_id_by_client_handle_[reinterpret_cast<uintptr_t>(client)] = remote_id;
  RegisterClientCallbacks(client);
  return true;
}

void BluetoothManager::RegisterClientCallbacks(bt_gatt_client_h client) {
  const uintptr_t client_key = reinterpret_cast<uintptr_t>(client);
  int rc =
      bt_gatt_client_set_service_changed_cb(client, OnServiceChangedCb, this);
  if (rc != BT_ERROR_NONE && rc != BT_ERROR_NOT_SUPPORTED) {
    events_.LogVerbose("set_service_changed_cb failed: " + DescribeBtError(rc));
  }
  rc = bt_gatt_client_set_att_mtu_changed_cb(client, OnMtuChangedCb, this);
  if (rc != BT_ERROR_NONE && rc != BT_ERROR_NOT_SUPPORTED) {
    events_.LogVerbose("set_att_mtu_changed_cb failed: " + DescribeBtError(rc));
  }
  // Re-resolve the remote_id from the client to keep the index stable even if
  // the caller passed a mixed-case MAC address.
  remote_id_by_client_handle_[client_key] = GetRemoteIdFromClient(client);
}

std::string BluetoothManager::GetRemoteIdFromClient(bt_gatt_client_h client) {
  char* remote_address = nullptr;
  const int rc = bt_gatt_client_get_remote_address(client, &remote_address);
  std::string remote_id;
  if (rc == BT_ERROR_NONE && remote_address != nullptr) {
    remote_id = NormalizeRemoteId(remote_address);
  }
  if (remote_address != nullptr) {
    free(remote_address);
  }
  return remote_id;
}

void BluetoothManager::DestroyClient(const std::string& remote_id) {
  auto client_it = clients_by_remote_id_.find(remote_id);
  if (client_it == clients_by_remote_id_.end() ||
      client_it->second == nullptr) {
    ClearCachedGattDatabase(remote_id);
    return;
  }

  bt_gatt_client_h client = client_it->second;
  const uintptr_t client_key = reinterpret_cast<uintptr_t>(client);
  pending_mtu_requests_.erase(client_key);
  remote_id_by_client_handle_.erase(client_key);
  ClearCachedGattDatabase(remote_id);

  const int service_changed_rc =
      bt_gatt_client_unset_service_changed_cb(client);
  if (service_changed_rc != BT_ERROR_NONE &&
      service_changed_rc != BT_ERROR_NOT_SUPPORTED) {
    events_.LogVerbose("unset_service_changed_cb failed: " +
                       DescribeBtError(service_changed_rc));
  }

  const int mtu_rc = bt_gatt_client_unset_att_mtu_changed_cb(client);
  if (mtu_rc != BT_ERROR_NONE && mtu_rc != BT_ERROR_NOT_SUPPORTED) {
    events_.LogVerbose("unset_att_mtu_changed_cb failed: " +
                       DescribeBtError(mtu_rc));
  }

  const int destroy_rc = bt_gatt_client_destroy(client);
  if (destroy_rc != BT_ERROR_NONE && destroy_rc != BT_ERROR_NOT_SUPPORTED) {
    events_.LogVerbose("bt_gatt_client_destroy failed: " +
                       DescribeBtError(destroy_rc));
  }
  clients_by_remote_id_.erase(client_it);
}

void BluetoothManager::ClearCachedGattDatabase(const std::string& remote_id) {
  std::vector<uintptr_t> notify_handles;
  for (const auto& entry : notify_characteristics_by_handle_) {
    auto characteristic_it = characteristics_.find(entry.second);
    if (characteristic_it != characteristics_.end() &&
        characteristic_it->second.remote_id == remote_id) {
      notify_handles.push_back(entry.first);
    }
  }
  for (uintptr_t handle_key : notify_handles) {
    auto notify_it = notify_characteristics_by_handle_.find(handle_key);
    if (notify_it == notify_characteristics_by_handle_.end()) {
      continue;
    }
    auto characteristic_it = characteristics_.find(notify_it->second);
    if (characteristic_it != characteristics_.end()) {
      bt_gatt_client_unset_characteristic_value_changed_cb(
          characteristic_it->second.handle);
    }
    notify_characteristics_by_handle_.erase(notify_it);
  }

  for (auto it = characteristics_.begin(); it != characteristics_.end();) {
    if (it->second.remote_id == remote_id) {
      it = characteristics_.erase(it);
    } else {
      ++it;
    }
  }
  for (auto it = descriptors_.begin(); it != descriptors_.end();) {
    if (it->second.remote_id == remote_id) {
      it = descriptors_.erase(it);
    } else {
      ++it;
    }
  }
  service_uuids_by_remote_id_.erase(remote_id);
}

EncodableList BluetoothManager::BuildServices(const std::string& remote_id,
                                              bt_gatt_client_h client,
                                              std::string* error_message) {
  std::vector<bt_gatt_h> service_handles = CollectGattHandles(
      [client](bt_gatt_foreach_cb callback, void* user_data) {
        return bt_gatt_client_foreach_services(client, callback, user_data);
      },
      error_message);
  if (service_handles.empty() && error_message != nullptr &&
      !error_message->empty()) {
    return {};
  }

  std::map<std::string, CharacteristicMeta> next_characteristics;
  std::map<std::string, DescriptorMeta> next_descriptors;
  std::set<std::string> service_uuids;
  EncodableList encoded_services;

  for (bt_gatt_h service_handle : service_handles) {
    const std::string service_uuid = GetGattUuid(service_handle);
    if (service_uuid.empty()) {
      continue;
    }
    service_uuids.insert(service_uuid);

    std::vector<bt_gatt_h> characteristic_handles = CollectGattHandles(
        [service_handle](bt_gatt_foreach_cb callback, void* user_data) {
          return bt_gatt_service_foreach_characteristics(service_handle,
                                                         callback, user_data);
        },
        error_message);
    if (error_message != nullptr && !error_message->empty()) {
      return {};
    }

    std::map<std::string, int> instance_counters;
    EncodableList encoded_characteristics;
    for (bt_gatt_h characteristic_handle : characteristic_handles) {
      const std::string characteristic_uuid =
          GetGattUuid(characteristic_handle);
      if (characteristic_uuid.empty()) {
        continue;
      }
      const int instance_id = instance_counters[characteristic_uuid]++;

      const CharacteristicProperties properties =
          ReadCharacteristicProperties(characteristic_handle);

      std::vector<bt_gatt_h> descriptor_handles = CollectGattHandles(
          [characteristic_handle](bt_gatt_foreach_cb callback,
                                  void* user_data) {
            return bt_gatt_characteristic_foreach_descriptors(
                characteristic_handle, callback, user_data);
          },
          error_message);
      if (error_message != nullptr && !error_message->empty()) {
        return {};
      }

      EncodableList encoded_descriptors;
      for (bt_gatt_h descriptor_handle : descriptor_handles) {
        const std::string descriptor_uuid = GetGattUuid(descriptor_handle);
        if (descriptor_uuid.empty()) {
          continue;
        }
        encoded_descriptors.push_back(EncodableValue(
            MakeDescriptorMap(remote_id, service_uuid, characteristic_uuid,
                              instance_id, descriptor_uuid)));
        next_descriptors.emplace(
            MakeDescriptorKey(remote_id, service_uuid, characteristic_uuid,
                              instance_id, descriptor_uuid),
            DescriptorMeta{remote_id, service_uuid, characteristic_uuid,
                           instance_id, descriptor_uuid, descriptor_handle});
      }

      encoded_characteristics.push_back(EncodableValue(
          MakeCharacteristicMap(remote_id, service_uuid, characteristic_uuid,
                                instance_id, encoded_descriptors, properties)));
      next_characteristics.emplace(
          MakeCharacteristicKey(remote_id, service_uuid, characteristic_uuid,
                                instance_id),
          CharacteristicMeta{remote_id, service_uuid, characteristic_uuid,
                             instance_id, properties, characteristic_handle});
    }

    encoded_services.push_back(EncodableValue(
        MakeServiceMap(remote_id, service_uuid, encoded_characteristics)));
  }

  ClearCachedGattDatabase(remote_id);
  ReplaceForRemoteId(characteristics_, next_characteristics, remote_id);
  ReplaceForRemoteId(descriptors_, next_descriptors, remote_id);
  service_uuids_by_remote_id_[remote_id] = std::move(service_uuids);
  return encoded_services;
}

// ---------------------------------------------------------------------------
// Bond / GATT value helpers
// ---------------------------------------------------------------------------

std::vector<DeviceInfoSnapshot> BluetoothManager::EnumerateBondedDevices() {
  struct Context {
    std::vector<DeviceInfoSnapshot> devices;
  } context;

  const int rc = bt_adapter_foreach_bonded_device(
      [](bt_device_info_s* device_info, void* user_data) -> bool {
        static_cast<Context*>(user_data)->devices.push_back(
            SnapshotDeviceInfo(*device_info));
        return true;
      },
      &context);
  if (rc != BT_ERROR_NONE && rc != BT_ERROR_NOT_ENABLED) {
    events_.LogVerbose("foreach_bonded_device failed: " + DescribeBtError(rc));
  }
  return context.devices;
}

int BluetoothManager::GetBondStateValue(const std::string& remote_id) {
  bt_device_info_s* device_info = nullptr;
  const int rc =
      bt_adapter_get_bonded_device_info(remote_id.c_str(), &device_info);
  if (rc == BT_ERROR_NONE && device_info != nullptr) {
    bt_adapter_free_device_info(device_info);
    return kBmBondBonded;
  }
  return kBmBondNone;
}

std::string BluetoothManager::GetGattUuid(bt_gatt_h handle) {
  char* uuid = nullptr;
  const int rc = bt_gatt_get_uuid(handle, &uuid);
  if (rc != BT_ERROR_NONE || uuid == nullptr) {
    return {};
  }
  std::string normalized = NormalizeUuid(uuid);
  free(uuid);
  return normalized;
}

int BluetoothManager::SetGattValue(bt_gatt_h handle,
                                   const std::vector<uint8_t>& value) {
  const char* data =
      value.empty() ? nullptr : reinterpret_cast<const char*>(value.data());
  return bt_gatt_set_value(handle, data, static_cast<int>(value.size()));
}

std::vector<uint8_t> BluetoothManager::GetGattValueBytes(bt_gatt_h handle) {
  char* value = nullptr;
  int value_length = 0;
  const int rc = bt_gatt_get_value(handle, &value, &value_length);
  std::vector<uint8_t> bytes;
  if (rc == BT_ERROR_NONE && value != nullptr && value_length > 0) {
    bytes = CopyBytes(value, value_length);
  }
  if (value != nullptr) {
    free(value);
  }
  return bytes;
}

// ---------------------------------------------------------------------------
// Argument extractors
// ---------------------------------------------------------------------------

bool BluetoothManager::BuildCharacteristicRequest(const EncodableMap* arguments,
                                                  PendingGattKind kind,
                                                  PendingGattRequest& info) {
  std::string remote_id;
  std::string service_uuid;
  std::string characteristic_uuid;
  int instance_id = 0;
  if (!GetValueFromEncodableMap(arguments, "remote_id", remote_id) ||
      !GetValueFromEncodableMap(arguments, "service_uuid", service_uuid) ||
      !GetValueFromEncodableMap(arguments, "characteristic_uuid",
                                characteristic_uuid) ||
      !GetIntLike(arguments, "instance_id", instance_id)) {
    return false;
  }
  info.kind = kind;
  info.remote_id = NormalizeRemoteId(remote_id);
  info.service_uuid = NormalizeUuid(service_uuid);
  info.characteristic_uuid = NormalizeUuid(characteristic_uuid);
  info.instance_id = instance_id;
  return true;
}

bool BluetoothManager::BuildDescriptorRequest(const EncodableMap* arguments,
                                              PendingGattKind kind,
                                              PendingGattRequest& info) {
  std::string descriptor_uuid;
  if (!BuildCharacteristicRequest(arguments, kind, info) ||
      !GetValueFromEncodableMap(arguments, "descriptor_uuid",
                                descriptor_uuid)) {
    return false;
  }
  info.descriptor_uuid = NormalizeUuid(descriptor_uuid);
  return true;
}

// ---------------------------------------------------------------------------
// Event emit helpers
// ---------------------------------------------------------------------------

void BluetoothManager::EmitConnectionState(
    const std::string& remote_id, int connection_state,
    int disconnect_reason_code, const std::string& disconnect_reason) {
  events_.EmitEvent(
      "connection_state_changed",
      MakeConnectionStateMap(remote_id, connection_state,
                             disconnect_reason_code, disconnect_reason));
}

void BluetoothManager::EmitBondState(const std::string& remote_id,
                                     int bond_state, int prev_state) {
  events_.EmitEvent("bond_state_changed",
                    MakeBondStateMap(remote_id, bond_state, prev_state));
}

void BluetoothManager::EmitDiscoverServicesFailure(
    const std::string& remote_id, int error_code,
    const std::string& error_string) {
  events_.EmitEvent(
      "discovered_services",
      MakeDiscoverServicesMap(remote_id, {}, false, error_code, error_string));
}

void BluetoothManager::EmitCharacteristicData(
    const std::string& remote_id, const std::string& service_uuid,
    const std::string& characteristic_uuid, int instance_id,
    const std::vector<uint8_t>& value, bool success, int error_code,
    const std::string& error_string, bool written) {
  events_.EmitEvent(
      written ? "characteristic_written" : "characteristic_received",
      MakeCharacteristicDataMap(remote_id, service_uuid, characteristic_uuid,
                                instance_id, value, success, error_code,
                                error_string));
}

void BluetoothManager::EmitDescriptorData(
    const std::string& remote_id, const std::string& service_uuid,
    const std::string& characteristic_uuid, int instance_id,
    const std::string& descriptor_uuid, const std::vector<uint8_t>& value,
    bool success, int error_code, const std::string& error_string,
    bool written) {
  events_.EmitEvent(
      written ? "descriptor_written" : "descriptor_read",
      MakeDescriptorDataMap(remote_id, service_uuid, characteristic_uuid,
                            instance_id, descriptor_uuid, value, success,
                            error_code, error_string));
}

void BluetoothManager::EmitCccdWrite(const std::string& remote_id,
                                     const std::string& service_uuid,
                                     const std::string& characteristic_uuid,
                                     int instance_id,
                                     const std::vector<uint8_t>& value,
                                     bool success, int error_code,
                                     const std::string& error_string) {
  // Dart's FBP layer matches notifications on a descriptor write to the
  // CCCD UUID (0x2902).
  EmitDescriptorData(remote_id, service_uuid, characteristic_uuid, instance_id,
                     NormalizeUuid("2902"), value, success, error_code,
                     error_string, /*written=*/true);
}

void BluetoothManager::EmitMtuChanged(const std::string& remote_id,
                                      unsigned int mtu, bool success,
                                      int error_code,
                                      const std::string& error_string) {
  events_.EmitEvent("mtu_changed", MakeMtuChangedMap(remote_id, mtu, success,
                                                     error_code, error_string));
}

void BluetoothManager::EmitReadRssi(const std::string& remote_id, int rssi,
                                    bool success, int error_code,
                                    const std::string& error_string) {
  events_.EmitEvent("read_rssi", MakeReadRssiMap(remote_id, rssi, success,
                                                 error_code, error_string));
}

void BluetoothManager::EmitScanFailure(int error_code,
                                       const std::string& error_string) {
  events_.EmitEvent("scan_response",
                    MakeScanResponseMap({}, false, error_code, error_string));
}

// ---------------------------------------------------------------------------
// Scan helpers
// ---------------------------------------------------------------------------

bool BluetoothManager::MatchesScanRequest(
    const ScanAdvertisementData& advertisement) const {
  if (!active_scan_settings_ || !active_scan_settings_->HasFilters()) {
    return true;
  }
  const ScanSettingsData& settings = *active_scan_settings_;

  if (std::find(settings.with_remote_ids.begin(),
                settings.with_remote_ids.end(),
                advertisement.remote_id) != settings.with_remote_ids.end()) {
    return true;
  }
  if (std::find(settings.with_names.begin(), settings.with_names.end(),
                advertisement.adv_name) != settings.with_names.end()) {
    return true;
  }
  auto name_it = device_names_.find(advertisement.remote_id);
  if (name_it != device_names_.end() &&
      std::find(settings.with_names.begin(), settings.with_names.end(),
                name_it->second) != settings.with_names.end()) {
    return true;
  }

  const std::string adv_name_lower = ToLower(advertisement.adv_name);
  const std::string platform_name_lower =
      name_it == device_names_.end() ? std::string() : ToLower(name_it->second);
  for (const auto& keyword : settings.with_keywords) {
    const std::string lower_keyword = ToLower(keyword);
    if ((!adv_name_lower.empty() &&
         adv_name_lower.find(lower_keyword) != std::string::npos) ||
        (!platform_name_lower.empty() &&
         platform_name_lower.find(lower_keyword) != std::string::npos)) {
      return true;
    }
  }

  for (const auto& service_uuid : advertisement.service_uuids) {
    if (std::find(settings.with_services.begin(), settings.with_services.end(),
                  service_uuid) != settings.with_services.end()) {
      return true;
    }
  }

  for (const auto& filter : settings.with_msd) {
    auto manufacturer_it =
        advertisement.manufacturer_data.find(filter.manufacturer_id);
    if (manufacturer_it != advertisement.manufacturer_data.end() &&
        MatchesMaskedBytes(manufacturer_it->second, filter.data, filter.mask)) {
      return true;
    }
  }

  for (const auto& filter : settings.with_service_data) {
    auto service_it = advertisement.service_data.find(filter.service_uuid);
    if (service_it != advertisement.service_data.end() &&
        MatchesMaskedBytes(service_it->second, filter.data, filter.mask)) {
      return true;
    }
  }
  return false;
}

void BluetoothManager::UpdateDeviceName(const std::string& remote_id,
                                        const std::string& name) {
  if (name.empty()) {
    return;
  }
  auto current = device_names_.find(remote_id);
  if (current != device_names_.end() && current->second == name) {
    return;
  }
  device_names_[remote_id] = name;
  events_.EmitEvent(
      "name_changed",
      EncodableMap{{EncodableValue("remote_id"), EncodableValue(remote_id)},
                   {EncodableValue("name"), EncodableValue(name)}});
}

void BluetoothManager::EmitCurrentMtu(const std::string& remote_id) {
  auto client_it = clients_by_remote_id_.find(remote_id);
  if (client_it == clients_by_remote_id_.end() ||
      client_it->second == nullptr) {
    return;
  }
  unsigned int mtu = 0;
  if (bt_gatt_client_get_att_mtu(client_it->second, &mtu) == BT_ERROR_NONE) {
    EmitMtuChanged(remote_id, mtu, true, 0, "");
  }
}

void BluetoothManager::FailPendingRequestsForRemoteId(
    const std::string& remote_id, int error_code,
    const std::string& error_string) {
  std::vector<uintptr_t> gatt_handles;
  for (const auto& entry : pending_gatt_requests_) {
    if (entry.second.remote_id == remote_id) {
      gatt_handles.push_back(entry.first);
    }
  }
  for (uintptr_t handle_key : gatt_handles) {
    auto pending_it = pending_gatt_requests_.find(handle_key);
    if (pending_it == pending_gatt_requests_.end()) {
      continue;
    }
    const PendingGattRequest info = pending_it->second;
    pending_gatt_requests_.erase(pending_it);
    switch (info.kind) {
      case PendingGattKind::kReadCharacteristic:
        EmitCharacteristicData(info.remote_id, info.service_uuid,
                               info.characteristic_uuid, info.instance_id, {},
                               false, error_code, error_string, false);
        break;
      case PendingGattKind::kWriteCharacteristic:
        EmitCharacteristicData(info.remote_id, info.service_uuid,
                               info.characteristic_uuid, info.instance_id,
                               info.value, false, error_code, error_string,
                               true);
        break;
      case PendingGattKind::kReadDescriptor:
        EmitDescriptorData(info.remote_id, info.service_uuid,
                           info.characteristic_uuid, info.instance_id,
                           info.descriptor_uuid, {}, false, error_code,
                           error_string, false);
        break;
      case PendingGattKind::kWriteDescriptor:
        EmitDescriptorData(info.remote_id, info.service_uuid,
                           info.characteristic_uuid, info.instance_id,
                           info.descriptor_uuid, info.value, false, error_code,
                           error_string, true);
        break;
    }
  }

  std::vector<uintptr_t> mtu_clients;
  for (const auto& entry : pending_mtu_requests_) {
    if (entry.second.remote_id == remote_id) {
      mtu_clients.push_back(entry.first);
    }
  }
  for (uintptr_t client_key : mtu_clients) {
    auto pending_it = pending_mtu_requests_.find(client_key);
    if (pending_it == pending_mtu_requests_.end()) {
      continue;
    }
    const PendingMtuRequest info = pending_it->second;
    pending_mtu_requests_.erase(pending_it);
    EmitMtuChanged(info.remote_id, info.requested_mtu, false, error_code,
                   error_string);
  }
}

// ---------------------------------------------------------------------------
// Main-thread handlers
// ---------------------------------------------------------------------------

void BluetoothManager::OnAdapterStateChangedOnMain(int result,
                                                   bt_adapter_state_e state) {
  if (result != BT_ERROR_NONE) {
    return;
  }
  adapter_state_ = state == BT_ADAPTER_ENABLED ? kBmAdapterOn : kBmAdapterOff;
  events_.EmitEvent("adapter_state_changed",
                    MakeAdapterStateMap(adapter_state_));
}

void BluetoothManager::OnAdapterNameChangedOnMain(std::string name) {
  adapter_name_ = std::move(name);
}

void BluetoothManager::OnConnectionStateChangedOnMain(int result,
                                                      bool connected,
                                                      std::string remote_id) {
  remote_id = NormalizeRemoteId(remote_id);
  pending_connects_.erase(remote_id);
  if (connected && result == BT_ERROR_NONE) {
    connected_remote_ids_.insert(remote_id);
    EmitConnectionState(remote_id, kBmConnectionConnected, kNoIntValue, "");
    EmitCurrentMtu(remote_id);
    return;
  }

  connected_remote_ids_.erase(remote_id);
  const int error_code =
      result == BT_ERROR_NONE ? BT_ERROR_REMOTE_DEVICE_NOT_CONNECTED : result;
  const std::string error_string =
      result == BT_ERROR_NONE ? "Device " + remote_id + " is disconnected."
                              : DescribeBtError(result);
  FailPendingRequestsForRemoteId(remote_id, error_code, error_string);
  if (auto_connect_remote_ids_.count(remote_id)) {
    ClearCachedGattDatabase(remote_id);
  } else {
    DestroyClient(remote_id);
  }
  EmitConnectionState(remote_id, kBmConnectionDisconnected,
                      result == BT_ERROR_NONE ? kNoIntValue : result,
                      result == BT_ERROR_NONE ? "" : DescribeBtError(result));
}

void BluetoothManager::OnBondCreatedOnMain(int result,
                                           DeviceInfoSnapshot device_info) {
  const int prev_state = pending_bond_states_.count(device_info.remote_id)
                             ? pending_bond_states_[device_info.remote_id]
                             : kBmBondBonding;
  pending_bond_states_.erase(device_info.remote_id);
  UpdateDeviceName(device_info.remote_id, device_info.name);
  EmitBondState(device_info.remote_id,
                result == BT_ERROR_NONE ? kBmBondBonded : kBmBondNone,
                prev_state);
}

void BluetoothManager::OnBondDestroyedOnMain(int result,
                                             std::string remote_id) {
  remote_id = NormalizeRemoteId(remote_id);
  pending_bond_states_.erase(remote_id);
  EmitBondState(remote_id,
                result == BT_ERROR_NONE ? kBmBondNone : kBmBondBonded,
                kBmBondBonded);
}

void BluetoothManager::OnMtuChangedOnMain(uintptr_t client_key,
                                          std::string remote_id,
                                          unsigned int mtu,
                                          unsigned int status) {
  pending_mtu_requests_.erase(client_key);
  EmitMtuChanged(remote_id, mtu, status == BT_ERROR_NONE,
                 status == BT_ERROR_NONE ? 0 : static_cast<int>(status),
                 status == BT_ERROR_NONE ? "" : DescribeBtError(status));
  if (status == BT_ERROR_NONE) {
    events_.LogVerbose("requestMtu(" + remote_id + ") -> " +
                       std::to_string(mtu));
  }
}

void BluetoothManager::OnServiceChangedOnMain(uintptr_t client_key,
                                              int change_type,
                                              std::string service_uuid) {
  auto remote_it = remote_id_by_client_handle_.find(client_key);
  if (remote_it == remote_id_by_client_handle_.end()) {
    return;
  }
  const std::string& remote_id = remote_it->second;
  FailPendingRequestsForRemoteId(
      remote_id, BT_ERROR_SERVICE_SEARCH_FAILED,
      "GATT services changed. Call discoverServices() again.");
  ClearCachedGattDatabase(remote_id);
  events_.EmitEvent("services_reset", MakeBluetoothDeviceMap(
                                          remote_id, device_names_[remote_id]));
  events_.LogVerbose("serviceChanged(" + remote_id +
                     ") type=" + std::to_string(change_type) +
                     " service=" + NormalizeUuid(service_uuid));
}

void BluetoothManager::OnGattRequestCompletedOnMain(PendingGattRequest info,
                                                    int result,
                                                    bt_gatt_h request_handle) {
  pending_gatt_requests_.erase(info.handle_key);
  const std::vector<uint8_t> value = result == BT_ERROR_NONE
                                         ? GetGattValueBytes(request_handle)
                                         : std::vector<uint8_t>();
  const int error_code = result == BT_ERROR_NONE ? 0 : result;
  const std::string error_string =
      result == BT_ERROR_NONE ? "" : DescribeBtError(result);
  const bool success = result == BT_ERROR_NONE;

  switch (info.kind) {
    case PendingGattKind::kReadCharacteristic:
      EmitCharacteristicData(info.remote_id, info.service_uuid,
                             info.characteristic_uuid, info.instance_id, value,
                             success, error_code, error_string, false);
      break;
    case PendingGattKind::kWriteCharacteristic:
      EmitCharacteristicData(info.remote_id, info.service_uuid,
                             info.characteristic_uuid, info.instance_id,
                             info.value, success, error_code, error_string,
                             true);
      break;
    case PendingGattKind::kReadDescriptor:
      EmitDescriptorData(info.remote_id, info.service_uuid,
                         info.characteristic_uuid, info.instance_id,
                         info.descriptor_uuid, value, success, error_code,
                         error_string, false);
      break;
    case PendingGattKind::kWriteDescriptor:
      EmitDescriptorData(info.remote_id, info.service_uuid,
                         info.characteristic_uuid, info.instance_id,
                         info.descriptor_uuid, info.value, success, error_code,
                         error_string, true);
      break;
  }
}

void BluetoothManager::OnCharacteristicValueChangedOnMain(
    uintptr_t handle_key, std::vector<uint8_t> value) {
  auto notify_it = notify_characteristics_by_handle_.find(handle_key);
  if (notify_it == notify_characteristics_by_handle_.end()) {
    return;
  }
  auto characteristic_it = characteristics_.find(notify_it->second);
  if (characteristic_it == characteristics_.end()) {
    return;
  }
  const CharacteristicMeta& meta = characteristic_it->second;
  EmitCharacteristicData(meta.remote_id, meta.service_uuid,
                         meta.characteristic_uuid, meta.instance_id, value,
                         true, 0, "", false);
}

void BluetoothManager::OnScanResultOnMain(int result,
                                          ScanAdvertisementData advertisement) {
  if (!scanning_ || !active_scan_settings_) {
    return;
  }

  if (result != BT_ERROR_NONE) {
    events_.LogVerbose("scanResult(error): " + DescribeBtError(result));
    EmitScanFailure(result, DescribeBtError(result));
    return;
  }

  if (!advertisement.adv_name.empty()) {
    UpdateDeviceName(advertisement.remote_id, advertisement.adv_name);
  }
  if (!MatchesScanRequest(advertisement)) {
    return;
  }

  const int seen_count = scan_event_counts_[advertisement.remote_id];
  scan_event_counts_[advertisement.remote_id] = seen_count + 1;
  last_scan_rssi_[advertisement.remote_id] = advertisement.rssi;

  // First sighting always emitted; subsequent sightings only when the caller
  // opted into continuous updates (optionally throttled by divisor).
  if (!active_scan_settings_->continuous_updates && seen_count > 0) {
    return;
  }
  if (active_scan_settings_->continuous_updates &&
      active_scan_settings_->continuous_divisor > 1 && seen_count > 0 &&
      (seen_count % active_scan_settings_->continuous_divisor) != 0) {
    return;
  }

  EncodableList advertisements;
  advertisements.push_back(EncodableValue(MakeScanAdvertisementMap(
      advertisement, device_names_[advertisement.remote_id])));
  events_.EmitEvent("scan_response",
                    MakeScanResponseMap(advertisements, true, 0, ""));
  events_.LogVerbose("scanResult(" + advertisement.remote_id +
                     ") rssi=" + std::to_string(advertisement.rssi));
}

Eina_Bool BluetoothManager::TickDiscoverServices(const std::string& remote_id,
                                                 int& remaining_attempts) {
  discover_service_timers_.erase(remote_id);

  auto client_it = clients_by_remote_id_.find(remote_id);
  if (client_it == clients_by_remote_id_.end() ||
      client_it->second == nullptr) {
    EmitDiscoverServicesFailure(remote_id, BT_ERROR_REMOTE_DEVICE_NOT_CONNECTED,
                                "Device " + remote_id + " is not connected.");
    return ECORE_CALLBACK_CANCEL;
  }

  std::string error_message;
  EncodableList services =
      BuildServices(remote_id, client_it->second, &error_message);
  if (!error_message.empty()) {
    EmitDiscoverServicesFailure(remote_id, BT_ERROR_OPERATION_FAILED,
                                error_message);
    return ECORE_CALLBACK_CANCEL;
  }
  if (!services.empty()) {
    events_.EmitEvent(
        "discovered_services",
        MakeDiscoverServicesMap(remote_id, services, true, 0, ""));
    events_.LogVerbose("discoverServices(" + remote_id + ") -> " +
                       std::to_string(services.size()) + " services");
    return ECORE_CALLBACK_CANCEL;
  }

  if (--remaining_attempts <= 0) {
    EmitDiscoverServicesFailure(
        remote_id, BT_ERROR_NO_DATA,
        "No GATT services were discovered for " + remote_id + ".");
    return ECORE_CALLBACK_CANCEL;
  }
  return ECORE_CALLBACK_RENEW;
}

// ---------------------------------------------------------------------------
// C callbacks (worker thread → DispatchToMain)
// ---------------------------------------------------------------------------

void BluetoothManager::OnAdapterStateChangedCb(int result,
                                               bt_adapter_state_e state,
                                               void* user_data) {
  auto* self = static_cast<BluetoothManager*>(user_data);
  DispatchToMain([self, result, state]() {
    self->OnAdapterStateChangedOnMain(result, state);
  });
}

void BluetoothManager::OnAdapterNameChangedCb(char* device_name,
                                              void* user_data) {
  auto* self = static_cast<BluetoothManager*>(user_data);
  std::string name = device_name == nullptr ? std::string() : device_name;
  DispatchToMain([self, name = std::move(name)]() mutable {
    self->OnAdapterNameChangedOnMain(std::move(name));
  });
}

void BluetoothManager::OnConnectionStateChangedCb(int result, bool connected,
                                                  const char* remote_address,
                                                  void* user_data) {
  if (remote_address == nullptr) {
    return;
  }
  auto* self = static_cast<BluetoothManager*>(user_data);
  std::string remote_id = remote_address;
  DispatchToMain(
      [self, result, connected, remote_id = std::move(remote_id)]() mutable {
        self->OnConnectionStateChangedOnMain(result, connected,
                                             std::move(remote_id));
      });
}

void BluetoothManager::OnBondCreatedCb(int result,
                                       bt_device_info_s* device_info,
                                       void* user_data) {
  if (device_info == nullptr) {
    return;
  }
  auto* self = static_cast<BluetoothManager*>(user_data);
  DeviceInfoSnapshot snapshot = SnapshotDeviceInfo(*device_info);
  DispatchToMain([self, result, snapshot = std::move(snapshot)]() mutable {
    self->OnBondCreatedOnMain(result, std::move(snapshot));
  });
}

void BluetoothManager::OnBondDestroyedCb(int result, char* remote_address,
                                         void* user_data) {
  if (remote_address == nullptr) {
    return;
  }
  auto* self = static_cast<BluetoothManager*>(user_data);
  std::string remote_id = remote_address;
  DispatchToMain([self, result, remote_id = std::move(remote_id)]() mutable {
    self->OnBondDestroyedOnMain(result, std::move(remote_id));
  });
}

void BluetoothManager::OnMtuChangedCb(
    bt_gatt_client_h client, const bt_gatt_client_att_mtu_info_s* mtu_info,
    void* user_data) {
  if (client == nullptr || mtu_info == nullptr ||
      mtu_info->remote_address == nullptr) {
    return;
  }
  auto* self = static_cast<BluetoothManager*>(user_data);
  const uintptr_t client_key = reinterpret_cast<uintptr_t>(client);
  std::string remote_id = NormalizeRemoteId(mtu_info->remote_address);
  const unsigned int mtu = mtu_info->mtu;
  const unsigned int status = mtu_info->status;
  DispatchToMain([self, client_key, remote_id = std::move(remote_id), mtu,
                  status]() mutable {
    self->OnMtuChangedOnMain(client_key, std::move(remote_id), mtu, status);
  });
}

void BluetoothManager::OnServiceChangedCb(
    bt_gatt_client_h client, bt_gatt_client_service_change_type_e change_type,
    const char* service_uuid, void* user_data) {
  if (client == nullptr) {
    return;
  }
  auto* self = static_cast<BluetoothManager*>(user_data);
  const uintptr_t client_key = reinterpret_cast<uintptr_t>(client);
  std::string uuid =
      service_uuid == nullptr ? std::string() : std::string(service_uuid);
  DispatchToMain(
      [self, client_key, change_type, uuid = std::move(uuid)]() mutable {
        self->OnServiceChangedOnMain(client_key, change_type, std::move(uuid));
      });
}

void BluetoothManager::OnGattRequestCompletedCb(int result,
                                                bt_gatt_h request_handle,
                                                void* user_data) {
  std::unique_ptr<GattRequestCallbackContext> context(
      static_cast<GattRequestCallbackContext*>(user_data));
  if (!context || context->manager == nullptr) {
    return;
  }
  BluetoothManager* self = context->manager;
  PendingGattRequest info = context->info;
  DispatchToMain([self, info = std::move(info), result,
                  request_handle]() mutable {
    self->OnGattRequestCompletedOnMain(std::move(info), result, request_handle);
  });
}

void BluetoothManager::OnCharacteristicValueChangedCb(bt_gatt_h characteristic,
                                                      char* value, int len,
                                                      void* user_data) {
  if (characteristic == nullptr) {
    return;
  }
  auto* self = static_cast<BluetoothManager*>(user_data);
  const uintptr_t handle_key = reinterpret_cast<uintptr_t>(characteristic);
  std::vector<uint8_t> bytes = CopyBytes(value, len);
  DispatchToMain([self, handle_key, bytes = std::move(bytes)]() mutable {
    self->OnCharacteristicValueChangedOnMain(handle_key, std::move(bytes));
  });
}

void BluetoothManager::OnScanResultCb(
    int result, bt_adapter_le_device_scan_result_info_s* info,
    void* user_data) {
  if (info == nullptr || info->remote_address == nullptr) {
    return;
  }
  auto* self = static_cast<BluetoothManager*>(user_data);
  ScanAdvertisementData advertisement;
  advertisement.remote_id = NormalizeRemoteId(info->remote_address);
  advertisement.rssi = info->rssi;
  ReadAdvertisementData(info, advertisement);

  DispatchToMain(
      [self, result, advertisement = std::move(advertisement)]() mutable {
        self->OnScanResultOnMain(result, std::move(advertisement));
      });
}

Eina_Bool BluetoothManager::OnDiscoverServicesTimerCb(void* data) {
  // The plugin owns the context; if Tick returns RENEW we hand ownership back
  // to the timer by reinserting it into discover_service_timers_.
  std::unique_ptr<DiscoverServicesContext> context(
      static_cast<DiscoverServicesContext*>(data));
  if (context == nullptr || context->manager == nullptr) {
    return ECORE_CALLBACK_CANCEL;
  }
  BluetoothManager* manager = context->manager;
  const std::string remote_id = context->remote_id;
  const Eina_Bool tick_result =
      manager->TickDiscoverServices(remote_id, context->remaining_attempts);
  if (tick_result == ECORE_CALLBACK_RENEW) {
    DiscoverServicesContext* released = context.release();
    Ecore_Timer* timer =
        ecore_timer_add(0.1, OnDiscoverServicesTimerCb, released);
    manager->discover_service_timers_[remote_id] = timer;
  }
  return ECORE_CALLBACK_CANCEL;
}

}  // namespace flutter_blue_plus_tizen
