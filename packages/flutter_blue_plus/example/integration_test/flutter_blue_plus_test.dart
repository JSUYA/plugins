// ignore_for_file: directives_ordering

import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';

import 'package:flutter_blue_plus_example/main.dart' as app;

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('renders example shell', (WidgetTester tester) async {
    app.main();
    await tester.pumpAndSettle();

    expect(find.text('Flutter Blue Plus Example'), findsOneWidget);
  });
}
