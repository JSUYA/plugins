// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_tizen/widgets.dart';
import 'package:tizen_log/tizen_log.dart';
import 'package:webview_flutter_platform_interface/webview_flutter_platform_interface.dart';
import 'tizen_webview.dart' as tizen_webview;

/// An implementation of [PlatformWebViewController] using Flutter for Web API.
class TizenWebViewController extends PlatformWebViewController {
  /// Constructs a [TizenWebViewController].
  TizenWebViewController(super.params)
      :
        //  _controllerChannel =
        // const MethodChannel('plugins.flutter.io/tizen_webview_controller'),
        _webview = tizen_webview.TizenWebview(),
        super.implementation() {
    // _controllerChannel.setMethodCallHandler(_onMethodCall);
  }

  final tizen_webview.TizenWebview _webview;

  final Map<String, JavaScriptChannelParams> _javaScriptChannelParams =
      <String, JavaScriptChannelParams>{};

  // final MethodChannel _controllerChannel;
  JavaScriptMode? _javaScriptMode;
  Color? _backgroundColor;
  bool _hasNavigationDelegate = false;
  String? _userAgent;
  String _initialUrl = 'about:blank';
  bool _isViewCreated = false;

  Future<bool?> _onMethodCall(MethodCall call) async {
    switch (call.method) {
      case 'javascriptChannelMessage':
        if (_isViewCreated) {
          final String channel = call.arguments['channel']! as String;
          final String message = call.arguments['message']! as String;
          if (_javaScriptChannelParams.containsKey(channel)) {
            _javaScriptChannelParams[channel]
                ?.onMessageReceived(JavaScriptMessage(message: message));
          }
        }
        return true;
    }

    throw MissingPluginException(
      '${call.method} was invoked but has no handler',
    );
  }

  void OnCreate() {
    _webview.OnCreate();
  }

  /// Applies the requested settings before [TizenView] is created.
  // void applySettings() {
  //   _isViewCreated = true;

  //   _controllerChannel.invokeMethod<void>(
  //       'setNavigationDelegate', _hasNavigationDelegate);

  //   _controllerChannel.invokeMethod<void>(
  //       'loadRequest', <String, String>{'url': _initialUrl});

  //   if (_javaScriptMode != null) {
  //     _controllerChannel.invokeMethod<void>(
  //         'javaScriptMode', _javaScriptMode?.index);
  //   }
  //   if (_backgroundColor != null) {
  //     _controllerChannel.invokeMethod<void>(
  //         'backgroundColor', _backgroundColor?.value);
  //   }

  //   if (_userAgent != null) {
  //     _controllerChannel.invokeMethod<void>('userAgent', _userAgent);
  //   }

  //   _javaScriptChannelParams.forEach(
  //       (String channel, JavaScriptChannelParams javaScriptChannelParams) {
  //     _controllerChannel.invokeMethod<void>(
  //         'addJavascriptChannel', javaScriptChannelParams.name);
  //   });
  // }

  @override
  Future<void> loadFile(String absoluteFilePath) async {
    assert(absoluteFilePath != null);
    _webview.loadFile(absoluteFilePath);
  }

  @override
  Future<void> loadFlutterAsset(String key) async {
    assert(key.isNotEmpty);
    _webview.loadFlutterAsset(key);
  }

  @override
  Future<void> loadHtmlString(
    String html, {
    String? baseUrl,
  }) async {
    assert(html != null);
    _webview.loadHtmlString(html, baseUrl: baseUrl);
  }

  @override
  Future<void> loadRequest(LoadRequestParams params) async {
    if (!params.uri.hasScheme) {
      throw ArgumentError(
          'LoadRequestParams#uri is required to have a scheme.');
    }

    switch (params.method) {
      case LoadRequestMethod.get:
        _webview.loadRequestPost(params.uri.toString());
        return;
      case LoadRequestMethod.post:
        break;
    }
    // The enum comes from a different package, which could get a new value at
    // any time, so a fallback case is necessary. Since there is no reasonable
    // default behavior, throw to alert the client that they need an updated
    // version. This is deliberately outside the switch rather than a `default`
    // so that the linter will flag the switch as needing an update.
    // ignore: dead_code
    throw UnimplementedError(
        'This version of `TizenWebViewController` currently has no '
        'implementation for HTTP method ${params.method.serialize()} in '
        'loadRequest.');
  }

  @override
  Future<String?> currentUrl() => _webview.currentUrl();

  @override
  Future<bool> canGoBack() =>
      _webview.canGoBack().then((bool? result) => result!);

  @override
  Future<bool> canGoForward() =>
      _webview.canGoForward().then((bool? result) => result!);

  @override
  Future<void> goBack() => _webview.goBack();

  @override
  Future<void> goForward() => _webview.goForward();

  @override
  Future<void> reload() => _webview.reload();

  @override
  Future<void> clearCache() => _webview.clearCache();

  @override
  Future<void> clearLocalStorage() async {
    Log.error('WebviewFlutterTizen', 'clearLocalStorage is not implemented.');
  }

  @override
  Future<void> setJavaScriptMode(JavaScriptMode javaScriptMode) =>
      _webview.setJavaScriptMode(javaScriptMode.index);

  @override
  Future<String?> getTitle() =>
      _webview.getTitle().then((String? result) => result!);

  @override
  Future<void> scrollTo(int x, int y) => _webview.scrollTo(x, y);

  @override
  Future<void> scrollBy(int x, int y) => _webview.scrollBy(x, y);

  @override
  Future<Offset> getScrollPosition() =>
      _webview.getScrollPosition().then((Offset? result) => result!);

  @override
  Future<void> setBackgroundColor(Color color) =>
      _webview.setBackgroundColor(color);

  @override
  Future<void> setPlatformNavigationDelegate(
      covariant TizenNavigationDelegate handler) async {
    handler.setOnLoadRequest(loadRequest);
    _webview.setHasNavigationDelegate(true);
  }

  @override
  Future<void> addJavaScriptChannel(
    JavaScriptChannelParams javaScriptChannelParams,
  ) async {
    // When JavaScript channel with the same name exists make sure to remove it
    // before registering the new channel.
    if (_javaScriptChannelParams.containsKey(javaScriptChannelParams.name)) {
      _javaScriptChannelParams.remove(javaScriptChannelParams.name);
    }

    _javaScriptChannelParams[javaScriptChannelParams.name] =
        javaScriptChannelParams;

    _webview.addJavaScriptChannel(javaScriptChannelParams);
  }

  @override
  Future<void> runJavaScript(String javaScript) =>
      _webview.runJavaScript(javaScript);

  @override
  Future<Object> runJavaScriptReturningResult(String javaScript) => _webview
      .runJavaScriptReturningResult(javaScript)
      .then((Object? result) => result!);

  @override
  Future<void> setUserAgent(String? userAgent) =>
      _webview.setUserAgent(userAgent);
}

/// An implementation of [PlatformWebViewWidget] with the WebKit api.
class TizenWebViewWidget extends PlatformWebViewWidget {
  /// Constructs a [WebKitWebViewWidget].
  TizenWebViewWidget(super.params) : super.implementation();

  @override
  Widget build(BuildContext context) {
    return TizenView(
      key: params.key,
      viewType: 'plugins.flutter.io/webview',
      onPlatformViewCreated: (_) {
        final TizenWebViewController controller =
            params.controller as TizenWebViewController;
        controller.OnCreate();
      },
      layoutDirection: params.layoutDirection,
      gestureRecognizers: params.gestureRecognizers,
    );
  }
}

/// Signature for the `loadRequest` callback responsible for loading the [url]
/// after a navigation request has been approved.
typedef LoadRequestCallback = Future<void> Function(LoadRequestParams params);

/// Error returned in `WebView.onWebResourceError` when a web resource loading error has occurred.
@immutable
class TizenWebResourceError extends WebResourceError {
  /// Creates a new [TizenWebResourceError].
  TizenWebResourceError._({
    required super.errorCode,
    required super.description,
    super.isForMainFrame,
    this.failingUrl,
  }) : super(
          errorType: _errorCodeToErrorType(errorCode),
        );

  /// Unknown error.
  static const int errorUnknown = 0;

  /// Failed to file I/O.
  static const int errorFailedFileIO = 3;

  /// Cannot connect to Network.
  static const int errorCantConnect = 4;

  /// Fail to look up host from DNS.
  static const int errorCantHostLookup = 5;

  /// Fail to SSL/TLS handshake.
  static const int errorFailedSslHandshake = 6;

  /// Connection timeout.
  static const int errorRequestTimeout = 8;

  /// Too many redirects.
  static const int errorTooManyRedirect = 9;

  /// Too many requests during this load.
  static const int errorTooManyRequests = 10;

  /// Malformed url.
  static const int errorBadUrl = 11;

  /// Unsupported scheme
  static const int errorUnsupportedScheme = 12;

  /// User authentication failed on server.
  static const int errorAuthentication = 13;

  /// Gets the URL for which the failing resource request was made.
  final String? failingUrl;

  static WebResourceErrorType? _errorCodeToErrorType(int errorCode) {
    switch (errorCode) {
      case errorUnknown:
        return WebResourceErrorType.unknown;
      case errorFailedFileIO:
        return WebResourceErrorType.file;
      case errorCantConnect:
        return WebResourceErrorType.connect;
      case errorCantHostLookup:
        return WebResourceErrorType.hostLookup;
      case errorFailedSslHandshake:
        return WebResourceErrorType.failedSslHandshake;
      case errorRequestTimeout:
        return WebResourceErrorType.timeout;
      case errorTooManyRedirect:
        return WebResourceErrorType.redirectLoop;
      case errorTooManyRequests:
        return WebResourceErrorType.tooManyRequests;
      case errorBadUrl:
        return WebResourceErrorType.badUrl;
      case errorUnsupportedScheme:
        return WebResourceErrorType.unsupportedScheme;
      case errorAuthentication:
        return WebResourceErrorType.authentication;
    }

    throw ArgumentError(
      'Could not find a WebResourceErrorType for errorCode: $errorCode',
    );
  }
}

/// A place to register callback methods responsible to handle navigation events
/// triggered by the [android_webview.WebView].
class TizenNavigationDelegate extends PlatformNavigationDelegate {
  /// Creates a new [TizenNavigationDelegate].
  TizenNavigationDelegate(super.params)
      : _navigationDelegateChannel = const MethodChannel(
            'plugins.flutter.io/tizen_webview_navigation_delegate'),
        super.implementation() {
    _navigationDelegateChannel.setMethodCallHandler(_onMethodCall);
  }

  Future<bool?> _onMethodCall(MethodCall call) async {
    final WeakReference<TizenNavigationDelegate> weakThis =
        WeakReference<TizenNavigationDelegate>(this);
    switch (call.method) {
      case 'navigationRequest':
        if (weakThis.target != null) {
          return weakThis.target!._handleNavigation(
              call.arguments['url']! as String,
              isForMainFrame: call.arguments['isForMainFrame']! as bool);
        }
        return null;
      case 'onPageFinished':
        if (weakThis.target?._onPageFinished != null) {
          weakThis.target!._onPageFinished!(call.arguments['url']! as String);
        }
        return null;
      case 'onProgress':
        if (weakThis.target?._onProgress != null) {
          weakThis.target!._onProgress!(call.arguments['progress']! as int);
        }
        return null;
      case 'onPageStarted':
        if (weakThis.target?._onPageStarted != null) {
          weakThis.target!._onPageStarted!(call.arguments['url']! as String);
        }
        return null;
      case 'onWebResourceError':
        if (weakThis.target?._onWebResourceError != null) {
          weakThis.target!._onWebResourceError!(TizenWebResourceError._(
            errorCode: call.arguments['errorCode']! as int,
            description: call.arguments['description']! as String,
            failingUrl: call.arguments['failingUrl']! as String,
            isForMainFrame: true,
          ));
        }
        return null;
    }

    throw MissingPluginException(
      '${call.method} was invoked but has no handler',
    );
  }

  final MethodChannel _navigationDelegateChannel;

  PageEventCallback? _onPageFinished;
  PageEventCallback? _onPageStarted;
  ProgressCallback? _onProgress;
  WebResourceErrorCallback? _onWebResourceError;
  NavigationRequestCallback? _onNavigationRequest;
  LoadRequestCallback? _onLoadRequest;

  bool _handleNavigation(String url, {required bool isForMainFrame}) {
    final LoadRequestCallback? onLoadRequest = _onLoadRequest;
    final NavigationRequestCallback? onNavigationRequest = _onNavigationRequest;

    if (onNavigationRequest == null) {
      return true;
    }
    if (onLoadRequest == null) {
      return false;
    }

    final FutureOr<NavigationDecision> returnValue =
        onNavigationRequest(NavigationRequest(
      url: url,
      isMainFrame: isForMainFrame,
    ));

    if (returnValue is NavigationDecision &&
        returnValue == NavigationDecision.navigate) {
      return true;
    } else if (returnValue is Future<NavigationDecision>) {
      returnValue.then((NavigationDecision shouldLoadUrl) {
        if (shouldLoadUrl == NavigationDecision.navigate) {
          return true;
        }
      });
    }
    return false;
  }

  /// Invoked when loading the url after a navigation request is approved.
  Future<void> setOnLoadRequest(
    LoadRequestCallback onLoadRequest,
  ) async {
    _onLoadRequest = onLoadRequest;
  }

  @override
  Future<void> setOnNavigationRequest(
    NavigationRequestCallback onNavigationRequest,
  ) async {
    _onNavigationRequest = onNavigationRequest;
  }

  @override
  Future<void> setOnPageStarted(
    PageEventCallback onPageStarted,
  ) async {
    _onPageStarted = onPageStarted;
  }

  @override
  Future<void> setOnPageFinished(
    PageEventCallback onPageFinished,
  ) async {
    _onPageFinished = onPageFinished;
  }

  @override
  Future<void> setOnProgress(
    ProgressCallback onProgress,
  ) async {
    _onProgress = onProgress;
  }

  @override
  Future<void> setOnWebResourceError(
    WebResourceErrorCallback onWebResourceError,
  ) async {
    _onWebResourceError = onWebResourceError;
  }
}
