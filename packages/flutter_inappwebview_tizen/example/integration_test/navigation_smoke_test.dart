import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';

import 'package:flutter_inappwebview_tizen_example/example_shared.dart';
import 'package:flutter_inappwebview_tizen_example/main.dart' as app;

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  Future<void> openDrawerAndTap(WidgetTester tester, String label) async {
    print('STEP open drawer for $label');
    final Finder drawerButton = find.byTooltip('Open navigation menu');
    expect(drawerButton, findsOneWidget);
    await tester.tap(drawerButton);
    await tester.pump(const Duration(milliseconds: 500));

    print('STEP tap $label');
    final Finder tile = find.text(label);
    expect(tile, findsOneWidget);
    await tester.tap(tile);
    await tester.pump(const Duration(seconds: 1));
    print('STEP done $label');
  }

  testWidgets('navigates through drawer routes without crashing',
      (tester) async {
    app.main();
    await tester.pump(const Duration(seconds: 2));
    print('STEP app booted');

    await openDrawerAndTap(tester, 'InAppBrowser');
    print('STEP verify InAppBrowser');
    expect(find.text('InAppBrowser'), findsOneWidget);
    expect(
      find.text(
        'InAppBrowser is not implemented for flutter_inappwebview_tizen yet.',
      ),
      findsOneWidget,
    );

    await openDrawerAndTap(tester, 'InAppWebView');
    print('STEP verify InAppWebView after browser');
    expect(find.text('InAppWebView'), findsOneWidget);
    expect(find.byKey(statusTextKey), findsOneWidget);

    await openDrawerAndTap(tester, 'HeadlessInAppWebView');
    print('STEP verify HeadlessInAppWebView');
    expect(find.text('HeadlessInAppWebView'), findsOneWidget);
    expect(
      find.text(
        'HeadlessInAppWebView is not implemented for flutter_inappwebview_tizen yet.',
      ),
      findsOneWidget,
    );

    await openDrawerAndTap(tester, 'InAppWebView');
    print('STEP verify InAppWebView after headless');
    expect(find.text('InAppWebView'), findsOneWidget);
    expect(find.byKey(statusTextKey), findsOneWidget);

    await openDrawerAndTap(tester, 'InAppBrowser');
    print('STEP verify InAppBrowser second cycle');
    expect(find.text('InAppBrowser'), findsOneWidget);
    expect(
      find.text(
        'InAppBrowser is not implemented for flutter_inappwebview_tizen yet.',
      ),
      findsOneWidget,
    );

    await openDrawerAndTap(tester, 'InAppWebView');
    print('STEP verify InAppWebView second cycle');
    expect(find.text('InAppWebView'), findsOneWidget);
    expect(find.byKey(statusTextKey), findsOneWidget);
  });
}
