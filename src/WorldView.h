#pragma once
// Walking round a generated Alpha world. Zones round the camera are generated and meshed by
// background workers as it moves (each zone has its own random stream, so the order does not
// matter), handed to the renderer when ready, and let go when far behind. A free-flying camera
// looks at it.
//
// Positions: the world is 2^24 blocks wide, more than a float holds to the block, so meshes and
// the camera are kept relative to a render origin near the start point.
#include "BlockEdits.h"
#include "ChunkMesher.h"
#include "CubModel.h"
#include "worldgen/MapTile.h"
#include "Frame.h"
#include "Motion.h"
#include "Renderer.h"
#include "worldgen/Clouds.h"
#include "worldgen/Terrain.h"
#include "worldgen/WorldMap.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

class Content;

struct WorldViewOptions {
    uint32_t seed = 123;          // Alpha's default world seed (random spec §1.3)
    int32_t x = worldgen::HOME;   // start column
    int32_t y = worldgen::HOME;
    int32_t radius = 2;           // zones kept round the camera's zone in each direction
    float z = -1e9f;              // start height; below -1e8: 40 blocks over the ground
    motion::MotionSettings motion;
    bool swimmableWater = true;
};

class WorldView {
  public:
    using TileReady = std::function<void(int32_t zx, int32_t zy, const worldgen::MapTile &tile)>;
    // Generates and meshes the zones round the start before returning.
    // Install persistence before the constructor can finish the initial map tiles.
    WorldView(Renderer &renderer, const Content &content, const WorldViewOptions &options, TileReady tileReady = {});
    ~WorldView();

    // Once a frame: asks for the zones the camera needs, takes in finished work, lets go of far
    // zones.
    void update();
    // True while zones or chunks are still being made.
    bool busy() { return !idle(); }
    // Moves the camera by `delta` blocks (for scripted runs).
    void shift(glm::vec3 delta) { eye += delta; }

    // The camera: where it is (render-origin relative) and where it looks.
    Camera camera(VkExtent2D size) const;
    void move(glm::vec3 local, float seconds, bool fast); // x right, y forward, z up
    void turn(float yawDegrees, float pitchDegrees);
    void setLook(float yawDegrees, float pitchDegrees);
    // Puts the camera at `at` (world blocks) looking along `direction`.
    void placeCamera(glm::dvec3 at, glm::vec3 direction);
    // The climate under the camera (for the sky palette) and the shadow focus.
    // and this frame's point lights (rendering §10).
    void fillScene(Scene &scene) const;
    // After dynamic sources have joined the scene, sort the common candidates
    // and rebuild each terrain chunk's resident light records.
    void finishLighting(Scene &scene) const;
    // The camera's place in world blocks, and a world position in render-origin terms.
    glm::dvec3 worldPosition() const { return glm::dvec3(origin.x + (double)eye.x, origin.y + (double)eye.y, eye.z); }
    glm::vec3 toRender(glm::dvec3 at) const {
        return glm::vec3((float)(at.x - origin.x), (float)(at.y - origin.y), (float)at.z);
    }
    // This frame's draw list. `clockMs` is the world's millisecond clock, which sways the
    // plants that sway (vegetation-wind.md §4).
    std::vector<DrawItem> items(double clockMs);
    // Screens §5.1.2, §5.2.4: the map tiles of the zones (zx0..zx1, zy0..zy1) in render-origin
    // blocks, each rising into place over its first 250 ms (`dtMs` advances that), with the
    // land-border markers. With `extras` (the world map) undiscovered tiles are dimmed and
    // zones without a tile get a blue placeholder block.
    std::vector<DrawItem> mapItems(int32_t zx0, int32_t zy0, int32_t zx1, int32_t zy1, float dtMs, bool extras);
    // §5.1.6: marks the zones under the 128 x 128 blocks round a point as discovered, and the
    // land (region cell) the point is in as visited.
    void discover(glm::dvec3 at);
    // §5.1.2, §5.1.6: the land pictures of the visited lands within `radius` lands of the one
    // holding `pivot`, tinted blue, as a slab under the pivot's level.
    std::vector<DrawItem> landItems(glm::dvec3 pivot, int32_t radius);
    bool discovered(int32_t zx, int32_t zy) const { return discoveredZones.count({zx, zy}) != 0; }
    // §5.1.6: a zone's map flags (bit 0 discovered, bit 1 has a saved tile), as the region
    // record keeps them, and putting them back from a saved record.
    uint8_t zoneFlags(int32_t zx, int32_t zy) const;
    void restoreZone(int32_t zx, int32_t zy, uint8_t flags);
    // Talk §2.5, missions §8.1: marks one zone discovered on the world map.
    void markDiscovered(int32_t zx, int32_t zy) { discoveredZones.insert({zx, zy}); }
    // Block edits §4: writes records into the held zones (live records with the removal rule of
    // §4.1, `saved` ones with the general writer of §4.2), drops plants left floating, lights
    // the changed zones again and re-meshes the 3 x 3 chunks round each record. Records for
    // zones not held are dropped.
    void applyEdits(const std::vector<BlockEdit> &edits, bool saved);
    std::string status() const;
    // The base height H of a column (terrain spec §4), for places not generated yet.
    float groundHeight(int32_t x, int32_t y) const { return terrain.height(x, y); }
    const worldgen::World &terrainWorld() const { return world; }
    const worldgen::Terrain &terrainGenerator() const { return terrain; }
    // Zones kept round the camera's in each direction (the options' render distance).
    void setRadius(int32_t zones) { radius = std::max(zones, 1); }
    // The zones held now (main thread).
    const worldgen::WorldMap &held() const { return map; }
    // Called with each land picture as it is rebuilt (the game saves it): the land's cell,
    // the picture's origin (zones) and its voxels.
    std::function<void(int32_t lx, int32_t ly, const worldgen::RegionCell &cell, glm::ivec2 origin, const CubModel &picture)>
        onLandReady;
    // Called on the main thread with each zone's new map tile (the game saves it).
    TileReady onTileReady;
    // §5.1.6 step 4: reads a saved tile back for a zone flagged as having one (null if none).
    std::function<std::optional<worldgen::MapTile>(int32_t zx, int32_t zy)> savedTile;
    // Called on the main thread each time a zone is taken in (mods hear of it).
    std::function<void(int32_t zx, int32_t zy)> onZoneReady;
    // Screens §5.2.4 item 2: whether a zone is the target of a discovered mission (its map
    // tile is then drawn undimmed).
    std::function<bool(int32_t zx, int32_t zy)> missionTarget;
    // A static object's model and its size in voxels (interaction §4.2).
    std::pair<MeshId, glm::vec3> staticModel(int32_t kind);
    // Interaction §4.4: the kinds with a moving part (doors, windows, gate, traps, lever,
    // chest), which the game draws each frame from their state instead of with the plants.
    static bool movingStatic(int32_t kind) {
        return kind == 1 || kind == 2 || kind == 3 || kind == 5 || kind == 7 || kind == 8 || kind == 9 || kind == 10;
    }

  private:
    using Key = std::pair<int32_t, int32_t>;
    void refreshPlantsNear(Key zone);
    // A placed plant model: placed = translate x scale, the yaw after the sway.
    struct PlantInstance {
        MeshId mesh;
        glm::mat4 placed, yaw;
        float scale, height;
        int phase;
        bool sways;
    };
    struct ChunkEntry {
        MeshId solid = 0, water = 0;
    };

    void post(std::function<void()> job);
    void workerLoop();
    void requestZone(Key zone);
    void requestChunk(int32_t x0, int32_t y0);
    void addZone(std::shared_ptr<worldgen::Zone> zone);
    void dropZone(Key zone);
    void placeChunk(glm::ivec2 corner, ChunkMeshes &meshes);
    std::vector<PlantInstance> plantsOf(const worldgen::Zone &zone);
    std::pair<MeshId, float> plantModel(int32_t kind);
    Key cameraZone() const;
    bool idle();

    Renderer &renderer;
    const Content &content;
    worldgen::World world;
    worldgen::Terrain terrain;
    worldgen::WorldMap map; // the zones taken in (main thread)
    int32_t radius;
    glm::ivec2 origin; // render origin, blocks
    glm::vec3 eye{0};  // render-origin relative
    float yaw = 0, pitch = 20;

    std::map<Key, std::vector<PlantInstance>> plants;
    // Rendering §10.1: a zone's light sources, in world blocks.
    struct LightSource {
        glm::dvec3 position;
        float radius;
        glm::vec3 colour;
        bool daylightDims; // statics fade with the sunlight, items do not
    };
    std::map<Key, std::vector<LightSource>> lightSources;
    mutable terrain_lights::Selector lightSelector;
    mutable std::vector<terrain_lights::Candidate> lightCandidates;
    mutable std::vector<terrain_lights::Chunk> lightChunks;
    static std::vector<LightSource> lightsOf(const worldgen::Zone &zone);
    std::map<Key, ChunkEntry> chunks; // by chunk corner
    std::map<int32_t, std::pair<MeshId, float>> plantModels;
    std::map<int32_t, std::pair<MeshId, glm::vec3>> staticModels;
    worldgen::CloudField clouds;
    MeshId cloudMesh = 0;
    motion::MotionSettings motionSettings;

    // The prefab models the zone generator asks for, loaded once (from any worker).
    std::mutex prefabLock;
    std::map<std::string, std::shared_ptr<const worldgen::Prefab>> prefabCache;

    // Background work.
    std::vector<std::thread> workers;
    std::mutex jobsLock;
    std::condition_variable jobsReady;
    std::deque<std::function<void()>> jobs;
    bool stopping = false;
    std::mutex doneLock;
    std::vector<std::shared_ptr<worldgen::Zone>> doneZones;
    // Screens §5.1.6: zone map tiles, meshed on the workers.
    struct DoneTile {
        Key key;
        ModelMesh mesh;
        int32_t base;
        std::vector<glm::ivec3> markers;
        std::shared_ptr<const worldgen::MapTile> tile;
    };
    std::vector<DoneTile> doneTiles;
    struct TileEntry {
        MeshId mesh = 0;
        int32_t base = 0;   // B, map units
        float riseMs = 250; // the rise-in timer
        std::vector<glm::ivec3> markers; // land-border points, blocks
    };
    MeshId borderMarker = 0; // §5.1.2: the 4 x 4 x 1 marker tile
    MeshId placeholder = 0;  // §5.2.4: the `maptile` block of zones without a tile
    std::set<Key> discoveredZones;
    std::set<Key> tiledZones;     // zones that have had a tile (map record flag bit 1)
    std::set<Key> unreadableTiles; // flagged zones whose saved tile could not be read
    void loadSavedTiles();
    // §5.1.6: a land's picture, one voxel per zone, rebuilt at most every 2 s while visited.
    struct LandPicture {
        MeshId mesh = 0;
        glm::ivec2 origin{0}, size{0}; // zones
        int32_t elevation = 0;
        double builtAt = -1e9;
        size_t knownZones = (size_t)-1; // discovered and tiled zones counted at the last build
    };
    std::map<Key, LandPicture> lands;
    std::set<Key> visitedLands;
    Key landOf(int32_t x, int32_t y) const; // the region cell owning a block column
    void buildLand(Key cell);
    std::map<Key, TileEntry> tiles;
    std::vector<std::pair<glm::ivec2, ChunkMeshes>> doneChunks;
    std::set<Key> zonesPending;
    std::set<Key> chunksPending, chunksStale; // stale: asked for again while pending

    double startSeconds = 0;
};
