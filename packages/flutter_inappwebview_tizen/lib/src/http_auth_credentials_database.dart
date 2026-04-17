import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

/// Object specifying creation parameters for creating a [TizenHttpAuthCredentialDatabase].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformHttpAuthCredentialDatabaseCreationParams] for
/// more information.
@immutable
class TizenHttpAuthCredentialDatabaseCreationParams
    extends PlatformHttpAuthCredentialDatabaseCreationParams {
  /// Creates a new [TizenHttpAuthCredentialDatabaseCreationParams] instance.
  const TizenHttpAuthCredentialDatabaseCreationParams(
    // This parameter prevents breaking changes later.
    // ignore: avoid_unused_constructor_parameters
    PlatformHttpAuthCredentialDatabaseCreationParams params,
  ) : super();

  /// Creates a [TizenHttpAuthCredentialDatabaseCreationParams] instance based on [PlatformHttpAuthCredentialDatabaseCreationParams].
  factory TizenHttpAuthCredentialDatabaseCreationParams.fromPlatformHttpAuthCredentialDatabaseCreationParams(
    PlatformHttpAuthCredentialDatabaseCreationParams params,
  ) {
    return TizenHttpAuthCredentialDatabaseCreationParams(params);
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformHttpAuthCredentialDatabase}
class TizenHttpAuthCredentialDatabase extends PlatformHttpAuthCredentialDatabase
    with ChannelController {
  static const String _unsupportedMessage =
      'HttpAuthCredentialDatabase is not implemented on flutter_inappwebview_tizen.';

  /// Creates a new [TizenHttpAuthCredentialDatabase].
  TizenHttpAuthCredentialDatabase(
    PlatformHttpAuthCredentialDatabaseCreationParams params,
  ) : super.implementation(
        params is TizenHttpAuthCredentialDatabaseCreationParams
            ? params
            : TizenHttpAuthCredentialDatabaseCreationParams.fromPlatformHttpAuthCredentialDatabaseCreationParams(
                params,
              ),
      ) {
    channel = const MethodChannel(
      'com.pichillilorenzo/flutter_inappwebview_credential_database',
    );
    handler = handleMethod;
    initMethodCallHandler();
  }

  static TizenHttpAuthCredentialDatabase? _instance;

  ///Gets the database shared instance.
  static TizenHttpAuthCredentialDatabase instance() {
    return (_instance != null) ? _instance! : _init();
  }

  static TizenHttpAuthCredentialDatabase _init() {
    _instance = TizenHttpAuthCredentialDatabase(
      TizenHttpAuthCredentialDatabaseCreationParams(
        const PlatformHttpAuthCredentialDatabaseCreationParams(),
      ),
    );
    return _instance!;
  }

  Future<dynamic> _handleMethod(MethodCall call) async {}

  @override
  Future<List<URLProtectionSpaceHttpAuthCredentials>>
  getAllAuthCredentials() async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<List<URLCredential>> getHttpAuthCredentials({
    required URLProtectionSpace protectionSpace,
  }) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<void> setHttpAuthCredential({
    required URLProtectionSpace protectionSpace,
    required URLCredential credential,
  }) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<void> removeHttpAuthCredential({
    required URLProtectionSpace protectionSpace,
    required URLCredential credential,
  }) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<void> removeHttpAuthCredentials({
    required URLProtectionSpace protectionSpace,
  }) async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  Future<void> clearAllAuthCredentials() async {
    throw UnsupportedError(_unsupportedMessage);
  }

  @override
  void dispose() {
    // empty
  }
}

extension InternalHttpAuthCredentialDatabase
    on TizenHttpAuthCredentialDatabase {
  Future<dynamic> Function(MethodCall call) get handleMethod => _handleMethod;
}
