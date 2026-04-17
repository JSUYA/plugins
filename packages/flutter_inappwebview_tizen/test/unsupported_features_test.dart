import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';
import 'package:flutter_inappwebview_tizen/flutter_inappwebview_tizen.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  test('unsupported service APIs throw UnsupportedError', () async {
    final webStorageManager = TizenWebStorageManager(
      TizenWebStorageManagerCreationParams(
        const PlatformWebStorageManagerCreationParams(),
      ),
    );
    final httpAuthCredentialDatabase = TizenHttpAuthCredentialDatabase(
      TizenHttpAuthCredentialDatabaseCreationParams(
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
      TizenWebViewEnvironment.static().create(),
      throwsA(isA<UnsupportedError>()),
    );
  });
}
