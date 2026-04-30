// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of '../google_maps_flutter_tizen.dart';

/// This class manages all [GroundOverlayController]s associated to a [GoogleMapController].
class GroundOverlaysController extends GeometryController {
  /// Creates a new [GroundOverlaysController] instance.
  GroundOverlaysController({
    required StreamController<MapEvent<Object?>> stream,
  }) : _streamController = stream,
       _groundOverlayIdToController =
           <GroundOverlayId, GroundOverlayController>{},
       _idToGroundOverlayId = <int, GroundOverlayId>{};

  // A cache of [GroundOverlayController]s indexed by their [GroundOverlayId].
  final Map<GroundOverlayId, GroundOverlayController>
  _groundOverlayIdToController;
  final Map<int, GroundOverlayId> _idToGroundOverlayId;

  // The stream over which ground overlays broadcast events.
  final StreamController<MapEvent<Object?>> _streamController;

  /// Adds new [GroundOverlay]s to the cache.
  void addGroundOverlays(Set<GroundOverlay> groundOverlaysToAdd) {
    groundOverlaysToAdd.forEach(_addGroundOverlay);
  }

  void _addGroundOverlay(GroundOverlay groundOverlay) {
    final util.GGroundOverlayOptions options =
        _groundOverlayOptionsFromGroundOverlay(groundOverlay);
    final util.GGroundOverlay gGroundOverlay = util.GGroundOverlay(
      _urlFromMapBitmap(groundOverlay.image),
      _boundsStringFromGroundOverlay(groundOverlay),
      options,
    );
    final GroundOverlayController controller = GroundOverlayController(
      groundOverlay: gGroundOverlay,
      onTap: () {
        _onGroundOverlayTap(groundOverlay.groundOverlayId);
      },
      controller: util.webController,
    );
    _idToGroundOverlayId[gGroundOverlay.id] = groundOverlay.groundOverlayId;
    _groundOverlayIdToController[groundOverlay.groundOverlayId] = controller;
  }

  /// Updates [GroundOverlay]s with new options.
  void changeGroundOverlays(Set<GroundOverlay> groundOverlaysToChange) {
    groundOverlaysToChange.forEach(_changeGroundOverlay);
  }

  void _changeGroundOverlay(GroundOverlay groundOverlay) {
    final GroundOverlayController? controller =
        _groundOverlayIdToController[groundOverlay.groundOverlayId];
    controller?.update(_groundOverlayOptionsFromGroundOverlay(groundOverlay));
  }

  /// Removes the ground overlays associated with the given [GroundOverlayId]s.
  void removeGroundOverlays(Set<GroundOverlayId> groundOverlayIdsToRemove) {
    groundOverlayIdsToRemove.forEach(_removeGroundOverlay);
  }

  void _removeGroundOverlay(GroundOverlayId groundOverlayId) {
    final GroundOverlayController? controller =
        _groundOverlayIdToController[groundOverlayId];
    controller?.remove();
    _groundOverlayIdToController.remove(groundOverlayId);
    _idToGroundOverlayId.removeWhere(
      (int id, GroundOverlayId value) => value == groundOverlayId,
    );
  }

  void _onGroundOverlayTap(GroundOverlayId groundOverlayId) {
    _streamController.add(GroundOverlayTapEvent(mapId, groundOverlayId));
  }
}
