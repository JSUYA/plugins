import 'package:flutter/material.dart';
import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_tizen/widgets.dart';
import 'package:tizen_surface_view/tizen_surface_view.dart';
import 'package:tizen_log/tizen_log.dart';

import 'package:flutter/gestures.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp();

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  static const logTag = 'ASDF';

  @override
  void initState() {
    super.initState();
    Log.error(logTag, 'CJS Init State');
  }

  int _counter = 0;

  void _incrementCounter() {
    setState(() {
      // This call to setState tells the Flutter framework that something has
      // changed in this State, which causes it to rerun the build method below
      // so that the display can reflect the updated values. If we changed
      // _counter without calling setState(), then the build method would not be
      // called again, and so nothing would appear to happen.
      _counter++;
    });
  }

  @override
  Widget build(BuildContext context) {
    final Map<String, dynamic> creationParams = <String, dynamic>{};
    // Set<Factory<OneSequenceGestureRecognizer>>? gestureRecognizers;
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Plugin example app'),
        ),
        body: Center(
          child: TizenSurfaceView(
            viewType: 'plugins.flutter.io/tizen_surface_view',
            // onPlatformViewCreated: (int id) {
            // if (onWebViewPlatformCreated == null) {
            //   return;
            // }
            // onWebViewPlatformCreated(MethodChannelWebViewPlatform(
            //   id,
            //   webViewPlatformCallbacksHandler,
            //   javascriptChannelRegistry,
            // ));
            // },
            // gestureRecognizers: gestureRecognizers,
            // creationParams: creationParams,
            // creationParamsCodec: const StandardMessageCodec(),
          ),
        ),
        floatingActionButton: FloatingActionButton(
          onPressed: _incrementCounter,
          tooltip: 'Increment',
          child: const Icon(Icons.add),
        ),
      ),
    );
  }
}

/// A factory interface that also reports the type of the created objects.
class Factory<T> {
  /// Creates a new factory.
  ///
  /// The `constructor` parameter must not be null.
  const Factory(this.constructor) : assert(constructor != null);

  /// Creates a new object of type T.
  final ValueGetter<T> constructor;

  /// The type of the objects created by this factory.
  Type get type => T;

  @override
  String toString() {
    return 'Factory(type: $type)';
  }
}
