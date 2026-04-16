# flutter_blue_plus_tizen

The Tizen implementation of [`flutter_blue_plus`](https://pub.dev/packages/flutter_blue_plus).

This package uses a native Tizen C++ plugin plus Dart `MethodChannel` /
`EventChannel` bindings, and it targets the public Bluetooth APIs available on
Tizen 6.5.

## Getting started

This package is not an endorsed implementation of `flutter_blue_plus`.
Add both packages to your `pubspec.yaml`:

```yaml
dependencies:
  flutter_blue_plus: ^2.2.1
  flutter_blue_plus_tizen: ^0.1.0
```

Then import the original package:

```dart
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
```

## Tizen 6.5 scope

The implementation uses the Tizen 6.5 native Bluetooth C API from
`<bluetooth.h>` and avoids direct Dart FFI callbacks for asynchronous Bluetooth
events.

It is designed for:

- BLE central / GATT client use cases
- Third-party Tizen applications with the public Bluetooth privilege
- Stable operation with defensive cleanup of callbacks, client handles, and
  pending GATT requests

## Interface status

### Supported through the public `flutter_blue_plus` API

| Interface | Status | Notes |
| --- | --- | --- |
| `isSupported` | Supported | Uses `bt_initialize()` and adapter state queries. |
| `adapterState`, `adapterName` | Supported | Adapter callbacks are registered during initialization. |
| `startScan`, `stopScan`, scan filters | Supported | Service UUID, name, keyword, manufacturer data, and service data filters work. |
| `connect`, `disconnect` | Supported | Uses `bt_gatt_connect()` / `bt_gatt_disconnect()`. |
| `discoverServices` | Supported | Enumerates the current GATT database from the client handle. |
| Characteristic `read` / `write` | Supported | Uses `bt_gatt_client_read_value()` / `bt_gatt_client_write_value()`. |
| Descriptor `read` / `write` | Supported | Uses the same GATT request completion path as Android. |
| `setNotifyValue` | Supported | Uses `bt_gatt_client_set_characteristic_value_changed_cb()`. |
| `device.onServicesReset` | Supported | Uses `bt_gatt_client_set_service_changed_cb()`. |
| `FlutterBluePlus.systemDevices()` | Partially supported | Limited to devices connected to this app and bonded devices reported as connected by Tizen. |
| `FlutterBluePlus.bondedDevices` | Supported | Enumerates bonded devices through the public adapter API. |
| `readRssi()` | Partially supported | Returns the latest RSSI observed during scanning, not a live connected-device RSSI read. |

### Implemented in the Tizen platform layer, but blocked by upstream Android-only guards

These methods are implemented here because Tizen 6.5 has public APIs for them,
but the upstream `flutter_blue_plus` Dart API currently throws on non-Android
before it reaches the platform implementation.

| Interface | Tizen implementation | Backend API |
| --- | --- | --- |
| `createBond()` | Implemented | `bt_device_create_bond()` |
| `removeBond()` | Implemented | `bt_device_destroy_bond()` |
| `requestMtu()` | Implemented | `bt_gatt_client_request_att_mtu_change()` |
| `requestConnectionPriority()` | Implemented | `bt_device_update_le_connection_mode()` |

### Unsupported on Tizen 6.5 public APIs

| Interface | Status | Reason |
| --- | --- | --- |
| `clearGattCache()` | Unsupported | Tizen 6.5 does not expose an Android-style public GATT cache refresh API. |
| `setPreferredPhy()` | Unsupported | No public Tizen 6.5 BLE PHY selection API is exposed for third-party apps. |
| `getPhySupport()` | Unsupported | No public Tizen 6.5 BLE PHY capability query API is exposed for third-party apps. |
| `turnOn()` / `turnOff()` | Unsupported for third-party apps | Requires privileged system control outside the public app privilege model. |

## Tizen-specific behavior differences

- `readRssi()` uses cached scan RSSI because Tizen 6.5 does not expose a public
  connected-device RSSI read for GATT clients.
- Scan results do not expose a reliable connectable/non-connectable flag in the
  Tizen 6.5 LE scan result structure. The plugin reports scanned LE devices as
  connectable to preserve the standard `flutter_blue_plus` scan/connect flow.
- `systemDevices()` is not as broad as Android. Tizen public APIs do not expose
  every BLE connection owned by other applications.
- `discoverServices()` must be called again after each reconnect, and again
  after `device.onServicesReset` fires.

## Stability notes

- Asynchronous native Bluetooth callbacks stay inside the native plugin and are
  forwarded to Flutter with `EventChannel`, instead of invoking Dart through
  direct FFI callbacks.
- Bluetooth API entry points are executed on the Tizen main loop, matching the
  Tizen Bluetooth guide requirement that Bluetooth not be used from worker
  threads.
- GATT client handles are reused per remote device and are destroyed when they
  are no longer needed.
- Pending GATT read/write and MTU requests are failed and cleaned up on
  disconnect or service reset to avoid stale callbacks and hanging operations.
- Duplicate read/write requests on the same characteristic or descriptor handle
  are rejected until the prior request completes. This avoids ambiguous request
  completion routing in the public Tizen 6.5 GATT client API.
- Notification callbacks, MTU callbacks, and service-changed callbacks are
  unregistered before client destruction.
- Scan filter mask length is validated defensively to avoid runtime crashes when
  invalid filter input is passed in release builds.
- Verbose logs are mirrored to both Flutter and the native Tizen system log
  (`dlog`) under the `FBP-Tizen` tag for device-side debugging.

## Required privileges and features

Tizen 6.5 applications should declare the following feature requirements and
privilege in `tizen-manifest.xml`:

```xml
<manifest api-version="6.5" ...>
  <feature name="http://tizen.org/feature/network.bluetooth"/>
  <feature name="http://tizen.org/feature/network.bluetooth.le"/>
  <feature name="http://tizen.org/feature/network.bluetooth.le.gatt.client"/>

  <privileges>
    <privilege>http://tizen.org/privilege/bluetooth</privilege>
  </privileges>
</manifest>
```

The additional GATT client feature requirement improves deployment reliability
on devices that do not expose the full BLE client stack.
