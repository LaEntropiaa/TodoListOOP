#pragma once

#include "database.h"
#include "tarea.h"
#include <chrono>
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
    bool is_on;
    int pestana_actual = 0;
    std::vector<std::string> pestanas = {"Inbox",  "Todo", "Due", "Completed" }; //variable de la lista que se mostrara despues en la salida
    ListaTarea& lista_actual (void);

    // Estado del dialogo para agregar tareas
    bool mostrar_dialogo = false;
    std::string nuevo_titulo;
    std::string nueva_descripcion;
    std::string nueva_fecha;
    std::string mensaje_error;

    // Decide en que lista cae la tarea y la agrega ahi
    TareaErr agregar_tarea(Tarea tarea);
  
  
  
    // More lists for the future
    // Database interaction necessary
    static std::optional<std::string> read_until(std::size_t limit);
    static std::optional<std::chrono::year_month_day>
    parse_date(const std::string &text);
    static void print(ListaTarea &lista);
    static std::string format_date(const std::chrono::year_month_day &date);
    void show_error(TareaErr e);
    void add_task_dialogue();
    void remove_task_dialogue();
    void update_task_dialogue();
    void complete_task_dialogue();

  public:
    void clear_screen();
    void print_list();
    void read_option();
    bool is_running();
    void run();

    UI();
    ~UI();
};
