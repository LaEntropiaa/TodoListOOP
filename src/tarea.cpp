#include "tarea.h"

#include <ctime>
#include <expected>
#include <iostream>

TareaErr ListaTarea::validate(const Tarea &tarea) {
    if (tarea.get_title().size() > Tarea::MAX_TITLE) {
        return TareaErr::OverSizeTitle;
    }

    if (tarea.get_description().has_value() &&
        tarea.get_description()->size() > Tarea::MAX_DESC) {
        return TareaErr::OverSizeDesc;
    }

    if (tarea.get_date().has_value() && !tarea.get_date()->ok()) {
        return TareaErr::InvalidDate;
    }

    return TareaErr::Ok;
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

    tareas[index] = nueva;

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
        std::cout << i << ". " << tareas[i].get_title();

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

    if (error != TareaErr::Ok) {
        return error;
    }

    tareas.push_back(tarea);

    return TareaErr::Ok;
}

TareaErr InboxLista::remove(size_t index) {
    if (tareas.empty()) {
        return TareaErr::EmptyList;
    }

    if (index >= tareas.size()) {
        return TareaErr::IndexErr;
    }

    tareas.erase(tareas.begin() + index);

    return TareaErr::Ok;
}

std::expected<Tarea, TareaErr> InboxLista::get() {
    if (tareas.empty()) {
        return std::unexpected(TareaErr::EmptyList);
    }

    return tareas.front();
}

// DUELISTA

std::chrono::year_month_day DueLista::today() {
    return std::chrono::year_month_day{
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())
    };
}

TareaErr DueLista::add(Tarea tarea) {
    if (!tarea.get_date().has_value()) {
        return TareaErr::InvalidDate;
    }
    
    TareaErr error = validate(tarea);

    if (error != TareaErr::Ok) {
        return error;
    }

    if (tarea.get_is_completed()) {
        return TareaErr::AlreadyCompleted;
    }

    std::chrono::year_month_day fecha = *tarea.get_date();

    if (fecha >= today()) {
        return TareaErr::NotOverdue;
    }

    size_t pos = 0;

    while (pos < tareas.size() && *tareas[pos].get_date() <= fecha) {
        pos++;
    }

    tareas.insert(tareas.begin() + pos, std::move(tarea));

    return TareaErr::Ok;
}

TareaErr DueLista::remove(size_t index) {
    if (tareas.empty()) {
        return TareaErr::EmptyList;
    }

    if (index >= tareas.size()) {
        return TareaErr::IndexErr;
    }

    tareas.erase(tareas.begin() + index);

    return TareaErr::Ok;
}

std::expected<Tarea, TareaErr> DueLista::get() {
    if (tareas.empty()) {
        return std::unexpected(TareaErr::EmptyList);
    }

    return tareas.front();
}