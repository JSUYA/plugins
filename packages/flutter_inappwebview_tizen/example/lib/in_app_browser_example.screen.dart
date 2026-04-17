import 'package:flutter/material.dart';

import 'main.dart';

class InAppBrowserExampleScreen extends StatelessWidget {
  const InAppBrowserExampleScreen({super.key});

  static const String _unsupportedMessage =
      'InAppBrowser is not implemented for flutter_inappwebview_tizen yet.';

  void _showUnsupported(BuildContext context) {
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text(_unsupportedMessage)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('InAppBrowser')),
      drawer: myDrawer(context: context),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            ElevatedButton(
              onPressed: () => _showUnsupported(context),
              child: const Text('Open In-App Browser'),
            ),
            const SizedBox(height: 40),
            ElevatedButton(
              onPressed: () => _showUnsupported(context),
              child: const Text('Open System Browser'),
            ),
            const SizedBox(height: 24),
            const Padding(
              padding: EdgeInsets.symmetric(horizontal: 24),
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
