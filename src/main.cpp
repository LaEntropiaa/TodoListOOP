#include "ui.h"

int main(void) {
    UI ui = UI();

    while (ui.is_running()) {
        ui.clear_screen();
        ui.print_list();
        ui.read_option();
    }
    return 0;
}
