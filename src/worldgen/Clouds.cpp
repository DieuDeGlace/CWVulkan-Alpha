#include "worldgen/Clouds.h"
#include <algorithm>
#include <bit>
#include <cstdint>

namespace worldgen {
namespace {
// Clouds contract §0: binary32 times 65536, truncated to signed 64-bit. Out-of-range or
// unordered input has no ordinary contract; the engine uses the minimum (clamped to 0 below).
int64_t toFixed(float value) {
    const float scaled = value * 65536.f;
    return double(scaled) >= -0x1p63 && double(scaled) < 0x1p63 ? int64_t(scaled) : INT64_MIN;
}
} // namespace

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
            // Each binary32 term is made fixed (x 65536, truncated) on its own; the base is clamped
            // in fixed point and the lift added there (clouds contract §3 step 4).
            const float zb = terrain.height(px, py) + 100.f * terrain.mountains(px, py);
            const int64_t base = std::max(toFixed(zb), int64_t(0));
            const int64_t lift = toFixed(50.f * noise(17.0 * X, 19.0 * Y) + 170.f);
            const double z = (double)std::bit_cast<int64_t>(uint64_t(base) + uint64_t(lift)) / 65536.0;
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
