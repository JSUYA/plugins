import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

/// Object specifying creation parameters for creating a [LweWebStorageManager].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebStorageManagerCreationParams] for
/// more information.
@immutable
class LweWebStorageManagerCreationParams
    extends PlatformWebStorageManagerCreationParams {
  /// Creates a new [LweWebStorageManagerCreationParams] instance.
  const LweWebStorageManagerCreationParams(
    // This parameter prevents breaking changes later.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebStorageManagerCreationParams params,
  ) : super();

  /// Creates a [LweWebStorageManagerCreationParams] instance based on [PlatformWebStorageManagerCreationParams].
  factory LweWebStorageManagerCreationParams.fromPlatformWebStorageManagerCreationParams(
    PlatformWebStorageManagerCreationParams params,
  ) {
    return LweWebStorageManagerCreationParams(params);
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformWebStorageManager}
class LweWebStorageManager extends PlatformWebStorageManager
    with ChannelController {
  static const String _unsupportedMessage =
      'WebStorageManager is not implemented on flutter_inappwebview_lwe.';

  /// Creates a new [LweWebStorageManager].
  LweWebStorageManager(PlatformWebStorageManagerCreationParams params)
    : super.implementation(
        params is LweWebStorageManagerCreationParams
            ? params
            : LweWebStorageManagerCreationParams.fromPlatformWebStorageManagerCreationParams(
                params,
              ),
      ) {
    channel = const MethodChannel(
      'com.pichillilorenzo/flutter_inappwebview_webstoragemanager',
    );
    handler = handleMethod;
    initMethodCallHandler();
  }

  static LweWebStorageManager? _instance;

  ///Gets the WebStorage manager shared instance.
  static LweWebStorageManager instance() {
    return (_instance != null) ? _instance! : _init();
  }

  static LweWebStorageManager _init() {
    _instance = LweWebStorageManager(
      LweWebStorageManagerCreationParams(
        const PlatformWebStorageManagerCreationParams(),
      ),
    );
    return _instance!;
  }

  Future<dynamic> _handleMethod(MethodCall call) async {}

  @override
  Future<List<WebStorageOrigin>> getOrigins() async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<void> deleteAllData() async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<void> deleteOrigin({required String origin}) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<int> getQuotaForOrigin({required String origin}) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<int> getUsageForOrigin({required String origin}) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<List<WebsiteDataRecord>> fetchDataRecords({
    required Set<WebsiteDataType> dataTypes,
  }) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<void> removeDataFor({
    required Set<WebsiteDataType> dataTypes,
    required List<WebsiteDataRecord> dataRecords,
  }) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<void> removeDataModifiedSince({
    required Set<WebsiteDataType> dataTypes,
    required DateTime date,
  }) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  void dispose() {
    // empty
  }
}

extension InternalWebStorageManager on LweWebStorageManager {
  Future<dynamic> Function(MethodCall call) get handleMethod => _handleMethod;
}
