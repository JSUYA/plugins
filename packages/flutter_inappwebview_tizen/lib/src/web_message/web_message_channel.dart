import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';
import 'web_message_port.dart';

/// Object specifying creation parameters for creating a [TizenWebMessageChannel].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebMessageChannelCreationParams] for
/// more information.
@immutable
class TizenWebMessageChannelCreationParams
    extends PlatformWebMessageChannelCreationParams {
  /// Creates a new [TizenWebMessageChannelCreationParams] instance.
  const TizenWebMessageChannelCreationParams({
    required super.id,
    required super.port1,
    required super.port2,
  });

  /// Creates a [TizenWebMessageChannelCreationParams] instance based on [PlatformWebMessageChannelCreationParams].
  factory TizenWebMessageChannelCreationParams.fromPlatformWebMessageChannelCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebMessageChannelCreationParams params,
  ) {
    return TizenWebMessageChannelCreationParams(
      id: params.id,
      port1: params.port1,
      port2: params.port2,
    );
  }

  @override
  String toString() {
    return 'TizenWebMessageChannelCreationParams{id: $id, port1: $port1, port2: $port2}';
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformWebMessageChannel}
class TizenWebMessageChannel extends PlatformWebMessageChannel
    with ChannelController {
  /// Constructs a [TizenWebMessageChannel].
  TizenWebMessageChannel(PlatformWebMessageChannelCreationParams params)
    : super.implementation(
        params is TizenWebMessageChannelCreationParams
            ? params
            : TizenWebMessageChannelCreationParams.fromPlatformWebMessageChannelCreationParams(
                params,
              ),
      ) {
    channel = MethodChannel(
      'com.pichillilorenzo/flutter_inappwebview_web_message_channel_${params.id}',
    );
    handler = _handleMethod;
    initMethodCallHandler();
  }

  static final TizenWebMessageChannel _staticValue = TizenWebMessageChannel(
    TizenWebMessageChannelCreationParams(
      id: '',
      port1: TizenWebMessagePort(TizenWebMessagePortCreationParams(index: 0)),
      port2: TizenWebMessagePort(TizenWebMessagePortCreationParams(index: 1)),
    ),
  );

  /// Provide static access.
  factory TizenWebMessageChannel.static() {
    return _staticValue;
  }

  TizenWebMessagePort get _tizenPort1 => port1 as TizenWebMessagePort;

  TizenWebMessagePort get _tizenPort2 => port2 as TizenWebMessagePort;

  static TizenWebMessageChannel? _fromMap(Map<String, dynamic>? map) {
    if (map == null) {
      return null;
    }
    var webMessageChannel = TizenWebMessageChannel(
      TizenWebMessageChannelCreationParams(
        id: map["id"],
        port1: TizenWebMessagePort(TizenWebMessagePortCreationParams(index: 0)),
        port2: TizenWebMessagePort(TizenWebMessagePortCreationParams(index: 1)),
      ),
    );
    webMessageChannel._tizenPort1.webMessageChannel = webMessageChannel;
    webMessageChannel._tizenPort2.webMessageChannel = webMessageChannel;
    return webMessageChannel;
  }

  Future<dynamic> _handleMethod(MethodCall call) async {
    switch (call.method) {
      case "onMessage":
        int index = call.arguments["index"];
        var port = index == 0 ? _tizenPort1 : _tizenPort2;
        if (port.onMessage != null) {
          WebMessage? message = call.arguments["message"] != null
              ? WebMessage.fromMap(
                  call.arguments["message"].cast<String, dynamic>(),
                )
              : null;
          port.onMessage!(message);
        }
        break;
      default:
        throw UnimplementedError("Unimplemented ${call.method} method");
    }
    return null;
  }

  @override
  TizenWebMessageChannel? fromMap(Map<String, dynamic>? map) {
    return _fromMap(map);
  }

  @override
  void dispose() {
    disposeChannel();
  }

  @override
  String toString() {
    return 'TizenWebMessageChannel{id: $id, port1: $port1, port2: $port2}';
  }
}

extension InternalWebMessageChannel on TizenWebMessageChannel {
  MethodChannel? get internalChannel => channel;
}
