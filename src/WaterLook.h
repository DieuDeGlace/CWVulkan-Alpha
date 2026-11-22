#pragma once
// Owner-requested revised surface from the GPL Steam renderer. Source revision in docs/water-extension.md.
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace waterlook {
inline glm::vec2 waveOffsets(glm::ivec2 origin, double seconds) {
    constexpr double turn = 2.0 * std::numbers::pi_v<double>;
    return {float(std::remainder(.65 * origin.x + .31 * origin.y + .80 * seconds, turn)),
            float(std::remainder(-.27 * origin.x + .72 * origin.y - .65 * seconds, turn))};
}
inline float brightness(float sunlight) { return .20f + .80f * std::clamp(sunlight, 0.f, 1.f); }
inline constexpr glm::vec3 underwaterMaterial{.38f, .82f, 1.f};
inline constexpr float underwaterVisibility = 48.f;

// Matches the Water descriptor in water_surface.glsl.
struct alignas(16) Constants {
    glm::vec4 planesAndWaves, body, screen, fog;
};
static_assert(sizeof(Constants) == 64);
} // namespace waterlook
