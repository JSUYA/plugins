import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

/// Object specifying creation parameters for creating a [TizenWebViewEnvironment].
///
/// Platform specific implementations can add additional fields by extending
/// this class.
@immutable
class TizenWebViewEnvironmentCreationParams
    extends PlatformWebViewEnvironmentCreationParams {
  /// Creates a new [TizenInAppWebViewControllerCreationParams] instance.
  const TizenWebViewEnvironmentCreationParams({super.settings});

  /// Creates a [TizenInAppWebViewControllerCreationParams] instance based on [PlatformInAppWebViewControllerCreationParams].
  factory TizenWebViewEnvironmentCreationParams.fromPlatformWebViewEnvironmentCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebViewEnvironmentCreationParams params,
  ) {
    return TizenWebViewEnvironmentCreationParams(settings: params.settings);
  }
}

///Controls a WebView Environment used by WebView instances.
///
///**Officially Supported Platforms/Implementations**:
///- Tizen
class TizenWebViewEnvironment extends PlatformWebViewEnvironment
    with ChannelController {
  static final MethodChannel _staticChannel = MethodChannel(
    'com.pichillilorenzo/flutter_webview_environment',
  );

  @override
  final String id = IdGenerator.generate();

  TizenWebViewEnvironment(PlatformWebViewEnvironmentCreationParams params)
    : super.implementation(
        params is TizenWebViewEnvironmentCreationParams
            ? params
            : TizenWebViewEnvironmentCreationParams.fromPlatformWebViewEnvironmentCreationParams(
                params,
              ),
      );

  static final TizenWebViewEnvironment _staticValue = TizenWebViewEnvironment(
    TizenWebViewEnvironmentCreationParams(),
  );

  factory TizenWebViewEnvironment.static() {
    return _staticValue;
  }

  _debugLog(String method, dynamic args) {
    debugLog(
      className: this.runtimeType.toString(),
      id: id,
      debugLoggingSettings: PlatformWebViewEnvironment.debugLoggingSettings,
      method: method,
      args: args,
    );
  }

  Never _unsupported(String method) {
    throw UnsupportedError(
      '$method is not implemented on flutter_inappwebview_tizen.',
    );
  }

  Future<T?> _invokeStaticMethod<T>(
    String method,
    Map<String, dynamic> args,
  ) async {
    try {
      return await _staticChannel.invokeMethod<T>(method, args);
    } on MissingPluginException {
      _unsupported(method);
    } on PlatformException catch (error) {
      if (error.code.toLowerCase() == 'unimplemented') {
        _unsupported(method);
      }
      rethrow;
    }
  }

  Future<T?> _invokeInstanceMethod<T>(
    String method,
    Map<String, dynamic> args,
  ) async {
    if (channel == null) {
      _unsupported(method);
    }
    try {
      return await channel?.invokeMethod<T>(method, args);
    } on MissingPluginException {
      _unsupported(method);
    } on PlatformException catch (error) {
      if (error.code.toLowerCase() == 'unimplemented') {
        _unsupported(method);
      }
      rethrow;
    }
  }

  Future<dynamic> _handleMethod(MethodCall call) async {
    if (PlatformWebViewEnvironment.debugLoggingSettings.enabled) {
      _debugLog(call.method, call.arguments);
    }

    switch (call.method) {
      default:
        throw UnimplementedError("Unimplemented ${call.method} method");
    }
  }

  @override
  Future<TizenWebViewEnvironment> create({
    WebViewEnvironmentSettings? settings,
  }) async {
    final env = TizenWebViewEnvironment(
      TizenWebViewEnvironmentCreationParams(settings: settings),
    );

    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('id', () => env.id);
    args.putIfAbsent('settings', () => env.settings?.toMap());
    await _invokeStaticMethod('create', args);

    env.channel = MethodChannel(
      'com.pichillilorenzo/flutter_webview_environment_${env.id}',
    );
    env.handler = env.handleMethod;
    env.initMethodCallHandler();
    return env;
  }

  @override
  Future<String?> getAvailableVersion({String? browserExecutableFolder}) async {
    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('browserExecutableFolder', () => browserExecutableFolder);
    return await _invokeStaticMethod<String>('getAvailableVersion', args);
  }

  @override
  Future<int?> compareBrowserVersions({
    required String version1,
    required String version2,
  }) async {
    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('version1', () => version1);
    args.putIfAbsent('version2', () => version2);
    return await _invokeStaticMethod<int>('compareBrowserVersions', args);
  }

  @override
  Future<void> dispose() async {
    Map<String, dynamic> args = <String, dynamic>{};
    await _invokeInstanceMethod('dispose', args);
    disposeChannel();
  }
}

extension InternalTizenWebViewEnvironment on TizenWebViewEnvironment {
  Future<dynamic> Function(MethodCall call) get handleMethod => _handleMethod;
}
