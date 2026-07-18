// ignore_for_file: public_member_api_docs

import 'package:flutter/material.dart';
import 'package:speech_to_text/speech_recognition_error.dart';
import 'package:speech_to_text/speech_recognition_result.dart';
import 'package:speech_to_text/speech_to_text.dart';

void main() {
  runApp(const SpeechToTextExampleApp());
}

class SpeechToTextExampleApp extends StatelessWidget {
  const SpeechToTextExampleApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(home: SpeechToTextExamplePage());
  }
}

class SpeechToTextExamplePage extends StatefulWidget {
  const SpeechToTextExamplePage({super.key});

  @override
  State<SpeechToTextExamplePage> createState() =>
      _SpeechToTextExamplePageState();
}

class _SpeechToTextExamplePageState extends State<SpeechToTextExamplePage> {
  final SpeechToText _speech = SpeechToText();

  bool _available = false;
  String _status = 'idle';
  String _error = '';
  String _words = '';
  double _soundLevel = 0.0;
  List<LocaleName> _locales = const <LocaleName>[];
  String? _selectedLocaleId;

  Future<void> _initialize() async {
    final bool available = await _speech.initialize(
      onError: _onError,
      onStatus: _onStatus,
    );
    final List<LocaleName> locales = available
        ? await _speech.locales()
        : const <LocaleName>[];
    final LocaleName? systemLocale = available
        ? await _speech.systemLocale()
        : null;

    setState(() {
      _available = available;
      _locales = locales;
      _selectedLocaleId = systemLocale?.localeId;
      _status = available ? 'ready' : 'unavailable';
    });
  }

  Future<void> _listen() async {
    await _speech.listen(
      onResult: _onResult,
      onSoundLevelChange: (double level) {
        setState(() {
          _soundLevel = level;
        });
      },
      listenOptions: SpeechListenOptions(
        localeId: _selectedLocaleId,
      ),
    );
  }

  Future<void> _stop() => _speech.stop();

  Future<void> _cancel() => _speech.cancel();

  void _onResult(SpeechRecognitionResult result) {
    setState(() {
      _words = result.recognizedWords;
    });
  }

  void _onError(SpeechRecognitionError error) {
    setState(() {
      _error = '${error.errorMsg} (permanent: ${error.permanent})';
    });
  }

  void _onStatus(String status) {
    setState(() {
      _status = status;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('speech_to_text_tizen Example')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: <Widget>[
            Wrap(
              spacing: 12,
              runSpacing: 12,
              children: <Widget>[
                ElevatedButton(
                  onPressed: _initialize,
                  child: const Text('Initialize'),
                ),
                ElevatedButton(
                  onPressed: _available && !_speech.isListening
                      ? _listen
                      : null,
                  child: const Text('Listen'),
                ),
                ElevatedButton(
                  onPressed: _speech.isListening ? _stop : null,
                  child: const Text('Stop'),
                ),
                ElevatedButton(
                  onPressed: _speech.isListening ? _cancel : null,
                  child: const Text('Cancel'),
                ),
              ],
            ),
            const SizedBox(height: 16),
            DropdownButtonFormField<String>(
              initialValue: _selectedLocaleId,
              items: _locales
                  .map(
                    (LocaleName locale) => DropdownMenuItem<String>(
                      value: locale.localeId,
                      child: Text(locale.name),
                    ),
                  )
                  .toList(),
              decoration: const InputDecoration(labelText: 'Locale'),
              onChanged: _locales.isEmpty
                  ? null
                  : (String? value) {
                      setState(() {
                        _selectedLocaleId = value;
                      });
                    },
            ),
            const SizedBox(height: 16),
            Text('Available: $_available'),
            Text('Status: $_status'),
            Text('Sound level: ${_soundLevel.toStringAsFixed(2)}'),
            const SizedBox(height: 16),
            const Text('Recognized words'),
            Expanded(
              child: Container(
                margin: const EdgeInsets.only(top: 8),
                padding: const EdgeInsets.all(12),
                color: Theme.of(context).colorScheme.surfaceContainerHighest,
                child: SingleChildScrollView(child: Text(_words)),
              ),
            ),
            const SizedBox(height: 16),
            const Text('Last error'),
            Container(
              margin: const EdgeInsets.only(top: 8),
              padding: const EdgeInsets.all(12),
              color: Theme.of(context).colorScheme.surfaceContainerHighest,
              child: Text(_error.isEmpty ? 'none' : _error),
            ),
          ],
        ),
      ),
    );
  }
}
