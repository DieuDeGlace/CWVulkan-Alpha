#pragma once
#include "creatures/Appearance.h"
#include "creatures/Combat.h"
#include "creatures/Physics.h"
#include <optional>

namespace creatures::creation {

// Resident configuration, outside copied network core state. Fresh actors use
// choice 1 and no candidates; descriptor activation supplies its own values.
struct Configuration {
    int32_t choice = 1;
    std::vector<int32_t> kinds;
};
struct Actor {
    const Combatant *fighter = nullptr;
    const Body *body = nullptr;
    const Appearance *looks = nullptr;
};
using Visitor = std::function<void(uint64_t key, const Combatant *)>;
struct World {
    uint8_t mode = 0;
    const Combatant *designated = nullptr;
    std::function<void(const Visitor &)> visit;
    std::function<Actor(uint64_t)> lookup;
};
struct Services {
    std::function<int32_t(int32_t)> anticipation; // negative argument resolves current action
    std::function<float(const Combatant &)> maximumHealth;
    std::function<int32_t()> draw;
};
enum class Child { Combat, Companion, Wander };
struct Setup {
    std::array<Child, 3> children{Child::Combat, Child::Companion, Child::Wander};
    int32_t specialMs = 20000, chaseMs = 0, cooldownMs = 0, walkMs = 0;
    float range = 20;
    uint64_t companionKey = 0;
};
struct Created {
    Combatant fighter;
    Body body;
    Appearance looks;
    Setup setup;
};
// The appearance producer's common prefix with a null descriptor: five draws forming a
// local descriptor of type zero. Draws 1-3 are reduced by signed remainder 256 and stored
// reversed (third draw -> byte 0, second -> byte 1, first -> byte 2); draws 4 and 5 are
// reduced by signed remainder 100 into the two selectors. The closed kinds (0x11, 0x25,
// 0x60, >0x9b) do not read it; kind routes that consume it read this descriptor.
struct AppearanceMaterial {
    uint8_t type = 0;
    std::array<uint8_t, 3> bytes{};
    std::array<int32_t, 2> selectors{};
};
AppearanceMaterial drawAppearanceMaterial(const std::function<int32_t()> &draw);

enum class Outcome { Rejected, Created, AppearancePending, MissingAppearanceState };
struct Result {
    Outcome outcome = Outcome::Rejected;
    std::optional<Created> actor;
};

// Held alpha-world-actor-creation.md. Call at the action-effect phase before
// clock advancement. The caller publishes the completed actor in its live map.
// The current actor is read-only here: every exit (admission, either threshold,
// link count, success) writes no actor field and the caller continues with its
// common per-actor processing.
// Stable supplied actor/map pointers and ordinary finite arithmetic are required.
// Generation outputs are closed for kinds 17, 37, 96 and unsigned kinds >0x9b;
// other generation kinds report an engine capability boundary, not a native gate.
Result create(Actor current, const Configuration &configuration, int32_t step,
              const World &world, const Services &services);

} // namespace creatures::creation
