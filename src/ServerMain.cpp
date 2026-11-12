// AlphaServer: a dedicated server for Cube World Alpha's protocol (src/net/Server.h). Original
// Alpha clients and AlphaVulkan clients can join it on TCP port 12345.
//
//   AlphaServer [--folder DIR] [--port N] [--bind IPv4] [--game DIR] [--spawn X Y] [--native-water]
//
// The game's install (found as the client finds it, or --game) gives the building models the
// zone generator places, so the server's zones match its players'.
//
// Like Alpha's Server.exe it keeps the world seed in server.cfg in its folder (one decimal
// number); without the file it picks one and writes it (docs/specs/alpha-net-server.md §1.1).
// --spawn puts new players' first record at block (X, Y) instead of the home point (original
// Alpha clients keep their own start). Type Q and Enter to stop.
#include "CubModel.h"
#include "Config.h"
#include "DataStore.h"
#include "Install.h"
#include "Mods.h"
#include "OwnerContent.h"
#include "GenerationProfile.h"
#include "net/Server.h"
#include "worldgen/Random.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

int main(int argc, char **argv) {
    std::filesystem::path folder = std::filesystem::current_path();
    uint16_t port = net::PORT;
    std::string bindAddress = "0.0.0.0";
    std::string game;
    int32_t spawnX = 0, spawnY = 0;
    bool swimmableWater = true;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--game") && i + 1 < argc)
            game = argv[++i];
        else if (!std::strcmp(argv[i], "--folder") && i + 1 < argc)
            folder = argv[++i];
        else if (!std::strcmp(argv[i], "--port") && i + 1 < argc)
            port = (uint16_t)std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--bind") && i + 1 < argc)
            bindAddress = argv[++i];
        else if (!std::strcmp(argv[i], "--native-water"))
            swimmableWater = false;
        else if (!std::strcmp(argv[i], "--spawn") && i + 2 < argc) {
            spawnX = std::atoi(argv[++i]);
            spawnY = std::atoi(argv[++i]);
        }
    }

    // The seed: server.cfg, or what Alpha's server would pick and write (its C runtime's first
    // rand() with no srand, modulo 1000000; server open question S-1).
    uint32_t seed = 0;
    const auto cfg = folder / "server.cfg";
    if (std::ifstream in(cfg); in && (in >> seed)) {
    } else {
        seed = (uint32_t)(worldgen::Random(1).draw() % 1000000);
        std::ofstream(cfg) << seed;
    }

    net::ServerOptions options;
    options.seed = seed;
    options.port = port;
    options.bindAddress = bindAddress;
    options.folder = folder;
    options.spawnX = spawnX;
    options.spawnY = spawnY;
    options.swimmableWater = swimmableWater;
    options.log = [](const std::string &line) {
        std::printf("%s\n", line.c_str());
        std::fflush(stdout); // (lines show at once, also in a log file)
    };
    Content content;
    Config configuration;
    configuration.load(folder / "AlphaVulkan.ini");
    options.extendedQuests = true; // core engine quests, enabled automatically
    const auto installation = game.empty() ? install::locate(configuration.text("Game", "Path")) : pathOfUtf8(game);
    const auto ownerContent = owner::inspect(installation, false);
    if (!ownerContent.ready()) {
        for (const auto &problem : ownerContent.problems)
            std::fprintf(stderr, "%s\n", problem.c_str());
        std::fprintf(stderr, "AlphaServer requires owned content. Run AlphaSetup --game DIR --check.\n");
        return 1;
    }
    ModHost modHost;
    modHost.content = &content;
    modHost.log = [](const std::string &name, const std::string &line) {
        std::fprintf(stderr, "mod %s: %s\n", name.c_str(), line.c_str());
    };
    ModLoader mods(modHost);
    std::mutex prefabLock;
    std::map<std::string, std::shared_ptr<const worldgen::Prefab>> prefabCache;
    if (content.open(installation)) {
        mods.load(folder / "Mods", splitModList(configuration.text("Mods", "Load")), content);
        options.prefabs = [&](const std::string &key) -> std::shared_ptr<const worldgen::Prefab> {
            std::lock_guard guard(prefabLock);
            if (const auto it = prefabCache.find(key); it != prefabCache.end())
                return it->second;
            std::shared_ptr<worldgen::Prefab> prefab;
            CubModel model;
            if (const auto bytes = content.read(key); bytes && model.load(bytes->data(), bytes->size())) {
                prefab = std::make_shared<worldgen::Prefab>();
                prefab->sx = model.sx, prefab->sy = model.sy, prefab->sz = model.sz;
                prefab->rgb = std::move(model.rgb);
            }
            prefabCache[key] = prefab;
            return prefab;
        };
    } else {
        std::fprintf(stderr, "Cannot open the owned Alpha content stores.\n");
        return 1;
    }
    options.generationProfile = generationProfile(options.swimmableWater, mods.loaded());
    // Mod SDK on the dedicated server (docs/modding.md): events come from the simulation thread
    // with the server's lock released, so mods may call the host functions below.
    options.modEvents.frame = [&](double seconds) { mods.frame(seconds); };
    options.modEvents.playerJoin = [&](uint64_t id) { mods.playerJoin(id); };
    options.modEvents.playerLeave = [&](uint64_t id) { mods.playerLeave(id); };
    options.modEvents.chat = [&](uint64_t sender, const std::string &text) { mods.chat(sender, text); };
    options.modEvents.creatureDeath = [&](uint64_t id, int32_t type, int32_t level, uint64_t credited) {
        mods.creatureDeath(id, type, level, credited);
    };
    options.modEvents.missionComplete = [&](int32_t cx, int32_t cy) { mods.missionComplete(cx, cy); };
    options.modEvents.creatureTamed = [&](uint64_t id, int32_t type, uint64_t owner) { mods.creatureTamed(id, type, owner); };
    net::Server server(options);
    ModHost &host = mods.hooks();
    host.getTimeOfDay = [&] { return (double)server.timeOfDayMs(); };
    host.setTimeOfDay = [&](double ms) { server.setTimeOfDay((uint32_t)std::clamp(ms, 0.0, 86399999.0)); };
    host.getCreatures = [&] {
        std::vector<AvCreature> out;
        for (const net::ServerCreature &c : server.creatureList())
            out.push_back({c.id, c.fighter.type, c.fighter.controller, c.fighter.level, c.fighter.health,
                           creatures::maxHealth(c.fighter), c.at.x, c.at.y, c.at.z});
        return out;
    };
    host.getBlock = [&](int32_t x, int32_t y, int32_t z, uint8_t out[4]) {
        const auto b = server.block(x, y, z);
        if (!b)
            return false;
        out[0] = b->r, out[1] = b->g, out[2] = b->b, out[3] = b->type();
        return true;
    };
    host.setBlock = [&](int32_t x, int32_t y, int32_t z, const uint8_t block[4]) {
        return server.editBlock(x, y, z, {block[0], block[1], block[2], block[3]});
    };
    host.say = [&](const std::string &text) {
        server.broadcast(text);
        return true;
    };
    host.tell = [&](uint64_t player, const std::string &text) { return server.tellPlayer(player, text); };
    host.playerName = [&](uint64_t id) { return server.playerName(id); };
    struct HookReset { // the mods outlive the server: their shutdown must not reach it
        ModHost &host;
        ~HookReset() { host.getTimeOfDay = {}, host.setTimeOfDay = {}, host.getCreatures = {}, host.getBlock = {},
                       host.setBlock = {}, host.say = {}, host.tell = {}, host.playerName = {}; }
    } hookReset{host};
    std::string error;
    if (!server.start(&error)) {
        std::printf("%s\n", error.c_str());
        return 1;
    }
    std::printf("World seed %u, port %u.\n", seed, server.port());
    std::fflush(stdout);
    bool quit = false;
    for (std::string line; !quit && std::getline(std::cin, line);)
        quit = line == "Q" || line == "q";
    // Without a console to read (run as a background service) the server runs until stopped.
    while (!quit)
        std::this_thread::sleep_for(std::chrono::hours(1));
    server.stop();
    return 0;
}
