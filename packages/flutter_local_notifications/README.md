# flutter_local_notifications_tizen

The Tizen implementation of
[`flutter_local_notifications`](https://pub.dev/packages/flutter_local_notifications).

## Getting Started

This package is not an endorsed implementation of
`flutter_local_notifications`. Add both packages to your `pubspec.yaml`:

```yaml
dependencies:
  flutter_local_notifications: ^22.0.0-dev.2
  flutter_local_notifications_tizen: ^0.1.0
```

Then import the original package:

```dart
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
```

## Supported features

- `show`
- `cancel`
- `cancelAll`
- `cancelAllPendingNotifications`
- `pendingNotificationRequests`
- `getActiveNotifications`
- `getNotificationAppLaunchDetails`
- `zonedSchedule`
  - one-shot notifications
  - recurring daily notifications via `DateTimeComponents.time`
  - recurring weekly notifications via `DateTimeComponents.dayOfWeekAndTime`
- `periodicallyShow`
  - `hourly`
  - `daily`
  - `weekly`
- `periodicallyShowWithDuration`
  - durations of 15 minutes or longer

## Tizen limitations

- Scheduled notification payloads are not available because the current
  `flutter_local_notifications_platform_interface` does not pass payload data
  to unsupported platforms for scheduling APIs.
- `DateTimeComponents.dayOfMonthAndTime` and
  `DateTimeComponents.dateAndTime` are not supported because Tizen public alarm
  APIs do not provide reliable monthly or yearly notification recurrence.
- `RepeatInterval.everyMinute` and custom periodic durations shorter than
  15 minutes are not supported because Tizen repeating alarms require a minimum
  interval of 15 minutes.
- Linux notification actions, custom hints, timeout semantics, and raw byte
  icons are ignored on Tizen.

## Required privileges

Add the following privileges to `tizen-manifest.xml`:

```xml
<privileges>
  <privilege>http://tizen.org/privilege/notification</privilege>
  <privilege>http://tizen.org/privilege/appmanager.launch</privilege>
  <privilege>http://tizen.org/privilege/alarm.set</privilege>
  <privilege>http://tizen.org/privilege/alarm.get</privilege>
</privileges>
```
