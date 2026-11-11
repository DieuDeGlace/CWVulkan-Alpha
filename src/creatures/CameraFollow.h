#pragma once
// Camera input targets, per-update smoothing and the late follow/collision
// caller from the held docs/specs/alpha-camera-follow-collision.md contract.
// Voxel classification, the ray query, widget gates and the followed creature
// are supplied inputs; their native producers remain open dependencies.
#include "creatures/CameraView.h"
#include <optional>

namespace camera_follow {

using camera_view::Fixed3;
using camera_view::Trig;

struct State {
    Fixed3 eye{}, anchor{};
    std::array<float, 3> angles{}, targetAngles{};
    float distance = 0, targetDistance = 0;
    float scale = 0, targetScale = 0;
    float retainedDistance = 0; // the copy written before the final eye correction
};

// The sensitivity product precedes each input product. Only the first target is
// bounded; the third has no wrap and the second is never written.
void mouseTargets(State &state, int32_t sensitivity, int32_t verticalDirection, float u, float v);
// The window-message route keeps only the sign of the signed high word.
int32_t wheelMessage(int16_t highWord);
// Distance route when `scaleMode` is false, otherwise the scale route, where a
// zero argument still divides.
void wheel(State &state, int32_t argument, bool scaleMode);

using Power = std::function<double(double, double)>;
double platformPower(double base, double exponent); // adapter, not a native accuracy claim
// Angles, then distance, then scale, each with the same signed count.
void smooth(State &state, int32_t count, int32_t smoothingOption, const Power &power = platformPower);

// The unnormalized backward-offset direction for the current angles.
glm::vec3 offsetDirection(const std::array<float, 3> &angles, const Trig &trig);

struct Subject {
    Fixed3 position{};
    float verticalOffset = 0; // subtracted from the position's third component
    glm::vec3 extent{0};
    uint8_t action = 0;
};
struct Column {
    int32_t base, extent;
};
struct Alternative {
    glm::vec3 anchor; // the world's three-float value
    // Receives the two truncated integer coordinates; empty for a missing column.
    std::function<std::optional<Column>(int32_t x, int32_t y)> column;
};
struct AnchorGates {
    const Alternative *alternative = nullptr; // any of the four first properties nonzero
    bool verticalOverride = false;            // either of the two later properties nonzero
};
// The ordinary route's raw vertical target for the subject's action.
int64_t verticalTarget(const Subject &subject, bool verticalOverride);
// Distance snap, then either ordinary following or the alternative anchor.
void followAnchor(State &state, const Subject &subject, int32_t count, const AnchorGates &gates = {});

// Negative raw coordinates subtract one unit before the truncating division.
int32_t voxelCoordinate(int64_t raw);
// True when the record's low five classification bits are neither 0 nor 2.
bool blocks(uint8_t classification);

struct Queries {
    std::function<bool(const Fixed3 &)> blocked;
    // Start, direction and maximum distance; both native option arguments are 1.
    std::function<double(const Fixed3 &start, glm::vec3 direction, float maximum)> ray;
};
// Eight corner samples from the first eye, distance retention, recomputed eye
// and the final centre correction. The caller adds no clamp to query results.
void placeEye(State &state, const Fixed3 &subjectPosition, glm::vec3 direction, const Queries &queries);

} // namespace camera_follow
