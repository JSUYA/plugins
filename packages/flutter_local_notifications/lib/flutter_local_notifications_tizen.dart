// ignore_for_file: always_specify_types
// ignore_for_file: avoid_redundant_argument_values
// ignore_for_file: avoid_slow_async_io
// ignore_for_file: cancel_subscriptions
// ignore_for_file: directives_ordering
// ignore_for_file: public_member_api_docs
// ignore_for_file: unused_field
// ignore_for_file: use_late_for_private_fields_and_variables

import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';
import 'package:flutter_local_notifications_linux/flutter_local_notifications_linux.dart';
import 'package:flutter_local_notifications_platform_interface/flutter_local_notifications_platform_interface.dart';
import 'package:tizen_app_control/tizen_app_control.dart';
import 'package:tizen_app_manager/tizen_app_manager.dart';
import 'package:tizen_interop/6.0/tizen.dart';
import 'package:timezone/timezone.dart' as tz;

class FlutterLocalNotificationsTizenPlugin
    extends LinuxFlutterLocalNotificationsPlugin {
  FlutterLocalNotificationsTizenPlugin._() {
    _appControlSubscription = AppControl.onAppControl.listen(_handleAppControl);
  }

  static FlutterLocalNotificationsTizenPlugin? _instance;

  static void register() {
    _instance ??= FlutterLocalNotificationsTizenPlugin._();
    FlutterLocalNotificationsPlatform.instance = _instance!;
  }

  static const String _tagPrefix = 'flutter_local_notifications_tizen:';
  static const String _extraIdKey =
      'flutter_local_notifications_tizen_notification_id';
  static const String _extraPayloadKey =
      'flutter_local_notifications_tizen_notification_payload';

  final _PluginStateStore _stateStore = _PluginStateStore();

  LinuxInitializationSettings? _initializationSettings;
  DidReceiveNotificationResponseCallback? _onDidReceiveNotificationResponse;
  StreamSubscription<ReceivedAppControl>? _appControlSubscription;
  final List<NotificationResponse> _queuedResponses = <NotificationResponse>[];
  NotificationAppLaunchDetails? _launchDetails;
  String? _cachedAppId;
  AppInfo? _cachedAppInfo;

  @override
  Future<bool?> initialize({
    required LinuxInitializationSettings settings,
    DidReceiveNotificationResponseCallback? onDidReceiveNotificationResponse,
  }) async {
    _initializationSettings = settings;
    _onDidReceiveNotificationResponse = onDidReceiveNotificationResponse;

    if (_queuedResponses.isNotEmpty &&
        onDidReceiveNotificationResponse != null) {
      for (final NotificationResponse response
          in List<NotificationResponse>.from(_queuedResponses)) {
        unawaited(
          Future<void>.sync(() {
            onDidReceiveNotificationResponse(response);
          }),
        );
      }
      _queuedResponses.clear();
    }
    return true;
  }

  @override
  Future<NotificationAppLaunchDetails?>
  getNotificationAppLaunchDetails() async {
    return _launchDetails ?? const NotificationAppLaunchDetails(false);
  }

  @override
  Future<void> show({
    required int id,
    String? title,
    String? body,
    LinuxNotificationDetails? notificationDetails,
    String? payload,
  }) async {
    validateId(id);
    await _deleteActiveNotification(id);

    final _NotificationRecord record = _NotificationRecord(
      id: id,
      title: title,
      body: body,
      payload: payload,
    );
    await _persistRecord(record);

    await _postNotification(
      id: id,
      title: title,
      body: body,
      payload: payload,
      notificationDetails: notificationDetails,
    );
  }

  @override
  Future<void> zonedSchedule({
    required int id,
    String? title,
    String? body,
    required tz.TZDateTime scheduledDate,
    String? payload,
    DateTimeComponents? matchDateTimeComponents,
  }) async {
    validateId(id);

    final tz.TZDateTime nextFireDate = _resolveNextScheduledDate(
      scheduledDate,
      matchDateTimeComponents,
    );

    if (matchDateTimeComponents == DateTimeComponents.dayOfMonthAndTime ||
        matchDateTimeComponents == DateTimeComponents.dateAndTime) {
      throw UnsupportedError(
        'Tizen does not support monthly or yearly local notification '
        'recurrence through public alarm APIs.',
      );
    }

    final _NotificationRecord record = _NotificationRecord(
      id: id,
      title: title,
      body: body,
      payload: payload,
    );

    if (matchDateTimeComponents == null) {
      record.alarmId = await _scheduleOneShot(
        id: id,
        title: title,
        body: body,
        payload: payload,
        scheduledDate: nextFireDate,
      );
    } else {
      record.alarmId = await _scheduleWeeklyRecurrence(
        id: id,
        title: title,
        body: body,
        payload: payload,
        scheduledDate: nextFireDate,
        weekFlag: _weekFlagForDateTimeComponents(
          matchDateTimeComponents,
          nextFireDate,
        ),
      );
    }

    await _persistRecord(record);
  }

  @override
  Future<void> periodicallyShow({
    required int id,
    String? title,
    String? body,
    required RepeatInterval repeatInterval,
  }) async {
    validateId(id);

    final Duration interval = switch (repeatInterval) {
      RepeatInterval.everyMinute => const Duration(minutes: 1),
      RepeatInterval.hourly => const Duration(hours: 1),
      RepeatInterval.daily => const Duration(days: 1),
      RepeatInterval.weekly => const Duration(days: 7),
    };

    if (repeatInterval == RepeatInterval.everyMinute) {
      throw UnsupportedError(
        'Tizen repeating alarms require a minimum interval of 15 minutes.',
      );
    }

    final _NotificationRecord record = _NotificationRecord(
      id: id,
      title: title,
      body: body,
      payload: null,
    );
    record.alarmId = await _schedulePeriodic(
      id: id,
      title: title,
      body: body,
      payload: null,
      firstDelay: interval,
      period: interval,
    );
    await _persistRecord(record);
  }

  @override
  Future<void> periodicallyShowWithDuration({
    required int id,
    String? title,
    String? body,
    required Duration repeatDurationInterval,
  }) async {
    validateId(id);
    validateRepeatDurationInterval(repeatDurationInterval);

    if (repeatDurationInterval < const Duration(minutes: 15)) {
      throw UnsupportedError(
        'Tizen repeating alarms require a minimum interval of 15 minutes.',
      );
    }

    final _NotificationRecord record = _NotificationRecord(
      id: id,
      title: title,
      body: body,
      payload: null,
    );
    record.alarmId = await _schedulePeriodic(
      id: id,
      title: title,
      body: body,
      payload: null,
      firstDelay: repeatDurationInterval,
      period: repeatDurationInterval,
    );
    await _persistRecord(record);
  }

  @override
  Future<void> cancel({required int id}) async {
    validateId(id);

    final _PluginState state = await _stateStore.read();
    final _NotificationRecord? record = state.records.remove(id);
    if (record?.alarmId != null) {
      _cancelAlarm(record!.alarmId!);
    }

    await _deleteActiveNotification(id);
    await _stateStore.write(state);
  }

  @override
  Future<void> cancelAll() async {
    final _PluginState state = await _stateStore.read();
    for (final _NotificationRecord record in state.records.values) {
      if (record.alarmId != null) {
        _cancelAlarm(record.alarmId!);
      }
      await _deleteActiveNotification(record.id);
    }
    state.records.clear();
    await _stateStore.write(state);
  }

  @override
  Future<void> cancelAllPendingNotifications() async {
    final _PluginState state = await _stateStore.read();
    for (final _NotificationRecord record in state.records.values) {
      if (record.alarmId != null) {
        _cancelAlarm(record.alarmId!);
        record.alarmId = null;
      }
    }
    state.records.removeWhere(
      (_, _NotificationRecord record) => !_shouldKeepRecord(record),
    );
    await _stateStore.write(state);
  }

  @override
  Future<List<PendingNotificationRequest>> pendingNotificationRequests() async {
    final _PluginState state = await _stateStore.read();
    bool changed = false;
    final List<PendingNotificationRequest> requests =
        <PendingNotificationRequest>[];

    for (final _NotificationRecord record in state.records.values) {
      final int? alarmId = record.alarmId;
      if (alarmId == null) {
        continue;
      }

      if (await _isAlarmScheduled(alarmId)) {
        requests.add(
          PendingNotificationRequest(
            record.id,
            record.title,
            record.body,
            record.payload,
          ),
        );
      } else {
        record.alarmId = null;
        changed = true;
      }
    }

    if (changed) {
      state.records.removeWhere(
        (_, _NotificationRecord record) => !_shouldKeepRecord(record),
      );
      await _stateStore.write(state);
    }

    return requests;
  }

  @override
  Future<List<ActiveNotification>> getActiveNotifications() async {
    final _PluginState state = await _stateStore.read();
    bool changed = false;
    final List<ActiveNotification> notifications = <ActiveNotification>[];

    for (final _NotificationRecord record in state.records.values) {
      final bool isActive = _isActiveNotification(record.id);

      if (!isActive && record.alarmId != null) {
        if (!(await _isAlarmScheduled(record.alarmId!))) {
          record.alarmId = null;
          changed = true;
        }
      }

      if (isActive) {
        notifications.add(
          ActiveNotification(
            id: record.id,
            title: record.title,
            body: record.body,
            payload: record.payload,
          ),
        );
      } else if (!_shouldKeepRecord(record)) {
        changed = true;
      }
    }

    if (changed) {
      state.records.removeWhere(
        (_, _NotificationRecord record) => !_shouldKeepRecord(record),
      );
      await _stateStore.write(state);
    }

    return notifications;
  }

  @override
  Future<LinuxServerCapabilities> getCapabilities() async {
    return const LinuxServerCapabilities(
      otherCapabilities: <String>{},
      body: true,
      bodyHyperlinks: false,
      bodyImages: false,
      bodyMarkup: false,
      iconMulti: false,
      iconStatic: true,
      persistence: true,
      sound: true,
      actions: false,
      actionIcons: false,
    );
  }

  @override
  Future<Map<int, int>> getSystemIdMap() async {
    return <int, int>{};
  }

  void _handleAppControl(ReceivedAppControl request) {
    final int? id = _intExtra(request.extraData[_extraIdKey]);
    if (id == null) {
      return;
    }

    final NotificationResponse response = NotificationResponse(
      notificationResponseType: NotificationResponseType.selectedNotification,
      id: id,
      payload: _stringExtra(request.extraData[_extraPayloadKey]),
    );

    _launchDetails ??= NotificationAppLaunchDetails(
      true,
      notificationResponse: response,
    );

    final DidReceiveNotificationResponseCallback? callback =
        _onDidReceiveNotificationResponse;
    if (callback != null) {
      unawaited(
        Future<void>.sync(() {
          callback(response);
        }),
      );
    } else {
      _queuedResponses.add(response);
    }
  }

  Future<void> _persistRecord(_NotificationRecord record) async {
    final _PluginState state = await _stateStore.read();
    state.records[record.id] = record;
    await _stateStore.write(state);
  }

  Future<String> _currentAppId() async {
    return _cachedAppId ??= await AppManager.currentAppId;
  }

  Future<AppInfo> _currentAppInfo() async {
    if (_cachedAppInfo != null) {
      return _cachedAppInfo!;
    }
    final String appId = await _currentAppId();
    _cachedAppInfo = await AppManager.getAppInfo(appId);
    return _cachedAppInfo!;
  }

  Future<void> _postNotification({
    required int id,
    String? title,
    String? body,
    String? payload,
    LinuxNotificationDetails? notificationDetails,
  }) async {
    await _createNotificationHandle(
      id: id,
      title: title,
      body: body,
      payload: payload,
      notificationDetails: notificationDetails,
      action: (_CreatedNotification created) {
        _checkResult(
          tizen.notification_post(created.notification),
          'notification_post',
        );
      },
    );
  }

  Future<int> _scheduleOneShot({
    required int id,
    String? title,
    String? body,
    String? payload,
    required tz.TZDateTime scheduledDate,
  }) async {
    return _createNotificationHandle(
      id: id,
      title: title,
      body: body,
      payload: payload,
      notificationDetails: null,
      action: (_CreatedNotification created) {
        final Pointer<tm> tmPtr = calloc<tm>();
        final Pointer<Int> alarmIdPtr = calloc<Int>();
        try {
          _writeTm(tmPtr.ref, scheduledDate);
          _checkResult(
            tizen.alarm_schedule_noti_once_at_date(
              created.notification,
              tmPtr,
              alarmIdPtr,
            ),
            'alarm_schedule_noti_once_at_date',
          );
          return alarmIdPtr.value;
        } finally {
          calloc.free(tmPtr);
          calloc.free(alarmIdPtr);
        }
      },
    );
  }

  Future<int> _scheduleWeeklyRecurrence({
    required int id,
    String? title,
    String? body,
    String? payload,
    required tz.TZDateTime scheduledDate,
    required int weekFlag,
  }) async {
    return _createNotificationHandle(
      id: id,
      title: title,
      body: body,
      payload: payload,
      notificationDetails: null,
      action: (_CreatedNotification created) {
        final Pointer<tm> tmPtr = calloc<tm>();
        final Pointer<Int> alarmIdPtr = calloc<Int>();
        try {
          _writeTm(tmPtr.ref, scheduledDate);
          _checkResult(
            tizen.alarm_schedule_noti_with_recurrence_week_flag(
              created.notification,
              tmPtr,
              weekFlag,
              alarmIdPtr,
            ),
            'alarm_schedule_noti_with_recurrence_week_flag',
          );
          return alarmIdPtr.value;
        } finally {
          calloc.free(tmPtr);
          calloc.free(alarmIdPtr);
        }
      },
    );
  }

  Future<int> _schedulePeriodic({
    required int id,
    String? title,
    String? body,
    String? payload,
    required Duration firstDelay,
    required Duration period,
  }) async {
    return _createNotificationHandle(
      id: id,
      title: title,
      body: body,
      payload: payload,
      notificationDetails: null,
      action: (_CreatedNotification created) {
        final Pointer<Int> alarmIdPtr = calloc<Int>();
        try {
          _checkResult(
            tizen.alarm_schedule_noti_after_delay(
              created.notification,
              firstDelay.inSeconds,
              period.inSeconds,
              alarmIdPtr,
            ),
            'alarm_schedule_noti_after_delay',
          );
          return alarmIdPtr.value;
        } finally {
          calloc.free(alarmIdPtr);
        }
      },
    );
  }

  Future<T> _createNotificationHandle<T>({
    required int id,
    String? title,
    String? body,
    String? payload,
    LinuxNotificationDetails? notificationDetails,
    required T Function(_CreatedNotification created) action,
  }) async {
    final String currentAppId = await _currentAppId();
    final AppInfo appInfo = await _currentAppInfo();
    final LinuxInitializationSettings? settings = _initializationSettings;

    final notification_h notification = tizen.notification_create(
      notification_type.NOTIFICATION_TYPE_NOTI,
    );
    if (notification == nullptr) {
      throw Exception('notification_create failed');
    }

    app_control_h? appControl;
    try {
      using((Arena arena) {
        _checkResult(
          tizen.notification_set_tag(
            notification,
            _tagForId(id).toNativeChar(allocator: arena),
          ),
          'notification_set_tag',
        );

        if (title != null) {
          _checkResult(
            tizen.notification_set_text(
              notification,
              notification_text_type.NOTIFICATION_TEXT_TYPE_TITLE,
              title.toNativeChar(allocator: arena),
              nullptr,
              notification_variable_type.NOTIFICATION_VARIABLE_TYPE_NONE,
            ),
            'notification_set_text(title)',
          );
        }

        if (body != null) {
          _checkResult(
            tizen.notification_set_text(
              notification,
              notification_text_type.NOTIFICATION_TEXT_TYPE_CONTENT,
              body.toNativeChar(allocator: arena),
              nullptr,
              notification_variable_type.NOTIFICATION_VARIABLE_TYPE_NONE,
            ),
            'notification_set_text(body)',
          );
        }

        final String? iconPath = _resolveIconPath(
          notificationDetails?.icon,
          settings?.defaultIcon,
          appInfo,
        );
        if (iconPath != null && File(iconPath).existsSync()) {
          _checkResult(
            tizen.notification_set_image(
              notification,
              notification_image_type.NOTIFICATION_IMAGE_TYPE_ICON,
              iconPath.toNativeChar(allocator: arena),
            ),
            'notification_set_image',
          );
        }

        final _SoundSpec soundSpec = _resolveSoundSpec(
          notificationDetails,
          settings,
          appInfo,
        );
        if (soundSpec.shouldConfigure) {
          _checkResult(
            tizen.notification_set_sound(
              notification,
              soundSpec.type,
              soundSpec.path?.toNativeChar(allocator: arena) ?? nullptr,
            ),
            'notification_set_sound',
          );
        }

        _checkResult(
          tizen.notification_set_display_applist(
            notification,
            notification_display_applist.NOTIFICATION_DISPLAY_APP_ALL,
          ),
          'notification_set_display_applist',
        );

        final Pointer<app_control_h> appControlPtr = arena<app_control_h>();
        _checkResult(
          tizen.app_control_create(appControlPtr),
          'app_control_create',
        );
        appControl = appControlPtr.value;

        _checkResult(
          tizen.app_control_set_app_id(
            appControl!,
            currentAppId.toNativeChar(allocator: arena),
          ),
          'app_control_set_app_id',
        );
        _checkResult(
          tizen.app_control_set_operation(
            appControl!,
            'http://tizen.org/appcontrol/operation/default'.toNativeChar(
              allocator: arena,
            ),
          ),
          'app_control_set_operation',
        );
        _checkResult(
          tizen.app_control_add_extra_data(
            appControl!,
            _extraIdKey.toNativeChar(allocator: arena),
            id.toString().toNativeChar(allocator: arena),
          ),
          'app_control_add_extra_data(id)',
        );
        if (payload != null) {
          _checkResult(
            tizen.app_control_add_extra_data(
              appControl!,
              _extraPayloadKey.toNativeChar(allocator: arena),
              payload.toNativeChar(allocator: arena),
            ),
            'app_control_add_extra_data(payload)',
          );
        }

        _checkResult(
          tizen.notification_set_launch_option(
            notification,
            notification_launch_option_type
                .NOTIFICATION_LAUNCH_OPTION_APP_CONTROL,
            appControl!.cast<Void>(),
          ),
          'notification_set_launch_option',
        );
      });

      return action(_CreatedNotification(notification: notification));
    } finally {
      if (appControl != null) {
        tizen.app_control_destroy(appControl!);
      }
      tizen.notification_free(notification);
    }
  }

  String? _resolveIconPath(
    LinuxNotificationIcon? detailsIcon,
    LinuxNotificationIcon? defaultIcon,
    AppInfo appInfo,
  ) {
    final LinuxNotificationIcon? icon = detailsIcon ?? defaultIcon;
    if (icon == null) {
      return appInfo.iconPath;
    }

    if (icon is FilePathLinuxIcon) {
      return _normalizeFilePath(icon.path);
    }

    if (icon is AssetsLinuxIcon) {
      return '${appInfo.sharedResourcePath}flutter_assets/${icon.relativePath}';
    }

    if (icon is ThemeLinuxIcon) {
      return appInfo.iconPath;
    }

    return appInfo.iconPath;
  }

  _SoundSpec _resolveSoundSpec(
    LinuxNotificationDetails? notificationDetails,
    LinuxInitializationSettings? settings,
    AppInfo appInfo,
  ) {
    if ((notificationDetails?.suppressSound ?? false) ||
        (settings?.defaultSuppressSound ?? false)) {
      return const _SoundSpec(
        shouldConfigure: true,
        type: notification_sound_type.NOTIFICATION_SOUND_TYPE_NONE,
      );
    }

    final LinuxNotificationSound? sound =
        notificationDetails?.sound ?? settings?.defaultSound;
    if (sound == null) {
      return const _SoundSpec(shouldConfigure: false, type: 0);
    }

    if (sound is AssetsLinuxSound) {
      return _SoundSpec(
        shouldConfigure: true,
        type: notification_sound_type.NOTIFICATION_SOUND_TYPE_USER_DATA,
        path:
            '${appInfo.sharedResourcePath}flutter_assets/${sound.relativePath}',
      );
    }

    if (sound is ThemeLinuxSound) {
      return const _SoundSpec(
        shouldConfigure: true,
        type: notification_sound_type.NOTIFICATION_SOUND_TYPE_DEFAULT,
      );
    }

    return const _SoundSpec(
      shouldConfigure: true,
      type: notification_sound_type.NOTIFICATION_SOUND_TYPE_DEFAULT,
    );
  }

  tz.TZDateTime _resolveNextScheduledDate(
    tz.TZDateTime scheduledDate,
    DateTimeComponents? matchDateTimeComponents,
  ) {
    final tz.TZDateTime now = tz.TZDateTime.now(scheduledDate.location);
    if (matchDateTimeComponents == null) {
      if (!scheduledDate.isAfter(now)) {
        throw ArgumentError.value(
          scheduledDate,
          'scheduledDate',
          'Must be in the future for non-repeating notifications.',
        );
      }
      return scheduledDate;
    }

    tz.TZDateTime nextFireDate = tz.TZDateTime(
      scheduledDate.location,
      now.year,
      now.month,
      now.day,
      scheduledDate.hour,
      scheduledDate.minute,
      scheduledDate.second,
      scheduledDate.millisecond,
      scheduledDate.microsecond,
    );

    while (!nextFireDate.isAfter(now)) {
      nextFireDate = nextFireDate.add(const Duration(days: 1));
    }

    switch (matchDateTimeComponents) {
      case DateTimeComponents.time:
        return nextFireDate;
      case DateTimeComponents.dayOfWeekAndTime:
        while (nextFireDate.weekday != scheduledDate.weekday) {
          nextFireDate = nextFireDate.add(const Duration(days: 1));
        }
        return nextFireDate;
      case DateTimeComponents.dayOfMonthAndTime:
      case DateTimeComponents.dateAndTime:
        return nextFireDate;
    }
  }

  int _weekFlagForDateTimeComponents(
    DateTimeComponents matchDateTimeComponents,
    tz.TZDateTime scheduledDate,
  ) {
    if (matchDateTimeComponents == DateTimeComponents.time) {
      return alarm_week_flag_e.ALARM_WEEK_FLAG_SUNDAY |
          alarm_week_flag_e.ALARM_WEEK_FLAG_MONDAY |
          alarm_week_flag_e.ALARM_WEEK_FLAG_TUESDAY |
          alarm_week_flag_e.ALARM_WEEK_FLAG_WEDNESDAY |
          alarm_week_flag_e.ALARM_WEEK_FLAG_THURSDAY |
          alarm_week_flag_e.ALARM_WEEK_FLAG_FRIDAY |
          alarm_week_flag_e.ALARM_WEEK_FLAG_SATURDAY;
    }

    return switch (scheduledDate.weekday) {
      DateTime.monday => alarm_week_flag_e.ALARM_WEEK_FLAG_MONDAY,
      DateTime.tuesday => alarm_week_flag_e.ALARM_WEEK_FLAG_TUESDAY,
      DateTime.wednesday => alarm_week_flag_e.ALARM_WEEK_FLAG_WEDNESDAY,
      DateTime.thursday => alarm_week_flag_e.ALARM_WEEK_FLAG_THURSDAY,
      DateTime.friday => alarm_week_flag_e.ALARM_WEEK_FLAG_FRIDAY,
      DateTime.saturday => alarm_week_flag_e.ALARM_WEEK_FLAG_SATURDAY,
      _ => alarm_week_flag_e.ALARM_WEEK_FLAG_SUNDAY,
    };
  }

  void _writeTm(tm tmValue, tz.TZDateTime scheduledDate) {
    tmValue.tm_sec = scheduledDate.second;
    tmValue.tm_min = scheduledDate.minute;
    tmValue.tm_hour = scheduledDate.hour;
    tmValue.tm_mday = scheduledDate.day;
    tmValue.tm_mon = scheduledDate.month - 1;
    tmValue.tm_year = scheduledDate.year - 1900;
    tmValue.tm_wday = scheduledDate.weekday % 7;
    tmValue.tm_yday = 0;
    tmValue.tm_isdst = -1;
    tmValue.tm_gmtoff = scheduledDate.timeZoneOffset.inSeconds;
    tmValue.tm_zone = nullptr;
  }

  Future<bool> _isAlarmScheduled(int alarmId) async {
    final Pointer<notification_h> notificationPtr = calloc<notification_h>();
    try {
      final int result = tizen.alarm_get_notification(alarmId, notificationPtr);
      if (result != 0 || notificationPtr.value == nullptr) {
        return false;
      }
      tizen.notification_free(notificationPtr.value);
      return true;
    } finally {
      calloc.free(notificationPtr);
    }
  }

  bool _isActiveNotification(int id) {
    final Pointer<Char> tag = _tagForId(id).toNativeChar();
    try {
      final notification_h handle = tizen.notification_load_by_tag(tag);
      if (handle == nullptr) {
        return false;
      }
      tizen.notification_free(handle);
      return true;
    } finally {
      calloc.free(tag);
    }
  }

  Future<void> _deleteActiveNotification(int id) async {
    final Pointer<Char> tag = _tagForId(id).toNativeChar();
    try {
      final notification_h handle = tizen.notification_load_by_tag(tag);
      if (handle != nullptr) {
        try {
          _checkResult(
            tizen.notification_delete(handle),
            'notification_delete',
          );
        } finally {
          tizen.notification_free(handle);
        }
      }
    } finally {
      calloc.free(tag);
    }
  }

  void _cancelAlarm(int alarmId) {
    final int result = tizen.alarm_cancel(alarmId);
    if (result != 0) {
      final String message = tizen.get_error_message(result).toDartString();
      throw Exception('alarm_cancel failed ($result): $message');
    }
  }

  bool _shouldKeepRecord(_NotificationRecord record) {
    return record.alarmId != null || _isActiveNotification(record.id);
  }

  String _tagForId(int id) => '$_tagPrefix$id';

  void _checkResult(int result, String operation) {
    if (result == 0) {
      return;
    }
    final String message = tizen.get_error_message(result).toDartString();
    throw Exception('$operation failed ($result): $message');
  }

  String? _normalizeFilePath(String path) {
    if (path.startsWith('file://')) {
      return Uri.parse(path).toFilePath();
    }
    return path;
  }

  int? _intExtra(Object? value) {
    final String? stringValue = _stringExtra(value);
    if (stringValue == null) {
      return null;
    }
    return int.tryParse(stringValue);
  }

  String? _stringExtra(Object? value) {
    if (value == null) {
      return null;
    }
    if (value is String) {
      return value;
    }
    if (value is List<dynamic> && value.isNotEmpty && value.first is String) {
      return value.first as String;
    }
    return null;
  }
}

class _CreatedNotification {
  const _CreatedNotification({required this.notification});

  final notification_h notification;
}

class _SoundSpec {
  const _SoundSpec({
    required this.shouldConfigure,
    required this.type,
    this.path,
  });

  final bool shouldConfigure;
  final int type;
  final String? path;
}

class _NotificationRecord {
  _NotificationRecord({
    required this.id,
    this.title,
    this.body,
    this.payload,
    this.alarmId,
  });

  factory _NotificationRecord.fromJson(Map<String, Object?> json) {
    return _NotificationRecord(
      id: json['id']! as int,
      title: json['title'] as String?,
      body: json['body'] as String?,
      payload: json['payload'] as String?,
      alarmId: json['alarmId'] as int?,
    );
  }

  final int id;
  String? title;
  String? body;
  String? payload;
  int? alarmId;

  Map<String, Object?> toJson() {
    return <String, Object?>{
      'id': id,
      'title': title,
      'body': body,
      'payload': payload,
      'alarmId': alarmId,
    };
  }
}

class _PluginState {
  _PluginState({required this.records});

  factory _PluginState.empty() {
    return _PluginState(records: <int, _NotificationRecord>{});
  }

  factory _PluginState.fromJson(Map<String, Object?> json) {
    final Map<String, Object?> encodedRecords =
        (json['records'] as Map<String, Object?>?) ?? <String, Object?>{};
    final Map<int, _NotificationRecord> records = <int, _NotificationRecord>{};
    encodedRecords.forEach((String key, Object? value) {
      if (value is Map<String, Object?>) {
        records[int.parse(key)] = _NotificationRecord.fromJson(value);
      } else if (value is Map) {
        records[int.parse(key)] = _NotificationRecord.fromJson(
          value.cast<String, Object?>(),
        );
      }
    });
    return _PluginState(records: records);
  }

  final Map<int, _NotificationRecord> records;

  Map<String, Object?> toJson() {
    return <String, Object?>{
      'records': <String, Object?>{
        for (final MapEntry<int, _NotificationRecord> entry in records.entries)
          entry.key.toString(): entry.value.toJson(),
      },
    };
  }
}

class _PluginStateStore {
  _PluginState? _cachedState;

  Future<_PluginState> read() async {
    if (_cachedState != null) {
      return _cachedState!;
    }

    final File file = await _file;
    if (!await file.exists()) {
      return _cachedState = _PluginState.empty();
    }

    final String encoded = await file.readAsString();
    if (encoded.trim().isEmpty) {
      return _cachedState = _PluginState.empty();
    }

    final Object? decoded = jsonDecode(encoded);
    if (decoded is Map<String, Object?>) {
      return _cachedState = _PluginState.fromJson(decoded);
    }
    if (decoded is Map) {
      return _cachedState = _PluginState.fromJson(
        decoded.cast<String, Object?>(),
      );
    }
    return _cachedState = _PluginState.empty();
  }

  Future<void> write(_PluginState state) async {
    final File file = await _file;
    await file.parent.create(recursive: true);
    await file.writeAsString(jsonEncode(state.toJson()));
    _cachedState = state;
  }

  Future<File> get _file async {
    final String dataPath = using((Arena arena) {
      final Pointer<Char> path = tizen.app_get_data_path();
      try {
        return path.toDartString();
      } finally {
        arena.using(path, calloc.free);
      }
    });
    return File('$dataPath/flutter_local_notifications_tizen_state.json');
  }
}
