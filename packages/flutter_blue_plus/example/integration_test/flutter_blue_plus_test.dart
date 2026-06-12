// ignore_for_file: directives_ordering

import 'package:flutter_blue_plus/flutter_blue_plus.dart';
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

  testWidgets('adapter queries respond over the platform channel',
      (WidgetTester tester) async {
    final bool supported = await FlutterBluePlus.isSupported;
    expect(supported, isA<bool>());

    final BluetoothAdapterState state =
        await FlutterBluePlus.adapterState.first;
    expect(BluetoothAdapterState.values, contains(state));

    if (supported) {
      expect(await FlutterBluePlus.adapterName, isA<String>());
    }
  });
}
