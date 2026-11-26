#include "database.h"
#include "tarea.h"

#include <bcrypt/BCrypt.hpp>
#include <chrono>
#include <cstring>
#include <expected>
#include <filesystem>
#include <optional>
#include <sqlite3.h>
#include <string>
#include <system_error>
#include <vector>

std::expected<Database, DatabaseError> Database::open() {
    Database db;
    db.user = std::nullopt;

    // Ensure directory exists

    std::error_code ec;

    if (!std::filesystem::create_directories("db", ec)) {
        if (ec || !std::filesystem::is_directory("db")) {
            return std::unexpected(DatabaseError::CantOpen);
        }
    }

    // Open database

    if (sqlite3_open("db/main.db", &db.db) != SQLITE_OK) {
        sqlite3_close(db.db);
        return std::unexpected(DatabaseError::CantOpen);
    }

    if (!db.integrity_check()) {
        sqlite3_close(db.db);
        return std::unexpected(DatabaseError::Corrupted);
    }

    int version = db.get_version();

    switch (version) {
    case 0:
        if (db.create_schema() != DatabaseError::Ok) {
            sqlite3_close(db.db);
            return std::unexpected(DatabaseError::CreateSchema);
        }
        break;

    case 1:
        // Sets user_version to 1.
        // If the schema changes, the designated migration
        // function must be used.
        break;

    default:
        sqlite3_close(db.db);
        return std::unexpected(DatabaseError::BadVersion);
    }

    return db;
}

bool Database::integrity_check() {
    char *error = nullptr;
    bool valid = false;

    auto callback = [](void *data, int argc, char **argv, char **) -> int {
        bool *result = static_cast<bool *>(data);

        if (argc == 1 && argv[0] != nullptr) {
            *result = std::strcmp(argv[0], "ok") == 0;
        }

        return 0;
    };

    int rc = sqlite3_exec(this->db,
                          "PRAGMA integrity_check;",
                          callback,
                          &valid,
                          &error);

    if (rc != SQLITE_OK) {
        sqlite3_free(error);
        return false;
    }

    return valid;
}

size_t Database::get_version() {
    size_t version = 0;

    auto callback = [](void *data, int argc, char **argv, char **) -> int {
        auto *version = static_cast<size_t *>(data);

        if (argc == 1 && argv[0] != nullptr) {
            *version = std::stoul(argv[0]);
        }

        return 0;
    };

    char *error = nullptr;

    int rc = sqlite3_exec(this->db,
                          "PRAGMA user_version;",
                          callback,
                          &version,
                          &error);

    if (rc != SQLITE_OK) {
        sqlite3_free(error);
    }

    return version;
}

DatabaseError Database::create_schema() {
    constexpr const char *schema = R"sql(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE
        );

        CREATE TABLE tasks (
            id INTEGER PRIMARY KEY,
            user_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            description TEXT,
            due_date INTEGER,
            completed BOOLEAN NOT NULL
        );

        CREATE TABLE passwords (
            id INTEGER PRIMARY KEY,
            user_id INTEGER NOT NULL UNIQUE,
            hash TEXT NOT NULL
        );

        PRAGMA user_version = 1;
    )sql";

    char *error = nullptr;

    const int rc = sqlite3_exec(this->db, schema, nullptr, nullptr, &error);

    if (rc != SQLITE_OK) {
        sqlite3_free(error);
        return DatabaseError::CreateSchema;
    }

    return DatabaseError::Ok;
}

std::optional<int> Database::get_user_id(const std::string &name) {
    std::optional<int> user_id = std::nullopt;

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "SELECT id FROM users WHERE name = ?;";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return user_id;
}

std::optional<std::string> Database::get_hash(int user_id) {
    std::optional<std::string> hash = std::nullopt;

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "SELECT hash FROM passwords WHERE user_id = ?;";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *value =
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));

        if (value != nullptr) {
            hash = value;
        }
    }

    sqlite3_finalize(stmt);

    return hash;
}

DatabaseError Database::login(const std::string &user,
                              const std::string &password) {
    auto user_id = this->get_user_id(user);

    if (!user_id.has_value()) {
        return DatabaseError::UserNotFound;
    }

    auto hash = this->get_hash(user_id.value());

    if (!hash.has_value()) {
        return DatabaseError::PasswordNotFound;
    }

    if (BCrypt::validatePassword(password, hash.value())) {
        this->user = user_id.value();
        return DatabaseError::Ok;
    }

    return DatabaseError::IncorrectPassword;
}

std::optional<int> Database::insert_user(const std::string &name) {
    sqlite3_stmt *stmt = nullptr;

    const char *sql = "INSERT INTO users (name) VALUES (?);";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    const int id = static_cast<int>(sqlite3_last_insert_rowid(this->db));

    sqlite3_finalize(stmt);

    return id;
}

DatabaseError Database::insert_password(const std::string &hash, int user_id) {
    sqlite3_stmt *stmt = nullptr;

    const char *sql = "INSERT INTO passwords (user_id, hash) VALUES (?, ?);";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return DatabaseError::FailedToSavePassword;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return DatabaseError::FailedToSavePassword;
    }

    sqlite3_finalize(stmt);

    return DatabaseError::Ok;
}

DatabaseError Database::create_user(const std::string &user,
                                    const std::string &password) {
    auto user_id = this->insert_user(user);

    if (!user_id.has_value()) {
        return DatabaseError::NotUniqueUserName;
    }

    auto hash = BCrypt::generateHash(password, 14);

    auto rc = this->insert_password(hash, user_id.value());

    if (rc != DatabaseError::Ok) {
        // TODO: destroy user so no conflict
        return rc;
    }

    return DatabaseError::Ok;
}

DatabaseError Database::insert_task(Tarea tarea) {
    if (!this->user.has_value()) {
        return DatabaseError::NoUserLogged;
    }

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "INSERT INTO tasks "
                      "(user_id, title, description, due_date, completed) "
                      "SELECT ?, ?, ?, ?, ? "
                      "WHERE ("
                      "    SELECT COUNT(*) "
                      "    FROM tasks "
                      "    WHERE user_id = ? AND title = ?"
                      ") < 2;";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return DatabaseError::FailedTaskInsert;
    }

    // user_id
    sqlite3_bind_int(stmt, 1, this->user.value());

    // title
    sqlite3_bind_text(stmt, 2, tarea.get_title().c_str(), -1, SQLITE_TRANSIENT);

    // description
    if (tarea.get_description().has_value()) {
        sqlite3_bind_text(stmt,
                          3,
                          tarea.get_description().value().c_str(),
                          -1,
                          SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    // due_date
    if (tarea.get_date().has_value()) {
        const auto timestamp =
            std::chrono::system_clock::to_time_t(tarea.get_date().value());

        sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(timestamp));
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    // completed
    sqlite3_bind_int(stmt, 5, tarea.get_is_completed() ? 1 : 0);

    // Parameters used by the duplicate check
    sqlite3_bind_int(stmt, 6, this->user.value());

    sqlite3_bind_text(stmt, 7, tarea.get_title().c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return DatabaseError::FailedTaskInsert;
    }

    if (sqlite3_changes(this->db) == 0) {
        sqlite3_finalize(stmt);
        return DatabaseError::TaskAlreadyExists;
    }

    sqlite3_finalize(stmt);

    return DatabaseError::Ok;
}

std::expected<std::vector<Tarea>, DatabaseError> Database::get_tasks() {
    if (!this->user.has_value()) {
        return std::unexpected(DatabaseError::NoUserLogged);
    }

    std::vector<Tarea> tasks;

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "SELECT id, title, description, due_date, completed "
                      "FROM tasks "
                      "WHERE user_id = ?;";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return std::unexpected(DatabaseError::FailedTaskInsert);
    }

    sqlite3_bind_int(stmt, 1, this->user.value());

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // id
        const int id = sqlite3_column_int(stmt, 0);

        // title
        const char *title =
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        if (title == nullptr) {
            continue;
        }

        // description
        std::optional<std::string> description;

        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            const char *value =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

            if (value != nullptr) {
                description = value;
            }
        }

        // due_date
        std::optional<std::chrono::system_clock::time_point> date;

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
            const auto timestamp = sqlite3_column_int64(stmt, 3);

            date = std::chrono::system_clock::from_time_t(
                static_cast<std::time_t>(timestamp));
        }

        // completed
        const bool completed = sqlite3_column_int(stmt, 4) != 0;

        Tarea task(title, description, date, id, completed);

        tasks.push_back(std::move(task));
    }

    sqlite3_finalize(stmt);

    return tasks;
}

DatabaseError Database::update_task(const std::string &title, Tarea new_tarea) {
    if (!this->user.has_value()) {
        return DatabaseError::NoUserLogged;
    }

    if (!new_tarea.get_id().has_value()) {
        return DatabaseError::FailedTaskUpdate;
    }

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "UPDATE tasks "
                      "SET title = ?, "
                      "    description = ?, "
                      "    due_date = ?, "
                      "    completed = ? "
                      "WHERE id = ? "
                      "AND user_id = ? "
                      "AND ("
                      "    title = ? "
                      "    OR "
                      "    ("
                      "        SELECT COUNT(*) "
                      "        FROM tasks "
                      "        WHERE user_id = ? "
                      "        AND title = ?"
                      "    ) < 2"
                      ");";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return DatabaseError::FailedTaskUpdate;
    }

    // New title
    sqlite3_bind_text(stmt,
                      1,
                      new_tarea.get_title().c_str(),
                      -1,
                      SQLITE_TRANSIENT);

    // New description
    if (new_tarea.get_description().has_value()) {
        sqlite3_bind_text(stmt,
                          2,
                          new_tarea.get_description().value().c_str(),
                          -1,
                          SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 2);
    }

    // New due date
    if (new_tarea.get_date().has_value()) {
        const auto timestamp =
            std::chrono::system_clock::to_time_t(new_tarea.get_date().value());

        sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(timestamp));
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    // Completed
    sqlite3_bind_int(stmt, 4, new_tarea.get_is_completed() ? 1 : 0);

    // Task ID
    sqlite3_bind_int(stmt, 5, new_tarea.get_id().value());

    // User ID
    sqlite3_bind_int(stmt, 6, this->user.value());

    // Original title
    sqlite3_bind_text(stmt, 7, title.c_str(), -1, SQLITE_TRANSIENT);

    // User ID for duplicate check
    sqlite3_bind_int(stmt, 8, this->user.value());

    // New title for duplicate check
    sqlite3_bind_text(stmt,
                      9,
                      new_tarea.get_title().c_str(),
                      -1,
                      SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return DatabaseError::FailedTaskUpdate;
    }

    if (sqlite3_changes(this->db) == 0) {
        sqlite3_finalize(stmt);
        return DatabaseError::TaskAlreadyExists;
    }

    sqlite3_finalize(stmt);

    return DatabaseError::Ok;
}

DatabaseError Database::delete_task(Tarea tarea) {
    if (!this->user.has_value()) {
        return DatabaseError::NoUserLogged;
    }

    if (!tarea.get_id().has_value()) {
        return DatabaseError::FailedTaskDelete;
    }

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "DELETE FROM tasks "
                      "WHERE id = ? AND user_id = ?;";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return DatabaseError::FailedTaskDelete;
    }

    // Task ID
    sqlite3_bind_int(stmt, 1, tarea.get_id().value());

    // User ID
    sqlite3_bind_int(stmt, 2, this->user.value());

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return DatabaseError::FailedTaskDelete;
    }

    if (sqlite3_changes(this->db) == 0) {
        sqlite3_finalize(stmt);
        return DatabaseError::FailedTaskDelete;
    }

    sqlite3_finalize(stmt);

    return DatabaseError::Ok;
}

void Database::close() {
    sqlite3_close(this->db);
}
