// Grouped row identities and ordered transfers follow the held vendor-widget
// contract. Panel art/placement and input-button mapping remain engine adapters.
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <algorithm>

namespace ui {
namespace {

using namespace widgets;
using creatures::ItemRecord;

// Screens §8.1.2: 350 x 568, right of the inventory, tops aligned; wide rows of 300 x 40.
constexpr glm::vec2 PANEL{350, 568}, ROW{300, 40};
constexpr float PITCH = 45;
constexpr int ROWS = (int)((568 - 40) / PITCH);
glm::vec2 origin(glm::vec2 W) { return {W.x / 2 + 200 + 10, W.y / 2 - 285}; }
glm::vec2 rowCorner(int r) { return {10, 40 + PITCH * (float)r}; }
glm::vec2 tabCorner(int k) { return {4 + 40.f * (float)k, -41.07f}; }
glm::vec2 upCorner() { return {PANEL.x - 30, 10}; }
glm::vec2 downCorner() { return {PANEL.x - 30, PANEL.y - 30}; }
const glm::vec4 COPPER{0.8f, 0.5f, 0, 1}, SILVER{0.7f, 0.7f, 0.7f, 1}, GOLD{1, 0.9f, 0, 1}, CYAN{0, 1, 1, 1};

} // namespace

void Interface::loadVendor() {
    if (const int32_t n = gui.find("blackwidget"); n >= 0) {
        vendorPanel = clone(gui, n);
        resizeWidget(*vendorPanel, 0, PANEL);
    }
    if (const int32_t n = gui.find("tab"); n >= 0)
        for (int k = 0; k < 2; k++)
            vendorTabs.push_back(clone(gui, n));
}

void Interface::sell(creatures::Stack &stack) {
    if (inventory)
        vendor::sellEntry(stack.count, stack.item, buyBack, inventory->money, vendorServices());
}

vendor::Services Interface::vendorServices() {
    vendor::Services services;
    services.price = [](const ItemRecord &item) { return creatures::price(item); };
    services.canCarry = [&](const ItemRecord &item) { return inventory->canCarry(item); };
    services.add = [&](const ItemRecord &item) { inventory->add(item); };
    services.feedback = [&](int32_t sound) { inventorySounds.push_back(sound); };
    services.carryLimit = [&] { chatAdd("You can't carry more of these items.\n", {255, 51, 51}); };
    services.refresh = [&] { refreshVendor(); };
    // The engine's player widget derives its layout from the current bags.
    services.updatePlayer = [&] { scrollRange(); };
    return services;
}

void Interface::refreshVendor() {
    if (vendorGrid.currentGroup != 1 && vendorExport) vendorExport(vendorInventory);
    vendor::refresh(vendorInventory, nullptr, vendorGrid.currentGroup, buyBack);
    vendorGrid.update(&vendorInventory, PANEL.x, PANEL.y, int32_t(ROW.x), int32_t(ROW.y));
}

void Interface::drawVendor(Painter &painter, glm::vec2 W, glm::vec2 mouse) {
    vendorHovered.reset();
    const glm::vec2 O = origin(W), local = mouse - O;
    if (vendorPanel) {
        placeWidget(*vendorPanel, 0, O);
        painter.draw(*vendorPanel, 0, Transform(1));
    }
    outlined(painter, u"Vendor", O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
    vendorGrid.update(&vendorInventory, PANEL.x, PANEL.y, int32_t(ROW.x), int32_t(ROW.y));
    vendorGrid.hover = {};
    const auto [first, end] = vendorGrid.visible(vendorInventory);
    for (int r = 0; r < ROWS; r++) {
        const glm::vec2 corner = O + rowCorner(r);
        if (wideItemBox) {
            placeWidget(*wideItemBox, 0, corner);
            painter.draw(*wideItemBox, 0, Transform(1));
        }
        const size_t i = first + size_t(r);
        if (i >= end)
            continue;
        const auto &entry = vendorInventory.groups[size_t(vendorGrid.currentGroup)][i];
        const ItemRecord &item = entry.item;
        const bool over = inside(mouse, corner, ROW);
        if (over) {
            vendorGrid.hover = {vendorGrid.currentGroup, int32_t(i)};
            vendorHovered = item;
        }
        if (itemModel) {
            const ItemModel m = itemModel(item);
            painter.item(itemDraw(m.mesh, m.size, creatures::itemType(item) == 9, corner + glm::vec2(20), itemTurn,
                                  over ? 0.045f : 0.03f, 0, W));
        }
        std::u16string name = itemName ? itemName(item) : u"";
        if (!creatures::quantityType(item))
            name += u" +" + number(creatures::powerOf(creatures::itemLevel(item)));
        outlined(painter, name, corner + glm::vec2(ROW.y + 10, 10), 10, 2, glm::vec4(creatures::nameColour(item), 1),
                 plasma::WRAP, ROW.x - ROW.y - 20);
        if (vendor::quantityText(entry.count, creatures::itemType(item)))
            outlined(painter, number(entry.count), corner + glm::vec2(36, 37), 10, 2, glm::vec4(1), plasma::RIGHT);
        // The price in coins at the row's right edge, each part 40 px further left, zero parts
        // skipped.
        const int32_t p = creatures::price(item);
        float right = ROW.x - 4;
        for (const auto &[amount, suffix, colour] : {std::tuple{p % 100, u" C ", COPPER}, std::tuple{p / 100 % 100, u" S ", SILVER},
                                                   std::tuple{p / 10000, u" G ", GOLD}})
            if (amount != 0) {
                outlined(painter, number(amount) + suffix, corner + glm::vec2(right, 37), 10, 2, colour, plasma::RIGHT);
                right -= 40;
            }
    }
    if (upButton && downButton && vendorGrid.range > 1)
        for (auto [doc, at] : {std::pair{&*upButton, upCorner()}, std::pair{&*downButton, downCorner()}}) {
            tint(*doc, 0, inside(local, at, {21, 19}) ? CYAN : glm::vec4(1));
            placeWidget(*doc, 0, O + at);
            painter.draw(*doc, 0, Transform(1));
        }
    for (int k = 0; k < (int)vendorTabs.size(); k++) {
        plasma::Document &doc = vendorTabs[k];
        const bool over = inside(local, tabCorner(k), {38, 38}), current = k == vendorGrid.currentGroup;
        tint(doc, 0, over || current ? glm::vec4(1) : glm::vec4(0.8f, 0.8f, 0.8f, 1));
        tint(doc, doc.find("frame"), over ? CYAN : current ? glm::vec4(0, 1, 0, 1) : glm::vec4(1));
        placeWidget(doc, 0, O + tabCorner(k));
        painter.draw(doc, 0, Transform(1));
    }
}

bool Interface::vendorPress(glm::vec2 W, glm::vec2 mouse, int button) {
    if (!inventory)
        return false;
    const glm::vec2 O = origin(W), local = mouse - O;
    vendorGrid.update(&vendorInventory, PANEL.x, PANEL.y, int32_t(ROW.x), int32_t(ROW.y));
    for (int k = 0; k < 2; k++)
        if (inside(local, tabCorner(k), {38, 38})) {
            if (button == 0 && vendorGrid.choose(k, vendorInventory, PANEL.x, PANEL.y, int32_t(ROW.x), int32_t(ROW.y))) {
                inventorySounds.push_back(creatures::SOUND_TAB);
                refreshVendor();
            }
            return true;
        }
    if (!inside(local, {0, 0}, PANEL))
        return false;
    const bool validGroup = vendorGrid.currentGroup >= 0 && size_t(vendorGrid.currentGroup) < vendorGrid.scroll.size();
    if (button == 0 && validGroup && inside(local, upCorner(), {21, 19}))
        vendorGrid.scroll[size_t(vendorGrid.currentGroup)] = std::max(vendorGrid.scroll[size_t(vendorGrid.currentGroup)] - 1, 0);
    else if (button == 0 && validGroup && inside(local, downCorner(), {21, 19}))
        vendorGrid.scroll[size_t(vendorGrid.currentGroup)] =
            std::min(vendorGrid.scroll[size_t(vendorGrid.currentGroup)] + 1, std::max(vendorGrid.range - 1, 0));
    const auto [first, end] = vendorGrid.visible(vendorInventory);
    vendorGrid.hover = {};
    for (int r = 0; r < ROWS; r++) {
        if (!inside(local, rowCorner(r), ROW))
            continue;
        const size_t i = first + size_t(r);
        if (i >= end) break;
        vendorGrid.hover = {vendorGrid.currentGroup, int32_t(i)};
        if (button == 0 && vendorGrid.select()) inventorySounds.push_back(0x55);
        if (button == 1 || button == 2)
            vendor::purchase(vendorInventory, vendorOpen, vendorGrid.hover, vendorGrid.currentGroup,
                             buyBack, inventory->money, vendorServices());
        break;
    }
    return true;
}

} // namespace ui
