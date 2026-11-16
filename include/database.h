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
    Corrupted,
    CreateSchema,
    BadVersion,
};

class Database {
  private:
    sqlite3 *db;
    std::optional<size_t> user;

    bool integrity_check();
    size_t get_version();
    DatabaseError create_schema();

  public:
    std::expected<Database, DatabaseError> open();
};
