import 'dart:io';

import 'package:flutter/services.dart';

///Platform native utilities
class PlatformUtil {
  static PlatformUtil? _instance;
  static const MethodChannel _channel = MethodChannel(
    'com.pichillilorenzo/flutter_inappwebview_platformutil',
  );

  PlatformUtil._();

  ///Get [PlatformUtil] instance.
  static PlatformUtil instance() {
    return (_instance != null) ? _instance! : _init();
  }

  static PlatformUtil _init() {
    _channel.setMethodCallHandler((call) async {
      try {
        return await _handleMethod(call);
      } on Error catch (e) {
        print(e);
        print(e.stackTrace);
      }
    });
    _instance = PlatformUtil._();
    return _instance!;
  }

  static Future<dynamic> _handleMethod(MethodCall call) async {}

  Future<T?> _invokeNativeMethod<T>(
    String method,
    Map<String, dynamic> args,
  ) async {
    try {
      return await _channel.invokeMethod<T>(method, args);
    } on MissingPluginException {
      return null;
    } on PlatformException catch (error) {
      if (error.code.toLowerCase() == 'unimplemented') {
        return null;
      }
      rethrow;
    }
  }

  String? _cachedSystemVersion;

  ///Get current platform system version.
  Future<String> getSystemVersion() async {
    if (_cachedSystemVersion != null) {
      return _cachedSystemVersion!;
    }
    Map<String, dynamic> args = <String, dynamic>{};
    _cachedSystemVersion =
        await _invokeNativeMethod<String>('getSystemVersion', args) ??
        Platform.operatingSystemVersion;
    return _cachedSystemVersion!;
  }

  ///Format date.
  Future<String> formatDate({
    required DateTime date,
    required String format,
    String locale = "en_US",
    String timezone = "UTC",
  }) async {
    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('date', () => date.millisecondsSinceEpoch);
    args.putIfAbsent('format', () => format);
    args.putIfAbsent('locale', () => locale);
    args.putIfAbsent('timezone', () => timezone);
    final nativeValue = await _invokeNativeMethod<String>('formatDate', args);
    if (nativeValue != null) {
      return nativeValue;
    }

    if (timezone == 'UTC' &&
        (format == "EEE, dd MMM yyyy HH:mm:ss 'GMT'" ||
            format == 'EEE, dd MMM yyyy HH:mm:ss zzz')) {
      return HttpDate.format(date.toUtc());
    }

    throw UnsupportedError(
      'formatDate is not implemented on flutter_inappwebview_lwe for format "$format".',
    );
  }

  ///Get cookie expiration date used by Web platform.
  Future<String> getWebCookieExpirationDate({required DateTime date}) async {
    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('date', () => date.millisecondsSinceEpoch);
    return await _invokeNativeMethod<String>(
          'getWebCookieExpirationDate',
          args,
        ) ??
        HttpDate.format(date.toUtc());
  }
}
