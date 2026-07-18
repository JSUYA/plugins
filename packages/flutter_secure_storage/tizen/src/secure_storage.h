// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_SECURE_STORAGE_H_
#define FLUTTER_PLUGIN_SECURE_STORAGE_H_

#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

class SecureStorageException : public std::runtime_error {
 public:
  SecureStorageException(const std::string &operation, int error_code);

  int error_code() const { return error_code_; }

 private:
  int error_code_;
};

class SecureStorage {
 public:
  SecureStorage();
  ~SecureStorage();

  void Write(const std::string &key, const std::string &value);

  std::optional<std::string> Read(const std::string &key);

  std::map<std::string, std::string> ReadAll();

  void Delete(const std::string &key);

  void DeleteAll();

  bool ContainsKey(const std::string &key);

 private:
  enum class AliasType {
    kKey,
    kData,
  };

  void CreateAesKeyOnce();

  std::vector<uint8_t> Encrypt(const std::string &value);

  std::string Decrypt(const std::vector<uint8_t> &value);

  std::vector<uint8_t> GenerateRandomVector();

  std::vector<std::string> GetAliasList(AliasType type);
};

#endif  // FLUTTER_PLUGIN_SECURE_STORAGE_H_
