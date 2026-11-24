#include "database.h"
#include "tarea.h"
#include <bcrypt/BCrypt.hpp>
#include <chrono>
#include <cstring>
#include <expected>
#include <format>
#include <istream>
#include <optional>
#include <sqlite3.h>
#include <sstream>
#include <string>
#include <vector>

std::expected<Database, DatabaseError> Database::open() {
    Database db;
    db.user = std::nullopt;

    if (sqlite3_open("db/main.db", &db.db) != SQLITE_OK) {
        return std::unexpected(DatabaseError::CantOpen);
    }

    if (!db.integrity_check()) {
        return std::unexpected(DatabaseError::Corrupted);
    }

    int version = db.get_version();

    switch (version) {
    case 0:
        if (db.create_schema() != DatabaseError::Ok) {
            return std::unexpected(DatabaseError::CreateSchema);
        }
        break;
    case 1:
        // sets user_version to 1, if scheme changes,
        // the designated migration funtion must be used
        break;
    default:
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
                          "PRAGMA integrity_check",
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
                          "PRAGMA user_version",
                          callback,
                          &version,
                          &error);

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
            due_date TEXT,
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

    sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    // if it fails then never assigns;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int user_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return user_id;
}

std::optional<std::string> Database::get_hash(int user_id) {
    std::optional<std::string> hash = std::nullopt;

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "SELECT hash FROM passwords WHERE user_id = ?;";

    sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int(stmt, 1, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string hash =
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
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

    const char *sql = "INSERT INTO users (name) VALUES(?)";

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

    auto id = this->get_user_id(name);
    if (!id.has_value()) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    sqlite3_finalize(stmt);
    return id;
}

DatabaseError Database::insert_password(const std::string &hash, int user_id) {
    sqlite3_stmt *stmt = nullptr;

    const char *sql = "INSERT INTO passwords (user_id, hash) VALUES(?,?);";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return DatabaseError::FailedToSavePassword;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_OK) {
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
        // TODO destroy user so no conflict
        return rc;
    }

    return DatabaseError::Ok;
}

DatabaseError Database::insert_task(Tarea tarea) {
    if (!this->user.has_value()) {
        return DatabaseError::NoUserLogged;
    }

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "INSERT INTO tasks (user_id, title, description, "
                      "due_date, completed) VALUES(?,?,?,?,?);";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return DatabaseError::FailedTaskInsert;
    }

    sqlite3_bind_int(stmt, 1, this->user.value());
    sqlite3_bind_text(stmt, 2, tarea.get_title().c_str(), -1, SQLITE_TRANSIENT);

    if (tarea.get_description().has_value()) {
        sqlite3_bind_text(stmt,
                          3,
                          tarea.get_description().value().c_str(),
                          -1,
                          SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    if (tarea.get_date().has_value()) {
        sqlite3_bind_text(
            stmt,
            4,
            std::format("{:%Y-%m-%d}", tarea.get_date().value()).c_str(),
            -1,
            SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    if (tarea.get_is_completed()) {
        sqlite3_bind_int(stmt, 5, 1);
    } else {
        sqlite3_bind_int(stmt, 5, 0);
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return DatabaseError::FailedTaskInsert;
    }

    sqlite3_finalize(stmt);
    return DatabaseError::Ok;
}

std::expected<std::vector<Tarea>, DatabaseError> Database::get_tasks() {
    if (!this->user.has_value()) {
        return std::unexpected(DatabaseError::NoUserLogged);
    }

    std::expected<std::vector<Tarea>, DatabaseError> tasks;

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "SELECT title, description, date, completed FROM tasks "
                      "WHERE user_id = ?;";

    int rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return tasks;
    }

    sqlite3_bind_int(stmt, 1, this->user.value());

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *title =
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));

        std::optional<std::string> description;

        if (sqlite3_column_type(stmt, 1) == SQLITE_NULL) {
            description = std::nullopt;
        } else {
            description =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        }

        std::optional<std::chrono::year_month_day> date;
        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            std::istringstream stream(
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));

            std::chrono::year_month_day parsed_date;

            std::chrono::from_stream(stream, "%F", parsed_date);

            if (!stream.fail() && parsed_date.ok()) {
                date = parsed_date;
            }
        }

        bool completed;
        if (sqlite3_column_int(stmt, 3) != 0) {
            completed = true;
        } else {
            completed = false;
        }

        Tarea task = Tarea(title, description, date, completed);
        tasks->push_back(task);
    }

    return tasks;
}
