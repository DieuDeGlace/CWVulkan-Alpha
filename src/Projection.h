#pragma once
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

namespace projection {

struct Inputs {
    float angle, signedAspect, nearPlane, farPlane;
};
// Inputs for the held main native producer. The supplied word's upstream
// receiver and camera/view production remain separate dependencies.
Inputs mainInputs(uint16_t word, int32_t width, int32_t height);
using Tangent = std::function<double(double)>;
double platformTangent(double angle);
// Native physical component layout, interpreted here as consecutive columns.
// This uses positive view Z and a negative horizontal coefficient for the main
// positive-dimension inputs. No validation/clamps are added to its arithmetic.
glm::mat4 perspective(const Inputs &inputs, const Tangent &tangent = platformTangent);
glm::mat4 multiply(const glm::mat4 &left, const glm::mat4 &right);
float flipSign(float value);
// Explicit engine adapter: reverse view X/Z for the existing RH camera and
// reverse clip Y for Vulkan. This does not establish native view equivalence.
glm::mat4 rightHandedVulkan(const glm::mat4 &nativeProjection);

} // namespace projection
