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

// EventDispatcher owns the Flutter `EventSink` for the plugin's event channel
// and emits the typed events that the Dart side
// (`flutter_blue_plus_tizen.dart`) consumes. The dispatcher must only be
// touched from the platform main thread; callers receiving bt_* callbacks from
// worker threads have to hop to the main loop via `DispatchToMain()` before
// calling any Emit* method.
//
// All typed events (adapter_state_changed, connection_state_changed, ...) are
// wrapped in {"type": ..., "data": <EncodableMap>}. The "log" event is special
// and uses {"type": "log", "message": ...} to match the Dart-side log handler.
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

  // Emits a {"type": "log", "message": message} envelope and writes the same
  // message to dlog at INFO level so the log shows up in journalctl.
  void EmitLog(const std::string& message);

  // EmitLog gated on `log_level_ == kBmLogVerbose`.
  void LogVerbose(const std::string& message);

 private:
  std::unique_ptr<FlEventSink> sink_;
  int log_level_ = 0;
};

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_EVENT_DISPATCHER_H_
