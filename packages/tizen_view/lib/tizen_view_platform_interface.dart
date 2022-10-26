import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'tizen_view_method_channel.dart';

abstract class TizenViewPlatform extends PlatformInterface {
  /// Constructs a TizenViewPlatform.
  TizenViewPlatform() : super(token: _token);

  static final Object _token = Object();

  static TizenViewPlatform _instance = MethodChannelTizenView();

  /// The default instance of [TizenViewPlatform] to use.
  ///
  /// Defaults to [MethodChannelTizenView].
  static TizenViewPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [TizenViewPlatform] when
  /// they register themselves.
  static set instance(TizenViewPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('platformVersion() has not been implemented.');
  }
}
