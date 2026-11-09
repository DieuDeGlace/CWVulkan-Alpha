// The inventory screen (docs/specs/alpha-ui-screens.md §8.1): the bag window with its tabs,
// scroll buttons and money row, the character panel, the twelve equipment boxes, the item on
// the cursor and the item tooltip, and what a mouse press on them does.
#include "ui/Interaction.h"
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include "creatures/Combat.h"
#include "creatures/Spirits.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace ui {
namespace {

using creatures::ItemRecord;
using namespace widgets;

// §8.1.2, §8.1.3: the bag window, 400 x 285, centred with its bottom on the screen's centre
// line; 8 x 5 slots of 40 px at a 45 px pitch.
constexpr glm::vec2 PANEL{400, 285};
constexpr int COLUMNS = 8, ROWS = 5;
constexpr float PITCH = 45, SLOT = 40;
constexpr glm::vec2 CHARACTER_AT{20, 340}, CHARACTER_SIZE{260, 200};
constexpr glm::vec2 TOOLTIP_SIZE{300, 300};

glm::vec2 panelOrigin(glm::vec2 W) { return {W.x / 2 - 200, W.y / 2 - 285}; }
glm::vec2 slotCorner(int c, int r) { return {10 + PITCH * (float)c, 40 + PITCH * (float)r}; }
glm::vec2 tabCorner(int k) { return {4 + 40.f * (float)k, -41.07f}; }
glm::vec2 upCorner() { return {PANEL.x - 30, 10}; }
glm::vec2 downCorner() { return {PANEL.x - 30, PANEL.y - 30}; }
constexpr glm::vec2 BUTTON{21, 19};

// §8.1.5: equipment box i's pivot, and its top-left corner (the right template's pivot is its
// top-left, the left template's its top-right).
glm::vec2 boxPivot(glm::vec2 W, int i) { return {W.x / 2 - 200 + 400.f * (float)(i % 2), W.y / 2 + 10 + 56.f * (float)(i / 2)}; }
glm::vec2 boxCorner(glm::vec2 W, int i) { return boxPivot(W, i) - glm::vec2(i % 2 ? 50.f : 0.f, 0); }

const glm::vec4 COPPER{0.8f, 0.5f, 0, 1}, SILVER{0.7f, 0.7f, 0.7f, 1}, GOLD{1, 0.9f, 0, 1}, PLATINUM{0.5f, 0.2f, 1, 1};
const glm::vec4 RED{1, 0, 0, 1}, STAT{0.75f, 0.5f, 1, 1}, CYAN{0, 1, 1, 1};

// §8.1.4: the colour band of a rating.
glm::vec4 band(float r) {
    if (r < 0.8f) return {0.7f, 0.7f, 0.7f, 1};
    if (r < 1.1f) return {1, 1, 1, 1};
    if (r < 1.2f) return {0, 1, 0, 1};
    if (r < 1.5f) return {0.25f, 0.25f, 1, 1};
    if (r < 1.8f) return {0.5f, 0, 1, 1};
    return {1, 1, 0, 1};
}

const char16_t *const CLASS_NAMES[5] = {u"", u" Warrior", u" Ranger", u" Mage", u" Rogue"};
const char16_t *const SPECIALIZATIONS[5][2] = {{u"", u""},
                                              {u" | Berserker", u" | Guardian"},
                                              {u" | Sniper", u" | Scout"},
                                              {u" | Fire", u" | Water"},
                                              {u" | Assassin", u" | Ninja"}};

} // namespace

void Interface::loadInventory() {
    if (const int32_t n = gui.find("blackwidget"); n >= 0) {
        inventoryPanel = clone(gui, n);
        resizeWidget(*inventoryPanel, 0, PANEL);
        characterPanel = clone(gui, n);
        resizeWidget(*characterPanel, 0, CHARACTER_SIZE);
    }
    if (const int32_t n = gui.find("itembox"); n >= 0)
        itemBox = clone(gui, n);
    if (const int32_t n = gui.find("upbutton"); n >= 0)
        upButton = clone(gui, n);
    if (const int32_t n = gui.find("downbutton"); n >= 0)
        downButton = clone(gui, n);
    if (const int32_t n = gui.find("scrollbutton"); n >= 0)
        scrollThumb = clone(gui, n);
    if (const int32_t n = gui.find("tab"); n >= 0)
        for (int k = 0; k < 4; k++)
            inventoryTabs.push_back(clone(gui, n));
    const int32_t right = gui.find("equipmentboxright"), left = gui.find("equipmentboxleft");
    if (right >= 0 && left >= 0)
        for (int i = 0; i < 12; i++) {
            equipmentBoxes.push_back(clone(gui, i % 2 ? left : right));
            setLabel(equipmentBoxes.back(), "info", creatures::equipmentBoxes[i].label);
            setLabel(equipmentBoxes.back(), "count", u"");
        }
    if (const int32_t n = gui.find("preview"); n >= 0)
        preview = clone(gui, n);
    if (const int32_t n = gui.find("equippedpreview"); n >= 0)
        equippedPreview = clone(gui, n);
}

// §8.1.3 (0x4c6350): how many scroll rows each bag has, the current rows clamped to them.
void Interface::scrollRange() {
    for (size_t b = 0; b < scrollRow.size(); b++) {
        const size_t m = inventory && b < inventory->bags.size() ? inventory->bags[b].size() : 0;
        const int32_t n = m == 0 ? 0 : std::max(1, (int32_t)((m - 1) / COLUMNS) - ROWS + 2);
        scrollPositions[b] = n;
        scrollRow[b] = std::clamp(scrollRow[b], 0, std::max(n - 1, 0));
    }
}

void Interface::drawItem(Painter &painter, glm::vec2 W, const ItemRecord &item, glm::vec2 at, float scale, float depth) {
    if (!itemModel || creatures::itemType(item) == 0)
        return;
    const ItemModel m = itemModel(item);
    const ItemDraw view = itemDraw(m.mesh, m.size, creatures::itemType(item) == 9, at, itemTurn, scale, depth, W);
    painter.item(view);
    // Item services §3.9: the attached cubes, one voxel each at their model cells.
    if (cubeMeshId && creatures::itemType(item) == 3) // (only weapons carry cubes; a pet item keeps its name there)
        for (int32_t k = 0; k < creatures::spiritCount(item); k++) {
            const creatures::SpiritCube cube = creatures::spiritAt(item, k);
            ItemDraw d = view;
            d.mesh = cubeMeshId;
            d.camera = view.camera * glm::translate(glm::mat4(1), glm::vec3(cube.cell) - glm::vec3(m.size.x / 2, m.size.y / 2, 0) +
                                                                       glm::vec3(0.5f, 0.5f, 0));
            painter.item(tinted(d, creatures::spiritColour(cube.material)));
        }
}

void Interface::drawInventory(Painter &painter, glm::vec2 W, glm::vec2 mouse) {
    if (!inventory)
        return;
    scrollRange();
    const glm::vec2 O = panelOrigin(W), local = mouse - O;
    if (inventoryPanel) {
        placeWidget(*inventoryPanel, 0, O);
        painter.draw(*inventoryPanel, 0, Transform(1));
    }
    // The slots: boxes first, then the items in them, then their counts.
    const std::vector<creatures::Stack> empty;
    const auto &bag = inventoryTab < (int)inventory->bags.size() ? inventory->bags[inventoryTab] : empty;
    const int32_t row0 = scrollRow[inventoryTab];
    std::optional<size_t> hovered;
    if (itemBox)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLUMNS; c++) {
                placeWidget(*itemBox, 0, O + slotCorner(c, r));
                painter.draw(*itemBox, 0, Transform(1));
            }
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLUMNS; c++) {
            const size_t i = (size_t)(row0 + r) * COLUMNS + c;
            if (i >= bag.size())
                continue;
            const glm::vec2 corner = O + slotCorner(c, r);
            const bool over = inside(mouse, corner, glm::vec2(SLOT));
            if (over)
                hovered = i;
            const creatures::Stack &s = bag[i];
            if (s.count == 0)
                continue;
            const float ring = creatures::itemType(s.item) == 9 ? 0.75f : 1.f;
            drawItem(painter, W, s.item, corner + glm::vec2(SLOT / 2), (over ? 0.045f : 0.03f) * ring);
        }
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLUMNS; c++) {
            const size_t i = (size_t)(row0 + r) * COLUMNS + c;
            if (i >= bag.size() || bag[i].count <= 0 || !creatures::stackable(creatures::itemType(bag[i].item)))
                continue;
            outlined(painter, number(bag[i].count), O + slotCorner(c, r) + glm::vec2(36, 37), 10, 2, glm::vec4(1),
                     plasma::RIGHT);
        }
    // Title and money row.
    static const char16_t *const TITLES[4] = {u"Equipment", u"Items", u"Ingredients", u""};
    outlined(painter, TITLES[inventoryTab & 3], O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
    const uint32_t M = inventory->money;
    const float baseline = PANEL.y - 10;
    outlined(painter, number(M % 100) + u" C ", O + glm::vec2(PANEL.x - 30, baseline), 10, 2, COPPER, plasma::RIGHT);
    outlined(painter, number(M / 100 % 100) + u" S ", O + glm::vec2(PANEL.x - 70, baseline), 10, 2, SILVER, plasma::RIGHT);
    outlined(painter, number(M / 10000) + u" G ", O + glm::vec2(PANEL.x - 110, baseline), 10, 2, GOLD, plasma::RIGHT);
    outlined(painter, number(inventory->platinum) + u"  Platinum Coins ", O + glm::vec2(15, baseline), 10, 2, PLATINUM);
    // Scroll buttons and the thumb, cyan under the cursor.
    for (auto [doc, at] : {std::pair{&upButton, upCorner()}, std::pair{&downButton, downCorner()}}) {
        if (!*doc)
            continue;
        tint(**doc, 0, inside(local, at, BUTTON) ? CYAN : glm::vec4(1));
        placeWidget(**doc, 0, O + at);
        painter.draw(**doc, 0, Transform(1));
    }
    const int32_t n = scrollPositions[inventoryTab];
    if (scrollThumb && n > 0) {
        const float track = PANEL.y - 70;
        const float th = n > 1 ? std::trunc(track / (float)n) : track;
        const float y = n > 1 ? 35 + std::trunc((track - th) * (float)row0 / (float)(n - 1)) : 35;
        resizeWidget(*scrollThumb, 0, {16, th});
        tint(*scrollThumb, 0, inside(local, {PANEL.x - 28, y}, {16, th}) ? CYAN : glm::vec4(1));
        placeWidget(*scrollThumb, 0, O + glm::vec2(PANEL.x - 28, y));
        painter.draw(*scrollThumb, 0, Transform(1));
    }
    // Tabs above the panel: bright when current or hovered; frame cyan hovered, green current.
    for (int k = 0; k < (int)inventoryTabs.size(); k++) {
        plasma::Document &doc = inventoryTabs[k];
        const bool over = inside(local, tabCorner(k), {38, 38}), current = k == inventoryTab;
        tint(doc, 0, over || current ? glm::vec4(1) : glm::vec4(0.8f, 0.8f, 0.8f, 1));
        tint(doc, doc.find("frame"), over ? CYAN : current ? glm::vec4(0, 1, 0, 1) : glm::vec4(1));
        placeWidget(doc, 0, O + tabCorner(k));
        painter.draw(doc, 0, Transform(1));
    }
    drawCharacter(painter);
    // §8.1.5: the equipment boxes and what they hold.
    std::optional<int> hoveredBox;
    for (int i = 0; i < (int)equipmentBoxes.size(); i++) {
        anchor(equipmentBoxes[i], boxPivot(W, i));
        painter.draw(equipmentBoxes[i], 0, Transform(1));
        const glm::vec2 corner = boxCorner(W, i);
        if (inside(mouse, corner, {50, 50}))
            hoveredBox = i;
        drawItem(painter, W, inventory->equipment[creatures::equipmentBoxes[i].slot], corner + glm::vec2(25), 44 * 0.0006f);
    }
    // §8.1.3: the item in hand follows the cursor, its count beside it when it stacks.
    const creatures::Stack &hand = inventory->hand;
    if (!hand.empty()) {
        drawItem(painter, W, hand.item, mouse, 0.05f);
        if (creatures::stackable(creatures::itemType(hand.item)))
            outlined(painter, number(hand.count), mouse + glm::vec2(0, -20), 12, 3, glm::vec4(1), plasma::CENTRE);
    }
    // §8.1.7: the tooltip of the hovered slot or box.
    const ItemRecord *shown = nullptr;
    if (hovered && bag[*hovered].count != 0)
        shown = &bag[*hovered].item;
    else if (hoveredBox && creatures::itemType(inventory->equipment[creatures::equipmentBoxes[*hoveredBox].slot]))
        shown = &inventory->equipment[creatures::equipmentBoxes[*hoveredBox].slot];
    if (shown)
        drawTooltip(painter, W, mouse + glm::vec2(20), *shown);
}

// §8.1.4.
void Interface::drawCharacter(Painter &painter) {
    if (characterPanel) {
        placeWidget(*characterPanel, 0, CHARACTER_AT);
        painter.draw(*characterPanel, 0, Transform(1));
    }
    const glm::vec2 P = CHARACTER_AT;
    const CharacterSheet &s = sheet;
    outlined(painter, utf16(s.name), P + glm::vec2(10, 20), 12, 3, glm::vec4(1), plasma::WRAP, 180);
    const int cls = std::clamp(s.characterClass, 0, 4);
    outlined(painter, u"LVL " + number(s.level) + CLASS_NAMES[cls] + SPECIALIZATIONS[cls][s.specialization & 1],
             P + glm::vec2(10, 36), 12, 3, {1, 0.5f, 0, 1});
    const glm::vec4 value{0.3f, 0.6f, 1, 1};
    const auto row = [&](float y, const char16_t *label, const std::u16string &text, glm::vec4 colour) {
        outlined(painter, label, P + glm::vec2(10, y), 10, 3, glm::vec4(1));
        outlined(painter, text, P + glm::vec2(160, y), 10, 3, colour);
    };
    row(56, u"Power", number(creatures::powerOf(s.level)), value);
    row(76, u"HP", tenths(s.health), value);
    row(92, u"ARMOR", tenths(s.armour), value);
    row(108, u"RESI", tenths(s.resistance), value);
    row(124, u"CRIT", tenths(100 * s.critical) + u"%", value);
    row(140, u"TEMPO", tenths(100 * s.tempo) + u"%", value);
    const float reg = 10 * s.regeneration / std::pow(2.f, (float)(s.level - 1) * 0.25f) + 100;
    row(156, u"REG", tenths(reg) + u"%", value);
    outlined(painter, u"(" + tenths(s.regeneration) + u")", P + glm::vec2(210, 156), 10, 3, value);
    row(172, u"Weapon Rating", number((int64_t)(100 * s.weaponRating + 0.5f)) + u"%", band(s.weaponRating));
    row(188, u"Armor Rating", number((int64_t)(100 * s.armorRating + 0.5f)) + u"%", band(s.armorRating));
}

// §8.1.7: the tooltip box at `X` (its top-left) and the item's text block.
void Interface::drawTooltip(Painter &painter, glm::vec2 W, glm::vec2 X, const ItemRecord &record, bool world) {
    const creatures::Item item = creatures::Item::parse(record.data());
    // Open points 2 §3: the equipped items of the same type, unless the shown one is one of
    // them (the two weapons and the two rings go as pairs).
    std::vector<const ItemRecord *> compared;
    if (inventory && item.type != 0) {
        const auto shownIs = [&](int slot) { return &record == &inventory->equipment[(size_t)slot]; };
        const bool weapons = shownIs(creatures::MAIN_HAND) || shownIs(creatures::OFF_HAND);
        const bool rings = shownIs(creatures::RING_RIGHT) || shownIs(creatures::RING_LEFT);
        static const int ORDER[10] = {creatures::MAIN_HAND, creatures::OFF_HAND, 12, creatures::SHOULDERS_SLOT,
                                      creatures::CHEST_SLOT, creatures::GLOVES_SLOT, creatures::BOOTS_SLOT,
                                      creatures::AMULET_SLOT, creatures::RING_RIGHT, creatures::RING_LEFT};
        for (int slot : ORDER) {
            const ItemRecord &e = inventory->equipment[(size_t)slot];
            const bool pairExcluded = ((slot == creatures::MAIN_HAND || slot == creatures::OFF_HAND) && weapons) ||
                                      ((slot == creatures::RING_RIGHT || slot == creatures::RING_LEFT) && rings);
            if (creatures::itemType(e) == item.type && !pairExcluded && !shownIs(slot))
                compared.push_back(&e);
        }
    }
    if (!compared.empty()) {
        if (equippedPreview) {
            anchor(*equippedPreview, world ? X + glm::vec2(0, -255) : X + glm::vec2(-255, 0));
            painter.draw(*equippedPreview, 0, Transform(1));
        }
        // (As read, the text stays left of the tooltip even when the box is above it.)
        outlined(painter, u"Currently equipped", X + glm::vec2(-190, 20), 8, 3, glm::vec4(1));
        for (size_t k = 0; k < compared.size(); k++)
            drawItemText(painter, X + glm::vec2(-240, 40 + 130 * (float)k), *compared[k], 230, true);
    }
    if (preview) {
        for (int k = 1; k <= 4; k++)
            setVisible(*preview, ("star" + std::to_string(k)).c_str(), item.rarity == k);
        anchor(*preview, X);
        painter.draw(*preview, 0, Transform(1));
    }
    drawItem(painter, W, record, X + glm::vec2(150), 0.1f, -0.05f);
    drawItemText(painter, X + glm::vec2(14, 25), record, 280, true);
}

// §8.1.7: an item's text block (0x4a28c0): name, power, class line, stats.
void Interface::drawItemText(Painter &painter, glm::vec2 at, const ItemRecord &record, float wrap, bool classLine) {
    const creatures::Item item = creatures::Item::parse(record.data());
    const bool quantity = creatures::quantityType(record);
    std::u16string name = item.type == 20 ? u"Pet food: " : u"";
    name += itemName ? itemName(record) : u"";
    const int32_t power = creatures::powerOf(item.level);
    if (!quantity && item.type != 19)
        name += u" +" + number(power);
    outlined(painter, name, at, 10, 3, glm::vec4(creatures::nameColour(record), 1), plasma::WRAP, wrap);
    at.y += 30;
    std::u16string second;
    if (quantity)
        second = item.level > 1 ? number(item.level) + u" X" : u"";
    else if (item.type == 19)
        second = u"LVL " + number(item.level);
    else
        second = u"Power " + number(power);
    if (record[0x0e] & 1)
        second += u" (adapted)";
    outlined(painter, second, at, 9, 3, creatures::powerOf(sheet.level) < power ? RED : glm::vec4(1), plasma::WRAP, wrap);
    if (classLine && !creatures::classMayUse(record, sheet.characterClass)) {
        at.y += 14;
        std::u16string classes;
        for (int c = 1; c <= 4; c++)
            if (creatures::classMayUse(record, c))
                classes += std::u16string(CLASS_NAMES[c] + 1) + u" ";
        outlined(painter, classes, at, 9, 3, RED, plasma::WRAP, wrap);
    }
    ItemRecord taught = record; // a formula's recipe key (crafting §1.1)
    taught[0x00] = record[0x08];
    std::fill(taught.begin() + 0x08, taught.begin() + 0x0c, 0);
    if (item.type == 2 && inventory)
        for (const ItemRecord &known : inventory->recipes)
            if (creatures::sameItem(known, taught)) {
                at.y += 27;
                outlined(painter, u"Already known", at, 9, 3, RED, plasma::WRAP, wrap);
                break;
            }
    at.y += 16;
    const auto line = [&](const std::u16string &text, glm::vec4 colour, float step) {
        outlined(painter, text, at, 10, 2, colour);
        at.y += step;
    };
    if (item.type == 19) {
        const float g = creatures::levelGrowth(item.level) / 3.f; // g(L) of §8.1 (levelGrowth is 3·g)
        line(u"X" + number(item.seed) + u"/" + number((int64_t)(g * 1000 + 50)), {0.5f, 1, 1, 1}, 14);
        if (rideable(item.subtype))
            line(u"Ridable", STAT, 0);
        at.y += 14;
    }
    if (item.type == 3)
        line(u"DMG " + tenths(creatures::itemDamage(item)), STAT, 14);
    if (const float v = creatures::itemHealth(item); v >= 0.1f)
        line(u"HP " + tenths(v), STAT, 14);
    if (const float v = creatures::itemArmour(item); v >= 0.1f)
        line(u"ARMOR " + tenths(v), STAT, 16);
    if (const float v = creatures::itemResistance(item); v >= 0.1f)
        line(u"RESI " + tenths(v), STAT, 16);
    if (const float v = creatures::itemSpeed(item); v >= 0.001f)
        line(u"TEMPO " + tenths(100 * v), CYAN, 16);
    if (const float v = creatures::itemCritical(item); v >= 0.001f)
        line(u"CRIT " + tenths(100 * v), CYAN, 16);
    if (const float v = creatures::itemRegeneration(item); v >= 0.1f)
        line(u"REG " + tenths(v), CYAN, 16);
    if (item.type == 1)
        if (const float v = creatures::itemHeal(item); v >= 0.1f)
            line(u"HP +" + number((int64_t)v), {0.25f, 1, 0.25f, 1}, 14);
}

bool Interface::inventoryPress(glm::vec2 W, glm::vec2 mouse, int button) {
    if (skillsPress(W, mouse, button))
        return true;
    if (craftingOpen && craftingPress(W, mouse, button))
        return true;
    if (servicesPress(W, mouse, button))
        return true;
    if (benchPress(W, mouse, button))
        return true;
    if (vendorOpen && vendorPress(W, mouse, button))
        return true;
    if (!inventory || !inventoryOpen)
        return false;
    scrollRange();
    const glm::vec2 O = panelOrigin(W), local = mouse - O;
    const auto sound = [&](int32_t s) {
        if (s)
            inventorySounds.push_back(s);
    };
    for (int k = 0; k < (int)inventoryTabs.size(); k++)
        if (inside(local, tabCorner(k), {38, 38})) {
            if (button == 0 && k != inventoryTab) {
                inventoryTab = k;
                scrollRange();
                sound(creatures::SOUND_TAB);
            }
            return true;
        }
    if (inside(local, {0, 0}, PANEL)) {
        int32_t &row = scrollRow[inventoryTab];
        const int32_t n = scrollPositions[inventoryTab];
        if (button == 0 && inside(local, upCorner(), BUTTON)) {
            row = std::max(row - 1, 0);
            return true;
        }
        if (button == 0 && inside(local, downCorner(), BUTTON)) {
            row = std::clamp(row + 1, 0, std::max(n - 1, 0));
            return true;
        }
        // A press on the track moves the thumb's centre there (the drag rule of 0x4c5bb0).
        const float track = PANEL.y - 70;
        if (button == 0 && n > 1 && inside(local, {PANEL.x - 28, 35}, {16, track})) {
            const float th = std::trunc(track / (float)n);
            const float top = std::clamp(local.y - th / 2, 35.f, 35 + track - th);
            row = (int32_t)((top - 35) * (float)(n - 1) / (track - th));
            return true;
        }
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLUMNS; c++) {
                if (!inside(local, slotCorner(c, r), glm::vec2(SLOT)))
                    continue;
                const size_t i = (size_t)(row + r) * COLUMNS + c;
                if (button == 0) {
                    sound(inventory->pressSlot(inventoryTab, i));
                } else if (adaptOpen) { // item services §2.2: the item to adapt
                    adaptBag = inventoryTab, adaptSlot = i, adaptBox = -1;
                } else if (identifyOpen) { // §1.3: the item to identify
                    identifyBag = inventoryTab, identifySlot = i;
                } else if (benchOpen) { // item services §3.2: the weapon to customize
                    const bool weapon = inventoryTab < (int)inventory->bags.size() && i < inventory->bags[inventoryTab].size() &&
                                        creatures::itemType(inventory->bags[inventoryTab][i].item) == 3;
                    if (weapon)
                        benchBag = inventoryTab, benchSlot = i, benchBox = -1, benchReset();
                    else
                        sound(creatures::SOUND_NOT_HERE);
                } else if (vendorOpen) { // talk §3.4: sell one
                    if (inventoryTab < (int)inventory->bags.size() && i < inventory->bags[inventoryTab].size())
                        sell(inventory->bags[inventoryTab][i]);
                } else {
                    const creatures::UseOutcome u = inventory->useSlot(inventoryTab, i, button == 2, sheet.level,
                                                                       sheet.characterClass, sheet.canAct);
                    sound(u.sound);
                    if (u.consume)
                        itemsToUse.push_back(*u.consume);
                    gliderEquipped |= u.clearAction;
                    equipmentChanged = true;
                }
                return true;
            }
        return true;
    }
    for (int i = 0; i < (int)equipmentBoxes.size(); i++)
        if (inside(mouse, boxCorner(W, i), {50, 50})) {
            if (button == 0) {
                sound(inventory->pressBox(i, sheet.level, sheet.characterClass));
            } else if (button == 1 && adaptOpen) { // item services §2.2: equipped gear, in place
                if (creatures::itemType(inventory->equipment[creatures::equipmentBoxes[i].slot]))
                    adaptBox = creatures::equipmentBoxes[i].slot, adaptBag = -1;
            } else if (button == 1 && benchOpen) { // the equipped weapon, edited in place
                if (creatures::itemType(inventory->equipment[creatures::equipmentBoxes[i].slot]) == 3)
                    benchBox = creatures::equipmentBoxes[i].slot, benchBag = -1, benchReset();
                else
                    sound(creatures::SOUND_NOT_HERE);
            } else if (button == 1 && vendorOpen) { // sold straight from the box
                creatures::ItemRecord &slot = inventory->equipment[creatures::equipmentBoxes[i].slot];
                if (creatures::itemType(slot)) {
                    creatures::Stack one{1, slot};
                    sell(one);
                    slot[0x00] = slot[0x01] = 0;
                }
            } else if (button == 1) {
                sound(inventory->unequip(i));
            }
            equipmentChanged = true;
            return true;
        }
    return inside(mouse, CHARACTER_AT, CHARACTER_SIZE);
}

} // namespace ui
