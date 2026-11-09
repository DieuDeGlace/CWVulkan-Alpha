// Checks Alpha's daylight curves and palette against the worked values in
// docs/specs/alpha-rendering.md (§2.1, §2.2, §3.8, §8.3).
#include "AlphaLook.h"
#include "Frame.h"
#include "Motion.h"
#include <cmath>
#include <bit>
#include <cstdio>
#include <string>

static int failures = 0;

static void near(float got, float want, const std::string &what, float tolerance = 1e-4f) {
    if (!std::isfinite(got) || std::abs(got - want) > tolerance) {
        failures++;
        std::printf("FAIL %s: got %.5f, want %.5f\n", what.c_str(), got, want);
    }
}

static void near(glm::vec3 got, glm::vec3 want, const std::string &what) {
    for (int i = 0; i < 3; i++)
        near(got[i], want[i], what + "[" + std::to_string(i) + "]");
}

static double at(double hour) { return hour * 3600000.0; }

static void exact(float got, uint32_t bits, const std::string &what) {
    if (std::bit_cast<uint32_t>(got) != bits) {
        failures++;
        std::printf("FAIL %s: got %08x, want %08x\n", what.c_str(), std::bit_cast<uint32_t>(got), bits);
    }
}

int main() {
    // Static binary32 anchors from the held CPU arithmetic, not native pixel captures.
    exact(alpha::dayPosition(16777217), 0xbf1c9458, "clock narrows before multiplication and division");
    const auto dry = alpha::palette(1.f, {1.f, 0.f, 0.f, 0.f}, false);
    // (1 - .6f) / .4f is just below one; include that rounded weight.
    exact(dry.fog.g, 0x3f333332, "dry fog retains its rounded weight and stored green coefficient");
    const auto wet = alpha::palette(1.f, {1.f, 1.f, 0.f, 0.f}, false);
    exact(wet.sky1.g, 0x3f666666, "wet sky retains its stored green coefficient");
    const auto overridden = alpha::palette(.5f, {.5f, .5f, 0.f, .13f}, false);
    exact(overridden.back.r, 0x3f204189, "Q override uses two weighted products before addition");
    const auto extrapolated = alpha::palette(.5f, {.5f, .5f, 0.f, 2.5f}, false);
    exact(extrapolated.back.r, 0x3f8ccccc, "positive Q is not clamped to one");
    const auto negativeQ = alpha::palette(1.f, {.5f, .5f, 0.f, -1.f}, false);
    exact(negativeQ.front.r, std::bit_cast<uint32_t>(1.1f), "nonpositive Q leaves directional palette unchanged");
    // §2.1: L = (1 - |x|^3)^4.
    near(alpha::paletteDaylight(at(12)), 1, "L noon");
    near(alpha::paletteDaylight(at(9)), .9390f, "L 09:00");
    near(alpha::paletteDaylight(at(6)), .5862f, "L 06:00");
    near(alpha::paletteDaylight(at(21)), .1117f, "L 21:00");
    near(alpha::paletteDaylight(at(0)), 0, "L midnight");
    // §2.2: S = (1 - x^4)^5.
    near(alpha::sunlight(at(12)), 1, "S noon");
    near(alpha::sunlight(at(9)), .9806f, "S 09:00");
    near(alpha::sunlight(at(6)), .7242f, "S 06:00");
    near(alpha::sunlight(at(3)), .1494f, "S 03:00", 2e-4f); // exactly 0.14928; the spec rounds up
    near(alpha::sunlight(at(1)), .0022f, "S 01:00");
    // §2.3: overhead at noon, (0, -1, 0) at 06:00.
    near(alpha::sunDirection(at(12)), {0, 0, 1}, "D noon");
    near(alpha::sunDirection(at(6)), {0, -1, 0}, "D 06:00");

    // §3.8: the temperate zone at noon, 06:00 and midnight.
    const alpha::ZoneFactors temperate{.5f, .5f, 0, 0};
    const auto noon = alpha::palette(1, temperate, false);
    near(noon.sky1, {.1f, .6f, 1}, "noon sky1");
    near(noon.sky2, {.01f, .05f, 1}, "noon sky2");
    near(noon.fog, {.01f, .1f, 1}, "noon fog");
    near(noon.front, {1.1f, 1.1f, .9f}, "noon front");
    near(noon.back, {.5f, .6f, .8f}, "noon back");
    near(noon.ambient, {.6f, .6f, .4f}, "noon ambient");
    const auto dawn = alpha::palette(alpha::paletteDaylight(at(6)), temperate, false);
    const float rounding = 2e-4f; // the spec's table is rounded to 4 places
    for (int i = 0; i < 3; i++) {
        near(dawn.sky1[i], glm::vec3(1, .8586f, .4862f)[i], "06:00 sky1", rounding);
        near(dawn.sky2[i], glm::vec3(.2207f, .0931f, .7793f)[i], "06:00 sky2", rounding);
        near(dawn.fog[i], glm::vec3(.9655f, .8517f, .7862f)[i], "06:00 fog", rounding);
        near(dawn.front[i], glm::vec3(1.0172f, .9345f, .6517f)[i], "06:00 front", rounding);
        near(dawn.back[i], glm::vec3(.5828f, .5172f, .9655f)[i], "06:00 back", rounding);
    }
    const auto midnight = alpha::palette(0, temperate, false);
    near(midnight.sky1, {.05f, 0, .1f}, "midnight sky1");
    near(midnight.front, {.01f, .02f, .01f}, "midnight front");
    near(midnight.back, {0, .02f, .03f}, "midnight back");

    // §8.3: 3000 stars on a 100-block sphere above the horizon; shown only while S < 0.5.
    const auto stars = alpha::stars();
    bool onSphere = stars.size() == 3000;
    for (const auto &s : stars)
        onSphere = onSphere && std::abs(glm::length(s.position) - 100) < .01f && s.position.z >= 0;
    if (!onSphere) {
        failures++;
        std::printf("FAIL stars\n");
    }
    Scene scene;
    scene.timeOfDayMs = at(12);
    near(drawStars(scene) ? 1.f : 0.f, 0, "no stars at noon");
    scene.timeOfDayMs = at(2);
    near(drawStars(scene) ? 1.f : 0.f, 1, "stars at 02:00");

    // Shadows.h: the caster direction moves in whole game minutes, is the sun by day and the
    // opposite direction at night, and the map centre snaps to whole texels.
    near((float)shadows::steppedTime(at(16) + 59999), (float)at(16), "shadow step");
    // Moon.h: the sun casts at noon, the moon (opposite the sun) at midnight, and at sunset
    // both are faded out so the switch does not pop.
    const MoonSettings moonOn;
    const auto noonCaster = moon::shadowCaster(at(12), .9f, moonOn);
    near(noonCaster.direction, {0, 0, 1}, "sun casts at noon");
    near(noonCaster.strength, .9f, "full sun shadow at noon");
    near(noonCaster.moon ? 1.f : 0.f, 0, "noon caster is the sun");
    const auto midnightCaster = moon::shadowCaster(at(0), .9f, moonOn);
    near(midnightCaster.direction, {0, 0, 1}, "moon casts at midnight, overhead");
    near(midnightCaster.strength, moonOn.shadowStrength, "full moon shadow at midnight");
    near(midnightCaster.moon ? 1.f : 0.f, 1, "midnight caster is the moon");
    near(moon::shadowCaster(at(18), .9f, moonOn).strength, 0, "no shadow at sunset");
    near(moon::strength(at(12)), 0, "no moon at noon");
    near(moon::strength(at(0)), 1, "full moon strength at midnight");
    near(moon::phase(4), .5f, "day 4 is full moon");
    MoonSettings moonOff;
    moonOff.enabled = false;
    near(moon::light(at(0), moonOff).front, {0, 0, 0}, "moon off: Alpha's black night");
    const glm::mat4 a = shadows::lightMatrix({10.3f, 0, 0}, {0, 0, 1}, 120, 4096);
    const glm::mat4 b = shadows::lightMatrix({10.31f, 0, 0}, {0, 0, 1}, 120, 4096);
    near(glm::vec3(a[3]), glm::vec3(b[3]), "focus within a texel keeps the map still");
    const glm::vec4 focusClip = a * glm::vec4(10.3f, 0, 0, 1);
    if (std::abs(focusClip.x) > 2.f / 4096 || std::abs(focusClip.y) > 2.f / 4096 || focusClip.z < 0 || focusClip.z > 1) {
        failures++;
        std::printf("FAIL focus not at the map centre\n");
    }

    // Motion.h, vegetation-wind.md §4.1: at t = 0 with phase 0, a = cos 84 + cos 93 and
    // b = 0.8 (1 + 1) = 1.6; the top of a plant moves 0.1 a and -0.1 b blocks whatever its size.
    const float a0 = std::cos(84.f) + std::cos(93.f);
    near(motion::swayA(0, 0), a0, "sway a at t = 0");
    near(motion::swayB(0, 0), 1.6f, "sway b at t = 0");
    for (float size : {0.5f, 1.f, 2.f}) {
        const float H = 20;
        const glm::vec4 top = motion::plantShear(0, 0, H, size) * glm::vec4(0, 0, H, 1);
        near(top.x * size, -0.16f, "top x offset, size " + std::to_string(size));
        near(top.y * size, 0.1f * a0, "top y offset, size " + std::to_string(size));
        near(top.z, H, "height unchanged");
    }
    const glm::vec4 base = motion::plantShear(1234, 3, 20, 1) * glm::vec4(2, 3, 0, 1);
    near(glm::vec3(base), {2, 3, 0}, "base stays put");

    // Screens §5.1.5: the corner map's worked example (1280 x 720, s = 0.8, Alpha yaw 30
    // degrees = ours 150), points in map units relative to the player.
    {
        const glm::mat4 M = minimapMatrix(glm::vec3(0), 150.f, 0.8f, 1280, 720);
        auto pixel = [&](glm::vec3 mapUnits) {
            const glm::vec4 c = M * glm::vec4(mapUnits * 8.f, 1);
            return glm::vec2((c.x / c.w + 1) * 0.5f * 1280, (c.y / c.w + 1) * 0.5f * 720);
        };
        const glm::vec2 corner = pixel({-16, -16, 0}), pivot = pixel({0, 0, 0}), far1 = pixel({-48, -48, 0}),
                        ahead = pixel({5, -8.6603f, 0});
        near(corner.x, 1089.98f, "minimap corner x", 0.02f);
        near(corner.y, 163.30f, "minimap corner y", 0.02f);
        near(pivot.x, 1040, "minimap pivot x", 0.01f);
        near(pivot.y, 170, "minimap pivot y", 0.01f);
        near(far1.x, 1186.04f, "minimap far corner x", 0.02f);
        near(far1.y, 150.43f, "minimap far corner y", 0.02f);
        near(ahead.x, 1040, "minimap forward is up", 0.02f);
        near(ahead.y, 158.67f, "minimap forward y", 0.02f);
    }

    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
