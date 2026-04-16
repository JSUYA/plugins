import 'dart:io';

import 'package:mime/mime.dart';
import 'package:share_plus_platform_interface/share_plus_platform_interface.dart';
import 'package:tizen_app_control/tizen_app_control.dart';

const String _shareOperation = 'http://tizen.org/appcontrol/operation/share';
const String _multiShareOperation =
    'http://tizen.org/appcontrol/operation/multi_share';
const String _shareTextOperation =
    'http://tizen.org/appcontrol/operation/share_text';
const String _textDataKey = 'http://tizen.org/appcontrol/data/text';
const String _pathDataKey = 'http://tizen.org/appcontrol/data/path';

/// The Tizen implementation of [SharePlatform].
class SharePlusTizenPlugin extends SharePlatform {
  /// Registers this class as the default instance of [SharePlatform].
  static void register() {
    SharePlatform.instance = SharePlusTizenPlugin();
  }

  @override
  Future<ShareResult> share(ShareParams params) async {
    final List<XFile> files = await _materializeFiles(
      params.files ?? const <XFile>[],
      params.fileNameOverrides,
    );

    final String? sharedText = params.text ?? params.uri?.toString();
    final AppControl request = _buildRequest(files, sharedText);
    final List<String> matched = await request.getMatchedAppIds();
    if (matched.isEmpty) {
      return ShareResult.unavailable;
    }

    await request.sendLaunchRequest();
    return ShareResult.unavailable;
  }

  AppControl _buildRequest(List<XFile> files, String? sharedText) {
    if (files.isEmpty) {
      return AppControl(
        operation: _shareTextOperation,
        extraData: <String, dynamic>{
          if (sharedText != null) _textDataKey: sharedText,
        },
      );
    }

    final List<String> paths = files
        .map((XFile file) => File(file.path).absolute.path)
        .toList();
    final String? mimeType = _resolveMimeType(files);

    return AppControl(
      operation: paths.length == 1 ? _shareOperation : _multiShareOperation,
      mime: mimeType,
      extraData: <String, dynamic>{
        _pathDataKey: paths.length == 1 ? paths.single : paths,
        if (sharedText != null) _textDataKey: sharedText,
      },
    );
  }

  Future<List<XFile>> _materializeFiles(
    List<XFile> files,
    List<String>? fileNameOverrides,
  ) async {
    final List<XFile> prepared = <XFile>[];
    for (int index = 0; index < files.length; index++) {
      prepared.add(
        await _materializeFile(
          files[index],
          nameOverride: fileNameOverrides?.elementAt(index),
        ),
      );
    }
    return prepared;
  }

  Future<XFile> _materializeFile(XFile file, {String? nameOverride}) async {
    if (file.path.isNotEmpty) {
      return file;
    }

    final Directory tempDir = await Directory.systemTemp.createTemp(
      'share_plus_tizen_',
    );
    final String extension =
        extensionFromMime(file.mimeType ?? 'application/octet-stream') ?? 'bin';
    final String fileName =
        nameOverride ??
        (file.name.isNotEmpty
            ? file.name
            : 'shared_${DateTime.now().microsecondsSinceEpoch}.$extension');
    final String path = '${tempDir.path}/$fileName';
    await File(path).writeAsBytes(await file.readAsBytes());
    return XFile(path, mimeType: file.mimeType, name: fileName);
  }

  String? _resolveMimeType(List<XFile> files) {
    if (files.isEmpty) {
      return null;
    }

    final Set<String> mimeTypes = files
        .map(
          (XFile file) =>
              file.mimeType ??
              lookupMimeType(file.path) ??
              'application/octet-stream',
        )
        .toSet();
    return mimeTypes.length == 1 ? mimeTypes.single : null;
  }
}
