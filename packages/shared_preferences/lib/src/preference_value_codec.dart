// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:convert';

/// Encodes string-based preference values for the Tizen native store.
class PreferenceValueCodec {
  static const String _legacySeparator = '␞';
  static const String _versionedPrefix = 'shared_preferences_tizen:v1:';

  /// Encodes a string with type information.
  static String encodeString(String value) =>
      '$_versionedPrefix${jsonEncode(value)}';

  /// Encodes a string list with type information.
  static String encodeStringList(List<String> value) =>
      '$_versionedPrefix${jsonEncode(value)}';

  /// Decodes a string or string list, including the legacy list format.
  static Object decode(String value) {
    if (value.startsWith(_versionedPrefix)) {
      try {
        final Object? decoded = jsonDecode(
          value.substring(_versionedPrefix.length),
        );
        if (decoded is String) {
          return decoded;
        }
        if (decoded is List &&
            decoded.every((Object? item) => item is String)) {
          return List<String>.from(decoded);
        }
      } on FormatException {
        // A value written outside this plugin can begin with the codec prefix.
      }
    }

    if (value == _legacySeparator) {
      return <String>[];
    }
    if (value.startsWith(_legacySeparator) &&
        value.endsWith(_legacySeparator)) {
      final List<String> values = value.split(_legacySeparator);
      return values.sublist(1, values.length - 1);
    }
    return value;
  }
}
