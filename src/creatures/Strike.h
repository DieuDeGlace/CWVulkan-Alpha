#pragma once
// How a melee action becomes hits (docs/specs/alpha-open-points-6.md): when in the action it
// tests for targets, what it reaches, and how much a hit is worth. Used for creatures and for
// the player alike; the hit itself is resolved by resolveHit (behaviour §5).
#include "creatures/Combat.h"
#include <array>
#include <functional>
#include <optional>

namespace creatures {

struct Body;

// Behaviour §13.2: the big types (0x4445d0), which never dodge and shrug off most stuns.
bool bigCreature(int32_t type);

// §1.1: the actions that strike with a body or a weapon.
bool meleeAction(int32_t action);

// §5.2: the strike base W, and the magic base M (W without the action's own factor).
float strikeBase(const Combatant &c);
float magicBase(const Combatant &c);
// The striking list (0x03DF60, attack-routine contract §5.4): the weapons a blow counts, in
// order; the second is empty when one is enough.
void strikingWeapons(const Combatant &c, const Item *(&first), const Item *(&second));

// Strike-call-sites contract S5-S9: whether the action strikes in the step that starts at
// action time `t` and lasts `dtMs`, and whether the area-heal flag is set. Also clears the
// struck and deferred sets when due and runs the deferred-key pass (Combatant::counterPending).
struct StrikeWindow {
    bool strikes = false, areaHeal = false;
    operator bool() const { return strikes; }
};
StrikeWindow strikeWindow(Combatant &c, float t, float dtMs);

// K2: the reach R, the strike centre and the geometric test's start point (raw units).
struct StrikeGeometry {
    float reach = 0;
    std::array<int64_t, 3> centre{}, start{};
};
StrikeGeometry strikeGeometry(const Combatant &attacker, const Body &body, float yaw);
// K5 steps 2-3: the fixed-point horizontal and binary32 vertical reach tests.
bool strikeReaches(const StrikeGeometry &geometry, const Body &striker, const Body &target);

// The same tests from block positions. `sightFrom`, when given, receives the start point.
bool reachesTarget(const Combatant &attacker, glm::dvec3 position, float yaw, glm::vec3 box, glm::dvec3 target,
                   glm::vec3 targetBox, glm::dvec3 *sightFrom = nullptr);

// Roll Cw (0x0441A0 / 0x00F220): one draw against the probability scaled by the main hand.
bool procChance(const Combatant &source, float probability, const std::function<int()> &draw);

// §2, §3.1, §4: the blow against one target: critical, spread, amount, knock-back, area and
// magic flags. Draws in the order of §2.3.
HitInput strikeInput(const Combatant &attacker, glm::dvec3 position, glm::vec3 box, glm::dvec3 target,
                     glm::vec3 targetBox, const std::function<int()> &draw);
// Runtime path: fixed-point subtraction precedes binary32 displacement conversion.
HitInput strikeInput(const Combatant &attacker, const Body &body, const Body &target,
                     const std::function<int()> &draw);

// §1.7: the blow of a beam.
HitInput beamInput(const Combatant &attacker, const std::function<int()> &draw);
// The same with the critical decided by the caller (the ray case draws it before admission).
HitInput beamInput(const Combatant &attacker, bool critical, const std::function<int()> &draw);

// Strike-call-sites §5: the work after an accepted strike. `first`: the struck set was empty
// and the striker not stunned; `hitAny`: the frame's earlier success (updated here).
struct StrikeAftermath {
    std::optional<BuffEvent> buff; // action 0x1E's kind-9 effect notice
    bool upgradeSound = false;      // record 47 at the striker
};
StrikeAftermath afterStrike(Combatant &attacker, bool first, bool &hitAny, const std::function<int()> &draw);

// §6: the area heal record of actions 0x20/0x21 (two draws: the roll, then the critical).
HitRecord areaHealRecord(const Combatant &attacker, uint64_t target, const std::array<int64_t, 3> &position,
                         const std::function<int()> &draw);

} // namespace creatures
