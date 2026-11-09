#pragma once
#include "creatures/ActorMap.h"
#include <array>
#include <functional>
#include <span>

namespace creatures {

// World-owned resident state, separate from core creature snapshots and buffs.
// Keys retain all eight bits of each byte; map order is signed 64-bit.
struct Relations {
    uint64_t contact = 0;
    ActorMap<float> counts;
    ActorMap<float> threats;
};

struct RelationshipRecord {
    uint64_t target = 0, source = 0; // object IDs at the notification boundary
    float value = 0;
    std::array<uint8_t, 4> opaque{}; // received bytes survive; fresh engine storage is deterministic
    std::array<uint8_t, 24> serialize() const;
    static RelationshipRecord parse(const uint8_t *bytes); // complete 24-byte payload required
};

struct Combatant;
// Later frame assignment, independent of world-hit production. The lookup
// supplies the current world table under its frame protection.
void receiveRelationships(std::span<const RelationshipRecord> records,
                          const std::function<Combatant *(uint64_t)> &lookup);

} // namespace creatures
