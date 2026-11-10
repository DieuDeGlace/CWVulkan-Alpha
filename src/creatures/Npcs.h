#pragma once
// The creatures of a world (docs/specs/alpha-creatures-behaviour.md §9, §13; on the server
// alpha-server-simulation.md §5, §6): the spawn objects of the zones round the players come
// alive when a player is near, run the behaviour sequence (Combat, LookAtPlayer,
// RandomWalk), fight, die, and come back after 20 minutes or on the next day. Offline the
// client runs them for its one player; online the server runs the same step for all.
#include "creatures/Appearance.h"
#include "creatures/ActorMap.h"
#include "creatures/Combat.h"
#include "creatures/Credits.h"
#include "creatures/Inventory.h"
#include "creatures/Paths.h"
#include "creatures/Projectiles.h"
#include "creatures/Physics.h"
#include "creatures/Pose.h"
#include "worldgen/Random.h"
#include "worldgen/WorldMap.h"
#include "Missions.h"
#include <functional>
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <tuple>
#include <vector>

namespace creatures {

// RandomInteraction (alpha-trails-poses-schedules.md §4): a static a creature may sit or lie
// on, by zone and index in the zone's static list, and its position (blocks).
struct Seat {
    int32_t zx = 0, zy = 0, index = -1;
    glm::dvec3 at{0};
};

struct Npc {
    Combatant fighter;
    Body body;
    Appearance looks;
    PoseState pose;
    float yaw = 0; // degrees, facing(ψ) = (−sin ψ, cos ψ)
    glm::dvec3 home{0};
    std::pair<int32_t, int32_t> zone{0, 0};
    static constexpr size_t NO_SPAWN = (size_t)-1;
    size_t spawn = 0; // index in the zone's spawn list (NO_SPAWN for bombs and pets)
    float despawnRadius = 200; // the spawn's radius (server §5.4)
    float deadMs = 0;          // time since death; the body goes after 1000 ms (server §6.2)
    bool fatalPresented = false; // engine bookkeeping: the world applier already emitted the death sound
    bool fatalLootApplied = false; // the synchronous world loot service already ran
    bool fatalRegionApplied = false; // regional service already admitted or rejected this invocation
    // Engine bridge to the remaining later-corpse adapter, not native storage.
    // Preserve the pre-region binary32 amount until recipient record conversion.
    std::optional<float> fatalCredit;
    int32_t talkKind = 0; // record field 42 (talk §2.4: 3 pet food, 4 a place to have explored)
    glm::ivec2 talkTarget{0}; // record field 41: the place talk kind 4 speaks of (read as a zone)
    std::vector<ItemRecord> inventory; // the spawn's bags (a vendor's stock, talk §3.2), one per unit
    uint8_t boss = 0; // raw descriptor +0x10e8 byte; the regional gate tests nonzero
    // §9.2 step 6: set by onActivate for a mission creature, whose gear is made again at its
    // mission level (activation mode) instead of taken from its spawn.
    bool regear = false;
    // RandomInteraction §4.2: its timer and the seat it is bound for (index -1: none).
    float interactMs = 0;
    Seat seat;
    // AlphaVulkan's quests (docs/design/quests.md): the quest it belongs to, its role there
    // (quests::Role: 1 enemy, 2 prisoner, 3 ally), and for a freed prisoner the time left
    // before it is gone (ms; negative: staying).
    uint32_t quest = 0;
    uint8_t questRole = 0;
    float leaveMs = -1;
    // Pets (pets and bombs Part B): the owner (record field 35), the bites of a feeding and
    // the taming progress it carries for 5 s (the kind-7 buff), and who fed it.
    int32_t bites = 0;
    float tameProgress = 0, tameMs = 0;
    uint64_t tamer = 0;

    // §13: behaviour state.
    float thinkMs = 0;         // accumulated until a decision (every ≥ 32 ms)
    float specialMs = 20000;   // special timer
    float chaseMs = 0, cooldownMs = 0, walkMs = 0;
    PathSearch path; // path search §4: kept between updates
    // Wildlife §4: the behaviour sequence (as the spawn's), the route and where on it, the
    // wait at a point, and the creature a member follows.
    uint8_t behaviour = 0;
    std::vector<glm::dvec3> route;
    size_t routeIndex = 0;
    float waitMs = 0;
    uint64_t follows = 0;
    // Behaviour §13.5: a townsperson's day (place, time of day) and the stop it is bound for.
    std::vector<std::pair<glm::dvec3, int32_t>> schedule;
    int32_t scheduleStop = 0, scheduleArrived = 0; // schedules §3.1: current and arrived entries
    float routeRadius = 10; // WalkPath's arrival radius (2 for a shopkeeper's counter)
    uint64_t target = 0;       // the creature with the highest threat
    float threat = 0;
};

// A player as the creatures see it.
struct PlayerView {
    Combatant &fighter;
    Body &body;
    float yaw = 0;          // degrees
    int32_t actionMsBefore = 0; // the player's action time before this frame (to find its hit moment)
    // Whether the step works out this player's own attacks (offline, the local player). On
    // the server players' attacks arrive as hits instead (server §1, the owner rule).
    bool strikes = true;
    bool advanceBuffs = false; // server view; local clients already ran their own countdown
    bool local = false; // distinguished controlled creature; absent in a dedicated world
};

// What a step of creatures did.
struct NpcFrame {
    std::vector<HitRecord> emittedHits; // producer output; received NPC hits are not automatically echoed
    std::vector<HitEmission> playerHits; // local policy is not sent over the wire
    std::vector<SoundEvent> sounds;
    std::vector<BuffEvent> buffs;
    std::vector<RelationshipRecord> relationships;
    std::vector<std::array<uint8_t, 88>> staticStates; // immediate section-six snapshots, in production order
    std::vector<KillRecord> kills;
    std::vector<Projectile> shots; // projectiles the NPCs launched (projectiles §2)
    std::vector<Burst> bursts;     // particles §5 (section 3)
    // Server §6.2 step 3: the creatures that died this step, for their loot.
    struct Death {
        Combatant fighter;
        uint16_t appearanceFlags = 0;
        int64_t position[3] = {0, 0, 0};
        std::vector<ItemRecord> inventory;
        std::pair<int32_t, int32_t> zone{0, 0}; // its home zone
        uint8_t boss = 0;
        uint64_t credited = 0; // missions §5.1: the first player in its threat map, or 0
        uint32_t quest = 0;    // a quest creature's quest and role (docs/design/quests.md)
        uint8_t questRole = 0;
        bool lootApplied = false; // caller must not replay an already completed world loot call
        bool regionApplied = false; // caller must not replay regional admission/progress
    };
    std::vector<Death> deaths;
    std::vector<MissionProgress> missions; // immediate snapshots with engine reward metadata
    // Block edits §9: the blows of creatures wider than 4 blocks, which dig craters where the
    // world is not online (centre in blocks).
    struct Impact {
        glm::dvec3 centre{0};
        int32_t radius = 0;
    };
    std::vector<Impact> craters;
    // Block edits §8: bombs that went off this step (their first dead step).
    struct Explosion {
        glm::dvec3 centre{0}; // blocks
        uint64_t bomb = 0;
        int32_t level = 1;
    };
    std::vector<Explosion> explosions;
    // Pets §7.3: a creature tamed this step (the owner's client turns the food into the pet
    // item, §7.4).
    struct Tamed {
        uint64_t creature = 0, owner = 0;
        int32_t type = 0, level = 1, experience = 0;
    };
    std::vector<Tamed> tamed;
    // Pets §12: a pet's new level and experience, for the owner's Pet slot item.
    struct PetItem {
        uint64_t owner = 0;
        int32_t level = 1, experience = 0;
    };
    std::vector<PetItem> petItems;
    int32_t experience = 0;            // the kills' experience for the first player (offline)
    // Hits dealt or taken, with where the target stood (for the combat text).
    std::vector<std::pair<HitRecord, glm::dvec3>> seen;

    // Append-only engine outputs can survive every world piece in a frame.
    // Consume these views immediately, before another append can reallocate.
    // A cursor belongs to one aggregate until reset; recreate it after reset.
    struct Cursor {
        size_t emittedHits = 0, playerHits = 0, sounds = 0, buffs = 0, kills = 0, shots = 0, bursts = 0;
        size_t deaths = 0, craters = 0, explosions = 0, tamed = 0, petItems = 0, seen = 0;
        size_t relationships = 0;
        size_t staticStates = 0;
        size_t missions = 0;
        int32_t experience = 0;
    };
    struct Added {
        std::span<const HitRecord> emittedHits;
        std::span<const HitEmission> playerHits;
        std::span<const SoundEvent> sounds;
        std::span<const BuffEvent> buffs;
        std::span<const KillRecord> kills;
        std::span<const Projectile> shots;
        std::span<const Burst> bursts;
        std::span<const Death> deaths;
        std::span<const Impact> craters;
        std::span<const Explosion> explosions;
        std::span<const Tamed> tamed;
        std::span<const PetItem> petItems;
        int32_t experience = 0;
        std::span<const std::pair<HitRecord, glm::dvec3>> seen;
        std::span<const RelationshipRecord> relationships;
        std::span<const std::array<uint8_t, 88>> staticStates;
        std::span<const MissionProgress> missions;
    };
    Added addedSince(Cursor &cursor) const;
    void reset(); // clear values while retaining engine buffer capacity
};

class Npcs {
  public:
    Npcs() = default; // standalone tools retain their explicit private generator
    explicit Npcs(worldgen::Random &random) : sharedRandom(&random) {}
    // One step: incoming hits (server §3 step 3), activation and respawn (§9.2), despawn
    // (server §5.4), behaviour (§13), actions, physics, hits, deaths.
    NpcFrame step(float dtMs, const worldgen::WorldMap &map, std::vector<PlayerView> players, double timeOfDayMs,
                  int32_t day, const BlockType &block, const std::vector<HitRecord> &incoming = {}, NpcFrame frame = {},
                  const std::vector<BuffEvent> &buffInput = {});
    void stepInto(float dtMs, const worldgen::WorldMap &map, std::vector<PlayerView> players, double timeOfDayMs,
                  int32_t day, const BlockType &block, NpcFrame &frame, const std::vector<HitRecord> &incoming = {},
                  const std::vector<BuffEvent> &buffInput = {},
                  const std::function<void(int32_t)> &advanceControlled = {});
    // Local player dispatch belongs between beginActionUpdate/endActionUpdate,
    // before movement. Carry its events into step and disable later strikes.
    void dispatchPlayerActions(std::span<PlayerView> players, int32_t dtMs, const BlockType &block, NpcFrame &frame);
    // Called only by authoritative resident countdown. Resolve the buff source,
    // emit and apply each target event before the producer continues. Payload
    // writes may deplete buffs, but this path never inserts/removes buff nodes.
    // The optional engine SDK callback adjusts the local application copy only.
    void periodicHit(Combatant &target, Body &body, const Buff &buff, std::span<PlayerView> players, NpcFrame &frame,
                     const std::function<void(HitRecord &)> &localHit = {});
    // Resolve a world emission again at its application boundary. Controlled
    // targets apply synchronously; worlds without a controlled actor relay
    // role-zero targets. The producer owns its original outgoing/seen record.
    // Full contact propagation and fatal world effects remain separate work.
    void applyWorldHit(const HitRecord &hit, std::span<PlayerView> players, NpcFrame &frame,
                       const std::function<void(HitRecord &)> &localHit = {});
    std::function<void(HitRecord &)> onControlledHit; // SDK extension: local application copy only
    // Cached-origin object work runs after first presentation/clock reset,
    // before loot. This service may append snapshots but must retain actor storage.
    std::function<void(const Combatant &, NpcFrame &)> onFatalObject;
    // World-owned service, called synchronously after first fatal presentation.
    // It may update drop storage, but must not structurally mutate the actor table.
    std::function<void(const Combatant &, const Body &, std::span<const ItemRecord>)> onFatalLoot;
    // After linked-health stores: append progress before completion-cell work.
    // The supplied service must retain actor storage throughout this call.
    std::function<void(Combatant &, uint8_t, const std::function<Combatant *(uint64_t)> &, NpcFrame &)> onFatalRegion;
    // Existing engine descriptor-tail adapter; retains live actors. Native
    // virtual destruction and descriptor lifetime remain separate contracts.
    void trimMissionSpawns(const Combatant &target, const worldgen::WorldMap &map);
    // Server §5.3: a spawn's creature id, (zy * 65536 + zx) * 256 + index.
    static uint64_t spawnId(int32_t zx, int32_t zy, size_t index) {
        return (((uint64_t)(uint32_t)zy << 16) + (uint32_t)zx) * 256 + index;
    }
    // Server §8 kind 2, talk §2.7: a night at the inn removes every NPC and resets the spawns'
    // countdowns, so they come back from their spawns.
    void restAll() {
        list.clear();
        actors.clear();
        spawns.clear();
    }
    // Missions §3: a zone's extra spawns, after the `base` ones its generator made. The old
    // extras' creatures are removed when `removeCreatures` (the zone pass); a completed
    // mission only drops the spawns (§3.4).
    void setExtraSpawns(int32_t zx, int32_t zy, size_t base, std::vector<worldgen::Spawn> extra,
                        bool removeCreatures = true);
    // Quests (docs/design/quests.md): a quest's creatures, activated near players like a
    // zone's own and never made again once dead or gone; and dropping a quest's creatures.
    void setQuestCreatures(uint32_t quest, std::vector<worldgen::Spawn> creatures);
    void removeQuest(uint32_t quest);
    bool hasQuestCreatures(uint32_t quest) const { return questSpawns.count(quest) != 0; }
    // Frees a quest's prisoner: it stands up, says nothing more and walks toward `home` for a
    // minute, then is gone. Returns its quest, or 0 when `id` is no captive.
    uint32_t free(uint64_t id, glm::dvec3 home);
    // Whether a living quest enemy of `quest` stands within `radius` blocks of `at`.
    bool guarded(uint32_t quest, glm::dvec3 at, double radius) const;
    static uint64_t questCreatureId(uint32_t quest, size_t index) {
        return 0x5100000000000000ull | (uint64_t)quest << 12 | (uint64_t)(index & 0xfff);
    }
    // Missions §2.5 step 1: every mission target (flag 0x2000) whose home zone is in `zones`.
    void removeTargets(const std::set<std::pair<int32_t, int32_t>> &zones);
    // Called for each creature as it comes alive, before its equipment is made (behaviour
    // §9.2 step 6: a dungeon's creatures take the mission level).
    std::function<void(Npc &)> onActivate;
    // A seed affects the supplied world stream too; no caller-side restoration.
    void reseed(uint32_t s) { random().seed(s); }
    // Engine admission adapter for the nonempty world-context maintenance gate.
    void setNamedWorld(bool named) { namedWorld = named; }
    // Physics §4.5: the static objects creatures collide with (left empty: none).
    ObjectTest objects;
    // Path search §6.5: opens the closed doors overlapping a box (left empty: none).
    std::function<void(glm::dvec3 lo, glm::dvec3 hi)> openDoors;
    // RandomInteraction §4.4: the seats round a block (in a house, its lists; outdoors, the
    // stools, benches and beach towels of the 3 x 3 cells of 8 blocks round it); whether one is
    // free; and sitting on one (packet 6 kind 3, interaction §6.1). Left empty, nobody sits.
    struct Seats {
        bool house = false;     // the creature stands in a storey of a house
        std::vector<Seat> keep; // a target among these is kept (the house's seats and beds)
        std::vector<Seat> pick; // the candidates (the house's seats by day, beds by night)
    };
    // Open points 2 §1: the statics the path search sees (left empty: none).
    StaticLookup pathStatics;
    std::function<Seats(glm::ivec3 block, double timeOfDayMs)> seats;
    std::function<bool(const Seat &)> seatFree;
    std::function<void(Npc &, const Seat &)> sit;
    // Physics §3.3: whether a hostile creature is in contact range of `fighter` (its lunge stops).
    bool hostileInContact(const Combatant &fighter, const Body &body) const;
    // Physics §4.4: the push the creatures of kinds 1 and 6 give a player's body.
    glm::vec3 pushOnPlayer(const Body &player) const;
    // Pets and bombs §3: a bomb (type 144, controller 6) at the placing player's position
    // (`at`, blocks, the box centre), of the bomb item's level, with the new-creature health of
    // 500. Each step its health is held to 100 P(L) and burns by 0.025 per millisecond; it
    // goes off on its first dead step (§4). Its id is one below the smallest in use. Returns it.
    uint64_t placeBomb(glm::dvec3 at, int32_t level);
    // Pets §13 (packet 6 kind 8, the T key): the owner's pet stops fighting and, more than
    // 200 blocks away, comes to `at`.
    void callPet(Combatant &owner, glm::dvec3 at);
    // The owner's pet, or null.
    const Npc *petOf(const Combatant &owner) const;
    const std::deque<Npc> &all() const { return list; }
    std::deque<Npc> &npcs() { return list; } // pose/test access; structural edits belong outside an actor pass

  private:
    struct SpawnState {
        float countdownMs = 0;
        int32_t deathDay = -1;
    };
    // §9.2 step 7: one creature of spawn `s` (zone `key`, index `index` among `base` zone
    // spawns) at `p`, added to the list.
    void spawnCreature(const worldgen::Spawn &s, std::pair<int32_t, int32_t> key, size_t index, size_t base, uint64_t id,
                       glm::dvec3 p, bool descriptorBacked = true);
    void activate(const worldgen::WorldMap &map, const std::vector<PlayerView> &players, double timeOfDayMs, int32_t day,
                  float dtMs);
    void think(Npc &npc, float dtMs, std::vector<PlayerView> &players, const BlockType &block);
    bool combat(Npc &npc, float dtMs, std::vector<PlayerView> &players, const BlockType &block);
    bool lookAtPlayer(Npc &npc, std::vector<PlayerView> &players);
    void randomWalk(Npc &npc, float dtMs);
    // §5: `attacker` hits what is in reach at its hit moment: NPCs when a player strikes,
    // players when an NPC does.
    // One blow of `attacker` (at `from`) on `target`: the hit, its sounds and bursts, and what the
    // attacker gains. True when it landed.
    bool landBlow(Combatant &attacker, const Body &attackerBody, Combatant &target, Body &body, NpcFrame &frame,
                  HitInput in, std::span<PlayerView> players);
    // Open points 1 §3: a beam (actions 28, 95) every 100 ms after its wind-up marches a ray
    // block by block to the first solid block (200 at most) and hits the hostile creatures in
    // it; when the ray met nobody, five fire sparks at its end and the combo goes.
    void beamTick(Combatant &attacker, const Body &attackerBody, float beforeMs, float dtMs, std::span<PlayerView> players,
                  NpcFrame &frame, const BlockType &block);
    // An NPC's swing also lands on `npcTarget` when either side is a quest ally.
    void strike(Combatant &attacker, const Body &attackerBody, float yaw, std::span<PlayerView> players,
                bool byPlayer, NpcFrame &frame, const BlockType &block, uint64_t npcTarget = 0, bool attackerAlly = false);
    // Server §6.1 step 3: a hit landing on an NPC (threat, then damage).
    // Pets and bombs §4.2: the blast of `bomb` hits every live creature in reach.
    void blast(Npc &bomb, std::vector<PlayerView> &players, NpcFrame &frame);
    // Pets (Pets.cpp).
    struct PetLink {
        int32_t skill = -1;        // copy of the taming skill (obj +0x1d44)
        std::vector<uint8_t> item; // copy of the Pet slot item (obj +0x1d48)
    };
    std::map<uint64_t, PetLink> pets; // by owner
    Npc *findNpc(uint64_t id);
    uint64_t freeId() const;
    void makePet(Npc &pet, const Item &item, const Combatant &owner);
    void petPass(std::vector<PlayerView> &players, NpcFrame &frame);
    uint64_t petTarget(const Npc &pet, std::vector<PlayerView> &players);
    // Pets §10.2, wildlife §4.7: follow creature `followed`; false when not handled.
    bool companion(Npc &n, uint64_t followed, std::vector<PlayerView> &players, float dtMs, const BlockType &block);
    // Trails spec §4: sit a while on a free seat in sight; false when not handled.
    bool randomInteraction(Npc &n, float dtMs, const BlockType &block);
    // Behaviour §13.5, path search §3: walk to the place of the day's current stop; false
    // when not handled.
    bool spawnLocation(Npc &n, const BlockType &block);
    double clockMs = 0; // the time of day of this step
    // Wildlife §4.6: walk the route point to point; false when not handled.
    bool walkPath(Npc &n, float dtMs, const BlockType &block);
    void bite(Npc &n, std::vector<PlayerView> &players, NpcFrame &frame);
    void sharePetExperience(Combatant &owner, int32_t experience, NpcFrame &frame);
    int draw(); // rand(): 0..32767
    worldgen::Random &random() { return sharedRandom ? *sharedRandom : privateRandom; }

    // Appending during the live pass preserves references to existing creatures.
    // Bulk removals occur before/after it, followed by an index refresh next call.
    std::deque<Npc> list;
    struct ActorEntry {
        Npc *npc = nullptr;
        PlayerView *player = nullptr;
        Combatant *combat() const { return npc ? &npc->fighter : player ? &player->fighter : nullptr; }
        Body *body() const { return npc ? &npc->body : player ? &player->body : nullptr; }
        bool local() const { return player && player->local; }
    };
    ActorMap<ActorEntry> actors;
    Combatant *controlledActor = nullptr; // valid while the refreshed index is protected
    bool actorPassActive = false;
    bool actorLookupActive = false;
    bool namedWorld = false;
    void appendNpc(Npc npc);
    void refreshActors(std::span<PlayerView> players);
    ActorEntry lookupActor(uint64_t key, std::span<PlayerView> players);
    void propagateThreat(Combatant &source, const ActorEntry &target, NpcFrame &frame);
    bool protectionGranted(const Combatant &target, std::span<PlayerView> players);
    std::map<std::tuple<int32_t, int32_t, size_t>, SpawnState> spawns;
    std::map<std::pair<int32_t, int32_t>, std::pair<size_t, std::vector<worldgen::Spawn>>> extras; // base, spawns
    std::map<uint32_t, std::vector<worldgen::Spawn>> questSpawns; // quest -> its creatures
    std::set<uint64_t> questGone; // quest creatures dead or gone (not made again)
    worldgen::Random privateRandom{12345};
    worldgen::Random *sharedRandom = nullptr; // owner must outlive this actor world
    float leftoverMs = 0;
};

// §9.3: the weapons and armour a spawned creature carries. `generating`: the zone generator's
// mode 0 (spawn objects spec §4.2), which also sets the per-type stat multipliers and specials,
// a boss's extras (its +0x10b8 choice into `bossChoice`) and the levels of both hands;
// otherwise the activation mode.
void equip(Combatant &c, const Appearance &looks, const std::function<int()> &draw, bool generating = false,
           int32_t *bossChoice = nullptr);

// Spawn objects spec §3.2 steps 2-7, for worldgen::Terrain::finishSpawn: the appearance and
// mode-0 equipment of a freshly generated spawn, a boss's spirit, the rarities and level
// re-rolls of its slots and the discarded draw, all from the zone's stream.
// `whole` false: steps 2 and 3 only (location spec §4.3.2, a ring creature made up inline).
void finishSpawn(worldgen::Spawn &s, worldgen::Random &random, bool whole = true);

// §13.2 step 7: whether `a` would fight `b` (0x596ca0).
bool hostile(const Combatant &a, const Combatant &b);

// §5 with the attack range of §13.2: whether an attack from `from`, facing `yaw` degrees, by a
// creature of collision box `box` reaches `target` (in front, within reach; inferred for the
// player).

// Travel §5.1: the creature types that can be ridden as pets.
bool rideable(int32_t type);

// §13.2: animals that never start a fight (0x444680).
bool harmless(int32_t type, uint16_t appearanceFlags);

} // namespace creatures
