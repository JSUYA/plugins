// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:convert';

import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:google_sign_in_platform_interface/google_sign_in_platform_interface.dart';

import 'src/device_flow_widget.dart' as device_flow_widget;
import 'src/oauth2.dart';

export 'src/authorization_exception.dart';

/// Holds authentication data after Google sign in for Tizen.
class _GoogleSignInTokenDataTizen {
  /// Creates an instance of [_GoogleSignInTokenDataTizen].
  _GoogleSignInTokenDataTizen({
    required this.accessToken,
    required this.accessTokenExpirationDate,
    required this.idToken,
    this.refreshToken,
  });

  /// The access token issued by the authorization server.
  final String accessToken;

  /// The estimated expiration date of [accessToken].
  final DateTime accessTokenExpirationDate;

  /// The ID token issued by the Google authorization server.
  final String idToken;

  /// The OAuth2 refresh token to exchange for new access tokens.
  final String? refreshToken;

  /// Returns `true` if [accessToken] is expired and needs to be refreshed,
  /// otherwise `false`.
  bool get isExpired {
    const Duration minimalTimeToExpire = Duration(minutes: 1);
    return accessTokenExpirationDate
        .add(minimalTimeToExpire)
        .isBefore(DateTime.now());
  }

  /// Creates a [_GoogleSignInTokenDataTizen] from a json object.
  static _GoogleSignInTokenDataTizen fromJson(Map<String, Object?> json) {
    return _GoogleSignInTokenDataTizen(
      accessToken: json['access_token']! as String,
      accessTokenExpirationDate: DateTime.parse(
        json['access_token_expiration_date']! as String,
      ),
      idToken: json['id_token']! as String,
      refreshToken: json['refresh_token'] as String?,
    );
  }

  /// Creates a json object from this token data.
  Map<String, Object> toJson() {
    return <String, Object>{
      'access_token': accessToken,
      'access_token_expiration_date': accessTokenExpirationDate.toString(),
      'id_token': idToken,
      if (refreshToken != null) 'refresh_token': refreshToken!,
    };
  }
}

/// The set of "Client ID" and "Client Secret" issued by the authorization server.
class _Credentials {
  const _Credentials(this.clientId, this.clientSecret);

  /// The unique public identifier for apps that is issued by the authorization
  /// server. It's analogous to a login id.
  final String clientId;

  /// The secret credential known only to the application and the authorization
  /// server, it's analogous to a password.
  final String clientSecret;
}

class _CachedTokenStorage {
  // ignore: invalid_use_of_visible_for_testing_member
  final FlutterSecureStorage _storage = const FlutterSecureStorage();

  final String _kToken = 'token';

  /// Cached token.
  _GoogleSignInTokenDataTizen? _token;

  Future<void> saveToken(_GoogleSignInTokenDataTizen token) async {
    await _storage.write(key: _kToken, value: jsonEncode(token.toJson()));
    _token = token;
  }

  Future<_GoogleSignInTokenDataTizen?> getToken() async {
    if (_token != null) {
      return _token!;
    }
    final String? jsonString = await _storage.read(key: _kToken);
    return jsonString != null
        ? _GoogleSignInTokenDataTizen.fromJson(
            jsonDecode(jsonString) as Map<String, Object?>,
          )
        : null;
  }

  Future<void> removeToken() async {
    await _storage.delete(key: _kToken);
    _token = null;
  }
}

/// Tizen implementation of [GoogleSignInPlatform].
class GoogleSignInTizen extends GoogleSignInPlatform {
  /// Registers this class as the default instance of [GoogleSignInPlatform].
  static void register() {
    GoogleSignInPlatform.instance = GoogleSignInTizen();
  }

  static _Credentials? _credentials;

  final _CachedTokenStorage _storage = _CachedTokenStorage();

  List<String> _scopes = <String>[];

  final DeviceAuthClient _authClient = DeviceAuthClient(
    authorizationEndPoint: Uri.parse(
      'https://oauth2.googleapis.com/device/code',
    ),
    tokenEndPoint: Uri.parse('https://oauth2.googleapis.com/token'),
    revokeEndPoint: Uri.parse('https://oauth2.googleapis.com/revoke'),
  );

  /// Sets [clientId] and [clientSecret] to be used for GoogleSignIn authentication.
  ///
  /// This must be called before calling the GoogleSignIn's signIn API.
  static void setCredentials({
    required String clientId,
    required String clientSecret,
  }) {
    _credentials = _Credentials(clientId, clientSecret);
  }

  /// Gets the [GlobalKey] that identifies a [NavigatorState].
  ///
  /// This object must be assigned to a valid [Navigator] widget to push
  /// a dialog that shows "verification url" and "user code" which are
  /// required to authorize sign-in.
  ///
  /// If [MaterialApp] or [CupertinoApp] is used, it's convinient to
  /// assign this object to their `navigatorKey` parameter.
  static GlobalKey<NavigatorState> get navigatorKey =>
      device_flow_widget.navigatorKey;

  /// Sets the [GlobalKey] that identifies a [NavigatorState].
  ///
  /// This object must be set if [GlobalKey] needs to be instantiated from
  /// client code.
  static set navigatorKey(GlobalKey<NavigatorState> navigatorKey) =>
      device_flow_widget.navigatorKey = navigatorKey;

  void _ensureSetCredentials() {
    if (_credentials == null) {
      throw PlatformException(
        code: 'credentials-missing',
        message: 'Cannot initialize GoogleSignInTizen: ClientID and '
            'ClientSecret has not been set, first call `setCredentials` '
            "in google_sign_in_tizen.dart before calling GoogleSignIn's signIn API.",
      );
    }
  }

  void _ensureNavigatorKeyAssigned() {
    if (device_flow_widget.navigatorKey.currentContext == null) {
      throw PlatformException(
        code: 'navigatorkey-unassigned',
        message: 'Cannot initialize GoogleSignInTizen: a default or custom '
            'navigator key must be assigned to `navigatorKey` parameter in '
            '`MaterialApp` or `CupertinoApp`.',
      );
    }
  }

  @override
  Future<void> init(InitParameters params) async {
    _ensureSetCredentials();
    if (params.clientId != null) {
      _credentials = _Credentials(params.clientId!, _credentials!.clientSecret);
    }
  }

  @override
  bool supportsAuthenticate() => true;

  @override
  Future<AuthenticationResults?> attemptLightweightAuthentication(
    AttemptLightweightAuthenticationParameters params,
  ) async {
    final _GoogleSignInTokenDataTizen? existingToken =
        await _storage.getToken();
    if (existingToken == null) {
      return null;
    }
    // Check if access token expired.
    if (!existingToken.isExpired) {
      return _createAuthResults(existingToken.idToken);
    }
    final _GoogleSignInTokenDataTizen token = await _refreshToken(
      existingToken,
    );
    await _storage.saveToken(token);

    return _createAuthResults(token.idToken);
  }

  @override
  Future<AuthenticationResults> authenticate(
    AuthenticateParameters params,
  ) async {
    _ensureSetCredentials();
    _ensureNavigatorKeyAssigned();

    final List<String> scopes =
        params.scopeHint.isNotEmpty ? params.scopeHint : _scopes;

    final AuthorizationResponse authorizationResponse =
        await _authClient.requestAuthorization(
      _credentials!.clientId,
      scopes,
    );

    final Future<TokenResponse?> tokenResponseFuture = _authClient.pollToken(
      clientId: _credentials!.clientId,
      clientSecret: _credentials!.clientSecret,
      deviceCode: authorizationResponse.deviceCode,
      interval: authorizationResponse.interval,
    );

    device_flow_widget.showDeviceFlowWidget(
      code: authorizationResponse.userCode,
      verificationUrl: authorizationResponse.verificationUrl,
      expiresIn: authorizationResponse.expiresIn,
      onExpired: () => _authClient.cancelPollToken(),
      onCanceled: () => _authClient.cancelPollToken(),
    );

    // Waits until user interaction on secondary device is finished, code is expired,
    // polling is cancelled, or networking error occurred.
    final TokenResponse? tokenResponse = await tokenResponseFuture.onError((
      _,
      __,
    ) {
      device_flow_widget.closeDeviceFlowWidget();
      return null;
    });
    if (tokenResponse == null) {
      throw PlatformException(
        code: 'sign-in-canceled',
        message: 'Sign in was canceled or failed.',
      );
    }
    device_flow_widget.closeDeviceFlowWidget();

    final _GoogleSignInTokenDataTizen token = _GoogleSignInTokenDataTizen(
      accessToken: tokenResponse.accessToken,
      accessTokenExpirationDate: DateTime.now().add(tokenResponse.expiresIn),
      refreshToken: tokenResponse.refreshToken,
      idToken: tokenResponse.idToken,
    );
    await _storage.saveToken(token);

    return _createAuthResults(token.idToken);
  }

  @override
  bool authorizationRequiresUserInteraction() => true;

  @override
  Future<ClientAuthorizationTokenData?> clientAuthorizationTokensForScopes(
    ClientAuthorizationTokensForScopesParameters params,
  ) async {
    final _GoogleSignInTokenDataTizen? existingToken =
        await _storage.getToken();
    if (existingToken == null) {
      return null;
    }

    // Check if access token expired.
    if (!existingToken.isExpired) {
      return ClientAuthorizationTokenData(
        accessToken: existingToken.accessToken,
      );
    }
    final _GoogleSignInTokenDataTizen token = await _refreshToken(
      existingToken,
    );

    await _storage.saveToken(token);
    return ClientAuthorizationTokenData(accessToken: token.accessToken);
  }

  @override
  Future<ServerAuthorizationTokenData?> serverAuthorizationTokensForScopes(
    ServerAuthorizationTokensForScopesParameters params,
  ) {
    throw UnimplementedError(
      'serverAuthorizationTokensForScopes() has not been implemented.',
    );
  }

  @override
  Future<void> signOut(SignOutParams params) => _storage.removeToken();

  @override
  Future<void> disconnect(DisconnectParams params) async {
    final _GoogleSignInTokenDataTizen? existingToken =
        await _storage.getToken();
    if (existingToken == null) {
      return;
    }

    await _authClient.revokeToken(existingToken.accessToken);
    await signOut(const SignOutParams());
  }

  AuthenticationResults _createAuthResults(String idToken) {
    // Decodes JWT payload as a json object.
    final List<String> splitTokens = idToken.split('.');
    if (splitTokens.length != 3) {
      throw const FormatException('Invalid idToken.');
    }
    final String normalizedPayload = base64.normalize(splitTokens[1]);
    final String payloadString = utf8.decode(base64.decode(normalizedPayload));
    final Map<String, Object?> json =
        jsonDecode(payloadString) as Map<String, Object?>;

    return AuthenticationResults(
      user: GoogleSignInUserData(
        email: json['email']! as String,
        id: json['sub']! as String,
        displayName: json['name'] as String?,
        photoUrl: json['picture'] as String?,
      ),
      authenticationTokens: AuthenticationTokenData(idToken: idToken),
    );
  }

  Future<_GoogleSignInTokenDataTizen> _refreshToken(
    _GoogleSignInTokenDataTizen token,
  ) async {
    if (token.refreshToken == null) {
      throw PlatformException(
        code: 'refresh-token-missing',
        message: 'Cannot refresh tokens as refresh tokens are missing. '
            'Request new tokens by signing-in again.',
      );
    }
    _ensureSetCredentials();

    final TokenResponse tokenResponse = await _authClient.refreshToken(
      clientId: _credentials!.clientId,
      clientSecret: _credentials!.clientSecret,
      refreshToken: token.refreshToken!,
    );

    return _GoogleSignInTokenDataTizen(
      accessToken: tokenResponse.accessToken,
      accessTokenExpirationDate: DateTime.now().add(tokenResponse.expiresIn),
      refreshToken: tokenResponse.refreshToken ?? token.refreshToken,
      idToken: tokenResponse.idToken,
    );
  }
}
