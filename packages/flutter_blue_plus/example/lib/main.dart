// ignore_for_file: public_member_api_docs
// ignore_for_file: always_specify_types
// ignore_for_file: avoid_redundant_argument_values

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_blue_plus_platform_interface/flutter_blue_plus_platform_interface.dart';
import 'package:flutter_blue_plus_tizen/flutter_blue_plus_tizen.dart';

void main() {
  FlutterBluePlus.setLogLevel(LogLevel.verbose, color: false);
  runApp(const FlutterBluePlusExampleApp());
}

class FlutterBluePlusExampleApp extends StatelessWidget {
  const FlutterBluePlusExampleApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.teal),
      ),
      home: const FlutterBluePlusExamplePage(),
    );
  }
}

class FlutterBluePlusExamplePage extends StatefulWidget {
  const FlutterBluePlusExamplePage({super.key});

  @override
  State<FlutterBluePlusExamplePage> createState() =>
      _FlutterBluePlusExamplePageState();
}

class _FlutterBluePlusExamplePageState
    extends State<FlutterBluePlusExamplePage> {
  BluetoothAdapterState _adapterState = BluetoothAdapterState.unknown;
  bool _isSupported = false;
  bool _isScanning = false;
  bool _scanStartedAtLeastOnce = false;
  String? _message;
  String? _scanError;
  List<ScanResult> _scanResults = <ScanResult>[];
  List<BluetoothDevice> _bondedDevices = <BluetoothDevice>[];
  BluetoothDevice? _selectedDevice;
  BluetoothConnectionState _connectionState =
      BluetoothConnectionState.disconnected;
  List<BluetoothService> _services = <BluetoothService>[];
  final TextEditingController _manualRemoteIdController =
      TextEditingController();

  int _rawEventCount = 0;
  int _scanAdvertisementCount = 0;
  String _lastEventType = '—';
  String? _firstAdSummary;
  String? _parseErrorSummary;

  late final StreamSubscription<BluetoothAdapterState>
      _adapterStateSubscription;
  late final StreamSubscription<bool> _isScanningSubscription;
  late final StreamSubscription<List<ScanResult>> _scanResultsSubscription;
  late final StreamSubscription<OnConnectionStateChangedEvent>
      _connectionEventSubscription;
  late final StreamSubscription<OnCharacteristicReceivedEvent>
      _characteristicEventSubscription;
  late final StreamSubscription<OnDescriptorReadEvent>
      _descriptorEventSubscription;
  late final StreamSubscription<OnDescriptorWrittenEvent>
      _descriptorWriteEventSubscription;
  late final StreamSubscription<OnServicesResetEvent>
      _servicesResetSubscription;
  late final StreamSubscription<dynamic> _rawEventSubscription;

  bool get _adapterOn => _adapterState == BluetoothAdapterState.on;
  bool get _canScan => _isSupported && _adapterOn;

  @override
  void initState() {
    super.initState();
    _adapterStateSubscription = FlutterBluePlus.adapterState.listen((state) {
      if (!mounted) {
        return;
      }
      setState(() {
        _adapterState = state;
        if (state == BluetoothAdapterState.on) {
          _scanError = null;
        }
      });
      _refreshBondedDevices();
    });
    _isScanningSubscription = FlutterBluePlus.isScanning.listen((value) {
      if (!mounted) {
        return;
      }
      setState(() {
        _isScanning = value;
        if (!value && _scanStartedAtLeastOnce && _scanResults.isEmpty) {
          _message =
              'No BLE advertisements were found. Verify that nearby devices '
              'are advertising and Bluetooth is enabled.';
        }
      });
    });
    _scanResultsSubscription = FlutterBluePlus.scanResults.listen(
      (results) {
        if (!mounted) {
          return;
        }
        setState(() {
          _scanResults = results;
          if (results.isNotEmpty) {
            _message = null;
            _scanError = null;
          }
        });
      },
      onError: (Object error, StackTrace stackTrace) {
        if (!mounted) {
          return;
        }
        setState(() {
          _scanError = _humanScanError(error);
        });
      },
    );
    _connectionEventSubscription =
        FlutterBluePlus.events.onConnectionStateChanged.listen((event) {
      if (_selectedDevice?.remoteId != event.device.remoteId || !mounted) {
        return;
      }
      setState(() {
        _connectionState = event.connectionState;
        if (event.connectionState == BluetoothConnectionState.disconnected) {
          _services = <BluetoothService>[];
        }
      });
    });
    _characteristicEventSubscription =
        FlutterBluePlus.events.onCharacteristicReceived.listen((event) {
      if (_selectedDevice?.remoteId == event.device.remoteId && mounted) {
        setState(() {});
      }
    });
    _descriptorEventSubscription =
        FlutterBluePlus.events.onDescriptorRead.listen((event) {
      if (_selectedDevice?.remoteId == event.device.remoteId && mounted) {
        setState(() {});
      }
    });
    _descriptorWriteEventSubscription =
        FlutterBluePlus.events.onDescriptorWritten.listen((event) {
      if (_selectedDevice?.remoteId == event.device.remoteId && mounted) {
        setState(() {});
      }
    });
    _servicesResetSubscription = FlutterBluePlus.events.onServicesReset.listen((
      event,
    ) {
      if (_selectedDevice?.remoteId != event.device.remoteId || !mounted) {
        return;
      }
      setState(() {
        _services = <BluetoothService>[];
        _message = 'Services changed. Rediscover services.';
      });
    });
    _rawEventSubscription = FlutterBluePlusTizen.rawEvents.listen(
      (Map<dynamic, dynamic> event) {
        if (!mounted) {
          return;
        }
        final String type = event['type']?.toString() ?? '';
        final Object? data = event['data'];

        String? adSummary;
        String? parseError;
        int advertisementsInEvent = 0;
        String? scanFailureMessage;
        if (type == 'scan_response' && data is Map) {
          final Object? success = data['success'];
          final Object? errorCode = data['error_code'];
          final Object? errorString = data['error_string'];
          if (success == 0 || success == false) {
            scanFailureMessage =
                _formatScanFailure(errorCode, errorString?.toString());
          }
          final Object? advertisements = data['advertisements'];
          if (advertisements is List) {
            advertisementsInEvent = advertisements.length;
            if (advertisements.isNotEmpty) {
              final Object? first = advertisements.first;
              if (first is Map) {
                final String remoteId = first['remote_id']?.toString() ?? '?';
                final String advName = first['adv_name']?.toString() ?? '';
                final Object? uuids = first['service_uuids'];
                final int uuidCount = uuids is List ? uuids.length : 0;
                final Object? mfg = first['manufacturer_data'];
                final int mfgCount = mfg is Map ? mfg.length : 0;
                adSummary =
                    '$remoteId / "${advName.isEmpty ? '—' : advName}" / '
                    'uuids=$uuidCount mfg=$mfgCount';
              }
            }
          }
          try {
            final Map<dynamic, dynamic> typed =
                Map<dynamic, dynamic>.from(data);
            BmScanResponse.fromMap(typed);
          } catch (error, stackTrace) {
            parseError = error.toString();
            debugPrintStack(stackTrace: stackTrace);
          }
        }

        setState(() {
          _rawEventCount += 1;
          if (type.isNotEmpty) {
            _lastEventType = type;
          }
          _scanAdvertisementCount += advertisementsInEvent;
          if (adSummary != null) {
            _firstAdSummary = adSummary;
          }
          if (parseError != null) {
            _parseErrorSummary = parseError;
          }
          if (scanFailureMessage != null) {
            _scanError = scanFailureMessage;
          }
        });
      },
      onError: (Object error, StackTrace stackTrace) {
        if (!mounted) {
          return;
        }
        setState(() {
          _message = 'Raw event channel error: $error';
        });
      },
    );
    _initialize();
  }

  Future<void> _initialize() async {
    final bool supported = await FlutterBluePlus.isSupported;
    if (!mounted) {
      return;
    }
    setState(() {
      _isSupported = supported;
      _message =
          supported ? null : 'Bluetooth is not supported on this device.';
    });
    if (supported) {
      try {
        _adapterState = await FlutterBluePlus.adapterState.first
            .timeout(const Duration(milliseconds: 500));
      } on TimeoutException {
        // Fall through; adapterState stream will deliver the value later.
      }
      await _refreshBondedDevices();
    }
  }

  Future<void> _refreshBondedDevices() async {
    if (!_isSupported) {
      return;
    }
    try {
      final List<BluetoothDevice> devices = await FlutterBluePlus.bondedDevices;
      if (!mounted) {
        return;
      }
      setState(() {
        _bondedDevices = devices;
      });
    } catch (_) {
      // bondedDevices may throw on platforms without the privilege; treat as
      // empty so the UI keeps rendering.
      if (!mounted) {
        return;
      }
      setState(() {
        _bondedDevices = <BluetoothDevice>[];
      });
    }
  }

  String _humanScanError(Object error) {
    final String text = error.toString();
    if (text.contains('NOT_ENABLED') || text.contains('not enabled')) {
      return 'Bluetooth adapter is OFF. Turn it on to scan.';
    }
    return text;
  }

  String _formatScanFailure(Object? errorCode, String? errorString) {
    final String code = errorCode?.toString() ?? '';
    final String description = errorString == null || errorString.isEmpty
        ? 'Scan failed'
        : errorString;
    if (description.contains('not enabled')) {
      return 'Bluetooth adapter is OFF. Turn it on to scan. ($description)';
    }
    return code.isEmpty ? description : '$description [code=$code]';
  }

  @override
  void dispose() {
    _adapterStateSubscription.cancel();
    _isScanningSubscription.cancel();
    _scanResultsSubscription.cancel();
    _connectionEventSubscription.cancel();
    _characteristicEventSubscription.cancel();
    _descriptorEventSubscription.cancel();
    _descriptorWriteEventSubscription.cancel();
    _servicesResetSubscription.cancel();
    _rawEventSubscription.cancel();
    _manualRemoteIdController.dispose();
    super.dispose();
  }

  Future<void> _toggleScan() async {
    if (!_canScan) {
      setState(() {
        _scanError = _adapterOn
            ? 'Bluetooth is unavailable on this device.'
            : 'Bluetooth adapter is OFF. Turn it on to scan.';
      });
      return;
    }
    try {
      if (_isScanning) {
        await FlutterBluePlus.stopScan();
      } else {
        if (mounted) {
          setState(() {
            _scanStartedAtLeastOnce = true;
            _scanResults = <ScanResult>[];
            _scanError = null;
          });
        }
        await FlutterBluePlus.startScan(timeout: const Duration(seconds: 12));
      }
      if (!mounted) {
        return;
      }
      setState(() {
        _message = null;
      });
    } catch (error) {
      if (!mounted) {
        return;
      }
      setState(() {
        _scanError = _humanScanError(error);
      });
    }
  }

  Future<void> _turnOnBluetooth() async {
    setState(() {
      _message = null;
    });
    try {
      await FlutterBluePlus.turnOn();
    } catch (error) {
      if (!mounted) {
        return;
      }
      setState(() {
        _message = 'Could not enable Bluetooth from the app ($error). '
            'On Tizen Common (RPi), run: `bluetoothctl power on` on the device '
            'shell, then retry.';
      });
    }
  }

  Future<void> _connectByRemoteId(String remoteId) async {
    final String trimmed = remoteId.trim();
    if (trimmed.isEmpty) {
      return;
    }
    final BluetoothDevice device =
        BluetoothDevice(remoteId: DeviceIdentifier(trimmed));
    await _connectDevice(device);
  }

  Future<void> _connect(ScanResult result) => _connectDevice(result.device,
      connectable: result.advertisementData.connectable);

  Future<void> _connectDevice(
    BluetoothDevice device, {
    bool connectable = true,
  }) async {
    if (!connectable) {
      return;
    }
    try {
      await device.connect(license: License.free, mtu: null);
      if (!mounted) {
        return;
      }
      setState(() {
        _selectedDevice = device;
        _connectionState = device.isConnected
            ? BluetoothConnectionState.connected
            : BluetoothConnectionState.disconnected;
        _services = <BluetoothService>[];
        _message = null;
      });

      final List<BluetoothService> services = await device.discoverServices();
      if (!mounted) {
        return;
      }
      setState(() {
        _selectedDevice = device;
        _connectionState = device.isConnected
            ? BluetoothConnectionState.connected
            : BluetoothConnectionState.disconnected;
        _services = services;
        _message = null;
      });
    } catch (error) {
      if (!mounted) {
        return;
      }
      setState(() {
        _selectedDevice = device.isConnected ? device : null;
        _connectionState = device.isConnected
            ? BluetoothConnectionState.connected
            : BluetoothConnectionState.disconnected;
        _message = error.toString();
      });
    }
  }

  Future<void> _disconnectSelectedDevice() async {
    final BluetoothDevice? device = _selectedDevice;
    if (device == null) {
      return;
    }
    try {
      await device.disconnect();
      if (!mounted) {
        return;
      }
      setState(() {
        _connectionState = BluetoothConnectionState.disconnected;
        _services = <BluetoothService>[];
      });
    } catch (error) {
      if (!mounted) {
        return;
      }
      setState(() {
        _message = error.toString();
      });
    }
  }

  Future<void> _rediscoverServices() async {
    final BluetoothDevice? device = _selectedDevice;
    if (device == null) {
      return;
    }
    try {
      final List<BluetoothService> services = await device.discoverServices();
      if (!mounted) {
        return;
      }
      setState(() {
        _services = services;
        _message = null;
      });
    } catch (error) {
      if (!mounted) {
        return;
      }
      setState(() {
        _message = error.toString();
      });
    }
  }

  Future<void> _readCharacteristic(
    BluetoothCharacteristic characteristic,
  ) async {
    try {
      await characteristic.read();
      if (mounted) {
        setState(() {});
      }
    } catch (error) {
      if (mounted) {
        setState(() {
          _message = error.toString();
        });
      }
    }
  }

  Future<void> _writeCharacteristic(
    BluetoothCharacteristic characteristic,
  ) async {
    try {
      await characteristic.write(const <int>[0x01], withoutResponse: false);
      if (mounted) {
        setState(() {});
      }
    } catch (error) {
      if (mounted) {
        setState(() {
          _message = error.toString();
        });
      }
    }
  }

  Future<void> _toggleNotify(BluetoothCharacteristic characteristic) async {
    try {
      await characteristic.setNotifyValue(!characteristic.isNotifying);
      if (mounted) {
        setState(() {});
      }
    } catch (error) {
      if (mounted) {
        setState(() {
          _message = error.toString();
        });
      }
    }
  }

  Future<void> _readDescriptor(BluetoothDescriptor descriptor) async {
    try {
      await descriptor.read();
      if (mounted) {
        setState(() {});
      }
    } catch (error) {
      if (mounted) {
        setState(() {
          _message = error.toString();
        });
      }
    }
  }

  Future<void> _writeDescriptor(BluetoothDescriptor descriptor) async {
    try {
      await descriptor.write(const <int>[0x00]);
      if (mounted) {
        setState(() {});
      }
    } catch (error) {
      if (mounted) {
        setState(() {
          _message = error.toString();
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final ColorScheme colorScheme = Theme.of(context).colorScheme;
    return Scaffold(
      appBar: AppBar(
        title: const Text('Flutter Blue Plus Example'),
        actions: <Widget>[
          TextButton(
            onPressed: _canScan ? _toggleScan : null,
            child: Text(
              _isScanning ? 'STOP' : 'SCAN',
              style: TextStyle(color: colorScheme.onSurface),
            ),
          ),
        ],
      ),
      body: Column(
        children: <Widget>[
          _buildStatusCard(context),
          Expanded(
            child: Row(
              children: <Widget>[
                Expanded(child: _buildLeftPane()),
                const VerticalDivider(width: 1),
                Expanded(child: _buildDeviceDetails()),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildStatusCard(BuildContext context) {
    final ColorScheme colorScheme = Theme.of(context).colorScheme;
    final bool adapterOff = _isSupported && !_adapterOn;
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(16),
      color: adapterOff
          ? colorScheme.errorContainer
          : colorScheme.surfaceContainerHighest,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: <Widget>[
          Text('Supported: $_isSupported'),
          Text('Adapter: ${_adapterState.name}'),
          Text('Scanning: $_isScanning'),
          Text(
            'Scan advertisements received: $_scanAdvertisementCount '
            '(unique devices: ${_scanResults.length})',
          ),
          Text('Bonded devices: ${_bondedDevices.length}'),
          Text(
            'Plugin events total: $_rawEventCount · last: $_lastEventType',
          ),
          if (_firstAdSummary != null)
            Text('First advertisement: ${_firstAdSummary!}'),
          if (_parseErrorSummary != null)
            Text(
              'Upstream parse error: ${_parseErrorSummary!}',
              style: TextStyle(color: colorScheme.error),
            ),
          if (_scanError != null && _scanError!.isNotEmpty) ...<Widget>[
            const SizedBox(height: 8),
            Text(_scanError!, style: TextStyle(color: colorScheme.error)),
          ],
          if (_message != null && _message!.isNotEmpty) ...<Widget>[
            const SizedBox(height: 8),
            Text(_message!, style: TextStyle(color: colorScheme.error)),
          ],
          const SizedBox(height: 12),
          Wrap(
            spacing: 12,
            runSpacing: 8,
            children: <Widget>[
              FilledButton.icon(
                onPressed: _canScan ? _toggleScan : null,
                icon: Icon(
                  _isScanning ? Icons.stop_circle : Icons.bluetooth_searching,
                ),
                label: Text(
                  _isScanning ? 'Stop scan' : 'Start scan',
                  style: const TextStyle(fontSize: 18),
                ),
                style: FilledButton.styleFrom(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 24,
                    vertical: 16,
                  ),
                ),
              ),
              if (adapterOff)
                OutlinedButton.icon(
                  onPressed: _turnOnBluetooth,
                  icon: const Icon(Icons.power_settings_new),
                  label: const Text('Turn on Bluetooth'),
                ),
              OutlinedButton.icon(
                onPressed: _isSupported ? _refreshBondedDevices : null,
                icon: const Icon(Icons.refresh),
                label: const Text('Refresh bonded'),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildLeftPane() {
    if (!_isSupported) {
      return const Center(child: Text('Bluetooth is unavailable.'));
    }
    return ListView(
      padding: const EdgeInsets.symmetric(vertical: 8),
      children: <Widget>[
        _buildSectionHeader(context, 'Scan results'),
        _buildScanResultsSection(),
        const Divider(height: 1),
        _buildSectionHeader(context, 'Bonded / system devices'),
        _buildBondedDevicesSection(),
        const Divider(height: 1),
        _buildSectionHeader(context, 'Connect by remote ID'),
        _buildManualConnectSection(),
      ],
    );
  }

  Widget _buildSectionHeader(BuildContext context, String label) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 12, 16, 4),
      child: Text(
        label,
        style: Theme.of(context).textTheme.titleSmall?.copyWith(
              fontWeight: FontWeight.bold,
            ),
      ),
    );
  }

  Widget _buildScanResultsSection() {
    if (!_adapterOn) {
      return const Padding(
        padding: EdgeInsets.symmetric(horizontal: 16, vertical: 12),
        child: Text(
          'Adapter is OFF. Scan is unavailable until Bluetooth is enabled.',
        ),
      );
    }
    if (_scanResults.isEmpty) {
      return Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
        child: Text(
          _scanStartedAtLeastOnce
              ? 'No advertisements observed yet.'
              : 'Start a scan to discover BLE devices.',
        ),
      );
    }
    return Column(
      children: <Widget>[
        for (final ScanResult result in _scanResults)
          ListTile(
            title: Text(
              result.device.platformName.isNotEmpty
                  ? result.device.platformName
                  : (result.advertisementData.advName.isNotEmpty
                      ? result.advertisementData.advName
                      : result.device.remoteId.str),
            ),
            subtitle:
                Text('${result.device.remoteId.str}\nRSSI: ${result.rssi}'),
            isThreeLine: true,
            trailing: FilledButton(
              onPressed: result.advertisementData.connectable
                  ? () => _connect(result)
                  : null,
              child: const Text('Connect'),
            ),
          ),
      ],
    );
  }

  Widget _buildBondedDevicesSection() {
    if (_bondedDevices.isEmpty) {
      return const Padding(
        padding: EdgeInsets.symmetric(horizontal: 16, vertical: 12),
        child: Text(
          'No bonded or system devices reported. Pair a device first, '
          'or use the manual connect field below.',
        ),
      );
    }
    return Column(
      children: <Widget>[
        for (final BluetoothDevice device in _bondedDevices)
          ListTile(
            title: Text(
              device.platformName.isNotEmpty
                  ? device.platformName
                  : device.remoteId.str,
            ),
            subtitle: Text(device.remoteId.str),
            trailing: FilledButton(
              onPressed: () => _connectDevice(device),
              child: const Text('Connect'),
            ),
          ),
      ],
    );
  }

  Widget _buildManualConnectSection() {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 4, 16, 16),
      child: Row(
        children: <Widget>[
          Expanded(
            child: TextField(
              controller: _manualRemoteIdController,
              decoration: const InputDecoration(
                labelText: 'Remote ID (MAC)',
                hintText: 'AA:BB:CC:DD:EE:FF',
                border: OutlineInputBorder(),
                isDense: true,
              ),
              textCapitalization: TextCapitalization.characters,
            ),
          ),
          const SizedBox(width: 8),
          FilledButton(
            onPressed: () => _connectByRemoteId(_manualRemoteIdController.text),
            child: const Text('Connect'),
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceDetails() {
    final BluetoothDevice? device = _selectedDevice;
    if (device == null) {
      return const Center(child: Text('No device selected.'));
    }

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: <Widget>[
        Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: <Widget>[
              Text(
                device.platformName.isNotEmpty
                    ? device.platformName
                    : device.remoteId.str,
                style: Theme.of(context).textTheme.titleLarge,
              ),
              const SizedBox(height: 8),
              Text('Remote ID: ${device.remoteId.str}'),
              Text('State: ${_connectionState.name}'),
              Text('MTU: ${device.mtuNow}'),
              const SizedBox(height: 12),
              Wrap(
                spacing: 8,
                runSpacing: 8,
                children: <Widget>[
                  FilledButton(
                    onPressed:
                        _connectionState == BluetoothConnectionState.connected
                            ? _disconnectSelectedDevice
                            : null,
                    child: const Text('Disconnect'),
                  ),
                  OutlinedButton(
                    onPressed:
                        _connectionState == BluetoothConnectionState.connected
                            ? _rediscoverServices
                            : null,
                    child: const Text('Discover Services'),
                  ),
                ],
              ),
            ],
          ),
        ),
        const Divider(height: 1),
        Expanded(
          child: _services.isEmpty
              ? const Center(child: Text('No discovered services yet.'))
              : ListView(
                  children: _services
                      .map(
                        (BluetoothService service) =>
                            _buildServiceTile(service),
                      )
                      .toList(),
                ),
        ),
      ],
    );
  }

  Widget _buildServiceTile(BluetoothService service) {
    return ExpansionTile(
      title: Text('Service ${service.uuid.str}'),
      initiallyExpanded: true,
      children: service.characteristics
          .map(
            (BluetoothCharacteristic characteristic) =>
                _buildCharacteristicTile(characteristic),
          )
          .toList(),
    );
  }

  Widget _buildCharacteristicTile(BluetoothCharacteristic characteristic) {
    final List<Widget> actionButtons = <Widget>[
      if (characteristic.properties.read)
        TextButton(
          onPressed: () => _readCharacteristic(characteristic),
          child: const Text('Read'),
        ),
      if (characteristic.properties.write)
        TextButton(
          onPressed: () => _writeCharacteristic(characteristic),
          child: const Text('Write 0x01'),
        ),
      if (characteristic.properties.notify ||
          characteristic.properties.indicate)
        TextButton(
          onPressed: () => _toggleNotify(characteristic),
          child: Text(characteristic.isNotifying ? 'Stop Notify' : 'Notify'),
        ),
    ];

    return ExpansionTile(
      title: Text('Characteristic ${characteristic.uuid.str}'),
      subtitle: Text('Value: ${_formatBytes(characteristic.lastValue)}'),
      children: <Widget>[
        Wrap(spacing: 8, children: actionButtons),
        ...characteristic.descriptors.map(_buildDescriptorTile),
      ],
    );
  }

  Widget _buildDescriptorTile(BluetoothDescriptor descriptor) {
    return ListTile(
      title: Text('Descriptor ${descriptor.uuid.str}'),
      subtitle: Text('Value: ${_formatBytes(descriptor.lastValue)}'),
      trailing: Wrap(
        spacing: 8,
        children: <Widget>[
          TextButton(
            onPressed: () => _readDescriptor(descriptor),
            child: const Text('Read'),
          ),
          TextButton(
            onPressed: () => _writeDescriptor(descriptor),
            child: const Text('Write 0x00'),
          ),
        ],
      ),
    );
  }

  String _formatBytes(List<int> bytes) {
    if (bytes.isEmpty) {
      return '(empty)';
    }
    return bytes
        .map((int value) => value.toRadixString(16).padLeft(2, '0'))
        .join(' ');
  }
}
