import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';
import 'package:flutter_inappwebview_lwe/flutter_inappwebview_lwe.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  test('unsupported service APIs throw UnsupportedError', () async {
    final webStorageManager = LweWebStorageManager(
      LweWebStorageManagerCreationParams(
        const PlatformWebStorageManagerCreationParams(),
      ),
    );
    final httpAuthCredentialDatabase = LweHttpAuthCredentialDatabase(
      LweHttpAuthCredentialDatabaseCreationParams(
        const PlatformHttpAuthCredentialDatabaseCreationParams(),
      ),
    );

    await expectLater(
      webStorageManager.deleteAllData(),
      throwsA(isA<UnsupportedError>()),
    );
    await expectLater(
      httpAuthCredentialDatabase.clearAllAuthCredentials(),
      throwsA(isA<UnsupportedError>()),
    );
    await expectLater(
      LweWebViewEnvironment.static().create(),
      throwsA(isA<UnsupportedError>()),
    );
  });
}
