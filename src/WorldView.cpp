#include "WorldView.h"
#include "ChestPose.h"
#include "StaticStates.h"
#include "WaterLook.h"
#include "creatures/Npcs.h"
#include "DataStore.h"
#include "worldgen/MapTile.h"
#include "worldgen/SpatialScalars.h"
#include "worldgen/WaterFeatures.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <utility>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace worldgen;

static_assert(ChunkMeshCache::SIDE == alpha::FOG_WINDOW_CHUNKS);

namespace {

double nowSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void remeshColumn(std::set<std::pair<int32_t, int32_t>> &chunks, int32_t x, int32_t y) {
    for (int32_t i = -1; i <= 1; i++)
        for (int32_t j = -1; j <= 1; j++)
            chunks.emplace((floorDiv(x, CHUNK) + i) * CHUNK, (floorDiv(y, CHUNK) + j) * CHUNK);
}

// Vegetation spec §1.1: plant kind -> model (the kinds the plant pass places).
const char *plantModelName(int32_t kind) {
    static const char *const names[] = {
        "flowers2.cub", "flowers.cub", "grass.cub", "grass2.cub", "grass3.cub", "underwater-plant.cub",
        "coral.cub", "alga.cub", "lava-flower.cub", "lava-grass.cub", "thorn-plant.cub", "echinacea2.cub",
        "leaf.cub", "lantern02.cub", "torch.cub", "stone.cub", "stone2.cub", "christmas-tree.cub", nullptr,
        "tendril.cub", "tulips-colorful.cub", "cornflower.cub", "reed.cub", "pumpkin-leaves.cub",
        "pineapple-leaves.cub", "sunflower.cub", "bean-tendril.cub", "desert-flower01.cub", "desert-flower02.cub",
        "wheat.cub", "corn.cub", "water-lily01.cub", "water-lily02.cub", "inn-sign.cub", "shop-sign.cub",
        "weapon-shop-sign.cub", "armor-shop-sign.cub", "identifier-sign.cub", "smithy-sign.cub",
        "carpentersshop-sign.cub", "weavingmill-sign.cub", "ivy.cub", "wall-roses-red.cub", "wall-roses-white.cub",
        "inca-art1.cub", "inca-art2.cub", "inca-art3.cub", "inca-art4.cub", "crest1.cub", "wall-skull.cub",
        "castle-chain.cub", "torch-read.cub", "torch-green.cub", "torch-blue.cub", "torch-yellow.cub",
        "liana.cub", "chandelier.cub", "cobwebs.cub", "cobwebs2.cub",
    };
    return kind >= 0 && size_t(kind) < std::size(names) ? names[kind] : nullptr;
}

} // namespace

WorldView::WorldView(Renderer &r, const Content &c, const WorldViewOptions &options, TileReady tileReady)
    : onTileReady(std::move(tileReady)), renderer(r), content(c), worldData(std::make_unique<WorldData>(options.seed)),
      radius(options.radius), meshRenderDistance(options.renderDistance), motionSettings(options.motion) {
    origin = {floorDiv(options.x, Zone::SIZE) * Zone::SIZE, floorDiv(options.y, Zone::SIZE) * Zone::SIZE};
    eye = glm::vec3(options.x - origin.x, options.y - origin.y, 0);
    startSeconds = nowSeconds();
    auto &terrain = worldData->terrain;
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
    const unsigned n = std::max(3u, std::thread::hardware_concurrency()) - 2; // mesh and render threads
    for (unsigned i = 0; i < n; i++)
        workers.emplace_back([this] { workerLoop(); });
    meshWorker = std::thread([this] { meshWorkerLoop(); });

    // Clouds spec §1: one model, cloud02.cub, meshed with the shade (0, 100, 200) the frame
    // gives it (rendering §11.1, rulings §1).
    if (const auto bytes = content.read("cloud02.cub")) {
        CubModel model;
        if (model.load(bytes->data(), bytes->size()))
            cloudMesh = renderer.addMesh(model.mesh(glm::vec3(0, 100, 200) / 255.f));
    }
    // Submit the first source requests before returning to the frame pump.
    while (true) {
        update();
        if (!options.preloadTerrain || idle())
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
        scheduledMesh.reset();
        cancelMeshBatch(true);
    }
    jobsReady.notify_all();
    meshWorker.join();
    for (auto &t : workers)
        t.join();
    for (const auto &[key, chunk] : chunks) {
        releaseGroups(chunk.active);
        releaseGroups(chunk.pending);
    }
}

bool WorldView::prepareWorldChange() {
    std::lock_guard guard(jobsLock);
    if (!worldChangePending) {
        worldChangePending = true;
        jobs.clear();
        scheduledMesh.reset();
        cancelMeshBatch(true);
    }
    return activeJobs == 0 && !meshBuilding;
}

void WorldView::activateWorld(uint32_t seed, glm::dvec3 focus) {
    // prepareWorldChange has stopped jobs until update runs again.
    auto next = std::make_unique<WorldData>(seed);
    next->terrain.swimmableWater = worldData->terrain.swimmableWater;
    next->terrain.prefabs = worldData->terrain.prefabs;
    next->terrain.finishSpawn = worldData->terrain.finishSpawn;
    for (const auto &[key, chunk] : chunks) {
        releaseGroups(chunk.active);
        releaseGroups(chunk.pending);
    }
    for (const auto &[key, tile] : tiles)
        renderer.removeMesh(tile.mesh);
    for (const auto &[key, land] : lands)
        renderer.removeMesh(land.mesh);
    renderer.dropHistory();

    map = {};
    plants.clear();
    exteriorChunks.clear();
    chunks.clear();
    renderSlots.fill({});
    admittedSlots.clear();
    sectorSources = {};
    chunkLightUpdates = {};
    changedLightColumns.clear();
    editedChunks.clear();
    requestedEdits.clear();
    tiles.clear();
    lands.clear();
    discoveredZones.clear();
    discoveredCount = 0;
    tiledZones.clear();
    unreadableTiles.clear();
    visitedLands.clear();
    lastZoneCleanup = {};

    doneZones.clear();
    doneTiles.clear();
    doneChunks.clear();
    meshBatchFinished = false;
    rescanMeshWindow = true;
    meshCache = {};
    meshBatch.reset();
    chunkRequests.clear();
    zonesPending.clear();
    chunksPending.clear();
    chunksStale.clear();
    worldData = std::move(next);
    origin = {floorDiv(int32_t(std::floor(focus.x)), Zone::SIZE) * Zone::SIZE,
              floorDiv(int32_t(std::floor(focus.y)), Zone::SIZE) * Zone::SIZE};
    eye = toRender(focus);
    worldChangePending = false;
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
            ++activeJobs;
        }
        job();
        {
            std::lock_guard guard(jobsLock);
            --activeJobs;
        }
    }
}

bool WorldView::idle() {
    std::lock_guard guard(doneLock);
    return !meshBatch && !rescanMeshWindow && zonesPending.empty() && chunksPending.empty() && doneZones.empty() &&
           doneChunks.empty() && editedChunks.empty();
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
        const auto &terrain = terrainGenerator();
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
    if (!map.zoneAt(x0, y0))
        return;
    const Key key{x0, y0};
    {
        std::lock_guard guard(doneLock);
        const auto [job, inserted] = chunksPending.try_emplace(key, false);
        if (!inserted) {
            if (job->second)
                chunksStale.insert(key);
            return;
        }
    }
    // A snapshot of the zones round it, so the job reads without locks.
    chunkRequests.push_back(key);
}

void WorldView::dispatchChunks() {
    if (meshBatch || (chunkRequests.empty() && !rescanMeshWindow))
        return;
    rescanMeshWindow = false;
    meshBatch = std::make_shared<MeshBatch>();
    meshBatch->requests.swap(chunkRequests);
    meshBatch->terrain = map;
    meshBatch->cache = meshCache;
    meshBatch->lightRequests = chunkLightUpdates;
    meshBatch->renderDistance = meshRenderDistance;
    const auto ordered = chunkPriority.window(windowOrigin);
    meshBatch->window.reserve(ordered.size());
    for (const auto &candidate : ordered) {
        const glm::ivec2 chunk = windowOrigin + candidate.offset;
        const Key corner{chunk.x * CHUNK, chunk.y * CHUNK};
        meshBatch->window.push_back({chunk, requestedEdits.contains(corner) || chunkLightUpdates.dirty(chunk),
                                     chunkMeshSourcesResident(map, chunk)});
    }
    for (const Key &corner : meshBatch->requests) {
        chunksPending.at(corner) = true;
        const int64_t dx = int64_t(corner.first / CHUNK) - windowOrigin.x;
        const int64_t dy = int64_t(corner.second / CHUNK) - windowOrigin.y;
        if (dx < 0 || dx >= ChunkMeshCache::SIDE || dy < 0 || dy >= ChunkMeshCache::SIDE)
            meshBatch->exterior.push_back(corner);
    }
    std::lock_guard guard(jobsLock);
    scheduledMesh = meshBatch;
}

void WorldView::setMeshRenderDistance(int32_t setting) {
    if (setting != meshRenderDistance) {
        meshRenderDistance = setting;
        rescanMeshWindow = true;
        cancelMeshBatch(false);
    }
}

void WorldView::cancelMeshBatch(bool discard) {
    if (meshBatch) {
        rescanMeshWindow = true;
        meshBatch->cancel.store(true);
        meshBatch->discard |= discard;
    }
}

void WorldView::meshWorkerLoop() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    std::shared_ptr<MeshBatch> batch;
    BlockLightUpdate::AreaCopies lightAreas;
    size_t next = 0;
    while (true) {
        {
            std::lock_guard guard(jobsLock);
            if (stopping)
                return;
            if (!batch && scheduledMesh) {
                batch = std::move(scheduledMesh);
                meshBuilding = true;
                next = 0;
            }
        }
        bool built = false;
        if (batch && !batch->cancel.load()) {
            const auto build = [&](glm::ivec2 coordinate, bool native) -> std::optional<int32_t> {
                if (batch->cancel.load())
                    return {};
                uint64_t change = 0;
                if (native) {
                    const auto admitted = batch->lightRequests.beginBuild(batch->terrain, coordinate);
                    if (!admitted)
                        return {};
                    change = *admitted;
                } else if (!chunkMeshSourcesResident(batch->terrain, coordinate)) {
                    return {};
                }
                const glm::ivec2 corner = coordinate * CHUNK;
                auto prepared = prepareChunkLight(batch->terrain, corner.x, corner.y, change != 0);
                if (prepared)
                    prepared->store(batch->terrain, lightAreas);
                const auto around = batch->terrain.neighbourhood(corner.x / Zone::SIZE, corner.y / Zone::SIZE);
                auto meshes = meshChunk(around, terrainGenerator(), corner.x, corner.y, prepared.get());
                const int32_t count = meshes.vertexRecords();
                if (native)
                    batch->lightRequests.publish(coordinate, change, meshes.preparedLighting);
                auto decorations = around.sampleDecorations(coordinate.x, coordinate.y);
                std::lock_guard guard(doneLock);
                doneChunks.push_back({corner, std::move(meshes), std::move(decorations), change, std::move(prepared),
                                      native ? DoneChunk::Operation::Publish : DoneChunk::Operation::Exterior});
                return count;
            };
            const auto reset = [&](glm::ivec2 coordinate) {
                batch->lightRequests.invalidate(coordinate);
                std::lock_guard guard(doneLock);
                doneChunks.push_back({coordinate * CHUNK, {}, {}, 0, {}, DoneChunk::Operation::Reset});
            };
            built = batch->cache.pass(batch->window, batch->renderDistance,
                                      [&](glm::ivec2 at) { return build(at, true); }, reset);
            while (!built && !batch->cancel.load() && next < batch->exterior.size()) {
                const auto [x, y] = batch->exterior[next++];
                built = build({x / CHUNK, y / CHUNK}, false).has_value();
            }
        }
        if (batch && (batch->cancel.load() || !built)) {
            lightAreas.clear();
            batch.reset();
            {
                std::lock_guard guard(doneLock);
                meshBatchFinished = true;
            }
            {
                std::lock_guard guard(jobsLock);
                meshBuilding = false;
            }
        }
        // Work and scheduling time are additional to the requested delay, including on an idle pass.
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void WorldView::addZone(std::shared_ptr<Zone> zone) {
    const int32_t zx = zone->zx, zy = zone->zy;
    cancelMeshBatch(map.zone(zx, zy) != nullptr);
    sectorSources.update(*zone);
    map.add(std::move(zone));
    map.refreshDecorations({zx, zy});
    if (onZoneReady)
        onZoneReady(zx, zy);
    refreshPlantsNear({zx, zy});
    visitZoneMeshChunks({zx, zy}, [&](glm::ivec2 chunk) {
        requestChunk(chunk.x * CHUNK, chunk.y * CHUNK);
    });
}

void WorldView::applyEdits(const std::vector<BlockEdit> &edits, TerrainEditSource source,
                           std::optional<glm::ivec2> savedArea) {
    // Each held zone the records touch is copied, changed and swapped in, so the mesh jobs
    // still reading the old one are not disturbed.
    const bool saved = source == TerrainEditSource::Saved;
    std::map<Key, std::vector<const BlockEdit *>> byZone;
    for (const BlockEdit &e : edits) {
        if (map.zoneAt(e.x, e.y)) {
            byZone[{floorDiv(e.x, Zone::SIZE), floorDiv(e.y, Zone::SIZE)}].push_back(&e);
            if (!saved)
                changedLightColumns.emplace_back(e.x, e.y);
        }
    }
    if (saved && savedArea && map.zone(savedArea->x, savedArea->y))
        byZone.try_emplace(Key{savedArea->x, savedArea->y}); // every serialized row may already have expired
    EditedZones replacements;
    for (const auto &[key, list] : byZone) {
        auto zone = std::make_shared<Zone>(*map.zone(key.first, key.second));
        const GapFill rock = [&](int32_t x, int32_t y, int32_t z) { return terrainGenerator().gapBlock(x, y, z, *zone); };
        for (const BlockEdit *e : list) {
            if (saved)
                applySaved(*zone, *e, rock);
            else
                applyLive(*zone, *e, rock);
            remeshColumn(editedChunks, e->x, e->y);
        }
        replacements.emplace(key, std::move(zone));
    }
    if (saved) {
        // A positive stored edit count belongs to one newly restored area.
        for (const auto &[key, records] : byZone)
            pruneUnsupportedDecorations(map, replacements, key, key);
    } else {
        // All received coordinates contribute bounds, including records for missing columns.
        // Surviving active render copies change only through the ordinary mesh transfer.
        pruneAfterLiveEdits(map, replacements, edits);
    }
    // A cleanup-only neighbor has no changed terrain to relight.
    if (saved || source == TerrainEditSource::Live)
        for (const auto &[key, list] : byZone)
            terrainGenerator().relight(*replacements.at(key));
    publishEditedZones(std::move(replacements), false, saved);
}

void WorldView::relightAfterDestruction(const std::array<int64_t, 3> &centre, float radius) {
    cancelMeshBatch(true);
    worldgen::relightAfterDestruction(map, centre, radius);
}

void WorldView::publishEditedZones(std::map<Key, std::shared_ptr<Zone>> edits, bool relightAreas, bool fromSave) {
    if (!edits.empty())
        cancelMeshBatch(true);
    for (auto &[key, zone] : edits) {
        if (relightAreas)
            terrainGenerator().relight(*zone);
        if (fromSave)
            zone->initializeDecorationLight();
        sectorSources.update(*zone);
        map.replaceTerrain(std::move(zone));
        map.refreshDecorations(key, !fromSave);
    }
    for (const auto &[key, zone] : edits)
        refreshPlantsNear(key);
}

std::vector<BlockEdit> WorldView::placeQueuedBlueprint(int8_t number, int8_t rotation, const int64_t position[3],
                                                       glm::vec3 aim, int32_t day) {
    const char *key = blueprintKey(number);
    if (!key)
        return {};
    auto model = blueprintModels.find(number);
    if (model == blueprintModels.end()) {
        const auto source = prefab(key);
        if (!source || source->sx <= 0 || source->sy <= 0 || source->sz <= 0)
            return {};
        CubModel decoded;
        decoded.sx = source->sx;
        decoded.sy = source->sy;
        decoded.sz = source->sz;
        decoded.rgb = source->rgb;
        model = blueprintModels.emplace(number, std::move(decoded)).first;
    }
    const auto base = placementBase(position, aim);
    auto placed = stageBlueprint(map, model->second, base, rotation, day, [&](int32_t x, int32_t y, int32_t z) {
        if (const auto *zone = map.zoneAt(x, y))
            return terrainGenerator().gapBlock(x, y, z, *zone);
        return IMPLICIT_SOLID;
    });
    for (const auto &[x, y] : placed.changedColumns)
        remeshColumn(editedChunks, x, y);
    publishEditedZones(std::move(placed.zones));
    map.appendPlacedObject({base[0] / Zone::SIZE, base[1] / Zone::SIZE}, {number, base[0], base[1], base[2]});
    return std::move(placed.records);
}

void WorldView::releaseGroups(TerrainMeshBuffers groups) {
    renderer.removeMesh(groups.solid);
    renderer.removeMesh(groups.water);
}

void WorldView::removeChunk(Key corner) {
    const auto it = chunks.find(corner);
    const glm::ivec2 at(corner.first / CHUNK, corner.second / CHUNK);
    meshCache.invalidate(at);
    chunkLightUpdates.invalidate(at);
    if (it == chunks.end())
        return;
    auto &physical = renderSlots[ChunkMeshCache::index(at)].resident;
    if (physical == &it->second)
        physical = nullptr;
    releaseGroups(it->second.active);
    releaseGroups(it->second.pending);
    exteriorChunks.erase(corner);
    chunks.erase(it);
}

void WorldView::dropZone(Key key) {
    cancelMeshBatch(true);
    rescanMeshWindow = true;
    map.remove(key.first, key.second);
    plants.erase(key);
    refreshPlantsNear(key);
    sectorSources.remove(key.first, key.second);
    const int32_t x0 = key.first * Zone::SIZE, y0 = key.second * Zone::SIZE;
    std::erase_if(requestedEdits, [&](const Key &corner) {
        return corner.first / Zone::SIZE == key.first && corner.second / Zone::SIZE == key.second;
    });
    for (auto it = chunks.lower_bound({x0, INT32_MIN}); it != chunks.end() && it->first.first < x0 + Zone::SIZE;) {
        const auto corner = (it++)->first;
        if (corner.second >= y0 && corner.second < y0 + Zone::SIZE &&
            !meshCache.owns({corner.first / CHUNK, corner.second / CHUNK}))
            removeChunk(corner);
    }
}

void WorldView::placeChunk(DoneChunk &chunk) {
    const glm::ivec2 corner = chunk.corner;
    ChunkMeshes &meshes = chunk.meshes;
    const glm::ivec2 coordinate = corner / CHUNK;
    if (!chunkMeshSourcesResident(map, coordinate))
        return; // A source was evicted after the geometry worker took its snapshot.
    requestedEdits.erase({corner.x, corner.y});
    if (chunk.operation == DoneChunk::Operation::Exterior && meshCache.owns(coordinate))
        return; // A wider-radius completion cannot replace a native slot's active or pending lists.
    if (!map.publishDecorations(coordinate.x, coordinate.y, chunk.decorations))
        return;
    auto &entry = chunks[{corner.x, corner.y}];
    const TerrainMeshBuffers incoming{meshes.solid.vertices.empty() ? 0 : renderer.addMesh(meshes.solid),
                                     meshes.water.vertices.empty() ? 0 : renderer.addMesh(meshes.water)};
    if (chunk.operation == DoneChunk::Operation::Publish) {
        exteriorChunks.erase({corner.x, corner.y});
        entry.bounds = terrain_models::SlotBox(corner.x, corner.y, meshes.slotLow, meshes.slotHigh);
        if (!meshCache.owns(coordinate)) {
            releaseGroups(std::exchange(entry.active, {})); // an exterior mesh can predate its native slot
            entry.activeDecorations.reset();
            entry.models.clear();
        }
        releaseGroups(entry.publish(incoming, std::move(chunk.decorations.sampled)));
        renderSlots[ChunkMeshCache::index(coordinate)].resident = &entry;
        meshCache.publish(coordinate, meshes.vertexRecords());
        chunkLightUpdates.publish(coordinate, chunk.lightChange, meshes.preparedLighting);
    } else {
        // The wider streaming radius draws exterior geometry without owning a native fog slot.
        exteriorChunks[{corner.x, corner.y}] = &entry;
        releaseGroups(entry.publish({}));
        releaseGroups(std::exchange(entry.active, incoming));
        entry.activeDecorations = incoming.populated() ? std::move(chunk.decorations.sampled) : nullptr;
        prepareDecorationModels(entry);
    }
}

void WorldView::update(std::optional<terrain_lights::FixedPosition> adjustedEye) {
    if (worldChangePending)
        return;
    // Mark the cache coordinates that were already published during the frame.
    // An in-flight mesh keeps its snapshot and is requested again after completion.
    const bool marked = changedLightColumns.empty() || chunkLightUpdates.markChanged(changedLightColumns, timeGetTime());
    if (marked) {
        // The followed eye retains its fixed precision after camera collision correction.
        // Free camera, scripted travel and constructor preloading supply the engine eye.
        if (!adjustedEye)
            adjustedEye = terrain_lights::fromBlocks(worldPosition());
        if (adjustedEye) {
            const auto nextOrigin = chunkMeshWindowOrigin(*adjustedEye, alpha::FOG_WINDOW_CHUNKS);
            if (nextOrigin != windowOrigin) {
                rescanMeshWindow = true;
                cancelMeshBatch(false);
            }
            windowOrigin = nextOrigin;
        }
    }
    changedLightColumns.clear();
    for (const Key &corner : editedChunks)
        if (map.zoneAt(corner.first, corner.second)) {
            requestedEdits.insert(corner);
            requestChunk(corner.first, corner.second);
        }
    editedChunks.clear();

    // Take in finished work.
    std::vector<std::shared_ptr<Zone>> zones;
    std::vector<DoneChunk> meshed;
    std::vector<DoneTile> newTiles;
    bool finished;
    {
        std::lock_guard guard(doneLock);
        zones.swap(doneZones);
        newTiles.swap(doneTiles);
        meshed.swap(doneChunks);
        finished = std::exchange(meshBatchFinished, false);
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
    const auto release = [&](Key key) {
        chunksPending.erase(key);
        return chunksStale.erase(key) != 0;
    };
    for (auto &chunk : meshed) {
        const auto corner = chunk.corner;
        const Key key{corner.x, corner.y};
        if (chunk.operation == DoneChunk::Operation::Reset) {
            if (!meshBatch->discard) {
                removeChunk(key);
                // Wider engine radii can request the old physical slot's coordinate
                // independently after it has left the native window.
                const auto at = corner / CHUNK;
                const int64_t dx = int64_t(at.x) - windowOrigin.x, dy = int64_t(at.y) - windowOrigin.y;
                if ((dx < 0 || dx >= ChunkMeshCache::SIDE || dy < 0 || dy >= ChunkMeshCache::SIDE) &&
                    !chunksPending.contains(key))
                    again.push_back(key);
            }
            continue;
        }
        if (const auto it = chunksPending.find(key); it != chunksPending.end() && it->second && release(key))
            cancelMeshBatch(true);
        if (meshBatch->discard) {
            // Later builds in the same snapshot can depend on this chunk's lighting.
            again.push_back(key);
            continue;
        }
        if (chunk.prepared)
            chunk.prepared->store(map, meshBatch->publishedLightAreas);
        placeChunk(chunk);
    }
    if (finished) {
        for (const Key &key : meshBatch->requests)
            if (const auto it = chunksPending.find(key); it != chunksPending.end() && it->second) {
                const bool stale = release(key);
                if (stale || meshBatch->cancel.load())
                    again.push_back(key);
            }
        meshBatch.reset();
    }
    for (const Key &k : again)
        requestChunk(k.first, k.second);
    const Key here = cameraZone();
    for (auto &z : zones)
        if (std::abs(z->zx - here.first) <= radius + 1 && std::abs(z->zy - here.second) <= radius + 1)
            addZone(std::move(z));
    dispatchChunks();

    // Ask for the zones round the camera, nearest first, and let go of far ones.
    std::vector<Key> wanted;
    for (int32_t i = -radius; i <= radius; i++)
        for (int32_t j = -radius; j <= radius; j++)
            if (const Key key{here.first + i, here.second + j};
                uint32_t(key.first) < 65536u && uint32_t(key.second) < 65536u && !map.zone(key.first, key.second))
                wanted.push_back(key);
    std::sort(wanted.begin(), wanted.end(), [&](const Key &a, const Key &b) {
        auto d = [&](const Key &k) { return std::abs(k.first - here.first) + std::abs(k.second - here.second); };
        return d(a) < d(b);
    });
    for (const Key &k : wanted)
        requestZone(k);
    // Extractor §B5.2: clean-up passes run when more than 1000 ms have passed, and keep the
    // disc of squared zone distance < 16 round the interest point (here the camera's zone).
    // Engine adapter: zones inside the render-distance window are kept as well, so a render
    // distance beyond the native disc does not drop and request the same zones again. A world
    // change makes a new WorldView, which unloads every zone.
    const auto now = std::chrono::steady_clock::now();
    if (now - lastZoneCleanup <= std::chrono::milliseconds(ZONE_CLEANUP_INTERVAL_MS))
        return;
    lastZoneCleanup = now;
    std::vector<Key> distantZones;
    for (const auto &[key, zone] : map.zones())
        if (!zoneKeptNear(key.first, key.second, here.first, here.second) &&
            (std::abs(key.first - here.first) > radius + 1 || std::abs(key.second - here.second) > radius + 1))
            distantZones.push_back(key);
    for (const Key &k : distantZones)
        dropZone(k);
}

WorldView::PlantModel WorldView::plantModel(int32_t kind) {
    if (const auto it = plantModels.find(kind); it != plantModels.end())
        return it->second;
    PlantModel entry;
    if (const char *name = plantModelName(kind))
        if (const auto bytes = content.read(name)) {
            CubModel model;
            if (model.load(bytes->data(), bytes->size()))
                entry = {renderer.addMesh(model.mesh()), model.sx, model.sz};
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
                entry = {renderer.addMesh(kind == 10 || kind == 11 ? model.cornerMesh() : model.mesh()),
                         glm::vec3(model.sx, model.sy, model.sz)};
        }
    staticModels[kind] = entry;
    return entry;
}

MeshId WorldView::airshipMesh() {
    if (airshipModelTried)
        return airshipModelMesh;
    airshipModelTried = true;
    // World objects §5: the model's pure red voxels are emptied when it loads.
    if (const auto prefab = terrainGenerator().prefabs("airship.cub")) {
        CubModel model;
        model.sx = prefab->sx, model.sy = prefab->sy, model.sz = prefab->sz;
        model.rgb = prefab->rgb;
        for (size_t i = 0; i + 2 < model.rgb.size(); i += 3)
            if (model.rgb[i] == 255 && model.rgb[i + 1] == 0 && model.rgb[i + 2] == 0)
                model.rgb[i] = 0;
        airshipModelMesh = renderer.addMesh(model.mesh());
    }
    return airshipModelMesh;
}

std::vector<WorldView::PlantInstance> WorldView::plantsOf(const Zone &zone) {
    std::vector<PlantInstance> out;
    const auto addPlant = [&](const Plant &p, size_t phase) {
        const auto model = plantModel(p.kind);
        if (!model.mesh)
            return;
        // Centred on the block, turned, swayed, scaled, placed (open points 12 §2,
        // vegetation-wind.md §4). The phase only needs to stay the same for a plant.
        PlantInstance instance;
        instance.mesh = model.mesh;
        instance.placed = glm::scale(glm::translate(glm::mat4(1), glm::vec3(p.x - origin.x + 0.5f, p.y - origin.y + 0.5f, p.z - (p.sink ? 0.1f : 0.f))),
                                     glm::vec3(p.scale));
        instance.yaw = glm::rotate(glm::mat4(1), glm::radians(p.rotation), glm::vec3(0, 0, 1));
        instance.scale = p.scale;
        instance.height = float(model.height);
        instance.phase = (int)phase + 1;
        instance.sways = (p.flags & motion::SWAYS) != 0;
        out.push_back(instance);
    };
    const bool bankWater = terrainGenerator().swimmableWater;
    if (bankWater) {
        const auto reeds = bankReeds(map, zone, terrainWorld().seed());
        for (const Plant &reed : reeds) {
            // Neighbor admission can add earlier reeds to the cache. A phase
            // derived from the place keeps existing bank plants from jumping.
            const uint32_t phase = ((uint32_t)reed.x ^ ((uint32_t)reed.y * 65537u)) & 0x3fffffffu;
            addPlant(reed, phase);
        }
    }
    return out;
}

void WorldView::prepareDecorationModels(ChunkEntry &entry) {
    entry.models.clear();
    if (!entry.activeDecorations)
        return;
    const bool bankWater = terrainGenerator().swimmableWater;
    int visit = 0;
    for (const auto &record : *entry.activeDecorations) {
        ++visit;
        const auto &s = record.sprite;
        if (bankWater && record.plantIndex >= 0 && s.kind == 22)
            continue; // the owner water mode supplies reeds independently on the bank
        const auto model = plantModel(s.kind);
        if (!model.mesh)
            continue;
        const auto bound = terrain_models::decorationBound({s.x, s.y, s.z}, model.width, model.height, s.size);
        if (!bound)
            continue;
        PlantInstance instance;
        instance.mesh = model.mesh;
        instance.bound = *bound;
        const auto at = toRender(glm::dvec3(double(s.x), double(s.y), double(s.z)) / 65536.0);
        instance.placed = glm::scale(glm::translate(glm::mat4(1), at), glm::vec3(s.size));
        instance.yaw = glm::rotate(glm::mat4(1), glm::radians(s.angle), glm::vec3(0, 0, 1));
        instance.scale = s.size;
        instance.height = float(model.height);
        instance.phase = visit;
        instance.sways = (s.flags & motion::SWAYS) != 0;
        instance.light = record.light;
        instance.emits = (s.flags & 1) != 0;
        entry.models.push_back(instance);
    }
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

void WorldView::markDiscovered(int32_t zx, int32_t zy) {
    if (uint32_t(zx) < 65536 && uint32_t(zy) < 65536 && discoveredZones.insert({zx, zy}).second)
        ++discoveredCount;
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
            markDiscovered(floorDiv((int32_t)std::floor(at.x + dx), Zone::SIZE),
                           floorDiv((int32_t)std::floor(at.y + dy), Zone::SIZE));
    // The land stood in is visited; its picture is rebuilt at most every 2 s (new zones show).
    const Key land = landOf((int32_t)std::floor(at.x), (int32_t)std::floor(at.y));
    visitedLands.insert(land);
    // (Here only when something new is known: the scan covers some 37,000 zones.)
    LandPicture &picture = lands[land];
    if (nowSeconds() - picture.builtAt > 2 && picture.knownZones != discoveredZones.size() + tiledZones.size())
        buildLand(land);
}

WorldView::Key WorldView::landOf(int32_t x, int32_t y) const {
    const auto &world = terrainWorld();
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
    const auto &world = terrainWorld();
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
                const worldgen::RegionCell *cell = terrainWorld().owner(zx * Zone::SIZE, zy * Zone::SIZE);
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
    // §5.1.2: the land-border markers, scaled by 5. Re-certified correction 35: the marker tile
    // is allocated 4 x 4 x 4 and only its bottom layer is filled (a white 2 x 2 centre inside a
    // (10, 10, 10) border); the three layers above stay empty.
    if (!borderMarker) {
        CubModel marker;
        marker.sx = marker.sy = marker.sz = 4;
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++) {
                const uint8_t v = (x == 1 || x == 2) && (y == 1 || y == 2) ? 255 : 10;
                marker.rgb.insert(marker.rgb.end(), {v, v, v});
            }
        marker.rgb.resize((size_t)4 * 4 * 4 * 3, 0); // layers 1..3: empty voxels
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

void WorldView::appendModel(std::vector<DrawItem> &immediate, const terrain_models::Distance &distance,
                            std::span<DrawItem> parts) {
    auto &draws = distance.deferred ? deferredDraws : immediate;
    if (distance.deferred)
        deferredModels.push_back({distance.squared, draws.size(), parts.size()});
    for (auto &part : parts) {
        part.viewAdmitted = true;
        part.layer = distance.deferred ? DrawLayer::Deferred : DrawLayer::Opaque;
        part.alpha = distance.opacity;
        draws.push_back(std::move(part));
    }
}

void WorldView::appendStatics(std::vector<DrawItem> &out, const terrain_models::ModelView &view,
                              const terrain_lights::SectorRange &range, const StaticStates *states) {
    for (const auto &[key, zone] : map.zones()) {
        if (!range.contains(key.first, key.second))
            continue;
        const auto *areaStates = states ? states->zone(key.first, key.second) : nullptr;
        for (size_t i = 0; i < zone->statics.size(); ++i) {
            const auto &o = zone->statics[i];
            const terrain_lights::FixedPosition base{o.x, o.y, o.z};
            const auto bound = terrain_models::staticBound(base, {o.scale[0], o.scale[1], o.scale[2]});
            if (!bound)
                continue;
            const auto distance = view.classify(*bound, base);
            if (!distance)
                continue;
            const auto [mesh, voxels] = staticModel(o.kind);
            if (!mesh)
                continue;
            const auto anchor = toRenderFixed(base);
            const auto *state = areaStates && i < areaStates->size() ? &(*areaStates)[i] : nullptr;
            if (o.kind == 10) {
                const ChestPose pose(anchor, o.scale[0], o.rotation);
                std::array<DrawItem, 2> parts{{{mesh, pose.base(voxels), true}}};
                size_t count = 1;
                if (const auto [lid, lidGrid] = staticModel(11); lid) {
                    const float fraction = state ? openFraction(10, *state) : openFraction(10, o.state, o.timer);
                    parts[count++] = {lid, pose.lid(lidGrid, fraction), true};
                }
                appendModel(out, *distance, std::span(parts).first(count));
                continue;
            }
            const glm::vec3 size(o.scale[0], o.scale[1], o.scale[2]);
            const glm::mat4 turn = glm::rotate(glm::mat4(1), glm::radians(90.f * float(o.rotation)), glm::vec3(0,0,1));
            const glm::mat4 place = glm::translate(glm::mat4(1), anchor) * turn;
            glm::mat4 fit = glm::scale(glm::mat4(1), glm::vec3(size.x / voxels.x));
            // A standalone slot-11 record uses the ordinary centred placement for this corner mesh.
            if (o.kind == 11)
                fit = glm::translate(fit, {-voxels.x * .5f, -voxels.y * .5f, 0});
            const float p = state ? openFraction(o.kind, *state) : 0.f;
            glm::mat4 part(1);
            switch (o.kind) {
            case 1: case 2: case 3:
                part = glm::translate(glm::mat4(1), glm::vec3(-size.x / 2,0,0)) *
                       glm::rotate(glm::mat4(1), glm::radians(90.f * p), glm::vec3(0,0,1)) *
                       glm::translate(glm::mat4(1), glm::vec3(size.x / 2,0,0));
                break;
            case 5: part = glm::translate(glm::mat4(1), glm::vec3(0,0,.8f * size.z * p)); break;
            case 7: part = glm::translate(glm::mat4(1), glm::vec3(0,0,-size.z * p - 1)); break;
            case 8: part = glm::translate(glm::mat4(1), glm::vec3(0,0,size.z * p)); break;
            case 9: part = glm::rotate(glm::mat4(1), glm::radians(60.f * p - 30.f), glm::vec3(1,0,0)); break;
            }
            DrawItem draw{mesh, place * part * fit, true};
            appendModel(out, *distance, std::span(&draw, 1));
        }
    }
}

std::vector<DrawItem> WorldView::items(double clockMs, const Scene &scene, const StaticStates *staticStates,
                                       std::optional<terrain_lights::FixedPosition> cameraFixed) {
    std::vector<DrawItem> out;
    out.reserve(chunks.size() * 2 + 4096);
    const float sunlight = alpha::sunlight(scene.timeOfDayMs);
    const glm::vec4 decorationTint = scene.underwater ? glm::vec4(waterlook::underwaterMaterial, sunlight)
                                                      : alpha::material(sunlight, false);
    if (!cameraFixed)
        cameraFixed = terrain_lights::fromBlocks(worldPosition());
    const terrain_lights::FixedPosition shift{-int64_t(origin.x) * 65536, -int64_t(origin.y) * 65536, 0};
    const auto planes = camera(renderer.size()).lightAdmissionPlanes();
    const float outer = alpha::RENDER_RADIUS * .2f;
    const float sceneRange = scene.fogDistance + 22.4f;
    std::optional<terrain_models::ModelView> modelView;
    if (cameraFixed)
        modelView.emplace(*cameraFixed, shift, planes, outer);
    admittedSlots.clear();
    deferredDraws.clear();
    deferredModels.clear();
    visitChunkRenderWindow(windowOrigin, [&](glm::ivec2 coordinate) {
        auto &slot = renderSlots[ChunkMeshCache::index(coordinate)];
        auto *entry = slot.resident;
        if (!entry)
            return;
        // Pending groups transfer before the logical tag and box tests, even on a physical alias.
        if (entry->pending.populated() && !chunkLightUpdates.transferBlocked(coordinate)) {
            releaseGroups(entry->renderVisit(false));
            prepareDecorationModels(*entry);
        }
        if (cameraFixed && slot.admit(meshCache.owns(coordinate), *cameraFixed, shift, storedLightPlanes, sceneRange + 32.f))
            admittedSlots.push_back(&slot);
    });
    terrain_models::orderSlots(admittedSlots);
    const auto terrainDraw = [&](MeshId mesh, glm::ivec2 corner, bool water, bool admitted) {
        if (!mesh)
            return;
        const auto place = glm::translate(glm::mat4(1), glm::vec3(corner - origin, 0));
        out.push_back({mesh, place, !water, water ? Shading::Water : Shading::World});
        out.back().viewAdmitted = admitted;
    };
    // Wider-radius engine geometry has no native slot in these passes.
    for (const auto &[corner, entry] : exteriorChunks) {
        const glm::ivec2 at(corner.first, corner.second);
        terrainDraw(entry->active.solid, at, false, false);
        terrainDraw(entry->active.water, at, true, false);
    }
    const auto cornerOf = [](const ChunkEntry &entry) {
        return glm::ivec2(entry.bounds.low[0] / 65536, entry.bounds.low[1] / 65536);
    };
    // A slot's immediate models follow its own first terrain resource before the next slot.
    for (const auto *slot : admittedSlots) {
        const auto *entry = slot->resident;
        terrainDraw(entry->active.solid, cornerOf(*entry), false, true);
        if (entry->models.empty() || !terrain_models::withinDecorationRange(entry->bounds, *cameraFixed, outer + 32.f))
            continue;
        for (const auto &p : entry->models) {
            const auto distance = modelView->classify(p.bound);
            if (!distance)
                continue;
            const glm::mat4 sway = p.sways ? motion::plantShear(clockMs, p.phase, p.height, p.scale) : glm::mat4(1);
            DrawItem draw{p.mesh, p.placed * sway * p.yaw, true};
            draw.material = terrain_lights::decorationMaterial(decorationTint, p.light, p.emits);
            draw.decorationMaterial = true;
            appendModel(out, *distance, std::span(&draw, 1));
        }
    }
    for (const auto &[key, list] : plants)
        for (const PlantInstance &p : list) {
            const glm::mat4 sway = p.sways ? motion::plantShear(clockMs, p.phase, p.height, p.scale) : glm::mat4(1);
            out.push_back({p.mesh, p.placed * sway * p.yaw, true});
        }
    if (cameraFixed) {
        const terrain_models::ModelView staticView(*cameraFixed, shift, planes, sceneRange * .3f);
        // The original range uses the selected actor's whole columns. Free camera uses the eye.
        const auto controlled = playerColumn ? terrain_lights::FixedPosition{
            int64_t(playerColumn->x) * 65536, int64_t(playerColumn->y) * 65536, 0} : *cameraFixed;
        appendStatics(out, staticView, terrain_lights::sectorRange(controlled, sceneRange), staticStates);
    }
    // Walk exactly the same membership in reverse for the second terrain resources, including ties.
    for (auto it = admittedSlots.rbegin(); it != admittedSlots.rend(); ++it) {
        const auto &entry = *(*it)->resident;
        terrainDraw(entry.active.water, cornerOf(entry), true, true);
    }
    terrain_models::orderDeferred(deferredModels);
    for (const auto &model : deferredModels)
        for (size_t i = model.first; i < model.first + model.count; ++i)
            out.push_back(std::move(deferredDraws[i]));
    // Clouds (clouds spec §3 step 7, rendering §11.4): centred (the mesh already is), turned,
    // scaled, placed; then AlphaVulkan's drift. Gated at the local player's column, gridded
    // round the camera's.
    if (cloudMesh) {
        const int32_t cx = origin.x + (int32_t)std::floor(eye.x), cy = origin.y + (int32_t)std::floor(eye.y);
        const glm::ivec2 gate = playerColumn.value_or(glm::ivec2(cx, cy));
        const glm::vec2 drift = motionSettings.clouds
                                    ? motion::cloudDrift(clockMs / 1000.0, motionSettings.cloudDistance, motionSettings.cloudSpeed)
                                    : glm::vec2(0);
        for (const auto &c : worldData->clouds.place(gate.x, gate.y, cx, cy)) {
            const glm::vec3 at((float)(c.x - origin.x) + drift.x, (float)(c.y - origin.y) + drift.y, (float)c.z);
            // Rulings §6: the quarter turn is degrees x 0.01745329 in binary32, with binary64
            // cosine and sine rounded to binary32; a vertex maps (x, y) -> (xc - ys, xs + yc).
            const float angle = c.rotation * 0.01745329f;
            const float cs = (float)std::cos((double)angle), sn = (float)std::sin((double)angle);
            glm::mat4 m(0);
            m[0] = glm::vec4(cs * c.scale, sn * c.scale, 0, 0);
            m[1] = glm::vec4(-sn * c.scale, cs * c.scale, 0, 0);
            m[2] = glm::vec4(0, 0, c.scale, 0);
            m[3] = glm::vec4(at, 1);
            out.push_back({cloudMesh, m, true, Shading::Cloud});
        }
    }
    return out;
}

float WorldView::fogTarget(std::optional<terrain_lights::FixedPosition> reference) const {
    if (!reference)
        reference = terrain_lights::fromBlocks(worldPosition());
    if (!reference)
        return alpha::RENDER_RADIUS - 22.4f;
    return alpha::fogTarget(*reference, windowOrigin, [&](int32_t i, int32_t j) {
        const glm::ivec2 at{i, j};
        const auto *slot = renderSlots[ChunkMeshCache::index(at)].resident;
        return slot && slot->active.populated() && meshCache.owns(at);
    });
}

Camera WorldView::camera(VkExtent2D size) const {
    Camera c;
    c.eye = eye;
    c.renderOrigin = origin;
    c.yawDegrees = yaw;
    c.pitchDegrees = pitch;
    c.aspect = (float)size.width / (float)std::max(1u, size.height);
    if (lensWord && size.width > 0 && size.height > 0)
        c.mainLens = Camera::MainLens{*lensWord, int32_t(size.width), int32_t(size.height)};
    c.nativeView = nativeView;
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

void WorldView::placeCamera(glm::dvec3 at, float yawDegrees, float pitchDegrees) {
    eye = toRender(at);
    yaw = yawDegrees;
    pitch = pitchDegrees;
}

void WorldView::setLook(float y, float p) {
    yaw = y;
    pitch = std::clamp(p, -89.f, 89.f);
}

void WorldView::fillScene(Scene &scene) const {
    scene.zone = {};
    scene.underwater = false;
    if (const auto reference = terrain_lights::fromBlocks(worldPosition())) {
        const auto &[rawX, rawY, rawZ] = *reference;
        // The regional sampler truncates the fixed camera coordinates toward zero.
        const int32_t x = worldgen::scalars::queryCoordinate(rawX);
        const int32_t y = worldgen::scalars::queryCoordinate(rawY);
        const auto &world = terrainWorld();
        const float warmth = world.warmth(x, y);
        const float moisture = world.moisture(x, y);
        const float sea = warmth > .3f && moisture > .6f ? world.seaShare(x, y) : 0.f;
        scene.zone = {warmth, moisture, sea, world.fireShare(x, y)};

        // The separate type-two query uses the biased point-cell coordinates.
        const int32_t cellX = blockOfFixed(rawX);
        const int32_t cellY = blockOfFixed(rawY);
        const int32_t cellZ = blockOfFixed(rawZ);
        scene.underwater = uint32_t(cellX) < 0x1000000u && uint32_t(cellY) < 0x1000000u &&
                           map.read(cellX, cellY, cellZ).type() == WATER;
    }
    scene.focus = glm::vec3(eye.x, eye.y, std::max(eye.z - 40.f, 0.f));
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
    visitChunkRenderWindow(windowOrigin, [&](glm::ivec2 coordinate) {
        const auto *entry = renderSlots[ChunkMeshCache::index(coordinate)].resident;
        if (entry && entry->activeDecorations)
            terrain_lights::appendDecorationLights(lightCandidates, *entry->activeDecorations,
                                                  earlyView, sourceClockMs, daylight);
    });
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
    const terrain_lights::ViewAdmission itemView(*reference, shift, storedLightPlanes,
                                                 terrain_lights::itemLightReach(alpha::RENDER_RADIUS));
    // A flying viewer has no controlled creature; use its camera as an explicit
    // engine fallback. Playing uses the controlled body's main fixed position.
    sectorSources.append(lightCandidates, items, view, controlled.value_or(*reference), daylight, &itemView);
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
        if (entry.active.populated())
            lightChunks.push_back({{corner.first / 32, corner.second / 32}, entry.active.solid, entry.active.water});
    renderer.setTerrainLights(lightSelector.rebuild(lightCandidates, lightChunks, shift));
}

bool WorldView::lightViewAccepts(glm::dvec3 at, float cullingRadius, float fogDistance) const {
    const auto view = viewAdmission(fogDistance + 22.4f);
    const auto position = terrain_lights::fromBlocks(at);
    return view && position && view->accepts(*position, cullingRadius);
}

std::optional<terrain_lights::ViewAdmission> WorldView::viewAdmission(float reach) const {
    const auto reference = terrain_lights::fromBlocks(worldPosition());
    if (!reference)
        return std::nullopt;
    const terrain_lights::FixedPosition shift{-int64_t(origin.x) * 65536, -int64_t(origin.y) * 65536, 0};
    return terrain_lights::ViewAdmission(*reference, shift, storedLightPlanes, reach);
}

std::string WorldView::status() const {
    size_t plantCount = 0;
    for (const auto &[key, list] : plants)
        plantCount += list.size();
    for (const auto &[key, entry] : chunks)
        plantCount += entry.models.size();
    char text[160];
    std::snprintf(text, sizeof text, "%zu zones, %zu chunks, %zu plants, ready in %.1f s", map.zones().size(),
                  chunks.size(), plantCount, nowSeconds() - startSeconds);
    return text;
}
