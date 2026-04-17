import 'dart:convert';

import 'package:flutter_inappwebview_platform_interface/flutter_inappwebview_platform_interface.dart';

import '../in_app_webview/in_app_webview_controller.dart';

/// Object specifying creation parameters for creating a [LweWebStorage].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformWebStorageCreationParams] for
/// more information.
class LweWebStorageCreationParams extends PlatformWebStorageCreationParams {
  /// Creates a new [LweWebStorageCreationParams] instance.
  LweWebStorageCreationParams({
    required super.localStorage,
    required super.sessionStorage,
  });

  /// Creates a [LweWebStorageCreationParams] instance based on [PlatformWebStorageCreationParams].
  factory LweWebStorageCreationParams.fromPlatformWebStorageCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformWebStorageCreationParams params,
  ) {
    return LweWebStorageCreationParams(
      localStorage: params.localStorage,
      sessionStorage: params.sessionStorage,
    );
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformWebStorage}
class LweWebStorage extends PlatformWebStorage {
  /// Constructs a [LweWebStorage].
  LweWebStorage(PlatformWebStorageCreationParams params)
    : super.implementation(
        params is LweWebStorageCreationParams
            ? params
            : LweWebStorageCreationParams.fromPlatformWebStorageCreationParams(
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

/// Object specifying creation parameters for creating a [LweStorage].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformStorageCreationParams] for
/// more information.
class LweStorageCreationParams extends PlatformStorageCreationParams {
  /// Creates a new [LweStorageCreationParams] instance.
  LweStorageCreationParams({
    required super.controller,
    required super.webStorageType,
  });

  /// Creates a [LweStorageCreationParams] instance based on [PlatformStorageCreationParams].
  factory LweStorageCreationParams.fromPlatformStorageCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformStorageCreationParams params,
  ) {
    return LweStorageCreationParams(
      controller: params.controller,
      webStorageType: params.webStorageType,
    );
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformStorage}
abstract mixin class LweStorage implements PlatformStorage {
  @override
  LweInAppWebViewController? controller;

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

/// Object specifying creation parameters for creating a [LweLocalStorage].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformLocalStorageCreationParams] for
/// more information.
class LweLocalStorageCreationParams extends PlatformLocalStorageCreationParams {
  /// Creates a new [LweLocalStorageCreationParams] instance.
  LweLocalStorageCreationParams(super.params);

  /// Creates a [LweLocalStorageCreationParams] instance based on [PlatformLocalStorageCreationParams].
  factory LweLocalStorageCreationParams.fromPlatformLocalStorageCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformLocalStorageCreationParams params,
  ) {
    return LweLocalStorageCreationParams(params);
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformLocalStorage}
class LweLocalStorage extends PlatformLocalStorage with LweStorage {
  /// Constructs a [LweLocalStorage].
  LweLocalStorage(PlatformLocalStorageCreationParams params)
    : super.implementation(
        params is LweLocalStorageCreationParams
            ? params
            : LweLocalStorageCreationParams.fromPlatformLocalStorageCreationParams(
                params,
              ),
      );

  /// Default storage
  factory LweLocalStorage.defaultStorage({
    required PlatformInAppWebViewController? controller,
  }) {
    return LweLocalStorage(
      LweLocalStorageCreationParams(
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
  LweInAppWebViewController? get controller =>
      params.controller as LweInAppWebViewController?;
}

/// Object specifying creation parameters for creating a [LweSessionStorage].
///
/// When adding additional fields make sure they can be null or have a default
/// value to avoid breaking changes. See [PlatformSessionStorageCreationParams] for
/// more information.
class LweSessionStorageCreationParams
    extends PlatformSessionStorageCreationParams {
  /// Creates a new [LweSessionStorageCreationParams] instance.
  LweSessionStorageCreationParams(super.params);

  /// Creates a [LweSessionStorageCreationParams] instance based on [PlatformSessionStorageCreationParams].
  factory LweSessionStorageCreationParams.fromPlatformSessionStorageCreationParams(
    // Recommended placeholder to prevent being broken by platform interface.
    // ignore: avoid_unused_constructor_parameters
    PlatformSessionStorageCreationParams params,
  ) {
    return LweSessionStorageCreationParams(params);
  }
}

///{@macro flutter_inappwebview_platform_interface.PlatformSessionStorage}
class LweSessionStorage extends PlatformSessionStorage with LweStorage {
  /// Constructs a [LweSessionStorage].
  LweSessionStorage(PlatformSessionStorageCreationParams params)
    : super.implementation(
        params is LweSessionStorageCreationParams
            ? params
            : LweSessionStorageCreationParams.fromPlatformSessionStorageCreationParams(
                params,
              ),
      );

  /// Default storage
  factory LweSessionStorage.defaultStorage({
    required PlatformInAppWebViewController? controller,
  }) {
    return LweSessionStorage(
      LweSessionStorageCreationParams(
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
  LweInAppWebViewController? get controller =>
      params.controller as LweInAppWebViewController?;
}
