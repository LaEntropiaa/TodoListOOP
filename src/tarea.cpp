#include "tarea.h"

#include <iostream>
#include <expected>

// Lista Tarea

namespace {
    bool is_digit(char c) {
        return c >= '0' && c <= '9';
    }

    // Espera exactamente el formato DD/MM/YYYY
    bool is_valid_date(const std::string& date) {
        if (date.size() != TareaLimits::DATE_LEN || date[2] != '/' || date[5] != '/') {
            return false;
        }

        for (size_t i = 0; i < date.size(); i++) {
            if (i == 2 || i == 5) {
                continue;
            }

            if (!is_digit(date[i])) {
                return false;
            }
        }

        int day = (date[0] - '0') * 10 + (date[1] - '0');
        int month = (date[3] - '0') * 10 + (date[4] - '0');

        return day >= 1 && day <= 31 && month >= 1 && month <= 12;
    }
}

TareaErr ListaTarea::validate(const Tarea& tarea) {
    if (tarea.get_title().size() > TareaLimits::MAX_TITLE_LEN) {
        return TareaErr::OverSizeTitle;
    }

    if (tarea.get_description().has_value() &&
        tarea.get_description()->size() > TareaLimits::MAX_DESC_LEN) {
        return TareaErr::OverSizeDesc;
    }

    if (tarea.get_date().has_value() && !is_valid_date(*tarea.get_date())) {
        return TareaErr::InvalidDate;
    }

    return TareaErr::None;
}

std::expected<Tarea, TareaErr> ListaTarea::get_by_index(size_t index) {
    if (tareas.empty()) {
        return std::unexpected(TareaErr::EmptyList);
    }

    if (index >= tareas.size()) {
        return std::unexpected(TareaErr::IndexErr);
    }

    return tareas[index];
}

std::expected<Tarea, TareaErr> ListaTarea::edit(size_t index, Tarea nueva) {
    if (tareas.empty()) {
        return std::unexpected(TareaErr::EmptyList);
    }

    if (index >= tareas.size()) {
        return std::unexpected(TareaErr::IndexErr);
    }

    tareas[index] = std::move(nueva);

    return tareas[index];
}

size_t ListaTarea::len() {
    return tareas.size();
}

void ListaTarea::print() {
    if (tareas.empty()) {
        std::cout << "No hay tareas." << std::endl;
        return;
    }

    for (size_t i = 0; i < tareas.size(); i++) {
        std::cout << i << ". "
                  << tareas[i].get_title();

        if (tareas[i].get_is_completed()) {
            std::cout << " [Completada]";
        } else {
            std::cout << " [Pendiente]";
        }

        std::cout << std::endl;
    }
}

// INBOXLISTA

TareaErr InboxLista::add(Tarea tarea) {
    TareaErr error = validate(tarea);

    if (error != TareaErr::None) {
        return error;
    }

    tareas.push_back(std::move(tarea));

    return TareaErr::None;
}


TareaErr InboxLista::remove(size_t index) {
    if (tareas.empty()) {
        return TareaErr::EmptyList;
    }

    if (index >= tareas.size()) {
        return TareaErr::IndexErr;
    }

    tareas.erase(tareas.begin() + index);

    return TareaErr::None;
}


std::expected<Tarea, TareaErr> InboxLista::get() {
    if (tareas.empty()) {
        return std::unexpected(TareaErr::EmptyList);
    }

    return tareas.front();
}