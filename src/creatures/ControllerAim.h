#pragma once
// The local player's aim and controller flags, from the held
// docs/specs/alpha-controller-aim-writers.md contract (client controller update 0x088EE0).
// Pure functions of supplied camera, player and creature state; the caller (Player) owns the
// order within a frame: after the frame's world calls and before the attack input.
//
// Engine adapters (the contract leaves these open):
//   * the pick's visibility maximum, which the original derives from a column query, is 200;
//   * the pick point's vertical adjustment (creature +0x1180) is zero;
//   * the projection after the view matrix is taken as sign-preserving on both screen axes
//     (a centred perspective), so the straddle test reads the view-space signs directly;
//   * admission routine 0x1988A0 is the engine's hostility stand-in, in either direction.
#include "creatures/CameraView.h"
#include "creatures/Combat.h"
#include "creatures/Physics.h"
#include "creatures/Visibility.h"
#include <span>

namespace creatures::controller_aim {

using camera_view::Fixed3;

// A creature the pick may choose (§3.2): its +0x1350 point and its size.
struct Candidate {
    const Combatant *fighter = nullptr;
    Fixed3 point{};
    glm::vec3 size{0};
};

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
};

// §3.1 and §3.2: the camera ray from F0 (60 units, else 100 along V), then the nearest
// admitted creature under the screen centre, measured from the eye.
AimPoint aimPoint(const CameraState &camera, const Fixed3 &playerPosition, const Combatant &player,
                  std::span<const Candidate> candidates, const visibility::Scene &scene);

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
