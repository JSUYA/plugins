// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_surface_view_factory.h"

#include <app_common.h>
#include <flutter/encodable_value.h>
#include <flutter/message_codec.h>

#include <string>
#include <variant>

#include "log.h"
#include "tizen_surface_view.h"

TizenSurfaceViewFactory::TizenSurfaceViewFactory(
    flutter::PluginRegistrar* registrar, void* win)
    : PlatformViewFactory(registrar), win_(win) {
  texture_registrar_ = registrar->texture_registrar();
}

PlatformView* TizenSurfaceViewFactory::Create(int view_id, double width,
                                              double height,
                                              const ByteMessage& params) {
  return new TizenSurfaceView(GetPluginRegistrar(), view_id, texture_registrar_,
                              width, height, *GetCodec().DecodeMessage(params),
                              win_);
}

void TizenSurfaceViewFactory::Dispose() {}
