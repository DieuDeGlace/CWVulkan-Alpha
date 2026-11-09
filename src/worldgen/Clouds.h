#pragma once
// Where Alpha draws its clouds (docs/specs/alpha-worldgen-clouds.md): copies of cloud02.cub on a
// staggered 200-block grid round the camera, chosen by noise and moisture, lifted over mountains.
// Each cell's cloud never changes, so it is worked out once and kept.
#include "worldgen/Terrain.h"
#include <map>
#include <optional>
#include <vector>

namespace worldgen {

struct CloudPlacement {
    double x, y, z;    // blocks
    float scale;       // uniform, 5..9
    float rotation;    // degrees about +Z, quarter turns
};

class CloudField {
  public:
    explicit CloudField(const Terrain &terrain) : terrain(terrain) {}

    // §2-§3: the clouds of this frame for the player's column and the camera's column.
    std::vector<CloudPlacement> place(int32_t playerX, int32_t playerY, int32_t cameraX, int32_t cameraY);

  private:
    std::optional<CloudPlacement> cell(int32_t X, int32_t Y);

    const Terrain &terrain;
    std::map<std::pair<int32_t, int32_t>, std::optional<CloudPlacement>> cells;
};

} // namespace worldgen
