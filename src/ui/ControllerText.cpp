#include "ui/Screens.h"
#include <algorithm>
#include <utility>

namespace ui {

void Interface::submitChat() {
    if (!chatInput.empty()) {
        std::string line;
        line.reserve(chatInput.size());
        for (char16_t character : chatInput)
            line += character < 0x80 ? char(character) : '?'; // existing engine transport adapter
        chatSubmitted.push_back(std::move(line));
    }
    chatInput.clear();
    chatOpen = false;
}

Interface::ControllerEdit Interface::controllerEdit() const {
    if (chatOpen)
        return ControllerEdit::Chat;
    switch (front) {
    case Front::Creation: return ControllerEdit::CharacterName;
    case Front::WorldCreation: return editFocus == 1 ? ControllerEdit::WorldSeed : ControllerEdit::WorldName;
    case Front::Connect: return ControllerEdit::Server;
    default: return ControllerEdit::None;
    }
}

bool Interface::canOpenControllerKeyboard() const {
    return controllerEdit() != ControllerEdit::None || (menuRow && hud && !startScreen && front == Front::None);
}

bool Interface::controllerKeyboardOpen() const {
    return keyboardTarget != ControllerEdit::None;
}

bool Interface::controllerKeyboardValid() const {
    return controllerKeyboardOpen() && controllerEdit() == keyboardTarget;
}

bool Interface::openControllerKeyboard() {
    if (controllerKeyboardOpen())
        return controllerKeyboardValid();
    if (!canOpenControllerKeyboard())
        return false;
    if (controllerEdit() == ControllerEdit::None) {
        menuRow = false;
        chatOpen = true;
    }
    keyboardTarget = controllerEdit();
    const std::u16string *initial = keyboardTarget == ControllerEdit::Chat ? &chatInput :
                                   keyboardTarget == ControllerEdit::Server ? &serverText :
                                   keyboardTarget == ControllerEdit::WorldSeed ? &seedText : &nameText;
    keyboard.begin(*initial);
    leftHeld = false;
    return true;
}

bool Interface::finishControllerKeyboard(bool accept) {
    const auto target = keyboardTarget;
    const bool valid = controllerKeyboardValid();
    keyboardTarget = ControllerEdit::None;
    if (!accept || !valid)
        return valid;
    switch (target) {
    case ControllerEdit::CharacterName: case ControllerEdit::WorldName: nameText = keyboard.text(); break;
    case ControllerEdit::WorldSeed: seedText = keyboard.text(); break;
    case ControllerEdit::Server: serverText = keyboard.text(); break;
    case ControllerEdit::Chat: chatInput = keyboard.text(); submitChat(); break;
    default: return false;
    }
    return true;
}

void Interface::controllerKeyboardType(char16_t character, bool backspace) {
    if (!controllerKeyboardValid()) {
        finishControllerKeyboard(false);
        return;
    }
    if (backspace)
        keyboard.backspace();
    else
        keyboard.type(character);
}

void Interface::controllerKeyboardMove(int x, int y) {
    if (controllerKeyboardValid())
        keyboard.move(x, y);
}

glm::vec2 Interface::controllerKeyboardFocus(glm::vec2 window) const {
    return ControllerKeyboard::keyBounds(window, keyboard.selected()).centre();
}

bool Interface::controllerKeyboardPress(glm::vec2 window, glm::vec2 mouse, int button) {
    if (!controllerKeyboardOpen())
        return false;
    if (!controllerKeyboardValid()) {
        finishControllerKeyboard(false);
        return true;
    }
    if (button == 1)
        keyboard.backspace();
    else if (button == 2)
        keyboard.shift();
    else if (button == 0)
        if (const auto key = ControllerKeyboard::hit(window, mouse)) {
            const auto result = keyboard.press(*key);
            if (result != ControllerKeyboard::Result::Editing)
                finishControllerKeyboard(result == ControllerKeyboard::Result::Done);
        }
    return true;
}

void Interface::drawControllerKeyboard(Painter &painter, glm::vec2 window, glm::vec2 mouse) {
    if (!controllerKeyboardValid()) {
        finishControllerKeyboard(false);
        return;
    }
    const auto panel = ControllerKeyboard::bounds(window);
    if (panel.size.x == 0)
        return;
    const float scale = panel.size.x / 760.f;
    painter.rectangle({0, 0}, window, {0, 0, 0, 0.62f});
    painter.rectangle(panel.origin, panel.origin + panel.size, {0.035f, 0.05f, 0.08f, 1});
    const std::u16string title = keyboardTarget == ControllerEdit::Chat ? u"Chat" :
                                keyboardTarget == ControllerEdit::Server ? u"Server address" :
                                keyboardTarget == ControllerEdit::WorldSeed ? u"World seed" :
                                keyboardTarget == ControllerEdit::WorldName ? u"World name" : u"Character name";
    TextStyle text;
    text.pixelSize = 20.f * scale;
    text.outline = scale;
    painter.text(title, panel.origin + glm::vec2(24, 32) * scale, text);
    const glm::vec2 field = panel.origin + glm::vec2(20, 50) * scale;
    painter.rectangle(field, field + glm::vec2(720, 42) * scale, {0.01f, 0.015f, 0.025f, 1});
    // Fit a suffix without changing the draft or wrapping it across the keys.
    // Binary search avoids repeated front erasure for long text.
    std::u16string visible = keyboard.text();
    const float available = 692.f * scale;
    if (painter.textWidth(visible, text) > available) {
        size_t first = 0, last = visible.size();
        while (first < last) {
            const size_t middle = first + (last - first) / 2;
            if (painter.textWidth(visible.substr(middle), text) <= available)
                last = middle;
            else
                first = middle + 1;
        }
        visible.erase(0, first);
    }
    painter.text(visible, field + glm::vec2(10, 29) * scale, text);
    const auto hovered = ControllerKeyboard::hit(window, mouse);
    text.flags = plasma::CENTRE;
    for (size_t row = 0; row < ControllerKeyboard::ROWS; row++)
        for (size_t column = 0; column < ControllerKeyboard::columns(row); column++) {
            const ControllerKeyboard::Index index{row, column};
            const auto rect = ControllerKeyboard::keyBounds(window, index);
            const auto key = keyboard.key(index);
            const bool selected = hovered ? *hovered == index : keyboard.selected() == index;
            const bool shift = key.action == ControllerKeyboard::Action::Shift && keyboard.shifted();
            painter.rectangle(rect.origin, rect.origin + rect.size, selected ? glm::vec4(0.12f, 0.32f, 0.52f, 1) :
                              shift ? glm::vec4(0.25f, 0.28f, 0.12f, 1) : glm::vec4(0.12f, 0.15f, 0.2f, 1));
            const std::u16string label = key.action == ControllerKeyboard::Action::Character ? std::u16string(1, key.character) :
                                        key.action == ControllerKeyboard::Action::Done && keyboardTarget == ControllerEdit::Chat ?
                                        u"Send" : std::u16string(key.caption);
            painter.text(label, rect.origin + glm::vec2(rect.size.x * 0.5f, 29.f * scale), text);
        }
    text.pixelSize = 12.f * scale;
    painter.text(u"A Select   X Erase   Y Shift   B Cancel   Stick press Done", panel.origin + glm::vec2(380, 418) * scale, text);
}

} // namespace ui
