// User extension checks against synthetic terrain and the actual movement code.
#include "creatures/Physics.h"
#include "ChunkMesher.h"
#include "worldgen/WaterFeatures.h"
#include <cstdio>
#include <memory>

using namespace worldgen;
namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}

std::shared_ptr<Zone> dryZone(int zx = 0) {
    auto zone = std::make_shared<Zone>();
    zone->zx = zx;
    zone->zy = 0;
    for (Column &column : zone->columns) {
        column.bottom = 1;
        column.blocks.assign(4, Block{100, 100, 100, GRASS});
    }
    return zone;
}
}

int main() {
    {
        auto nativeZone = dryZone();
        nativeZone->column(10, 10).blocks.back().typeByte = LAKE_BED;
        WorldMap nativeMap;
        nativeMap.add(nativeZone);
        World nativeWorld(1);
        Terrain nativeTerrain(nativeWorld);
        const auto meshes = meshChunk(nativeMap, nativeTerrain, 0, 0);
        check(meshes.water.indices.size() == 6, "native wet ground adds one water-stream top face");
        check(!meshes.solid.indices.empty(), "native wet ground retains ordinary solid faces");
        check(nativeZone->read(10, 10, 4).type() == LAKE_BED, "native meshing does not turn wet ground into liquid");
        nativeZone->column(10, 10).blocks.push_back(Block{255, 255, 255, WATER});
        const auto submerged = meshChunk(nativeMap, nativeTerrain, 0, 0);
        bool extraSubmergedTop = false;
        for (const auto &vertex : submerged.water.vertices)
            extraSubmergedTop |= vertex.normal.z > 0 && vertex.position.z == 5;
        check(!extraSubmergedTop, "submerged wet ground does not receive the extra air-facing water quad");
    }
    auto zone = dryZone();
    zone->column(20, 20).blocks.back().typeByte = LAKE_BED | KEEP;
    zone->column(21, 20).blocks.back().typeByte = WATER;
    zone->column(22, 20).blocks.back().typeByte = LAKE_BED;
    zone->column(22, 20).blocks[2].typeByte = ROCK | KEEP;
    zone->column(23, 20).blocks.back().typeByte = LAKE_BED;
    zone->column(23, 20).blocks[2].typeByte = AIR | DRY_AIR;
    for (int y = 30; y <= 32; y++)
        for (int x = 30; x <= 32; x++)
            zone->column(x, y).blocks.back().typeByte = LAKE_BED;
    makeWaterSwimmable(*zone);
    for (int depth = 0; depth < SWIMMING_DEPTH; depth++) {
        check(zone->read(20, 20, 4 - depth).type() == WATER, "walkable wet ground becomes three liquid cells");
        check(zone->read(21, 20, 4 - depth).type() == WATER, "ordinary shallow water gains swimming depth");
    }
    check(zone->read(20, 20, 4).typeByte & KEEP, "wet surface retains its keep bit without remaining solid");
    check(zone->read(20, 20, 1).type() == GRASS, "the lowered natural bed still has a floor");
    check(zone->read(19, 20, 4).type() == GRASS, "adjacent dry ground stays dry");
    check(zone->read(22, 20, 3).typeByte == (ROCK | KEEP), "protected construction remains the bed");
    check(zone->read(23, 20, 3).type() == AIR, "water depth does not fill a cavity");
    const auto first = zone->column(20, 20).blocks;
    makeWaterSwimmable(*zone);
    const auto &second = zone->column(20, 20).blocks;
    bool stable = first.size() == second.size();
    for (size_t i = 0; stable && i < first.size(); i++)
        stable = first[i].typeByte == second[i].typeByte;
    check(stable, "reapplying the extension does not lower the surface again");

    {
        // A column stored only two blocks deep: carving reaches below its lowest stored
        // cell, so the floor must become a stored block that the mesher gives a face.
        auto thin = dryZone();
        for (int y = 40; y <= 42; y++)
            for (int x = 40; x <= 42; x++) {
                Column &column = thin->column(x, y);
                column.bottom = 3;
                column.blocks.assign(2, Block{100, 100, 100, GRASS});
                column.blocks.back().typeByte = LAKE_BED;
            }
        makeWaterSwimmable(*thin, [](int32_t, int32_t, int32_t) { return Block{90, 80, 70, ROCK}; });
        const Column &column = thin->column(41, 41);
        check(column.bottom == 1 && column.blocks.front().type() == ROCK,
              "carving below the stored cells stores the supplied bed block");
        check(thin->read(41, 41, 2).type() == WATER && thin->read(41, 41, 4).type() == WATER,
              "the thin column still gains full swimming depth");
        WorldMap thinMap;
        thinMap.add(thin);
        World thinWorld(1);
        Terrain thinTerrain(thinWorld);
        bool floorFace = false;
        for (const auto &vertex : meshChunk(thinMap, thinTerrain, 32, 32).solid.vertices)
            floorFace |= vertex.normal.z > 0 && vertex.position.z == 2; // only the pit's floor is that low
        check(floorFace, "the stored bed under carved water has a drawn top face");
    }

    // A normal player-sized body can occupy the converted volume and enters the
    // real swimming/contact path rather than landing on the former solid top.
    creatures::Body body;
    body.size = {1.2f, 1.2f, 2.7f};
    body.local = true;
    body.place({31.5, 31.5, 3.5});
    creatures::worldStep(body, 20, [&](int32_t x, int32_t y, int32_t z) {
        return zone->contains(x, y) ? zone->read(x, y, z).type() : (uint8_t)SOLID;
    });
    check(body.contact & creatures::IN_WATER, "converted shallows enter the actual swimming state");
    check(!(body.contact & creatures::ON_GROUND), "converted wet ground does not set standing contact");

    // A bank crossing a zone seam: missing neighbors must not be guessed, and
    // newly admitted water must produce the same reeds regardless of load order.
    auto bank = dryZone();
    auto water = dryZone(1);
    for (int y = 0; y < Zone::SIZE; y++)
        water->column(256, y).blocks.back().typeByte = WATER;
    WorldMap map;
    map.add(bank);
    check(bankReeds(map, *bank, 41).empty(), "no reeds appear against an absent neighbor");
    map.add(water);
    const auto reeds = bankReeds(map, *bank, 41);
    check(!reeds.empty(), "reeds appear on the dry side of the admitted shoreline");
    for (const Plant &reed : reeds) {
        check(reed.x == 255 && reed.z == 5, "reeds stay on the bank, including across a zone seam");
        check(map.read(reed.x, reed.y, reed.z).type() == AIR, "reed feet are outside water");
        check(map.read(reed.x + 1, reed.y, reed.z - 1).type() == WATER, "reed has adjacent surface water");
    }
    WorldMap reversed;
    reversed.add(water);
    reversed.add(bank);
    const auto again = bankReeds(reversed, *bank, 41);
    check(again.size() == reeds.size(), "bank density is independent of zone admission order");
    for (size_t i = 0; i < reeds.size() && i < again.size(); i++)
        check(reeds[i].x == again[i].x && reeds[i].y == again[i].y && reeds[i].rotation == again[i].rotation,
              "bank placement and rotation are deterministic");
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
