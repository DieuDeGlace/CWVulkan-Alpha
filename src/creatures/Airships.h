#pragma once
// World objects (docs/specs/alpha-world-objects.md): the world's airship list (world +0x0C).
// Templates a landing place leaves in its zone are copied in near players and dropped far from
// them (§2.4); creatures collide with their voxels and stand on them (§3, §4); each frame a
// ship integrates its velocity, its drawn pose follows its target and the creatures it carries
// are moved with it (§6). Open, so not modelled: the course steering of §6.1 and the yaw-follow
// helper of §6.2 step 4 (ships keep the template's zero velocity and their yaw), the unstuck
// routine's ship part and the companion creature placement (§6.4).
#include "creatures/Physics.h"
#include "worldgen/Prefab.h"
#include "worldgen/WorldMap.h"
#include "worldgen/Zone.h"
#include <array>
#include <functional>
#include <map>
#include <set>
#include <span>
#include <vector>

namespace creatures {

class Airships final : public VoxelObstacles {
  public:
    using Raw = std::array<int64_t, 3>;
    struct Ship {
        worldgen::AirshipTemplate state; // identifier, target, home, destination, target yaw
        glm::vec3 velocity{0};           // +0x28, blocks per second
        uint8_t course = 1;              // +0x70
        std::set<int64_t> carried;       // +0x78: creatures that touched it
        Raw position{};                  // +0x80: the drawn and colliding pose
        float yaw = 0;                   // +0x98, degrees
    };
    // §5: model 0xA03 (airship.cub). Pure red voxels are emptied and never collide.
    void setModel(const worldgen::Prefab &model);
    bool hasModel() const { return !voxels.empty(); }
    glm::ivec3 modelSize() const { return {sx, sy, sz}; }

    // §2.3, §2.4: removal, then copies from the loaded zones of `map` in the 3 x 3 round each
    // player. `players` are the role-0 creatures' raw positions in creature-map order.
    void refresh(std::span<const Raw> players, const worldgen::WorldMap &map);

    // §6.3: what a carried creature exposes: its position, its follow point and its yaw.
    struct Carried {
        int64_t *position = nullptr;
        int64_t *follow = nullptr;
        float *yaw = nullptr;
    };
    // §6.2, §6.3: one frame of `dtMs`. `lookup(key, out)` fills `out` for a living creature.
    void step(int32_t dtMs, const std::function<bool(int64_t key, Carried &out)> &lookup);

    const std::map<int64_t, Ship> &ships() const { return list; }

    bool collide(const int64_t position[3], glm::vec3 size, uint64_t key, int64_t &base) override;
    bool blocksRaised(const int64_t position[3], glm::vec3 size, uint64_t key) override;

  private:
    // §3.2 steps 1-3: the box centre in the ship's model frame, or false beyond 64 blocks.
    bool modelFrame(const Ship &ship, const int64_t position[3], glm::vec3 &centre) const;
    // §3.2 steps 4-5: whether any voxel of the box at `centre` (model frame) is solid.
    bool touches(glm::vec3 centre, glm::vec3 size) const;
    bool solid(int32_t x, int32_t y, int32_t z) const;

    std::map<int64_t, Ship> list; // ascending signed identifier
    int32_t sx = 0, sy = 0, sz = 0;
    float hx = 0, hy = 0, hz = 0; // half sizes, binary32
    std::vector<uint8_t> voxels;  // 1 where solid, x fastest, then y, then z
};

} // namespace creatures
