#include "tarea.h"

#include <iostream>
#include <expected>

// Lista Tarea

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

// INBOXLISTA 

TareaErr InboxLista::add(Tarea tarea) {
    tareas.push_back(tarea);

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


void InboxLista::print() {
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