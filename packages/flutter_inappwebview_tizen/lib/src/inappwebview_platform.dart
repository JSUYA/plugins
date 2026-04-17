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
class TizenInAppWebViewPlatform extends InAppWebViewPlatform {
  /// Registers this class as the default instance of [InAppWebViewPlatform].
  static void register() {
    registerWith();
  }

  /// Registers this class as the default instance of [InAppWebViewPlatform].
  static void registerWith() {
    InAppWebViewPlatform.instance = TizenInAppWebViewPlatform();
  }

  /// Creates a new [TizenCookieManager].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [CookieManager] in `flutter_inappwebview` instead.
  @override
  TizenCookieManager createPlatformCookieManager(
    PlatformCookieManagerCreationParams params,
  ) {
    return TizenCookieManager(params);
  }

  /// Creates a new [TizenInAppWebViewController].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppWebViewController] in `flutter_inappwebview` instead.
  @override
  TizenInAppWebViewController createPlatformInAppWebViewController(
    PlatformInAppWebViewControllerCreationParams params,
  ) {
    return TizenInAppWebViewController(params);
  }

  /// Creates a new empty [TizenInAppWebViewController] to access static methods.
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppWebViewController] in `flutter_inappwebview` instead.
  @override
  TizenInAppWebViewController createPlatformInAppWebViewControllerStatic() {
    return TizenInAppWebViewController.static();
  }

  /// Creates a new [TizenInAppWebViewWidget].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppWebView] in `flutter_inappwebview` instead.
  @override
  TizenInAppWebViewWidget createPlatformInAppWebViewWidget(
    PlatformInAppWebViewWidgetCreationParams params,
  ) {
    return TizenInAppWebViewWidget(params);
  }

  /// Creates a new [TizenInAppBrowser].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppBrowser] in `flutter_inappwebview` instead.
  @override
  TizenInAppBrowser createPlatformInAppBrowser(
    PlatformInAppBrowserCreationParams params,
  ) {
    return TizenInAppBrowser(params);
  }

  /// Creates a new empty [TizenInAppBrowser] to access static methods.
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [InAppBrowser] in `flutter_inappwebview` instead.
  @override
  TizenInAppBrowser createPlatformInAppBrowserStatic() {
    return TizenInAppBrowser.static();
  }

  /// Creates a new [TizenHeadlessInAppWebView].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [HeadlessInAppWebView] in `flutter_inappwebview` instead.
  @override
  TizenHeadlessInAppWebView createPlatformHeadlessInAppWebView(
    PlatformHeadlessInAppWebViewCreationParams params,
  ) {
    return TizenHeadlessInAppWebView(params);
  }

  /// Creates a new [TizenWebViewEnvironment].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [WebViewEnvironment] in `flutter_inappwebview` instead.
  @override
  TizenWebViewEnvironment createPlatformWebViewEnvironment(
    PlatformWebViewEnvironmentCreationParams params,
  ) {
    return TizenWebViewEnvironment(params);
  }

  /// Creates a new empty [TizenWebViewEnvironment] to access static methods.
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [WebViewEnvironment] in `flutter_inappwebview` instead.
  @override
  TizenWebViewEnvironment createPlatformWebViewEnvironmentStatic() {
    return TizenWebViewEnvironment.static();
  }

  /// Creates a new [TizenWebStorage].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [WebStorage] in `flutter_inappwebview` instead.
  @override
  TizenWebStorage createPlatformWebStorage(
    PlatformWebStorageCreationParams params,
  ) {
    return TizenWebStorage(params);
  }

  @override
  TizenWebStorageManager createPlatformWebStorageManager(
    PlatformWebStorageManagerCreationParams params,
  ) {
    return TizenWebStorageManager(params);
  }

  @override
  TizenHttpAuthCredentialDatabase createPlatformHttpAuthCredentialDatabase(
    PlatformHttpAuthCredentialDatabaseCreationParams params,
  ) {
    return TizenHttpAuthCredentialDatabase(params);
  }

  /// Creates a new [TizenLocalStorage].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [LocalStorage] in `flutter_inappwebview` instead.
  @override
  TizenLocalStorage createPlatformLocalStorage(
    PlatformLocalStorageCreationParams params,
  ) {
    return TizenLocalStorage(params);
  }

  /// Creates a new [TizenSessionStorage].
  ///
  /// This function should only be called by the app-facing package.
  /// Look at using [SessionStorage] in `flutter_inappwebview` instead.
  @override
  TizenSessionStorage createPlatformSessionStorage(
    PlatformSessionStorageCreationParams params,
  ) {
    return TizenSessionStorage(params);
  }

  @override
  TizenFindInteractionController createPlatformFindInteractionController(
    PlatformFindInteractionControllerCreationParams params,
  ) {
    return TizenFindInteractionController(params);
  }

  @override
  TizenPrintJobController createPlatformPrintJobController(
    PlatformPrintJobControllerCreationParams params,
  ) {
    return TizenPrintJobController(params);
  }

  @override
  TizenWebMessageChannel createPlatformWebMessageChannel(
    PlatformWebMessageChannelCreationParams params,
  ) {
    return TizenWebMessageChannel(params);
  }

  @override
  TizenWebMessageChannel createPlatformWebMessageChannelStatic() {
    return TizenWebMessageChannel.static();
  }

  @override
  TizenWebMessageListener createPlatformWebMessageListener(
    PlatformWebMessageListenerCreationParams params,
  ) {
    return TizenWebMessageListener(params);
  }

  @override
  TizenJavaScriptReplyProxy createPlatformJavaScriptReplyProxy(
    PlatformJavaScriptReplyProxyCreationParams params,
  ) {
    return TizenJavaScriptReplyProxy(params);
  }

  @override
  TizenWebMessagePort createPlatformWebMessagePort(
    PlatformWebMessagePortCreationParams params,
  ) {
    return TizenWebMessagePort(params);
  }
}
