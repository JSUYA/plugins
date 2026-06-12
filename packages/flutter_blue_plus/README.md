# flutter_blue_plus_tizen

The Tizen implementation of [`flutter_blue_plus`](https://pub.dev/packages/flutter_blue_plus).

## Usage

This package is not an endorsed implementation of `flutter_blue_plus`.
Therefore, you have to include `flutter_blue_plus_tizen` alongside
`flutter_blue_plus` as dependencies in your `pubspec.yaml` file.

```yaml
dependencies:
  flutter_blue_plus: ^2.3.8
  flutter_blue_plus_tizen: ^0.1.0
```

Then you can import `flutter_blue_plus` in your Dart code:

```dart
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
```

## Required privileges

To use this plugin, add the following feature and privilege declarations to
your `tizen-manifest.xml` file:

```xml
<feature name="http://tizen.org/feature/network.bluetooth"/>
<feature name="http://tizen.org/feature/network.bluetooth.le"/>
<feature name="http://tizen.org/feature/network.bluetooth.le.gatt.client"/>

<privileges>
  <privilege>http://tizen.org/privilege/bluetooth</privilege>
</privileges>
```

## Supported features

- `isSupported`
- `adapterState` / `adapterName`
- `startScan` / `stopScan`, including scan filters
  (`withServices`, `withRemoteIds`, `withNames`, `withKeywords`, `withMsd`,
  `withServiceData`, `continuousUpdates`, `continuousDivisor`)
- `connect` / `disconnect`
- `discoverServices`
- Characteristic `read` / `write` (with and without response)
- Descriptor `read` / `write`
- `setNotifyValue` (notifications and indications)
- `device.onServicesReset`
- `bondedDevices` / `bondState`
- `mtu` change events

## Implemented but blocked by upstream platform guards

The Tizen platform layer implements the following methods, but the upstream
`flutter_blue_plus` Dart API currently throws on non-Android platforms before
the call reaches this implementation:

- `createBond` (`bt_device_create_bond`)
- `removeBond` (`bt_device_destroy_bond`)
- `requestMtu` (`bt_gatt_client_request_att_mtu_change`)
- `requestConnectionPriority` (`bt_device_update_le_connection_mode`)

## Partially supported features

- `systemDevices`: Limited to devices connected by this app and bonded
  devices reported as connected by the platform. Tizen does not expose BLE
  connections owned by other applications.
- `readRssi`: Returns the most recent RSSI observed while scanning. Tizen
  does not provide a public API to read the live RSSI of a connected device.

## Unsupported features

- `turnOn` / `turnOff`: Third-party Tizen applications are not allowed to
  change the Bluetooth adapter state.
- `clearGattCache`: Tizen does not expose a public GATT cache refresh API.
- `setPreferredPhy` / `getPhySupport`: Tizen does not expose public BLE PHY
  control APIs. `getPhySupport` reports that 2M and Coded PHY are
  unsupported.
- BLE peripheral (advertising) and L2CAP channels: Not part of the
  `flutter_blue_plus` Tizen scope.

## Tizen-specific behavior

- Scan results do not include a reliable connectable flag, so scanned LE
  devices are always reported as connectable.
- `discoverServices` must be called again after each reconnect, and after
  `device.onServicesReset` fires.
