#include "NewCharacter.h"
#include "creatures/Combat.h"
#include "creatures/Crafting.h"
#include "creatures/Inventory.h"
#include <cstring>

int32_t creatureTypeOf(int32_t race, bool female) {
    static const int32_t BASE[8] = {2, 0, 9, 11, 4, 7, 15, 13};
    return (race >= 0 && race < 8 ? BASE[race] : 2) + (female ? 1 : 0);
}

namespace {

// §3.9: faces and haircuts per creature type (human male for any other type).
std::pair<int32_t, int32_t> counts(int32_t type) {
    switch (type) {
    case 0: return {4, 10};
    case 1: return {6, 10};
    case 3: return {6, 7};
    case 4: case 5: return {5, 6};
    case 7: return {2, 6};
    case 8: return {5, 6};
    case 9: return {5, 3};
    case 10: return {5, 5};
    case 11: return {5, 10};
    case 12: return {5, 4};
    case 13: return {5, 5};
    case 14: return {4, 4};
    case 15: case 16: return {6, 6};
    default: return {6, 15};
    }
}

creatures::ItemRecord item(uint8_t type, uint8_t subtype, uint8_t material, int16_t level) {
    creatures::ItemRecord r{};
    r[0x00] = type, r[0x01] = subtype, r[0x0d] = material;
    creatures::setItemLevel(r, level);
    return r;
}

} // namespace

int32_t faceCount(int32_t type) { return counts(type).first; }
int32_t haircutCount(int32_t type) { return counts(type).second; }

std::array<uint8_t, 20> choiceBytes(const creatures::CharacterChoice &c) {
    std::array<uint8_t, 20> out{};
    std::memcpy(out.data() + 0x00, &c.race, 4);
    out[0x04] = c.female ? 1 : 0;
    std::memcpy(out.data() + 0x08, &c.head, 4);
    std::memcpy(out.data() + 0x0c, &c.hair, 4);
    std::memcpy(out.data() + 0x10, c.hairColour, 3);
    return out;
}

CharacterRecord newCharacter(const std::string &name, const creatures::CharacterChoice &choice, int32_t cls,
                             const std::function<int()> &draw) {
    CharacterRecord r;
    r.name = name;
    r.level = 1;
    r.characterClass = (uint8_t)cls;
    r.specialization = 0;
    r.extra = choiceBytes(choice);
    // §3.10: equipment. Main hand by class, the rogue's second dagger, two rings and a lantern.
    static const std::array<uint8_t, 2> MAIN[5] = {{0, 0}, {17, 2}, {6, 2}, {10, 2}, {3, 1}};
    const auto &[mainSub, mainMaterial] = MAIN[cls >= 1 && cls <= 4 ? cls : 1];
    r.equipment[creatures::MAIN_HAND] = item(3, mainSub, mainMaterial, 1);
    if (cls == 4)
        r.equipment[creatures::OFF_HAND] = item(3, 3, 1, 1);
    r.equipment[creatures::RING_LEFT] = item(9, 0, 12, 0);
    r.equipment[creatures::RING_RIGHT] = item(9, 0, 11, 0);
    r.equipment[10] = item(24, 0, 1, 1);
    // The inventory, in this order through the add rule (the ranger's wand is Alpha's own).
    creatures::Inventory inv = creatures::Inventory::fresh();
    switch (cls) {
    case 1: inv.add(item(3, 0, 1, 1)), inv.add(item(3, 0, 1, 1)), inv.add(item(3, 13, 1, 1)); break;
    case 2: inv.add(item(3, 7, 2, 1)), inv.add(item(3, 11, 2, 1)); break;
    case 3: inv.add(item(3, 12, 11, 1)), inv.add(item(3, 12, 11, 1)), inv.add(item(3, 11, 2, 1)); break;
    case 4: inv.add(item(3, 4, 1, 1)), inv.add(item(3, 4, 1, 1)), inv.add(item(3, 5, 1, 1)); break;
    default: break;
    }
    for (int i = 0; i < 5; i++)
        inv.add(item(1, 1, 0, 1));
    for (const auto &bag : inv.bags) {
        auto &tab = r.tabs.emplace_back();
        for (const creatures::Stack &s : bag)
            tab.push_back({(uint32_t)s.count, s.item});
    }
    // The level-1 recipes, and full health.
    std::vector<creatures::ItemRecord> recipes;
    creatures::grantLevel(recipes, 1, cls, draw);
    r.recipes.assign(recipes.begin(), recipes.end());
    creatures::Combatant c;
    c.characterClass = (uint8_t)cls;
    c.level = 1;
    for (size_t slot = 0; slot < r.equipment.size(); slot++)
        c.setEquipment(slot, r.equipment[slot]);
    r.health = creatures::maxHealth(c);
    r.skills.assign(11, 0);
    return r;
}
