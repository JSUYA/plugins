# flutter_inappwebview_tizen

Tizen implementation of [`flutter_inappwebview`](https://pub.dev/packages/flutter_inappwebview) backed by Chromium EWK (`libchromium-efl`).

This package targets the `flutter-tizen` plugin workspace and follows the existing `webview_flutter_tizen` engine integration patterns while adapting the platform channel contract expected by `flutter_inappwebview`.

## Status

Implemented in this package:

- `InAppWebView` platform view
- URL loading, file/data loading, back/forward/reload/stop
- JavaScript evaluation
- navigation interception via `shouldOverrideUrlLoading`
- progress, title, console, history, scroll, zoom callbacks
- settings subset: JavaScript enablement, zoom support, user agent, transparent background
- cookie operations with native `deleteAllCookies` support and JavaScript fallback for per-page cookie mutation/query

Currently stubbed or unsupported:

- `InAppBrowser`
- `HeadlessInAppWebView`
- `ChromeSafariBrowser`
- service worker / tracing / proxy / print / path handler APIs
- advanced WebMessage / reply-port native plumbing

## Required privileges

Add the internet privilege to the app manifest:

```xml
<privileges>
  <privilege>http://tizen.org/privilege/internet</privilege>
</privileges>
```

## Usage

Add both the app-facing package and this Tizen implementation:

```yaml
dependencies:
  flutter_inappwebview: ^6.1.5
  flutter_inappwebview_tizen:
    path: ../flutter_inappwebview_tizen
```

## Example

```dart
import 'package:flutter/material.dart';
import 'package:flutter_inappwebview/flutter_inappwebview.dart';

class WebViewExample extends StatelessWidget {
  const WebViewExample({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: InAppWebView(
        initialUrlRequest: URLRequest(
          url: WebUri('https://flutter.dev'),
        ),
        initialSettings: InAppWebViewSettings(
          javaScriptEnabled: true,
          useShouldOverrideUrlLoading: true,
        ),
      ),
    );
  }
}
```

## Notes

- `CookieManager.setCookie/getCookies/deleteCookie/deleteCookies` use the current webview context when a `webViewController` is supplied.
- `CookieManager.deleteAllCookies()` clears the engine cookie store.
- `InAppBrowser` and `HeadlessInAppWebView` currently throw `UnsupportedError` instead of failing with a missing plugin channel.
