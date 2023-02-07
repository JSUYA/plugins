// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter/foundation.dart';
import 'package:webview_flutter_platform_interface/webview_flutter_platform_interface.dart';

// import 'tizen_webview.dart';
import 'method_channel/tizen_webview_method_channel.dart';

/// Handles all cookie operations for the current platform.
class TizenCookieManager {
  TizenCookieManager._();

  static TizenCookieManager? _instance;

  /// Gets the globally set CookieManager instance.
  static TizenCookieManager get instance =>
      _instance ??= TizenCookieManager._();

  /// Setter for the singleton value, for testing purposes only.
  @visibleForTesting
  static set instance(TizenCookieManager value) => _instance = value;

  Future<bool> clearCookies() => MethodChannelWebViewPlatform.clearCookies();

  Future<void> setCookie(WebViewCookie cookie) async {
    if (!_isValidPath(cookie.path)) {
      throw ArgumentError(
          'The path property for the provided cookie was not given a legal value.');
    }
    throw ArgumentError('no!!!!!!!!1');
    // return MethodChannelWebViewPlatform.setCookie(cookie);
  }

  bool _isValidPath(String path) {
    // Permitted ranges based on RFC6265bis: https://datatracker.ietf.org/doc/html/draft-ietf-httpbis-rfc6265bis-02#section-4.1.1
    for (final int char in path.codeUnits) {
      if ((char < 0x20 || char > 0x3A) && (char < 0x3C || char > 0x7E)) {
        return false;
      }
    }
    return true;
  }
}

/// Object specifying creation parameters for creating a [TizenWebViewCookieManager].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebViewCookieManagerCreationParams] for
/// more information.
@immutable
class TizenWebViewCookieManagerCreationParams
    extends PlatformWebViewCookieManagerCreationParams {
  /// Creates a new [TizenWebViewCookieManagerCreationParams] instance.
  const TizenWebViewCookieManagerCreationParams._(
    // This parameter prevents breaking changes later.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebViewCookieManagerCreationParams params,
  ) : super();

  /// Creates a [TizenWebViewCookieManagerCreationParams] instance based on [PlatformWebViewCookieManagerCreationParams].
  factory TizenWebViewCookieManagerCreationParams.fromPlatformWebViewCookieManagerCreationParams(
      PlatformWebViewCookieManagerCreationParams params) {
    return TizenWebViewCookieManagerCreationParams._(params);
  }
}

/// Handles all cookie operations for the Tizen platform.
class TizenWebViewCookieManager extends PlatformWebViewCookieManager {
  /// Creates a new [TizenWebViewCookieManager].
  TizenWebViewCookieManager(
    PlatformWebViewCookieManagerCreationParams params, {
    TizenCookieManager? cookieManager,
  })  : _cookieManager = cookieManager ?? TizenCookieManager.instance,
        super.implementation(
          params is TizenWebViewCookieManagerCreationParams
              ? params
              : TizenWebViewCookieManagerCreationParams
                  .fromPlatformWebViewCookieManagerCreationParams(params),
        );

  final TizenCookieManager _cookieManager;

  @override
  Future<bool> clearCookies() {
    return _cookieManager.clearCookies();
  }

  @override
  Future<void> setCookie(WebViewCookie cookie) {
    if (!_isValidPath(cookie.path)) {
      throw ArgumentError(
          'The path property for the provided cookie was not given a legal value.');
    }
    return _cookieManager.setCookie(cookie
        // cookie.domain,
        // '${Uri.encodeComponent(cookie.name)}=${Uri.encodeComponent(cookie.value)}; path=${cookie.path}',
        );
  }

  bool _isValidPath(String path) {
    // Permitted ranges based on RFC6265bis: https://datatracker.ietf.org/doc/html/draft-ietf-httpbis-rfc6265bis-02#section-4.1.1
    for (final int char in path.codeUnits) {
      if ((char < 0x20 || char > 0x3A) && (char < 0x3C || char > 0x7E)) {
        return false;
      }
    }
    return true;
  }
}
