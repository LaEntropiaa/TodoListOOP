#pragma once

#include "database.h"
#include "qtmetamacros.h"
#include "tarea.h"

#include <QFrame>
#include <QMainWindow>
#include <QMouseEvent>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class QScrollArea;
class QStackedWidget;
class QCheckBox;
class QDateEdit;
class QTextEdit;

class List : public QWidget {
    Q_OBJECT

  public:
    explicit List(const QString &title, QWidget *parent = nullptr);

    void clearTasks();
    void addTask(const Tarea &tarea);

    QWidget *taskContainer() const;

  private:
    QVBoxLayout *task_layout;
    QVBoxLayout *layout;
    QScrollArea *scroll;
    QPushButton *add_button;
    QString title;

    void createTaskWidget(const Tarea &tarea);

  signals:
    void taskClicked(int id);
    void addRequested();
};

class TaskWidget : public QFrame {
    Q_OBJECT

  public:
    explicit TaskWidget(int id, QWidget *parent = nullptr);

  signals:
    void clicked(int id);

  protected:
    void mousePressEvent(QMouseEvent *event) override;

  private:
    int id;
};

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(Database &database, QWidget *parent = nullptr);

  private:
    Database &db;

    QStackedWidget *pages;

    QWidget *login_page;
    QWidget *register_page;
    QWidget *task_page;

    QLineEdit *login_user;
    QLineEdit *login_password;

    QLineEdit *register_user;
    QLineEdit *register_password;
    QLineEdit *register_confirm;

    List *inbox_list;
    List *todo_list;
    List *due_list;
    List *completed_list;

    void setupLoginPage();
    void setupRegisterPage();
    void setupTaskPage();

    void login();
    void createUser();
    void logout();

    void reloadTasks();

    void showCreateTaskDialog();
    void showEditTaskDialog(int id);

    List *listForTask(const Tarea &tarea);

    static bool isOverdue(const Tarea &tarea);
    static bool isTodayOrFuture(const Tarea &tarea);

    static QString errorMessage(DatabaseError error);
    static QString errorMessage(TareaErr error);

    static QString formatDate(
        const std::optional<std::chrono::system_clock::time_point> &date);
};

class UI {
  private:
    Database db;

    CompleteLista complete;
    DueLista due;
    TodoLista todo;
    InboxLista inbox;

  public:
    UI();
    ~UI();

    int run(int argc, char **argv);
};
;
