#pragma once
// The local player in the world: keyboard and mouse to intent, jump and dodge
// (alpha-player-control.md §3, §4), the body's physics (alpha-physics-camera.md), the follow
// camera, and the player drawn as its creature.
#include "CreatureView.h"
#include "Gamepad.h"
#include "WorldView.h"
#include "creatures/Abilities.h"
#include "creatures/Combat.h"
#include <utility>
#include "creatures/FollowCamera.h"

struct GLFWwindow;

class Player {
  public:
    // Stands the player on the ground at the view's start column, looking like `looks`.
    Player(WorldView &view, const creatures::Appearance &looks);

    // Mouse look and wheel (counts since the last call).
    void look(float dx, float dy) { camera.look(dx, dy); }
    void wheel(int notches) { camera.wheel(notches); }
    // Common actor/physics phase; the world caller owns preliminary health/clock.
    // World calls never poll devices or start actions from current-frame input.
    void advance(int32_t dtMs, bool replica = false);
    void refreshAbilityBindings() { bindings.refresh(fighter); }
    std::span<const int32_t> abilityBindings() const { return bindings.values(); }
    // Called once after all world pieces; leaves intent/actions for the next call.
    void readInput(GLFWwindow *window, int32_t dtMs, bool controls, const Gamepad *pad = nullptr);
    void finishFrame(int32_t dtMs); // engine camera adapter, once per rendered frame
    // Adds the player's creature to the draw list.
    // Also the player's footsteps (models §7.3), into the ability effects' sounds.
    void draw(std::vector<DrawItem> &items, CreatureView &creatureView, float dtMs,
              std::vector<EffectVertex> *effects = nullptr);

    const creatures::Body &body() const { return state; }
    creatures::Body &body() { return state; }
    // The action clock before this frame's update (to find the moment a hit lands).
    int32_t actionBefore() const { return actionMsBefore; }
    float facing() const { return yaw; } // degrees, the creature's displayed rotation
    float &facing() { return yaw; }
    const creatures::Appearance &appearance() const { return looks; }
    void setLooks(const creatures::Appearance &a) { looks = a; } // (character creation's preview)
    // Stats, action and health (alpha-creatures-behaviour.md).
    creatures::Combatant &combat() { return fighter; }
    // Screens §5.8: R while dead. Stamina full, stealth and timers cleared, full health, back
    // at the start (the original searches the zones round for a respawn point; that search is
    // not specified, so the place the player entered the world stands in).
    void revive();
    // A hit record aimed at the player (packets §5.1).
    void takeHit(const creatures::HitRecord &hit) {
        const auto *source = hitSource ? hitSource(hit.attacker) : nullptr;
        creatures::applyHit(fighter, state, hit, {source, true, replicaWorld}, &abilityEffects.buffs);
    }
    void receiveHit(const creatures::HitRecord &hit, std::vector<creatures::BuffEvent> *updates = nullptr) {
        creatures::receiveHit(fighter, state, hit, updates);
    }
    const creatures::Combatant &combat() const { return fighter; }
    // Mouse look settings (options window, screens §6.4).
    creatures::CameraOptions &cameraOptions() { return camera.options; }
    creatures::FollowCamera &followCamera() { return camera; }
    // Physics §4.5: the static objects the player collides with.
    creatures::ObjectTest objects;
    float cameraPitch() const { return camera.pitch; } // θ, 90 level, 180 straight down
    // Behaviour §16: what this frame's abilities produced (buff records, sounds), taken by the
    // game.
    creatures::AbilityEffects takeAbilityEffects() { return std::exchange(abilityEffects, {}); }
    // World-owned action effects run after input/lock handling and before buff,
    // stun, cooldown and clock progression, using the pre-movement body.
    std::function<void(int32_t beforeMs, int32_t dtMs)> onActionEffects;
    // Authoritative periodic hits run inside resident countdown, before cleanup.
    std::function<void(const creatures::Buff &)> onPeriodicHit;
    std::function<const creatures::Combatant *(uint64_t)> hitSource;

  private:
    uint8_t blockType(int32_t x, int32_t y, int32_t z) const { return view.held().read(x, y, z).type(); }

    WorldView &view;
    creatures::Appearance looks;
    creatures::Body state;
    creatures::PoseState pose;
    creatures::FollowCamera camera;
    creatures::AbilityEffects abilityEffects;
    creatures::AbilityBindings bindings;
    creatures::Combatant fighter;
    float yaw = 0;
    float jumpHeld = 0, jumpLock = 0; // §4, milliseconds
    bool gWasDown = false;            // travel §3.1: G acts on the press
    glm::vec3 wish{0}; // most recently published movement input
    int32_t actionMsBefore = 0;
    bool replicaWorld = false;
    glm::dvec3 start{0}; // where the player entered the world, blocks
};
