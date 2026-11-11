#include "creatures/FollowCamera.h"
#include <algorithm>
#include <cmath>

namespace creatures {
namespace {

using camera_view::Fixed3;

constexpr double DEG = 3.141592653589793 / 180.0;
constexpr double STEP = .05; // engine ray-march resolution, blocks

glm::dvec3 blocksOf(const Fixed3 &raw) { return glm::dvec3(double(raw[0]), double(raw[1]), double(raw[2])) / 65536.0; }

bool blockedAt(const BlockType &block, glm::dvec3 at) {
    return camera_follow::blocks(block(int32_t(std::floor(at.x)), int32_t(std::floor(at.y)), int32_t(std::floor(at.z))));
}

// Engine stand-in for the unresolved native ray query: the distance to the
// first change of blocked state. From a blocked start it reports how far the
// direction stays inside blocks; from a free start it reports the last free
// distance before the first blocked cell, or a negative value when nothing
// blocks within the maximum.
camera_follow::Queries engineQueries(const BlockType &block) {
    camera_follow::Queries queries;
    queries.blocked = [&block](const Fixed3 &at) {
        return camera_follow::blocks(block(camera_follow::voxelCoordinate(at[0]), camera_follow::voxelCoordinate(at[1]),
                                           camera_follow::voxelCoordinate(at[2])));
    };
    queries.ray = [&block](const Fixed3 &start, glm::vec3 direction, float maximum) {
        const glm::dvec3 from = blocksOf(start), along(direction);
        if (blockedAt(block, from)) {
            double inside = 0;
            while (inside < maximum && blockedAt(block, from + along * inside))
                inside += STEP;
            return inside;
        }
        for (double travelled = 0; travelled < maximum; travelled += STEP)
            if (blockedAt(block, from + along * travelled))
                return std::max(travelled - .1, 0.0);
        return -1.0;
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

glm::dvec3 FollowCamera::position() const {
    // Engine adapter for the native view-coordinate shake translation: the
    // right-handed engine view mirrors x, so the eye moves with x and against y.
    const glm::dvec3 r(right()), up = glm::cross(r, glm::dvec3(direction()));
    return blocksOf(eye) + r * double(offset.x) - up * double(offset.y);
}

glm::vec3 FollowCamera::forward() const {
    const double p = yaw * DEG;
    return glm::vec3((float)std::sin(p), (float)-std::cos(p), 0);
}

glm::vec3 FollowCamera::right() const {
    const double p = yaw * DEG;
    return glm::vec3((float)-std::cos(p), (float)-std::sin(p), 0);
}

void FollowCamera::update(const Body &player, int32_t elapsedMs, const BlockType &block, const camera_view::Clock &clock) {
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
    camera_follow::placeEye(s, subject.position, camera_follow::offsetDirection(s.angles, trig), engineQueries(block));
    if (!(s.distance >= 0.f)) // engine bound on its own ray adapter's result
        s.distance = 0.f;
    store(s);

    // The view uses the amplitude from before this frame's decay.
    offset = camera_view::shake(shake, clock, trig);
    shake = camera_view::decayedAmplitude(shake, elapsedMs);
}

} // namespace creatures
