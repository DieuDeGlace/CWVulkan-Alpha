#pragma once
// Missions and the map cells' monsters (docs/specs/alpha-missions.md): the mission and monster
// areas of each map cell (§1), the daily pass that hands out a region's missions (§2), the
// extra spawns each zone gets from it, a cell's big monster or a wandering group (§3),
// discovery (§4), kills, completion and rewards (§5), the 56-byte section-13 record (§6) and
// the `mission` and `monster` save values (§7).
//
// A map cell is 8 x 8 zones (2048 blocks); cell (cx, cy) uses location record
// 8 (cx mod 8) + (cy mod 8) of region (cx / 8, cy / 8).
#include "worldgen/Random.h"
#include "worldgen/Regions.h"
#include "worldgen/Zone.h"
#include "creatures/DescriptorRefresh.h"
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

// §1.1: the mission area of a location record.
struct MissionArea {
    uint32_t unused = 0;      // +0x2c, never written
    uint32_t seed = 0;        // one draw, names the target
    int32_t kind = 0;         // 0 none; §1.3
    int32_t targetType = 0;   // the creature type to defeat or named
    int32_t level = 0;        // the strongest player's level when made
    uint8_t tier = 0;         // the location's sub-level
    uint8_t state = 0;        // 0 new, 1 discovered, 2 complete
    int32_t count = 0, required = 0;
    int32_t targetZoneX = -1, targetZoneY = -1;

    // §6.1: the section-13 record of cell (cx, cy).
    std::array<uint8_t, 56> record(int32_t cx, int32_t cy) const;
    static MissionArea fromRecord(const uint8_t *bytes, int32_t &cx, int32_t &cy);
    // §7: the `mission<cx>_<cy>` value (38 bytes).
    std::vector<uint8_t> save() const;
    static std::optional<MissionArea> parse(const std::vector<uint8_t> &value);
    bool operator==(const MissionArea &o) const { return record(0, 0) == o.record(0, 0); }
};

// §1.2: the monster area.
struct MonsterArea {
    int32_t type = 0; // 0: no monster
    int32_t level = 0;
    uint8_t tier = 0;
    int32_t zoneX = 0, zoneY = 0;
    // §7: the `monster<cx>_<cy>` value (21 bytes).
    std::vector<uint8_t> save() const;
    static std::optional<MonsterArea> parse(const std::vector<uint8_t> &value);
};

// §1.3: the dictionary key of a kind's objective text (`objective:<key>`), or null.
const char *objectiveKey(int32_t kind);
// §2.3 step 4: the big monster for a level in the climate of region cell C (troll when there
// is no cell, without a draw).
int32_t monsterForLevel(const worldgen::RegionCell *C, int32_t level, worldgen::Random &random);
// §2.5 step 5: the nine-entry list of mission creatures.
int32_t missionCreature(worldgen::Random &random);
// §5.3: the category-13 Platinum Coins quantity for a mission's level and tier.
int32_t rewardPlatinumAmount(int32_t level, int32_t tier);
// Generate only the second grant, after delivery of the first grant completes.
worldgen::ItemRecord missionRewardGear(const MissionArea &mission, int32_t characterClass, worldgen::Random &random);

// §5.3: a completed mission's rewards for a player of class `characterClass`: Platinum
// Coins, then (one draw) armour when even or a weapon when odd, at the mission level with the
// tier as rarity.
std::vector<worldgen::ItemRecord> missionRewards(const MissionArea &mission, int32_t characterClass,
                                                 worldgen::Random &random);

// The map cell of a zone, and the cell's location record (null when its region is not ready).
inline std::pair<int32_t, int32_t> cellOfZone(int32_t zx, int32_t zy) {
    return {worldgen::floorDiv(zx, 8), worldgen::floorDiv(zy, 8)};
}
const worldgen::Location *cellLocation(const worldgen::World &world, int32_t cx, int32_t cy);
// bump(L, p) = max(1 − f(L, p), 0)² at a point in blocks.
float locationBump(const worldgen::World &world, const worldgen::Location &L, double x, double y);

namespace creatures {
struct Npc;
struct Combatant;
class Npcs;
} // namespace creatures

// Engine metadata beside the immediate native-format regional snapshot. The
// contributor and victim role support the retained later reward adapter.
struct MissionProgress {
    int32_t x = 0, y = 0;
    MissionArea mission;
    uint64_t contributor = 0;
    uint8_t victimRole = 0;
    bool rewardsApplied = false; // engine metadata: the synchronous recipient sequence owns completion grants
    bool completed() const { return mission.state == 2; }
    std::array<uint8_t, 56> record() const { return mission.record(x, y); }
};

// Re-read cached coordinates at row/continuation boundaries. The supplied cell
// service owns descriptor disposal; this does not certify its native lifetime.
void visitMissionCells(const creatures::Combatant &target, const std::function<void(int32_t, int32_t)> &visit);

class Missions {
  public:
    using Key = std::pair<int32_t, int32_t>;
    struct Cell {
        MissionArea mission;
        MonsterArea monster;
    };
    // A player as the missions see one.
    struct Player {
        glm::dvec3 position{0}; // blocks
        int32_t level = 1;
    };

    // §2.5 step 5, kind 4: the wildlife picker (wildlife spec §2) at a column and height, mode
    // 0; the game sets it from its terrain.
    std::function<int32_t(int32_t x, int32_t y, int32_t h, worldgen::Random &)> wildlife;

    // §2.1: whether region (rx, ry)'s pass has run today; midnight and inn rests clear all.
    bool generated(int32_t rx, int32_t ry) const { return done.count({rx, ry}) != 0; }
    void clearGenerated() { done.clear(); }
    // §2.3 - §2.6 (without the zone pass, which the caller runs for the loaded zones): the
    // day's monsters and missions of region (rx, ry). Every cell whose mission changed is
    // listed for section 13 (takeChanged).
    void dailyPass(const worldgen::World &world, int32_t rx, int32_t ry, const std::vector<Player> &players,
                   worldgen::Random &random);
    // §3.1, §3.2: the extra spawns of zone (zx, zy): its cell's monster, or a wandering group.
    // `ground` is the zone's ground height at a column.
    std::vector<worldgen::Spawn> zonePass(const worldgen::World &world, int32_t zx, int32_t zy,
                                          const std::function<float(int32_t, int32_t)> &ground,
                                          const std::vector<Player> &players, worldgen::Random &random) const;
    // §4: a player inside a location's area discovers its mission.
    void discover(const worldgen::World &world, glm::dvec3 position);
    // §5.1, §5.2: a death. `homeZone` is where the victim spawned, `flags` its appearance
    // flags, `boss` whether it came from a boss spawn. Returns the cell when the death
    // completed its mission (the caller removes the cell's extra spawns and gives rewards).
    std::optional<Key> kill(std::pair<int32_t, int32_t> homeZone, uint16_t flags, bool boss);
    // Held fatal regional stage: supplied mode, allocated-region availability
    // and current actor lookup. No record/cell is created to satisfy admission.
    std::optional<MissionProgress> fatal(const creatures::Combatant &target, uint8_t marker, uint8_t mode,
        const std::function<bool(int32_t, int32_t)> &regionAvailable,
        const std::function<creatures::Combatant *(uint64_t)> &lookup);
    // Engine publication adapter: ordered immediate snapshots before final
    // changed/requested state; explicit requests retain their complete response.
    void appendUpdates(std::span<const MissionProgress> immediate, const std::set<Key> &changedCells,
        const std::set<Key> &requestedCells, std::vector<std::vector<uint8_t>> &output) const;

    // Held containing-cell lookup: truncating zone/8 conversion, domain check,
    // resident parent and current payload only. Never allocate/load missing data.
    std::optional<creatures::activation::Context> descriptorContext(int32_t zx, int32_t zy,
        const std::function<bool(int32_t, int32_t)> &parentPresent) const;

    const Cell *cell(int32_t cx, int32_t cy) const;
    Cell &edit(int32_t cx, int32_t cy) { return cells[{cx, cy}]; }
    // §6.3: a record from the server.
    void apply(const uint8_t *record);
    // §7: a region's saved cells; reading any mission marks the region as generated today.
    void restore(int32_t cx, int32_t cy, const std::optional<MissionArea> &mission, const std::optional<MonsterArea> &monster);
    // Cells whose mission changed since the last call (for section 13 and the saves).
    std::set<Key> takeChanged() { return std::exchange(changed, {}); }
    // Regions whose pass ran since the last call (their zones get the zone pass).
    std::set<Key> takePassed() { return std::exchange(passed, {}); }

  private:
    void build(const worldgen::World &world, int32_t cx, int32_t cy, int32_t pMax, worldgen::Random &random);

    std::map<Key, Cell> cells;
    std::set<Key> done, changed, passed;
};
