// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_window_manager.h"

#include <flutter/encodable_value.h>
#include <flutter/standard_method_codec.h>

#include <memory>

#include "ecore_wl2_window_proxy.h"
#include "log.h"

TizenWindowManager::TizenWindowManager(void* handle)
    : window_handle_(handle), proxy_(std::make_unique<EcoreWl2WindowProxy>()) {}

TizenWindowManager::~TizenWindowManager() {}

bool TizenWindowManager::Activate() {
  if (!window_handle_) {
    LOG_ERROR("Window handle is null");
    return false;
  }

  return proxy_->ecore_wl2_window_activate(window_handle_);
}

bool TizenWindowManager::Lower() {
  if (!window_handle_) {
    LOG_ERROR("Window handle is null");
    return false;
  }

  return proxy_->ecore_wl2_window_lower(window_handle_);
}

bool TizenWindowManager::GetGeometry(flutter::EncodableMap* geometry) {
  if (!window_handle_) {
    LOG_ERROR("Window handle is null");
    return false;
  }

  int x = 0, y = 0, width = 0, height = 0;
  if (!proxy_->ecore_wl2_window_geometry_get(window_handle_, &x, &y, &width,
                                             &height)) {
    return false;
  }

  (*geometry)[flutter::EncodableValue("x")] = flutter::EncodableValue(x);
  (*geometry)[flutter::EncodableValue("y")] = flutter::EncodableValue(y);
  (*geometry)[flutter::EncodableValue("width")] =
      flutter::EncodableValue(width);
  (*geometry)[flutter::EncodableValue("height")] =
      flutter::EncodableValue(height);

  return true;
}
