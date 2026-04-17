import 'dart:convert';

import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

import '../in_app_webview/in_app_webview_controller.dart';

/// Object specifying creation parameters for creating a [TizenWebStorage].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebStorageCreationParams] for
/// more information.
class TizenWebStorageCreationParams extends PlatformWebStorageCreationParams {
  /// Creates a new [TizenWebStorageCreationParams] instance.
  TizenWebStorageCreationParams({
    required super.localStorage,
    required super.sessionStorage,
  });

  /// Creates a [TizenWebStorageCreationParams] instance based on [PlatformWebStorageCreationParams].
  factory TizenWebStorageCreationParams.fromPlatformWebStorageCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebStorageCreationParams params,
  ) {
    return TizenWebStorageCreationParams(
      localStorage: params.localStorage,
      sessionStorage: params.sessionStorage,
    );
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformWebStorage}
class TizenWebStorage extends PlatformWebStorage {
  /// Constructs a [TizenWebStorage].
  TizenWebStorage(PlatformWebStorageCreationParams params)
    : super.implementation(
        params is TizenWebStorageCreationParams
            ? params
            : TizenWebStorageCreationParams.fromPlatformWebStorageCreationParams(
                params,
              ),
      );

  @override
  PlatformLocalStorage get localStorage => params.localStorage;

  @override
  PlatformSessionStorage get sessionStorage => params.sessionStorage;

  @override
  void dispose() {
    localStorage.dispose();
    sessionStorage.dispose();
  }
}

/// Object specifying creation parameters for creating a [TizenStorage].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformStorageCreationParams] for
/// more information.
class TizenStorageCreationParams extends PlatformStorageCreationParams {
  /// Creates a new [TizenStorageCreationParams] instance.
  TizenStorageCreationParams({
    required super.controller,
    required super.webStorageType,
  });

  /// Creates a [TizenStorageCreationParams] instance based on [PlatformStorageCreationParams].
  factory TizenStorageCreationParams.fromPlatformStorageCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformStorageCreationParams params,
  ) {
    return TizenStorageCreationParams(
      controller: params.controller,
      webStorageType: params.webStorageType,
    );
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformStorage}
abstract mixin class TizenStorage implements PlatformStorage {
  @override
  TizenInAppWebViewController? controller;

  @override
  Future<int?> length() async {
    var result = await controller?.evaluateJavascript(
      source:
          """
    window.$webStorageType.length;
    """,
    );
    return result != null ? int.parse(json.decode(result)) : null;
  }

  @override
  Future<void> setItem({required String key, required dynamic value}) async {
    var encodedValue = json.encode(value);
    await controller?.evaluateJavascript(
      source:
          """
    window.$webStorageType.setItem("$key", ${value is String ? encodedValue : "JSON.stringify($encodedValue)"});
    """,
    );
  }

  @override
  Future<dynamic> getItem({required String key}) async {
    var itemValue = await controller?.evaluateJavascript(
      source:
          """
    window.$webStorageType.getItem("$key");
    """,
    );

    if (itemValue == null) {
      return null;
    }

    try {
      return json.decode(itemValue);
    } catch (e) {}

    return itemValue;
  }

  @override
  Future<void> removeItem({required String key}) async {
    await controller?.evaluateJavascript(
      source:
          """
    window.$webStorageType.removeItem("$key");
    """,
    );
  }

  @override
  Future<List<WebStorageItem>> getItems() async {
    var webStorageItems = <WebStorageItem>[];

    List<Map<dynamic, dynamic>>? items = (await controller?.evaluateJavascript(
      source:
          """
(function() {
  var webStorageItems = [];
  for(var i = 0; i < window.$webStorageType.length; i++){
    var key = window.$webStorageType.key(i);
    webStorageItems.push(
      {
        key: key,
        value: window.$webStorageType.getItem(key)
      }
    );
  }
  return webStorageItems;
})();
    """,
    ))?.cast<Map<dynamic, dynamic>>();

    if (items == null) {
      return webStorageItems;
    }

    for (var item in items) {
      webStorageItems.add(
        WebStorageItem(key: item["key"], value: item["value"]),
      );
    }

    return webStorageItems;
  }

  @override
  Future<void> clear() async {
    await controller?.evaluateJavascript(
      source:
          """
    window.$webStorageType.clear();
    """,
    );
  }

  @override
  Future<String> key({required int index}) async {
    var result = await controller?.evaluateJavascript(
      source:
          """
    window.$webStorageType.key($index);
    """,
    );
    return result != null ? json.decode(result) : null;
  }

  @override
  void dispose() {
    controller = null;
  }
}

/// Object specifying creation parameters for creating a [TizenLocalStorage].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformLocalStorageCreationParams] for
/// more information.
class TizenLocalStorageCreationParams
    extends PlatformLocalStorageCreationParams {
  /// Creates a new [TizenLocalStorageCreationParams] instance.
  TizenLocalStorageCreationParams(super.params);

  /// Creates a [TizenLocalStorageCreationParams] instance based on [PlatformLocalStorageCreationParams].
  factory TizenLocalStorageCreationParams.fromPlatformLocalStorageCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformLocalStorageCreationParams params,
  ) {
    return TizenLocalStorageCreationParams(params);
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformLocalStorage}
class TizenLocalStorage extends PlatformLocalStorage with TizenStorage {
  /// Constructs a [TizenLocalStorage].
  TizenLocalStorage(PlatformLocalStorageCreationParams params)
    : super.implementation(
        params is TizenLocalStorageCreationParams
            ? params
            : TizenLocalStorageCreationParams.fromPlatformLocalStorageCreationParams(
                params,
              ),
      );

  /// Default storage
  factory TizenLocalStorage.defaultStorage({
    required PlatformInAppWebViewController? controller,
  }) {
    return TizenLocalStorage(
      TizenLocalStorageCreationParams(
        PlatformLocalStorageCreationParams(
          PlatformStorageCreationParams(
            controller: controller,
            webStorageType: WebStorageType.LOCAL_STORAGE,
          ),
        ),
      ),
    );
  }

  @override
  TizenInAppWebViewController? get controller =>
      params.controller as TizenInAppWebViewController?;
}

/// Object specifying creation parameters for creating a [TizenSessionStorage].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformSessionStorageCreationParams] for
/// more information.
class TizenSessionStorageCreationParams
    extends PlatformSessionStorageCreationParams {
  /// Creates a new [TizenSessionStorageCreationParams] instance.
  TizenSessionStorageCreationParams(super.params);

  /// Creates a [TizenSessionStorageCreationParams] instance based on [PlatformSessionStorageCreationParams].
  factory TizenSessionStorageCreationParams.fromPlatformSessionStorageCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformSessionStorageCreationParams params,
  ) {
    return TizenSessionStorageCreationParams(params);
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformSessionStorage}
class TizenSessionStorage extends PlatformSessionStorage with TizenStorage {
  /// Constructs a [TizenSessionStorage].
  TizenSessionStorage(PlatformSessionStorageCreationParams params)
    : super.implementation(
        params is TizenSessionStorageCreationParams
            ? params
            : TizenSessionStorageCreationParams.fromPlatformSessionStorageCreationParams(
                params,
              ),
      );

  /// Default storage
  factory TizenSessionStorage.defaultStorage({
    required PlatformInAppWebViewController? controller,
  }) {
    return TizenSessionStorage(
      TizenSessionStorageCreationParams(
        PlatformSessionStorageCreationParams(
          PlatformStorageCreationParams(
            controller: controller,
            webStorageType: WebStorageType.SESSION_STORAGE,
          ),
        ),
      ),
    );
  }

  @override
  TizenInAppWebViewController? get controller =>
      params.controller as TizenInAppWebViewController?;
}
