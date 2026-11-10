// AlphaVulkan: a Vulkan engine for Cube World Alpha that reads the owner's own install.
//
//   AlphaVulkan --world [--seed N] [--at X Y] [--radius ZONES]     walk a generated world
//   AlphaVulkan --world --connect HOST [--port N] [--name NAME]    ... on a server's world
//   AlphaVulkan [--model NAME.cub] [--ground/--no-ground] [--sway]  look at one model
//   AlphaVulkan --list [.cub|.wav|.png|.xml]
//   AlphaVulkan --play NAME.wav
// and for both views: [--game DIR] [--capture FILE.png] [--yaw DEG] [--pitch DEG] [--hour H]
//                     [--no-taa] [--no-smaa] [--no-shadows]
//
// World keys: W/A/S/D move, Q/E down/up, Shift faster, drag to look, Up/Down an hour later/
// earlier, Esc to quit.
// Viewer keys: Left/Right previous/next model, Up/Down an hour later/earlier, S sway the model
// as Alpha sways its plants, drag to orbit, wheel to zoom, Esc to quit.
#include "Audio.h"
#include "Config.h"
#include "Frame.h"
#include "SimulationFrame.h"
#include "Motion.h"
#include "CubModel.h"
#include "DataStore.h"
#include "Install.h"
#include "OwnerContent.h"
#include "GenerationProfile.h"
#include "Mods.h"
#include "Renderer.h"
#include "CreatureView.h"
#include "CreatureLights.h"
#include "Player.h"
#include "creatures/Strike.h"
#include "creatures/Inventory.h"
#include "creatures/Credits.h"
#include "creatures/Spirits.h"
#include "creatures/Names.h"
#include "creatures/Npcs.h"
#include "worldgen/Items.h"
#include "Dictionary.h"
#include "GameOptions.h"
#include "ItemNames.h"
#include "Gamepad.h"
#include "WorldSession.h"
#include "StaticStates.h"
#include "Loot.h"
#include "NewCharacter.h"
#include "BlockEdits.h"
#include "Missions.h"
#include "Quests.h"
#include "WorldItems.h"
#include "WorldView.h"
#include "ui/Interaction.h"
#include "ui/LandNames.h"
#include "ui/Screens.h"
#include "net/Client.h"
#include "net/RemoteFighter.h"
#include <windows.h> // (before GLFW, which would otherwise define APIENTRY first)
#include <shobjidl.h> // (the setup folder picker)
#undef interface // (a COM macro; the game names its interface object so)
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ctime>
#include <optional>
#include <set>
#include <thread>

namespace {

struct Options {
    std::string game, model, capture, list, play;
    bool listing = false, noTaa = false, noSmaa = false, noShadows = false, noGround = false, sway = false;
    bool world = false, lookGiven = false;
    bool start = false; // --start: the start screen over the world
    bool walk = false;  // --walk: play a character instead of flying
    int creature = -1;  // --creature TYPE: the viewer shows a creature type instead of a model
    bool systemPanel = false; // --system-panel: open the system panel (for captures)
    bool inventory = false;   // --inventory: open the inventory (needs --walk)
    double cursorX = -1, cursorY = -1; // --cursor X Y: the mouse held there (for captures)
    bool bench = false;       // --bench: the customization bench on the equipped weapon (captures)
    int front = 0;            // --front N: a character/world screen (1 characters … 4 world creation)
    bool skills = false;      // --skills: the skill window open (captures)
    bool adapt = false;       // --adapt: the Adaption window with the first bag item chosen (captures)
    bool vendor = false;      // --vendor: the vendor window, stocked with the --loot items (captures)
    bool speech = false;      // --speech: the player says "There is nothing special." (captures)
    int crafting = -1;        // --crafting TAB: open the crafting window on a tab, first row chosen
    int loot = 0;             // --loot N: N items from the dungeon-loot generator in the bags (tests)
    bool crater = false;
    bool food = false;        // --food: collie food in the Pet slot, held out (pets §7.1, tests)
    bool pet = false;         // --pet: a level-3 collie pet item in the Pet slot (pets, tests)
    bool quest = false;       // --quest: stand at the first monster hunt of the region (missions, tests)      // --crater: a bomb-sized crater 10 blocks ahead (block edits §8, tests)
    bool menuRow = false;     // --menu-row: open the Esc menu row (for captures)
    bool allBars = false;     // --all-bars: life bars over every creature (V, for captures)
    bool optionsWindow = false; // --options: open the options window (for captures)
    bool worldMap = false;      // --map ZOOM: open the world map at that zoom (for captures)
    float mapZoom = 1;
    float yaw = 35, pitch = 25, hour = 12;
    bool hourOverride = false; // explicit diagnostic time in an unsaved world
    WorldViewOptions worldOptions;
    float travel = 0; // --travel: blocks to fly east before a capture
    std::string connect, name;
    std::string saveWorld; // --save-world: a single-player world kept in Save/
    int character = -1;    // --character: who plays in it (characters.db index)
    WaterLook water = WaterLook::Omega;
    bool extendedQuests = true; // required core engine quests (docs/design/quests.md)
    uint16_t port = 12345;
};

Options parse(int argc, char **argv) {
    Options o;
    for (int i = 1; i < argc; i++) {
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (!std::strcmp(argv[i], "--game"))
            o.game = next();
        else if (!std::strcmp(argv[i], "--model"))
            o.model = next();
        else if (!std::strcmp(argv[i], "--capture"))
            o.capture = next();
        else if (!std::strcmp(argv[i], "--yaw")) {
            o.yaw = (float)std::atof(next().c_str());
            o.lookGiven = true;
        } else if (!std::strcmp(argv[i], "--world"))
            o.world = true;
        else if (!std::strcmp(argv[i], "--system-panel"))
            o.systemPanel = o.world = true;
        else if (!std::strcmp(argv[i], "--inventory"))
            o.inventory = true;
        else if (!std::strcmp(argv[i], "--cursor")) {
            o.cursorX = std::stod(next());
            o.cursorY = std::stod(next());
        } else if (!std::strcmp(argv[i], "--bench")) {
            o.bench = true;
        } else if (!std::strcmp(argv[i], "--front")) {
            o.front = std::stoi(next());
        } else if (!std::strcmp(argv[i], "--skills")) {
            o.skills = true;
        } else if (!std::strcmp(argv[i], "--adapt")) {
            o.adapt = true;
        } else if (!std::strcmp(argv[i], "--vendor")) {
            o.vendor = true;
        } else if (!std::strcmp(argv[i], "--speech")) {
            o.speech = true;
        } else if (!std::strcmp(argv[i], "--crafting")) {
            o.crafting = std::stoi(next());
        } else if (!std::strcmp(argv[i], "--loot"))
            o.loot = std::stoi(next());
        else if (!std::strcmp(argv[i], "--crater"))
            o.crater = o.world = true;
        else if (!std::strcmp(argv[i], "--quest"))
            o.quest = o.world = true;
        else if (!std::strcmp(argv[i], "--pet"))
            o.pet = o.world = o.walk = true;
        else if (!std::strcmp(argv[i], "--food"))
            o.food = o.world = o.walk = true;
        else if (!std::strcmp(argv[i], "--menu-row"))
            o.menuRow = o.world = true;
        else if (!std::strcmp(argv[i], "--all-bars"))
            o.allBars = true;
        else if (!std::strcmp(argv[i], "--options"))
            o.optionsWindow = o.world = true;
        else if (!std::strcmp(argv[i], "--map")) {
            o.worldMap = o.walk = o.world = true;
            o.mapZoom = (float)std::atof(next().c_str());
        }
        else if (!std::strcmp(argv[i], "--creature"))
            o.creature = std::atoi(next().c_str());
        else if (!std::strcmp(argv[i], "--walk"))
            o.walk = o.world = true;
        else if (!std::strcmp(argv[i], "--start"))
            o.start = o.world = true;
        else if (!std::strcmp(argv[i], "--z"))
            o.worldOptions.z = (float)std::atof(next().c_str());
        else if (!std::strcmp(argv[i], "--travel"))
            o.travel = (float)std::atof(next().c_str());
        else if (!std::strcmp(argv[i], "--connect"))
            o.connect = next();
        else if (!std::strcmp(argv[i], "--port"))
            o.port = (uint16_t)std::atoi(next().c_str());
        else if (!std::strcmp(argv[i], "--name"))
            o.name = next();
        else if (!std::strcmp(argv[i], "--save-world"))
            o.saveWorld = next();
        else if (!std::strcmp(argv[i], "--character"))
            o.character = std::atoi(next().c_str());
        else if (!std::strcmp(argv[i], "--seed"))
            o.worldOptions.seed = (uint32_t)std::strtoul(next().c_str(), nullptr, 10);
        else if (!std::strcmp(argv[i], "--radius"))
            o.worldOptions.radius = std::clamp(std::atoi(next().c_str()), 0, 8);
        else if (!std::strcmp(argv[i], "--at")) {
            o.worldOptions.x = std::atoi(next().c_str());
            o.worldOptions.y = std::atoi(next().c_str());
        }
        else if (!std::strcmp(argv[i], "--play"))
            o.play = next();
        else if (!std::strcmp(argv[i], "--no-taa"))
            o.noTaa = true;
        else if (!std::strcmp(argv[i], "--no-smaa"))
            o.noSmaa = true;
        else if (!std::strcmp(argv[i], "--no-shadows"))
            o.noShadows = true;
        else if (!std::strcmp(argv[i], "--sway"))
            o.sway = true;
        else if (!std::strcmp(argv[i], "--no-ground"))
            o.noGround = true;
        else if (!std::strcmp(argv[i], "--hour")) {
            o.hour = (float)std::atof(next().c_str());
            o.hourOverride = true;
        }
        else if (!std::strcmp(argv[i], "--pitch")) {
            o.pitch = (float)std::atof(next().c_str());
            o.lookGiven = true;
        }
        else if (!std::strcmp(argv[i], "--list")) {
            o.listing = true;
            if (i + 1 < argc && argv[i + 1][0] == '.')
                o.list = argv[++i];
        }
    }
    return o;
}

// The viewer's stand-in ground: a flat square at z = 0 under the model, so its shadow has
// somewhere to fall. A plain colour of the viewer's own, not game content. It is made of
// 4-block tiles because Alpha's fog is worked out per vertex, like the game's small faces.
void addGround(ModelMesh &mesh, float halfSize) {
    const glm::vec4 colour(.42f, .55f, .30f, 1);
    constexpr float TILE = 4;
    const int tiles = (int)std::ceil(halfSize / TILE);
    for (int ty = -tiles; ty < tiles; ty++)
        for (int tx = -tiles; tx < tiles; tx++) {
            const uint32_t first = (uint32_t)mesh.vertices.size();
            for (const glm::vec2 corner : {glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)})
                mesh.vertices.push_back({glm::vec3((glm::vec2(tx, ty) + corner) * TILE, 0), glm::vec3(0, 0, 1), colour});
            for (uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u})
                mesh.indices.push_back(first + i);
        }
}

// The folder AlphaVulkan.exe is in (mods live beside it, not in the working folder).
fs::path executableFolder() {
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    return length ? fs::path(std::wstring(path, length)).parent_path() : fs::current_path();
}

// Orbit camera around the model's middle, z up.
struct Orbit {
    float yaw, pitch, distance = 60;
    glm::vec3 target{0};

    // The camera sits `distance` out from the target and looks back at it.
    Camera camera(VkExtent2D size) const {
        Camera c;
        c.yawDegrees = -yaw;
        c.pitchDegrees = pitch;
        c.aspect = (float)size.width / (float)std::max(1u, size.height);
        c.eye = target - distance * c.forward();
        return c;
    }
};

struct Viewer {
    const Content *content = nullptr;
    Renderer *renderer = nullptr;
    GLFWwindow *window = nullptr;
    std::vector<std::string> models;
    size_t current = 0;
    Orbit orbit{35, 25};
    Scene scene;
    bool dragging = false, ground = true, sway = false;
    MeshId modelMesh = 0, groundMesh = 0;
    float modelHeight = 1;
    double lastX = 0, lastY = 0;
    // Creature mode (--creature): a creature type standing or walking in place.
    int creature = -1;
    bool walking = true;
    creatures::Appearance looks;
    creatures::PoseState pose;

    void showCreature(int type) {
        creature = (type % creatures::TYPE_COUNT + creatures::TYPE_COUNT) % creatures::TYPE_COUNT;
        looks = creatures::Appearance();
        pose = creatures::PoseState();
        int32_t t = creature;
        uint32_t seed = 1; // NPC looks from a fixed stream, so a type always looks the same here
        creatures::generate(t, looks, nullptr, [&] {
            seed = seed * 214013u + 2531011u;
            return (int)((seed >> 16) & 0x7fff);
        });
        // Model units: a creature is about 25 units tall.
        renderer->removeMesh(groundMesh);
        ModelMesh plane;
        addGround(plane, 80);
        groundMesh = ground ? renderer->addMesh(plane) : 0;
        renderer->dropHistory();
        orbit.target = glm::vec3(0, 0, 11.2f * looks.box.z / looks.box.x * 0.5f);
        orbit.distance = 70;
        const std::string title = "AlphaVulkan - creature type " + std::to_string(creature);
        glfwSetWindowTitle(window, title.c_str());
    }

    bool show(size_t index) {
        if (models.empty())
            return false;
        current = index % models.size();
        const std::string &key = models[current];
        CubModel model;
        std::string error;
        const auto bytes = content->read(key);
        if (!bytes || !model.load(bytes->data(), bytes->size(), &error)) {
            std::fprintf(stderr, "%s: %s\n", key.c_str(), bytes ? error.c_str() : "not found");
            return false;
        }
        const float size = (float)std::max({model.sx, model.sy, model.sz});
        renderer->removeMesh(modelMesh);
        renderer->removeMesh(groundMesh);
        modelMesh = renderer->addMesh(model.mesh());
        groundMesh = 0;
        if (ground) {
            ModelMesh plane;
            addGround(plane, 2 * size + 16);
            groundMesh = renderer->addMesh(plane);
        }
        renderer->dropHistory();
        orbit.target = glm::vec3(0, 0, model.sz * .5f);
        orbit.distance = 1.6f * size + 4;
        modelHeight = (float)model.sz;
        scene.focus = glm::vec3(0);
        const std::string title = "AlphaVulkan - " + key + " (" + std::to_string(model.sx) + "x" +
                                  std::to_string(model.sy) + "x" + std::to_string(model.sz) + ")";
        glfwSetWindowTitle(window, title.c_str());
        return true;
    }
};

// The --world loop: fly over a generated world.
int runWorld(GLFWwindow *window, Renderer &renderer, const Content &content, const fs::path &install,
             const Options &options,
             const ShadowSettings &shadowSettings, const MoonSettings &moonSettings, ModLoader &mods,
             Scene *&activeScene, WorldView *&activeView, Player *&activePlayer, ui::Interface *&activeInterface,
             Audio &audio, std::optional<Options> &restart) {
    // One engine gameplay stream. Native startup/thread state is still unclosed;
    // seed one is an explicit engine admission policy, independent of terrain.
    worldgen::Random worldRandom{1};
    // Joining a server: its seed decides the world (packets §3.9).
    net::Client client;
    WorldViewOptions worldOptions = options.worldOptions;
    if (!options.connect.empty()) {
        std::string error;
        if (!client.connect(options.connect, options.port, &error)) {
            std::fprintf(stderr, "Error: could not connect to %s: %s\n", options.connect.c_str(), error.c_str());
            return 1;
        }
        worldOptions.seed = client.seed();
        client.say("\x01" "AV1"); // AlphaVulkan's hello (docs/design/quests.md); Alpha servers pass it on as chat
        std::fprintf(stderr, "Connected. Player id %llu, world online_%u.\n", (unsigned long long)client.id(), client.seed());
    }
    // Online, the character still comes from the saves (characters.db), and goes back there.
    std::optional<CharacterRecord> onlineCharacter;
    if (!options.connect.empty() && options.character >= 0) {
        SaveDb db;
        if (db.open(executableFolder() / "Save" / "characters.db", SaveDb::Mode::ReadWrite)) {
            std::vector<CharacterRecord> all = readCharacters(db);
            if (options.character < (int)all.size())
                onlineCharacter = all[options.character];
        }
    }
    // A single-player world from the saves: its seed, its clock, where the character stood.
    std::optional<WorldSession> session;
    if (options.connect.empty() && !options.saveWorld.empty()) {
        std::string error;
        session.emplace();
        if (!session->open(executableFolder(), options.saveWorld, worldOptions.seed, options.character, &error,
                           generationProfile(worldOptions.swimmableWater, mods.loaded()))) {
            std::fprintf(stderr, "Error: cannot open world %s: %s\n", options.saveWorld.c_str(), error.c_str());
            return 1;
        }
        worldOptions.seed = session->seed();
        if (const auto at = session->startPosition()) {
            worldOptions.x = (int32_t)std::floor(at->x);
            worldOptions.y = (int32_t)std::floor(at->y);
            worldOptions.z = (float)at->z;
        }
    }
    // The character playing: the session's, or the one brought online.
    const auto playing = [&]() -> CharacterRecord * {
        if (session && session->playing())
            return &*session->playing();
        return onlineCharacter ? &*onlineCharacter : nullptr;
    };
    std::fprintf(stderr, "generating the world round (%d, %d), seed %u...\n", worldOptions.x, worldOptions.y,
                 worldOptions.seed);
    const auto persistMapTile = [&session](int32_t zx, int32_t zy, const worldgen::MapTile &tile) {
        if (!session)
            return;
        TileSave save;
        save.base = tile.base;
        save.sx = tile.sx, save.sy = tile.sy, save.sz = tile.sz;
        save.rgb = tile.rgb;
        for (const glm::ivec3 &m : tile.markers)
            save.markers.push_back({m.x, m.y, m.z});
        session->saveTile(zx, zy, save);
    };
    WorldView view(renderer, content, worldOptions, persistMapTile);
    activeView = &view;
    // World items §4: each zone's items as generated, replaced by its save.
    WorldItems worldItems;
    // Interaction §4, §6: the statics' states, timers and occupants.
    StaticStates statics;
    // Block edits §3: each held zone's edits, saved with the zone.
    BlockEditLists blockEdits;
    // Missions (missions §9): run here offline; online the server's records arrive in section 13.
    Missions missions;
    // AlphaVulkan's quests (docs/design/quests.md): this player's book, read from the world's
    // `av_quests` value, and the quest the tag follows.
    quests::Book questBook;
    bool questBookRead = false, questBookChanged = false;
    uint32_t trackedQuest = 0;
    std::u16string questObjective;
    worldgen::Random questRandom{(uint32_t)worldRandom.draw()}; // engine quest extension
    // A speech bubble's page of plain words.
    const auto saying = [](const std::string &text) {
        SpeechPage page;
        size_t at = 0;
        while (at < text.size()) {
            const size_t end = std::min(text.find(' ', at), text.size());
            if (end > at)
                page.push_back({text.substr(at, end - at), glm::vec4(1)});
            at = end + 1;
        }
        return std::vector<SpeechPage>{page};
    };
    missions.wildlife = [&](int32_t x, int32_t y, int32_t h, worldgen::Random &random) {
        return view.terrainGenerator().wildlifeSpecies(x, y, h, 0, random);
    };
    std::set<std::pair<int32_t, int32_t>> missionRegionsRead, zonesForMissions;
    int32_t missionDay = -1;
    ui::Interface::Front cameraFront = ui::Interface::Front::None; // character screens §7
    bool windowHeld = false; // skills §1.3: whether a window held the input last frame
    // Timed lights: materials §1.5's critical-hit flashes, particles §7.2's bomb flash.
    terrain_lights::FadingSources flashes;
    std::set<uint64_t> shattered; // particles §7.2: dead creatures already broken up
    std::pair<int32_t, int32_t> missionRegionAsked{INT32_MIN, INT32_MIN}; // online: the region last asked for
    std::optional<MissionArea> questRemembered; // missions §8.2: the mission the quest tag shows
    // Mod SDK v5: missions to read and block edits to make, while this world runs.
    mods.hooks().getMission = [&](int32_t cx, int32_t cy, AvMission &out) {
        const Missions::Cell *cell = missions.cell(cx, cy);
        if (!cell || cell->mission.kind == 0)
            return false;
        const MissionArea &m = cell->mission;
        out = {cx, cy, m.kind, m.targetType, m.level, m.tier, m.state, m.count, m.required, m.targetZoneX, m.targetZoneY};
        return true;
    };
    mods.hooks().setBlock = [&](int32_t x, int32_t y, int32_t z, const uint8_t block[4]) {
        if (client.connected() || !view.held().zoneAt(x, y))
            return false;
        const BlockEdit e{x, y, z, block[0], block[1], block[2], block[3], session ? (int32_t)session->clock.day : 0};
        view.applyEdits({e}, false);
        blockEdits.add(e);
        return true;
    };
    struct HookReset {
        ModLoader &mods;
        ~HookReset() {
            mods.hooks().getMission = {}, mods.hooks().setBlock = {}, mods.hooks().getCreatures = {};
            mods.hooks().spawnParticles = {}, mods.hooks().getQuests = {};
            mods.hooks().say = {}, mods.hooks().showChat = {};
        }
    } hookReset{mods};
    // Screens §5.2.4 item 2: a discovered mission's target zone is drawn undimmed on the map.
    view.missionTarget = [&](int32_t zx, int32_t zy) {
        const auto [cx, cy] = cellOfZone(zx, zy);
        const Missions::Cell *cell = missions.cell(cx, cy);
        return cell && cell->mission.kind != 0 && cell->mission.state == 1 && cell->mission.targetZoneX == zx &&
               cell->mission.targetZoneY == zy;
    };
    const auto zoneReady = [&](int32_t zx, int32_t zy) {
        mods.zoneReady(zx, zy);
        if (const worldgen::Zone *zone = view.held().zone(zx, zy)) {
            worldItems.generated(*zone);
            statics.generated(*zone);
            zonesForMissions.insert({zx, zy}); // missions §2.6: a new zone gets the zone pass
        }
        if (session)
            if (const auto saved = session->loadZone(zx, zy)) {
                worldItems.restore(zx, zy, saved->items, (int32_t)session->clock.day);
                statics.restore(zx, zy, saved->staticStates);
                // Block edits §4.2: the edits of the last three days are written again.
                const auto &kept = blockEdits.restore(zx, zy, saved->blockEdits, (int32_t)session->clock.day);
                if (!kept.empty())
                    view.applyEdits(kept, true);
            }
        worldItems.takeChanged(); // (what came from the save is already saved)
        statics.takeChanged();
    };
    for (const auto &[key, zone] : view.held().zones()) // (the zones made before the first frame)
        zoneReady(key.first, key.second);
    view.onZoneReady = zoneReady;
    // Screens §5.1.6: land pictures and map tiles go into map_<name>.db as they are made.
    view.onLandReady = [&](int32_t lx, int32_t ly, const worldgen::RegionCell &cell, glm::ivec2 origin, const CubModel &picture) {
        if (!session)
            return;
        LandSave save;
        save.originX = origin.x, save.originY = origin.y;
        save.visited = true;
        save.sx = picture.sx, save.sy = picture.sy, save.sz = picture.sz;
        save.rgb = picture.rgb;
        const uint8_t fire = cell.fire ? 1 : 0;
        std::memcpy(&save.cell[0], &cell.x, 4);
        std::memcpy(&save.cell[4], &cell.y, 4);
        save.cell[8] = fire;
        std::memcpy(&save.cell[12], &cell.moisture, 4);
        std::memcpy(&save.cell[16], &cell.warmth, 4);
        std::memcpy(&save.cell[20], &cell.id, 4);
        std::memcpy(&save.cell[24], &cell.elevation, 4);
        session->saveLand(lx, ly, save);
    };
    view.savedTile = [&](int32_t zx, int32_t zy) -> std::optional<worldgen::MapTile> {
        if (!session)
            return std::nullopt;
        const std::optional<TileSave> saved = session->loadTile(zx, zy);
        if (!saved)
            return std::nullopt;
        worldgen::MapTile tile;
        tile.base = saved->base;
        tile.sx = saved->sx, tile.sy = saved->sy, tile.sz = saved->sz;
        tile.rgb = saved->rgb;
        for (const auto &m : saved->markers)
            tile.markers.push_back({m[0], m[1], m[2]});
        return tile;
    };
    // §5.1.6: the region records round the player (zone flags and copies), read when first
    // near, written whenever they change (a new one at once), let go beyond 8 regions.
    std::map<std::pair<int32_t, int32_t>, std::pair<RegionSave, std::vector<uint8_t>>> mapRegions;
    double lastRegionPass = -1e9;
    auto regionPass = [&](glm::dvec3 at) {
        if (!session)
            return;
        const int32_t rx = worldgen::floorDiv((int32_t)std::floor(at.x), worldgen::REGION);
        const int32_t ry = worldgen::floorDiv((int32_t)std::floor(at.y), worldgen::REGION);
        for (int32_t i = rx - 3; i <= rx + 3; i++)
            for (int32_t j = ry - 3; j <= ry + 3; j++) {
                if (mapRegions.count({i, j}))
                    continue;
                auto &[record, written] = mapRegions[{i, j}];
                if (const std::optional<RegionSave> saved = session->loadRegion(i, j)) {
                    record = *saved;
                    written = record.serialize();
                    for (int32_t k = 0; k < 64 * 64; k++)
                        view.restoreZone(64 * i + k / 64, 64 * j + k % 64, record.zones[k].flags);
                }
            }
        const worldgen::World &world = view.terrainWorld();
        for (auto it = mapRegions.begin(); it != mapRegions.end();) {
            const auto [i, j] = it->first;
            auto &[record, written] = it->second;
            for (int32_t k = 0; k < 64 * 64; k++)
                record.zones[k].flags = view.zoneFlags(64 * i + k / 64, 64 * j + k % 64);
            if (const worldgen::Region *region = world.region(i, j))
                record.refresh(*region);
            const bool distant = std::abs(i - rx) > 8 || std::abs(j - ry) > 8;
            std::vector<uint8_t> bytes = record.serialize();
            if (bytes != written) {
                session->saveRegion(i, j, record);
                written = std::move(bytes);
            }
            it = distant ? mapRegions.erase(it) : std::next(it);
        }
    };
    // Creatures: the other players.
    CreatureView creatureView(renderer, content);
    std::map<uint64_t, creatures::PoseState> otherPoses;
    std::map<uint64_t, creatures::Appearance> otherLooks; // for the corner map's markers
    // Other players' fighting state as last received; the action clock runs on locally
    // between updates so their swings play smoothly.
    creatures::ActorMap<net::RemoteFighter> otherFighters;
    std::map<uint64_t, net::CreatureRecord> receivedActors;
    net::CreatureRecord own;
    own.setName(options.name.empty() ? "AlphaVulkan" : options.name);
    if (options.name.empty() && playing())
        own.setName(playing()->name);
    // The player (--walk): a male human, or the saved character's looks.
    std::optional<Player> player;
    // Screens §8.1: the player's bags, equipment and money; the combat record's equipment is
    // refreshed from it after every change.
    creatures::Inventory inventory = creatures::Inventory::fresh();
    int32_t manaCubes = 0; // skills §6: record field 47, saved as field 22
    const auto wearInventory = [&] {
        if (!player)
            return;
        for (size_t slot = 0; slot < inventory.equipment.size(); slot++)
            player->combat().equipment[slot] = creatures::Item::parse(inventory.equipment[slot].data());
    };
    creatures::Npcs npcs{worldRandom};
    npcs.objects = [&](glm::dvec3 lo, glm::dvec3 hi, int axis, double move, const creatures::IgnoredStatics *ignored) {
        return statics.blocks(view.held(), lo, hi, axis, move, ignored);
    };
    npcs.openDoors = [&](glm::dvec3 lo, glm::dvec3 hi) { statics.openDoors(view.held(), lo, hi); };
    npcs.seats = [&](glm::ivec3 block, double ms) { return statics.seats(view.held(), block, ms); };
    npcs.seatFree = [&](const creatures::Seat &s) { return statics.free(s); };
    npcs.pathStatics = [&](int32_t ix0, int32_t ix1, int32_t iy0, int32_t iy1,
                           const std::function<void(const creatures::SearchStatic &)> &visit) {
        statics.searchStatics(view.held(), ix0, ix1, iy0, iy1, visit);
    };
    npcs.sit = [&](creatures::Npc &n, const creatures::Seat &s) {
        if (const worldgen::Zone *zone = view.held().zone(s.zx, s.zy)) {
            std::vector<creatures::SoundEvent> ignored;
            statics.use(*zone, s.index, n.fighter, n.body, n.yaw, worldItems, 0, // (the day matters only for chests)
                        [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); },
                        [&] { return worldRandom.draw(); }, ignored);
        }
    };
    // Behaviour §9.2 step 6: in a cell with a dungeon mission (kind 5) the location's creatures
    // take the mission's level and tier, and its boss becomes the target.
    // Mod SDK v6: the creatures held now (offline the ones run here, online the server's list).
    mods.hooks().getCreatures = [&] {
        std::vector<AvCreature> out;
        const auto add = [&](uint64_t id, const creatures::Combatant &f, glm::dvec3 at) {
            out.push_back({id, f.type, f.controller, f.level, f.health, creatures::maxHealth(f), at.x, at.y, at.z});
        };
        if (!client.connected())
            for (const creatures::Npc &n : npcs.all())
                add(n.fighter.id, n.fighter, n.body.blocks());
        for (const auto &[id, other] : receivedActors)
            add(id, other.fighter(), glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0);
        return out;
    };
    npcs.onActivate = [&](creatures::Npc &n) { missions.wake(n, npcs); };
    // Projectiles (alpha-projectiles.md): everything in flight, and their hits on NPCs for the
    // next offline step.
    creatures::Projectiles projectiles;
    // Particles §3.2: the one list, and the bursts gathered this frame for it (§2.1).
    creatures::Particles particles;
    std::vector<creatures::Burst> bursts;
    double leafClockMs = -1; // the world clock the falling leaves last looked at (§7.1)
    mods.hooks().getQuests = [&] { // mod SDK v8
        std::vector<AvQuest> out;
        for (const quests::Quest &q : questBook.quests)
            out.push_back({q.id, q.kind, q.state, q.giver, q.level, q.count, q.required, q.site.x, q.site.y, q.site.z});
        return out;
    };
    // Audio §1.2, §2: a sound at a place, heard from the camera: fading linearly to silence at
    // 100 blocks, no panning.
    const auto playAt = [&](int32_t id, glm::dvec3 at, float volume, float pitch) {
        if ((id == 81 || id == 82) && player) // explosions and punches shake the camera, at any distance
            player->followCamera().shake = 1;
        const glm::dvec3 d = at - view.worldPosition();
        const double d2 = glm::dot(d, d);
        if (d2 > 10000)
            return;
        if (const char *key = creatures::soundKey(id))
            audio.play(key, (float)(1 - std::sqrt(d2) / 100) * volume, pitch);
    };
    const auto sendBuff = [&](const creatures::BuffEvent &event) {
        if (!client.connected() || event.source != client.id())
            return;
        client.sendEvent40(event.serialize());
    };
    mods.hooks().spawnParticles = [&](double x, double y, double z, const float velocity[3], const float rgb[3],
                                      float size, int32_t count, int32_t style, float spread) {
        creatures::Burst b = creatures::Burst::at({x, y, z}, {velocity[0], velocity[1], velocity[2]},
                                                  {rgb[0], rgb[1], rgb[2], 1}, size, std::clamp(count, 0, 5000), style);
        b.spread = spread;
        bursts.push_back(b);
        return true;
    };
    std::vector<creatures::HitRecord> shotHitsOnNpcs;
    std::vector<creatures::Projectile> playerLaunched;
    creatures::ShotEffects playerShot;
    creatures::NpcFrame playerActionFrame;
    bool frontListsStale = true; // character screens: reread the saves' lists
    // Character screens §4.1: the figures of the character panels, posed and turning.
    struct PanelFigure {
        creatures::Appearance looks;
        creatures::Combatant fighter;
        creatures::PoseState pose;
    };
    std::vector<PanelFigure> panelFigures;
    std::vector<std::pair<MeshId, glm::vec3>> worldPreviews; // §4.2: each listed world's preview mesh
    glm::dvec2 benchMouse{0};
    int chosenCharacter = options.character;
    // Last observed recipe level. Saved characters synchronize this after load;
    // only the engine's unsaved diagnostic character needs a first-frame grant.
    int32_t grantedLevel = 0;
    double lastZoneRequest = -1;
    // Server simulation §8: a packet-6 request (an item, a target, a kind).
    const auto itemRequest = [](const creatures::ItemRecord &item, std::array<int32_t, 3> target, uint8_t kind) {
        std::array<uint8_t, 300> r{};
        std::memcpy(r.data(), item.data(), 280);
        std::memcpy(r.data() + 0x118, target.data(), 12);
        r[0x128] = kind;
        return r;
    };
    // Interaction §2.4: the world item E would pick up, (zone x, zone y, index).
    std::optional<std::array<int32_t, 3>> itemTarget;
    bool eWasDown = false;
    bool tWasDown = false; // pets §13: T calls the pet back
    if (options.walk) {
        creatures::Appearance looks;
        int32_t type = 2;
        creatures::CharacterChoice choice;
        if (playing())
            choice = creatures::CharacterChoice::parse(playing()->extra.data());
        creatures::generate(type, looks, &choice, [] { return 0; });
        player.emplace(view, looks, worldRandom);
        player->objects = [&](glm::dvec3 lo, glm::dvec3 hi, int axis, double move, const creatures::IgnoredStatics *) {
            return statics.blocks(view.held(), lo, hi, axis, move);
        };
        activePlayer = &*player;
        player->combat().id = client.connected() ? client.id() : 1; // never 0, which means "nobody"
        // Without a saved character the engine hands out a level 1 iron sword, so that there is
        // something to fight with (not Alpha's; a new Alpha character's kit is unspecified).
        inventory.equipment[creatures::MAIN_HAND][0] = 3;
        inventory.equipment[creatures::MAIN_HAND][0x0d] = 1;
        creatures::setItemLevel(inventory.equipment[creatures::MAIN_HAND], 1);
        wearInventory();
        own.setAppearance(looks);
        if (playing()) {
            // The saved character's stats (saves §3).
            const CharacterRecord &saved = *playing();
            creatures::Combatant &c = player->combat();
            c.level = std::max(saved.level, 1);
            c.experience = saved.experience;
            c.characterClass = saved.characterClass;
            c.specialization = saved.specialization;
            for (size_t i = 0; i < saved.skills.size() && i < c.skills.size(); i++)
                c.skills[i] = saved.skills[i];
            inventory.equipment = saved.equipment;
            if (!saved.tabs.empty())
                inventory.bags.clear();
            for (const auto &tab : saved.tabs) {
                auto &bag = inventory.bags.emplace_back();
                for (const CharacterRecord::Slot &slot : tab)
                    bag.push_back({(int32_t)slot.count, slot.item});
            }
            inventory.money = saved.money;
            inventory.platinum = saved.platinum;
            inventory.recipes = saved.recipes;
            grantedLevel = c.level;
            manaCubes = (int32_t)saved.manaCubes;
            wearInventory();
            c.health = saved.health > 0 ? std::min(saved.health, creatures::maxHealth(c)) : creatures::maxHealth(c);
        }
        // --loot: items from the dungeon-loot generator (items spec §1) at the player's level,
        // with a few potions and coins, to try the inventory with.
        if (options.loot > 0) {
            worldgen::Random random(123);
            for (int i = 0; i < options.loot; i++)
                inventory.add(creatures::toRecord(worldgen::lootItem(random, player->combat().level, 4)));
            creatures::ItemRecord potion{}, coins{};
            potion[0] = 1, potion[1] = 1;
            creatures::setItemLevel(potion, 1);
            for (int i = 0; i < 3; i++)
                inventory.add(potion);
            coins[0] = 12, coins[0x0d] = 12; // silver
            creatures::setItemLevel(coins, 345);
            inventory.add(coins);
        }
        if (options.crater) {
            const float yaw = glm::radians(player->facing());
            const glm::dvec3 at = player->body().blocks() + glm::dvec3(-std::sin(yaw) * 10, std::cos(yaw) * 10, 0);
            const auto removed = naturalSphere(at, 8, session ? (int32_t)session->clock.day : 0, [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z); });
            for (const BlockEdit &e : removed) { // particles §5.2
                const worldgen::Block b = view.held().read(e.x, e.y, e.z);
                bursts.push_back(creatures::blockBurst({e.x, e.y, e.z}, {b.r, b.g, b.b}));
            }
            view.applyEdits(removed, false);
            for (const BlockEdit &e : removed)
                blockEdits.add(e);
            std::fprintf(stderr, "--crater: %zu blocks removed\n", removed.size());
        }
    }
    std::fprintf(stderr, "%s\n", view.status().c_str());
    float travelled = 0;
    view.setLook(options.lookGiven ? options.yaw : 30.f, options.lookGiven ? options.pitch : 15.f);
    Scene scene;
    scene.timeOfDayMs = std::fmod(options.hour, 24.f) * 3600000.0;
    if (session) {
        scene.timeOfDayMs = session->clock.timeOfDayMs;
        scene.day = (int)session->clock.day;
    }
    scene.water = options.water;
    activeScene = &scene;
    npcs.onFatalObject = [&](const creatures::Combatant &fighter, creatures::NpcFrame &frame) {
        if (!client.connected())
            statics.fatal(view.held(), fighter.backing, frame);
    };
    npcs.onFatalLoot = [&](const creatures::Combatant &fighter, const creatures::Body &body,
                           std::span<const creatures::ItemRecord> items) {
        if (!client.connected())
            dropLoot(lootVictim(fighter, body, items), worldItems, scene.day,
                     [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); }, worldRandom);
    };
    npcs.onFatalRegion = [&](creatures::Combatant &fighter, uint8_t marker,
                            const std::function<creatures::Combatant *(uint64_t)> &lookup, creatures::NpcFrame &frame) {
        if (client.connected())
            return;
        if (const auto progress = missions.fatal(fighter, marker, 0,
                [&](int32_t x, int32_t y) { return view.terrainWorld().region(x, y) != nullptr; }, lookup)) {
            frame.missions.push_back(*progress);
            if (progress->completed())
                npcs.trimMissionSpawns(fighter, view.held());
        }
    };

    // The interface (alpha-ui-screens.md): the start screen and the system menu.
    // The dictionary the interface reads names from (dict_en.xml, data4.db).
    Dictionary dictionary;
    if (const auto bytes = content.read("dict_en.xml"))
        dictionary.parse(std::string(bytes->begin(), bytes->end()));
    ui::Interface interface;
    activeInterface = &interface;
    mods.hooks().say = [&](const std::string &text) {
        interface.chatSubmitted.push_back(text); // (as if typed, /commands included)
        return true;
    };
    mods.hooks().showChat = [&](const std::string &text, float r, float g, float b) {
        interface.chatAdd(text + "\n", {r, g, b});
        return true;
    };
    interface.worldPreview = [&](size_t index) {
        return index < worldPreviews.size() ? worldPreviews[index] : std::pair<MeshId, glm::vec3>{0, glm::vec3(0)};
    };
    interface.characterParts = [&](size_t index, float yaw) {
        std::vector<std::pair<uint32_t, glm::mat4>> out;
        if (index >= panelFigures.size())
            return out;
        PanelFigure &figure = panelFigures[index];
        creatures::Motion still;
        still.yawDegrees = yaw;
        still.fighter = &figure.fighter;
        for (const auto &[mesh, matrix] : creatureView.parts(figure.looks, figure.pose, still, 16))
            out.push_back({mesh, matrix});
        return out;
    };
    {
        std::string error;
        if (!interface.load(install, &error))
            std::fprintf(stderr, "the interface is not available: %s\n", error.c_str());
    }
    interface.loadIcons([&](const std::string &key) { return content.read(key); });
    // Screens §8.1: the inventory screen shows the player's belongings, each item by its model.
    interface.inventory = &inventory;
    interface.dictionary = &dictionary;
    // Quests: a creature type's plural name ("Goblins"), empty when the dictionary has none.
    const auto plural = [&](int32_t type) {
        const char *key = creatures::speciesKey(type);
        const DictionaryEntry *e = key ? dictionary.find("name", key) : nullptr;
        return e && e->field("plural") ? *e->field("plural") : std::string();
    };
    interface.itemVoxels = [&](const creatures::ItemRecord &record) {
        const uint16_t slot = creatures::itemModel(creatures::Item::parse(record.data()));
        return slot == creatures::NO_MODEL ? nullptr : creatureView.voxelModel(slot);
    };
    interface.itemName = [&](const creatures::ItemRecord &record) { return ui::utf16(itemName(record, dictionary)); };
    // Screens §8.1.9: an item the player received, into the bags and announced in the chat.
    const auto receive = [&](const creatures::ItemRecord &got) {
        const uint8_t type = creatures::itemType(got);
        mods.itemReceived(type, creatures::itemSubtype(got), creatures::itemLevel(got));
        if (type == 25) {
            // §8.1.9: a mana cube; every fourth is a skill point (skills §3.1).
            manaCubes++;
            interface.chatAdd("You receive a mana cube.\n", {255, 255, 255});
            if (manaCubes % 4 == 0) {
                interface.chatAdd("You gain one skill point.\n", {255, 255, 255});
                if (const char *key = creatures::soundKey(0x1e))
                    audio.play(key);
            }
            return;
        }
        inventory.add(got);
        interface.chatAdd("You receive ", {255, 255, 255});
        interface.chatAdd(receivedText(got, dictionary), type == 12 ? glm::vec3(255) : creatures::nameColour(got) * 255.f);
        interface.chatAdd(".\n", {255, 255, 255});
    };
    interface.itemModel = [&](const creatures::ItemRecord &record) {
        const uint16_t slot = creatures::itemModel(creatures::Item::parse(record.data()));
        if (slot == creatures::NO_MODEL)
            return ui::Interface::ItemModel{};
        return ui::Interface::ItemModel{creatureView.plainMesh(slot), creatureView.voxels(slot)};
    };
    interface.startScreen = options.start;
    interface.systemMenu = options.systemPanel;
    interface.inventoryOpen = options.inventory && player;
    bool sayAtStart = options.speech;
    if (options.skills && player)
        interface.toggleSkills();
    if (options.food && player) {
        creatures::ItemRecord food{};
        food[0x00] = 20, food[0x01] = 19;
        creatures::setItemLevel(food, 1);
        inventory.equipment[12] = food;
        wearInventory();
    }
    if (options.pet && player) {
        creatures::ItemRecord collie{};
        collie[0x00] = 19, collie[0x01] = 19;
        creatures::setItemLevel(collie, 3);
        inventory.equipment[12] = collie;
        wearInventory();
    }
    if (options.bench && player) {
        interface.benchOpen = interface.inventoryOpen = true;
        interface.benchBox = creatures::MAIN_HAND;
        creatures::ItemRecord &weapon = inventory.equipment[creatures::MAIN_HAND];
        creatures::placeSpirit(weapon, {0, 0, 2}, 128, 1);
        creatures::placeSpirit(weapon, {0, 0, 3}, 130, 1);
        creatures::ItemRecord spirit{};
        spirit[0] = 11, spirit[1] = 14, spirit[0x0d] = 129;
        creatures::setItemLevel(spirit, 1);
        inventory.add(spirit);
        wearInventory(); // (the held weapon shows its cubes)
    }
    if (options.front >= 1 && options.front <= 4) {
        interface.front = (ui::Interface::Front)options.front;
        interface.creationChanged = true;
    }
    if (options.adapt && player) {
        interface.adaptOpen = interface.inventoryOpen = true;
        interface.adaptBag = 0, interface.adaptSlot = 0;
    }
    if (options.vendor && player) {
        interface.vendorOpen = interface.inventoryOpen = true;
        for (const auto &bag : inventory.bags)
            for (const creatures::Stack &slot : bag)
                if (!slot.empty())
                    interface.vendorStock.push_back(slot.item);
    }
    if (options.crafting >= 0 && player) {
        interface.craftingOpen = true;
        interface.selectRecipe(std::clamp(options.crafting, 0, 5), 0);
    }
    interface.menuRow = options.menuRow;
    interface.allLifeBars = options.allBars;
    interface.optionsOpen = options.optionsWindow;
    interface.mapOpen = options.worldMap;
    interface.mapZoom = interface.mapZoomTarget = options.mapZoom;

    // Screens §6: the game settings from options.cfg beside the engine, the display modes'
    // distinct sizes, and what each setting changes here.
    const fs::path optionsFile = executableFolder() / "options.cfg";
    GameOptions game;
    if (const GLFWvidmode *desktop = glfwGetVideoMode(glfwGetPrimaryMonitor()))
        game[GameOptions::RESOLUTION_X] = desktop->width, game[GameOptions::RESOLUTION_Y] = desktop->height;
    game.load(optionsFile);
    {
        int count = 0;
        const GLFWvidmode *modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &count);
        for (int i = 0; i < count; i++) {
            const glm::ivec2 size(modes[i].width, modes[i].height);
            if (std::find(interface.resolutions.begin(), interface.resolutions.end(), size) == interface.resolutions.end())
                interface.resolutions.push_back(size);
        }
    }
    interface.setOptions(game);
    auto applyOptions = [&](bool first) {
        audio.setMasterVolume((float)game[GameOptions::SOUND_VOLUME] * 0.01f);
        if (player) {
            creatures::CameraOptions &look = player->cameraOptions();
            look.speed = game[GameOptions::CAMERA_SPEED];
            look.smoothness = game[GameOptions::CAMERA_SMOOTHNESS];
            look.invertY = game[GameOptions::INVERT_Y] != 0;
        }
        // Render distance: a radius of (distance % of 384) blocks, in whole zones (§6.4).
        view.setRadius((int32_t)std::ceil((float)game[GameOptions::RENDER_DISTANCE] * 0.01f * 384.f / 256.f));
        // Fullscreen at the chosen size, or windowed as the window is (the resolution only
        // counts in fullscreen). Anti-aliasing stays the engine's own (SMAA + TAA).
        GLFWmonitor *monitor = glfwGetWindowMonitor(window);
        if (game[GameOptions::FULLSCREEN] && !monitor)
            glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, game[GameOptions::RESOLUTION_X],
                                 game[GameOptions::RESOLUTION_Y], GLFW_DONT_CARE);
        else if (!game[GameOptions::FULLSCREEN] && monitor)
            glfwSetWindowMonitor(window, nullptr, 80, 80, 1280, 720, GLFW_DONT_CARE);
        if (!first)
            game.save(optionsFile);
    };
    applyOptions(true);

    struct Input {
        WorldView *view;
        Scene *scene;
        ui::Interface *interface;
        Renderer *renderer;
        Player *player;
        bool dragging = false;
        double lastX = 0, lastY = 0;
        bool dropOne = false; // screens §8.1.8: a left press over nothing with an item in hand
    } input{&view, &scene, &interface, &renderer, player ? &*player : nullptr};
    glfwSetWindowUserPointer(window, &input);
    glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int, int action, int) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        if (action == GLFW_RELEASE)
            return;
        ui::Interface &ui = *in->interface;
        // Screens §9: while the chat input is open, keys go to it.
        if (ui.chatOpen) {
            if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                if (!ui.chatInput.empty()) {
                    std::string line;
                    for (char16_t ch : ui.chatInput)
                        line += ch < 0x80 ? (char)ch : '?';
                    ui.chatSubmitted.push_back(line);
                }
                ui.chatInput.clear();
                ui.chatOpen = false;
            } else if (key == GLFW_KEY_BACKSPACE && !ui.chatInput.empty()) {
                ui.chatInput.pop_back();
            } else if (key == GLFW_KEY_ESCAPE) {
                ui.chatInput.clear();
                ui.chatOpen = false;
            }
            return;
        }
        // Character screens §1.2: these screens take no keys but the edits' backspace.
        if (ui.front != ui::Interface::Front::None) {
            if (key == GLFW_KEY_BACKSPACE)
                ui.frontType(0, true);
            return;
        }
        if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && ui.hud && !ui.wantsMouse()) {
            ui.chatOpen = true;
            return;
        }
        if (key == GLFW_KEY_M && action == GLFW_PRESS && !ui.startScreen && in->player) {
            ui.mapOpen = !ui.mapOpen; // screens §5.2.1
            return;
        }
        if (key == GLFW_KEY_C && action == GLFW_PRESS && !ui.startScreen && in->player) {
            ui.craftingOpen = !ui.craftingOpen; // crafting §4.1
            return;
        }
        if (key == GLFW_KEY_X && action == GLFW_PRESS && !ui.startScreen && in->player) {
            ui.craftingOpen = false; // skills §1.3: X toggles the skill window, closing crafting
            ui.toggleSkills();
            return;
        }
        if ((key == GLFW_KEY_B || key == GLFW_KEY_I) && action == GLFW_PRESS && !ui.startScreen && in->player) {
            // Screens §8.1.2: B or I toggles the inventory; opening it hides the system panel.
            ui.inventoryOpen = !ui.inventoryOpen;
            if (ui.inventoryOpen)
                ui.systemMenu = false;
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            // Screens §7, §11.2: Esc toggles the menu button row (and closes the map). On the
            // start screen it quits.
            if (in->interface->startScreen) {
                glfwSetWindowShouldClose(w, 1);
            } else {
                in->interface->mapOpen = false;
                in->interface->menuRow = !in->interface->menuRow;
            }
        } else if (key == GLFW_KEY_V && action == GLFW_PRESS && !in->interface->startScreen) {
            in->interface->allLifeBars = !in->interface->allLifeBars; // screens §5.4
        } else if (key == GLFW_KEY_R && action == GLFW_PRESS && in->player && in->player->combat().health <= 0) {
            in->player->revive(); // §5.8
        } else if (key == GLFW_KEY_O && !in->interface->startScreen) {
            in->interface->systemMenu = !in->interface->systemMenu; // O: the system panel
        } else if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
            const double hour = 3600000.0 * (key == GLFW_KEY_UP ? 1 : -1);
            in->scene->timeOfDayMs = std::fmod(in->scene->timeOfDayMs + hour + alpha::DAY_MS, alpha::DAY_MS);
        }
    });
    glfwSetCharCallback(window, [](GLFWwindow *w, unsigned int ch) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        if (in->interface->chatOpen && ch >= 32 && ch < 0x10000)
            in->interface->chatInput += (char16_t)ch;
        else if (in->interface->front != ui::Interface::Front::None && ch >= 32 && ch < 0x10000)
            in->interface->frontType((char16_t)ch, false);
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        ui::Interface &menu = *in->interface;
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && menu.mapOpen && !menu.startScreen &&
            !menu.systemMenu && !menu.optionsOpen) {
            menu.mapRelease(); // screens §5.2.6
            return;
        }
        if (action == GLFW_PRESS && menu.front != ui::Interface::Front::None) {
            double x, y;
            glfwGetCursorPos(w, &x, &y);
            const VkExtent2D size = in->renderer->size();
            menu.frontPress({(float)size.width, (float)size.height}, {(float)x, (float)y},
                            button == GLFW_MOUSE_BUTTON_LEFT ? 0 : 1);
            return;
        }
        if (action == GLFW_PRESS && (menu.inventoryOpen || menu.craftingOpen || menu.skillsOpen) && !menu.startScreen &&
            !menu.mapOpen &&
            !menu.systemMenu && !menu.optionsOpen) {
            // Screens §8.1.8: presses on the inventory's windows move items; a right press over
            // none of them closes it (§8.1.2).
            double x, y;
            glfwGetCursorPos(w, &x, &y);
            const VkExtent2D size = in->renderer->size();
            const int b = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 1
                          : button == GLFW_MOUSE_BUTTON_MIDDLE ? 2 : -1;
            if (b >= 0 && menu.inventoryPress({(float)size.width, (float)size.height}, {(float)x, (float)y}, b))
                return;
            if (b == 0 && menu.inventory && !menu.inventory->hand.empty() && !menu.menuRow) {
                in->dropOne = true;
                return;
            }
            if (b == 1) {
                menu.inventoryOpen = false;
                menu.craftingOpen = false;
                menu.skillsOpen = false;
                menu.benchOpen = false;
                menu.menuRow = false;
                return;
            }
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && menu.menuRow && !menu.systemMenu) {
            menu.menuRow = false; // §7: a right click off the interface closes the row
            return;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && menu.wantsMouse()) {
            double x, y;
            glfwGetCursorPos(w, &x, &y);
            const VkExtent2D size = in->renderer->size();
            switch (menu.click({(float)size.width, (float)size.height}, {(float)x, (float)y})) {
            case ui::Action::StartGame: // character screens §1.2: on to the characters
                menu.startScreen = false;
                menu.front = ui::Interface::Front::Characters;
                break;
            case ui::Action::StartMenu:
                menu.systemMenu = false;
                menu.startScreen = true;
                break;
            case ui::Action::Exit:
                glfwSetWindowShouldClose(w, 1);
                break;
            case ui::Action::SystemPanel:
                menu.systemMenu = !menu.systemMenu;
                break;
            case ui::Action::Options: // screens §6.1
                menu.optionsOpen = true;
                menu.systemMenu = false;
                menu.menuRow = false;
                break;
            case ui::Action::OptionsApply:
                menu.optionsToApply = true;
                break;
            case ui::Action::OptionsOk:
                menu.optionsToApply = true;
                menu.optionsOpen = false;
                break;
            case ui::Action::OptionsCancel: // closes only; the edits stay in the window (§6.4)
                menu.optionsOpen = false;
                break;
            case ui::Action::Inventory: // §7: the menu row's inventory button, as B
                if (in->player) {
                    menu.inventoryOpen = !menu.inventoryOpen;
                    if (menu.inventoryOpen)
                        menu.systemMenu = false;
                }
                break;
            case ui::Action::Crafting: // §7: the menu row's crafting button, as C
                if (in->player)
                    menu.craftingOpen = !menu.craftingOpen;
                break;
            case ui::Action::Skills: // §7: the menu row's skills button, as X
                if (in->player) {
                    menu.craftingOpen = false;
                    menu.toggleSkills();
                }
                break;
            case ui::Action::Help:
            case ui::Action::WorldMap:
            case ui::Action::None:
                break;
            }
            return;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            in->dragging = action == GLFW_PRESS;
        glfwGetCursorPos(w, &in->lastX, &in->lastY);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        const bool menus = in->interface->wantsMouse();
        if (in->player && !menus)
            in->player->look((float)(x - in->lastX), (float)(y - in->lastY)); // §7.2: look mode
        else if (in->dragging)
            in->view->turn((float)(x - in->lastX) * 0.25f, (float)(y - in->lastY) * 0.25f);
        in->lastX = x;
        in->lastY = y;
    });

    glfwSetScrollCallback(window, [](GLFWwindow *w, double, double dy) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        ui::Interface &ui = *in->interface;
        if (ui.mapOpen) {
            // §5.2.2: each notch up divides the target zoom by 0.9 (closer), down multiplies.
            ui.mapZoomTarget = std::clamp(dy > 0 ? ui.mapZoomTarget / 0.9f : ui.mapZoomTarget * 0.9f, 0.01f, 10.f);
            return;
        }
        if (in->player)
            in->player->wheel(dy > 0 ? 1 : dy < 0 ? -1 : 0);
    });

    // The built-in controller (Gamepad.h). Its menu, map and revive buttons go through the same
    // key, click and wheel handlers as the keyboard and mouse.
    Gamepad pad;
    {
        Config padConfig;
        if (!padConfig.load("AlphaVulkan.ini"))
            padConfig.load("config/AlphaVulkan.ini");
        pad.load(padConfig);
    }
    const GLFWkeyfun keyHandler = glfwSetKeyCallback(window, nullptr);
    glfwSetKeyCallback(window, keyHandler);
    const GLFWmousebuttonfun clickHandler = glfwSetMouseButtonCallback(window, nullptr);
    glfwSetMouseButtonCallback(window, clickHandler);
    const GLFWscrollfun wheelHandler = glfwSetScrollCallback(window, nullptr);
    glfwSetScrollCallback(window, wheelHandler);

    int status = 0;
    int framesUntilCapture = options.capture.empty() ? -1 : 16;
    double last = glfwGetTime();
    const double start = last;
    uint64_t simulationWorldCalls = 0, simulationInputFrames = 0;
    int32_t simulationMaxPiece = 0;
    double frameLeftoverMs = 0;
    uint32_t lightClockMs = 0;
    std::vector<DrawItem> items;
    std::vector<EffectVertex> effects; // weapon trails (trails §1.3)
    glm::dvec2 mapCursor(0); // the cursor last frame, for the map's drags
    // Interaction §1: what the crosshair picks each frame, a creature or a static (zone x,
    // zone y, index in the zone's list), and R's last state for its edge.
    uint64_t creatureTarget = 0;
    std::optional<std::tuple<int32_t, int32_t, size_t>> objectTarget;
    bool rWasDown = false;
    if (player)
        player->hitSource = [&](uint64_t id) -> const creatures::Combatant * {
            if (id == player->combat().id)
                return &player->combat();
            if (client.connected()) {
                const auto actor = otherFighters.find(id);
                return actor == otherFighters.end() ? nullptr : &actor->second.fighter;
            }
            for (const auto &npc : npcs.all())
                if (npc.fighter.id == id)
                    return &npc.fighter;
            return nullptr;
        };
    if (player) {
        npcs.onControlledHit = [&](creatures::HitRecord &hit) { mods.playerHit(hit.attacker, hit.kind, hit.amount); };
        player->onPeriodicHit = [&](const creatures::Buff &buff) {
            creatures::PlayerView local{player->combat(), player->body(), player->facing(),
                                       player->actionBefore(), false, false, true};
            npcs.periodicHit(local.fighter, local.body, buff, {&local, 1}, playerActionFrame,
                            [&](creatures::HitRecord &hit) { mods.playerHit(hit.attacker, hit.kind, hit.amount); });
        };
        player->onActionEffects = [&](int32_t beforeMs, int32_t dtMs) {
            auto &actor = player->combat();
            creatures::chargeSound(actor, beforeMs, dtMs, player->body().blocks(), playerShot);
            creatures::actionSounds(actor, beforeMs, (float)dtMs, player->body().blocks(),
                                    [&] { return worldRandom.draw(); }, playerShot.sounds);
            creatures::launch(actor, player->body(), actor.aim, beforeMs, (float)dtMs,
                              [&] { return worldRandom.draw(); }, playerLaunched, playerShot,
                              [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); });
            if (!client.connected()) {
                creatures::PlayerView local{actor, player->body(), player->facing(), beforeMs};
                npcs.dispatchPlayerActions({&local, 1}, dtMs,
                    [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); }, playerActionFrame);
            } else {
                auto &c = actor;
                // Server simulation §1, §6.1: online the player's own client decides its hits on
                // the creatures the server lists and sends them as packet 7.
                if (c.health > 0 && creatures::meleeAction(c.action) &&
                    creatures::strikeWindow(c, beforeMs, (float)dtMs)) {
                    for (const auto &[id, other] : receivedActors) {
                        creatures::Combatant target = other.fighter();
                        target.id = id;
                        target.health = other.health();
                        target.controller = other.controller();
                        if (const auto resident = otherFighters.find(id); resident != otherFighters.end())
                            target.buffs = resident->second.fighter.buffs;
                        if (target.health <= 0 || target.controller == 6 ||
                            (target.controller == 0 && !creatures::hostile(c, target)))
                            continue;
                        creatures::Body body;
                        for (int axis = 0; axis < 3; axis++)
                            body.position[axis] = other.position(axis);
                        body.size = other.appearance().box;
                        body.velocity = other.velocity();
                        body.intent = other.intent();
                        body.push = other.response();
                        if (std::find(c.struck.begin(), c.struck.end(), id) != c.struck.end() ||
                            !creatures::reachesTarget(c, player->body().blocks(), player->facing(), player->body().size,
                                                      body.blocks(), body.size))
                            continue;
                        const bool first = c.struck.empty();
                        c.struck.push_back(id);
                        c.hitLanded = true;
                        const auto rolls = [&] { return worldRandom.draw(); };
                        const creatures::HitInput in = creatures::strikeInput(c, player->body(), body, rolls);
                        const creatures::HitOutcome out = creatures::resolveHit(target, body.velocity, &c, in, rolls);
                        for (const auto &event : out.buffs) {
                            sendBuff(event);
                            auto [resident, created] = otherFighters.try_emplace(event.target);
                            if (created) {
                                resident->second.fighter = other.fighter();
                                resident->second.fighter.id = id;
                            }
                            creatures::applyBuff(resident->second.fighter, event.record);
                        }
                        for (const creatures::HitRecord &r : out.records) {
                            client.sendHit(net::Hit::from(r));
                            const glm::dvec3 at = glm::dvec3(r.position[0], r.position[1], r.position[2]) / 65536.0;
                            if (auto text = ui::hitText(r.amount, r.kind, r.critical, r.target == c.id, at))
                                interface.floating.push_back(*text);
                            mods.hitSeen(r.attacker, r.target, r.kind, r.amount);
                        }
                        for (const creatures::SoundEvent &e : out.sounds)
                            playAt(e.sound, e.position, e.volume, e.pitch);
                        bursts.insert(bursts.end(), out.bursts.begin(), out.bursts.end());
                        if (first && out.hitAccepted)
                            creatures::afterFirstTarget(c, in.area, rolls);
                    }
                }
            }
        };
    }
    const auto acceleratesClock = [](const creatures::Combatant &actor) {
        return actor.controller == 0 && actor.action == 84 && actor.health > 0;
    };
    const auto advanceWorldClock = [&](int32_t elapsed, bool accelerated) {
        lightClockMs += uint32_t(elapsed);
        const bool named = client.connected() || (session && !session->name().empty());
        npcs.setNamedWorld(named || options.pet); // --pet retains its explicit diagnostic world adapter
        // Explicit unsaved capture/viewer time remains a diagnostic override.
        if (!session && !client.connected() && options.hourOverride)
            return;
        WorldClock clock{uint32_t(scene.day), uint32_t(scene.timeOfDayMs)};
        // Admitted online worlds use a nonempty selection string. Native
        // reset/transition scheduling remains separate from this engine adapter.
        clock.advanceWorld(elapsed, accelerated, named);
        scene.day = std::bit_cast<int32_t>(clock.day);
        scene.timeOfDayMs = clock.timeOfDayMs;
        if (session)
            session->clock = clock;
        if (session && named && simulation::clockWriteDue(lightClockMs, elapsed))
            session->saveClock();
    };
    const auto frameActor = [&](uint64_t key) -> creatures::Combatant * {
        if (player && player->combat().id == key)
            return &player->combat();
        if (const auto remote = otherFighters.find(key); remote != otherFighters.end())
            return &remote->second.fighter;
        if (!client.connected())
            for (auto &npc : npcs.npcs())
                if (npc.fighter.id == key)
                    return &npc.fighter;
        return nullptr;
    };
    const auto applyReceivedRelationships = [&] {
        const auto received = client.takeRelationships();
        creatures::receiveRelationships(received, frameActor);
    };
    const auto refreshReceivedLinks = [&] {
        for (const auto &[key, remote] : otherFighters)
            creatures::refreshForwardLink(remote.fighter, frameActor);
    };
    // Online mode is an engine caller adapter, independent of the consumer's
    // supplied mode-zero rules. Reuse this world view across incoming frames.
    const creatures::CreditWorld creditWorld{frameActor,
        [&]() -> creatures::Combatant * { return player ? &player->combat() : nullptr; }, 1, {}};
    while (!glfwWindowShouldClose(window) && !restart) {
        glfwPollEvents();
        if (interface.optionsToApply) {
            interface.optionsToApply = false;
            game = interface.optionsCopy;
            applyOptions(false);
        }
        // §6.4: the FPS limit, a frame lasts at least minTimeStep milliseconds.
        if (const int32_t step = game[GameOptions::MIN_TIME_STEP]; step > 0)
            while ((glfwGetTime() - last) * 1000.0 < step)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        const double now = glfwGetTime();
        const float seconds = (float)(now - last);
        frameLeftoverMs += (now - last) * 1000.0;
        // GLFW supplies fractional seconds. Keep an integer-duration adapter
        // for this engine clock; native timeGetTime/mode scheduling stays open.
        const int32_t frameElapsedMs = int32_t(std::clamp(frameLeftoverMs, 0.0, double(INT32_MAX)));
        frameLeftoverMs -= double(frameElapsedMs);
        last = now;
        std::unique_lock<std::mutex> networkFrame;
        if (client.connected())
            networkFrame = client.lockFrame();
        if (const auto clock = client.takeClock()) {
            scene.day = std::bit_cast<int32_t>(clock->first);
            scene.timeOfDayMs = clock->second;
        }
        receivedActors = client.connected() ? client.others() : std::map<uint64_t, net::CreatureRecord>{};
        std::erase_if(otherFighters, [&](const auto &entry) { return !receivedActors.contains(entry.first); });
        for (const auto &[id, record] : receivedActors) {
            auto snapshot = record.fighter();
            snapshot.id = id;
            otherFighters[id].refresh(std::move(snapshot), frameElapsedMs);
        }
        mods.frame(seconds);
        auto held = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS ? 1.f : 0.f; };
        const bool menus = interface.wantsMouse();
        if (player) {
            // In look mode the cursor is hidden and the mouse turns the camera (§7.2).
            const int cursor = menus ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
            if (glfwGetInputMode(window, GLFW_CURSOR) != cursor)
                glfwSetInputMode(window, GLFW_CURSOR, cursor);
            // The pad: Start is Esc, Back the map, A while dead revives, the D-pad zooms; on a
            // menu a stick moves the cursor and A clicks; on the map the left stick pans and the
            // triggers zoom (Gamepad.h).
            pad.poll();
            if (pad.connected && !interface.typing()) {
                auto key = [&](int k) { keyHandler(window, k, 0, GLFW_PRESS, 0); };
                if (pad.pressed(Gamepad::START))
                    key(GLFW_KEY_ESCAPE);
                if (pad.pressed(Gamepad::BACK))
                    key(GLFW_KEY_M);
                if (pad.pressed(Gamepad::A) && player->combat().health <= 0)
                    key(GLFW_KEY_R);
                if (pad.pressed(Gamepad::UP))
                    wheelHandler(window, 0, 1);
                if (pad.pressed(Gamepad::DOWN))
                    wheelHandler(window, 0, -1);
                const float dt = seconds;
                if (interface.mapOpen) {
                    const float psi = glm::radians(180.f - view.camera(renderer.size()).yawDegrees);
                    const glm::vec2 v = pad.move * (600.f * dt / interface.mapZoom); // blocks
                    interface.mapPan += glm::dvec3(v.x * std::cos(psi) - v.y * std::sin(psi),
                                                   v.x * std::sin(psi) + v.y * std::cos(psi), 0);
                    const float zoom = std::pow(0.9f, (pad.leftTrigger - pad.rightTrigger) * dt * 10.f);
                    interface.mapZoomTarget = std::clamp(interface.mapZoomTarget * zoom, 0.01f, 10.f);
                    if (glm::dot(pad.look, pad.look) > 0)
                        player->look(pad.look.x * pad.lookSpeed * dt, -pad.look.y * pad.lookSpeed * dt);
                } else if (menus) {
                    const glm::vec2 stick = glm::length(pad.move) > glm::length(pad.look) ? pad.move : pad.look;
                    if (glm::dot(stick, stick) > 0) {
                        double cx, cy;
                        glfwGetCursorPos(window, &cx, &cy);
                        glfwSetCursorPos(window, cx + stick.x * pad.cursorSpeed * dt, cy - stick.y * pad.cursorSpeed * dt);
                    }
                    if (pad.pressed(Gamepad::A)) {
                        clickHandler(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
                        clickHandler(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
                    }
                    if (pad.pressed(Gamepad::B)) // B backs out of a menu, as Esc
                        key(GLFW_KEY_ESCAPE);
                }
            }
            const auto updatePlayerInterface = [&] {
                // Skills: what Learn committed goes to the player; the window sees the committed
                // values, and whether a trainer of the player's class stands within 4 blocks (§4.5).
                {
                    creatures::Combatant &c = player->combat();
                    if (std::exchange(interface.skillsLearnt, false)) {
                        for (size_t i = 0; i < c.skills.size(); i++)
                            c.skills[i] = interface.playerSkills[i];
                        c.specialization = (uint8_t)interface.playerSpecialization;
                    }
                    for (size_t i = 0; i < c.skills.size(); i++)
                        interface.playerSkills[i] = c.skills[i];
                    interface.playerSpecialization = c.specialization;
                    interface.manaCubes = manaCubes;
                    interface.trainerNear = [&] {
                        const glm::dvec3 me = player->body().blocks();
                        for (const creatures::Npc &n : npcs.all()) {
                            const glm::dvec3 d = n.body.blocks() - me;
                            if ((n.looks.flags & 0x40) && n.fighter.characterClass == c.characterClass && glm::dot(d, d) < 16)
                                return true;
                        }
                        return false;
                    };
                }
                // Crafting §2.2, §3.2, §5: recipes granted at each new level, the station test, the
                // craft timer and what a finished craft makes.
                {
                    creatures::Combatant &c = player->combat();
                    if (c.level != grantedLevel) {
                        grantedLevel = c.level;
                        creatures::grantLevel(inventory.recipes, c.level, c.characterClass, [&] { return worldRandom.draw(); });
                    }
                    interface.stationNear = [&](int32_t station) {
                        const glm::dvec3 me = player->body().blocks();
                        if (station == creatures::WATER) {
                            const glm::dvec3 feet = me - glm::dvec3(0, 0, player->body().size.z / 2);
                            return (player->body().contact & creatures::IN_WATER) != 0 ||
                                   view.held().read((int32_t)std::floor(feet.x), (int32_t)std::floor(feet.y),
                                                    (int32_t)std::floor(feet.z)).type() == worldgen::WATER;
                        }
                        const int32_t kind = creatures::stationKind(station);
                        const int32_t pzx = worldgen::floorDiv((int32_t)std::floor(me.x), worldgen::ZONE);
                        const int32_t pzy = worldgen::floorDiv((int32_t)std::floor(me.y), worldgen::ZONE);
                        for (int32_t zx = pzx - 1; zx <= pzx + 1; zx++)
                            for (int32_t zy = pzy - 1; zy <= pzy + 1; zy++)
                                if (const worldgen::Zone *zone = view.held().zone(zx, zy))
                                    for (const worldgen::StaticObject &o : zone->statics) {
                                        const glm::dvec3 d = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0 - me;
                                        if (o.kind == kind && glm::dot(d, d) < 16)
                                            return true;
                                    }
                        return false;
                    };
                    interface.stepCraft(seconds * 1000.f, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
                    if (const auto recipe = std::exchange(interface.finishedCraft, std::nullopt)) {
                        const creatures::CraftOutcome made =
                            creatures::craft(inventory, *recipe, [&] { return worldRandom.draw(); });
                        if (made.refused) {
                            interface.chatAdd("You can't carry more of these items.\n", {255, 51, 51});
                            audio.play("human-voice01.wav");
                        }
                        for (const int32_t sound : made.sounds)
                            if (const char *key = creatures::soundKey(sound))
                                audio.play(key);
                        if (made.made) {
                            interface.chatAdd("You receive ", {255, 255, 255});
                            interface.chatAdd(receivedText(*made.made, dictionary), creatures::nameColour(*made.made) * 255.f);
                            interface.chatAdd(".\n", {255, 255, 255});
                        }
                    }
                }
                // Screens §8.1: the inventory screen's view of the player, and what presses on it did.
                {
                    creatures::Combatant &c = player->combat();
                    ui::Interface::CharacterSheet &sheet = interface.sheet;
                    sheet.name = own.name();
                    sheet.level = c.level;
                    sheet.characterClass = c.characterClass;
                    sheet.specialization = c.specialization;
                    sheet.health = creatures::maxHealth(c);
                    sheet.armour = creatures::armour(c);
                    sheet.resistance = creatures::resistance(c);
                    sheet.critical = creatures::criticalChance(c);
                    sheet.tempo = creatures::attackSpeed(c);
                    sheet.regeneration = creatures::regeneration(c);
                    sheet.weaponRating = creatures::weaponRating(c);
                    sheet.armorRating = creatures::armorRating(c);
                    sheet.canAct = c.hitStun < 1;
                    for (const int32_t sound : interface.inventorySounds)
                        if (const char *key = creatures::soundKey(sound))
                            audio.play(key);
                    interface.inventorySounds.clear();
                    // Skills §1.3: when a window starts holding the input the interface plays 0x53
                    // (menu-open2.wav), when none holds it any more 0x54 (menu-close2.wav).
                    {
                        const bool holding = interface.inventoryOpen || interface.craftingOpen || interface.skillsOpen;
                        if (holding != windowHeld)
                            if (const char *key = creatures::soundKey(holding ? 0x53 : 0x54))
                                audio.play(key);
                        windowHeld = holding;
                    }
                    if (interface.equipmentChanged) {
                        wearInventory();
                        c.health = std::min(c.health, creatures::maxHealth(c));
                        interface.equipmentChanged = false;
                    }
                    if (interface.gliderEquipped) {
                        c.action = 0;
                        interface.gliderEquipped = false;
                    }
                    // §8.1.10: drinking (80) or eating (81) a consumable; the action step heals
                    // (item names §9). A bomb (subtype 7) is placed at the player's feet: by the
                    // server online (packet 6 kind 1), here offline (server §8 kind 1).
                    for (const creatures::ItemRecord &item : interface.itemsToUse)
                        if (creatures::itemSubtype(item) == 7) {
                            if (client.connected())
                                client.sendItemRequest(itemRequest(item, {0, 0, 0}, 1));
                            else
                                npcs.placeBomb(player->body().blocks(), creatures::itemLevel(item));
                        } else {
                            const uint8_t sub = creatures::itemSubtype(item);
                            c.action = sub == 1 || sub == 2 ? 80 : 81;
                            c.actionMs = 0;
                            c.held = creatures::Item::parse(item.data());
                        }
                    interface.itemsToUse.clear();
                }
                {
                    const auto block = [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); };
                    if (input.dropOne) {
                        input.dropOne = false;
                        if (const auto item = inventory.dropOne()) {
                            // Online the server drops it (packet 6 kind 6) and sends the zone's list.
                            if (client.connected()) {
                                client.sendItemRequest(itemRequest(*item, {0, 0, 0}, 6));
                            } else {
                                worldItems.drop(*item, player->body().blocks() - glm::dvec3(0, 0, player->body().size.z / 2),
                                                1, scene.day, block, [&] { return worldRandom.draw(); });
                                playAt(57, player->body().blocks(), 1, 1);
                            }
                        }
                    }
                }
            };
            const auto publishPlayerEffects = [&] {
                // Behaviour §16.3, §16.6: ability sounds and outgoing buff records at the player.
                const creatures::AbilityEffects effects = player->takeAbilityEffects();
                bursts.insert(bursts.end(), effects.bursts.begin(), effects.bursts.end());
                for (const creatures::SoundEvent &s : effects.placed)
                    playAt(s.sound, s.position, s.volume, s.pitch);
                for (const int32_t sound : effects.sounds) // at the player (audio §4 "P")
                    playAt(sound, player->body().blocks(), 1, 1);
                // §16.6: each buff's 40-byte record goes to the server as packet 8.
                for (const creatures::BuffEvent &b : effects.buffs)
                    sendBuff(b);
            };
            // Select concrete bindings once before all world pieces. Later
            // equipment, action-clock and skill changes do not rebuild this list.
            player->refreshAbilityBindings();
            playerActionFrame.reset();
            creatures::NpcFrame::Cursor npcEvents;
            bool firstWorldCall = true;
            simulation::clientFrame(frameElapsedMs, true, playerActionFrame,
                [&](int32_t elapsed, creatures::NpcFrame &aggregate) {
                    simulationWorldCalls++;
                    simulationMaxPiece = std::max(simulationMaxPiece, elapsed);
                    bool accelerated = acceleratesClock(player->combat());
                    for (const auto &[id, remote] : otherFighters)
                        accelerated |= acceleratesClock(remote.fighter);
                    for (const auto &npc : npcs.all())
                        accelerated |= acceleratesClock(npc.fighter);
                    creatures::regenerate(player->combat(), elapsed);
                    advanceWorldClock(elapsed, accelerated);
                    if (client.connected())
                        refreshReceivedLinks();
                    const auto advancePlayer = [&](int32_t actorElapsed) {
                        if (!client.connected()) // physics §4.4: hostile/inert creatures push the player
                            player->body().separate = npcs.pushOnPlayer(player->body());
                        player->body().targetInContact = npcs.hostileInContact(player->combat(), player->body());
                        player->advance(actorElapsed, client.connected());
                        publishPlayerEffects();
                        // Keep offline launch order in the same actor output list.
                        // Online sends remain at this actor's producer boundary.
                        creatures::Combatant &c = player->combat();
                        auto shot = std::exchange(playerShot, {});
                        auto launched = std::exchange(playerLaunched, {});
                        for (const auto &buff : shot.buffs)
                            sendBuff(buff);
                        for (creatures::Projectile &p : launched) {
                            if (p.kind == creatures::BOOMERANG_SHOT) // §2.3: the thrower's weapon scale
                                p.scale = player->appearance().weaponScale;
                            if (client.connected()) {
                                client.sendShoot(p.serialize());
                                projectiles.list.push_back(p);
                            } else {
                                aggregate.shots.push_back(p);
                            }
                        }
                        if (firstWorldCall)
                            for (const auto &record : client.takeShoots()) {
                                creatures::Projectile p = creatures::Projectile::parse(record.data());
                                if (p.shooter != c.id)
                                    projectiles.list.push_back(p);
                            }
                        for (const creatures::SoundEvent &s : shot.sounds)
                            playAt(s.sound, s.position, s.volume, s.pitch);
                        bursts.insert(bursts.end(), shot.bursts.begin(), shot.bursts.end());

                    };
                    if (client.connected())
                        creatures::visitReceivedActors(otherFighters, player->combat().id,
                            [&] { advancePlayer(elapsed); }, [&](net::RemoteFighter &remote) {
                                if (remote.fighter.health > 0)
                                    creatures::tickBuffs(remote.fighter, elapsed, false);
                            });
                    // World items §3: timers and landing sounds, a dropped item from the hand, and the
                    // zones whose items changed saved.
                    {
                        const auto block = [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); };
                        const int32_t dt = elapsed;
                        std::vector<creatures::SoundEvent> landed;
                        worldItems.step(dt, !client.connected(), block, [&] { return worldRandom.draw(); }, landed);
                        // Online: the zones round the player are asked for once a second (net server
                        // §4.3); their item lists and the player's gifts come back.
                        if (firstWorldCall && client.connected()) {
                            if (lastZoneRequest < 0 || now - lastZoneRequest >= 1) {
                                lastZoneRequest = now;
                                const glm::dvec3 me = player->body().blocks();
                                const int32_t zx = worldgen::floorDiv((int32_t)std::floor(me.x), worldgen::ZONE);
                                const int32_t zy = worldgen::floorDiv((int32_t)std::floor(me.y), worldgen::ZONE);
                                std::vector<std::pair<int32_t, int32_t>> wanted;
                                for (int32_t dx = -1; dx <= 1; dx++)
                                    for (int32_t dy = -1; dy <= 1; dy++)
                                        if (view.held().zone(zx + dx, zy + dy))
                                            wanted.push_back({zx + dx, zy + dy});
                                client.requestZones(wanted);
                                // Missions §6.3: the cell records of the player's region, asked for when
                                // the player enters it and while the map is open.
                                const std::pair<int32_t, int32_t> region{worldgen::floorDiv(zx, 64), worldgen::floorDiv(zy, 64)};
                                if (region != missionRegionAsked || interface.mapOpen) {
                                    missionRegionAsked = region;
                                    client.requestRegions({region});
                                }
                            }
                            for (const auto &record : client.takeMissionRecords()) {
                                missions.apply(record.data());
                                // Open points 1 §2: a completed mission's record shows the banner (every
                                // player with the region loaded sees it, as in Alpha).
                                int32_t cx = 0, cy = 0;
                                const MissionArea m = MissionArea::fromRecord(record.data(), cx, cy);
                                if (m.kind != 0 && m.state == 2)
                                    if (const worldgen::Location *L = cellLocation(view.terrainWorld(), cx, cy)) {
                                        interface.questComplete(ui::objectiveText(m, *L, dictionary));
                                        if (const char *key = creatures::soundKey(30))
                                            audio.play(key);
                                    }
                            }
                            for (const auto &[key, records] : client.takeZoneItems()) {
                                std::vector<WorldItem> list;
                                for (const auto &r : records)
                                    list.push_back(WorldItem::fromRecord(r.data()));
                                worldItems.replace(key.first, key.second, std::move(list));
                            }
                            for (const auto &record : client.takeStaticStates())
                                statics.apply(record.data());
                            // Block edits §4.1, §3 step 7: the server's edits are applied and forgotten.
                            std::vector<BlockEdit> edits;
                            for (const auto &record : client.takeBlockEdits())
                                edits.push_back(BlockEdit::parse(record.data()));
                            if (!edits.empty())
                                view.applyEdits(edits, false);
                            for (const auto &gift : client.takeGifts()) {
                                creatures::ItemRecord got;
                                std::memcpy(got.data(), gift.data(), 280);
                                receive(got);
                            }
                        }
                        for (const creatures::SoundEvent &s : landed)
                            playAt(s.sound, s.position, s.volume, s.pitch);
                        // Interaction §6.3, §6.4: the statics' timers, and seats freed.
                        std::map<StaticStates::Key, const worldgen::Zone *> held;
                        for (const auto &[key, zone] : view.held().zones())
                            held[key] = zone.get();
                        statics.step(dt,
                                     [&](uint64_t id, glm::dvec3 &at, glm::vec3 &intent, int32_t &action) {
                                         if (id == player->combat().id) {
                                             at = player->body().blocks();
                                             intent = player->body().intent;
                                             action = player->combat().action;
                                             return true;
                                         }
                                         for (const creatures::Npc &n : npcs.npcs()) // seated residents
                                             if (n.fighter.id == id && n.fighter.health > 0) {
                                                 at = n.body.blocks();
                                                 intent = n.body.intent;
                                                 action = n.fighter.action;
                                                 return true;
                                             }
                                         return false;
                                     },
                                     held);
                        // Traps (alpha-open-points-1.md §1): offline this game runs them; the hits land
                        // on the player at once and on creatures in the next creature step.
                        if (!client.connected()) {
                            std::vector<StaticStates::Creature> here{{player->combat().id, player->body().blocks(), 0}};
                            for (const creatures::Npc &n : npcs.all())
                                if (n.fighter.health > 0)
                                    here.push_back({n.fighter.id, n.body.blocks(), n.fighter.controller});
                            std::vector<creatures::HitRecord> trapHits;
                            std::vector<creatures::SoundEvent> trapSounds;
                            const worldgen::World &world = view.terrainWorld();
                            statics.traps(dt, scene.timeOfDayMs, here, held,
                                          [&](glm::dvec3 at) { return world.levelAt((int32_t)std::floor(at.x), (int32_t)std::floor(at.y)); },
                                          trapHits, trapSounds);
                            for (const creatures::HitRecord &h : trapHits) {
                                if (h.target == player->combat().id) {
                                    player->takeHit(h);
                                    if (auto text = ui::hitText(h.amount, h.kind, false, true, player->body().blocks()))
                                        interface.floating.push_back(*text);
                                } else {
                                    shotHitsOnNpcs.push_back(h);
                                }
                            }
                            for (const creatures::SoundEvent &e : trapSounds)
                                playAt(e.sound, e.position, e.volume, e.pitch);
                        }
                        // Changed zones are saved, items and states together (a zone save without its
                        // items would empty the zone when it loads again, world items §4.3).
                        std::set<WorldItems::Key> changedZones = worldItems.takeChanged();
                        for (const auto &key : statics.takeChanged())
                            changedZones.insert(key);
                        for (const auto &key : blockEdits.takeChanged())
                            changedZones.insert(key);
                        for (const auto &[zx, zy] : changedZones)
                            if (session && !client.connected())
                                if (const auto *list = worldItems.zone(zx, zy)) {
                                    std::vector<std::array<uint8_t, 325>> saved;
                                    for (const WorldItem &w : *list)
                                        saved.push_back(w.save());
                                    session->saveZoneItems(zx, zy, std::move(saved));
                                    session->saveZoneStates(zx, zy, statics.stateBytes(zx, zy));
                                    session->saveZoneEdits(zx, zy, blockEdits.saved(zx, zy));
                                }
                        std::vector<WorldItems::Key> gone;
                        for (const auto &[key, list] : worldItems.all())
                            if (!view.held().zone(key.first, key.second))
                                gone.push_back(key);
                        for (const auto &[zx, zy] : gone) {
                            worldItems.forget(zx, zy);
                            statics.forget(zx, zy);
                            blockEdits.forget(zx, zy);
                        }
                    }
                    // Offline the creatures live here (behaviour §13.1); online the server runs them.
                    if (!client.connected()) {
                        creatures::Combatant &c = player->combat();
                        const float before = c.level;
                        // Missions §2.1: the region's daily pass once a day (cleared at midnight and by
                        // inn rests), §2.6 the zone passes, §4 discovery, §5.4 leftover targets.
                        {
                            const worldgen::World &world = view.terrainWorld();
                            const glm::dvec3 me = player->body().blocks();
                            const std::vector<Missions::Player> players{{me, c.level}};
                            if (scene.day != missionDay) {
                                missionDay = scene.day;
                                missions.clearGenerated();
                            }
                            const int32_t rx = worldgen::floorDiv((int32_t)std::floor(me.x), worldgen::REGION);
                            const int32_t ry = worldgen::floorDiv((int32_t)std::floor(me.y), worldgen::REGION);
                            // §7: the region's saved cells, read the first time it is needed.
                            if (session && world.region(rx, ry) && missionRegionsRead.insert({rx, ry}).second) {
                                std::vector<std::string> keys;
                                for (int32_t i = 0; i < 64; i++)
                                    for (const char *kind : {"mission", "monster"})
                                        keys.push_back(kind + std::to_string(8 * rx + i / 8) + "_" + std::to_string(8 * ry + i % 8));
                                const auto saved = session->values(keys);
                                for (int32_t i = 0; i < 64; i++) {
                                    const std::string cell = std::to_string(8 * rx + i / 8) + "_" + std::to_string(8 * ry + i % 8);
                                    const auto mission = saved.find("mission" + cell);
                                    const auto monster = saved.find("monster" + cell);
                                    missions.restore(8 * rx + i / 8, 8 * ry + i % 8,
                                                     mission == saved.end() ? std::nullopt : MissionArea::parse(mission->second),
                                                     monster == saved.end() ? std::nullopt : MonsterArea::parse(monster->second));
                                }
                            }
                            if (world.region(rx, ry) && !missions.generated(rx, ry)) {
                                std::set<std::pair<int32_t, int32_t>> regionZones;
                                for (const auto &[key, zone] : view.held().zones())
                                    if (worldgen::floorDiv(key.first, 64) == rx && worldgen::floorDiv(key.second, 64) == ry)
                                        regionZones.insert(key);
                                npcs.removeTargets(regionZones);
                                missions.dailyPass(world, rx, ry, players, worldRandom);
                                zonesForMissions.insert(regionZones.begin(), regionZones.end());
                                for (int32_t k = 0; k < 64 && options.quest; k++)
                                    if (const Missions::Cell *cell = missions.cell(8 * rx + k / 8, 8 * ry + k % 8);
                                        cell && cell->mission.kind == 1) {
                                        const double x = 256.0 * cell->mission.targetZoneX + 100, y = 256.0 * cell->mission.targetZoneY + 100;
                                        player->body().place({x, y, view.groundHeight((int32_t)x, (int32_t)y) + 3});
                                        view.placeCamera({x, y, view.groundHeight((int32_t)x, (int32_t)y) + 3}, glm::vec3(0, 1, 0));
                                        std::fprintf(stderr, "--quest: cell (%d, %d), zone (%d, %d), type %d\n", 8 * rx + k / 8,
                                                     8 * ry + k % 8, cell->mission.targetZoneX, cell->mission.targetZoneY,
                                                     cell->mission.targetType);
                                        break;
                                    }
                            }
                            const auto ground = [&](int32_t x, int32_t y) {
                                const worldgen::Zone *z = view.held().zoneAt(x, y);
                                return z ? (float)z->column(x, y).top() : 0.f;
                            };
                            for (const auto &[zx, zy] : std::exchange(zonesForMissions, {}))
                                if (const worldgen::Zone *zone = view.held().zone(zx, zy))
                                    npcs.setExtraSpawns(zx, zy, zone->spawns.size(),
                                                        missions.zonePass(world, zx, zy, ground, players, worldRandom));
                            missions.discover(world, me);
                            for (creatures::Npc &n : npcs.npcs()) {
                                const auto [cx, cy] = cellOfZone(n.zone.first, n.zone.second);
                                const Missions::Cell *cell = missions.cell(cx, cy);
                                if ((n.looks.flags & 0x2000) && n.fighter.relations.threats.empty() && cell && cell->mission.state == 2)
                                    n.fighter.health = 0;
                            }
                            // §7: the saves, of every cell of a region whose pass ran and of changed cells.
                            if (session) {
                                std::set<std::pair<int32_t, int32_t>> cells = missions.takeChanged();
                                for (const auto &[i, j] : missions.takePassed())
                                    for (int32_t k = 0; k < 64; k++)
                                        cells.insert({8 * i + k / 8, 8 * j + k % 8});
                                std::map<std::string, std::vector<uint8_t>> values;
                                for (const auto &[cx, cy] : cells)
                                    if (const Missions::Cell *cell = missions.cell(cx, cy)) {
                                        const std::string name = std::to_string(cx) + "_" + std::to_string(cy);
                                        values["mission" + name] = cell->mission.save();
                                        values["monster" + name] = cell->monster.save();
                                    }
                                if (!values.empty())
                                    session->setValues(values);
                            }
                        }
                        if (options.extendedQuests) {
                            if (session && !questBookRead) {
                                questBookRead = true;
                                const auto saved = session->values({"av_quests"});
                                if (const auto it = saved.find("av_quests"); it != saved.end())
                                    questBook = quests::Book::load(it->second);
                                if (!questBook.quests.empty())
                                    trackedQuest = questBook.quests.back().id;
                            }
                            // The quest's creatures, made once the zone of its place is loaded, standing
                            // on the first open block over the ground.
                            const auto ground = [&](int32_t x, int32_t y) {
                                for (int32_t z = 400; z > 0; z--) {
                                    const uint8_t t = view.held().read(x, y, z - 1).type();
                                    if (t != 0 && t != 2)
                                        return z;
                                }
                                return 0;
                            };
                            for (quests::Quest &q : questBook.quests)
                                if (q.state == quests::ACTIVE && !npcs.hasQuestCreatures(q.id) &&
                                    view.held().zoneAt((int32_t)std::floor(q.site.x), (int32_t)std::floor(q.site.y))) {
                                    q.site.z = ground((int32_t)std::floor(q.site.x), (int32_t)std::floor(q.site.y));
                                    npcs.setQuestCreatures(q.id, quests::creatures(q, ground));
                                }
                            if (questBookChanged && session) { // the world's `av_quests` value
                                session->setValues({{"av_quests", questBook.save()}});
                                questBookChanged = false;
                            }
                        }
                        npcs.stepInto((float)elapsed, view.held(),
                            {{c, player->body(), player->facing(), player->actionBefore(), false, false, true}}, scene.timeOfDayMs, scene.day,
                            [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); },
                            aggregate, std::exchange(shotHitsOnNpcs, {}), {}, advancePlayer);
                        const auto happened = aggregate.addedSince(npcEvents);
                        projectiles.list.insert(projectiles.list.end(), happened.shots.begin(), happened.shots.end());
                        for (const auto &event : happened.buffs)
                            sendBuff(event);
                        // Server §6.4, loot rolls §3: what the dead leave behind.
                        for (const creatures::NpcFrame::Death &death : happened.deaths)
                            if (!death.lootApplied) // retained non-hit death adapter
                                dropLoot(lootVictim(death), worldItems, scene.day,
                                         [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); }, worldRandom);
                        for (const creatures::NpcFrame::Death &death : happened.deaths) // mod SDK v6
                            mods.creatureDeath(death.fighter.id, death.fighter.type, death.fighter.level, death.credited);
                        for (const creatures::NpcFrame::Death &death : happened.deaths) // quests: hunts and battles
                            if (death.quest && questBook.find(death.quest)) {
                                questBookChanged = true;
                                if (questBook.died(death.quest, death.questRole)) {
                                    const quests::Quest &q = *questBook.find(death.quest);
                                    interface.chatAdd("Quest done! Return to " + quests::giverName(q) + ".\n", {1, 0.8f, 0.2f});
                                    interface.questComplete(quests::objectiveText(q, q.giverAt));
                                    if (const char *key = creatures::soundKey(30))
                                        audio.play(key);
                                }
                            }
                        // Presentation and rewards retain this later engine adapter;
                        // fatal regional state/snapshots already changed synchronously.
                        const auto showMission = [&](const MissionProgress &progress) {
                            if (!progress.completed())
                                return;
                            mods.missionComplete(progress.x, progress.y);
                            if (const auto *location = cellLocation(view.terrainWorld(), progress.x, progress.y)) {
                                interface.questComplete(ui::objectiveText(progress.mission, *location, dictionary));
                                if (const char *key = creatures::soundKey(30))
                                    audio.play(key);
                            }
                            if (progress.victimRole == 1 && progress.contributor == c.id)
                                for (const auto &item : missionRewards(progress.mission, c.characterClass, worldRandom))
                                    receive(creatures::toRecord(item));
                        };
                        for (const auto &progress : happened.missions)
                            showMission(progress);
                        // Non-hit corpse deaths retain their separate legacy admission.
                        for (const creatures::NpcFrame::Death &death : happened.deaths) {
                            if (death.regionApplied)
                                continue;
                            if (const auto done = missions.kill(death.zone, death.appearanceFlags, death.boss)) {
                                for (int32_t zx = 8 * done->first; zx < 8 * done->first + 8; zx++)
                                    for (int32_t zy = 8 * done->second; zy < 8 * done->second + 8; zy++)
                                        if (const worldgen::Zone *zone = view.held().zone(zx, zy))
                                            npcs.setExtraSpawns(zx, zy, zone->spawns.size(), {}, false);
                                showMission({done->first, done->second, missions.cell(done->first, done->second)->mission,
                                             death.credited, death.fighter.controller});
                            }
                        }
                        // Travel §5.5: a pet standing on wet lake-bed ground (type 3) refills its
                        // owner's riding energy by 0.05 a second.
                        if (const creatures::Npc *pet = npcs.petOf(c); pet && !(pet->body.contact & creatures::IN_WATER)) {
                            const glm::dvec3 feet = pet->body.blocks() - glm::dvec3(0, 0, pet->body.size.z / 2 + 0.2);
                            if (view.held().read((int32_t)std::floor(feet.x), (int32_t)std::floor(feet.y), (int32_t)std::floor(feet.z)).type() ==
                                worldgen::LAKE_BED)
                                player->body().ridingEnergy = std::min(1.f, player->body().ridingEnergy + 0.05f * seconds);
                        }
                        for (const creatures::HitEmission &emission : happened.playerHits) {
                            if (!emission.applyLocally)
                                continue;
                            creatures::HitRecord hit = emission.record;
                            if (hit.kind == 6)
                                continue; // pets §7.3: a bite's signal, not damage
                            mods.playerHit(hit.attacker, hit.kind, hit.amount);
                            player->takeHit(hit);
                        }
                        for (const auto &[hit, at] : happened.seen) {
                            // Materials §1.5: a critical hit by a weapon with spirits flashes at the hit
                            // for 400 ms (radius 20).
                            if (hit.critical) {
                                const creatures::Combatant *by = hit.attacker == c.id ? &c : nullptr;
                                for (const creatures::Npc &n : npcs.all())
                                    if (n.fighter.id == hit.attacker)
                                        by = &n.fighter;
                                if (by)
                                    for (const creatures::Item *weapon : {&by->mainHand(), &by->offHand()})
                                        if (glm::vec3 a; weapon->type == 3 && creatures::averageSpiritColour(*weapon, a))
                                            flashes.append({hit.position, a, 20, 0, 400});
                            }
                            if (auto text = ui::hitText(hit.amount, hit.kind, hit.critical, hit.target == c.id, at))
                                interface.floating.push_back(*text);
                            mods.hitSeen(hit.attacker, hit.target, hit.kind, hit.amount);
                        }
                        for (const creatures::SoundEvent &s : happened.sounds)
                            playAt(s.sound, s.position, s.volume, s.pitch);
                        bursts.insert(bursts.end(), happened.bursts.begin(), happened.bursts.end());
                        const auto blockBursts = [&](const std::vector<BlockEdit> &removed) { // particles §5.2
                            for (const BlockEdit &e : removed) {
                                const worldgen::Block b = view.held().read(e.x, e.y, e.z);
                                bursts.push_back(creatures::blockBurst({e.x, e.y, e.z}, {b.r, b.g, b.b}));
                            }
                        };
                        // Pets §7.4: a creature tamed by this player turns the food in the Pet slot into
                        // its pet item; §12: the pet item follows the pet's level and experience.
                        for (const auto &t : happened.tamed)
                            if (t.owner == c.id && creatures::itemType(inventory.equipment[12]) == 20) {
                                creatures::ItemRecord pet{};
                                pet[0x00] = 19, pet[0x01] = (uint8_t)t.type;
                                const uint32_t experience = (uint32_t)t.experience;
                                std::memcpy(pet.data() + 0x04, &experience, 4);
                                creatures::setItemLevel(pet, (int16_t)t.level);
                                inventory.equipment[12] = pet;
                                c.action = 0;
                                wearInventory();
                            }
                        for (const auto &p : happened.petItems)
                            if (p.owner == c.id && creatures::itemType(inventory.equipment[12]) == 19) {
                                const uint32_t experience = (uint32_t)p.experience;
                                std::memcpy(inventory.equipment[12].data() + 0x04, &experience, 4);
                                creatures::setItemLevel(inventory.equipment[12], (int16_t)p.level);
                                wearInventory();
                            }
                        // Block edits §8: a bomb that went off takes the natural ground within 8 blocks.
                        for (const auto &blast : happened.explosions) {
                            const auto read = [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z); };
                            const auto removed = naturalSphere(blast.centre, 8, scene.day, read);
                            if (removed.empty())
                                continue;
                            blockBursts(removed);
                            view.applyEdits(removed, false);
                            for (const BlockEdit &e : removed)
                                blockEdits.add(e);
                            playAt(1, blast.centre, 1, 0.5f + 0.4f * (float)worldRandom.draw() / 32767.f);
                        }
                        // Block edits §9: big creatures' blows take the natural ground away.
                        for (const auto &impact : happened.craters) {
                            const auto read = [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z); };
                            const auto removed = naturalSphere(impact.centre, impact.radius, scene.day, read);
                            if (removed.empty())
                                continue;
                            blockBursts(removed);
                            view.applyEdits(removed, false);
                            for (const BlockEdit &e : removed)
                                blockEdits.add(e);
                            playAt(2, impact.centre, 1, 0.5f + 0.4f * (float)worldRandom.draw() / 32767.f);
                        }
                        if (happened.experience > 0) {
                            creatures::addExperience(c, happened.experience);
                            interface.floating.push_back(ui::experienceText(happened.experience, player->body().blocks()));
                            interface.chatAdd("You gain " + std::to_string(happened.experience) + " XP.\n", {150, 128, 255});
                        }
                        for (int32_t l = (int32_t)before + 1; l <= c.level; l++)
                            mods.levelUp(l);
                        if (c.level > before) {
                            audio.play("levelup2.wav");
                            interface.floating.push_back(ui::levelUpText(player->body().blocks()));
                        }
                    }
                    // Flight follows every creature countdown. Player launches were inserted
                    // before NPC launches; both participate in this frame in that order.
                    {
                        creatures::Combatant &c = player->combat();
                        // §4-§6: the flight, on whole milliseconds. Offline every shot's hits count;
                        // online only our own (the shooter's client decides, §6.2).
                        const int32_t dt = elapsed;
                        if (!projectiles.list.empty()) {
                            std::vector<creatures::Combatant> fighters;
                            std::vector<creatures::Body> bodies;
                            const auto &others = receivedActors;
                            fighters.reserve(others.size());
                            bodies.reserve(others.size());
                            for (const auto &[id, other] : others) {
                                creatures::Combatant f = other.fighter();
                                f.id = id;
                                f.health = other.health();
                                f.controller = other.controller();
                                if (const auto resident = otherFighters.find(id); resident != otherFighters.end())
                                    f.buffs = resident->second.fighter.buffs;
                                creatures::Body b;
                                for (int axis = 0; axis < 3; axis++)
                                    b.position[axis] = other.position(axis);
                                b.size = other.appearance().box;
                                b.velocity = other.velocity();
                                b.intent = other.intent();
                                b.push = other.response();
                                fighters.push_back(std::move(f));
                                bodies.push_back(b);
                            }
                            std::vector<creatures::ShotTarget> targets{{&c, &player->body()}};
                            for (size_t i = 0; i < fighters.size(); i++)
                                targets.push_back({&fighters[i], &bodies[i]});
                            if (!client.connected())
                                for (creatures::Npc &n : npcs.npcs())
                                    targets.push_back({&n.fighter, &n.body});
                            const auto source = [&](uint64_t id) -> creatures::Combatant * {
                                if (id == c.id)
                                    return &c;
                                if (!client.connected())
                                    for (creatures::Npc &n : npcs.npcs())
                                        if (n.fighter.id == id)
                                            return &n.fighter;
                                return nullptr;
                            };
                            creatures::ShotEffects flight;
                            projectiles.step(dt, targets, source, [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); },
                                             [&] { return worldRandom.draw(); }, flight, {client.connected(), &c});
                            for (const auto &event : flight.buffs)
                                sendBuff(event);
                            for (const creatures::HitEmission &emission : flight.hits) {
                                const creatures::HitRecord &h = emission.record;
                                glm::dvec3 at = player->body().blocks();
                                for (const creatures::ShotTarget &t : targets)
                                    if (t.fighter->id == h.target)
                                        at = t.body->blocks();
                                if (client.connected()) {
                                    if (h.attacker != c.id)
                                        continue;
                                    client.sendHit(net::Hit::from(h));
                                } else if (emission.applyLocally && h.target == c.id) {
                                    player->takeHit(h);
                                } else if (emission.applyLocally) {
                                    shotHitsOnNpcs.push_back(h);
                                }
                                if (auto text = ui::hitText(h.amount, h.kind, h.critical, h.target == c.id, at))
                                    interface.floating.push_back(*text);
                                mods.hitSeen(h.attacker, h.target, h.kind, h.amount);
                            }
                            for (const creatures::SoundEvent &s : flight.sounds)
                                playAt(s.sound, s.position, s.volume, s.pitch);
                            bursts.insert(bursts.end(), flight.bursts.begin(), flight.bursts.end());
                            for (const auto &s : flight.struck) { // particles §5.7
                                const worldgen::Block b = view.held().read(s.cell.x, s.cell.y, s.cell.z);
                                bursts.push_back(creatures::arrowBurst(s.at, {b.r, b.g, b.b}));
                            }
                        }
                    }
                    firstWorldCall = false;
                },
                [&](creatures::NpcFrame &) {
                    applyReceivedRelationships();
                    simulationInputFrames++;
                    player->readInput(window, frameElapsedMs, !menus && !interface.typing() && player->combat().health > 0,
                                      interface.mapOpen ? nullptr : &pad);
                    updatePlayerInterface();
                    publishPlayerEffects();
                    player->finishFrame(frameElapsedMs);
                });
        } else {
            // A missing controlled actor does not suppress the world call.
            // Preview bootstrap and native creature selection remain separate.
            playerActionFrame.reset();
            simulation::clientFrame(frameElapsedMs, true, playerActionFrame,
                [&](int32_t elapsed, creatures::NpcFrame &aggregate) {
                    simulationWorldCalls++;
                    simulationMaxPiece = std::max(simulationMaxPiece, elapsed);
                    bool accelerated = false;
                    for (const auto &[id, remote] : otherFighters)
                        accelerated |= acceleratesClock(remote.fighter);
                    advanceWorldClock(elapsed, accelerated);
                    if (client.connected())
                        refreshReceivedLinks();
                    for (auto &[id, remote] : otherFighters)
                        if (remote.fighter.health > 0)
                            creatures::tickBuffs(remote.fighter, elapsed, false);
                    const auto block = [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); };
                    worldItems.step(elapsed, !client.connected(), block, [&] { return worldRandom.draw(); }, aggregate.sounds);
                    if (!client.connected())
                        npcs.stepInto((float)elapsed, view.held(), {}, scene.timeOfDayMs, scene.day, block, aggregate);
                },
                [&](creatures::NpcFrame &aggregate) {
                    applyReceivedRelationships();
                    simulationInputFrames++;
                    view.move({held(GLFW_KEY_D) - held(GLFW_KEY_A), held(GLFW_KEY_W) - held(GLFW_KEY_S), held(GLFW_KEY_E) - held(GLFW_KEY_Q)},
                              seconds, held(GLFW_KEY_LEFT_SHIFT) > 0);
                    for (const auto &sound : aggregate.sounds)
                        playAt(sound.sound, sound.position, sound.volume, sound.pitch);
                    bursts.insert(bursts.end(), aggregate.bursts.begin(), aggregate.bursts.end());
                });
        }
        // A scripted flight east (--travel), then waiting for the zones it reached.
        if (travelled < options.travel) {
            const float step = std::min(8.f, options.travel - travelled);
            view.shift({step, 0, 0});
            travelled += step;
        }
        view.update();
        const bool settling = travelled < options.travel || (options.travel > 0 && view.busy());
        view.fillScene(scene);
        scene.seconds = now - start;
        const auto &motionSettings = options.worldOptions.motion;
        scene.leafSway = motionSettings.leaves
                             ? motion::leafOffset(scene.seconds * motionSettings.windSpeed, motionSettings.leafAmplitude)
                             : glm::vec2(0);
        scene.leafRange = motionSettings.leafRange;
        items = view.items((now - start) * 1000);
        effects.clear();
        // Screens §5.2.2: with the map open, the zoom eases to its target, a middle drag pans
        // (turned with the camera) and a left drag turns and tilts the camera; closed, the pan
        // resets. The world itself is not drawn under the map.
        {
            double cx, cy;
            glfwGetCursorPos(window, &cx, &cy);
            const float dx = (float)(cx - mapCursor.x), dy = (float)(cy - mapCursor.y);
            mapCursor = {cx, cy};
            if (interface.mapOpen && player) {
                interface.mapZoom += (interface.mapZoomTarget - interface.mapZoom) *
                                     (1.f - std::pow(0.996f, seconds * 1000.f));
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
                    const float psi = glm::radians(180.f - view.camera(renderer.size()).yawDegrees);
                    const glm::vec2 v = glm::vec2(dx, -dy) * (4.f / interface.mapZoom);
                    interface.mapPan += glm::dvec3(v.x * std::cos(psi) - v.y * std::sin(psi),
                                                   v.x * std::sin(psi) + v.y * std::cos(psi), 0);
                }
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
                    player->look(dx, dy);
                items.clear();
            } else {
                // §5.2.1: while the map is closed it forgets the pan, teleport mode and selection.
                interface.mapPan = glm::dvec3(0);
                interface.teleportMode = false;
                interface.selectedPortal = glm::ivec2(-1);
            }
        }
        // Particles §8: flame cubes round a creature during a healing stream (action 34, blue)
        // and while a fire explosion charges (87, 88, red to yellow), on the action's clock.
        // Models §7.3: a foot coming down at `feet` (blocks), softer on type-3 ground.
        const auto footstep = [&](glm::dvec3 feet) {
            const glm::ivec3 below = glm::ivec3(glm::floor(feet - glm::dvec3(0, 0, 0.5)));
            const auto u = [&] { return (float)worldRandom.draw() / 32767.f; };
            if (view.held().read(below.x, below.y, below.z).type() == 3)
                playAt(33 + worldRandom.draw() % 3, feet, 1, 1);
            else
                playAt(32, feet, 1, 1 + 0.2f * u());
        };
        const auto drawFlames = [&](const std::vector<creatures::FlameCube> &cubes, glm::dvec3 base) {
            for (const creatures::FlameCube &k : cubes) {
                glm::mat4 m = glm::translate(glm::mat4(1), view.toRender(base + glm::dvec3(k.position)));
                m = glm::rotate(m, glm::radians(k.turn.x), glm::vec3(1, 0, 0));
                m = glm::rotate(m, glm::radians(k.turn.y), glm::vec3(0, 1, 0));
                m = glm::rotate(m, glm::radians(k.turn.z), glm::vec3(0, 0, 1));
                m = glm::scale(m, glm::vec3(k.edge)) * glm::translate(glm::mat4(1), glm::vec3(0, 0, -0.5f));
                items.push_back({creatureView.cubeMesh(), m, false, Shading::World, glm::vec4(k.colour, 1)});
            }
        };
        // Particles §8, open points 3 §3: the flame cubes round a creature (at its centre,
        // `width` its box width): a healing stream (34, blue), a fire explosion's charge (87, 88:
        // four rings that collapse after the wind-up), a staff's charge (30-33: cones in front of
        // it, a second wider one for 31 and 33), and a burning creature (a damage-over-time buff:
        // eight green flames on its own buff clock).
        const double wallMs = (now - start) * 1000.0;
        const auto actionFlames = [&](const creatures::Combatant &c, float width, glm::dvec3 at) {
            const creatures::Phases P = creatures::phases(c, c.action);
            const float t = c.actionMs, P1 = (float)std::max(P.windUp, 1), D = (float)std::max(P.total(), 1);
            const glm::vec3 red{1, 0, 0}, yellow{1, 1, 0};
            if (c.action == 34 && t < D)
                drawFlames(creatures::flames(t, width, 0.1f, 20, {0, 0, 1}, {0.25f, 0.75f, 1}), at);
            if ((c.action == 87 || c.action == 88) && t < D) {
                const float p = t / P1, charge = p <= 1 ? p : std::max(20 - 19 * p, 0.f);
                const glm::dvec3 base = at - glm::dvec3(0, 0, 1 + 4 * charge);
                for (int j = 0; j < 4; j++)
                    drawFlames(creatures::flames(wallMs, 1 + 2 * j * charge, 0.5f * charge, 20 + 13 * j, red, yellow), base);
            }
            if (c.action >= 30 && c.action <= 33 && t < D) {
                const float p = t / P1, q = t / D;
                const bool after = t > P1;
                const float r = (after ? 4 : 2) * q, z = 0.4f * q, w = (after ? 0.2f : 0.1f) * p;
                const int n = after ? 86 : 43;
                const bool water = c.specialization == 1;
                const glm::vec3 a = water ? glm::vec3(0, 0.1f, 1) : red, b = water ? glm::vec3(0.5f, 0.7f, 1) : yellow;
                glm::vec3 d = c.aim;
                if (glm::dot(d, d) > 0)
                    d = glm::normalize(d); // (obj +0x138c, believed the aim direction)
                drawFlames(creatures::flames(wallMs, r, z, n, a, b, w), at + glm::dvec3(d) - glm::dvec3(0, 0, r / 2));
                if (c.action == 31 || c.action == 33)
                    drawFlames(creatures::flames(wallMs, 2 * r, z, 2 * n, a, b, w / 2), at + glm::dvec3(d) - glm::dvec3(0, 0, r));
            }
            for (const creatures::Buff &buff : c.buffs)
                if (buff.kind == creatures::DAMAGE_OVER_TIME) // (radius: the local player's box, as read)
                    drawFlames(creatures::flames(100000.0 - buff.timeMs, player ? player->body().size.x : width, 0.1f, 8,
                                                 {0, 1, 0}, {0.5f, 1, 0}),
                               at);
        };
        if (player) {
            player->draw(items, creatureView, seconds * 1000.f, &effects);
            actionFlames(player->combat(), player->body().size.x, player->body().blocks());
        }
        // World-list sources precede fading and creature sources. Read canonical
        // fixed positions directly; drawing a projectile model is independent.
        for (const creatures::Projectile &projectile : projectiles.list)
            if (projectile.kind == creatures::SPELL) {
                const terrain_lights::FixedPosition at{projectile.position[0], projectile.position[1], projectile.position[2]};
                scene.lights.push_back({view.toRenderFixed(at), 16, glm::vec3(.5f, .5f, .2f) * projectile.radius, at});
            }
        flashes.advance(frameElapsedMs);
        for (const auto &flash : flashes.entries())
            scene.lights.push_back({view.toRenderFixed(flash.position), flash.radius, flash.fadedColour(), flash.position});
        const auto creatureLights = [&](const creatures::Combatant &creature,
                                         const terrain_lights::FixedPosition &main, glm::dvec3 displayed,
                                         float height, float yaw) {
            const auto append = [&](const auto &sources) {
                for (const auto &source : sources)
                    if (source)
                        scene.lights.push_back({view.toRenderFixed(source->position), source->radius,
                                                source->colour, source->position});
            };
            // Displayed position is the engine's current draw adapter. Native
            // rendered-position production/interpolation remains unverified.
            if (const auto rendered = terrain_lights::fromBlocks(displayed)) {
                append(terrain_lights::yawSources(creature, *rendered, yaw, height));
                append(terrain_lights::bodySources(creature, main, *rendered));
                // The engine currently uses its displayed yaw for both angle
                // inputs. Native rendered-angle following remains separate.
                append(std::array{terrain_lights::actionSource(creature, *rendered, yaw)});
            }
            // Native float weapon attachments are not supplied by the engine pose
            // yet. Keep an explicit main-position fallback for those two sites.
            append(terrain_lights::equipmentSources(creature, main, {main, main}));
        };
        if (player) {
            const creatures::Body &body = player->body();
            creatureLights(player->combat(), {body.position[0], body.position[1], body.position[2]},
                           body.blocks() - glm::dvec3(0, 0, body.bob), player->appearance().box.z, player->facing());
        }
        for (creatures::Npc &npc : npcs.npcs()) {
            creatures::Motion motion;
            motion.velocity = npc.body.velocity;
            motion.onGround = (npc.body.contact & creatures::ON_GROUND) != 0;
            motion.inWater = (npc.body.contact & creatures::IN_WATER) != 0;
            motion.yawDegrees = npc.yaw;
            motion.intent = npc.body.intent;
            motion.running = (npc.body.mode & 0x40) != 0;
            motion.dodgeMs = (float)npc.body.dodgeMs;
            motion.fighter = &npc.fighter;
            glm::dvec3 at = npc.body.blocks();
            at.z -= npc.body.bob;
            // Particles §7.2: the dead are not drawn; they crumble once, a bomb with a flash.
            if (npc.fighter.health <= 0) {
                if (shattered.insert(npc.fighter.id).second) {
                    creatureView.shatter(npc.looks, npc.pose, motion, at, [&] { return worldRandom.draw(); },
                                         particles.list);
                    if (npc.fighter.type == 144)
                        flashes.append({{npc.body.position[0], npc.body.position[1], npc.body.position[2]},
                                        {1, 0.8f, 0.5f}, 16, 0, 800});
                }
                continue;
            }
            shattered.erase(npc.fighter.id);
            if (creatureView.draw(items, npc.looks, npc.pose, motion, view.toRender(at), seconds * 1000.f, &effects))
                footstep(npc.body.blocks() - glm::dvec3(0, 0, npc.body.size.z * 0.5));
            actionFlames(npc.fighter, npc.looks.box.x, at);
            creatureLights(npc.fighter, {npc.body.position[0], npc.body.position[1], npc.body.position[2]},
                           at, npc.looks.box.z, npc.yaw);
        }
        // Quests: the icon each giver shows (none when it has nothing for this player).
        // (Online the giver is one of the server's creatures: its home is its spawn's place.)
        const auto giverIcon = [&](uint64_t id, uint8_t controller, uint8_t characterClass, float health,
                                   glm::dvec3 home) -> std::optional<ui::Interface::QuestIcon> {
            if (!options.extendedQuests || controller != 3 || health <= 0)
                return std::nullopt;
            if (const quests::Quest *q = questBook.byGiver(id))
                return q->state == quests::DONE ? ui::Interface::QuestIcon::HandIn : ui::Interface::QuestIcon::Underway;
            if (!quests::isGiver(id, characterClass) || questBook.full())
                return std::nullopt;
            const quests::Quest offer = quests::offer(view.terrainWorld().seed(), id, 2, home, scene.day, 1, 0);
            if (std::find(questBook.handedIn.begin(), questBook.handedIn.end(), offer.id) != questBook.handedIn.end())
                return std::nullopt;
            return ui::Interface::QuestIcon::Offer;
        };
        const auto questIcon = [&](const creatures::Npc &n) {
            return client.connected() ? std::nullopt
                                      : giverIcon(n.fighter.id, n.fighter.controller, n.fighter.characterClass, n.fighter.health, n.home);
        };
        interface.questMarks.clear();
        if (player) {
            const glm::dvec3 me = player->body().blocks();
            for (const creatures::Npc &n : npcs.all())
                if (const auto icon = questIcon(n)) {
                    const glm::dvec3 d = n.body.blocks() - me;
                    if (glm::dot(d, d) < 80.0 * 80.0)
                        interface.questMarks.push_back({n.body.blocks() + glm::dvec3(0, 0, n.body.size.z * 0.5 + 0.8), *icon});
                }
            if (client.connected())
                for (const auto &[id, other] : receivedActors) {
                    const int32_t zx = (int32_t)((id >> 8) & 0xffff), zy = (int32_t)((id >> 24) & 0xffff);
                    const size_t index = (size_t)(id & 0xff);
                    const worldgen::Zone *zone = view.held().zone(zx, zy);
                    if (!zone || index >= zone->spawns.size())
                        continue;
                    const worldgen::Spawn &s = zone->spawns[index];
                    const glm::dvec3 home = glm::dvec3((double)s.x, (double)s.y, (double)s.z) / 65536.0;
                    const glm::dvec3 at = glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0;
                    const glm::dvec3 d = at - me;
                    if (glm::dot(d, d) < 80.0 * 80.0)
                        if (const auto icon = giverIcon(id, other.controller(), other.fighter().characterClass, other.health(), home))
                            interface.questMarks.push_back({at + glm::dvec3(0, 0, other.appearance().box.z * 0.5 + 0.8), *icon});
                }
            for (const quests::Quest &q : questBook.quests) // and over a quest's place while under way
                if (q.state == quests::ACTIVE && q.site.z > 0) {
                    const glm::dvec3 d = q.site - me;
                    if (glm::dot(d, d) < 150.0 * 150.0)
                        interface.questMarks.push_back({q.site + glm::dvec3(0, 0, 8), ui::Interface::QuestIcon::Underway});
                }
        }
        // Projectiles §7: arrows (model 840), the shooter's boomerang, rocks (2101); spells and
        // bursts have no model but light their surroundings. The model's longest side points
        // along the flight.
        for (const creatures::Projectile &p : projectiles.list) {
            const glm::vec3 at = view.toRender(p.blocks());
            if (p.kind == creatures::SPELL)
                continue;
            if (p.kind == creatures::BURST)
                continue;
            uint16_t model = p.kind == creatures::ROCK ? 2101 : 840;
            if (p.kind == creatures::BOOMERANG_SHOT && player && p.shooter == player->combat().id &&
                player->combat().mainHand().type == 3 && player->combat().mainHand().subtype == creatures::BOOMERANG)
                model = creatures::itemModel(player->combat().mainHand());
            const MeshId mesh = creatureView.plainMesh(model);
            const glm::vec3 size = creatureView.voxels(model);
            if (!mesh)
                continue;
            glm::mat3 turn(1);
            if (p.kind == creatures::ARROW && glm::dot(p.velocity, p.velocity) > 0) {
                const int a = size.x >= size.y && size.x >= size.z ? 0 : size.y >= size.z ? 1 : 2;
                const glm::vec3 f = glm::normalize(p.velocity);
                const glm::vec3 up = std::abs(f.z) < 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
                const glm::vec3 r = glm::normalize(glm::cross(up, f));
                turn[a] = f;
                turn[(a + 1) % 3] = r;
                turn[(a + 2) % 3] = glm::cross(f, r);
            } else {
                // Boomerangs spin as they fly, rocks tumble (a plain spin about z here).
                const float spin = glm::radians((p.kind == creatures::BOOMERANG_SHOT ? -1.5f : 0.5f) * (float)p.age);
                turn = glm::mat3(glm::rotate(glm::mat4(1), spin, glm::vec3(0, 0, 1)));
            }
            glm::mat4 world = glm::translate(glm::mat4(1), at) * glm::mat4(turn) *
                              glm::scale(glm::mat4(1), glm::vec3(p.scale / 14.f)) *
                              glm::translate(glm::mat4(1), glm::vec3(0, 0, -size.z / 2));
            items.push_back({mesh, world, false});
        }
        // Interaction §4.4: statics with a moving part, posed by how far open they are.
        if (player) {
            const glm::dvec3 me = player->body().blocks();
            for (const auto &[key, zone] : view.held().zones()) {
                const auto *states = statics.zone(key.first, key.second);
                for (size_t k = 0; states && k < zone->statics.size() && k < states->size(); k++) {
                    const worldgen::StaticObject &o = zone->statics[k];
                    if (!WorldView::movingStatic(o.kind))
                        continue;
                    const glm::dvec3 at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
                    if (glm::dot(at - me, at - me) > 200.0 * 200.0)
                        continue;
                    const auto [mesh, voxels] = view.staticModel(o.kind);
                    if (!mesh)
                        continue;
                    const float p = openFraction(o.kind, (*states)[k]);
                    const glm::vec3 size(o.scale[0], o.scale[1], o.scale[2]);
                    const glm::mat4 fit = glm::scale(glm::mat4(1), size / voxels); // model voxels -> the box
                    const glm::mat4 turn = glm::rotate(glm::mat4(1), glm::radians(90.f * (float)o.rotation), glm::vec3(0, 0, 1));
                    glm::mat4 part(1);
                    switch (o.kind) {
                    case 1: case 2: case 3: // about a hinge at one edge
                        part = glm::translate(glm::mat4(1), glm::vec3(-size.x / 2, 0, 0)) *
                               glm::rotate(glm::mat4(1), glm::radians(90.f * p), glm::vec3(0, 0, 1)) *
                               glm::translate(glm::mat4(1), glm::vec3(size.x / 2, 0, 0));
                        break;
                    case 5: part = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0.8f * size.z * p)); break;
                    case 7: part = glm::translate(glm::mat4(1), glm::vec3(0, 0, -(size.z * p) - 1)); break;
                    case 8: part = glm::translate(glm::mat4(1), glm::vec3(0, 0, size.z * p)); break;
                    case 9: part = glm::rotate(glm::mat4(1), glm::radians(60.f * p - 30.f), glm::vec3(1, 0, 0)); break;
                    default: break;
                    }
                    const glm::mat4 place = glm::translate(glm::mat4(1), view.toRender(at)) * turn;
                    items.push_back({mesh, place * part * fit, true});
                    if (o.kind == 10) // the chest's lid (kind 11's model), opened about its back edge
                        if (const auto [lid, lidVoxels] = view.staticModel(11); lid) {
                            const glm::mat4 hinge = glm::translate(glm::mat4(1), glm::vec3(0, size.y / 2, size.z)) *
                                                    glm::rotate(glm::mat4(1), glm::radians(80.f * p), glm::vec3(1, 0, 0)) *
                                                    glm::translate(glm::mat4(1), glm::vec3(0, -size.y / 2, -size.z));
                            items.push_back({lid, place * hinge * glm::scale(glm::mat4(1), size / lidVoxels), true});
                        }
                }
            }
        }
        // World items §2: each item's model in its pose, near enough to see.
        if (player) {
            const glm::dvec3 me = player->body().blocks();
            for (const auto &[key, list] : worldItems.all())
                for (size_t k = 0; k < list.size(); k++) {
                    const WorldItem &w = list[k];
                    if (w.hideMs != 0)
                        continue;
                    const glm::dvec3 d = w.blocks() - me;
                    if (glm::dot(d, d) > 160.0 * 160.0)
                        continue;
                    const uint16_t model = creatures::itemModel(creatures::Item::parse(w.item.data()));
                    if (model == creatures::NO_MODEL)
                        continue;
                    const MeshId mesh = creatureView.plainMesh(model);
                    if (!mesh)
                        continue;
                    const double clockMs = (now - start) * 1000.0;
                    const glm::mat4 world = glm::translate(glm::mat4(1), view.toRender(w.blocks())) *
                                            worldItemPose(w, creatureView.voxels(model), clockMs);
                    // §2.2, §2.3: lit by the light of its block; shimmer mushrooms and candles
                    // glow as if in daylight; coins sparkle (× 1 … 2 every 900 ms); the item
                    // target is lit 0.25 more.
                    const uint8_t type = creatures::itemType(w.item);
                    const uint8_t subtype = creatures::itemSubtype(w.item);
                    glm::vec4 material(1);
                    {
                        const glm::ivec3 cell = glm::ivec3(glm::floor(w.blocks()));
                        const uint8_t t = view.held().read(cell.x, cell.y, cell.z).type();
                        const int l = t == 13 ? 255 : (t != 0 && t != 2) ? 0 : std::max<int>(view.held().light(cell.x, cell.y, cell.z), 5);
                        const float S = alpha::sunlight(scene.timeOfDayMs);
                        material.a = (type == 11 && subtype == 19) || type == 18 ? 1 / std::max(S, 0.05f) : l / 255.f;
                    }
                    if (type == 12 || type == 13) {
                        const float pulse = 0.5f * std::cos(glm::radians(0.4f * (float)std::fmod(clockMs, 900.0))) + 0.5f;
                        material *= pulse * pulse + 1;
                    }
                    if (itemTarget && (*itemTarget)[0] == key.first && (*itemTarget)[1] == key.second &&
                        (*itemTarget)[2] == (int32_t)k)
                        material += glm::vec4(0.25f);
                    items.push_back({mesh, world, true, Shading::World, material});
                    // World items §2.5: the item's attached cubes, at their model cells, shining
                    // with an iron, gold, silver or ice item.
                    const glm::vec3 size = creatureView.voxels(model);
                    const uint8_t itemMaterial = creatures::itemMaterial(w.item);
                    const bool shine = itemMaterial == 1 || itemMaterial == 11 || itemMaterial == 12 || itemMaterial == 22;
                    for (const creatures::Item::Cube &cube : creatures::Item::parse(w.item.data()).cubes)
                        items.push_back({creatureView.cubeMesh(),
                                         world * glm::translate(glm::mat4(1), glm::vec3(cube.x + 0.5f - size.x / 2,
                                                                                        cube.y + 0.5f - size.y / 2, cube.z)),
                                         true, Shading::World, glm::vec4(creatures::spiritColour(cube.material), material.a),
                                         shine});
                }
        }
        if (client.connected()) {
            const auto &otherRecords = receivedActors;
            for (const auto &[id, other] : otherRecords) {
                const glm::vec3 p = view.toRender(glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0);
                net::RemoteFighter &f = otherFighters[id];

                creatures::Motion motion;
                motion.velocity = other.velocity();
                motion.yawDegrees = other.yaw();
                motion.fighter = &f.fighter;
                if (other.health() <= 0) { // particles §7.2
                    if (shattered.insert(id).second)
                        creatureView.shatter(other.appearance(), otherPoses[id], motion,
                                             glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0,
                                             [&] { return worldRandom.draw(); }, particles.list);
                    continue;
                }
                shattered.erase(id);
                const glm::dvec3 centre = glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0;
                if (creatureView.draw(items, other.appearance(), otherPoses[id], motion, p, seconds * 1000.f, &effects))
                    footstep(centre - glm::dvec3(0, 0, other.appearance().box.z * 0.5));
                actionFlames(f.fighter, other.appearance().box.x,
                             glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0);
                // Core fields are current and resident buffs stay on the actor.
                // Native interpolation and render scheduling remain separate.
                creatureLights(f.fighter, {other.position(0), other.position(1), other.position(2)},
                               centre, other.appearance().box.z, other.yaw());
            }
            if (player) {
                creatures::Combatant &c = player->combat();
                std::vector<creatures::BuffEvent> incomingHitUpdates;
                for (const net::Hit &h : client.takeHits()) {
                    // Pets §7.4: a bite (kind-6 hit) from a creature the server has made this
                    // player's pet turns the food in the Pet slot into its pet item.
                    if (h.target() == c.id && h.kind() == 6 && creatures::itemType(inventory.equipment[12]) == 20)
                        for (const auto &[id, other] : receivedActors) {
                            uint64_t owner = 0;
                            std::memcpy(&owner, &other.bytes[0x188], 8);
                            if (id != h.attacker() || other.controller() != 5 || owner != c.id)
                                continue;
                            const creatures::Combatant tamed = other.fighter();
                            creatures::ItemRecord pet{};
                            pet[0x00] = 19, pet[0x01] = (uint8_t)tamed.type;
                            const uint32_t experience = (uint32_t)tamed.experience;
                            std::memcpy(pet.data() + 0x04, &experience, 4);
                            creatures::setItemLevel(pet, (int16_t)tamed.level);
                            inventory.equipment[12] = pet;
                            c.action = 0;
                            wearInventory();
                        }
                    if (h.kind() == 6)
                        continue; // (a signal, not damage)
                    creatures::HitRecord hit = h.record();
                    if (hit.target == c.id) {
                        mods.playerHit(hit.attacker, hit.kind, hit.amount);
                        player->receiveHit(hit, &incomingHitUpdates);
                    }
                    if (hit.attacker != c.id) {
                        const glm::dvec3 at = glm::dvec3(hit.position[0], hit.position[1], hit.position[2]) / 65536.0;
                        if (auto text = ui::hitText(hit.amount, hit.kind, hit.critical, hit.target == c.id, at))
                            interface.floating.push_back(*text);
                        mods.hitSeen(hit.attacker, hit.target, hit.kind, hit.amount);
                    }
                }
                for (const auto &event : incomingHitUpdates)
                    sendBuff(event);
            }
            // Incoming credits retain their later caller phase, after hits.
            // Mode one is this engine's online-world adapter; the independent
            // mode-zero consumer is also used by supplied-state checks.
            for (const auto &credit : client.takeCredits()) {
                const int32_t previousLevel = player ? player->combat().level : 0;
                const auto result = creatures::receiveCredit(credit, creditWorld);
                if (result.healthZeroed)
                    for (const auto &[key, remote] : otherFighters)
                        if (&remote.fighter == result.healthZeroed) {
                            client.setReceivedHealth(key, remote.fighter.health);
                            if (const auto record = receivedActors.find(key); record != receivedActors.end())
                                record->second.setHealth(remote.fighter.health);
                            break;
                        }
                if (!player)
                    continue;
                const auto &c = player->combat();
                if (result.directAward == &c && credit.experience != 0) {
                    interface.floating.push_back(ui::experienceText(credit.experience, player->body().blocks()));
                    interface.chatAdd("You gain " + std::to_string(credit.experience) + " XP.\n", {150, 128, 255});
                }
                for (int64_t level = int64_t(previousLevel) + 1; level <= c.level; level++)
                    mods.levelUp(int32_t(level));
                if (c.level > previousLevel) {
                    audio.play("levelup2.wav");
                    interface.floating.push_back(ui::levelUpText(player->body().blocks()));
                }
            }
            const auto receivedBuffs = client.takeBuffs();
            if (!receivedBuffs.empty()) {
                const auto &currentRecords = receivedActors;
                for (const creatures::BuffEvent &event : receivedBuffs) {
                    if (player && event.target == player->combat().id) {
                        creatures::applyBuff(player->combat(), event.record);
                    } else if (const auto target = currentRecords.find(event.target); target != currentRecords.end()) {
                        auto [entry, created] = otherFighters.try_emplace(event.target);
                        if (created) {
                            auto &state = entry->second;
                            auto snapshot = target->second.fighter();
                            snapshot.id = event.target;
                            state.refresh(std::move(snapshot), 0);
                        }
                        creatures::applyBuff(entry->second.fighter, event.record);
                    }
                }
            }
            for (const auto &record : client.takeBursts())
                bursts.push_back(creatures::Burst::parse(record.data()));
            for (const net::Client::Sound &s : client.takeSounds())
                playAt(s.id, glm::dvec3(s.position), s.volume, s.pitch);
            // Screens §9.1: "name: " in cyan, the message in white.
            const auto &senders = receivedActors;
            if (auto book = client.takeQuestBook()) { // quests: the server keeps this player's book
                quests::Book received = quests::Book::load(*book);
                for (uint32_t id : received.handedIn) // (mod SDK v8: a quest newly handed in)
                    if (questBook.find(id))
                        mods.questComplete(id);
                questBook = std::move(received);
                if (!questBook.find(trackedQuest))
                    trackedQuest = questBook.quests.empty() ? 0 : questBook.quests.back().id;
            }
            for (const auto &[from, text] : client.takeChat()) {
                const auto sender = senders.find(from);
                interface.chatAdd((sender != senders.end() ? sender->second.name() : own.name()) + ": ", {0, 255, 255});
                interface.chatAdd(text + "\n", {255, 255, 255});
                mods.chat(from, text);
            }
        }
        // Screens §9: what was typed. /name renames the player; /namepet checks its length;
        // /connect and /disconnect need a restart here; anything else is said.
        for (const std::string &line : interface.chatSubmitted) {
            if (line.rfind("/name ", 0) == 0) {
                own.setName(line.substr(6));
            } else if (line.rfind("/namepet ", 0) == 0) {
                // Pets §15: the name goes into the pet item, one character (low byte) per entry;
                // the next pet pass rebuilds the pet with it.
                const std::vector<uint16_t> name = net::toUtf16(line.substr(9));
                if (name.size() >= 16) {
                    interface.chatAdd("Pet name too long\n", {255, 51, 51});
                } else if (creatures::itemType(inventory.equipment[12]) == 19) {
                    creatures::ItemRecord &pet = inventory.equipment[12];
                    const int32_t count = (int32_t)name.size();
                    std::memcpy(pet.data() + 0x114, &count, 4);
                    for (int32_t k = 0; k < count; k++) {
                        uint8_t *entry = pet.data() + 0x14 + 8 * k;
                        entry[0] = entry[1] = entry[2] = 0;
                        entry[3] = (uint8_t)name[(size_t)k];
                    }
                    wearInventory();
                }
            } else if (line.rfind("/connect", 0) == 0 || line == "/disconnect") {
                interface.chatAdd("Start AlphaVulkan with --connect to join a server.\n", {255, 51, 51});
            } else if (client.connected()) {
                client.say(line);
            } else {
                interface.chatAdd(own.name() + ": ", {0, 255, 255});
                interface.chatAdd(line + "\n", {255, 255, 255});
                mods.chat(0, line);
            }
        }
        interface.chatSubmitted.clear();
        // Behind the start screen the camera turns slowly round the spawn.
        if (interface.startScreen && !player)
            view.turn(seconds * 3.f, 0);
        // Character screens §7: behind the start, connect, character and world lists the game
        // camera eases to 10 blocks at pitch 100 and turns 5° a second; entering character or
        // world creation snaps it level at 5 blocks (yaw 0, or 180 for a world).
        if (player) {
            using F = ui::Interface::Front;
            creatures::FollowCamera &cam = player->followCamera();
            const F front = interface.front;
            if (interface.startScreen || front == F::Characters || front == F::Worlds || front == F::Connect) {
                cam.targetDistance = 10, cam.targetPitch = 100;
                cam.targetYaw += 0.005f * (float)(seconds * 1000.0);
            } else if ((front == F::Creation || front == F::WorldCreation) && front != cameraFront) {
                cam.targetPitch = cam.pitch = 90;
                cam.targetYaw = cam.yaw = front == F::Creation ? 0.f : 180.f;
                cam.targetDistance = cam.distance = 5;
            }
            cameraFront = front;
        }
        std::optional<terrain_lights::FixedPosition> lightController;
        if (player) {
            const auto &position = player->body().position;
            lightController = terrain_lights::FixedPosition{position[0], position[1], position[2]};
        }
        view.finishLighting(scene, worldItems, std::bit_cast<int32_t>(lightClockMs), lightController);
        FrameConstants frame = makeFrame(view.camera(renderer.size()), scene, shadowSettings, moonSettings);
        // Screens §5.1: the corner map while playing, 3 x 3 zones round the player's at the
        // starting map zoom (s 0.8).
        if (player && !interface.startScreen) {
            const glm::dvec3 at = player->body().blocks();
            view.discover(at); // §5.1.6
            if (now - lastRegionPass > 1) {
                regionPass(at);
                lastRegionPass = now;
            }
            const VkExtent2D sz = renderer.size();
            const float W = (float)sz.width, H = (float)sz.height, yaw = view.camera(sz).yawDegrees;
            // The corner map (§5.1) or, with M, the world map (§5.2.3): centred on the player
            // and the pan, at the map zoom, tilted with the camera but never flatter than 20°.
            const bool worldMap = interface.mapOpen;
            interface.compass = {creatureView.plainMesh(2559), creatureView.voxels(2559)};
            interface.compassYaw = 180.f - yaw; // Alpha's ψ
            // §5.1.2: the corner map's view scale follows the map zoom (0.025 / 0.8 / 1.6); the
            // world map takes the zoom itself. Below 0.35 both draw land pictures.
            const float d = interface.mapZoom;
            const float s = worldMap ? d : d < 0.35f ? 0.025f : d < 2 ? 0.8f : 1.6f;
            const bool lands = s < 0.35f;
            const glm::dvec3 pivot = worldMap ? at + interface.mapPan : at;
            frame.minimap = worldMap ? mapMatrix(view.toRender(pivot), yaw, std::max(player->cameraPitch(), 110.f), s, W, H,
                                                 {W / 2, H / 2})
                                     : minimapMatrix(view.toRender(at), yaw, s, W, H);
            frame.minimapTint = glm::vec4(1);
            const int32_t zx = (int32_t)std::floor(pivot.x / 256), zy = (int32_t)std::floor(pivot.y / 256);
            std::vector<DrawItem> map;
            if (lands) {
                map = view.landItems(pivot, worldMap ? 16 : 0);
            } else if (worldMap) {
                map = view.mapItems(zx - 16, zy - 16, zx + 16, zy + 16, seconds * 1000.f, true);
            } else if (s > 1) {
                // 2 x 2 zones: the player's and the neighbours towards the nearer edges.
                const int32_t nx = at.x - 256.0 * zx < 128 ? zx - 1 : zx + 1, ny = at.y - 256.0 * zy < 128 ? zy - 1 : zy + 1;
                map = view.mapItems(std::min(zx, nx), std::min(zy, ny), std::max(zx, nx), std::max(zy, ny), seconds * 1000.f, false);
            } else {
                map = view.mapItems(zx - 1, zy - 1, zx + 1, zy + 1, seconds * 1000.f, false);
            }
            // §5.1.2: player markers, head then hair, a black outline pass at 5/s without depth
            // writes and a coloured pass at 4/s, nearest last.
            struct Marker {
                const creatures::Appearance *looks;
                glm::dvec3 at;
                float yaw;
            };
            std::vector<Marker> markers;
            markers.push_back({&player->appearance(), at, player->facing()});
            for (const auto &[id, other] : receivedActors)
                otherLooks[id] = other.appearance();
            for (const auto &[id, other] : receivedActors) {
                const glm::dvec3 p = glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0;
                if (std::abs(p.x - at.x) <= 256 && std::abs(p.y - at.y) <= 256)
                    markers.push_back({&otherLooks[id], p, other.yaw()});
            }
            auto depth = [&](const Marker &m) {
                const glm::vec4 c = frame.minimap * glm::vec4(view.toRender(m.at), 1);
                return c.z / c.w;
            };
            std::sort(markers.begin(), markers.end(), [&](const Marker &a, const Marker &b) { return depth(a) > depth(b); });
            for (const Marker &mk : markers)
                for (int pass = 0; pass < 2; pass++)
                    for (creatures::Part part : {creatures::Part::Head, creatures::Part::Hair}) {
                        const uint16_t slot = mk.looks->model(part);
                        const MeshId mesh = slot == creatures::NO_MODEL ? 0 : creatureView.plainMesh(slot);
                        if (!mesh)
                            continue;
                        const glm::vec3 size = creatureView.voxels(slot);
                        glm::mat4 m = glm::translate(glm::mat4(1), view.toRender(mk.at));
                        m = glm::rotate(m, glm::radians(mk.yaw), glm::vec3(0, 0, 1));
                        m = glm::scale(m, glm::vec3((pass == 0 ? 5.f : 4.f) / s));
                        m = glm::translate(m, glm::vec3(0, 0, -size.z * 0.5f));
                        glm::vec4 material(pass == 0 ? glm::vec3(0) : glm::vec3(1), 2);
                        if (pass == 1 && part == creatures::Part::Hair)
                            material = glm::vec4(mk.looks->hairColour[0] / 255.f, mk.looks->hairColour[1] / 255.f,
                                                 mk.looks->hairColour[2] / 255.f, 2);
                        map.push_back({mesh, m, false, pass == 0 ? Shading::MapOutline : Shading::Map, material});
                    }
            interface.mapLabels.clear();
            interface.mapQuestMarks.clear();
            interface.hoveredPortal = glm::ivec2(-1);
            if (!worldMap && options.extendedQuests) {
                // Quests on the corner map: the same marks as on the world map, kept inside
                // the map's round area (150 px from its centre at (W - 240, 170)).
                const auto corner = [&](glm::dvec3 q) -> std::optional<glm::vec2> {
                    const glm::vec4 c = frame.minimap * glm::vec4(view.toRender(q), 1);
                    if (c.w <= 0)
                        return std::nullopt;
                    const glm::vec2 n = glm::vec2(c) / c.w;
                    const glm::vec2 p((n.x + 1) * 0.5f * W, (n.y + 1) * 0.5f * H);
                    if (glm::length(p - glm::vec2(W - 240, 170)) > 150)
                        return std::nullopt;
                    return p;
                };
                for (const creatures::Npc &n : npcs.all())
                    if (const auto icon = questIcon(n))
                        if (const auto p = corner(n.home))
                            interface.mapQuestMarks.push_back({*p, *icon});
                for (const quests::Quest &q : questBook.quests) {
                    if (const auto p = corner(q.giverAt))
                        interface.mapQuestMarks.push_back(
                            {*p, q.state == quests::DONE ? ui::Interface::QuestIcon::HandIn : ui::Interface::QuestIcon::Underway});
                    if (q.state == quests::ACTIVE)
                        if (const auto p = corner(glm::dvec3(q.site.x, q.site.y, q.site.z > 0 ? q.site.z : q.giverAt.z)))
                            interface.mapQuestMarks.push_back({*p, ui::Interface::QuestIcon::Underway});
                }
            }
            if (worldMap) {
                const worldgen::World &world = view.terrainWorld();
                const float myPower = creatures::levelCurve(player->combat().level);
                // §5.2.4: town markers (`city`) over towns more than 512 blocks away, scaled
                // 8/s within [2, 10], centred, 100 blocks above the ground.
                const float k = std::clamp(8.f / s, 2.f, 10.f);
                if (const worldgen::Region *region = world.region(zx >> 6, zy >> 6))
                    for (const worldgen::Location &L : region->records) {
                        if (L.type != 1)
                            continue;
                        const glm::dvec3 centre((double)L.x / 65536.0, (double)L.y / 65536.0, L.height + 100.0);
                        const glm::dvec2 d = glm::dvec2(centre) - glm::dvec2(at);
                        if (glm::dot(d, d) <= 262144.0)
                            continue;
                        if (const MeshId city = creatureView.plainMesh(2561))
                            map.push_back({city, glm::scale(glm::translate(glm::mat4(1), view.toRender(centre)), glm::vec3(k)),
                                           false, Shading::Map, glm::vec4(1)});
                    }
                // §5.2.4: mission markers over every unfinished mission of the region, at the
                // hunt's target zone or the location's centre, 100 blocks up, turning 20° a second.
                if (const worldgen::Region *region = world.region(zx >> 6, zy >> 6))
                    for (int32_t slot = 0; slot < 64; slot++) {
                        const Missions::Cell *cell = missions.cell(8 * (zx >> 6) + slot / 8, 8 * (zy >> 6) + slot % 8);
                        if (!cell || cell->mission.kind == 0 || cell->mission.state == 2)
                            continue;
                        const worldgen::Location &L = region->records[(size_t)slot];
                        glm::dvec3 centre((double)L.x / 65536.0, (double)L.y / 65536.0, L.height + 100.0);
                        if (cell->mission.kind == 1)
                            centre.x = 256.0 * cell->mission.targetZoneX + 128, centre.y = 256.0 * cell->mission.targetZoneY + 128;
                        if (const MeshId marker = creatureView.plainMesh(2560)) {
                            glm::mat4 m = glm::translate(glm::mat4(1), view.toRender(centre));
                            m = glm::rotate(m, glm::radians((float)std::fmod(glfwGetTime() * 20.0, 360.0)), glm::vec3(0, 0, 1));
                            map.push_back({marker, glm::scale(m, glm::vec3(k)), false, Shading::Map, glm::vec4(1)});
                        }
                    }
                // §5.2.5: labels at points of the map, through the map's own matrix.
                auto pixel = [&](glm::dvec3 q) -> std::optional<glm::vec2> {
                    const glm::vec4 c = frame.minimap * glm::vec4(view.toRender(q), 1);
                    if (c.w <= 0)
                        return std::nullopt;
                    const glm::vec2 n = glm::vec2(c) / c.w;
                    if (std::abs(n.x) > 1 || std::abs(n.y) > 1)
                        return std::nullopt;
                    return glm::vec2((n.x + 1) * 0.5f * W, (n.y + 1) * 0.5f * H);
                };
                // Quests: the givers in reach and every quest's giver and place.
                for (const creatures::Npc &n : npcs.all())
                    if (const auto icon = questIcon(n))
                        if (const auto p = pixel(n.home))
                            interface.mapQuestMarks.push_back({*p, *icon});
                for (const quests::Quest &q : questBook.quests) {
                    if (const auto p = pixel(q.giverAt))
                        interface.mapQuestMarks.push_back(
                            {*p, q.state == quests::DONE ? ui::Interface::QuestIcon::HandIn : ui::Interface::QuestIcon::Underway});
                    if (q.state == quests::ACTIVE)
                        if (const auto p = pixel(glm::dvec3(q.site.x, q.site.y, q.site.z > 0 ? q.site.z : q.giverAt.z)))
                            interface.mapQuestMarks.push_back({*p, ui::Interface::QuestIcon::Underway});
                }
                std::set<const worldgen::Location *> named;
                for (int32_t i = zx - 32; i < zx + 32; i++)
                    for (int32_t j = zy - 32; j < zy + 32; j++) {
                        if (!view.discovered(i, j))
                            continue;
                        const int32_t x = 256 * i + 128, y = 256 * j + 128;
                        // Pass A, from zoom 2: district and portal names.
                        if (s > 2)
                            if (const worldgen::ZoneEntry *E = world.zoneEntry(i, j); E && E->kind != 0) {
                                const std::string text = ui::zoneDetail(world, i, j, dictionary);
                                glm::vec4 colour(1);
                                if (E->kind == 3) {
                                    const int32_t lv = player->combat().level;
                                    colour = E->level > lv + 2 ? glm::vec4(1, 0.2f, 0.2f, 1)
                                             : E->level >= lv - 2 ? glm::vec4(0.2f, 1, 0.2f, 1)
                                                                  : glm::vec4(1);
                                }
                                const auto p = text.empty() ? std::nullopt : pixel({x, y, view.groundHeight(x, y)});
                                // Portals in teleport mode: purple, pink when selected, cyan
                                // under the cursor (which then becomes the hovered portal).
                                if (p && E->kind == 4 && interface.teleportMode) {
                                    const glm::vec2 c = glm::vec2(mapCursor);
                                    colour = glm::vec4(0.6f, 0.25f, 1, 1);
                                    if (interface.selectedPortal == glm::ivec2(i, j))
                                        colour = glm::vec4(1, 0.25f, 1, 1);
                                    if (c.x >= p->x - 100 && c.x < p->x + 100 && c.y >= p->y - 20 && c.y < p->y + 10) {
                                        colour = glm::vec4(0.2f, 1, 1, 1);
                                        interface.hoveredPortal = {i, j};
                                    }
                                }
                                if (p)
                                    interface.mapLabels.push_back({*p, ui::utf16(text), colour, 10});
                            }
                        // Pass B: the name of a place once a zone inside it is discovered.
                        const worldgen::Location *L = world.location(x, y);
                        if (!L || L->type == 0 || L->type == 10 || named.count(L))
                            continue;
                        const float f = world.locationDistanceFx(*L, (int64_t)x << 16, (int64_t)y << 16);
                        if (f >= 1)
                            continue;
                        named.insert(L);
                        const float p = creatures::levelCurve(L->level);
                        const glm::vec4 colour = L->type == 1           ? glm::vec4(1)
                                                 : p > myPower + 0.1f   ? glm::vec4(1, 0.2f, 0.2f, 1)
                                                 : p > myPower - 0.1f   ? glm::vec4(0, 1, 1, 1)
                                                                        : glm::vec4(1);
                        const double lx = (double)L->x / 65536.0, ly = (double)L->y / 65536.0;
                        const std::string text = ui::locationName(*L, dictionary);
                        if (!text.empty())
                            if (const auto px = pixel({lx, ly, view.groundHeight((int32_t)lx, (int32_t)ly)}))
                                interface.mapLabels.push_back({*px, ui::utf16(text), colour, 12});
                    }
            }
            // §5.2.6: "Teleport" at the cursor over the selected portal; the second click there
            // moves the player to the centre of the portal's zone. The game puts it at height 0
            // and lets physics lift it out of the ground; the engine sets it on the ground.
            interface.cursorLabel = interface.mapOpen && interface.teleportMode &&
                                            interface.selectedPortal != glm::ivec2(-1) &&
                                            interface.selectedPortal == interface.hoveredPortal
                                        ? u"Teleport"
                                        : u"";
            if (interface.teleportTo) {
                const int32_t x = 256 * interface.teleportTo->x + 128, y = 256 * interface.teleportTo->y + 128;
                player->body().place(glm::dvec3(x, y, view.groundHeight(x, y) + 1));
                player->body().velocity = glm::vec3(0);
                interface.teleportTo.reset();
            }
            renderer.setMinimap(std::move(map));
        } else {
            renderer.setMinimap({});
        }
        {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            if (options.cursorX >= 0)
                mx = options.cursorX, my = options.cursorY;
            const VkExtent2D size = renderer.size();
            // World points to window pixels with this frame's camera (Vulkan's clip y points down).
            interface.project = [&](glm::dvec3 at) -> std::optional<glm::vec2> {
                const glm::vec4 clip = frame.viewProjection * glm::vec4(view.toRender(at), 1);
                if (clip.w <= 0)
                    return std::nullopt;
                const glm::vec2 ndc = glm::vec2(clip) / clip.w;
                return glm::vec2((ndc.x + 1) * 0.5f * (float)size.width, (ndc.y + 1) * 0.5f * (float)size.height);
            };
            if (player) {
                // Screens §5.3: what the player's bars show.
                const creatures::Combatant &c = player->combat();
                ui::HudState hud;
                hud.name = own.name();
                hud.health = c.health;
                hud.maxHealth = creatures::maxHealth(c);
                hud.level = c.level;
                hud.characterClass = c.characterClass;
                hud.experience = c.experience;
                hud.experienceNeeded = creatures::experienceForLevel(c.level);
                hud.mana = c.mana;
                hud.charge = c.charge;
                hud.stamina = player->body().stamina;
                // Screens §5.3: the pet's bars (its name from the pet item, else its species).
                if (const creatures::Npc *pet = npcs.petOf(c)) {
                    hud.hasPet = true;
                    const DictionaryEntry *species = creatures::speciesKey(pet->fighter.type)
                                                         ? dictionary.find("name", creatures::speciesKey(pet->fighter.type))
                                                         : nullptr;
                    hud.petSpecies = species && species->field("singular") ? *species->field("singular") : std::string();
                    hud.petName.clear();
                    for (const creatures::Item::Cube &ch : c.equipment[12].cubes)
                        hud.petName += (char)ch.material;
                    if (hud.petName.empty())
                        hud.petName = hud.petSpecies;
                    hud.petHealth = pet->fighter.health;
                    hud.petMaxHealth = creatures::maxHealth(pet->fighter);
                    hud.petLevel = pet->fighter.level;
                    hud.petExperience = pet->fighter.experience;
                    hud.petExperienceNeeded = creatures::experienceForLevel(pet->fighter.level);
                    hud.ridingEnergy = player->body().ridingEnergy;
                }
                // §5.5, behaviour §16: the bar's slots, grey when not usable now, with the
                // cooldown left over its length.
                const auto slots = player->abilityBindings();
                for (int k = 0; k < 6; k++) {
                    ui::HudState::AbilitySlot &slot = hud.abilities[k];
                    slot.id = (size_t)k < slots.size() ? slots[k] : 0;
                    slot.usable = c.health > 0 && (k == 0 || slot.id == 0 || creatures::usableNow(c, slot.id));
                    const int32_t length = slot.id ? creatures::cooldownLength(c, slot.id) : 0;
                    const auto left = c.cooldowns.find(slot.id);
                    slot.cooldown = length > 0 && left != c.cooldowns.end() ? std::max(0.f, left->second / (float)length) : 0.f;
                }
                const bool free = !interface.wantsMouse() && !interface.typing() && c.health > 0;
                hud.abilities[0].held = free && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS || pad.triggerHeld(true));
                hud.abilities[1].held = free && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS || pad.triggerHeld(false));
                for (int k = 0; k < 3; k++)
                    hud.abilities[2 + k].held = free && (glfwGetKey(window, GLFW_KEY_1 + k) == GLFW_PRESS ||
                                                         pad.held(k == 0 ? Gamepad::X : k == 1 ? Gamepad::Y : Gamepad::RB));
                hud.quickHeld = free && (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || pad.held(Gamepad::RIGHT));
                // §5.4, §5.4.1: bars over creatures within 200 blocks that the camera sees: the
                // one under the crosshair (focus), players, and with V every one.
                const glm::dvec3 me = player->body().blocks();
                const glm::dvec3 eye = view.worldPosition();
                const float myPower = creatures::levelCurve(c.level);
                auto clearSight = [&](glm::dvec3 to, double range) {
                    // 0x59ee90, in half-block steps here.
                    const double length = glm::distance(eye, to);
                    if (length > range)
                        return false;
                    for (double t = 1; t < length - 1; t += 0.5) {
                        const glm::dvec3 q = eye + (to - eye) * (t / length);
                        const uint8_t type = view.held().read((int32_t)std::floor(q.x), (int32_t)std::floor(q.y),
                                                              (int32_t)std::floor(q.z)).type();
                        if (type != worldgen::AIR && type != worldgen::WATER)
                            return false;
                    }
                    return true;
                };
                // §5.4.1 item 6: the creature's box, one block larger, covers the screen centre.
                auto underCrosshair = [&](glm::dvec3 centre, glm::vec3 box) {
                    bool left = false, right = false, up = false, down = false;
                    for (int k = 0; k < 8; k++) {
                        const glm::dvec3 corner = centre + glm::dvec3((k & 1) ? 0.5 : -0.5, (k & 2) ? 0.5 : -0.5,
                                                                      (k & 4) ? 0.5 : -0.5) * glm::dvec3(box + 1.f);
                        const glm::vec4 clip = frame.viewProjection * glm::vec4(view.toRender(corner), 1);
                        if (clip.w <= 0)
                            continue;
                        const glm::vec2 ndc = glm::vec2(clip) / clip.w;
                        left = left || ndc.x < 0, right = right || ndc.x > 0;
                        up = up || ndc.y < 0, down = down || ndc.y > 0;
                    }
                    return left && right && up && down;
                };
                auto addBar = [&](const creatures::Combatant &who, uint64_t id, glm::dvec3 centre, glm::vec3 box,
                                  uint16_t appearanceFlags, const std::string &ownName) {
                    const double distance = glm::distance(centre, me);
                    if (distance > 200)
                        return;
                    const bool isPlayer = who.controller == 0;
                    const bool focused = distance * distance <= 3600 && clearSight(centre, 256) && underCrosshair(centre, box);
                    if (!focused && !interface.allLifeBars && !isPlayer)
                        return;
                    const glm::dvec3 head = centre + glm::dvec3(0, 0, box.z * 0.5);
                    const auto p = interface.project(head);
                    if (!p || !clearSight(head, 200))
                        return;
                    const glm::vec2 ndc = *p / glm::vec2((float)size.width, (float)size.height) * 2.f - 1.f;
                    if (!focused && (std::abs(ndc.x) > 1 || std::abs(ndc.y) > 1))
                        return;
                    ui::HudState::CreatureBar bar;
                    bar.at = {p->x, std::max(p->y, 50.f)};
                    bar.style = who.controller == 1 ? (creatures::peacefulAnimal(who.type, appearanceFlags)
                                                           ? ui::HudState::BarStyle::Neutral
                                                           : ui::HudState::BarStyle::Enemy)
                                : who.controller == 6 ? ui::HudState::BarStyle::Static
                                                      : ui::HudState::BarStyle::Friend;
                    const float most = creatures::maxHealth(who);
                    bar.fraction = most > 0 ? std::clamp(who.health / most, 0.f, 1.f) : 0.f;
                    // §5.4.1 items 1, 2: the species, then " +tier" when there is one. (Alpha
                    // ends that text with a line break, which the painter does not lay out yet.)
                    std::string species;
                    if (const char *key = creatures::speciesKey(who.type))
                        if (const DictionaryEntry *e = dictionary.find("name", key))
                            if (const std::string *singular = e->field("singular"))
                                species = *singular;
                    if (who.powerTier)
                        species += " +" + std::to_string(who.powerTier);
                    bar.info = ui::utf16(species);
                    // §5.4.1 item 3: the own name, else a title for bosses and NPCs.
                    std::string title = ownName;
                    if (title.empty() && ((appearanceFlags & 0x200) || who.controller == 3))
                        title = creatures::generatedTitle(id, who.type);
                    bar.rareName = ui::utf16(title);
                    bar.rareColour = who.controller == 1 ? glm::vec4(0.502f, 0, 1, 1) : glm::vec4(0, 0.731f, 1, 1);
                    const float power = creatures::levelCurve(who.level);
                    bar.infoColour = isPlayer                  ? glm::vec4(0, 0.8f, 1, 1)
                                     : who.controller == 6     ? glm::vec4(0.8f, 0.8f, 0.8f, 1)
                                     : power > myPower + 0.3f  ? glm::vec4(1, 0.2f, 0.2f, 1)
                                     : power > myPower + 0.1f  ? glm::vec4(1, 0.6f, 0.2f, 1)
                                     : power > myPower - 0.1f  ? glm::vec4(0, 1, 1, 1)
                                                               : glm::vec4(1);
                    hud.creatureBars.push_back(bar);                };
                if (!client.connected())
                    for (const creatures::Npc &n : npcs.all())
                        if (n.fighter.health > 0)
                            addBar(n.fighter, n.fighter.id, n.body.blocks(), n.body.size, n.looks.flags, "");
                for (const auto &[id, other] : receivedActors) {
                    const creatures::Appearance looks = other.appearance();
                    addBar(other.fighter(), id, glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0,
                           looks.box, looks.flags, other.name());
                }
                // Interaction §2: the creature and static passes compete on the camera distance
                // B of the boxes under the crosshair; within 16 blocks the nearest to the player
                // is the fall-back when nothing is aimed at.
                creatureTarget = 0;
                objectTarget.reset();
                double B = -1, Fd = -1;
                uint64_t fallbackCreature = 0;
                std::optional<std::tuple<int32_t, int32_t, size_t>> fallbackObject;
                auto creaturePass = [&](uint64_t id, const creatures::Combatant &who, glm::dvec3 centre, glm::vec3 box) {
                    const double dp = glm::distance(me, centre);
                    if (who.health <= 0 || dp * dp > 3600 || !clearSight(centre, 256))
                        return;
                    if (dp < 16 && (Fd < 0 || dp < Fd))
                        Fd = dp, fallbackCreature = id, fallbackObject.reset();
                    const double dc = glm::distance(eye, centre);
                    if (underCrosshair(centre, box) && (B < 0 || dc < B))
                        B = dc, creatureTarget = id;
                };
                if (!client.connected())
                    for (const creatures::Npc &n : npcs.all())
                        creaturePass(n.fighter.id, n.fighter, n.body.blocks(), n.body.size);
                for (const auto &[id, other] : receivedActors)
                    creaturePass(id, other.fighter(), glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0,
                                 other.appearance().box);
                const int32_t pzx = worldgen::floorDiv((int32_t)std::floor(me.x), worldgen::ZONE);
                const int32_t pzy = worldgen::floorDiv((int32_t)std::floor(me.y), worldgen::ZONE);
                for (int32_t zx = pzx - 1; zx <= pzx + 1; zx++)
                    for (int32_t zy = pzy - 1; zy <= pzy + 1; zy++) {
                        const worldgen::Zone *zone = view.held().zone(zx, zy);
                        for (size_t k = 0; zone && k < zone->statics.size(); k++) {
                            const worldgen::StaticObject &o = zone->statics[k];
                            const glm::dvec3 at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
                            const double dp = glm::distance(me, at);
                            if (dp > 16)
                                continue;
                            const glm::dvec3 centre = at + glm::dvec3(0, 0, o.scale[2] * 0.5);
                            const double dc = glm::distance(eye, centre);
                            if (Fd < 0 || dp < Fd)
                                Fd = dp, fallbackObject = {zx, zy, k}, fallbackCreature = 0;
                            if (!(B < 0 || dc < B) || !clearSight(centre, 200))
                                continue;
                            // The box: the scale, x and y swapped for odd turns, centred in x and
                            // y, from the base up.
                            const bool odd = (o.rotation & 1) != 0;
                            const glm::dvec3 size(odd ? o.scale[1] : o.scale[0], odd ? o.scale[0] : o.scale[1], o.scale[2]);
                            bool left = false, right = false, up = false, down = false;
                            for (int c = 0; c < 8; c++) {
                                const glm::dvec3 corner = at + glm::dvec3(((c & 1) - 0.5) * size.x, (((c >> 1) & 1) - 0.5) * size.y,
                                                                          ((c >> 2) & 1) * size.z);
                                const glm::vec4 clip = frame.viewProjection * glm::vec4(view.toRender(corner), 1);
                                if (clip.w <= 0)
                                    continue;
                                const glm::vec2 ndc = glm::vec2(clip) / clip.w;
                                left = left || ndc.x < 0, right = right || ndc.x > 0;
                                up = up || ndc.y < 0, down = down || ndc.y > 0;
                            }
                            if (left && right && up && down)
                                B = dc, objectTarget = {zx, zy, k}, creatureTarget = 0;
                        }
                    }
                // §2.4: the item pass, on squared distances, within 4 blocks of the player; an
                // item it cannot carry counts 16 further away.
                itemTarget.reset();
                std::optional<std::array<int32_t, 3>> fallbackItem;
                double Fi = -1;
                for (int32_t zx = pzx - 1; zx <= pzx + 1; zx++)
                    for (int32_t zy = pzy - 1; zy <= pzy + 1; zy++) {
                        const std::vector<WorldItem> *list = worldItems.zone(zx, zy);
                        for (size_t k = 0; list && k < list->size(); k++) {
                            const WorldItem &w = (*list)[k];
                            const glm::dvec3 at = w.blocks();
                            double q = glm::dot(at - me, at - me), qc = glm::dot(at - eye, at - eye);
                            if (q > 16)
                                continue;
                            if (!inventory.canCarry(w.item))
                                q += 16, qc += 16;
                            if (Fi < 0 || q < Fi)
                                Fi = q, fallbackItem = std::array<int32_t, 3>{zx, zy, (int32_t)k};
                            if (!(B < 0 || qc < B) || !clearSight(at + glm::dvec3(0, 0, 0.25), 200))
                                continue;
                            const uint16_t model = creatures::itemModel(creatures::Item::parse(w.item.data()));
                            if (model == creatures::NO_MODEL)
                                continue;
                            const glm::vec3 size = creatureView.voxels(model);
                            const glm::mat4 pose = worldItemPose(w, size, 0, true);
                            bool left = false, right = false, up = false, down = false;
                            for (int corner = 0; corner < 8; corner++) {
                                // (Corners of the engine's mesh: centred on x and y, base at 0.)
                                const glm::vec3 v(((corner & 1) - 0.5f) * size.x, (((corner >> 1) & 1) - 0.5f) * size.y,
                                                  ((corner >> 2) & 1) * size.z);
                                const glm::dvec3 p = at + glm::dvec3(glm::vec3(pose * glm::vec4(v, 1)));
                                const glm::vec4 clip = frame.viewProjection * glm::vec4(view.toRender(p), 1);
                                if (clip.w <= 0)
                                    continue;
                                const glm::vec2 ndc = glm::vec2(clip) / clip.w;
                                left = left || ndc.x < 0, right = right || ndc.x > 0;
                                up = up || ndc.y < 0, down = down || ndc.y > 0;
                            }
                            if (left && right && up && down) {
                                B = qc, itemTarget = std::array<int32_t, 3>{zx, zy, (int32_t)k};
                                objectTarget.reset(), creatureTarget = 0;
                            }
                        }
                    }
                if (c.health <= 0) {
                    creatureTarget = 0, objectTarget.reset(), itemTarget.reset();
                } else {
                    if (!creatureTarget && !objectTarget) {
                        creatureTarget = fallbackCreature;
                        objectTarget = fallbackObject;
                    }
                    if (!itemTarget)
                        itemTarget = fallbackItem;
                }
                // Screens §8.1.9: E picks the item target up, if it can be carried.
                const WorldItem *targetedItem = nullptr;
                if (itemTarget)
                    if (const auto *list = worldItems.zone((*itemTarget)[0], (*itemTarget)[1]))
                        if ((*itemTarget)[2] < (int32_t)list->size())
                            targetedItem = &(*list)[(*itemTarget)[2]];
                interface.targetItem = targetedItem ? std::optional<creatures::ItemRecord>(targetedItem->item) : std::nullopt;
                const bool eDown = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !interface.typing();
                if (eDown && !eWasDown && targetedItem && c.health > 0) {
                    if (!inventory.canCarry(targetedItem->item)) {
                        interface.chatAdd("You can't carry more of these items.\n", {255, 51, 51});
                        audio.play("human-voice01.wav");
                    } else if (client.connected()) {
                        // Online the server takes it from the zone and gives it back in section 9.
                        client.sendItemRequest(itemRequest({}, *itemTarget, 5));
                    } else if (const auto got = worldItems.pickUp((*itemTarget)[0], (*itemTarget)[1], (*itemTarget)[2], me)) {
                        // World items §3.3 kind 5: into the bags, announced in the chat, with the
                        // pick-up sound.
                        receive(*got);
                        audio.play("pickup.wav", 1, 1 + 0.1f * (float)worldRandom.draw() / 32767.f);
                        interface.targetItem.reset();
                    }
                }
                eWasDown = eDown;
                // §3: the prompt, from the creature target or else the object target.
                const worldgen::StaticObject *targeted = nullptr;
                if (objectTarget)
                    if (const worldgen::Zone *zone = view.held().zone(std::get<0>(*objectTarget), std::get<1>(*objectTarget));
                        zone && std::get<2>(*objectTarget) < zone->statics.size())
                        targeted = &zone->statics[std::get<2>(*objectTarget)];
                if (creatureTarget) {
                    uint8_t controller = 0;
                    for (const creatures::Npc &n : npcs.all())
                        if (n.fighter.id == creatureTarget && !client.connected())
                            controller = n.fighter.controller;
                    for (const auto &[id, other] : receivedActors)
                        if (id == creatureTarget)
                            controller = other.controller();
                    hud.prompt = controller == 3 ? u"[R] Talk" : u"";
                    for (const creatures::Npc &n : npcs.all()) // quests: a tied-up prisoner
                        if (n.fighter.id == creatureTarget && n.questRole == quests::PRISONER && n.fighter.controller == 6 &&
                            !client.connected())
                            hud.prompt = u"[R] Free";
                    if (client.connected() && controller == 6 && (creatureTarget >> 56) == 0x51) // (online: by its id)
                        hud.prompt = u"[R] Free";
                    // Travel §5.2: the player's own rideable pet offers a ride (with the riding
                    // skill, not already riding).
                    if (const creatures::Npc *pet = client.connected() ? nullptr : npcs.petOf(c);
                        pet && pet->fighter.id == creatureTarget && creatures::rideable(pet->fighter.type) &&
                        c.skills[1] != 0 && c.action != 106)
                        hud.prompt = u"[R] Ride";
                } else if (targeted) {
                    const auto *states = statics.zone(std::get<0>(*objectTarget), std::get<1>(*objectTarget));
                    const StaticState *live = states && std::get<2>(*objectTarget) < states->size()
                                                  ? &(*states)[std::get<2>(*objectTarget)]
                                                  : nullptr;
                    hud.prompt = ui::staticPrompt(targeted->kind, live ? live->state : targeted->state,
                                                  live && live->occupant == c.id);
                }
                // §5.3: R at a runestone opens the map in teleport mode. (Talk, doors, seats and
                // stations wait for their windows and the static states.)
                const bool rDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !interface.typing();
                if (rDown && !rWasDown && hud.prompt == u"[R] Ride") {
                    player->combat().action = 106; // travel §5.2: mounted
                    player->combat().actionMs = 0;
                }
                // Pets §13: T calls the pet back (packet 6 kind 8 online).
                const bool tDown = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !interface.typing();
                if (tDown && !tWasDown && c.health > 0) {
                    if (client.connected())
                        client.sendItemRequest(itemRequest({}, {0, 0, 0}, 8));
                    else
                        npcs.callPet(player->combat(), me);
                }
                tWasDown = tDown;
                // Travel §5.3: riding needs the pet alive and rideable (online the server's
                // pet is trusted).
                {
                    const creatures::Npc *pet = npcs.petOf(c);
                    player->body().mountValid = client.connected() ||
                                                (pet && pet->fighter.health >= 0 && creatures::rideable(pet->fighter.type));
                }
                if (rDown && !rWasDown && c.health > 0 && !creatureTarget && targeted && targeted->kind == ui::RUNESTONE) {
                    interface.mapOpen = true;
                    interface.teleportMode = true;
                } else if (rDown && !rWasDown && c.health > 0 && !creatureTarget && targeted) {
                    // §5.3: crafting stations open their windows (not yet); every other static is
                    // used with packet 6 kind 3 (§6), run here offline.
                    const int32_t kind = targeted->kind;
                    const bool station = kind == 65 || (kind >= 71 && kind <= 77);
                    const auto [zx, zy, k] = *objectTarget;
                    // Crafting §4.1: a station opens the crafting window (the campfire on Cooking,
                    // the furnace, spinning wheel and sawbench on the ingredients). (The
                    // customization bench's spirit editor is not in yet.)
                    if (kind == 77) { // item services §3.2: the bench opens with the inventory
                        interface.benchOpen = !interface.benchOpen;
                        interface.inventoryOpen = interface.benchOpen;
                        interface.benchReset();
                    }
                    if (station && kind != 77) {
                        interface.craftingOpen = !interface.craftingOpen;
                        if (kind == 65)
                            interface.craftingTab = 3;
                        else if (kind == 71 || kind == 73 || kind == 75)
                            interface.craftingTab = 5;
                    }
                    // (Online the server decides and answers in section 6; the client seats its
                    // own player at once, as the server cannot move a player's record.)
                    if (!station && client.connected())
                        client.sendItemRequest(itemRequest({}, {zx, zy, (int32_t)k}, 3));
                    // Talk §1.3: most statics make the player say "There is nothing special."
                    static const std::set<int32_t> QUIET = {1, 2, 3, 10, 16, 18, 19, 69};
                    if (!station && !QUIET.count(kind))
                        interface.talk(c.id, nothingSpecial);
                    if (!station) {
                        if (const worldgen::Zone *zone = view.held().zone(zx, zy)) {
                            std::vector<creatures::SoundEvent> used;
                            statics.use(*zone, (int32_t)k, player->combat(), player->body(), player->facing(), worldItems,
                                        scene.day,
                                        [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); },
                                        [&] { return worldRandom.draw(); }, used);
                            for (const creatures::SoundEvent &e : used)
                                playAt(e.sound, e.position, e.volume, e.pitch);
                        }
                    }
                }
                // Talk §1.3, §2.4: R on a town NPC opens or advances its bubble; the talk
                // request (packet 6 kind 2) rests the player at an inn at night (§2.7).
                if (rDown && !rWasDown && c.health > 0 && creatureTarget) {
                    uint8_t controller = 0, characterClass = 0;
                    int32_t talkKind = 0;
                    glm::ivec2 target{-1, -1};
                    std::array<int32_t, 3> home{-1, -1, -1};
                    if (!client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            if (n.fighter.id == creatureTarget) {
                                controller = n.fighter.controller, characterClass = n.fighter.characterClass;
                                talkKind = n.talkKind;
                                target = n.talkTarget;
                                home = {n.zone.first, n.zone.second, (int32_t)n.spawn};
                            }
                    // Talk §2.4 steps 0-2: the target zone's location, dropped when its level is
                    // more than 2 from the player's; §2.5: the zone is marked on the map.
                    const auto cellOf = [](int32_t z) { return (z < 0 ? z + 7 : z) / 8; }; // (C division, +7 fix-up)
                    const worldgen::Location *place = nullptr;
                    if (const int32_t cx = target.x / 8, cy = target.y / 8; cx >= 0 && cy >= 0 && cx < 8192 && cy < 8192)
                        if (const worldgen::Location *L = cellLocation(view.terrainWorld(), cx, cy);
                            L && std::abs(L->level - c.level) > 2)
                            target = {-1, -1};
                    place = cellLocation(view.terrainWorld(), cellOf(target.x), cellOf(target.y));
                    if (characterClass != 132 && target != glm::ivec2(-1, -1))
                        view.markDiscovered(target.x, target.y);
                    for (const auto &[id, other] : receivedActors)
                        if (id == creatureTarget)
                            controller = other.controller(), characterClass = other.fighter().characterClass;
                    // Quests: R on a tied-up prisoner frees it once no guard is near (online the
                    // server decides: a talk request naming the quest and the creature's index).
                    if (controller == 6 && client.connected() && (creatureTarget >> 56) == 0x51)
                        client.sendItemRequest(
                            itemRequest({}, {-2, (int32_t)(uint32_t)(creatureTarget >> 12), (int32_t)(creatureTarget & 0xfff)}, 2));
                    if (controller == 6 && options.extendedQuests && !client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            if (n.fighter.id == creatureTarget && n.questRole == quests::PRISONER) {
                                const uint32_t questId = n.quest;
                                const quests::Quest *q = questBook.find(questId);
                                if (!q) {
                                    interface.chatAdd("This prisoner waits for someone else.\n", {1, 1, 1});
                                } else if (npcs.guarded(questId, n.body.blocks(), quests::FREE_CLEAR)) {
                                    interface.chatAdd("Defeat the guards first!\n", {1, 0.4f, 0.4f});
                                } else {
                                    const glm::dvec3 at = n.body.blocks();
                                    npcs.free(creatureTarget, q->giverAt);
                                    interface.floating.push_back({at + glm::dvec3(0, 0, 1.5), {0, 0}, {1, 1, 1, 1}, u"Thank you!", 20, -60, 2000});
                                    questBookChanged = true;
                                    if (questBook.freed(questId)) {
                                        interface.chatAdd("Quest done! Return to " + quests::giverName(*q) + ".\n", {1, 0.8f, 0.2f});
                                        interface.questComplete(quests::objectiveText(*q, q->giverAt));
                                        if (const char *key = creatures::soundKey(30))
                                            audio.play(key);
                                    }
                                }
                                break;
                            }
                    if (controller == 3) {
                        const uint64_t who = creatureTarget;
                        // Quests: a giver offers its quest of the day, or takes back a done one.
                        bool questTalk = false;
                        if (options.extendedQuests && !client.connected())
                            for (const creatures::Npc &n : npcs.all()) {
                                if (n.fighter.id != who)
                                    continue;
                                if (quests::Quest *q = questBook.byGiver(who)) {
                                    questTalk = true;
                                    if (q->state == quests::DONE) {
                                        const quests::Quest done = *q;
                                        interface.talk(who, [&] { return saying(quests::thanksText(done)); });
                                        for (const worldgen::ItemRecord &item : quests::rewards(done, c.characterClass, questRandom)) {
                                            receive(creatures::toRecord(item));
                                        }
                                        mods.questComplete(done.id);
                                        npcs.removeQuest(done.id);
                                        questBook.handedIn.push_back(done.id);
                                        std::erase_if(questBook.quests, [&](const quests::Quest &k) { return k.id == done.id; });
                                        if (trackedQuest == done.id)
                                            trackedQuest = questBook.quests.empty() ? 0 : questBook.quests.back().id;
                                        if (const char *key = creatures::soundKey(30))
                                            audio.play(key);
                                    } else {
                                        const std::u16string goal = quests::objectiveText(*q, player->body().blocks(), plural(q->enemyType));
                                        interface.talk(who, [&] {
                                            return saying("Please hurry! " + net::fromUtf16(std::vector<uint16_t>(goal.begin(), goal.end())) + ".");
                                        });
                                        trackedQuest = q->id;
                                    }
                                    questBookChanged = true;
                                } else if (quests::isGiver(who, characterClass)) {
                                    const worldgen::World &world = view.terrainWorld();
                                    const worldgen::Location *L = cellLocation(world, worldgen::floorDiv((int32_t)std::floor(n.home.x), worldgen::SUBCELL),
                                                                               worldgen::floorDiv((int32_t)std::floor(n.home.y), worldgen::SUBCELL));
                                    const quests::Quest q = quests::offer(world.seed(), who, n.fighter.type, n.home, scene.day,
                                                                          L ? L->level : c.level, L ? L->subLevel : 0);
                                    const bool taken = std::find(questBook.handedIn.begin(), questBook.handedIn.end(), q.id) != questBook.handedIn.end();
                                    if (!taken && !questBook.full()) {
                                        questBook.quests.push_back(q);
                                        trackedQuest = q.id;
                                        questBookChanged = true;
                                        interface.talk(who, [&] { return saying(quests::offerText(q, n.home, plural(q.enemyType))); });
                                        interface.chatAdd("New quest taken.\n", {1, 0.8f, 0.2f});
                                        questTalk = true;
                                    } else if (!taken) {
                                        interface.talk(who, [&] { return saying("You have your hands full already. Come back later!"); });
                                        questTalk = true;
                                    }
                                }
                                break;
                            }
                        // Talk §3.3: shops open the vendor window with the inventory beside it.
                        // (Online the stock would come from this client's own spawns, §3.2; only
                        // the offline copy is used yet.)
                        // Item services: class 131 identifies, class 137 adapts.
                        if (characterClass == 131)
                            interface.identifyOpen = interface.inventoryOpen = true, interface.identifyBag = -1;
                        if (characterClass == 137)
                            interface.adaptOpen = interface.inventoryOpen = true;
                        if (characterClass >= 128 && characterClass <= 130) {
                            interface.vendorOpen = interface.inventoryOpen = true;
                            interface.vendorTab = 0;
                            interface.vendorStock.clear();
                            for (const creatures::Npc &n : npcs.all())
                                if (n.fighter.id == who)
                                    interface.vendorStock = n.inventory;
                            // Talk §3.2: online the stock is not sent; it is this client's own copy of
                            // the vendor's spawn, found by the spawn id (zy·65536 + zx)·256 + index.
                            if (client.connected()) {
                                const int32_t zx = (int32_t)((who >> 8) & 0xffff), zy = (int32_t)((who >> 24) & 0xffff);
                                const size_t index = (size_t)(who & 0xff);
                                if (const worldgen::Zone *zone = view.held().zone(zx, zy); zone && index < zone->spawns.size())
                                    for (const worldgen::ItemRecord &item : zone->spawns[index].inventory)
                                        interface.vendorStock.push_back(creatures::toRecord(item));
                            }
                        }
                        if (!questTalk)
                            interface.talk(who, [&] { return npcSpeech(dictionary, who, characterClass, talkKind, place); });
                        if (client.connected()) {
                            client.sendItemRequest(itemRequest({}, home, 2));
                        } else if (characterClass == 132 &&
                                   (scene.timeOfDayMs >= 18 * 3600000.0 || scene.timeOfDayMs <= 6 * 3600000.0)) {
                            scene.day++;
                            scene.timeOfDayMs = 7 * 3600000.0;
                            if (session)
                                session->clock.day = (uint32_t)scene.day, session->clock.timeOfDayMs = 7 * 3600000u;
                            audio.play("levelup2.wav", 0.75f);
                            npcs.restAll();
                            missions.clearGenerated(); // missions §2.1: a new day's missions
                        }
                    }
                }
                rWasDown = rDown;
                interface.hud = hud;
                // Missions §8.2: the small quest tag, shown inside a location's area (for a hunt,
                // within 130 blocks of the target zone's centre), with hysteresis between 0.05
                // and 0.1.
                {
                    const worldgen::World &world = view.terrainWorld();
                    const int32_t cx = worldgen::floorDiv((int32_t)std::floor(me.x), worldgen::SUBCELL);
                    const int32_t cy = worldgen::floorDiv((int32_t)std::floor(me.y), worldgen::SUBCELL);
                    const worldgen::Location *L = cellLocation(world, cx, cy);
                    const Missions::Cell *cell = missions.cell(cx, cy);
                    const MissionArea m = cell ? cell->mission : MissionArea{};
                    float v = L ? locationBump(world, *L, me.x, me.y) : 0.f;
                    if (L && m.kind == 1) {
                        const double dx = me.x - (256.0 * m.targetZoneX + 128), dy = me.y - (256.0 * m.targetZoneY + 128);
                        const float k = 1 - std::min((float)((dx * dx + dy * dy) / 16900.0), 1.f);
                        v = k * k;
                    }
                    if (!L || v <= 0) {
                        interface.questTagShown = false;
                        questRemembered.reset();
                    } else if (m.kind != 0 && m.state != 2 && v > 0.1f) {
                        const bool differs = !questRemembered || questRemembered->kind != m.kind ||
                                             questRemembered->targetZoneX != m.targetZoneX ||
                                             questRemembered->targetZoneY != m.targetZoneY ||
                                             questRemembered->seed != m.seed || questRemembered->state != m.state;
                        if (differs) {
                            questRemembered = m;
                            if (m.state == 1) {
                                interface.setObjective(ui::objectiveText(m, *L, dictionary));
                                if (const char *key = creatures::soundKey(0x2e))
                                    audio.play(key);
                            }
                        }
                        interface.questTagShown = true;
                        interface.questBar = m.required != 0 && m.count != 0;
                        interface.questFill = m.required ? (float)m.count / (float)m.required : 0.f;
                    } else if (m.state == 2 || v < 0.05f) {
                        interface.questTagShown = false;
                    }
                }
                // Quests: the tag shows the tracked quest when no mission of Alpha's is shown.
                if (options.extendedQuests && !interface.questTagShown)
                    if (const quests::Quest *q = questBook.find(trackedQuest)) {
                        const std::u16string goal = quests::objectiveText(*q, glm::dvec3(me), plural(q->enemyType));
                        if (goal != questObjective) {
                            questObjective = goal;
                            interface.setObjective(goal);
                        }
                        interface.questTagShown = true;
                        interface.questBar = q->required > 0;
                        interface.questFill = q->required ? (float)q->count / (float)q->required : 0.f;
                    }
                // §5.9.1: the land the player stands in.
                const ui::LandText land = ui::landText(view.terrainWorld(), (int32_t)me.x, (int32_t)me.y, dictionary);
                interface.showLandName(land.name, land.detail);
            }
            interface.leftHeld = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            // Item services §3.2: the middle button turns the bench's weapon.
            {
                double cx, cy;
                glfwGetCursorPos(window, &cx, &cy);
                if (interface.benchOpen && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
                    interface.benchRotate({(float)(cx - benchMouse.x), (float)(cy - benchMouse.y)});
                benchMouse = {cx, cy};
                interface.cubeMeshId = creatureView.cubeMesh();
            }
            // Character screens: the lists from the saves, and what their buttons ask for.
            if (interface.front != ui::Interface::Front::None) {
                const fs::path saves = executableFolder() / "Save";
                std::error_code ec;
                fs::create_directories(saves, ec);
                const auto characters = [&] {
                    SaveDb db;
                    return db.open(saves / "characters.db", SaveDb::Mode::ReadWrite) ? readCharacters(db)
                                                                                      : std::vector<CharacterRecord>{};
                };
                const auto worlds = [&] {
                    SaveDb db;
                    return db.open(saves / "worlds.db", SaveDb::Mode::ReadWrite) ? readWorlds(db) : std::vector<WorldEntry>{};
                };
                if (frontListsStale) {
                    interface.characterList.clear();
                    panelFigures.clear();
                    for (const CharacterRecord &r : characters()) {
                        const creatures::CharacterChoice choice = creatures::CharacterChoice::parse(r.extra.data());
                        // Character screens §4.1: the figure in its panel, in its equipment.
                        PanelFigure figure;
                        int32_t figureType = 2;
                        creatures::generate(figureType, figure.looks, &choice, [] { return 0; });
                        figure.fighter.type = figureType;
                        for (size_t k = 0; k < r.equipment.size() && k < figure.fighter.equipment.size(); k++)
                            figure.fighter.equipment[k] = creatures::Item::parse(r.equipment[k].data());
                        panelFigures.push_back(std::move(figure));
                        interface.characterList.push_back(
                            {r.name, r.level, creatureTypeOf(choice.race, choice.female), r.characterClass, r.specialization});
                    }
                    interface.worldList.clear();
                    for (const auto &[mesh, size] : worldPreviews)
                        renderer.removeMesh(mesh);
                    worldPreviews.clear();
                    // §4.1: online worlds (online_…) show only in the multiplayer list.
                    for (const WorldEntry &w : worlds())
                        if ((w.name.size() > 7 && w.name.rfind("online_", 0) == 0) == interface.multiplayerWorlds) {
                            interface.worldList.push_back({w.name, w.seed, w.discovered});
                            // §4.2: the preview model, meshed once for the panel.
                            CubModel model;
                            model.sx = w.previewSize[0], model.sy = w.previewSize[1], model.sz = w.previewSize[2];
                            model.rgb = w.previewRgb;
                            MeshId mesh = 0;
                            if (!model.rgb.empty())
                                if (const ModelMesh m = model.mesh(); !m.indices.empty())
                                    mesh = renderer.addMesh(m);
                            worldPreviews.push_back({mesh, glm::vec3(model.sx, model.sy, model.sz)});
                        }
                    frontListsStale = false;
                }
                const auto request = std::exchange(interface.frontRequest, ui::Interface::FrontRequest::None);
                using R = ui::Interface::FrontRequest;
                const auto startWorld = [&](const std::string &name, uint32_t seed) {
                    Options next = options;
                    next.world = next.walk = true;
                    next.start = false;
                    next.saveWorld = name;
                    next.worldOptions.seed = seed;
                    next.character = chosenCharacter;
                    restart = next;
                };
                if (request == R::ListWorlds)
                    frontListsStale = true;
                if (request == R::Connect) {
                    // §6: connect to the address, playing the chosen character.
                    Options next = options;
                    next.world = next.walk = true;
                    next.start = false;
                    next.connect.clear();
                    for (char16_t ch : interface.serverText)
                        next.connect += ch < 0x80 ? (char)ch : '?';
                    next.saveWorld.clear();
                    next.character = chosenCharacter;
                    restart = next;
                }
                if (request == R::SelectCharacter) {
                    chosenCharacter = interface.selectedCharacter;
                    interface.front = ui::Interface::Front::Worlds;
                } else if (request == R::CreateCharacter) {
                    creatures::CharacterChoice choice;
                    choice.race = interface.creation.race;
                    choice.female = interface.creation.gender == 1;
                    choice.head = interface.creation.face;
                    choice.hair = interface.creation.haircut;
                    std::copy(interface.creation.colour.begin(), interface.creation.colour.end(), choice.hairColour);
                    std::string name;
                    for (char16_t ch : interface.nameText)
                        name += ch < 0x80 ? (char)ch : '?';
                    std::vector<CharacterRecord> all = characters();
                    all.push_back(newCharacter(name, choice, interface.creation.characterClass + 1,
                                               [&] { return worldRandom.draw(); }));
                    SaveDb db;
                    if (db.open(saves / "characters.db", SaveDb::Mode::ReadWrite))
                        writeCharacters(db, all);
                    chosenCharacter = (int)all.size() - 1;
                    frontListsStale = true;
                    interface.front = ui::Interface::Front::Worlds;
                } else if (request == R::DeleteCharacter) {
                    std::vector<CharacterRecord> all = characters();
                    if (interface.selectedCharacter < (int)all.size()) {
                        all.erase(all.begin() + interface.selectedCharacter);
                        SaveDb db;
                        if (db.open(saves / "characters.db", SaveDb::Mode::ReadWrite))
                            writeCharacters(db, all);
                    }
                    frontListsStale = true;
                } else if (request == R::SelectWorld && interface.selectedWorld < (int)interface.worldList.size()) {
                    startWorld(interface.worldList[interface.selectedWorld].name, interface.worldList[interface.selectedWorld].seed);
                } else if (request == R::CreateWorld) {
                    std::string name;
                    for (char16_t ch : interface.nameText)
                        name += ch < 0x80 ? (char)ch : '?';
                    uint32_t seed = 0;
                    for (char16_t ch : interface.seedText)
                        seed = seed * 10 + (uint32_t)(ch - u'0');
                    startWorld(name, seed);
                } else if (request == R::DeleteWorld && interface.selectedWorld < (int)interface.worldList.size()) {
                    // §4.5: the world's files go, and its entry. (The list is written back in order;
                    // Alpha's own rewrite leaves its last world in the deleted one's place.)
                    const std::string name = interface.worldList[interface.selectedWorld].name;
                    fs::remove(saves / ("world_" + name + ".db"), ec);
                    fs::remove(saves / ("map_" + name + ".db"), ec);
                    std::vector<WorldEntry> all = worlds();
                    all.erase(std::remove_if(all.begin(), all.end(), [&](const WorldEntry &w) { return w.name == name; }),
                              all.end());
                    SaveDb db;
                    if (db.open(saves / "worlds.db", SaveDb::Mode::ReadWrite)) {
                        const uint32_t old = db.number("num").value_or(0);
                        for (size_t i = 0; i < all.size(); i++)
                            db.put(std::to_string(i), all[i].serialize());
                        for (uint32_t i = (uint32_t)all.size(); i < old; i++)
                            db.remove(std::to_string(i));
                        db.putNumber("num", (uint32_t)all.size());
                    }
                    frontListsStale = true;
                }
                // §3.9 Apply: the player wears the creation's choices (the hover colour too).
                if (std::exchange(interface.creationChanged, false) && player) {
                    creatures::CharacterChoice choice;
                    choice.race = interface.creation.race;
                    choice.female = interface.creation.gender == 1;
                    choice.head = interface.creation.face;
                    choice.hair = interface.creation.haircut;
                    std::copy(interface.creation.hover.begin(), interface.creation.hover.end(), choice.hairColour);
                    creatures::Appearance looks;
                    int32_t type = 2;
                    creatures::generate(type, looks, &choice, [] { return 0; });
                    player->setLooks(looks);
                }
            } else {
                frontListsStale = true;
            }
            // Talk §1: the bubbles follow their creatures (the player, NPCs offline, others online).
            interface.creatureAt = [&](uint64_t id) -> std::optional<std::pair<glm::dvec3, float>> {
                if (player && id == player->combat().id)
                    return std::pair{player->body().blocks(), player->body().size.z};
                if (!client.connected())
                    for (const creatures::Npc &n : npcs.all())
                        if (n.fighter.id == id)
                            return std::pair{n.body.blocks(), n.body.size.z};
                for (const auto &[other, record] : receivedActors)
                    if (other == id)
                        return std::pair{glm::dvec3(record.position(0), record.position(1), record.position(2)) / 65536.0,
                                         record.appearance().box.z};
                return std::nullopt;
            };
            if (sayAtStart && player) {
                sayAtStart = false;
                interface.talk(player->combat().id, nothingSpecial);
            }
            interface.stepBubbles(seconds * 1000.f);
            // Screens §8.1.6: every interface item turns 30 degrees a second about z.
            interface.itemTurn.z += seconds * 1000.f * 0.03f;
            renderer.setInterface(
                interface.draw({(float)size.width, (float)size.height}, {(float)mx, (float)my}, now - start));
            if (interface.atlas.takeChanged())
                renderer.setInterfaceAtlas(interface.atlas.size(), interface.atlas.size(), interface.atlas.pixels());
        }
        const bool capturing = framesUntilCapture == 0;
        // Particles §2.1, §4: this frame's bursts become particles, which then move one frame;
        // §6: each is the white unit cube, centred, tumbling, in its colour.
        {
            const auto read = [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); };
            for (const creatures::Burst &b : std::exchange(bursts, {}))
                particles.expand(b, [&] { return worldRandom.draw(); });
            const glm::dvec3 eye = view.worldPosition();
            const double reach = scene.fogDistance + 32;
            // §7.1: as each world second passes, every kind-60 record of the zones in sight
            // lets one leaf in sixteen fall.
            const bool second = leafClockMs >= 0 && std::floor(leafClockMs / 1000) != std::floor(scene.timeOfDayMs / 1000);
            // Audio §3: the clock's step this frame, for the ambience's ticks.
            double clockStep = leafClockMs >= 0 ? scene.timeOfDayMs - leafClockMs : 0;
            if (clockStep < 0)
                clockStep += alpha::DAY_MS;
            const double clockBefore = leafClockMs;
            leafClockMs = scene.timeOfDayMs;
            // A record ticks when its own phase (from its position) plus the clock passes a
            // multiple of the period.
            const auto ticks = [&](int64_t x, int64_t y, int64_t period) {
                if (clockBefore < 0 || clockStep <= 0)
                    return false;
                const int64_t xb = x / 65536, yb = (y + 333ll * 65536) / 65536;
                const double phase = (double)(xb % period + yb % period) + clockBefore;
                return std::trunc(phase / (double)period) != std::trunc((phase + clockStep) / (double)period);
            };
            const double t = scene.timeOfDayMs;
            const bool day = t > 21600000 && t < 75600000, evening = t > 43200000 && t < 86400000;
            const auto u = [&] { return (float)worldRandom.draw() / 32767.f; };
            for (const auto &[key, zone] : view.held().zones()) {
                const glm::dvec2 centre = glm::dvec2(key.first + 0.5, key.second + 0.5) * (double)worldgen::ZONE;
                if (glm::length(centre - glm::dvec2(eye)) > reach)
                    continue;
                // Audio §3: birds sing from the crowns by day and owls call at night; crickets chirp
                // in the flowers and grass in the afternoon and evening.
                for (const auto &post : zone->songPosts)
                    if (ticks(post[0], post[1], 1000) && worldRandom.draw() % 10 == 0) {
                        const glm::dvec3 at = glm::dvec3((double)post[0], (double)post[1], (double)post[2]) / 65536.0;
                        if (day) {
                            playAt(94, at, 0.1f, 0.9f + 0.2f * u());
                        } else {
                            const float pitch = 0.5f + 0.2f * u();
                            playAt(99 + (worldRandom.draw() & 1), at, 0.1f, pitch);
                        }
                    }
                if (evening)
                    for (const worldgen::Plant &plant : zone->plants)
                        if (plant.kind <= 4 && ticks((int64_t)plant.x * 65536, (int64_t)plant.y * 65536, 5000) &&
                            worldRandom.draw() % 70 == 0)
                            playAt(plant.kind <= 1 ? 98 : 97, glm::dvec3(plant.x, plant.y, plant.z), 0.05f,
                                   0.9f + 0.2f * u());
                for (const worldgen::LeafSource &s : zone->leafSources) {
                    if (!second || worldRandom.draw() % 16 != 0)
                        continue;
                    creatures::Particle p;
                    p.style = creatures::LEAF;
                    p.collides = true;
                    p.lifetimeMs = 20000;
                    p.position[0] = s.x, p.position[1] = s.y, p.position[2] = s.z;
                    p.velocity.y = 4 * (u() - 0.5f);
                    p.velocity.x = 4 * (u() - 0.5f);
                    p.velocity.z = -2;
                    p.spin.z = 10 * (u() - 0.5f);
                    p.spin.y = 10 * (u() - 0.5f);
                    p.spin.x = 10 * (u() - 0.5f);
                    p.colour = {s.r, s.g, s.b, 1};
                    p.size = 0.5f;
                    particles.list.push_back(p);
                }
                // §8: a campfire burns as twenty flame cubes, red at the base, yellow at the tip,
                // worked out afresh from the clock.
                for (const worldgen::StaticObject &o : zone->statics) {
                    if (o.kind != 65)
                        continue;
                    const glm::dvec3 base = glm::dvec3(o.x, o.y, o.z) / 65536.0 + glm::dvec3(0, 0, o.scale[2] / 2);
                    for (const creatures::FlameCube &k : creatures::flames(scene.timeOfDayMs, 0.8f, 0.2f, 20, {1, 0, 0}, {1, 1, 0})) {
                        glm::mat4 at = glm::translate(glm::mat4(1), view.toRender(base + glm::dvec3(k.position)));
                        at = glm::rotate(at, glm::radians(k.turn.x), glm::vec3(1, 0, 0));
                        at = glm::rotate(at, glm::radians(k.turn.y), glm::vec3(0, 1, 0));
                        at = glm::rotate(at, glm::radians(k.turn.z), glm::vec3(0, 0, 1));
                        at = glm::scale(at, glm::vec3(k.edge)) * glm::translate(glm::mat4(1), glm::vec3(0, 0, -0.5f));
                        items.push_back({creatureView.cubeMesh(), at, false, Shading::World, glm::vec4(k.colour, 1)});
                    }
                }
            }
            particles.update((int32_t)std::lround(seconds * 1000.0), read);
            for (const creatures::Particle &p : particles.list) {
                glm::mat4 at = glm::translate(glm::mat4(1), view.toRender(p.blocks()));
                at = glm::rotate(at, glm::radians(p.rotation.z), glm::vec3(0, 0, 1));
                at = glm::rotate(at, glm::radians(p.rotation.y), glm::vec3(0, 1, 0));
                at = glm::rotate(at, glm::radians(p.rotation.x), glm::vec3(1, 0, 0));
                at = glm::scale(at, glm::vec3(p.size)) * p.extra * glm::translate(glm::mat4(1), glm::vec3(0, 0, -0.5f));
                // §6.3: splashes, block bits and leaves are lit like the world where they are;
                // the others at full strength.
                float light = 1;
                if (p.style == creatures::LIT || p.style == creatures::LEAF) {
                    const glm::dvec3 b = glm::floor(p.blocks());
                    const auto x = (int32_t)b.x, y = (int32_t)b.y, z = (int32_t)b.z;
                    const uint8_t type = view.held().read(x, y, z).type();
                    const int value = type == 13 ? 255 : type == 0 || type == 2 ? std::max<int>(view.held().light(x, y, z), 5) : 0;
                    light = std::max(value / 255.f, 0.2f);
                }
                items.push_back({creatureView.cubeMesh(), at, false, Shading::World, glm::vec4(glm::vec3(p.colour), light)});
            }
        }
        // Screens §5.2.3: under the world map only the sky is drawn.
        if (interface.mapOpen)
            items.clear(), effects.clear();
        if (client.connected()) {
            if (player) {
                for (int axis = 0; axis < 3; axis++)
                    own.setPosition(axis, player->body().position[axis]);
                own.setMotion(player->body().velocity, player->facing(), player->body().push, player->body().intent);
                own.setFighter(player->combat());
            } else {
                const glm::dvec3 at = view.worldPosition();
                for (int axis = 0; axis < 3; axis++)
                    own.setPosition(axis, (int64_t)(at[axis] * 65536.0));
            }
            client.setOwn(own);
        }
        if (networkFrame.owns_lock())
            networkFrame.unlock();
        renderer.setEffects(effects);
        const bool drawn = renderer.draw(frame, drawStars(scene), items, capturing ? pathOfUtf8(options.capture) : fs::path());
        if (capturing) {
            if (!drawn) {
                std::fprintf(stderr, "capture failed\n");
                status = 1;
            }
            break;
        }
        if (framesUntilCapture > 0 && drawn && !settling)
            framesUntilCapture--;
    }
    if (!options.capture.empty())
        std::printf("Simulation: %llu world calls, %llu input frames, max piece %d ms\n",
                    (unsigned long long)simulationWorldCalls, (unsigned long long)simulationInputFrames, simulationMaxPiece);
    if (player && playing()) {
        CharacterRecord &saved = *playing();
        const creatures::Combatant &c = player->combat();
        saved.level = c.level;
        saved.experience = c.experience;
        saved.health = c.health;
        saved.skills.assign(c.skills.begin(), c.skills.end());
        // The item in hand is not saved (§8.1.1): it goes back to the bags first.
        if (!inventory.hand.empty()) {
            for (int32_t i = 0; i < inventory.hand.count; i++)
                inventory.add(inventory.hand.item);
            inventory.hand = {};
        }
        saved.equipment = inventory.equipment;
        saved.tabs.clear();
        for (const auto &bag : inventory.bags) {
            auto &tab = saved.tabs.emplace_back();
            for (const creatures::Stack &slot : bag)
                tab.push_back({(uint32_t)slot.count, slot.item});
        }
        saved.money = inventory.money;
        saved.platinum = inventory.platinum;
        saved.recipes = inventory.recipes;
        saved.manaCubes = (uint32_t)manaCubes;
    }
    if (onlineCharacter && player) { // written back to its place in characters.db
        SaveDb db;
        if (db.open(executableFolder() / "Save" / "characters.db", SaveDb::Mode::ReadWrite)) {
            std::vector<CharacterRecord> all = readCharacters(db);
            if (options.character < (int)all.size()) {
                all[options.character] = *onlineCharacter;
                writeCharacters(db, all);
            }
        }
    }
    if (session) {
        regionPass(player ? player->body().blocks() : view.worldPosition());
        {
            // Saves §4.1: the world's preview is the map tile of the zone the player stands in.
            const glm::dvec3 at = player ? player->body().blocks() : view.worldPosition();
            const int32_t zx = worldgen::floorDiv((int32_t)std::floor(at.x), worldgen::ZONE);
            const int32_t zy = worldgen::floorDiv((int32_t)std::floor(at.y), worldgen::ZONE);
            if (const worldgen::Zone *zone = view.held().zone(zx, zy)) {
                const worldgen::MapTile tile = worldgen::buildMapTile(*zone, view.terrainGenerator());
                if (tile.sz > 0)
                    session->setPreview(tile.sx, tile.sy, tile.sz, tile.rgb);
            }
        }
        session->leave(player ? player->body().blocks() : view.worldPosition());
    }
    if (options.travel > 0)
        std::fprintf(stderr, "after travelling %.0f blocks: %s\n", travelled, view.status().c_str());
    return status;
}

// Setup: asks the player for the Cube World Alpha folder with the system's folder picker, until
// a folder holding the game is chosen or the picker is cancelled (empty).
fs::path askForInstall() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    fs::path chosen;
    IFileOpenDialog *dialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        DWORD flags = 0;
        dialog->GetOptions(&flags);
        dialog->SetOptions(flags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        dialog->SetTitle(L"AlphaVulkan: choose your Cube World Alpha folder (the one with Cube.exe)");
        while (chosen.empty() && SUCCEEDED(dialog->Show(nullptr))) {
            IShellItem *item = nullptr;
            PWSTR path = nullptr;
            if (SUCCEEDED(dialog->GetResult(&item)) && SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                if (install::looksLikeAlpha(path))
                    chosen = path;
                else
                    MessageBoxW(nullptr, L"That folder has no Cube World Alpha in it (Cube.exe and data1.db .. data4.db).",
                                L"AlphaVulkan", MB_OK | MB_ICONWARNING);
                CoTaskMemFree(path);
            }
            if (item)
                item->Release();
        }
        dialog->Release();
    }
    CoUninitialize();
    return chosen;
}

// Setup: writes [Game] Path into AlphaVulkan.ini beside the program (made from the sample when
// missing), so the folder is asked for only once.
void rememberInstall(const fs::path &game) {
    std::error_code ignored;
    fs::copy_file("config/AlphaVulkan.ini", "AlphaVulkan.ini", fs::copy_options::none, ignored);
    std::string error;
    if (!owner::remember("AlphaVulkan.ini", game, error))
        std::fprintf(stderr, "%s\n", error.c_str());
}

} // namespace

int main(int argc, char **argv) {
    const Options options = parse(argc, argv);
    Config config;
    if (!config.load("AlphaVulkan.ini"))
        config.load("config/AlphaVulkan.ini");

    fs::path game = options.game.empty() ? install::locate(config.text("Game", "Path")) : pathOfUtf8(options.game);
    bool pickedInstallation = false;
    if (game.empty() && options.game.empty()) {
        game = askForInstall();
        pickedInstallation = !game.empty();
    }
    if (game.empty()) {
        std::fprintf(stderr, "Cube World Alpha was not found. Set [Game] Path in AlphaVulkan.ini or pass --game DIR.\n");
        return 1;
    }
    Content content;
    std::string error;
    const auto ownerContent = owner::inspect(game, false);
    if (!ownerContent.ready()) {
        for (const auto &problem : ownerContent.problems)
            std::fprintf(stderr, "%s\n", problem.c_str());
        std::fprintf(stderr, "Run AlphaSetup --game DIR --check for the required owner files.\n");
        return 1;
    }
    if (pickedInstallation)
        rememberInstall(game);
    if (!content.open(game, &error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    // Mods (docs/modding.md): only those named in [Mods] Load, from the Mods folder beside the
    // executable. They load first so their content is in place before anything reads it.
    Audio audio;
    Scene *activeScene = nullptr;     // the viewer's, once it exists
    WorldView *activeView = nullptr;  // the world, in --world
    Player *activePlayer = nullptr;   // the player, in --walk
    ui::Interface *activeInterface = nullptr; // the game's interface, in --world
    ModHost modHost;
    modHost.log = [](const std::string &mod, const std::string &message) {
        std::fprintf(stderr, "[mod %s] %s\n", mod.c_str(), message.c_str());
    };
    modHost.content = &content;
    modHost.playSound = [&](const std::string &key, float volume) { return audio.play(key, volume); };
    modHost.getTimeOfDay = [&] { return activeScene ? activeScene->timeOfDayMs : 0.0; };
    modHost.setTimeOfDay = [&](double t) {
        if (activeScene)
            activeScene->timeOfDayMs = std::fmod(std::fmod(t, alpha::DAY_MS) + alpha::DAY_MS, alpha::DAY_MS);
    };
    modHost.getBlock = [&](int32_t x, int32_t y, int32_t z, uint8_t out[4]) {
        const worldgen::Zone *zone = activeView ? activeView->held().zoneAt(x, y) : nullptr;
        if (!zone)
            return false;
        const worldgen::Block b = zone->read(x, y, z);
        out[0] = b.r;
        out[1] = b.g;
        out[2] = b.b;
        out[3] = b.type();
        return true;
    };
    modHost.getPlayer = [&](AvPlayer &out) {
        if (!activePlayer)
            return false;
        const glm::dvec3 at = activePlayer->body().blocks();
        const creatures::Combatant &c = activePlayer->combat();
        out = {at.x, at.y, at.z, c.health, creatures::maxHealth(c), c.level, c.experience};
        return true;
    };
    modHost.showText = [&](double x, double y, double z, const std::string &text, float r, float g, float b, float size) {
        if (!activeInterface)
            return false;
        ui::FloatingText t;
        t.anchor = {x, y, z};
        t.text = ui::utf16(text);
        t.colour = {r, g, b, 1};
        t.size = size;
        activeInterface->floating.push_back(t);
        return true;
    };
    modHost.showBanner = [&](const std::string &name, const std::string &detail) {
        if (!activeInterface)
            return false;
        activeInterface->showLandName(name, detail);
        return true;
    };
    std::map<std::pair<int32_t, int32_t>, std::vector<AvStatic>> modStatics; // the last list each zone gave
    modHost.getStatics = [&](int32_t zx, int32_t zy) -> const std::vector<AvStatic> * {
        const worldgen::Zone *zone = activeView ? activeView->held().zone(zx, zy) : nullptr;
        if (!zone)
            return nullptr;
        std::vector<AvStatic> &list = modStatics[{zx, zy}];
        list.clear();
        for (const worldgen::StaticObject &o : zone->statics)
            list.push_back({o.kind, (double)o.x / 65536.0, (double)o.y / 65536.0, (double)o.z / 65536.0, o.rotation,
                            {o.scale[0], o.scale[1], o.scale[2]}});
        return &list;
    };
    ModLoader mods(modHost);
    mods.load(executableFolder() / "Mods", splitModList(config.text("Mods", "Load")), content);

    if (!options.play.empty()) {
        // Plays one sound to the end, then exits.
        if (!audio.init(content, &error) || !audio.play(options.play)) {
            std::fprintf(stderr, "cannot play %s: %s\n", options.play.c_str(),
                         error.empty() ? "unknown sound" : error.c_str());
            return 1;
        }
        while (audio.busy())
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        return 0;
    }
    if (options.listing) {
        for (const std::string &key : content.list(options.list))
            std::printf("%s\n", key.c_str());
        return 0;
    }

    if (!glfwInit()) {
        std::fprintf(stderr, "cannot start GLFW\n");
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(config.number("Window", "Width", 1280), config.number("Window", "Height", 720),
                                          "AlphaVulkan", nullptr, nullptr);
    int status = 0;
    {
        Renderer renderer;
        AntiAliasing aa;
        aa.load(config);
        aa.taa = aa.taa && !options.noTaa;
        aa.smaa = aa.smaa && !options.noSmaa;
        ShadowSettings shadowSettings;
        MoonSettings moonSettings;
        moonSettings.load(config);
        shadowSettings.load(config);
        shadowSettings.enabled = shadowSettings.enabled && !options.noShadows;
        if (!window || !renderer.init(window, config.number("Window", "VSync", 1) != 0, aa, shadowSettings, &error)) {
            std::fprintf(stderr, "%s\n", window ? error.c_str() : "cannot open a window");
            glfwTerminate();
            return 1;
        }

        audio.init(content); // (without a sound device the viewer runs silent)
        if (options.world) {
            Options worldRun = options;
            worldRun.worldOptions.motion.load(config);
            worldRun.water = config.text("Water", "Style", "Omega") == "Alpha" ? WaterLook::Alpha : WaterLook::Omega;
            worldRun.worldOptions.swimmableWater = config.number("Water", "Swimmable", 1) != 0;
            worldRun.extendedQuests = true; // core feature; no opt-in setting
            // Character screens §1.2: a world chosen on the front screens starts in this window.
            for (;;) {
                std::optional<Options> restart;
                status = runWorld(window, renderer, content, game, worldRun, shadowSettings, moonSettings, mods,
                                  activeScene, activeView, activePlayer, activeInterface, audio, restart);
                if (!restart)
                    break;
                worldRun = *restart;
            }
            activeScene = nullptr;
            activeView = nullptr;
            activePlayer = nullptr;
            activeInterface = nullptr;
            mods.shutdown();
            glfwDestroyWindow(window);
            glfwTerminate();
            return status;
        }
        Viewer viewer;
        activeScene = &viewer.scene;
        viewer.content = &content;
        viewer.renderer = &renderer;
        viewer.window = window;
        viewer.models = content.list(".cub");
        viewer.ground = !options.noGround;
        viewer.sway = options.sway;
        viewer.orbit.yaw = options.yaw;
        viewer.orbit.pitch = options.pitch;
        viewer.scene.timeOfDayMs = std::fmod(options.hour, 24.f) * 3600000.0;
        size_t first = 0;
        if (!options.model.empty()) {
            auto it = std::find(viewer.models.begin(), viewer.models.end(), options.model);
            if (it == viewer.models.end()) {
                std::fprintf(stderr, "no model named %s (see --list .cub)\n", options.model.c_str());
                status = 1;
            }
            first = (size_t)(it - viewer.models.begin());
        }
        if (!status && !viewer.show(first))
            status = 1;
        CreatureView creatureView(renderer, content);
        if (!status && options.creature >= 0)
            viewer.showCreature(options.creature);

        glfwSetWindowUserPointer(window, &viewer);
        glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int, int action, int) {
            auto *v = (Viewer *)glfwGetWindowUserPointer(w);
            if (action == GLFW_RELEASE)
                return;
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(w, 1);
            else if (key == GLFW_KEY_RIGHT && v->creature >= 0)
                v->showCreature(v->creature + 1);
            else if (key == GLFW_KEY_LEFT && v->creature >= 0)
                v->showCreature(v->creature - 1);
            else if (key == GLFW_KEY_W && v->creature >= 0)
                v->walking = !v->walking;
            else if (key == GLFW_KEY_RIGHT)
                v->show(v->current + 1);
            else if (key == GLFW_KEY_LEFT)
                v->show(v->current + v->models.size() - 1);
            else if (key == GLFW_KEY_S)
                v->sway = !v->sway;
            else if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
                const double hour = 3600000.0 * (key == GLFW_KEY_UP ? 1 : -1);
                v->scene.timeOfDayMs = std::fmod(v->scene.timeOfDayMs + hour + alpha::DAY_MS, alpha::DAY_MS);
            }
        });
        glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int) {
            auto *v = (Viewer *)glfwGetWindowUserPointer(w);
            if (button == GLFW_MOUSE_BUTTON_LEFT)
                v->dragging = action == GLFW_PRESS;
            glfwGetCursorPos(w, &v->lastX, &v->lastY);
        });
        glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y) {
            auto *v = (Viewer *)glfwGetWindowUserPointer(w);
            if (v->dragging) {
                v->orbit.yaw += (float)(x - v->lastX) * 0.4f;
                v->orbit.pitch = std::clamp(v->orbit.pitch + (float)(y - v->lastY) * 0.4f, -89.f, 89.f);
            }
            v->lastX = x;
            v->lastY = y;
        });
        glfwSetScrollCallback(window, [](GLFWwindow *w, double, double dy) {
            auto *v = (Viewer *)glfwGetWindowUserPointer(w);
            v->orbit.distance = std::max(2.f, v->orbit.distance * (float)std::pow(0.9, dy));
        });

        // A capture draws a few frames first so the window has settled at its real size and the
        // TAA history has filled.
        int framesUntilCapture = options.capture.empty() ? -1 : 16;
        // The world's millisecond clock (vegetation-wind.md §2); the viewer runs it in real time.
        const double start = glfwGetTime();
        double lastFrame = start;
        while (!status && !glfwWindowShouldClose(window)) {
            glfwPollEvents();
            const double now = glfwGetTime();
            const float frameMs = (float)(now - lastFrame) * 1000.f;
            mods.frame(now - lastFrame);
            lastFrame = now;
            const bool capturing = framesUntilCapture == 0;
            const FrameConstants frame = makeFrame(viewer.orbit.camera(renderer.size()), viewer.scene, shadowSettings, moonSettings);
            const double clockMs = (glfwGetTime() - start) * 1000;
            std::vector<DrawItem> items;
            if (viewer.creature >= 0) {
                // Walking in place: the walk cycle runs on a forward speed, the creature stays put.
                creatures::Motion motion;
                motion.velocity = viewer.walking ? glm::vec3(0, 3, 0) : glm::vec3(0);
                const creatures::Appearance &a = viewer.looks;
                const float toUnits = 11.2f / a.box.x;
                std::vector<DrawItem> parts;
                creatureView.draw(parts, a, viewer.pose, motion, glm::vec3(0, 0, a.box.z * 0.5f), frameMs);
                for (DrawItem &item : parts) {
                    item.world = glm::scale(glm::mat4(1), glm::vec3(toUnits)) * item.world;
                    items.push_back(item);
                }
            } else {
                items.push_back({viewer.modelMesh,
                                 viewer.sway ? motion::plantShear(clockMs, 0, viewer.modelHeight, 1) : glm::mat4(1)});
            }
            if (viewer.groundMesh)
                items.push_back({viewer.groundMesh, glm::mat4(1), false});
            const bool drawn = renderer.draw(frame, drawStars(viewer.scene), items,
                                             capturing ? pathOfUtf8(options.capture) : fs::path());
            if (capturing) {
                if (!drawn) {
                    std::fprintf(stderr, "capture failed\n");
                    status = 1;
                }
                break;
            }
            if (framesUntilCapture > 0 && drawn)
                framesUntilCapture--;
        }
        // Mods say goodbye while the viewer they may still ask about is alive.
        mods.shutdown();
        activeScene = nullptr;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return status;
}
