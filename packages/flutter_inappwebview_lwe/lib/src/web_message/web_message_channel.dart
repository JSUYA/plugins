import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';
import 'web_message_port.dart';

/// Object specifying creation parameters for creating a [LweWebMessageChannel].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebMessageChannelCreationParams] for
/// more information.
@immutable
class LweWebMessageChannelCreationParams
    extends PlatformWebMessageChannelCreationParams {
  /// Creates a new [LweWebMessageChannelCreationParams] instance.
  const LweWebMessageChannelCreationParams({
    required super.id,
    required super.port1,
    required super.port2,
  });

  /// Creates a [LweWebMessageChannelCreationParams] instance based on [PlatformWebMessageChannelCreationParams].
  factory LweWebMessageChannelCreationParams.fromPlatformWebMessageChannelCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebMessageChannelCreationParams params,
  ) {
    return LweWebMessageChannelCreationParams(
      id: params.id,
      port1: params.port1,
      port2: params.port2,
    );
  }

  @override
  String toString() {
    return 'LweWebMessageChannelCreationParams{id: $id, port1: $port1, port2: $port2}';
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformWebMessageChannel}
class LweWebMessageChannel extends PlatformWebMessageChannel
    with ChannelController {
  /// Constructs a [LweWebMessageChannel].
  LweWebMessageChannel(PlatformWebMessageChannelCreationParams params)
    : super.implementation(
        params is LweWebMessageChannelCreationParams
            ? params
            : LweWebMessageChannelCreationParams.fromPlatformWebMessageChannelCreationParams(
                params,
              ),
      ) {
    channel = MethodChannel(
      'com.pichillilorenzo/flutter_inappwebview_web_message_channel_${params.id}',
    );
    handler = _handleMethod;
    initMethodCallHandler();
  }

  static final LweWebMessageChannel _staticValue = LweWebMessageChannel(
    LweWebMessageChannelCreationParams(
      id: '',
      port1: LweWebMessagePort(LweWebMessagePortCreationParams(index: 0)),
      port2: LweWebMessagePort(LweWebMessagePortCreationParams(index: 1)),
    ),
  );

  /// Provide static access.
  factory LweWebMessageChannel.static() {
    return _staticValue;
  }

  LweWebMessagePort get _lwePort1 => port1 as LweWebMessagePort;

  LweWebMessagePort get _lwePort2 => port2 as LweWebMessagePort;

  static LweWebMessageChannel? _fromMap(Map<String, dynamic>? map) {
    if (map == null) {
      return null;
    }
    var webMessageChannel = LweWebMessageChannel(
      LweWebMessageChannelCreationParams(
        id: map["id"],
        port1: LweWebMessagePort(LweWebMessagePortCreationParams(index: 0)),
        port2: LweWebMessagePort(LweWebMessagePortCreationParams(index: 1)),
      ),
    );
    webMessageChannel._lwePort1.webMessageChannel = webMessageChannel;
    webMessageChannel._lwePort2.webMessageChannel = webMessageChannel;
    return webMessageChannel;
  }

  Future<dynamic> _handleMethod(MethodCall call) async {
    switch (call.method) {
      case "onMessage":
        int index = call.arguments["index"];
        var port = index == 0 ? _lwePort1 : _lwePort2;
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
  LweWebMessageChannel? fromMap(Map<String, dynamic>? map) {
    return _fromMap(map);
  }

  @override
  void dispose() {
    disposeChannel();
  }

  @override
  String toString() {
    return 'LweWebMessageChannel{id: $id, port1: $port1, port2: $port2}';
  }
}

extension InternalWebMessageChannel on LweWebMessageChannel {
  MethodChannel? get internalChannel => channel;
}
