#pragma once
// Items lying in the world (docs/specs/alpha-world-items.md): each zone's list, made from the
// generator's ground items and loot and replaced by the zone's save, the landing timer, drops
// that fall to the first solid block, and pick-ups.
#include "creatures/Combat.h"
#include "creatures/Inventory.h"
#include "creatures/Physics.h"
#include "worldgen/Zone.h"
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <vector>

// §1: one record (328 bytes on the network, 325 in a save).
struct WorldItem {
    creatures::ItemRecord item{};
    int64_t position[3] = {0, 0, 0}; // 1/65536 blocks: the footprint's centre, z its base
    float angle = 0;                 // degrees about the vertical
    float scale = 1.f / 14;          // blocks per model voxel
    uint8_t origin = 0;              // 0 dropped, 1 structure or cave loot, 2 vegetation (no behaviour)
    int32_t landingMs = 0;           // the hop and tumble run while this counts down
    int32_t hideMs = 0;              // hidden while not 0
    int32_t day = -1;                // the day it was dropped; -1 for generated items

    glm::dvec3 blocks() const {
        return glm::dvec3((double)position[0], (double)position[1], (double)position[2]) / 65536.0;
    }
    std::array<uint8_t, 325> save() const;
    static WorldItem load(const uint8_t *record); // 325 bytes
    std::array<uint8_t, 328> record() const;      // the network layout (packets §3.7)
    static WorldItem fromRecord(const uint8_t *record);
    bool operator==(const WorldItem &o) const { return save() == o.save(); }
};

// §2.1: model voxels -> blocks relative to the item's position, for an item whose model is
// `size` voxels, at `clockMs`; `forBox` gives the pose of the item-target box (§2.6).
glm::mat4 worldItemPose(const WorldItem &w, glm::vec3 size, double clockMs, bool forBox = false);

class WorldItems {
  public:
    using Key = std::pair<int32_t, int32_t>;

    // §4: the items a zone's generator placed, if the zone has no list yet.
    void generated(const worldgen::Zone &zone);
    // §3.5: a zone's saved items, kept when dropped within three days of `today` (or
    // generated); they replace the generated list when different.
    void restore(int32_t zx, int32_t zy, const std::vector<std::array<uint8_t, 325>> &saved, int32_t today);
    // Forgets a zone (it has gone out of range).
    void forget(int32_t zx, int32_t zy) { lists.erase({zx, zy}); }
    const std::vector<WorldItem> *zone(int32_t zx, int32_t zy) const;
    // Online: the server's list for a zone (section 7).
    void replace(int32_t zx, int32_t zy, std::vector<WorldItem> items);
    const std::map<Key, std::vector<WorldItem>> &all() const { return lists; }
    // Zones whose list changed since the last call (to save).
    std::set<Key> takeChanged() { return std::exchange(changed, {}); }

    // §3.1: one step: paintings snap to walls, the timers run, and offline a landing plays its
    // sound.
    void step(int32_t dtMs, bool offline, const creatures::BlockType &block, const std::function<int()> &draw,
              std::vector<creatures::SoundEvent> &sounds);
    // §3.2: drops an item at `at` (blocks) with `factor` × 1/14 blocks per voxel; it falls to
    // the first solid block. False when the zone is not loaded (the item is lost).
    bool drop(const creatures::ItemRecord &item, glm::dvec3 at, float factor, int32_t day,
              const creatures::BlockType &block, const std::function<int()> &draw);
    bool drop(const creatures::ItemRecord &item, const int64_t at[3], float factor, int32_t day,
              const creatures::BlockType &block, const std::function<int()> &draw);
    // §3.3 kind 5: takes item `index` of a zone for a creature at `from` (within 4 blocks).
    std::optional<creatures::ItemRecord> pickUp(int32_t zx, int32_t zy, int32_t index, glm::dvec3 from);

  private:
    std::map<Key, std::vector<WorldItem>> lists;
    std::set<Key> changed;
};
