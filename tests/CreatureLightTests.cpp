#include "CreatureLights.h"
#include "creatures/Combat.h"
#include "net/Protocol.h"
#include <bit>
#include <cstdio>
#include <limits>

using namespace terrain_lights;
using namespace creatures;

static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { ++failures; std::printf("FAIL %s\n", message); }
}
static Item spirit(uint8_t type, uint8_t material) {
    Item item;
    item.type = type;
    item.spirits = 1;
    item.cubes = {{0, 0, 0, material}};
    return item;
}
template<size_t N> static bool empty(const std::array<std::optional<Candidate>, N> &sources) {
    for (const auto &source : sources)
        if (source)
            return false;
    return true;
}
static bool colourBits(glm::vec3 colour, std::array<uint32_t, 3> expected) {
    return std::bit_cast<uint32_t>(colour.r) == expected[0] &&
           std::bit_cast<uint32_t>(colour.g) == expected[1] &&
           std::bit_cast<uint32_t>(colour.b) == expected[2];
}

static void bodyLights() {
    Combatant creature;
    const FixedPosition main{(int64_t(1) << 54) + 1, -17, 33}, rendered{-5, 7, -9};
    constexpr std::array<glm::vec3, 4> colours{{{.1f, .5f, .1f}, {.1f, .1f, .5f},
                                               {.5f, .1f, .2f}, {.8f, .8f, .6f}}};
    for (int32_t kind = 135; kind <= 138; ++kind) {
        creature.type = kind;
        const auto lights = bodySources(creature, main, rendered);
        check(lights[0] && !lights[1] && lights[0]->position == FixedPosition{main[0], main[1], 98337} &&
                  lights[0]->colour == colours[size_t(kind - 135)] && lights[0]->radius == 16,
              "kinds135..138 use exact main-position Z+1.5 and their separate constant colors");
    }
    creature.type = 117;
    const auto displayed = bodySources(creature, main, rendered);
    check(displayed[0] && displayed[0]->position == rendered && displayed[0]->colour == glm::vec3(0, .4f, .8f),
          "kind117 uses the supplied rendered triplet rather than main or shifted main position");
    creature.appearanceFlags = 0x0200;
    creature.controller = 1;
    const auto combined = bodySources(creature, main, rendered);
    check(combined[0] && combined[1] && combined[1]->position == rendered && combined[1]->radius == 16 &&
              combined[1]->colour == glm::vec3(.4f, 0, .25f),
          "the independent role-one flag source follows the kind source without merging at the same point");
    creature.type = 2;
    for (uint8_t role : {uint8_t(0), uint8_t(5), uint8_t(255)}) {
        creature.controller = role;
        const auto lights = bodySources(creature, main, rendered);
        check(!lights[0] && lights[1] && lights[1]->colour == glm::vec3(.5f),
              "all roles other than one use neutral flag-source RGB");
    }
    creature.appearanceFlags = 0x0002;
    check(empty(bodySources(creature, main, rendered)), "the flag test uses bit0x0200 rather than its low-byte counterpart");
    creature.type = 134;
    check(empty(bodySources(creature, main, rendered)), "nearby unlisted kinds do not acquire invented glow");
    creature.type = 135;
    for (float health : {0.f, -1.f, std::numeric_limits<float>::quiet_NaN()}) {
        creature.health = health;
        check(empty(bodySources(creature, main, rendered)), "body sources require ordered positive health");
    }
}

static void receivedLights() {
    net::CreatureRecord record;
    Appearance appearance;
    appearance.flags = 0x0210;
    record.setAppearance(appearance);
    Combatant creature;
    creature.type = 117;
    creature.controller = 1;
    record.setFighter(creature);
    const auto before = record.bytes;
    const auto decoded = record.fighter();
    const auto lights = bodySources(decoded, {}, {1, 2, 3});
    check(decoded.appearanceFlags == 0x0210 && record.bytes == before && lights[0] && lights[1] &&
              lights[1]->colour == glm::vec3(.4f, 0, .25f),
          "received fighter views carry appearance flags without mutating the stored record");
    net::CreatureRecord changed = record;
    appearance.flags = 0;
    changed.setAppearance(appearance);
    const auto packet = net::encodeDelta(42, changed, uint64_t(1) << 13);
    check(net::applyDelta(packet, record) == 42, "appearance-only wire delta is accepted");
    const auto after = bodySources(record.fighter(), {}, {1, 2, 3});
    check(after[0] && !after[1] && record.fighter().type == 117,
          "an appearance-only update removes the flag light while retaining the kind source and other core fields");
}

static int speedQueries = 0;
static float suppliedSpeed = 2;
static float querySpeed(const Combatant &) { ++speedQueries; return suppliedSpeed; }

static void yawLights() {
    Combatant creature;
    creature.equipment[MAIN_HAND].subtype = 20; // The gate deliberately has no category check.
    creature.equipment[OFF_HAND].subtype = 20;
    creature.mode = 0x0200;
    creature.equipment[10].type = 24;
    creature.equipment[10].rarity = 255;
    creature.buffs = {{9, 0, -1, 7}, {9, 1, 0, 8}};
    const auto firstBuff = creature.buffs.front().serialize();
    const FixedPosition base{(int64_t(1) << 54) + 1, -17, 33};
    const auto lights = yawSources(creature, base, 0, 10);
    check(lights[0] && lights[1] && lights[2], "all three independent yaw gates contribute once");
    if (lights[0] && lights[1] && lights[2]) {
        const FixedPosition expected{base[0] + 32768, base[1] + 65536, base[2] + 196608};
        check(lights[0]->position == expected && lights[1]->position == expected && lights[2]->position == expected &&
                  lights[0]->radius == 32 && lights[1]->radius == 2050 && lights[2]->radius == 16,
              "separate yaw sources preserve raw rendered coordinates and the full unsigned lantern range");
        check(lights[0]->colour == glm::vec3(1.2f, 1, .5f) &&
                  colourBits(lights[1]->colour, {0x421acccd, 0x42010000, 0x41ce6667}) &&
                  lights[2]->colour == glm::vec3(.6f, .4f, 0),
              "yaw colors retain source order and unclamped lantern scaling");
    }
    check(creature.buffs.size() == 2 && creature.buffs.front().serialize() == firstBuff,
          "type9 presence neither expires nor consumes a resident record");
    const auto quarter = yawSources(creature, {}, 90, -.1f);
    check(quarter[0] && quarter[0]->position == FixedPosition{-65536, 32767, -1966},
          "rounded ninety-degree trig and negative height convert by truncation before integer placement");
    creature.equipment[MAIN_HAND].subtype = creature.equipment[OFF_HAND].subtype = 0;
    creature.mode = 2;
    creature.buffs.clear();
    check(empty(yawSources(creature, {}, 0, 10)), "low-byte mode bit2 does not substitute for word bit0200");
    creature.mode = 0x0200;
    creature.equipment[10].type = 23;
    check(empty(yawSources(creature, {}, 0, 10)), "mode alone does not bypass the lantern category gate");
    creature.equipment[MAIN_HAND].subtype = 20;
    check(empty(yawSources(creature, {}, std::numeric_limits<float>::infinity(), 10)),
          "out-of-contract nonfinite placements use the engine omission policy without invalid integer casts");
    creature.health = 0;
    check(empty(yawSources(creature, {}, 0, 10)), "dead actors do not emit yaw sources");
}

static void actionLights() {
    Combatant creature;
    speedQueries = 0;
    suppliedSpeed = 2;
    for (int action : {28, 36, 94, 95}) {
        creature.action = action;
        creature.actionMs = std::numeric_limits<int32_t>::max();
        check(bool(actionSource(creature, {}, 0, querySpeed)), "unconditional action sites do not test their clock");
    }
    creature.action = 45;
    check(!actionSource(creature, {}, 0, querySpeed) && speedQueries == 0,
          "unconditional and unsupported actions never call the threshold stat helper");
    const std::array<std::pair<int, int>, 8> thresholds{{{37, 600}, {38, 250}, {43, 600}, {44, 250},
                                                       {87, 2500}, {88, 200}, {89, 600}, {92, 200}}};
    for (const auto &[action, threshold] : thresholds) {
        creature.action = action;
        creature.actionMs = threshold - 1;
        const int previous = speedQueries;
        check(bool(actionSource(creature, {}, 0, querySpeed)) && speedQueries == previous + 1,
              "each conditional action queries its stat once and admits the integer below its threshold");
        creature.actionMs = threshold;
        check(!actionSource(creature, {}, 0, querySpeed), "equality rejects every conditional action");
    }
    creature.action = 37;
    creature.equipment[MAIN_HAND].subtype = 12;
    creature.equipment[MAIN_HAND].type = 0;
    creature.actionMs = 299;
    check(bool(actionSource(creature, {}, 0, querySpeed)), "raw main subtype12 selects numerator600 without a category test");
    creature.actionMs = 300;
    check(!actionSource(creature, {}, 0, querySpeed), "subtype12 boundary remains a signed integer comparison");
    creature.action = 38;
    creature.multipliers[1] = .75f;
    suppliedSpeed = 3;
    creature.actionMs = 221;
    check(bool(actionSource(creature, {}, 0, querySpeed)), "fractional thresholds truncate after multiplying the stat by its creature scalar");
    creature.actionMs = 222;
    check(!actionSource(creature, {}, 0, querySpeed), "the fractional part does not extend the integer admission boundary");
    creature.multipliers[1] = 1;
    suppliedSpeed = std::bit_cast<float>(0x37f9fffcu);
    creature.actionMs = 16777219;
    check(bool(actionSource(creature, {}, 0, querySpeed)), "large action clocks are not narrowed to float for threshold comparison");
    suppliedSpeed = -2;
    creature.actionMs = -251;
    check(bool(actionSource(creature, {}, 0, querySpeed)), "negative clocks and thresholds retain their signed relation");
    creature.actionMs = -250;
    check(!actionSource(creature, {}, 0, querySpeed), "negative threshold equality also rejects");
    suppliedSpeed = 0;
    check(!actionSource(creature, {}, 0, querySpeed), "invalid zero-divisor conversion uses the engine omission policy");
    creature.action = 28;
    creature.actionMs = 0;
    const FixedPosition base{(int64_t(1) << 54) + 1, -17, 33};
    const auto light = actionSource(creature, base, 90, querySpeed);
    check(light && light->position == FixedPosition{base[0] - 65536, base[1], base[2]} && light->radius == 20 &&
              colourBits(light->colour, {0x3ea8f5c3, 0x3e6147af, 0x3de147af}),
          "action sites use the separate vector attachment and ordered cosine color modulation");
    creature.health = 0;
    const int previous = speedQueries;
    check(!actionSource(creature, {}, 0, querySpeed) && speedQueries == previous,
          "ordered positive health precedes any action stat query");

    Combatant equipped;
    equipped.characterClass = 2;
    equipped.action = 38;
    constexpr std::array<int, 9> slots{OFF_HAND, MAIN_HAND, CHEST_SLOT, BOOTS_SLOT, GLOVES_SLOT,
                                       SHOULDERS_SLOT, AMULET_SLOT, RING_LEFT, RING_RIGHT};
    constexpr std::array<uint8_t, 9> types{3, 3, 4, 6, 5, 7, 8, 9, 9};
    for (size_t i = 0; i < slots.size(); ++i) {
        equipped.equipment[slots[i]].type = types[i];
        equipped.equipment[slots[i]].seed = uint32_t(i + 1);
    }
    auto original = itemBonuses().speed;
    std::vector<uint32_t> visits;
    itemBonuses().speed = [&](const Item &item) { visits.push_back(item.seed); return 0.f; };
    const auto defaultLight = actionSource(equipped, {}, 0);
    itemBonuses().speed = std::move(original);
    check(defaultLight && visits == std::vector<uint32_t>{1, 2, 3, 4, 5, 6, 7, 8, 9},
          "the production default queries the stat once and visits qualifying equipment in the held order");
}

int main() {
    bodyLights();
    receivedLights();
    yawLights();
    actionLights();
    Combatant creature;
    creature.combo = 100; // Default limit 50: Q = 2 without a clamp.
    creature.equipment[MAIN_HAND] = spirit(3, 128);
    creature.equipment[OFF_HAND] = spirit(3, 129);
    creature.equipment[SHOULDERS_SLOT] = spirit(7, 130);
    creature.equipment[CHEST_SLOT] = spirit(4, 131);
    const FixedPosition main{(int64_t(1) << 54) + 1, -17, 33};
    const std::array<FixedPosition, 2> hands{{{1, 2, 3}, {-5, -6, -7}}};
    const auto lights = equipmentSources(creature, main, hands);
    check(lights[0] && lights[1] && lights[2] && lights[3], "all four independent equipment sites contribute");
    if (lights[0] && lights[1] && lights[2] && lights[3]) {
        check(lights[0]->position == hands[0] && lights[1]->position == hands[1] &&
                  lights[2]->position == main && lights[3]->position == main,
              "main/off attachments precede shoulders/chest at the unrounded main position");
        check(lights[0]->radius == 10 && lights[1]->radius == 10 &&
                  lights[2]->radius == 10 && lights[3]->radius == 10,
              "every item site retains radius ten, including coincident body sources");
        check(colourBits(lights[0]->colour, {0x3f666667, 0x3ee66667, 0x3db851ec}) &&
                  colourBits(lights[1]->colour, {0x3e8a3d71, 0x3f666667, 0x3ee66667}) &&
                  colourBits(lights[2]->colour, {0x3db851ec, 0x3e19999a, 0x3e99999a}) &&
                  colourBits(lights[3]->colour, {0x3e75c290, 0x3e75c290, 0x3e99999a}),
              "hand and body multipliers preserve the uncapped ratio and source order");
    }
    creature.combo = -100;
    const auto negative = equipmentSources(creature, main, hands);
    check(negative[0] && negative[2] && negative[0]->colour.r < 0 && negative[2]->colour.b < 0,
          "finite negative counter ratios remain sources");
    creature.combo = 0;
    check(empty(equipmentSources(creature, main, hands)), "exact zero ratio rejects all four sites");
    creature.combo = 1;
    creature.multipliers[1] = .01f; // Integer limit truncates to zero.
    check(empty(equipmentSources(creature, main, hands)), "undefined zero-divisor ratios use the engine omission policy");
    creature.multipliers[1] = 1;

    creature.equipment[MAIN_HAND].type = 8;
    creature.equipment[OFF_HAND].type = 0;
    creature.equipment[SHOULDERS_SLOT].type = 3;
    creature.equipment[CHEST_SLOT].type = 9;
    const auto categories = equipmentSources(creature, main, hands);
    check(!categories[0] && !categories[1] && categories[2] && categories[3],
          "hands require category three while body sites accept every nonzero category");
    creature.equipment[SHOULDERS_SLOT].spirits = 0;
    creature.equipment[CHEST_SLOT].cubes[0].material = 127;
    check(empty(equipmentSources(creature, main, hands)), "nonqualifying item colors do not become zero replacement lights");

    creature.equipment[MAIN_HAND] = spirit(3, 128);
    creature.combo = 1;
    const auto rounded = equipmentSources(creature, main, hands);
    check(rounded[0] && std::bit_cast<uint32_t>(rounded[0]->colour.r) == 0x3c1374bd,
          "averaged color multiplies by the divided counter before the final point-three factor");
    creature.health = 0;
    check(empty(equipmentSources(creature, main, hands)), "nonpositive health emits no equipment sources");
    creature.health = std::numeric_limits<float>::quiet_NaN();
    check(empty(equipmentSources(creature, main, hands)), "health must pass ordered positive admission");
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
