#pragma once
// Particles (docs/specs/alpha-particles.md): the 72-byte burst record a step writes to section
// 3, the client's one particle list with its update, and the flame cubes of campfires and
// some actions (§8), which look like particles but are worked out afresh every frame.
//
// The server never holds particles: it only writes bursts, and each client expands them with
// its own random draws (§9).
#include "creatures/Physics.h"
#include <array>
#include <functional>
#include <vector>

namespace creatures {

// §2.2: what a particle does.
enum ParticleStyle : int32_t {
    LIT = 0,    // gravity, lit like the world (splash, block bursts, shatter)
    FIRE = 1,   // gravity, full light, a random red-to-yellow colour
    BRIGHT = 2, // gravity, full light (spirits, the water staff)
    STILL = 3,  // never moves
    LEAF = 4,   // no gravity, lit like the world, passes wood and leaves
};

// §1: a burst asks every client that gets it to spawn `count` particles at one point.
struct Burst {
    int64_t position[3] = {0, 0, 0}; // 1/65536 block
    glm::vec3 velocity{0};           // shared by every particle, blocks/s
    glm::vec4 colour{0};             // ignored for FIRE
    float size = 0;                  // cube edge, blocks
    int32_t count = 0;
    int32_t style = LIT;
    float spread = 3; // each particle's velocity gets ± this per axis

    static Burst at(glm::dvec3 blocks, glm::vec3 velocity, glm::vec4 colour, float size, int32_t count, int32_t style);
    std::array<uint8_t, 72> serialize() const; // the padding at 0x44 is written as 0
    static Burst parse(const uint8_t *record);
};

// §3.1: one particle.
struct Particle {
    int32_t style = LIT;
    int32_t ageMs = 0; // negative: not started yet (it waits where it is)
    int64_t position[3] = {0, 0, 0}; // centre of the cube
    glm::vec3 rotation{0};           // degrees about x, y, z
    glm::vec4 colour{1};
    glm::vec3 velocity{0}, spin{0}; // blocks/s, degrees/s
    glm::mat4 extra{1};             // applied after rotation and scale (the shatter's model frame)
    float size = 1;
    bool collides = false;
    int32_t lifetimeMs = 4000;

    glm::dvec3 blocks() const {
        return glm::dvec3((double)position[0], (double)position[1], (double)position[2]) / 65536.0;
    }
};

// §3.2, §4: the client's particle list.
class Particles {
  public:
    static constexpr size_t CAP = 5000;
    std::vector<Particle> list;

    // §2.1: a burst's particles, with `draw` the C runtime's rand() (0 … 32767).
    void expand(const Burst &burst, const std::function<int()> &draw);
    // §4: one frame: the cap, then age, gravity, spin, motion and removal.
    void update(int32_t dtMs, const BlockType &block);
};

// §5: the bursts of the producers. Block colours are 0..255.
Burst bombBurst(glm::dvec3 at);                         // §5.1: a bomb goes off
// §5.2 / strike-cell-helpers §8.1 step 6: a broken cell's burst at the unsnapped raw cell position.
Burst blockBurst(const std::array<int64_t, 3> &at, glm::vec3 rgb);
Burst splashBurst(glm::dvec3 at);                       // §5.5: water entered falling fast
Burst arrowBurst(glm::dvec3 at, glm::vec3 rgb);         // §5.7: an arrow into a block

// §8: one flame cube of a fire, centred on `position`, `edge` blocks on a side.
struct FlameCube {
    glm::vec3 position{0}; // blocks, relative to the fire's base
    glm::vec3 colour{0};
    glm::vec3 turn{0}; // degrees about x, y, z
    float edge = 0;
};
// §8: the `count` flames of a fire at clock `t` (ms), on a circle of `radius` blocks, `size`
// the largest edge, coloured from `base` at the bottom to `tip` at the top.
// With `twist` >= 0 the swirl variant (open points 3 §3.1): a cone opening upward (radius·f)
// whose flames turn a further 4π·twist·f².
std::vector<FlameCube> flames(double t, float radius, float size, int count, glm::vec3 base, glm::vec3 tip,
                              float twist = -1);

} // namespace creatures
