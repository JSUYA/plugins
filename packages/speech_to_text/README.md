# speech_to_text_tizen

The Tizen implementation of [`speech_to_text`](https://pub.dev/packages/speech_to_text).

## Getting Started

This package is not an endorsed implementation of `speech_to_text`.
Add both packages to your `pubspec.yaml`:

```yaml
dependencies:
  speech_to_text: ^7.4.0-beta.8
  speech_to_text_tizen: ^0.1.1
```

Then import the original package:

```dart
import 'package:speech_to_text/speech_to_text.dart';
```

## Tizen behavior

- Recognition is backed by the public Tizen STT API.
- `initialize()` requests the `recorder` privacy privilege when needed.
- `partialResults` uses `FREE.PARTIAL` recognition when the engine supports it,
  otherwise it falls back to `FREE`.
- `listenMode: ListenMode.search` maps to the Tizen `SEARCH` recognition type.
- `onDevice`, `sampleRate`, `autoPunctuation`, and haptics options are ignored.
- Locale names are reported as their locale identifiers because the native STT
  API does not expose localized display names.

## Required privileges

Add the following privilege to `tizen-manifest.xml`:

```xml
<privileges>
  <privilege>http://tizen.org/privilege/recorder</privilege>
</privileges>
```
