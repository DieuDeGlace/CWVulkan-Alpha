#include "Gamepad.h"
#include "ui/Screens.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}
struct Devices {
    std::array<std::optional<Gamepad::Sample>, Gamepad::DEVICE_COUNT> samples;
    std::vector<int> reads;
    bool read(int id, Gamepad::Sample &sample) {
        reads.push_back(id);
        if (!samples[id])
            return false;
        sample = *samples[id];
        return true;
    }
};
void selectionAndRelease() {
    Devices devices;
    Gamepad pad;
    const Gamepad::Reader read = [&](int id, auto &sample) { return devices.read(id, sample); };
    devices.samples[5] = Gamepad::Sample{};
    devices.samples[5]->buttons[Gamepad::A] = true;
    devices.samples[5]->axes[Gamepad::RIGHT_TRIGGER] = 1;
    pad.poll(read);
    check(pad.connected && pad.device() == 5 && !pad.held(Gamepad::A) && !pad.triggerHeld(true),
          "attaching a held pad does not jump, click or attack");
    pad.poll(read);
    check(!pad.pressed(Gamepad::A) && !pad.triggerHeld(true), "attach suppression lasts through held samples");
    devices.samples[5] = Gamepad::Sample{};
    pad.poll(read);
    devices.samples[5]->buttons[Gamepad::A] = true;
    devices.samples[5]->axes[Gamepad::RIGHT_TRIGGER] = 1;
    pad.poll(read);
    check(pad.held(Gamepad::A) && pad.pressed(Gamepad::A) && pad.triggerHeld(true), "release rearms real button and trigger input");
    devices.samples[1] = Gamepad::Sample{};
    devices.reads.clear();
    pad.poll(read);
    check(pad.device() == 5 && devices.reads == std::vector<int>{5} && !pad.pressed(Gamepad::A),
          "new lower-index pad cannot steal the active device or repeat its edge");
    devices.samples[5].reset();
    devices.samples[1]->buttons[Gamepad::B] = true;
    pad.poll(read);
    check(pad.device() == 1 && !pad.held(Gamepad::B) && !pad.pressed(Gamepad::B), "replacement held buttons require release");
    devices.samples[1].reset();
    pad.poll(read);
    check(!pad.connected && pad.device() == -1 && pad.move == glm::vec2(0) && pad.look == glm::vec2(0) &&
          pad.leftTrigger == 0 && pad.rightTrigger == 0, "disconnect clears all output state");
    devices.samples[1] = Gamepad::Sample{};
    devices.samples[1]->axes[Gamepad::LEFT_X] = 1;
    pad.poll(read);
    check(pad.move.x == 1, "connected stick is available");
    pad.enabled = false;
    devices.reads.clear();
    pad.poll(read);
    check(!pad.connected && pad.move == glm::vec2(0) && devices.reads.empty(), "disabled input is neutral without polling devices");
    pad.enabled = true;
    pad.poll(read);
    pad.clear();
    check(!pad.connected && pad.move == glm::vec2(0) && !pad.held(Gamepad::A), "focus-loss clear releases all controls");
    pad.poll(Gamepad::Reader{});
    check(!pad.connected, "missing backend is neutral");
}
void axesAndMappings() {
    Gamepad pad;
    Gamepad::Sample sample;
    const Gamepad::Reader read = [&](int id, auto &out) {
        if (id != 0)
            return false;
        out = sample;
        return true;
    };
    pad.deadZone = 0.5f;
    pad.triggerDeadZone = 0.25f;
    pad.poll(read);
    sample.axes = {0.5f, 0, 0, 0, -0.5f, -1};
    pad.poll(read);
    check(pad.move == glm::vec2(0) && pad.leftTrigger == 0, "exact radial and trigger dead-zone boundaries are neutral");
    sample.axes = {0, -0.75f, 1, 1, 1, 1};
    sample.buttons.fill(true);
    pad.poll(read);
    const auto input = pad.gameplay();
    check(input.move == glm::vec2(0, 0.5f) && std::abs(glm::length(input.look) - 1) < 0.00001f && input.look.y < 0,
          "rescaled radial sticks retain direction and cap diagonal magnitude");
    check(input.jump && input.dodge && input.climb && input.special && input.interact && input.quick,
          "gameplay maps jump, dodge, climb, special, interact and quick item");
    check(std::all_of(input.abilities.begin(), input.abilities.end(), [](bool held) { return held; }),
          "both attacks and all four abilities share the gameplay mapping");
    pad.poll(read);
    check(!pad.gameplay().interact, "interaction is one press, not a repeating held action");
    pad.setContext(Gamepad::Context::Menu);
    check(!pad.held(Gamepad::A) && !pad.triggerHeld(true) && !pad.gameplay().jump && pad.gameplay().move == glm::vec2(0),
          "menu transition suppresses held gameplay and click controls");
    pad.poll(read);
    pad.setContext(Gamepad::Context::Gameplay);
    check(!pad.gameplay().dodge && !pad.triggerHeld(true), "closing a menu cannot turn held back/click/trigger into gameplay");
    sample = {};
    pad.poll(read);
    sample.buttons[Gamepad::B] = true;
    sample.axes[Gamepad::RIGHT_TRIGGER] = 1;
    pad.poll(read);
    check(pad.gameplay().dodge && pad.triggerHeld(true), "controls rearm after release across a context change");
    pad.suppressHeld();
    check(!pad.gameplay().dodge && !pad.triggerHeld(true), "explicit UI/revive consumption suppresses the same-frame action");
    sample = {};
    pad.poll(read);
    sample.axes = {std::numeric_limits<float>::quiet_NaN(), 1, 100, -100,
                   std::numeric_limits<float>::infinity(), -100};
    pad.poll(read);
    check(pad.move == glm::vec2(0) && std::isfinite(pad.look.x) && glm::length(pad.look) <= 1.00001f &&
          pad.leftTrigger == 0 && pad.rightTrigger == 0, "malformed axes cannot inject NaNs, oversized vectors or triggers");
    sample = {};
    pad.poll(read);
    pad.deadZone = std::numeric_limits<float>::quiet_NaN();
    sample.axes[Gamepad::LEFT_X] = 1;
    pad.poll(read);
    check(pad.move.x == 1, "invalid external dead-zone setting uses a finite default");
}
void menuBack() {
    ui::Interface ui;
    ui.chatOpen = true;
    ui.chatInput = u"unsent";
    check(ui.controllerBack() && !ui.chatOpen && ui.chatInput.empty() && ui.chatSubmitted.empty(), "back cancels chat without submitting");
    ui.optionsOpen = true;
    check(ui.controllerBack() && !ui.optionsOpen && !ui.optionsToApply, "back leaves options without applying a draft");
    ui.front = ui::Interface::Front::Creation;
    check(ui.controllerBack() && ui.front == ui::Interface::Front::Characters, "creation back returns to characters");
    ui.front = ui::Interface::Front::WorldCreation;
    check(ui.controllerBack() && ui.front == ui::Interface::Front::Worlds, "world creation back returns to worlds");
    ui.front = ui::Interface::Front::Connect;
    check(ui.controllerBack() && ui.front == ui::Interface::Front::Worlds, "connect back returns to worlds");
    check(ui.controllerBack() && ui.front == ui::Interface::Front::Characters, "world list back returns to characters");
    ui.frontRequest = ui::Interface::FrontRequest::DeleteCharacter;
    check(ui.controllerBack() && ui.startScreen && ui.front == ui::Interface::Front::None &&
          ui.frontRequest == ui::Interface::FrontRequest::None, "character list back cancels pending front requests");
    ui.startScreen = false;
    ui.inventoryOpen = ui.vendorOpen = ui.benchOpen = ui.skillsOpen = ui.craftingOpen = ui.identifyOpen = ui.adaptOpen = true;
    check(ui.controllerBack() && !ui.wantsMouse() && !ui.vendorOpen && !ui.benchOpen && !ui.skillsLearnt,
          "back closes inventory panels without learning a draft");
    ui.mapOpen = ui.teleportMode = true;
    ui.inventoryOpen = true;
    ui.selectedPortal = {1, 2};
    check(ui.controllerBack() && !ui.mapOpen && !ui.teleportMode && ui.inventoryOpen && ui.selectedPortal == glm::ivec2(-1),
          "map back leaves teleport selection and returns to an underlying inventory");
    ui.controllerBack();
    ui.menuRow = true;
    check(ui.controllerBack() && !ui.menuRow && !ui.controllerBack(), "back closes the menu row once");
}
void mixedPointer() {
    Gamepad::PointerButtons buttons;
    using Source = Gamepad::PointerButtons::Source;
    check(buttons.update(0, true, Source::Mouse) == true, "physical mouse press reaches the interface");
    check(!buttons.update(0, true, Source::Controller), "controller press cannot double-click an already held button");
    check(!buttons.update(0, false, Source::Mouse) && buttons.held(0), "mouse release cannot end a controller drag");
    check(buttons.update(0, false, Source::Controller) == false && !buttons.held(0), "last source release ends the drag exactly once");
    check(buttons.update(1, true, Source::Controller) == true, "controller right click reaches the interface");
    check(!buttons.update(1, true, Source::Mouse) && !buttons.update(1, false, Source::Controller) && buttons.held(1),
          "controller release cannot end a physical mouse hold");
    check(buttons.update(1, false, Source::Mouse) == false, "physical release ends the remaining hold");
    buttons.update(0, true, Source::Controller);
    buttons.update(2, true, Source::Mouse);
    buttons.cancelController();
    check(!buttons.held(0) && buttons.held(2) && !buttons.update(0, false, Source::Controller),
          "controller cancellation clears its hold without fabricating a release or losing mouse state");
    buttons.update(0, true, Source::Mouse);
    buttons.update(0, true, Source::Controller);
    buttons.clear();
    check(!buttons.held(0) && !buttons.held(2) && !buttons.update(0, false, Source::Mouse) &&
          !buttons.update(0, false, Source::Controller), "modal handoff cannot deliver either old release to a new screen");
    check(buttons.update(0, true, Source::Mouse) == true, "fresh physical press rearms after modal consumption");
}
}

int main(int argc, char **argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--devices") {
        if (!glfwInit()) {
            std::puts("GLFW device initialization failed");
            return 1;
        }
        int count = 0;
        for (int id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; id++)
            if (glfwJoystickIsGamepad(id)) {
                std::printf("Mapped controller %d: %s\n", id, glfwGetGamepadName(id));
                count++;
            }
        Gamepad pad;
        pad.poll();
        std::printf("Mapped controllers: %d; selected: %d\n", count, pad.device());
        glfwTerminate();
        return 0;
    }
    selectionAndRelease();
    axesAndMappings();
    menuBack();
    mixedPointer();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
