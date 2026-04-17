# flutter_inappwebview_lwe

Tizen implementation of [`flutter_inappwebview`](https://pub.dev/packages/flutter_inappwebview) backed by the Lightweight Web Engine (LWE).

This package follows the existing `webview_flutter_lwe` engine integration and maps the `flutter_inappwebview` platform interface onto LWE where the engine exposes matching capabilities.

## Status

Implemented in this package:

- `InAppWebView` platform view
- URL loading, file/data loading, back/forward/reload
- JavaScript evaluation
- navigation interception via `shouldOverrideUrlLoading`
- progress, title, scroll callbacks
- settings subset: user agent, transparent background, scrollbar visibility
- cookie clearing through the engine plus JavaScript fallback for per-page cookie mutation/query

Current LWE-specific limitations:

- no POST request support
- no native JS dialog / console / find-in-page / zoom implementation
- `InAppBrowser` unsupported
- `HeadlessInAppWebView` unsupported
- advanced WebMessage / reply-port native plumbing unsupported

## Required privileges

Add the internet privilege to the app manifest:

```xml
<privileges>
  <privilege>http://tizen.org/privilege/internet</privilege>
</privileges>
```

## Usage

Add both the app-facing package and this LWE implementation:

```yaml
dependencies:
  flutter_inappwebview: ^6.1.5
  flutter_inappwebview_lwe:
    path: ../flutter_inappwebview_lwe
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

- `CookieManager.setCookie/getCookies/deleteCookie/deleteCookies` use JavaScript fallback against the active webview when a `webViewController` is supplied.
- `CookieManager.deleteAllCookies()` clears the LWE cookie store.
- `InAppBrowser` and `HeadlessInAppWebView` currently throw `UnsupportedError`.
