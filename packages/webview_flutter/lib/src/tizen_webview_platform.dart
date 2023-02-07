import 'package:webview_flutter_platform_interface/webview_flutter_platform_interface.dart';

// import 'tizen_navigation_delegate.dart';
import 'tizen_webview_controller.dart';
import 'tizen_webview_cookie_manager.dart';

/// Implementation of [WebViewPlatform] using the WebKit API.
class TizenWebViewPlatform extends WebViewPlatform {
  /// Registers this class as the default instance of [WebViewPlatform].
  static void registerWith() {
    WebViewPlatform.instance = TizenWebViewPlatform();
  }

  // ??
  static void register() {
    WebViewPlatform.instance = TizenWebViewPlatform();
  }

  @override
  TizenWebViewController createPlatformWebViewController(
    PlatformWebViewControllerCreationParams params,
  ) {
    return TizenWebViewController(params);
  }

  @override
  TizenNavigationDelegate createPlatformNavigationDelegate(
    PlatformNavigationDelegateCreationParams params,
  ) {
    return TizenNavigationDelegate(params);
  }

  @override
  TizenWebViewWidget createPlatformWebViewWidget(
    PlatformWebViewWidgetCreationParams params,
  ) {
    return TizenWebViewWidget(params);
  }

  @override
  TizenWebViewCookieManager createPlatformCookieManager(
    PlatformWebViewCookieManagerCreationParams params,
  ) {
    return TizenWebViewCookieManager(params);
  }
}
