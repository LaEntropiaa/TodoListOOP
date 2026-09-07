#include <expected>
#include <optional>
#include <string>
#include <vector>

class Tarea {
  private:
    std::string title;
    std::optional<std::string> description;
    std::optional<std::string> date;
    bool is_completed;

  public:
    Tarea(std::string title,
          std::optional<std::string> description,
          std::optional<std::string> date,
          bool is_completed)
        : title(std::move(title)), description(std::move(description)),
          date(std::move(date)), is_completed(is_completed) {};
};

enum class TareaErr {
    OverSizeTitle,
    OverSizeDesc,
    InvalidDate,
    IndexErr,
    EmptyList,

    // TODO - functions to pass info for errors, specially for the UI
};

class ListaTarea {
  protected:
    std::vector<Tarea> tareas;

  public:
    TareaErr get_by_index(size_t index);
    TareaErr edit(size_t index);
    size_t len();
    ListaTarea();
    ~ListaTarea();

    virtual TareaErr remove(size_t index) = 0;
    virtual TareaErr add(Tarea) = 0;
    virtual std::expected<Tarea, TareaErr> get() = 0;
};

class InboxLista : public ListaTarea {
  public:
    TareaErr remove(size_t index) override;
    TareaErr add(Tarea) override;
    std::expected<Tarea, TareaErr> get() override;
};

class TodoLista : public ListaTarea {
  public:
    TareaErr remove(size_t index) override;
    TareaErr add(Tarea) override;
    std::expected<Tarea, TareaErr> get() override;
};

class DueLista : public ListaTarea {
  public:
    TareaErr remove(size_t index) override;
    TareaErr add(Tarea) override;
    std::expected<Tarea, TareaErr> get() override;
};

class CompleteLista : public ListaTarea {
  public:
    TareaErr remove(size_t index) override;
    TareaErr add(Tarea) override;
    std::expected<Tarea, TareaErr> get() override;
};
