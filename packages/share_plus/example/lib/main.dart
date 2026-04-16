// ignore_for_file: public_member_api_docs

import 'dart:io';

import 'package:flutter/material.dart';
import 'package:share_plus/share_plus.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  String _lastResult = 'Idle';

  Future<void> _shareText() async {
    final ShareResult result = await SharePlus.instance.share(
      ShareParams(text: 'Hello from Flutter on Tizen'),
    );
    setState(() {
      _lastResult = result.status.name;
    });
  }

  Future<void> _shareFile() async {
    final Directory tempDir = await Directory.systemTemp.createTemp(
      'share_plus_example_',
    );
    final File file = File('${tempDir.path}/hello.txt');
    await file.writeAsString('Hello from share_plus_tizen');
    final ShareResult result = await SharePlus.instance.share(
      ShareParams(files: <XFile>[XFile(file.path)], text: 'Shared file'),
    );
    setState(() {
      _lastResult = result.status.name;
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('Share Plus Example')),
        body: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: <Widget>[
              Text('Last result: $_lastResult'),
              const SizedBox(height: 24),
              FilledButton(
                onPressed: _shareText,
                child: const Text('Share text'),
              ),
              const SizedBox(height: 12),
              FilledButton(
                onPressed: _shareFile,
                child: const Text('Share file'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
