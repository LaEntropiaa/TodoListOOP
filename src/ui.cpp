#include "ui.h"

#include <QApplication>
#include <QCheckBox>
#include <QDate>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include <chrono>
#include <fstream>

UI::UI() {
    auto db_result = Database::open();

    if (!db_result.has_value()) {
        std::exit(1);
    }

    this->db = std::move(db_result.value());

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

    QFile file("styles/main.qss");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(file.readAll());
    }

    MainWindow window(this->db);
    window.show();

    return app.exec();
}

List::List(const QString &title, QWidget *parent)
    : QWidget(parent), title(title) {

    setProperty("listType", title);

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title_label = new QLabel(title);
    title_label->setObjectName("list-title");

    scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget;

    task_layout = new QVBoxLayout(container);
    task_layout->setContentsMargins(0, 0, 0, 0);
    task_layout->setSpacing(8);

    task_layout->addStretch(1);

    scroll->setWidget(container);

    layout->addWidget(title_label);
    layout->addWidget(scroll, 1);

    if (title == "Inbox") {
        add_button = new QPushButton("+ Agregar tarea");
        add_button->setObjectName("add-task");

        layout->addWidget(add_button);

        connect(add_button, &QPushButton::clicked, this, &List::addRequested);
    } else {
        add_button = nullptr;
    }
}

void List::clearTasks() {
    while (task_layout->count() > 1) {
        QLayoutItem *item = task_layout->takeAt(0);

        if (item->widget()) {
            item->widget()->deleteLater();
        }

        delete item;
    }
}

void List::addTask(const Tarea &tarea) {
    createTaskWidget(tarea);
}

QWidget *List::taskContainer() const {
    return scroll->widget();
}

void List::createTaskWidget(const Tarea &tarea) {
    if (!tarea.get_id().has_value()) {
        return;
    }

    auto *button = new QPushButton;
    button->setObjectName("task");

    auto *layout = new QVBoxLayout(button);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(4);

    auto *title_label = new QLabel(QString::fromStdString(tarea.get_title()));

    title_label->setObjectName("task-title");

    layout->addWidget(title_label);

    if (tarea.get_description().has_value() &&
        !tarea.get_description()->empty()) {

        auto *description =
            new QLabel(QString::fromStdString(*tarea.get_description()));

        description->setObjectName("task-description");
        description->setWordWrap(true);

        layout->addWidget(description);
    }

    if (tarea.get_date().has_value()) {
        auto date = tarea.get_date().value();

        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                 date.time_since_epoch())
                                 .count();

        auto qdate = QDateTime::fromSecsSinceEpoch(seconds).date();

        auto *date_label = new QLabel(qdate.toString("dd/MM/yyyy"));

        date_label->setObjectName("task-date");

        layout->addWidget(date_label);
    }

    const int id = tarea.get_id().value();

    task_layout->insertWidget(task_layout->count() - 1, button);

    connect(button, &QPushButton::clicked, this, [this, id]() {
        emit taskClicked(id);
    });
}

MainWindow::MainWindow(Database &database, QWidget *parent)
    : QMainWindow(parent), db(database) {

    resize(1200, 700);

    pages = new QStackedWidget;

    setupLoginPage();
    setupRegisterPage();
    setupTaskPage();

    pages->setCurrentWidget(login_page);

    setCentralWidget(pages);
}

void MainWindow::setupLoginPage() {
    login_page = new QWidget;

    auto *outer = new QVBoxLayout(login_page);

    outer->setContentsMargins(0, 0, 0, 0);

    auto *form = new QVBoxLayout;

    auto *title = new QLabel("Todo");
    title->setObjectName("login-title");
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel("Iniciar sesión");
    subtitle->setObjectName("login-subtitle");
    subtitle->setAlignment(Qt::AlignCenter);

    login_user = new QLineEdit;
    login_user->setPlaceholderText("Usuario");

    login_password = new QLineEdit;
    login_password->setPlaceholderText("Contraseña");
    login_password->setEchoMode(QLineEdit::Password);

    auto *login_button = new QPushButton("Iniciar sesión");
    login_button->setObjectName("primary-button");

    auto *register_button = new QPushButton("Crear cuenta");

    form->addWidget(title);
    form->addWidget(subtitle);

    form->addSpacing(20);

    form->addWidget(login_user);
    form->addWidget(login_password);

    form->addSpacing(10);

    form->addWidget(login_button);
    form->addWidget(register_button);

    auto *container = new QWidget;
    container->setObjectName("login-container");
    container->setLayout(form);

    outer->addStretch();
    outer->addWidget(container, 0, Qt::AlignCenter);
    outer->addStretch();

    pages->addWidget(login_page);

    connect(login_button, &QPushButton::clicked, this, &MainWindow::login);

    connect(register_button, &QPushButton::clicked, this, [this]() {
        register_user->clear();
        register_password->clear();
        register_confirm->clear();

        pages->setCurrentWidget(register_page);
    });
}

void MainWindow::setupRegisterPage() {
    register_page = new QWidget;

    auto *outer = new QVBoxLayout(register_page);

    outer->setContentsMargins(0, 0, 0, 0);

    auto *form = new QVBoxLayout;

    auto *title = new QLabel("Crear cuenta");
    title->setObjectName("login-title");
    title->setAlignment(Qt::AlignCenter);

    register_user = new QLineEdit;
    register_user->setPlaceholderText("Usuario");

    register_password = new QLineEdit;
    register_password->setPlaceholderText("Contraseña");
    register_password->setEchoMode(QLineEdit::Password);

    register_confirm = new QLineEdit;
    register_confirm->setPlaceholderText("Confirmar contraseña");
    register_confirm->setEchoMode(QLineEdit::Password);

    auto *create_button = new QPushButton("Crear cuenta");

    create_button->setObjectName("primary-button");

    auto *back_button = new QPushButton("Volver");

    form->addWidget(title);

    form->addSpacing(20);

    form->addWidget(register_user);
    form->addWidget(register_password);
    form->addWidget(register_confirm);

    form->addSpacing(10);

    form->addWidget(create_button);
    form->addWidget(back_button);

    auto *container = new QWidget;
    container->setObjectName("login-container");
    container->setLayout(form);

    outer->addStretch();
    outer->addWidget(container, 0, Qt::AlignCenter);
    outer->addStretch();

    pages->addWidget(register_page);

    connect(create_button,
            &QPushButton::clicked,
            this,
            &MainWindow::createUser);

    connect(back_button, &QPushButton::clicked, this, [this]() {
        pages->setCurrentWidget(login_page);
    });
}

void MainWindow::login() {
    const QString user = login_user->text().trimmed();
    const QString password = login_password->text();

    if (user.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this,
                             "Iniciar sesión",
                             "Debes introducir usuario y contraseña.");

        return;
    }

    const auto result = db.login(user.toStdString(), password.toStdString());

    if (result != DatabaseError::Ok) {
        QMessageBox::critical(this,
                              "Error al iniciar sesión",
                              errorMessage(result));

        return;
    }

    login_password->clear();

    reloadTasks();

    pages->setCurrentWidget(task_page);
}

void MainWindow::createUser() {
    const QString user = register_user->text().trimmed();
    const QString password = register_password->text();
    const QString confirm = register_confirm->text();

    if (user.isEmpty()) {
        QMessageBox::warning(this,
                             "Crear cuenta",
                             "El usuario no puede estar vacío.");

        return;
    }

    if (password.isEmpty()) {
        QMessageBox::warning(this,
                             "Crear cuenta",
                             "La contraseña no puede estar vacía.");

        return;
    }

    if (password != confirm) {
        QMessageBox::warning(this,
                             "Crear cuenta",
                             "Las contraseñas no coinciden.");

        return;
    }

    const auto result =
        db.create_user(user.toStdString(), password.toStdString());

    if (result != DatabaseError::Ok) {
        QMessageBox::critical(this,
                              "Error al crear la cuenta",
                              errorMessage(result));

        return;
    }

    QMessageBox::information(this,
                             "Cuenta creada",
                             "La cuenta se creó correctamente.");

    register_user->clear();
    register_password->clear();
    register_confirm->clear();

    pages->setCurrentWidget(login_page);
}

void MainWindow::setupTaskPage() {
    task_page = new QWidget;

    auto *main_layout = new QVBoxLayout(task_page);

    main_layout->setContentsMargins(16, 16, 16, 16);
    main_layout->setSpacing(12);

    auto *header = new QHBoxLayout;

    auto *title = new QLabel("Todo");
    title->setObjectName("main-title");

    auto *logout_button = new QPushButton("Cerrar sesión");
    logout_button->setObjectName("logout-button");

    header->addWidget(title);
    header->addStretch();
    header->addWidget(logout_button);

    auto *lists = new QHBoxLayout;
    lists->setSpacing(12);

    inbox_list = new List("Inbox");
    todo_list = new List("Todo");
    due_list = new List("Due");
    completed_list = new List("Completed");

    lists->addWidget(inbox_list, 1);
    lists->addWidget(todo_list, 1);
    lists->addWidget(due_list, 1);
    lists->addWidget(completed_list, 1);

    main_layout->addLayout(header);
    main_layout->addLayout(lists, 1);

    pages->addWidget(task_page);

    connect(logout_button, &QPushButton::clicked, this, &MainWindow::logout);

    connect(inbox_list,
            &List::addRequested,
            this,
            &MainWindow::showCreateTaskDialog);

    connect(inbox_list,
            &List::taskClicked,
            this,
            &MainWindow::showEditTaskDialog);

    connect(todo_list,
            &List::taskClicked,
            this,
            &MainWindow::showEditTaskDialog);

    connect(due_list,
            &List::taskClicked,
            this,
            &MainWindow::showEditTaskDialog);

    connect(completed_list,
            &List::taskClicked,
            this,
            &MainWindow::showEditTaskDialog);
}

void MainWindow::logout() {
    pages->setCurrentWidget(login_page);
    login_user->clear();
    login_password->clear();
}

void MainWindow::reloadTasks() {
    inbox_list->clearTasks();
    todo_list->clearTasks();
    due_list->clearTasks();
    completed_list->clearTasks();

    const auto result = db.get_tasks();

    if (!result.has_value()) {
        QMessageBox::critical(this, "Error", errorMessage(result.error()));

        return;
    }

    for (const auto &task : result.value()) {
        List *list = listForTask(task);

        if (list != nullptr) {
            list->addTask(task);
        }
    }
}

List *MainWindow::listForTask(const Tarea &tarea) {
    if (tarea.get_is_completed()) {
        return completed_list;
    }

    if (!tarea.get_date().has_value()) {
        return inbox_list;
    }

    if (isOverdue(tarea)) {
        return due_list;
    }

    return todo_list;
}

bool MainWindow::isOverdue(const Tarea &tarea) {
    if (!tarea.get_date().has_value()) {
        return false;
    }

    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                             tarea.get_date()->time_since_epoch())
                             .count();

    const QDate date = QDateTime::fromSecsSinceEpoch(seconds).date();

    return date < QDate::currentDate();
}

bool MainWindow::isTodayOrFuture(const Tarea &tarea) {
    if (!tarea.get_date().has_value()) {
        return false;
    }

    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                             tarea.get_date()->time_since_epoch())
                             .count();

    const QDate date = QDateTime::fromSecsSinceEpoch(seconds).date();

    return date >= QDate::currentDate();
}

void MainWindow::showCreateTaskDialog() {
    QDialog dialog(this);

    dialog.setWindowTitle("Nueva tarea");
    dialog.setModal(true);

    auto *layout = new QVBoxLayout(&dialog);

    auto *title = new QLineEdit;
    title->setPlaceholderText("Título");

    auto *description = new QTextEdit;
    description->setPlaceholderText("Descripción");

    auto *has_date = new QCheckBox("Tiene fecha de vencimiento");

    auto *date = new QDateEdit(QDate::currentDate());
    date->setCalendarPopup(true);
    date->setDisplayFormat("dd/MM/yyyy");
    date->setEnabled(false);

    auto *buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    layout->addWidget(new QLabel("Título"));
    layout->addWidget(title);

    layout->addWidget(new QLabel("Descripción"));
    layout->addWidget(description);

    layout->addWidget(has_date);
    layout->addWidget(date);
    layout->addWidget(buttons);

    connect(has_date, &QCheckBox::toggled, date, &QDateEdit::setEnabled);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString title_text = title->text().trimmed();

    if (title_text.isEmpty()) {
        QMessageBox::warning(this,
                             "Nueva tarea",
                             "El título no puede estar vacío.");

        return;
    }

    if (title_text.size() > static_cast<int>(Tarea::MAX_TITLE)) {
        QMessageBox::warning(this,
                             "Nueva tarea",
                             "El título es demasiado largo.");

        return;
    }

    const QString description_text = description->toPlainText();

    if (description_text.size() > static_cast<int>(Tarea::MAX_DESC)) {

        QMessageBox::warning(this,
                             "Nueva tarea",
                             "La descripción es demasiado larga.");

        return;
    }

    std::optional<std::string> description_value;

    if (!description_text.isEmpty()) {
        description_value = description_text.toStdString();
    }

    std::optional<std::chrono::system_clock::time_point> date_value;

    if (has_date->isChecked()) {
        const auto seconds = date->date().startOfDay().toSecsSinceEpoch();

        date_value = std::chrono::system_clock::time_point(
            std::chrono::seconds(seconds));
    }

    Tarea task(title_text.toStdString(),
               description_value,
               date_value,
               std::nullopt,
               false);

    const auto result = db.insert_task(task);

    if (result != DatabaseError::Ok) {
        QMessageBox::critical(this,
                              "Error al crear tarea",
                              errorMessage(result));

        return;
    }

    reloadTasks();
}

void MainWindow::showEditTaskDialog(int id) {
    const auto result = db.get_tasks();

    if (!result.has_value()) {
        QMessageBox::critical(this, "Error", errorMessage(result.error()));

        return;
    }

    std::optional<Tarea> current;

    for (const auto &task : result.value()) {
        if (task.get_id().has_value() && task.get_id().value() == id) {

            current = task;
            break;
        }
    }

    if (!current.has_value()) {
        QMessageBox::critical(this, "Error", "No se encontró la tarea.");

        return;
    }

    QDialog dialog(this);

    dialog.setWindowTitle("Editar tarea");
    dialog.setModal(true);

    auto *layout = new QVBoxLayout(&dialog);

    auto *title = new QLineEdit(QString::fromStdString(current->get_title()));

    auto *description = new QTextEdit;

    if (current->get_description().has_value()) {
        description->setPlainText(
            QString::fromStdString(current->get_description().value()));
    }

    auto *has_date = new QCheckBox("Tiene fecha de vencimiento");

    auto *date = new QDateEdit(QDate::currentDate());

    date->setCalendarPopup(true);
    date->setDisplayFormat("dd/MM/yyyy");

    if (current->get_date().has_value()) {
        has_date->setChecked(true);

        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                 current->get_date()->time_since_epoch())
                                 .count();

        date->setDate(QDateTime::fromSecsSinceEpoch(seconds).date());
    } else {
        date->setEnabled(false);
    }

    auto *completed = new QCheckBox("Completada");

    completed->setChecked(current->get_is_completed());

    auto *buttons =
        new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);

    layout->addWidget(new QLabel("Título"));
    layout->addWidget(title);

    layout->addWidget(new QLabel("Descripción"));
    layout->addWidget(description);

    layout->addWidget(has_date);
    layout->addWidget(date);

    layout->addWidget(completed);

    layout->addWidget(buttons);

    connect(has_date, &QCheckBox::toggled, date, &QDateEdit::setEnabled);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString title_text = title->text().trimmed();

    if (title_text.isEmpty()) {
        QMessageBox::warning(this,
                             "Editar tarea",
                             "El título no puede estar vacío.");

        return;
    }

    if (title_text.size() > static_cast<int>(Tarea::MAX_TITLE)) {

        QMessageBox::warning(this,
                             "Editar tarea",
                             "El título es demasiado largo.");

        return;
    }

    const QString description_text = description->toPlainText();

    if (description_text.size() > static_cast<int>(Tarea::MAX_DESC)) {

        QMessageBox::warning(this,
                             "Editar tarea",
                             "La descripción es demasiado larga.");

        return;
    }

    std::optional<std::string> description_value;

    if (!description_text.isEmpty()) {
        description_value = description_text.toStdString();
    }

    std::optional<std::chrono::system_clock::time_point> date_value;

    if (has_date->isChecked()) {
        const auto seconds = date->date().startOfDay().toSecsSinceEpoch();

        date_value = std::chrono::system_clock::time_point(
            std::chrono::seconds(seconds));
    }

    Tarea new_task(title_text.toStdString(),
                   description_value,
                   date_value,
                   current->get_id(),
                   completed->isChecked());

    const auto update_result = db.update_task(current->get_title(), new_task);

    if (update_result != DatabaseError::Ok) {
        QMessageBox::critical(this,
                              "Error al actualizar",
                              errorMessage(update_result));

        return;
    }

    reloadTasks();
}

QString MainWindow::errorMessage(DatabaseError error) {
    switch (error) {
    case DatabaseError::Ok:
        return "Operación realizada correctamente.";

    case DatabaseError::CantOpen:
        return "No se pudo abrir la base de datos.";

    case DatabaseError::UserNotFound:
        return "El usuario no existe.";

    case DatabaseError::PasswordNotFound:
        return "No se encontró la contraseña del usuario.";

    case DatabaseError::Corrupted:
        return "La base de datos está dañada.";

    case DatabaseError::CreateSchema:
        return "No se pudo crear el esquema de la base de datos.";

    case DatabaseError::BadVersion:
        return "La versión de la base de datos no es compatible.";

    case DatabaseError::IncorrectPassword:
        return "La contraseña es incorrecta.";

    case DatabaseError::NotUniqueUserName:
        return "Ese nombre de usuario ya está en uso.";

    case DatabaseError::FailedToSavePassword:
        return "No se pudo guardar la contraseña.";

    case DatabaseError::NoUserLogged:
        return "No hay ningún usuario iniciado.";

    case DatabaseError::FailedTaskInsert:
        return "No se pudo guardar la tarea.";

    case DatabaseError::BadQuery:
        return "No se pudo ejecutar la consulta.";

    case DatabaseError::InvalidTask:
        return "La tarea no es válida.";

    case DatabaseError::TaskAlreadyExists:
        return "Ya existe una tarea con ese título.";

    case DatabaseError::FailedTaskUpdate:
        return "No se pudo actualizar la tarea.";

    case DatabaseError::FailedTaskDelete:
        return "No se pudo eliminar la tarea.";
    }

    return "Ocurrió un error desconocido.";
}

QString MainWindow::errorMessage(TareaErr error) {
    switch (error) {
    case TareaErr::Ok:
        return "Operación realizada correctamente.";

    case TareaErr::NullTitle:
        return "El título no puede estar vacío.";

    case TareaErr::OverSizeTitle:
        return "El título supera el límite permitido.";

    case TareaErr::OverSizeDesc:
        return "La descripción supera el límite permitido.";

    case TareaErr::InvalidDate:
        return "La fecha no es válida.";

    case TareaErr::IndexErr:
        return "El índice de la tarea no es válido.";

    case TareaErr::EmptyList:
        return "La lista de tareas está vacía.";

    case TareaErr::InvalidOption:
        return "La opción seleccionada no es válida.";

    case TareaErr::NotOverdue:
        return "La tarea todavía no está vencida.";

    case TareaErr::Overdue:
        return "La tarea está vencida.";

    case TareaErr::AlreadyCompleted:
        return "La tarea ya está completada.";

    case TareaErr::NotCompleted:
        return "La tarea no está completada.";
    }

    return "Ocurrió un error desconocido.";
}

QString MainWindow::formatDate(
    const std::optional<std::chrono::system_clock::time_point> &date) {
    if (!date.has_value()) {
        return "Sin fecha";
    }

    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                             date->time_since_epoch())
                             .count();

    return QDateTime::fromSecsSinceEpoch(seconds).date().toString("dd/MM/yyyy");
}
