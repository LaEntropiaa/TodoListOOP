#pragma once

#include "database.h"
#include "tarea.h"
#include <chrono>
#include <optional>
#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

class UI {
  private:
    InboxLista list;
    TodoLista todo;
    DueLista due;
    CompleteLista completed;
    // Database interaction necessary

    int pestana_actual = 0;
    std::vector<std::string> pestanas = {"Inbox", "Todo", "Due", "Completed"}; //variable de la lista que se mostrara despues en la salida
    ListaTarea& lista_actual();

    // Estado del dialogo para agregar tareas
    bool mostrar_dialogo = false;
    std::string nuevo_titulo;
    std::string nueva_descripcion;
    std::string nueva_fecha;
    std::string mensaje_error;

    // Decide en que lista cae la tarea y la agrega ahi
    TareaErr agregar_tarea(Tarea tarea);

    static std::optional<std::chrono::year_month_day>
    parse_date(const std::string &text);
    static std::string format_date(const std::chrono::year_month_day &date);

  public:
    void run();

    UI();
    ~UI();
};
