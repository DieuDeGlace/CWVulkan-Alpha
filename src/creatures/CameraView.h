#pragma once
// Camera numerics and final view construction from the held
// docs/specs/alpha-camera-view-production.md contract. Everything here is a
// supplied-input relation: the caller owns the eye, anchor, angles, amplitude
// and clock. Imported trigonometry is a parameter because its native accuracy
// and floating controls remain open dependencies.
#include <array>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

namespace camera_view {

// Signed raw coordinates, 65,536 units per coordinate unit.
using Fixed3 = std::array<int64_t, 3>;
// Sixteen binary32 components described as four consecutive stored rows.
using Matrix = std::array<std::array<float, 4>, 4>;

struct Trig {
    std::function<double(double)> cosine, sine;
};
// Platform adapter; not a claim about the original runtime's results.
Trig platformTrig();

// Each component rounds to binary32 first and is then scaled by 1/65536.
glm::vec3 toFloat(const Fixed3 &raw);
// Each component is scaled in binary32 first and then truncated toward zero.
// Nonfinite and out-of-range inputs are outside the contract's ordinary domain.
int64_t toFixed(float value);
Fixed3 toFixed(glm::vec3 value);
Fixed3 add(const Fixed3 &left, const Fixed3 &right);      // low 64 bits
Fixed3 subtract(const Fixed3 &left, const Fixed3 &right); // low 64 bits
int64_t negate(int64_t raw);                              // two's complement, wraps
// Fixed multiplication by raw -65536: wrapped product, then signed division.
Fixed3 opposite(const Fixed3 &raw);

Matrix identity();
// Degree-angle row-pair rotations. Each requests cosine before sine once.
void rotateRows12(Matrix &matrix, float degrees, const Trig &trig); // angle component zero
void rotateRows02(Matrix &matrix, float degrees, const Trig &trig); // angle component one
void rotateRows01(Matrix &matrix, float degrees, const Trig &trig); // angle component two
// Only the fourth stored row changes.
void translate(Matrix &matrix, glm::vec3 offset);
void translate(Matrix &matrix, float x, float y);
// x*row0 + y*row1, then + z*row2; no translation and no normalization.
glm::vec3 transformDirection(const Matrix &matrix, glm::vec3 direction);

// A count-fold binary64 approach to one, narrowed once at the end. Nonpositive
// counts give zero.
float weight(int32_t count, float coefficient);
float blend(float previous, float target, float weight);

using Clock = std::function<int32_t()>;
struct Shake {
    float x, y;
};
// Reads the clock twice: first for y, then for x. No zero-amplitude shortcut.
Shake shake(float amplitude, const Clock &clock, const Trig &trig);
// The amplitude stored after the views have been built.
float decayedAmplitude(float amplitude, int32_t frameDuration);

struct Inputs {
    Fixed3 eye{}, anchor{};
    std::array<float, 3> angles{};
    float amplitude = 0;
};
struct Views {
    Matrix companion, absolute, rebased;
    int64_t rebaseX, rebaseY; // opposite anchor x/y, published after `absolute`
};
// Eleven cosine and nine sine requests in the contract's order.
Views construct(const Inputs &inputs, const Clock &clock, const Trig &trig);

} // namespace camera_view
