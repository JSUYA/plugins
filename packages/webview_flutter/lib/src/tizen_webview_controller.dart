// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(a14n): remove this import once Flutter 3.1 or later reaches stable (including flutter/flutter#104231)
// ignore: unnecessary_import
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';
import 'package:webview_flutter_platform_interface/webview_flutter_platform_interface.dart';
import 'package:flutter_tizen/widgets.dart';
import 'method_channel/tizen_webview_method_channel.dart';
// import 'tizen_proxy.dart';
// import 'tizen_webview.dart' as tizen_webview;
// import 'tizen_webview.dart';
// import 'instance_manager.dart';
import 'platform_views_service_proxy.dart';
// import 'weak_reference_utils.dart';

/// Object specifying creation parameters for creating a [TizenWebViewController].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebViewControllerCreationParams] for
/// more information.
@immutable
class TizenWebViewControllerCreationParams
    extends PlatformWebViewControllerCreationParams {
  /// Creates a new [TizenWebViewControllerCreationParams] instance.
  TizenWebViewControllerCreationParams(
      // {
      // @visibleForTesting this.tizenWebViewProxy = const TizenWebViewProxy(),
      // @visibleForTesting tizen_webview.WebStorage? tizenWebStorage,
      // }
      )
      :
        //  tizenWebStorage =
        //           tizenWebStorage ?? tizen_webview.WebStorage.instance,
        super();

  /// Creates a [TizenWebViewControllerCreationParams] instance based on [PlatformWebViewControllerCreationParams].
  factory TizenWebViewControllerCreationParams.fromPlatformWebViewControllerCreationParams(
      // Recommended placeholder to prevent being broken by platform interface.
      // ignore: avoid_unused_constructor_parameters
      PlatformWebViewControllerCreationParams params,
      nullptr
      //   {
      //   @visibleForTesting
      //       TizenWebViewProxy tizenWebViewProxy = const TizenWebViewProxy(),
      //   @visibleForTesting tizen_webview.WebStorage? tizenWebStorage,
      // }
      ) {
    return TizenWebViewControllerCreationParams(
        // tizenWebViewProxy: tizenWebViewProxy,
        // tizenWebStorage:
        //     tizenWebStorage ?? tizen_webview.WebStorage.instance,
        );
  }

  /// Handles constructing objects and calling static methods for the Tizen WebView
  /// native library.
  // @visibleForTesting
  // final TizenWebViewProxy tizenWebViewProxy;

  /// Manages the JavaScript storage APIs provided by the [tizen_webview.WebView].
  // @visibleForTesting
  // final tizen_webview.WebStorage tizenWebStorage;
}

/// Implementation of the [PlatformWebViewController] with the Tizen WebView API.
class TizenWebViewController extends PlatformWebViewController {
  /// Creates a new [TizenWebViewCookieManager].
  TizenWebViewController(PlatformWebViewControllerCreationParams params)
      : super.implementation(params is TizenWebViewControllerCreationParams
            ? params
            : TizenWebViewControllerCreationParams
                .fromPlatformWebViewControllerCreationParams(params, null)) {
    // _channel = MethodChannel('plugins.flutter.io/webview_');//$id');
    _channel.setMethodCallHandler(_onMethodCall);
    // _webView.settings.setDomStorageEnabled(true);
    // _webView.settings.setJavaScriptCanOpenWindowsAutomatically(true);
    // _webView.settings.setSupportMultipleWindows(true);
    // _webView.settings.setLoadWithOverviewMode(true);
    // _webView.settings.setUseWideViewPort(true);
    // _webView.settings.setDisplayZoomControls(false);
    // _webView.settings.setBuiltInZoomControls(true);
  }

  //이걸 각 view_controller로 채널을 열고 각 함수에서 호출
  // final
  MethodChannel _channel =
      MethodChannel('plugins.flutter.io/webview_'); //$id');;

  Future<bool?> _onMethodCall(MethodCall call) async {
    switch (call.method) {
      case 'javascriptChannelMessage':
      // final String channel = call.arguments['channel']! as String;
      // final String message = call.arguments['message']! as String;
      // _javascriptChannelRegistry.onJavascriptChannelMessage(channel, message);
      // return true;
      case 'navigationRequest':
      // return await _platformCallbacksHandler.onNavigationRequest(
      //   url: call.arguments['url']! as String,
      //   isForMainFrame: call.arguments['isForMainFrame']! as bool,
      // );
      case 'onPageFinished':
        // _platformCallbacksHandler
        //     .onPageFinished(call.arguments['url']! as String);
        return null;
      case 'onProgress':
        // _platformCallbacksHandler.onProgress(call.arguments['progress'] as int);
        return null;
      case 'onPageStarted':
      // _platformCallbacksHandler
      //     .onPageStarted(call.arguments['url']! as String);
      // return null;
      case 'onWebResourceError':
        // _platformCallbacksHandler.onWebResourceError(
        //   WebResourceError(
        //     errorCode: call.arguments['errorCode']! as int,
        //     description: call.arguments['description']! as String,
        //     // iOS doesn't support `failingUrl`.
        //     failingUrl: call.arguments['failingUrl'] as String?,
        //     domain: call.arguments['domain'] as String?,
        //     errorType: call.arguments['errorType'] == null
        //         ? null
        //         : WebResourceErrorType.values.firstWhere(
        //             (WebResourceErrorType type) {
        //               return type.toString() ==
        //                   '$WebResourceErrorType.${call.arguments['errorType']}';
        //             },
        //           ),
        //   ),
        // );
        return null;
    }

    return null;
    // throw MissingPluginException(
    //   '${call.method} was invoked but has no handler',
    // );
  }

  TizenWebViewControllerCreationParams get _tizenWebViewParams =>
      params as TizenWebViewControllerCreationParams;

  /// The native [tizen_webview.WebView] being controlled.
  // late final tizen_webview.WebView _webView =
  //     _tizenWebViewParams.tizenWebViewProxy.createTizenWebView(
  //   // Due to changes in Flutter 3.0 the `useHybridComposition` doesn't have
  //   // any effect and is purposefully not exposed publicly by the
  //   // [TizenWebViewController]. More info here:
  //   // https://github.com/flutter/flutter/issues/108106
  //   // useHybridComposition: true,
  // );

  /// The native [tizen_webview.FlutterAssetManager] allows managing assets.
  // late final tizen_webview.FlutterAssetManager _flutterAssetManager =
  //     _tizenWebViewParams.tizenWebViewProxy.createFlutterAssetManager();

  // 채널 파람이 별도로 필요해 보이지 않음
  // final Map<String, TizenJavaScriptChannelParams> _javaScriptChannelParams =
  //     <String, TizenJavaScriptChannelParams>{};

  // The keeps a reference to the current NavigationDelegate so that the
  // callback methods remain reachable.
  // ignore: unused_field
  // late TizenNavigationDelegate _currentNavigationDelegate;

  /// Whether to enable the platform's webview content debugging tools.
  ///
  /// Defaults to false.
  // static Future<void> enableDebugging(
  //   bool enabled, {
  //   @visibleForTesting
  //       TizenWebViewProxy webViewProxy = const TizenWebViewProxy(),
  // }) {
  //   return webViewProxy.setWebContentsDebuggingEnabled(enabled);
  // }

  @override
  Future<void> loadFile(
    String absoluteFilePath,
  ) async {
    assert(absoluteFilePath != null);
    try {
      return await _channel.invokeMethod<void>('loadFile', absoluteFilePath);
    } on PlatformException catch (ex) {
      if (ex.code == 'loadFile_failed') {
        throw ArgumentError(ex.message);
      }
      rethrow;
    }
  }

  @override
  Future<void> loadFlutterAsset(
    String key,
  ) async {
    assert(key.isNotEmpty);

    try {
      return await _channel.invokeMethod<void>('loadFlutterAsset', key);
    } on PlatformException catch (ex) {
      if (ex.code == 'loadFlutterAsset_invalidKey') {
        throw ArgumentError(ex.message);
      }

      rethrow;
    }
  }

  @override
  Future<void> loadHtmlString(
    String html, {
    String? baseUrl,
  }) {
    assert(html != null);
    return _channel.invokeMethod<void>('loadHtmlString', <String, dynamic>{
      'html': html,
      'baseUrl': baseUrl,
    });
  }

  @override
  Future<void> loadRequest(
    LoadRequestParams params,
  ) {
    // if (!params.uri.hasScheme) {
    //   throw ArgumentError('WebViewRequest#uri is required to have a scheme.');
    // }
    // switch (params.method) {
    //   case LoadRequestMethod.get:
    //     return _webView.loadUrl(params.uri.toString(), params.headers);
    //   case LoadRequestMethod.post:
    //     return _webView.postUrl(
    //         params.uri.toString(), params.body ?? Uint8List(0));
    //   default:
    //     throw UnimplementedError(
    //       'This version of `TizenWebViewController` currently has no implementation for HTTP method ${params.method.serialize()} in loadRequest.',
    //     );
    // }
    // assert(request != null);
    // return _channel.invokeMethod<void>('loadRequest', <String, dynamic>{
    //   'request': request.toJson(),
    // });
    throw UnimplementedError(
      'nononono',
    );
  }

  @override
  Future<String?> currentUrl() => _channel.invokeMethod<String>('currentUrl');

  @override
  Future<bool> canGoBack() =>
      _channel.invokeMethod<bool>('canGoBack').then((bool? result) => result!);

  @override
  Future<bool> canGoForward() => _channel
      .invokeMethod<bool>('canGoForward')
      .then((bool? result) => result!);

  @override
  Future<void> goBack() => _channel.invokeMethod<void>('goBack');

  @override
  Future<void> goForward() => _channel.invokeMethod<void>('goForward');

  @override
  Future<void> reload() => _channel.invokeMethod<void>('reload');

  @override
  Future<void> clearCache() => _channel.invokeMethod<void>('clearCache');

  // @override
  // Future<void> clearLocalStorage() =>
  //     _tizenWebViewParams.tizenWebStorage.deleteAllData();

  @override
  Future<void> setPlatformNavigationDelegate(
      covariant TizenNavigationDelegate handler) async {
    // _currentNavigationDelegate = handler;
    // handler.setOnLoadRequest(loadRequest);
    // _webView.setWebViewClient(handler.tizenWebViewClient);
    // _webView.setWebChromeClient(handler.tizenWebChromeClient);
    // _webView.setDownloadListener(handler.tizenDownloadListener);
  }

  @override
  Future<void> runJavaScript(String javaScript) async {
    await _channel.invokeMethod<String>('runJavascript', javaScript);
  }

  @override
  Future<Object> runJavaScriptReturningResult(String javaScript) async {
    // final String? result = await _webView.evaluateJavascript(javaScript);

    // if (result == null) {
    //   return '';
    // } else if (result == 'true') {
    //   return true;
    // } else if (result == 'false') {
    //   return false;
    // }

    // return num.tryParse(result) ?? result;
    return _channel
        .invokeMethod<String>('runJavascriptReturningResult', javaScript)
        .then((String? result) => result!);
  }

  // @override
  // Future<void> addJavaScriptChannel(
  //   JavaScriptChannelParams javaScriptChannelParams,
  // ) {
  //   // final TizenJavaScriptChannelParams tizenJavaScriptParams =
  //   //     javaScriptChannelParams is TizenJavaScriptChannelParams
  //   //         ? javaScriptChannelParams
  //   //         : TizenJavaScriptChannelParams.fromJavaScriptChannelParams(
  //   //             javaScriptChannelParams);

  //   // // When JavaScript channel with the same name exists make sure to remove it
  //   // // before registering the new channel.
  //   // if (_javaScriptChannelParams.containsKey(tizenJavaScriptParams.name)) {
  //   //   _webView
  //   //       .removeJavaScriptChannel(tizenJavaScriptParams._javaScriptChannel);
  //   // }

  //   // _javaScriptChannelParams[tizenJavaScriptParams.name] =
  //   //     tizenJavaScriptParams;

  //   // return _webView
  //   //     .addJavaScriptChannel(tizenJavaScriptParams._javaScriptChannel);
  // }

  // @override
  // Future<void> removeJavaScriptChannel(String javaScriptChannelName) async {
  //   // final TizenJavaScriptChannelParams? javaScriptChannelParams =
  //   //     _javaScriptChannelParams[javaScriptChannelName];
  //   // if (javaScriptChannelParams == null) {
  //   //   return;
  //   // }

  //   // _javaScriptChannelParams.remove(javaScriptChannelName);
  //   // return _webView
  //   //     .removeJavaScriptChannel(javaScriptChannelParams._javaScriptChannel);
  // }

  @override
  Future<String?> getTitle() => _channel.invokeMethod<String>('getTitle');

  @override
  Future<void> scrollTo(int x, int y) {
    return _channel.invokeMethod<void>('scrollTo', <String, int>{
      'x': x,
      'y': y,
    });
  }

  @override
  Future<void> scrollBy(int x, int y) {
    return _channel.invokeMethod<void>('scrollBy', <String, int>{
      'x': x,
      'y': y,
    });
  }

  // @override
  // Future<Offset> getScrollPosition() {
  //   return _webView.getScrollPosition();
  // }

  // @override
  // Future<void> enableZoom(bool enabled) =>
  //     _webView.settings.setSupportZoom(enabled);

  @override
  Future<void> setBackgroundColor(Color color) async {
    return;
  }
  // _webView.setBackgroundColor(color);

  @override
  Future<void> setJavaScriptMode(JavaScriptMode javaScriptMode) async {
    // _webView.settings
    //         .setJavaScriptEnabled(javaScriptMode == JavaScriptMode.unrestricted);

    // return setJavaScriptEnable(javaScriptMode == JavaScriptMode.unrestricted);
    return;
  }

  // void setJavaScriptEnable(bool enable) {}

  // @override
  // Future<void> setUserAgent(String? userAgent) =>
  //     _webView.settings.setUserAgentString(userAgent);

  /// Sets the restrictions that apply on automatic media playback.
  // Future<void> setMediaPlaybackRequiresUserGesture(bool require) {
  //   return _webView.settings.setMediaPlaybackRequiresUserGesture(require);
  // }
}
/*
/// An implementation of [JavaScriptChannelParams] with the Tizen WebView API.
///
/// See [TizenWebViewController.addJavaScriptChannel].
@immutable
class TizenJavaScriptChannelParams extends JavaScriptChannelParams {
  /// Constructs a [TizenJavaScriptChannelParams].
  TizenJavaScriptChannelParams({
    required super.name,
    required super.onMessageReceived,
    @visibleForTesting
        TizenWebViewProxy webViewProxy = const TizenWebViewProxy(),
  })  : assert(name.isNotEmpty),
        _javaScriptChannel = webViewProxy.createJavaScriptChannel(
          name,
          postMessage: withWeakRefenceTo(
            onMessageReceived,
            (WeakReference<void Function(JavaScriptMessage)> weakReference) {
              return (
                String message,
              ) {
                if (weakReference.target != null) {
                  weakReference.target!(
                    JavaScriptMessage(message: message),
                  );
                }
              };
            },
          ),
        );

  /// Constructs a [TizenJavaScriptChannelParams] using a
  /// [JavaScriptChannelParams].
  TizenJavaScriptChannelParams.fromJavaScriptChannelParams(
    JavaScriptChannelParams params, {
    @visibleForTesting
        TizenWebViewProxy webViewProxy = const TizenWebViewProxy(),
  }) : this(
          name: params.name,
          onMessageReceived: params.onMessageReceived,
          webViewProxy: webViewProxy,
        );

  final tizen_webview.JavaScriptChannel _javaScriptChannel;
}*/

/// Object specifying creation parameters for creating a [TizenWebViewWidget].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebViewWidgetCreationParams] for
/// more information.
@immutable
class TizenWebViewWidgetCreationParams
    extends PlatformWebViewWidgetCreationParams {
  /// Creates [TizenWebWidgetCreationParams].
  TizenWebViewWidgetCreationParams({
    super.key,
    required super.controller,
    super.layoutDirection,
    super.gestureRecognizers,
    // this.displayWithHybridComposition = false,
    // @visibleForTesting InstanceManager? instanceManager,
    // @visibleForTesting
    //     this.platformViewsServiceProxy = const PlatformViewsServiceProxy(),
  }); // : instanceManager = instanceManager ?? JavaObject.globalInstanceManager;

  /// Constructs a [WebKitWebViewWidgetCreationParams] using a
  /// [PlatformWebViewWidgetCreationParams].
  TizenWebViewWidgetCreationParams.fromPlatformWebViewWidgetCreationParams(
      PlatformWebViewWidgetCreationParams params
      // , {
      // bool displayWithHybridComposition = false,
      // @visibleForTesting InstanceManager? instanceManager,
      // @visibleForTesting PlatformViewsServiceProxy platformViewsServiceProxy =
      //     const PlatformViewsServiceProxy(),
      // }
      )
      : this(
          key: params.key,
          controller: params.controller,
          layoutDirection: params.layoutDirection,
          gestureRecognizers: params.gestureRecognizers,
          // displayWithHybridComposition: displayWithHybridComposition,
          // instanceManager: instanceManager,
          // platformViewsServiceProxy: platformViewsServiceProxy,
        );

  /// Maintains instances used to communicate with the native objects they
  /// represent.
  ///
  /// This field is exposed for testing purposes only and should not be used
  /// outside of tests.
  // @visibleForTesting
  // final InstanceManager instanceManager;

  /// Proxy that provides access to the platform views service.
  ///
  /// This service allows creating and controlling platform-specific views.
  // @visibleForTesting
  // final PlatformViewsServiceProxy platformViewsServiceProxy;

  /// Whether the [WebView] will be displayed using the Hybrid Composition
  /// PlatformView implementation.
  ///
  /// For most use cases, this flag should be set to false. Hybrid Composition
  /// can have performance costs but doesn't have the limitation of rendering to
  /// an Tizen SurfaceTexture. See
  /// * https://flutter.dev/docs/development/platform-integration/platform-views#performance
  /// * https://github.com/flutter/flutter/issues/104889
  /// * https://github.com/flutter/flutter/issues/116954
  ///
  /// Defaults to false.
  // final bool displayWithHybridComposition;
}

/// An implementation of [PlatformWebViewWidget] with the Tizen WebView API.
class TizenWebViewWidget extends PlatformWebViewWidget {
  /// Constructs a [WebKitWebViewWidget].
  TizenWebViewWidget(PlatformWebViewWidgetCreationParams params)
      : super.implementation(
          params is TizenWebViewWidgetCreationParams
              ? params
              : TizenWebViewWidgetCreationParams
                  .fromPlatformWebViewWidgetCreationParams(params),
        );

  TizenWebViewWidgetCreationParams get _tizenParams =>
      params as TizenWebViewWidgetCreationParams;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onLongPress: () {},
      excludeFromSemantics: true,
      child: TizenView(
        viewType: 'plugins.flutter.io/webview',
        // onPlatformViewCreated: (int id) {
        //   if (onWebViewPlatformCreated == null) {
        //     return;
        //   }
        //   onWebViewPlatformCreated(MethodChannelWebViewPlatform(
        //     id,
        //     webViewPlatformCallbacksHandler,
        //     javascriptChannelRegistry,
        //   ));
        // },
        // gestureRecognizers: gestureRecognizers,
        // layoutDirection: Directionality.maybeOf(context) ?? TextDirection.rtl,
        // creationParams:
        //     MethodChannelWebViewPlatform.creationParamsToMap(creationParams),
        // creationParamsCodec: const StandardMessageCodec(),
      ),
    );
  }

  // TizenViewController _initTizenView(
  //   PlatformViewCreationParams params
  // ) {
  //     return _tizenParams.platformViewsServiceProxy.initSurfaceTizenView(
  //       id: params.id,
  //       viewType: 'plugins.flutter.io/webview',
  //       layoutDirection: _tizenParams.layoutDirection,
  //       creationParams: _tizenParams.instanceManager.getIdentifier(
  //           (_tizenParams.controller as TizenWebViewController)._webView),
  //       creationParamsCodec: const StandardMessageCodec(),
  //     );
  // }
}
