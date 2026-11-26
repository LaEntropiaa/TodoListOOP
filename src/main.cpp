#include "database.h"
#include "ui.h"
#include <print>

int main(int argc, char **argv) {
    UI ui = UI();
    return ui.run(argc, argv);
}
