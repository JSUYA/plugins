// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_TIZEN_WINDOW_MANAGER_H_
#define FLUTTER_PLUGIN_TIZEN_WINDOW_MANAGER_H_

#include <flutter/encodable_value.h>
#include <flutter/method_result.h>

#include <memory>

class EcoreWl2WindowProxy;

class TizenWindowManager {
 public:
  explicit TizenWindowManager(void* handle);
  virtual ~TizenWindowManager();

  bool Activate();
  bool Lower();
  bool GetGeometry(flutter::EncodableMap* geometry);

 private:
  void* window_handle_;
  std::unique_ptr<EcoreWl2WindowProxy> proxy_;
};

#endif  // FLUTTER_PLUGIN_TIZEN_WINDOW_MANAGER_H_
