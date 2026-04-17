import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

import 'cookie_manager.dart';
import 'find_interaction/find_interaction_controller.dart';
import 'http_auth_credentials_database.dart';
import 'in_app_browser/in_app_browser.dart';
import 'in_app_webview/in_app_webview.dart';
import 'in_app_webview/in_app_webview_controller.dart';
import 'in_app_webview/headless_in_app_webview.dart';
import 'print_job/print_job_controller.dart';
import 'web_message/web_message_channel.dart';
import 'web_message/web_message_listener.dart';
import 'web_message/web_message_port.dart';
import 'webview_environment/webview_environment.dart';
import 'web_storage/web_storage.dart';
import 'web_storage/web_storage_manager.dart';

/// Implementation of [InAppWebViewPlatform] using the WebKit API.
class LweInAppWebViewPlatform extends InAppWebViewPlatform {
  /// Registers this class as the default instance of [InAppWebViewPlatform].
  static void register() {
    registerWith();
  }

  /// Registers this class as the default instance of [InAppWebViewPlatform].
  static void registerWith() {
    InAppWebViewPlatform.instance = LweInAppWebViewPlatform();
  }

  /// Creates a new [LweCookieManager].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [CookieManager] in `flutter_inappwebview` instead.
  @override
  LweCookieManager createPlatformCookieManager(
    PlatformCookieManagerCreationParams params,
  ) {
    return LweCookieManager(params);
  }

  /// Creates a new [LweInAppWebViewController].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppWebViewController] in `flutter_inappwebview` instead.
  @override
  LweInAppWebViewController createPlatformInAppWebViewController(
    PlatformInAppWebViewControllerCreationParams params,
  ) {
    return LweInAppWebViewController(params);
  }

  /// Creates a new empty [LweInAppWebViewController] to access static methods.
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppWebViewController] in `flutter_inappwebview` instead.
  @override
  LweInAppWebViewController createPlatformInAppWebViewControllerStatic() {
    return LweInAppWebViewController.static();
  }

  /// Creates a new [LweInAppWebViewWidget].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppWebView] in `flutter_inappwebview` instead.
  @override
  LweInAppWebViewWidget createPlatformInAppWebViewWidget(
    PlatformInAppWebViewWidgetCreationParams params,
  ) {
    return LweInAppWebViewWidget(params);
  }

  /// Creates a new [LweInAppBrowser].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppBrowser] in `flutter_inappwebview` instead.
  @override
  LweInAppBrowser createPlatformInAppBrowser(
    PlatformInAppBrowserCreationParams params,
  ) {
    return LweInAppBrowser(params);
  }

  /// Creates a new empty [LweInAppBrowser] to access static methods.
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppBrowser] in `flutter_inappwebview` instead.
  @override
  LweInAppBrowser createPlatformInAppBrowserStatic() {
    return LweInAppBrowser.static();
  }

  /// Creates a new [LweHeadlessInAppWebView].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [HeadlessInAppWebView] in `flutter_inappwebview` instead.
  @override
  LweHeadlessInAppWebView createPlatformHeadlessInAppWebView(
    PlatformHeadlessInAppWebViewCreationParams params,
  ) {
    return LweHeadlessInAppWebView(params);
  }

  /// Creates a new [LweWebViewEnvironment].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [WebViewEnvironment] in `flutter_inappwebview` instead.
  @override
  LweWebViewEnvironment createPlatformWebViewEnvironment(
    PlatformWebViewEnvironmentCreationParams params,
  ) {
    return LweWebViewEnvironment(params);
  }

  /// Creates a new empty [LweWebViewEnvironment] to access static methods.
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [WebViewEnvironment] in `flutter_inappwebview` instead.
  @override
  LweWebViewEnvironment createPlatformWebViewEnvironmentStatic() {
    return LweWebViewEnvironment.static();
  }

  /// Creates a new [LweWebStorage].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [WebStorage] in `flutter_inappwebview` instead.
  @override
  LweWebStorage createPlatformWebStorage(
    PlatformWebStorageCreationParams params,
  ) {
    return LweWebStorage(params);
  }

  @override
  LweWebStorageManager createPlatformWebStorageManager(
    PlatformWebStorageManagerCreationParams params,
  ) {
    return LweWebStorageManager(params);
  }

  @override
  LweHttpAuthCredentialDatabase createPlatformHttpAuthCredentialDatabase(
    PlatformHttpAuthCredentialDatabaseCreationParams params,
  ) {
    return LweHttpAuthCredentialDatabase(params);
  }

  /// Creates a new [LweLocalStorage].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [LocalStorage] in `flutter_inappwebview` instead.
  @override
  LweLocalStorage createPlatformLocalStorage(
    PlatformLocalStorageCreationParams params,
  ) {
    return LweLocalStorage(params);
  }

  /// Creates a new [LweSessionStorage].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [SessionStorage] in `flutter_inappwebview` instead.
  @override
  LweSessionStorage createPlatformSessionStorage(
    PlatformSessionStorageCreationParams params,
  ) {
    return LweSessionStorage(params);
  }

  @override
  LweFindInteractionController createPlatformFindInteractionController(
    PlatformFindInteractionControllerCreationParams params,
  ) {
    return LweFindInteractionController(params);
  }

  @override
  LwePrintJobController createPlatformPrintJobController(
    PlatformPrintJobControllerCreationParams params,
  ) {
    return LwePrintJobController(params);
  }

  @override
  LweWebMessageChannel createPlatformWebMessageChannel(
    PlatformWebMessageChannelCreationParams params,
  ) {
    return LweWebMessageChannel(params);
  }

  @override
  LweWebMessageChannel createPlatformWebMessageChannelStatic() {
    return LweWebMessageChannel.static();
  }

  @override
  LweWebMessageListener createPlatformWebMessageListener(
    PlatformWebMessageListenerCreationParams params,
  ) {
    return LweWebMessageListener(params);
  }

  @override
  LweJavaScriptReplyProxy createPlatformJavaScriptReplyProxy(
    PlatformJavaScriptReplyProxyCreationParams params,
  ) {
    return LweJavaScriptReplyProxy(params);
  }

  @override
  LweWebMessagePort createPlatformWebMessagePort(
    PlatformWebMessagePortCreationParams params,
  ) {
    return LweWebMessagePort(params);
  }
}
