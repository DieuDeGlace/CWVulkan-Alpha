#include "Projection.h"
#include "Frame.h"
#include <bit>
#include <cstdio>

static int failures = 0;
static void check(bool value, const char *message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
static uint32_t bits(float value) { return std::bit_cast<uint32_t>(value); }

static void inputsAndComponents() {
    const auto wide = projection::mainInputs(0xfbff, 1920, 1080);
    const auto narrow = projection::mainInputs(0xffff, 1920, 1080);
    check(bits(wide.angle) == 0x3f5f66f4 && bits(narrow.angle) == 0x3f060a92 &&
          bits(wide.nearPlane) == 0x3dcccccd && wide.farPlane == 2000,
          "main input constants use exact native bits and only the 0x0400 decision");
    const auto rounded = projection::mainInputs(0, 16777217, 16777218);
    check(bits(rounded.signedAspect) == 0xbf7ffffe, "signed integer dimensions narrow separately before aspect division and sign-bit inversion");
    check(bits(projection::flipSign(0.f)) == 0x80000000 && bits(projection::flipSign(-0.f)) == 0,
          "sign operation flips signed zero rather than subtracting from zero");
    int calls = 0;
    const auto matrix = projection::perspective({1.f, -2.f, 1.f, 3.f}, [&](double half) {
        ++calls;
        check(half == 0.5, "tangent receives the promoted separately rounded half angle");
        return 2.00000005; // narrows to float two before reciprocal
    });
    check(calls == 1 && matrix[0][0] == -0.25f && matrix[1][1] == 0.5f && matrix[2][2] == 1.5f &&
          matrix[2][3] == 1.f && matrix[3][2] == -1.5f, "projection has the supplied tangent rounding, depth arithmetic and physical component layout");
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            if (!((column == row && column < 3) || (column == 2 && row == 3) || (column == 3 && row == 2)))
                check(bits(matrix[column][row]) == 0, "every unused perspective component is exact positive zero");
    const auto nearClip = matrix * glm::vec4(0, 0, 1, 1);
    const auto farClip = matrix * glm::vec4(0, 0, 3, 1);
    check(nearClip.z == 0 && nearClip.w == 1 && farClip.z / farClip.w == 1,
          "finite exact anchor has positive-view-z denominator and zero-to-one depth");
}

static void productsAndAdapter() {
    glm::mat4 left(0), right(0);
    for (int i = 0; i < 4; ++i) left[i][0] = 1;
    right[0] = {1e20f, 3.f, -1e20f, 4.f};
    check(projection::multiply(left, right)[0][0] == 4.f, "dot sum adds the third and fourth terms after the rounded first pair");
    left = glm::mat4(0); right = glm::mat4(0);
    left[0][0] = 0x1.000002p0f; left[1][0] = -1.f;
    right[0][0] = 0x1.fffffcp-1f; right[0][1] = 1.f;
    check(bits(projection::multiply(left, right)[0][0]) == 0, "matrix products round before summation without fused cancellation");
    const auto native = projection::perspective({1.f, -2.f, 1.f, 3.f}, [](double) { return 2.; });
    const auto adapted = projection::rightHandedVulkan(native);
    const glm::vec4 enginePoint(2, 3, -4, 1);
    auto expected = native * glm::vec4(-enginePoint.x, enginePoint.y, -enginePoint.z, 1);
    expected.y = -expected.y;
    check(adapted * enginePoint == expected && adapted[2][3] == -1,
          "explicit RH/Vulkan adapter changes view X/Z and clip Y while retaining the depth offset");
    Camera camera;
    camera.aspect = 2;
    const auto live = camera.projection(1, 3, 90);
    check(live[0][0] == 0.5f && live[1][1] == -1.f && live[2][2] == -1.5f && live[2][3] == -1 && live[3][2] == -1.5f,
          "production Camera projection reaches the scalar constructor and explicit Vulkan adapter");
}

int main() {
    inputsAndComponents(); productsAndAdapter();
    if (!failures) std::puts("PASS: projection input widths, tangent/depth boundaries, matrix rounding and production adapter");
    return failures ? 1 : 0;
}
