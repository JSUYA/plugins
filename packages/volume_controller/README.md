# volume_controller_tizen

The Tizen implementation of [`volume_controller`](https://pub.dev/packages/volume_controller).

## Getting Started

This package is not an endorsed implementation of `volume_controller`.
Add both packages to your `pubspec.yaml`:

```yaml
dependencies:
  volume_controller: ^3.4.4
  volume_controller_tizen: ^0.1.1
```

Then import the original package:

```dart
import 'package:volume_controller/volume_controller.dart';
```

## Tizen behavior

- Tizen maps the plugin's system volume API to the `media` audio volume stream.
- `showSystemUI` is ignored.
- `isMuted` returns `true` when the media volume is `0`.
- `setMute(true)` stores the previous non-zero media volume and sets the media volume to `0`.
- `setMute(false)` restores the previous non-zero media volume when possible.

## Required privileges

To change the volume level, add the following privilege to `tizen-manifest.xml`:

```xml
<privileges>
  <privilege>http://tizen.org/privilege/volume.set</privilege>
</privileges>
```
