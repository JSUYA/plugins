// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences_tizen/src/preference_value_codec.dart';

void main() {
  const String separator = '␞';

  test('round-trips strings containing the legacy separator', () {
    for (final String value in <String>[
      'before${separator}after',
      '${separator}legacy-looking$separator',
    ]) {
      expect(
        PreferenceValueCodec.decode(PreferenceValueCodec.encodeString(value)),
        value,
      );
    }
  });

  test('round-trips string lists containing the legacy separator', () {
    const List<String> value = <String>[
      'before${separator}after',
      separator,
      '',
    ];

    expect(
      PreferenceValueCodec.decode(PreferenceValueCodec.encodeStringList(value)),
      value,
    );
  });

  test('decodes legacy string lists', () {
    expect(
      PreferenceValueCodec.decode('${separator}one${separator}two$separator'),
      <String>['one', 'two'],
    );
    expect(PreferenceValueCodec.decode(separator), <String>[]);
  });

  test('leaves invalid versioned values as strings', () {
    const String value = 'shared_preferences_tizen:v1:not-json';
    expect(PreferenceValueCodec.decode(value), value);
  });
}
