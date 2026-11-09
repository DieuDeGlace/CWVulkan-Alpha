#include "CreatureLights.h"
#include "creatures/Combat.h"
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
static bool empty(const std::array<std::optional<Candidate>, 4> &sources) {
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

int main() {
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
