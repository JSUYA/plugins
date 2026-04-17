import 'dart:async';

import 'package:flutter/material.dart';

const Key statusTextKey = Key('status_text');
const Key loadFixtureButtonKey = Key('load_fixture_button');
const Key runSmokeButtonKey = Key('run_smoke_button');

const String fixtureHtml = '''
<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>fixture-title</title>
    <style>
      html, body {
        margin: 0;
        padding: 0;
      }
      body {
        font-family: sans-serif;
      }
      #page {
        min-width: 2200px;
        min-height: 2200px;
        padding: 16px;
        box-sizing: border-box;
      }
      #css-target {
        color: rgb(0, 0, 0);
        margin-top: 16px;
      }
      #wide {
        width: 2200px;
        height: 24px;
        background: #d0d7de;
        margin-top: 16px;
      }
      #tall {
        height: 2200px;
        width: 1px;
      }
    </style>
  </head>
  <body>
    <div id="page">
      <h1>Fixture Page</h1>
      <p id="select-target">Selected text fixture</p>
      <div id="css-target">Color target</div>
      <div id="wide"></div>
      <div id="tall"></div>
    </div>
  </body>
</html>
''';

abstract class ExampleDriverDelegate {
  Future<void> waitUntilControllerReady();

  Future<String> runSmokeScenarioForTest();
}

class ExampleAppDriver {
  final Completer<ExampleDriverDelegate> _delegateCompleter =
      Completer<ExampleDriverDelegate>();
  ExampleDriverDelegate? _delegate;

  void attach(ExampleDriverDelegate delegate) {
    _delegate = delegate;
    if (!_delegateCompleter.isCompleted) {
      _delegateCompleter.complete(delegate);
    }
  }

  Future<void> waitUntilReady() async {
    final delegate = _delegate ?? await _delegateCompleter.future;
    await delegate.waitUntilControllerReady();
  }

  Future<String> runSmokeScenario() async {
    final delegate = _delegate ?? await _delegateCompleter.future;
    return delegate.runSmokeScenarioForTest();
  }
}

final ExampleAppDriver exampleAppDriver = ExampleAppDriver();
