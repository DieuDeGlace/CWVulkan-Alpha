#pragma once
// Owner-requested engine controller support. A standard-mapped pad is read once
// per frame through GLFW. Keep the selected device until it disconnects.
//
//   left stick   move            right stick  camera (the map: turn; menus: the cursor)
//   A            jump            B            dodge
//   RT / LT      M1 / M2         X, Y, RB, LB ability keys 1, 2, 3, 4
//   Start        Esc menu row    Back         world map
//   D-pad up/down camera zoom    D-pad right  quick item (Q)
//   A on a menu  click           A while dead revive (R)
//   right stick press interact/pick up; left stick press special item; D-pad left climb
#include <array>
#include <functional>
#include <optional>
#include <glm/glm.hpp>

class Config;
struct GLFWwindow;

struct Gamepad {
    enum Button { A, B, X, Y, LB, RB, BACK, START, GUIDE, LEFT_THUMB, RIGHT_THUMB, UP, RIGHT, DOWN, LEFT, COUNT };
    enum Axis { LEFT_X, LEFT_Y, RIGHT_X, RIGHT_Y, LEFT_TRIGGER, RIGHT_TRIGGER, AXIS_COUNT };
    static constexpr int DEVICE_COUNT = 16;
    struct Sample {
        std::array<bool, COUNT> buttons{};
        std::array<float, AXIS_COUNT> axes{0, 0, 0, 0, -1, -1};
    };
    // Alternative input backends and tests use the same state/selection path.
    using Reader = std::function<bool(int device, Sample &)>;
    enum class Context { Gameplay, Menu, Map, Text };
    struct GameplayInput {
        glm::vec2 move{0}, look{0};
        bool jump = false, dodge = false, climb = false, special = false;
        bool interact = false, quick = false;
        std::array<bool, 6> abilities{};
    };
    class PointerButtons {
      public:
        enum class Source { Mouse, Controller };
        // Only the combined up/down transition reaches UI callbacks. One
        // device cannot release a button still held by the other.
        std::optional<bool> update(size_t button, bool down, Source source);
        bool held(size_t button) const;
        void cancelController(); // focus/device loss is cancellation, not a click
      private:
        std::array<bool, 3> mouse{}, controller{};
    };

    bool enabled = true;
    float deadZone = 0.2f;   // of the stick's travel
    float triggerDeadZone = 0.05f;
    float lookSpeed = 300;   // camera degrees-equivalent pixels per second at full tilt
    float cursorSpeed = 900; // menu cursor pixels per second at full tilt
    bool invertY = false;

    bool connected = false;
    glm::vec2 move{0}, look{0}; // sticks after the dead zone, -1..1 (y up is +)
    float leftTrigger = 0, rightTrigger = 0; // 0..1

    void load(const Config &config);
    // Reads the pad; call once per frame.
    void poll();
    void poll(const Reader &read);
    void clear(); // loss of focus, disabled input or no mapped device: neutral state
    int device() const { return selectedDevice; }
    void setContext(Context value);
    Context context() const { return currentContext; }
    // Prevent a menu click/back or held trigger from becoming a gameplay action.
    // Suppression lasts until that physical control is released.
    void suppressHeld();
    GameplayInput gameplay() const;
    bool held(Button b) const { return connected && b >= A && b < COUNT && now[b]; }
    // Went down this frame.
    bool pressed(Button b) const { return held(b) && !before[b]; }
    bool triggerHeld(bool right) const { return connected && (right ? rightTrigger : leftTrigger) > 0.5f; }

  private:
    bool now[COUNT] = {}, before[COUNT] = {};
    bool suppressed[COUNT] = {}, suppressedTriggers[2] = {};
    int selectedDevice = -1;
    Context currentContext = Context::Gameplay;
    void accept(int device, const Sample &sample);
};
