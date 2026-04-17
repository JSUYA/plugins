import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';

import 'package:flutter_inappwebview_tizen_example/main.dart' as app;

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('example app boots', (tester) async {
    app.main();
    await tester.pump(const Duration(seconds: 1));

    expect(find.text('InAppWebView Tizen (EWK)'), findsOneWidget);
    expect(find.byKey(app.statusTextKey), findsOneWidget);
  });
}
