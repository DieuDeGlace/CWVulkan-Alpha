#pragma once
// What changes about a zone's static objects (docs/specs/alpha-interaction.md §4, §6): their
// state byte (open, closed, a chest with loot), the timer that animates a change, who sits
// or sleeps on them, and a chest's contents. Using one is packet 6 kind 3; the owner of the
// simulation (an offline client or the server) runs it.
#include "worldgen/Regions.h"
#include "worldgen/WorldMap.h"
#include "creatures/Combat.h"
#include "creatures/Inventory.h"
#include "creatures/Npcs.h"
#include "creatures/Physics.h"
#include "worldgen/Zone.h"
#include <array>
#include <functional>
#include <map>
#include <set>
#include <span>
#include <vector>

class WorldItems;

struct StaticState {
    uint8_t state = 1;     // +0x30: 1 closed, 0 open, 2 a closed chest with contents
    int32_t timerMs = 0;   // +0x34: ms since the last change
    uint64_t occupant = 0; // +0x40: who sits or sleeps on it
    std::vector<creatures::ItemRecord> contents; // +0x48: a chest's loot
    uint32_t stateWord = 0; // uninterpreted common-state word +0x38
};

// §6.3: how far a moving static is open (0 closed … 1 open) for the drawing of §4.4.
float openFraction(int32_t kind, const StaticState &s);

class StaticStates {
  public:
    using Key = std::pair<int32_t, int32_t>;
    // A zone's statics as generated, then the state bytes of its save (alpha-saves §5.2).
    void generated(const worldgen::Zone &zone);
    void restore(int32_t zx, int32_t zy, const std::vector<uint8_t> &states);
    void forget(int32_t zx, int32_t zy) { zones.erase({zx, zy}); }
    const std::vector<StaticState> *zone(int32_t zx, int32_t zy) const;
    std::vector<uint8_t> stateBytes(int32_t zx, int32_t zy) const;
    // Physics §4.5: whether a static object's box overlaps the box `lo`..`hi` (blocks). Traps
    // (6, 7) and levers (9) never block; doors, big doors, windows, gates and stomp traps (1,
    // 2, 3, 5, 8) only while closed.
    // With `axis` 0..2 and `move`, only statics the box moved toward on that axis count (their
    // centre on x and y, their base on z; open points 2 §2.2).
    bool blocks(const worldgen::WorldMap &map, glm::dvec3 lo, glm::dvec3 hi, int axis = -1, double move = 0,
                const creatures::IgnoredStatics *ignored = nullptr) const;
    // Path search §6.5: a creature pressing against closed doors and big doors (kinds 1, 2)
    // whose boxes overlap `lo`..`hi` opens them (state 0, timer 0).
    void openDoors(const worldgen::WorldMap &map, glm::dvec3 lo, glm::dvec3 hi);
    // Trails spec §4.4: the stools, benches and beach towels (kinds 16, 18, 68) in the 3 x 3
    // cells of 8 x 8 blocks round `block`, in cell order (x outer, y inner); and whether one
    // has nobody on it.
    // In a storey of a furnished house the house's lists count instead: seats from 06:00 to
    // 22:59, beds otherwise.
    creatures::Npcs::Seats seats(const worldgen::WorldMap &map, glm::ivec3 block, double timeOfDayMs) const;
    bool free(const creatures::Seat &seat) const;
    // Open points 2 §1.1: the statics listed in the 8 x 8-block index cells ix0..ix1 x iy0..iy1,
    // as the path search sees them.
    void searchStatics(const worldgen::WorldMap &map, int32_t ix0, int32_t ix1, int32_t iy0, int32_t iy1,
                       const std::function<void(const creatures::SearchStatic &)> &visit) const;
    // Packets §3.6: a static's 88-byte section-6 record, and applying one received.
    std::array<uint8_t, 88> record(const worldgen::Zone &zone, int32_t index) const;
    void apply(const uint8_t *record);
    // Cached coordinates select the zone; the first kind-five, nonzero-state
    // object with a matching retained association changes before loot runs.
    bool fatal(const worldgen::WorldMap &map, const creatures::BackingReference &backing, creatures::NpcFrame &frame);
    // Engine publication adapter: ordered immediate records, then final changed
    // or requested zone state. Suppress an identical final record already sent
    // immediately unless the peer explicitly requested that complete zone.
    void appendUpdates(const worldgen::WorldMap &map, std::span<const std::array<uint8_t, 88>> immediate,
                       const std::set<Key> &changedZones, const std::set<Key> &requestedZones,
                       std::vector<std::vector<uint8_t>> &output) const;
    // Zones whose states changed since the last call (to save and to send in section 6).
    std::set<Key> takeChanged() { return std::exchange(changed, {}); }

    // §6.1: creature `c` (with its body) uses static `index` of zone (zx, zy). Sounds go to
    // `sounds`; a chest's contents are dropped into `items`.
    void use(const worldgen::Zone &zone, int32_t index, creatures::Combatant &c, creatures::Body &body, float &yaw,
             WorldItems &items, int32_t day, const creatures::BlockType &block, const std::function<int()> &draw,
             std::vector<creatures::SoundEvent> &sounds);
    // §6.3 timers and §6.4 occupants: a seat is freed when its occupant has gone, moves, is
    // more than 4 blocks away or stops sitting or sleeping. `where(id)` gives a creature's
    // position, walking intent (+0x40) and action, or false when it no longer exists.
    void step(int32_t dtMs, const std::function<bool(uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &)> &where,
              const std::map<Key, const worldgen::Zone *> &held);

    // Traps (alpha-open-points-1.md §1), where the world is run (offline, the server), after
    // `step`: an extended trap retracts after 2 s; a retracted one extends after 2 s when a
    // player is within 30 blocks; an extended spike or unseen trap hurts every non-hostile
    // creature within 5 blocks each time the world clock passes 100 ms, a stomp trap once as
    // it slams. `creatures` are (id, position, controller) in the world's order; `level(at)`
    // is the level of the typed location a trap stands in (1 outside). Hits are kind-0
    // records with no attacker.
    struct Creature {
        uint64_t id = 0;
        glm::dvec3 at{0};
        uint8_t controller = 0;
    };
    void traps(int32_t dtMs, double clockMs, const std::vector<Creature> &creatures,
               const std::map<Key, const worldgen::Zone *> &held, const std::function<int32_t(glm::dvec3)> &level,
               std::vector<creatures::HitRecord> &hits, std::vector<creatures::SoundEvent> &sounds);

  private:
    // §6.3: the one state setter (the hook a mod would use for new usable kinds).
    void set(const worldgen::StaticObject &o, StaticState &s, uint8_t to, std::vector<creatures::SoundEvent> &sounds);
    std::map<Key, std::vector<StaticState>> zones;
    std::set<Key> changed;
};
