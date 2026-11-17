#pragma once
// How a creature moves through the blocks (docs/specs/alpha-physics-camera.md): the world step
// in pieces of at most 20 ms (§1), intent as acceleration with a speed cap (§3), gravity and
// buoyancy (§3.5, §5.2), the per-axis substep collision with step-up (§4), contact flags,
// ground and step-down (§5), and friction (§6). Travel (docs/specs/alpha-travel.md): the
// glider and its wind, sailing, riding, climbing and fall damage, with the skills' factors.
//
// Creature separation (§4.4), static objects and voxel objects (§4.5) are in, given by the
// caller.
//
// Not in yet: the attachment branches (§2 item 17, obj +0x11c0), the static-object 3 x 3
// sector restriction and the first-sector-only scan after a z cell hit (§4.1, §4.5; the
// caller's ObjectTest scans zones), the debug speed (S = 20) and the dodge-cancel exception
// bytes (§3.2, unresolved).
#include <array>
#include <bit>
#include <cstdint>
#include <functional>
#include <set>
#include <glm/glm.hpp>
#include <vector>

namespace creatures {

// §5.1: contact flags.
enum Contact : uint32_t {
    ON_GROUND = 0x01,
    IN_WATER = 0x02,
    ON_WALL = 0x04,
    NOT_SUBMERGED = 0x10,
    STUCK = 0x20,
    ON_OBJECT = 0x40, // §4.5: an object blocked the move
};

// Record field 14, the mode flags the step reads (travel §1).
enum Mode : uint32_t {
    CLIMB = 0x01,  // Left Ctrl held: hold on to walls
    GLIDER = 0x10, // the glider is open
    AIMING = 0x04, // facing its look vector (a fighter in range)
    RUN = 0x40,    // normal speed ×2 (Left Shift not held); while gliding, the lift (Space)
    LANTERN = 0x200, // physics §2 item 18.2: cleared unless the lantern is equipped
    // Rulings camera-lens-pan §2: the lens (30° field of view, zoom crosshair, pose override,
    // the skill-scaled meter); cleared by any intent (physics §2 item 4). Not sitting, which is
    // action 83. No native setter is known.
    LENS = 0x400,
};

// Travel §0: a skill's factor v(n) = 1 − 1/(0.1·n + 1).
float skillFactor(int32_t n);

// The physical state of one creature.
// A static object by zone x, zone y and index in the zone's list; a set of them.
using StaticRef = std::array<int32_t, 3>;
using IgnoredStatics = std::set<StaticRef>;

// World objects §3, §4: the voxel objects (airships) a moving body collides with. `collide`
// tests the box at `position` (raw) of size `size` against every object, inserting `key` into
// each object hit and leaving `base` at the hit object's base height (the last in key order);
// `blocksRaised` is the step-up attempt's test of the box raised by 1.01 in each object's
// frame, stopping at the first hit.
struct VoxelObstacles {
    virtual ~VoxelObstacles() = default;
    virtual bool collide(const int64_t position[3], glm::vec3 size, uint64_t key, int64_t &base) = 0;
    virtual bool blocksRaised(const int64_t position[3], glm::vec3 size, uint64_t key) = 0;
    // Airship steering §5: after the terrain unstick, push a box overlapping an object's
    // voxels out along the nearest free horizontal axis (objects in key order).
    virtual void unstick(int64_t position[3], glm::vec3 size) = 0;
};

// The item and actor range tests convert the difference, not the coordinates.
inline float fixedDistanceSquared(const int64_t *a, const int64_t *b) {
    float d[3];
    for (int axis = 0; axis < 3; ++axis)
        d[axis] = (float)std::bit_cast<int64_t>(uint64_t(a[axis]) - uint64_t(b[axis])) * (1.f / 65536.f);
    return (d[0] * d[0] + d[1] * d[1]) + d[2] * d[2];
}

struct Body {
    int64_t position[3] = {0, 0, 0}; // centre of the box, 1/65536 blocks
    // Cast-point contract §2: the follow point (object +0x1350), zero at construction and
    // set after each world step of a live actor; combat copies a target's into its cast point.
    int64_t follow[3] = {0, 0, 0};
    glm::vec3 velocity{0}, intent{0}, push{0};
    glm::vec3 separate{0}; // §4.4: the push from overlapping creatures this frame, blocks/s
    glm::vec3 size{0.96f, 0.96f, 2.16f}; // collision box, blocks
    uint32_t contact = 0;
    glm::vec3 wallNormal{0};
    float fallStart = 0;  // blocks
    float bob = 0;        // landing bob, blocks (alpha-creatures-models.md §7.3)
    float bank = 0, swimPitch = 0; // orientation y and x, degrees
    float stamina = 1;
    uint32_t mode = 0;    // record field 14; actor phases refresh/commit the complete word
    int32_t dodgeMs = 0;
    bool swimmer = false; // appearance flag 0x0100: blocked by anything but water
    bool flyer = false;   // appearance flag 0x0002: falls at a tenth of gravity

    // What the step needs to know of the creature beyond its body (travel §1).
    std::array<int32_t, 11> skills{}; // record field 46
    int32_t action = 0;      // record field 9; the step ends riding (106) and sailing (107) by setting 0
    int32_t stunMs = 0;      // hit-stun
    float yaw = 0;           // degrees, facing(ψ) = (−sin ψ, cos ψ)
    bool local = false;      // the game's own player: glider control, riding, the ledge, falls
    bool mountValid = false; // riding: the player's pet exists, lives and can be ridden
    float ridingEnergy = 1;  // obj +0x1198, 0..1
    // §3.2, §3.3: what the speed limit and the action motions read of the creature.
    int32_t controller = 0, type = 2;
    uint16_t appearanceFlags = 0;
    float actionMs = 0, windUpMs = 0, activeMs = 0, durationMs = 0; // the action's time, P1, X (P2), duration
    float haste = 1; // the haste buff's factor (buff kind 12), 1 without it
    // §3.2 sneak: a non-player's skill when its own is 0 (obj +0x190 / 2 + tier + 1).
    int32_t skillFallback = 0;
    // §3.2 action 81: the use length (RVA 0xc6b80 on obj +0x1e8); negative when not supplied,
    // which leaves the rule out.
    float useLimitMs = -1;
    // §2 item 18.2: equipment byte +0xf08/+0xf09 holds the glider (0x17, 0) and +0xdf0 the
    // lantern (0x18). Callers that do not supply them keep the modes.
    bool gliderEquipped = true, lanternEquipped = true;
    float maxHealth = 1; // §4.2 fall damage scale (1: fallHurt is a share of maximum health)
    bool targetInContact = false; // §3.3: a hostile creature within contact range (the caller's test)
    bool slowed = false, hastened = false; // §3.2: record fields 18 and 19 running
    glm::vec3 look{0}; // record field 26: where it aims, used while mode 0x04 is set
    const IgnoredStatics *ignoredStatics = nullptr; // statics its path search walks through
    VoxelObstacles *obstacles = nullptr; // world objects §3: the airships it can stand on (owner's)
    uint64_t key = 0;                    // the creature's id, inserted into the objects it touches
    bool replica = false; // a received creature advanced locally: no unstuck (remote smoothing §3.1)
    bool pathActive = false; // supplies the navigation-container response-decay gate

    // What the step did, for the game to act on; the game clears them.
    bool crashed = false; // travel §3.6: the glider crashed (stun 1500 ms, sound 23)
    float fallHurt = 0;   // §4.2 step 2: the local player's fall damage (× maxHealth)
    bool splashed = false; // §5.3: water entered falling faster than 3 blocks/s (a splash, sound 31)
    // §4.2 step 1, §2 item 18.5: any creature; the sound plays only when a local player exists.
    bool thudded = false;  // landed faster than 2 blocks/s (sound 32)
    bool rolled = false;   // the dodge passed 500 ms left (sound 26)

    glm::dvec3 blocks() const {
        return glm::dvec3((double)position[0], (double)position[1], (double)position[2]) / 65536.0;
    }
    void place(glm::dvec3 at) {
        for (int k = 0; k < 3; k++)
            position[k] = (int64_t)(at[k] * 65536.0);
    }
};

// The type byte of the block at a cell (low 5 bits: 0 air, 2 water, anything else solid).
// Reads a cell's block type; UNLOADED_CELL (worldgen::UNHELD) for a zone that is not loaded.
constexpr uint8_t UNLOADED_CELL = 31;
using BlockType = std::function<uint8_t(int32_t x, int32_t y, int32_t z)>;

// §3.3: whether action `action` at time `ms` lunges (the intercept 48, and the melee swings in
// their first 200 ms).
bool lunges(int32_t action, float ms);
// §3.3 (0x5a7b50): whether a creature with box `size` at `self` is in contact range of another.
bool inContact(glm::vec3 size, glm::dvec3 self, glm::vec3 other, glm::dvec3 them);

// §4.4: how fast (blocks per second) a creature with box `size` at `self` (raw) is pushed away
// from another at `them` (raw) with box `other`. The overlap uses the other box enlarged by
// (1, 1, 0) when `enlarge` (every kind but 6); the push scales by the unenlarged other.x.
glm::vec3 separationPush(glm::vec3 size, const int64_t self[3], glm::vec3 other, bool enlarge, const int64_t them[3],
                         bool stunned);

// Physics §3.5 / travel §2.1: gliding in the physics sense (mode 0x10, falling, in the air,
// not stunned or dodging).
bool gliding(const Body &body);
// Travel §3.5: the wind a glider feels at a position (1/65536 blocks), blocks/s².
glm::vec3 wind(const int64_t position[3]);
// Travel §3.5: the 2-D value noise N2 the wind is made of.
float valueNoise(double x, double y);
// Travel §0 Blend: the weight w := w + (1 − w)·rate applied `dtMs` times from 0 in double
// precision, rounded to a float (the caller mixes x := (1 − w)·x + w·target in single precision).
float blendWeight(double rate, int32_t dtMs);
// Travel §6.4 and rulings walk-flag-pitch §2: the head pitch (+0x58) over one world update of
// `dtMs`, for every updated creature. On the climb-hold path (0x01, wall contact, 0x10 clear)
// it relaxes toward 45 at 0.005 per millisecond (and the aim block is skipped); otherwise it
// is first multiplied by float(exp(float(dt) × −0.01f)), before any aim blend.
float stepHeadPitch(float pitch, bool climbing, int32_t dtMs);

// §4.5: whether a static object's box overlaps the box from `lo` to `hi` (blocks). With
// `axis` 0..2 the box just moved by `move` blocks on that axis, and only statics it moved
// toward count (open points 2 §2.2: one can always walk out of a static); -1 tests plain
// overlap.
// `ignored` lists statics (zone x, zone y, index) the creature walks through (path search
// §4.1 step 3), or is null.
using ObjectTest =
    std::function<bool(glm::dvec3 lo, glm::dvec3 hi, int axis, double move, const IgnoredStatics *ignored)>;

// §1: runs the world step for a frame of `dtMs`, in pieces of at most 20 ms.
void worldStep(Body &body, int32_t dtMs, const BlockType &block, const ObjectTest &objects = {},
               float *riderEnergy = nullptr);
// §2: one piece. A role-five actor can refill its resolved owner's meter during terrain contact.
void step(Body &body, int32_t dtMs, const BlockType &block, const ObjectTest &objects = {},
          float *riderEnergy = nullptr);

// §1 test helper: the pieces a frame is cut into.
std::vector<int32_t> stepPieces(int32_t dtMs);
// §0: the box conversion of a fixed coordinate to a block index: truncate toward zero, then
// one less when negative (so an exact negative multiple of 65536 goes one block lower).
int32_t boxBlock(int64_t fixed);

} // namespace creatures
