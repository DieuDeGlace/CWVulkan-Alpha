#include "worldgen/Clouds.h"
#include <algorithm>

namespace worldgen {

std::optional<CloudPlacement> CloudField::cell(int32_t X, int32_t Y) {
    const auto key = std::make_pair(X, Y);
    if (const auto it = cells.find(key); it != cells.end())
        return it->second;
    const World &w = terrain.world();
    std::optional<CloudPlacement> result;
    // §3 step 1: coverage, with the moisture sampled at the cell's index as a block column (as
    // the game does; clouds spec open question 1).
    const float c = (noise(X, Y) + 1.f) * 0.5f;
    if (c < 0.8f * w.moisture(X, Y)) {
        // Step 2: position, odd columns of cells half a cell up.
        const int32_t px = 200 * X, py = 200 * Y + 100 * (X % 2);
        // Step 3: local moisture.
        if (w.moisture(px, py) >= 0.3f) {
            // Step 4: height over the ground and the mountains.
            const float zb = std::max(terrain.height(px, py) + 100.f * terrain.mountains(px, py), 0.f);
            const double z = (double)(int64_t)(zb * 65536.f) / 65536.0 + (50.f * noise(17.0 * X, 19.0 * Y) + 170.f);
            // Steps 5-6: size and quarter-turn rotation.
            const float scale = 2.f * noise(15.0 * X, 23.0 * Y) + 7.f;
            result = CloudPlacement{(double)px, (double)py, z, scale, (float)((X + Y) * 90)};
        }
    }
    cells[key] = result;
    return result;
}

std::vector<CloudPlacement> CloudField::place(int32_t playerX, int32_t playerY, int32_t cameraX, int32_t cameraY) {
    std::vector<CloudPlacement> out;
    const World &w = terrain.world();
    // §2: none in dry or burning country.
    if (w.moisture(playerX, playerY) <= 0.01f || w.fireShare(playerX, playerY) >= 0.01f)
        return out;
    // §3: the 13 x 13 cells round the camera's cell.
    const int32_t cx = cameraX / 200, cy = cameraY / 200;
    for (int32_t i = -6; i <= 6; i++)
        for (int32_t j = -6; j <= 6; j++)
            if (const auto c = cell(cx + i, cy + j))
                out.push_back(*c);
    return out;
}

} // namespace worldgen
