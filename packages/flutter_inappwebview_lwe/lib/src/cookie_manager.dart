import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

import 'webview_environment/webview_environment.dart';

/// Object specifying creation parameters for creating a [LweCookieManager].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformCookieManagerCreationParams] for
/// more information.
@immutable
class LweCookieManagerCreationParams
    extends PlatformCookieManagerCreationParams {
  /// Creates a new [LweCookieManagerCreationParams] instance.
  const LweCookieManagerCreationParams({this.webViewEnvironment});

  /// Creates a [LweCookieManagerCreationParams] instance based on [PlatformCookieManagerCreationParams].
  factory LweCookieManagerCreationParams.fromPlatformCookieManagerCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformCookieManagerCreationParams params,
  ) {
    return LweCookieManagerCreationParams(
      webViewEnvironment: params.webViewEnvironment as LweWebViewEnvironment?,
    );
  }

  @override
  final LweWebViewEnvironment? webViewEnvironment;
}

///{@macro flutter_inappwebview_platform_interface.PlatformCookieManager}
class LweCookieManager extends PlatformCookieManager with ChannelController {
  /// Creates a new [LweCookieManager].
  LweCookieManager(PlatformCookieManagerCreationParams params)
    : super.implementation(
        params is LweCookieManagerCreationParams
            ? params
            : LweCookieManagerCreationParams.fromPlatformCookieManagerCreationParams(
                params,
              ),
      ) {
    channel = const MethodChannel(
      'com.pichillilorenzo/flutter_inappwebview_cookiemanager',
    );
    handler = handleMethod;
    initMethodCallHandler();
  }

  static LweCookieManager? _instance;

  ///Gets the [LweCookieManager] shared instance.
  static LweCookieManager instance({
    LweWebViewEnvironment? webViewEnvironment,
  }) {
    if (webViewEnvironment == null) {
      if (_instance == null) {
        _instance = _init();
      }
      return _instance!;
    } else {
      return LweCookieManager(
        LweCookieManagerCreationParams(webViewEnvironment: webViewEnvironment),
      );
    }
  }

  static LweCookieManager _init() {
    _instance = LweCookieManager(LweCookieManagerCreationParams());
    return _instance!;
  }

  Future<dynamic> _handleMethod(MethodCall call) async {}

  Future<T?> _invokeNativeMethod<T>(
    String method,
    Map<String, dynamic> args,
  ) async {
    try {
      return await channel?.invokeMethod<T>(method, args);
    } on MissingPluginException {
      return null;
    } on PlatformException catch (error) {
      final code = error.code.toLowerCase();
      if (code == 'unimplemented' || code == 'invalid operation') {
        return null;
      }
      rethrow;
    }
  }

  static String _sameSiteValue(HTTPCookieSameSitePolicy? sameSite) {
    switch (sameSite) {
      case HTTPCookieSameSitePolicy.LAX:
        return 'Lax';
      case HTTPCookieSameSitePolicy.STRICT:
        return 'Strict';
      case HTTPCookieSameSitePolicy.NONE:
        return 'None';
      case null:
        return '';
    }
    return '';
  }

  static String _buildCookieHeader({
    required String name,
    required String value,
    required String path,
    String? domain,
    int? expiresDate,
    int? maxAge,
    bool? isSecure,
    HTTPCookieSameSitePolicy? sameSite,
  }) {
    final parts = <String>[
      '${Uri.encodeComponent(name)}=${Uri.encodeComponent(value)}',
      'Path=$path',
    ];
    if (domain != null && domain.isNotEmpty) {
      parts.add('Domain=$domain');
    }
    if (expiresDate != null) {
      parts.add(
        'Expires=${HttpDate.format(DateTime.fromMillisecondsSinceEpoch(expiresDate, isUtc: true))}',
      );
    }
    if (maxAge != null) {
      parts.add('Max-Age=$maxAge');
    }
    if (isSecure == true) {
      parts.add('Secure');
    }
    final sameSiteValue = _sameSiteValue(sameSite);
    if (sameSiteValue.isNotEmpty) {
      parts.add('SameSite=$sameSiteValue');
    }
    return parts.join('; ');
  }

  static String _normalizeJavascriptString(dynamic value) {
    if (value == null) {
      return '';
    }
    if (value is! String) {
      return value.toString();
    }

    final trimmed = value.trim();
    if ((trimmed.startsWith('"') && trimmed.endsWith('"')) ||
        (trimmed.startsWith("'") && trimmed.endsWith("'"))) {
      try {
        return jsonDecode(trimmed) as String;
      } catch (_) {
        return trimmed.substring(1, trimmed.length - 1);
      }
    }
    return value;
  }

  static PlatformInAppWebViewController _requireController(
    PlatformInAppWebViewController? controller,
  ) {
    if (controller == null) {
      throw UnsupportedError(
        'This cookie operation requires a webViewController on flutter_inappwebview_lwe.',
      );
    }
    return controller;
  }

  static int? _defaultPort(String scheme) {
    switch (scheme) {
      case 'http':
        return 80;
      case 'https':
        return 443;
      default:
        return null;
    }
  }

  static bool _isSameOrigin(WebUri currentUrl, WebUri targetUrl) {
    final currentUri = Uri.parse(currentUrl.toString());
    final targetUri = Uri.parse(targetUrl.toString());
    return currentUri.scheme == targetUri.scheme &&
        currentUri.host == targetUri.host &&
        (currentUri.hasPort
                ? currentUri.port
                : _defaultPort(currentUri.scheme)) ==
            (targetUri.hasPort
                ? targetUri.port
                : _defaultPort(targetUri.scheme));
  }

  Future<PlatformInAppWebViewController> _requireJavascriptFallbackController({
    required WebUri url,
    required PlatformInAppWebViewController? controller,
    bool rejectHttpOnly = false,
  }) async {
    if (rejectHttpOnly) {
      throw UnsupportedError(
        'JavaScript cookie fallback does not support HttpOnly cookies on flutter_inappwebview_lwe.',
      );
    }

    final webViewController = _requireController(controller);
    final currentUrl = await webViewController.getUrl();
    if (currentUrl == null || !_isSameOrigin(currentUrl, url)) {
      throw UnsupportedError(
        'JavaScript cookie fallback only supports same-origin cookies for the currently loaded page on flutter_inappwebview_lwe.',
      );
    }
    return webViewController;
  }

  Future<bool> _setCookieWithJavascript({
    required WebUri url,
    required PlatformInAppWebViewController? controller,
    required String name,
    required String value,
    required String path,
    String? domain,
    int? expiresDate,
    int? maxAge,
    bool? isSecure,
    bool? isHttpOnly,
    HTTPCookieSameSitePolicy? sameSite,
  }) async {
    final webViewController = await _requireJavascriptFallbackController(
      url: url,
      controller: controller,
      rejectHttpOnly: isHttpOnly == true,
    );
    final cookie = _buildCookieHeader(
      name: name,
      value: value,
      path: path,
      domain: domain,
      expiresDate: expiresDate,
      maxAge: maxAge,
      isSecure: isSecure,
      sameSite: sameSite,
    );
    await webViewController.evaluateJavascript(
      source: 'document.cookie = ${jsonEncode(cookie)}; true;',
    );
    return true;
  }

  Future<List<Cookie>> _getCookiesWithJavascript({
    required WebUri url,
    required PlatformInAppWebViewController? controller,
  }) async {
    final webViewController = await _requireJavascriptFallbackController(
      url: url,
      controller: controller,
    );
    final dynamic result = await webViewController.evaluateJavascript(
      source: 'document.cookie',
    );
    final rawCookie = _normalizeJavascriptString(result);
    if (rawCookie.isEmpty) {
      return <Cookie>[];
    }

    return rawCookie
        .split(RegExp(r';\s*'))
        .where((entry) => entry.isNotEmpty && entry.contains('='))
        .map((entry) {
          final separator = entry.indexOf('=');
          return Cookie(
            name: Uri.decodeComponent(entry.substring(0, separator)),
            value: Uri.decodeComponent(entry.substring(separator + 1)),
            domain: url.host,
            path: '/',
          );
        })
        .toList();
  }

  Future<bool> _deleteCookieWithJavascript({
    required WebUri url,
    required PlatformInAppWebViewController? controller,
    required String name,
    required String path,
    String? domain,
  }) async {
    return _setCookieWithJavascript(
      url: url,
      controller: controller,
      name: name,
      value: '',
      path: path,
      domain: domain,
      expiresDate: 0,
      maxAge: 0,
      isHttpOnly: false,
    );
  }

  @override
  Future<bool> setCookie({
    required WebUri url,
    required String name,
    required String value,
    String path = "/",
    String? domain,
    int? expiresDate,
    int? maxAge,
    bool? isSecure,
    bool? isHttpOnly,
    HTTPCookieSameSitePolicy? sameSite,
    @Deprecated("Use webViewController instead")
    PlatformInAppWebViewController? iosBelow11WebViewController,
    PlatformInAppWebViewController? webViewController,
  }) async {
    assert(url.toString().isNotEmpty);
    assert(name.isNotEmpty);
    assert(value.isNotEmpty);
    assert(path.isNotEmpty);

    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('url', () => url.toString());
    args.putIfAbsent('name', () => name);
    args.putIfAbsent('value', () => value);
    args.putIfAbsent('domain', () => domain);
    args.putIfAbsent('path', () => path);
    args.putIfAbsent('expiresDate', () => expiresDate);
    args.putIfAbsent('maxAge', () => maxAge);
    args.putIfAbsent('isSecure', () => isSecure);
    args.putIfAbsent('isHttpOnly', () => isHttpOnly);
    args.putIfAbsent('sameSite', () => sameSite?.toNativeValue());
    args.putIfAbsent(
      'webViewEnvironmentId',
      () => params.webViewEnvironment?.id,
    );

    final nativeResult = await _invokeNativeMethod<bool>('setCookie', args);
    if (nativeResult != null) {
      return nativeResult;
    }

    return _setCookieWithJavascript(
      url: url,
      controller: webViewController ?? iosBelow11WebViewController,
      name: name,
      value: value,
      path: path,
      domain: domain,
      expiresDate: expiresDate,
      maxAge: maxAge,
      isSecure: isSecure,
      isHttpOnly: isHttpOnly,
      sameSite: sameSite,
    );
  }

  @override
  Future<List<Cookie>> getCookies({
    required WebUri url,
    @Deprecated("Use webViewController instead")
    PlatformInAppWebViewController? iosBelow11WebViewController,
    PlatformInAppWebViewController? webViewController,
  }) async {
    assert(url.toString().isNotEmpty);

    List<Cookie> cookies = [];

    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('url', () => url.toString());
    args.putIfAbsent(
      'webViewEnvironmentId',
      () => params.webViewEnvironment?.id,
    );
    List<dynamic>? cookieListMap = await _invokeNativeMethod<List<dynamic>>(
      'getCookies',
      args,
    );

    if (cookieListMap == null) {
      return _getCookiesWithJavascript(
        url: url,
        controller: webViewController ?? iosBelow11WebViewController,
      );
    }

    cookieListMap = cookieListMap.cast<Map<dynamic, dynamic>>();

    cookieListMap.forEach((cookieMap) {
      cookies.add(
        Cookie(
          name: cookieMap["name"],
          value: cookieMap["value"],
          expiresDate: cookieMap["expiresDate"],
          isSessionOnly: cookieMap["isSessionOnly"],
          domain: cookieMap["domain"],
          sameSite: HTTPCookieSameSitePolicy.fromNativeValue(
            cookieMap["sameSite"],
          ),
          isSecure: cookieMap["isSecure"],
          isHttpOnly: cookieMap["isHttpOnly"],
          path: cookieMap["path"],
        ),
      );
    });
    return cookies;
  }

  @override
  Future<Cookie?> getCookie({
    required WebUri url,
    required String name,
    @Deprecated("Use webViewController instead")
    PlatformInAppWebViewController? iosBelow11WebViewController,
    PlatformInAppWebViewController? webViewController,
  }) async {
    assert(url.toString().isNotEmpty);
    assert(name.isNotEmpty);

    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('url', () => url.toString());
    args.putIfAbsent(
      'webViewEnvironmentId',
      () => params.webViewEnvironment?.id,
    );
    List<dynamic> cookies =
        await _invokeNativeMethod<List<dynamic>>('getCookies', args) ?? [];
    if (cookies.isEmpty) {
      final cookieList = await _getCookiesWithJavascript(
        url: url,
        controller: webViewController ?? iosBelow11WebViewController,
      );
      for (final cookie in cookieList) {
        if (cookie.name == name) {
          return cookie;
        }
      }
      return null;
    }
    cookies = cookies.cast<Map<dynamic, dynamic>>();
    for (var i = 0; i < cookies.length; i++) {
      cookies[i] = cookies[i].cast<String, dynamic>();
      if (cookies[i]["name"] == name)
        return Cookie(
          name: cookies[i]["name"],
          value: cookies[i]["value"],
          expiresDate: cookies[i]["expiresDate"],
          isSessionOnly: cookies[i]["isSessionOnly"],
          domain: cookies[i]["domain"],
          sameSite: HTTPCookieSameSitePolicy.fromNativeValue(
            cookies[i]["sameSite"],
          ),
          isSecure: cookies[i]["isSecure"],
          isHttpOnly: cookies[i]["isHttpOnly"],
          path: cookies[i]["path"],
        );
    }
    return null;
  }

  @override
  Future<bool> deleteCookie({
    required WebUri url,
    required String name,
    String path = "/",
    String? domain,
    @Deprecated("Use webViewController instead")
    PlatformInAppWebViewController? iosBelow11WebViewController,
    PlatformInAppWebViewController? webViewController,
  }) async {
    assert(url.toString().isNotEmpty);
    assert(name.isNotEmpty);

    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('url', () => url.toString());
    args.putIfAbsent('name', () => name);
    args.putIfAbsent('domain', () => domain);
    args.putIfAbsent('path', () => path);
    args.putIfAbsent(
      'webViewEnvironmentId',
      () => params.webViewEnvironment?.id,
    );
    final nativeResult = await _invokeNativeMethod<bool>('deleteCookie', args);
    if (nativeResult != null) {
      return nativeResult;
    }

    return _deleteCookieWithJavascript(
      url: url,
      controller: webViewController ?? iosBelow11WebViewController,
      name: name,
      path: path,
      domain: domain,
    );
  }

  @override
  Future<bool> deleteCookies({
    required WebUri url,
    String path = "/",
    String? domain,
    @Deprecated("Use webViewController instead")
    PlatformInAppWebViewController? iosBelow11WebViewController,
    PlatformInAppWebViewController? webViewController,
  }) async {
    assert(url.toString().isNotEmpty);

    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent('url', () => url.toString());
    args.putIfAbsent('domain', () => domain);
    args.putIfAbsent('path', () => path);
    args.putIfAbsent(
      'webViewEnvironmentId',
      () => params.webViewEnvironment?.id,
    );
    final nativeResult = await _invokeNativeMethod<bool>('deleteCookies', args);
    if (nativeResult != null) {
      return nativeResult;
    }

    final cookies = await _getCookiesWithJavascript(
      url: url,
      controller: webViewController ?? iosBelow11WebViewController,
    );
    for (final cookie in cookies) {
      await _deleteCookieWithJavascript(
        url: url,
        controller: webViewController ?? iosBelow11WebViewController,
        name: cookie.name,
        path: path,
        domain: domain,
      );
    }
    return true;
  }

  @override
  Future<bool> deleteAllCookies() async {
    Map<String, dynamic> args = <String, dynamic>{};
    args.putIfAbsent(
      'webViewEnvironmentId',
      () => params.webViewEnvironment?.id,
    );
    return await _invokeNativeMethod<bool>('deleteAllCookies', args) ??
        await _invokeNativeMethod<bool>('clearCookies', args) ??
        false;
  }

  @override
  void dispose() {
    // empty
  }
}

extension InternalCookieManager on LweCookieManager {
  Future<dynamic> Function(MethodCall call) get handleMethod => _handleMethod;
}
