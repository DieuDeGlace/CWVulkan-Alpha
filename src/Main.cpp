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
#include "SoundRules.h"
#include "Config.h"
#include "Frame.h"
#include "SimulationFrame.h"
#include "Motion.h"
#include "CubModel.h"
#include "DataStore.h"
#include "Install.h"
#include "OwnerContent.h"
#include "GenerationProfile.h"
#include "ItemRequest.h"
#include "Mods.h"
#include "Renderer.h"
#include "CreatureView.h"
#include "CreatureLights.h"
#include "CreatureShading.h"
#include "Player.h"
#include "creatures/Strike.h"
#include "creatures/RayActions.h"
#include "creatures/Inventory.h"
#include "creatures/Credits.h"
#include "creatures/Spirits.h"
#include "creatures/Names.h"
#include "net/RemoteCreatures.h"
#include "creatures/Airships.h"
#include "creatures/Npcs.h"
#include "creatures/Pets.h"
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
    bool zoneRadiusSpecified = false;
    float travel = 0; // --travel: blocks to fly east before a capture
    std::string connect, name;
    std::string saveWorld; // --save-world: a single-player world kept in Save/
    int character = -1;    // --character: who plays in it (characters.db index)
    bool extendedQuests = true; // required core engine quests (docs/design/quests.md)
    uint16_t port = 12345;
};

Options parse(int argc, char **argv) {
    Options o;
    bool seedGiven = false;
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
            o.start = o.world = o.walk = true;
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
        else if (!std::strcmp(argv[i], "--seed")) {
            o.worldOptions.seed = (uint32_t)std::strtoul(next().c_str(), nullptr, 10);
            seedGiven = true;
        }
        else if (!std::strcmp(argv[i], "--radius")) {
            o.worldOptions.radius = std::clamp(std::atoi(next().c_str()), 0, 8);
            o.zoneRadiusSpecified = true;
        }
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
    if (o.start && !seedGiven)
        o.worldOptions.seed = GetTickCount();
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
    net::WorldIdentity activeWorld{worldOptions.seed, options.saveWorld};
    if (!options.connect.empty()) {
        std::string error;
        if (!client.connect(options.connect, options.port, &error)) {
            std::fprintf(stderr, "Error: could not connect to %s: %s\n", options.connect.c_str(), error.c_str());
            return 1;
        }
        activeWorld = client.worldIdentity();
        worldOptions.seed = activeWorld.seed;
        client.say("\x01" "AV1"); // AlphaVulkan's hello (docs/design/quests.md); Alpha servers pass it on as chat
        std::fprintf(stderr, "Connected. Player id %llu, world %s.\n", (unsigned long long)client.id(), activeWorld.name.c_str());
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
    constexpr int64_t homeFixed = int64_t(worldgen::HOME) * 65536;
    constexpr std::array<int64_t, 3> homeAnchor{homeFixed, homeFixed, 0};
    const bool atHome = worldOptions.x == worldgen::HOME && worldOptions.y == worldgen::HOME;
    auto savedVisit = onlineCharacter && atHome
        ? onlineCharacter->visitPosition(activeWorld.seed, activeWorld.name) : std::nullopt;
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
        savedVisit = session->startFixedPosition();
    }
    if (savedVisit) {
        // The view starts in a terrain column. The actor keeps the original fixed coordinates.
        worldOptions.x = int32_t((*savedVisit)[0] >> 16);
        worldOptions.y = int32_t((*savedVisit)[1] >> 16);
        worldOptions.z = float((*savedVisit)[2] / 65536.0);
    }
    if (session)
        activeWorld = {session->seed(), session->name()};
    std::optional<WorldSession> remoteMap;
    const auto openRemoteMap = [&](const net::WorldIdentity &world) {
        remoteMap.reset();
        std::string error;
        if (!remoteMap.emplace().openRemoteMap(executableFolder(), world.name, world.seed, &error)) {
            remoteMap.reset();
            std::fprintf(stderr, "Cannot open map for %s: %s\n", world.name.c_str(), error.c_str());
        }
    };
    if (client.connected())
        openRemoteMap(activeWorld);
    const auto mapSession = [&]() -> WorldSession * {
        if (session)
            return &*session;
        return remoteMap ? &*remoteMap : nullptr;
    };
    // The character playing: the session's, or the one brought online.
    const auto playing = [&]() -> CharacterRecord * {
        if (session && session->playing())
            return &*session->playing();
        return onlineCharacter ? &*onlineCharacter : nullptr;
    };
    std::fprintf(stderr, "generating the world round (%d, %d), seed %u...\n", worldOptions.x, worldOptions.y,
                 worldOptions.seed);
    const auto persistMapTile = [&](int32_t zx, int32_t zy, const worldgen::MapTile &tile) {
        WorldSession *saved = mapSession();
        if (!saved)
            return;
        TileSave save;
        save.base = tile.base;
        save.sx = tile.sx, save.sy = tile.sy, save.sz = tile.sz;
        save.rgb = tile.rgb;
        for (const glm::ivec3 &m : tile.markers)
            save.markers.push_back({m.x, m.y, m.z});
        saved->saveTile(zx, zy, save);
    };
    const fs::path optionsFile = executableFolder() / "options.cfg";
    GameOptions game;
    if (const GLFWvidmode *desktop = glfwGetVideoMode(glfwGetPrimaryMonitor()))
        game[GameOptions::RESOLUTION_X] = desktop->width, game[GameOptions::RESOLUTION_Y] = desktop->height;
    game.load(optionsFile);
    worldOptions.renderDistance = game[GameOptions::RENDER_DISTANCE];
    const auto configuredZoneRadius = [&] {
        return std::max(1, int32_t(std::ceil(float(game[GameOptions::RENDER_DISTANCE]) * 0.01f * 384.f / 256.f)));
    };
    if (!options.zoneRadiusSpecified)
        worldOptions.radius = configuredZoneRadius();
    const bool localEntry = options.start || (options.walk && options.connect.empty() && !options.saveWorld.empty());
    worldOptions.preloadTerrain = !localEntry || !options.capture.empty() || options.travel != 0 ||
                                  options.crater || options.quest;
    WorldView view(renderer, content, worldOptions, persistMapTile);
    if (remoteMap)
        view.restoreDiscoveryCount(remoteMap->discoveryCount());
    activeView = &view;
    // World items §4: each zone's items as generated, replaced by its save.
    WorldItems worldItems;
    // Interaction §4, §6: the statics' states, timers and occupants.
    StaticStates statics;
    // Block edits §3: each held zone's edits, saved with the zone.
    BlockEditLists blockEdits;
    std::vector<BlockEdit> frameBlockEdits;
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
    // Held weapon-attachment contract: the weapon points of each creature's last
    // pose refresh. Lights read them before this frame's refresh replaces them.
    std::map<uint64_t, std::array<std::optional<terrain_lights::FixedPosition>, 2>> weaponPoints;
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
        view.applyEdits({e}, TerrainEditSource::Live);
        blockEdits.add(e);
        return true;
    };
    struct HookReset {
        ModLoader &mods;
        ~HookReset() {
            mods.hooks().getMission = {}, mods.hooks().setBlock = {}, mods.hooks().getCreatures = {};
            mods.hooks().spawnParticles = {}, mods.hooks().getQuests = {};
            mods.hooks().say = {}, mods.hooks().showChat = {};
            mods.hooks().tell = {}, mods.hooks().playerName = {};
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
                if (!saved->blockEdits.empty())
                    view.applyEdits(kept, TerrainEditSource::Saved, glm::ivec2(zx, zy));
            }
        worldItems.takeChanged(); // (what came from the save is already saved)
        statics.takeChanged();
    };
    for (const auto &[key, zone] : view.held().zones()) // (the zones made before the first frame)
        zoneReady(key.first, key.second);
    view.onZoneReady = zoneReady;
    // Screens §5.1.6: land pictures and map tiles go into map_<name>.db as they are made.
    view.onLandReady = [&](int32_t lx, int32_t ly, const worldgen::RegionCell &cell, glm::ivec2 origin, const CubModel &picture) {
        WorldSession *saved = mapSession();
        if (!saved)
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
        saved->saveLand(lx, ly, save);
    };
    view.savedTile = [&](int32_t zx, int32_t zy) -> std::optional<worldgen::MapTile> {
        WorldSession *mapSave = mapSession();
        if (!mapSave)
            return std::nullopt;
        const std::optional<TileSave> saved = mapSave->loadTile(zx, zy);
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
        WorldSession *mapSave = mapSession();
        if (!mapSave)
            return;
        const worldgen::World &world = view.terrainWorld();
        const int32_t rx = worldgen::floorDiv((int32_t)std::floor(at.x), worldgen::REGION);
        const int32_t ry = worldgen::floorDiv((int32_t)std::floor(at.y), worldgen::REGION);
        for (int32_t i = rx - 3; i <= rx + 3; i++)
            for (int32_t j = ry - 3; j <= ry + 3; j++) {
                if (remoteMap && (uint32_t(i) >= 1024 || uint32_t(j) >= 1024))
                    continue;
                if (mapRegions.count({i, j}))
                    continue;
                const auto saved = mapSave->loadRegion(i, j);
                if (remoteMap && !saved && !world.region(i, j))
                    continue;
                auto &[record, written] = mapRegions[{i, j}];
                if (saved) {
                    record = *saved;
                    written = record.serialize();
                    for (int32_t k = 0; k < 64 * 64; k++)
                        view.restoreZone(64 * i + k / 64, 64 * j + k % 64, record.zones[k].flags);
                }
            }
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
                mapSave->saveRegion(i, j, record);
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
    net::RemoteCreatures remotes; // remote smoothing: received creatures' live and drawn poses
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
            player->combat().setEquipment(slot, inventory.equipment[slot]);
    };
    creatures::Npcs npcs{worldRandom};
    npcs.enableClientMotion();
    // World objects: the airships (offline this program runs them; online none arrive).
    creatures::Airships airships;
    if (const auto model = view.prefab("airship.cub"))
        airships.setModel(*model);
    npcs.obstacles = &airships;
    npcs.placeOnObject = [&](int64_t key, creatures::Body &body, float &yaw) { return airships.place(key, body, yaw); };
    // §2.4 (unconnected world): drop and copy ships round the players; §6: then each frame.
    const auto refreshAirships = [&] {
        std::vector<creatures::Airships::Raw> players;
        if (player) {
            const int64_t *p = player->body().position;
            players.push_back({p[0], p[1], p[2]});
        }
        airships.refresh(players, view.held());
    };
    const auto stepAirships = [&](int32_t elapsed) {
        // Steering runs only in an unconnected world; the yaw carried is the facing (+0x30).
        airships.step(elapsed, !client.connected(), view.held(), [&](int64_t key, creatures::Airships::Carried &out) {
            if (player && std::bit_cast<int64_t>(player->combat().id) == key) {
                out = {player->body().position, player->body().follow, &player->facing()};
                return true;
            }
            for (creatures::Npc &n : npcs.npcs())
                if (std::bit_cast<int64_t>(n.fighter.id) == key) {
                    out = {n.body.position, n.body.follow, &n.yaw};
                    return true;
                }
            return false;
        });
    };
    npcs.creditItems = [&](creatures::Combatant &actor) -> creatures::CreditItems {
        const auto storage = creatures::actorCreditItems(actor);
        if (player && &actor == &player->combat()) {
            creatures::copyItemFields(storage.source, inventory.equipment[12]);
            return {inventory.equipment[12], storage.snapshot};
        }
        return storage;
    };
    npcs.onFatalItem = [&](const creatures::ItemRecord &item) { inventory.add(item); };
    npcs.objects = [&](glm::dvec3 lo, glm::dvec3 hi, int axis, double move, const creatures::IgnoredStatics *ignored) {
        return statics.blocks(view.held(), lo, hi, axis, move, ignored);
    };
    npcs.routeActivate = [&](const creatures::route::Record &record) { statics.activateRouteRecord(view.held(), record); };
    npcs.seats = [&](glm::ivec3 block, double ms) { return statics.seats(view.held(), block, ms); };
    npcs.seatFree = [&](const creatures::Seat &s) { return statics.free(s); };
    npcs.sightRecords = [&](int32_t zx, int32_t zy, const creatures::visibility::RecordVisitor &visit) {
        statics.visibilityRecords(view.held(), zx, zy, visit);
    };
    npcs.routeRecords = [&](int32_t x, int32_t y, const creatures::route::RecordVisitor &visit) {
        statics.routeRecords(view.held(), x, y, visit);
    };
    npcs.routeColumns = [&](int32_t x, int32_t y) -> std::optional<creatures::route::Column> {
        const auto *zone = view.held().zoneAt(x, y);
        if (!zone) return std::nullopt;
        const auto &column = zone->column(x, y);
        return creatures::route::Column{column.bottom, int32_t(column.blocks.size()), std::as_bytes(std::span(column.blocks))};
    };
    npcs.sit = [&](creatures::Npc &n, const creatures::Seat &s, creatures::NpcFrame &frame) {
        if (n.fighter.action == 79)
            n.fighter.action = 0, n.fighter.stealth = 0;
        if (const worldgen::Zone *zone = view.held().zone(s.zx, s.zy))
            statics.use(*zone, s.index, n.fighter, n.body, n.yaw, worldItems, 0,
                        [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); },
                        [&] { return worldRandom.draw(); }, frame.sounds, &frame.staticStates);
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
    mods.hooks().playerName = [&](uint64_t id) -> std::optional<std::string> { // mod SDK v10
        if (id == 0 || (client.connected() && id == client.id()))
            return own.name();
        if (const auto it = receivedActors.find(id); it != receivedActors.end() && it->second.fighter().controller == 0)
            return it->second.name();
        return std::nullopt;
    };
    npcs.descriptorContext = [&](int32_t zx, int32_t zy) {
        return missions.descriptorContext(zx, zy, [&](int32_t rx, int32_t ry) {
            return view.terrainWorld().region(rx, ry) != nullptr;
        });
    };
    // Projectiles (alpha-projectiles.md): everything in flight, and their hits on NPCs for the
    // next offline step.
    creatures::Projectiles projectiles;
    std::vector<creatures::ShotTarget> projectileTargets;
    // Particles §3.2: the one list, and the bursts gathered this frame for it (§2.1).
    creatures::Particles particles;
    std::vector<creatures::Burst> bursts;
    double leafClockMs = -1; // the world clock the falling leaves last looked at (§7.1)
    // Audio §3, zone-fields ruling 1: the ambience's chunk slots and the zones' sounding records.
    sound::AmbienceGrid ambienceGrid;
    sound::ZoneAmbience zoneAmbience;
    mods.hooks().getQuests = [&] { // mod SDK v8
        std::vector<AvQuest> out;
        for (const quests::Quest &q : questBook.quests)
            out.push_back({q.id, q.kind, q.state, q.giver, q.level, q.count, q.required, q.site.x, q.site.y, q.site.z});
        return out;
    };
    // Audio §2: the listener L is the camera position, the follow camera's fixed eye once it is
    // placed; without one, the free camera's position (engine adapter).
    const auto listener = [&]() -> sound::Fixed {
        if (player && player->followCamera().placed())
            return player->followCamera().eyeFixed();
        return sound::fixedOf(view.worldPosition());
    };
    // Audio §1.2: the play routine, a sound at a fixed position heard from L: nothing past
    // 100 blocks, fading linearly to silence there; the pan is computed natively and discarded,
    // so none is applied (ruling audio-ui-sounds 1).
    const auto playFixed = [&](int32_t id, const sound::Fixed &at, float volume, float pitch) {
        if (const auto a = sound::attenuation(at, listener(), volume))
            if (const char *key = creatures::soundKey(id))
                audio.play(key, *a, pitch);
    };
    // §1.3: a sound record, its position carried as float blocks. Right after a record with id
    // 81 or 82 the camera shakes, at any distance.
    const auto playAt = [&](int32_t id, glm::dvec3 at, float volume, float pitch) {
        playFixed(id, sound::recordPosition(at), volume, pitch);
        if ((id == 81 || id == 82) && player)
            player->followCamera().shake = 1;
    };
    mods.hooks().spawnParticles = [&](double x, double y, double z, const float velocity[3], const float rgb[3],
                                      float size, int32_t count, int32_t style, float spread) {
        creatures::Burst b = creatures::Burst::at({x, y, z}, {velocity[0], velocity[1], velocity[2]},
                                                  {rgb[0], rgb[1], rgb[2], 1}, size, std::clamp(count, 0, 5000), style);
        b.spread = spread;
        bursts.push_back(b);
        return true;
    };
    creatures::ShotEffects playerShot;
    creatures::NpcFrame playerActionFrame;
    creatures::creation::Configuration playerCreationConfig;
    bool frontListsStale = true; // character screens: reread the saves' lists
    // Character screens §4.1: the figures of the character panels, posed and turning.
    struct PanelFigure {
        creatures::Appearance looks;
        creatures::Combatant fighter;
        creatures::PoseState pose;
    };
    std::vector<PanelFigure> panelFigures;
    std::vector<std::pair<MeshId, glm::vec3>> worldPreviews; // §4.2: each world entry's preview mesh
    // §4.5: the world list as held in memory (read once; a deletion shifts it, while the disk
    // keeps Alpha's rewrite order until the next start).
    std::optional<std::vector<WorldEntry>> worldMemory;
    // §3.9 Apply: the record the last Apply made (kit and level-1 grants), saved by Create Character.
    std::optional<CharacterRecord> creationRecord;
    glm::dvec2 benchMouse{0};
    int chosenCharacter = options.character;
    // Last observed recipe level. Saved characters synchronize this after load;
    // only the engine's unsaved diagnostic character needs a first-frame grant.
    int32_t grantedLevel = 0;
    std::vector<ItemRequest> offlineItemRequests;
    const auto queueItemRequest = [&](ItemRequest request) {
        if (client.connected())
            client.sendItemRequest(request.packet());
        else
            offlineItemRequests.push_back(std::move(request));
    };
    const auto useConsumable = [&](const creatures::ItemRecord &item) {
        const uint8_t subtype = creatures::itemSubtype(item);
        if (subtype == 7) {
            queueItemRequest({ItemRequest::UseItem, item});
            return;
        }
        auto &fighter = player->combat();
        fighter.copyHeld(item);
        fighter.action = subtype == 1 || subtype == 2 ? 80 : 81;
        fighter.actionMs = 0;
        fighter.dodgeMs = 0;
        player->body().dodgeMs = 0;
    };
    const auto playInterfaceSound = [&](int32_t id) {
        if (const char *key = creatures::soundKey(id))
            audio.play(key);
    };
    const auto useQuickItem = [&](ui::QuickItemWheel::Slot choice) {
        const auto &fighter = player->combat();
        const auto [bag, slot] = choice;
        if (creatures::itemLevel(inventory.bags[bag][slot].item) > fighter.level) {
            playInterfaceSound(creatures::SOUND_REFUSED);
            return false;
        }
        inventory.useConsumable(bag, slot, fighter.hitStun <= 0, useConsumable);
        return true;
    };
    // Interaction §2.4: the world item E would pick up, (zone x, zone y, index).
    std::optional<std::array<int32_t, 3>> itemTarget;
    bool eWasDown = false;
    bool tWasDown = false; // pets §13: T calls the pet back
    bool qWasDown = false;
    if (options.walk) {
        creatures::Appearance looks;
        int32_t type = 2;
        creatures::CharacterChoice choice;
        if (playing())
            choice = creatures::CharacterChoice::parse(playing()->extra.data());
        creatures::generate(type, looks, &choice, [] { return 0; });
        player.emplace(view, looks, worldRandom);
        if (options.start) {
            player->changeWorldPosition(homeAnchor);
            player->resetCreationPose();
        } else if (savedVisit) {
            player->changeWorldPosition(*savedVisit);
        } else if (session && session->playing() && atHome) {
            player->changeWorldPosition(homeAnchor);
        }
        player->objects = [&](glm::dvec3 lo, glm::dvec3 hi, int axis, double move, const creatures::IgnoredStatics *) {
            return statics.blocks(view.held(), lo, hi, axis, move);
        };
        activePlayer = &*player;
        player->combat().id = client.connected() ? client.id() : 1; // native initial ID; a received zero remains valid
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
            const auto broken = breakCells(rawBlocks(at), 8, CellRounding::FloorAdjusted,
                                           session ? (int32_t)session->clock.day : 0,
                                           [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z); });
            for (const BrokenCell &cell : broken) // particles §5.2
                bursts.push_back(creatures::blockBurst(cell.at, glm::vec3(cell.colour)));
            const auto removed = removals(broken);
            view.applyEdits(removed, TerrainEditSource::Live);
            for (const BlockEdit &e : removed)
                blockEdits.add(e);
            std::fprintf(stderr, "--crater: %zu blocks removed\n", removed.size());
        }
    }
    if (worldOptions.preloadTerrain)
        std::fprintf(stderr, "%s\n", view.status().c_str());
    float travelled = 0;
    view.setLook(options.lookGiven ? options.yaw : 30.f, options.lookGiven ? options.pitch : 15.f);
    Scene scene;
    alpha::FogDistance fogDistance;
    scene.timeOfDayMs = std::fmod(options.hour, 24.f) * 3600000.0;
    npcs.breakBlocks = [&](const std::array<int64_t, 3> &centre, int32_t radius, CellRounding rounding,
                           std::vector<creatures::Burst> &effects) {
        if (rounding == CellRounding::Truncate && client.connected())
            return false;
        const auto broken = breakCells(centre, radius, rounding, scene.day,
            [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z); });
        if (broken.empty())
            return false;
        for (const BrokenCell &cell : broken)
            effects.push_back(creatures::blockBurst(cell.at, glm::vec3(cell.colour)));
        const auto removed = removals(broken);
        view.applyEdits(removed, TerrainEditSource::Destruction);
        frameBlockEdits.insert(frameBlockEdits.end(), removed.begin(), removed.end());
        return true;
    };
    npcs.lightBrokenBlocks = [&](const std::array<int64_t, 3> &centre, float radius) {
        if (!player)
            view.relightAfterDestruction(centre, radius);
    };
    if (session) {
        scene.timeOfDayMs = session->clock.timeOfDayMs;
        scene.day = (int)session->clock.day;
    }
    activeScene = &scene;
    npcs.onFatalObject = [&](const creatures::Combatant &fighter, creatures::NpcFrame &frame) {
        if (!client.connected())
            statics.fatal(view.held(), fighter.backing, frame);
    };
    npcs.onFatalLoot = [&](const creatures::Combatant &fighter, const creatures::Body &body,
                           std::span<const creatures::CountedItem> items) {
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
    npcs.onFatalBacking = [&](const creatures::Combatant &fighter) {
        const auto &backing = fighter.backing;
        if (!client.connected() && backing.x >= 0 && backing.y >= 0 && backing.x < 65536 && backing.y < 65536 &&
            view.terrainWorld().region(backing.x / 64, backing.y / 64))
            npcs.markFatalBacking(fighter, view.held(), scene.day);
    };

    // The interface (alpha-ui-screens.md): the start screen and the system menu.
    // The dictionary the interface reads names from (dict_en.xml, data4.db).
    Dictionary dictionary;
    if (const auto bytes = content.read("dict_en.xml"))
        dictionary.parse(std::string(bytes->begin(), bytes->end()));
    ui::Interface interface;
    const auto presentTimedHeal = [&](const creatures::HitRecord &hit) {
        if (hit.target == player->combat().id)
            if (auto text = ui::timedHealText(hit, player->appearance().box.z))
                interface.floating.push_back(*text);
    };
    activeInterface = &interface;
    interface.speechDraw = [&] { return worldRandom.draw(); }; // talk §0: the one shared stream
    mods.hooks().say = [&](const std::string &text) {
        interface.chatSubmitted.push_back(text); // (as if typed, /commands included)
        return true;
    };
    mods.hooks().showChat = [&](const std::string &text, float r, float g, float b) {
        interface.chatAdd(text + "\n", {r, g, b});
        return true;
    };
    mods.hooks().tell = [&](uint64_t to, const std::string &text) { // mod SDK v10: the local player only
        if (to != 0 && !(client.connected() && to == client.id()))
            return false;
        interface.chatAdd(text + "\n", {255, 255, 255});
        return true;
    };
    interface.worldPreview = [&](size_t index) {
        return index < worldPreviews.size() ? worldPreviews[index] : std::pair<MeshId, glm::vec3>{0, glm::vec3(0)};
    };
    interface.characterParts = [&](size_t index, const ui::Interface::FigureMotion &m) {
        // Character screens §3.2: the panel figure turns and walks on the spot only while hovered.
        std::vector<std::pair<uint32_t, glm::mat4>> out;
        if (index >= panelFigures.size())
            return out;
        PanelFigure &figure = panelFigures[index];
        creatures::Motion motion;
        motion.yawDegrees = m.yawDegrees;
        motion.velocity = m.velocity;
        motion.intent = m.intent;
        motion.fighter = &figure.fighter;
        for (const auto &[mesh, matrix] : creatureView.parts(figure.looks, figure.pose, motion, m.dtMs))
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
    const auto announceCube = [&] {
        interface.manaCubes = manaCubes;
        interface.chatAdd("You receive a mana cube.\n", {100, 150, 255});
        if (const char *key = creatures::soundKey(0x1e))
            audio.play(key);
        if (manaCubes % 4 == 0) {
            interface.playManaCubes();
            interface.chatAdd("You gain one skill point.\n", {150, 128, 255});
        }
    };
    const auto announceItem = [&](const creatures::ItemRecord &item, const std::string &prefix) {
        interface.chatAdd(prefix, {255, 255, 255});
        const glm::vec3 colour = creatures::itemType(item) == 12 ? glm::vec3(255) : glm::trunc(creatures::nameColour(item) * 255.f);
        interface.chatAdd(receivedText(item, dictionary), colour);
        interface.chatAdd(".\n", {255, 255, 255});
    };
    // Direct engine quest/reward adapters supply local items outside the frame grant list.
    const auto receive = [&](const creatures::ItemRecord &item) {
        const uint8_t type = creatures::itemType(item);
        mods.itemReceived(type, creatures::itemSubtype(item), creatures::itemLevel(item));
        if (type == 25) {
            manaCubes = std::bit_cast<int32_t>(uint32_t(manaCubes) + 1u);
            announceCube();
        } else {
            inventory.add(item);
            announceItem(item, "You receive ");
        }
    };
    const creatures::BlockType worldBlock = [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); };
    const std::function<int()> worldDraw = [&] { return worldRandom.draw(); };
    const auto useStaticRequest = [&](const std::array<int32_t, 3> &target, bool authoritative, creatures::NpcFrame &frame) {
        auto &fighter = player->combat();
        const auto [x, y, index] = target;
        if (const auto *zone = view.held().zone(x, y))
            statics.use(*zone, index, fighter, player->body(), player->facing(), worldItems, scene.day, worldBlock,
                        worldDraw, frame.sounds, &frame.staticStates, authoritative, &view.held());
        else if (fighter.action == 79)
            fighter.action = 0, fighter.stealth = 0;
    };
    const auto consumeOfflineItemRequests = [&](creatures::PlayerView &actor, creatures::NpcFrame &frame) {
        auto &fighter = actor.fighter;
        const auto &body = actor.body;
        for (const ItemRequest &request : offlineItemRequests) {
            const auto [x, y, index] = request.target;
            const glm::dvec3 at = body.blocks();
            switch (request.kind) {
            case ItemRequest::UseItem:
                npcs.useItemRequest(actor, request.item, frame);
                break;
            case ItemRequest::Talk:
                if (npcs.innRestAvailable(request.target)) {
                    scene.day = std::bit_cast<int32_t>(uint32_t(scene.day) + 1u);
                    scene.timeOfDayMs = 7 * 3600000.0;
                    if (session) {
                        session->clock.day = (uint32_t)scene.day;
                        session->clock.timeOfDayMs = 7 * 3600000u;
                    }
                    frame.sounds.push_back({at, 29, 1, 0.75f});
                    npcs.restAll(view.held());
                    missions.clearGenerated();
                }
                break;
            case ItemRequest::UseStatic:
                useStaticRequest(request.target, true, frame);
                actor.yaw = player->facing();
                break;
            case ItemRequest::PickUp:
                worldItems.pickUpFixed(x, y, index, body.position, manaCubes, [&](const creatures::ItemRecord &item) {
                    frame.grants.push_back({fighter.id, item});
                    if (creatures::itemType(item) != 25)
                        inventory.add(item);
                    frame.sounds.push_back({at, 45, (float)worldDraw() * 0.1f / 32767.f + 1.f, 1});
                });
                break;
            case ItemRequest::Drop:
                worldItems.drop(request.item, body.position, 1.f, scene.day, worldBlock, worldDraw);
                frame.sounds.push_back({at, 57, 1, 1});
                break;
            case ItemRequest::CallPet:
                npcs.callPetFixed(fighter, body.position);
                break;
            case ItemRequest::PlaceObject: {
                const auto edits = view.placeQueuedBlueprint(fighter.blueprintModel, fighter.blueprintRotation,
                                                             body.position, fighter.aim, scene.day);
                frameBlockEdits.insert(frameBlockEdits.end(), edits.begin(), edits.end());
                break;
            }
            default:
                break;
            }
        }
        offlineItemRequests.clear();
    };
    interface.itemModel = [&](const creatures::ItemRecord &record) {
        const uint16_t slot = creatures::itemModel(creatures::Item::parse(record.data()));
        if (slot == creatures::NO_MODEL)
            return ui::Interface::ItemModel{};
        return ui::Interface::ItemModel{creatureView.plainMesh(slot), creatureView.voxels(slot)};
    };
    interface.startScreen = options.start;
    // World entry. The engine restarts into a world, so the front-end camera
    // state is not carried over. A local world is entered natively through the
    // world-list Select or Create World button, both of which snap the camera
    // (held front-end and initial-state contracts). A server connection has no
    // snap and keeps the front-end targets; starting there is an engine adapter.
    if (player && !options.start) {
        creatures::FollowCamera &cam = player->followCamera();
        if (options.connect.empty()) {
            cam.distance = cam.targetDistance = 5;
            cam.pitch = cam.targetPitch = 90;
            cam.yaw = cam.targetYaw = 180;
        } else {
            cam.distance = cam.targetDistance = 10;
            cam.pitch = cam.targetPitch = 100;
        }
    }
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
        creatures::InventoryValue stock;
        for (const auto &bag : inventory.bags)
            for (const creatures::Stack &slot : bag)
                if (!slot.empty())
                    stock.appendUnit(slot.item);
        interface.vendorExport = [stock = std::move(stock)](creatures::InventoryValue &buffer) { buffer.copyFrom(stock); };
        interface.refreshVendor();
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
        // Audio §1.2 step 6: soundVolume scales every play. §5: the (never heard) music volumes
        // are not modelled; the engine has no music streamer.
        audio.setSoundVolume(game[GameOptions::SOUND_VOLUME]);
        if (player) {
            creatures::CameraOptions &look = player->cameraOptions();
            look.speed = game[GameOptions::CAMERA_SPEED];
            look.smoothness = game[GameOptions::CAMERA_SMOOTHNESS];
            look.invertY = game[GameOptions::INVERT_Y] != 0;
        }
        if (!first && !options.zoneRadiusSpecified)
            view.setRadius(configuredZoneRadius());
        view.setMeshRenderDistance(game[GameOptions::RENDER_DISTANCE]);
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
        bool cursorKnown = false; // no look delta until one sample in the current cursor mode
        bool dropOne = false; // screens §8.1.8: a left press over nothing with an item in hand
        bool revived = false; // interaction §5.2: this R press revived (the rest of the R block waits)
        Gamepad::PointerButtons pointerButtons;
        std::function<void(int, int, Gamepad::PointerButtons::Source)> mouseDispatch;
    } input{&view, &scene, &interface, &renderer, player ? &*player : nullptr};
    auto syncCursor = [&] {
        const int mode = !interface.wantsMouse() ? GLFW_CURSOR_DISABLED :
                         interface.creationCursorVisible() ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL;
        if (glfwGetInputMode(window, GLFW_CURSOR) != mode) {
            glfwSetInputMode(window, GLFW_CURSOR, mode);
            input.cursorKnown = false;
        }
    };
    glfwSetWindowUserPointer(window, &input);
    glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int, int action, int) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        if (action == GLFW_RELEASE)
            return;
        ui::Interface &ui = *in->interface;
        if (ui.controllerKeyboardOpen()) {
            if (key == GLFW_KEY_ESCAPE)
                ui.finishControllerKeyboard(false);
            else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
                ui.finishControllerKeyboard(true);
            else if (key == GLFW_KEY_BACKSPACE)
                ui.controllerKeyboardType(0, true);
            return;
        }
        // Screens §9: while the chat input is open, keys go to it.
        if (ui.chatOpen) {
            if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                ui.submitChat();
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
        // Screens §9, §11.2 (correction 112): Enter opens the chat whenever the key gate passes
        // (the five screen containers are handled above); no window condition. The chat box only
        // exists in game, hence the HUD test.
        if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && ui.hud && !ui.startScreen) {
            ui.chatOpen = true;
            return;
        }
        if (key == GLFW_KEY_M && action == GLFW_PRESS && !ui.startScreen && in->player) {
            ui.mapOpen = !ui.mapOpen; // screens §5.2.1
            return;
        }
        if (key == GLFW_KEY_TAB && !ui.startScreen && in->player) {
            ui.quickWheel.open = !ui.quickWheel.open;
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
        } else if (key == GLFW_KEY_R && action == GLFW_PRESS && in->player && in->player->combat().health <= 0 &&
                   !ui.quickWheel.open) {
            in->player->revive(); // §5.8
            in->revived = true;
        } else if (key == GLFW_KEY_O && !in->interface->startScreen) {
            in->interface->systemMenu = !in->interface->systemMenu; // O: the system panel
        } else if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
            const double hour = 3600000.0 * (key == GLFW_KEY_UP ? 1 : -1);
            in->scene->timeOfDayMs = std::fmod(in->scene->timeOfDayMs + hour + alpha::DAY_MS, alpha::DAY_MS);
        }
    });
    glfwSetCharCallback(window, [](GLFWwindow *w, unsigned int ch) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        if (in->interface->controllerKeyboardOpen() && ch >= 32 && ch < 0x10000)
            in->interface->controllerKeyboardType((char16_t)ch);
        else if (in->interface->chatOpen && ch >= 32 && ch < 0x10000)
            in->interface->chatInput += (char16_t)ch;
        else if (in->interface->front != ui::Interface::Front::None && ch >= 32 && ch < 0x10000)
            in->interface->frontType((char16_t)ch, false);
    });
    for (size_t button = 0; button < 3; button++)
        input.pointerButtons.update(button, glfwGetMouseButton(window, int(button)) == GLFW_PRESS,
                                    Gamepad::PointerButtons::Source::Mouse);
    input.mouseDispatch = [window](int button, int action, Gamepad::PointerButtons::Source source) {
        GLFWwindow *w = window;
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        if (button >= 0 && button < 3) {
            const auto transition = in->pointerButtons.update(size_t(button), action == GLFW_PRESS, source);
            if (!transition)
                return;
            action = *transition ? GLFW_PRESS : GLFW_RELEASE;
        }
        ui::Interface &menu = *in->interface;
        if (menu.controllerKeyboardOpen()) {
            if (action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(w, &x, &y);
                const auto size = in->renderer->size();
                int width, height;
                glfwGetWindowSize(w, &width, &height);
                if (width > 0 && height > 0) {
                    x *= double(size.width) / width;
                    y *= double(size.height) / height;
                }
                menu.controllerKeyboardPress({float(size.width), float(size.height)}, {float(x), float(y)}, button);
            }
            return;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && menu.mapOpen && !menu.startScreen &&
            !menu.systemMenu && !menu.optionsOpen) {
            menu.mapRelease(in->player ? &in->player->body() : nullptr);
            return;
        }
        if (action == GLFW_PRESS) {
            // Character screens §3.3: every press, on any screen, re-anchors the list drag.
            double x, y;
            glfwGetCursorPos(w, &x, &y);
            menu.pointerPress((float)y, button == GLFW_MOUSE_BUTTON_LEFT ? 0 : 1);
        }
        if (menu.front != ui::Interface::Front::None) {
            double x, y;
            glfwGetCursorPos(w, &x, &y);
            const VkExtent2D size = in->renderer->size();
            if (action == GLFW_PRESS) {
                menu.frontPress({(float)size.width, (float)size.height}, {(float)x, (float)y},
                                button == GLFW_MOUSE_BUTTON_LEFT ? 0 : 1);
                return;
            }
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                menu.frontRelease({(float)x, (float)y}); // §2.1: the buttons answer the release
                return;
            }
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
                // Held initial-state contract, in-game menu row 1: both distances,
                // then the first and third angle targets; current angles stay.
                if (in->player) {
                    creatures::FollowCamera &cam = in->player->followCamera();
                    cam.distance = cam.targetDistance = 0;
                    cam.targetPitch = 120, cam.targetYaw = 0;
                }
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
    };
    glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        in->mouseDispatch(button, action, Gamepad::PointerButtons::Source::Mouse);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y) {
        auto *in = (Input *)glfwGetWindowUserPointer(w);
        const bool menus = in->interface->wantsMouse();
        // Character screens §3.3: the list drag's smoothing follows every move (ms clock).
        in->interface->frontMouseMove((float)y, (uint32_t)(uint64_t)(glfwGetTime() * 1000.0));
        // The first sample only establishes a reference: where the cursor rests
        // when the window opens or changes cursor mode is not a movement.
        if (!in->cursorKnown) {
            in->cursorKnown = true;
            in->lastX = x;
            in->lastY = y;
            return;
        }
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
            // §5.2.2 (correction 54): n < 0 multiplies the target zoom by 0.9, otherwise ÷ 0.9.
            ui.mapZoomTarget = std::clamp(dy < 0 ? ui.mapZoomTarget * 0.9f : ui.mapZoomTarget / 0.9f, 0.01f, 10.f);
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
    const GLFWscrollfun wheelHandler = glfwSetScrollCallback(window, nullptr);
    glfwSetScrollCallback(window, wheelHandler);
    std::array<bool, 3> controllerMouse{}; // left/right/middle virtual UI buttons

    int status = 0;
    int framesUntilCapture = options.capture.empty() ? -1 : 16;
    double last = glfwGetTime();
    const double start = last;
    uint64_t simulationWorldCalls = 0, simulationInputFrames = 0;
    int32_t simulationMaxPiece = 0;
    uint32_t previousFrameTick = 0;
    uint32_t lightClockMs = 0;
    std::vector<DrawItem> items;
    std::vector<EffectVertex> effects; // weapon trails (trails §1.3)
    glm::dvec2 mapCursor(0); // the cursor last frame, for the map's drags
    // Interaction §1: what the crosshair picks each frame, a creature or a static (zone x,
    // zone y, index in the zone's list), and R's last state for its edge.
    uint64_t creatureTarget = 0;
    std::optional<std::tuple<int32_t, int32_t, size_t>> objectTarget;
    bool rWasDown = false;
    // Talk §3.3, §3.6: the talk partner the vendor window copies its stock from (0: none). It
    // is cleared at 5 blocks while the windows stay open on their last copy.
    uint64_t talkPartner = 0;
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
            creatures::updateSwingCount(actor, beforeMs, dtMs);
            creatures::chargeSound(actor, beforeMs, dtMs, player->body().blocks(), playerShot);
            creatures::actionSounds(actor, beforeMs, (float)dtMs, player->body().blocks(),
                                    [&] { return worldRandom.draw(); }, playerShot.sounds);
            const size_t firstLaunch = playerActionFrame.shots.size();
            creatures::launch(actor, player->body(), actor.aim, beforeMs, (float)dtMs,
                [&] { return worldRandom.draw(); }, playerActionFrame.shots, playerShot, {},
                [&](const creatures::Projectile &shot) {
                    auto resident = shot;
                    if (resident.kind == creatures::BOOMERANG_SHOT)
                        resident.scale = player->appearance().weaponScale;
                    projectiles.list.push_back(resident);
                });
            for (auto &shot : std::span{playerActionFrame.shots}.subspan(firstLaunch))
                if (shot.kind == creatures::BOOMERANG_SHOT)
                    shot.scale = player->appearance().weaponScale;
            if (!client.connected()) {
                creatures::PlayerView local{actor, player->body(), player->facing(), beforeMs};
                local.local = true;
                local.appearance = &player->appearance();
                local.creationConfig = &playerCreationConfig;
                npcs.dispatchPlayerActions({&local, 1}, dtMs,
                    [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); }, playerActionFrame);
            } else {
                auto &c = actor;
                auto &frame = playerActionFrame;
                frame.sounds.insert(frame.sounds.end(), playerShot.sounds.begin(), playerShot.sounds.end());
                frame.bursts.insert(frame.bursts.end(), playerShot.bursts.begin(), playerShot.bursts.end());
                frame.buffs.insert(frame.buffs.end(), playerShot.buffs.begin(), playerShot.buffs.end());
                playerShot.sounds.clear();
                playerShot.bursts.clear();
                playerShot.buffs.clear();
                if (c.health > 0 && creatures::isRayAction(c.action)) {
                    std::vector<creatures::ShotTarget> targets;
                    targets.reserve(receivedActors.size());
                    for (const auto &[id, record] : receivedActors) {
                        auto [resident, created] = otherFighters.try_emplace(id);
                        if (created)
                            resident->second.receive(id, record);
                        if (const auto *remote = remotes.find(id))
                            targets.push_back({&resident->second.fighter, &remote->live});
                    }
                    creatures::RayActionWorld world;
                    world.connected = true;
                    world.controlled = &c;
                    world.targets = targets;
                    world.sight = {
                        [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); },
                        npcs.sightRecords};
                    const auto rolls = [&] { return worldRandom.draw(); };
                    world.hit = [&](creatures::Combatant &target, const creatures::Body &body, creatures::HitInput input) {
                        const auto outcome = creatures::resolveHit(target, body.velocity, &c, input, rolls);
                        frame.emittedHits.insert(frame.emittedHits.end(), outcome.records.begin(), outcome.records.end());
                        frame.buffs.insert(frame.buffs.end(), outcome.buffs.begin(), outcome.buffs.end());
                        frame.sounds.insert(frame.sounds.end(), outcome.sounds.begin(), outcome.sounds.end());
                        frame.bursts.insert(frame.bursts.end(), outcome.bursts.begin(), outcome.bursts.end());
                        return outcome.hitAccepted;
                    };
                    world.launch = [&](const creatures::Projectile &shot) {
                        projectiles.list.push_back(shot);
                        frame.shots.push_back(shot);
                    };
                    creatures::stepRayAction(c, player->body(), dtMs, world,
                        {frame.emittedHits, frame.buffs, frame.sounds, frame.bursts}, rolls);
                    return;
                }
                // Server simulation §1, §6.1: online the player's own client decides its hits on
                // the creatures the server lists and sends them as packet 7.
                const bool melee = c.health > 0 && creatures::meleeAction(c.action);
                const creatures::StrikeWindow window =
                    melee ? creatures::strikeWindow(c, beforeMs, (float)dtMs) : creatures::StrikeWindow{};
                // Append local melee output to the same collector as received turns.
                for (const uint64_t key : std::exchange(c.counterPending, {}))
                    if (const auto other = receivedActors.find(key); other != receivedActors.end()) {
                        creatures::HitRecord counter;
                        counter.attacker = c.id, counter.target = key, counter.kind = creatures::HIT_COUNTER;
                        for (int axis = 0; axis < 3; axis++)
                            counter.position[axis] = other->second.position(axis);
                        frame.emittedHits.push_back(counter);
                    }
                bool hitAny = false;
                if (melee && window) {
                    const creatures::StrikeGeometry geometry =
                        creatures::strikeGeometry(c, player->body(), player->facing());
                    if (c.action == 0x1F)
                        if (auto consumed = creatures::consumeBuff(c, 9))
                            frame.buffs.push_back(*consumed);
                    const auto rolls = [&] { return worldRandom.draw(); };
                    const creatures::visibility::Scene sightScene{
                        [&](int32_t x, int32_t y, int32_t z) { return view.held().read(x, y, z).type(); },
                        npcs.sightRecords};
                    // K5: the listed creatures in ascending signed id.
                    std::vector<uint64_t> visitOrder;
                    visitOrder.reserve(receivedActors.size());
                    for (const auto &entry : receivedActors)
                        visitOrder.push_back(entry.first);
                    std::sort(visitOrder.begin(), visitOrder.end(), [](uint64_t a, uint64_t b) {
                        return std::bit_cast<int64_t>(a) < std::bit_cast<int64_t>(b);
                    });
                    for (const uint64_t id : visitOrder) {
                        const auto &other = receivedActors.at(id);
                        creatures::Combatant target = other.fighter();
                        target.id = id;
                        target.health = other.health();
                        target.controller = other.controller();
                        if (const auto resident = otherFighters.find(id); resident != otherFighters.end())
                            target.buffs = resident->second.fighter.buffs;
                        if (!(target.health > 0 || std::isnan(target.health)) ||
                            std::find(c.struck.begin(), c.struck.end(), id) != c.struck.end())
                            continue;
                        creatures::Body body;
                        for (int axis = 0; axis < 3; axis++)
                            body.position[axis] = other.position(axis);
                        body.size = other.appearance().box;
                        body.velocity = other.velocity();
                        body.intent = other.intent();
                        body.push = other.response();
                        if (!creatures::strikeReaches(geometry, player->body(), body))
                            continue;
                        // K5 step 4: the geometric test with option 1 and maximum 200.
                        const creatures::visibility::Position end{body.position[0], body.position[1], body.position[2]};
                        if (!creatures::visibility::visible(geometry.start, end, 1, 200.f, sightScene))
                            continue;
                        const bool hostile = creatures::hostilityTest(c, target);
                        // Strike-call-sites §6 online: actions 0x20/0x21 of class pair (_, 1) on a
                        // flagged frame heal a non-hostile target, sent as this client's hit.
                        const auto healCheck = [&] {
                            if ((c.action != 0x21 && c.action != 0x20) || !window.areaHeal || hostile ||
                                c.specialization != 1)
                                return;
                            std::array<int64_t, 3> at;
                            std::copy_n(body.position, 3, at.begin());
                            const creatures::HitRecord heal = creatures::areaHealRecord(c, id, at, rolls);
                            frame.emittedHits.push_back(heal);
                            frame.sounds.push_back({player->body().blocks(), 42, 1, 1});
                        };
                        if (!hostile) { // K5 step 5
                            healCheck();
                            continue;
                        }
                        if (target.dodgeMs != 0) { // K5 step 6: deferred, no heal check
                            if (std::find(c.deferred.begin(), c.deferred.end(), id) == c.deferred.end())
                                c.deferred.push_back(id);
                            continue;
                        }
                        // Construction precedes the K8 bookkeeping: first-target flag, the
                        // target's key and the swing byte, then the call.
                        const creatures::HitInput in = creatures::strikeInput(c, player->body(), body, rolls);
                        const bool first = c.struck.empty() && c.hitStun <= 0;
                        c.struck.push_back(id);
                        c.hitLanded = true;
                        const creatures::HitOutcome out = creatures::resolveHit(target, body.velocity, &c, in, rolls);
                        for (const auto &event : out.buffs) {
                            frame.buffs.push_back(event);
                            auto [resident, created] = otherFighters.try_emplace(event.target);
                            if (created)
                                resident->second.receive(id, other);
                            creatures::applyBuff(resident->second.fighter, event.record);
                        }
                        frame.emittedHits.insert(frame.emittedHits.end(), out.records.begin(), out.records.end());
                        frame.sounds.insert(frame.sounds.end(), out.sounds.begin(), out.sounds.end());
                        frame.bursts.insert(frame.bursts.end(), out.bursts.begin(), out.bursts.end());
                        if (out.hitAccepted) {
                            const creatures::StrikeAftermath after = creatures::afterStrike(c, first, hitAny, rolls);
                            if (after.buff)
                                frame.buffs.push_back(*after.buff);
                            if (after.upgradeSound)
                                frame.sounds.push_back({player->body().blocks(), 47, 1, 1});
                        }
                        healCheck(); // re-evaluated: never heals a hostile target
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
    const std::function<creatures::Combatant *(uint64_t)> frameActor = [&](uint64_t key) -> creatures::Combatant * {
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
    const creatures::BodyLookup liveActor = [&](uint64_t key) -> creatures::ActorBody {
        if (player && player->combat().id == key)
            return {&player->combat(), &player->body()};
        if (!client.connected())
            return npcs.actorBody(key);
        const auto *remote = remotes.find(key);
        return remote ? creatures::ActorBody{frameActor(key), &remote->live} : creatures::ActorBody{};
    };
    interface.creatureAt = [&](uint64_t id) -> std::optional<std::pair<glm::dvec3, float>> {
        const creatures::Body *body = liveActor(id).body;
        if (!body)
            return std::nullopt;
        return std::pair{glm::dvec3(body->follow[0], body->follow[1], body->follow[2]) / 65536.0,
                         body->size.z};
    };
    const auto talkTo = [&](const creatures::Combatant &actor, int32_t kind, glm::ivec2 zone, bool restart) {
        interface.talk(actor.id, [&]() -> std::vector<SpeechPage> {
            const worldgen::World &world = view.terrainWorld();
            const auto mark = [&](int32_t zx, int32_t zy) {
                if (uint32_t(zx) >= 65536 || uint32_t(zy) >= 65536)
                    return;
                // A saved map admits existing region records. Unsaved viewers
                // use the prepared zone summaries as their map backing.
                if (mapSession() ? mapRegions.contains({zx / 64, zy / 64}) : world.zoneEntry(zx, zy) != nullptr)
                    view.markDiscovered(zx, zy);
            };
            const uint32_t level = uint32_t(player->combat().level);
            const int32_t maxLevel = std::bit_cast<int32_t>(level + 2);
            if (actor.appearanceFlags & 0x80) {
                if (const auto *region = world.region(actor.backing.x / 64, actor.backing.y / 64))
                    for (const auto &location : region->records)
                        if (location.type != 0 && location.level <= maxLevel) {
                            const auto [zx, zy] = net::zoneForRawPosition(location.x, location.y);
                            mark(zx, zy);
                        }
                return {};
            }

            const auto placeAt = [&] { return cellLocation(world, zone.x / 8, zone.y / 8); };
            if (actor.characterClass != 132) {
                const int32_t minLevel = std::bit_cast<int32_t>(level - 2);
                if (const auto *place = placeAt(); place && (place->level < minLevel || place->level > maxLevel))
                    zone = {-1, -1};
            }
            auto pages = npcSpeech(dictionary, actor.id, actor.characterClass, kind, placeAt, worldRandom);
            if (actor.characterClass != 132)
                mark(zone.x, zone.y);
            return pages;
        }, restart);
    };
    const auto replayObjectiveInteraction = [&](uint64_t id) {
        if (!frameActor(id))
            return;
        if (const auto *actor = frameActor(id)) {
            const auto record = receivedActors.find(id);
            const bool remote = record != receivedActors.end();
            talkTo(*actor, remote ? record->second.talkKind() : own.talkKind(),
                   remote ? record->second.talkTarget() : own.talkTarget(), true);
        }
    };
    const auto applyReceivedBuffs = [&] {
        for (const auto &event : client.takeBuffs())
            if (auto *target = frameActor(event.target))
                creatures::applyBuff(*target, event.record);
    };
    const auto applyConnectedHit = [&](creatures::NpcFrame &frame, creatures::HitRecord &hit) {
        mods.playerHit(hit.attacker, hit.kind, hit.amount);
        player->receiveHit(hit, &frame.buffs);
    };
    const auto capturePet = [&](const creatures::HitRecord &hit, const creatures::Combatant *source) {
        if (source && creatures::captureTamedPet(player->combat(), inventory.equipment[12], *source, hit))
            wearInventory();
    };
    const auto presentConnectedHits = [&](const std::vector<creatures::HitRecord> &hits, size_t produced) {
        const auto *local = &player->combat();
        for (size_t index = 0; index < hits.size(); index++) {
            const auto &hit = hits[index];
            const auto *target = frameActor(hit.target);
            const auto *source = frameActor(hit.attacker);
            const bool toLocal = target == local;
            const bool fromLocal = source == local;
            const bool timedHeal = hit.attacker == creatures::TIMED_CONSUMABLE_SOURCE &&
                                   hit.kind == creatures::HIT_DAMAGE && hit.amount < 0;
            if (target) {
                if (source)
                    terrain_lights::appendConnectedHitLights(*source, hit, flashes);
                if (timedHeal) {
                    presentTimedHeal(hit);
                } else if (toLocal || fromLocal) {
                    const auto *remote = toLocal ? nullptr : remotes.find(hit.target);
                    if (toLocal || remote) {
                        const float height = toLocal ? player->body().size.z : remote->live.size.z;
                        if (auto text = ui::hitText(hit, toLocal, height))
                            interface.floating.push_back(*text);
                    }
                }
            }
            if (toLocal && hit.kind == 6)
                capturePet(hit, source);
            if (index >= produced || toLocal || fromLocal || timedHeal)
                mods.hitSeen(hit.attacker, hit.target, hit.kind, hit.amount);
        }
    };
    const auto applyReceivedRelationships = [&] {
        const auto received = client.takeRelationships();
        creatures::receiveRelationships(received, frameActor);
    };
    const auto applyReceivedZoneContents = [&] {
        if (!client.connected())
            return;
        // Blueprint zone delivery §4–5: apply these after the world pieces
        // and before rebuilding the live requests.
        for (const auto &[key, records] : client.takeZoneItems()) {
            if (!view.held().containsNetworkZone(key))
                continue;
            std::vector<WorldItem> list;
            list.reserve(records.size());
            for (const auto &r : records)
                list.push_back(WorldItem::fromRecord(r.data()));
            worldItems.replace(key.first, key.second, std::move(list));
            view.acknowledgeZoneContents(key);
        }
        std::vector<BlockEdit> edits;
        for (const auto &record : client.takeBlockEdits())
            edits.push_back(BlockEdit::parse(record.data()));
        if (!edits.empty())
            view.applyEdits(edits, TerrainEditSource::Queued);
        for (const auto &record : client.takeStaticStates())
            if (const auto key = statics.apply(record.data(), &view.held()))
                view.acknowledgeZoneContents(*key);
        for (auto &[key, records] : client.takeZonePlacements())
            view.replacePlacedObjects(key, std::move(records));
    };
    const auto refreshReceivedLinks = [&] {
        for (const auto &[key, remote] : otherFighters)
            creatures::refreshForwardLink(remote.fighter, frameActor);
    };
    // Online mode is an engine caller adapter, independent of the consumer's
    // supplied mode-zero rules. Reuse this world view across incoming frames.
    const creatures::CreditWorld creditWorld{frameActor,
        [&]() -> creatures::Combatant * { return player ? &player->combat() : nullptr; }, 1, {}};
    std::optional<Options> requestedLocalWorld;
    if (options.start && !glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
        glfwShowWindow(window);
        glfwFocusWindow(window);
    }
    while (!glfwWindowShouldClose(window) && !restart) {
        glfwPollEvents();
        if (glfwWindowShouldClose(window))
            break;
        if (player && interface.front == ui::Interface::Front::Creation &&
            cameraFront != ui::Interface::Front::Creation) {
            player->resetCreationPose();
            if (!playing()) {
                auto &actor = player->combat();
                actor.controller = actor.specialization = 0;
                actor.characterClass = 1;
                actor.level = 1;
                actor.experience = 0;
                actor.health = creatures::maxHealth(actor);
                own.setName("");
            }
        }
        const bool closeQuickWheel = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
                                     glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
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
        const int32_t frameElapsedMs = simulation::displayElapsed(previousFrameTick, uint32_t(uint64_t(now * 1000.0)));
        const float seconds = (float)(now - last);
        last = now;
        std::unique_lock<std::mutex> networkFrame;
        if (client.connected())
            networkFrame = client.lockFrame();
        auto worldChange = networkFrame.owns_lock() ? client.worldChange(activeWorld) : std::nullopt;
        if (!requestedLocalWorld && (worldChange || view.changingWorld()) && view.prepareWorldChange()) {
            if (remoteMap) {
                regionPass(player ? player->body().blocks() : view.worldPosition());
                remoteMap->saveDiscoveryCount(view.discoveryCount());
            }
            if (worldChange) {
                activeWorld = std::move(*worldChange);
                worldChange.reset();
            }
            const auto visit = onlineCharacter ? onlineCharacter->visitPosition(activeWorld.seed, activeWorld.name) : std::nullopt;
            const auto destination = visit.value_or(homeAnchor);
            const glm::dvec3 focus = glm::dvec3(destination[0], destination[1], destination[2]) / 65536.0;
            view.activateWorld(activeWorld.seed, focus);
            worldItems = {};
            statics.forgetWorld();
            blockEdits = {};
            missions.forgetWorld();
            missionRegionsRead.clear();
            zonesForMissions.clear();
            missionDay = -1;
            questRemembered.reset();
            mapRegions.clear();
            lastRegionPass = -1e9;
            openRemoteMap(activeWorld);
            if (remoteMap)
                view.restoreDiscoveryCount(remoteMap->discoveryCount());
            if (player)
                player->changeWorldPosition(destination);
            std::fprintf(stderr, "Activated world %s.\n", activeWorld.name.c_str());
        }
        const bool worldMatches = !worldChange && (!requestedLocalWorld ||
            (requestedLocalWorld->worldOptions.seed == activeWorld.seed && requestedLocalWorld->saveWorld == activeWorld.name));
        if (const auto clock = client.takeClock()) {
            scene.day = std::bit_cast<int32_t>(clock->first);
            scene.timeOfDayMs = clock->second;
        }
        receivedActors = client.connected() ? client.others() : std::map<uint64_t, net::CreatureRecord>{};
        remotes.retain(receivedActors);
        std::erase_if(otherFighters, [&](const auto &entry) { return !receivedActors.contains(entry.first); });
        // Publish snap poses in receipt order. An ID re-created between frames
        // starts with empty client-side state even when it is still listed.
        for (const auto &receipt : client.takeReceipts()) {
            if (!receivedActors.contains(receipt.id))
                continue;
            if (receipt.initialAim) {
                otherFighters.erase(receipt.id);
                otherPoses.erase(receipt.id);
                otherLooks.erase(receipt.id);
                weaponPoints.erase(receipt.id);
                shattered.erase(receipt.id);
            }
            remotes.receive(receipt);
        }
        for (const auto &[id, record] : receivedActors)
            remotes.updateLive(id, record, otherFighters[id]);
        mods.frame(seconds);
        auto held = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS ? 1.f : 0.f; };
        if (player) {
            const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;
            if (focused)
                pad.poll();
            else
                pad.clear();
            const auto syncControllerContext = [&] {
                const auto previousContext = pad.context();
                const auto currentContext = interface.controllerKeyboardOpen() ? Gamepad::Context::Keyboard :
                               interface.typing() ? Gamepad::Context::Text : interface.mapOpen ? Gamepad::Context::Map :
                               interface.wantsMouse() ? Gamepad::Context::Menu : Gamepad::Context::Gameplay;
                pad.setContext(currentContext);
                if (previousContext != currentContext) {
                    // A release belongs to the layer that received its press.
                    // A modal keyboard handoff also consumes a physical click.
                    if (previousContext == Gamepad::Context::Keyboard || currentContext == Gamepad::Context::Keyboard)
                        input.pointerButtons.clear();
                    else
                        input.pointerButtons.cancelController();
                    controllerMouse.fill(false);
                }
                syncCursor();
            };
            syncControllerContext();
            if (pad.connected) {
                auto key = [&](int k) { keyHandler(window, k, 0, GLFW_PRESS, 0); };
                if (pad.pressed(Gamepad::START)) {
                    if (!interface.controllerBack() && !interface.startScreen)
                        key(GLFW_KEY_ESCAPE);
                    pad.suppressHeld();
                    syncControllerContext();
                }
                if (pad.pressed(Gamepad::BACK) && (interface.controllerKeyboardOpen() || !interface.typing())) {
                    if (interface.controllerKeyboardOpen()) {
                        interface.finishControllerKeyboard(false);
                        pad.suppressHeld();
                    } else
                        key(GLFW_KEY_M);
                    syncControllerContext();
                }
                if (pad.pressed(Gamepad::B) && pad.context() != Gamepad::Context::Gameplay) {
                    if (interface.controllerBack())
                        pad.suppressHeld();
                    syncControllerContext();
                }
                if (pad.pressed(Gamepad::RIGHT_THUMB) && pad.context() != Gamepad::Context::Gameplay) {
                    if (interface.controllerKeyboardOpen())
                        interface.finishControllerKeyboard(true);
                    else
                        interface.openControllerKeyboard();
                    pad.suppressHeld();
                    syncControllerContext();
                }
                if (pad.pressed(Gamepad::A) && pad.context() == Gamepad::Context::Gameplay && player->combat().health <= 0) {
                    key(GLFW_KEY_R);
                    pad.suppressHeld();
                }
                if (interface.controllerKeyboardOpen()) {
                    const int x = int(pad.pressed(Gamepad::RIGHT)) - int(pad.pressed(Gamepad::LEFT));
                    const int y = int(pad.pressed(Gamepad::DOWN)) - int(pad.pressed(Gamepad::UP));
                    if (x || y) {
                        interface.controllerKeyboardMove(x, y);
                        const auto size = renderer.size();
                        const auto point = interface.controllerKeyboardFocus({float(size.width), float(size.height)});
                        int width, height;
                        glfwGetWindowSize(window, &width, &height);
                        if (size.width && size.height)
                            glfwSetCursorPos(window, double(point.x) * width / size.width, double(point.y) * height / size.height);
                    }
                } else {
                    if (pad.pressed(Gamepad::UP) && !interface.typing())
                        wheelHandler(window, 0, 1);
                    if (pad.pressed(Gamepad::DOWN) && !interface.typing())
                        wheelHandler(window, 0, -1);
                }
                const float dt = seconds;
                const auto moveCursor = [&](glm::vec2 stick) {
                    if (glm::dot(stick, stick) == 0)
                        return;
                    double x, y;
                    int width, height;
                    glfwGetCursorPos(window, &x, &y);
                    glfwGetWindowSize(window, &width, &height);
                    const double toY = std::clamp(y - stick.y * pad.cursorSpeed * dt, 0.0, double(std::max(0, height - 1)));
                    glfwSetCursorPos(window, std::clamp(x + stick.x * pad.cursorSpeed * dt, 0.0, double(std::max(0, width - 1))), toY);
                    // A moved cursor is a mouse move for the character screens' list drag (the
                    // platform may not report a programmatic move).
                    interface.frontMouseMove((float)toY, (uint32_t)(uint64_t)(glfwGetTime() * 1000.0));
                };
                if (pad.context() == Gamepad::Context::Map) {
                    const float psi = glm::radians(180.f - view.camera(renderer.size()).yawDegrees);
                    const glm::vec2 v = pad.move * (600.f * dt / interface.mapZoom); // blocks
                    interface.mapPan += glm::dvec3(v.x * std::cos(psi) - v.y * std::sin(psi),
                                                   v.x * std::sin(psi) + v.y * std::cos(psi), 0);
                    const float zoom = std::pow(0.9f, (pad.leftTrigger - pad.rightTrigger) * dt * 10.f);
                    interface.mapZoomTarget = std::clamp(interface.mapZoomTarget * zoom, 0.01f, 10.f);
                    moveCursor(pad.look);
                    const float turn = float(pad.held(Gamepad::RIGHT)) - float(pad.held(Gamepad::LEFT));
                    if (turn != 0)
                        player->look(turn * pad.lookSpeed * dt, 0);
                } else if (pad.context() == Gamepad::Context::Menu || pad.context() == Gamepad::Context::Keyboard) {
                    const glm::vec2 stick = glm::length(pad.move) > glm::length(pad.look) ? pad.move : pad.look;
                    moveCursor(stick);
                }
            }
            const Gamepad::Button uiButtons[] = {Gamepad::A, Gamepad::X, Gamepad::Y};
            const int mouseButtons[] = {GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE};
            for (size_t button = 0; button < controllerMouse.size(); button++) {
                // Focus/device loss cancels a drag without activating a release action.
                if (!pad.connected) {
                    controllerMouse[button] = false;
                    input.pointerButtons.cancelController();
                    continue;
                }
                const bool ui = pad.context() == Gamepad::Context::Menu || pad.context() == Gamepad::Context::Map ||
                                pad.context() == Gamepad::Context::Keyboard;
                const bool down = ui && pad.held(uiButtons[button]);
                if (down != controllerMouse[button]) {
                    controllerMouse[button] = down;
                    input.mouseDispatch(mouseButtons[button], down ? GLFW_PRESS : GLFW_RELEASE,
                                        Gamepad::PointerButtons::Source::Controller);
                    syncControllerContext();
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
                    // On any level change (not only a rise): the level-up notice, sound 0x1d at the
                    // player, then the grant of the new level only. (A character without a save
                    // starts at 0 here: that first grant stands in for creation's, without notice.)
                    if (c.level != grantedLevel) {
                        if (grantedLevel != 0) {
                            interface.floating.push_back(ui::levelUpText(player->body().blocks()));
                            playAt(0x1d, player->body().blocks(), 1, 1);
                        }
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
                        if (kind == 0)
                            return true; // §3.2: ids 0, 2 and unlisted ones need nothing
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
                    interface.stepCraft(seconds * 1000.f, !interface.controllerKeyboardOpen() && input.pointerButtons.held(0));
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
                    // Audio §4 "P": direct calls at the player's fixed position.
                    const int64_t *const playerFixed = player->body().position;
                    const sound::Fixed atPlayer{playerFixed[0], playerFixed[1], playerFixed[2]};
                    // Ruling audio-ui-sounds 3: buying (0x39, volume 1) and selling (0x3a, volume
                    // 0.75) sound at the player, as do disenchanting and adapting into the bag
                    // (0x2e, audio §4); the other interface sounds play at the listener.
                    for (const int32_t id : interface.inventorySounds)
                        if (id == 0x39 || id == 0x3a || id == 0x2e)
                            playFixed(id, atPlayer, id == 0x3a ? 0.75f : 1.f, 1);
                        else if (const char *key = creatures::soundKey(id))
                            audio.play(key);
                    interface.inventorySounds.clear();
                    // Talk §1.7, audio §4: the voice blips at the player's position (not the speaker's).
                    for (const float pitch : interface.voiceBlips)
                        playFixed(0x32, atPlayer, 1, pitch);
                    interface.voiceBlips.clear();
                    if (interface.equipmentChanged) {
                        wearInventory();
                        c.health = std::min(c.health, creatures::maxHealth(c));
                        interface.equipmentChanged = false;
                    }
                    if (interface.gliderEquipped) {
                        c.action = 0;
                        interface.gliderEquipped = false;
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
                if (client.connected())
                    playerActionFrame.buffs.insert(playerActionFrame.buffs.end(), effects.buffs.begin(), effects.buffs.end());
            };
            // Sample the combined interface state before early or late wheel closure.
            const bool holding = interface.quickWheel.open || interface.inventoryOpen || interface.craftingOpen ||
                                 interface.skillsOpen || interface.mapOpen || interface.optionsOpen;
            if (holding != windowHeld)
                playInterfaceSound(holding ? 0x53 : 0x54);
            windowHeld = holding;
            // Inventory mouse-down callbacks precede the world pieces.
            for (const auto &item : interface.itemsToUse)
                useConsumable(item);
            interface.itemsToUse.clear();
            // Select concrete bindings once before all world pieces. Later
            // equipment, action-clock and skill changes do not rebuild this list.
            player->refreshAbilityBindings();
            playerActionFrame.reset();
            frameBlockEdits.clear();
            creatures::NpcFrame::Cursor npcEvents;
            bool firstWorldCall = true;
            simulation::clientFrame(frameElapsedMs, worldMatches, playerActionFrame,
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
                    else
                        refreshAirships();
                    if (firstWorldCall) {
                        std::vector<WorldItems::Key> gone;
                        for (const auto &[key, items] : worldItems.all())
                            if (!view.held().zone(key.first, key.second))
                                gone.push_back(key);
                        for (const auto &[zx, zy] : gone) {
                            worldItems.forget(zx, zy);
                            statics.forget(zx, zy);
                            blockEdits.forget(zx, zy);
                            npcs.forgetZone(zx, zy);
                        }
                    }
                    creatures::SneakLight sneakLight;
                    const auto advancePlayer = [&](int32_t actorElapsed) {
                        // Physics §4.4: living creatures of kind 1 or 6 push the player (kind 0), the
                        // overlap on their box enlarged by (1, 1, 0) unless kind 6, the push scaled by
                        // their unenlarged x and by 0.1 while the player is stunned.
                        if (!client.connected()) {
                            creatures::Body &me = player->body();
                            const bool stunned = player->combat().hitStun > 0;
                            me.separate = glm::vec3(0);
                            for (const creatures::Npc &o : npcs.all())
                                if (o.fighter.health > 0 && (o.fighter.controller == 1 || o.fighter.controller == 6))
                                    me.separate += creatures::separationPush(me.size, me.position, o.body.size,
                                                                             o.fighter.controller != 6, o.body.position, stunned);
                        }
                        player->body().obstacles = &airships, player->body().key = player->combat().id;
                        player->body().targetInContact = npcs.hostileInContact(player->combat(), player->body());
                        const creatures::TimedConsumableEvents consumables{
                            player->body().position, aggregate.emittedHits, aggregate.sounds};
                        player->advance(actorElapsed, client.connected(), &consumables, sneakLight, liveActor);
                        publishPlayerEffects();
                        // Launches already have distinct resident and output copies.
                        auto shot = std::exchange(playerShot, {});
                        for (const creatures::SoundEvent &s : shot.sounds)
                            playAt(s.sound, s.position, s.volume, s.pitch);
                        bursts.insert(bursts.end(), shot.bursts.begin(), shot.bursts.end());

                    };
                    const auto advanceStaticState = [&](int32_t dt, std::span<creatures::PlayerView> players) {
                        std::map<StaticStates::Key, const worldgen::Zone *> held;
                        for (const auto &[key, zone] : view.held().zones())
                            held[key] = zone.get();
                        statics.step(dt,
                                     [&](uint64_t id, glm::dvec3 &at, glm::vec3 &intent, int32_t &action, int32_t &stun) {
                                         if (id == player->combat().id) {
                                             at = player->body().blocks();
                                             intent = player->body().intent;
                                             action = player->combat().action;
                                             stun = player->combat().hitStun;
                                             return true;
                                         }
                                         for (const creatures::Npc &n : npcs.npcs()) // seated residents
                                             if (n.fighter.id == id && n.fighter.health > 0) {
                                                 at = n.body.blocks();
                                                 intent = n.body.intent;
                                                 action = n.fighter.action;
                                                 stun = n.fighter.hitStun;
                                                 return true;
                                             }
                                         return false;
                                     },
                                     held, !client.connected(), // open points 1 §1.6: releases not remote
                                     // §6.4: the local actor follows its occupied static.
                                     [&](uint64_t id, glm::dvec3 top, float yaw, int32_t action) -> std::optional<glm::dvec3> {
                                         const auto seat = [&](creatures::Body &body, creatures::Combatant &who, float &facing) {
                                             body.place(top + glm::dvec3(0, 0, body.size.z / 2));
                                             body.velocity = glm::vec3(0);
                                             facing = yaw;
                                             who.action = action;
                                             return body.blocks();
                                         };
                                         if (id == player->combat().id)
                                             return seat(player->body(), player->combat(), player->facing());
                                         if (client.connected())
                                             return std::nullopt;
                                         for (creatures::Npc &n : npcs.npcs())
                                             if (n.fighter.id == id && n.fighter.health > 0)
                                                 return seat(n.body, n.fighter, n.yaw);
                                         return std::nullopt;
                                     });
                        // Traps run in the authoritative zone phase before the actor walk.
                        if (!client.connected()) {
                            std::vector<StaticStates::Creature> here{{player->combat().id, player->body().blocks(), 0}};
                            for (const creatures::Npc &n : npcs.all())
                                if (n.fighter.health > 0)
                                    here.push_back({n.fighter.id, n.body.blocks(), n.fighter.controller});
                            std::sort(here.begin(), here.end(), [](const auto &a, const auto &b) {
                                return creatures::ActorKeyLess{}(a.id, b.id);
                            });
                            const worldgen::World &world = view.terrainWorld();
                            // Attack-routine site 1: each trap hit runs the attack routine with no
                            // attacker on the target as it is reached (one discarded draw, the 0.5
                            // flash, the damage-taken buffs), then lands as before.
                            const auto trapStrike = [&](const StaticStates::Creature &c, float damage, glm::vec3 push,
                                                        bool resistance) {
                                creatures::Combatant *target = nullptr;
                                if (c.id == player->combat().id)
                                    target = &player->combat();
                                else
                                    for (creatures::Npc &n : npcs.npcs())
                                        if (n.fighter.id == c.id)
                                            target = &n.fighter;
                                if (!target || target->health <= 0)
                                    return;
                                const auto in = creatures::trapHitInput(c.at, damage, push, resistance);
                                glm::vec3 velocity(0);
                                const auto out = creatures::resolveHit(*target, velocity, nullptr, in,
                                                                       [&] { return worldRandom.draw(); });
                                for (const creatures::HitRecord &h : out.records) {
                                    if (h.target == player->combat().id) {
                                        player->takeHit(h);
                                        if (auto text = ui::hitText(h.amount, h.kind, false, false, true, player->body().blocks()))
                                            interface.floating.push_back(*text);
                                    } else {
                                        npcs.applyWorldHit(h, players, aggregate);
                                        aggregate.seen.push_back({h, c.at});
                                    }
                                }
                            };
                            statics.traps(dt, here, held,
                                          [&](glm::dvec3 at) { return world.levelAtFx((int64_t)(at.x * 65536.0), (int64_t)(at.y * 65536.0)); },
                                          trapStrike, aggregate.sounds);
                        }
                    };
                    if (client.connected()) {
                        worldItems.step(elapsed, false, worldBlock, worldDraw, aggregate.sounds);
                        advanceStaticState(elapsed, {});
                        const net::RemoteCreatures::Controlled controlled{player->combat(), player->body()};
                        const net::RemoteCreatures::StepEffects remoteEffects{aggregate, worldDraw};
                        sneakLight = remotes.makeSneakLight(view.held(), statics, worldItems,
                            std::bit_cast<int32_t>(uint32_t(scene.timeOfDayMs)), otherFighters, &controlled);
                        remotes.step(otherFighters, elapsed, worldBlock, npcs.objects, &controlled,
                            [&] {
                                for (const auto &target : client.pendingStaticRequests())
                                    useStaticRequest(target, false, aggregate);
                                if (player->combat().action == 106) {
                                    const auto *pet = frameActor(player->combat().linkedId);
                                    player->body().mountValid = pet && pet->health >= 0 && creatures::rideable(pet->type);
                                }
                                advancePlayer(elapsed);
                            }, &remoteEffects, sneakLight, npcs.sightRecords);
                        stepAirships(elapsed);
                        remotes.finishStep(otherFighters, &controlled);
                    }
                    const auto flushZoneState = [&] {
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
                    };
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
                            // §7: a region's saved cells, read the first time it is needed (the
                            // player's own region for discovery, the pass region below).
                            const auto readSavedCells = [&](int32_t rx, int32_t ry) {
                                if (!session || !world.region(rx, ry) || !missionRegionsRead.insert({rx, ry}).second)
                                    return;
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
                            };
                            readSavedCells(worldgen::floorDiv((int32_t)std::floor(me.x), worldgen::REGION),
                                           worldgen::floorDiv((int32_t)std::floor(me.y), worldgen::REGION));
                            // §2.1, Ruling 3: the region tested is the owner of the player's position.
                            const auto passRegion = missionPassRegion(world, me);
                            const int32_t rx = passRegion ? passRegion->first : -1, ry = passRegion ? passRegion->second : -1;
                            if (passRegion)
                                readSavedCells(rx, ry);
                            const auto zonePass = [&](int32_t zx, int32_t zy) {
                                if (const worldgen::Zone *zone = view.held().zone(zx, zy))
                                    npcs.setExtraSpawns(zx, zy, zone->spawns.size(),
                                                        missions.zonePass(world, *zone, players, worldRandom));
                            };
                            if (passRegion && world.region(rx, ry) && !missions.generated(rx, ry)) {
                                std::set<std::pair<int32_t, int32_t>> regionZones;
                                for (const auto &[key, zone] : view.held().zones())
                                    if (worldgen::floorDiv(key.first, 64) == rx && worldgen::floorDiv(key.second, 64) == ry)
                                        regionZones.insert(key);
                                npcs.removeTargets(regionZones);
                                missions.dailyPass(world, rx, ry, players, worldRandom);
                                // §2.6, §2.7 step 4: the driver ends with the zone pass of every loaded
                                // zone of its region, x outer, each zone's draws complete before the next.
                                for (const auto &[zx, zy] : regionZones) {
                                    zonesForMissions.erase({zx, zy});
                                    zonePass(zx, zy);
                                }
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
                            for (const auto &[zx, zy] : std::exchange(zonesForMissions, {}))
                                zonePass(zx, zy); // §2.6: newly generated zones
                            missions.discover(world, me);
                            npcs.killCompletedMissionTargets(missions);
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
                            {{c, player->body(), player->facing(), player->actionBefore(), false, false, true,
                              &player->appearance(), &playerCreationConfig}}, scene.timeOfDayMs, scene.day,
                            worldBlock, aggregate, {}, {}, advancePlayer,
                            [&](creatures::PlayerView &actor) { consumeOfflineItemRequests(actor, aggregate); },
                            [&](int32_t dt, std::span<creatures::PlayerView> actors) {
                                const glm::dvec3 at = player->body().blocks();
                                const int32_t zx = worldgen::floorDiv((int32_t)std::floor(at.x), worldgen::ZONE);
                                const int32_t zy = worldgen::floorDiv((int32_t)std::floor(at.y), worldgen::ZONE);
                                std::set<WorldItems::Key> active;
                                for (int32_t x = zx - 1; x <= zx + 1; ++x)
                                    for (int32_t y = zy - 1; y <= zy + 1; ++y)
                                        if (view.held().zone(x, y))
                                            active.insert({x, y});
                                worldItems.step(dt, true, worldBlock, worldDraw, aggregate.sounds, &active);
                                advanceStaticState(dt, actors);
                            },
                            [&](int32_t dt, std::span<creatures::PlayerView> actors) {
                                stepAirships(dt);
                                npcs.stepProjectiles(projectiles, dt, view.held(), worldBlock, actors,
                                                     aggregate, npcEvents.shots);
                            }, &projectiles);
                        const auto happened = aggregate.addedSince(npcEvents);
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
                            // Rulings Ruling 1: every player creature is rewarded, in or out of the
                            // victim's threat map; offline that is the local player.
                            if (!progress.rewardsApplied && progress.victimRole == 1)
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
                            // Materials §1.5: a critical hit flashes at the hit for 400 ms (radius
                            // 20), once per spirit-carrying item of the attacker's action weapon list.
                            if (hit.critical) {
                                const creatures::Combatant *by = hit.attacker == c.id ? &c : nullptr;
                                for (const creatures::Npc &n : npcs.all())
                                    if (n.fighter.id == hit.attacker)
                                        by = &n.fighter;
                                if (by)
                                    for (const auto &colour : terrain_lights::criticalFlashColours(*by))
                                        if (colour)
                                            flashes.append({hit.position, *colour, 20, 0, 400});
                            }
                            if (auto text = ui::hitText(hit.amount, hit.kind, hit.critical, hit.area, hit.target == c.id, at))
                                interface.floating.push_back(*text);
                            mods.hitSeen(hit.attacker, hit.target, hit.kind, hit.amount);
                        }
                        bursts.insert(bursts.end(), happened.bursts.begin(), happened.bursts.end());
                        for (const auto &t : happened.tamed) // mod SDK v10
                            mods.creatureTamed(t.creature, t.type, t.owner);
                        // Pets §12: the pet item follows the pet's level and experience.
                        for (const auto &p : happened.petItems)
                            if (p.owner == c.id && creatures::itemType(inventory.equipment[12]) == 19) {
                                const uint32_t experience = (uint32_t)p.experience;
                                std::memcpy(inventory.equipment[12].data() + 0x04, &experience, 4);
                                creatures::setItemLevel(inventory.equipment[12], (int16_t)p.level);
                                wearInventory();
                            }
                        for (const auto &credit : happened.appliedCredits)
                            if (credit.direct && credit.record.experience != 0) {
                                interface.floating.push_back(ui::experienceText(credit.record.experience, player->body().blocks()));
                                interface.chatAdd("You gain " + std::to_string(credit.record.experience) + " XP.\n", {150, 128, 255});
                            }
                        if (happened.experience > 0) { // separate retained non-hit corpse adapter
                            creatures::addExperience(c, happened.experience);
                            interface.floating.push_back(ui::experienceText(happened.experience, player->body().blocks()));
                            interface.chatAdd("You gain " + std::to_string(happened.experience) + " XP.\n", {150, 128, 255});
                        }
                        for (int32_t l = (int32_t)before + 1; l <= c.level; l++)
                            mods.levelUp(l); // (the notice and sound follow in the level-change step)
                    }
                    // Connected flight uses the received actors and receipt rules.
                    if (client.connected() && !projectiles.list.empty()) {
                        creatures::Combatant &c = player->combat();
                        projectileTargets.clear();
                        const auto addControlled = [&] { projectileTargets.push_back({&c, &player->body()}); };
                        projectileTargets.reserve(otherFighters.size() + 1);
                        creatures::visitReceivedActors(otherFighters, c.id, addControlled,
                            [&](uint64_t id, net::RemoteFighter &actor) {
                                if (const auto *body = remotes.find(id))
                                    projectileTargets.push_back({&actor.fighter, &body->live});
                            });
                        creatures::ShotEffects flight;
                        projectiles.step(elapsed, projectileTargets, frameActor, worldBlock, worldDraw, flight,
                            {true, &c, &npcs.sightRecords,
                             [&](const creatures::HitRecord &hit) { player->takeHit(hit); }, &view.held()});
                        aggregate.buffs.insert(aggregate.buffs.end(), flight.buffs.begin(), flight.buffs.end());
                        aggregate.sounds.insert(aggregate.sounds.end(), flight.sounds.begin(), flight.sounds.end());
                        for (const auto &hit : flight.hits)
                            aggregate.emittedHits.push_back(hit.record);
                        aggregate.bursts.insert(aggregate.bursts.end(), flight.bursts.begin(), flight.bursts.end());
                    }
                    blockEdits.appendWorldHistory(view.held(), frameBlockEdits);
                    flushZoneState();
                    firstWorldCall = false;
                },
                [&](creatures::NpcFrame &aggregate) {
                    if (client.connected()) {
                        const size_t produced = aggregate.emittedHits.size();
                        const worldgen::World &world = view.terrainWorld();
                        client.takeCombatReceipts().apply(player->combat().id, aggregate, projectiles, missions, world, frameActor,
                            [&](creatures::HitRecord &hit) { applyConnectedHit(aggregate, hit); });
                        missions.presentReceived(world, aggregate.receivedMissions, replayObjectiveInteraction,
                            [&](const MissionArea &mission, const worldgen::Location &location) {
                                interface.questComplete(ui::objectiveText(mission, location, dictionary));
                                playInterfaceSound(30);
                            });
                        presentConnectedHits(aggregate.emittedHits, produced);
                    } else {
                        for (const auto &hit : aggregate.emittedHits)
                            if (hit.kind == 6 && hit.target == player->combat().id)
                                capturePet(hit, frameActor(hit.attacker));
                    }
                    applyReceivedZoneContents();
                    const auto pendingGrants = client.connected() ? client.takeItemGrants() : std::vector<creatures::ItemGrant>{};
                    if (!aggregate.grants.empty() || !pendingGrants.empty())
                        creatures::applyItemGrants(aggregate.grants, pendingGrants, player->combat(), inventory, manaCubes,
                            frameActor, {[&] { interface.refreshInventoryRows(); }, [&] { interface.refreshCraftingRows(); },
                            [&](const creatures::ItemGrant &grant, const creatures::Combatant &recipient) {
                                const auto &item = grant.item;
                                const uint8_t type = creatures::itemType(item);
                                const bool local = &recipient == &player->combat();
                                if (local)
                                    mods.itemReceived(type, creatures::itemSubtype(item), creatures::itemLevel(item));
                                if (type == 25) {
                                    announceCube();
                                } else if (local) {
                                    announceItem(item, "You receive ");
                                } else {
                                    const auto &name = recipient.nameBytes;
                                    const std::vector<uint16_t> units(name.begin(), std::find(name.begin(), name.end(), 0));
                                    announceItem(item, net::fromUtf16(units) + " receives ");
                                }
                            }});
                    applyReceivedRelationships();
                    simulationInputFrames++;
                    auto &wheel = interface.quickWheel;
                    const auto choices = inventory.quickItems(player->combat().level);
                    wheel.earlyCount = choices.size();
                    wheel.hudSlot.reset();
                    if (choices.empty()) {
                        wheel.open = false;
                    } else if (!interface.wantsMouse() && !interface.typing()) {
                        const auto choice = choices[wheel.selected(choices.size())];
                        const auto [bag, slot] = choice;
                        wheel.hudSlot = choice;
                        wheel.hudCount = inventory.bags[bag][slot].count;
                        const bool qDown = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || pad.gameplay().quick;
                        if (qDown && !qWasDown)
                            useQuickItem(choice);
                        qWasDown = qDown;
                    }
                    wheel.selectorVisible = wheel.open;
                    const std::array<bool, 2> mouseButtons{input.pointerButtons.held(0), input.pointerButtons.held(1)};
                    // Controller aim §3.2: the pick uses the displayed +0x1350
                    // point. The attack origin still uses the live record position.
                    std::vector<Player::AimCandidate> aimCandidates;
                    if (!client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            aimCandidates.push_back({&n.fighter, {n.body.follow[0], n.body.follow[1], n.body.follow[2]}, n.body.size,
                                                     {n.body.position[0], n.body.position[1], n.body.position[2]}});
                    for (const auto &[id, record] : receivedActors) {
                        if (const auto *remote = remotes.find(id)) {
                            const auto &body = remote->live;
                            aimCandidates.push_back({&otherFighters.at(id).fighter, {body.follow[0], body.follow[1], body.follow[2]},
                                                     body.size, {body.position[0], body.position[1], body.position[2]}});
                        } else {
                            const camera_view::Fixed3 at{record.position(0), record.position(1), record.position(2)};
                            aimCandidates.push_back({&otherFighters.at(id).fighter, at, record.appearance().box, at});
                        }
                    }
                    player->timeOfDay = [&] { return std::bit_cast<int32_t>(uint32_t(scene.timeOfDayMs)); };
                    if (interface.front != ui::Interface::Front::Creation)
                        player->readInput(window, frameElapsedMs, !interface.wantsMouse() && !interface.typing() && player->combat().health > 0,
                                          interface.mapOpen ? nullptr : &pad, &mouseButtons, &npcs.sightRecords, aimCandidates,
                                          !wheel.open || closeQuickWheel);
                    // Skills §1.3: every input pass with a non-zero walking intent (object +0x40)
                    // hides the inventory, crafting and skill windows. (The other nodes it hides are
                    // not identified; movement keys stay gated while a window holds the mouse.)
                    if (const glm::vec3 intent = player->body().intent; glm::dot(intent, intent) > 0)
                        interface.inventoryOpen = interface.craftingOpen = interface.skillsOpen = false;
                    updatePlayerInterface();
                    if (!client.connected())
                        for (const auto &hit : aggregate.emittedHits)
                            if (hit.attacker == creatures::TIMED_CONSUMABLE_SOURCE) {
                                presentTimedHeal(hit);
                                mods.hitSeen(hit.attacker, hit.target, hit.kind, hit.amount);
                            }
                    for (const auto &sound : aggregate.sounds)
                        playAt(sound.sound, sound.position, sound.volume, sound.pitch);
                    if (client.connected())
                        bursts.insert(bursts.end(), aggregate.bursts.begin(), aggregate.bursts.end());
                    if (std::exchange(input.dropOne, false))
                        if (const auto item = inventory.dropOne())
                            queueItemRequest({ItemRequest::Drop, *item});
                    publishPlayerEffects();
                    if (client.connected())
                        client.queueCombatFrame(aggregate.emittedHits, aggregate.buffs, aggregate.shots);
                    using F = ui::Interface::Front;
                    const F front = interface.front;
                    const bool creationView = front == F::Creation || front == F::WorldCreation;
                    const bool menuView = interface.menuAnchorVisible();
                    creatures::FollowCamera &cam = player->followCamera();
                    if (menuView) {
                        cam.targetDistance = 10, cam.targetPitch = 100;
                        cam.targetYaw = float(frameElapsedMs) * std::bit_cast<float>(0x3ba3d70au) + cam.targetYaw;
                    } else if (creationView && front != cameraFront) {
                        cam.targetPitch = cam.pitch = 90;
                        cam.targetYaw = cam.yaw = front == F::Creation ? 0.f : 180.f;
                        cam.targetDistance = cam.distance = 5;
                    }
                    cameraFront = front;
                    const camera_follow::Alternative menuFocus{
                        {float(worldgen::HOME), float(worldgen::HOME), 0.f},
                        [&](int32_t x, int32_t y) -> std::optional<camera_follow::Column> {
                            if (uint32_t(x) >= 0x1000000u || uint32_t(y) >= 0x1000000u)
                                return std::nullopt;
                            const auto *area = view.held().zoneAt(x, y);
                            if (!area)
                                return std::nullopt;
                            const auto &column = area->column(x, y);
                            return camera_follow::Column{column.bottom, std::bit_cast<int32_t>(uint32_t(column.blocks.size()))};
                        }};
                    // The engine world clock stands in for the native world member.
                    player->finishFrame(frameElapsedMs, [&] { return std::bit_cast<int32_t>(uint32_t(scene.timeOfDayMs)); },
                                        npcs.sightRecords, {.alternative = menuView ? &menuFocus : nullptr,
                                                           .verticalOverride = creationView});
                });
        } else {
            // A missing controlled actor does not suppress the world call.
            // Preview bootstrap and native creature selection remain separate.
            playerActionFrame.reset();
            simulation::clientFrame(frameElapsedMs, worldMatches, playerActionFrame,
                [&](int32_t elapsed, creatures::NpcFrame &aggregate) {
                    simulationWorldCalls++;
                    simulationMaxPiece = std::max(simulationMaxPiece, elapsed);
                    bool accelerated = false;
                    for (const auto &[id, remote] : otherFighters)
                        accelerated |= acceleratesClock(remote.fighter);
                    advanceWorldClock(elapsed, accelerated);
                    if (client.connected())
                        refreshReceivedLinks();
                    else
                        refreshAirships();
                    worldItems.step(elapsed, !client.connected(), worldBlock, worldDraw, aggregate.sounds);
                    if (client.connected()) {
                        const net::RemoteCreatures::StepEffects remoteEffects{aggregate, worldDraw};
                        const auto sneakLight = remotes.makeSneakLight(view.held(), statics, worldItems,
                            std::bit_cast<int32_t>(uint32_t(scene.timeOfDayMs)), otherFighters);
                        remotes.step(otherFighters, elapsed, worldBlock, npcs.objects, nullptr, {}, &remoteEffects,
                                     sneakLight, npcs.sightRecords);
                    } else
                        npcs.stepInto((float)elapsed, view.held(), {}, scene.timeOfDayMs, scene.day, worldBlock, aggregate);
                    stepAirships(elapsed);
                    if (client.connected())
                        remotes.finishStep(otherFighters);
                },
                [&](creatures::NpcFrame &aggregate) {
                    if (client.connected())
                        applyReceivedBuffs();
                    applyReceivedZoneContents();
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
        const auto terrainEye = player && player->followCamera().placed() && options.travel == 0.f ?
            std::optional(player->followCamera().eyeFixed()) : std::nullopt;
        view.update(terrainEye);
        const bool settling = travelled < options.travel || (options.travel > 0 && view.busy());
        view.fillScene(scene);
        {
            const float target = view.fogTarget(terrainEye);
            fogDistance.update(target, frameElapsedMs, worldMatches);
            scene.fogDistance = fogDistance.value();
            scene.currentFogAngle = player ? std::optional(player->cameraPitch()) : std::nullopt;
            scene.mapFog = interface.mapOpen;
        }
        // An active group transferred by items() later in this frame first affects the next update.
        const bool terrainWait = player && options.connect.empty() &&
                                 interface.terrainWaitVisible(worldMatches, fogDistance.value());
        scene.seconds = now - start;
        const auto &motionSettings = options.worldOptions.motion;
        scene.leafSway = motionSettings.leaves
                             ? motion::leafOffset(scene.seconds * motionSettings.windSpeed, motionSettings.leafAmplitude)
                             : glm::vec2(0);
        scene.leafRange = motionSettings.leafRange;
        // Rendering §11.4: the clouds are gated at the local player's block column.
        view.playerColumn = player ? std::optional<glm::ivec2>(glm::ivec2(int32_t(player->body().position[0] / 65536),
                                                                          int32_t(player->body().position[1] / 65536)))
                                   : std::nullopt;
        items = view.items((now - start) * 1000, scene);
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
        // Models §7.3: a foot coming down, sounded at the creature's position (1/65536 blocks)
        // and softer on type-3 ground; only while the world has a local player.
        const auto footstep = [&](const int64_t position[3], float boxZ) {
            if (!player)
                return;
            const glm::dvec3 at = glm::dvec3(double(position[0]), double(position[1]), double(position[2])) / 65536.0;
            const glm::ivec3 below = creatures::footstepCell(position, boxZ);
            const auto u = [&] { return (float)worldRandom.draw() / 32767.f; };
            if (view.held().read(below.x, below.y, below.z).type() == 3)
                playAt(33 + worldRandom.draw() % 3, at, 1, 1);
            else
                playAt(32, at, 1, 1 + 0.2f * u());
        };
        const auto drawFlames = [&](const std::vector<creatures::FlameCube> &cubes, glm::dvec3 base) {
            for (const creatures::FlameCube &k : cubes) {
                glm::mat4 m = glm::translate(glm::mat4(1), view.toRender(base + glm::dvec3(k.position)));
                m = glm::rotate(m, glm::radians(k.turn.x), glm::vec3(1, 0, 0));
                m = glm::rotate(m, glm::radians(k.turn.y), glm::vec3(0, 1, 0));
                m = glm::rotate(m, glm::radians(k.turn.z), glm::vec3(0, 0, 1));
                m = glm::scale(m, glm::vec3(k.edge)) * glm::translate(glm::mat4(1), glm::vec3(0, 0, -0.5f));
                DrawItem flame{creatureView.cubeMesh(), m, false, Shading::World, glm::vec4(k.colour, 1)};
                flame.layer = DrawLayer::Effect; // rendering §1.3 step 15: particles, depth writes off
                items.push_back(flame);
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
        // Rendering §11.2: a creature's material from the light of the block 0.1 above its
        // position (0.1 block = 6553 fixed units, truncated), its tints, hit flash, motion trail
        // and translucency.
        const auto creatureLook = [&](const creatures::Combatant &c, const int64_t position[3], glm::vec3 velocity) {
            const int32_t x = worldgen::blockOfFixed(position[0]), y = worldgen::blockOfFixed(position[1]),
                          z = worldgen::blockOfFixed(position[2] + 6553);
            const int light = creature_shading::standingLight(view.held().read(x, y, z).type(), view.held().light(x, y, z));
            CreatureLook look;
            look.material = creature_shading::material(light, c, false);
            look.deferredMaterial = creature_shading::material(light, c, true);
            look.white = c.hitFlash;
            look.trail = creature_shading::hasMotionTrail(c);
            look.stealth = c.stealth;
            look.velocity = velocity;
            look.camera = view.toRender(view.worldPosition()); // trails §1.1 point-0 shift
            return look;
        };
        const bool drawActors = !interface.menuAnchorVisible();
        if (drawActors && player) {
            const CreatureLook look = creatureLook(player->combat(), player->body().position, player->body().velocity);
            player->draw(items, creatureView, seconds * 1000.f, &effects, &look);
            // (Less the landing bob, as for every other creature: open points 3 §3.3's base.)
            actionFlames(player->combat(), player->body().size.x,
                         player->body().blocks() - glm::dvec3(0, 0, player->body().bob));
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
            // Weapon sites read the points of the previous pose refresh. A creature
            // never refreshed keeps the main-position adapter (native storage there
            // is uninitialised).
            std::array<terrain_lights::FixedPosition, 2> hands{main, main};
            if (const auto points = weaponPoints.find(creature.id); points != weaponPoints.end())
                for (size_t hand = 0; hand < 2; ++hand)
                    if (points->second[hand])
                        hands[hand] = *points->second[hand];
            append(terrain_lights::equipmentSources(creature, main, hands));
        };
        const auto refreshWeaponPoints = [&](uint64_t id, const creatures::PoseState &pose, glm::dvec3 displayed) {
            weaponPoints[id] = terrain_lights::weaponPoints(pose, displayed);
        };
        if (drawActors && player) {
            const creatures::Body &body = player->body();
            const glm::dvec3 displayed = body.blocks() - glm::dvec3(0, 0, body.bob);
            creatureLights(player->combat(), {body.position[0], body.position[1], body.position[2]},
                           displayed, player->appearance().box.z, player->facing());
            refreshWeaponPoints(player->combat().id, player->poseState(), displayed);
        }
        for (creatures::Npc &npc : npcs.npcs()) {
            if (!drawActors)
                break;
            creatures::Motion motion;
            if (npc.body.ordinaryAttached)
                motion.walk = &npc.body.walk;
            motion.velocity = npc.body.velocity;
            motion.onGround = (npc.body.contact & creatures::ON_GROUND) != 0;
            motion.inWater = (npc.body.contact & creatures::IN_WATER) != 0;
            motion.yawDegrees = npc.yaw;
            motion.headPitch = npc.headPitch; // aim contract §3.7, drawn by head-pitch-and-pick §1
            motion.intent = npc.body.intent;
            motion.running = (npc.body.mode & 0x40) != 0;
            motion.lens = (npc.body.mode & creatures::LENS) != 0; // the 0x400 pose override
            motion.gliding = (npc.body.mode & creatures::GLIDER) != 0;
            motion.lantern = (npc.body.mode & creatures::LANTERN) != 0;
            motion.dodgeMs = (float)npc.body.dodgeMs;
            motion.fighter = &npc.fighter;
            glm::dvec3 at = npc.body.blocks();
            at.z -= npc.body.bob;
            // Particles §7.2: the dead are not drawn; they crumble once, a bomb with a flash. The
            // once-flag is spent even when the creature fails the draw-visibility test, which
            // then neither shatters it nor flashes.
            if (npc.fighter.health <= 0) {
                if (shattered.insert(npc.fighter.id).second) {
                    // alpha-terrain-light-source-lifecycle.md: the view test on the rendered
                    // position (engine +0x1180 lag is zero) with culling radius +0x88, tripled for
                    // action 106.
                    const float cull = npc.looks.box.z * (uint8_t(npc.fighter.action) == 106 ? 3.f : 1.f);
                    if (view.lightViewAccepts(at, cull, scene.fogDistance)) {
                        creatureView.shatter(npc.looks, npc.pose, motion, at, [&] { return worldRandom.draw(); },
                                             particles.list);
                        if (npc.fighter.type == 144)
                            flashes.append({{npc.body.position[0], npc.body.position[1], npc.body.position[2]},
                                            {1, 0.8f, 0.5f}, 16, 0, 800});
                    }
                }
                continue;
            }
            shattered.erase(npc.fighter.id);
            const CreatureLook look = creatureLook(npc.fighter, npc.body.position, npc.body.velocity);
            if (creatureView.draw(items, npc.looks, npc.pose, motion, view.toRender(at), seconds * 1000.f, &effects, &look))
                footstep(npc.body.position, npc.body.size.z);
            actionFlames(npc.fighter, npc.looks.box.x, at);
            creatureLights(npc.fighter, {npc.body.position[0], npc.body.position[1], npc.body.position[2]},
                           at, npc.looks.box.z, npc.yaw);
            refreshWeaponPoints(npc.fighter.id, npc.pose, at);
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
                    // §4.4: one uniform factor, scale x over the model's x size.
                    const glm::mat4 fit = glm::scale(glm::mat4(1), glm::vec3(size.x / voxels.x));
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
                            items.push_back({lid, place * hinge * glm::scale(glm::mat4(1), glm::vec3(size.x / lidVoxels.x)), true});
                        }
                }
            }
        }
        // World objects §7: each airship at its current pose, turned by +yaw, its grid centred
        // on the position (the mesh is centred on x and y with its base at z = 0).
        if (const MeshId ship = airships.ships().empty() ? 0 : view.airshipMesh()) {
            const float halfHeight = float(airships.modelSize().z) * 0.5f;
            for (const auto &[id, s] : airships.ships()) {
                const glm::dvec3 at = glm::dvec3(double(s.position[0]), double(s.position[1]), double(s.position[2])) / 65536.0;
                const glm::mat4 world = glm::translate(glm::mat4(1), view.toRender(at)) *
                                        glm::rotate(glm::mat4(1), glm::radians(s.yaw), glm::vec3(0, 0, 1)) *
                                        glm::translate(glm::mat4(1), glm::vec3(0, 0, -halfHeight));
                items.push_back({ship, world, true});
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
                    if (glm::dot(d, d) > 160.0 * 160.0 || !manaCubeFor(w.item, manaCubes))
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
                    // World items §2.5, materials §1.3, §1.4: the main model above draws without
                    // shimmer; a nonzero item type draws its attached cubes at their model cells,
                    // each in K(cube material, the item's lit colour, 0), shining with an iron,
                    // gold, silver or ice item.
                    if (type == 0)
                        continue;
                    const glm::vec3 size = creatureView.voxels(model);
                    const uint8_t itemMaterial = creatures::itemMaterial(w.item);
                    const bool shine = itemMaterial == 1 || itemMaterial == 11 || itemMaterial == 12 || itemMaterial == 22;
                    for (const creatures::Item::Cube &cube : creatures::Item::parse(w.item.data()).cubes)
                        items.push_back({creatureView.cubeMesh(),
                                         world * glm::translate(glm::mat4(1), glm::vec3(cube.x + 0.5f - size.x / 2,
                                                                                        cube.y + 0.5f - size.y / 2, cube.z)),
                                         true, Shading::World, creatures::materialColour(cube.material, material, 0.f),
                                         shine});
                }
        }
        if (drawActors && client.connected()) {
            const auto &otherRecords = receivedActors;
            for (const auto &[id, other] : otherRecords) {
                // Remote smoothing §2: drawn at the drawn pose (the record until the first update).
                const net::RemoteCreatures::Remote *remote = remotes.find(id);
                const int64_t recordPosition[] = {other.position(0), other.position(1), other.position(2)};
                const int64_t *drawn = remote ? remote->live.follow : recordPosition;
                const float drawnYaw = remote ? remote->drawnRotation.z : other.yaw();
                glm::dvec3 centre = glm::dvec3(double(drawn[0]), double(drawn[1]), double(drawn[2])) / 65536.0;
                if (remote)
                    centre.z -= remote->live.bob;
                const glm::vec3 p = view.toRender(centre);
                net::RemoteFighter &f = otherFighters[id];

                creatures::Motion motion;
                motion.velocity = remote ? remote->live.velocity : other.velocity();
                motion.intent = remote ? remote->live.intent : other.intent();
                motion.onGround = (f.fighter.contact & creatures::ON_GROUND) != 0;
                motion.inWater = (f.fighter.contact & creatures::IN_WATER) != 0;
                motion.climbing = (f.fighter.contact & creatures::ON_WALL) && (f.fighter.mode & creatures::CLIMB);
                motion.running = (f.fighter.mode & creatures::RUN) != 0;
                motion.dodgeMs = float(f.fighter.dodgeMs);
                motion.knockedDown = f.fighter.hitStun > 0;
                motion.yawDegrees = drawnYaw;
                motion.headPitch = remote ? remote->headPitch : other.headPitch(); // aim §3.7
                motion.fighter = &f.fighter;
                if (remote)
                    motion.walk = &remote->live.walk;
                motion.lens = (f.fighter.mode & creatures::LENS) != 0; // the 0x400 pose override
                motion.gliding = (f.fighter.mode & creatures::GLIDER) != 0;
                motion.lantern = (f.fighter.mode & creatures::LANTERN) != 0;
                if (f.fighter.health <= 0) { // particles §7.2: once, and only if it passes the view test
                    const float cull = other.appearance().box.z * (uint8_t(f.fighter.action) == 106 ? 3.f : 1.f);
                    if (shattered.insert(id).second && view.lightViewAccepts(centre, cull, scene.fogDistance))
                        creatureView.shatter(other.appearance(), otherPoses[id], motion, centre,
                                             [&] { return worldRandom.draw(); }, particles.list);
                    continue;
                }
                shattered.erase(id);
                const CreatureLook look = creatureLook(f.fighter, drawn, motion.velocity);
                creatureView.draw(items, other.appearance(), otherPoses[id], motion, p, seconds * 1000.f, &effects, &look);
                actionFlames(f.fighter, other.appearance().box.x, centre);
                // Core fields are current and resident buffs stay on the actor.
                creatureLights(f.fighter, {drawn[0], drawn[1], drawn[2]}, centre, other.appearance().box.z, drawnYaw);
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
                    mods.levelUp(int32_t(level)); // (the notice and sound follow in the level-change step)
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
        // /connect and /disconnect need a restart here; anything else is said. Packets §4.3:
        // these are recognised as commands (the first word exactly) and never sent.
        for (const std::string &line : interface.chatSubmitted) {
            const size_t space = line.find(' ');
            const std::string command = line.substr(0, space);
            const std::string argument = space == std::string::npos ? std::string() : line.substr(space + 1);
            if (command == "/name") {
                if (!argument.empty()) // screens §9 correction 113: an empty /name is ignored
                    own.setName(argument);
            } else if (command == "/namepet") {
                // Pets §15: the name goes into the pet item, one character (low byte) per entry;
                // the next pet pass rebuilds the pet with it.
                const std::vector<uint16_t> name = net::toUtf16(argument);
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
            } else if (command == "/connect" || command == "/disconnect") {
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
        std::optional<terrain_lights::FixedPosition> lightController;
        if (player) {
            const auto &position = player->body().position;
            lightController = terrain_lights::FixedPosition{position[0], position[1], position[2]};
        }
        view.finishLighting(scene, worldItems, std::bit_cast<int32_t>(lightClockMs), lightController);
        FrameConstants frame = makeFrame(view.camera(renderer.size()), scene, shadowSettings, moonSettings);
        // Screens §5.1: the corner map while playing, 3 x 3 zones round the player's at the
        // starting map zoom (s 0.8).
        if (player && interface.inGameVisible()) {
            const glm::dvec3 at = player->body().blocks();
            if (now - lastRegionPass > 1) {
                regionPass(at);
                lastRegionPass = now;
            }
            view.discover(at); // §5.1.6
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
            // §5.1.2 creature markers: players (controller 0), mission targets (controller 1 with
            // flag 0x2000) and, from s 1.6, the town NPCs of classes 0x80..0x87, within
            // max(r, 1)·256 blocks of the pivot in x and y. Re-certified correction 37: sorted
            // farthest first; both passes of one creature (black outline at 5/s without depth
            // writes, then coloured at 4/s) are drawn before the next.
            struct Marker {
                const creatures::Appearance *looks; // players: head then hair
                uint16_t model;                     // otherwise: the skull or the shop sign
                glm::dvec3 at;
                float yaw;
                float scale;
            };
            const int32_t radius = worldMap ? 16 : s == 0.8f ? 1 : 0;
            const double reach = 256.0 * std::max(radius, 1);
            std::vector<Marker> markers;
            const auto collect = [&](const creatures::Combatant &who, uint16_t flags, const creatures::Appearance *looks,
                                     glm::dvec3 p, float bodyYaw) {
                const bool townNpc = s >= 1.6f && who.characterClass >= 0x80 && who.characterClass <= 0x87;
                if (!(who.controller == 0 || (who.controller == 1 && (flags & 0x2000)) || townNpc))
                    return;
                if (std::abs(p.x - pivot.x) > reach || std::abs(p.y - pivot.y) > reach)
                    return;
                static const uint16_t SIGNS[8] = {2452, 2453, 2454, 2455, 2451, 2456, 2457, 2458};
                if (who.controller == 1)
                    markers.push_back({nullptr, 2562, p, bodyYaw, 1}); // `skull`
                else if (who.controller == 3 && townNpc)
                    markers.push_back({nullptr, SIGNS[who.characterClass - 0x80], p, 180.f - yaw, 0.5f}); // +ψ: faces the viewer
                else if (looks)
                    markers.push_back({looks, 0, p, bodyYaw, 1});
            };
            collect(player->combat(), 0, &player->appearance(), at, player->facing());
            if (!client.connected())
                for (const creatures::Npc &n : npcs.all())
                    collect(n.fighter, n.looks.flags, &n.looks, n.body.blocks(), n.yaw);
            for (const auto &[id, other] : receivedActors)
                otherLooks[id] = other.appearance();
            for (const auto &[id, other] : receivedActors)
                collect(other.fighter(), otherLooks[id].flags, &otherLooks[id],
                        glm::dvec3(other.position(0), other.position(1), other.position(2)) / 65536.0, other.yaw());
            auto depth = [&](const Marker &m) {
                const glm::vec4 c = frame.minimap * glm::vec4(view.toRender(m.at), 1);
                return c.z / c.w;
            };
            std::sort(markers.begin(), markers.end(), [&](const Marker &a, const Marker &b) { return depth(a) > depth(b); });
            const auto drawMarker = [&](const Marker &mk, uint16_t slot, int pass, glm::vec3 colour) {
                const MeshId mesh = slot == creatures::NO_MODEL ? 0 : creatureView.plainMesh(slot);
                if (!mesh)
                    return;
                const glm::vec3 size = creatureView.voxels(slot);
                glm::mat4 m = glm::translate(glm::mat4(1), view.toRender(mk.at));
                m = glm::rotate(m, glm::radians(mk.yaw), glm::vec3(0, 0, 1));
                m = glm::scale(m, glm::vec3((pass == 0 ? 5.f : 4.f) * mk.scale / s));
                m = glm::translate(m, glm::vec3(0, 0, -size.z * 0.5f));
                map.push_back({mesh, m, false, pass == 0 ? Shading::MapOutline : Shading::Map,
                               glm::vec4(pass == 0 ? glm::vec3(0) : colour, 2)});
            };
            for (const Marker &mk : markers)
                for (int pass = 0; pass < 2; pass++) {
                    if (!mk.looks) {
                        drawMarker(mk, mk.model, pass, glm::vec3(1));
                        continue;
                    }
                    drawMarker(mk, mk.looks->model(creatures::Part::Head), pass, glm::vec3(1));
                    drawMarker(mk, mk.looks->model(creatures::Part::Hair), pass,
                               glm::vec3(mk.looks->hairColour[0], mk.looks->hairColour[1], mk.looks->hairColour[2]) / 255.f);
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
            // §5.2.6: show "Teleport" at the cursor over the selected portal.
            interface.cursorLabel = interface.mapOpen && interface.teleportMode &&
                                            interface.selectedPortal != glm::ivec2(-1) &&
                                            interface.selectedPortal == interface.hoveredPortal
                                        ? u"Teleport"
                                        : u"";
            renderer.setMinimap(std::move(map));
        } else {
            renderer.setMinimap({});
        }
        {
            // Talk §1.5 steps 2 and 4: the bubbles' time and auto-advance precede this frame's
            // partner check and talk press; their glide follows below.
            interface.advanceBubbles(seconds * 1000.f);
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
                hud.lens = (c.mode & creatures::LENS) != 0; // rulings camera-lens-pan §2.1
                // Screens §5.3: the pet's bars (its name from the pet item, else its species).
                const creatures::Combatant *pet = nullptr;
                if (client.connected()) {
                    if (const auto linked = otherFighters.find(c.linkedId); linked != otherFighters.end() &&
                        linked->second.fighter.controller == 5 && linked->second.fighter.ownerId == c.id)
                        pet = &linked->second.fighter;
                } else if (const auto *residentPet = npcs.petOf(c)) {
                    pet = &residentPet->fighter;
                }
                if (pet) {
                    hud.hasPet = true;
                    const char *speciesKey = creatures::speciesKey(pet->type);
                    const DictionaryEntry *species = speciesKey ? dictionary.find("name", speciesKey) : nullptr;
                    hud.petSpecies = species && species->field("singular") ? *species->field("singular") : std::string();
                    hud.petName.clear();
                    for (const creatures::Item::Cube &ch : c.equipment[12].cubes)
                        hud.petName += (char)ch.material;
                    if (hud.petName.empty())
                        hud.petName = hud.petSpecies;
                    hud.petHealth = pet->health;
                    hud.petMaxHealth = creatures::maxHealth(*pet);
                    hud.petLevel = pet->level;
                    hud.petExperience = pet->experience;
                    hud.petExperienceNeeded = creatures::experienceForLevel(pet->level);
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
                const auto gamepadInput = pad.gameplay();
                hud.abilities[0].held = free && (input.pointerButtons.held(0) || gamepadInput.abilities[0]);
                hud.abilities[1].held = free && (input.pointerButtons.held(1) || gamepadInput.abilities[1]);
                for (int k = 0; k < 4; k++)
                    hud.abilities[2 + k].held = free && (glfwGetKey(window, GLFW_KEY_1 + k) == GLFW_PRESS ||
                                                         gamepadInput.abilities[k + 2]);
                hud.quickHeld = free && (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || gamepadInput.quick);
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
                // Re-certified §5.4 (correction 51): no bar for a creature whose link (+0x198, the
                // owner identifier) names a creature in state 0x6a, nor, while the local player is
                // in 0x6a, for the creature linked to the local player.
                std::vector<uint64_t> riding;
                const auto noteRiding = [&](uint64_t id, const creatures::Combatant &who) {
                    if (uint8_t(who.action) == 0x6a)
                        riding.push_back(id);
                };
                noteRiding(c.id, c);
                if (!client.connected())
                    for (const creatures::Npc &n : npcs.all())
                        noteRiding(n.fighter.id, n.fighter);
                for (const auto &[id, other] : otherFighters)
                    noteRiding(id, other.fighter);
                const glm::dvec3 viewForward = glm::dvec3(view.camera(renderer.size()).forward());
                auto addBar = [&](const creatures::Combatant &who, uint64_t id, glm::dvec3 centre, glm::vec3 box, float bob,
                                  uint16_t appearanceFlags, const std::string &ownName) {
                    if (who.ownerId != 0 && std::find(riding.begin(), riding.end(), who.ownerId) != riding.end())
                        return;
                    const double distance = glm::distance(centre, me);
                    if (distance > 200)
                        return;
                    const bool isPlayer = who.controller == 0;
                    const bool focused = distance * distance <= 3600 && clearSight(centre, 256) && underCrosshair(centre, box);
                    if (!focused && !interface.allLifeBars && !isPlayer)
                        return;
                    // §5.4: raised by half the height less the drawn-height offset (+0x1180); for
                    // states 0x6a/0x6b 2 is added to the camera-space depth (farther, not higher).
                    glm::dvec3 head = centre + glm::dvec3(0, 0, box.z * 0.5 - bob);
                    const uint8_t state = uint8_t(who.action);
                    if (state == 0x6a || state == 0x6b)
                        head += viewForward * 2.0;
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
                    hud.creatureBars.push_back(bar);
                };
                if (!client.connected())
                    for (const creatures::Npc &n : npcs.all())
                        if (n.fighter.health > 0)
                            addBar(n.fighter, n.fighter.id, n.body.blocks(), n.body.size, n.body.bob, n.looks.flags, "");
                for (const auto &[id, other] : receivedActors) {
                    const creatures::Appearance looks = other.appearance();
                    const auto *remote = remotes.find(id);
                    const glm::dvec3 position = remote ? glm::dvec3(remote->live.follow[0], remote->live.follow[1], remote->live.follow[2])
                                                       : glm::dvec3(other.position(0), other.position(1), other.position(2));
                    addBar(otherFighters.at(id).fighter, id, position / 65536.0, looks.box, remote ? remote->live.bob : 0.f,
                           looks.flags, other.name());
                }
                // Interaction §2: the creature, static and item passes compete on the squared
                // camera distance B of the boxes under the crosshair; within 4 blocks (squared
                // 16) the nearest to the player is the fall-back when nothing is aimed at.
                ui::TargetPasses passes;
                {
                    // §2.2: the living creatures within squared distance 3600, sorted on that
                    // distance (non-stable, U-11), each past the skip rule and its line of sight
                    // (whose reach falls with the light at the creature and the daylight).
                    struct Listed {
                        uint64_t id;
                        uint8_t controller;
                        glm::dvec3 centre;
                        glm::vec3 box;
                        double dp;
                        float bob;
                    };
                    std::vector<Listed> listed;
                    const auto list = [&](uint64_t id, const creatures::Combatant &who, glm::dvec3 centre, glm::vec3 box, float bob) {
                        const double dp = glm::dot(centre - me, centre - me);
                        if (who.health > 0 && dp <= 3600 && id != c.id)
                            listed.push_back({id, who.controller, centre, box, dp, bob});
                    };
                    if (!client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            list(n.fighter.id, n.fighter, n.body.blocks(), n.body.size, n.body.bob);
                    for (const auto &[id, other] : receivedActors) {
                        const auto *remote = remotes.find(id);
                        const glm::dvec3 position = remote ? glm::dvec3(remote->live.follow[0], remote->live.follow[1], remote->live.follow[2])
                                                           : glm::dvec3(other.position(0), other.position(1), other.position(2));
                        list(id, otherFighters.at(id).fighter, position / 65536.0, other.appearance().box,
                             remote ? remote->live.bob : 0.f);
                    }
                    std::sort(listed.begin(), listed.end(), [](const Listed &a, const Listed &b) { return a.dp < b.dp; });
                    const bool healing = c.action == 34 && c.actionMs < (float)creatures::duration(c, c.action);
                    const float daylight = creatures::controller_aim::daylight(std::bit_cast<int32_t>(uint32_t(scene.timeOfDayMs)));
                    for (const Listed &l : listed) {
                        if (healing && (l.controller == 1 || l.controller == 6))
                            continue;
                        const uint8_t light = player->lightAt((int32_t)std::floor(l.centre.x), (int32_t)std::floor(l.centre.y),
                                                              (int32_t)std::floor(l.centre.z));
                        const glm::dvec3 lowered = l.centre - glm::dvec3(0, 0, l.bob);
                        if (!clearSight(lowered, creatures::controller_aim::pickRange(light, daylight)))
                            continue;
                        passes.creature(l.id, l.dp, glm::dot(l.centre - eye, l.centre - eye), underCrosshair(lowered, l.box));
                    }
                }
                const int32_t pzx = worldgen::floorDiv((int32_t)std::floor(me.x), worldgen::ZONE);
                const int32_t pzy = worldgen::floorDiv((int32_t)std::floor(me.y), worldgen::ZONE);
                for (int32_t zx = pzx - 1; zx <= pzx + 1; zx++)
                    for (int32_t zy = pzy - 1; zy <= pzy + 1; zy++) {
                        const worldgen::Zone *zone = view.held().zone(zx, zy);
                        for (size_t k = 0; zone && k < zone->statics.size(); k++) {
                            const worldgen::StaticObject &o = zone->statics[k];
                            const glm::dvec3 at = glm::dvec3((double)o.x, (double)o.y, (double)o.z) / 65536.0;
                            const glm::dvec3 centre = at + glm::dvec3(0, 0, o.scale[2] * 0.5);
                            // §2.3: no kind filter; reach and fall-back on the squared distance
                            // to the base, B on the squared distance to the centre.
                            passes.staticObject({zx, zy, (int32_t)k}, glm::dot(at - me, at - me),
                                                glm::dot(centre - eye, centre - eye), [&] {
                                if (!clearSight(centre, 200))
                                    return false;
                                // The box: the scale, x and y swapped for odd turns, centred in x
                                // and y, from the base up.
                                const bool odd = (o.rotation & 1) != 0;
                                const glm::dvec3 size(odd ? o.scale[1] : o.scale[0], odd ? o.scale[0] : o.scale[1], o.scale[2]);
                                bool left = false, right = false, up = false, down = false;
                                for (int corner = 0; corner < 8; corner++) {
                                    const glm::dvec3 p = at + glm::dvec3(((corner & 1) - 0.5) * size.x,
                                                                         (((corner >> 1) & 1) - 0.5) * size.y,
                                                                         ((corner >> 2) & 1) * size.z);
                                    const glm::vec4 clip = frame.viewProjection * glm::vec4(view.toRender(p), 1);
                                    if (clip.w <= 0)
                                        continue;
                                    const glm::vec2 ndc = glm::vec2(clip) / clip.w;
                                    left = left || ndc.x < 0, right = right || ndc.x > 0;
                                    up = up || ndc.y < 0, down = down || ndc.y > 0;
                                }
                                return left && right && up && down;
                            });
                        }
                    }
                // §2.1 step 6: dead, the creature and object targets are emptied; the item pass
                // and the fall-backs below can still set them again.
                if (c.health <= 0)
                    passes.dead();
                // §2.4: the item pass over the items the player may pick up (0x3E550), on squared
                // distances within 4 blocks of the player; one it cannot carry counts 16 further.
                for (int32_t zx = pzx - 1; zx <= pzx + 1; zx++)
                    for (int32_t zy = pzy - 1; zy <= pzy + 1; zy++) {
                        const std::vector<WorldItem> *list = worldItems.zone(zx, zy);
                        for (size_t k = 0; list && k < list->size(); k++) {
                            const WorldItem &w = (*list)[k];
                            if (!manaCubeFor(w.item, manaCubes))
                                continue;
                            const glm::dvec3 at = w.blocks();
                            passes.item({zx, zy, (int32_t)k}, glm::dot(at - me, at - me), glm::dot(at - eye, at - eye),
                                        inventory.canCarry(w.item), [&] {
                                if (!clearSight(at + glm::dvec3(0, 0, 0.25), 200))
                                    return false;
                                const uint16_t model = creatures::itemModel(creatures::Item::parse(w.item.data()));
                                if (model == creatures::NO_MODEL)
                                    return false;
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
                                return left && right && up && down;
                            });
                        }
                    }
                // §2.5: the fall-backs, whatever the player's health.
                {
                    const ui::Targets chosen = passes.finish();
                    creatureTarget = chosen.creature;
                    itemTarget = chosen.item;
                    objectTarget.reset();
                    if (chosen.object)
                        objectTarget = std::tuple<int32_t, int32_t, size_t>{(*chosen.object)[0], (*chosen.object)[1],
                                                                           (size_t)(*chosen.object)[2]};
                }
                // Screens §8.1.9: E picks the item target up, if it can be carried.
                const WorldItem *targetedItem = nullptr;
                if (itemTarget)
                    if (const auto *list = worldItems.zone((*itemTarget)[0], (*itemTarget)[1]))
                        if ((*itemTarget)[2] >= 0 && (*itemTarget)[2] < (int32_t)list->size())
                            targetedItem = &(*list)[(*itemTarget)[2]];
                interface.targetItem = targetedItem ? std::optional<creatures::ItemRecord>(targetedItem->item) : std::nullopt;
                const bool padInteract = !interface.wantsMouse() && pad.gameplay().interact;
                const bool padPickup = padInteract && targetedItem;
                // §3: the prompt. Revive while dead; a later assignment replaces an earlier one:
                // the creature target's Talk or Ride, else the object target's prompt.
                const worldgen::StaticObject *targeted = nullptr;
                if (objectTarget)
                    if (const worldgen::Zone *zone = view.held().zone(std::get<0>(*objectTarget), std::get<1>(*objectTarget));
                        zone && std::get<2>(*objectTarget) < zone->statics.size())
                        targeted = &zone->statics[std::get<2>(*objectTarget)];
                hud.prompt = c.health <= 0 ? u"[R] Revive" : u"";
                bool talkShown = false, creaturePromptShown = false;
                if (creatureTarget) {
                    bool found = false, prisoner = false;
                    uint8_t controller = 0;
                    int32_t type = 0;
                    uint64_t owner = 0;
                    if (!client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            if (n.fighter.id == creatureTarget) {
                                found = true, controller = n.fighter.controller, type = n.fighter.type;
                                owner = n.fighter.ownerId, prisoner = n.questRole == quests::PRISONER;
                            }
                    if (const auto other = receivedActors.find(creatureTarget); other != receivedActors.end()) {
                        const creatures::Combatant fighter = other->second.fighter();
                        found = true, controller = other->second.controller(), type = fighter.type, owner = fighter.ownerId;
                        prisoner = client.connected() && (creatureTarget >> 56) == 0x51; // (online: by its id)
                    }
                    if (found) {
                        // (Look mode: no window, map or menu holds the cursor.)
                        std::u16string shown = ui::creaturePrompt(controller, !interface.wantsMouse(), type, owner == c.id,
                                                                  c.skills[1] != 0, c.action);
                        talkShown = shown == u"[R] Talk";
                        // Engine quests (required extension): a tied-up prisoner offers to be freed.
                        if (controller == 6 && prisoner)
                            shown = u"[R] Free";
                        if (!shown.empty())
                            hud.prompt = shown, creaturePromptShown = true;
                    }
                }
                // When Talk or Ride (or the quests' Free) is shown, the object branch is skipped.
                if (!creaturePromptShown && targeted) {
                    const auto *states = statics.zone(std::get<0>(*objectTarget), std::get<1>(*objectTarget));
                    const StaticState *live = states && std::get<2>(*objectTarget) < states->size()
                                                  ? &(*states)[std::get<2>(*objectTarget)]
                                                  : nullptr;
                    const std::u16string shown = ui::staticPrompt(targeted->kind, live ? live->state : targeted->state,
                                                                  live && live->occupant == c.id);
                    if (!shown.empty())
                        hud.prompt = shown;
                }
                const bool rDown = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS || (padInteract && !padPickup)) && !interface.typing();
                const bool rPressed = rDown && !rWasDown;
                // §5.2 dead: the key handler ran the revive on this press; the rest of the R block
                // is skipped.
                const bool revivedNow = std::exchange(input.revived, false);
                // §5.1: Ride responds inside the prompt code (no quick-item wheel test): mounted.
                if (rPressed && hud.prompt == u"[R] Ride") {
                    player->combat().action = 106;
                    player->combat().actionMs = 0;
                }
                // §5.2 b): the object effects of an R press, run below after the talk record and
                // the creature effects of the same press, whenever the object target resolves.
                const auto objectEffects = [&] {
                    const int32_t kind = targeted->kind;
                    const auto [zx, zy, k] = *objectTarget;
                    if (kind == ui::RUNESTONE) { // the map in teleport mode, no packet
                        if (zx >= 0) {
                            interface.mapOpen = true;
                            interface.teleportMode = true;
                        }
                        return;
                    }
                    // Crafting stations toggle their windows. Generic statics
                    // append kind 3 for the next actor turn.
                    const bool station = kind == 65 || (kind >= 71 && kind <= 77);
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
                    if (!station)
                        queueItemRequest({ItemRequest::UseStatic, {}, {zx, zy, (int32_t)k}});
                    // §5.2 steps 3-4, after the kind-3 record: a bubble already on the player
                    // advances or completes whatever the kind; otherwise most statics make the
                    // player say "There is nothing special."
                    static const std::set<int32_t> QUIET = {1, 2, 3, 10, 16, 18, 19, 69};
                    if (!station)
                        interface.talk(c.id, [&] { return QUIET.count(kind) ? std::vector<SpeechPage>{} : nothingSpecial(); });
                };
                // §5.1 Talk, on a frame showing "[R] Talk" (no quick-item wheel test). Talk §1.3,
                // §2.4: the bubble opens or advances; the talk request (packet 6 kind 2) rests
                // the player at an inn at night (§2.7).
                // Talk §1.5 step 3, §3.6: before the talk press, a partner at a squared distance
                // of 25 or more (5 blocks) is cleared; the windows stay open.
                if (talkPartner != 0) {
                    std::optional<std::array<int64_t, 3>> at;
                    if (!client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            if (n.fighter.id == talkPartner)
                                at = std::array<int64_t, 3>{n.body.position[0], n.body.position[1], n.body.position[2]};
                    if (const auto other = receivedActors.find(talkPartner); other != receivedActors.end())
                        at = std::array<int64_t, 3>{other->second.position(0), other->second.position(1),
                                                    other->second.position(2)};
                    // Engine lifetime policy: a partner that no longer exists is cleared too.
                    if (!at || ui::vendor::partnerOutOfReach(*at, player->body().position))
                        talkPartner = 0;
                }
                if (rPressed && talkShown) {
                    uint8_t characterClass = 0;
                    int32_t talkKind = 0;
                    glm::ivec2 target{-1, -1};
                    std::array<int32_t, 3> home{-1, -1, -1};
                    if (!client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            if (n.fighter.id == creatureTarget) {
                                characterClass = n.fighter.characterClass;
                                talkKind = n.talkKind;
                                target = n.talkTarget;
                                home = {n.zone.first, n.zone.second, (int32_t)n.spawn};
                            }
                    if (const auto other = receivedActors.find(creatureTarget); other != receivedActors.end()) {
                        characterClass = other->second.fighter().characterClass;
                        talkKind = other->second.talkKind();
                        target = other->second.talkTarget();
                        home = other->second.homeSpawn();
                    }
                    // §5.1 step 2: the talk record, queued on every Talk press before the bubble.
                    queueItemRequest({ItemRequest::Talk, {}, home});
                    {
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
                        if (!questTalk)
                            if (const auto *actor = frameActor(who))
                                talkTo(*actor, talkKind, target, false);
                    }
                }
                // §5.2: the R block waits for the quick-item wheel to close, and is skipped
                // on the press that revived. a) The creature effects.
                const bool rBlock = rPressed && !interface.quickWheel.open && !revivedNow && c.health > 0;
                if (rBlock)
                    talkPartner = 0; // talk §3.3: replaced below when the target is a creature
                if (rBlock && !creatureTarget && !client.connected()) {
                    // With no creature target, the nearest free pet is claimed (no packet).
                    std::vector<ui::PetCandidate> pets;
                    for (const creatures::Npc &n : npcs.all()) {
                        const glm::dvec3 d = n.body.blocks() - me;
                        pets.push_back({n.fighter.id, n.fighter.controller, glm::dot(d, d), n.fighter.ownerId});
                    }
                    if (const uint64_t claimed = ui::freePetClaim(pets))
                        player->combat().linkedId = claimed;
                }
                if (rBlock && creatureTarget) {
                    const uint64_t who = creatureTarget;
                    uint8_t controller = 0, characterClass = 0;
                    if (!client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            if (n.fighter.id == who) {
                                controller = n.fighter.controller, characterClass = n.fighter.characterClass;
                                talkPartner = who;
                            }
                    if (const auto other = receivedActors.find(who); other != receivedActors.end()) {
                        controller = other->second.controller(), characterClass = other->second.fighter().characterClass;
                        talkPartner = who;
                    }
                    // Engine quests (required extension): R on a tied-up prisoner frees it once no
                    // guard is near (online the server decides: a talk request naming the quest
                    // and the creature's index).
                    if (controller == 6 && client.connected() && (who >> 56) == 0x51)
                        queueItemRequest({ItemRequest::Talk, {}, {-2, (int32_t)(uint32_t)(who >> 12), (int32_t)(who & 0xfff)}});
                    if (controller == 6 && options.extendedQuests && !client.connected())
                        for (const creatures::Npc &n : npcs.all())
                            if (n.fighter.id == who && n.questRole == quests::PRISONER) {
                                const uint32_t questId = n.quest;
                                const quests::Quest *q = questBook.find(questId);
                                if (!q) {
                                    interface.chatAdd("This prisoner waits for someone else.\n", {1, 1, 1});
                                } else if (npcs.guarded(questId, n.body.blocks(), quests::FREE_CLEAR)) {
                                    interface.chatAdd("Defeat the guards first!\n", {1, 0.4f, 0.4f});
                                } else {
                                    const glm::dvec3 at = n.body.blocks();
                                    npcs.free(who, q->giverAt);
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
                    {
                        // By class, whatever the controller. Talk §3.3: shops open the vendor
                        // window with the inventory beside it (online stock comes from this
                        // client's descriptor copy). Item services: class 131 identifies, class
                        // 137 adapts.
                        if (characterClass == 131)
                            interface.identifyOpen = interface.inventoryOpen = true, interface.identifyBag = -1;
                        if (characterClass == 137)
                            interface.adaptOpen = interface.inventoryOpen = true;
                        if (characterClass >= 128 && characterClass <= 130) {
                            interface.vendorOpen = interface.inventoryOpen = true;
                            interface.vendorGrid.currentGroup = 0;
                            // Talk §3.3 tab 0: the partner's bags are copied again only while a
                            // partner is set; once cleared the shadow keeps its last copy.
                            interface.vendorExport = [&](creatures::InventoryValue &buffer) {
                                const uint64_t who = talkPartner;
                                if (who == 0)
                                    return;
                                for (const creatures::Npc &n : npcs.all())
                                    if (n.fighter.id == who) {
                                        buffer.copyFrom(n.inventory);
                                        break;
                                    }
                                // This ID lookup is an engine lifetime adapter;
                                // native cached-pointer equivalence stays open.
                                if (client.connected()) {
                                    const int32_t zx = (int32_t)((who >> 8) & 0xffff), zy = (int32_t)((who >> 24) & 0xffff);
                                    const size_t index = (size_t)(who & 0xff);
                                    if (const worldgen::Zone *zone = view.held().zone(zx, zy); zone && index < zone->spawns.size()) {
                                        const auto &source = zone->spawns[index];
                                        if (source.inventoryValue) {
                                            buffer.copyFrom(*source.inventoryValue);
                                        } else {
                                            creatures::InventoryValue projected;
                                            for (const auto &item : source.inventory)
                                                projected.appendUnit(creatures::toRecord(item));
                                            buffer.copyFrom(projected);
                                        }
                                    }
                                }
                            };
                            interface.refreshVendor();
                        }
                    }
                }
                // b) The object effects of the same press.
                if (rBlock && targeted)
                    objectEffects();
                const bool eDown = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS || padPickup) && !interface.typing();
                const bool ePressed = eDown && !eWasDown;
                if (ePressed && targetedItem) {
                    if (inventory.canCarry(targetedItem->item))
                        queueItemRequest({ItemRequest::PickUp, {}, *itemTarget});
                    else {
                        interface.chatAdd("You can't carry more of these items.\n", {255, 51, 51});
                        audio.play("human-voice01.wav");
                    }
                }
                const bool tDown = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !interface.typing();
                if (tDown && !tWasDown)
                    queueItemRequest({ItemRequest::CallPet});
                auto &wheel = interface.quickWheel;
                const int32_t steps = wheel.steer(frameElapsedMs, glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS,
                                                  glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS, closeQuickWheel);
                for (int32_t i = 0; i < steps; ++i)
                    playInterfaceSound(0x55);
                size_t wheelCount = 0;
                if (wheel.open) {
                    const auto choices = inventory.quickItems(c.level);
                    wheelCount = choices.size();
                    if (choices.empty()) {
                        interface.setQuickWheelLabel(nullptr);
                    } else {
                        const auto choice = choices[wheel.lateSelection(choices.size())];
                        const auto [bag, slot] = choice;
                        interface.setQuickWheelLabel(&inventory.bags[bag][slot]);
                        if ((rPressed || ePressed) && useQuickItem(choice))
                            wheel.open = false;
                    }
                }
                wheel.layout(frameElapsedMs, wheelCount);
                eWasDown = eDown;
                tWasDown = tDown;
                rWasDown = rDown;
                interface.hud = hud;
                // Missions §8.2: the small quest tag, shown inside a location's area (for a hunt,
                // within 130 blocks of the centre of the monster area's zone, +0x60/+0x64, which a
                // client of a server never receives and so holds zone (0, 0)), with hysteresis
                // between 0.05 and 0.1.
                {
                    const worldgen::World &world = view.terrainWorld();
                    const int32_t cx = worldgen::floorDiv((int32_t)std::floor(me.x), worldgen::SUBCELL);
                    const int32_t cy = worldgen::floorDiv((int32_t)std::floor(me.y), worldgen::SUBCELL);
                    const worldgen::Location *L = cellLocation(world, cx, cy);
                    const Missions::Cell *cell = missions.cell(cx, cy);
                    const MissionArea m = cell ? cell->mission : MissionArea{};
                    float v = L ? locationBump(world, *L, me.x, me.y) : 0.f;
                    if (L && m.kind == 1) {
                        const MonsterArea monster = cell ? cell->monster : MonsterArea{};
                        const double dx = me.x - (256.0 * monster.zoneX + 128), dy = me.y - (256.0 * monster.zoneY + 128);
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
                        interface.questTagShown = false; // step 5 hides the tag and the bar
                        interface.questBar = false;
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
            interface.leftHeld = !interface.controllerKeyboardOpen() && input.pointerButtons.held(0);
            // Item services §3.2: the middle button turns the bench's weapon.
            {
                double cx, cy;
                glfwGetCursorPos(window, &cx, &cy);
                if (interface.benchOpen && !interface.controllerKeyboardOpen() && input.pointerButtons.held(2))
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
                const bool picker = interface.front == ui::Interface::Front::Characters ||
                                    interface.front == ui::Interface::Front::Worlds;
                if (frontListsStale && (!picker || !terrainWait)) {
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
                        interface.characterList.push_back({r.name, r.level, creatureTypeOf(choice.race, choice.female),
                                                           r.characterClass, r.specialization, r.lastWorldSeed, r.lastWorldName});
                    }
                    interface.worldList.clear();
                    for (const auto &[mesh, size] : worldPreviews)
                        renderer.removeMesh(mesh);
                    worldPreviews.clear();
                    // §4.1: every entry, by world index; the screen lists one mode's worlds.
                    if (!worldMemory)
                        worldMemory = worlds();
                    for (const WorldEntry &w : *worldMemory) {
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
                // §3.9 Apply, run on Create, every arrow, palette click and hover change: the
                // player wears the choices (the hover colour), then the starting kit is made again
                // with its level-1 grants, spending their draws each time; Create Character saves
                // the last run's record.
                if (std::exchange(interface.creationChanged, false)) {
                    creatures::CharacterChoice choice;
                    choice.race = interface.creation.race;
                    choice.female = interface.creation.gender == 1;
                    choice.head = interface.creation.face;
                    choice.hair = interface.creation.haircut;
                    std::copy(interface.creation.hover.begin(), interface.creation.hover.end(), choice.hairColour);
                    if (player) {
                        creatures::Appearance looks;
                        int32_t type = 2;
                        creatures::generate(type, looks, &choice, [] { return 0; });
                        player->setCreationLook(type, looks);
                    }
                    creationRecord = newCharacter("", choice, interface.creation.characterClass + 1, [&] { return worldRandom.draw(); });
                    if (player && !playing()) {
                        auto &actor = player->combat();
                        actor.characterClass = creationRecord->characterClass;
                        actor.specialization = creationRecord->specialization;
                        inventory.equipment = creationRecord->equipment;
                        wearInventory();
                    }
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
                    interface.front = ui::Interface::Front::None;
                    requestedLocalWorld = std::move(next);
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
                    // §3.8: the record of the last Apply, with the typed name and level 1.
                    if (!creationRecord) { // (no Apply ran: the engine makes one now)
                        creatures::CharacterChoice choice;
                        choice.race = interface.creation.race;
                        choice.female = interface.creation.gender == 1;
                        choice.head = interface.creation.face;
                        choice.hair = interface.creation.haircut;
                        std::copy(interface.creation.hover.begin(), interface.creation.hover.end(), choice.hairColour);
                        creationRecord = newCharacter("", choice, interface.creation.characterClass + 1, [&] { return worldRandom.draw(); });
                    }
                    CharacterRecord created = *creationRecord;
                    created.name.clear();
                    for (char16_t ch : interface.nameText)
                        created.name += ch < 0x80 ? (char)ch : '?';
                    created.level = 1;
                    // Saves §3.1: `num` grows first, then the new character is saved under key N.
                    SaveDb db;
                    if (db.open(saves / "characters.db", SaveDb::Mode::ReadWrite)) {
                        chosenCharacter = (int)db.number("num").value_or(0);
                        addCharacter(db, created);
                    }
                    frontListsStale = true;
                    interface.front = ui::Interface::Front::Worlds;
                } else if (request == R::DeleteCharacter) {
                    // Saves §3.1: `num` shrinks, the key goes and the later keys are renamed down.
                    SaveDb db;
                    if (interface.selectedCharacter >= 0 && db.open(saves / "characters.db", SaveDb::Mode::ReadWrite))
                        deleteCharacter(db, (uint32_t)interface.selectedCharacter);
                    frontListsStale = true;
                } else if (request == R::SelectWorld && interface.selectedWorld < (int)interface.worldList.size()) {
                    startWorld(interface.worldList[interface.selectedWorld].name, interface.worldList[interface.selectedWorld].seed);
                } else if (request == R::CreateWorld) {
                    std::string name;
                    for (char16_t ch : interface.nameText)
                        name += ch < 0x80 ? (char)ch : '?';
                    startWorld(name, interface.worldSeed()); // §5.4: signed parse (overflow: C-6)
                } else if (request == R::DeleteWorld && worldMemory && interface.selectedWorld >= 0 &&
                           interface.selectedWorld < (int)worldMemory->size()) {
                    // §4.5: the world's two files go, the entry leaves the list in memory and Alpha's
                    // rewrite puts every remaining entry under the selected key. The selection stays.
                    const uint32_t k = (uint32_t)interface.selectedWorld;
                    const std::string name = (*worldMemory)[k].name;
                    fs::remove(worldSavePath(saves, name), ec);
                    fs::remove(worldSavePath(saves, name, WorldSaveFile::Map), ec);
                    SaveDb db;
                    if (db.open(saves / "worlds.db", SaveDb::Mode::ReadWrite))
                        deleteWorldEntry(db, *worldMemory, k);
                    else
                        worldMemory->erase(worldMemory->begin() + k);
                    frontListsStale = true;
                }
            } else {
                frontListsStale = true;
            }
            if (sayAtStart && player) {
                sayAtStart = false;
                interface.talk(player->combat().id, nothingSpecial);
            }
            interface.followBubbles(seconds * 1000.f); // talk §1.5 step 6, after the talk press
            // Screens §8.1.6: every interface item turns 30 degrees a second about z.
            interface.itemTurn.z += seconds * 1000.f * 0.03f;
            glm::vec2 uiMouse{float(mx), float(my)};
            if (interface.controllerKeyboardOpen()) {
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                if (width > 0 && height > 0)
                    uiMouse *= glm::vec2(float(size.width) / width, float(size.height) / height);
            }
            if (player)
                syncCursor(); // the front form can change after controller input
            renderer.setInterface(interface.draw({float(size.width), float(size.height)}, uiMouse, now - start, terrainWait));
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
            const double leafReach = creatures::leafZoneReach(scene.fogDistance); // §7.1, horizontal
            // §7.1: as each world second passes, every kind-60 record of the zones in sight
            // lets one leaf in sixteen fall.
            const bool second = leafClockMs >= 0 && std::floor(leafClockMs / 1000) != std::floor(scene.timeOfDayMs / 1000);
            leafClockMs = scene.timeOfDayMs;
            // Audio §3 with zone-fields ruling 1: birds sing from the crowns by day and owls call
            // at night; crickets chirp in the flowers and grass in the afternoon and evening. The
            // walk visits the chunk slot grid; a slot's records are tested only while its (possibly
            // stale) squared camera distance is below 4096 blocks². Engine adapters: the grid is
            // the fog window round the camera's chunk, the world clock is the time of day as an
            // integer, the view test is the shared admission with reach fog + 22.4 + 32 on the
            // chunk's bounding sphere, and a chunk's bounds span its columns' stored blocks.
            {
                const auto sight = view.viewAdmission(scene.fogDistance + 22.4f + 32.f);
                const int32_t clock = std::bit_cast<int32_t>(uint32_t(scene.timeOfDayMs));
                std::vector<sound::AmbientRecord> records;
                std::vector<sound::Request> calls;
                sound::AmbienceGrid::Source source;
                source.built = [&](int32_t x, int32_t y) { return view.chunkBuilt(x, y); };
                source.shape = [&](int32_t x, int32_t y) {
                    const int32_t x0 = x * 32, y0 = y * 32;
                    int32_t bottom = 0, top = 0;
                    if (const worldgen::Zone *zone = view.held().zoneAt(x0, y0)) {
                        bottom = INT32_MAX, top = INT32_MIN;
                        for (int32_t i = x0; i < x0 + 32; i++)
                            for (int32_t j = y0; j < y0 + 32; j++) {
                                const worldgen::Column &column = zone->column(i, j);
                                bottom = std::min(bottom, column.bottom);
                                top = std::max(top, column.top());
                            }
                    }
                    const int64_t one = 65536;
                    return sound::chunkShape({x, y}, {x0 * one, y0 * one, bottom * one},
                                             {(x0 + 32) * one, (y0 + 32) * one, top * one});
                };
                source.visible = [&](const sound::ChunkShape &c) { return sight && sight->accepts(c.midpoint, c.radius); };
                source.records = [&](glm::ivec2 c) {
                    const worldgen::Zone *zone = view.held().zoneAt(c.x * 32, c.y * 32);
                    if (!zone)
                        return; // (engine adapter: the slot's copies went with the zone)
                    records.clear();
                    zoneAmbience.records(*zone, c, records);
                    for (const sound::AmbientRecord &r : records)
                        sound::ambientRecord(r, scene.timeOfDayMs, clock, frameElapsedMs,
                                             [&] { return (int32_t)worldRandom.draw(); }, calls);
                };
                ambienceGrid.walk(view.chunkWindow(), alpha::FOG_WINDOW_CHUNKS, listener(), source);
                for (const sound::Request &call : calls) // (playing draws nothing)
                    playFixed(call.id, call.position, call.volume, call.pitch);
                zoneAmbience.retain([&](int32_t zx, int32_t zy) { return view.held().zone(zx, zy) != nullptr; });
            }
            const auto u = [&] { return (float)worldRandom.draw() / 32767.f; };
            for (const auto &[key, zone] : view.held().zones()) {
                const glm::dvec2 centre = glm::dvec2(key.first + 0.5, key.second + 0.5) * (double)worldgen::ZONE;
                const double away = glm::length(centre - glm::dvec2(eye));
                if (away > reach)
                    continue;
                // Per record and crossing: the mod-16 draw, then velocity y, x, then spin z, y, x.
                for (const worldgen::LeafSource &s : zone->leafSources) {
                    if (!second || away > leafReach || worldRandom.draw() % 16 != 0)
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
                        DrawItem flame{creatureView.cubeMesh(), at, false, Shading::World, glm::vec4(k.colour, 1)};
                        flame.layer = DrawLayer::Effect;
                        items.push_back(flame);
                    }
                }
            }
            particles.update((int32_t)std::lround(seconds * 1000.0), read);
            // §6.2: a particle is drawn only within R of the camera and inside the view planes
            // with a margin of its size.
            const auto sight = view.viewAdmission(creatures::particleCullRadius(scene.fogDistance));
            for (const creatures::Particle &p : particles.list) {
                if (!sight || !sight->accepts({p.position[0], p.position[1], p.position[2]}, p.size))
                    continue;
                glm::mat4 at = glm::translate(glm::mat4(1), view.toRender(p.blocks()));
                at = glm::rotate(at, glm::radians(p.rotation.z), glm::vec3(0, 0, 1));
                at = glm::rotate(at, glm::radians(p.rotation.y), glm::vec3(0, 1, 0));
                at = glm::rotate(at, glm::radians(p.rotation.x), glm::vec3(1, 0, 0));
                at = glm::scale(at, glm::vec3(p.size)) * p.extra * glm::translate(glm::mat4(1), glm::vec3(0, 0, -0.5f));
                // §6.3: splashes, block bits and leaves are lit like the world where they are;
                // the others at full strength.
                float light = 1;
                if (p.style == creatures::LIT || p.style == creatures::LEAF) {
                    // The light cell truncates toward zero, also for negative coordinates.
                    const int32_t x = creatures::lightCell(p.position[0]), y = creatures::lightCell(p.position[1]),
                                  z = creatures::lightCell(p.position[2]);
                    const uint8_t type = view.held().read(x, y, z).type();
                    const int value = type == 13 ? 255 : type == 0 || type == 2 ? std::max<int>(view.held().light(x, y, z), 5) : 0;
                    light = std::max(value / 255.f, 0.2f);
                }
                DrawItem particle{creatureView.cubeMesh(), at, false, Shading::World, glm::vec4(glm::vec3(p.colour), light)};
                particle.layer = DrawLayer::Effect; // rendering §1.3 step 15: depth writes off
                items.push_back(particle);
            }
        }
        // Screens §5.2.3: under the world map only the sky is drawn.
        if (interface.mapOpen)
            items.clear(), effects.clear();
        if (client.connected()) {
            std::optional<Missions::Key> region;
            if (player) {
                const auto &position = player->body().position;
                for (int axis = 0; axis < 3; axis++)
                    own.setPosition(axis, position[axis]);
                own.setMotion(player->body().velocity, player->facing(), player->body().push, player->body().intent);
                own.setFighter(player->combat());
                own.setContactFlags(player->body().contact);
                own.setManaCubeCounter(manaCubes);
                const auto zone = net::zoneForRawPosition(position[0], position[1]);
                if (view.held().needsZoneContents(zone))
                    client.requestZones(std::array{zone});
                else
                    client.requestZones({});
                region = Missions::Key{zone.first / 64, zone.second / 64};
            } else {
                const glm::dvec3 at = view.worldPosition();
                for (int axis = 0; axis < 3; axis++)
                    own.setPosition(axis, (int64_t)(at[axis] * 65536.0));
                client.requestZones({});
            }
            if (region && missions.needsRegionContents(view.terrainWorld(), *region))
                client.requestRegions(std::array{*region});
            else
                client.requestRegions({});
            client.setOwn(own);
        }
        if (networkFrame.owns_lock()) {
            if (client.connected())
                client.publishReceivedState(remotes, otherFighters);
            networkFrame.unlock();
        }
        renderer.setEffects(effects);
        const bool drawn = renderer.draw(frame, drawStars(scene), items,
                                         capturing ? pathOfUtf8(options.capture) : fs::path(), terrainWait);
        // Old source jobs may still hold the old world's generator. Keep polling input and drawing
        // the wait frame until they finish, then run the chosen session with its own persistent state.
        if (requestedLocalWorld && view.prepareWorldChange())
            restart = std::move(requestedLocalWorld);
        if (capturing) {
            if (!drawn) {
                std::fprintf(stderr, "capture failed\n");
                status = 1;
            }
            // Capture diagnostics: where the eye and the player were in the captured frame.
            const glm::dvec3 eyeAt = view.worldPosition();
            std::printf("Capture eye (%.2f, %.2f, %.2f), underwater %d\n", eyeAt.x, eyeAt.y, eyeAt.z, scene.underwater ? 1 : 0);
            if (player) {
                const glm::dvec3 feet = player->body().blocks();
                std::printf("Capture player (%.2f, %.2f, %.2f), contact 0x%x, health %.1f\n", feet.x, feet.y, feet.z,
                            unsigned(player->body().contact), double(player->combat().health));
                const auto &focus = player->followCamera().followAnchor();
                std::printf("Capture focus raw (%lld, %lld, %lld)\n", (long long)focus[0], (long long)focus[1],
                            (long long)focus[2]);
            }
            break;
        }
        if (framesUntilCapture > 0 && drawn && !settling)
            framesUntilCapture--;
    }
    // Window operations can dispatch input synchronously, including while the next world opens.
    glfwSetKeyCallback(window, nullptr);
    glfwSetCharCallback(window, nullptr);
    glfwSetMouseButtonCallback(window, nullptr);
    glfwSetCursorPosCallback(window, nullptr);
    glfwSetScrollCallback(window, nullptr);
    glfwSetWindowUserPointer(window, nullptr);
    if (glfwWindowShouldClose(window)) {
        view.prepareWorldChange();
        glfwHideWindow(window);
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
    if (onlineCharacter && player) { // written back under its own key in characters.db (saves §3.1)
        onlineCharacter->saveVisit(activeWorld.seed, activeWorld.name, std::to_array(player->body().position));
        const auto requested = client.worldIdentity();
        onlineCharacter->lastWorldSeed = requested.seed;
        onlineCharacter->lastWorldName = requested.name;
        SaveDb db;
        if (db.open(executableFolder() / "Save" / "characters.db", SaveDb::Mode::ReadWrite) &&
            (uint32_t)options.character < db.number("num").value_or(0))
            saveCharacter(db, (uint32_t)options.character, *onlineCharacter);
    }
    if (remoteMap) {
        regionPass(player ? player->body().blocks() : view.worldPosition());
        remoteMap->saveDiscoveryCount(view.discoveryCount());
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
        if (player)
            session->leaveAtFixedPosition(std::to_array(player->body().position));
        else
            session->leave(view.worldPosition());
    }
    if (options.travel > 0)
        std::fprintf(stderr, "after travelling %.0f blocks: %s\n", travelled, view.status().c_str());
    activeScene = nullptr;
    activeView = nullptr;
    activePlayer = nullptr;
    activeInterface = nullptr;
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
    glfwWindowHint(GLFW_MAXIMIZED, options.start ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, options.start && options.capture.empty() ? GLFW_FALSE : GLFW_TRUE);
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
            worldRun.extendedQuests = true; // core feature; no opt-in setting
            // Character screens §1.2: a world chosen on the front screens starts in this window.
            for (;;) {
                std::optional<Options> restart;
                status = runWorld(window, renderer, content, game, worldRun, shadowSettings, moonSettings, mods,
                                  activeScene, activeView, activePlayer, activeInterface, audio, restart);
                renderer.releaseWorld();
                if (!restart || glfwWindowShouldClose(window))
                    break;
                worldRun = *restart;
            }
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
