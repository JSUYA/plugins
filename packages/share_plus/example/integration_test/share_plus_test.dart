import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:share_plus/share_plus.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('share text returns a valid result', (WidgetTester tester) async {
    final ShareResult result = await SharePlus.instance.share(
      ShareParams(text: 'Integration test share'),
    );
    expect(result.status, isNotNull);
  });
}
