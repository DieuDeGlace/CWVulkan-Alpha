#pragma once
// A creature's procedural pose (docs/specs/alpha-creatures-models.md §7): the walk cycle kept
// per creature (§7.3), the smoothed pose channels that the action poses drive (§7.7, §7.8,
// Appendix C), and each frame the body parts and the equipment placed from them (§6.5, §7.9).
//
// Weapon trails follow alpha-trails-poses-schedules.md §1.
#include "creatures/Appearance.h"
#include "creatures/Combat.h"
#include <array>
#include <vector>

namespace creatures {

// §7.1: what the pose is made from.
struct Motion {
    glm::vec3 velocity{0}; // blocks per second
    glm::vec3 intent{0};
    bool onGround = true, inWater = false;
    bool running = false;   // mode 0x40 with intent and speed
    bool climbing = false;  // mode 0x01 and touching a wall
    bool gliding = false;   // mode 0x10
    bool sitting = false;   // mode 0x400
    bool lantern = false;   // mode 0x200
    float yawDegrees = 0;   // displayed body rotation about z
    float height = 0;       // displayed z in blocks (weapon trails stay at the heights they were made)
    float dodgeMs = 0;      // record field 15
    bool knockedDown = false; // record field 16 > 0
    // The creature's action, equipment and class; without one the creature only stands and
    // walks.
    const Combatant *fighter = nullptr;
};

// §7.3, §7.7: the state kept from frame to frame.
struct PoseState {
    float phase = 0, amplitude = 0; // walk cycle
    bool fresh = true;              // channels not smoothed yet
    // §7.7 channels.
    glm::vec3 handL{0}, handR{0}, turnL{0}, turnR{0}; // ΔL, ΔR, ρL, ρR
    glm::vec3 torso{0}, tilt{0}, shift{0}, headShift{0}, head{0}, tail{0}, roll{0}, lean{0}; // T B S K H Q U E
    float aim = 0, pivot = 0; // Y, h
    // The hand targets of the last frame, base plus channels (for tests and attachments).
    glm::vec3 rightHand{0}, leftHand{0};
    // Trails §1.1: four histories of 16 points, point 0 the newest: main tip, main butt, off
    // tip, off butt. In blocks from the creature's displayed position in x and y; z from
    // `trailHeight`, the height they were last moved to.
    std::array<std::array<glm::vec3, 16>, 4> trail{};
    float trailHeight = 0;
};

// Trails §1.3: one corner of a trail triangle, in world blocks, and its alpha (the colour is
// white).
struct TrailCorner {
    glm::vec3 position;
    uint8_t alpha;
};

// One model drawn for a part: `matrix` takes the model's mesh (centred on x/y, base at z = 0,
// voxel units) to blocks relative to the creature's position.
struct PartDraw {
    uint16_t model = NO_MODEL;
    glm::mat4 matrix{1};
    bool mirrored = false; // draw the mirror image in x (the right hand, §7.9.4)
    bool hairTint = false; // multiply by the hair colour (§5)
    // Item services §3.9: an attached cube, one voxel of `colour`, drawn instead of `model`.
    bool cube = false;
    glm::vec3 colour{1};
    bool shine = false; // materials §1.2: an iron, gold, silver or ice item
};

// §6.2, §6.3: the model of an item, or NO_MODEL.
uint16_t itemModel(const Item &item);
// §7.2: the pose of an action.
int32_t poseOf(const Combatant &c);

// §7.3: advances the walk cycle by dt milliseconds. True when a foot came down: the phase
// passed a multiple of π while walking faster than 5 blocks/s, not dodging and not in water.
bool stepWalk(PoseState &state, const Appearance &a, const Motion &m, float dtMs);
// §7.4–§7.9: the parts of this frame. `voxels(model)` gives a model's size in voxels.
std::vector<PartDraw> poseParts(PoseState &state, const Appearance &a, const Motion &m, float dtMs,
                                const std::function<glm::vec3(uint16_t)> &voxels);
// Trails §1.3: the creature's trail strips as triangles (three corners each), standing at
// `position`. Drawn from the history before this frame's poseParts ages it, as Alpha does.
void trailTriangles(const PoseState &state, const Combatant &c, glm::vec3 position, std::vector<TrailCorner> &out);

} // namespace creatures
