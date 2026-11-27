#include "ui.h"
#include "database.h"
#include "qapplication.h"
#include "qboxlayout.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qmainwindow.h"
#include "qnamespace.h"
#include "qpushbutton.h"
#include "qwidget.h"
#include "tarea.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

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

    MainWindow window;

    window.show();

    return app.exec();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto *main_ly = new QVBoxLayout;

    auto *title = new QLabel("Todo");
    title->setAlignment(Qt::AlignCenter);

    auto *cards_ly = new QVBoxLayout;

    auto *card1 = new QWidget;
    auto *card2 = new QWidget;
    auto *card3 = new QWidget;
    auto *card4 = new QWidget;

    cards_ly->addWidget(card1);
    cards_ly->addWidget(card2);
    cards_ly->addWidget(card3);
    cards_ly->addWidget(card4);

    main_ly->addWidget(title);
    main_ly->addLayout(cards_ly, 1);

    auto *central = new QWidget;
    central->setLayout(main_ly);

    setCentralWidget(central);
}
