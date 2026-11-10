#include "ui/ControllerKeyboard.h"
#include "ui/Screens.h"
#include <array>
#include <cstdio>

namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}
void modelAndLayout() {
    using Keyboard = ui::ControllerKeyboard;
    Keyboard keyboard;
    keyboard.begin(u"a");
    keyboard.press({1, 0});
    keyboard.press({5, 0});
    keyboard.press({1, 0});
    check(keyboard.text() == u"aqQ" && keyboard.shifted(), "letter selection and shift edit only the draft");
    check(keyboard.key({0, 0}).character == u'!' && keyboard.key({0, 9}).character == u')', "shift exposes numeric-row symbols");
    keyboard.press({5, 1});
    check(keyboard.text() == u"aqQ ", "space command inserts a space");
    keyboard.press({5, 2});
    check(keyboard.text() == u"aqQ", "erase command removes one character");
    keyboard.type(0);
    keyboard.type(u'\n');
    check(keyboard.text() == u"aqQ", "control characters do not enter the draft");
    check(keyboard.press({5, 3}) == Keyboard::Result::Done && keyboard.press({5, 4}) == Keyboard::Result::Cancelled,
          "Done and Cancel return distinct actions without submitting anything");
    check(keyboard.press({100, 100}) == Keyboard::Result::Editing && keyboard.text() == u"aqQ", "invalid key leaves state intact");
    keyboard.begin({});
    keyboard.backspace();
    keyboard.move(-1, 0);
    check(keyboard.selected() == Keyboard::Index{1, 9}, "horizontal navigation wraps a row");
    keyboard.move(0, 1);
    check(keyboard.selected() == Keyboard::Index{2, 8}, "vertical navigation fits a shorter row");
    for (int step = 0; step < 100; step++) {
        keyboard.move(1, 1);
        const auto selected = keyboard.selected();
        check(selected.row < Keyboard::ROWS && selected.column < Keyboard::columns(selected.row), "focus stays on a valid key");
    }
    for (glm::vec2 window : {glm::vec2(1280, 720), glm::vec2(320, 180), glm::vec2(1920, 1080)}) {
        const auto panel = Keyboard::bounds(window);
        for (size_t row = 0; row < Keyboard::ROWS; row++)
            for (size_t column = 0; column < Keyboard::columns(row); column++) {
                const Keyboard::Index index{row, column};
                const auto key = Keyboard::keyBounds(window, index);
                check(key.size.x > 0 && key.size.y > 0 && panel.contains(key.origin) && panel.contains(key.origin + key.size - glm::vec2(0.01f)),
                      "keys fit within the scaled panel");
                check(Keyboard::hit(window, key.centre()) == index, "every rendered key centre selects exactly that key");
            }
        check(!Keyboard::hit(window, {-1, -1}) && !Keyboard::hit(window, panel.origin), "background and outside clicks select no key");
    }
    check(!Keyboard::hit({0, 0}, {0, 0}), "zero-sized window has no selectable keys");
}
void frontDrafts() {
    ui::Interface screen;
    screen.front = ui::Interface::Front::Creation;
    screen.nameText = u"Alpha";
    check(screen.openControllerKeyboard() && screen.controllerKeyboardOpen() && screen.wantsMouse(), "character editor opens a modal keyboard");
    screen.controllerKeyboardType(u'Z');
    check(screen.nameText == u"Alpha" && screen.controllerKeyboardState().text() == u"AlphaZ", "typing cannot change the underlying field before Done");
    check(screen.controllerBack() && !screen.controllerKeyboardOpen() && screen.front == ui::Interface::Front::Creation &&
          screen.nameText == u"Alpha", "Back cancels the draft before leaving creation");
    screen.openControllerKeyboard();
    const auto q = ui::ControllerKeyboard::keyBounds({1280, 720}, {1, 0}).centre();
    screen.controllerKeyboardPress({1280, 720}, q, 0);
    screen.finishControllerKeyboard(true);
    check(screen.nameText == u"Alphaq" && screen.frontRequest == ui::Interface::FrontRequest::None,
          "actual key hit commits the name without creating a character");
    screen.front = ui::Interface::Front::WorldCreation;
    screen.nameText = u"World";
    screen.seedText = u"41";
    screen.editFocus = 0;
    screen.openControllerKeyboard();
    screen.controllerKeyboardType(u'X');
    screen.editFocus = 1;
    check(!screen.finishControllerKeyboard(true) && screen.nameText == u"World" && screen.seedText == u"41",
          "changed edit target cancels a stale draft instead of writing another field");
    screen.openControllerKeyboard();
    screen.controllerKeyboardType(u'2');
    screen.finishControllerKeyboard(true);
    check(screen.seedText == u"412" && screen.nameText == u"World" && screen.frontRequest == ui::Interface::FrontRequest::None,
          "world seed commits separately without requesting a world save");
    screen.front = ui::Interface::Front::Connect;
    screen.serverText = u"localhost";
    screen.openControllerKeyboard();
    screen.controllerKeyboardType(u':');
    screen.controllerKeyboardType(u'1');
    screen.finishControllerKeyboard(true);
    check(screen.serverText == u"localhost:1" && screen.frontRequest == ui::Interface::FrontRequest::None,
          "server text commits without connecting");
    screen.openControllerKeyboard();
    screen.front = ui::Interface::Front::None;
    screen.controllerKeyboardType(u'Z');
    check(!screen.controllerKeyboardOpen() && screen.serverText == u"localhost:1", "removed editor invalidates its keyboard target");
}
void chatQueue() {
    ui::Interface screen;
    screen.chatOpen = true;
    screen.chatInput = u"draft";
    screen.openControllerKeyboard();
    screen.controllerKeyboardType(u'!');
    screen.finishControllerKeyboard(false);
    check(screen.chatOpen && screen.chatInput == u"draft" && screen.chatSubmitted.empty(), "chat cancel neither changes nor sends text");
    screen.openControllerKeyboard();
    screen.controllerKeyboardType(u'?');
    screen.finishControllerKeyboard(true);
    check(!screen.chatOpen && screen.chatInput.empty() && screen.chatSubmitted == std::vector<std::string>{"draft?"},
          "Send uses the existing local submission queue once");
    screen.finishControllerKeyboard(true);
    check(screen.chatSubmitted.size() == 1, "repeated Done cannot replay a submission");
    screen.menuRow = true;
    screen.hud = ui::HudState{};
    check(screen.canOpenControllerKeyboard() && screen.openControllerKeyboard() && !screen.menuRow && screen.chatOpen,
          "controller menu row opens chat without a keyboard Enter key");
    screen.controllerKeyboardType(u'h');
    screen.controllerKeyboardType(u'i');
    screen.finishControllerKeyboard(true);
    check(screen.chatSubmitted == std::vector<std::string>{"draft?", "hi"}, "controller-only chat reaches the same queue");
    screen.chatInput = u"\u03b1";
    screen.submitChat();
    check(screen.chatSubmitted.back() == "?", "shared submission retains the existing non-ASCII transport adapter");
}
}

int main() {
    modelAndLayout();
    frontDrafts();
    chatQueue();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
