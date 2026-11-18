#pragma once

#include "creatures/Combat.h"

namespace creatures {

// Late type-six transition. The source is the live actor after all received
// buffs and world pieces have been applied.
bool captureTamedPet(Combatant &local, ItemRecord &held, const Combatant &source, const HitRecord &hit);

} // namespace creatures
