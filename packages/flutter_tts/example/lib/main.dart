import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_tts/flutter_tts.dart';

void main() => runApp(const MyApp());

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  MyAppState createState() => MyAppState();
}

enum TtsState { playing, stopped, paused, continued }

class MyAppState extends State<MyApp> {
  late FlutterTts flutterTts;
  List<Map<String, String>?> rawVoices = [];
  List<DropdownMenuItem<Map<String, String>?>> voiceItems = [];
  Map<String, String>? voice;
  List<String?> rawLanguages = [];
  List<DropdownMenuItem<String?>> languageItems = [];
  String? language;
  double volume = 0.8;
  double pitch = 1.0;
  double rate = 0.5;

  String? _newVoiceText;
  int? _inputLength;

  TtsState ttsState = TtsState.stopped;

  bool get isPlaying => ttsState == TtsState.playing;
  bool get isStopped => ttsState == TtsState.stopped;
  bool get isPaused => ttsState == TtsState.paused;
  bool get isContinued => ttsState == TtsState.continued;

  @override
  initState() {
    super.initState();
    initTts();
  }

  void initTts() {
    flutterTts = FlutterTts();

    _setAwaitOptions();

    _getDefaultVoice();

    flutterTts.setStartHandler(() {
      setState(() {
        if (kDebugMode) debugPrint("TtsState: Playing");
        ttsState = TtsState.playing;
      });
    });

    flutterTts.setCompletionHandler(() {
      setState(() {
        if (kDebugMode) debugPrint("TtsState: Complete");
        ttsState = TtsState.stopped;
      });
    });

    flutterTts.setCancelHandler(() {
      setState(() {
        if (kDebugMode) debugPrint("TtsState: Cancel");
        ttsState = TtsState.stopped;
      });
    });

    flutterTts.setPauseHandler(() {
      setState(() {
        if (kDebugMode) debugPrint("TtsState: Paused");
        ttsState = TtsState.paused;
      });
    });

    flutterTts.setContinueHandler(() {
      setState(() {
        if (kDebugMode) debugPrint("TtsState: Continued");
        ttsState = TtsState.continued;
      });
    });

    flutterTts.setErrorHandler((msg) {
      setState(() {
        if (kDebugMode) debugPrint("TtsState: Error: $msg");
        ttsState = TtsState.stopped;
      });
    });
  }

  Future<dynamic> _getVoices() async => await flutterTts.getVoices;

  Future<dynamic> _getLanguages() async => await flutterTts.getLanguages;

  Future<void> _getDefaultVoice() async {
    var defVoice = await flutterTts.getDefaultVoice;
    if (defVoice != null) {
      if (kDebugMode) debugPrint('Default Voice: $defVoice');
    }
  }

  Future<void> _speak() async {
    await flutterTts.setVolume(volume);
    await flutterTts.setSpeechRate(rate);

    if (_newVoiceText != null) {
      if (_newVoiceText!.isNotEmpty) {
        await flutterTts.speak(_newVoiceText!);
      }
    }
  }

  Future<void> _setAwaitOptions() async {
    await flutterTts.awaitSpeakCompletion(true);
  }

  Future<void> _stop() async {
    var result = await flutterTts.stop();
    if (result == 1) setState(() => ttsState = TtsState.stopped);
  }

  Future<void> _pause() async {
    var result = await flutterTts.pause();
    if (result == 1) setState(() => ttsState = TtsState.paused);
  }

  @override
  void dispose() {
    super.dispose();
    flutterTts.stop();
  }

  List<DropdownMenuItem<Map<String, String>?>> getVoicesDropDownMenuItems(
      List<dynamic> voices) {
    if (voiceItems.isEmpty) {
      rawVoices.clear();
      for (dynamic item in voices) {
        var v = Map<String, String>.from(item);
        rawVoices.add(v);
        var menuItem = DropdownMenuItem<Map<String, String>?>(
          value: v,
          child: Text("${v['name']} (${v['locale']})"),
        );
        if (!voiceItems
            .any((element) => mapEquals(element.value, menuItem.value))) {
          voiceItems.add(menuItem);
        }
      }
      voiceItems.sort((a, b) {
        return a.child
            .toString()
            .toLowerCase()
            .compareTo(b.child.toString().toLowerCase());
      });
    }
    return voiceItems;
  }

  Future<void> changedVoicesDropDownItem(
      Map<String, String>? selectedVoice) async {
    if (selectedVoice == null || selectedVoice.isEmpty) {
      return;
    }
    await flutterTts.setVoice(selectedVoice);
    voice = selectedVoice;
    language = selectedVoice['locale'];
    setState(() {});
  }

  List<DropdownMenuItem<String?>> getLanguagesDropDownMenuItems(
      List<dynamic> languages) {
    if (languageItems.isEmpty) {
      rawLanguages.clear();
      for (dynamic item in languages) {
        rawLanguages.add(item);
        var menuItem =
            DropdownMenuItem<String?>(value: item, child: Text(item));
        if (!languageItems.any((element) => element.value == menuItem.value)) {
          languageItems.add(menuItem);
        }
      }
      languageItems.sort((a, b) {
        return a.child
            .toString()
            .toLowerCase()
            .compareTo(b.child.toString().toLowerCase());
      });
    }
    return languageItems;
  }

  Future<void> changedLanguagesDropDownItem(String? selectedLanguage) async {
    if (selectedLanguage == null || selectedLanguage.trim().isEmpty) {
      return;
    }
    await flutterTts.setLanguage(selectedLanguage);
    language = selectedLanguage;

    if (voiceItems.isNotEmpty) {
      var voiceItem =
          voiceItems.firstWhere((v) => v.value?['locale'] == selectedLanguage);
      voice = voiceItem.value;
      if (voice != null) changedVoicesDropDownItem(voice);
    }
  }

  void _onChange(String text) {
    setState(() {
      _newVoiceText = text;
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Flutter TTS'),
        ),
        body: SingleChildScrollView(
          scrollDirection: Axis.vertical,
          child: Column(
            children: [
              _inputSection(),
              _btnSection(),
              _voiceSection(),
              _languageSection(),
              _buildSliders(),
              _getMaxSpeechInputLengthSection(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _voiceSection() {
    if (voiceItems.isNotEmpty) {
      return _voicesDropDownSection(<dynamic>[]);
    } else {
      return FutureBuilder<dynamic>(
          future: _getVoices(),
          builder: (BuildContext context, AsyncSnapshot<dynamic> snapshot) {
            if (snapshot.connectionState == ConnectionState.done) {
              if (snapshot.hasData) {
                return _voicesDropDownSection(snapshot.data as List<dynamic>);
              } else if (snapshot.hasError) {
                return Text('Error: ${snapshot.error}');
              } else {
                return const Text('No data to load voices');
              }
            } else if (snapshot.connectionState == ConnectionState.waiting) {
              return const Text('Loading voices...');
            } else {
              return const Text('Waiting to start loading voices...');
            }
          });
    }
  }

  Widget _languageSection() {
    if (languageItems.isNotEmpty) {
      return _languageDropDownSection(<dynamic>[]);
    } else {
      return FutureBuilder<dynamic>(
          future: _getLanguages(),
          builder: (BuildContext context, AsyncSnapshot<dynamic> snapshot) {
            if (snapshot.connectionState == ConnectionState.done) {
              if (snapshot.hasData) {
                return _languageDropDownSection(snapshot.data as List<dynamic>);
              } else if (snapshot.hasError) {
                return Text('Error: ${snapshot.error}');
              } else {
                return const Text('No data to load languages');
              }
            } else if (snapshot.connectionState == ConnectionState.waiting) {
              return const Text('Loading Languages...');
            } else {
              return const Text('Waiting to start loading languages...');
            }
          });
    }
  }

  Widget _inputSection() => Container(
      alignment: Alignment.topCenter,
      padding: const EdgeInsets.only(top: 25.0, left: 25.0, right: 25.0),
      child: TextField(
        maxLines: 11,
        minLines: 6,
        onChanged: (String value) {
          _onChange(value);
        },
      ));

  Widget _btnSection() {
    return Container(
      padding: const EdgeInsets.only(top: 50.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          _buildButtonColumn(Colors.green, Colors.greenAccent, Icons.play_arrow,
              'PLAY', _speak),
          _buildButtonColumn(
              Colors.red, Colors.redAccent, Icons.stop, 'STOP', _stop),
          _buildButtonColumn(
              Colors.blue, Colors.blueAccent, Icons.pause, 'PAUSE', _pause),
        ],
      ),
    );
  }

  Widget _voicesDropDownSection(List<dynamic> voices) {
    return Container(
      padding: const EdgeInsets.only(top: 10.0),
      child: DropdownButton<Map<String, String>?>(
        value: voice,
        hint: const Text('Choose a voice'),
        items: getVoicesDropDownMenuItems(voices),
        onChanged: changedVoicesDropDownItem,
      ),
    );
  }

  Widget _languageDropDownSection(List<dynamic> languages) {
    return Container(
        padding: const EdgeInsets.only(top: 10.0),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            DropdownButton<String?>(
              value: language,
              hint: const Text('Choose a language'),
              items: getLanguagesDropDownMenuItems(languages),
              onChanged: changedLanguagesDropDownItem,
            ),
          ],
        ));
  }

  Column _buildButtonColumn(Color color, Color splashColor, IconData icon,
      String label, Function func) {
    return Column(
        mainAxisSize: MainAxisSize.min,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          IconButton(
              icon: Icon(icon),
              color: color,
              splashColor: splashColor,
              onPressed: () => func()),
          Container(
              margin: const EdgeInsets.only(top: 8.0),
              child: Text(label,
                  style: TextStyle(
                      fontSize: 12.0,
                      fontWeight: FontWeight.w400,
                      color: color)))
        ]);
  }

  Widget _getMaxSpeechInputLengthSection() {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: [
        ElevatedButton(
          child: const Text('Get max speech input length'),
          onPressed: () async {
            _inputLength = await flutterTts.getMaxSpeechInputLength;
            setState(() {});
          },
        ),
        Text("$_inputLength characters"),
      ],
    );
  }

  Widget _buildSliders() {
    return Column(
      children: [_volume(), _rate()],
    );
  }

  Widget _volume() {
    return Slider(
        value: volume,
        onChanged: (newVolume) {
          setState(() => volume = newVolume);
        },
        min: 0.0,
        max: 1.0,
        divisions: 10,
        label: "Volume: $volume");
  }

  Widget _rate() {
    return Slider(
      value: rate,
      onChanged: (newRate) {
        setState(() => rate = newRate);
      },
      min: 0.0,
      max: 1.0,
      divisions: 10,
      label: "Rate: $rate",
      activeColor: Colors.green,
    );
  }
}
