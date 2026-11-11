#include "Player.h"
#include "creatures/Projectiles.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

Player::Player(WorldView &v, const creatures::Appearance &a, worldgen::Random &r) : view(v), random(r), looks(a) {
    state.size = looks.box;
    state.swimmer = (looks.flags & creatures::SWIMMER) != 0;
    const glm::dvec3 at = view.worldPosition();
    const int32_t x = (int32_t)std::floor(at.x), y = (int32_t)std::floor(at.y);
    const worldgen::Zone *zone = view.held().zoneAt(x, y);
    const double ground = zone ? (double)zone->column(x, y).top() : std::max(view.groundHeight(x, y), 0.f);
    state.place({at.x + 0.5, at.y + 0.5, ground + state.size.z * 0.5 + 0.01});
    state.fallStart = (float)(ground + state.size.z * 0.5);
    start = state.blocks();
    fighter.appearanceFlags = looks.flags;
    fighter.health = creatures::maxHealth(fighter);
    // Facing away from the camera, along its forward direction.
    const glm::vec3 f = camera.forward();
    yaw = (float)(std::atan2(-f.x, f.y) * 180.0 / 3.141592653589793);
}

void Player::revive() {
    state.stamina = 1;
    state.dodgeMs = 0;
    state.velocity = glm::vec3(0);
    fighter.stealth = 0;
    fighter.dodgeMs = 0;
    fighter.hitStun = 0;
    fighter.combo = 0;
    fighter.criticalLatched = false;
    fighter.buffs.clear();
    state.place(start);
    fighter.health = creatures::maxHealth(fighter);
}

void Player::readInput(GLFWwindow *window, int32_t milliseconds, bool controls, const Gamepad *pad,
                       const std::array<bool, 2> *mouseButtons) {
    using namespace creatures;
    const float dtMs = (float)milliseconds;
    const bool usePad = controls && pad && pad->connected && pad->context() == Gamepad::Context::Gameplay;
    const auto controller = usePad ? pad->gameplay() : Gamepad::GameplayInput{};
    // Combat owns the complete mode word between phases. Input edits a physics
    // view and publishes it below, retaining flags unrelated to movement.
    state.mode = fighter.mode;
    // A key, or the pad button that stands for it (Gamepad.h).
    auto held = [&](int key) {
        if (controls && glfwGetKey(window, key) == GLFW_PRESS)
            return true;
        switch (key) {
        case GLFW_KEY_SPACE: return controller.jump;
        case GLFW_KEY_LEFT_SHIFT: return controller.dodge;
        case GLFW_KEY_LEFT_CONTROL: return controller.climb;
        case GLFW_KEY_G: return controller.special;
        default: return false;
        }
    };
    // The right stick turns the camera as the mouse does.
    if (usePad && glm::dot(controller.look, controller.look) > 0) {
        const float k = pad->lookSpeed * dtMs * 0.001f;
        camera.look(controller.look.x * k, (pad->invertY ? 1.f : -1.f) * controller.look.y * k);
    }
    const bool onGround = state.contact & ON_GROUND, inWater = state.contact & IN_WATER;
    state.stunMs = fighter.hitStun;
    // Travel §2.1: the intent starts each frame from rest, except while gliding, when it
    // turns toward the flight and leans with the bank, keeping what the keys added.
    if (!gliding(state)) {
        state.intent = glm::vec3(0);
    } else if (const glm::vec3 h(state.velocity.x, state.velocity.y, 0); glm::dot(h, h) > 0) {
        const glm::vec3 ahead = glm::normalize(h), side(-ahead.y, ahead.x, 0);
        const float w = 1 - std::pow(0.999f, dtMs);
        state.intent = (1 - w) * state.intent + w * 10.f * ahead;
        state.intent -= 0.05f * state.bank * side;
    }
    // Travel §2.2: run unless Left Shift is held (with the glider in the air, Space lifts
    // instead), and hold on to walls with Left Ctrl.
    if ((state.mode & GLIDER) && !onGround)
        state.mode = held(GLFW_KEY_SPACE) && state.stamina > 0 ? state.mode | RUN : state.mode & ~RUN;
    else
        state.mode = held(GLFW_KEY_LEFT_SHIFT) ? state.mode & ~RUN : state.mode | RUN;
    state.mode = held(GLFW_KEY_LEFT_CONTROL) ? state.mode | CLIMB : state.mode & ~CLIMB;
    // Travel §3.1, §4.1: G opens or folds the glider, or boards or leaves the boat.
    const bool gDown = held(GLFW_KEY_G);
    if (gDown && !gWasDown) {
        const Item &special = fighter.equipment[11];
        if (special.type == 0) {
            state.mode &= ~GLIDER;
            if (fighter.action == 107)
                fighter.action = 0;
        } else if (special.subtype != 1) {
            const int64_t fallen = (int64_t)(state.fallStart * 65536.0) - state.position[2];
            const bool open = fallen < 2 * 65536 && !(state.mode & GLIDER);
            state.mode = open ? state.mode | GLIDER : state.mode & ~GLIDER;
            fighter.action = 0;
        } else if (fighter.action == 107) {
            fighter.action = 0;
        } else if (inWater) {
            fighter.action = 107;
            fighter.actionMs = 0;
        }
    }
    gWasDown = gDown;

    // Player control §3: the keys add to the intent, each by the first rule that applies.
    const float A = gliding(state) ? 8.f : 80.f;
    const glm::vec3 f = camera.forward(), r = camera.right();
    float ahead = (float)held(GLFW_KEY_W) - (float)held(GLFW_KEY_S);
    float aside = (float)held(GLFW_KEY_D) - (float)held(GLFW_KEY_A);
    if (usePad) // the left stick, as far as it is pushed
        ahead += controller.move.y, aside += controller.move.x;
    wish = ahead * f + aside * r;
    const bool climbing = (state.mode & CLIMB) && (state.contact & ON_WALL);
    if (climbing) {
        // Rule 1: up and down the wall, and along it.
        const glm::vec3 along(-state.wallNormal.y, state.wallNormal.x, 0);
        state.intent += 0.2f * A * (glm::vec3(0, 0, ahead) + aside * along);
    } else if (held(GLFW_KEY_LEFT_SHIFT) && glm::dot(wish, wish) > 0 && state.stamina >= 0.25f &&
               (state.dodgeMs < 1 || fighter.action != 0) && (onGround || inWater)) {
        // Rule 2: the dodge (Left Shift) needs stamina and ground or water.
        state.velocity = 40.f * glm::normalize(wish) + glm::vec3(0, 0, 5);
        state.dodgeMs = 600;
        state.stamina -= 0.25f;
        fighter.action = 0;
    } else if ((state.mode & GLIDER) && !onGround && state.stamina <= 0) {
        // Rule 4: a tired glider cannot steer.
    } else if (inWater && fighter.action != 107) {
        // Rule 3: swimming follows the camera in three dimensions.
        const glm::vec3 F = camera.direction(); // the camera's full forward axis
        glm::vec3 swim(0);
        if (held(GLFW_KEY_W))
            swim += F;
        if (held(GLFW_KEY_S))
            swim -= F;
        if (held(GLFW_KEY_D))
            swim += r;
        if (held(GLFW_KEY_A))
            swim -= r;
        if (usePad)
            swim += controller.move.y * F + controller.move.x * r;
        state.intent += A * swim;
    } else {
        state.intent += A * wish; // rule 5: walking
    }
    // §4: jumping and swimming up.
    if (!held(GLFW_KEY_SPACE)) {
        jumpHeld = 0;
    } else {
        jumpHeld += dtMs;
        if (jumpHeld >= 1) {
            if (jumpHeld < 200 && (onGround || state.dodgeMs > 0) && state.velocity.z <= 0) {
                state.velocity.z = std::min(10.f, glm::length(glm::vec2(state.velocity)) + 2.f);
                jumpLock = 100;
            }
            if (inWater && state.velocity.z < 10) {
                jumpLock = 100;
                state.velocity.z += 0.001f * dtMs;
            }
        }
        if (jumpLock > 0 && state.velocity.z < 10)
            state.velocity.z = std::min(state.velocity.z + 0.08f * dtMs, 10.f);
    }
    jumpLock = std::max(0.f, jumpLock - dtMs);

    // Behaviour §16.3: the ability bar's controls, M1, M2 and keys 1-4.
    fighter.dodgeMs = state.dodgeMs;
    actionMsBefore = fighter.actionMs;
    const bool mouse = controls && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    const bool primary = mouseButtons ? (*mouseButtons)[0] : glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool secondary = mouseButtons ? (*mouseButtons)[1] : glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const std::array<bool, 6> slotsHeld = {
        (mouse && primary) || controller.abilities[0],
        (mouse && secondary) || controller.abilities[1],
        held(GLFW_KEY_1) || controller.abilities[2], held(GLFW_KEY_2) || controller.abilities[3],
        held(GLFW_KEY_3) || controller.abilities[4], held(GLFW_KEY_4) || controller.abilities[5]};
    // §16.1: the aim, from the player to the first solid block under the crosshair (or 100
    // blocks out), in half-block steps.
    const glm::dvec3 eye = camera.position(), me = state.blocks();
    const glm::dvec3 ray = glm::dvec3(camera.direction());
    glm::dvec3 target = eye + ray * 100.0;
    for (double t = 0.5; t < 100; t += 0.5) {
        const glm::dvec3 q = eye + ray * t;
        const uint8_t type = blockType((int32_t)std::floor(q.x), (int32_t)std::floor(q.y), (int32_t)std::floor(q.z));
        if (type != worldgen::AIR && type != worldgen::WATER) {
            target = q;
            break;
        }
    }
    fighter.aim = glm::vec3(target - me); // record field 26, what projectiles fly towards
    // Pets §7.1: with pet food in the Pet slot the player holds it out (action 82) half a second
    // after the last action ends, unless busy or holding the right button.
    {
        static const int32_t BUSY[] = {8, 28, 29, 35, 36, 79, 82, 83, 84, 106, 107};
        const bool busy = std::find(std::begin(BUSY), std::end(BUSY), fighter.action) != std::end(BUSY);
        if (!busy && !slotsHeld[1]) {
            if (fighter.equipment[12].type == 20 && fighter.actionMs > (float)duration(fighter, fighter.action) + 500)
                fighter.action = 82;
            else if (fighter.actionMs > 10000)
                fighter.action = 0, fighter.actionMs = 0;
        }
    }
    const creatures::AbilityEffects effects = useBoundAbilities(fighter, state, bindings.values(), slotsHeld, fighter.aim);
    abilityEffects.buffs.insert(abilityEffects.buffs.end(), effects.buffs.begin(), effects.buffs.end());
    abilityEffects.sounds.insert(abilityEffects.sounds.end(), effects.sounds.begin(), effects.sounds.end());
    fighter.mode = state.mode;
}

void Player::advance(int32_t dt, bool replica) {
    using namespace creatures;
    replicaWorld = replica;
    const float dtMs = (float)dt;
    if (!admitCreatureUpdate(fighter))
        return;
    state.mode = fighter.mode;
    beginActionUpdate(fighter, dt, &abilityEffects.buffs);
    state.dodgeMs = (int32_t)fighter.dodgeMs;
    actionMsBefore = fighter.actionMs;
    if (onActionEffects)
        onActionEffects(actionMsBefore, dt);
    endActionUpdate(fighter, dt, {true, replica}, onPeriodicHit);

    // The physics on whole milliseconds.
    state.mode = fighter.mode;
    state.local = true;
    state.flyer = (looks.flags & FLYER) != 0;
    state.skills = fighter.skills;
    state.action = fighter.action;
    state.controller = fighter.controller, state.type = fighter.type, state.appearanceFlags = looks.flags;
    {
        const Phases p = phases(fighter, fighter.action);
        state.actionMs = fighter.actionMs, state.windUpMs = (float)p.windUp, state.activeMs = (float)p.active;
        state.durationMs = (float)duration(fighter, fighter.action);
        state.haste = fighter.hasBuff(12) ? 1 + skillFactor(fighter.skills[8]) : 1.f;
        state.slowed = fighter.slowMs > 0, state.hastened = fighter.hasteMs > 0;
    }
    state.stunMs = fighter.hitStun;
    state.yaw = yaw;
    if (dt > 0)
        worldStep(state, dt, [&](int32_t x, int32_t y, int32_t z) { return blockType(x, y, z); }, objects);
    const auto u = [&] { return (float)random.draw() / 32767.f; };
    if (std::exchange(state.thudded, false)) // physics §4.2 step 1
        abilityEffects.placed.push_back({state.blocks(), 32, 1 + 0.2f * u(), 0.5f});
    if (std::exchange(state.rolled, false)) // physics §2 step 15
        abilityEffects.sounds.push_back(26);
    if (std::exchange(state.splashed, false)) { // physics §5.3
        abilityEffects.bursts.push_back(splashBurst(state.blocks()));
        abilityEffects.sounds.push_back(31);
    }
    if (state.action == 0 && (fighter.action == 106 || fighter.action == 107))
        fighter.action = 0, fighter.actionMs = 0; // the step ended the ride or the voyage
    if (fighter.action == 72)
        yaw = state.yaw; // the spin turns the player
    // Travel §3.2: the glider folds when the Special slot no longer holds it.
    if (fighter.equipment[11].type != 23 || fighter.equipment[11].subtype == 1)
        state.mode &= ~GLIDER;
    // Travel §3.6, §8: a crash or a hard landing stuns, with sound 23 (and the landing hurts).
    if (state.crashed || state.fallHurt > 0) {
        fighter.health -= state.fallHurt * maxHealth(fighter);
        fighter.hitStun = std::max(fighter.hitStun, 1500);
        abilityEffects.sounds.push_back(23);
        state.crashed = false;
        state.fallHurt = 0;
    }
    fighter.dodgeMs = (float)state.dodgeMs;
    fighter.mode = state.mode;
    advanceCharge(fighter, dtMs);
    state.mode = fighter.mode;
    if ((state.mode & CLIMB) && (state.contact & ON_WALL) && !gliding(state)) {
        // Travel §6.4: a climber turns to face the wall.
        glm::vec2 u = -glm::vec2(state.wallNormal);
        if (glm::dot(u, u) > 0) {
            u = glm::normalize(u);
            float theta = (float)(std::acos(std::clamp(u.y, -1.f, 1.f)) * 180.0 / 3.141592653589793);
            if (u.x > 0)
                theta = -theta;
            const float turn = (float)(std::asin(std::sin(glm::radians(theta - yaw))) * 180.0 / 3.141592653589793);
            yaw += turn * (1 - std::pow(1 - 0.005f, dtMs));
        }
    } else if (glm::dot(wish, wish) > 0) {
        // The creature turns to where it is going (facing(ψc) = Rz(ψc)(0, 1, 0), §3.9).
        yaw = (float)(std::atan2(-wish.x, wish.y) * 180.0 / 3.141592653589793);
    }

}

void Player::finishFrame(int32_t dtMs, const camera_view::Clock &worldClock) {
    // Camera following runs once after the frame's world calls, with the whole
    // frame duration rather than a world piece.
    camera.update(state, dtMs, [&](int32_t x, int32_t y, int32_t z) { return blockType(x, y, z); }, worldClock);
    view.placeCamera(camera.position(), camera.direction());
}

void Player::draw(std::vector<DrawItem> &items, CreatureView &creatureView, float dtMs,
                  std::vector<EffectVertex> *effects) {
    if (camera.distance < 0.8f)
        return; // looking from inside the head
    creatures::Motion motion;
    motion.velocity = state.velocity;
    motion.onGround = (state.contact & creatures::ON_GROUND) != 0;
    motion.inWater = (state.contact & creatures::IN_WATER) != 0;
    motion.yawDegrees = yaw;
    motion.intent = state.intent;
    motion.running = (state.mode & 0x40) != 0;
    motion.climbing = (state.mode & creatures::CLIMB) && (state.contact & creatures::ON_WALL);
    motion.gliding = (state.mode & creatures::GLIDER) != 0;
    motion.dodgeMs = (float)state.dodgeMs;
    motion.fighter = &fighter;
    glm::dvec3 at = state.blocks();
    at.z -= state.bob;
    if (creatureView.draw(items, looks, pose, motion, view.toRender(at), dtMs, effects)) {
        // Models §7.3: a footstep, softer on type-3 ground.
        const glm::dvec3 feet = state.blocks() - glm::dvec3(0, 0, state.size.z * 0.5);
        const glm::ivec3 below = glm::ivec3(glm::floor(feet - glm::dvec3(0, 0, 0.5)));
        if (blockType(below.x, below.y, below.z) == 3)
            abilityEffects.placed.push_back({feet, 33 + random.draw() % 3, 1, 1});
        else
            abilityEffects.placed.push_back({feet, 32, 1 + 0.2f * (float)random.draw() / 32767.f, 1});
    }
}
