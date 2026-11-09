#include "creatures/Relationships.h"
#include "creatures/Combat.h"
#include <cstring>

namespace creatures {

std::array<uint8_t, 24> RelationshipRecord::serialize() const {
    std::array<uint8_t, 24> bytes;
    std::memcpy(bytes.data(), &target, 8);
    std::memcpy(bytes.data() + 8, &source, 8);
    std::memcpy(bytes.data() + 16, &value, 4);
    std::memcpy(bytes.data() + 20, opaque.data(), 4);
    return bytes;
}

RelationshipRecord RelationshipRecord::parse(const uint8_t *bytes) {
    RelationshipRecord record;
    std::memcpy(&record.target, bytes, 8);
    std::memcpy(&record.source, bytes + 8, 8);
    std::memcpy(&record.value, bytes + 16, 4);
    std::memcpy(record.opaque.data(), bytes + 20, 4);
    return record;
}

void receiveRelationships(std::span<const RelationshipRecord> records,
                          const std::function<Combatant *(uint64_t)> &lookup) {
    for (const RelationshipRecord &record : records) {
        if (!lookup(record.target) || !lookup(record.source) || record.target == record.source)
            continue;
        if (Combatant *target = lookup(record.target))
            target->relations.threats[record.source] = record.value;
    }
}

} // namespace creatures
