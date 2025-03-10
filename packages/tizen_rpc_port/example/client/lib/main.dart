// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ignore_for_file: public_member_api_docs

import 'dart:async';

import 'package:flutter/material.dart';

import 'message_client.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final Message _client = Message('org.tizen.tizen_rpc_port_server_example');
  String _message = 'Not connected';
  bool _isConnected = false;
  String _input = '';

  Future<void> _connect() async {
    setState(() {
      _message = 'Connecting...';
    });

    try {
      await _client.connect(
        onError: (Object error) {
          setState(() {
            _message = 'Error: $error';
          });
        },
        onDisconnected: () async {
          setState(() {
            _isConnected = false;
            _message = 'Disconnected';
          });
        },
      );
      _client.register('ClientApp', onMessage);

      setState(() {
        _isConnected = true;
        _message = 'Connected';
      });
    } catch (error) {
      setState(() {
        _message = 'Error: $error';
      });
    }
  }

  void onMessage(String sender, String message) {
    setState(() {
      _message = 'Received: $message';
    });
  }

  Future<void> _send() async {
    if (_isConnected) {
      _client.send(_input);
    }
  }

  @override
  void dispose() {
    if (_isConnected) {
      _client.unregister();
      _client.disconnect();
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('TizenRpcPort Client Demo')),
        body: Padding(
          padding: const EdgeInsets.all(10),
          child: Column(
            children: <Widget>[
              Padding(
                padding: const EdgeInsets.symmetric(vertical: 20),
                child: Text(_message),
              ),
              TextField(
                onChanged: (String text) {
                  setState(() => _input = text);
                },
                decoration: const InputDecoration(
                  border: OutlineInputBorder(),
                  labelText: 'Input',
                ),
              ),
            ],
          ),
        ),
        persistentFooterButtons: <Widget>[
          TextButton(
            onPressed: _isConnected ? null : _connect,
            child: const Text('Connect'),
          ),
          TextButton(
            onPressed: _isConnected ? _send : null,
            child: const Text('Send'),
          ),
        ],
      ),
    );
  }
}
