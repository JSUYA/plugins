import 'dart:async';
import 'dart:collection';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_inappwebview/flutter_inappwebview.dart';

import 'example_shared.dart';
import 'main.dart';

class InAppWebViewExampleScreen extends StatefulWidget {
  const InAppWebViewExampleScreen({
    required this.packageLabel,
    required this.initialUrl,
    this.autoSmoke = false,
    super.key,
  });

  final String packageLabel;
  final String initialUrl;
  final bool autoSmoke;

  @override
  State<InAppWebViewExampleScreen> createState() =>
      _InAppWebViewExampleScreenState();
}

class _InAppWebViewExampleScreenState extends State<InAppWebViewExampleScreen>
    implements ExampleDriverDelegate {
  final GlobalKey _webViewKey = GlobalKey();
  final TextEditingController _urlController = TextEditingController();
  final Completer<InAppWebViewController> _controllerReadyCompleter =
      Completer<InAppWebViewController>();
  final InAppWebViewSettings _settings = InAppWebViewSettings(
    isInspectable: kDebugMode,
    javaScriptEnabled: true,
    mediaPlaybackRequiresUserGesture: false,
    allowsInlineMediaPlayback: true,
    useShouldOverrideUrlLoading: true,
    transparentBackground: false,
  );

  InAppWebViewController? _controller;
  Completer<void>? _pendingLoadCompleter;
  String _url = '';
  double _progress = 0;
  String _status = 'booting';

  @override
  void initState() {
    super.initState();
    exampleAppDriver.attach(this);
  }

  @override
  void dispose() {
    _urlController.dispose();
    super.dispose();
  }

  @override
  Future<void> waitUntilControllerReady() => _controllerReadyCompleter.future;

  @override
  Future<String> runSmokeScenarioForTest() async {
    await waitUntilControllerReady();
    return _runSmokeScenario();
  }

  void _updateUrl(WebUri? url) {
    _url = url?.toString() ?? '';
    _urlController.text = _url;
  }

  WebUri _normalizeUrl(String value) {
    var url = WebUri(value);
    if (url.scheme.isEmpty) {
      url = WebUri('https://www.google.com/search?q=$value');
    }
    return url;
  }

  void _completePendingLoad() {
    final pendingLoadCompleter = _pendingLoadCompleter;
    if (pendingLoadCompleter != null && !pendingLoadCompleter.isCompleted) {
      pendingLoadCompleter.complete();
    }
    _pendingLoadCompleter = null;
  }

  void _failPendingLoad(Object error) {
    final pendingLoadCompleter = _pendingLoadCompleter;
    if (pendingLoadCompleter != null && !pendingLoadCompleter.isCompleted) {
      pendingLoadCompleter.completeError(error);
    }
    _pendingLoadCompleter = null;
  }

  void _setStateIfMounted(VoidCallback fn) {
    if (!mounted) {
      return;
    }
    setState(fn);
  }

  Future<void> _loadFixtureIntoWebView() async {
    final controller = _controller;
    if (controller == null) {
      throw StateError('Controller is not ready.');
    }

    final pendingLoadCompleter = Completer<void>();
    _pendingLoadCompleter = pendingLoadCompleter;

    _setStateIfMounted(() {
      _progress = 0;
      _status = 'fixture loading';
    });

    await controller.loadData(
      data: fixtureHtml,
      mimeType: 'text/html',
      encoding: 'utf8',
    );
    await pendingLoadCompleter.future.timeout(const Duration(seconds: 10));
  }

  Future<void> _loadFixturePage() async {
    try {
      await _loadFixtureIntoWebView();
    } catch (error) {
      _setStateIfMounted(() {
        _status = 'fixture failed: $error';
      });
    }
  }

  Future<String> _runSmokeScenario() async {
    final controller = _controller;
    if (controller == null) {
      return _status;
    }

    _setStateIfMounted(() {
      _status = 'smoke starting';
    });

    try {
      await _loadFixtureIntoWebView();
      await controller.injectCSSCode(
        source: '#css-target { color: rgb(255, 0, 0); }',
      );
      final cssColor = await controller.evaluateJavascript(
        source:
            "getComputedStyle(document.getElementById('css-target')).color;",
      );
      final asyncResult = await controller.callAsyncJavaScript(
        functionBody: 'return args.value + 7;',
        arguments: <String, dynamic>{'value': 35},
      );
      await controller.evaluateJavascript(
        source: '''
(function() {
  var target = document.getElementById('select-target');
  var selection = window.getSelection();
  if (!target || !selection) {
    return false;
  }
  var range = document.createRange();
  range.selectNodeContents(target);
  selection.removeAllRanges();
  selection.addRange(range);
  return true;
})();
        ''',
      );
      final selectedText = await controller.getSelectedText();
      final canScrollVertically = await controller.canScrollVertically();
      final canScrollHorizontally = await controller.canScrollHorizontally();
      await controller.pauseAllMediaPlayback();
      await controller.closeAllMediaPresentations();
      final mediaPlaybackState = await controller.requestMediaPlaybackState();
      final isSecureContext = await controller.isSecureContext();
      final title = await controller.getTitle();
      final defaultUserAgent =
          await InAppWebViewController.getDefaultUserAgent();
      final handlesHttps =
          await InAppWebViewController.handlesURLScheme('https');
      final handlesCustom =
          await InAppWebViewController.handlesURLScheme('custom-scheme');

      final hasExpectedCssColor =
          cssColor?.toString().contains('255, 0, 0') ?? false;
      final hasExpectedAsyncValue =
          asyncResult?.error == null && asyncResult?.value?.toString() == '42';
      final hasExpectedSelection =
          selectedText?.trim() == 'Selected text fixture';
      final passed = title == 'fixture-title' &&
          defaultUserAgent.isNotEmpty &&
          handlesHttps &&
          !handlesCustom &&
          hasExpectedCssColor &&
          hasExpectedAsyncValue &&
          hasExpectedSelection &&
          canScrollVertically &&
          canScrollHorizontally &&
          mediaPlaybackState == MediaPlaybackState.NONE;

      _setStateIfMounted(() {
        _status = passed
            ? 'smoke passed secure=$isSecureContext'
            : 'smoke failed title=$title css=$cssColor async=${asyncResult?.value}/${asyncResult?.error} selected=$selectedText scroll=$canScrollVertically/$canScrollHorizontally ua=${defaultUserAgent.isNotEmpty} schemes=$handlesHttps/$handlesCustom media=$mediaPlaybackState secure=$isSecureContext';
      });
    } catch (error) {
      _setStateIfMounted(() {
        _status = 'smoke failed: $error';
      });
    }
    return _status;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('InAppWebView')),
      drawer: myDrawer(context: context),
      body: SafeArea(
        child: Column(
          children: <Widget>[
            Padding(
              padding: const EdgeInsets.fromLTRB(12, 12, 12, 4),
              child: Text(widget.packageLabel),
            ),
            TextField(
              decoration: const InputDecoration(
                prefixIcon: Icon(Icons.search),
                hintText: 'Enter a URL or search query',
              ),
              controller: _urlController,
              keyboardType: TextInputType.url,
              onSubmitted: (value) {
                _controller?.loadUrl(
                  urlRequest: URLRequest(url: _normalizeUrl(value)),
                );
              },
            ),
            Padding(
              padding: const EdgeInsets.all(12),
              child: Text(_status, key: statusTextKey),
            ),
            Expanded(
              child: Stack(
                children: <Widget>[
                  InAppWebView(
                    key: _webViewKey,
                    webViewEnvironment: webViewEnvironment,
                    initialUrlRequest: URLRequest(
                      url: WebUri(widget.initialUrl),
                    ),
                    initialUserScripts: UnmodifiableListView<UserScript>([]),
                    initialSettings: _settings,
                    onWebViewCreated: (controller) {
                      _controller = controller;
                      if (!_controllerReadyCompleter.isCompleted) {
                        _controllerReadyCompleter.complete(controller);
                      }
                      _setStateIfMounted(() {
                        _status = 'controller ready';
                      });
                      if (widget.autoSmoke) {
                        Future<void>.delayed(
                          const Duration(milliseconds: 100),
                          () {
                            if (mounted) {
                              _runSmokeScenario();
                            }
                          },
                        );
                      }
                    },
                    onLoadStart: (controller, url) {
                      _setStateIfMounted(() {
                        _updateUrl(url);
                        _status = 'load start: $url';
                      });
                    },
                    onLoadStop: (controller, url) async {
                      _completePendingLoad();
                      final title = await controller.getTitle();
                      _setStateIfMounted(() {
                        _updateUrl(url);
                        _progress = 1;
                        _status = 'load stop: ${title ?? url}';
                      });
                    },
                    onReceivedError: (controller, request, error) {
                      _failPendingLoad(error);
                      _setStateIfMounted(() {
                        _status = 'load error: ${error.description}';
                      });
                    },
                    onProgressChanged: (controller, progress) {
                      _setStateIfMounted(() {
                        _progress = progress / 100.0;
                        _status = 'progress: $progress%';
                      });
                    },
                    onUpdateVisitedHistory: (controller, url, isReload) {
                      _setStateIfMounted(() {
                        _updateUrl(url);
                      });
                    },
                    onConsoleMessage: (controller, consoleMessage) {
                      _setStateIfMounted(() {
                        _status =
                            'console: ${consoleMessage.messageLevel} ${consoleMessage.message}';
                      });
                    },
                    shouldOverrideUrlLoading: (controller, navigationAction) {
                      return Future<NavigationActionPolicy>.value(
                        NavigationActionPolicy.ALLOW,
                      );
                    },
                  ),
                  if (_progress < 1.0)
                    LinearProgressIndicator(value: _progress),
                ],
              ),
            ),
            ButtonBar(
              alignment: MainAxisAlignment.center,
              children: <Widget>[
                ElevatedButton(
                  onPressed: () => _controller?.goBack(),
                  child: const Icon(Icons.arrow_back),
                ),
                ElevatedButton(
                  onPressed: () => _controller?.goForward(),
                  child: const Icon(Icons.arrow_forward),
                ),
                ElevatedButton(
                  onPressed: () => _controller?.reload(),
                  child: const Icon(Icons.refresh),
                ),
              ],
            ),
          ],
        ),
      ),
      floatingActionButton: Column(
        mainAxisAlignment: MainAxisAlignment.end,
        children: <Widget>[
          FloatingActionButton.small(
            key: loadFixtureButtonKey,
            heroTag: 'load_fixture',
            onPressed: _loadFixturePage,
            child: const Icon(Icons.description),
          ),
          const SizedBox(height: 12),
          FloatingActionButton.small(
            key: runSmokeButtonKey,
            heroTag: 'run_smoke',
            onPressed: _runSmokeScenario,
            child: const Icon(Icons.science),
          ),
          const SizedBox(height: 12),
          FloatingActionButton.small(
            heroTag: 'eval',
            onPressed: () async {
              final result = await _controller?.evaluateJavascript(
                source: 'document.title',
              );
              if (!mounted) {
                return;
              }
              ScaffoldMessenger.of(
                context,
              ).showSnackBar(SnackBar(content: Text('JS result: $result')));
            },
            child: const Icon(Icons.code),
          ),
          const SizedBox(height: 12),
          FloatingActionButton.small(
            heroTag: 'cookies',
            onPressed: () async {
              await CookieManager.instance().deleteAllCookies();
              if (!mounted) {
                return;
              }
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('Cookies cleared')),
              );
            },
            child: const Icon(Icons.cookie),
          ),
        ],
      ),
    );
  }
}
