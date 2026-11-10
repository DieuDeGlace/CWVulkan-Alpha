#include "creatures/Credits.h"
#include "creatures/Combat.h"
#include <bit>
#include <cstring>

namespace creatures {

std::array<uint8_t, 24> KillRecord::serialize() const {
    std::array<uint8_t, 24> bytes;
    std::memcpy(bytes.data(), &player, 8);
    std::memcpy(bytes.data() + 8, &victim, 8);
    std::memcpy(bytes.data() + 16, &victimType, 4);
    std::memcpy(bytes.data() + 20, &experience, 4);
    return bytes;
}

KillRecord KillRecord::parse(const uint8_t *bytes) {
    KillRecord record;
    std::memcpy(&record.player, bytes, 8);
    std::memcpy(&record.victim, bytes + 8, 8);
    std::memcpy(&record.victimType, bytes + 16, 4);
    std::memcpy(&record.experience, bytes + 20, 4);
    return record;
}

CreditResult receiveCredit(const KillRecord &record, const CreditWorld &world) {
    CreditResult result;
    Combatant *credited = world.lookup(record.player);
    Combatant *victim = world.lookup(record.victim);
    if (!credited || !victim)
        return result;

    if (Combatant *controlled = world.controlled(); victim == controlled) {
        const uint64_t key = controlled->linkedId;
        if (world.lookup(key)) {
            if (Combatant *linked = world.lookup(key)) {
                linked->health = 0;
                result.healthZeroed = linked;
            }
            if (Combatant *current = world.controlled())
                if (Combatant *linked = world.lookup(current->linkedId))
                    linked->relations.threats.clear();
        }
        controlled->relations.contact = 0;
    }
    if (Combatant *controlled = world.controlled(); controlled && credited == controlled && victim->controller == 1) {
        addExperience(*controlled, record.experience);
        result.directAward = controlled;
    }
    if (world.mode == 0)
        if (Combatant *linked = world.lookup(credited->linkedId); linked && linked->health > 0) {
            addExperience(*linked, record.experience);
            result.linkedAward = linked;
            const auto [source, snapshot] = world.linkedItems(*credited);
            std::memcpy(source.data() + 4, &linked->experience, 4);
            const int16_t level = std::bit_cast<int16_t>(uint16_t(linked->level));
            setItemLevel(source, level);
            credited->equipment[12].seed = uint32_t(linked->experience);
            credited->equipment[12].level = level;
            copyItemFields(source, snapshot);
        }
    return result;
}

} // namespace creatures
