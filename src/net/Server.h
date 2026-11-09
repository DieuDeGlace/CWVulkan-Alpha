#pragma once
// AlphaVulkan's dedicated server, speaking Alpha's protocol so original Alpha clients and
// AlphaVulkan clients can join it (docs/specs/alpha-net-server.md §7). What it does today:
//
//   * the handshake (transport §2), with the 1..10 player ids and a real "server is full";
//   * every 20 ms per player: the creatures it may see (players at any distance, §4.1), the end
//     of the batch, the clock and the world update;
//   * relaying chat, player hits and shoots; applying buff inputs and distributing generated buff events;
//   * a world clock that runs and is saved as `time` in Save/world_server_<seed>.db (§1.2).
//
//   * the world simulation (docs/specs/alpha-server-simulation.md): zones generated round
//     the players (§2.1), their NPCs run by the same creature step as an offline client (§5),
//     players' hits on NPCs applied here and not relayed, NPC hits on players sent to them,
//     kills credited in section 10, sounds in section 4 (§6).
//
//   * world items (docs/specs/alpha-world-items.md): each zone's list, sent in section 7 when a
//     client asks for the zone or it changes, picked up and dropped through packet 6 (§8), with
//     the gifts in section 9; the lists are saved with the zones.
//
//   * static objects (docs/specs/alpha-interaction.md §6): doors, chests, seats and beds used
//     through packet 6 kind 3, their states in section 6 and saved with the zones.
//
//   * block edits (docs/specs/alpha-block-edits.md): big creatures' craters and voxel objects
//     placed through packet 6 kind 7, written into the zones, sent to everyone in section 1,
//     kept per zone, sent whole for a requested zone, and saved with the zones; the placed
//     objects' lists in section 8.
//
//   * missions (docs/specs/alpha-missions.md): each region's daily pass while a player is in
//     it, the zones' extra spawns (cell monsters, wandering groups), discovery, kills and
//     their rewards as gifts, the records in section 13 to everyone and a region's 64 on
//     packet 12, and the `mission` and `monster` saves.
#include "BlockEdits.h"
#include "Missions.h"
#include "Quests.h"
#include "StaticStates.h"
#include "WorldItems.h"
#include "creatures/Npcs.h"
#include "net/Protocol.h"
#include "net/Socket.h"
#include "worldgen/Terrain.h"
#include "worldgen/WorldMap.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace net {

struct ServerOptions {
    uint32_t seed = 0;
    uint16_t port = PORT;
    std::string bindAddress = "0.0.0.0";
    std::filesystem::path folder; // where Save/ lives (the working folder by default)
    std::function<void(const std::string &)> log; // console lines
    // The game's building models (towns, dungeons), so the zones match the clients'.
    worldgen::PrefabSource prefabs;
    bool extendedQuests = true; // required core feature in production (docs/design/quests.md)
    bool swimmableWater = true; // runtime extension; original clients generate different shallow terrain
    std::string generationProfile; // production entry point pins water/mod settings in the world save
    // Where a new player's first record puts it, in blocks (0, 0: the world's home point).
    // Original Alpha clients keep their own start (seen in a test on 2026-11-02).
    int32_t spawnX = 0, spawnY = 0;
};

class Server {
  public:
    explicit Server(ServerOptions options);
    ~Server();

    bool start(std::string *error = nullptr);
    void stop(); // closes every connection and saves the clock
    uint16_t port() const { return listener.boundPort(); }
    int players() const;
    // The world clock (packet 5).
    uint32_t day() const { return clockDay; }
    uint32_t timeOfDayMs() const { return clockMs; }

  private:
    struct Connection;
    void acceptLoop();
    void handshake(Socket socket);
    void receiveLoop(Connection &c);
    void sendLoop(Connection &c);
    void simulationLoop();
    void generationLoop(); // §2.1
    void stepWorld(float dtMs); // §3, under the lock
    std::vector<std::pair<int32_t, int32_t>> playerZones() const; // under the lock
    void drop(Connection &c);
    void saveClock();
    // Alpha-saves §5.2: a zone's items read back on loading, and written when they change.
    // The zone's saved edits are written into `zone` (block edits §4.2) before it is held.
    void restoreZoneItems(worldgen::Zone &zone);
    void saveZoneItems(int32_t zx, int32_t zy);
    void say(const std::string &line) const;
    // Block edits §4.3: writes records into the held zones (each changed zone is copied, so
    // readers of the old one are undisturbed). Recorded ones are listed for section 1 and
    // stored with their zones; a placement's interior air is not.
    void writeBlocks(const std::vector<BlockEdit> &edits, bool recorded = true);
    // §6: a placement from packet 6 kind 7, by the sender's record.
    void placeObject(const CreatureRecord &sender);

    ServerOptions options;
    Socket listener;
    std::atomic<bool> running{false};
    std::thread acceptThread, simulationThread, generationThread;
    std::unique_ptr<worldgen::World> world;
    std::unique_ptr<worldgen::Terrain> terrain;

    mutable std::mutex lock; // guards everything below
    std::map<uint64_t, std::shared_ptr<Connection>> connections; // by player id
    std::vector<std::thread> handshakes;
    std::vector<std::array<uint8_t, 72>> hits;
    std::vector<std::array<uint8_t, 112>> shoots;
    std::vector<std::array<uint8_t, 40>> events40;
    std::atomic<uint32_t> clockDay{0}, clockMs{43200000};
    worldgen::WorldMap zones; // the loaded zones (§4)
    // Dedicated-world engine policy: one stream, serialized by lock. This does
    // not certify original-server RNG/thread/startup behavior.
    worldgen::Random worldRandom{1};
    creatures::Npcs npcs{worldRandom};
    // Projectiles §2: the NPCs' projectiles (players' own are simulated by their clients), and
    // their hits on NPCs, applied with the next step's incoming hits.
    creatures::Projectiles projectiles;
    std::vector<creatures::HitRecord> shotHits;
    WorldItems items; // world items §1, per loaded zone
    StaticStates statics; // interaction §4.1, per loaded zone
    BlockEditLists edits;               // block edits §3, per loaded zone
    std::vector<BlockEdit> stepEdits;   // this step's records, for section 1
    // §6.4: each zone's placed objects (number, base block) and which lists changed.
    std::map<std::pair<int32_t, int32_t>, std::vector<std::array<int32_t, 4>>> placedObjects;
    std::set<std::pair<int32_t, int32_t>> placedChanged;
    std::map<int32_t, CubModel> blueprints; // §6.2, loaded on first use
    Missions missions;
    // Quests (docs/design/quests.md): each player's book, by player id, and whose changed.
    std::map<uint64_t, quests::Book> questBooks;
    std::set<uint64_t> questBooksChanged, questBooksUnsaved, questBooksLoaded;
    void questChanged(uint64_t player) { questBooksChanged.insert(player), questBooksUnsaved.insert(player); }
    // Each player's book lives in the world save as `av_quests_<character name>`: read when the
    // player's name is first known, written when it changes.
    void questTalk(uint64_t player, uint64_t giver, glm::dvec3 at, std::vector<std::array<uint8_t, 288>> &gifts,
                   std::vector<creatures::SoundEvent> &sounds);
    void questFree(uint64_t player, uint32_t quest, size_t index);
    void tell(uint64_t player, const std::string &line); // a chat line to one player
    std::set<std::pair<int32_t, int32_t>> missionRegionsRead, zonesForMissions;
    int32_t missionDay = -1;
    // Missions §7: a region's saved cells read back, and changed cells written.
    void readMissionRegion(int32_t rx, int32_t ry);
    void saveMissionCells(const std::set<std::pair<int32_t, int32_t>> &cells);
};

} // namespace net
