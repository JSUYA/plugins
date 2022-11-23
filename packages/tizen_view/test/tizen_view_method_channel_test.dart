import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tizen_view/tizen_view_method_channel.dart';

void main() {
  MethodChannelTizenView platform = MethodChannelTizenView();
  const MethodChannel channel = MethodChannel('tizen_view');

  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() {
    channel.setMockMethodCallHandler((MethodCall methodCall) async {
      return '42';
    });
  });

  tearDown(() {
    channel.setMockMethodCallHandler(null);
  });

  test('getPlatformVersion', () async {
    expect(await platform.getPlatformVersion(), '42');
  });
}
