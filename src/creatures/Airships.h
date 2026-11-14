#pragma once
// World objects (docs/specs/alpha-world-objects.md, alpha-airship-steering.md): the world's
// airship list (world +0x0C). Templates a landing place leaves in its zone are copied in near
// players and dropped far from them; creatures collide with their voxels, stand on them and
// are pushed out of them; each frame a ship steers round its four courses (unconnected world),
// integrates its velocity, its pose follows its target and the creatures it carries move with
// it; its companion creature is placed on it.
#include "creatures/Physics.h"
#include "worldgen/Prefab.h"
#include "worldgen/WorldMap.h"
#include "worldgen/Zone.h"
#include <array>
#include <functional>
#include <map>
#include <optional>
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
    // One frame of `dtMs`: steering §1 when `steer` (an unconnected world; `map` gives the
    // column heights), then world objects §6.2 (integration, pose and yaw follow) and §6.3.
    // `lookup(key, out)` fills `out` for a living creature.
    void step(int32_t dtMs, bool steer, const worldgen::WorldMap &map,
              const std::function<bool(int64_t key, Carried &out)> &lookup);

    // Steering §3: a creature keyed to ship `key` (after its movement): moved to the model's
    // first marker voxel above the ship when more than one block from it, and turned to the
    // ship's yaw. False when no ship has that key.
    bool place(int64_t key, Body &body, float &yaw) const;

    const std::map<int64_t, Ship> &ships() const { return list; }

    bool collide(const int64_t position[3], glm::vec3 size, uint64_t key, int64_t &base) override;
    bool blocksRaised(const int64_t position[3], glm::vec3 size, uint64_t key) override;
    void unstick(int64_t position[3], glm::vec3 size) override;

  private:
    void steerShip(Ship &ship, int32_t dtMs, const worldgen::WorldMap &map) const;
    // §3.2 steps 1-3: the box centre in the ship's model frame, or false beyond 64 blocks.
    bool modelFrame(const Ship &ship, const int64_t position[3], glm::vec3 &centre) const;
    // §3.2 steps 4-5: whether any voxel of the box at `centre` (model frame) is solid.
    bool touches(glm::vec3 centre, glm::vec3 size) const;
    bool solid(int32_t x, int32_t y, int32_t z) const;

    std::map<int64_t, Ship> list; // ascending signed identifier
    int32_t sx = 0, sy = 0, sz = 0;
    float hx = 0, hy = 0, hz = 0; // half sizes, binary32
    std::vector<uint8_t> voxels;  // 1 where solid, x fastest, then y, then z
    std::optional<glm::ivec3> marker; // steering §3.2: the first pure red voxel (x, y, z order)
};

} // namespace creatures
