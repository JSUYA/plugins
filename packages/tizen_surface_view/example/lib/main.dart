/*import 'package:flutter/material.dart';
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
      Log.error(logTag, 'CJS click counter');
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
*/
/*
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
}*/



// Copyright 2019-present the Flutter authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:window_size/window_size.dart';
import 'package:tizen_surface_view/tizen_surface_view.dart';
import 'package:tizen_log/tizen_log.dart';

import 'data_transfer_page.dart';
// import 'infinite_process_page.dart';
import 'performance_page.dart';

void main() {
  setupWindow();
  runApp(
    const MaterialApp(
      home: HomePage(),
    ),
  );
}

const double windowWidth = 1024;
const double windowHeight = 800;

void setupWindow() {
  if (!kIsWeb && (Platform.isWindows || Platform.isLinux || Platform.isMacOS)) {
    WidgetsFlutterBinding.ensureInitialized();
    setWindowTitle('Isolate Example');
    setWindowMinSize(const Size(windowWidth, windowHeight));
  }
}

class HomePage extends StatelessWidget {
  const HomePage();

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: DefaultTabController(
        length: 3,
        child: Scaffold(
          appBar: AppBar(
            bottom: const TabBar(
              tabs: [
                Tab(
                  icon: Icon(Icons.flash_on),
                  text: 'Performance',
                ),
                Tab(
                  icon: Icon(Icons.favorite),
                  text: 'TizenView',
                ),
                Tab(
                  icon: Icon(Icons.storage),
                  text: 'Data Transfer',
                ),
              ],
            ),
            title: const Text('Isolate Example'),
          ),
          body: const TabBarView(
            children: [
              PerformancePage(),
              TizenSurfaceView(
                viewType: 'plugins.flutter.io/tizen_surface_view',
              ),
              DataTransferPageStarter(),
            ],
          ),
        ),
      ),
    );
  }
}
