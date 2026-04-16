// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "event_dispatcher.h"

#include <dlog.h>

#include <utility>

#include "bt_constants.h"

namespace flutter_blue_plus_tizen {

void EventDispatcher::SetSink(std::unique_ptr<FlEventSink> sink) {
  sink_ = std::move(sink);
}

void EventDispatcher::ResetSink() { sink_.reset(); }

void EventDispatcher::EmitEvent(const std::string& type, EncodableMap data) {
  if (!sink_) {
    return;
  }
  sink_->Success(EncodableValue(EncodableMap{
      {EncodableValue("type"), EncodableValue(type)},
      {EncodableValue("data"), EncodableValue(std::move(data))},
  }));
}

void EventDispatcher::EmitLog(const std::string& message) {
  dlog_print(DLOG_INFO, kLogTag, "%s", message.c_str());
  if (!sink_) {
    return;
  }
  sink_->Success(EncodableValue(EncodableMap{
      {EncodableValue("type"), EncodableValue("log")},
      {EncodableValue("message"), EncodableValue(message)},
  }));
}

void EventDispatcher::LogVerbose(const std::string& message) {
  if (log_level_ == kBmLogVerbose) {
    EmitLog("[FBP-Tizen] " + message);
  }
}

}  // namespace flutter_blue_plus_tizen
