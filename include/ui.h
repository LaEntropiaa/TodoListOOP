#include "database.h"
#include "tarea.h"

class UI {
  private:
    InboxLista list;
    bool is_on;
    // More lists for the future
    // Database interaction necessary

    void show_error(TareaErr e);
    void add_task_dialogue();
    void remove_task_dialogue();
    void update_task_dialogue();

  public:
    void print_list();
    void read_option();
    bool is_running();

    UI();
    ~UI();
};
