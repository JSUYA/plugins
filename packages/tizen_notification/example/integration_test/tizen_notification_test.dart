// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:tizen_notification/tizen_notification.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  late TizenNotificationPlugin plugin;

  setUp(() {
    plugin = TizenNotificationPlugin();
  });

  tearDown(() async {
    await plugin.cancelAll();
  });

  group('TizenNotificationPlugin', () {
    testWidgets('rejects invalid method channel arguments', (
      WidgetTester tester,
    ) async {
      const MethodChannel channel = MethodChannel('tizen/notification');

      await expectLater(
        channel.invokeMethod<void>('show'),
        throwsA(isA<PlatformException>()),
      );
      await expectLater(
        channel.invokeMethod<void>('cancel', 1),
        throwsA(isA<PlatformException>()),
      );
    });

    testWidgets('show notification does not throw',
        (WidgetTester tester) async {
      await plugin.show(1, title: 'Test Title', body: 'Test Body');
    });

    testWidgets('show notification with default title and body does not throw',
        (WidgetTester tester) async {
      await plugin.show(2);
    });

    testWidgets('cancel notification does not throw',
        (WidgetTester tester) async {
      await plugin.show(3, title: 'To Cancel');
      await plugin.cancel(3);
    });

    testWidgets('cancelAll does not throw', (WidgetTester tester) async {
      await plugin.show(4, title: 'Notification 1');
      await plugin.show(5, title: 'Notification 2');
      await plugin.cancelAll();
    });

    testWidgets(
        'show notification with TizenNotificationDetails does not throw',
        (WidgetTester tester) async {
      final TizenNotificationDetails details = TizenNotificationDetails(
        properties: NotificationProperty.disableAutoDelete,
        style: NotificationStyle.tray,
      );
      await plugin.show(
        6,
        title: 'Detailed',
        body: 'With details',
        notificationDetails: details,
      );
      await plugin.cancel(6);
    });

    testWidgets('show notification preserves app control category', (
      WidgetTester tester,
    ) async {
      final TizenNotificationDetails details = TizenNotificationDetails(
        appControl: AppControl(
          appId: 'org.tizen.tizen_notification_example',
          category: 'http://tizen.org/category/homeapp',
        ),
      );

      await plugin.show(7, notificationDetails: details);
      await plugin.cancel(7);
    });

    testWidgets('rejects invalid app control extra data', (
      WidgetTester tester,
    ) async {
      final TizenNotificationDetails details = TizenNotificationDetails(
        appControl: AppControl(
          appId: 'org.tizen.tizen_notification_example',
          extraData: <String, dynamic>{'invalid': 1},
        ),
      );

      await expectLater(
        plugin.show(8, notificationDetails: details),
        throwsA(isA<PlatformException>()),
      );
    });
  });
}
