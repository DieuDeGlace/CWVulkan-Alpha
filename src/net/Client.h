#pragma once
// Joining an Alpha server, original or AlphaVulkan's (docs/specs/alpha-net-server.md §7.1):
// the handshake, then every 20 ms the player's own creature (all fields first, then what
// changed) and any chat line; meanwhile the creatures the server lists, the clock, the world
// updates and chat are taken in. The client is authoritative over its own creature: updates
// the server sends about it are ignored (packets §2.3).
#include "net/Protocol.h"
#include "net/Socket.h"
#include "creatures/Credits.h"
#include <atomic>
#include <array>
#include <deque>
#include <map>
#include <mutex>
#include <thread>

namespace net {

class Client {
  public:
    ~Client();

    // Connects and runs the handshake; on success `id()` and `seed()` are set and the threads
    // run. `error` says why not (including Alpha's version and "full" replies).
    bool connect(const std::string &host, uint16_t port, std::string *error = nullptr);
    void disconnect();
    bool connected() const { return online; }

    uint64_t id() const { return ownId; }
    uint32_t seed() const { std::lock_guard guard(lock); return worldSeed; }

    // Hold across frame mutations and queueing, publish setOwn at its end, then
    // release before rendering or disconnecting. Ordinary methods use a separate
    // lock, so callbacks may still queue messages while this guard is held.
    std::unique_lock<std::mutex> lockFrame() { return std::unique_lock<std::mutex>(frameGate); }

    // The local player's creature, sent every tick.
    void setOwn(const CreatureRecord &record);
    // Queues a chat line (one is sent per tick).
    void say(const std::string &text);
    // Queues an "event 40" record of ours, sent as packet 8 (packets §4.2; a buff ability's
    // record, behaviour §16.6).
    void sendEvent40(const std::array<uint8_t, 40> &record);
    // Queues a hit the local player dealt, sent as packet 7: the server applies it to an NPC or
    // relays it to the player hit (server simulation §6.1).
    void sendHit(const Hit &hit);
    // Queues a projectile the local player launched, sent as packet 9 (projectiles §1).
    void sendShoot(const std::array<uint8_t, 112> &record);
    // Queues an interaction request, sent as packet 6, one per tick (server simulation §8).
    void sendItemRequest(const std::array<uint8_t, 300> &record);
    // Asks for zones' data (packet 11; the server answers with their item lists).
    void requestZones(const std::vector<std::pair<int32_t, int32_t>> &zones);
    // Asks for regions' map-cell records (packet 12; answered in section 13).
    void requestRegions(const std::vector<std::pair<int32_t, int32_t>> &regions);

    // What the server last listed: other creatures by id.
    std::map<uint64_t, CreatureRecord> others() const;
    // Frame-side credit cleanup, under lockFrame. Later network deltas may
    // replace this field; unrelated received core bytes remain untouched.
    void setReceivedHealth(uint64_t key, float health);
    // The server's clock (packet 5), and whether one has arrived.
    bool clock(uint32_t &day, uint32_t &timeOfDayMs) const;
    // New packet publication only, so a frame does not replay a cached clock.
    std::optional<std::pair<uint32_t, uint32_t>> takeClock();
    // AlphaVulkan's own: the server's latest copy of this player's quest book, once per change
    // (only an AlphaVulkan server sends it).
    std::optional<std::vector<uint8_t>> takeQuestBook();
    // Chat lines received since the last call, as (sender id, text).
    std::vector<std::pair<uint64_t, std::string>> takeChat();
    // Ordered incoming hits; target application and source-based presentation
    // filtering belong to the frame consumer.
    std::vector<Hit> takeHits();
    // Ordered section-12 records; the caller applies them after world simulation.
    std::vector<::creatures::BuffEvent> takeBuffs();
    // Section 11; frame applies the drained queue after world calls, before input.
    std::vector<::creatures::RelationshipRecord> takeRelationships();
    // Complete ordered section-ten records; identity/award admission belongs to
    // the frame consumer, including records addressed to other actors.
    std::vector<::creatures::KillRecord> takeCredits();
    // Sounds the server relayed (world-update section 4, behaviour spec §6).
    struct Sound {
        glm::vec3 position;
        int32_t id;
        float pitch, volume;
    };
    std::vector<Sound> takeSounds();
    // Particle bursts relayed in world-update section 3 (particles §1, §9), 72 bytes each.
    std::vector<std::array<uint8_t, 72>> takeBursts();
    // Shoot records relayed in world-update section 5 since the last call (projectiles §3).
    std::vector<std::array<uint8_t, 112>> takeShoots();
    // Zone item lists received in section 7 since the last call: (zone x, zone y) and its
    // 328-byte records.
    using ZoneItems = std::pair<std::pair<int32_t, int32_t>, std::vector<std::array<uint8_t, 328>>>;
    std::vector<ZoneItems> takeZoneItems();
    // Inventory grants to the local player in section 9 (pick-ups and rewards).
    std::vector<std::array<uint8_t, 280>> takeGifts();
    // Static-object states received in section 6 (88-byte records, interaction §4.5).
    std::vector<std::array<uint8_t, 88>> takeStaticStates();
    // Block edits received in section 1 (20-byte records, block edits §2), in order.
    std::vector<std::array<uint8_t, 20>> takeBlockEdits();
    // Map-cell mission records received in section 13 (56 bytes, missions §6.1), in order.
    std::vector<std::array<uint8_t, 56>> takeMissionRecords();

  private:
    void receiveLoop();
    void sendLoop();

    Socket socket;
    std::atomic<bool> online{false};
    uint64_t ownId = 1; // native initial identity, retained when an adoption is bypassed
    uint32_t worldSeed = 0;
    std::thread receiver, sender;

    std::mutex frameGate;
    mutable std::mutex lock;
    CreatureRecord own, ownSent;
    bool sentOnce = false;
    std::deque<std::string> outgoingChat;
    std::vector<std::array<uint8_t, 40>> outgoingEvents;
    std::vector<Hit> outgoingHits;
    std::vector<std::array<uint8_t, 112>> outgoingShoots, shoots;
    std::deque<std::array<uint8_t, 300>> outgoingItems;
    std::vector<std::pair<int32_t, int32_t>> outgoingZones, outgoingRegions;
    std::vector<std::array<uint8_t, 56>> missionRecords;
    std::vector<ZoneItems> zoneItems;
    std::vector<std::array<uint8_t, 280>> gifts;
    std::vector<std::array<uint8_t, 88>> staticStates;
    std::vector<std::array<uint8_t, 20>> blockEdits;
    std::map<uint64_t, CreatureRecord> creatures;
    bool haveClock = false;
    uint32_t clockDay = 0, clockMs = 0;
    std::optional<std::pair<uint32_t, uint32_t>> pendingClock;
    std::vector<std::pair<uint64_t, std::string>> chat;
    std::optional<std::vector<uint8_t>> questBook;
    std::vector<Hit> hits;
    std::vector<::creatures::BuffEvent> buffEvents;
    std::vector<::creatures::RelationshipRecord> relationshipEvents;
    std::vector<::creatures::KillRecord> credits;
    std::vector<Sound> sounds;
    std::vector<std::array<uint8_t, 72>> bursts;
};

} // namespace net
