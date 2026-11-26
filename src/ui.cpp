#include "ui.h"
#include "database.h"
#include "qapplication.h"
#include "qpushbutton.h"
#include "tarea.h"

#include <QApplication>
#include <QPushButton>

UI::UI() {
    auto db_result = Database::open();

    if (!db_result.has_value()) {
        exit(1);
    }

    this->db = db_result.value();
    this->complete = CompleteLista();
    this->due = DueLista();
    this->todo = TodoLista();
    this->inbox = InboxLista();
}

UI::~UI() {
    this->db.close();
}

int UI::run(int argc, char **argv) {
    QApplication app(argc, argv);

    QPushButton button("Hola");
    button.resize(300, 100);

    button.show();

    return app.exec();
}
