# share_plus_tizen

The Tizen implementation of [`share_plus`](https://pub.dev/packages/share_plus).

## Getting Started

This package is not an endorsed implementation of `share_plus`.
Add both packages to your `pubspec.yaml`:

```yaml
dependencies:
  share_plus: ^13.0.0
  share_plus_tizen: ^0.1.0
```

Then import the original package:

```dart
import 'package:share_plus/share_plus.dart';
```

## Tizen behavior

- Text and URI sharing use Tizen App Control share operations.
- File sharing uses `http://tizen.org/appcontrol/data/path`.
- `XFile.fromData` is materialized to a temporary file before sharing.
- `ShareResult` is reported as `unavailable` because Tizen does not expose the
  final user selection through the public App Control API.
- `title` and `subject` are ignored by Tizen share targets.

## Required privileges

To launch the share target, add the following privilege to
`tizen-manifest.xml`:

```xml
<privileges>
  <privilege>http://tizen.org/privilege/appmanager.launch</privilege>
</privileges>
```
