#include "database.h"
#include "ui.h"
#include <print>

int main(void) {
    UI ui = UI();

    auto db = Database::open();
    if (!db.has_value()) {
        switch (db.error()) {
        case DatabaseError::Ok:
            std::println("Oki??");
            break;
        case DatabaseError::BadVersion:
            std::println("bad version");
            break;
        case DatabaseError::CantOpen:
            std::println("couldnt open");
            break;
        case DatabaseError::Corrupted:
            std::println("Corrupted db");
            break;
        case DatabaseError::CreateSchema:
            std::println("schema not created");
            break;
        case DatabaseError::UserNotFound:
            std::println("no user??");
            break;
        }
    }

    // while (ui.is_running()) {
    //   ui.clear_screen();
    // ui.print_list();
    // ui.read_option();
    //}
    return 0;
}
