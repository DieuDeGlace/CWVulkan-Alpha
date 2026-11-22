#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <bit>
#include <cstdint>

// The indexed base and lid each use their own grid dimensions and the same object width.
class ChestPose {
  public:
    ChestPose(glm::vec3 anchor, float width, int32_t orientation)
        : anchor(anchor), width(width), yaw(rotation(float(std::bit_cast<int32_t>(uint32_t(orientation) * 90u)), {0,0,1})) {}

    glm::mat4 base(glm::vec3 grid) const {
        const auto placed = glm::translate(glm::mat4(1), anchor);
        const auto oriented = glm::scale(placed, glm::vec3(width / grid.x)) * yaw;
        return glm::translate(oriented, {-grid.x * .5f, -grid.y * .5f, 0});
    }

    glm::mat4 lid(glm::vec3 grid, float fraction) const {
        const auto pivot = glm::translate(base(grid), {0, 0, 9});
        const auto opened = pivot * rotation(80.f * fraction, {1,0,0});
        return glm::translate(opened, {0, 0, -1});
    }

  private:
    static glm::mat4 rotation(float degrees, glm::dvec3 axis) {
        const float radians = degrees * 0.01745329238474369f;
        return glm::mat4(glm::rotate(glm::dmat4(1), double(radians), axis));
    }
    glm::vec3 anchor;
    float width;
    glm::mat4 yaw;
};
