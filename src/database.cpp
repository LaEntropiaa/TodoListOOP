#include "database.h"
#include "tarea.h"
#include <bcrypt/BCrypt.hpp>
#include <chrono>
#include <cstring>
#include <expected>
#include <format>
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
    std::optional<int> user_id;

    auto callback = [](void *data, int argc, char **argv, char **) -> int {
        auto *id = static_cast<std::optional<int> *>(data);

        if (argc == 1 && argv[0] != nullptr) {
            *id = std::stoi(argv[0]);
        }

        return 0;
    };

    char *error = nullptr;

    std::string query = "SELECT id FROM users WHERE name = '" + name + "';";

    int rc = sqlite3_exec(this->db, query.c_str(), callback, &user_id, &error);

    if (rc != SQLITE_OK) {
        sqlite3_free(error);
        return std::nullopt;
    }

    return user_id;
}

std::optional<std::string> Database::get_hash(int user_id) {
    std::optional<std::string> hash;

    auto callback = [](void *data, int argc, char **argv, char **) -> int {
        auto *hash = static_cast<std::optional<std::string> *>(data);

        if (argc == 1 && argv[0] != nullptr) {
            *hash = std::string(argv[0]);
        }

        return 0;
    };

    char *error = nullptr;

    std::string query = "SELECT hash FROM passwords WHERE user_id = " +
                        std::to_string(user_id) + ";";

    int rc = sqlite3_exec(this->db, query.c_str(), callback, &hash, &error);

    if (rc != SQLITE_OK) {
        sqlite3_free(error);
        return std::nullopt;
    }

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
    std::string query = "INSERT INTO users (name) VALUES('" + name + "');";

    char *error = nullptr;

    int rc = sqlite3_exec(this->db, query.c_str(), nullptr, nullptr, &error);

    if (rc != SQLITE_OK) {
        return std::nullopt;
    }

    auto id = this->get_user_id(name);
    if (!id.has_value()) {
        return std::nullopt;
    }

    return id;
}

DatabaseError Database::insert_password(const std::string &hash, int user_id) {
    std::string query = "INSERT INTO passwords (user_id, hash) VALUES('" +
                        hash + "', " + std::to_string(user_id) + ");";

    char *error = nullptr;

    int rc = sqlite3_exec(this->db, query.c_str(), nullptr, nullptr, &error);

    if (rc != SQLITE_OK) {
        return DatabaseError::FailedToSavePassword;
    }

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

    std::string desc;
    if (!tarea.get_description().has_value()) {
        desc = "NULL";
    } else {
        desc = tarea.get_description().value();
    }

    std::string date;
    if (!tarea.get_date().has_value()) {
        date = "NULL";
    } else {
        date = std::format("{:%Y-%m-%d}", tarea.get_date().value());
    }

    std::string completed;
    if (!tarea.get_is_completed()) {
        completed = "0";
    } else {
        completed = "1";
    }

    std::string query =
        "INSERT INTO tasks (user_id, title, description, due_date, "
        "completed) VALUES(" +
        std::to_string(this->user.value()) + ", '" + tarea.get_title() +
        "', '" + desc + "', '" + date + "', " + completed + ");";

    char *error = nullptr;
    int rc = sqlite3_exec(this->db, query.c_str(), nullptr, nullptr, &error);

    if (rc != SQLITE_OK) {
        return DatabaseError::FailedTaskInsert;
    }

    return DatabaseError::Ok;
}

int Database::parse_user_tasks(void *vector, int argc, char **argv, char **) {
    auto *tasks =
        static_cast<std::expected<std::vector<Tarea>, DatabaseError> *>(vector);

    if (argc != 4) {
        *tasks = std::unexpected(DatabaseError::BadQuery);
        return 1;
    }

    auto task = parse_task(argv);

    if (!task.has_value()) {
        *tasks = std::unexpected(DatabaseError::InvalidTask);
        return 1;
    }

    tasks->value().push_back(*task);

    return 0;
}

std::optional<Tarea> Database::parse_task(char **row) {
    std::string title = row[0];

    std::optional<std::string> desc = row[1];
    if (strcmp(row[1], "NULL") == 0) {
        desc = std::nullopt;
    }

    std::istringstream stream(row[2]);
    std::optional<std::chrono::year_month_day> date;
    std::chrono::from_stream(stream, "%F", date.value());

    if (stream.fail() || !date.value().ok()) {
        date = std::nullopt;
    }

    bool completed = false;
    if (strcmp(row[3], "1") == 0) {
        completed = true;
    }

    return Tarea(title, desc, date, completed);
}

std::expected<std::vector<Tarea>, DatabaseError> Database::get_tasks() {
    if (!this->user.has_value()) {
        return std::unexpected(DatabaseError::NoUserLogged);
    }

    std::expected<std::vector<Tarea>, DatabaseError> tasks;

    char *error = nullptr;

    std::string query = "SELECT title, description, date, completed FROM tasks "
                        "WHERE user_id = " +
                        std::to_string(this->user.value()) + ";";

    int rc =
        sqlite3_exec(this->db, query.c_str(), parse_user_tasks, &tasks, &error);

    if (rc != SQLITE_OK) {
        sqlite3_free(error);
        return tasks;
    }

    return tasks;
}
