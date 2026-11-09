#pragma once
// Built-in controller support (AlphaVulkan's own; Alpha reads only keyboard and mouse). The first
// connected pad with a standard (Xbox-style) mapping is read each frame through GLFW, with a
// dead zone on the sticks and triggers. [Controller] in AlphaVulkan.ini turns it off or tunes it.
//
//   left stick   move            right stick  camera (the map: turn; menus: the cursor)
//   A            jump            B            dodge
//   RT / LT      M1 / M2         X, Y, RB, LB ability keys 1, 2, 3, 4
//   Start        Esc menu row    Back         world map
//   D-pad up/down camera zoom    D-pad right  quick item (Q)
//   A on a menu  click           A while dead revive (R)
#include <glm/glm.hpp>

class Config;
struct GLFWwindow;

struct Gamepad {
    enum Button { A, B, X, Y, LB, RB, BACK, START, GUIDE, LEFT_THUMB, RIGHT_THUMB, UP, RIGHT, DOWN, LEFT, COUNT };

    bool enabled = true;
    float deadZone = 0.2f;   // of the stick's travel
    float lookSpeed = 300;   // camera degrees-equivalent pixels per second at full tilt
    float cursorSpeed = 900; // menu cursor pixels per second at full tilt
    bool invertY = false;

    bool connected = false;
    glm::vec2 move{0}, look{0}; // sticks after the dead zone, -1..1 (y up is +)
    float leftTrigger = 0, rightTrigger = 0; // 0..1

    void load(const Config &config);
    // Reads the pad; call once per frame.
    void poll();
    bool held(Button b) const { return connected && now[b]; }
    // Went down this frame.
    bool pressed(Button b) const { return connected && now[b] && !before[b]; }
    bool triggerHeld(bool right) const { return connected && (right ? rightTrigger : leftTrigger) > 0.5f; }

  private:
    bool now[COUNT] = {}, before[COUNT] = {};
};
