#include "ui/Vendor.h"
#include "ui/Screens.h"
#include "creatures/Inventory.h"
#include <bit>
#include <cstdio>
#include <limits>
#include <string>

using namespace creatures;
using namespace ui::vendor;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
static ItemRecord item(uint8_t type, uint8_t subtype = 0, int16_t level = 1) {
    auto value = freshInventoryItem();
    value[0] = type; value[1] = subtype;
    setItemLevel(value, level);
    return value;
}
struct Trace {
    std::vector<std::string> events;
    Services services;
    Trace() {
        services.price = [&](const ItemRecord &i) { events.push_back("price" + std::to_string(i[0])); return 2; };
        services.canCarry = [&](const ItemRecord &i) { events.push_back("capacity" + std::to_string(i[0])); return true; };
        services.add = [&](const ItemRecord &i) { events.push_back("add" + std::to_string(i[0])); };
        services.feedback = [&](int32_t sound) { events.push_back("sound" + std::to_string(sound)); };
        services.carryLimit = [&] { events.push_back("limit"); };
        services.refresh = [&] { events.push_back("refresh"); };
        services.updatePlayer = [&] { events.push_back("player"); };
        services.continueInterface = [&] { events.push_back("continue"); };
    }
};

static void rowsAndCopies() {
    InventoryValue buffer;
    buffer.groups.resize(3);
    for (int i = 0; i < 21; ++i) buffer.groups[0].push_back({i % 3 - 1, item(1, uint8_t(i))});
    buffer.groups[2] = {{-7, item(4)}};
    Grid grid;
    grid.update(&buffer, 100, 130, 40, 40);
    check(grid.columns == 2 && grid.rows == 2 && grid.range == 10 && grid.scroll.size() == 3,
          "layout counts every zero/negative row with separately rounded dimensions");
    grid.scroll[0] = 20;
    grid.update(&buffer, 100, 130, 40, 40);
    check(grid.scroll[0] == 9 && grid.visible(buffer) == std::pair<size_t, size_t>{18, 21},
          "high scroll clamps to M-1 and visible entries retain actual indices");
    grid.scroll[0] = -5;
    grid.update(&buffer, 100, 130, 40, 40);
    check(grid.scroll[0] == -5, "layout does not normalize a negative existing scroll");
    check(grid.choose(1, buffer, 100, 130, 40, 40) && grid.range == 0 && grid.scroll[1] == 0 && grid.selected == Index{},
          "empty group selects -1/-1 and has zero scroll range");
    check(grid.choose(7, buffer, 100, 130, 40, 40) && grid.range == 1 && grid.selected == Index{},
          "unavailable group uses range one without selecting an invented row");
    grid.update(nullptr, 0, 0, 0, 0);
    check(grid.range == 1, "null inventory makes layout a no-op");
    grid.hover = {2, 0};
    check(grid.select() && grid.selected == grid.hover, "selection uses hover indices without quantity admission");
    check(lookup(buffer, true, {0, 0}) == &buffer.groups[0][0].item &&
          lookup(buffer, true, {2, 0}) == &buffer.groups[2][0].item && !lookup(buffer, false, {0, 0}) &&
          !lookup(buffer, true, {-1, 0}) && !lookup(buffer, true, {0, 21}),
          "vendor lookup admits zero/negative quantities and validates only activity and hover bounds");
    check(quantityText(1, 1) && quantityText(1, 21) && !quantityText(0, 1) &&
          !quantityText(-1, 11) && !quantityText(3, 20) && !quantityText(2, 3),
          "quantity text has its separate positive-count category predicate");
    Buyback live{{0, item(4)}, {-3, item(5)}, {2, item(6)}};
    buffer.auxiliary = 0x12345678;
    buffer.accumulators = {0x80000000, 0xffffffff};
    buffer.separate[2] = 0x66;
    refresh(buffer, nullptr, 1, live);
    check(buffer.groups.size() == 2 && buffer.groups[0].size() == 21 &&
          buffer.groups[1][0].count == 2 && buffer.groups[1][1].count == -3 && buffer.groups[1][2].count == 0 &&
          buffer.groups[1][2].item[0] == 4 && buffer.auxiliary == 0x12345678 && buffer.separate[2] == 0x66 &&
          buffer.accumulators[1] == 0xffffffff,
          "group-one refresh reverses all live entries and retains other buffer members");
    live.back().item[0] = 9;
    refresh(buffer, nullptr, 0, live);
    check(buffer.groups[1][0].item[0] == 6, "null actor retains an independently owned stale buffer");
}

static void purchases() {
    InventoryValue buffer;
    buffer.groups = {{{0, item(3)}}};
    Buyback live;
    uint32_t money = 10;
    Trace normal;
    int calls = 0;
    normal.services.price = [&](const ItemRecord &) { normal.events.push_back("price"); return ++calls == 1 ? 5 : 7; };
    check(purchase(buffer, true, {0, 0}, 0, live, money, normal.services) && money == 3 && buffer.groups[0][0].count == 0 &&
          normal.events == std::vector<std::string>{"price", "capacity3", "add3", "price", "sound57", "player", "continue"},
          "normal purchase reprices after add, keeps zero stock and does not refresh vendor");
    Trace denied;
    money = 0x80000001;
    purchase(buffer, true, {0, 0}, 0, live, money, denied.services);
    check(money == 0x80000001 && denied.events == std::vector<std::string>{"price3"},
          "payment affordability uses a signed word and stops before capacity");
    denied.events.clear(); money = 20;
    denied.services.canCarry = [&](const ItemRecord &) { denied.events.push_back("capacity"); return false; };
    purchase(buffer, true, {0, 0}, 0, live, money, denied.services);
    check(money == 20 && denied.events == std::vector<std::string>{"price3", "capacity", "limit", "sound49"},
          "capacity rejection has notification/feedback without payment or insertion");
    buffer.groups[0][0].item = item(0);
    Trace zero;
    Inventory owner;
    owner.money = 5;
    zero.services.add = [&](const ItemRecord &i) { owner.add(i); zero.events.push_back("add0"); };
    purchase(buffer, true, {0, 0}, 0, live, owner.money, zero.services);
    check(owner.money == 3 && owner.bags.empty() && zero.events[2] == "add0",
          "a category-zero add no-op does not cancel the caller's ordered payment");

    buffer.groups[0][0].item = item(3);
    live = {{2, item(4)}, {3, item(6)}};
    money = 7;
    Trace mismatch;
    mismatch.services.price = [&](const ItemRecord &i) { mismatch.events.push_back("price" + std::to_string(i[0])); return i[0] == 3 ? 2 : 5; };
    purchase(buffer, true, {0, 0}, 1, live, money, mismatch.services);
    check(money == uint32_t(-3) && live.back().count == 1 && live.front().count == 2 &&
          mismatch.events == std::vector<std::string>{"capacity6", "price3", "add6", "price6", "capacity6", "price3", "add6", "price6",
                                                      "capacity6", "price3", "sound57", "refresh", "player", "continue"},
          "buyback maps live index separately, affords displayed price, pays live price and retains partial units");
    Trace poor;
    live = {{3, item(4)}}; money = 0;
    purchase(buffer, true, {0, 0}, 1, live, money, poor.services);
    check(poor.events == std::vector<std::string>{"capacity4", "price3", "capacity4", "price3", "capacity4", "price3",
                                                "sound57", "refresh", "player", "continue"} && live.front().count == 3,
          "unaffordable units still attempt every capacity/price call and reach feedback/refresh");
    Trace partial;
    live = {{3, item(4)}}; money = 10; int capacity = 0;
    partial.services.canCarry = [&](const ItemRecord &) { return ++capacity == 1; };
    purchase(buffer, true, {0, 0}, 1, live, money, partial.services);
    check(money == 8 && live.front().count == 2 && capacity == 2 && partial.events[3] == "limit" &&
          partial.events[4] == "sound49" && partial.events[5] == "sound57", "later capacity failure preserves earlier additions/payments");
    Trace changing;
    live = {{3, item(4)}}; money = 10;
    changing.services.add = [&](const ItemRecord &) { live.front().count = 1; };
    purchase(buffer, true, {0, 0}, 1, live, money, changing.services);
    check(money == 8 && live.empty(), "unit loop rereads live quantity instead of freezing its initial bound");
    for (int32_t count : {0, -3}) {
        Trace empty;
        live = {{count, item(4)}}; money = 10;
        purchase(buffer, true, {0, 0}, 1, live, money, empty.services);
        check(live.empty() == (count == 0) && money == 10 &&
              empty.events == std::vector<std::string>{"sound57", "refresh", "player", "continue"},
              "post-loop exact-zero erase removes zero nodes but preserves negative nodes");
    }
}

static void salesAndCapacity() {
    Trace sale;
    Buyback live;
    uint32_t money = 0;
    ItemRecord source = item(3, 7);
    source[2] = 0x99;
    int32_t count = -1;
    sale.services.price = [&](const ItemRecord &i) {
        check(count == 0 && source[0] == 0 && i[0] == 3, "bag sale copies before decrement and prices saved data after source clear");
        return 7;
    };
    sale.services.refresh = [&] { check(live.size() == 1 && live.back().count == 1, "sale publishes count-one node before refresh"); };
    sellEntry(count, source, live, money, sale.services);
    check(money == 7 && source[2] == 0x99 && live.back().item[2] == 0 && live.back().item[1] == 7,
          "sale accepts negative nonzero quantity, retains source gaps and semantically copies list item");
    Trace slot;
    source = item(4);
    slot.services.price = [&](const ItemRecord &i) { check(&i == &source && source[0] == 4, "direct sale prices original slot before copy"); return 3; };
    slot.services.refresh = [&] { check(source[0] == 4 && live.back().item[0] == 4 && money == 10,
                                      "direct source remains set through credit, list publication and refresh"); };
    sellSlot(source, live, money, slot.services);
    check(source[0] == 0 && source[1] == 0, "direct sale clears header only after refresh");
    Trace bounded;
    for (int i = 0; i < 12; ++i) { source = item(3, uint8_t(i)); count = 1; sellEntry(count, source, live, money, bounded.services); }
    check(live.size() == 10 && live.front().item[1] == 2 && live.back().item[1] == 11, "buyback tail append retains ten newest nodes without merging");
    source = item(3); count = std::numeric_limits<int32_t>::min();
    sellEntry(count, source, live, money, bounded.services);
    check(count == std::numeric_limits<int32_t>::max() && source[0] == 3, "sale decrement wraps before signed normalization");

    Inventory owner;
    auto herb = item(11, 0, 1);
    owner.bags = {{{49, herb}}};
    owner.hand = {1, herb};
    check(!owner.canCarry(herb), "capacity includes the separate hand quantity");
    setItemLevel(herb, 7);
    check(owner.canCarry(herb), "capacity compares the supplied item without quantity-word normalization");
    setItemLevel(herb, 1);
    owner.hand = {};
    owner.bags = {{{std::numeric_limits<int32_t>::max(), herb}, {1, herb}, {0, herb}}};
    check(owner.canCarry(herb), "capacity wraps raw 32-bit sums before signed limits");
    auto pet = item(20);
    owner.bags = {{{-2, pet}, {3, pet}}};
    check(!owner.canCarry(pet), "category twenty rejects signed matching total one");
    owner.bags[0][1].count = 1;
    check(owner.canCarry(pet), "capacity retains negative matching contributions");
}

static void pricesAndPublicInput() {
    auto weapon = item(3, 5, -7); weapon[0x0c] = 255;
    int calls = 0;
    check(price(weapon, [&](float level, uint32_t rarity) {
        check(level == -7 && rarity == 255, "price scalar receives signed level and zero-extended rarity");
        return ++calls == 1 ? 1.f : -2.f;
    }) == -80 && calls == 2, "price recomputes the scalar after its first minimum test without a second clamp");
    calls = 0;
    check(price(weapon, [&](float, uint32_t) { ++calls; return 0.f; }) == 1 && calls == 1, "first nonpositive price returns one without re-evaluation");
    check(price(item(23), [](float, uint32_t) { return 1.f; }) == 1000 &&
          price(item(8), [](float, uint32_t) { return 1.f; }) == 15 &&
          price(item(3, 0), [](float, uint32_t) { return 1.f; }) == 20 &&
          price(item(1)) == 2, "price category multipliers and default level-one anchor");

    Inventory owner = Inventory::fresh(); owner.money = 20;
    ui::Interface ui;
    ui.inventory = &owner;
    ui.vendorOpen = ui.inventoryOpen = true;
    InventoryValue stock; stock.groups = {{{0, item(1, 1)}, {-4, item(3)}}};
    int exports = 0;
    ui.vendorExport = [&](InventoryValue &buffer) { ++exports; buffer.copyFrom(stock); };
    ui.refreshVendor();
    const glm::vec2 window{1280, 720}, row{880, 135};
    check(ui.inventoryPress(window, row, 0) && ui.vendorGrid.selected == Index{0, 0}, "public input selects zero-quantity actual row");
    ui.inventoryPress(window, row, 1);
    check(owner.money == 18 && owner.bags[1][0].count == 1 && ui.vendorInventory.groups[0][0].count == 0 && exports == 1,
          "public input buys from grouped zero-quantity stock without decrement or vendor refresh");
    ui.inventoryPress(window, {460, 135}, 1); // player's bag-zero first slot
    owner.bags[0][0] = {1, item(3)};
    ui.inventoryPress(window, {460, 135}, 1);
    check(owner.bags[0][0].count == 0 && ui.buyBack.size() == 1 && exports == 2, "public bag sale reaches list publication and vendor refresh");
    ui.inventoryPress(window, {904, 44}, 0);
    check(ui.vendorGrid.currentGroup == 1 && ui.vendorInventory.groups[1].size() == 1 && exports == 2,
          "public tab change refreshes group one from live buyback rather than actor stock");
    ui.inventoryPress(window, row, 2);
    check(ui.buyBack.empty() && ui.vendorInventory.groups[1].empty() && owner.bags[0][0].count == 1,
          "second transaction input route buys back the live node and refreshes the displayed group");
}

int main() {
    rowsAndCopies(); purchases(); salesAndCapacity(); pricesAndPublicInput();
    if (!failures) std::puts("PASS: grouped vendor rows, ordered transactions, raw capacity, prices and public input");
    return failures ? 1 : 0;
}
