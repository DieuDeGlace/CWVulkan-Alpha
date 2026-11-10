#include "Projection.h"
#include <bit>
#include <cmath>

namespace projection {

float flipSign(float value) { return std::bit_cast<float>(std::bit_cast<uint32_t>(value) ^ 0x80000000u); }

Inputs mainInputs(uint16_t word, int32_t width, int32_t height) {
    const float angle = std::bit_cast<float>((word & 0x0400) ? 0x3f060a92u : 0x3f5f66f4u);
    const float w = float(width), h = float(height);
    const float aspect = w / h;
    return {angle, flipSign(aspect), std::bit_cast<float>(0x3dcccccdu), 2000.f};
}

double platformTangent(double angle) { return std::tan(angle); }

glm::mat4 perspective(const Inputs &inputs, const Tangent &tangent) {
    const float halfAngle = inputs.angle * 0.5f;
    const float roundedTangent = float(tangent(double(halfAngle)));
    const float vertical = 1.f / roundedTangent;
    const float horizontal = vertical / inputs.signedAspect;
    const float difference = inputs.farPlane - inputs.nearPlane;
    const float product = inputs.nearPlane * inputs.farPlane;
    const float quotient = product / difference;
    const float offset = flipSign(quotient);
    const float depth = inputs.farPlane / difference;
    glm::mat4 result(0.f);
    result[0][0] = horizontal;
    result[1][1] = vertical;
    result[2][2] = depth;
    result[2][3] = 1.f;
    result[3][2] = offset;
    return result;
}

glm::mat4 multiply(const glm::mat4 &left, const glm::mat4 &right) {
    glm::mat4 result(0.f);
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row) {
            const float first = right[column][0] * left[0][row];
            const float second = right[column][1] * left[1][row];
            float sum = first + second;
            const float third = right[column][2] * left[2][row];
            sum = sum + third;
            const float fourth = right[column][3] * left[3][row];
            result[column][row] = sum + fourth;
        }
    return result;
}

glm::mat4 rightHandedVulkan(const glm::mat4 &nativeProjection) {
    glm::mat4 result = nativeProjection;
    // Only the perspective constructor's five occupied components are used;
    // its other components remain the constructor's exact positive zeros.
    result[0][0] = flipSign(result[0][0]);
    result[1][1] = flipSign(result[1][1]);
    result[2][2] = flipSign(result[2][2]);
    result[2][3] = flipSign(result[2][3]);
    return result;
}
} // namespace projection
