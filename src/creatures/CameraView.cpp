#include "creatures/CameraView.h"
#include <bit>
#include <cmath>

namespace camera_view {
namespace {

constexpr int64_t UNIT = 65536;

float constant(uint32_t bits) { return std::bit_cast<float>(bits); }
float flipSign(float value) { return std::bit_cast<float>(std::bit_cast<uint32_t>(value) ^ 0x80000000u); }
int64_t wrap(uint64_t value) { return std::bit_cast<int64_t>(value); }
uint64_t raw(int64_t value) { return std::bit_cast<uint64_t>(value); }

struct Rounded {
    float cosine, sine;
};
Rounded rounded(float degrees, const Trig &trig) {
    const float radians = degrees * constant(0x3c8efa35);
    const double wide = double(radians);
    const float cosine = float(trig.cosine(wide));
    const float sine = float(trig.sine(wide));
    return {cosine, sine};
}

// first' = first*c + second*s, second' = second*c - first*s, from saved values.
void mixTowardFirst(std::array<float, 4> &first, std::array<float, 4> &second, Rounded angle) {
    for (int i = 0; i < 4; ++i) {
        const float a = first[i], b = second[i];
        const float bs = b * angle.sine, ac = a * angle.cosine;
        const float bc = b * angle.cosine, as = a * angle.sine;
        first[i] = bs + ac;
        second[i] = bc - as;
    }
}

float cosineOfRaw(float argument, const Trig &trig) { return float(trig.cosine(double(argument))); }

void rotateInverse(Matrix &matrix, const std::array<float, 3> &angles, const Trig &trig) {
    rotateRows12(matrix, flipSign(angles[0]), trig);
    rotateRows02(matrix, flipSign(angles[1]), trig);
    rotateRows01(matrix, flipSign(angles[2]), trig);
}

} // namespace

Trig platformTrig() {
    return {[](double angle) { return std::cos(angle); }, [](double angle) { return std::sin(angle); }};
}

glm::vec3 toFloat(const Fixed3 &value) {
    const float scale = constant(0x37800000);
    glm::vec3 result;
    for (int i = 0; i < 3; ++i) {
        const float narrowed = float(value[i]);
        result[i] = narrowed * scale;
    }
    return result;
}

int64_t toFixed(float value) {
    const float scaled = value * 65536.f;
    return int64_t(scaled);
}

Fixed3 toFixed(glm::vec3 value) { return {toFixed(value.x), toFixed(value.y), toFixed(value.z)}; }

Fixed3 add(const Fixed3 &left, const Fixed3 &right) {
    return {wrap(raw(left[0]) + raw(right[0])), wrap(raw(left[1]) + raw(right[1])), wrap(raw(left[2]) + raw(right[2]))};
}

Fixed3 subtract(const Fixed3 &left, const Fixed3 &right) {
    return {wrap(raw(left[0]) - raw(right[0])), wrap(raw(left[1]) - raw(right[1])), wrap(raw(left[2]) - raw(right[2]))};
}

int64_t negate(int64_t value) { return wrap(uint64_t(0) - raw(value)); }

Fixed3 opposite(const Fixed3 &value) {
    Fixed3 result;
    for (int i = 0; i < 3; ++i)
        result[i] = wrap(raw(value[i]) * raw(-UNIT)) / UNIT;
    return result;
}

Matrix identity() {
    Matrix matrix{};
    for (int i = 0; i < 4; ++i)
        matrix[i][i] = 1.f;
    return matrix;
}

void rotateRows12(Matrix &matrix, float degrees, const Trig &trig) { mixTowardFirst(matrix[1], matrix[2], rounded(degrees, trig)); }

void rotateRows02(Matrix &matrix, float degrees, const Trig &trig) {
    // The opposite mixing sign: row zero loses and row two gains the sine term.
    const Rounded angle = rounded(degrees, trig);
    for (int i = 0; i < 4; ++i) {
        const float a = matrix[0][i], b = matrix[2][i];
        const float ac = a * angle.cosine, bs = b * angle.sine;
        const float bc = b * angle.cosine, as = a * angle.sine;
        matrix[0][i] = ac - bs;
        matrix[2][i] = bc + as;
    }
}

void rotateRows01(Matrix &matrix, float degrees, const Trig &trig) { mixTowardFirst(matrix[0], matrix[1], rounded(degrees, trig)); }

void translate(Matrix &matrix, glm::vec3 offset) {
    for (int i = 0; i < 4; ++i) {
        const float x = offset.x * matrix[0][i], y = offset.y * matrix[1][i];
        float sum = x + y;
        const float z = offset.z * matrix[2][i];
        sum = sum + z;
        matrix[3][i] = sum + matrix[3][i];
    }
}

void translate(Matrix &matrix, float x, float y) {
    for (int i = 0; i < 4; ++i) {
        const float first = x * matrix[0][i], second = y * matrix[1][i];
        const float sum = first + second;
        matrix[3][i] = sum + matrix[3][i];
    }
}

glm::vec3 transformDirection(const Matrix &matrix, glm::vec3 direction) {
    glm::vec3 result;
    for (int i = 0; i < 3; ++i) {
        const float x = direction.x * matrix[0][i], y = direction.y * matrix[1][i];
        const float sum = x + y;
        const float z = direction.z * matrix[2][i];
        result[i] = sum + z;
    }
    return result;
}

float weight(int32_t count, float coefficient) {
    const double wide = double(coefficient);
    double value = 0;
    for (int32_t i = 0; i < count; ++i) {
        const double remaining = 1.0 - value;
        const double step = remaining * wide;
        const double next = value + step;
        if (next == value) // a fixed point: every later approach repeats it exactly
            break;
        value = next;
    }
    return float(value);
}

float blend(float previous, float target, float amount) {
    const float complement = 1.f - amount;
    const float kept = complement * previous;
    const float gained = target * amount;
    return kept + gained;
}

Shake shake(float amplitude, const Clock &clock, const Trig &trig) {
    const float half = amplitude * .5f;
    const float firstArgument = float(clock()) * constant(0x3d23d70a);
    const float firstCosine = cosineOfRaw(firstArgument, trig);
    const float y = firstCosine * half;
    const float secondArgument = float(clock()) * constant(0x3cf5c28f);
    const float secondCosine = cosineOfRaw(secondArgument, trig);
    const float x = secondCosine * half;
    return {x, y};
}

float decayedAmplitude(float amplitude, int32_t frameDuration) {
    return blend(amplitude, 0.f, weight(frameDuration, constant(0x3ba3d70a)));
}

Views construct(const Inputs &inputs, const Clock &clock, const Trig &trig) {
    Views views;
    views.companion = identity();
    translate(views.companion, toFloat(inputs.eye));
    rotateRows01(views.companion, inputs.angles[2], trig);
    rotateRows02(views.companion, inputs.angles[1], trig);
    rotateRows12(views.companion, inputs.angles[0], trig);

    const Fixed3 oppositeEye = opposite(inputs.eye);
    views.absolute = identity();
    rotateInverse(views.absolute, inputs.angles, trig);
    translate(views.absolute, toFloat(oppositeEye));

    views.rebaseX = negate(inputs.anchor[0]);
    views.rebaseY = negate(inputs.anchor[1]);

    views.rebased = identity();
    const Shake offset = shake(inputs.amplitude, clock, trig);
    translate(views.rebased, offset.x, offset.y);
    rotateInverse(views.rebased, inputs.angles, trig);
    // The horizontal rebasing is a raw subtraction before any float conversion.
    translate(views.rebased, toFloat(subtract(oppositeEye, Fixed3{views.rebaseX, views.rebaseY, 0})));
    return views;
}

} // namespace camera_view
