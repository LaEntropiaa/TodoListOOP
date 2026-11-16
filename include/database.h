#pragma once

#include "tarea.h"
#include <cstddef>
#include <expected>
#include <inttypes.h>
#include <optional>
#include <sqlite3.h>

constexpr int DATABASE_VERSION = 1;

enum class DatabaseError {
    Ok,
    CantOpen,
    UserNotFound,
    PasswordNotFound,
    Corrupted,
    CreateSchema,
    BadVersion,
    IncorrectPassword,
};

class Database {
  private:
    sqlite3 *db;
    std::optional<size_t> user;

    bool integrity_check();
    size_t get_version();
    DatabaseError create_schema();

    std::optional<int> get_user_id(const std::string &name);
    std::optional<std::string> get_salt(int user_id);
    std::optional<std::string> get_hash(int user_id);

  public:
    static std::expected<Database, DatabaseError> open();
    DatabaseError login(const std::string &user, const std::string &password);
};
