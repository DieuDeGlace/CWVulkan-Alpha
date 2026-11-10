// Built-in controller support (see Gamepad.h).
#include "Gamepad.h"
#include "Config.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

void Gamepad::load(const Config &config) {
    enabled = config.number("Controller", "Enabled", 1) != 0;
    deadZone = std::clamp((float)config.number("Controller", "DeadZone", 20) * 0.01f, 0.f, 0.9f); // percent
    triggerDeadZone = std::clamp((float)config.number("Controller", "TriggerDeadZone", 5) * 0.01f, 0.f, 0.9f);
    lookSpeed = std::max(0.f, (float)config.number("Controller", "LookSpeed", (int)lookSpeed));
    cursorSpeed = std::max(0.f, (float)config.number("Controller", "CursorSpeed", (int)cursorSpeed));
    invertY = config.number("Controller", "InvertY", 0) != 0;
}

namespace {

// A stick past the dead zone, rescaled so its travel starts at 0 again.
glm::vec2 stick(float x, float y, float dead) {
    if (!std::isfinite(x) || !std::isfinite(y))
        return glm::vec2(0);
    dead = std::isfinite(dead) ? std::clamp(dead, 0.f, 0.9f) : 0.2f;
    const glm::vec2 v(std::clamp(x, -1.f, 1.f), -std::clamp(y, -1.f, 1.f)); // GLFW y points down
    const float length = glm::length(v);
    if (length <= dead)
        return glm::vec2(0);
    return v / length * std::min((length - dead) / (1 - dead), 1.f);
}

float trigger(float raw, float dead) {
    if (!std::isfinite(raw))
        return 0;
    dead = std::isfinite(dead) ? std::clamp(dead, 0.f, 0.9f) : 0.05f;
    const float value = (std::clamp(raw, -1.f, 1.f) + 1) * 0.5f;
    return value <= dead ? 0.f : (value - dead) / (1 - dead);
}

} // namespace

void Gamepad::poll() {
    static_assert(COUNT == GLFW_GAMEPAD_BUTTON_LAST + 1 && AXIS_COUNT == GLFW_GAMEPAD_AXIS_LAST + 1);
    static_assert(DEVICE_COUNT == GLFW_JOYSTICK_LAST + 1);
    poll([](int jid, Sample &sample) {
        GLFWgamepadstate state;
        if (!glfwJoystickIsGamepad(jid) || !glfwGetGamepadState(jid, &state))
            return false;
        for (int b = 0; b < COUNT; b++)
            sample.buttons[b] = state.buttons[b] == GLFW_PRESS;
        std::copy(std::begin(state.axes), std::end(state.axes), sample.axes.begin());
        return true;
    });
}

void Gamepad::poll(const Reader &read) {
    if (!enabled || !read) {
        clear();
        return;
    }
    Sample sample;
    if (selectedDevice >= 0 && read(selectedDevice, sample)) {
        accept(selectedDevice, sample);
        return;
    }
    for (int device = 0; device < DEVICE_COUNT; device++) {
        if (device == selectedDevice)
            continue;
        sample = {};
        if (read(device, sample)) {
            accept(device, sample);
            return;
        }
    }
    clear();
}

void Gamepad::accept(int device, const Sample &sample) {
    const bool fresh = !connected || selectedDevice != device;
    std::copy(std::begin(now), std::end(now), std::begin(before));
    for (int b = 0; b < COUNT; b++) {
        if (fresh)
            suppressed[b] = sample.buttons[b];
        else if (!sample.buttons[b])
            suppressed[b] = false;
        now[b] = sample.buttons[b] && !suppressed[b];
    }
    move = stick(sample.axes[LEFT_X], sample.axes[LEFT_Y], deadZone);
    look = stick(sample.axes[RIGHT_X], sample.axes[RIGHT_Y], deadZone);
    float triggers[] = {trigger(sample.axes[LEFT_TRIGGER], triggerDeadZone), trigger(sample.axes[RIGHT_TRIGGER], triggerDeadZone)};
    for (int t = 0; t < 2; t++) {
        if (fresh)
            suppressedTriggers[t] = triggers[t] > 0;
        else if (triggers[t] == 0)
            suppressedTriggers[t] = false;
        if (suppressedTriggers[t])
            triggers[t] = 0;
    }
    leftTrigger = triggers[0];
    rightTrigger = triggers[1];
    selectedDevice = device;
    connected = true;
}

void Gamepad::clear() {
    std::fill(std::begin(now), std::end(now), false);
    std::fill(std::begin(before), std::end(before), false);
    std::fill(std::begin(suppressed), std::end(suppressed), false);
    suppressedTriggers[0] = suppressedTriggers[1] = false;
    move = look = glm::vec2(0);
    leftTrigger = rightTrigger = 0;
    selectedDevice = -1;
    connected = false;
}

void Gamepad::suppressHeld() {
    for (int b = 0; b < COUNT; b++)
        if (now[b]) {
            suppressed[b] = true;
            now[b] = false;
        }
    if (leftTrigger > 0)
        suppressedTriggers[0] = true;
    if (rightTrigger > 0)
        suppressedTriggers[1] = true;
    leftTrigger = rightTrigger = 0;
}

void Gamepad::setContext(Context value) {
    if (currentContext != value) {
        suppressHeld();
        currentContext = value;
    }
}

Gamepad::GameplayInput Gamepad::gameplay() const {
    if (!connected || currentContext != Context::Gameplay)
        return {};
    return {move, look, held(A), held(B), held(LEFT), held(LEFT_THUMB),
            pressed(RIGHT_THUMB), held(RIGHT),
            {triggerHeld(true), triggerHeld(false), held(X), held(Y), held(RB), held(LB)}};
}

std::optional<bool> Gamepad::PointerButtons::update(size_t button, bool down, Source source) {
    if (button >= mouse.size())
        return std::nullopt;
    const bool before = held(button);
    (source == Source::Mouse ? mouse : controller)[button] = down;
    const bool after = held(button);
    return before != after ? std::optional<bool>(after) : std::nullopt;
}

bool Gamepad::PointerButtons::held(size_t button) const {
    return button < mouse.size() && (mouse[button] || controller[button]);
}

void Gamepad::PointerButtons::cancelController() {
    controller.fill(false);
}
