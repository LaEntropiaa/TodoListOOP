#pragma once

#include "database.h"
#include "tarea.h"

class UI {
  private:
    InboxLista inbox;
    DueLista due;
    CompleteLista complete;
    TodoLista todo;
    Database db;

  public:
};
