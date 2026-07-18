import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:speech_to_text_example/main.dart' as app;

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('renders controls', (WidgetTester tester) async {
    app.main();
    await tester.pumpAndSettle();

    expect(find.text('Initialize'), findsOneWidget);
    expect(find.text('Listen'), findsOneWidget);
    expect(find.text('Stop'), findsOneWidget);
    expect(find.text('Cancel'), findsOneWidget);
  });
}
