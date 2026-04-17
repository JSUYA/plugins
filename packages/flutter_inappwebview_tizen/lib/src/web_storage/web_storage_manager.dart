import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

/// Object specifying creation parameters for creating a [TizenWebStorageManager].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebStorageManagerCreationParams] for
/// more information.
@immutable
class TizenWebStorageManagerCreationParams
    extends PlatformWebStorageManagerCreationParams {
  /// Creates a new [TizenWebStorageManagerCreationParams] instance.
  const TizenWebStorageManagerCreationParams(
    // This parameter prevents breaking changes later.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebStorageManagerCreationParams params,
  ) : super();

  /// Creates a [TizenWebStorageManagerCreationParams] instance based on [PlatformWebStorageManagerCreationParams].
  factory TizenWebStorageManagerCreationParams.fromPlatformWebStorageManagerCreationParams(
    PlatformWebStorageManagerCreationParams params,
  ) {
    return TizenWebStorageManagerCreationParams(params);
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformWebStorageManager}
class TizenWebStorageManager extends PlatformWebStorageManager
    with ChannelController {
  static const String _unsupportedMessage =
      'WebStorageManager is not implemented on flutter_inappwebview_tizen.';

  /// Creates a new [TizenWebStorageManager].
  TizenWebStorageManager(PlatformWebStorageManagerCreationParams params)
    : super.implementation(
        params is TizenWebStorageManagerCreationParams
            ? params
            : TizenWebStorageManagerCreationParams.fromPlatformWebStorageManagerCreationParams(
                params,
              ),
      ) {
    channel = const MethodChannel(
      'com.pichillilorenzo/flutter_inappwebview_webstoragemanager',
    );
    handler = handleMethod;
    initMethodCallHandler();
  }

  static TizenWebStorageManager? _instance;

  ///Gets the WebStorage manager shared instance.
  static TizenWebStorageManager instance() {
    return (_instance != null) ? _instance! : _init();
  }

  static TizenWebStorageManager _init() {
    _instance = TizenWebStorageManager(
      TizenWebStorageManagerCreationParams(
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

extension InternalWebStorageManager on TizenWebStorageManager {
  Future<dynamic> Function(MethodCall call) get handleMethod => _handleMethod;
}
