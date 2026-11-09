#include "net/RemoteFighter.h"
#include <bit>
#include <utility>

namespace net {

void RemoteFighter::refresh(creatures::Combatant snapshot, int32_t elapsedMs) {
    const int32_t action = snapshot.action, clock = snapshot.actionMs;
    const bool changedClock = action != receivedAction || clock != receivedClock;
    const int32_t displayedClock = changedClock ? clock :
        std::bit_cast<int32_t>(uint32_t(fighter.actionMs) + uint32_t(elapsedMs));
    auto residentBuffs = std::move(fighter.buffs);
    auto relations = std::move(fighter.relations);
    fighter = std::move(snapshot);
    fighter.buffs = std::move(residentBuffs); // Resident events are outside core snapshots.
    fighter.relations = std::move(relations);
    fighter.actionMs = displayedClock;
    receivedAction = action;
    receivedClock = clock;
}

} // namespace net
