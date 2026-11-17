#pragma once
// Resident actor state with a separate receive boundary for the wire fields.
#include "creatures/Combat.h"

namespace net {

struct CreatureRecord;

struct RemoteFighter {
    creatures::Combatant fighter;
    void receive(uint64_t id, const CreatureRecord &record);
};

} // namespace net
