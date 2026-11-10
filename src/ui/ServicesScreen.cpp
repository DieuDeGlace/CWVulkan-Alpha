// The identifier's and the adapter's windows (docs/specs/alpha-item-services.md §1, §2), right
// of the inventory: Leftovers identified into a random weapon or armour piece for a fee in
// copper, and gear raised (or lowered) to the player's level for platinum coins.
#include "Loot.h"
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <bit>

namespace ui {
namespace {

using namespace widgets;
using creatures::ItemRecord;

constexpr glm::vec2 IDENTIFY{350, 285}, ADAPT{350, 400};
glm::vec2 origin(glm::vec2 W) { return {W.x / 2 + 200 + 10, W.y / 2 - 285}; }
// §1.4: the left half of the bottom 30 px strip is the action, the right half "Goodbye!".
bool inAction(glm::vec2 local, glm::vec2 size) {
    return local.x > 0 && local.x < size.x / 2 && local.y > size.y - 30 && local.y < size.y;
}
bool inGoodbye(glm::vec2 local, glm::vec2 size) {
    return local.x > size.x / 2 && local.x < size.x && local.y > size.y - 30 && local.y < size.y;
}
const glm::vec4 GREY{0.7f, 0.7f, 0.7f, 1}, CYAN{0, 1, 1, 1};

} // namespace

void Interface::loadServices() {
    if (const int32_t n = gui.find("blackwidget"); n >= 0) {
        identifyPanel = clone(gui, n);
        resizeWidget(*identifyPanel, 0, IDENTIFY);
        adaptPanel = clone(gui, n);
        resizeWidget(*adaptPanel, 0, ADAPT);
    }
    if (const int32_t n = gui.find("itemframe"); n >= 0)
        itemFrame = clone(gui, n);
    if (const int32_t n = gui.find("rightarrow"); n >= 0)
        adaptArrow = clone(gui, n);
}

ItemRecord *Interface::identifySource() {
    if (!inventory || identifyBag < 0 || identifyBag >= (int)inventory->bags.size() ||
        identifySlot >= inventory->bags[identifyBag].size())
        return nullptr;
    creatures::Stack &s = inventory->bags[identifyBag][identifySlot];
    return !s.empty() && creatures::itemType(s.item) == 14 ? &s.item : nullptr;
}

ItemRecord *Interface::adaptSource() {
    if (!inventory)
        return nullptr;
    ItemRecord *item = nullptr;
    if (adaptBox >= 0)
        item = &inventory->equipment[adaptBox];
    else if (adaptBag >= 0 && adaptBag < (int)inventory->bags.size() && adaptSlot < inventory->bags[adaptBag].size() &&
             !inventory->bags[adaptBag][adaptSlot].empty())
        item = &inventory->bags[adaptBag][adaptSlot].item;
    const uint8_t type = item ? creatures::itemType(*item) : 0;
    return item && type >= 3 && type <= 9 && !((*item)[0x0e] & 1) ? item : nullptr;
}

int32_t Interface::adaptPrice() {
    const ItemRecord *item = adaptSource();
    if (!item)
        return 0;
    const int16_t level = creatures::itemLevel(*item);
    const uint8_t rarity = creatures::itemRarity(*item);
    return rebalancedAdaptionCost(level, rarity, sheet.level);
}

void Interface::drawServices(Painter &painter, glm::vec2 W, glm::vec2 mouse) {
    const glm::vec2 O = origin(W), local = mouse - O;
    const auto frame = [&](glm::vec2 size) {
        if (itemFrame) {
            placeWidget(*itemFrame, 0, O + glm::vec2((size.x - 100) / 2, 50));
            painter.draw(*itemFrame, 0, Transform(1));
        }
    };
    const auto buttons = [&](glm::vec2 size, const char16_t *action, bool valid) {
        outlined(painter, action, O + glm::vec2(size.x / 3, size.y - 20), 12, 3,
                 !valid ? GREY : inAction(local, size) ? CYAN : glm::vec4(1), plasma::CENTRE);
        outlined(painter, u"Goodbye!", O + glm::vec2(2 * size.x / 3, size.y - 20), 12, 3,
                 inGoodbye(local, size) ? CYAN : glm::vec4(1), plasma::CENTRE);
    };
    if (identifyOpen) {
        // §1.4.
        if (identifyPanel) {
            placeWidget(*identifyPanel, 0, O);
            painter.draw(*identifyPanel, 0, Transform(1));
        }
        frame(IDENTIFY);
        const ItemRecord *v = identifySource();
        if (v) {
            drawItem(painter, W, *v, O + glm::vec2(175, 100), 0.06f);
            drawItemText(painter, O + glm::vec2(15, 180), *v, 300, false);
        }
        outlined(painter, u"Identification", O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
        buttons(IDENTIFY, u"Identify", v != nullptr);
        if (v) {
            const int32_t fee = identificationFee(*v);
            outlined(painter, u"COST:", O + glm::vec2(15, 235), 10, 2, glm::vec4(1));
            outlined(painter, number(fee % 100) + u" C ", O + glm::vec2(220, 235), 10, 2, {0.8f, 0.5f, 0, 1}, plasma::RIGHT);
            outlined(painter, number(fee / 100 % 100) + u" S ", O + glm::vec2(180, 235), 10, 2, GREY, plasma::RIGHT);
            outlined(painter, number(fee / 10000) + u" G ", O + glm::vec2(140, 235), 10, 2, {1, 0.9f, 0, 1}, plasma::RIGHT);
        }
    } else if (adaptOpen) {
        // §2.3.
        if (adaptPanel) {
            placeWidget(*adaptPanel, 0, O);
            painter.draw(*adaptPanel, 0, Transform(1));
        }
        frame(ADAPT);
        const ItemRecord *s = adaptSource();
        if (s) {
            drawItem(painter, W, *s, O + glm::vec2(175, 100), 0.06f);
            drawItemText(painter, O + glm::vec2(15, 180), *s, 300, false);
            ItemRecord adapted = *s; // the preview at the player's level
            creatures::setItemLevel(adapted, (int16_t)sheet.level);
            drawItemText(painter, O + glm::vec2(225, 180), adapted, 300, false);
            if (adaptArrow) {
                placeWidget(*adaptArrow, 0, O + glm::vec2(165, 230));
                painter.draw(*adaptArrow, 0, Transform(1));
            }
        }
        outlined(painter, u"Adaption", O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
        buttons(ADAPT, u"Adapt", s != nullptr);
        outlined(painter, number(adaptPrice()) + u" Platinum Coins ", O + glm::vec2(220, 350), 10, 2.5f,
                 {0.5f, 0.2f, 1, 1}, plasma::RIGHT);
        outlined(painter, u"COST:", O + glm::vec2(15, 350), 10, 2, glm::vec4(1));
    }
}

bool Interface::servicesPress(glm::vec2 W, glm::vec2 mouse, int button) {
    if (!inventory || (!identifyOpen && !adaptOpen))
        return false;
    const glm::vec2 local = mouse - origin(W);
    const glm::vec2 size = identifyOpen ? IDENTIFY : ADAPT;
    if (!inside(local, {0, 0}, size))
        return false;
    // "Goodbye!" closes (Alpha does it on any button's release; here on the press).
    if (inGoodbye(local, size)) {
        identifyOpen = adaptOpen = false;
        identifyBag = adaptBag = adaptBox = -1;
        return true;
    }
    if (button != 0 || !inAction(local, size))
        return true;
    if (identifyOpen) {
        // §1.6: pay, take one Leftovers, and add what it turns into.
        ItemRecord *v = identifySource();
        if (!v)
            return true;
        const uint32_t fee = (uint32_t)identificationFee(*v);
        if (fee > inventory->money)
            return true;
        inventory->money -= fee;
        const ItemRecord leftovers = *v;
        creatures::Stack &slot = inventory->bags[identifyBag][identifySlot];
        if (--slot.count < 1)
            slot.count = 0, slot.item[0x00] = slot.item[0x01] = 0;
        const ItemRecord made = identify(leftovers, serviceRandom);
        inventory->add(made);
        inventorySounds.push_back(0x2e); // disenchant2.wav
        chatAdd("You receive ", {255, 255, 255});
        std::string name;
        for (char16_t ch : itemName ? itemName(made) : std::u16string())
            name += ch < 0x80 ? (char)ch : '?';
        chatAdd(name + " +" + std::to_string(creatures::powerOf(creatures::itemLevel(made))), creatures::nameColour(made) * 255.f);
        chatAdd(".\n", {255, 255, 255});
    } else {
        // §2.5: pay in platinum, set the player's level in place and mark it adapted.
        if (!adaptSource())
            return true;
        const int32_t comparisonPrice = adaptPrice();
        if (std::bit_cast<int32_t>(inventory->platinum) < comparisonPrice)
            return true;
        inventory->platinum -= uint32_t(adaptPrice());
        const int16_t level = std::bit_cast<int16_t>(uint16_t(sheet.level));
        ItemRecord *levelItem = adaptSource();
        if (!levelItem)
            return true; // retained debit; invalidated selection has no rollback policy
        creatures::setItemLevel(*levelItem, level);
        ItemRecord *flagItem = adaptSource();
        if (!flagItem)
            return true;
        (*flagItem)[0x0e] |= 1;
        inventorySounds.push_back(0x2e);
        equipmentChanged = true;
    }
    return true;
}

} // namespace ui
