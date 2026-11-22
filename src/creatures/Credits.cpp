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

CreditItems actorCreditItems(Combatant &actor) {
    actor.equipment[12].write(actor.linkedItem.source);
    return {actor.linkedItem.source, actor.linkedItem.retained.item};
}

std::array<uint8_t, 288> ItemGrant::serialize() const {
    std::array<uint8_t, 288> bytes;
    std::memcpy(bytes.data(), &recipient, 8);
    std::memcpy(bytes.data() + 8, item.data(), item.size());
    return bytes;
}

ItemGrant ItemGrant::parse(const uint8_t *bytes) {
    ItemGrant grant;
    std::memcpy(&grant.recipient, bytes, 8);
    std::memcpy(grant.item.data(), bytes + 8, grant.item.size());
    return grant;
}

void applyItemGrants(std::span<const ItemGrant> produced, std::span<const ItemGrant> received,
                     Combatant &controlled, Inventory &inventory, int32_t &manaCubes,
                     const std::function<Combatant *(uint64_t)> &lookup, const ItemGrantView &view) {
    for (const auto &grant : received)
        if (lookup(grant.recipient) == &controlled && itemType(grant.item) != 25) {
            inventory.add(grant.item);
            view.refreshInventory();
        }

    const auto results = [&](std::span<const ItemGrant> grants) {
        for (const auto &grant : grants) {
            const auto *recipient = lookup(grant.recipient);
            if (!recipient)
                continue;
            const bool local = recipient == &controlled;
            if (local)
                view.refreshCrafting();
            switch (itemType(grant.item)) {
            case 13:
                if (grant.recipient == controlled.id)
                    addExperience(controlled, 10 * int32_t(itemLevel(grant.item)));
                break;
            case 25:
                manaCubes = std::bit_cast<int32_t>(uint32_t(manaCubes) + 1u);
                controlled.action = 109;
                controlled.actionMs = 0;
                break;
            }
            view.present(grant, *recipient);
        }
    };
    results(produced);
    results(received);
}

void stampLinkedCredit(Combatant &actor, const Combatant &linked, CreditItems items) {
    std::memcpy(items.source.data() + 4, &linked.experience, 4);
    const int16_t level = std::bit_cast<int16_t>(uint16_t(linked.level));
    setItemLevel(items.source, level);
    actor.equipment[12].seed = uint32_t(linked.experience);
    actor.equipment[12].level = level;
    copyItemFields(items.source, items.snapshot);
    if (&items.source != &actor.linkedItem.source)
        copyItemFields(items.source, actor.linkedItem.source); // coherence with an owner's canonical inventory bridge
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
        if (Combatant *current = world.controlled())
            current->relations.contact = 0;
    }
    if (Combatant *controlled = world.controlled(); controlled && credited == controlled && victim->controller == 1) {
        controlled->accumulateExperience(record.experience);
        // The level helper takes the designated creature as it stands after the write.
        if (Combatant *current = world.controlled())
            applyEarnedLevels(*current);
        result.directAward = controlled;
    }
    if (world.mode == 0)
        if (Combatant *linked = world.lookup(credited->linkedId); linked && linked->health > 0) {
            addExperience(*linked, record.experience);
            result.linkedAward = linked;
            const auto items = world.linkedItems ? world.linkedItems(*credited) : actorCreditItems(*credited);
            stampLinkedCredit(*credited, *linked, items);
        }
    return result;
}

} // namespace creatures
