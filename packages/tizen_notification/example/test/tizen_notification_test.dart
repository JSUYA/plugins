// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tizen_notification/tizen_notification.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  const MethodChannel channel = MethodChannel('tizen/notification');

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, null);
  });

  test('show preserves the app control category', () async {
    MethodCall? methodCall;
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (MethodCall call) async {
      methodCall = call;
      return null;
    });

    await TizenNotificationPlugin().show(
      1,
      notificationDetails: TizenNotificationDetails(
        appControl: _FakeAppControl(category: 'test-category'),
      ),
    );

    expect(methodCall?.method, 'show');
    final Map<Object?, Object?> arguments =
        methodCall!.arguments! as Map<Object?, Object?>;
    final Map<Object?, Object?> appControl =
        arguments['appControl']! as Map<Object?, Object?>;
    expect(appControl['category'], 'test-category');
  });
}

class _FakeAppControl extends Fake implements AppControl {
  _FakeAppControl({this.category});

  @override
  String? appId;

  @override
  String? operation;

  @override
  String? uri;

  @override
  String? mime;

  @override
  String? category;

  @override
  Map<String, dynamic> extraData = <String, dynamic>{};
}
