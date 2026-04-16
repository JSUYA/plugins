// ignore_for_file: public_member_api_docs

import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_blue_plus_platform_interface/flutter_blue_plus_platform_interface.dart';

final class FlutterBluePlusTizen extends FlutterBluePlusPlatform {
  FlutterBluePlusTizen._();

  static FlutterBluePlusTizen? _instance;

  static const MethodChannel _methodChannel = MethodChannel(
    'plugins.flutter.io/flutter_blue_plus_tizen/methods',
  );
  static const EventChannel _eventChannel = EventChannel(
    'plugins.flutter.io/flutter_blue_plus_tizen/events',
  );

  // Subscribing to an EventChannel requires a binary messenger, which is only
  // available after WidgetsFlutterBinding.ensureInitialized() runs. Plugin
  // registration (register/registerWith) is invoked before runApp(), so the
  // event stream and its log forwarder must be initialized lazily on first
  // access instead of in the constructor.
  late final Stream<Map<dynamic, dynamic>> _events = _buildEventStream();

  Stream<Map<dynamic, dynamic>> _buildEventStream() {
    final Stream<Map<dynamic, dynamic>> events = _eventChannel
        .receiveBroadcastStream()
        .map<Map<dynamic, dynamic>>(
          (dynamic event) =>
              Map<dynamic, dynamic>.from(event as Map<dynamic, dynamic>),
        )
        .asBroadcastStream();
    events
        .where((Map<dynamic, dynamic> event) => event['type'] == 'log')
        .map(
          (Map<dynamic, dynamic> event) => event['message']?.toString() ?? '',
        )
        .where((String message) => message.isNotEmpty)
        .listen(FlutterBluePlusPlatform.log);
    return events;
  }

  static void register() {
    _instance ??= FlutterBluePlusTizen._();
    FlutterBluePlusPlatform.instance = _instance!;
  }

  static void registerWith() => register();

  /// Raw native event stream, shared with the instance used by the platform
  /// interface. Exposed so on-device diagnostics in the example app (or other
  /// tools) can observe events without creating a second EventChannel — which
  /// would cause the native StreamHandler to overwrite its single event_sink_
  /// on the latest OnListen call and starve the platform-interface stream.
  static Stream<Map<dynamic, dynamic>> get rawEvents {
    register();
    return _instance!._events;
  }

  Stream<T> _typedEventStream<T>(
    String type,
    T Function(Map<dynamic, dynamic> data) fromMap,
  ) {
    return _events
        .where((Map<dynamic, dynamic> event) => event['type'] == type)
        .map((Map<dynamic, dynamic> event) {
      final Object? data = event['data'];
      return fromMap(
        data is Map<dynamic, dynamic>
            ? Map<dynamic, dynamic>.from(data)
            : <dynamic, dynamic>{},
      );
    });
  }

  Future<bool> _invokeBool(
    String method, [
    Map<dynamic, dynamic>? arguments,
  ]) async {
    return await _methodChannel.invokeMethod<bool>(method, arguments) ?? false;
  }

  Map<dynamic, dynamic> _remoteIdArguments(DeviceIdentifier remoteId) {
    return <dynamic, dynamic>{'remote_id': remoteId.str};
  }

  @override
  Stream<BmBluetoothAdapterState> get onAdapterStateChanged =>
      _typedEventStream<BmBluetoothAdapterState>(
        'adapter_state_changed',
        BmBluetoothAdapterState.fromMap,
      );

  @override
  Stream<BmBondStateResponse> get onBondStateChanged =>
      _typedEventStream<BmBondStateResponse>(
        'bond_state_changed',
        BmBondStateResponse.fromMap,
      );

  @override
  Stream<BmCharacteristicData> get onCharacteristicReceived =>
      _typedEventStream<BmCharacteristicData>(
        'characteristic_received',
        BmCharacteristicData.fromMap,
      );

  @override
  Stream<BmCharacteristicData> get onCharacteristicWritten =>
      _typedEventStream<BmCharacteristicData>(
        'characteristic_written',
        BmCharacteristicData.fromMap,
      );

  @override
  Stream<BmConnectionStateResponse> get onConnectionStateChanged =>
      _typedEventStream<BmConnectionStateResponse>(
        'connection_state_changed',
        BmConnectionStateResponse.fromMap,
      );

  @override
  Stream<BmDescriptorData> get onDescriptorRead =>
      _typedEventStream<BmDescriptorData>(
        'descriptor_read',
        BmDescriptorData.fromMap,
      );

  @override
  Stream<BmDescriptorData> get onDescriptorWritten =>
      _typedEventStream<BmDescriptorData>(
        'descriptor_written',
        BmDescriptorData.fromMap,
      );

  @override
  Stream<BmDetachedFromEngineResponse> get onDetachedFromEngine =>
      const Stream<BmDetachedFromEngineResponse>.empty();

  @override
  Stream<BmDiscoverServicesResult> get onDiscoveredServices =>
      _typedEventStream<BmDiscoverServicesResult>(
        'discovered_services',
        BmDiscoverServicesResult.fromMap,
      );

  @override
  Stream<BmMtuChangedResponse> get onMtuChanged =>
      _typedEventStream<BmMtuChangedResponse>(
        'mtu_changed',
        BmMtuChangedResponse.fromMap,
      );

  @override
  Stream<BmNameChanged> get onNameChanged => _typedEventStream<BmNameChanged>(
        'name_changed',
        BmNameChanged.fromMap,
      );

  @override
  Stream<BmReadRssiResult> get onReadRssi =>
      _typedEventStream<BmReadRssiResult>(
        'read_rssi',
        BmReadRssiResult.fromMap,
      );

  @override
  Stream<BmScanResponse> get onScanResponse =>
      _typedEventStream<BmScanResponse>(
        'scan_response',
        BmScanResponse.fromMap,
      );

  @override
  Stream<BmBluetoothDevice> get onServicesReset =>
      _typedEventStream<BmBluetoothDevice>(
        'services_reset',
        BmBluetoothDevice.fromMap,
      );

  @override
  Stream<BmTurnOnResponse> get onTurnOnResponse =>
      _typedEventStream<BmTurnOnResponse>(
        'turn_on_response',
        BmTurnOnResponse.fromMap,
      );

  @override
  Future<bool> clearGattCache(BmClearGattCacheRequest request) {
    return _invokeBool('clearGattCache', _remoteIdArguments(request.remoteId));
  }

  @override
  Future<bool> connect(BmConnectRequest request) {
    return _invokeBool('connect', request.toMap());
  }

  @override
  Future<bool> createBond(BmCreateBondRequest request) {
    return _invokeBool('createBond', request.toMap());
  }

  @override
  Future<bool> disconnect(BmDisconnectRequest request) {
    return _invokeBool('disconnect', _remoteIdArguments(request.remoteId));
  }

  @override
  Future<bool> discoverServices(BmDiscoverServicesRequest request) {
    return _invokeBool(
      'discoverServices',
      _remoteIdArguments(request.remoteId),
    );
  }

  @override
  Future<BmBluetoothAdapterName> getAdapterName(
    BmBluetoothAdapterNameRequest request,
  ) async {
    final String adapterName =
        await _methodChannel.invokeMethod<String>('getAdapterName') ?? '';
    return BmBluetoothAdapterName(adapterName: adapterName);
  }

  @override
  Future<BmBluetoothAdapterState> getAdapterState(
    BmBluetoothAdapterStateRequest request,
  ) async {
    final Map<dynamic, dynamic>? response = await _methodChannel
        .invokeMapMethod<dynamic, dynamic>('getAdapterState');
    return BmBluetoothAdapterState.fromMap(
      response ?? <dynamic, dynamic>{'adapter_state': 0},
    );
  }

  @override
  Future<BmBondStateResponse> getBondState(BmBondStateRequest request) async {
    final Map<dynamic, dynamic>? response =
        await _methodChannel.invokeMapMethod<dynamic, dynamic>(
      'getBondState',
      _remoteIdArguments(request.remoteId),
    );
    return BmBondStateResponse.fromMap(
      response ??
          <dynamic, dynamic>{
            'remote_id': request.remoteId.str,
            'bond_state': BmBondStateEnum.none.index,
          },
    );
  }

  @override
  Future<BmDevicesList> getBondedDevices(BmBondedDevicesRequest request) async {
    final Map<dynamic, dynamic>? response =
        await _methodChannel.invokeMapMethod<dynamic, dynamic>(
      'getBondedDevices',
    );
    return BmDevicesList.fromMap(
      response ?? <dynamic, dynamic>{'devices': <dynamic>[]},
    );
  }

  @override
  Future<PhySupport> getPhySupport(PhySupportRequest request) async {
    final Map<dynamic, dynamic>? response =
        await _methodChannel.invokeMapMethod<dynamic, dynamic>('getPhySupport');
    return PhySupport.fromMap(
      response ?? <dynamic, dynamic>{'le_2M': false, 'le_coded': false},
    );
  }

  @override
  Future<BmDevicesList> getSystemDevices(BmSystemDevicesRequest request) async {
    final Map<dynamic, dynamic>? response =
        await _methodChannel.invokeMapMethod<dynamic, dynamic>(
      'getSystemDevices',
      request.toMap(),
    );
    return BmDevicesList.fromMap(
      response ?? <dynamic, dynamic>{'devices': <dynamic>[]},
    );
  }

  @override
  Future<bool> isSupported(BmIsSupportedRequest request) {
    return _invokeBool('isSupported');
  }

  @override
  Future<bool> readCharacteristic(BmReadCharacteristicRequest request) {
    return _invokeBool('readCharacteristic', request.toMap());
  }

  @override
  Future<bool> readDescriptor(BmReadDescriptorRequest request) {
    return _invokeBool('readDescriptor', request.toMap());
  }

  @override
  Future<bool> readRssi(BmReadRssiRequest request) {
    return _invokeBool('readRssi', _remoteIdArguments(request.remoteId));
  }

  @override
  Future<bool> removeBond(BmRemoveBondRequest request) {
    return _invokeBool('removeBond', _remoteIdArguments(request.remoteId));
  }

  @override
  Future<bool> requestConnectionPriority(
    BmConnectionPriorityRequest request,
  ) {
    return _invokeBool('requestConnectionPriority', request.toMap());
  }

  @override
  Future<bool> requestMtu(BmMtuChangeRequest request) {
    return _invokeBool('requestMtu', request.toMap());
  }

  @override
  Future<bool> setLogLevel(BmSetLogLevelRequest request) {
    return _invokeBool('setLogLevel', <dynamic, dynamic>{
      'log_level': request.logLevel.index,
      'log_color': request.logColor,
    });
  }

  @override
  Future<bool> setNotifyValue(BmSetNotifyValueRequest request) {
    return _invokeBool('setNotifyValue', request.toMap());
  }

  @override
  Future<bool> setOptions(BmSetOptionsRequest request) {
    return _invokeBool('setOptions', request.toMap());
  }

  @override
  Future<bool> setPreferredPhy(BmPreferredPhy request) {
    return _invokeBool('setPreferredPhy', request.toMap());
  }

  @override
  Future<bool> startScan(BmScanSettings request) {
    return _invokeBool('startScan', request.toMap());
  }

  @override
  Future<bool> stopScan(BmStopScanRequest request) {
    return _invokeBool('stopScan');
  }

  @override
  Future<bool> turnOff(BmTurnOffRequest request) {
    return _invokeBool('turnOff');
  }

  @override
  Future<bool> turnOn(BmTurnOnRequest request) {
    return _invokeBool('turnOn');
  }

  @override
  Future<bool> writeCharacteristic(BmWriteCharacteristicRequest request) {
    return _invokeBool('writeCharacteristic', request.toMap());
  }

  @override
  Future<bool> writeDescriptor(BmWriteDescriptorRequest request) {
    return _invokeBool('writeDescriptor', request.toMap());
  }
}
