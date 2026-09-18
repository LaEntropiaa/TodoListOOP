#include "ui.h"

#include <cstdio>
#include <iostream>
#include <string>

// Inicio Luis el 08/09/2026

namespace {
    // Tamano fijo para el buffer de lectura, suficiente para el limite mas grande (descripcion).
    constexpr size_t READ_BUFFER_SIZE = TareaLimits::MAX_DESC_LEN + 2;

    // Lee una linea de stdin con fgets, sin permitir mas de max_len caracteres.
    // Si el usuario escribe de mas, el sobrante se descarta hasta el siguiente '\n'.
    std::string read_line_limited(size_t max_len) {
        char buffer[READ_BUFFER_SIZE];
        size_t capped_len = max_len < TareaLimits::MAX_DESC_LEN ? max_len : TareaLimits::MAX_DESC_LEN;
        size_t read_size = capped_len + 2; // +1 '\n', +1 '\0'

        if (!std::fgets(buffer, static_cast<int>(read_size), stdin)) {
            return "";
        }

        std::string result(buffer);

        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        } else {
            int c;
            while ((c = std::getchar()) != '\n' && c != EOF) {}
        }

        return result;
    }
}

void UI::add_task_dialogue() {
    std::cout << "Ingrese el titulo de la tarea: ";
    std::string title = read_line_limited(TareaLimits::MAX_TITLE_LEN);

    if (title.empty()) {
        std::cerr << "El título de la tarea no puede estar vacío.";
        return;
    }

    std::cout << "Ingrese una descripcion (opcional): ";
    std::string description = read_line_limited(TareaLimits::MAX_DESC_LEN);

    std::cout << "Ingrese una fecha (opcional): ";
    std::string date = read_line_limited(TareaLimits::DATE_LEN);

    std::optional<std::string> desc_opt;
    std::optional<std::string> date_opt;

    if (!description.empty()) {
        desc_opt = description;
    }

    if (!date.empty()) {
        date_opt = date;
    }

    Tarea nueva_tarea(
        title,
        desc_opt,
        date_opt,
        false
    );

    TareaErr error = list.add(nueva_tarea);

    if (error == TareaErr::None) {
        std::cout << "Tarea agregada exitosamente." << std::endl;
    } else {
        show_error(error);
    }
}

void UI::remove_task_dialogue() {
    size_t index;

    std::cout << "Ingrese el numero de la tarea a eliminar: ";

     while (!(std::cin >> index)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Entrada invalida, ingrese un numero: ";
    }
    std::cin.ignore();

    TareaErr error = list.remove(index);

    if (error == TareaErr::None) {
        std::cout << "Tarea eliminada exitosamente." << std::endl;
    } else {
        show_error(error);
    }
}

void UI::update_task_dialogue() {
    size_t index;

    std::cout << "Ingrese el numero de la tarea a actualizar: ";

     while (!(std::cin >> index)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Entrada invalida, ingrese un numero: ";
    }
    std::cin.ignore();

    std::cout << "Ingrese el nuevo titulo: ";
    std::string title = read_line_limited(TareaLimits::MAX_TITLE_LEN);

    if(title.empty()){
        std::cerr << "El título no puede estar vacío." << std::endl;
        return;
    }

    std::cout << "Ingrese la nueva descripcion (opcional): ";
    std::string description = read_line_limited(TareaLimits::MAX_DESC_LEN);

    std::cout << "Ingrese la nueva fecha (opcional): ";
    std::string date = read_line_limited(TareaLimits::DATE_LEN);

    std::optional<std::string> desc_opt;
    std::optional<std::string> date_opt;

    if (!description.empty()) {
        desc_opt = description;
    }

    if (!date.empty()) {
        date_opt = date;
    }

    Tarea nueva_tarea(title, desc_opt, date_opt, false);

    TareaErr error = list.add(nueva_tarea);

    if (error != TareaErr::None) {
        show_error(error);
        return;
    }

    error = list.remove(index);

    if(error == TareaErr::None) {
        std::cout << "Tarea actualizada exitosamente." << std::endl;
    } else{
        show_error(error);
    }
}

void UI::complete_task_dialogue() {
    size_t index;

    std::cout << "Ingrese el numero de la tarea a marcar como completada: ";

     while (!(std::cin >> index)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Entrada invalida, ingrese un numero: ";
    }
    std::cin.ignore();

    std::expected<Tarea, TareaErr> tarea = list.get_by_index(index);

    if (!tarea.has_value()) {
        show_error(tarea.error());
        return;
    }

    Tarea actualizada = tarea.value();
    actualizada.set_completed(true);

    std::expected<Tarea, TareaErr> resultado = list.edit(index, actualizada);

    if (resultado.has_value()) {
        std::cout << "Tarea marcada como completada." << std::endl;
    } else {
        show_error(resultado.error());
    }
}

void UI::clear_screen() {
    std::cout << "\033[2J\033[1;1H";
}

void UI::show_error(TareaErr e) {
    switch (e) {
        case TareaErr::OverSizeTitle:
            std::cerr << "Error: El titulo es demasiado largo." << std::endl;
            break;

        case TareaErr::OverSizeDesc:
            std::cerr << "Error: La descripcion es demasiado larga." << std::endl;
            break;

        case TareaErr::InvalidDate:
            std::cerr << "Error: La fecha ingresada no es valida." << std::endl;
            break;

        case TareaErr::IndexErr:
            std::cerr << "Error: El indice de la tarea no existe." << std::endl;
            break;

        case TareaErr::EmptyList:
            std::cerr << "Error: La lista esta vacia." << std::endl;
            break;

        case TareaErr::None:
            break;
    }
}

  void UI::print_list() {
      list.print();
  }

void UI::read_option() {
    int option;

    std::cout << "Seleccione una opción:\n";
    std::cout << "1. Agregar tarea\n";
    std::cout << "2. Eliminar tarea\n";
    std::cout << "3. Actualizar tarea\n";
    std::cout << "4. Mostrar tareas\n";
    std::cout << "5. Marcar tarea como completada\n";
    std::cout << "6. Salir\n";

    while (!(std::cin >> option)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Entrada invalida, ingrese un numero: ";
    }
    std::cin.ignore();

    switch (option) {
        case 1:
            add_task_dialogue();
            break;

        case 2:
            remove_task_dialogue();
            break;

        case 3:
            update_task_dialogue();
            break;

        case 4:
            print_list();
            break;

        case 5:
            complete_task_dialogue();
            break;

        case 6:
            is_on = false;
            break;

        default:
            std::cerr << "Opción inválida." << std::endl;
            break;
    }
}

bool UI::is_running() {
    return is_on;
}

UI::UI() : is_on(true) {
}

UI::~UI() {
}