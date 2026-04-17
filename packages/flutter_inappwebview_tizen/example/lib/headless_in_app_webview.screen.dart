import 'package:flutter/material.dart';

import 'main.dart';

class HeadlessInAppWebViewExampleScreen extends StatefulWidget {
  const HeadlessInAppWebViewExampleScreen({super.key});

  @override
  State<HeadlessInAppWebViewExampleScreen> createState() =>
      _HeadlessInAppWebViewExampleScreenState();
}

class _HeadlessInAppWebViewExampleScreenState
    extends State<HeadlessInAppWebViewExampleScreen> {
  static const String _unsupportedMessage =
      'HeadlessInAppWebView is not implemented for flutter_inappwebview_tizen yet.';

  String _status = '';

  void _showUnsupported(String action) {
    setState(() {
      _status = action;
    });
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(_unsupportedMessage)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('HeadlessInAppWebView')),
      drawer: myDrawer(context: context),
      body: SafeArea(
        child: Column(
          children: <Widget>[
            Padding(
              padding: const EdgeInsets.all(20),
              child: Text(
                'CURRENT URL\n${_status.isEmpty ? '' : _status}',
                textAlign: TextAlign.center,
              ),
            ),
            Center(
              child: ElevatedButton(
                onPressed: () => _showUnsupported('Run requested'),
                child: const Text('Run HeadlessInAppWebView'),
              ),
            ),
            const SizedBox(height: 10),
            Center(
              child: ElevatedButton(
                onPressed: () => _showUnsupported('Console log requested'),
                child: const Text('Send console.log message'),
              ),
            ),
            const SizedBox(height: 10),
            Center(
              child: ElevatedButton(
                onPressed: () => _showUnsupported('Dispose requested'),
                child: const Text('Dispose HeadlessInAppWebView'),
              ),
            ),
            const Padding(
              padding: EdgeInsets.all(24),
              child: Text(
                _unsupportedMessage,
                textAlign: TextAlign.center,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
