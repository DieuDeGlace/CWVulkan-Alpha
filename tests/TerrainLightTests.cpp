#include "ChunkMesher.h"
#include "FileHash.h"
#include "CachedLights.h"
#include "TerrainLights.h"
#include "creatures/Spirits.h"
#include <bit>
#include <cstdio>
#include <limits>
#include <memory>

using namespace terrain_lights;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { ++failures; std::printf("FAIL %s\n", message); }
}
static FixedPosition point(int64_t x, int64_t y, int64_t z = 0) {
    return {x * 65536, y * 65536, z * 65536};
}
static Candidate light(int64_t x, int64_t y, float radius, glm::vec3 colour = {1, 1, 1}) {
    Candidate result;
    result.position = point(x, y);
    result.colour = colour;
    result.radius = radius;
    return result;
}

static void positionedDecorationLights() {
    using namespace worldgen;
    constexpr int64_t unit = 65536;
    const auto zone = std::make_shared<Zone>();
    auto &column = zone->column(0, 0);
    column.bottom = -3;
    column.blocks = {{7, 230, 240, AIR}, {1, 230, 240, uint8_t(AIR | DRY_AIR)}, {14, 230, 240, WATER},
                     {17, 230, 240, GLOWING}, {12, 230, 240, ROCK}, {71, 230, 240, AIR}};
    const auto sprite = [](int32_t kind, int64_t x, int64_t z, int32_t flags = 2) {
        return Sprite{kind, x, 0, z, .125f, 17.f, flags, {.2f, .4f, .8f}};
    };
    zone->addSprite(sprite(14, -1, -2 * unit - 1, 1));
    zone->addSongPost({0, 0, -3 * unit});
    zone->addLeafSource({0, 0, -unit, 1.f, .5f, .25f});
    zone->addPlant({0, 0, 3, .09f, 91.f, 31, 2, true});
    zone->addSprite(sprite(1, -unit, 0));
    zone->addSprite(sprite(2, 0, 1));
    zone->addSprite(sprite(3, 0, unit));
    zone->addSprite(sprite(4, 0, 100 * unit));
    zone->addSprite(sprite(5, 0, -4 * unit));
    zone->addSprite(sprite(6, 32 * unit, 0));

    WorldMap map;
    map.add(zone);
    map.refreshDecorations({0, 0});
    const WorldMap previous = map.neighbourhood(0, 0);
    auto result = map.sampleDecorations(0, 0);
    check(result.sampled && result.sampled->size() == 9 && map.decorations(1, 0)->size() == 1,
          "chunk sampling merges tree, plant and structure occurrences and splits exact chunk edges");
    if (!result.sampled || result.sampled->size() != 9)
        return;
    constexpr int32_t kinds[] = {14, 61, 60, 31, 1, 2, 3, 4, 5};
    constexpr float values[] = {5, 255, 14, 71, 0, 255, 0, 255, 0};
    bool ordered = true, sampled = true;
    for (size_t i = 0; i < result.sampled->size(); ++i) {
        ordered &= (*result.sampled)[i].sprite.kind == kinds[i];
        sampled &= (*result.sampled)[i].light == values[i];
    }
    check(ordered, "native lists preserve encounter order across typed generators");
    check(sampled, "signed truncation, invalid X, dry flag, water fallback, committed byte, glow and solid bounds");
    const auto &flame = result.sampled->front().sprite;
    const auto &lily = (*result.sampled)[3].sprite;
    check(flame.flags == 1 && flame.tint[1] == .4f && flame.angle == 17.f && flame.size == .125f &&
          lily.x == unit / 2 && lily.y == unit / 2 && lily.z == 3 * unit - 6553 && lily.tint[0] == 1,
          "copies preserve original payload, light color and the lily's fixed elevation");
    auto emitters = *result.sampled;
    emitters[1].sprite.flags = 1;
    emitters[1].sprite.kind = 13;
    emitters[1].light = 0;
    std::vector<Candidate> sources;
    appendDecorationLights(sources, emitters, ViewAdmission({}, {}, {}, 100), 0, 1.f);
    check(sources.size() == 2 && sources[0].colour == glm::vec3(.2f, .4f, .8f) &&
              sources[1].colour == glm::vec3(.5f) && sources[0].radius != sources[1].radius,
          "only flagged active records emit in encounter order; midday halves the lantern despite its zero sample");
    check(map.publishDecorations(0, 0, result) && map.decorations(0, 0)->front().light == 5 &&
              previous.decorations(0, 0)->front().light == 0 && previous.zone(0, 0) == map.zone(0, 0),
          "resident field changes while a retained area keeps its old values and shares all terrain columns");
    ChunkRenderState slot;
    slot.publish({1, 2}, result.sampled);
    slot.renderVisit(true);
    check(!slot.activeDecorations, "a dirty key holds the pending decoration with its groups");
    slot.renderVisit(false);
    check(slot.activeDecorations == result.sampled && slot.pendingDecorations == result.sampled,
          "first accepted groups copy pending decorations to the active slot");
    auto change = std::make_shared<Zone>(*zone);
    change->sprites.erase(change->sprites.begin());
    map.replaceTerrain(change);
    map.refreshDecorations({0, 0});
    const auto edited = map.sampleDecorations(0, 0);
    check(!map.publishDecorations(0, 0, result) && map.decorations(0, 0)->front().light == 255,
          "a support edit preserves surviving sampled values and rejects an old job");
    slot.publish({}, edited.sampled);
    slot.renderVisit(false);
    check(slot.activeDecorations == result.sampled && slot.pendingDecorations == edited.sampled,
          "empty geometry preserves active copies independently of fresh pending and resident data");
    slot.publish({3, 0}, edited.sampled);
    slot.renderVisit(false);
    check(slot.activeDecorations->front().sprite.kind == 61 && slot.activeDecorations->size() == 8,
          "successful rebuilt geometry transfers the pruned list");
    const uint64_t before = map.recordBase({0, 0});
    map.remove(0, 0);
    map.add(zone);
    map.refreshDecorations({0, 0});
    check(map.recordBase({0, 0}) != before && !map.publishDecorations(0, 0, edited) &&
              map.decorations(0, 0)->front().light == 0,
          "departure and a new generation reject pending samples and begin with new resident fields");
    // Existing source modules may mutate the same fields directly, including inserting
    // an unranked record in the middle of a typed collection.
    auto altered = std::make_shared<Zone>(*zone);
    altered->sprites.insert(altered->sprites.begin(), sprite(29, 0, 0));
    std::swap(altered->sprites[1], altered->sprites[2]);
    map.replaceTerrain(altered);
    map.refreshDecorations({0, 0});
    std::vector<int32_t> typed;
    for (const auto &entry : *map.decorations(0, 0))
        if (entry.sprite.kind == 29 || entry.sprite.kind == 1 || entry.sprite.kind == 14)
            typed.push_back(entry.sprite.kind);
    check(typed == std::vector<int32_t>({29, 1, 14}), "SDK typed vector edits retain caller order and coordinates");
    const glm::vec4 wet(.4f, .5f, 1, .25f);
    check(decorationMaterial(wet, 71, false) == glm::vec4(.4f, .5f, 1, (71.f / 255.f) * .25f) &&
          decorationMaterial({1, 1, 1, 0}, 0, true) == glm::vec4(1) &&
          decorationMaterial({.4f, .5f, 1, 0}, 255, false) == glm::vec4(.4f, .5f, 1, 0),
          "unflagged illumination uses sample then daytime; a source stays full at midnight under its camera tint");
}

static void packedTerrainLight() {
    auto zone = std::make_shared<worldgen::Zone>(0, 0);
    worldgen::WorldMap map;
    map.add(zone);
    const worldgen::World world(1);
    const worldgen::Terrain terrain(world);
    constexpr int32_t x = 42, y = 42, z = 5;
    for (int32_t i = x - 3; i <= x + 2; i++)
        for (int32_t j = y - 3; j <= y + 2; j++) {
            auto &column = zone->column(i, j);
            column.bottom = z;
            column.blocks = {{255, 255, 255, worldgen::AIR}};
        }
    auto &support = zone->column(x, y);
    support.bottom = z - 1;
    support.blocks.insert(support.blocks.begin(), {128, 64, 32, worldgen::GRASS});
    const auto topColour = [&](bool lightingRequested = false) {
        const auto mesh = meshChunk(map, terrain, 32, 32, lightingRequested);
        check(mesh.preparedLighting == lightingRequested, "only the dirty interior mesh prepares lighting");
        for (size_t i = 0; i < mesh.solid.vertices.size(); i += 4) {
            const auto &vertex = mesh.solid.vertices[i]; // lower X/Y corner of this top quad
            if (vertex.position == glm::vec3(x - 32, y - 32, z) && vertex.normal == glm::vec3(0, 0, 1))
                return vertex.colour;
        }
        return glm::vec4(-1);
    };
    const auto uniform = [&](uint8_t intensity) {
        for (int32_t i = x - 3; i <= x + 2; i++)
            for (int32_t j = y - 3; j <= y + 2; j++)
                zone->column(i, j).blocks.back() = {intensity, intensity, intensity, worldgen::AIR};
    };
    check(topColour() == glm::vec4(128.f / 255.f, 64.f / 255.f, 32.f / 255.f, 254.f / 255.f),
          "an open top face uses the original normalized color bytes, including full-light alpha 254");
    uniform(5);
    check(topColour().a == 4.f / 255.f, "a uniform intensity-five layer packs alpha four");
    check(topColour(true).a == 254.f / 255.f, "a dirty interior mesh uses its newly prepared sky light");
    uniform(128);
    check(topColour().a == 128.f / 255.f, "a uniform intensity-128 layer packs alpha 128");
    uniform(255);
    zone->column(x - 1, y - 1).blocks[0].typeByte = worldgen::ROCK;
    check(topColour().a == 238.f / 255.f, "a nearest blocker keeps its weight in the native alpha denominator");
    zone->column(x - 1, y - 1).blocks[0].typeByte = worldgen::AIR;
    zone->column(x - 3, y - 3).blocks[0].typeByte = worldgen::ROCK;
    check(topColour().a == 251.f / 255.f, "a far blocker uses its smaller native contribution");
}

static void meshLightRequests() {
    ChunkLightUpdates updates;
    worldgen::WorldMap map;
    for (const int32_t x : {0, 5, 65530, 65531})
        map.add(std::make_shared<worldgen::Zone>(x, 0));
    const auto change = [&](glm::ivec2 at) {
        const auto admitted = updates.beginBuild(map, at);
        check(admitted.has_value(), "the light request fixture has all required source zones");
        return admitted.value_or(0);
    };
    const glm::ivec2 chunk(2, 2), neighbour(2, 3), unpublished(3, 3), alias(42, 2);
    updates.publish(chunk, 0, false);
    updates.publish(neighbour, 0, false);
    const std::array<glm::ivec2, 2> edits{{{96, 96}, {96, 96}}};
    check(updates.markChanged(edits), "interior marking reaches the window publication");
    const auto first = change(chunk);
    check(first != 0 && change(neighbour) == first && change(unpublished) == 0,
          "a frame coalesces edits on its matching published neighbours only");
    const std::array<glm::ivec2, 1> aliasEdit{{alias * CHUNK}};
    updates.markChanged(aliasEdit);
    check(change(chunk) == first, "a different coordinate in the same slot does not mark it");

    updates.markChanged(edits);
    const auto second = change(chunk);
    updates.publish(chunk, first, true);
    check(second != first && change(chunk) == second, "a newer mark survives older worker publication");
    updates.publish(chunk, second, false);
    check(change(chunk) == second, "an unprepared publication preserves a pending light request");
    updates.invalidate(chunk);
    check(change(alias) == second, "slot reuse carries a pending lighting change");
    updates.publish(alias, second, true);
    check(change(alias) == 0, "prepared publication clears the matching change");

    const glm::ivec2 edge(0x7ffff, 4);
    updates.publish(edge, 0, false);
    updates.publish(chunk, 0, false);
    const std::array<glm::ivec2, 2> edgeThenInterior{{edge * CHUNK, chunk * CHUNK}};
    check(!updates.markChanged(edgeThenInterior), "the out-of-range traversal skips the window publication");
    check(change(edge - glm::ivec2(40, 0)) != 0 && change(chunk) == 0,
          "world bounds retain marks before cleanup and skip later records");

    check(meshLightPadding(32, 32, false) == 0 && meshLightPadding(32, 32, true) == 4 &&
              meshLightPadding(224, 32, false) == 16 && meshLightPadding(32, 256, false) == 16 &&
              meshLightPadding(224, 224, true) == 4,
          "dirty padding takes priority over both zone seam axes");
}

static void meshDispatch() {
    worldgen::WorldMap map;
    auto origin = std::make_shared<worldgen::Zone>(0, 0);
    auto &column = origin->column(230, 230);
    column.bottom = 4;
    column.blocks.push_back({64, 80, 100, worldgen::GRASS});
    map.add(origin);
    const worldgen::World world(1);
    const worldgen::Terrain terrain(world);
    ChunkLightUpdates updates;
    check(!updates.beginBuild(map, {-1, 0}) && !updates.beginBuild(map, {0, 524288}),
          "out-of-world candidates cannot enter the builder");
    check(meshChunk(map, terrain, 0, 0).preparedLighting, "the world origin admits both outward samples in region zero");

    const glm::ivec2 corner(7, 7), edge(7, 3);
    const auto arrival = [&](glm::ivec2 area, glm::ivec2 watched) {
        map.add(std::make_shared<worldgen::Zone>(area.x, area.y));
        std::optional<uint64_t> admitted;
        visitZoneMeshChunks(area, [&](glm::ivec2 candidate) {
            if (candidate == watched)
                admitted = updates.beginBuild(map, candidate);
        });
        return admitted;
    };
    check(!updates.beginBuild(map, corner) && !updates.beginBuild(map, edge) && updates.beginBuild(map, {3, 3}) == 0,
          "missing edge sources leave a later clean interior candidate available for dispatch");
    check(arrival({1, 0}, edge) == 0, "the side arrival reconsiders and admits a clean edge");
    check(!arrival({0, 1}, corner), "two side regions still leave a clean corner waiting");
    for (const bool dirty : {false, true}) {
        const auto denied = meshChunk(map, terrain, 224, 224, dirty);
        check(denied.solid.indices.empty() && denied.water.indices.empty() && !denied.preparedLighting,
              "a missing diagonal prevents clean and dirty geometry as well as lighting");
    }
    check(arrival({1, 1}, corner) == 0, "the diagonal arrival reconsiders and admits the clean corner");
    const auto built = meshChunk(map, terrain, 224, 224);
    check(!built.solid.indices.empty() && built.preparedLighting, "the admitted corner produces real geometry and lighting");

    updates.publish(corner, 0, true);
    updates.markChanged(std::array{corner * CHUNK});
    const auto change = updates.beginBuild(map, corner);
    check(change && *change != 0, "the published corner acquired the edit");
    for (const glm::ivec2 missing : {glm::ivec2(1, 1), glm::ivec2(0, 1), glm::ivec2(1, 0)}) {
        const auto snapshot = map;
        map.remove(missing.x, missing.y);
        check(!updates.beginBuild(map, corner) && chunkMeshSourcesResident(snapshot, corner) &&
                  !chunkMeshSourcesResident(map, corner),
              "an evicted required source disqualifies the still-readable geometry snapshot");
        check(arrival(missing, corner) == change, "late side or diagonal arrival retries the original dirty request");
    }

    map.add(std::make_shared<worldgen::Zone>(65535, 0));
    map.add(std::make_shared<worldgen::Zone>(65536, 0));
    check(updates.beginBuild(map, {524286, 2}) == 0 && !updates.beginBuild(map, {524287, 2}),
          "the last interior index admits while the last edge needs the invalid region 65536");
}

static void meshDistanceOrder() {
    const std::array<std::pair<int64_t, int32_t>, 9> origins{{
        {0, -20}, {1, -20}, {32, -19}, {65536, -19}, {41877504, 0}, {41943040, 0},
        {44040192, 1}, {2097151999, 980}, {1099511562240, 524267}
    }};
    for (const auto &[raw, expected] : origins) {
        const auto actual = chunkMeshWindowOrigin({raw, 32, INT64_MIN}, 40);
        check(actual == glm::ivec2(expected, -19), "adjusted raw eye follows the held origin conversion on each axis");
    }
    check(chunkMeshWindowOrigin({0, 0, 0}, 5) == glm::ivec2(-2), "odd widths use the signed integer half");
    const auto window = [](ChunkMeshPriority &priority, glm::ivec2 origin, int32_t width) {
        std::vector<glm::ivec2> offsets;
        for (const auto &candidate : priority.window(origin))
            offsets.push_back(candidate.offset - glm::ivec2(width / 2));
        return offsets;
    };
    const auto hash = [](const auto &offsets) {
        std::string pairs;
        for (const auto offset : offsets)
            pairs += std::to_string(offset.x) + "," + std::to_string(offset.y) + "\n";
        return bytesSha256({reinterpret_cast<const uint8_t *>(pairs.data()), pairs.size()});
    };
    ChunkMeshPriority priority(40);
    const auto full = window(priority, {100, 100}, 40);
    check(full.size() == 1600 && hash(full) == "707a7e8569fdd443bb78e180e4c7311d2b9640ed7d183703a7cd078603f42b62",
          "all 1600 offsets match the held full-window visit sequence");
    check(window(priority, {0, 0}, 40) == full, "translation to the first unclipped origin preserves all ranks");
    const auto left = window(priority, {-20, 100}, 40);
    check(left.size() == 800 && hash(left) == "b866783955323e71fdb83a19e0311a3ca20e4e98d4a016de0d786f3bfecf8501",
          "all 800 offsets match the held lower-X visit sequence");
    const auto bottom = window(priority, {100, -20}, 40);
    const std::array<glm::ivec2, 4> lowerY{{{0, 0}, {0, 1}, {-1, 0}, {1, 0}}};
    check(bottom.size() == 800 && std::equal(lowerY.begin(), lowerY.end(), bottom.begin()),
          "lower-Y clipping has its own central tie order");
    ChunkMeshPriority shortWindow(5);
    const auto small = window(shortWindow, {100, 100}, 5);
    const std::array<glm::ivec2, 5> stable{{{0, 0}, {-1, 0}, {0, -1}, {0, 1}, {1, 0}}};
    check(small.size() == 25 && std::equal(stable.begin(), stable.end(), small.begin()),
          "the full width-five window keeps enumeration order within the distance-one tie");

    const glm::ivec2 origin(100, 100);
    const std::array<glm::ivec2, 5> available{{{121,120}, {120,119}, {119,120}, {120,121}, {120,120}}};
    std::vector<ChunkMeshCache::Candidate> sparse;
    for (const auto &candidate : priority.window(origin)) {
        const auto at = origin + candidate.offset;
        sparse.push_back({at, false, std::find(available.begin(), available.end(), at) != available.end()});
    }
    std::vector<glm::ivec2> built;
    ChunkMeshCache cache;
    while (cache.pass(sparse, 100, [&](glm::ivec2 at) { built.push_back(at); return std::optional<int32_t>(4); },
                      [](glm::ivec2) {})) {}
    check(built == std::vector<glm::ivec2>{{120,120}, {120,121}, {119,120}, {120,119}, {121,120}},
          "sparse source availability inherits the complete window's tied visit order over successive passes");
}

static void meshVertexBudget() {
    using namespace worldgen;
    auto zone = std::make_shared<Zone>(0, 0);
    WorldMap map;
    map.add(zone);
    const World world(1);
    const Terrain terrain(world);
    // A constant floor plus a dry empty store isolates the new elevated cells' contribution.
    for (int x = 28; x < 69; ++x)
        for (int y = 28; y < 69; ++y) {
            auto &column = zone->column(x, y);
            column.bottom = 1;
            column.blocks.assign(8, {255, 255, 255, AIR});
        }
    const int32_t floor = meshChunk(map, terrain, 32, 32).vertexRecords();
    check(floor == CHUNK * CHUNK * 4, "the implicit floor emits one exposed top per dry empty column");
    const auto counts = [&](uint8_t a, uint8_t b = AIR, bool above = false) {
        zone->column(42, 42).blocks[3].typeByte = a;
        auto &second = zone->column(above ? 42 : 43, 42).blocks[above ? 4 : 3];
        second.typeByte = b;
        const auto mesh = meshChunk(map, terrain, 32, 32);
        second.typeByte = AIR;
        return mesh.vertexRecords() - floor;
    };
    check(counts(SOLID) == 24 && counts(SOLID, SOLID) == 40 && counts(WATER) == 24 && counts(WATER, WATER) == 40,
          "adjacent ordinary and liquid faces contribute four new primary records per exposed quad");
    check(counts(SOLID, WATER) == 44 && counts(LAKE_BED) == 28 && counts(LAKE_BED, WATER, true) == 44,
          "the ordinary/liquid interface and the additional type-three film have the held record counts");

    const std::array<glm::vec3, 6> normals{{{-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1}}};
    for (const uint8_t material : {SOLID, WATER}) {
        zone->column(42, 42).blocks[3].typeByte = material;
        const auto mesh = meshChunk(map, terrain, 32, 32);
        const auto &stream = material == WATER ? mesh.water : mesh.solid;
        size_t direction = 0;
        for (size_t i = 0; i < stream.vertices.size(); i += 4) {
            const auto &a = stream.vertices[i];
            if (a.position.z < 4) // skip the fixture's floor, below the isolated cell
                continue;
            const glm::vec3 winding = glm::cross(stream.vertices[i + 1].position - a.position,
                                                 stream.vertices[i + 2].position - a.position);
            check(direction < normals.size() && a.normal == normals[direction] && winding == a.normal,
                  "a cell emits negative then positive faces along X, Y, Z with outward winding");
            const size_t base = i / 4 * 6;
            const std::array<uint32_t, 6> quad{0, 1, 2, 0, 2, 3};
            for (size_t j = 0; j < quad.size(); ++j)
                check(stream.indices[base + j] == i + quad[j], "direction order preserves the two indexed triangles");
            ++direction;
        }
        check(direction == normals.size(), "both ordinary and liquid isolated cells emit the six directed quads");
    }

    ChunkMeshCache cache;
    check(cache.allowance(100) == 25165824 && cache.allowance(50) == 12582912 &&
              cache.allowance(1) == 251658 && cache.allowance(-1) == -251658,
          "the signed percentage widens, scales and truncates to the native vertex-record allowance");
    using Candidate = ChunkMeshCache::Candidate;
    std::array<Candidate, 5> order{{{{2, 2}, true, false}, {{3, 2}, false, true}, {{4, 2}, false, true},
                                    {{5, 2}, false, true}, {{6, 2}, false, true}}};
    cache.publish({2, 2}, 251658); // A denied dirty slot continues to contribute its published count.
    cache.publish({44, 2}, 333);  // Alias of (4, 2).
    cache.publish({5, 2}, 12);
    std::vector<glm::ivec2> built, reset;
    const auto build = [&](glm::ivec2 at) -> std::optional<int32_t> { built.push_back(at); return 4; };
    const auto evict = [&](glm::ivec2 at) { reset.push_back(at); };
    check(cache.pass(order, 1, build, evict) && built == std::vector<glm::ivec2>{{3, 2}} &&
              reset == std::vector<glm::ivec2>{{44, 2}, {5, 2}},
          "equality admits the new four-record mesh; the next two physical slots reset in visit order");
    built.clear();
    reset.clear();
    check(!cache.pass(order, 1, build, evict) && built.empty() && reset.empty(),
          "the overshooting chunk remains and later empty slots consume no opportunities");
    order[0].hasSources = true;
    check(cache.pass(order, 1, build, evict) && built == std::vector<glm::ivec2>{{2, 2}},
          "the first dirty slot replaces its old count once its source returns");
    built.clear();
    check(cache.pass(order, 1, build, evict) && built == std::vector<glm::ivec2>{{4, 2}},
          "the smaller replacement admits the first previously budget-evicted chunk on the next pass");
    reset.clear();
    check(!cache.pass(order, -1, build, evict) && reset == std::vector<glm::ivec2>{{2, 2}, {3, 2}, {4, 2}},
          "a negative allowance evicts every populated candidate before testing its source");
    cache.publish({2, 2}, 24);
    order[0].requested = true;
    reset.clear();
    check(cache.pass(order, 0, [](glm::ivec2) { return std::optional<int32_t>(0); }, evict) &&
              cache.owns({2, 2}) && reset.empty(),
          "an empty rebuild consumes the pass, keeps its coordinate and replaces its charge with zero");
    built.clear();
    check(cache.pass(order, 0, build, evict) && built == std::vector<glm::ivec2>{{3, 2}},
          "the zero charge of the previous empty rebuild admits the next chunk at equality");
}

static void fogMeshTransfers() {
    std::vector<glm::ivec2> visited;
    const auto record = [&](glm::ivec2 at) { visited.push_back(at); };
    visitChunkRenderWindow({-1, 10}, record);
    check(visited.size() == 1560 && visited.front() == glm::ivec2(0, 10) && visited[39] == glm::ivec2(0, 49) &&
              visited[40] == glm::ivec2(1, 10) && visited.back() == glm::ivec2(38, 49),
          "the lower-edge transfer window omits physical column 39 and visits Y within X");
    visited.clear();
    visitChunkRenderWindow({0x7ffff, 0x7fffe}, record);
    check(visited == std::vector<glm::ivec2>{{0x7ffff, 0x7fffe}, {0x7ffff, 0x7ffff}},
          "the transfer window clips both upper world edges");

    ChunkRenderState state;
    state.publish({});
    check(!state.renderVisit(false).populated() && !state.active.populated(), "the first empty publication leaves a fog hole");
    check(!state.publish({0, 11}).populated() && !state.active.populated() && state.pending.water == 11,
          "a water-only publication awaits the terrain renderer");
    check(!state.renderVisit(true).populated() && state.pending.water == 11, "a blocked visit retains pending water");
    check(!state.renderVisit(false).populated() && state.active.water == 11 && !state.pending.populated(),
          "an admitted renderer visit activates water-only terrain");
    state.publish({12, 0});
    check(state.publish({}).solid == 12 && !state.renderVisit(false).populated() && state.active.water == 11,
          "an empty worker result discards pending terrain and retains the old active buffers");
    state.publish({13, 14});
    check(state.renderVisit(false).water == 11 && state.active.solid == 13 && state.active.water == 14,
          "the next admitted nonempty replacement retires the active geometry");

    ChunkLightUpdates updates;
    worldgen::WorldMap map;
    map.add(std::make_shared<worldgen::Zone>(0, 0));
    const glm::ivec2 a{2, 2}, b{6, 6};
    updates.publish(a, 0, false);
    updates.publish(b, 0, false);
    updates.markChanged(std::array{a * CHUNK}, 700);
    updates.markChanged(std::array{b * CHUNK}, 700); // distinct edits sampled in the same millisecond
    auto finish = [&](glm::ivec2 chunk) {
        const auto change = updates.beginBuild(map, chunk);
        check(change.has_value(), "fixture chunk has all its source areas");
        updates.publish(chunk, change.value_or(0), true);
    };
    finish(a);
    check(!updates.dirty(a) && updates.transferBlocked(a) && updates.dirty(b), "a sibling dirty slot holds the same clock key");
    updates.markChanged(std::array{b * CHUNK}, 701);
    check(!updates.transferBlocked(a) && updates.transferBlocked(b), "remarking a slot moves its dirty count to the new key");
    updates.markChanged(std::array{a * CHUNK}, 701);
    finish(a);
    updates.invalidate(b);
    check(updates.transferBlocked(a), "coordinate reset leaves the physical slot's dirty byte and key in the count");
    finish(b);
    check(!updates.transferBlocked(a), "clearing the last dirty sibling releases its whole batch");
    updates.markChanged(std::array{a * CHUNK}, 0);
    check(updates.dirty(a) && !updates.transferBlocked(a), "zero timestamp marks lighting dirty but never blocks transfer");

    const glm::ivec2 omitted{39, 12}, alias{42, 42};
    updates.publish(omitted, 0, false);
    updates.markChanged(std::array{a * CHUNK, omitted * CHUNK}, 702);
    finish(a);
    check(updates.transferBlocked(alias), "an omitted dirty column blocks the visited physical slot independently of its owner");
    updates.markChanged(std::array{omitted * CHUNK}, 0);
    check(!updates.transferBlocked(alias), "clearing the omitted key unblocks the same physical slot");
}

static void fogMeshElevations() {
    using namespace worldgen;
    WorldMap map;
    auto zone = std::make_shared<Zone>(0, 0);
    map.add(zone);
    for (int x = 28; x < 69; ++x)
        for (int y = 28; y < 69; ++y)
            zone->column(x, y).bottom = 18; // exposed top of implicit solid at elevation 17
    auto &buried = zone->column(43, 43);
    buried.bottom = -10;
    buried.blocks.assign(28, {130, 130, 130, SOLID});
    for (const int z : {266, 267}) {
        auto &column = zone->column(z == 266 ? 42 : 45, 42);
        column.blocks.assign(z - column.bottom + 1, {255, 255, 255, AIR});
        column.blocks.back().typeByte = z == 266 ? SOLID : WATER;
    }
    const World world(41);
    const Terrain terrain(world);
    const auto mesh = meshChunk(map, terrain, 32, 32);
    check(mesh.solid.vertices.size() == CHUNK * CHUNK * 4 + 24 && mesh.water.vertices.size() == 24,
          "the buried base emits nothing extra while ordinary and liquid cells across the 250-elevation boundary reach the Vulkan streams");
}

int main() {
    fogMeshTransfers();
    fogMeshElevations();
    positionedDecorationLights();
    packedTerrainLight();
    meshLightRequests();
    meshDispatch();
    meshDistanceOrder();
    meshVertexBudget();
    FadingSources timed;
    const FixedPosition precise{(int64_t(1) << 54) + 1, 17, -33};
    timed.append({precise, {1, .5f, .25f}, 20, 0, 400});
    timed.append({point(16, 16), {0, 0, 1}, 16, 0, 800});
    timed.advance(400);
    check(timed.entries().size() == 2 && timed.entries()[0].ageMs == 400 &&
              timed.entries()[0].fadedColour() == glm::vec3(0) &&
              timed.entries()[1].fadedColour() == glm::vec3(0, 0, .5f),
          "duration equality preserves a zero-colour source before later sources");
    check(timed.entries()[0].position == precise && timed.entries()[0].radius == 20,
          "fading retains complete fixed coordinates and the original radius");
    timed.advance(1);
    check(timed.entries().size() == 1 && timed.entries()[0].ageMs == 401 &&
              timed.entries()[0].position == point(16, 16),
          "strictly expired sources are removed while survivor order is retained");
    timed.append({point(16, 16), {0, 0, 1}, 16, 0, 800});
    timed.append({point(16, 16), {0, 0, 1}, 16, 0, 800});
    check(timed.entries().size() == 3 && timed.entries()[1].ageMs == 0 && timed.entries()[2].ageMs == 0,
          "later producer appends do not merge coincident identities or inherit old age");
    timed.advance(399);
    check(timed.entries().size() == 3 && timed.entries()[0].ageMs == 800,
          "mixed source durations retain the later equality boundary");
    timed.advance(1);
    check(timed.entries().size() == 2 && timed.entries()[0].ageMs == 400 && timed.entries()[1].ageMs == 400,
          "deferred expiry keeps both younger duplicates in append order");
    check(FadingSource{{}, {1, .5f, .25f}, 20, -200, 400}.fadedColour() == glm::vec3(1.5f, .75f, .375f),
          "fading has a lower-only clamp rather than an invented upper cap");
    check(FadingSource{{}, {1, 1, 1}, 20, 401, 400}.fadedColour() == glm::vec3(0),
          "render evaluation clamps an ordinary negative fade");
    FadingSources largeAge;
    largeAge.append({{}, {1, 1, 1}, 20, 16777216, 16777217});
    check(largeAge.entries()[0].fadedColour() == glm::vec3(0),
          "age and duration narrow to float separately before division");
    largeAge.advance(1);
    check(largeAge.entries().size() == 1 && largeAge.entries()[0].ageMs == 16777217,
          "integer ages advance beyond binary32's unit precision and preserve equality");
    largeAge.advance(1);
    check(largeAge.entries().empty(), "integer expiry does not stall at the float precision boundary");

    creatures::Item glowing;
    glowing.spirits = 3;
    glowing.cubes = {{0, 0, 0, 128}, {0, 0, 0, 132}, {0, 0, 0, 255}, {0, 0, 0, 130}};
    glm::vec4 average = creatures::averageSpiritColour(glowing);
    check(std::bit_cast<uint32_t>(average.r) == 0x3f555556 &&
              std::bit_cast<uint32_t>(average.g) == 0x3f155556 &&
              std::bit_cast<uint32_t>(average.b) == 0x3ec44444 &&
              average.a == 5.f * (1.f / 3.f),
          "all material bytes >=128 qualify; four ordered sums use one float reciprocal");
    glowing.spirits = 1;
    average = creatures::averageSpiritColour(glowing);
    check(average == glm::vec4(1.5f, .75f, .15f, 3.f),
          "only the descriptor's signed-count prefix contributes to item glow; spirit alpha is 3");
    glowing.spirits = -1;
    check(creatures::averageSpiritColour(glowing) == glm::vec4(0),
          "nonpositive descriptor count writes zero even with retained decoded storage");
    glowing.spirits = 1;
    glowing.cubes[0].material = 127;
    check(creatures::averageSpiritColour(glowing) == glm::vec4(0),
          "ordinary materials below128 do not qualify and leave zero sums");
    // Materials §1.1: K(m, C, s).
    const glm::vec4 lit(.8f, .6f, 1.25f, .9f);
    check(creatures::materialColour(1, lit, 7) == glm::vec4(.8f * .7f, .6f * .7f, 1.25f * .7f, .9f),
          "ordinary materials scale the input colour, pass its alpha and ignore strength");
    check(creatures::materialColour(11, lit, 0).r == .8f, "gold copies the red input channel");
    check(creatures::materialColour(10, lit, 0) == glm::vec4(.4f, .3f, .625f, .9f), "copper halves the input");
    check(creatures::materialColour(128, glm::vec4(1, 1, 1, .8f), .5f) == glm::vec4(1.25f, .625f, .1f * 1.25f, (.8f + .5f) * 1.25f),
          "fire ignores the input RGB: f = s * 0.5 + 1 and alpha (Ca + s) * f (example 2)");
    check(creatures::materialColour(131, lit, 0) == glm::vec4(.8f, .8f, 1.f, .9f), "wind at strength zero");

    const auto fixed = fromBlocks({1.0 / 65536, -1.0 / 65536, .5});
    check(fixed && *fixed == FixedPosition{1, -1, 32768}, "world coordinates retain fixed-point fractions");
    check(!fromBlocks({std::numeric_limits<double>::infinity(), 0, 0}),
          "invalid engine coordinates fail conversion without an overflowing cast");
    check(priority({200, 200, 0}, {}) == 0.f, "each squared component divides before priority addition");
    const FixedPosition reference = point(8396928, 8396928);
    const FixedPosition nearReference{reference[0] + 512, reference[1] + 512, 0};
    check(priority(nearReference, reference) == 1.f / 8192.f,
          "priority subtracts complete large fixed coordinates before narrowing");

    std::vector<Candidate> small(5);
    const float keys[] = {2, 1, 1, 0, 1};
    for (size_t i = 0; i < small.size(); ++i) {
        small[i].position[2] = int64_t(i);
        small[i].priority = keys[i];
    }
    order(small);
    const int64_t expected[] = {3, 1, 2, 4, 0};
    bool stable = true;
    for (size_t i = 0; i < small.size(); ++i) stable &= small[i].position[2] == expected[i];
    check(stable, "small finite-key ties retain source order without a secondary key");
    std::vector<Candidate> partition(33);
    for (size_t i = 0; i < partition.size(); ++i) {
        partition[i].position[2] = int64_t(i);
        partition[i].priority = i == 16 ? 0.f : 1.f;
    }
    order(partition);
    bool witness = true;
    for (size_t i = 0; i < partition.size(); ++i)
        witness &= partition[i].position[2] == int64_t(i == 0 ? 16 : i == 16 ? 0 : i);
    check(witness, "the 33-record native tie witness changes the retained prefix");
    std::vector<Candidate> equal(96);
    for (size_t i = 0; i < equal.size(); ++i) {
        equal[i].position[2] = int64_t(i);
        equal[i].priority = std::bit_cast<float>(i & 1 ? 0x80000000u : 0u);
    }
    order(equal);
    bool unchanged = true;
    for (size_t i = 0; i < equal.size(); ++i) unchanged &= equal[i].position[2] == int64_t(i);
    check(unchanged, "all-equal signed-zero keys retain their sequence above the partition threshold");

    Selector selector;
    const std::array<Chunk, 3> chunks{{{{0, 0}, 7}, {{1, 0}, 8}, {{1, 1}, 9}}};
    std::vector<Candidate> candidates{light(31, 31, 1, {.3f, .5f, .7f})};
    candidates[0].position[2] = 10000 * 65536ll;
    const auto &corner = selector.rebuild(candidates, chunks, {-32768, 65536, 65536});
    check(corner[0].count == 1 && corner[1].count == 1 && corner[2].count == 1,
          "inclusive XY rectangles admit corner chunks without a sphere or vertical test");
    check(corner[0].positions[0] == glm::vec4(30.5f, 32.f, 10000.f, 1.f) &&
              corner[1].positions[0] == corner[0].positions[0] &&
              corner[0].colours[0] == glm::vec4(.3f, .5f, .7f, 0),
          "horizontal shifts precede narrowing and the selected position is shared across chunks");

    candidates.clear();
    for (int i = 0; i < 19; ++i) {
        auto candidate = light(16, 16, 0, glm::vec3(float(i)));
        candidate.position[2] = i * 65536ll;
        candidates.push_back(candidate);
    }
    candidates.push_back(light(48, 16, 1, {0, 0, 1}));
    const auto &capped = selector.rebuild(candidates, chunks, {});
    check(capped[0].count == 16 && capped[1].count == 1 && capped[2].count == 0 &&
              capped[0].positions[15].z == 15.f,
          "each chunk retains its first sixteen qualifying candidates rather than a global prefix");
    check(capped[0].positions[0].w == 0.f && capped[0].colours[0] == glm::vec4(0),
          "zero radius and zero colour still occupy selected slots");
    const auto uploaded = shaderPrefix(capped[0]);
    check(uploaded.positions.size() == 10 && uploaded.positions[9].z == 9.f &&
              uploaded.colours[9] == glm::vec4(9, 9, 9, 0),
          "the shader upload copies exactly the first ten resident slots");

    candidates = {light(-1, 0, 0)};
    const auto &negative = selector.rebuild(candidates, chunks, {});
    check(negative[0].count == 1, "negative endpoints use two truncating divisions before world bounds");
    const std::array wrappedCandidates{light(0xffff'ffffll, 0, 0, {1, 0, 0}),
                                      light(0x1'0000'0020ll, 0, 0, {0, 1, 0})};
    const auto &wrapped = selector.rebuild(wrappedCandidates, chunks, {});
    check(wrapped[0].count == 1 && wrapped[0].colours[0] == glm::vec4(1, 0, 0, 0) &&
              wrapped[0].positions[0].x == float(0xffff'ffffll) &&
              wrapped[1].count == 1 && wrapped[1].colours[0] == glm::vec4(0, 1, 0, 0) && wrapped[2].count == 0,
          "chunk endpoints divide the signed low 32 column bits and keep the full light position for upload");
    const auto &cleared = selector.rebuild({}, chunks, {});
    bool allZero = true;
    for (const auto &selection : cleared) {
        allZero &= selection.count == 0;
        for (size_t i = 0; i < CAPACITY; ++i)
            allZero &= selection.positions[i] == glm::vec4(0) && selection.colours[i] == glm::vec4(0);
    }
    check(allZero, "rebuild clears every unused position, radius and colour after lights disappear");
    const std::array<Chunk, 1> replaced{{{{0, 0}, 42}}};
    const auto &changed = selector.rebuild(candidates, replaced, {});
    check(changed.size() == 1 && changed[0].mesh == 42 && changed[0].count == 1,
          "geometry changes refresh cached chunk-to-selection indices");
    const std::array<Chunk, 2> outside{{{{-1, 0}, 1}, {{0x80000, 0}, 2}}};
    const auto &rejected = selector.rebuild(candidates, outside, {});
    check(rejected[0].count == 0 && rejected[1].count == 0, "native chunk bounds reject both exterior edges");
    check(selector.rebuild(candidates, {}, {}).empty(), "unloading all geometry removes its light selections");
    // Rendering rulings §13: a chunk's water sub-mesh sees that chunk's own lights, also for a
    // chunk with water and no solid faces.
    const std::array<Chunk, 2> wet{{{{0, 0}, 5, 6}, {{1, 0}, 0, 11}}};
    candidates = {light(40, 16, 1, {0, 1, 0})};
    const auto &waterRows = selector.rebuild(candidates, wet, {});
    check(waterRows.size() == 2 && waterRows[0].water == 6 && waterRows[0].count == 0 && waterRows[1].mesh == 0 &&
              waterRows[1].water == 11 && waterRows[1].count == 1 && waterRows[1].colours[0] == glm::vec4(0, 1, 0, 0),
          "water sub-meshes carry their own chunk's light selection");
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
