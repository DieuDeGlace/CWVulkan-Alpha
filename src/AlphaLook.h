#pragma once
// Cube World Alpha's look: its clock, its daylight curves, the time-of-day palette with the
// zone modifiers, the fog distance and the stars. Everything here is evaluated on the CPU
// once per frame and handed to the Alpha shaders (shaders/alpha*.glsl) as constants.
//
// Inherited rendering sections supply the existing API. Clock/palette arithmetic
// now also follows the independently held alpha-terrain-shader-color-numerics.md.
// Spatial factor generators and final display transfer remain unverified.
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace alpha {

constexpr double DAY_MS = 86400000.0;

// §2: x runs from -1 at midnight through 0 at noon to +1 at the next midnight.
float dayPosition(double timeOfDayMs);
// §2.1, the palette's daylight L = (1 - |x|^3)^4: 1 at noon, 0 at midnight.
float paletteDaylight(double timeOfDayMs);
// §2.2, the sunlight strength S = (1 - x^4)^5. It scales all sun and sky light, so Alpha's
// nights are black apart from point lights, the sky and the stars.
float sunlight(double timeOfDayMs);
// §2.3, the light direction D, pointing from a surface towards the sun: overhead at noon,
// (0, -1, 0) at 06:00, straight down at midnight. Alpha has no moon.
glm::vec3 sunDirection(double timeOfDayMs);

// Existing names for the palette's spatial inputs. The held numeric contract
// identifies A, B, C and Q, without establishing their meanings or ranges.
struct ZoneFactors {
    float warmth = .5f;   // low: the cold ambient colour
    float moisture = .5f; // high: the green "wet" fog; low: the pale "dry" sky
    float sea = 0;        // share of nearby ground below sea level
    float fire = 0;       // share of nearby fire (lava) ground: red skies
};

// §3: the six colours of the frame.
struct Palette {
    glm::vec3 sky1{0};    // horizon side of the sky gradient
    glm::vec3 sky2{0};    // zenith side
    glm::vec3 fog{0};     // distance haze
    glm::vec3 front{0};   // the sun's colour (lights faces turned towards D)
    glm::vec3 back{0};    // lights faces turned away from D
    glm::vec3 ambient{0}; // lights every face; depends on the zone's warmth only
};
// The palette for palette daylight L, the zone around the camera and whether the camera is
// under water (§3.2 to §3.7).
Palette palette(float daylight, const ZoneFactors &zone, bool underwater);

// §7.4 and §4: the material colour every world draw is multiplied by. Its alpha is the
// sunlight strength S.
glm::vec4 material(float sunlight, bool underwater);

// §7.2: how far the sky gradient shifts with the camera's tilt. `zenithDegrees` is the angle
// between straight up and the view direction (90 = level, 120 = 30 degrees down).
float fogGradientTranslation(float zenithDegrees);

// §7.1: the render radius R (a 40-chunk window times 0.5 times 32 blocks); fixed in Alpha.
constexpr int32_t FOG_WINDOW_CHUNKS = 40;
constexpr float RENDER_RADIUS = 640;
// §7.1 steps 1-3: the fog target for a camera at block column (x, y) and the 40 x 40 chunk
// window whose first chunk is `window`. Starting from R², every window chunk with both
// coordinates in [0, 0x80000) that `loaded(i, j)` denies lowers the minimum to the squared
// horizontal distance to its centre (32 i + 16, 32 j + 16). Returns sqrt(min) - 22.4.
template <class Loaded> float fogTarget(glm::dvec2 camera, glm::ivec2 window, Loaded loaded) {
    double nearest = double(RENDER_RADIUS) * RENDER_RADIUS;
    for (int32_t i = window.x; i < window.x + FOG_WINDOW_CHUNKS; i++)
        for (int32_t j = window.y; j < window.y + FOG_WINDOW_CHUNKS; j++) {
            if (uint32_t(i) >= 0x80000u || uint32_t(j) >= 0x80000u || loaded(i, j))
                continue;
            const double dx = 32.0 * i + 16 - camera.x, dy = 32.0 * j + 16 - camera.y;
            nearest = std::min(nearest, dx * dx + dy * dy);
        }
    return float(std::sqrt(nearest)) - 22.4f;
}

// §7.1: the fog distance F follows the nearest unloaded ground, smoothed over time.
class FogDistance {
  public:
    // Distance to the nearest missing chunk centre (or the render radius) in blocks.
    static float target(float nearestMissing) { return nearestMissing - 22.4f; }
    // Moves towards `target` over `elapsedMs`: w = 1 - 0.999^ms; F <- F (1 - w) + target w.
    void update(float targetDistance, double elapsedMs);
    float value() const { return distance; }
    void reset(float value) { distance = value; }

  private:
    float distance = 0;
};

// §8.3: one star on the sky sphere (100 blocks out, z up) and its size.
struct Star {
    glm::vec3 position;
    float size;
};
// The 3000 stars, from the C runtime's rand() sequence started at its default seed.
std::vector<Star> stars();
// Stars are drawn while S < 0.5 and the camera is not under water (§8.3).
inline bool starsVisible(float sunlight, bool underwater) { return sunlight < .5f && !underwater; }
// §8.4: the sun sprite's vertex colour (0..1) for sunlight strength S.
glm::vec4 sunSpriteColour(float sunlight);

// §10.3: how much of a point light a vertex receives (CPU form of the shader's sum).
// `fromLight` is the vector from the light to the vertex, `normal` the face normal.
float pointLightShare(glm::vec3 fromLight, glm::vec3 normal, float radius);

} // namespace alpha
