import 'package:flutter/material.dart';
import 'package:flutter_inappwebview/flutter_inappwebview.dart';

import 'headless_in_app_webview.screen.dart';
import 'in_app_browser_example.screen.dart';
import 'in_app_webiew_example.screen.dart';

export 'example_shared.dart'
    show
        exampleAppDriver,
        loadFixtureButtonKey,
        runSmokeButtonKey,
        statusTextKey;

WebViewEnvironment? webViewEnvironment;

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

void runForIntegrationTest() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp(autoSmoke: true, initialUrl: 'about:blank'));
}

Widget myDrawer({required BuildContext context}) {
  return Drawer(
    child: ListView(
      padding: EdgeInsets.zero,
      children: <Widget>[
        const DrawerHeader(
          decoration: BoxDecoration(color: Colors.blue),
          child: Text('flutter_inappwebview example'),
        ),
        ListTile(
          title: const Text('InAppWebView'),
          onTap: () => Navigator.pushReplacementNamed(context, '/'),
        ),
        ListTile(
          title: const Text('InAppBrowser'),
          onTap: () => Navigator.pushReplacementNamed(context, '/InAppBrowser'),
        ),
        ListTile(
          title: const Text('HeadlessInAppWebView'),
          onTap: () =>
              Navigator.pushReplacementNamed(context, '/HeadlessInAppWebView'),
        ),
      ],
    ),
  );
}

class MyApp extends StatelessWidget {
  const MyApp({
    this.autoSmoke = false,
    this.initialUrl = 'https://flutter.dev',
    super.key,
  });

  final bool autoSmoke;
  final String initialUrl;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'flutter_inappwebview example',
      debugShowCheckedModeBanner: false,
      initialRoute: '/',
      routes: <String, WidgetBuilder>{
        '/': (_) => InAppWebViewExampleScreen(
          packageLabel: 'InAppWebView Tizen (LWE)',
          autoSmoke: autoSmoke,
          initialUrl: initialUrl,
        ),
        '/InAppBrowser': (_) => const InAppBrowserExampleScreen(),
        '/HeadlessInAppWebView': (_) =>
            const HeadlessInAppWebViewExampleScreen(),
      },
    );
  }
}
