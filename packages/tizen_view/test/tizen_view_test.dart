import 'package:flutter_test/flutter_test.dart';
import 'package:tizen_view/tizen_view.dart';
import 'package:tizen_view/tizen_view_platform_interface.dart';
import 'package:tizen_view/tizen_view_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockTizenViewPlatform
    with MockPlatformInterfaceMixin
    implements TizenViewPlatform {

  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final TizenViewPlatform initialPlatform = TizenViewPlatform.instance;

  test('$MethodChannelTizenView is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelTizenView>());
  });

  test('getPlatformVersion', () async {
    TizenView tizenViewPlugin = TizenView();
    MockTizenViewPlatform fakePlatform = MockTizenViewPlatform();
    TizenViewPlatform.instance = fakePlatform;

    expect(await tizenViewPlugin.getPlatformVersion(), '42');
  });
}
