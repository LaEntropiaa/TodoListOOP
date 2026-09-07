#include "database.h"
#include "tarea.h"

class UI {
  private:
    InboxLista list;
    bool is_on;
    // More lists for the future
    // Database interaction necessary

    void show_error(TareaErr e);
    void add_list_dialogue();
    void remove_list_dialogue();
    void update_list_dialogue();

  public:
    void print_list();
    void read_option();
    bool is_running();

    UI();
    ~UI();
};
