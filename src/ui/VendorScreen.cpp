// The vendor window (docs/specs/alpha-talk-vendors.md §3, alpha-ui-screens.md §8.1.3 type 3):
// the vendor's stock and the buy-back list in wide rows with their prices, right of the
// inventory; buying, selling and buying back, all with the one price and no margin.
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
    if (stack.empty() || !inventory)
        return;
    const ItemRecord item = stack.item;
    if (--stack.count < 1) {
        stack.count = 0;
        stack.item[0x00] = stack.item[0x01] = 0;
    }
    inventory->money += (uint32_t)creatures::price(item);
    inventorySounds.push_back(0x3a); // drop-coin.wav
    buyBack.push_back({1, item});
    if (buyBack.size() > 10)
        buyBack.erase(buyBack.begin());
}

void Interface::drawVendor(Painter &painter, glm::vec2 W, glm::vec2 mouse) {
    vendorHovered.reset();
    const glm::vec2 O = origin(W), local = mouse - O;
    if (vendorPanel) {
        placeWidget(*vendorPanel, 0, O);
        painter.draw(*vendorPanel, 0, Transform(1));
    }
    outlined(painter, u"Vendor", O + glm::vec2(15, 25), 12, 3, glm::vec4(1));
    // Tab 0 the stock, tab 1 the buy-back list newest first.
    std::vector<std::pair<int32_t, ItemRecord>> rows;
    if (vendorTab == 0)
        for (const ItemRecord &item : vendorStock)
            rows.push_back({1, item});
    else
        for (auto it = buyBack.rbegin(); it != buyBack.rend(); ++it)
            rows.push_back({it->count, it->item});
    vendorRow = std::clamp(vendorRow, 0, std::max(0, (int32_t)rows.size() - ROWS));
    for (int r = 0; r < ROWS; r++) {
        const glm::vec2 corner = O + rowCorner(r);
        if (wideItemBox) {
            placeWidget(*wideItemBox, 0, corner);
            painter.draw(*wideItemBox, 0, Transform(1));
        }
        const size_t i = (size_t)(vendorRow + r);
        if (i >= rows.size())
            continue;
        const ItemRecord &item = rows[i].second;
        const bool over = inside(mouse, corner, ROW);
        if (over)
            vendorHovered = item;
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
        if (rows[i].first > 1)
            outlined(painter, number(rows[i].first), corner + glm::vec2(36, 37), 10, 2, glm::vec4(1), plasma::RIGHT);
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
    if (upButton && downButton && (int32_t)rows.size() > ROWS)
        for (auto [doc, at] : {std::pair{&*upButton, upCorner()}, std::pair{&*downButton, downCorner()}}) {
            tint(*doc, 0, inside(local, at, {21, 19}) ? CYAN : glm::vec4(1));
            placeWidget(*doc, 0, O + at);
            painter.draw(*doc, 0, Transform(1));
        }
    for (int k = 0; k < (int)vendorTabs.size(); k++) {
        plasma::Document &doc = vendorTabs[k];
        const bool over = inside(local, tabCorner(k), {38, 38}), current = k == vendorTab;
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
    for (int k = 0; k < 2; k++)
        if (inside(local, tabCorner(k), {38, 38})) {
            if (button == 0 && k != vendorTab) {
                vendorTab = k, vendorRow = 0;
                inventorySounds.push_back(creatures::SOUND_TAB);
            }
            return true;
        }
    if (!inside(local, {0, 0}, PANEL))
        return false;
    if (button == 0 && inside(local, upCorner(), {21, 19}))
        vendorRow = std::max(vendorRow - 1, 0);
    else if (button == 0 && inside(local, downCorner(), {21, 19}))
        vendorRow++;
    for (int r = 0; r < ROWS && button == 1; r++) {
        if (!inside(local, rowCorner(r), ROW))
            continue;
        const size_t i = (size_t)(vendorRow + r);
        const auto refuse = [&] {
            chatAdd("You can't carry more of these items.\n", {255, 51, 51});
            inventorySounds.push_back(creatures::SOUND_NOT_HERE);
        };
        if (vendorTab == 0 && i < vendorStock.size()) {
            // §3.4 buy: unlimited stock.
            const ItemRecord &item = vendorStock[i];
            const uint32_t cost = (uint32_t)creatures::price(item);
            if (cost > inventory->money)
                break;
            if (!inventory->canCarry(item)) {
                refuse();
                break;
            }
            inventory->add(item);
            inventory->money -= cost;
            inventorySounds.push_back(0x39); // drop.wav
        } else if (vendorTab == 1 && i < buyBack.size()) {
            // §3.4 buy back: the whole stack, as far as the money goes.
            creatures::Stack &entry = buyBack[buyBack.size() - 1 - i];
            const uint32_t cost = (uint32_t)creatures::price(entry.item);
            int32_t bought = 0;
            for (int32_t n = 0; n < entry.count; n++) {
                if (!inventory->canCarry(entry.item)) {
                    refuse();
                    break;
                }
                if (cost <= inventory->money) {
                    inventory->add(entry.item);
                    inventory->money -= cost;
                    bought++;
                }
            }
            entry.count -= bought;
            if (entry.count <= 0)
                buyBack.erase(buyBack.begin() + (ptrdiff_t)(buyBack.size() - 1 - i));
            inventorySounds.push_back(0x39);
        }
    }
    return true;
}

} // namespace ui
