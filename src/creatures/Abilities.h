#pragma once
// The player's abilities (docs/specs/alpha-creatures-behaviour.md §16): the six slots of the
// ability bar, what pressing and releasing their controls does, mana costs and cooldowns.
#include "creatures/Combat.h"
#include "creatures/Physics.h"
#include <array>
#include <span>

namespace creatures {

// §16.1: the skill an ability levels with (6, 7, 8), or -1.
int32_t skillOf(int32_t id);
// §16.1: v(id) = 1 - 1 / (0.1 n + 1), n the skill level (1 for abilities without a skill).
float abilityValue(const Combatant &c, int32_t id);
// §16.4: the share of the mana bar an ability costs.
float manaCost(const Combatant &c, int32_t id);
// §16.5: an ability's cooldown in ms (0: none).
int32_t cooldownLength(const Combatant &c, int32_t id);
// §16.4: usable now (not stunned, enough mana, no cooldown running).
bool usableNow(const Combatant &c, int32_t id);
// §16.3: free to act (an ability may cut the current action short).
bool freeToAct(const Combatant &c);

// §16.2: the bar's six ids (0 = empty): M1, M2, keys 1, 2, 3, 4.
std::array<int32_t, 6> abilitySlots(const Combatant &c);

// One concrete pre-world selection, shared by later input and the ability bar.
// Fixed engine storage avoids allocations; only the native two/five slots are
// exposed. Construction is empty, and every refresh replaces all logical values.
class AbilityBindings {
  public:
    void refresh(const Combatant &c);
    std::span<const int32_t> values() const { return std::span(storage).first(count); }

  private:
    std::array<int32_t, 6> storage{};
    size_t count = 0;
};

// §16.3 for one frame. `held` are the controls of the six slots; `aim` is the vector from the
// player to the point under the crosshair, blocks. Returns the buff records the frame
// produced (§16.6; packet 8 when online).
struct AbilityEffects {
    std::vector<BuffEvent> buffs;
    std::vector<int32_t> sounds; // sound ids played at the player (§14)
    std::vector<Burst> bursts;   // particles at the player (the splash)
    std::vector<SoundEvent> placed; // sounds with their own pitch and volume (landing, steps)
};
AbilityEffects useAbilities(Combatant &c, Body &body, const std::array<bool, 6> &held, glm::vec3 aim);
// Execute supplied bindings separately from the engine's preset selection.
// Valid binding/input storage remains unchanged during this phase. The engine
// bounds configured inputs to its six supported controls.
AbilityEffects useBoundAbilities(Combatant &c, Body &body, std::span<const int32_t> bindings,
                                const std::array<bool, 6> &held, glm::vec3 aim);

} // namespace creatures
