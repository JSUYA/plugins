// ignore_for_file: public_member_api_docs

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:volume_controller/volume_controller.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  late final VolumeController _volumeController;
  StreamSubscription<double>? _subscription;

  double _streamVolume = 0;
  double _queriedVolume = 0;
  bool _isMuted = false;

  @override
  void initState() {
    super.initState();
    _volumeController = VolumeController.instance;
    _subscription = _volumeController.addListener((double volume) {
      if (!mounted) {
        return;
      }
      setState(() {
        _streamVolume = volume;
      });
    });
    _refreshState();
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  Future<void> _refreshState() async {
    final double volume = await _volumeController.getVolume();
    final bool muted = await _volumeController.isMuted();
    if (!mounted) {
      return;
    }
    setState(() {
      _queriedVolume = volume;
      _isMuted = muted;
    });
  }

  Future<void> _setMuted(bool muted) async {
    await _volumeController.setMute(muted);
    await _refreshState();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('Volume Controller Example')),
        body: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: <Widget>[
              Text('Stream volume: ${_streamVolume.toStringAsFixed(2)}'),
              const SizedBox(height: 8),
              Text('Queried volume: ${_queriedVolume.toStringAsFixed(2)}'),
              const SizedBox(height: 8),
              Text('Muted: $_isMuted'),
              const SizedBox(height: 24),
              Slider(
                value: _streamVolume.clamp(0, 1),
                onChanged: (double value) async {
                  await _volumeController.setVolume(value);
                  await _refreshState();
                },
              ),
              const SizedBox(height: 16),
              Wrap(
                spacing: 12,
                runSpacing: 12,
                children: <Widget>[
                  FilledButton(
                    onPressed: _refreshState,
                    child: const Text('Refresh'),
                  ),
                  FilledButton(
                    onPressed: () => _setMuted(true),
                    child: const Text('Mute'),
                  ),
                  FilledButton(
                    onPressed: () => _setMuted(false),
                    child: const Text('Unmute'),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
