#pragma once
// How a melee action becomes hits (docs/specs/alpha-open-points-6.md): when in the action it
// tests for targets, what it reaches, and how much a hit is worth. Used for creatures and for
// the player alike; the hit itself is resolved by resolveHit (behaviour §5).
#include "creatures/Combat.h"
#include <functional>

namespace creatures {

struct Body;

// Behaviour §13.2: the big types (0x4445d0), which never dodge and shrug off most stuns.
bool bigCreature(int32_t type);

// §1.1: the actions that strike with a body or a weapon.
bool meleeAction(int32_t action);

// §5.2: the strike base W, and the magic base M (W without the action's own factor).
float strikeBase(const Combatant &c);
float magicBase(const Combatant &c);

// §1.3: whether the action strikes in the step that starts at action time `t` and lasts `dtMs`.
// Also forgets the targets of the last swing or pulse (§1.5) when that is due.
bool strikeWindow(Combatant &c, float t, float dtMs);

// §1.4, §1.5: whether a target's box is within the attacker's reach. `sightFrom`, when given,
// receives the point the line of sight is taken from.
bool reachesTarget(const Combatant &attacker, glm::dvec3 position, float yaw, glm::vec3 box, glm::dvec3 target,
                   glm::vec3 targetBox, glm::dvec3 *sightFrom = nullptr);

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

// §1.6, §2.6: what the first target of a swing or pulse gives the attacker: the combo, and mana
// unless the blow was an area one. `damaged`: the hit was not blocked or refused.
void afterFirstTarget(Combatant &attacker, bool area, const std::function<int()> &draw);

} // namespace creatures
