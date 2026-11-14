#include "creatures/AngleStep.h"
#include <cmath>

namespace creatures {

float angleStep(float from, float to, float factor) {
    constexpr double PI = 3.141592653589793; // binary64 0x400921FB54442D18
    // Degrees / 180 in binary32, then x pi in binary64, rounded to binary32.
    const auto radians = [](float degrees) { return float(double(degrees / 180.f) * PI); };
    const auto rounded = [](double v) { return double(float(v)); };
    const float a = radians(from), b = radians(to);
    const double ca = rounded(std::cos(double(a))), sa = rounded(std::sin(double(a)));
    const double cb = rounded(std::cos(double(b))), sb = rounded(std::sin(double(b)));
    float s = float(sb * ca - cb * sa);
    if (s > 1.f)
        s = 1.f;
    else if (-1.f > s)
        s = -1.f;
    const double turn = rounded(std::asin(double(s)));
    const double scale = (double(factor) * 180.0) / PI;
    return float(turn * scale) + from;
}

} // namespace creatures
