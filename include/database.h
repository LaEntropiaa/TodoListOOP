#pragma once

#include "tarea.h"
#include <cstddef>
#include <expected>
#include <inttypes.h>
#include <optional>
#include <sqlite3.h>
#include <string>
#include <vector>

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
    NotUniqueUserName,
    FailedToSavePassword,
    NoUserLogged,
    FailedTaskInsert,
    BadQuery,
    InvalidTask,
    TaskAlreadyExists,
    FailedTaskUpdate,
};

class Database {
  private:
    sqlite3 *db;
    std::optional<size_t> user;

    bool integrity_check();
    size_t get_version();
    DatabaseError create_schema();

    std::optional<int> get_user_id(const std::string &name);
    std::optional<std::string> get_hash(int user_id);

    std::optional<int> insert_user(const std::string &name);
    DatabaseError insert_password(const std::string &hash, int user_id);
    bool task_exists(const std::string &name);

  public:
    static std::expected<Database, DatabaseError> open();
    DatabaseError login(const std::string &user, const std::string &password);
    DatabaseError create_user(const std::string &user,
                              const std::string &password);

    DatabaseError insert_task(Tarea tarea);
    std::expected<std::vector<Tarea>, DatabaseError> get_tasks();
    DatabaseError update_task(const std::string &title, Tarea new_tarea);
};
