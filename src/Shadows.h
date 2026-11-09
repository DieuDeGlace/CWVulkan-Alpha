#pragma once
// AlphaVulkan's own sun and moon shadows (neither Alpha nor Steam casts shadows: CubeVulkan's
// docs/specs/shadows.md §1). One shadow map around the focus point, drawn from the light. Only
// the light's direct term is taken away in shadow, scaled by the vertex light, so places that
// Alpha's baked sky light already darkens are not darkened twice (shadows.md §4).
//
// By day the caster is the sun; by night it is AlphaVulkan's moon (Moon.h), each fading out
// near the horizon.
#include <glm/glm.hpp>
#include <cstdint>

class Config;

struct ShadowSettings {
    bool enabled = true;
    uint32_t resolution = 4096;
    float radius = 120;  // blocks around the focus covered by the map
    float strength = .9f; // share of the direct light taken away in full shadow
    void load(const Config &config);
};

namespace shadows {

// Light boxes reach this far towards and away from the light, so tall casters outside the
// view still cast into it.
constexpr float DEPTH_RANGE = 420;

// The shadow direction moves in steps of one game minute (0.25 degrees of the sun's turn), so
// the map is not redrawn from a slightly rotated light every frame, which makes edges crawl.
double steppedTime(double timeOfDayMs);

// World -> shadow clip space: an orthographic box `radius` blocks around `focus`, looking along
// -towardsLight, with its centre snapped to whole map texels so the map does not shimmer as the
// focus moves.
glm::mat4 lightMatrix(glm::vec3 focus, glm::vec3 towardsLight, float radius, uint32_t resolution);

} // namespace shadows
