// User extension checks against synthetic terrain and the actual movement code.
#include "creatures/Physics.h"
#include "ChunkMesher.h"
#include "Frame.h"
#include "WaterLook.h"
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

void revisedFrame() {
    Camera camera;
    Scene scene;
    scene.seconds = 1;
    auto frame = makeFrame(camera, scene, {}, {});
    check(std::abs(frame.water.z - .80f) < 1e-6f && std::abs(frame.water.w + .65f) < 1e-6f,
          "zero origin and one monotonic second supply the two signed phase offsets");
    camera.renderOrigin = {8397056, 8396288};
    scene.seconds = 935000.125;
    frame = makeFrame(camera, scene, {}, {});
    const glm::vec2 local{113.5f, 27.25f};
    const auto phases = [](const FrameConstants &f, glm::vec2 point) {
        return glm::vec2(glm::dot(point, {.65f, .31f}) + f.water.z,
                         glm::dot(point, {-.27f, .72f}) + f.water.w);
    };
    const auto before = phases(frame, local);
    const glm::ivec2 rebase{256, -256};
    camera.renderOrigin += rebase;
    const auto after = phases(makeFrame(camera, scene, {}, {}), local - glm::vec2(rebase));
    for (int axis = 0; axis < 2; ++axis)
        check(std::abs(std::remainder(double(after[axis]) - before[axis], 2 * std::numbers::pi)) < 3e-5,
              "a fixed water point keeps its wave phase across a render-origin change");
    for (double time : {0., 43200000.}) {
        scene.timeOfDayMs = time;
        scene.underwater = true;
        scene.fogDistance = 200;
        frame = makeFrame(camera, scene, {}, {});
        const float bright = .2f + .8f * alpha::sunlight(time);
        check(frame.fog == glm::vec4(glm::vec3(0, .42f, .79f) * bright, 1) &&
              frame.sky1 == glm::vec4(glm::vec3(.02f, .45f, .82f) * bright, 1) &&
              frame.sky2 == glm::vec4(glm::vec3(0, .35f, .72f) * bright, 1) &&
              frame.underwater == frame.fog && frame.params.x == 1.f / 48.f &&
              glm::vec3(frame.material) == glm::vec3(.38f, .82f, 1.f),
              "the revised submerged palette dims with sunlight and limits the Alpha scene to 48 blocks");
        scene.fogDistance = 24;
        check(makeFrame(camera, scene, {}, {}).params.x == 1.f / 24.f,
              "a shorter Alpha view distance survives the submerged visibility ceiling");
        scene.underwater = false;
        scene.fogDistance = 200;
        frame = makeFrame(camera, scene, {}, {});
        check(frame.underwater.w == 0 && frame.params.x == 1.f / 200.f &&
              frame.material == alpha::material(alpha::sunlight(time), false),
              "dry-camera Alpha fog and material remain available to the surface pass");
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
    revisedFrame();
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
        // Rendering §9.1, §9.5 (correction 26): the film starts as (0, 0, 1, 1) and goes through
        // the corner light and occlusion like any face: rg stay 0, blue lies between the
        // shaded 0.901 and 1, alpha is the corner's light, and its corners match the solid top.
        bool filmShaded = meshes.water.vertices.size() == 4;
        for (const auto &vertex : meshes.water.vertices) {
            filmShaded &= vertex.colour.r == 0 && vertex.colour.g == 0 && vertex.colour.b >= 0.9f &&
                          vertex.colour.b <= 1.f && vertex.colour.a > 0 && vertex.colour.a <= 1.f && vertex.normal.z == 1;
            bool matchesTop = false;
            for (const auto &solid : meshes.solid.vertices)
                matchesTop |= solid.normal.z == 1 && solid.position == vertex.position && solid.colour.a == vertex.colour.a;
            filmShaded &= matchesTop;
        }
        check(filmShaded, "the type-3 film quad is shaded through the corner light and occlusion");
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
