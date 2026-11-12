#pragma once
// The local player's aim and controller flags, from the held
// docs/specs/alpha-controller-aim-writers.md contract (client controller update 0x088EE0).
// Pure functions of supplied camera, player and creature state; the caller (Player) owns the
// order within a frame: after the frame's world calls and before the attack input.
//
// The pick's reach, box and target id follow docs/specs/alpha-head-pitch-and-pick.md.
// Engine adapters (the contracts leave these open):
//   * the pick point's vertical adjustment (creature +0x1180, a drawn-height lag the engine
//     does not keep) is zero;
//   * the projection after the view matrix (client +0x800A90) is taken as a centred
//     perspective with a positive w, so the straddle test reads the view-space signs directly;
//   * admission routine 0x1988A0 is the engine's hostility stand-in, in either direction;
//   * the light byte is the engine's propagated light (Zone::light) for clear cells.
#include "creatures/CameraView.h"
#include "creatures/Combat.h"
#include "creatures/Physics.h"
#include "creatures/Visibility.h"
#include <functional>
#include <span>

namespace creatures::controller_aim {

using camera_view::Fixed3;

// A creature the pick may choose (§3.2): its +0x1350 point, its size and its entity position
// (+0x10, where the pick samples the light; head-pitch-and-pick contract §2.1).
struct Candidate {
    const Combatant *fighter = nullptr;
    Fixed3 point{};
    glm::vec3 size{0};
    Fixed3 position{};
};

// Head-pitch-and-pick contract §2.1: the daylight factor of a time of day (milliseconds) and
// the pick's maximum distance for a block light value (0..255).
float daylight(int32_t timeOfDayMs);
float pickRange(uint8_t light, float daylightFactor);
// The block light query (0x0718B0) over the engine's held zones: 255 for glowing, open sky and
// implicit water, 0 for solid or missing, else the propagated light raised to at least 5.
using LightQuery = std::function<uint8_t(int32_t x, int32_t y, int32_t z)>;

// The camera as the controller reads it: V before normalization, the follow anchor F0, the eye
// E, the shaken view matrix and the origin it is rebased on.
struct CameraState {
    glm::vec3 forward{0, 1, 0};
    Fixed3 anchor{}, eye{}, origin{};
    camera_view::Matrix view = camera_view::identity();
};

struct AimPoint {
    Fixed3 point{};
    uint64_t hovered = 0; // C+0x800A70: the nearest creature straddling the screen centre
    uint64_t nearest = 0; // the third id pair: the straddling creature nearest the player
};

// §3.1 and §3.2: the camera ray from F0 (60 units, else 100 along V), then the nearest
// admitted creature under the screen centre, measured from the eye. Each candidate's
// visibility reach comes from the light at its position and the daylight factor; without a
// light query it is 256 (full light).
AimPoint aimPoint(const CameraState &camera, const Fixed3 &playerPosition, const Combatant &player,
                  std::span<const Candidate> candidates, const visibility::Scene &scene,
                  float daylightFactor = 1.f, const LightQuery &light = {});

// §3.2 target id: while action 0x22 is before its wind-up end, the nearest straddling creature
// becomes the healing stream's target; Left Shift targets the player itself (written last).
void writeHealTarget(Combatant &player, const AimPoint &aim, bool shift);

// §3.2, close-spell freeze for actions 0x1E-0x21: the point is held from the update in which
// clock - elapsed first exceeds the wind-up.
void freezeSpellPoint(Fixed3 &point, Fixed3 &held, const Combatant &player, int32_t elapsedMs);

// §3.4, free control: with Left Shift the unit forward of the yaw, otherwise the offset from
// the player to the aim point.
glm::vec3 freeAim(bool shift, float yawDegrees, const Fixed3 &point, const Fixed3 &playerPosition);

// §4: the flags block on the mode word (face-aim 0x04, 0x40, 0x01). `glide` is the engine's
// value for bit 0x40 on the gliding route (bit 0x10 set and off the ground).
struct FlagInputs {
    bool shift = false, ctrl = false;
    bool gliding = false;     // bit 0x10 set and +0x5C bit 0 clear
    bool glideLift = false;   // that route's value: Space with stamina above zero
};
uint32_t flags(uint32_t mode, const Combatant &player, const FlagInputs &in);

} // namespace creatures::controller_aim
