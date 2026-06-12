// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_EVENT_DISPATCHER_H_
#define FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_EVENT_DISPATCHER_H_

#include <flutter/encodable_value.h>
#include <flutter/event_sink.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "encodable_utils.h"

namespace flutter_blue_plus_tizen {

// Owns the Flutter EventSink for the plugin's event channel. Must only be
// touched from the platform main thread. Events are wrapped in
// {"type": ..., "data": <map>}.
class EventDispatcher {
 public:
  using FlEventSink = flutter::EventSink<EncodableValue>;

  EventDispatcher() = default;
  EventDispatcher(const EventDispatcher&) = delete;
  EventDispatcher& operator=(const EventDispatcher&) = delete;

  void SetSink(std::unique_ptr<FlEventSink> sink);
  void ResetSink();
  bool HasSink() const { return sink_ != nullptr; }

  void SetLogLevel(int level) { log_level_ = level; }
  int log_level() const { return log_level_; }

  // Emits a {"type": type, "data": data} envelope on the event channel.
  void EmitEvent(const std::string& type, EncodableMap data);

  // Writes the message to dlog at INFO level.
  void EmitLog(const std::string& message);

  // EmitLog gated on `log_level_ == kBmLogVerbose`.
  void LogVerbose(const std::string& message);

 private:
  std::unique_ptr<FlEventSink> sink_;
  int log_level_ = 0;
};

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_EVENT_DISPATCHER_H_
