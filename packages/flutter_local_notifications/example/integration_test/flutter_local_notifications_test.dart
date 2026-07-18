// ignore_for_file: directives_ordering

import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:flutter_local_notifications_example/main.dart' as app;

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('renders example app', (WidgetTester tester) async {
    app.main();
    await tester.pumpAndSettle();

    expect(find.text('flutter_local_notifications_tizen'), findsOneWidget);
  });
}
