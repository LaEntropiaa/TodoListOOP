#include "ui.h"

#include <iostream>
#include <string>

// Inicio Luis el 08/09/2026

void UI::add_task_dialogue() {
    std::string title;
    std::string description;
    std::string date;

    std::cout << "Ingrese el titulo de la tarea: ";
    std::getline(std::cin, title);

    if (title.empty()) {
        std::cerr << "El título de la tarea no puede estar vacío.";
        return;
    }

    std::cout << "Ingrese una descripcion (opcional): ";
    std::getline(std::cin, description);

    std::cout << "Ingrese una fecha (opcional): ";
    std::getline(std::cin, date);

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
    std::string title;
    std::string description;
    std::string date;

    std::cout << "Ingrese el numero de la tarea a actualizar: ";

     while (!(std::cin >> index)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Entrada invalida, ingrese un numero: ";
    }
    std::cin.ignore();

    std::cout << "Ingrese el nuevo titulo: ";
    std::getline(std::cin, title);

    if(title.empty()){
        std::cerr << "El título no puede estar vacío." << std::endl;
        return;
    }

    std::cout << "Ingrese la nueva descripcion (opcional): ";
    std::getline(std::cin, description);

    std::cout << "Ingrese la nueva fecha (opcional): ";
    std::getline(std::cin, date);

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
    std::cout << "5. Salir\n";

    std::cin >> option;
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