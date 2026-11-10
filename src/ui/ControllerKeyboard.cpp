#include "ui/ControllerKeyboard.h"
#include <algorithm>
#include <array>
#include <utility>

namespace ui {
namespace {
constexpr std::array<std::u16string_view, 5> letters{
    u"1234567890", u"qwertyuiop", u"asdfghjkl", u"zxcvbnm", u".,-_:/[]?'"
};
constexpr std::array<ControllerKeyboard::Key, 5> commands{{
    {ControllerKeyboard::Action::Shift, 0, u"Shift"},
    {ControllerKeyboard::Action::Space, 0, u"Space"},
    {ControllerKeyboard::Action::Backspace, 0, u"Backspace"},
    {ControllerKeyboard::Action::Done, 0, u"Done"},
    {ControllerKeyboard::Action::Cancel, 0, u"Cancel"}
}};
constexpr std::array<float, 5> widths{1.3f, 2.3f, 2.5f, 1.4f, 1.7f};
}

bool ControllerKeyboard::Rect::contains(glm::vec2 point) const {
    return size.x > 0 && size.y > 0 && point.x >= origin.x && point.y >= origin.y &&
           point.x < origin.x + size.x && point.y < origin.y + size.y;
}

void ControllerKeyboard::begin(std::u16string initial) {
    draft = std::move(initial);
    upper = false;
    focus = {};
}

void ControllerKeyboard::type(char16_t character) {
    if (character >= 32 && character != 127)
        draft.push_back(character);
}

void ControllerKeyboard::backspace() {
    if (!draft.empty())
        draft.pop_back();
}

size_t ControllerKeyboard::columns(size_t row) {
    return row < letters.size() ? letters[row].size() : row == letters.size() ? commands.size() : 0;
}

ControllerKeyboard::Key ControllerKeyboard::key(Index index) const {
    if (index.column >= columns(index.row))
        return {Action::Cancel, 0, {}};
    if (index.row == letters.size())
        return commands[index.column];
    char16_t character = letters[index.row][index.column];
    if (upper && index.row == 0)
        character = u"!@#$%^&*()"[index.column];
    else if (upper && character >= u'a' && character <= u'z')
        character = char16_t(character - (u'a' - u'A'));
    return {Action::Character, character, {}};
}

void ControllerKeyboard::move(int x, int y) {
    const int horizontal = (x > 0) - (x < 0), vertical = (y > 0) - (y < 0);
    focus.row = size_t((int(focus.row) + vertical + int(ROWS)) % int(ROWS));
    const auto count = columns(focus.row);
    focus.column = std::min(focus.column, count - 1);
    focus.column = size_t((int(focus.column) + horizontal + int(count)) % int(count));
}

ControllerKeyboard::Result ControllerKeyboard::press(Index index) {
    if (index.column >= columns(index.row))
        return Result::Editing;
    focus = index;
    const auto selected = key(index);
    switch (selected.action) {
    case Action::Character: type(selected.character); break;
    case Action::Shift: shift(); break;
    case Action::Space: type(u' '); break;
    case Action::Backspace: backspace(); break;
    case Action::Done: return Result::Done;
    case Action::Cancel: return Result::Cancelled;
    }
    return Result::Editing;
}

ControllerKeyboard::Rect ControllerKeyboard::bounds(glm::vec2 window) {
    if (!(window.x > 0 && window.y > 0))
        return {};
    const float scale = std::min({1.f, window.x / 800.f, window.y / 480.f});
    const glm::vec2 size = glm::vec2(760, 440) * scale;
    return {(window - size) * 0.5f, size};
}

ControllerKeyboard::Rect ControllerKeyboard::keyBounds(glm::vec2 window, Index index) {
    if (index.column >= columns(index.row))
        return {};
    const Rect panel = bounds(window);
    if (panel.size.x == 0)
        return {};
    const float scale = panel.size.x / 760.f, unit = 70.f * scale, gap = 4.f * scale;
    float total = float(columns(index.row)) * unit, offset = float(index.column) * unit, width = unit;
    if (index.row == letters.size()) {
        total = offset = 0;
        for (size_t i = 0; i < widths.size(); i++) {
            total += widths[i] * unit;
            if (i < index.column)
                offset += widths[i] * unit;
        }
        width = widths[index.column] * unit;
    }
    return {{panel.origin.x + (panel.size.x - total) * 0.5f + offset + gap * 0.5f,
             panel.origin.y + (110.f + float(index.row) * 46.f) * scale},
            {width - gap, 42.f * scale}};
}

std::optional<ControllerKeyboard::Index> ControllerKeyboard::hit(glm::vec2 window, glm::vec2 point) {
    for (size_t row = 0; row < ROWS; row++)
        for (size_t column = 0; column < columns(row); column++)
            if (keyBounds(window, {row, column}).contains(point))
                return Index{row, column};
    return std::nullopt;
}

} // namespace ui
