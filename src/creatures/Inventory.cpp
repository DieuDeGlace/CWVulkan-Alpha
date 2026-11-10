#include "creatures/Inventory.h"
#include "creatures/Combat.h"
#include "worldgen/Zone.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace creatures {

ItemRecord toRecord(const worldgen::ItemRecord &item) {
    ItemRecord r{};
    r[0x00] = item.type;
    r[0x01] = item.subtype;
    std::memcpy(r.data() + 0x04, &item.seed, 4);
    std::memcpy(r.data() + 0x08, &item.product, 4);
    r[0x0c] = item.rarity;
    r[0x0d] = item.material;
    r[0x0e] = item.byte0e;
    setItemLevel(r, item.level);
    return r;
}

uint8_t itemType(const ItemRecord &item) { return item[0x00]; }
uint8_t itemSubtype(const ItemRecord &item) { return item[0x01]; }
uint8_t itemRarity(const ItemRecord &item) { return item[0x0c]; }
uint8_t itemMaterial(const ItemRecord &item) { return item[0x0d]; }

int16_t itemLevel(const ItemRecord &item) {
    int16_t v;
    std::memcpy(&v, item.data() + 0x10, 2);
    return v;
}

void setItemLevel(ItemRecord &item, int16_t value) { std::memcpy(item.data() + 0x10, &value, 2); }

static int32_t bodyCount(const ItemRecord &item) {
    int32_t n;
    std::memcpy(&n, item.data() + 0x114, 4);
    return n;
}

bool sameItem(const ItemRecord &a, const ItemRecord &b) {
    if (a[0x00] != b[0x00] || a[0x01] != b[0x01])
        return false;
    if (std::memcmp(a.data() + 0x04, b.data() + 0x04, 4) != 0 || a[0x0e] != b[0x0e] ||
        std::memcmp(a.data() + 0x08, b.data() + 0x08, 4) != 0 || a[0x0c] != b[0x0c] ||
        itemLevel(a) != itemLevel(b) || a[0x0d] != b[0x0d])
        return false;
    const int32_t n = bodyCount(a);
    if (n != bodyCount(b) || n > 32)
        return false;
    for (int32_t i = 0; i < n; i++) {
        const size_t entry = 0x14 + size_t(i) * 8;
        if (a[entry + 3] != b[entry + 3] || std::memcmp(a.data() + entry, b.data() + entry, 3) != 0)
            return false;
    }
    return true;
}

void copyItemFields(const ItemRecord &source, ItemRecord &destination) {
    destination[0] = source[0];
    destination[1] = source[1];
    std::memmove(destination.data() + 4, source.data() + 4, 8);
    std::memmove(destination.data() + 0x0c, source.data() + 0x0c, 3);
    std::memmove(destination.data() + 0x10, source.data() + 0x10, 2);
    std::memmove(destination.data() + 0x14, source.data() + 0x14, 0x104);
}

static void swapItemFields(ItemRecord &first, ItemRecord &second) {
    const ItemRecord saved = first;
    copyItemFields(second, first);
    copyItemFields(saved, second);
}

int bagOf(const ItemRecord &item) {
    const uint8_t t = itemType(item);
    if ((t >= 3 && t <= 9) || t == 23 || t == 24)
        return 0;
    if (t == 11)
        return 2;
    if (t == 19 || t == 20)
        return 3;
    return 1;
}

bool stackable(uint8_t type) {
    switch (type) {
    case 1: case 10: case 11: case 12: case 13: case 21: return true;
    default: return false;
    }
}

bool quantityType(const ItemRecord &item) {
    switch (itemType(item)) {
    case 0: case 12: case 13: case 20: case 21: case 23: case 24: case 25: return true;
    case 11: return itemSubtype(item) != 14;
    default: return false;
    }
}

bool twoHanded(const ItemRecord &item) {
    if (itemType(item) != 3)
        return false;
    switch (itemSubtype(item)) {
    case 5: case 6: case 7: case 8: case 10: case 11: case 15: case 16: case 17: case 18: return true;
    default: return false;
    }
}

int32_t powerOf(int32_t level) {
    const float g = 1.f - 1.f / ((float)(level - 1) * 0.05f + 1.f);
    return (int32_t)(g * 100.f + 1.f);
}

bool classMayUse(const ItemRecord &item, int32_t cls) {
    const uint8_t t = itemType(item);
    if (t == 2) { // a formula: the test of what it teaches
        if (item[0x08] == 2)
            return true;
        ItemRecord taught = item;
        taught[0x00] = item[0x08];
        return classMayUse(taught, cls);
    }
    int32_t needs = 0; // 0 anyone
    if (t == 3) {
        switch (itemSubtype(item)) {
        case 0: case 1: case 2: case 13: case 15: case 16: case 17: needs = 1; break;
        case 3: case 4: case 5: needs = 4; break;
        case 6: case 7: case 8: case 14: needs = 2; break;
        case 10: case 11: case 12: needs = 3; break;
        default: break;
        }
    } else if (t >= 4 && t <= 7) {
        switch (itemMaterial(item)) {
        case 1: case 5: case 18: case 22: needs = 1; break;
        case 19: case 26: needs = 2; break;
        case 23: case 25: needs = 3; break;
        case 27: needs = 4; break;
        default: break;
        }
    }
    return needs == 0 || needs == cls;
}

bool canUse(const ItemRecord &item, int32_t level, int32_t cls) {
    return powerOf(level) >= powerOf(itemLevel(item)) && classMayUse(item, cls);
}

glm::vec3 nameColour(const ItemRecord &item) {
    switch (itemType(item)) {
    case 21: return {0.5f, 0.5f, 0.5f};
    case 13: return {0.5f, 0.1f, 1};
    case 25: return {0.2f, 0.5f, 1};
    default: break;
    }
    switch (itemRarity(item)) {
    case 0: return {1, 1, 1};
    case 1: return {0, 1, 0};
    case 2: return {0.25f, 0.25f, 1};
    case 3: return {0.5f, 0, 1};
    case 4: return {1, 1, 0};
    default: return {1, 0, 0};
    }
}

const std::array<EquipmentBox, 12> equipmentBoxes = {{
    {u"Left\nWeapon", OFF_HAND, {3, 0}},     {u"Right\nWeapon", MAIN_HAND, {3, 0}},
    {u"Left Ring", RING_LEFT, {9, 0}},       {u"Right Ring", RING_RIGHT, {9, 0}},
    {u"Neck", AMULET_SLOT, {8, 0}},          {u"Shoulder", SHOULDERS_SLOT, {7, 0}},
    {u"Chest", CHEST_SLOT, {4, 0}},          {u"Hands", GLOVES_SLOT, {5, 0}},
    {u"Feet", BOOTS_SLOT, {6, 0}},           {u"Light", 10, {24, 0}},
    {u"Special", 11, {23, 0}},               {u"Pet", 12, {19, 20}},
}};

// Emptying a slot clears its count and the item's type and subtype (§8.1.1).
static void clearItem(ItemRecord &item) { item[0x00] = item[0x01] = 0; }

static void takeOne(Stack &s) {
    s.count = std::bit_cast<int32_t>(uint32_t(s.count) - 1u);
    if (s.count < 1) {
        s.count = 0;
        clearItem(s.item);
    }
}

Inventory Inventory::fresh() {
    Inventory inv;
    inv.bags.assign(4, std::vector<Stack>(50));
    return inv;
}

void Inventory::add(const ItemRecord &item) {
    const uint8_t t = itemType(item);
    if (t == 0)
        return;
    const int b = bagOf(item);
    if ((int)bags.size() <= b)
        bags.resize(b + 1); // automatic group admission precedes currency credit too
    ItemRecord stored = item;
    int32_t q = 1;
    if (quantityType(item)) {
        q = itemLevel(item);
        setItemLevel(stored, 1);
    }
    if (t == 12) { // coins: the kind is the material (world items spec, correcting screens §8.1.1)
        const uint8_t kind = itemMaterial(item);
        if (kind == 10 || kind == 11 || kind == 12) {
            const uint32_t unit = kind == 10 ? 1u : kind == 11 ? 10000u : 100u;
            money += uint32_t(q) * unit;
            return;
        }
    }
    if (t == 13) {
        platinum += q;
        return;
    }
    std::vector<Stack> &bag = bags[b];
    Stack *firstEmpty = nullptr;
    for (Stack &s : bag) {
        if (s.empty()) {
            if (!firstEmpty)
                firstEmpty = &s;
        }
        if (stackable(t) && sameItem(s.item, stored)) {
            s.count = std::bit_cast<int32_t>(uint32_t(s.count) + uint32_t(q));
            return;
        }
    }
    if (!firstEmpty)
        firstEmpty = &bag.emplace_back();
    firstEmpty->count = q;
    copyItemFields(stored, firstEmpty->item);
}

void Inventory::store(const ItemRecord &item) {
    if (itemType(item) != 0)
        add(item);
}

bool Inventory::canCarry(const ItemRecord &item) const {
    const uint8_t t = itemType(item);
    if (t != 1 && t != 11 && t != 20)
        return true;
    uint32_t total = 0;
    if (sameItem(hand.item, item))
        total += uint32_t(hand.count);
    for (const auto &bag : bags)
        for (const Stack &s : bag)
            if (sameItem(s.item, item))
                total += uint32_t(s.count);
    const int32_t n = std::bit_cast<int32_t>(total);
    return !(t == 20 && n >= 1) && n < 50;
}

int32_t Inventory::pressSlot(int b, size_t index) {
    if (b < 0 || b >= (int)bags.size() || index >= bags[b].size())
        return 0;
    Stack &slot = bags[b][index];
    if (!hand.empty() && bagOf(hand.item) != b)
        return SOUND_REFUSED;
    if (!hand.empty() && stackable(itemType(hand.item)) && sameItem(hand.item, slot.item)) {
        slot.count = std::bit_cast<int32_t>(uint32_t(slot.count) + uint32_t(hand.count));
        hand.count = 0;
        clearItem(hand.item);
        return 0;
    }
    std::swap(slot.count, hand.count);
    swapItemFields(slot.item, hand.item);
    return hand.empty() ? SOUND_PUT_DOWN : SOUND_GRAB;
}

int32_t Inventory::pressBox(int box, int32_t level, int32_t cls) {
    const EquipmentBox &e = equipmentBoxes[box];
    ItemRecord &slot = equipment[e.slot];
    if (!hand.empty() && !canUse(hand.item, level, cls))
        return SOUND_REFUSED;
    const uint8_t t = itemType(hand.item);
    if (!hand.empty() && t != e.accepts[0] && (e.accepts[1] == 0 || t != e.accepts[1]))
        return 0;
    if (e.slot == MAIN_HAND && !hand.empty() && twoHanded(hand.item) && itemType(equipment[OFF_HAND])) {
        add(equipment[OFF_HAND]);
        clearItem(equipment[OFF_HAND]);
    }
    if (e.slot == OFF_HAND && !hand.empty()) {
        const uint8_t sub = itemSubtype(hand.item);
        if (twoHanded(hand.item) && sub != BOW && sub != CROSSBOW)
            return SOUND_REFUSED;
    }
    if (e.slot == OFF_HAND && ((!hand.empty() && twoHanded(hand.item)) || twoHanded(equipment[MAIN_HAND]))) {
        store(equipment[MAIN_HAND]);
        clearItem(equipment[MAIN_HAND]);
    }
    if (e.slot == 12 && t == 20 && hand.count >= 2) { // one piece of pet food from the stack
        if (!sameItem(hand.item, slot)) {
            store(slot);
            copyItemFields(hand.item, slot);
            takeOne(hand);
        }
        return SOUND_GRAB;
    }
    if (e.slot == 12)
        swapItemFields(slot, hand.item);
    else
        std::swap(slot, hand.item);
    hand.count = itemType(hand.item) ? 1 : 0;
    return hand.empty() ? SOUND_PUT_DOWN : SOUND_GRAB;
}

UseOutcome Inventory::useSlot(int b, size_t index, bool middle, int32_t level, int32_t cls, bool canAct) {
    UseOutcome out;
    if (b < 0 || b >= (int)bags.size() || index >= bags[b].size() || bags[b][index].empty())
        return out;
    const ItemRecord item = bags[b][index].item;
    if (!canUse(item, level, cls)) {
        out.sound = SOUND_REFUSED;
        return out;
    }
    const auto take = [&] { takeOne(bags[b][index]); }; // reacquire after a possible bag allocation
    const auto equip = [&](int slot) {
        take();
        const ItemRecord old = equipment[slot];
        equipment[slot] = item;
        store(old);
        out.sound = SOUND_PUT_DOWN;
    };
    const auto equipLinked = [&] {
        const ItemRecord old = equipment[12];
        store(old);
        copyItemFields(item, equipment[12]);
        take();
        out.sound = SOUND_PUT_DOWN;
    };
    const auto remove = [&](int slot) {
        const ItemRecord old = equipment[slot];
        clearItem(equipment[slot]);
        store(old);
    };
    const uint8_t t = itemType(item), sub = itemSubtype(item);
    switch (t) {
    case 3:
        if (sub == SHIELD || sub == QUIVER) {
            if (twoHanded(equipment[MAIN_HAND]))
                remove(MAIN_HAND);
            equip(OFF_HAND);
        } else if (!middle && (sub == BOW || sub == CROSSBOW)) {
            equip(OFF_HAND);
            remove(MAIN_HAND);
        } else if (middle && !twoHanded(item)) {
            equip(OFF_HAND);
        } else {
            equip(MAIN_HAND);
            if (twoHanded(item))
                remove(OFF_HAND);
        }
        break;
    case 19:
        equipLinked();
        break;
    case 20:
        if (!sameItem(item, equipment[12]))
            equipLinked();
        break;
    case 4: equip(CHEST_SLOT); break;
    case 5: equip(GLOVES_SLOT); break;
    case 6: equip(BOOTS_SLOT); break;
    case 7: equip(SHOULDERS_SLOT); break;
    case 8: equip(AMULET_SLOT); break;
    case 24: equip(10); break;
    case 9: equip(middle ? RING_LEFT : RING_RIGHT); break;
    case 23:
        equip(11);
        out.clearAction = true;
        break;
    case 1:
        if (canAct) {
            take();
            out.consume = item;
        }
        break;
    case 2: {
        // Crafting §1.1: the recipe key is the formula with its product as the type.
        ItemRecord key = item;
        key[0x00] = item[0x08];
        std::fill(key.begin() + 0x08, key.begin() + 0x0c, 0);
        for (const ItemRecord &r : recipes)
            if (sameItem(r, key)) {
                out.sound = SOUND_NOT_HERE;
                return out;
            }
        recipes.push_back(key);
        take();
        out.sound = SOUND_RECIPE_LEARNT;
        break;
    }
    default:
        // Type 11 with count > 1 re-adds one item with its +0x10 field raised (§8.1.8, purpose
        // not clear); not carried out.
        break;
    }
    return out;
}

int32_t Inventory::unequip(int box) {
    ItemRecord &slot = equipment[equipmentBoxes[box].slot];
    if (itemType(slot) == 0)
        return 0;
    const ItemRecord old = slot;
    clearItem(slot);
    add(old);
    return SOUND_PUT_DOWN;
}

std::optional<ItemRecord> Inventory::dropOne() {
    if (hand.empty())
        return std::nullopt;
    const ItemRecord item = hand.item;
    takeOne(hand);
    return item;
}

std::vector<std::pair<int, size_t>> Inventory::quickItems(int32_t level) const {
    std::vector<std::pair<int, size_t>> out;
    for (size_t b = 0; b < bags.size(); b++)
        for (size_t i = 0; i < bags[b].size(); i++) {
            const Stack &s = bags[b][i];
            if (!s.empty() && itemType(s.item) == 1 && itemLevel(s.item) <= level)
                out.push_back({(int)b, i});
        }
    return out;
}

} // namespace creatures
