// The crafting window (docs/specs/alpha-crafting.md §4, §5): six tabs of recipes in wide rows,
// the blueprint preview below it with the product, its ingredients and the station it needs,
// and the craft button held for a second per item.
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <algorithm>

namespace ui {
namespace {

using namespace widgets;
using creatures::ItemRecord;

// §4.1: the list window, 350 x 285, left of the inventory; wide rows of 300 x 40 at a 45 px
// pitch (one column, five rows). §4.2: the preview, 350 x 330, under it.
constexpr glm::vec2 LIST{350, 285}, PREVIEW{350, 330}, ROW{300, 40};
constexpr int ROWS = 5;
constexpr float PITCH = 45;
glm::vec2 listOrigin(glm::vec2 W) { return {W.x / 2 - 200 - 350 - 10, W.y / 2 - 285}; }
glm::vec2 previewOrigin(glm::vec2 W) { return {W.x / 2 - 200 - 350 - 10, W.y / 2 + 10}; }
glm::vec2 rowCorner(int r) { return {10, 40 + PITCH * (float)r}; }
glm::vec2 tabCorner(int k) { return {4 + 40.f * (float)k, -41.07f}; }
glm::vec2 upCorner() { return {LIST.x - 30, 10}; }
glm::vec2 downCorner() { return {LIST.x - 30, LIST.y - 30}; }
// (Where the button and the bar sit in the preview comes from gui.plx and is not traced, open
// point C-3: the bottom right here.)
constexpr glm::vec2 BUTTON_AT{PREVIEW.x - 56, PREVIEW.y - 56}, BAR_AT{PREVIEW.x - 106, PREVIEW.y - 56};
constexpr glm::vec2 BUTTON_SIZE{46, 46};

const char16_t *const TITLES[6] = {u"Weapons", u"Armor", u"Amulets", u"Cooking", u"Alchemy", u"Formulas"};
const glm::vec4 GREEN{0, 1, 0, 1}, CYAN{0, 1, 1, 1};

// §4.1: the class's icons for the first two tabs, the same for all others.
const char *tabIcon(int tab, int cls) {
    static const char *const WEAPONS[4] = {"craft-melee-weapons.png", "craft-ranged-weapons.png",
                                           "craft-magic-weapons.png", "craft-rogue-weapons.png"};
    static const char *const ARMOUR[4] = {"craft-heavy-armor.png", "craft-medium-armor.png", "craft-light-armor.png",
                                          "craft-rogue-armor.png"};
    const int c = std::clamp(cls, 1, 4) - 1;
    switch (tab) {
    case 0: return WEAPONS[c];
    case 1: return ARMOUR[c];
    case 2: return "craft-amulets.png";
    case 3: return "craft-cooking.png";
    case 4: return "craft-alchemy.png";
    default: return "inventory-ingredients.png";
    }
}

} // namespace

void Interface::loadCrafting() {
    if (const int32_t n = gui.find("blackwidget"); n >= 0) {
        craftingPanel = clone(gui, n);
        resizeWidget(*craftingPanel, 0, LIST);
        previewPanel = clone(gui, n);
        resizeWidget(*previewPanel, 0, PREVIEW);
    }
    if (const int32_t n = gui.find("wideitembox"); n >= 0)
        wideItemBox = clone(gui, n);
    if (const int32_t n = gui.find("itemselector"); n >= 0)
        itemSelector = clone(gui, n);
    if (const int32_t n = gui.find("craftbutton"); n >= 0)
        craftButton = clone(gui, n);
    if (const int32_t n = gui.find("craftbar"); n >= 0)
        craftBar = clone(gui, n);
    if (const int32_t n = gui.find("tab"); n >= 0)
        for (int k = 0; k < 6; k++)
            craftingTabs.push_back(clone(gui, n));
}

void Interface::stepCraft(float dtMs, bool leftHeld) {
    // §5.1, §5.2: the timer runs only while the window shows; held, it starts again.
    if (!craftingOpen)
        return;
    if (craftReady && leftHeld && craftTimer < 0) {
        craftTimer = 0;
        if (craftSelection && inventory) {
            const auto tabs = creatures::recipeTabs(inventory->recipes);
            const auto &list = tabs[craftSelection->first];
            if (craftSelection->second < list.size())
                crafting = list[craftSelection->second];
        }
    }
    if (craftTimer >= 0) {
        craftTimer += dtMs * 0.001f;
        if (craftTimer >= 1) {
            craftTimer = -1;
            finishedCraft = crafting;
        }
    }
}

void Interface::drawCrafting(Painter &painter, glm::vec2 W, glm::vec2 mouse, float dtMs) {
    if (!inventory)
        return;
    const auto tabs = creatures::recipeTabs(inventory->recipes);
    const auto &list = tabs[craftingTab];
    const int32_t rows = (int32_t)list.size();
    int32_t &row0 = craftingRow[craftingTab];
    row0 = std::clamp(row0, 0, std::max(0, rows - ROWS));

    // §4.1: the list window.
    const glm::vec2 O = listOrigin(W), local = mouse - O;
    if (craftingPanel) {
        placeWidget(*craftingPanel, 0, O);
        painter.draw(*craftingPanel, 0, Transform(1));
    }
    outlined(painter, TITLES[craftingTab], O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
    for (int r = 0; r < ROWS; r++) {
        const size_t i = (size_t)(row0 + r);
        const glm::vec2 corner = O + rowCorner(r);
        if (wideItemBox) {
            placeWidget(*wideItemBox, 0, corner);
            painter.draw(*wideItemBox, 0, Transform(1));
        }
        if (i >= list.size())
            continue;
        const ItemRecord &product = list[i].product;
        const bool over = inside(mouse, corner, ROW);
        if (itemModel && creatures::itemType(product)) {
            const ItemModel m = itemModel(product);
            painter.item(itemDraw(m.mesh, m.size, creatures::itemType(product) == 9, corner + glm::vec2(20), itemTurn,
                                  over ? 0.045f : 0.03f, 0, W));
        }
        std::u16string name = itemName ? itemName(product) : u"";
        if (!creatures::quantityType(product))
            name += u" +" + number(creatures::powerOf(creatures::itemLevel(product)));
        outlined(painter, name, corner + glm::vec2(ROW.y + 10, 10), 10, 2, glm::vec4(creatures::nameColour(product), 1),
                 plasma::WRAP, ROW.x - ROW.y - 20);
        if (const int32_t n = creatures::timesCraftable(*inventory, list[i]); n > 0)
            outlined(painter, number(n), corner + glm::vec2(ROW.x - 4, 37), 10, 2, glm::vec4(1), plasma::RIGHT);
        if (craftSelection && craftSelection->first == craftingTab && craftSelection->second == i && itemSelector) {
            placeWidget(*itemSelector, 0, corner);
            painter.draw(*itemSelector, 0, Transform(1));
        }
    }
    if (upButton && downButton && rows > ROWS) {
        for (auto [doc, at] : {std::pair{&*upButton, upCorner()}, std::pair{&*downButton, downCorner()}}) {
            tint(*doc, 0, inside(local, at, {21, 19}) ? CYAN : glm::vec4(1));
            placeWidget(*doc, 0, O + at);
            painter.draw(*doc, 0, Transform(1));
        }
    }
    for (int k = 0; k < (int)craftingTabs.size(); k++) {
        plasma::Document &doc = craftingTabs[k];
        const plasma::Node &root = doc.nodes[0];
        if (const auto it = craftingIcons.find(tabIcon(k, sheet.characterClass));
            it != craftingIcons.end() && root.shape >= 0 && doc.shapes[root.shape].kind == plasma::Shape::Kind::Mesh)
            doc.shapes[root.shape].mesh.texture.value = it->second;
        const bool over = inside(local, tabCorner(k), {38, 38}), current = k == craftingTab;
        tint(doc, 0, over || current ? glm::vec4(1) : glm::vec4(0.8f, 0.8f, 0.8f, 1));
        tint(doc, doc.find("frame"), over ? CYAN : current ? GREEN : glm::vec4(1));
        placeWidget(doc, 0, O + tabCorner(k));
        painter.draw(doc, 0, Transform(1));
    }

    // §4.2: the preview of the selected recipe.
    const glm::vec2 P = previewOrigin(W);
    if (previewPanel) {
        placeWidget(*previewPanel, 0, P);
        painter.draw(*previewPanel, 0, Transform(1));
    }
    const creatures::Recipe *recipe = nullptr;
    if (craftSelection && craftSelection->first < 6 && craftSelection->second < tabs[craftSelection->first].size())
        recipe = &tabs[craftSelection->first][craftSelection->second];
    previewTurn.z += dtMs * 0.02f;
    craftReady = false;
    if (!recipe) {
        outlined(painter, u"Unknown", P + glm::vec2(15, 65), 15, 3, glm::vec4(1));
        return;
    }
    const ItemRecord &product = recipe->product;
    const bool overModel = mouse.x > P.x + PREVIEW.x / 2 - 60 && mouse.x < P.x + PREVIEW.x / 2 + 60 &&
                           mouse.y > P.y + 50 && mouse.y < P.y + 170;
    if (itemModel) {
        const ItemModel m = itemModel(product);
        painter.item(itemDraw(m.mesh, m.size, creatures::itemType(product) == 9, P + glm::vec2(PREVIEW.x / 2, 110),
                              previewTurn, overModel ? 0.09f : 0.06f, 0, W));
    }
    std::u16string name = itemName ? itemName(product) : u"";
    if (!creatures::quantityType(product))
        name += u" +" + number(creatures::powerOf(creatures::itemLevel(product)));
    outlined(painter, name, P + glm::vec2(PREVIEW.x / 2, 30), 12, 3, glm::vec4(creatures::nameColour(product), 1),
             plasma::CENTRE | plasma::WRAP, PREVIEW.x);
    // Ingredients, seven to a row, with have/need under each.
    bool missing = false;
    const int columns = (int)((PREVIEW.x - 10) / 45);
    const ItemRecord *hovered = overModel ? &product : nullptr;
    for (size_t k = 0; k < recipe->ingredients.size(); k++) {
        const auto &[item, need] = recipe->ingredients[k];
        const int col = (int)k % columns, row = (int)k / columns;
        const glm::vec2 box = P + glm::vec2(10 + 45.f * col, PREVIEW.y - 120 + 45.f * row);
        const bool over = inside(mouse, box, glm::vec2(40));
        if (over)
            hovered = &item;
        if (itemModel) {
            const ItemModel m = itemModel(item);
            painter.item(itemDraw(m.mesh, m.size, false, box + glm::vec2(20), itemTurn, over ? 0.0375f : 0.025f, 0, W));
        }
        const int32_t have = creatures::holding(*inventory, item);
        missing = missing || have < need;
        outlined(painter, number(have) + u"/" + number(need), P + glm::vec2(50 + 45.f * col, PREVIEW.y - 75 + 45.f * row),
                 10, 2, have >= need ? GREEN : glm::vec4(1), plasma::RIGHT);
    }
    // The station.
    const bool stationThere = recipe->station == creatures::NO_STATION || (stationNear && stationNear(recipe->station));
    if (!stationThere)
        outlined(painter, utf16(creatures::stationText(recipe->station)), P + glm::vec2(15, PREVIEW.y - 40), 9, 3,
                 {1, 0.25f, 0.25f, 1}, plasma::WRAP, PREVIEW.x - 50);
    // The button: white while something is missing, green when ready, cyan under the mouse.
    if (craftButton) {
        const bool overButton = inside(mouse, P + BUTTON_AT, BUTTON_SIZE);
        const bool ready = stationThere && !missing && !recipe->ingredients.empty();
        craftReady = ready && overButton;
        tint(*craftButton, craftButton->find("frame"), !ready ? glm::vec4(1) : overButton ? CYAN : GREEN);
        placeWidget(*craftButton, 0, P + BUTTON_AT);
        painter.draw(*craftButton, 0, Transform(1));
    }
    if (craftBar && craftTimer >= 0) {
        if (const int32_t bar = craftBar->find("bar"); bar >= 0 && craftBar->nodes[bar].transformation >= 0) {
            glm::mat4 &d = craftBar->transformations[craftBar->nodes[bar].transformation].deformation.value;
            d = glm::mat4(1);
            d[0][0] = std::max(craftTimer, 0.001f) * (40 - 8) / 40;
        }
        placeWidget(*craftBar, 0, P + BAR_AT);
        painter.draw(*craftBar, 0, Transform(1));
    }
    if (hovered && !inventoryOpen)
        drawTooltip(painter, W, mouse + glm::vec2(20), *hovered);
}

bool Interface::craftingPress(glm::vec2 W, glm::vec2 mouse, int button) {
    if (!inventory)
        return false;
    const glm::vec2 O = listOrigin(W), local = mouse - O;
    for (int k = 0; k < 6; k++)
        if (inside(local, tabCorner(k), {38, 38})) {
            if (button == 0 && k != craftingTab) {
                craftingTab = k;
                inventorySounds.push_back(creatures::SOUND_TAB);
            }
            return true;
        }
    if (inside(local, {0, 0}, LIST)) {
        const int32_t rows = (int32_t)creatures::recipeTabs(inventory->recipes)[craftingTab].size();
        int32_t &row0 = craftingRow[craftingTab];
        if (inside(local, upCorner(), {21, 19}))
            row0 = std::max(row0 - 1, 0);
        else if (inside(local, downCorner(), {21, 19}))
            row0 = std::min(row0 + 1, std::max(0, rows - ROWS));
        for (int r = 0; r < ROWS; r++)
            if (inside(local, rowCorner(r), ROW) && row0 + r < rows) {
                craftSelection = std::pair{craftingTab, (size_t)(row0 + r)};
                inventorySounds.push_back(0x55);
            }
        return true;
    }
    return inside(mouse, previewOrigin(W), PREVIEW);
}

} // namespace ui
