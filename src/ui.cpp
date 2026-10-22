#include "ui.h"
#include "tarea.h"

#include <chrono>
#include <optional>
#include <string>

// Inicio Luis el 08/09/2026
//
std::string UI::format_date(const std::chrono::year_month_day &date) {
    unsigned day = static_cast<unsigned>(date.day());
    unsigned month = static_cast<unsigned>(date.month());
    int year = static_cast<int>(date.year());

    std::string result;

    if (day < 10)
        result += '0';
    result += std::to_string(day) + "/";

    if (month < 10)
        result += '0';
    result += std::to_string(month) + "/";

    result += std::to_string(year);

    return result;
}

std::optional<std::chrono::year_month_day>
UI::parse_date(const std::string &text) {
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
        std::chrono::day{static_cast<unsigned>(day)}};
}

UI::UI() {}

UI::~UI() {}

void UI::run() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    auto bar = ftxui::Toggle(&pestanas, &pestana_actual);

    auto view = ftxui::Renderer(bar, [&] {
        // Un texto por cada tarea de la lista de la pestaña activa
        ftxui::Elements lineas;
        ListaTarea& lista = lista_actual();

        for (size_t i = 0; i < lista.len(); i++) {
            Tarea tarea = lista.get_by_index(i).value();
            std::string linea = "• " + tarea.get_title();

            if (tarea.get_date().has_value()) {
                linea += "  (" + format_date(*tarea.get_date()) + ")";
            }

            lineas.push_back(ftxui::text(linea));
        }

        if (lineas.empty()) {
            lineas.push_back(ftxui::text("No hay tareas") | ftxui::dim);
        }

        return ftxui::vbox({
            bar->Render(),
            ftxui::separator(),
            ftxui::vbox(lineas) | ftxui::flex,
            ftxui::separator(),
            ftxui::text("a: agregar   ←/→: cambiar pestaña   q: salir") | ftxui::dim,
        }) | ftxui::border;
    });

    // ---- Dialogo para agregar tarea ----

    auto cerrar_dialogo = [&] {
        mostrar_dialogo = false;
    };

    auto guardar = [&] {
        if (nuevo_titulo.empty()) {
            mensaje_error = "El titulo no puede estar vacio.";
            return;
        }

        std::optional<std::chrono::year_month_day> fecha;
        if (!nueva_fecha.empty()) {
            fecha = parse_date(nueva_fecha);
            if (!fecha.has_value()) {
                mensaje_error = "Fecha invalida, usa dd/mm/aaaa.";
                return;
            }
        }

        std::optional<std::string> descripcion;
        if (!nueva_descripcion.empty()) {
            descripcion = nueva_descripcion;
        }

        TareaErr error = agregar_tarea(Tarea(nuevo_titulo, descripcion, fecha, false));

        if (error != TareaErr::Ok) {
            mensaje_error = "No se pudo agregar: revisa titulo, descripcion o fecha.";
            return;
        }

        mostrar_dialogo = false;
    };

    // Enter en cualquier caja guarda la tarea
    ftxui::InputOption opcion;
    opcion.multiline = false;
    opcion.on_enter = guardar;

    auto input_titulo = ftxui::Input(&nuevo_titulo, "obligatorio", opcion);
    auto input_descripcion = ftxui::Input(&nueva_descripcion, "opcional", opcion);
    auto input_fecha = ftxui::Input(&nueva_fecha, "dd/mm/aaaa (opcional)", opcion);

    auto botones = ftxui::Container::Horizontal({
        ftxui::Button("Guardar", guardar),
        ftxui::Button("Cancelar", cerrar_dialogo),
    });

    // Las flechas arriba/abajo se mueven entre estas piezas
    auto campos = ftxui::Container::Vertical({
        input_titulo,
        input_descripcion,
        input_fecha,
        botones,
    });

    auto dialogo = ftxui::Renderer(campos, [&] {
        return ftxui::vbox({
            ftxui::text("Nueva tarea") | ftxui::bold | ftxui::center,
            ftxui::separator(),
            ftxui::hbox({ftxui::text("Titulo:      "), input_titulo->Render()}),
            ftxui::hbox({ftxui::text("Descripcion: "), input_descripcion->Render()}),
            ftxui::hbox({ftxui::text("Fecha:       "), input_fecha->Render()}),
            ftxui::text(mensaje_error) | ftxui::color(ftxui::Color::Red),
            botones->Render() | ftxui::center,
        }) | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 50);
    });

    // Modal: dibuja el dialogo encima de la vista cuando mostrar_dialogo es true
    auto con_dialogo = ftxui::Modal(view, dialogo, &mostrar_dialogo);

    auto app = ftxui::CatchEvent(con_dialogo, [&](ftxui::Event event) {
        // Con el dialogo abierto, las teclas son para escribir (menos Esc)
        if (mostrar_dialogo) {
            if (event == ftxui::Event::Escape) {
                cerrar_dialogo();
                return true;
            }
            return false;
        }

        if (event == ftxui::Event::Character('q')) {
            screen.Exit();
            return true;
        }

        if (event == ftxui::Event::Character('a')) {
            nuevo_titulo.clear();
            nueva_descripcion.clear();
            nueva_fecha.clear();
            mensaje_error.clear();
            mostrar_dialogo = true;
            input_titulo->TakeFocus();
            return true;
        }

        return false;
    });

    screen.Loop(app);
}

ListaTarea& UI::lista_actual() {
    if (pestana_actual == 0) {
        return list;
    }
    if (pestana_actual == 1) {
        return todo;
    }
    if (pestana_actual == 2) {
        return due;
    }

    return completed;
}

TareaErr UI::agregar_tarea(Tarea tarea) {
    std::chrono::year_month_day hoy{
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};

    TareaErr error;
    int destino;

    if (tarea.get_is_completed()) {
        error = completed.add(tarea);
        destino = 3;
    } else if (!tarea.get_date().has_value()) {
        error = list.add(tarea);
        destino = 0;
    } else if (*tarea.get_date() < hoy) {
        error = due.add(tarea);
        destino = 2;
    } else {
        error = todo.add(tarea);
        destino = 1;
    }

    // Brincar a la pestaña donde quedo la tarea para que se vea
    if (error == TareaErr::Ok) {
        pestana_actual = destino;
    }

    return error;
}
