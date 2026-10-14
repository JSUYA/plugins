# keyboard_detection_tizen

A Flutter plugin for keyboard visibility and geometry on Tizen, with an API matching [`keyboard_detection`](https://pub.dev/packages/keyboard_detection).

Unlike the original package, which uses `MediaQuery.viewInsetsOf(context).bottom`, this plugin reads state, height, width, and position from the embedder's `tizen/internal/inputpanel` channel. This also works on older embedders without `viewInsets` support.

> **Warning**
> Requires flutter-tizen `3.41.9-tizen.1.0.0` or later. Earlier versions of the embedder do not publish geometry on the `tizen/internal/inputpanel` channel, so the controller will stay in `KeyboardState.unknown`.

## Usage

```yaml
dependencies:
  keyboard_detection_tizen: ^0.1.1
```

```dart
import 'package:keyboard_detection_tizen/keyboard_detection_tizen.dart';

final controller = KeyboardDetectionController(
  onChanged: (state) => debugPrint('keyboard: $state'),
);

@override
Widget build(BuildContext context) {
  return Scaffold(
    body: Column(
      children: [
        const TextField(),
        StreamBuilder<KeyboardState>(
          stream: controller.stream,
          builder: (_, snapshot) => Text(
            'state: ${snapshot.data ?? KeyboardState.unknown} '
            'size: ${controller.size}',
          ),
        ),
      ],
    ),
  );
}
```

## Using Flutter viewInsets

[flutter-tizen/embedder#207](https://github.com/flutter-tizen/embedder/pull/207) enables keyboard `viewInsets` on Ecore and Tizen Core. Automatic resizing and inset-based padding require this embedder change; updating this plugin alone is not enough.

The controller continues to work for state and geometry. For layout, choose one of the following approaches to avoid applying keyboard spacing twice.

### Automatic resizing

Keep `Scaffold` resizing enabled and remove manual keyboard padding or height reductions.

```dart
@override
Widget build(BuildContext context) {
  return Scaffold(
    resizeToAvoidBottomInset: true, // Default.
    body: const Align(
      alignment: Alignment.bottomCenter,
      child: TextField(),
    ),
  );
}
```

### Manual padding

Disable `Scaffold` resizing and apply the bottom inset yourself. `viewInsetsOf` rebuilds the widget when the inset changes; its value is already in logical pixels.

```dart
@override
Widget build(BuildContext context) {
  final bottomInset = MediaQuery.viewInsetsOf(context).bottom;

  return Scaffold(
    resizeToAvoidBottomInset: false,
    body: Padding(
      padding: EdgeInsets.only(bottom: bottomInset),
      child: const Align(
        alignment: Alignment.bottomCenter,
        child: TextField(),
      ),
    ),
  );
}
```

If the OS already resized the window above the keyboard, the bottom inset can be zero. Do not substitute `controller.size`: keyboard height is not necessarily the obscured area.

### Existing layouts on older embedders

Without `viewInsets` support, retain your controller-based layout and set `resizeToAvoidBottomInset: false`. Rebuild from `controller.stream` and convert its physical height before using it in layout:

```dart
final keyboardHeight = controller.size / MediaQuery.of(context).devicePixelRatio;
```

Only apply this height as padding when the keyboard overlaps the view and the OS has not already resized the window.

## Limitations

- The Tizen input panel channel emits `show` / `hide` / `will_show` only. `KeyboardState.hiding` is therefore never reached on Tizen.
- Geometry values are physical pixels reported by the Tizen input method context. Convert with `MediaQueryData.devicePixelRatio` if you need logical pixels.
- Floating / split keyboards are not supported.
