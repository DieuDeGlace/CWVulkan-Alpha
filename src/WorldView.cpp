#include "WorldView.h"
#include "creatures/Npcs.h"
#include "DataStore.h"
#include "worldgen/MapTile.h"
#include "worldgen/WaterFeatures.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

using namespace worldgen;

namespace {

double nowSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Vegetation spec §1.1: plant kind -> model (the kinds the plant pass places).
const char *plantModelName(int32_t kind) {
    static const char *const NAMES[] = {
        "flowers2.cub", "flowers.cub", "grass.cub", "grass2.cub", "grass3.cub", "underwater-plant.cub",
        "coral.cub", "alga.cub", "lava-flower.cub", "lava-grass.cub", "thorn-plant.cub", "echinacea2.cub",
        "leaf.cub",
    };
    if (kind >= 0 && kind < (int32_t)(sizeof NAMES / sizeof *NAMES))
        return NAMES[kind];
    switch (kind) {
    case 13: return "lantern02.cub";
    case 14: return "torch.cub";
    case 22: return "reed.cub";
    case 27: return "desert-flower01.cub";
    case 28: return "desert-flower02.cub";
    case 31: return "water-lily01.cub";
    case 32: return "water-lily02.cub";
    default: break;
    }
    // Structure sprites (structures spec §1.10.2 item 5, §6.2).
    static const char *const STRUCTURE[] = {
        "inn-sign.cub", "shop-sign.cub", "weapon-shop-sign.cub", "armor-shop-sign.cub", "identifier-sign.cub",
        "smithy-sign.cub", "carpentersshop-sign.cub", "weavingmill-sign.cub", "ivy.cub", "wall-roses-red.cub",
        "wall-roses-white.cub", "inca-art1.cub", "inca-art2.cub", "inca-art3.cub", "inca-art4.cub", "crest1.cub",
        "wall-skull.cub", "castle-chain.cub", "torch-red.cub", "torch-green.cub", "torch-blue.cub", "torch-yellow.cub",
        "liana.cub", "chandelier.cub", "cobwebs.cub", "cobwebs2.cub",
    };
    if (kind >= 33 && kind < 33 + (int32_t)(sizeof STRUCTURE / sizeof *STRUCTURE))
        return STRUCTURE[kind - 33];
    return nullptr;
}

// Static object kinds -> model (vegetation spec §1.2, structures spec §1.10, §2, §4, §6.2).
// The stone and sandstone tables and stools (13, 14, 15, 17) are named from the style they go
// with; the spec gives only their kinds.
const char *staticModelName(int32_t kind) {
    static const char *const SHELVES[] = {
        "shelf1.cub", "shelf2.cub", "shelf3.cub", "castle-shelf1.cub", "castle-shelf2.cub", "castle-shelf3.cub",
        "stone-shelf1.cub", "stone-shelf2.cub", "stone-shelf3.cub", "sandstone-shelf1.cub", "sandstone-shelf2.cub",
        "sandstone-shelf3.cub",
    };
    static const char *const VASES[] = {
        "flower-vase1.cub", "flower-vase2.cub", "flower-vase3.cub", "flower-vase4.cub", "flower-vase5.cub",
        "flower-vase6.cub", "flower-vase7.cub", "flower-vase8.cub", "flower-vase9.cub",
    };
    if (kind >= 32 && kind <= 43)
        return SHELVES[kind - 32];
    if (kind >= 56 && kind <= 64)
        return VASES[kind - 56];
    switch (kind) {
    case 0: return "goddess2.cub";
    case 1: return "door.cub";
    case 2: return "big-door.cub";
    case 3: return "window.cub";
    case 4: return "castle-window.cub";
    case 7: return "spike-trap.cub";
    case 10: return "chest-base02.cub";
    case 12: return "table.cub";
    case 13: return "stone-table.cub";
    case 14: return "sandstone-table.cub";
    case 15: return "stone-stool.cub";
    case 16: return "stool.cub";
    case 17: return "sandstone-stool.cub";
    case 19: return "bed.cub";
    case 21: return "market-stand1.cub";
    case 22: return "market-stand2.cub";
    case 23: return "market-stand3.cub";
    case 24: return "barrel.cub";
    case 25: return "crate.cub";
    case 26: return "open-crate.cub";
    case 27: return "sack.cub";
    case 20: return "bedtable.cub";
    case 47: return "flower-box01.cub";
    case 48: return "flower-box02.cub";
    case 49: return "flower-box03.cub";
    case 52: return "fence01.cub";
    case 53: return "fence02.cub";
    case 54: return "fence03.cub";
    case 55: return "fence04.cub";
    case 71: return "furnace.cub";
    case 72: return "anvil.cub";
    case 73: return "spinningwheel.cub";
    case 74: return "loom.cub";
    case 75: return "sawbench.cub";
    case 76: return "workbench.cub";
    case 77: return "customization-bench.cub";
    case 45: return "runestone.cub";
    case 50: return "street-light02.cub";
    case 51: return "street-light01.cub";
    case 65: return "campfire.cub";
    case 66: return "tent.cub";
    case 69: return "wood-mat.cub";
    default: return nullptr;
    }
}

} // namespace

WorldView::WorldView(Renderer &r, const Content &c, const WorldViewOptions &options, TileReady tileReady)
    : onTileReady(std::move(tileReady)), renderer(r), content(c), world(options.seed), terrain(world), radius(options.radius), clouds(terrain),
      motionSettings(options.motion) {
    origin = {floorDiv(options.x, Zone::SIZE) * Zone::SIZE, floorDiv(options.y, Zone::SIZE) * Zone::SIZE};
    eye = glm::vec3(options.x - origin.x, options.y - origin.y, 0);
    startSeconds = nowSeconds();
    terrain.swimmableWater = options.swimmableWater;
    terrain.finishSpawn = creatures::finishSpawn; // spawn objects §3
    terrain.prefabs = [this](const std::string &key) -> std::shared_ptr<const Prefab> {
        std::lock_guard lock(prefabLock);
        if (const auto it = prefabCache.find(key); it != prefabCache.end())
            return it->second;
        std::shared_ptr<Prefab> prefab;
        CubModel model;
        if (const auto bytes = content.read(key); bytes && model.load(bytes->data(), bytes->size())) {
            prefab = std::make_shared<Prefab>();
            prefab->sx = model.sx, prefab->sy = model.sy, prefab->sz = model.sz;
            prefab->rgb = std::move(model.rgb);
        }
        prefabCache[key] = prefab;
        return prefab;
    };
    const unsigned n = std::max(2u, std::thread::hardware_concurrency()) - 1; // one core for drawing
    for (unsigned i = 0; i < n; i++)
        workers.emplace_back([this] { workerLoop(); });

    // Clouds spec §1: one model, cloud02.cub.
    if (const auto bytes = content.read("cloud02.cub")) {
        CubModel model;
        if (model.load(bytes->data(), bytes->size()))
            cloudMesh = renderer.addMesh(model.mesh());
    }
    // The first zones, before the first frame.
    while (true) {
        update();
        if (idle())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    // Start a little above the ground at the start column.
    eye.z = options.z > -1e8f ? options.z : std::max(terrain.height(options.x, options.y), 0.f) + 40.f;
}

WorldView::~WorldView() {
    {
        std::lock_guard guard(jobsLock);
        stopping = true;
        jobs.clear();
    }
    jobsReady.notify_all();
    for (auto &t : workers)
        t.join();
}

void WorldView::post(std::function<void()> job) {
    {
        std::lock_guard guard(jobsLock);
        jobs.push_back(std::move(job));
    }
    jobsReady.notify_one();
}

void WorldView::workerLoop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock guard(jobsLock);
            jobsReady.wait(guard, [&] { return stopping || !jobs.empty(); });
            if (stopping)
                return;
            job = std::move(jobs.front());
            jobs.pop_front();
        }
        job();
    }
}

bool WorldView::idle() {
    std::lock_guard guard(doneLock);
    return zonesPending.empty() && chunksPending.empty() && doneZones.empty() && doneChunks.empty();
}

WorldView::Key WorldView::cameraZone() const {
    return {floorDiv(origin.x + (int32_t)std::floor(eye.x), Zone::SIZE),
            floorDiv(origin.y + (int32_t)std::floor(eye.y), Zone::SIZE)};
}

void WorldView::requestZone(Key key) {
    {
        std::lock_guard guard(doneLock);
        if (!zonesPending.insert(key).second)
            return;
    }
    post([this, key] {
        std::shared_ptr<Zone> zone = terrain.generate(key.first, key.second);
        // Screens §5.1.6: its map tile, meshed like a .cub model with a black shade.
        auto tile = std::make_shared<const MapTile>(buildMapTile(*zone, terrain));
        CubModel model;
        model.sx = tile->sx, model.sy = tile->sy, model.sz = tile->sz;
        model.rgb = tile->rgb;
        ModelMesh mesh = model.mesh(glm::vec3(0));
        std::lock_guard guard(doneLock);
        doneZones.push_back(std::move(zone));
        doneTiles.push_back({key, std::move(mesh), tile->base, tile->markers, tile});
    });
}

void WorldView::requestChunk(int32_t x0, int32_t y0) {
    const Key key{x0, y0};
    {
        std::lock_guard guard(doneLock);
        if (!chunksPending.insert(key).second) {
            chunksStale.insert(key); // mesh it again once this one is back
            return;
        }
    }
    // A snapshot of the zones round it, so the job reads without locks.
    auto around = std::make_shared<WorldMap>(map.neighbourhood(floorDiv(x0, Zone::SIZE), floorDiv(y0, Zone::SIZE)));
    post([this, around, x0, y0] {
        ChunkMeshes meshes = meshChunk(*around, terrain, x0, y0);
        std::vector<terrain_lights::CachedSource> lights;
        if (const Zone *zone = around->zoneAt(x0, y0))
            lights = terrain_lights::chunkSources(zone->sprites, {x0 / CHUNK, y0 / CHUNK});
        std::lock_guard guard(doneLock);
        doneChunks.push_back({{x0, y0}, std::move(meshes), std::move(lights)});
    });
}

void WorldView::addZone(std::shared_ptr<Zone> zone) {
    const int32_t zx = zone->zx, zy = zone->zy;
    sectorSources.update(*zone);
    map.add(std::move(zone));
    if (onZoneReady)
        onZoneReady(zx, zy);
    refreshPlantsNear({zx, zy});
    // Its own chunks, and the neighbours' chunks along the shared edges (their faces towards it
    // were closed while it was missing).
    const int32_t x0 = zx * Zone::SIZE, y0 = zy * Zone::SIZE;
    for (int32_t i = 0; i < Zone::SIZE; i += CHUNK)
        for (int32_t j = 0; j < Zone::SIZE; j += CHUNK)
            requestChunk(x0 + i, y0 + j);
    for (int32_t k = 0; k < Zone::SIZE; k += CHUNK) {
        if (map.zone(zx - 1, zy))
            requestChunk(x0 - CHUNK, y0 + k);
        if (map.zone(zx + 1, zy))
            requestChunk(x0 + Zone::SIZE, y0 + k);
        if (map.zone(zx, zy - 1))
            requestChunk(x0 + k, y0 - CHUNK);
        if (map.zone(zx, zy + 1))
            requestChunk(x0 + k, y0 + Zone::SIZE);
    }
}

void WorldView::applyEdits(const std::vector<BlockEdit> &edits, bool saved) {
    // Each held zone the records touch is copied, changed and swapped in, so the mesh jobs
    // still reading the old one are not disturbed.
    std::map<Key, std::vector<const BlockEdit *>> byZone;
    for (const BlockEdit &e : edits)
        if (map.zoneAt(e.x, e.y))
            byZone[{floorDiv(e.x, Zone::SIZE), floorDiv(e.y, Zone::SIZE)}].push_back(&e);
    std::set<Key> remesh;
    for (const auto &[key, list] : byZone) {
        auto zone = std::make_shared<Zone>(*map.zone(key.first, key.second));
        const GapFill rock = [&](int32_t x, int32_t y, int32_t z) { return terrain.gapBlock(x, y, z, *zone); };
        for (const BlockEdit *e : list) {
            if (saved)
                applySaved(*zone, *e, rock);
            else
                applyLive(*zone, *e, rock);
            for (int32_t i = -1; i <= 1; i++)
                for (int32_t j = -1; j <= 1; j++)
                    remesh.insert({(floorDiv(e->x, CHUNK) + i) * CHUNK, (floorDiv(e->y, CHUNK) + j) * CHUNK});
        }
        dropFloating(*zone);
        terrain.relight(*zone);
        map.add(std::move(zone));
    }
    for (const auto &[key, list] : byZone)
        refreshPlantsNear(key);
    for (const Key &corner : remesh)
        if (map.zoneAt(corner.first, corner.second))
            requestChunk(corner.first, corner.second);
}

void WorldView::dropZone(Key key) {
    map.remove(key.first, key.second);
    plants.erase(key);
    refreshPlantsNear(key);
    sectorSources.remove(key.first, key.second);
    // (Its map tile stays for the map; tiles go when they are 16 zones away, in update.)
    const int32_t x0 = key.first * Zone::SIZE, y0 = key.second * Zone::SIZE;
    for (auto it = chunks.lower_bound({x0, INT32_MIN}); it != chunks.end() && it->first.first < x0 + Zone::SIZE;) {
        if (it->first.second >= y0 && it->first.second < y0 + Zone::SIZE) {
            renderer.removeMesh(it->second.solid);
            renderer.removeMesh(it->second.water);
            cachedSources.reset({it->first.first / CHUNK, it->first.second / CHUNK});
            it = chunks.erase(it);
        } else
            ++it;
    }
}

void WorldView::placeChunk(DoneChunk &chunk) {
    const glm::ivec2 corner = chunk.corner;
    ChunkMeshes &meshes = chunk.meshes;
    if (!map.zoneAt(corner.x, corner.y))
        return; // its zone was let go meanwhile
    const glm::ivec2 coordinate = corner / CHUNK;
    cachedSources.stage(coordinate, std::move(chunk.lights));
    // Engine jobs supply geometry and source state from the same snapshot. The
    // original cache-key lookup and geometry counter producer remain unverified.
    const bool geometryReady = !meshes.solid.indices.empty() || !meshes.water.indices.empty();
    cachedSources.refresh(coordinate, true, geometryReady ? 1 : 0);
    ChunkEntry &entry = chunks[{corner.x, corner.y}];
    renderer.removeMesh(entry.solid);
    renderer.removeMesh(entry.water);
    entry.solid = meshes.solid.indices.empty() ? 0 : renderer.addMesh(meshes.solid);
    entry.water = meshes.water.indices.empty() ? 0 : renderer.addMesh(meshes.water);
}

void WorldView::update() {
    // Take in finished work.
    std::vector<std::shared_ptr<Zone>> zones;
    std::vector<DoneChunk> meshed;
    std::vector<DoneTile> newTiles;
    {
        std::lock_guard guard(doneLock);
        zones.swap(doneZones);
        newTiles.swap(doneTiles);
        meshed.swap(doneChunks);
        for (const auto &z : zones)
            zonesPending.erase({z->zx, z->zy});
    }
    // Map tiles more than 16 zones from the camera's zone are let go.
    {
        const Key here = cameraZone();
        for (auto it = tiles.begin(); it != tiles.end();) {
            if (std::abs(it->first.first - here.first) > 16 || std::abs(it->first.second - here.second) > 16) {
                renderer.removeMesh(it->second.mesh);
                it = tiles.erase(it);
            } else {
                ++it;
            }
        }
    }
    loadSavedTiles();
    for (DoneTile &t : newTiles) {
        TileEntry &entry = tiles[t.key];
        if (entry.mesh)
            renderer.removeMesh(entry.mesh); // a rebuild keeps its timer
        entry.mesh = t.mesh.indices.empty() ? 0 : renderer.addMesh(t.mesh);
        entry.base = t.base;
        entry.markers = std::move(t.markers);
        tiledZones.insert(t.key);
        if (onTileReady)
            onTileReady(t.key.first, t.key.second, *t.tile);
    }
    std::vector<Key> again;
    for (auto &chunk : meshed) {
        placeChunk(chunk);
        const auto corner = chunk.corner;
        std::lock_guard guard(doneLock);
        chunksPending.erase({corner.x, corner.y});
        if (chunksStale.erase({corner.x, corner.y}))
            again.push_back({corner.x, corner.y});
    }
    for (const Key &k : again)
        requestChunk(k.first, k.second);
    const Key here = cameraZone();
    for (auto &z : zones)
        if (std::abs(z->zx - here.first) <= radius + 1 && std::abs(z->zy - here.second) <= radius + 1)
            addZone(std::move(z));

    // Ask for the zones round the camera, nearest first, and let go of far ones.
    std::vector<Key> wanted;
    for (int32_t i = -radius; i <= radius; i++)
        for (int32_t j = -radius; j <= radius; j++)
            if (!map.zone(here.first + i, here.second + j))
                wanted.push_back({here.first + i, here.second + j});
    std::sort(wanted.begin(), wanted.end(), [&](const Key &a, const Key &b) {
        auto d = [&](const Key &k) { return std::abs(k.first - here.first) + std::abs(k.second - here.second); };
        return d(a) < d(b);
    });
    for (const Key &k : wanted)
        requestZone(k);
    std::vector<Key> far;
    for (const auto &[key, zone] : map.zones())
        if (std::abs(key.first - here.first) > radius + 1 || std::abs(key.second - here.second) > radius + 1)
            far.push_back(key);
    for (const Key &k : far)
        dropZone(k);
}

std::pair<MeshId, float> WorldView::plantModel(int32_t kind) {
    if (const auto it = plantModels.find(kind); it != plantModels.end())
        return it->second;
    std::pair<MeshId, float> entry{0, 1.f};
    if (const char *name = plantModelName(kind))
        if (const auto bytes = content.read(name)) {
            CubModel model;
            if (model.load(bytes->data(), bytes->size()))
                entry = {renderer.addMesh(model.mesh()), (float)model.sz};
        }
    plantModels[kind] = entry;
    return entry;
}

std::pair<MeshId, glm::vec3> WorldView::staticModel(int32_t kind) {
    if (const auto it = staticModels.find(kind); it != staticModels.end())
        return it->second;
    std::pair<MeshId, glm::vec3> entry{0, glm::vec3(1)};
    if (const char *name = staticModelName(kind))
        if (const auto bytes = content.read(name)) {
            CubModel model;
            if (model.load(bytes->data(), bytes->size()))
                entry = {renderer.addMesh(model.mesh()), glm::vec3(model.sx, model.sy, model.sz)};
        }
    staticModels[kind] = entry;
    return entry;
}

std::vector<WorldView::PlantInstance> WorldView::plantsOf(const Zone &zone) {
    std::vector<PlantInstance> out;
    // Static objects: the model fills the object's scale box, turned by quarter turns.
    for (const StaticObject &o : zone.statics) {
        const auto [mesh, voxels] = staticModel(o.kind);
        if (!mesh || movingStatic(o.kind))
            continue;
        const glm::vec3 at((float)((double)o.x / 65536.0 - origin.x), (float)((double)o.y / 65536.0 - origin.y),
                           (float)((double)o.z / 65536.0));
        PlantInstance instance;
        instance.mesh = mesh;
        instance.placed = glm::translate(glm::mat4(1), at);
        instance.yaw = glm::scale(glm::rotate(glm::mat4(1), glm::radians(90.f * (float)o.rotation), glm::vec3(0, 0, 1)),
                                  glm::vec3(o.scale[0], o.scale[1], o.scale[2]) / voxels);
        instance.scale = 1;
        instance.height = 1;
        instance.phase = 0;
        instance.sways = false;
        out.push_back(instance);
    }
    // Structure sprites: the model at its own voxel size, centred on the point, turned.
    for (const Sprite &s : zone.sprites) {
        const auto [mesh, height] = plantModel(s.kind);
        if (!mesh)
            continue;
        const glm::vec3 at((float)((double)s.x / 65536.0 - origin.x), (float)((double)s.y / 65536.0 - origin.y),
                           (float)((double)s.z / 65536.0));
        PlantInstance instance;
        instance.mesh = mesh;
        instance.placed = glm::scale(glm::translate(glm::mat4(1), at), glm::vec3(s.size));
        instance.yaw = glm::rotate(glm::mat4(1), glm::radians(s.angle), glm::vec3(0, 0, 1));
        instance.scale = s.size;
        instance.height = height;
        instance.phase = (int)(s.x ^ s.y);
        instance.sways = (s.flags & motion::SWAYS) != 0;
        out.push_back(instance);
    }
    const auto addPlant = [&](const Plant &p, size_t phase) {
        const auto [mesh, height] = plantModel(p.kind);
        if (!mesh)
            return;
        // Centred on the block, turned, swayed, scaled, placed (open points 12 §2,
        // vegetation-wind.md §4). The phase only needs to stay the same for a plant.
        PlantInstance instance;
        instance.mesh = mesh;
        instance.placed = glm::scale(glm::translate(glm::mat4(1), glm::vec3(p.x - origin.x + 0.5f, p.y - origin.y + 0.5f, p.z - (p.sink ? 0.1f : 0.f))),
                                     glm::vec3(p.scale));
        instance.yaw = glm::rotate(glm::mat4(1), glm::radians(p.rotation), glm::vec3(0, 0, 1));
        instance.scale = p.scale;
        instance.height = height;
        instance.phase = (int)phase + 1;
        instance.sways = (p.flags & motion::SWAYS) != 0;
        out.push_back(instance);
    };
    for (size_t n = 0; n < zone.plants.size(); n++)
        if (!terrain.swimmableWater || zone.plants[n].kind != 22)
            addPlant(zone.plants[n], n);
    if (terrain.swimmableWater) {
        const auto reeds = bankReeds(map, zone, world.seed());
        for (const Plant &reed : reeds) {
            // Neighbor admission can add earlier reeds to the cache. A phase
            // derived from the place keeps existing bank plants from jumping.
            const uint32_t phase = ((uint32_t)reed.x ^ ((uint32_t)reed.y * 65537u)) & 0x3fffffffu;
            addPlant(reed, phase);
        }
    }
    return out;
}

void WorldView::refreshPlantsNear(Key key) {
    constexpr int offsets[5][2] = {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto &offset : offsets) {
        const Key neighbour{key.first + offset[0], key.second + offset[1]};
        if (const Zone *zone = map.zone(neighbour.first, neighbour.second))
            plants[neighbour] = plantsOf(*zone);
    }
}

uint8_t WorldView::zoneFlags(int32_t zx, int32_t zy) const {
    return (uint8_t)((discovered(zx, zy) ? 1 : 0) | (tiledZones.count({zx, zy}) ? 2 : 0));
}

void WorldView::restoreZone(int32_t zx, int32_t zy, uint8_t flags) {
    if (flags & 1)
        discoveredZones.insert({zx, zy});
    if (flags & 2)
        tiledZones.insert({zx, zy});
}

// §5.1.6 step 4: zones within 10 of the camera's that have a saved tile but none in memory
// read it back, nearest first, up to 10 per frame.
void WorldView::loadSavedTiles() {
    if (!savedTile)
        return;
    const Key here = cameraZone();
    std::vector<std::pair<int64_t, Key>> needed;
    for (int32_t zx = here.first - 10; zx < here.first + 10; zx++)
        for (int32_t zy = here.second - 10; zy < here.second + 10; zy++) {
            const Key key{zx, zy};
            if (tiledZones.count(key) && !tiles.count(key) && !unreadableTiles.count(key) && !map.zone(zx, zy)) {
                const int64_t dx = zx - here.first, dy = zy - here.second;
                needed.push_back({dx * dx + dy * dy, key});
            }
        }
    std::stable_sort(needed.begin(), needed.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    if (needed.size() > 10)
        needed.resize(10);
    for (const auto &[d, key] : needed) {
        const std::optional<worldgen::MapTile> tile = savedTile(key.first, key.second);
        if (!tile) {
            unreadableTiles.insert(key);
            continue;
        }
        CubModel model;
        model.sx = tile->sx, model.sy = tile->sy, model.sz = tile->sz;
        model.rgb = tile->rgb;
        TileEntry &entry = tiles[key];
        entry.mesh = model.sx * model.sy * model.sz ? renderer.addMesh(model.mesh(glm::vec3(0))) : 0;
        entry.base = tile->base;
        entry.markers = tile->markers;
    }
}

void WorldView::discover(glm::dvec3 at) {
    for (const double dx : {-64.0, 64.0})
        for (const double dy : {-64.0, 64.0})
            discoveredZones.insert({floorDiv((int32_t)std::floor(at.x + dx), Zone::SIZE),
                                    floorDiv((int32_t)std::floor(at.y + dy), Zone::SIZE)});
    // The land stood in is visited; its picture is rebuilt at most every 2 s (new zones show).
    const Key land = landOf((int32_t)std::floor(at.x), (int32_t)std::floor(at.y));
    visitedLands.insert(land);
    // (Here only when something new is known: the scan covers some 37,000 zones.)
    LandPicture &picture = lands[land];
    if (nowSeconds() - picture.builtAt > 2 && picture.knownZones != discoveredZones.size() + tiledZones.size())
        buildLand(land);
}

WorldView::Key WorldView::landOf(int32_t x, int32_t y) const {
    const worldgen::RegionCell *c = world.owner(x, y);
    if (!c)
        return {-1, -1};
    const int32_t v = c->id - world.A(); // id = 1024 ry + rx + A
    return {v % 1024, v / 1024};
}

// §5.1.6 "Land pictures": the zones the land owns, one voxel each, white where discovered,
// light grey where only a tile is known, grey otherwise.
void WorldView::buildLand(Key cell) {
    LandPicture &land = lands[cell];
    land.builtAt = nowSeconds();
    land.knownZones = discoveredZones.size() + tiledZones.size();
    const worldgen::RegionCell *c = world.cell(cell.first, cell.second);
    if (!c)
        return;
    land.elevation = c->elevation;
    auto owns = [&](int32_t zx, int32_t zy) { return world.landOwnsZone(cell.first, cell.second, zx, zy); };
    const int32_t z0x = 64 * cell.first - 64, z0y = 64 * cell.second - 64;
    glm::ivec2 lo(INT32_MAX), hi(INT32_MIN);
    for (int32_t zx = z0x; zx <= 64 * cell.first + 128; zx++)
        for (int32_t zy = z0y; zy <= 64 * cell.second + 128; zy++)
            if (owns(zx, zy)) {
                lo = glm::min(lo, glm::ivec2(zx, zy));
                hi = glm::max(hi, glm::ivec2(zx, zy));
            }
    if (lo.x > hi.x)
        return;
    CubModel picture;
    picture.sx = hi.x - lo.x + 1, picture.sy = hi.y - lo.y + 1, picture.sz = 1;
    picture.rgb.assign((size_t)picture.sx * picture.sy * 3, 0);
    for (int32_t zx = z0x; zx < 64 * cell.first + 128; zx++)
        for (int32_t zy = z0y; zy < 64 * cell.second + 128; zy++) {
            if (!owns(zx, zy))
                continue;
            const uint8_t v = discovered(zx, zy) ? 255 : tiledZones.count({zx, zy}) ? 220 : 200;
            uint8_t *p = &picture.rgb[3 * ((size_t)(zy - lo.y) * picture.sx + (zx - lo.x))];
            p[0] = p[1] = p[2] = v;
        }
    if (land.mesh)
        renderer.removeMesh(land.mesh);
    land.mesh = renderer.addMesh(picture.mesh(glm::vec3(0)));
    land.origin = lo;
    land.size = {picture.sx, picture.sy};
    if (onLandReady)
        onLandReady(cell.first, cell.second, *c, lo, picture);
}

std::vector<DrawItem> WorldView::landItems(glm::dvec3 pivot, int32_t radius) {
    std::vector<DrawItem> out;
    const Key here = landOf((int32_t)std::floor(pivot.x), (int32_t)std::floor(pivot.y));
    for (const Key &cell : visitedLands) {
        if (std::abs(cell.first - here.first) > radius || std::abs(cell.second - here.second) > radius)
            continue;
        const auto it = lands.find(cell);
        if (it == lands.end() || !it->second.mesh)
            continue;
        const LandPicture &land = it->second;
        // A slab one zone per voxel from 256 blocks under the pivot up to its level, taller with
        // the land's elevation, or sunk half a voxel more below sea level.
        const float bottom = land.elevation < 0 ? -384.f : -256.f;
        const float height = land.elevation < 0 ? 256.f : 256.f * (1.f + (float)land.elevation / 256.f);
        glm::mat4 m = glm::translate(glm::mat4(1), glm::vec3(land.origin.x * Zone::SIZE - origin.x,
                                                             land.origin.y * Zone::SIZE - origin.y, (float)pivot.z + bottom));
        m = glm::scale(m, glm::vec3(256, 256, height));
        m = glm::translate(m, glm::vec3(land.size.x * 0.5f, land.size.y * 0.5f, 0));
        const float k = 0.8f + 0.1f * (float)(((cell.first + 2 * cell.second) % 4 + 4) % 4);
        out.push_back({land.mesh, m, false, Shading::Map,
                       glm::vec4(std::min(0.2f * k, 1.f), std::min(0.4f * k, 1.f), std::min(1.f * k, 1.f), 1)});
    }
    return out;
}

std::vector<DrawItem> WorldView::mapItems(int32_t zx0, int32_t zy0, int32_t zx1, int32_t zy1, float dtMs, bool extras) {
    std::vector<DrawItem> out;
    if (extras && !placeholder)
        if (const auto bytes = content.read("maptile.cub")) {
            CubModel model;
            if (model.load(bytes->data(), bytes->size()))
                placeholder = renderer.addMesh(model.mesh(glm::vec3(0)));
        }
    for (int32_t zx = zx0; zx <= zx1; zx++)
        for (int32_t zy = zy0; zy <= zy1; zy++) {
            const auto it = tiles.find({zx, zy});
            TileEntry *t = it != tiles.end() && it->second.mesh ? &it->second : nullptr;
            const bool known = discovered(zx, zy);
            // §5.2.4 step 1: a blue block where there is no tile yet, sinking as the tile rises.
            if (extras && placeholder && (!t || t->riseMs > 0)) {
                const worldgen::RegionCell *cell = world.owner(zx * Zone::SIZE, zy * Zone::SIZE);
                const float E = cell ? (float)cell->elevation : 0.f;
                const float rise = t ? (t->riseMs / 250.f - 1.f) * 20.f : 0.f;
                glm::mat4 m = glm::translate(glm::mat4(1), glm::vec3(zx * Zone::SIZE - origin.x, zy * Zone::SIZE - origin.y,
                                                                     (E - 100.f) + rise * 8.f));
                m = glm::scale(m, glm::vec3(32));
                m = glm::translate(m, glm::vec3(4, 4, 0));
                const glm::vec4 tint = known ? glm::vec4(0.2f, 0.4f, 1, 1) : t ? glm::vec4(0.2f, 0.35f, 1, 1) : glm::vec4(0.2f, 0.3f, 1, 1);
                out.push_back({placeholder, m, false, Shading::Map, tint});
            }
            if (!t)
                continue;
            // §5.1.2: h = B - 0.1 timer map units; the mesh is centred on x/y, voxels are 8 blocks.
            const float h = (float)t->base - 0.1f * t->riseMs;
            t->riseMs = std::max(0.f, t->riseMs - dtMs);
            glm::mat4 m = glm::translate(glm::mat4(1), glm::vec3(zx * Zone::SIZE - origin.x, zy * Zone::SIZE - origin.y, h * 8));
            m = glm::scale(m, glm::vec3(8));
            m = glm::translate(m, glm::vec3(16, 16, 0));
            // (The target zone of a discovered mission is never dimmed.)
            const bool target = missionTarget && missionTarget(zx, zy);
            const glm::vec4 material = extras && !known && !target ? glm::vec4(0.6f, 0.6f, 0.6f, 1) : glm::vec4(1);
            out.push_back({t->mesh, m, false, Shading::Map, material});
        }
    // §5.1.2: the land-border markers, a white 2 x 2 square with a dark rim, scaled by 5.
    if (!borderMarker) {
        CubModel marker;
        marker.sx = marker.sy = 4, marker.sz = 1;
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++) {
                const uint8_t v = (x == 1 || x == 2) && (y == 1 || y == 2) ? 255 : 10;
                marker.rgb.insert(marker.rgb.end(), {v, v, v});
            }
        borderMarker = renderer.addMesh(marker.mesh(glm::vec3(0)));
    }
    for (int32_t zx = zx0; zx <= zx1; zx++)
        for (int32_t zy = zy0; zy <= zy1; zy++)
            if (const auto it = tiles.find({zx, zy}); it != tiles.end())
                for (const glm::ivec3 &p : it->second.markers) {
                    glm::mat4 m = glm::translate(glm::mat4(1), toRender(glm::dvec3(p)));
                    m = glm::scale(m, glm::vec3(5));
                    m = glm::translate(m, glm::vec3(2, 2, 0));
                    out.push_back({borderMarker, m, false, Shading::Map, glm::vec4(1, 1, 1, 3)});
                }
    return out;
}

std::vector<DrawItem> WorldView::items(double clockMs) {
    std::vector<DrawItem> out;
    out.reserve(chunks.size() * 2 + 4096);
    for (const auto &[corner, entry] : chunks) {
        const glm::mat4 place = glm::translate(glm::mat4(1), glm::vec3(corner.first - origin.x, corner.second - origin.y, 0));
        if (entry.solid)
            out.push_back({entry.solid, place, true, Shading::World});
        if (entry.water)
            out.push_back({entry.water, place, false, Shading::Water});
    }
    for (const auto &[key, list] : plants)
        for (const PlantInstance &p : list) {
            const glm::mat4 sway = p.sways ? motion::plantShear(clockMs, p.phase, p.height, p.scale) : glm::mat4(1);
            out.push_back({p.mesh, p.placed * sway * p.yaw, true});
        }
    // Clouds (clouds spec §3 step 7): turned, scaled, placed; then AlphaVulkan's drift.
    if (cloudMesh) {
        const int32_t cx = origin.x + (int32_t)std::floor(eye.x), cy = origin.y + (int32_t)std::floor(eye.y);
        const glm::vec2 drift = motionSettings.clouds
                                    ? motion::cloudDrift(clockMs / 1000.0, motionSettings.cloudDistance, motionSettings.cloudSpeed)
                                    : glm::vec2(0);
        for (const auto &c : clouds.place(cx, cy, cx, cy)) {
            const glm::vec3 at((float)(c.x - origin.x) + drift.x, (float)(c.y - origin.y) + drift.y, (float)c.z);
            glm::mat4 m = glm::translate(glm::mat4(1), at);
            m = glm::scale(m, glm::vec3(c.scale));
            m = glm::rotate(m, glm::radians(c.rotation), glm::vec3(0, 0, 1));
            out.push_back({cloudMesh, m, true, Shading::Cloud});
        }
    }
    return out;
}

Camera WorldView::camera(VkExtent2D size) const {
    Camera c;
    c.eye = eye;
    c.yawDegrees = yaw;
    c.pitchDegrees = pitch;
    c.aspect = (float)size.width / (float)std::max(1u, size.height);
    return c;
}

void WorldView::move(glm::vec3 local, float seconds, bool fast) {
    const float speed = (fast ? 120.f : 30.f) * seconds;
    const float y = glm::radians(yaw);
    const glm::vec3 forward(std::sin(y), std::cos(y), 0), right(std::cos(y), -std::sin(y), 0);
    eye += (right * local.x + forward * local.y + glm::vec3(0, 0, local.z)) * speed;
}

void WorldView::turn(float dy, float dp) { setLook(yaw + dy, pitch + dp); }

void WorldView::placeCamera(glm::dvec3 at, glm::vec3 d) {
    eye = toRender(at);
    // This view's angles: yaw from +y towards +x, pitch positive looking down.
    setLook(glm::degrees(std::atan2(d.x, d.y)), -glm::degrees(std::asin(std::clamp(d.z, -1.f, 1.f))));
}

void WorldView::setLook(float y, float p) {
    yaw = y;
    pitch = std::clamp(p, -89.f, 89.f);
}

void WorldView::fillScene(Scene &scene) const {
    const int32_t x = origin.x + (int32_t)std::floor(eye.x), y = origin.y + (int32_t)std::floor(eye.y);
    const Climate cl = terrain.climate(x, y, map.zoneAt(x, y));
    scene.zone = {cl.warmth, cl.moisture, world.seaShare(x, y), cl.fire};
    scene.focus = glm::vec3(eye.x, eye.y, std::max(eye.z - 40.f, 0.f));
    scene.underwater = map.read(x, y, (int32_t)std::floor(eye.z)).type() == WATER;

    scene.lights.clear();
}

void WorldView::finishLighting(Scene &scene, const WorldItems &items, int32_t sourceClockMs,
                                std::optional<terrain_lights::FixedPosition> controlled) const {
    const auto reference = terrain_lights::fromBlocks(worldPosition());
    if (!reference) {
        renderer.setTerrainLights({});
        return;
    }
    const terrain_lights::FixedPosition shift{-int64_t(origin.x) * 65536, -int64_t(origin.y) * 65536, 0};
    lightCandidates.clear();
    lightCandidates.reserve(scene.lights.size());
    const float daylight = alpha::sunlight(scene.timeOfDayMs);
    const float reach = scene.fogDistance + 22.4f;
    const terrain_lights::ViewAdmission earlyView(*reference, shift, storedLightPlanes, reach);
    cachedSources.append(lightCandidates, earlyView, sourceClockMs, daylight);
    // Native cached-source reads precede current plane production. This engine
    // retains its last completed publication; native publication age is unknown.
    storedLightPlanes = camera(renderer.size()).lightAdmissionPlanes();
    for (const PointLight &source : scene.lights) {
        const auto position = source.fixedPosition ? source.fixedPosition :
            terrain_lights::fromBlocks(glm::dvec3(source.position) + glm::dvec3(origin.x, origin.y, 0));
        if (position)
            lightCandidates.push_back({*position, source.colour, source.radius, 0});
    }
    const terrain_lights::ViewAdmission view(*reference, shift, storedLightPlanes, reach);
    // A flying viewer has no controlled creature; use its camera as an explicit
    // engine fallback. Playing uses the controlled body's main fixed position.
    sectorSources.append(lightCandidates, items, view, controlled.value_or(*reference), daylight);
    for (auto &candidate : lightCandidates)
        candidate.priority = terrain_lights::priority(candidate.position, *reference);
    terrain_lights::order(lightCandidates);
    scene.lights.clear();
    for (const auto &candidate : lightCandidates)
        scene.lights.push_back({terrain_lights::renderPosition(candidate.position, shift), candidate.radius,
                                candidate.colour, candidate.position});
    lightChunks.clear();
    lightChunks.reserve(chunks.size());
    for (const auto &[corner, entry] : chunks)
        if (entry.solid)
            lightChunks.push_back({{corner.first / 32, corner.second / 32}, entry.solid});
    renderer.setTerrainLights(lightSelector.rebuild(lightCandidates, lightChunks, shift));
}

std::string WorldView::status() const {
    size_t plantCount = 0;
    for (const auto &[key, list] : plants)
        plantCount += list.size();
    char text[160];
    std::snprintf(text, sizeof text, "%zu zones, %zu chunks, %zu plants, ready in %.1f s", map.zones().size(),
                  chunks.size(), plantCount, nowSeconds() - startSeconds);
    return text;
}
