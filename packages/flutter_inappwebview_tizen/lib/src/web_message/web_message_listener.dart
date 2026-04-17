import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

/// Object specifying creation parameters for creating a [TizenWebMessageListener].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebMessageListenerCreationParams] for
/// more information.
@immutable
class TizenWebMessageListenerCreationParams
    extends PlatformWebMessageListenerCreationParams {
  /// Creates a new [TizenWebMessageListenerCreationParams] instance.
  const TizenWebMessageListenerCreationParams({
    required this.allowedOriginRules,
    required super.jsObjectName,
    super.onPostMessage,
  });

  /// Creates a [TizenWebMessageListenerCreationParams] instance based on [PlatformWebMessageListenerCreationParams].
  factory TizenWebMessageListenerCreationParams.fromPlatformWebMessageListenerCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebMessageListenerCreationParams params,
  ) {
    return TizenWebMessageListenerCreationParams(
      allowedOriginRules: params.allowedOriginRules ?? Set.from(["*"]),
      jsObjectName: params.jsObjectName,
      onPostMessage: params.onPostMessage,
    );
  }

  @override
  final Set<String> allowedOriginRules;

  @override
  String toString() {
    return 'TizenWebMessageListenerCreationParams{jsObjectName: $jsObjectName, allowedOriginRules: $allowedOriginRules, onPostMessage: $onPostMessage}';
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformWebMessageListener}
class TizenWebMessageListener extends PlatformWebMessageListener
    with ChannelController {
  /// Constructs a [TizenWebMessageListener].
  TizenWebMessageListener(PlatformWebMessageListenerCreationParams params)
    : super.implementation(
        params is TizenWebMessageListenerCreationParams
            ? params
            : TizenWebMessageListenerCreationParams.fromPlatformWebMessageListenerCreationParams(
                params,
              ),
      ) {
    assert(
      !_tizenParams.allowedOriginRules.contains(""),
      "allowedOriginRules cannot contain empty strings",
    );
    channel = MethodChannel(
      'com.pichillilorenzo/flutter_inappwebview_web_message_listener_${_id}_${params.jsObjectName}',
    );
    handler = _handleMethod;
    initMethodCallHandler();
  }

  ///Message Listener ID used internally.
  final String _id = IdGenerator.generate();

  TizenJavaScriptReplyProxy? _replyProxy;

  TizenWebMessageListenerCreationParams get _tizenParams =>
      params as TizenWebMessageListenerCreationParams;

  Future<dynamic> _handleMethod(MethodCall call) async {
    switch (call.method) {
      case "onPostMessage":
        if (_replyProxy == null) {
          _replyProxy = TizenJavaScriptReplyProxy(
            PlatformJavaScriptReplyProxyCreationParams(
              webMessageListener: this,
            ),
          );
        }
        if (onPostMessage != null) {
          WebMessage? message = call.arguments["message"] != null
              ? WebMessage.fromMap(
                  call.arguments["message"].cast<String, dynamic>(),
                )
              : null;
          WebUri? sourceOrigin = call.arguments["sourceOrigin"] != null
              ? WebUri(call.arguments["sourceOrigin"])
              : null;
          bool isMainFrame = call.arguments["isMainFrame"];
          onPostMessage!(message, sourceOrigin, isMainFrame, _replyProxy!);
        }
        break;
      default:
        throw UnimplementedError("Unimplemented ${call.method} method");
    }
    return null;
  }

  @override
  void dispose() {
    disposeChannel();
  }

  @override
  Map<String, dynamic> toMap() {
    return {
      "id": _id,
      "jsObjectName": params.jsObjectName,
      "allowedOriginRules": _tizenParams.allowedOriginRules.toList(),
    };
  }

  @override
  Map<String, dynamic> toJson() {
    return this.toMap();
  }

  @override
  String toString() {
    return 'TizenWebMessageListener{id: ${_id}, jsObjectName: ${params.jsObjectName}, allowedOriginRules: ${params.allowedOriginRules}, replyProxy: $_replyProxy}';
  }
}

/// Object specifying creation parameters for creating a [TizenJavaScriptReplyProxy].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformJavaScriptReplyProxyCreationParams] for
/// more information.
@immutable
class TizenJavaScriptReplyProxyCreationParams
    extends PlatformJavaScriptReplyProxyCreationParams {
  /// Creates a new [TizenJavaScriptReplyProxyCreationParams] instance.
  const TizenJavaScriptReplyProxyCreationParams({
    required super.webMessageListener,
  });

  /// Creates a [TizenJavaScriptReplyProxyCreationParams] instance based on [PlatformJavaScriptReplyProxyCreationParams].
  factory TizenJavaScriptReplyProxyCreationParams.fromPlatformJavaScriptReplyProxyCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformJavaScriptReplyProxyCreationParams params,
  ) {
    return TizenJavaScriptReplyProxyCreationParams(
      webMessageListener: params.webMessageListener,
    );
  }
}

///{@macro flutter_inappwebview_platform_interface.JavaScriptReplyProxy}
class TizenJavaScriptReplyProxy extends PlatformJavaScriptReplyProxy {
  /// Constructs a [TizenWebMessageListener].
  TizenJavaScriptReplyProxy(PlatformJavaScriptReplyProxyCreationParams params)
    : super.implementation(
        params is TizenJavaScriptReplyProxyCreationParams
            ? params
            : TizenJavaScriptReplyProxyCreationParams.fromPlatformJavaScriptReplyProxyCreationParams(
                params,
              ),
      );

  TizenWebMessageListener get _tizenWebMessageListener =>
      params.webMessageListener as TizenWebMessageListener;

  @override
  Future<void> postMessage(WebMessage message) async {
    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('message', () => message.toMap());
    await _tizenWebMessageListener.channel?.invokeMethod('postMessage', args);
  }

  @override
  String toString() {
    return 'TizenJavaScriptReplyProxy{}';
  }
}
