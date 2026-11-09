#pragma once
// The follow camera (docs/specs/alpha-physics-camera.md §7): mouse look with Alpha's angles
// (pitch θ 0..180 with 90 level, yaw ψ), smoothing, wheel zoom 0..14 blocks, a focus that
// trails the player, and the pull-in when blocks are in the way.
#include "creatures/Physics.h"

namespace creatures {

struct CameraOptions {
    int speed = 50, smoothness = 100;
    bool invertY = false;
};

class FollowCamera {
  public:
    CameraOptions options;
    float pitch = 120, yaw = 0;             // current θ, ψ (degrees)
    float targetPitch = 120, targetYaw = 0; // targets
    // The spec reads the starting distance as 0 (open question Q-10); a few blocks out shows
    // the player.
    float distance = 6, targetDistance = 6;
    glm::dvec3 focus{0};
    bool focusSet = false;
    // §7.3 step 11: the shake amplitude (+0x1e8), set to 1 by explosion and punch sounds
    // (audio §1.3) and easing back to 0.
    float shake = 0;

    // §7.2: relative mouse counts in look mode.
    void look(float dx, float dy);
    // §7.3 step 3: one wheel notch, +1 up (zoom in).
    void wheel(int notches);
    // §7.3: per frame. `head` is the player's position, box height and landing bob.
    void update(const Body &player, float dtMs, const BlockType &block);

    // §7.3 steps 6-7: the view direction and the camera position (blocks).
    glm::vec3 direction() const;
    glm::dvec3 position() const { return eye; }
    // Camera-relative input directions (§3.9): forward f and right r.
    glm::vec3 forward() const;
    glm::vec3 right() const;

  private:
    glm::dvec3 eye{0};
    double clockMs = 0; // drives the shake's wobble (Alpha uses the world clock)
};

} // namespace creatures
