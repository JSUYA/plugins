// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "secure_storage.h"

#include <app_common.h>
#include <ckmc/ckmc-manager.h>
#include <ckmc/ckmc-type.h>
#include <sys/random.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <memory>
#include <type_traits>

constexpr char kSecureStorageAesKey[] = "SecureStorageAesKey";
constexpr size_t kInitializationVectorSize = 12;

namespace {

struct CkmcParamListDeleter {
  void operator()(std::remove_pointer_t<ckmc_param_list_h> *params) const {
    ckmc_param_list_free(params);
  }
};

struct CkmcBufferDeleter {
  void operator()(ckmc_raw_buffer_s *buffer) const { ckmc_buffer_free(buffer); }
};

struct CkmcAliasListDeleter {
  void operator()(ckmc_alias_list_s *list) const {
    ckmc_alias_list_all_free(list);
  }
};

using CkmcParamListPtr =
    std::unique_ptr<std::remove_pointer_t<ckmc_param_list_h>,
                    CkmcParamListDeleter>;
using CkmcBufferPtr = std::unique_ptr<ckmc_raw_buffer_s, CkmcBufferDeleter>;
using CkmcAliasListPtr =
    std::unique_ptr<ckmc_alias_list_s, CkmcAliasListDeleter>;

void CheckCkmcResult(const std::string &operation, int result) {
  if (result != CKMC_ERROR_NONE) {
    throw SecureStorageException(operation, result);
  }
}

}  // namespace

SecureStorageException::SecureStorageException(const std::string &operation,
                                               int error_code)
    : std::runtime_error(operation + " failed with error " +
                         std::to_string(error_code)),
      error_code_(error_code) {}

SecureStorage::SecureStorage() {}

SecureStorage::~SecureStorage() {}

void SecureStorage::Write(const std::string &key, const std::string &value) {
  CreateAesKeyOnce();
  std::vector<uint8_t> encrypted = Encrypt(value);

  ckmc_raw_buffer_s write_buffer;
  write_buffer.data = encrypted.data();
  write_buffer.size = encrypted.size();

  ckmc_policy_s policy = {
      .password = nullptr,
      .extractable = true,
  };

  CheckCkmcResult("ckmc_save_data",
                  ckmc_save_data(key.c_str(), write_buffer, policy));
}

std::optional<std::string> SecureStorage::Read(const std::string &key) {
  ckmc_raw_buffer_s *read_buffer = nullptr;
  int ret = ckmc_get_data(key.c_str(), nullptr, &read_buffer);
  CkmcBufferPtr read_buffer_ptr(read_buffer);
  if (ret == CKMC_ERROR_DB_ALIAS_UNKNOWN) {
    return std::nullopt;
  }
  CheckCkmcResult("ckmc_get_data", ret);
  if (read_buffer == nullptr ||
      (read_buffer->size > 0 && read_buffer->data == nullptr)) {
    throw SecureStorageException("ckmc_get_data", CKMC_ERROR_UNKNOWN);
  }

  std::vector<uint8_t> encrypted;
  if (read_buffer->size > 0) {
    encrypted.assign(read_buffer->data, read_buffer->data + read_buffer->size);
  }

  std::string decrypted = Decrypt(encrypted);
  return decrypted;
}

std::map<std::string, std::string> SecureStorage::ReadAll() {
  std::vector<std::string> keys = GetAliasList(AliasType::kData);

  std::map<std::string, std::string> map;
  for (const std::string &key : keys) {
    std::optional<std::string> value = Read(key);
    if (!value.has_value()) {
      throw SecureStorageException("ckmc_get_data",
                                   CKMC_ERROR_DB_ALIAS_UNKNOWN);
    }
    map[key] = value.value();
  }

  return map;
}

void SecureStorage::Delete(const std::string &key) {
  int ret = ckmc_remove_alias(key.c_str());
  if (ret != CKMC_ERROR_DB_ALIAS_UNKNOWN) {
    CheckCkmcResult("ckmc_remove_alias", ret);
  }
}

void SecureStorage::DeleteAll() {
  std::vector<std::string> keys = GetAliasList(AliasType::kData);
  for (std::string key : keys) {
    Delete(key);
  }
}

bool SecureStorage::ContainsKey(const std::string &key) {
  std::vector<std::string> keys = GetAliasList(AliasType::kData);
  return std::find(keys.begin(), keys.end(), key) != keys.end();
}

void SecureStorage::CreateAesKeyOnce() {
  std::vector<std::string> names = GetAliasList(AliasType::kKey);
  for (const auto &name : names) {
    if (name == kSecureStorageAesKey) {
      return;
    }
  }
  ckmc_policy_s policy = {
      .password = nullptr,
      .extractable = false,
  };
  int ret = ckmc_create_key_aes(256, kSecureStorageAesKey, policy);
  if (ret != CKMC_ERROR_DB_ALIAS_EXISTS) {
    CheckCkmcResult("ckmc_create_key_aes", ret);
  }
}

std::vector<uint8_t> SecureStorage::Encrypt(const std::string &value) {
  if (value.empty()) {
    return std::vector<uint8_t>{0x00};
  }

  ckmc_raw_buffer_s plain_buffer;
  plain_buffer.data =
      const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(value.c_str()));
  plain_buffer.size = value.length();

  ckmc_param_list_h params = nullptr;
  int ret = ckmc_generate_new_params(CKMC_ALGO_AES_GCM, &params);
  CkmcParamListPtr params_ptr(params);
  CheckCkmcResult("ckmc_generate_new_params", ret);
  if (params == nullptr) {
    throw SecureStorageException("ckmc_generate_new_params",
                                 CKMC_ERROR_UNKNOWN);
  }

  std::vector<uint8_t> iv = GenerateRandomVector();
  ckmc_raw_buffer_s iv_buffer;
  iv_buffer.data = iv.data();
  iv_buffer.size = iv.size();
  CheckCkmcResult(
      "ckmc_param_list_set_buffer",
      ckmc_param_list_set_buffer(params, CKMC_PARAM_ED_IV, &iv_buffer));

  ckmc_raw_buffer_s *encrypted_buffer = nullptr;
  ret = ckmc_encrypt_data(params, kSecureStorageAesKey, nullptr, plain_buffer,
                          &encrypted_buffer);
  CkmcBufferPtr encrypted_buffer_ptr(encrypted_buffer);
  CheckCkmcResult("ckmc_encrypt_data", ret);
  if (encrypted_buffer == nullptr || encrypted_buffer->size == 0 ||
      encrypted_buffer->data == nullptr) {
    throw SecureStorageException("ckmc_encrypt_data", CKMC_ERROR_UNKNOWN);
  }

  std::vector<uint8_t> encrypted(iv);
  encrypted.insert(encrypted.end(), encrypted_buffer->data,
                   encrypted_buffer->data + encrypted_buffer->size);

  return encrypted;
}

std::string SecureStorage::Decrypt(const std::vector<uint8_t> &value) {
  if (value.size() == 1 && value[0] == 0x00) {
    return "";
  }
  if (value.size() <= kInitializationVectorSize) {
    throw SecureStorageException("encrypted data", CKMC_ERROR_INVALID_FORMAT);
  }

  std::vector<uint8_t> iv(value.begin(),
                          value.begin() + kInitializationVectorSize);
  ckmc_raw_buffer_s iv_buffer;
  iv_buffer.data = iv.data();
  iv_buffer.size = iv.size();

  ckmc_param_list_h params = nullptr;
  int ret = ckmc_generate_new_params(CKMC_ALGO_AES_GCM, &params);
  CkmcParamListPtr params_ptr(params);
  CheckCkmcResult("ckmc_generate_new_params", ret);
  if (params == nullptr) {
    throw SecureStorageException("ckmc_generate_new_params",
                                 CKMC_ERROR_UNKNOWN);
  }

  CheckCkmcResult(
      "ckmc_param_list_set_buffer",
      ckmc_param_list_set_buffer(params, CKMC_PARAM_ED_IV, &iv_buffer));

  std::vector<uint8_t> encrypted_value(
      value.begin() + kInitializationVectorSize, value.end());
  ckmc_raw_buffer_s encrypted_buffer;
  encrypted_buffer.data = encrypted_value.data();
  encrypted_buffer.size = encrypted_value.size();

  ckmc_raw_buffer_s *decrypted_buffer = nullptr;
  ret = ckmc_decrypt_data(params, kSecureStorageAesKey, nullptr,
                          encrypted_buffer, &decrypted_buffer);
  CkmcBufferPtr decrypted_buffer_ptr(decrypted_buffer);
  CheckCkmcResult("ckmc_decrypt_data", ret);
  if (decrypted_buffer == nullptr ||
      (decrypted_buffer->size > 0 && decrypted_buffer->data == nullptr)) {
    throw SecureStorageException("ckmc_decrypt_data", CKMC_ERROR_UNKNOWN);
  }

  std::string decrypted;
  if (decrypted_buffer->size > 0) {
    decrypted.assign(decrypted_buffer->data,
                     decrypted_buffer->data + decrypted_buffer->size);
  }

  return decrypted;
}

std::vector<uint8_t> SecureStorage::GenerateRandomVector() {
  std::vector<uint8_t> vector(kInitializationVectorSize);
  if (getentropy(vector.data(), vector.size()) != 0) {
    throw SecureStorageException("getentropy", errno);
  }
  return vector;
}

std::vector<std::string> SecureStorage::GetAliasList(AliasType type) {
  ckmc_alias_list_s *ckmc_alias_list = nullptr;
  int ret = CKMC_ERROR_INVALID_PARAMETER;
  if (type == AliasType::kKey) {
    ret = ckmc_get_key_alias_list(&ckmc_alias_list);
  } else if (type == AliasType::kData) {
    ret = ckmc_get_data_alias_list(&ckmc_alias_list);
  }
  CkmcAliasListPtr alias_list_ptr(ckmc_alias_list);
  CheckCkmcResult("ckmc_get_alias_list", ret);

  char *app_id = nullptr;
  ret = app_get_id(&app_id);
  std::unique_ptr<char, decltype(&free)> app_id_ptr(app_id, &free);
  if (ret != 0 || app_id == nullptr) {
    throw SecureStorageException("app_get_id",
                                 ret != 0 ? ret : CKMC_ERROR_UNKNOWN);
  }
  std::string id = app_id;

  std::vector<std::string> names;
  ckmc_alias_list_s *current = ckmc_alias_list;

  while (current != nullptr) {
    std::string name = current->alias;
    // Remove prefix of alias.
    names.push_back(name.substr(name.find(id) + id.length() + 1));
    current = current->next;
  }

  return names;
}
