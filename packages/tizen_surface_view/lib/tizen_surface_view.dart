// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// import 'dart:async';

// import 'package:flutter/foundation.dart';
// import 'package:flutter/gestures.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_tizen/widgets.dart';

class TizenSurfaceView extends TizenView {
  static void register() {}

  const TizenSurfaceView({super.key, required this.viewType})
      : super(
          viewType: viewType,
          layoutDirection: TextDirection.ltr,
        );

  final String viewType;
}
