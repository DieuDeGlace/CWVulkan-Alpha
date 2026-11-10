#pragma once
// A player's belongings and the rules that move them (docs/specs/alpha-ui-screens.md §8.1):
// the bags, the item on the mouse cursor, the equipment slots, money, and what a click on a
// bag slot or an equipment box does.
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

namespace worldgen {
struct ItemRecord;
}

namespace creatures {

// alpha-net-packets.md §1.1: an item record, 280 bytes; type 0 is no item.
using ItemRecord = std::array<uint8_t, 280>;

// A bag slot or the hand: a count and an item. Empty when the count is 0.
struct Stack {
    int32_t count = 0;
    ItemRecord item{};
    bool empty() const { return count == 0; }
};

// Sounds the inventory plays (§8.1.11).
enum InventorySound : int32_t {
    SOUND_PICKUP = 0x2d,
    SOUND_RECIPE_LEARNT = 0x2f,
    SOUND_NOT_HERE = 0x31,
    SOUND_REFUSED = 0x32,
    SOUND_TAB = 0x56,
    SOUND_GRAB = 0x57,
    SOUND_PUT_DOWN = 0x58,
};

// The generator's item (worldgen/Zone.h) as a full record.
ItemRecord toRecord(const worldgen::ItemRecord &item);

// Fields of an item record.
uint8_t itemType(const ItemRecord &item);
uint8_t itemSubtype(const ItemRecord &item);
uint8_t itemRarity(const ItemRecord &item);
uint8_t itemMaterial(const ItemRecord &item);
int16_t itemLevel(const ItemRecord &item); // also the quantity of the quantity types
void setItemLevel(ItemRecord &item, int16_t value);

// Held alpha-inventory-equality-consumers.md: fields and active entry prefixes.
// Positive body counts above owned capacity compare unequal as engine policy.
bool sameItem(const ItemRecord &a, const ItemRecord &b);
// Selected-field assignment retains the destination's five alignment bytes and
// copies every body entry regardless of the signed body count. Self-copy is valid.
void copyItemFields(const ItemRecord &source, ItemRecord &destination);
// 0x47fa30: the bag an item goes to (0 Equipment, 1 Items, 2 Ingredients, 3 Pets).
int bagOf(const ItemRecord &item);
// 0x47f9f0: items that pile up in one slot.
bool stackable(uint8_t type);
// §8.1.1 step 2: items whose +0x10 field is a quantity rather than a level.
bool quantityType(const ItemRecord &item);
// 0x444820.
bool twoHanded(const ItemRecord &item);
// 0x4c76a0 / 0x445f10: power shown for a level, trunc(g(L)·100 + 1).
int32_t powerOf(int32_t level);
// 0x4c6f20: whether a class (1 warrior … 4 rogue) may use the item.
bool classMayUse(const ItemRecord &item, int32_t characterClass);
// 0x43e420.
bool canUse(const ItemRecord &item, int32_t level, int32_t characterClass);
// 0x4c76e0: the price in copper.
int32_t price(const ItemRecord &item);
// 0x4c7d20: the colour of an item's name.
glm::vec3 nameColour(const ItemRecord &item);

// §8.1.5: the twelve equipment boxes, left column even, right column odd.
struct EquipmentBox {
    const char16_t *label;
    int slot;                    // equipment slot (models spec §6.4)
    std::array<uint8_t, 2> accepts; // item types, 0 when only one
};
extern const std::array<EquipmentBox, 12> equipmentBoxes;

// What a use-or-equip asked of the game besides moving items.
struct UseOutcome {
    int32_t sound = 0;
    std::optional<ItemRecord> consume; // a type-1 item to use (§8.1.10)
    bool clearAction = false;          // a glider or boat was put on (action := 0)
};

struct Inventory {
    std::vector<std::vector<Stack>> bags;
    Stack hand; // the item on the mouse cursor; not saved
    std::array<ItemRecord, 13> equipment{};
    uint32_t money = 0;    // copper; 100 silver, 10 000 gold
    uint32_t platinum = 0; // platinum coins
    std::vector<ItemRecord> recipes; // formulas learnt (save field 18)

    // 0x4772b0: four bags of fifty empty slots.
    static Inventory fresh();

    // 0x46ebe0: puts an item into its bag (coins become money).
    void add(const ItemRecord &item);
    // 0x43e4a0: whether one more of `item` may be taken.
    bool canCarry(const ItemRecord &item) const;

    // §8.1.8, a left press on bag `bag`'s slot `index` (the window shows bag `bag`). Returns
    // the sound, 0 for none.
    int32_t pressSlot(int bag, size_t index);
    // §8.1.8, a left press on equipment box `box`.
    int32_t pressBox(int box, int32_t level, int32_t characterClass);
    // §8.1.8, a right (`middle` false) or middle press on a bag slot with no window open.
    UseOutcome useSlot(int bag, size_t index, bool middle, int32_t level, int32_t characterClass, bool canAct);
    // §8.1.8, a right press on an equipment box: the item goes to the bags.
    int32_t unequip(int box);
    // §8.1.8, a left press over nothing: one item leaves the hand to be dropped in the world.
    std::optional<ItemRecord> dropOne();

    // §8.1.10: the items the quick-item wheel offers, bag then slot order.
    std::vector<std::pair<int, size_t>> quickItems(int32_t level) const;

  private:
    void store(const ItemRecord &item); // into the bags if it is an item
};

} // namespace creatures
