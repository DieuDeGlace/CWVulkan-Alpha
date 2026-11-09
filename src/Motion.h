#pragma once
// Things that move with the world clock.
//
// Alpha's own plant sway (docs/specs/vegetation-wind.md §3-§4): models whose record has flag 4
// (grass, reeds, underwater plants, lava grass; never flowers) lean back and forth by a shear
// that grows with height. AlphaVulkan's built-in additions (tree leaves and cloud drift, which
// Alpha keeps still, §1 and §6) are added with the world.
#include <glm/glm.hpp>
#include <cstdint>

class Config;

namespace motion {

// The record flag that makes a model sway (§3.1).
constexpr uint32_t SWAYS = 4;

// §4.1: the two sway factors for clock `t` (world milliseconds) and the model's phase index.
float swayA(double t, int phase);
float swayB(double t, int phase);

// §4.1-§4.2: the shear applied to a swaying model, in its own voxel units after its yaw and
// before its size scale. A vertex `z` voxels above the base moves by
// (-0.1 b z / (H size), 0.1 a z / (H size), 0) voxels, i.e. by up to 0.2 blocks at the top
// whatever the plant's size. `heightVoxels` is H, the model's z size.
glm::mat4 plantShear(double t, int phase, float heightVoxels, float size);

// AlphaVulkan's cloud drift (Alpha's clouds stand still, vegetation-wind.md §6). The rule of the
// user's AlphaCWWorldMotion mod: a slow back-and-forth of up to `distance` blocks in x and 40 %
// of it in y, so clouds never wander away from the cells they belong to. `speed` scales the
// rate (the mod ships 2.5).
glm::vec2 cloudDrift(double seconds, float distance, float speed);

// AlphaVulkan's leaf sway (Alpha's tree leaves stand still): the AlphaCWWorldMotion rule, one
// shared offset for every leaf corner, `amplitude` blocks at most.
glm::vec2 leafOffset(double seconds, float amplitude);

// [Motion] in AlphaVulkan.ini.
struct MotionSettings {
    bool clouds = true;          // cloud drift
    float cloudDistance = 32;    // blocks
    float cloudSpeed = 2.5f;
    bool leaves = true;          // leaf sway
    float leafAmplitude = 0.12f; // blocks
    float leafRange = 96;        // full sway this far from the camera, fading over 32 more
    float windSpeed = 1;
    void load(const Config &config);
};

} // namespace motion
