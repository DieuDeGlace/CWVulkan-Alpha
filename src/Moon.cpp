#include "Moon.h"
#include "AlphaLook.h"
#include "Config.h"
#include <algorithm>
#include <cmath>

void MoonSettings::load(const Config &c) {
    auto percent = [&](const char *key, float value) {
        return std::clamp(c.number("Moon", key, (int)(value * 100 + .5f)) / 100.f, 0.f, 1.f);
    };
    enabled = c.number("Moon", "Enabled", enabled) != 0;
    brightness = percent("BrightnessPercent", brightness);
    shadowStrength = percent("ShadowStrengthPercent", shadowStrength);
    nightAmbient = percent("NightAmbientPercent", nightAmbient);
}

namespace moon {
namespace {
// 0 below `edge0`, 1 above `edge1`, an S-curve between.
float ease(float edge0, float edge1, float x) {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return t * t * (3 - 2 * t);
}

// The moon's colours at full strength, before the settings scale them: a cool blue-white for
// the light from the moon, a dimmer blue for the faces turned away from it, and a deep blue
// for the ambient floor.
const glm::vec3 FRONT(.55f, .65f, .9f);
const glm::vec3 BACK(.12f, .15f, .24f);
const glm::vec3 NIGHT_AMBIENT(.16f, .24f, .55f);

// The moon (and its shadows) fade in over its first stretch above the horizon.
constexpr float RISE = .25f; // height of the moon's direction (sine of its elevation)
} // namespace

glm::vec3 direction(double timeOfDayMs) { return -alpha::sunDirection(timeOfDayMs); }

float strength(double timeOfDayMs) {
    // The darker the day, the stronger the moon; held back while it is near the horizon.
    const float dark = 1 - alpha::sunlight(timeOfDayMs);
    const float above = ease(0, RISE, direction(timeOfDayMs).z);
    return ease(0, 1, dark) * above;
}

Light light(double timeOfDayMs, const MoonSettings &settings) {
    Light m;
    if (!settings.enabled)
        return m;
    m.strength = strength(timeOfDayMs);
    m.front = FRONT * settings.brightness * m.strength;
    m.back = BACK * settings.brightness * m.strength;
    m.ambient = NIGHT_AMBIENT * settings.nightAmbient * m.strength;
    return m;
}

ShadowCaster shadowCaster(double timeOfDayMs, float sunShadow, const MoonSettings &settings) {
    ShadowCaster s;
    const glm::vec3 sun = alpha::sunDirection(timeOfDayMs);
    if (sun.z >= 0 || !settings.enabled) {
        // By day the sun casts, fading out as it nears the horizon.
        s.direction = sun;
        s.strength = sunShadow * ease(0, .2f, sun.z);
        return s;
    }
    // By night the moon casts, fading in the same way as it climbs: at the moment the sun
    // touches the horizon both are 0.
    s.direction = -sun;
    s.strength = settings.shadowStrength * ease(0, RISE, -sun.z);
    s.moon = true;
    return s;
}

float discVisibility(double timeOfDayMs) {
    const float dark = 1 - alpha::sunlight(timeOfDayMs);
    const float height = direction(timeOfDayMs).z;
    return ease(.2f, .7f, dark) * ease(-.05f, .05f, height);
}

float phase(int worldDay) {
    constexpr int CYCLE = 8;
    const int day = ((worldDay % CYCLE) + CYCLE) % CYCLE;
    return (float)day / CYCLE;
}

} // namespace moon
