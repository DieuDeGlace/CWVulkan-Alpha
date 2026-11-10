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
enum class Outcome { Rejected, Created, AppearancePending, MissingAppearanceState };
struct Result {
    Outcome outcome = Outcome::Rejected;
    std::optional<Created> actor;
};

// Held alpha-world-actor-creation.md. Call at the action-effect phase before
// clock advancement. The caller publishes the completed actor in its live map.
// Stable supplied actor/map pointers and ordinary finite arithmetic are required.
// Generation outputs are closed for kinds 17, 37, 96 and unsigned kinds >0x9b;
// other generation kinds report an engine capability boundary, not a native gate.
Result create(Actor current, const Configuration &configuration, int32_t step,
              const World &world, const Services &services);

} // namespace creatures::creation
