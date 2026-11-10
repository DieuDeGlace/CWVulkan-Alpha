#include "creatures/SearchGeometry.h"
#include <cstdio>
#include <limits>

using namespace creatures::search;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
constexpr creatures::route::Dimensions size{2, 2, 2};
creatures::route::Position raw(Cell value) {
    return {int64_t(value[0]) * 65536, int64_t(value[1]) * 65536, int64_t(value[2]) * 65536};
}
Classification closed() {
    Classification c;
    c.lower = c.upper = true;
    c.faceX = c.faceY = {true, true};
    c.verticalCorner.fill(true);
    return c;
}
void setupAndClear() {
    check(coordinate(creatures::route::Position{-65536, -1, 65536}) == Cell{-2, -1, 1},
          "negative integral raw coordinates receive the extra grid decrement");
    State s;
    s.start = raw({8, 8, 5});
    s.goal = raw({16, 8, 5});
    s.radius = 4;
    s.reference = {3, 4, 5};
    s.counter = 99;
    std::vector<Cell> queries;
    const creatures::route::Record record{123, 0, {8 * 65536 + 32768, 8 * 65536 + 32768, 5 * 65536}, 0, size};
    s.setup(size, [&](int32_t x, int32_t y, const creatures::route::RecordVisitor &visit) {
        check(s.records.size() == 1 && s.pending.size() == 1 && s.counter == 0,
              "seed and zero counter are published before either endpoint scan");
        queries.push_back({x, y, 0});
        visit(record);
        visit(record);
    });
    check(s.records.at({8, 8, 5}) == Node{1, 160, 160, {8, 8, 5}},
          "seed selection is rounded destination score without adding accumulated one");
    check(queries.size() == 18 && queries.front() == Cell{0, 0, 0} && queries[8] == Cell{2, 2, 0} &&
          queries[9] == Cell{1, 0, 0} && queries.back() == Cell{3, 2, 0} && s.exclusions == std::set<uintptr_t>{123},
          "endpoint scans retain start/goal bucket order and one pointer union despite duplicates");
    s.path = {{1, 2, 3}};
    s.counter = 77;
    s.clear();
    check(s.records.empty() && s.pending.empty() && !s.remembered && s.path.empty() && s.exclusions.empty() &&
          s.start == raw({8, 8, 5}) && s.goal == raw({16, 8, 5}) && s.radius == 4 && s.counter == 77 &&
          s.anchor == Cell{8, 8, 5} && s.reference == Cell{3, 4, 5},
          "clear preserves supplied endpoint, radius, counter, anchor and reference storage");
    s.start = {};
    s.goal = raw({3355443, 3, 0});
    s.setup(size);
    check(destinationScore({}, {3355443, 3, 0}) == 67108884 && s.records.at({}).destination == 67108880,
          "large seed score crosses binary32 rounding before integer truncation");
}
void selectionAndLimits() {
    State s;
    int calls = 0;
    const auto noMoves = [&](const Cell &) { calls++; return closed(); };
    check(!s.step(noMoves) && calls == 0, "empty record map returns before geometry");
    s.goal = raw({100, 100, 100});
    for (const Cell key : {Cell{-1, 3, 5}, Cell{0, 0, 0}, Cell{-1, 3, 4}}) {
        s.records[key] = {1, 50, -7, key};
        s.pending.insert(key);
    }
    Cell chosen{};
    check(s.step([&](const Cell &key) { chosen = key; return closed(); }) && chosen == Cell{-1, 3, 4},
          "equal signed selection scores choose the first signed lexicographic pending key");
    check(!s.pending.contains(chosen) && s.records.contains(chosen), "selection removes only the pending entry");
    s.clear();
    s.goal = raw({1, 2, 3});
    s.records[{1, 2, 3}] = {5, 0, 5, {1, 2, 3}};
    s.pending.insert({1, 2, 3});
    s.remembered = Cell{1, 2, 3};
    s.counter = 17;
    s.path = {{9, 9, 9}};
    s.exclusions.insert(321);
    check(!s.step(noMoves) && s.pending.empty() && s.records.size() == 1 && s.counter == 17 &&
          s.path.size() == 1 && s.exclusions.contains(321) && calls == 0,
          "exact goal is popped before false return without clearing or querying geometry");
    check(!s.step(noMoves) && s.records.size() == 1 && s.remembered,
          "no-pending false retains map and remembered entry");
    s.clear();
    for (int i = 0; i < 501; i++) { s.records[{i, 0, 0}] = {}; s.pending.insert({i, 0, 0}); }
    s.path = {{9, 9, 9}};
    s.exclusions.insert(321);
    check(!s.step(noMoves) && s.records.empty() && s.pending.empty() && s.path.empty() && s.exclusions.empty(),
          "pending count501 clears search, list and pointer exclusions before expansion");
    s.goal = raw({100, 100, 100});
    for (int i = 0; i < 20000; i++) s.records[{i, 0, 0}] = {1000, 1, 1000, {i, 0, 0}};
    s.records.at({0, 0, 0}).selection = 0;
    for (int i = 0; i < 500; i++) s.pending.insert({i, 0, 0});
    check(s.step([](const Cell &) { return Classification{}; }) && s.records.size() == 20001 && s.pending.size() == 500,
          "count equality admits one expansion and does not clamp a new record at20000");
    check(!s.step(noMoves) && s.records.empty() && calls == 0,
          "over-limit record count is cleared on the subsequent call");
}
void transitions() {
    State s;
    s.goal = raw({0, 0, 10});
    s.setup(size);
    check(s.step([](const Cell &) { Classification c; c.sampleBit = true; return c; }), "ordinary expansion returns true");
    check(s.records.at({-1, 0, 0}).accumulated == 21 && s.records.at({1, 0, 0}).accumulated == 21 &&
          s.records.at({0, -1, 0}).accumulated == 31 && s.records.at({0, 1, 0}).accumulated == 21 &&
          s.records.at({-1, -1, 0}).accumulated == 25 && s.records.at({0, 0, -1}).accumulated == 1 &&
          s.records.at({0, 0, 1}).accumulated == 41,
          "sample-bit admission retains new Y-minus30, diagonal24 and vertical0/40 costs");
    check(s.remembered == Cell{0, 0, 1}, "remembered candidate follows destination score independently of selection score");
    s.clear();
    s.records[{}] = {5, 1000, -1, {}};
    s.pending.insert(Cell{});
    s.remembered = Cell{};
    s.records[{-1, 0, 0}] = {100, 500, 600, {-9, 0, 0}}; // closed: never reopened
    s.records[{1, 0, 0}] = {25, 600, 625, {-9, 0, 0}}; // equal proposed score
    s.records[{0, -1, 0}] = {100, 777, 877, {-9, 0, 0}};
    s.pending.insert({1, 0, 0});
    s.pending.insert({0, -1, 0});
    s.step([](const Cell &) { Classification c; c.sampleBit = true; return c; });
    check(s.records.at({-1, 0, 0}).parent == Cell{-9, 0, 0} && !s.pending.contains({-1, 0, 0}) &&
          s.records.at({1, 0, 0}).parent == Cell{-9, 0, 0},
          "closed and equal-accumulated neighbors retain scores and predecessors");
    check(s.records.at({0, -1, 0}) == Node{25, 777, 802, {}},
          "pending Y-minus improvement costs20 and retains its existing destination score");
    check(!s.records.contains({-1, -1, 0}) && !s.records.contains({1, -1, 0}) &&
          !s.records.contains({-1, 1, 0}) && !s.records.contains({1, 1, 0}),
          "unchanged cardinal records prevent diagonals despite clear geometry");
    s.clear();
    s.start = {};
    s.goal = raw({0, 0, 10});
    s.records[{}] = {1, 1000, 0, {}};
    s.pending.insert(Cell{});
    s.remembered = Cell{};
    s.step([](const Cell &) { Classification c; c.lower = true; c.upper = true; return c; });
    check(s.remembered == Cell{-1, 0, 0}, "equal destination-score publications retain the first produced candidate");
    s.clear();
    s.setup(size);
    s.step([&](const Cell &) { s.goal = raw({50, 0, 0}); Classification c; c.lower = true; return c; });
    check(s.records.at({1, 0, 0}).destination == 980 && s.records.at({}).destination == 200,
          "new neighbors convert the current goal while existing seed scores retain their epoch");
}
void partialLists() {
    State s;
    s.start = raw({99, 0, 0});
    s.goal = raw({5, 0, 0});
    s.path = {{8, 8, 8}};
    s.rebuild();
    check(s.path == std::vector<Cell>{{5, 0, 0}}, "empty map rebuild publishes the current goal as a partial terminal");
    s.remembered = Cell{1, 0, 0};
    s.records[{1, 0, 0}] = {7, 8, 9, {2, 0, 0}};
    s.rebuild();
    check(s.path == std::vector<Cell>{{2, 0, 0}, {1, 0, 0}}, "missing predecessor retains the partial list already built");
    s.records[{2, 0, 0}] = {10, 11, 12, {1, 0, 0}};
    s.rebuild();
    check(s.path == std::vector<Cell>{{2, 0, 0}, {1, 0, 0}, {2, 0, 0}, {1, 0, 0}},
          "two-record cycle allows count-plus-one predecessor lookups and retains repeated coordinates");
    s.records.at({1, 0, 0}).parent = {1, 0, 0};
    s.rebuild();
    check(s.path == std::vector<Cell>{{1, 0, 0}}, "self predecessor stops without duplicating its terminal");
    s.start = raw({1, 0, 0});
    s.rebuild();
    check(s.path.size() == 1, "terminal equal to start is still published once");
}

uint32_t terrainMask(const Classification &c) {
    uint32_t result = uint32_t(c.lower) | uint32_t(c.upper) << 1 | uint32_t(c.sampleBit) << 2;
    for (size_t i = 0; i < 2; i++) {
        result |= uint32_t(c.faceX[i]) << (3 + i);
        result |= uint32_t(c.faceY[i]) << (5 + i);
        result |= uint32_t(c.lowerX[i]) << (7 + i);
        result |= uint32_t(c.lowerY[i]) << (9 + i);
        result |= uint32_t(c.upperX[i]) << (11 + i);
        result |= uint32_t(c.upperY[i]) << (13 + i);
    }
    for (size_t i = 0; i < 4; i++) {
        result |= uint32_t(c.lowerCorner[i]) << (15 + i);
        result |= uint32_t(c.verticalCorner[i]) << (19 + i);
    }
    return result;
}
struct GeometryFixture {
    Cell selected{10, 20, 5}, reference{-1, -1, 0};
    creatures::route::Dimensions dimensions = size;
    Cell solidCell{std::numeric_limits<int32_t>::max(), 0, 0};
    uint8_t value = 1;
    std::vector<Cell> queries, buckets;
    std::vector<uintptr_t> pointers;
    std::vector<creatures::route::Record> records;
    uintptr_t excluded = 0;
    Classification run() {
        return classify(selected, {dimensions, reference, [&](uintptr_t pointer) {
            pointers.push_back(pointer); return pointer == excluded;
        }}, {[&](int32_t x, int32_t y, int32_t z) {
            const Cell at{x, y, z};
            queries.push_back(at);
            return at == solidCell ? value : uint8_t(0);
        }, [&](int32_t x, int32_t y, const creatures::route::RecordVisitor &visit) {
            buckets.push_back({x, y, 0});
            for (const auto &record : records) if (!visit(record)) break;
        }});
    }
};
void terrainClassification() {
    GeometryFixture empty;
    check(terrainMask(empty.run()) == 0 && empty.queries.size() == 99 && empty.queries.front() == Cell{10, 20, 6} &&
          empty.queries[1] == Cell{8, 18, 4} && empty.queries.back() == Cell{12, 22, 8},
          "classification samples first then all98 expanded boundary cells in X/Y/Z order");
    const std::array<std::pair<Cell, uint32_t>, 22> cases{{
        {{10, 20, 4}, 1u}, {{10, 20, 8}, 1u << 1},
        {{8, 20, 6}, 1u << 3}, {{12, 20, 6}, 1u << 4},
        {{10, 18, 6}, 1u << 5}, {{10, 22, 6}, 1u << 6},
        {{8, 20, 4}, 1u << 7}, {{12, 20, 4}, 1u << 8},
        {{10, 18, 4}, 1u << 9}, {{10, 22, 4}, (1u << 10) | (1u << 14)},
        {{8, 20, 8}, 1u << 11}, {{12, 20, 8}, 1u << 12}, {{10, 18, 8}, 1u << 13},
        {{10, 22, 8}, 0},
        {{8, 18, 4}, 1u << 15}, {{12, 18, 4}, 1u << 16},
        {{8, 22, 4}, 1u << 17}, {{12, 22, 4}, 1u << 18},
        {{8, 18, 6}, 1u << 19}, {{12, 18, 6}, 1u << 20},
        {{8, 22, 6}, 1u << 21}, {{12, 22, 6}, 1u << 22}}};
    for (const auto &[at, expected] : cases) {
        GeometryFixture f;
        f.solidCell = at;
        check(terrainMask(f.run()) == expected && f.queries.size() == 99,
              "one boundary cell sets its exact independent flags without suppressing later queries");
    }
    for (uint8_t value : {uint8_t(3), uint8_t(6), uint8_t(0x82)}) {
        GeometryFixture f;
        f.solidCell = {10, 20, 6};
        f.value = value;
        check(terrainMask(f.run()) == 4, "single interior sample retains bit1 independently of low-five-bit type");
    }
    GeometryFixture water;
    water.solidCell = {8, 20, 6};
    water.value = 0x82;
    check(terrainMask(water.run()) == 0, "boundary type2 does not set blocking flags even with high bits");
    GeometryFixture negative;
    negative.selected = {-1, -1, -1};
    negative.run();
    check(negative.queries.front() == Cell{-1, -1, 0} && negative.queries[1] == Cell{-3, -3, -2},
          "negative raw sample and expanded corners retain their distinct conversion boundaries");
    GeometryFixture inverted;
    inverted.dimensions[0] = -10;
    inverted.run();
    check(inverted.queries.size() == 1 && inverted.buckets.empty(), "inverted bounds retain the sample but suppress loops without normalization");
}
creatures::route::Record support() {
    return {123, 9, {10 * 65536 + 32768, 20 * 65536 + 32768, 4 * 65536}, 0, {2, 2, 3}, 0, {1, 2, 3}};
}
void recordClassification() {
    for (int32_t selector : {1, 2, 6, 7, 8}) {
        GeometryFixture f;
        auto record = support(); record.selector = selector;
        f.records.push_back(record);
        const auto c = f.run();
        check(c.recordBelow && c.recordSide == std::array<bool, 4>{}, "selector and zero state do not prevent the earlier below-record probe");
    }
    GeometryFixture all;
    all.records.push_back(support());
    const auto c = all.run();
    check(c.recordBelow && c.recordSide == std::array<bool, 4>{true, true, true, true},
          "selector9 with zero state admits all five record probes");
    GeometryFixture skipped;
    skipped.excluded = 123;
    skipped.records.push_back(support());
    const auto excluded = skipped.run();
    check(!excluded.recordBelow && excluded.recordSide == std::array<bool, 4>{}, "pointer exclusion precedes every record probe");
    GeometryFixture tag;
    tag.reference = {1, 2, 3};
    tag.records.push_back(support());
    const auto tagged = tag.run();
    check(tagged.recordBelow && tagged.recordSide == std::array<bool, 4>{}, "matching raw tag skips only the horizontal probes");
    GeometryFixture shortRecord;
    auto low = support(); low.size[2] = 2;
    shortRecord.records.push_back(low);
    const auto shortFlags = shortRecord.run();
    check(shortFlags.recordBelow && shortFlags.recordSide == std::array<bool, 4>{}, "height strictly below3 skips horizontal probes in an open upper box");
    GeometryFixture upper;
    upper.solidCell = {8, 20, 8};
    upper.records.push_back(low);
    check(upper.run().recordSide == std::array<bool, 4>{true, true, true, true}, "upper terrain flag admits a short record's horizontal probes");
    for (int side : {-1, 1}) {
        GeometryFixture edge;
        auto record = support(); record.position[0] += side * 2 * 65536;
        edge.records.push_back(record);
        check(edge.run().recordBelow == (side == 1), "record overlap retains inclusive lower and strict upper horizontal edges");
    }
    GeometryFixture odd;
    auto record = support();
    record.position[0] += 3 * 65536;
    record.size = {.1f, 10, 3};
    record.orientation = -1;
    odd.records.push_back(record);
    check(odd.run().recordBelow, "negative odd orientation swaps horizontal dimensions");
    GeometryFixture order;
    order.selected = {7, 7, 5};
    order.records = {support(), support()};
    order.records[1].identity = 456;
    order.run();
    check(order.buckets == std::vector<Cell>{{0, 0, 0}, {0, 1, 0}, {1, 0, 0}, {1, 1, 0}} &&
          order.pointers == std::vector<uintptr_t>{123, 456, 123, 456, 123, 456, 123, 456},
          "all coarse buckets retain X/Y and record order without deduplicating repeated pointers");
    Classification below;
    below.recordBelow = true;
    const std::array<bool, 4> updated{true, true, true, true};
    check(!below.allows(0, updated) && below.allows(1, updated) && below.allows(2, updated) && below.allows(3, updated) &&
          !below.allows(8, updated) && !below.allows(9, updated),
          "record-below support deliberately excludes X-minus and both vertical candidates");
}

void integratedSearch() {
    State s;
    s.start = {10 * 65536 + 32768, 20 * 65536 + 32768, 5 * 65536 + 6553};
    s.goal = {13 * 65536 + 32768, 22 * 65536 + 32768, 5 * 65536 + 6553};
    s.setup(size);
    const Scene scene{[](int32_t, int32_t, int32_t z) { return uint8_t(z <= 4 ? 1 : 0); }, {}};
    int expanded = 0;
    bool active;
    do {
        active = s.step([&](const Cell &at) {
            expanded++;
            return classify(at, {size, s.reference, [&](uintptr_t pointer) { return s.exclusions.contains(pointer); }}, scene);
        });
        s.rebuild();
    } while (active && expanded < 10);
    check(!active && expanded == 3 && s.path == std::vector<Cell>{{10, 20, 5}, {11, 21, 5}, {12, 22, 5}, {13, 22, 5}} &&
          !s.pending.contains({13, 22, 5}) && s.records.contains({13, 22, 5}),
          "real classifier and search produce the ordered flat-ground route then pop the exact goal without another classification");
}
}

int main() {
    setupAndClear(); selectionAndLimits(); transitions(); partialLists(); terrainClassification(); recordClassification();
    integratedSearch();
    if (failures) std::printf("%d search-state failures\n", failures);
    else std::puts("SearchStateTests PASS");
    return failures ? 1 : 0;
}
