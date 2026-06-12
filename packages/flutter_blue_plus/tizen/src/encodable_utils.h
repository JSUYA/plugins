// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_ENCODABLE_UTILS_H_
#define FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_ENCODABLE_UTILS_H_

#include <flutter/encodable_value.h>

#include <cstdint>
#include <string>
#include <vector>

namespace flutter_blue_plus_tizen {

using EncodableList = flutter::EncodableList;
using EncodableMap = flutter::EncodableMap;
using EncodableValue = flutter::EncodableValue;

template <typename T>
bool GetValueFromEncodableMap(const EncodableMap* map, const char* key,
                              T& out) {
  if (map == nullptr) {
    return false;
  }
  auto iter = map->find(EncodableValue(key));
  if (iter == map->end() || iter->second.IsNull()) {
    return false;
  }
  if (const auto* value = std::get_if<T>(&iter->second)) {
    out = *value;
    return true;
  }
  return false;
}

// The Dart side encodes integers as either int32 or int64 depending on value
// range, and occasionally as bool for flag fields. Accept all three.
inline bool GetIntLike(const EncodableMap* map, const char* key, int& out) {
  int32_t i32 = 0;
  if (GetValueFromEncodableMap(map, key, i32)) {
    out = i32;
    return true;
  }
  int64_t i64 = 0;
  if (GetValueFromEncodableMap(map, key, i64)) {
    out = static_cast<int>(i64);
    return true;
  }
  bool b = false;
  if (GetValueFromEncodableMap(map, key, b)) {
    out = b ? 1 : 0;
    return true;
  }
  return false;
}

inline bool GetBoolLike(const EncodableMap* map, const char* key, bool& out) {
  bool b = false;
  if (GetValueFromEncodableMap(map, key, b)) {
    out = b;
    return true;
  }
  int i = 0;
  if (GetIntLike(map, key, i)) {
    out = i != 0;
    return true;
  }
  return false;
}

inline std::vector<uint8_t> GetBytesOrEmpty(const EncodableMap* map,
                                            const char* key) {
  std::vector<uint8_t> out;
  GetValueFromEncodableMap(map, key, out);
  return out;
}

inline std::vector<std::string> GetStringList(const EncodableMap* map,
                                              const char* key) {
  std::vector<std::string> values;
  if (map == nullptr) {
    return values;
  }
  auto iter = map->find(EncodableValue(key));
  if (iter == map->end() || iter->second.IsNull()) {
    return values;
  }
  const auto* list = std::get_if<EncodableList>(&iter->second);
  if (list == nullptr) {
    return values;
  }
  for (const auto& value : *list) {
    if (const auto* string_value = std::get_if<std::string>(&value)) {
      values.push_back(*string_value);
    }
  }
  return values;
}

inline std::vector<EncodableMap> GetMapList(const EncodableMap* map,
                                            const char* key) {
  std::vector<EncodableMap> values;
  if (map == nullptr) {
    return values;
  }
  auto iter = map->find(EncodableValue(key));
  if (iter == map->end() || iter->second.IsNull()) {
    return values;
  }
  const auto* list = std::get_if<EncodableList>(&iter->second);
  if (list == nullptr) {
    return values;
  }
  for (const auto& value : *list) {
    if (const auto* nested = std::get_if<EncodableMap>(&value)) {
      values.push_back(*nested);
    }
  }
  return values;
}

inline EncodableValue IntFlag(bool value) {
  return EncodableValue(value ? 1 : 0);
}

}  // namespace flutter_blue_plus_tizen

#endif  // FLUTTER_PLUGIN_FLUTTER_BLUE_PLUS_TIZEN_ENCODABLE_UTILS_H_
