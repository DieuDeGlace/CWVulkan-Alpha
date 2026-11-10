#include "net/Server.h"
#include "net/CreaturePublication.h"
#include "GenerationProfile.h"
#include "Loot.h"
#include "Bytes.h"
#include "Saves.h"
#include "ZlibStream.h"
#include "worldgen/Regions.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <set>

namespace net {

using Clock = std::chrono::steady_clock;

struct Server::Connection {
    uint64_t id = 0;
    bool alphaVulkan = false; // sent our hello: gets the quest book
    Socket socket;
    CreatureRecord creature;
    std::vector<creatures::Buff> buffs; // core snapshots exclude the resident list
    creatures::Relations relations;
    uint64_t linkedId = 0;
    creatures::LinkedItemSnapshot linkedItemSnapshot;
    std::map<uint64_t, CreatureRecord> known; // what this player was sent last tick
    Clock::time_point joined = Clock::now();
    std::deque<std::pair<uint64_t, std::vector<uint16_t>>> chat;
    WorldUpdate outgoing;
    std::deque<std::array<uint8_t, 300>> itemRequests;     // packet 6, for the next step
    std::set<std::pair<int32_t, int32_t>> wantedZones;     // packet 11, answered in section 7
    std::set<std::pair<int32_t, int32_t>> wantedRegions;   // packet 12, answered in section 13
    std::thread receiver, sender;
    std::atomic<bool> alive{true};

    ~Connection() {
        // A connection that ended on its own is let go by whichever of its threads held it
        // last; one still listed at shutdown has its threads joined by Server::stop first.
        if (receiver.joinable())
            receiver.detach();
        if (sender.joinable())
            sender.detach();
    }
};

namespace {

template <class T> void put(CreatureRecord &r, size_t offset, T value) { std::memcpy(&r.bytes[offset], &value, sizeof value); }

// Server spec §3.1: the record a new player starts with (others see it for a moment, until the
// player's own first update replaces it). `spawnX/Y` (blocks) move it from the home point.
CreatureRecord newPlayer(int32_t spawnX, int32_t spawnY) {
    CreatureRecord r;
    const int64_t x = (int64_t)(spawnX ? spawnX : worldgen::HOME) << 16;
    const int64_t y = (int64_t)(spawnY ? spawnY : worldgen::HOME) << 16, z = (int64_t)64 << 16;
    r.setPosition(0, x);
    r.setPosition(1, y);
    r.setPosition(2, z);
    put(r, 0x1b0, x); // home position
    put(r, 0x1b8, y);
    put(r, 0x1c0, z);
    put(r, 0x070, 1.2f); // collision box (appearance +0x08)
    put(r, 0x074, 1.2f);
    put(r, 0x078, 2.7f);
    put(r, 0x054, (int32_t)44); // creature type
    put(r, 0x180, (int32_t)1);  // level
    put(r, 0x130, (uint8_t)1);  // class: warrior
    put(r, 0x15c, 100.f);       // health (the real maximum comes from stats not specified yet)
    return r;
}

// Simulation §5.5: the record clients are sent for an NPC.
CreatureRecord npcRecord(const creatures::Npc &n) {
    CreatureRecord r;
    const glm::dvec3 at = n.body.blocks();
    for (int axis = 0; axis < 3; axis++)
        r.setPosition(axis, (int64_t)(at[axis] * 65536.0));
    r.setMotion(n.body.velocity, n.yaw, n.body.push, n.body.intent);
    r.setAppearance(n.looks);
    r.setFighter(n.fighter);
    r.bytes[0x50] = n.fighter.controller;
    return r;
}

std::filesystem::path savePath(const ServerOptions &o) {
    return o.folder / "Save" / ("world_server_" + std::to_string(o.seed) + ".db");
}

} // namespace

Server::Server(ServerOptions o) : options(std::move(o)) {
    npcs.onFatalObject = [this](const creatures::Combatant &fighter, creatures::NpcFrame &frame) {
        statics.fatal(zones, fighter.backing, frame);
    };
    npcs.onFatalLoot = [this](const creatures::Combatant &fighter, const creatures::Body &body,
                              std::span<const creatures::CountedItem> inventory) {
        dropLoot(lootVictim(fighter, body, inventory), items, (int32_t)clockDay,
                 [this](int32_t x, int32_t y, int32_t z) { return zones.read(x, y, z).type(); }, worldRandom);
    };
    npcs.onFatalRegion = [this](creatures::Combatant &fighter, uint8_t marker,
                               const std::function<creatures::Combatant *(uint64_t)> &lookup, creatures::NpcFrame &frame) {
        if (const auto progress = missions.fatal(fighter, marker, 0,
                [this](int32_t x, int32_t y) { return world && world->region(x, y); }, lookup)) {
            frame.missions.push_back(*progress);
            if (progress->completed())
                npcs.trimMissionSpawns(fighter, zones);
        }
    };
    npcs.onFatalBacking = [this](const creatures::Combatant &fighter) {
        const auto &backing = fighter.backing;
        if (backing.x >= 0 && backing.y >= 0 && backing.x < 65536 && backing.y < 65536 && world &&
            world->region(backing.x / 64, backing.y / 64))
            npcs.markFatalBacking(fighter, zones, std::bit_cast<int32_t>(uint32_t(clockDay.load())));
    };
}

Server::~Server() { stop(); }

void Server::tell(uint64_t player, const std::string &line) {
    if (const auto it = connections.find(player); it != connections.end())
        it->second->chat.emplace_back(0, toUtf16(line));
}

// Quests (docs/design/quests.md), under the lock: talking to a giver takes its quest of the day
// or hands in a done one (the reward comes as gifts); the words come as chat lines, which
// original Alpha clients show too.
void Server::questTalk(uint64_t player, uint64_t giver, glm::dvec3 at, std::vector<std::array<uint8_t, 288>> &gifts,
                       std::vector<creatures::SoundEvent> &sounds) {
    const auto who = connections.find(player);
    const creatures::Npc *n = nullptr;
    for (const creatures::Npc &k : npcs.all())
        if (k.fighter.id == giver)
            n = &k;
    if (who == connections.end() || !n || n->fighter.controller != 3)
        return;
    quests::Book &book = questBooks[player];
    if (quests::Quest *q = book.byGiver(giver)) {
        if (q->state == quests::DONE) {
            const quests::Quest done = *q;
            tell(player, quests::thanksText(done));
            for (const worldgen::ItemRecord &item : quests::rewards(done, who->second->creature.fighter().characterClass, worldRandom)) {
                const creatures::ItemRecord record = creatures::toRecord(item);
                std::array<uint8_t, 288> gift{};
                std::memcpy(gift.data(), &player, 8);
                std::memcpy(gift.data() + 8, record.data(), 280);
                gifts.push_back(gift);
            }
            sounds.push_back({at, 30, 1, 1}); // missioncomplete.wav
            npcs.removeQuest(done.id);
            book.handedIn.push_back(done.id);
            std::erase_if(book.quests, [&](const quests::Quest &k) { return k.id == done.id; });
        } else {
            const std::u16string goal = quests::objectiveText(*q, at);
            tell(player, "Please hurry! " + fromUtf16(std::vector<uint16_t>(goal.begin(), goal.end())) + ".");
        }
        questChanged(player);
        return;
    }
    if (!quests::isGiver(giver, n->fighter.characterClass))
        return;
    const worldgen::Location *L = cellLocation(*world, worldgen::floorDiv((int32_t)std::floor(n->home.x), worldgen::SUBCELL),
                                               worldgen::floorDiv((int32_t)std::floor(n->home.y), worldgen::SUBCELL));
    const quests::Quest q = quests::offer(world->seed(), giver, n->fighter.type, n->home, (int32_t)clockDay,
                                          L ? L->level : who->second->creature.fighter().level, L ? L->subLevel : 0);
    if (std::find(book.handedIn.begin(), book.handedIn.end(), q.id) != book.handedIn.end())
        return;
    if (book.full()) {
        tell(player, "You have your hands full already. Come back later!");
        return;
    }
    book.quests.push_back(q);
    if (book.handedIn.size() > 64) // (only the recent ones matter: offers change daily)
        book.handedIn.erase(book.handedIn.begin(), book.handedIn.end() - 64);
    tell(player, quests::offerText(q, n->home));
    questChanged(player);
}

void Server::questFree(uint64_t player, uint32_t quest, size_t index) {
    quests::Book &book = questBooks[player];
    const quests::Quest *q = book.find(quest);
    const uint64_t id = creatures::Npcs::questCreatureId(quest, index);
    const creatures::Npc *n = nullptr;
    for (const creatures::Npc &k : npcs.all())
        if (k.fighter.id == id)
            n = &k;
    if (!q || !n)
        return;
    if (npcs.guarded(quest, n->body.blocks(), quests::FREE_CLEAR)) {
        tell(player, "Defeat the guards first!");
        return;
    }
    npcs.free(id, q->giverAt);
    questChanged(player);
    if (book.freed(quest))
        tell(player, "Quest done! Return to " + quests::giverName(*book.find(quest)) + ".");
}

void Server::say(const std::string &line) const {
    if (options.log)
        options.log(line);
}

bool Server::start(std::string *error) {
    say("Starting server...");
    if (!ensureGenerationProfile(savePath(options), options.generationProfile, error))
        return false;
    // Server spec §1.2: the clock comes back from the world save.
    {
        SaveDb db;
        if (db.open(savePath(options), SaveDb::Mode::ReadOnly))
            if (const auto t = db.get("time"))
                if (const auto clock = WorldClock::parse(*t)) {
                    clockDay = clock->day;
                    clockMs = clock->timeOfDayMs;
                }
    }
    if (!listener.listen(options.port, error, options.bindAddress))
        return false;
    world = std::make_unique<worldgen::World>(options.seed);
    terrain = std::make_unique<worldgen::Terrain>(*world);
    terrain->prefabs = options.prefabs;
    terrain->finishSpawn = creatures::finishSpawn;
    terrain->swimmableWater = options.swimmableWater;
    running = true;
    acceptThread = std::thread([this] { acceptLoop(); });
    simulationThread = std::thread([this] { simulationLoop(); });
    generationThread = std::thread([this] { generationLoop(); });
    say("Server started. Enter Q to quit.");
    return true;
}

void Server::stop() {
    if (!running.exchange(false))
        return;
    listener.shutdown();
    listener.close();
    std::vector<std::shared_ptr<Connection>> all;
    {
        std::lock_guard guard(lock);
        for (auto &[id, c] : connections)
            all.push_back(c);
    }
    for (auto &c : all) {
        c->alive = false;
        c->socket.shutdown();
    }
    if (acceptThread.joinable())
        acceptThread.join();
    std::vector<std::thread> pending;
    {
        std::lock_guard guard(lock);
        pending.swap(handshakes);
    }
    for (auto &t : pending)
        t.join();
    if (simulationThread.joinable())
        simulationThread.join();
    if (generationThread.joinable())
        generationThread.join();
    for (auto &c : all) {
        if (c->receiver.joinable())
            c->receiver.join();
        if (c->sender.joinable())
            c->sender.join();
    }
    {
        std::lock_guard guard(lock);
        connections.clear();
    }
    saveClock();
    say("stopping generation thread...");
}

int Server::players() const {
    std::lock_guard guard(lock);
    return (int)connections.size();
}

void Server::acceptLoop() {
    while (running) {
        Socket s = listener.accept();
        if (!s.valid())
            continue;
        // Each join gets its own thread, so a silent client cannot hold up the others
        // (transport §2.1 notes Alpha's single accept thread; this does not copy it).
        std::lock_guard guard(lock);
        handshakes.emplace_back([this, socket = std::move(s)]() mutable { handshake(std::move(socket)); });
    }
}

void Server::handshake(Socket socket) {
    socket.setReadTimeout(5000); // a client that says nothing is not kept waiting for
    uint32_t hello = 0, version = 0;
    if (!socket.readU32(hello) || hello != HELLO)
        return; // closed silently
    if (!socket.readU32(version))
        return;
    say("Client version: " + std::to_string(version));
    if (version != VERSION) {
        socket.sendU32(WRONG_VERSION);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        say("Wrong client version. Closing client connection.");
        return;
    }
    auto c = std::make_shared<Connection>();
    {
        std::lock_guard guard(lock);
        for (uint64_t id = 1; id <= MAX_PLAYERS && !c->id; id++)
            if (!connections.count(id))
                c->id = id;
        if (!c->id) {
            socket.sendU32(FULL); // what Alpha meant to send (transport §2.3)
            return;
        }
        c->creature = newPlayer(options.spawnX, options.spawnY);
        c->socket = std::move(socket);
        connections[c->id] = c;
    }
    c->socket.sendU32(ACCEPTED);
    say("New connection.");
    ByteWriter w;
    w.u32(CREATURE);
    w.u64(c->id);
    w.write(c->creature.bytes.data(), c->creature.bytes.size());
    w.u32(SEED);
    w.u32(options.seed);
    if (!c->socket.send(w.data())) {
        std::lock_guard guard(lock);
        connections.erase(c->id);
        return;
    }
    say("Player " + std::to_string(c->id) + " joined.");
    c->socket.setReadTimeout(0);
    c->receiver = std::thread([this, c] { receiveLoop(*c); });
    c->sender = std::thread([this, c] { sendLoop(*c); });
}

void Server::drop(Connection &c) {
    if (!c.alive.exchange(false))
        return;
    c.socket.shutdown();
    say("Player " + std::to_string(c.id) + " left.");
}

void Server::receiveLoop(Connection &c) {
    auto readArray = [&](auto &a) { return c.socket.readExact(a.data(), a.size()); };
    while (c.alive && running) {
        uint32_t id;
        if (!c.socket.readU32(id))
            break;
        bool ok = true;
        switch (id) {
        case CREATURE: { // the client's own creature; its id field is ignored (packets §2.3)
            uint32_t length;
            ok = c.socket.readU32(length) && length > 0 && length < (4u << 20);
            std::vector<uint8_t> packed(ok ? length : 0);
            ok = ok && c.socket.readExact(packed.data(), length);
            const auto plain = ok ? zlibInflate(packed, 1u << 20) : std::nullopt;
            const CreaturePublication publication{
                lock,
                [&](uint64_t key) -> CreatureRecord * {
                    const auto found = connections.find(key);
                    return found == connections.end() ? nullptr : &found->second->creature;
                },
                // Existing engine connection admission is separate from the
                // held raw assignment; it does not mask the resident mode word.
                [](CreatureRecord &record) { put(record, 0x050, uint8_t(0)); }
            };
            ok = plain && publishCreatureUpdate(c.id, *plain, publication);
            break;
        }
        case ITEM_RECORD: { // server simulation §8: handled by the next step
            std::array<uint8_t, 300> record;
            ok = readArray(record);
            std::lock_guard guard(lock);
            c.itemRequests.push_back(record);
            break;
        }
        case HIT: {
            std::array<uint8_t, 72> record;
            ok = readArray(record);
            std::lock_guard guard(lock);
            hits.push_back(record);
            break;
        }
        case EVENT40: {
            std::array<uint8_t, 40> record;
            ok = readArray(record);
            std::lock_guard guard(lock);
            events40.push_back(record);
            break;
        }
        case SHOOT: {
            std::array<uint8_t, 112> record;
            ok = readArray(record);
            std::lock_guard guard(lock);
            shoots.push_back(record);
            break;
        }
        case CHAT: { // to everyone, the sender included (packets §4.3)
            uint32_t n;
            ok = c.socket.readU32(n) && n <= 4096;
            std::vector<uint16_t> units(ok ? n : 0);
            ok = ok && c.socket.readExact(units.data(), n * 2);
            if (ok && n > 0) {
                std::lock_guard guard(lock);
                if (avMessage(units, '1')) { // an AlphaVulkan client: not a line to pass on
                    c.alphaVulkan = true;
                    questChanged(c.id);
                } else {
                    for (auto &[pid, other] : connections)
                        other->chat.emplace_back(c.id, units);
                }
            }
            break;
        }
        case ZONE_REQUEST: { // net server §4.3: answered with the zone's items
            int32_t zone[2];
            ok = c.socket.readExact(zone, 8);
            std::lock_guard guard(lock);
            c.wantedZones.insert({zone[0], zone[1]});
            break;
        }
        case REGION_REQUEST: { // net packets §4.4: answered with the region's 64 cell records
            int32_t region[2];
            ok = c.socket.readExact(region, 8);
            std::lock_guard guard(lock);
            c.wantedRegions.insert({region[0], region[1]});
            break;
        }
        default:
            ok = false; // an unknown id would desynchronise the stream: end the connection
        }
        if (!ok)
            break;
    }
    drop(c);
}

void Server::sendLoop(Connection &c) {
    auto next = Clock::now();
    while (c.alive && running) {
        ByteWriter w;
        {
            std::lock_guard guard(lock);
            // §4.1: every visible creature, each against what this player was sent last tick.
            std::map<uint64_t, CreatureRecord> toSend;
            bool introduced = false;
            const bool settled = Clock::now() - c.joined > std::chrono::milliseconds(500);
            for (auto &[id, other] : connections) {
                if (id == c.id || !other->alive)
                    continue;
                // (Players are visible at any distance; the 200-block radius is for NPCs.)
                if (!c.known.count(id)) {
                    if (!settled || introduced)
                        continue;
                    introduced = true;
                }
                toSend[id] = other->creature;
            }
            // Simulation §5.5: the NPCs within 200 blocks.
            const glm::dvec3 me = glm::dvec3(c.creature.position(0), c.creature.position(1), c.creature.position(2)) / 65536.0;
            for (const creatures::Npc &n : npcs.all()) {
                const glm::dvec3 d = n.body.blocks() - me;
                if (glm::dot(d, d) < 200.0 * 200.0 && !connections.count(n.fighter.id)) // (ids of zones by (0, 0) could clash)
                    toSend[n.fighter.id] = npcRecord(n);
            }
            for (auto &[id, record] : toSend) {
                const auto it = c.known.find(id);
                const uint64_t mask = changedFields(record, it == c.known.end() ? nullptr : &it->second);
                w.u32(CREATURE);
                w.bytes(compressed(encodeDelta(id, record, mask)));
            }
            c.known = std::move(toSend);
            w.u32(END_OF_BATCH);
            w.u32(CLOCK);
            w.u32(clockDay);
            w.u32(clockMs);
            w.u32(WORLD_UPDATE);
            w.bytes(compressed(c.outgoing.encode()));
            c.outgoing = {};
            if (!c.chat.empty()) {
                const auto &[from, units] = c.chat.front();
                w.u32(CHAT);
                w.u64(from);
                w.u32((uint32_t)units.size());
                w.write(units.data(), units.size() * 2);
                c.chat.pop_front();
            }
        }
        if (!c.socket.send(w.data()))
            break;
        next += std::chrono::milliseconds(TICK_MS);
        std::this_thread::sleep_until(std::max(next, Clock::now()));
    }
    drop(c);
    std::lock_guard guard(lock);
    // Quests: the leaving player's book is written now and forgotten (ids are reused).
    if (questBooksUnsaved.erase(c.id) && !c.creature.name().empty()) {
        SaveDb db;
        if (db.open(savePath(options), SaveDb::Mode::ReadWrite))
            db.put("av_quests_" + c.creature.name(), questBooks[c.id].save());
    }
    for (const quests::Quest &q : questBooks[c.id].quests) // (made again when the player is back)
        npcs.removeQuest(q.id);
    questBooks.erase(c.id), questBooksLoaded.erase(c.id), questBooksChanged.erase(c.id);
    connections.erase(c.id); // others stop seeing it; their next packet 2 removes it
}

void Server::simulationLoop() {
    auto last = Clock::now(), lastSave = last;
    double leftoverMs = 0;
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));
        const auto now = Clock::now();
        leftoverMs += std::chrono::duration<double, std::milli>(now - last).count();
        const int32_t dt = (int32_t)leftoverMs;
        leftoverMs -= (double)dt;
        last = now;
        {
            std::lock_guard guard(lock);
            // The time of day runs 10 times as fast as real time (vegetation-wind.md §2).
            uint64_t ms = clockMs + (uint64_t)(dt * 10);
            uint32_t day = clockDay;
            while (ms >= 86400000) {
                ms -= 86400000;
                day++;
            }
            clockMs = (uint32_t)ms;
            clockDay = day;
            stepWorld((float)dt);
            // Received shoots are relayed. Buff inputs were applied by stepWorld;
            // their list is separate from snapshots and generated outgoing events.
            for (auto &[id, c] : connections)
                for (const auto &s : shoots)
                    c->outgoing.sections[4].emplace_back(s.begin(), s.end());
            shoots.clear();
        }
        if (now - lastSave > std::chrono::seconds(10)) {
            saveClock();
            lastSave = now;
        }
    }
}

std::vector<std::pair<int32_t, int32_t>> Server::playerZones() const {
    std::vector<std::pair<int32_t, int32_t>> out;
    for (const auto &[id, c] : connections)
        if (c->alive)
            out.push_back({worldgen::floorDiv((int32_t)(c->creature.position(0) >> 16), worldgen::ZONE),
                           worldgen::floorDiv((int32_t)(c->creature.position(1) >> 16), worldgen::ZONE)});
    return out;
}

// Simulation §2.1: one zone per pass, the missing one nearest a player within the 5 x 5 block
// round each player's zone; once a second, zones no player is near go.
void Server::generationLoop() {
    auto lastUnload = Clock::now();
    while (running) {
        std::optional<std::pair<int32_t, int32_t>> wanted;
        {
            std::lock_guard guard(lock);
            int64_t best = INT64_MAX;
            for (const auto &[px, py] : playerZones())
                for (int32_t dx = -2; dx <= 2; dx++)
                    for (int32_t dy = -2; dy <= 2; dy++) {
                        const int64_t d = (int64_t)dx * dx + (int64_t)dy * dy;
                        if (d < 9 && d < best && !zones.zone(px + dx, py + dy)) {
                            best = d;
                            wanted = {px + dx, py + dy};
                        }
                    }
            if (Clock::now() - lastUnload > std::chrono::seconds(1)) {
                lastUnload = Clock::now();
                const auto near = playerZones();
                std::vector<std::pair<int32_t, int32_t>> gone;
                for (const auto &[key, zone] : zones.zones())
                    if (std::none_of(near.begin(), near.end(), [&](const auto &p) {
                            const int64_t dx = key.first - p.first, dy = key.second - p.second;
                            return dx * dx + dy * dy < 9;
                        }))
                        gone.push_back(key);
                for (const auto &key : gone) {
                    zones.remove(key.first, key.second);
                    items.forget(key.first, key.second);
                    statics.forget(key.first, key.second);
                    edits.forget(key.first, key.second);
                    placedObjects.erase(key);
                }
            }
        }
        if (!wanted) {
            // (Alpha's thread spins without sleeping; a short wait spares the processor.)
            std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));
            continue;
        }
        std::shared_ptr<worldgen::Zone> zone = terrain->generate(wanted->first, wanted->second);
        std::lock_guard guard(lock);
        items.generated(*zone);
        statics.generated(*zone);
        restoreZoneItems(*zone);
        zonesForMissions.insert({zone->zx, zone->zy}); // missions §2.6
        zones.add(std::move(zone));
    }
}

// Simulation §3, the parts the engine has: incoming hits, the creature step (activation,
// despawn, behaviour, hits, deaths), and what it produced into each player's world update.
void Server::stepWorld(float dtMs) {
    npcs.setNamedWorld(true); // this engine's dedicated world has a persistent profile
    // The players as the creatures see them: their own records, which they move themselves.
    std::vector<creatures::Combatant> fighters;
    std::vector<creatures::Body> bodies;
    std::vector<std::shared_ptr<Connection>> who;
    for (const auto &[id, c] : connections) {
        if (!c->alive)
            continue;
        creatures::Combatant f = c->creature.fighter();
        f.id = id;
        f.controller = 0;
        f.health = c->creature.health();
        f.buffs = std::move(c->buffs);
        f.relations = std::move(c->relations);
        f.linkedId = c->linkedId;
        f.linkedItem.retained = c->linkedItemSnapshot;
        creatures::Body b;
        b.place(glm::dvec3(c->creature.position(0), c->creature.position(1), c->creature.position(2)) / 65536.0);
        b.velocity = c->creature.velocity();
        b.intent = c->creature.intent();
        b.push = c->creature.response();
        b.size = c->creature.appearance().box;
        b.mode = f.mode;
        fighters.push_back(std::move(f));
        bodies.push_back(b);
        who.push_back(c);
    }
    std::vector<creatures::PlayerView> players;
    for (size_t i = 0; i < fighters.size(); i++)
        players.push_back({fighters[i], bodies[i], who[i]->creature.yaw(), fighters[i].actionMs, false, true});

    // §6.1: a hit on a player is relayed for that player's client to apply; one on an NPC is
    // applied here and not relayed.
    std::vector<creatures::HitRecord> onNpcs;
    for (const auto &bytes : hits) {
        Hit h;
        h.bytes = bytes;
        const creatures::Combatant *target = nullptr;
        for (const auto &fighter : fighters)
            if (fighter.id == h.target()) {
                target = &fighter;
                break;
            }
        if (!target)
            for (const auto &npc : npcs.all())
                if (npc.fighter.id == h.target()) {
                    target = &npc.fighter;
                    break;
                }
        if (!target || target->health <= 0)
            continue;
        // This standalone world has no distinguished local creature. Native
        // role-zero relay precedes source lookup and includes the sender.
        if (target->controller == 0) {
            for (auto &[id, c] : connections)
                c->outgoing.sections[1].emplace_back(bytes.begin(), bytes.end());
            continue;
        }
        onNpcs.push_back(h.record());
    }
    hits.clear();
    onNpcs.insert(onNpcs.end(), shotHits.begin(), shotHits.end());
    shotHits.clear();
    std::vector<creatures::BuffEvent> buffInput;
    buffInput.reserve(events40.size());
    for (const auto &bytes : events40)
        buffInput.push_back(creatures::BuffEvent::parse(bytes.data()));
    events40.clear();

    const auto block = [this](int32_t x, int32_t y, int32_t z) { return zones.read(x, y, z).type(); };
    const auto draw = [this] { return worldRandom.draw(); };

    // §8: the players' packet-6 requests. Picking up (kind 5) gives the item back in section 9;
    // dropping (kind 6) puts it in the world at the player's feet.
    std::vector<creatures::SoundEvent> itemSounds;
    std::vector<std::array<uint8_t, 288>> gifts;
    std::vector<Hit> healHits; // §8 kind 1: a negative hit on the drinker heals its client
    for (auto &[id, c] : connections) {
        const glm::dvec3 at = glm::dvec3(c->creature.position(0), c->creature.position(1), c->creature.position(2)) / 65536.0;
        for (const auto &r : c->itemRequests) {
            creatures::ItemRecord item;
            std::memcpy(item.data(), r.data(), 280);
            int32_t target[3];
            std::memcpy(target, &r[0x118], 12);
            switch (r[0x128]) {
            case 5:
                if (const auto got = items.pickUp(target[0], target[1], target[2], at)) {
                    std::array<uint8_t, 288> gift{};
                    std::memcpy(gift.data(), &id, 8);
                    std::memcpy(gift.data() + 8, got->data(), 280);
                    gifts.push_back(gift);
                    itemSounds.push_back({at, 45, 1 + 0.1f * (float)draw() / 32767.f, 1}); // pickup.wav
                }
                break;
            case 6: {
                const glm::dvec3 feet = at - glm::dvec3(0, 0, c->creature.appearance().box.z / 2);
                if (items.drop(item, feet, 1, (int32_t)clockDay, block, draw))
                    itemSounds.push_back({at, 57, 1, 1}); // drop.wav
                break;
            }
            case 3: // interaction §6: the player's own record moves on its client
                if (const worldgen::Zone *zone = zones.zone(target[0], target[1])) {
                    creatures::Combatant who = c->creature.fighter();
                    who.id = id;
                    creatures::Body body;
                    body.place(at);
                    body.size = c->creature.appearance().box;
                    float yaw = c->creature.yaw();
                    statics.use(*zone, target[2], who, body, yaw, items, (int32_t)clockDay, block, draw, itemSounds);
                }
                break;
            case 1: // §8 kind 1: a bomb is placed at the player; anything else heals
                if (creatures::itemType(item) == 1 && creatures::itemSubtype(item) == 7) {
                    npcs.placeBomb(at, creatures::itemLevel(item));
                } else {
                    const float heal = creatures::itemHeal(creatures::Item::parse(item.data()));
                    itemSounds.push_back({at, 44, 1, 1}); // drink2.wav
                    healHits.push_back(Hit::make(id, id, -heal, 0, glm::vec3(0), 0));
                }
                break;
            case 2: { // §8 kind 2: talking to an innkeeper at night rests the world to 7:00
                if (target[0] == -2) { // (AlphaVulkan's own: freeing quest `target[1]`'s prisoner `target[2]`)
                    questFree(id, (uint32_t)target[1], (size_t)target[2]);
                    break;
                }
                const uint64_t npcId = creatures::Npcs::spawnId(target[0], target[1], (size_t)target[2]);
                if (options.extendedQuests)
                    questTalk(id, npcId, at, gifts, itemSounds);
                bool innkeeper = false;
                for (const creatures::Npc &n : npcs.all())
                    innkeeper |= n.fighter.id == npcId && n.fighter.characterClass == 132;
                const uint32_t ms = clockMs;
                if (innkeeper && (ms >= 18 * 3600000u || ms <= 6 * 3600000u)) {
                    clockDay = clockDay + 1;
                    clockMs = 7 * 3600000u;
                    itemSounds.push_back({at, 29, 1, 0.75f}); // levelup2.wav
                    npcs.restAll();
                    missions.clearGenerated(); // missions §2.1: a new day's missions
                }
                break;
            }
            case 8: // pets §13: the T key calls the pet back
                for (auto &player : players)
                    if (player.fighter.id == id) {
                        npcs.callPet(player.fighter, at);
                        break;
                    }
                break;
            case 7: // block edits §6: a voxel object, by the sender's record
                placeObject(c->creature);
                break;
            default:
                break;
            }
        }
        c->itemRequests.clear();
    }
    items.step((int32_t)dtMs, true, block, draw, itemSounds);
    // Interaction §6.3, §6.4: the statics' timers; a seat is freed when its player leaves it.
    std::map<StaticStates::Key, const worldgen::Zone *> held;
    for (const auto &[key, zone] : zones.zones())
        held[key] = zone.get();
    statics.step((int32_t)dtMs,
                 [this](uint64_t who, glm::dvec3 &at, glm::vec3 &intent, int32_t &action) {
                     const auto it = connections.find(who);
                     if (it == connections.end() || !it->second->alive) {
                         for (const creatures::Npc &n : npcs.npcs()) // seated residents
                             if (n.fighter.id == who && n.fighter.health > 0) {
                                 at = n.body.blocks();
                                 intent = n.body.intent;
                                 action = n.fighter.action;
                                 return true;
                             }
                         return false;
                     }
                     const CreatureRecord &r = it->second->creature;
                     at = glm::dvec3(r.position(0), r.position(1), r.position(2)) / 65536.0;
                     intent = glm::vec3(0); // (only the action tells the server the player got up)
                     action = r.fighter().action;
                     return true;
                 },
                 held);
    // Traps (alpha-open-points-1.md §1): their cycle and hits; NPCs take theirs in this step,
    // players through section 2.
    std::vector<creatures::HitRecord> trapHits;
    {
        std::vector<StaticStates::Creature> here;
        for (const auto &[pid, c] : connections)
            if (c->alive)
                here.push_back({pid, glm::dvec3(c->creature.position(0), c->creature.position(1), c->creature.position(2)) / 65536.0, 0});
        for (const creatures::Npc &n : npcs.all())
            if (n.fighter.health > 0)
                here.push_back({n.fighter.id, n.body.blocks(), n.fighter.controller});
        statics.traps((int32_t)dtMs, clockMs, here, held,
                      [this](glm::dvec3 at) { return world->levelAt((int32_t)std::floor(at.x), (int32_t)std::floor(at.y)); },
                      trapHits, itemSounds);
        for (const creatures::HitRecord &h : trapHits)
            if (!connections.count(h.target))
                onNpcs.push_back(h);
    }

    if (!npcs.objects) // physics §4.5: the creatures collide with the zones' statics
        npcs.objects = [this](glm::dvec3 lo, glm::dvec3 hi, int axis, double move, const creatures::IgnoredStatics *ignored) {
            return statics.blocks(zones, lo, hi, axis, move, ignored);
        };
    if (!npcs.descriptorContext)
        npcs.descriptorContext = [this](int32_t zx, int32_t zy) {
            return missions.descriptorContext(zx, zy, [this](int32_t rx, int32_t ry) {
                return world && world->region(rx, ry) != nullptr;
            });
        };
    if (!npcs.routeActivate)
        npcs.routeActivate = [this](const creatures::route::Record &record) { statics.activateRouteRecord(zones, record); };
    if (!npcs.sightRecords)
        npcs.sightRecords = [this](int32_t zx, int32_t zy, const creatures::visibility::RecordVisitor &visit) {
            statics.visibilityRecords(zones, zx, zy, visit);
        };
    if (!npcs.routeRecords)
        npcs.routeRecords = [this](int32_t x, int32_t y, const creatures::route::RecordVisitor &visit) {
            statics.routeRecords(zones, x, y, visit);
        };
    if (!npcs.routeColumns)
        npcs.routeColumns = [this](int32_t x, int32_t y) -> std::optional<creatures::route::Column> {
            const auto *zone = zones.zoneAt(x, y);
            if (!zone) return std::nullopt;
            const auto &column = zone->column(x, y);
            return creatures::route::Column{column.bottom, int32_t(column.blocks.size()), std::as_bytes(std::span(column.blocks))};
        };
    if (!npcs.sit) { // trails spec §4: residents sit on the seats round them
        npcs.seats = [this](glm::ivec3 block, double ms) { return statics.seats(zones, block, ms); };
        npcs.seatFree = [this](const creatures::Seat &s) { return statics.free(s); };
        npcs.sit = [this](creatures::Npc &n, const creatures::Seat &s) {
            if (const worldgen::Zone *zone = zones.zone(s.zx, s.zy)) {
                std::vector<creatures::SoundEvent> ignored;
                statics.use(*zone, s.index, n.fighter, n.body, n.yaw, items, (int32_t)clockDay,
                            [this](int32_t x, int32_t y, int32_t z) { return zones.read(x, y, z).type(); },
                            [this] { return worldRandom.draw(); }, ignored);
            }
        };
    }
    if (!missions.wildlife)
        missions.wildlife = [this](int32_t x, int32_t y, int32_t h, worldgen::Random &random) {
            return terrain->wildlifeSpecies(x, y, h, 0, random);
        };
    // Missions §2.1, §2.6, §4, §5.4: the daily pass of each region a player stands in, the
    // zone passes, discovery and leftover targets.
    {
        const worldgen::World &w = *world;
        std::vector<Missions::Player> mplayers;
        for (size_t i = 0; i < fighters.size(); i++)
            mplayers.push_back({bodies[i].blocks(), fighters[i].level});
        if ((int32_t)clockDay != missionDay) {
            missionDay = (int32_t)clockDay;
            missions.clearGenerated();
        }
        for (const Missions::Player &p : mplayers) {
            const int32_t rx = worldgen::floorDiv((int32_t)std::floor(p.position.x), worldgen::REGION);
            const int32_t ry = worldgen::floorDiv((int32_t)std::floor(p.position.y), worldgen::REGION);
            if (!w.region(rx, ry))
                continue;
            readMissionRegion(rx, ry);
            if (missions.generated(rx, ry))
                continue;
            std::set<std::pair<int32_t, int32_t>> regionZones;
            for (const auto &[key, zone] : zones.zones())
                if (worldgen::floorDiv(key.first, 64) == rx && worldgen::floorDiv(key.second, 64) == ry)
                    regionZones.insert(key);
            npcs.removeTargets(regionZones);
            missions.dailyPass(w, rx, ry, mplayers, worldRandom);
            zonesForMissions.insert(regionZones.begin(), regionZones.end());
        }
        const auto ground = [this](int32_t x, int32_t y) {
            const worldgen::Zone *z = zones.zoneAt(x, y);
            return z ? (float)z->column(x, y).top() : 0.f;
        };
        for (const auto &[zx, zy] : std::exchange(zonesForMissions, {}))
            if (const worldgen::Zone *zone = zones.zone(zx, zy))
                npcs.setExtraSpawns(zx, zy, zone->spawns.size(), missions.zonePass(w, zx, zy, ground, mplayers, worldRandom));
        for (const Missions::Player &p : mplayers)
            missions.discover(w, p.position);
        for (creatures::Npc &n : npcs.npcs()) {
            const auto [cx, cy] = cellOfZone(n.zone.first, n.zone.second);
            const Missions::Cell *cell = missions.cell(cx, cy);
            if ((n.looks.flags & 0x2000) && n.fighter.relations.threats.empty() && cell && cell->mission.state == 2)
                n.fighter.health = 0;
        }
    }

    if (options.extendedQuests) {
        // Books from the save once a player's name is known; changed ones written back.
        std::vector<std::pair<std::string, std::vector<uint8_t>>> writes;
        {
            SaveDb db;
            bool open = false;
            for (const auto &[pid, c] : connections) {
                const std::string name = c->creature.name();
                if (name.empty() || questBooksLoaded.count(pid))
                    continue;
                questBooksLoaded.insert(pid);
                if (!open)
                    open = db.open(savePath(options), SaveDb::Mode::ReadOnly);
                if (open)
                    if (const auto value = db.get("av_quests_" + name); value && questBooks[pid].quests.empty()) {
                        questBooks[pid] = quests::Book::load(*value);
                        questBooksChanged.insert(pid);
                    }
            }
            for (uint64_t pid : std::exchange(questBooksUnsaved, {}))
                if (const auto c = connections.find(pid); c != connections.end() && !c->second->creature.name().empty())
                    writes.push_back({"av_quests_" + c->second->creature.name(), questBooks[pid].save()});
        }
        if (!writes.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(options.folder / "Save", ec);
            SaveDb db;
            if (db.open(savePath(options), SaveDb::Mode::ReadWrite))
                for (const auto &[key, value] : writes)
                    db.put(key, value);
        }
        const auto ground = [this](int32_t x, int32_t y) {
            for (int32_t z = 400; z > 0; z--) {
                const uint8_t t = zones.read(x, y, z - 1).type();
                if (t != 0 && t != 2)
                    return z;
            }
            return 0;
        };
        for (auto &[who, book] : questBooks)
            for (quests::Quest &q : book.quests)
                if (q.state == quests::ACTIVE && !npcs.hasQuestCreatures(q.id) &&
                    zones.zoneAt((int32_t)std::floor(q.site.x), (int32_t)std::floor(q.site.y))) {
                    q.site.z = ground((int32_t)std::floor(q.site.x), (int32_t)std::floor(q.site.y));
                    npcs.setQuestCreatures(q.id, quests::creatures(q, ground));
                    questChanged(who);
                }
    }
    const creatures::NpcFrame frame = npcs.step(dtMs, zones, std::move(players), clockMs, (int32_t)clockDay,
                                               block, onNpcs, {}, buffInput);
    for (const auto &grant : frame.grants)
        gifts.push_back(grant.serialize());
    for (const creatures::NpcFrame::Death &death : frame.deaths) // quests: hunts and battles
        if (death.quest)
            for (auto &[who, book] : questBooks)
                if (book.find(death.quest)) {
                    questChanged(who);
                    if (book.died(death.quest, death.questRole))
                        tell(who, "Quest done! Return to " + quests::giverName(*book.find(death.quest)) + ".");
                }

    // Gift generation retains the later engine adapter; hit-driven regional
    // admission, snapshots and extra-spawn disposal have already run.
    const auto rewardMission = [&](const MissionProgress &progress) {
        if (progress.rewardsApplied || !progress.completed() || progress.victimRole != 1 || !connections.count(progress.contributor))
            return;
        const auto who = connections.at(progress.contributor);
        for (const auto &item : missionRewards(progress.mission, who->creature.fighter().characterClass, worldRandom)) {
            const auto record = creatures::toRecord(item);
            std::array<uint8_t, 288> gift{};
            std::memcpy(gift.data(), &progress.contributor, 8);
            std::memcpy(gift.data() + 8, record.data(), 280);
            gifts.push_back(gift);
        }
    };
    for (const auto &progress : frame.missions)
        rewardMission(progress);
    // Separate legacy adapter for non-hit corpse deaths.
    for (const creatures::NpcFrame::Death &death : frame.deaths) {
        if (death.regionApplied)
            continue;
        if (const auto done = missions.kill(death.zone, death.appearanceFlags, death.boss)) {
            for (int32_t zx = 8 * done->first; zx < 8 * done->first + 8; zx++)
                for (int32_t zy = 8 * done->second; zy < 8 * done->second + 8; zy++)
                    if (const worldgen::Zone *zone = zones.zone(zx, zy))
                        npcs.setExtraSpawns(zx, zy, zone->spawns.size(), {}, false);
            rewardMission({done->first, done->second, missions.cell(done->first, done->second)->mission,
                           death.credited, death.fighter.controller});
        }
    }
    const std::set<std::pair<int32_t, int32_t>> missionCells = missions.takeChanged();
    {
        std::set<std::pair<int32_t, int32_t>> saved = missionCells;
        for (const auto &[i, j] : missions.takePassed())
            for (int32_t k = 0; k < 64; k++)
                saved.insert({8 * i + k / 8, 8 * j + k % 8});
        saveMissionCells(saved);
    }

    // Particles §5: this step's bursts, for the players near them.
    std::vector<creatures::Burst> bursts = frame.bursts;
    const auto blockBursts = [&](const std::vector<BlockEdit> &removed) { // particles §5.2
        for (const BlockEdit &e : removed) {
            const worldgen::Block b = zones.read(e.x, e.y, e.z);
            bursts.push_back(creatures::blockBurst({e.x, e.y, e.z}, {b.r, b.g, b.b}));
        }
    };
    // Block edits §8: a bomb that went off takes the natural ground within 8 blocks.
    for (const auto &blast : frame.explosions) {
        const auto removed = naturalSphere(blast.centre, 8, (int32_t)clockDay,
                                           [this](int32_t x, int32_t y, int32_t z) { return zones.read(x, y, z); });
        if (removed.empty())
            continue;
        blockBursts(removed);
        writeBlocks(removed);
        itemSounds.push_back({blast.centre, 1, 0.5f + 0.4f * (float)draw() / 32767.f, 1});
    }
    // Block edits §9: big creatures' blows take the natural ground away.
    for (const auto &impact : frame.craters) {
        const auto removed = naturalSphere(impact.centre, impact.radius, (int32_t)clockDay,
                                           [this](int32_t x, int32_t y, int32_t z) { return zones.read(x, y, z); });
        if (removed.empty())
            continue;
        blockBursts(removed);
        writeBlocks(removed);
        itemSounds.push_back({impact.centre, 2, 0.5f + 0.4f * (float)draw() / 32767.f, 1});
    }

    // Server §6.4: what the dead leave behind, sent in section 7 with their zones.
    for (const creatures::NpcFrame::Death &death : frame.deaths)
        if (!death.lootApplied) // retained non-hit death adapter
            dropLoot(lootVictim(death), items, (int32_t)clockDay, block, worldRandom);

    // Projectiles §4-§6: the NPCs' shots fly; hits on players go to their clients, hits on
    // NPCs come back in the next step.
    projectiles.list.insert(projectiles.list.end(), frame.shots.begin(), frame.shots.end());
    std::vector<creatures::ShotTarget> targets;
    for (size_t i = 0; i < fighters.size(); i++)
        targets.push_back({&fighters[i], &bodies[i]});
    for (creatures::Npc &n : npcs.npcs())
        targets.push_back({&n.fighter, &n.body});
    const auto source = [this, &fighters](uint64_t id) -> creatures::Combatant * {
        for (creatures::Npc &n : npcs.npcs())
            if (n.fighter.id == id)
                return &n.fighter;
        for (auto &fighter : fighters)
            if (fighter.id == id)
                return &fighter;
        return nullptr;
    };
    creatures::ShotEffects shot;
    projectiles.step((int32_t)dtMs, targets, source, block, [this] { return worldRandom.draw(); }, shot);
    bursts.insert(bursts.end(), shot.bursts.begin(), shot.bursts.end());
    for (const auto &s : shot.struck) {
        const worldgen::Block b = zones.read(s.cell.x, s.cell.y, s.cell.z);
        bursts.push_back(creatures::arrowBurst(s.at, {b.r, b.g, b.b}));
    }
    std::vector<creatures::HitRecord> outgoingHits = frame.emittedHits;
    for (const auto &emission : shot.hits) {
        const auto &record = emission.record;
        outgoingHits.push_back(record);
        if (!connections.count(record.target) && emission.applyLocally)
            shotHits.push_back(record);
    }
    for (const creatures::HitRecord &h : trapHits)
        if (connections.count(h.target))
            outgoingHits.push_back(h);
    for (size_t index = 0; index < fighters.size(); index++) {
        who[index]->buffs = std::move(fighters[index].buffs);
        who[index]->relations = std::move(fighters[index].relations);
        who[index]->linkedId = fighters[index].linkedId;
        who[index]->linkedItemSnapshot = fighters[index].linkedItem.retained;
        who[index]->creature.setLinkedItem(fighters[index].linkedItem.source);
    }

    // World items §3.5: changed zones are saved and listed to everyone.
    const std::set<std::pair<int32_t, int32_t>> changedZones = items.takeChanged();
    for (const auto &[zx, zy] : changedZones)
        saveZoneItems(zx, zy);
    // Interaction §4.5: changed statics are saved and sent in section 6.
    const std::set<std::pair<int32_t, int32_t>> changedStatics = statics.takeChanged();
    for (const auto &[zx, zy] : changedStatics)
        saveZoneItems(zx, zy);
    for (const auto &[zx, zy] : edits.takeChanged()) // block edits §3 step 4
        saveZoneItems(zx, zy);
    const auto objectsChanged = std::exchange(placedChanged, {});
    for (auto &[id, c] : connections) {
        WorldUpdate &out = c->outgoing;
        for (const auto &event : frame.relationships) {
            const auto bytes = event.serialize();
            out.sections[10].emplace_back(bytes.begin(), bytes.end());
        }
        for (const auto &event : frame.buffs) {
            const auto bytes = event.serialize();
            out.sections[11].emplace_back(bytes.begin(), bytes.end());
        }
        for (const auto &event : shot.buffs) {
            const auto bytes = event.serialize();
            out.sections[11].emplace_back(bytes.begin(), bytes.end());
        }
        for (const auto &s : frame.shots) { // section 5: every shot an NPC launched
            const auto bytes = s.serialize();
            out.sections[4].emplace_back(bytes.begin(), bytes.end());
        }
        for (const creatures::HitRecord &r : outgoingHits) {
            const Hit h = Hit::from(r);
            out.sections[1].emplace_back(h.bytes.begin(), h.bytes.end());
        }
        for (const Hit &h : healHits)
            out.sections[1].emplace_back(h.bytes.begin(), h.bytes.end());
        std::vector<creatures::SoundEvent> sounds = frame.sounds;
        sounds.insert(sounds.end(), shot.sounds.begin(), shot.sounds.end());
        sounds.insert(sounds.end(), itemSounds.begin(), itemSounds.end());
        for (const auto &g : gifts) // section 9: the gifts, for the named player's client
            out.sections[8].emplace_back(g.begin(), g.end());
        // Section 7: every changed zone's items, and those of the zones this player asked for.
        std::set<std::pair<int32_t, int32_t>> listed = changedZones;
        for (const auto &key : c->wantedZones)
            if (items.zone(key.first, key.second))
                listed.insert(key);
        statics.appendUpdates(zones, frame.staticStates, changedStatics, c->wantedZones, out.sections[5]);
        // Block edits §3, §10: this step's edits go to everyone; a requested zone's whole list
        // goes to whoever asked. Section 8: the changed placed-object lists.
        for (const BlockEdit &e : stepEdits) {
            const auto bytes = e.bytes();
            out.sections[0].emplace_back(bytes.begin(), bytes.end());
        }
        for (const auto &[zx, zy] : c->wantedZones)
            if (const auto *list = edits.zone(zx, zy))
                for (const BlockEdit &e : *list) {
                    const auto bytes = e.bytes();
                    out.sections[0].emplace_back(bytes.begin(), bytes.end());
                }
        for (const auto &key : objectsChanged) {
            WorldUpdate::KeyedList list;
            list.key = (uint64_t)(uint32_t)key.first | (uint64_t)(uint32_t)key.second << 32;
            for (const auto &entry : placedObjects[key]) {
                std::vector<uint8_t> bytes(16);
                std::memcpy(bytes.data(), entry.data(), 16);
                list.records.push_back(std::move(bytes));
            }
            out.keyed[1].push_back(std::move(list));
        }
        c->wantedZones.clear();
        // Missions §6.2: changed cells to everyone; a requested region's 64 to whoever asked.
        std::set<std::pair<int32_t, int32_t>> requestedCells;
        for (const auto &[rx, ry] : c->wantedRegions)
            if (world->region(rx, ry))
                for (int32_t k = 0; k < 64; k++)
                    requestedCells.insert({8 * rx + k / 8, 8 * ry + k % 8});
        c->wantedRegions.clear();
        missions.appendUpdates(frame.missions, missionCells, requestedCells, out.sections[12]);
        for (const auto &[zx, zy] : listed) {
            WorldUpdate::KeyedList list;
            list.key = (uint64_t)(uint32_t)zx | (uint64_t)(uint32_t)zy << 32;
            if (const auto *zoneItems = items.zone(zx, zy))
                for (const WorldItem &w : *zoneItems) {
                    const auto record = w.record();
                    list.records.emplace_back(record.begin(), record.end());
                }
            out.keyed[0].push_back(std::move(list));
        }
        const glm::dvec3 me =
            glm::dvec3(c->creature.position(0), c->creature.position(1), c->creature.position(2)) / 65536.0;
        for (const creatures::Burst &b : bursts) { // section 3, within 200 blocks
            const glm::dvec3 d = glm::dvec3(b.position[0], b.position[1], b.position[2]) / 65536.0 - me;
            if (glm::dot(d, d) <= 200.0 * 200.0) {
                const auto bytes = b.serialize();
                out.sections[2].emplace_back(bytes.begin(), bytes.end());
            }
        }
        if (c->alphaVulkan && questBooksChanged.count(id)) { // quests: this player's book
            c->chat.emplace_back(0, avPacket('Q', questBooks[id].save()));
            questBooksChanged.erase(id);
        }
        for (const creatures::SoundEvent &s : sounds) {
            ByteWriter w;
            w.f32((float)s.position.x), w.f32((float)s.position.y), w.f32((float)s.position.z);
            w.i32(s.sound), w.f32(s.pitch), w.f32(s.volume);
            out.sections[3].push_back(w.data());
        }
        for (const creatures::KillRecord &k : frame.kills) { // §6.2 step 7: section 10
            const auto bytes = k.serialize();
            out.sections[9].emplace_back(bytes.begin(), bytes.end());
        }
    }
    stepEdits.clear();
}


void Server::restoreZoneItems(worldgen::Zone &zone) {
    const int32_t zx = zone.zx, zy = zone.zy;
    SaveDb db;
    if (!db.open(savePath(options), SaveDb::Mode::ReadWrite))
        return;
    if (const auto bytes = db.get(zoneKey(zx, zy)))
        if (const auto saved = ZoneSave::parse(*bytes)) {
            items.restore(zx, zy, saved->items, (int32_t)clockDay);
            statics.restore(zx, zy, saved->staticStates);
            // Block edits §4.2: the last three days' edits, before the zone's light.
            const auto &kept = edits.restore(zx, zy, saved->blockEdits, (int32_t)clockDay);
            if (!kept.empty()) {
                const GapFill rock = [&](int32_t x, int32_t y, int32_t z) { return terrain->gapBlock(x, y, z, zone); };
                for (const BlockEdit &e : kept)
                    applySaved(zone, e, rock);
                dropFloating(zone);
                terrain->relight(zone);
            }
        }
    items.takeChanged(); // (what came from the save is already saved)
    statics.takeChanged();
}

void Server::readMissionRegion(int32_t rx, int32_t ry) {
    if (!missionRegionsRead.insert({rx, ry}).second)
        return;
    SaveDb db;
    if (!db.open(savePath(options), SaveDb::Mode::ReadWrite))
        return;
    for (int32_t k = 0; k < 64; k++) {
        const int32_t cx = 8 * rx + k / 8, cy = 8 * ry + k % 8;
        const std::string name = std::to_string(cx) + "_" + std::to_string(cy);
        const auto mission = db.get("mission" + name);
        const auto monster = db.get("monster" + name);
        missions.restore(cx, cy, mission ? MissionArea::parse(*mission) : std::nullopt,
                         monster ? MonsterArea::parse(*monster) : std::nullopt);
    }
}

void Server::saveMissionCells(const std::set<std::pair<int32_t, int32_t>> &cells) {
    if (cells.empty())
        return;
    std::error_code ec;
    std::filesystem::create_directories(options.folder / "Save", ec);
    SaveDb db;
    if (!db.open(savePath(options), SaveDb::Mode::ReadWrite))
        return;
    for (const auto &[cx, cy] : cells)
        if (const Missions::Cell *cell = missions.cell(cx, cy)) {
            const std::string name = std::to_string(cx) + "_" + std::to_string(cy);
            db.put("mission" + name, cell->mission.save());
            db.put("monster" + name, cell->monster.save());
        }
}

void Server::writeBlocks(const std::vector<BlockEdit> &list, bool recorded) {
    std::map<std::pair<int32_t, int32_t>, std::vector<const BlockEdit *>> byZone;
    for (const BlockEdit &e : list)
        if (const worldgen::Zone *zone = zones.zoneAt(e.x, e.y))
            byZone[{zone->zx, zone->zy}].push_back(&e);
    for (const auto &[key, records] : byZone) {
        auto zone = std::make_shared<worldgen::Zone>(*zones.zone(key.first, key.second));
        const GapFill rock = [&](int32_t x, int32_t y, int32_t z) { return terrain->gapBlock(x, y, z, *zone); };
        for (const BlockEdit *e : records)
            applyLive(*zone, *e, rock);
        dropFloating(*zone);
        terrain->relight(*zone);
        zones.add(std::move(zone));
    }
    if (!recorded)
        return;
    for (const BlockEdit &e : list) {
        stepEdits.push_back(e);
        if (zones.zoneAt(e.x, e.y))
            edits.add(e); // (a record whose zone is not loaded is sent but not stored)
    }
}

void Server::placeObject(const CreatureRecord &sender) {
    const int32_t number = (int8_t)sender.bytes[0x17d];
    const char *key = blueprintKey(number);
    if (!key)
        return;
    if (!blueprints.count(number)) {
        CubModel &model = blueprints[number];
        if (const auto prefab = options.prefabs ? options.prefabs(key) : nullptr)
            model.sx = prefab->sx, model.sy = prefab->sy, model.sz = prefab->sz, model.rgb = prefab->rgb;
    }
    const CubModel &model = blueprints[number];
    if (model.sx == 0)
        return; // (the install lacks the model)
    glm::vec3 aim;
    std::memcpy(&aim, &sender.bytes[0x150], 12);
    const int64_t position[3] = {sender.position(0), sender.position(1), sender.position(2)};
    const auto base = placementBase(position, aim);
    const auto blocks = placeBlueprint(model, base, (int8_t)sender.bytes[0x17c],
                                       [this](int32_t x, int32_t y, int32_t z) { return zones.read(x, y, z); });
    std::vector<BlockEdit> recorded, air;
    for (const BlueprintBlock &b : blocks)
        (b.recorded ? recorded : air)
            .push_back({b.x, b.y, b.z, b.block.r, b.block.g, b.block.b, b.block.typeByte, (int32_t)clockDay});
    writeBlocks(recorded);
    writeBlocks(air, false);
    // §6.4: the zone's placed-object list.
    if (const worldgen::Zone *zone = zones.zoneAt(base[0], base[1])) {
        placedObjects[{zone->zx, zone->zy}].push_back({number, base[0], base[1], base[2]});
        placedChanged.insert({zone->zx, zone->zy});
    }
}

void Server::saveZoneItems(int32_t zx, int32_t zy) {
    std::error_code ec;
    std::filesystem::create_directories(options.folder / "Save", ec);
    SaveDb db;
    const auto *list = items.zone(zx, zy);
    if (!list || !db.open(savePath(options), SaveDb::Mode::ReadWrite))
        return;
    ZoneSave zone;
    if (const auto bytes = db.get(zoneKey(zx, zy)))
        if (auto old = ZoneSave::parse(*bytes))
            zone = std::move(*old);
    zone.items.clear();
    for (const WorldItem &w : *list)
        zone.items.push_back(w.save());
    zone.staticStates = statics.stateBytes(zx, zy);
    zone.blockEdits = edits.saved(zx, zy);
    db.put(zoneKey(zx, zy), zone.serialize());
}

void Server::saveClock() {
    std::error_code ec;
    std::filesystem::create_directories(options.folder / "Save", ec);
    SaveDb db;
    if (db.open(savePath(options), SaveDb::Mode::ReadWrite))
        db.put("time", WorldClock{clockDay, clockMs}.serialize());
}

} // namespace net
