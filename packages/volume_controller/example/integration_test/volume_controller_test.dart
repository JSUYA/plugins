import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:volume_controller/volume_controller.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('can query and update media volume', (WidgetTester tester) async {
    final VolumeController controller = VolumeController.instance;
    final double initialVolume = await controller.getVolume();
    expect(initialVolume, inInclusiveRange(0.0, 1.0));

    final double targetVolume = initialVolume > 0.1 ? 0.1 : 0.3;
    await controller.setVolume(targetVolume);
    await tester.pump(const Duration(milliseconds: 300));

    final double updatedVolume = await controller.getVolume();
    expect(updatedVolume, closeTo(targetVolume, 0.15));

    await controller.setMute(true);
    await tester.pump(const Duration(milliseconds: 300));
    expect(await controller.isMuted(), isTrue);

    await controller.setMute(false);
    await tester.pump(const Duration(milliseconds: 300));
    expect(await controller.isMuted(), isFalse);

    await controller.setVolume(initialVolume);
  });
}
