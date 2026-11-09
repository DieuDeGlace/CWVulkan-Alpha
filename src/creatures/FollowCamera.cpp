#include "creatures/FollowCamera.h"
#include <algorithm>
#include <cmath>

namespace creatures {
namespace {

constexpr double DEG = 3.141592653589793 / 180.0;

// Blend(x → t, r): w = 1 − (1 − r)^dt (§0).
double weight(double r, float dt) { return 1.0 - std::pow(1.0 - r, (double)dt); }
bool solid(uint8_t type) { return type != 0 && type != 2; }
bool solidAt(const BlockType &block, glm::dvec3 p) {
    return solid(block((int32_t)std::floor(p.x), (int32_t)std::floor(p.y), (int32_t)std::floor(p.z)));
}

} // namespace

void FollowCamera::look(float dx, float dy) {
    const float s = (float)options.speed * 0.005f;
    targetPitch = std::clamp(targetPitch + (options.invertY ? -s : s) * dy, 0.f, 180.f);
    targetYaw -= s * dx;
}

void FollowCamera::wheel(int notches) {
    const int n = notches > 0 ? 1 : notches < 0 ? -1 : 0;
    targetDistance = std::clamp(targetDistance - 2.f * (float)n, 0.f, 14.f);
}

glm::vec3 FollowCamera::direction() const {
    const double t = pitch * DEG, p = yaw * DEG;
    return glm::vec3((float)(std::sin(p) * std::sin(t)), (float)(-std::cos(p) * std::sin(t)), (float)std::cos(t));
}

glm::vec3 FollowCamera::forward() const {
    const double p = yaw * DEG;
    return glm::vec3((float)std::sin(p), (float)-std::cos(p), 0);
}

glm::vec3 FollowCamera::right() const {
    const double p = yaw * DEG;
    return glm::vec3((float)-std::cos(p), (float)-std::sin(p), 0);
}

void FollowCamera::update(const Body &player, float dt, const BlockType &block) {
    // Step 1: angles.
    const double k = std::pow(0.005, options.smoothness / 100.0);
    const double wa = weight(k, dt);
    pitch += (float)((targetPitch - pitch) * wa);
    yaw += (float)((targetYaw - yaw) * wa);
    // Step 2: distance.
    distance += (float)((targetDistance - distance) * weight(0.004, dt));
    // Step 5: focus, half a block above the head.
    const glm::dvec3 at = player.blocks();
    const double headZ = at.z - player.bob + player.size.z * 0.5 + 0.5;
    if (!focusSet || glm::dot(focus - at, focus - at) > 2500.0) {
        focus = {at.x, at.y, headZ};
        focusSet = true;
    }
    const double wxy = weight(0.025, dt);
    focus.x += (at.x - focus.x) * wxy;
    focus.y += (at.y - focus.y) * wxy;
    focus.z += (headZ - focus.z) * weight(0.005, dt);
    // Steps 6-8: behind the focus, pulled in where a corner of the camera's cube is in a block.
    const glm::dvec3 d(direction());
    double dist = distance;
    glm::dvec3 p = focus - dist * d;
    for (int c = 0; c < 8; c++) {
        const glm::dvec3 corner = p + glm::dvec3(c & 1 ? 0.5 : -0.5, c & 2 ? 0.5 : -0.5, c & 4 ? 0.5 : -0.5);
        if (!solidAt(block, corner))
            continue;
        // How far along d the corner runs through solid blocks before it is free.
        double h = 0;
        while (h < distance && solidAt(block, corner + d * h))
            h += 0.05;
        dist = std::min(dist, distance - h);
    }
    dist = std::max(dist, 0.0);
    distance = (float)dist;
    p = focus - dist * d;
    // Step 9: still inside a block: the first solid cell from the player toward P.
    if (solidAt(block, p)) {
        const glm::dvec3 u = glm::normalize(p - at);
        for (double t = 0; t < 100; t += 0.05)
            if (solidAt(block, at + u * t)) {
                p = at + u * std::max(t - 0.1, 0.0);
                break;
            }
    }
    eye = p;
    // Step 11: the shake wobbles the view sideways and up, then dies away.
    clockMs += dt;
    if (shake > 0) {
        const glm::dvec3 up = glm::cross(glm::dvec3(right()), glm::dvec3(direction()));
        eye += glm::dvec3(right()) * (0.5 * shake * std::cos(0.04 * clockMs)) + up * (0.5 * shake * std::cos(0.03 * clockMs));
        shake = (float)(shake * std::pow(1 - 0.005, (double)dt));
        if (shake < 1e-4f)
            shake = 0;
    }
}

} // namespace creatures
