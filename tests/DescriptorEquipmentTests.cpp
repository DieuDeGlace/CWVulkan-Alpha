#include "creatures/DescriptorEquipment.h"
#include "creatures/Npcs.h"
#include <bit>
#include <cstdio>
#include <cstring>

using namespace creatures;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
uint32_t seed(const ItemRecord &item) { uint32_t value; std::memcpy(&value, item.data() + 4, 4); return value; }
struct Fixture {
    int32_t kind = 118, role = 1, choice = 9;
    uint16_t flags = 0x20, level = 0x8001;
    uint8_t classification = 0, companion = 0x56;
    std::array<ItemRecord, 13> items{};
    std::array<float, 5> multipliers{100, 1.25f, 2, 3, 4};
    std::vector<int32_t> actions{1234}, candidates{-7}, values;
    std::function<void(int)> beforeDraw;
    int calls = 0;
    Fixture() {
        for (size_t slot = 0; slot < items.size(); slot++) {
            for (size_t byte = 0; byte < items[slot].size(); byte++)
                items[slot][byte] = uint8_t(slot * 13 + byte * 17);
            items[slot][0x0d] = uint8_t(0xa0 + slot);
            setItemLevel(items[slot], 0x1234);
        }
    }
    void run(int32_t option = 1) {
        equipment::setup({kind, role, level, flags, classification, companion, items, multipliers, actions, choice, candidates},
            option, [&] {
                if (beforeDraw) beforeDraw(calls);
                const int index = calls++;
                return size_t(index) < values.size() ? values[size_t(index)] : 1000 + index;
            });
    }
};
void earlyReturns() {
    Fixture stop;
    stop.flags = 0x240; stop.classification = 0x88; stop.kind = 117;
    const auto original = stop.items;
    stop.run(0);
    check(stop.calls == 0 && stop.items == original && stop.multipliers == std::array<float, 5>{100, 1.25f, 2, 3, 4} &&
          stop.actions == std::vector<int32_t>{1234} && stop.choice == 9 && stop.candidates == std::vector<int32_t>{-7},
          "flag40 precedes special classification, equipment, numeric, configuration and all draws");
    for (uint8_t classification = 0x80; classification <= 0x89; classification++) {
        Fixture f;
        f.classification = classification; f.kind = 117; f.flags = 0x200; f.role = 6;
        f.values = {-4};
        auto expected = f.items;
        expected[2][0] = 4; expected[2][1] = classification == 0x88 ? 0xfe : 0;
        expected[2][0x0d] = 6;
        f.beforeDraw = [&](int call) {
            check(call == 0 && f.items[2][0] == 4 && f.items[2][1] == 0 && f.items[2][0x0d] == 6,
                  "special classification publishes chest header/material before its only draw");
        };
        f.run(0);
        check(f.items == expected && f.calls == int(classification == 0x88) && f.classification == classification &&
              f.companion == 0x56 && f.multipliers == std::array<float, 5>{100, .3f, .1f, 10, 10} &&
              f.actions == std::vector<int32_t>{1234} && f.choice == 9 && f.candidates == std::vector<int32_t>{-7},
              "all ten special classes retain unlisted bytes/levels and bypass generic seeds and boss phase");
    }
}
void fixedEquipment() {
    struct Case { int32_t kind; uint8_t u, material, v, vMaterial; std::vector<int32_t> actions; int prefix; };
    const std::vector<Case> cases{
        {108, 17, 2, 0, 0xa6, {91, 93}, 0}, {114, 17, 2, 0, 0xa6, {91, 93}, 0},
        {46, 17, 2, 0, 0xa6, {}, 0}, {115, 4, 1, 4, 1, {86, 93}, 0},
        {109, 2, 7, 2, 7, {86, 93, 91}, 0}, {81, 12, 11, 12, 11, {87}, 0},
        {82, 0, 1, 0, 1, {}, 0}, {43, 11, 2, 0, 0xa6, {95}, 1}, {45, 11, 2, 0, 0xa6, {95}, 1}};
    for (const auto &entry : cases) {
        Fixture f;
        f.kind = entry.kind; f.values = {5};
        const auto original = f.items;
        auto expectedActions = std::vector<int32_t>{1234};
        expectedActions.insert(expectedActions.end(), entry.actions.begin(), entry.actions.end());
        f.run();
        check(f.items[7][0] == 3 && f.items[7][1] == entry.u && f.items[7][0x0d] == entry.material &&
              f.items[6][0] == (entry.v || entry.kind == 82 ? 3 : 0) && f.items[6][1] == entry.v &&
              f.items[6][0x0d] == entry.vMaterial && f.actions == expectedActions && f.calls == 6 + entry.prefix &&
              uint16_t(itemLevel(f.items[7])) == 0x8001 && uint16_t(itemLevel(f.items[6])) == 0x8001 &&
              f.items[8] == original[8] && f.items[9] == original[9],
              "fixed equipment appends actions and retains inactive fields, accessories and raw hand levels");
    }
    for (int32_t role : {6, 0x106}) {
        Fixture f; f.kind = 117; f.role = role;
        f.run(); f.run();
        check(f.calls == 12 && f.actions == (role == 6 ? std::vector<int32_t>{1234} : std::vector<int32_t>{1234, 92, 92}) &&
              f.candidates == (role == 6 ? std::vector<int32_t>{-7} : std::vector<int32_t>{-7, 17, 96, 17, 96}) &&
              f.choice == (role == 6 ? 9 : 0),
              "full role6 is distinct from106 and repeated setup retains duplicate appends");
    }
    Fixture wide;
    wide.kind = 0x10075; wide.values = {0, 0, 0, 0};
    wide.run();
    check(wide.classification == 1 && wide.calls == 10 && wide.actions == std::vector<int32_t>{1234} && wide.choice == 9,
          "full kind comparison does not narrow10075 to fixed kind75");
    Fixture untouched;
    untouched.flags = 8;
    const auto before = untouched.items;
    auto expected = before;
    for (size_t slot : {6u, 7u, 5u, 2u, 3u, 4u, 1u}) expected[slot][0] = expected[slot][1] = 0;
    for (size_t slot : {7u, 6u}) setItemLevel(expected[slot], std::bit_cast<int16_t>(uint16_t(0x8001)));
    int value = 1000;
    for (size_t slot : {6u, 7u, 5u, 2u, 4u, 3u}) { std::memcpy(expected[slot].data() + 4, &value, 4); value++; }
    untouched.run();
    check(untouched.items == expected && untouched.calls == 6, "generic clears affect only specified bytes; every other byte survives");
}
void randomHands() {
    struct Case { std::vector<int32_t> input; uint8_t cls, uType, u, vType, v, vMaterial; int count; };
    const std::vector<Case> cases{
        {{0, 1, 0, 2}, 1, 3, 17, 0, 0, 0xa6, 10},
        {{0, 1, 1, 2, 1}, 1, 3, 2, 3, 1, 1, 11},
        {{0, 1, 2, 1}, 1, 3, 1, 3, 13, 1, 10},
        {{0, 1, -1}, 1, 0, 0, 0, 0, 0xa6, 9},
        {{1, 0, -2}, 2, 3, 4, 0, 0, 0xa6, 9},
        {{2, 0, 0}, 3, 3, 10, 0, 0, 0xa6, 9},
        {{2, 0, 1}, 3, 3, 11, 0, 0, 0xa6, 9},
        {{2, 0, 2}, 3, 3, 12, 3, 12, 11, 9},
        {{2, 0, -1}, 3, 0, 0, 0, 0, 0xa6, 9},
        {{3, 0, 2}, 4, 3, 5, 0, 0, 0, 9},
        {{3, 0, -2}, 4, 3, 1, 3, 1, 1, 9},
        {{-2}, 255, 0, 0, 0, 0, 0xa6, 7}};
    for (const auto &entry : cases) {
        Fixture f; f.kind = 2; f.values = entry.input;
        f.run();
        check(f.classification == entry.cls && f.items[7][0] == entry.uType && f.items[7][1] == entry.u &&
              f.items[6][0] == entry.vType && f.items[6][1] == entry.v && f.items[6][0x0d] == entry.vMaterial &&
              f.calls == entry.count && f.actions == std::vector<int32_t>{1234},
              "random hand branches retain signed remainder, fallback and exact draw distinctions");
        if (entry.cls == 255) check(f.companion == 0x56, "unrecognized classification retains companion and skips its draw");
    }
    Fixture forced; forced.kind = 97; forced.values = {0, 1, 0}; forced.run();
    check(forced.classification == 4 && forced.companion == 1 && forced.items[7][1] == 3 && forced.calls == 9 && seed(forced.items[6]) == 1003,
          "forced class4 still consumes the discarded classification draw before companion, hand and seeds");
    Fixture trace; trace.kind = 2; trace.values = {0, 0, 1, 2, 1};
    trace.beforeDraw = [&](int call) {
        if (call == 3) check(trace.items[7][0] == 3 && trace.items[6][0] == 0, "class1 publishes U type before its subtype draw");
        if (call == 4) check(trace.items[6][0] == 3 && trace.items[7][1] == 2 && trace.items[7][0x0d] == 1,
                             "dual-hand V subtype draw follows U subtype and V type publication");
    };
    trace.run();
}
void optionalEquipment() {
    Fixture f; f.kind = 2; f.flags = 0;
    f.values = {0, 0, 0, 0, 1, 0, -1, 0, -1, 1, 0, 0};
    const auto original = f.items;
    f.run();
    check(f.calls == 18 && f.items[2][0] == 4 && f.items[2][0x0d] == 1 && f.items[5][0] == 7 &&
          f.items[4][0] == 0 && itemLevel(f.items[4]) == 0x1234 && f.items[4][0x0d] == 0xa4 && f.items[3][0] == 6 &&
          f.items[1][0] == 8 && f.items[1][0x0d] == 11 && f.items[9] == original[9] &&
          f.items[8][0] == 9 && f.items[8][1] == original[8][1] && f.items[8][0x0d] == 12 &&
          uint16_t(itemLevel(f.items[8])) == 0x8001 && seed(f.items[6]) == 1012 && seed(f.items[3]) == 1017,
          "optional gates retain failures, negative parity, ring subtype and conditional-draw ordering before all six seeds");
}
void numericSetup() {
    struct Case { int32_t kind; std::array<float, 5> values; int32_t action = 0; };
    const std::vector<Case> cases{
        {17, {400, 1.25f, 1.5f, 3, 3}, 93}, {94, {400, 1.25f, 1.5f, 3, 3}, 93}, {97, {400, 1.25f, 1.5f, 3, 3}, 93},
        {21, {250, 1.25f, 2, 3, 3}, 69}, {25, {200, 1.25f, .5f, 3, 3}},
        {46, {250, 1.25f, 2, 3, 3}, 86}, {82, {250, 1.25f, 2, 3, 3}, 86},
        {47, {250, .625f, 3, 3, 3}}, {88, {250, .625f, 3, 3, 3}}, {81, {250, 1.25f, 2, 3, 3}, 92},
        {85, {1000, 1.25f, 2, 3, 3}}, {117, {2500, 1.25f, 5, 4, 5}}, {119, {2500, .5f, 6, 10, 10}},
        {110, {100, 1.25f, 2, 3, 4}}, {0x10011, {100, 1.25f, 2, 3, 4}}};
    for (const auto &entry : cases) {
        Fixture f; f.kind = entry.kind; f.role = 6;
        f.run(0x100);
        auto actions = std::vector<int32_t>{1234};
        if (entry.action) actions.push_back(entry.action);
        check(f.multipliers == entry.values && f.actions == actions && f.calls == 6,
              "numeric full-kind table follows low-byte option0 despite role6 and retains omitted multipliers");
    }
    for (int32_t kind : {107, 108, 109, 111, 112, 113, 114, 115, 116, 118}) {
        Fixture f; f.kind = kind; f.role = 6; f.run(0);
        check(f.multipliers == std::array<float, 5>{2500, .5f, 6, 5, 5}, "complete high-kind table excludes110 and uses the reached hand predicate");
    }
    Fixture predicate; predicate.kind = 108; predicate.run(0);
    check(predicate.multipliers[1] == .75f, "numeric hand predicate sees the previously installed type3/subtype17");
    Fixture special; special.kind = 86; special.role = 6; special.run(0);
    check(special.classification == 3 && special.companion == 1 && special.multipliers == std::array<float, 5>{100, 1.25f, 2, 3, 4},
          "kind86 changes class after equipment without inventing a numeric row");
    Fixture skipped; skipped.kind = 17; skipped.role = 6; skipped.run(0x101);
    check(skipped.multipliers[0] == 100 && skipped.actions == std::vector<int32_t>{1234} && skipped.calls == 6,
          "nonzero low option byte skips numeric phase but retains common hand levels and tail");
    Fixture boss; boss.kind = 117; boss.flags = 0x200; boss.values = {-1, -1};
    boss.beforeDraw = [&](int call) {
        if (call == 0) check(boss.multipliers == std::array<float, 5>{10000, .9375f, 15, 6, 7} && boss.choice == 0,
                             "boss numeric phase precedes pool selection and preserves earlier kind117 choice");
        if (call == 1) check(boss.actions == std::vector<int32_t>{1234, 92, 92}, "unsigned pool draw appends before separate signed choice draw");
    };
    boss.run(0);
    check(boss.calls == 8 && boss.choice == -1 && boss.candidates == std::vector<int32_t>{-7, 17, 96, 17, 96} &&
          seed(boss.items[6]) == 1002 && seed(boss.items[3]) == 1007,
          "boss configuration preserves signed choice, duplicated candidates and six subsequent seed draws");
}
void actorBacking() {
    Combatant actor;
    actor.type = 118;
    ItemRecord source;
    source.fill(0xa5); source[0] = 9; source[1] = 17;
    const int32_t count = 0;
    std::memcpy(source.data() + 0x114, &count, 4);
    actor.setEquipment(8, source);
    actor.setEquipment(5, source);
    const auto before = actor.equipmentRecord(8);
    Appearance looks;
    creation::Configuration configuration;
    int calls = 0;
    equip(actor, looks, [&] { return ++calls; }, false, 1, configuration);
    const auto after = actor.equipmentRecord(5);
    check(actor.equipmentRecord(8) == before && after[0] == 0 && after[1] == 0 && after[0x0d] == 0xa5 &&
          after[2] == 0xa5 && after[0x13] == 0xa5 && after[0x100] == 0xa5 && calls == 6,
          "production actor adapter preserves complete untouched accessory and inactive item payload/gaps across partial setup");
}
}

int main() {
    earlyReturns(); fixedEquipment(); randomHands(); optionalEquipment(); numericSetup(); actorBacking();
    if (failures) std::printf("%d descriptor-equipment failures\n", failures);
    else std::puts("DescriptorEquipmentTests PASS");
    return failures ? 1 : 0;
}
