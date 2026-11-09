#include "Motion.h"
#include "Config.h"
#include <algorithm>
#include <cmath>

namespace motion {

// The game works these out in single precision from the clock converted to float (§4.2).
float swayA(double t, int phase) {
    const float tf = (float)t;
    return std::cos(0.004f * tf + 30.f * (float)phase + 84.f) + std::cos(0.0027f * tf + 93.f);
}

float swayB(double t, int phase) {
    const float tf = (float)t;
    return 0.8f * (std::cos(0.005f * tf + 30.f * (float)phase) + std::cos(0.003f * tf));
}

glm::mat4 plantShear(double t, int phase, float heightVoxels, float size) {
    glm::mat4 shear(1);
    if (heightVoxels <= 0 || size <= 0)
        return shear;
    const float k = 0.1f / (heightVoxels * size);
    // Column 2 is what z contributes: x and y move in proportion to the height.
    shear[2][0] = -k * swayB(t, phase);
    shear[2][1] = k * swayA(t, phase);
    return shear;
}

glm::vec2 cloudDrift(double seconds, float distance, float speed) {
    const double t = seconds * speed;
    return {distance * (float)std::sin(t * 0.012), distance * 0.4f * (float)std::sin(t * 0.009)};
}

glm::vec2 leafOffset(double seconds, float amplitude) {
    return {amplitude * (float)(0.7 * std::sin(seconds * 1.7) + 0.3 * std::sin(seconds * 3.1)),
            amplitude * 0.65f * (float)std::sin(seconds * 1.35 + 0.8)};
}

void MotionSettings::load(const Config &c) {
    leaves = c.number("Motion", "Leaves", leaves) != 0;
    leafAmplitude = std::clamp(c.number("Motion", "LeafAmplitudePercent", (int)(leafAmplitude * 100 + 0.5f)), 0, 25) / 100.f;
    leafRange = (float)std::clamp(c.number("Motion", "LeafRange", (int)leafRange), 32, 160);
    windSpeed = std::clamp(c.number("Motion", "WindSpeedPercent", (int)(windSpeed * 100)), 10, 200) / 100.f;
    clouds = c.number("Motion", "Clouds", clouds) != 0;
    cloudDistance = (float)std::clamp(c.number("Motion", "CloudDriftDistance", (int)cloudDistance), 0, 64);
    cloudSpeed = std::clamp(c.number("Motion", "CloudSpeedPercent", (int)(cloudSpeed * 100)), 25, 500) / 100.f;
}

} // namespace motion
