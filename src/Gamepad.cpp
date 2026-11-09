// Built-in controller support (see Gamepad.h).
#include "Gamepad.h"
#include "Config.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

void Gamepad::load(const Config &config) {
    enabled = config.number("Controller", "Enabled", 1) != 0;
    deadZone = std::clamp((float)config.number("Controller", "DeadZone", 20) * 0.01f, 0.f, 0.9f); // percent
    lookSpeed = (float)config.number("Controller", "LookSpeed", (int)lookSpeed);
    cursorSpeed = (float)config.number("Controller", "CursorSpeed", (int)cursorSpeed);
    invertY = config.number("Controller", "InvertY", 0) != 0;
}

namespace {

// A stick past the dead zone, rescaled so its travel starts at 0 again.
glm::vec2 stick(float x, float y, float dead) {
    const glm::vec2 v(x, -y); // GLFW's y points down
    const float length = glm::length(v);
    if (length <= dead)
        return glm::vec2(0);
    return v / length * std::min((length - dead) / (1 - dead), 1.f);
}

float trigger(float raw) { return (raw + 1) * 0.5f; } // GLFW's triggers run -1..1

} // namespace

void Gamepad::poll() {
    std::copy(std::begin(now), std::end(now), std::begin(before));
    connected = false;
    if (!enabled)
        return;
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; jid++) {
        GLFWgamepadstate state;
        if (!glfwJoystickIsGamepad(jid) || !glfwGetGamepadState(jid, &state))
            continue;
        connected = true;
        for (int b = 0; b < COUNT; b++)
            now[b] = state.buttons[b] == GLFW_PRESS;
        move = stick(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], deadZone);
        look = stick(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], deadZone);
        leftTrigger = trigger(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
        rightTrigger = trigger(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);
        return;
    }
    std::fill(std::begin(now), std::end(now), false);
    move = look = glm::vec2(0);
    leftTrigger = rightTrigger = 0;
}
