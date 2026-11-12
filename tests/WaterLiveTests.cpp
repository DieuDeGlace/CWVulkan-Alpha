// User water extension checked on generated terrain: the first lake location of
// seed 123's home region, generated with and without the extension, its bank
// reeds, and the actual movement code in both converted and ordinary lake water.
#include "creatures/Physics.h"
#include "worldgen/Terrain.h"
#include "worldgen/WaterFeatures.h"
#include <cmath>
#include <cstdio>
#include <memory>
#include <optional>

using namespace worldgen;
namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}

// The highest exposed liquid or wet-ground surface of a column, if any.
std::optional<int32_t> surface(const Zone &zone, int32_t x, int32_t y, uint8_t type) {
    const Column &column = zone.column(x, y);
    for (int32_t z = column.top() - 1; z >= column.bottom; z--)
        if (zone.read(x, y, z).type() == type && zone.read(x, y, z + 1).type() == AIR)
            return z;
    return std::nullopt;
}

struct Swim {
    int steps = 0, wet = 0, standing = 0;
    double lowest = 1e9, highest = -1e9;
};

// Drops a player-sized body one block into the water and runs the real step.
Swim swim(const WorldMap &map, int32_t x, int32_t y, int32_t top) {
    creatures::Body body;
    body.size = {1.2f, 1.2f, 2.7f};
    body.local = true;
    body.place({x + .5, y + .5, top - 1.0});
    Swim result;
    for (int step = 0; step < 150; step++) {
        creatures::worldStep(body, 20, [&](int32_t bx, int32_t by, int32_t bz) { return map.read(bx, by, bz).type(); });
        result.steps++;
        result.wet += (body.contact & creatures::IN_WATER) != 0;
        result.standing += (body.contact & creatures::ON_GROUND) != 0;
        result.lowest = std::min(result.lowest, body.blocks().z);
        result.highest = std::max(result.highest, body.blocks().z);
    }
    return result;
}

// The surface counts of one natively generated zone, away from its edges.
struct Survey {
    int water = 0, wet = 0, dry = 0;
};
Survey survey(const Zone &zone) {
    Survey s;
    for (int32_t y = zone.originY() + 2; y < zone.originY() + Zone::SIZE - 2; y++)
        for (int32_t x = zone.originX() + 2; x < zone.originX() + Zone::SIZE - 2; x++) {
            const Column &column = zone.column(x, y);
            for (int32_t z = column.top() - 1; z >= column.bottom; z--) {
                const uint8_t type = zone.read(x, y, z).type();
                if (type == AIR)
                    continue;
                s.water += type == WATER;
                s.wet += type == LAKE_BED;
                s.dry += type != WATER && type != LAKE_BED;
                break;
            }
        }
    return s;
}

// Generates the 3 x 3 extended zones round (zx, zy) and checks the centre: native
// surfaces of `kind` gain swimming depth, the bank receives reeds, a body swims.
void site(const World &world, const Terrain &native, const Terrain &extended, int32_t zx, int32_t zy, uint8_t kind,
          const char *label, bool needReeds) {
    WorldMap map;
    std::shared_ptr<const Zone> centre;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            std::shared_ptr<const Zone> zone = extended.generate(zx + dx, zy + dy);
            if (!dx && !dy)
                centre = zone;
            map.add(zone);
        }
    const std::unique_ptr<Zone> plain = native.generate(zx, zy);
    int deepened = 0, kept = 0;
    std::optional<glm::ivec3> place;
    for (int32_t y = centre->originY() + 2; y < centre->originY() + Zone::SIZE - 2; y++)
        for (int32_t x = centre->originX() + 2; x < centre->originX() + Zone::SIZE - 2; x++) {
            const auto top = surface(*plain, x, y, kind);
            if (!top || (kind == WATER && surface(*plain, x, y, LAKE_BED)))
                continue;
            bool liquid = true;
            for (int depth = 0; depth < SWIMMING_DEPTH; depth++)
                liquid &= centre->read(x, y, *top - depth).type() == WATER;
            deepened += liquid;
            kept += !liquid;
            // A swimming spot with liquid on all four sides, so the body is not on a rim.
            bool open = liquid;
            for (const auto side : {glm::ivec2(-1, 0), glm::ivec2(1, 0), glm::ivec2(0, -1), glm::ivec2(0, 1)})
                open &= map.read(x + side.x, y + side.y, *top - 1).type() == WATER;
            if (open && !place)
                place = glm::ivec3(x, y, *top);
        }
    std::printf("%s zone (%d, %d): native surfaces with swimming depth %d, kept %d\n", label, zx, zy, deepened, kept);
    check(deepened > 0, "native surfaces of the site gain swimming depth");
    // Rendering: water resting on an implicit cell has no drawn floor.
    int floorless = 0, nativeFloorless = 0;
    for (int32_t y = centre->originY(); y < centre->originY() + Zone::SIZE; y++)
        for (int32_t x = centre->originX(); x < centre->originX() + Zone::SIZE; x++) {
            const Column &column = centre->column(x, y), &before = plain->column(x, y);
            floorless += !column.blocks.empty() && column.blocks.front().type() == WATER && column.bottom > 1;
            nativeFloorless += !before.blocks.empty() && before.blocks.front().type() == WATER && before.bottom > 1;
        }
    std::printf("  water columns over an implicit floor above sea level: %d (native %d)\n", floorless, nativeFloorless);
    check(floorless == 0, "the extension leaves no water over an undrawn floor");

    const auto reeds = bankReeds(map, *centre, world.seed());
    int dryFeet = 0, besideWater = 0;
    for (const Plant &reed : reeds) {
        dryFeet += map.read(reed.x, reed.y, reed.z).type() == AIR;
        bool beside = false;
        for (const auto side : {glm::ivec2(-1, 0), glm::ivec2(1, 0), glm::ivec2(0, -1), glm::ivec2(0, 1)})
            for (int level = reed.z - 2; level <= reed.z; level++)
                beside |= map.read(reed.x + side.x, reed.y + side.y, level).type() == WATER &&
                          map.read(reed.x + side.x, reed.y + side.y, level + 1).type() == AIR;
        besideWater += beside;
    }
    std::printf("  bank reeds %zu", reeds.size());
    if (!reeds.empty())
        std::printf(", first at (%d, %d, %d)", reeds.front().x, reeds.front().y, reeds.front().z);
    std::printf("\n");
    if (needReeds)
        check(!reeds.empty(), "the generated shore receives bank reeds");
    check(dryFeet == int(reeds.size()), "every reed stands in air");
    check(besideWater == int(reeds.size()), "every reed is next to a water surface");

    check(place.has_value(), "the site has an open swimming spot");
    if (place) {
        const Swim result = swim(map, place->x, place->y, place->z);
        std::printf("  swim at (%d, %d, %d): in water %d/%d steps, standing %d, height %.2f..%.2f\n", place->x,
                    place->y, place->z, result.wet, result.steps, result.standing, result.lowest, result.highest);
        check(result.wet == result.steps, "a body in generated water stays in the swimming state");
        check(result.standing == 0, "a body in generated water does not stand on the bed");
    }
}
}

int main() {
    constexpr uint32_t seed = 123;
    World world(seed);
    Terrain native(world), extended(world);
    extended.swimmableWater = true;
    const int32_t rx = floorDiv(HOME, REGION), ry = floorDiv(HOME, REGION);
    extended.prepareRegion(rx, ry);
    const Region *region = world.region(rx, ry);
    const Location *lake = nullptr;
    for (const Location &record : region ? region->records : decltype(region->records){})
        if (record.type == 4 && !lake)
            lake = &record;
    check(lake != nullptr, "the home region holds a lake location");

    // Lake shore: east from the lake centre, the first zone holding both water
    // and dry ground in the native generator.
    if (lake) {
        const int32_t cx = int32_t(lake->x >> 16), cy = int32_t(lake->y >> 16);
        std::printf("lake centre (%d, %d), radius %.1f\n", cx, cy, lake->radius);
        std::optional<int32_t> shore;
        for (int32_t k = 0; k < 4 && !shore; k++) {
            const Survey s = survey(*native.generate(floorDiv(cx, Zone::SIZE) + k, floorDiv(cy, Zone::SIZE)));
            if (s.water > 2000 && s.dry > 2000)
                shore = floorDiv(cx, Zone::SIZE) + k;
        }
        check(shore.has_value(), "the lake has a shore zone");
        if (shore)
            site(world, native, extended, *shore, floorDiv(cy, Zone::SIZE), WATER, "lake shore", true);
    }

    // Walkable wet ground (native stream beds): the nearest home zone holding it.
    const int32_t hx = floorDiv(HOME, Zone::SIZE), hy = floorDiv(HOME, Zone::SIZE);
    std::optional<glm::ivec2> stream;
    for (int32_t ring = 0; ring <= 3 && !stream; ring++)
        for (int32_t dy = -ring; dy <= ring && !stream; dy++)
            for (int32_t dx = -ring; dx <= ring && !stream; dx++)
                if (std::max(std::abs(dx), std::abs(dy)) == ring && survey(*native.generate(hx + dx, hy + dy)).wet >= 20)
                    stream = glm::ivec2(hx + dx, hy + dy);
    check(stream.has_value(), "a home-area zone holds native walkable wet ground");
    if (stream)
        site(world, native, extended, stream->x, stream->y, LAKE_BED, "wet ground", false);
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}