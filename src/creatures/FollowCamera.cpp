#include "creatures/FollowCamera.h"
#include <algorithm>
#include <cmath>

namespace creatures {
namespace {

using camera_view::Fixed3;

constexpr double DEG = 3.141592653589793 / 180.0;
glm::dvec3 blocksOf(const Fixed3 &raw) { return glm::dvec3(double(raw[0]), double(raw[1]), double(raw[2])) / 65536.0; }

// The caller's own voxel test and the held ray query. Both camera calls pass
// option values one and one: the walk continues through blocking cells, and
// spatial-record side faces can shorten the result.
camera_follow::Queries sceneQueries(const visibility::Scene &scene) {
    camera_follow::Queries queries;
    queries.blocked = [&scene](const Fixed3 &at) {
        return camera_follow::blocks(scene.voxel(camera_follow::voxelCoordinate(at[0]), camera_follow::voxelCoordinate(at[1]),
                                                 camera_follow::voxelCoordinate(at[2])));
    };
    queries.ray = [&scene](const Fixed3 &start, glm::vec3 direction, float maximum) {
        return double(visibility::trace(start, {direction.x, direction.y, direction.z}, maximum, 1, 1, scene));
    };
    return queries;
}

} // namespace

camera_follow::State FollowCamera::state() const {
    camera_follow::State s;
    s.eye = eye;
    s.anchor = anchor;
    s.angles = {pitch, 0.f, yaw};
    s.targetAngles = {targetPitch, 0.f, targetYaw};
    s.distance = distance;
    s.targetDistance = targetDistance;
    return s;
}

void FollowCamera::store(const camera_follow::State &s) {
    eye = s.eye;
    anchor = s.anchor;
    pitch = s.angles[0];
    yaw = s.angles[2];
    targetPitch = s.targetAngles[0];
    targetYaw = s.targetAngles[2];
    distance = s.distance;
    targetDistance = s.targetDistance;
}

void FollowCamera::look(float dx, float dy) {
    camera_follow::State s = state();
    camera_follow::mouseTargets(s, options.speed, options.invertY ? 1 : 0, dx, dy);
    store(s);
}

void FollowCamera::wheel(int notches) {
    camera_follow::State s = state();
    camera_follow::wheel(s, notches > 0 ? 1 : notches < 0 ? -1 : 0, false);
    store(s);
}

glm::vec3 FollowCamera::direction() const {
    return camera_follow::offsetDirection({pitch, 0.f, yaw}, camera_view::platformTrig());
}

glm::dvec3 FollowCamera::position() const { return blocksOf(eye); }

glm::vec3 FollowCamera::forward() const {
    const double p = yaw * DEG;
    return glm::vec3((float)std::sin(p), (float)-std::cos(p), 0);
}

glm::vec3 FollowCamera::right() const {
    const double p = yaw * DEG;
    return glm::vec3((float)-std::cos(p), (float)-std::sin(p), 0);
}

void FollowCamera::update(const Body &player, int32_t elapsedMs, const visibility::Scene &scene,
                          const camera_view::Clock &clock, const camera_view::Fixed3 &renderOrigin) {
    const camera_view::Trig trig = camera_view::platformTrig();
    camera_follow::State s = state();
    camera_follow::smooth(s, elapsedMs, options.smoothness);

    camera_follow::Subject subject;
    subject.position = {player.position[0], player.position[1], player.position[2]};
    subject.verticalOffset = player.bob;
    subject.extent = player.size;
    subject.action = uint8_t(player.action);
    if (!anchorSet) {
        // Engine start: the first frame begins at the ordinary vertical target.
        s.anchor = {subject.position[0], subject.position[1], camera_follow::verticalTarget(subject, false)};
        anchorSet = true;
    }
    camera_follow::followAnchor(s, subject, elapsedMs);
    camera_follow::placeEye(s, subject.position, camera_follow::offsetDirection(s.angles, trig), sceneQueries(scene));
    store(s);

    // The view uses the amplitude from before this frame's decay. The engine
    // render origin takes the place of the followed anchor for rebasing.
    camera_view::Inputs inputs;
    inputs.eye = eye;
    inputs.anchor = {renderOrigin[0], renderOrigin[1], 0};
    inputs.angles = s.angles;
    inputs.amplitude = shake;
    rebased = camera_view::construct(inputs, clock, trig).rebased;
    shake = camera_view::decayedAmplitude(shake, elapsedMs);
}

} // namespace creatures
