#include "ui.h"
#include "tarea.h"

#include <cstdio>
#include <expected>
#include <iostream>
#include <optional>
#include <print>
#include <string>

// Inicio Luis el 08/09/2026

std::optional<std::string> UI::read_until(std::size_t limit) {
    std::string result;
    result.reserve(limit);

    std::getline(std::cin, result);

    if (result.size() < 1) {
        return std::nullopt;
    }

    if (result.size() > limit) {
        result.resize(limit);
    }

    return result;
}

std::optional<std::chrono::year_month_day> UI::parse_date(const std::string& text) {
    if (text.size() != Tarea::MAX_DATE || text[2] != '/' || text[5] != '/') {
        return std::nullopt;
    }

    for (size_t i = 0; i < text.size(); i++) {
        if (i == 2 || i == 5) {
            continue;
        }

        if (text[i] < '0' || text[i] > '9') {
            return std::nullopt;
        }
    }

    int day = (text[0] - '0') * 10 + (text[1] - '0');
    int month = (text[3] - '0') * 10 + (text[4] - '0');
    int year = 0;

    for (size_t i = 6; i < text.size(); i++) {
        year = year * 10 + (text[i] - '0');
    }

    return std::chrono::year_month_day{
        std::chrono::year{year},
        std::chrono::month{static_cast<unsigned>(month)},
        std::chrono::day{static_cast<unsigned>(day)}
    };
}

void UI::add_task_dialogue() {
    std::print("Ingrese el titulo de la tarea: ");
    auto title = read_until(Tarea::MAX_TITLE);

    if (!title.has_value()) {
        show_error(TareaErr::NullTitle);
        return;
    }

    std::print("Ingrese una descripcion (opcional): ");
    auto description = read_until(Tarea::MAX_DESC);

    std::print("Ingrese una fecha (opcional): ");
    auto date_text = read_until(Tarea::MAX_DATE);

    std::optional<std::chrono::year_month_day> date;

    if (date_text.has_value()) {
        date = parse_date(*date_text);

        if (!date.has_value()) {
            show_error(TareaErr::InvalidDate);
            return;
        }
    }

    Tarea nueva_tarea(title.value(), description, date, false);

    TareaErr error = list.add(nueva_tarea);

    if (error == TareaErr::Ok) {
        std::println("Tarea agregada exitosamente.");
    } else {
        show_error(error);
    }
}

void UI::remove_task_dialogue() {
    size_t index;

    std::print("Ingrese el numero de la tarea a eliminar: ");

    while (!(std::cin >> index)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::print("Entrada invalida, ingrese un numero: ");
    }
    std::cin.ignore();

    TareaErr error = list.remove(index);

    if (error == TareaErr::Ok) {
        std::println("Tarea eliminada exitosamente.");
    } else {
        show_error(error);
    }
}

void UI::update_task_dialogue() {
    size_t index;

    std::print("Ingrese el numero de la tarea a actualizar: ");

    while (!(std::cin >> index)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::print("Entrada invalida, ingrese un numero: ");
    }
    std::cin.ignore();

    std::print("Ingrese el nuevo titulo: ");
    auto title = read_until(Tarea::MAX_TITLE);

    if (!title.has_value()) {
        show_error(TareaErr::NullTitle);
        return;
    }

    std::print("Ingrese la nueva descripcion (opcional): ");
    auto description = read_until(Tarea::MAX_DESC);

    std::print("Ingrese la nueva fecha (opcional): ");
    auto date_text = read_until(Tarea::MAX_DATE);

    std::optional<std::chrono::year_month_day> date;

    if (date_text.has_value()) {
        date = parse_date(*date_text);

        if (!date.has_value()) {
            show_error(TareaErr::InvalidDate);
            return;
        }
    }
    
    Tarea nueva_tarea(title.value(), description, date, false);

    TareaErr error = list.add(nueva_tarea);

    if (error != TareaErr::Ok) {
        show_error(error);
        return;
    }

    error = list.remove(index);

    if (error == TareaErr::Ok) {
        std::println("Tarea actualizada exitosamente.");
    } else {
        show_error(error);
    }
}

void UI::complete_task_dialogue() {
    size_t index;

    std::print("Ingrese el numero de la tarea a marcar como completada: ");

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
        std::println("Tarea marcada como completada.");
    } else {
        show_error(resultado.error());
    }
}

void UI::clear_screen() {
    std::print("\033[2J\033[1;1H");
}

void UI::show_error(TareaErr e) {
    switch (e) {
    case TareaErr::InvalidOption:
        std::println(stderr, "Error: Opcion Invalida");
    case TareaErr::NullTitle:
        std::println(stderr, "Error: El titulo no puede estar vacio.");
        break;
    case TareaErr::OverSizeTitle:
        std::println(stderr, "Error: El titulo es demasiado largo.");
        break;

    case TareaErr::OverSizeDesc:
        std::println(stderr, "Error: La descripcion es demasiado larga.");
        break;

    case TareaErr::InvalidDate:
        std::println(stderr, "Error: La fecha ingresada no es valida.");
        break;

    case TareaErr::IndexErr:
        std::println(stderr, "Error: El indice de la tarea no existe.");
        break;

    case TareaErr::EmptyList:
        std::println(stderr, "Error: La lista esta vacia.");
        break;

    case TareaErr::NotOverdue:
        std::println(stderr, "Error: la tarea aun no esta vencida.");
        break;

    case TareaErr::AlreadyCompleted:
        std::println(stderr, "Error: no se puede agregar una tarea ya completada.");
        break;

    case TareaErr::Ok:
        break;
    }
}

void UI::print_list() {
    list.print();
}

void UI::read_option() {
    int option;

    std::println("Seleccione una opción:");
    std::println("1. Agregar tarea");
    std::println("2. Eliminar tarea\n");
    std::println("3. Actualizar tarea");
    std::println("4. Mostrar tareas\n");
    std::println("5. Marcar tarea como completada");
    std::println("6. Salir");

    while (!(std::cin >> option)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::print("Entrada invalida, ingrese un numero: ");
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
        show_error(TareaErr::InvalidOption);
        break;
    }
}

bool UI::is_running() {
    return is_on;
}

UI::UI() : is_on(true) {}

UI::~UI() {}
