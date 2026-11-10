#pragma once
// Independent engine accessibility UI, separate from original Alpha screens.
#include <glm/glm.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace ui {

class ControllerKeyboard {
  public:
    enum class Action { Character, Shift, Space, Backspace, Done, Cancel };
    enum class Result { Editing, Done, Cancelled };
    struct Index {
        size_t row = 1, column = 0;
        bool operator==(const Index &) const = default;
    };
    struct Key {
        Action action;
        char16_t character = 0;
        std::u16string_view caption;
    };
    struct Rect {
        glm::vec2 origin{0}, size{0};
        bool contains(glm::vec2 point) const;
        glm::vec2 centre() const { return origin + size * 0.5f; }
    };

    void begin(std::u16string initial);
    const std::u16string &text() const { return draft; }
    bool shifted() const { return upper; }
    void type(char16_t character);
    void backspace();
    void shift() { upper = !upper; }
    void move(int x, int y); // one step in each supplied direction
    Index selected() const { return focus; }
    Result press(Index index);
    static constexpr size_t ROWS = 6;
    static size_t columns(size_t row);
    Key key(Index index) const;
    static Rect bounds(glm::vec2 window);
    static Rect keyBounds(glm::vec2 window, Index index);
    static std::optional<Index> hit(glm::vec2 window, glm::vec2 point);

  private:
    std::u16string draft;
    bool upper = false;
    Index focus;
};

} // namespace ui
