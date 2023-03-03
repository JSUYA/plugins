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

class TizenSettings {}

class TizenWebview {
  TizenWebview()
      : _controllerChannel =
            const MethodChannel('plugins.flutter.io/tizen_webview_controller') {
    _controllerChannel.setMethodCallHandler(_onMethodCall);
  }

  final bool _isCreated = false;
  final MethodChannel _controllerChannel;
  JavaScriptMode? _javaScriptMode;
  Color? _backgroundColor;
  bool _hasNavigationDelegate = false;
  String? _userAgent;
  final String _initialUrl = 'about:blank';

  void OnCreate() {
    _isCreated = true;
    _applySettings();
  }

  Future<bool?> _onMethodCall(MethodCall call) async {
    switch (call.method) {
      case 'javaScriptChannelMessage':
        Log.error('WebviewFlutterTizen', 'CJS OnMethodCall ${call.method}');
        final String channel = call.arguments['channel']! as String;
        final String message = call.arguments['message']! as String;
        if (_javaScriptChannelParams.containsKey(channel)) {
          _javaScriptChannelParams[channel]
              ?.onMessageReceived(JavaScriptMessage(message: message));
        }

        return true;
    }

    throw MissingPluginException(
      '${call.method} was invoked but has no handler',
    );
  }

  final Map<String, dynamic> _settings = <String, dynamic>{};
  // TizenSettings _settings;

  Future<T?> _invokeChannelMethod<T>(String method, [dynamic arguments]) async {
    if (!_isCreated) {
      _settings[method] = arguments;
      return Future<T?>(() => null);
    }

    return _controllerChannel.invokeMethod<T>(method, arguments);
  }

  /// Applies the requested settings before [TizenView] is created.
  void _applySettings() {
    _invokeChannelMethod<void>('setNavigationDelegate', _hasNavigationDelegate);

    _settings.forEach((String method, dynamic arguments) {
      Log.error('WebviewFlutterTizen', 'CJS ${method}');
      _controllerChannel.invokeMethod<void>(method, arguments);
    });
    _settings.clear();

    // _invokeChannelMethod<void>(
    //     'loadRequest', <String, String>{'url': _initialUrl});

    // if (_javaScriptMode != null) {
    //   _invokeChannelMethod<void>('javaScriptMode', _javaScriptMode?.index);
    // }
    // if (_backgroundColor != null) {
    //   _invokeChannelMethod<void>('backgroundColor', _backgroundColor?.value);
    // }

    // if (_userAgent != null) {
    //   _invokeChannelMethod<void>('userAgent', _userAgent);
    // }

    // _javaScriptChannelParams.forEach(
    //     (String channel, JavaScriptChannelParams javaScriptChannelParams) {
    //   _invokeChannelMethod<void>(
    //       'addJavascriptChannel', javaScriptChannelParams.name);
    // });
  }

  Future<void> loadFile(String absoluteFilePath) async {
    assert(absoluteFilePath != null);
    _invokeChannelMethod<void>('loadFile', absoluteFilePath);
  }

  Future<void> loadFlutterAsset(String key) async {
    assert(key.isNotEmpty);
    _invokeChannelMethod<void>('loadFlutterAsset', key);
  }

  Future<void> loadHtmlString(
    String html, {
    String? baseUrl,
  }) async {
    assert(html != null);
    _invokeChannelMethod<void>('loadHtmlString', <String, dynamic>{
      'html': html,
      'baseUrl': baseUrl,
    });
  }

  Future<void> loadRequestPost(String uri) async {
    _invokeChannelMethod<void>('loadRequest', <String?, String?>{'url': uri});
  }

  Future<String?> currentUrl() => _invokeChannelMethod<String>('currentUrl');

  Future<bool> canGoBack() =>
      _invokeChannelMethod<bool>('canGoBack').then((bool? result) => result!);

  Future<bool> canGoForward() => _invokeChannelMethod<bool>('canGoForward')
      .then((bool? result) => result!);

  Future<void> goBack() => _invokeChannelMethod<void>('goBack');

  Future<void> goForward() => _invokeChannelMethod<void>('goForward');

  Future<void> reload() async {
    _invokeChannelMethod<void>('reload');
  }

  Future<void> clearCache() async {
    _invokeChannelMethod<void>('clearCache');
  }

  Future<void> clearLocalStorage() async {
    Log.error('WebviewFlutterTizen', 'clearLocalStorage is not implemented.');
  }

  Future<void> setJavaScriptMode(int javaScriptMode) async {
    _invokeChannelMethod<void>('javaScriptMode', javaScriptMode);
  }

  Future<String?> getTitle() {
    return _invokeChannelMethod<String>('getTitle');
  }

  Future<void> scrollTo(int x, int y) async {
    _invokeChannelMethod<void>('scrollTo', <String, int>{
      'x': x,
      'y': y,
    });
  }

  Future<void> scrollBy(int x, int y) {
    return _invokeChannelMethod<void>('scrollBy', <String, int>{
      'x': x,
      'y': y,
    });
  }

  Future<Offset> getScrollPosition() async {
    final dynamic position =
        await _invokeChannelMethod<dynamic>('getScrollPosition');
    if (position == null) {
      return Offset.zero;
    }
    return Offset(position['x'] as double, position['y'] as double);
  }

  Future<void> setBackgroundColor(Color color) async {
    _invokeChannelMethod<void>('backgroundColor', color.value);
  }

  Future<void> setHasNavigationDelegate(bool hasNavigationDelegate) async {
    _hasNavigationDelegate = hasNavigationDelegate;
  }

  Future<void> addJavaScriptChannel(
      JavaScriptChannelParams javaScriptChannelParams) async {
    _invokeChannelMethod<void>(
        'addJavaScriptChannel', javaScriptChannelParams.name);
  }

  Future<void> runJavaScript(String javaScript) {
    Log.error('WebviewFlutterTizen', 'CJS javaScript ${javaScript}');
    return _invokeChannelMethod<void>('runJavascript', javaScript);
  }

  Future<Object> runJavaScriptReturningResult(String javaScript) async {
    final String? result = await _invokeChannelMethod<String?>(
        'runJavascriptReturningResult', javaScript);
    if (result == null) {
      return '';
    } else if (result == 'true') {
      return true;
    } else if (result == 'false') {
      return false;
    }
    return num.tryParse(result) ?? result;
  }

  Future<void> setUserAgent(String? userAgent) async {
    _invokeChannelMethod<void>('userAgent', userAgent);
  }
}
