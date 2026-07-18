#include "database_manager.h"

#include <flutter/standard_method_codec.h>
#include <sqlite3.h>

#include <list>
#include <limits>
#include <variant>

#include "errors.h"
#include "log.h"
#include "log_level.h"

namespace sqflite_database {

namespace {

template <typename T>
int GetBlobByteCount(const std::vector<T> &value) {
  if (value.size() >
      static_cast<size_t>(std::numeric_limits<int>::max()) / sizeof(T)) {
    throw sqflite_errors::DatabaseError(
        sqflite_errors::kUnknownErrorCode,
        "statement parameter is too large");
  }
  return static_cast<int>(value.size() * sizeof(T));
}

template <typename T>
int BindBlob(sqlite3_stmt *statement, int index,
             const std::vector<T> &value) {
  if (value.empty()) {
    return sqlite3_bind_zeroblob(statement, index, 0);
  }
  return sqlite3_bind_blob(statement, index, value.data(),
                           GetBlobByteCount(value), SQLITE_TRANSIENT);
}

}  // namespace

DatabaseManager::~DatabaseManager() noexcept {
  for (auto &&statement : statement_cache_) {
    FinalizeStmt(statement.second);
    statement.second = nullptr;
  }
  statement_cache_.clear();

  Close(false);
}

void DatabaseManager::ThrowCurrentDatabaseError() {
  throw sqflite_errors::DatabaseError(GetErrorCode(), GetErrorMsg());
}

void DatabaseManager::Open() {
  int result_code =
      sqlite3_open_v2(path_.c_str(), &database_,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  if (result_code != SQLITE_OK) {
    const int error_code = GetErrorCode();
    const std::string error_message = GetErrorMsg();
    Close(false);
    throw sqflite_errors::DatabaseError(error_code, error_message.c_str());
  }
  result_code = sqlite3_busy_timeout(database_, kBusyTimeoutMs);
  if (result_code != SQLITE_OK) {
    const int error_code = GetErrorCode();
    const std::string error_message = GetErrorMsg();
    Close(false);
    throw sqflite_errors::DatabaseError(error_code, error_message.c_str());
  }
}

void DatabaseManager::OpenReadOnly() {
  int result_code =
      sqlite3_open_v2(path_.c_str(), &database_, SQLITE_OPEN_READONLY, NULL);
  if (result_code != SQLITE_OK) {
    const int error_code = GetErrorCode();
    const std::string error_message = GetErrorMsg();
    Close(false);
    throw sqflite_errors::DatabaseError(error_code, error_message.c_str());
  }
  result_code = sqlite3_busy_timeout(database_, kBusyTimeoutMs);
  if (result_code != SQLITE_OK) {
    const int error_code = GetErrorCode();
    const std::string error_message = GetErrorMsg();
    Close(false);
    throw sqflite_errors::DatabaseError(error_code, error_message.c_str());
  }
}

const char *DatabaseManager::GetErrorMsg() { return sqlite3_errmsg(database_); }

int DatabaseManager::GetErrorCode() {
  return sqlite3_extended_errcode(database_);
}

void DatabaseManager::Close(bool raise_error) {
  if (!database_) {
    return;
  }
  Database database = database_;
  int result_code = sqlite3_close_v2(database);
  if (result_code != SQLITE_OK && raise_error) {
    throw sqflite_errors::DatabaseError(sqlite3_extended_errcode(database),
                                        sqlite3_errmsg(database));
  }
  if (result_code == SQLITE_OK) {
    database_ = nullptr;
  }
}

void DatabaseManager::BindStmtParams(DatabaseManager::Statement statement,
                                     SQLParameters parameters) {
  int result_code = SQLITE_OK;
  const int parameters_length = parameters.size();
  for (int i = 0; i < parameters_length; i++) {
    auto idx = i + 1;
    auto parameter = parameters[i];
    switch (parameter.index()) {
      case 0: {
        result_code = sqlite3_bind_null(statement, idx);
        break;
      }
      case 1: {
        auto value = std::get<bool>(parameter);
        result_code = sqlite3_bind_int(statement, idx, int(value));
        break;
      }
      case 2: {
        auto value = std::get<int32_t>(parameter);
        result_code = sqlite3_bind_int(statement, idx, value);
        break;
      }
      case 3: {
        auto value = std::get<int64_t>(parameter);
        result_code = sqlite3_bind_int64(statement, idx, value);
        break;
      }
      case 4: {
        auto value = std::get<double>(parameter);
        result_code = sqlite3_bind_double(statement, idx, value);
        break;
      }
      case 5: {
        auto value = std::get<std::string>(parameter);
        result_code = sqlite3_bind_text(statement, idx, value.c_str(),
                                        value.size(), SQLITE_TRANSIENT);
        break;
      }
      case 6: {
        auto vector = std::get<std::vector<uint8_t>>(parameter);
        result_code = BindBlob(statement, idx, vector);
        break;
      }
      case 7: {
        auto vector = std::get<std::vector<int32_t>>(parameter);
        result_code = BindBlob(statement, idx, vector);
        break;
      }
      case 8: {
        auto vector = std::get<std::vector<int64_t>>(parameter);
        result_code = BindBlob(statement, idx, vector);
        break;
      }
      case 9: {
        auto vector = std::get<std::vector<double>>(parameter);
        result_code = BindBlob(statement, idx, vector);
        break;
      }
      case 10: {
        auto value = std::get<flutter::EncodableList>(parameter);
        std::vector<uint8_t> vector;
        // Only  a list of uint8_t for flutter EncodableValue is supported
        // to store it as a BLOB, otherwise a DatabaseError is triggered
        try {
          for (auto item : value) {
            vector.push_back(std::get<int>(item));
          }
        } catch (const std::bad_variant_access) {
          throw sqflite_errors::DatabaseError(
              sqflite_errors::kUnknownErrorCode,
              "statement parameter is not supported");
        }
        result_code = BindBlob(statement, idx, vector);
        break;
      }
      default: {
        throw sqflite_errors::DatabaseError(
            sqflite_errors::kUnknownErrorCode,
            "statement parameter is not supported");
      }
    }
    if (result_code != SQLITE_OK) {
      ThrowCurrentDatabaseError();
    }
  }
}

DatabaseManager::Statement DatabaseManager::PrepareStmt(std::string sql) {
  auto cache_entry = statement_cache_.find(sql);
  if (cache_entry != statement_cache_.end()) {
    DatabaseManager::Statement statement = cache_entry->second;
    sqlite3_reset(statement);
    sqlite3_clear_bindings(statement);
    return statement;
  } else {
    DatabaseManager::Statement statement;
    int result_code =
        sqlite3_prepare_v2(database_, sql.c_str(), -1, &statement, nullptr);
    if (result_code) {
      FinalizeStmt(statement);
      ThrowCurrentDatabaseError();
    }
    if (statement != nullptr) {
      statement_cache_[sql] = statement;
    }
    return statement;
  }
}

void DatabaseManager::ExecuteStmt(DatabaseManager::Statement statement) {
  int result_code = SQLITE_OK;
  do {
    result_code = sqlite3_step(statement);
  } while (result_code == SQLITE_ROW);
  if (result_code != SQLITE_DONE) {
    ThrowCurrentDatabaseError();
  }
}

int DatabaseManager::GetStmtColumnsCount(DatabaseManager::Statement statement) {
  return sqlite3_column_count(statement);
}

int DatabaseManager::GetColumnType(DatabaseManager::Statement statement,
                                   int column_index) {
  return sqlite3_column_type(statement, column_index);
}

const char *DatabaseManager::GetColumnName(DatabaseManager::Statement statement,
                                           int column_index) {
  return sqlite3_column_name(statement, column_index);
}

std::pair<Columns, Resultset> DatabaseManager::QueryStmt(
    DatabaseManager::Statement statement) {
  Columns columns;
  Resultset resultset;
  const int columns_count = GetStmtColumnsCount(statement);
  int result_code = SQLITE_OK;
  for (int i = 0; i < columns_count; i++) {
    auto column_name = GetColumnName(statement, i);
    columns.push_back(std::string(column_name));
  }
  do {
    result_code = sqlite3_step(statement);
    if (result_code == SQLITE_ROW) {
      Result result;
      for (int i = 0; i < columns_count; i++) {
        ResultValue value;
        auto column_type = GetColumnType(statement, i);
        switch (column_type) {
          case SQLITE_INTEGER:
            value = (int64_t)sqlite3_column_int64(statement, i);
            result.push_back(value);
            break;
          case SQLITE_FLOAT:
            value = sqlite3_column_double(statement, i);
            result.push_back(value);
            break;
          case SQLITE_TEXT:
            value =
                std::string((const char *)sqlite3_column_text(statement, i));
            result.push_back(value);
            break;
          case SQLITE_BLOB: {
            const int byte_count = sqlite3_column_bytes(statement, i);
            const uint8_t *blob = reinterpret_cast<const uint8_t *>(
                sqlite3_column_blob(statement, i));
            std::vector<uint8_t> v;
            if (byte_count > 0) {
              v.assign(blob, blob + byte_count);
            }
            result.push_back(v);
            break;
          }
          case SQLITE_NULL:
            value = nullptr;
            result.push_back(value);
            break;
          default:
            break;
        }
      }
      resultset.push_back(result);
    }
  } while (result_code == SQLITE_ROW);
  if (result_code != SQLITE_DONE) {
    ThrowCurrentDatabaseError();
  }
  return std::make_pair(columns, resultset);
}

void DatabaseManager::FinalizeStmt(DatabaseManager::Statement statement) {
  sqlite3_finalize(statement);
}

void DatabaseManager::LogQuery(Statement statement) {
  char *expanded_sql = sqlite3_expanded_sql(statement);
  if (expanded_sql) {
    LOG_DEBUG("%s", expanded_sql);
    sqlite3_free(expanded_sql);
  }
}

std::pair<Columns, Resultset> DatabaseManager::Query(std::string sql,
                                                     SQLParameters parameters) {
  auto statement = PrepareStmt(sql);
  BindStmtParams(statement, parameters);
  if (sqflite_log_level::HasSqlLevel(log_level_)) {
    LogQuery(statement);
  }
  return QueryStmt(statement);
}

void DatabaseManager::Execute(std::string sql, SQLParameters parameters) {
  Statement statement = PrepareStmt(sql);
  BindStmtParams(statement, parameters);
  if (sqflite_log_level::HasSqlLevel(log_level_)) {
    LogQuery(statement);
  }
  ExecuteStmt(statement);
}
}  // namespace sqflite_database
