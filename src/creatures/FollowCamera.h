#pragma once
// The production follow camera. Targets, smoothing, anchor following, corner
// sampling and shake use the held camera contracts through CameraFollow and
// CameraView, the held ray query through creatures/Visibility, and the held
// constructed values. The world-clock source and the render origin used for
// horizontal rebasing are explicit engine adapters.
#include "creatures/CameraFollow.h"
#include "creatures/Physics.h"
#include "creatures/Visibility.h"

namespace creatures {

struct CameraOptions {
    int speed = 50, smoothness = 80; // the constructed option defaults
    bool invertY = false;
};

class FollowCamera {
  public:
    CameraOptions options;
    float pitch = 120, yaw = 0;             // current first and third angles (degrees)
    float targetPitch = 120, targetYaw = 0; // targets
    // Constructed values per the held initial-state contract: both distances
    // are zero until a front-end state, button handler or the wheel moves them.
    float distance = 0, targetDistance = 0;
    // Shake amplitude, set to 1 by explosion and punch sounds, easing back to 0.
    float shake = 0;

    // Relative mouse counts in look mode.
    void look(float dx, float dy);
    // One wheel notch, +1 up (zoom in).
    void wheel(int notches);
    // Once per rendered frame, after the frame's world calls. `elapsedMs` is the
    // whole frame duration; `clock` supplies the engine world clock twice.
    // `renderOrigin` is the engine's horizontal rebasing origin in raw units.
    void update(const Body &player, int32_t elapsedMs, const visibility::Scene &scene, const camera_view::Clock &clock,
                const camera_view::Fixed3 &renderOrigin = {});
    // The constructed view relative to the render origin, shake included.
    const camera_view::Matrix &view() const { return rebased; }

    // The view direction and the camera position (blocks).
    glm::vec3 direction() const;
    glm::dvec3 position() const;
    // Camera-relative input directions: forward f and right r.
    glm::vec3 forward() const;
    glm::vec3 right() const;

  private:
    camera_follow::State state() const;
    void store(const camera_follow::State &state);

    // The anchor is constructed as zero; the first update's distance snap moves
    // it to the followed creature.
    camera_view::Fixed3 anchor{}, eye{};
    camera_view::Matrix rebased = camera_view::identity();
};

} // namespace creatures
