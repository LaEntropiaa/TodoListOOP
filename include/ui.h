#pragma once

#include "database.h"
#include "tarea.h"

enum class UIErrorType { Tarea, Database, Ok };

struct UIError {
    UIErrorType type;
    union {
        TareaErr tarea;
        DatabaseError db;
    };
};

class UI {
  private:
    InboxLista inbox;
    DueLista due;
    CompleteLista complete;
    TodoLista todo;
    Database db;

  public:
    UI();
    ~UI();

    int run(int argc, char **argv);
};
