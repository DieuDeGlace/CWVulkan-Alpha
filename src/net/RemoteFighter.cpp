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
    const uint64_t linkedId = fighter.linkedId;
    const auto itemSnapshot = fighter.linkedItem.retained;
    constexpr std::array<size_t, 5> gaps{2, 3, 15, 18, 19};
    std::array<std::array<uint8_t, gaps.size()>, creatures::SLOT_COUNT> padding;
    for (size_t slot = 0; slot < padding.size(); slot++)
        for (size_t gap = 0; gap < gaps.size(); gap++)
            padding[slot][gap] = fighter.equipmentSource(slot)[gaps[gap]];
    fighter = std::move(snapshot);
    fighter.buffs = std::move(residentBuffs); // Resident events are outside core snapshots.
    fighter.relations = std::move(relations);
    fighter.linkedId = linkedId; // This identifier is outside the native core state copy.
    fighter.linkedItem.retained = itemSnapshot;
    for (size_t slot = 0; slot < padding.size(); slot++)
        for (size_t gap = 0; gap < gaps.size(); gap++)
            fighter.equipmentSource(slot)[gaps[gap]] = padding[slot][gap];
    fighter.actionMs = displayedClock;
    receivedAction = action;
    receivedClock = clock;
}

} // namespace net
