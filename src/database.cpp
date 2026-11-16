#include "database.h"
#include <cstring>
#include <expected>
#include <optional>
#include <sqlite3.h>

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
            due_date TEXT
        );

        CREATE TABLE passwords (
            id INTEGER PRIMARY KEY,
            user_id INTEGER NOT NULL UNIQUE,
            hash TEXT NOT NULL,
            salt TEXT NOT NULL
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
