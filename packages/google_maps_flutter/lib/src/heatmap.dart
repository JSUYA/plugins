// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of '../google_maps_flutter_tizen.dart';

/// The `HeatmapController` class wraps a [util.GHeatmapLayer] and its `onTap` behavior.
class HeatmapController {
  /// Creates a `HeatmapController`, which wraps a [util.GHeatmapLayer] object and its `onTap` behavior.
  HeatmapController({required util.GHeatmapLayer heatmap}) : _heatmap = heatmap;

  util.GHeatmapLayer? _heatmap;

  /// Returns the wrapped [util.GHeatmapLayer]. Only used for testing.
  @visibleForTesting
  util.GHeatmapLayer? get heatmap => _heatmap;

  /// Updates the options of the wrapped [util.GHeatmapLayer] object.
  ///
  /// This cannot be called after [remove].
  void update(util.GHeatmapLayerOptions options) {
    assert(_heatmap != null, 'Cannot `update` Heatmap after calling `remove`.');
    _heatmap!.options = options;
  }

  /// Disposes of the currently wrapped [util.GHeatmapLayer].
  void remove() {
    if (_heatmap != null) {
      _heatmap!.map = null;
      _heatmap = null;
    }
  }
}
