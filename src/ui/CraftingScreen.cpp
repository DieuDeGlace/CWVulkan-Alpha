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
constexpr glm::vec2 LIST{350, 285}, PREVIEW{350, 330};
// §2, §8.1.2 (corrections 7, 8, 92): integer halves of W and H; tops level with the inventory's
// (its height 285), the blueprint preview 10 below the crafting window (also 285 high).
glm::vec2 listOrigin(glm::vec2 W) { return {std::trunc(W.x / 2) - 200 - 350 - 10, std::trunc(W.y / 2) - 285}; }
glm::vec2 previewOrigin(glm::vec2 W) { return {std::trunc(W.x / 2) - 200 - 350 - 10, std::trunc(W.y / 2) - 285 + 10 + 285}; }
glm::vec2 tabCorner(int k) { return {4 + 40.f * (float)k, -41.07f}; }
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
    if (const int32_t n = gui.find("scrollbutton"); n >= 0)
        craftList.thumb = clone(gui, n);
    craftList.measure(craftingPanel, wideItemBox);
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
            const auto &list = craftRecipes[craftSelection->first];
            if (craftSelection->second < list.size())
                crafting = list[craftSelection->second].recipe;
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

void Interface::refreshCraftingRows() {
    if (!inventory)
        return;
    auto sorted = creatures::recipeTabs(inventory->recipes);
    for (size_t tab = 0; tab < sorted.size(); ++tab) {
        auto &rows = craftRecipes[tab];
        rows.clear();
        rows.reserve(sorted[tab].size());
        for (auto &recipe : sorted[tab]) {
            const int32_t count = creatures::timesCraftable(*inventory, recipe);
            rows.push_back({std::move(recipe), count});
        }
    }
    craftList.recalculate(craftRecipes);
    craftList.placeThumb();
}

void Interface::toggleCrafting(std::optional<int> tab) {
    craftingOpen = !craftingOpen;
    craftList.releaseThumb();
    if (tab)
        craftList.currentGroup = *tab;
    refreshCraftingRows();
}

void Interface::drawCrafting(Painter &painter, glm::vec2 W, glm::vec2 mouse, float dtMs) {
    if (!inventory)
        return;
    craftList.recalculate(craftRecipes);
    const int craftingTab = craftList.currentGroup;
    auto &list = craftRecipes[craftingTab];
    const glm::vec2 cell = craftList.cellSize;
    const int row0 = craftList.offset();

    // §4.1: the list window.
    const glm::vec2 O = listOrigin(W), local = mouse - O;
    if (craftingPanel) {
        placeWidget(*craftingPanel, 0, O);
        painter.draw(*craftingPanel, 0, Transform(1));
    }
    outlined(painter, TITLES[craftingTab], O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
    for (int slot = 0; slot < craftList.columns * craftList.rows; ++slot) {
        const size_t i = size_t(row0) * craftList.columns + slot;
        const glm::vec2 corner = O + craftList.cellAt(slot % craftList.columns, slot / craftList.columns);
        if (wideItemBox) {
            placeWidget(*wideItemBox, 0, corner);
            painter.draw(*wideItemBox, 0, Transform(1));
        }
        if (i >= list.size())
            continue;
        const ItemRecord &product = list[i].recipe.product;
        const bool over = inside(mouse, corner, cell);
        if (itemModel && creatures::itemType(product)) {
            const ItemModel m = itemModel(product);
            painter.item(itemDraw(m.mesh, m.size, creatures::itemType(product) == 9, corner + glm::vec2(20), itemTurn,
                                  over ? 0.045f : 0.03f, 0, W));
        }
        std::u16string name = itemName ? itemName(product) : u"";
        if (!creatures::quantityType(product))
            name += u" +" + number(creatures::powerOf(creatures::itemLevel(product)));
        outlined(painter, name, corner + glm::vec2(cell.y + 10, 10), 10, 2, glm::vec4(creatures::nameColour(product), 1),
                 plasma::WRAP, cell.x - cell.y - 20);
        // §4.1: the count shows for a non-empty item type with a count above 0.
        // Bag quantities can also change through engine-side services between rebuilds.
        list[i].available = creatures::timesCraftable(*inventory, list[i].recipe);
        if (list[i].available > 0 && creatures::itemType(product) != 0)
            outlined(painter, number(list[i].available), corner + glm::vec2(cell.x - 4, 37), 10, 2, glm::vec4(1), plasma::RIGHT);
        if (craftSelection && craftSelection->first == craftingTab && craftSelection->second == i && itemSelector) {
            placeWidget(*itemSelector, 0, corner);
            painter.draw(*itemSelector, 0, Transform(1));
        }
    }
    drawScrollChildren(craftList, painter, O, local);
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
    if (craftSelection && craftSelection->first < 6 && craftSelection->second < craftRecipes[craftSelection->first].size())
        recipe = &craftRecipes[craftSelection->first][craftSelection->second].recipe;
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
    // The button: white while something is missing, green when ready, cyan under the mouse. A
    // listed key with no ingredients and no station passes (§2.1; no native grant makes one).
    if (craftButton) {
        const bool overButton = inside(mouse, P + BUTTON_AT, BUTTON_SIZE);
        const bool ready = stationThere && !missing;
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
            if (button == 0 && k != craftList.currentGroup) {
                craftList.currentGroup = k;
                craftList.recalculate(craftRecipes);
                const size_t first = size_t(craftList.offset()) * craftList.columns;
                if (first < craftRecipes[k].size())
                    craftSelection = std::pair{k, first};
                else
                    craftSelection.reset();
                craftList.placeThumb();
                inventorySounds.push_back(creatures::SOUND_TAB);
            }
            return true;
        }
    if (inside(local, {0, 0}, LIST)) {
        if (button == 0 && pressScrollChild(craftList, local, mouse.y, craftRecipes))
            return true;
        const int tab = craftList.currentGroup;
        for (int slot = 0; slot < craftList.columns * craftList.rows; ++slot) {
            const size_t i = size_t(craftList.offset()) * craftList.columns + slot;
            if (i < craftRecipes[tab].size() &&
                inside(local, craftList.cellAt(slot % craftList.columns, slot / craftList.columns), craftList.cellSize)) {
                craftSelection = std::pair{tab, i};
                inventorySounds.push_back(0x55);
                break;
            }
        }
        return true;
    }
    return inside(mouse, previewOrigin(W), PREVIEW);
}

} // namespace ui
