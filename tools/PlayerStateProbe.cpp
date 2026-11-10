// Exercise the production Player input and actor phases with a supplied pad.
// Owned content is read only; no original process, saves or network are used.
#include "DataStore.h"
#include "Install.h"
#include "Player.h"
#include "net/Protocol.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

void run(GLFWwindow *window, const std::filesystem::path &install) {
    Content content;
    std::string error;
    if (!content.open(install, &error))
        throw std::runtime_error(error);
    Renderer renderer;
    AntiAliasing aa;
    aa.taa = aa.smaa = false;
    ShadowSettings shadows;
    shadows.enabled = false;
    shadows.resolution = 512;
    if (!renderer.init(window, false, aa, shadows, &error))
        throw std::runtime_error(error);
    WorldViewOptions options;
    options.radius = 0; // one generated zone is sufficient for these actor phases
    WorldView view(renderer, content, options);
    creatures::Appearance appearance;
    worldgen::Random random{123};
    Player player(view, appearance, random);
    auto &actor = player.combat();
    auto &body = player.body();
    const std::array<bool, 2> noMouse{};

    actor.mode = 0xa620;
    body.mode = creatures::CLIMB;
    player.readInput(window, 20, false, nullptr, &noMouse);
    require(actor.mode == 0xa660 && body.mode == actor.mode,
            "Input lost copied flags or retained stale climb state");
    player.advance(20);
    require(actor.mode == 0xa660 && body.mode == actor.mode, "Physics lost copied mode flags");

    actor.mode = 0x5a00;
    player.readInput(window, 20, false, nullptr, &noMouse);
    require(actor.mode == 0x5a40 && body.mode == actor.mode, "Input revived flags from the replaced core");

    Gamepad pad;
    Gamepad::Sample sample;
    const Gamepad::Reader reader = [&](int device, Gamepad::Sample &out) {
        if (device != 0)
            return false;
        out = sample;
        return true;
    };
    pad.poll(reader); // attach neutral, then make a fresh press
    sample.buttons[Gamepad::LEFT] = true;
    pad.poll(reader);
    require(pad.gameplay().climb, "Supplied pad did not reach the gameplay mapping");
    player.readInput(window, 20, true, &pad, &noMouse);
    require(actor.mode == 0x5a41 && body.mode == actor.mode,
            "Actual controller input did not add climb while preserving the raw word");

    actor.mode = 0xa671;
    actor.hitStun = 100;
    player.advance(20);
    require(actor.mode == 0xa660 && body.mode == actor.mode, "Stun left the two production views inconsistent");
    player.advance(20);
    require(actor.mode == 0xa660 && body.mode == actor.mode, "The next piece revived cleared movement flags");
    std::array<creatures::ItemRecord, creatures::SLOT_COUNT> ownedEquipment{};
    for (size_t slot = 0; slot < ownedEquipment.size(); slot++) {
        auto &item = ownedEquipment[slot];
        creatures::setItemLevel(item, 1);
        item[2] = uint8_t(0xa0 + slot);
        item[0x80] = uint8_t(0xb0 + slot);
        item[0x113] = uint8_t(0xc0 + slot);
        actor.setEquipment(slot, item);
    }
    creatures::ItemRecord held{};
    held[0] = held[1] = 1;
    held[2] = 0x9a;
    held[0x113] = 0x7b;
    creatures::setItemLevel(held, 1);
    actor.setHeld(held);
    player.readInput(window, 20, false, nullptr, &noMouse);
    player.advance(20);
    net::CreatureRecord outgoing;
    outgoing.setFighter(actor);
    require(std::equal(held.begin(), held.end(), outgoing.bytes.begin() + 0x1d8),
            "Production Player input/physics or outgoing codec lost held-item backing");
    for (size_t slot = 0; slot < ownedEquipment.size(); slot++)
        require(std::equal(ownedEquipment[slot].begin(), ownedEquipment[slot].end(),
                           outgoing.bytes.begin() + 0x2f0 + slot * ownedEquipment[slot].size()),
                "Production Player input/physics or outgoing codec lost equipment backing");
    actor.health = 0;
    actor.mode = 0xf020;
    body.mode = 0x1200;
    player.advance(20);
    require(actor.mode == 0xf020 && body.mode == 0x1200, "A rejected dead update replaced state");
    std::cout << "PlayerStateProbe passed: actual input, supplied controller, physics, stun, replacement, complete equipment/held item and dead admission\n";
}
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: PlayerStateProbe <owned Alpha folder>\n";
        return 1;
    }
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(640, 360, "Player state diagnostic", nullptr, nullptr);
    int result = 0;
    try {
        require(window != nullptr, "Cannot create diagnostic window");
        run(window, pathOfUtf8(argv[1]));
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    if (window)
        glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
