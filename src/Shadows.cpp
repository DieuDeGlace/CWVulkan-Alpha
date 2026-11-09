#include "Shadows.h"
#include "Config.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

void ShadowSettings::load(const Config &c) {
    enabled = c.number("Shadows", "Enabled", enabled) != 0;
    resolution = (uint32_t)std::clamp(c.number("Shadows", "Resolution", (int)resolution), 512, 8192);
    radius = (float)std::clamp(c.number("Shadows", "Radius", (int)radius), 16, 400);
    strength = std::clamp(c.number("Shadows", "StrengthPercent", (int)(strength * 100 + .5f)) / 100.f, 0.f, 1.f);
}

namespace shadows {

double steppedTime(double timeOfDayMs) {
    constexpr double STEP = 60000; // one game minute
    return std::floor(timeOfDayMs / STEP) * STEP;
}

glm::mat4 lightMatrix(glm::vec3 focus, glm::vec3 towardsLight, float radius, uint32_t resolution) {
    const glm::vec3 forward = -glm::normalize(towardsLight);
    // Any up vector not parallel to the light. The sun turns in the y-z plane, so x always is.
    const glm::mat4 view = glm::lookAt(glm::vec3(0), forward, glm::vec3(1, 0, 0));
    // Snap the focus, seen from the light, to whole texels of the map.
    const float texel = 2 * radius / (float)resolution;
    glm::vec3 centre = glm::vec3(view * glm::vec4(focus, 1));
    centre.x = std::round(centre.x / texel) * texel;
    centre.y = std::round(centre.y / texel) * texel;
    const glm::mat4 projection = glm::orthoRH_ZO(centre.x - radius, centre.x + radius, centre.y - radius,
                                                 centre.y + radius, -centre.z - DEPTH_RANGE, -centre.z + DEPTH_RANGE);
    return projection * view;
}

} // namespace shadows
