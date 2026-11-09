#pragma once
// AlphaVulkan's moon. Cube World Alpha has none: its nights are black apart from lamps, the sky
// and the stars, because every sun and sky term is scaled by the sunlight strength S
// (AlphaLook.h). This is an addition, carried over from CubeVulkan (which adapted the user's
// MIT-licensed MoonandShadow mod), and [Moon] Enabled=0 gives Alpha's nights back.
//
// The moon sits opposite the sun and lights the world the way the sun does: a directional term
// with a weaker back light, multiplied by the light baked into each vertex (so caves and
// interiors stay dark), plus a small cool ambient floor. At night the shadow map follows the
// moon and takes away only the moonlight.
#include <glm/glm.hpp>

class Config;

// [Moon] in AlphaVulkan.ini.
struct MoonSettings {
    bool enabled = true;
    float brightness = .30f;    // scales the moonlight colour
    float shadowStrength = .6f; // share of the moonlight a moon shadow takes away
    float nightAmbient = .30f;  // scales the night ambient colour
    void load(const Config &config);
};

namespace moon {

// Towards the moon from a surface: always opposite the sun, so it rises at 18:00, is overhead
// at midnight and sets at 06:00.
glm::vec3 direction(double timeOfDayMs);

// The moon's strength M (0..1). It grows as the sunlight S fades, eased so dusk and dawn blend
// smoothly, and is 0 while the moon is below the horizon. 0 at noon, 1 at midnight.
float strength(double timeOfDayMs);

// What the moon adds to the vertex light this frame. Every colour already carries M and the
// settings; the shader multiplies each by the vertex's baked light.
struct Light {
    float strength = 0;   // M
    glm::vec3 front{0};   // on faces turned towards the moon
    glm::vec3 back{0};    // on faces turned away from it
    glm::vec3 ambient{0}; // on every face: the night floor
};
Light light(double timeOfDayMs, const MoonSettings &settings);

// The light the shadow map follows: the sun by day, the moon by night. Each one's shadows fade
// out towards the horizon, so the switch at sunset and sunrise happens while both are 0.
struct ShadowCaster {
    glm::vec3 direction{0, 0, 1}; // towards the light
    float strength = 0;           // 0..1 of that light's own term taken away in shadow
    bool moon = false;            // the moon casts (the shader then shadows the moonlight)
};
// `sunShadow` is the full daytime strength ([Shadows] StrengthPercent).
ShadowCaster shadowCaster(double timeOfDayMs, float sunShadow, const MoonSettings &settings);

// How visible the moon's disc is (0..1): it fades in as the sky darkens and out as it sets.
float discVisibility(double timeOfDayMs);

// The moon's phase for a world day, 0..1 round an eight-day cycle: 0 and 1 new, 0.5 full.
float phase(int worldDay);

} // namespace moon
