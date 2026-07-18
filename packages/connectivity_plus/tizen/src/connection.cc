// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "connection.h"

#include <utility>

#include "log.h"

static ConnectionType ToConnectionType(connection_type_e type) {
  switch (type) {
    case CONNECTION_TYPE_WIFI:
      return ConnectionType::kWiFi;
    case CONNECTION_TYPE_CELLULAR:
      return ConnectionType::kMobile;
    case CONNECTION_TYPE_ETHERNET:
      return ConnectionType::kEthernet;
    case CONNECTION_TYPE_BT:
      return ConnectionType::kBluetooth;
    case CONNECTION_TYPE_DISCONNECTED:
      return ConnectionType::kNone;
    case CONNECTION_TYPE_NET_PROXY:
    default:
      return ConnectionType::kOther;
  }
}

Connection::Connection() {
  int ret = connection_create(&connection_);
  if (ret != CONNECTION_ERROR_NONE) {
    LOG_ERROR("Failed to create handle: %s", get_error_message(ret));
    last_error_ = ret;
  }
}

Connection::~Connection() {
  if (connection_) {
    if (listening_) {
      connection_unset_type_changed_cb(connection_);
    }
    connection_destroy(connection_);
    connection_ = nullptr;
  }
}

bool Connection::StartListen(ConnectionTypeCallback callback) {
  if (!connection_) {
    return false;
  }

  if (listening_) {
    callback_ = std::move(callback);
    return true;
  }

  callback_ = std::move(callback);
  int ret = connection_set_type_changed_cb(
      connection_,
      [](connection_type_e type, void *user_data) -> void {
        auto *self = static_cast<Connection *>(user_data);
        if (self->callback_) {
          self->callback_(ToConnectionType(type));
        }
      },
      this);
  if (ret != CONNECTION_ERROR_NONE) {
    LOG_ERROR("Failed to add callback: %s", get_error_message(ret));
    last_error_ = ret;
    callback_ = nullptr;
    return false;
  }

  listening_ = true;
  return true;
}

void Connection::StopListen() {
  if (!connection_ || !listening_) {
    return;
  }

  int ret = connection_unset_type_changed_cb(connection_);
  if (ret != CONNECTION_ERROR_NONE) {
    LOG_ERROR("Failed to remove callback: %s", get_error_message(ret));
    last_error_ = ret;
    return;
  }

  listening_ = false;
  callback_ = nullptr;
}

ConnectionType Connection::GetType() {
  if (!connection_) {
    return ConnectionType::kError;
  }

  connection_type_e type;
  int ret = connection_get_type(connection_, &type);
  if (ret != CONNECTION_ERROR_NONE) {
    LOG_ERROR("Failed to get connection type: %s", get_error_message(ret));
    last_error_ = ret;
    return ConnectionType::kError;
  }
  return ToConnectionType(type);
}
