// ignore_for_file: public_member_api_docs

import 'package:flutter/material.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:timezone/data/latest.dart' as tz;
import 'package:timezone/timezone.dart' as tz;

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  tz.initializeTimeZones();
  runApp(const MyApp());
}

final FlutterLocalNotificationsPlugin flutterLocalNotificationsPlugin =
    FlutterLocalNotificationsPlugin();

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  String _status = 'idle';

  @override
  void initState() {
    super.initState();
    _initialize();
  }

  Future<void> _initialize() async {
    final NotificationAppLaunchDetails? launchDetails =
        await flutterLocalNotificationsPlugin.getNotificationAppLaunchDetails();
    final NotificationResponse? launchResponse =
        launchDetails?.notificationResponse;

    await flutterLocalNotificationsPlugin.initialize(
      settings: const InitializationSettings(
        linux: LinuxInitializationSettings(
          defaultActionName: 'Open notification',
        ),
      ),
      onDidReceiveNotificationResponse: (NotificationResponse response) {
        setState(() {
          _status =
              'selected id=${response.id} payload=${response.payload ?? 'null'}';
        });
      },
    );

    if (!mounted) {
      return;
    }

    setState(() {
      _status = launchResponse == null
          ? 'initialized'
          : 'launched from notification id=${launchResponse.id} '
                'payload=${launchResponse.payload ?? 'null'}';
    });
  }

  Future<void> _showNow() async {
    await flutterLocalNotificationsPlugin.show(
      id: 1,
      title: 'Immediate notification',
      body: 'Posted from flutter_local_notifications_tizen',
      payload: 'immediate-payload',
      notificationDetails: const NotificationDetails(
        linux: LinuxNotificationDetails(),
      ),
    );
    setState(() {
      _status = 'show() posted';
    });
  }

  Future<void> _scheduleOneShot() async {
    final tz.TZDateTime scheduledDate = tz.TZDateTime.now(
      tz.local,
    ).add(const Duration(minutes: 1));
    await flutterLocalNotificationsPlugin.zonedSchedule(
      id: 2,
      title: 'Scheduled notification',
      body: 'This notification should appear in one minute.',
      scheduledDate: scheduledDate,
      notificationDetails: const NotificationDetails(
        linux: LinuxNotificationDetails(),
      ),
      androidScheduleMode: AndroidScheduleMode.inexactAllowWhileIdle,
    );
    setState(() {
      _status = 'zonedSchedule() posted for $scheduledDate';
    });
  }

  Future<void> _scheduleDaily() async {
    final tz.TZDateTime scheduledDate = tz.TZDateTime.now(
      tz.local,
    ).add(const Duration(minutes: 2));
    await flutterLocalNotificationsPlugin.zonedSchedule(
      id: 3,
      title: 'Daily notification',
      body: 'Repeats daily at the same time.',
      scheduledDate: scheduledDate,
      notificationDetails: const NotificationDetails(
        linux: LinuxNotificationDetails(),
      ),
      androidScheduleMode: AndroidScheduleMode.inexactAllowWhileIdle,
      matchDateTimeComponents: DateTimeComponents.time,
    );
    setState(() {
      _status = 'daily schedule created';
    });
  }

  Future<void> _loadPending() async {
    final List<PendingNotificationRequest> pending =
        await flutterLocalNotificationsPlugin.pendingNotificationRequests();
    setState(() {
      _status = 'pending=${pending.length}';
    });
  }

  Future<void> _loadActive() async {
    final List<ActiveNotification> active =
        await flutterLocalNotificationsPlugin.getActiveNotifications();
    setState(() {
      _status = 'active=${active.length}';
    });
  }

  Future<void> _cancelAll() async {
    await flutterLocalNotificationsPlugin.cancelAll();
    setState(() {
      _status = 'cancelAll() called';
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('flutter_local_notifications_tizen')),
        body: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: <Widget>[
              Text('status: $_status'),
              const SizedBox(height: 16),
              ElevatedButton(
                onPressed: _showNow,
                child: const Text('Show notification'),
              ),
              ElevatedButton(
                onPressed: _scheduleOneShot,
                child: const Text('Schedule in 1 minute'),
              ),
              ElevatedButton(
                onPressed: _scheduleDaily,
                child: const Text('Schedule daily'),
              ),
              ElevatedButton(
                onPressed: _loadPending,
                child: const Text('Load pending notifications'),
              ),
              ElevatedButton(
                onPressed: _loadActive,
                child: const Text('Load active notifications'),
              ),
              ElevatedButton(
                onPressed: _cancelAll,
                child: const Text('Cancel all'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
