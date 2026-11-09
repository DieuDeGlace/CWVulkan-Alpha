#pragma once
// Ordered queries over stable region samples. Instantiate in a strict-float
// translation unit. Native cache population and lifetime are separate contracts.
#include "worldgen/Random.h"
#include <glm/vec2.hpp>
#include <bit>
#include <cstdint>
#include <limits>
#include <optional>
#if defined(_M_X64) || defined(_M_IX86) || defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace worldgen::scalars {

struct Window {
    int32_t firstX, lastX, firstY, lastY;
};

inline int32_t wrapSubtract(int32_t a, int32_t b) {
    return std::bit_cast<int32_t>(uint32_t(a) - uint32_t(b));
}

inline Window window(int32_t x, int32_t y) {
    const auto lower = [](int32_t c) { return wrapSubtract(c, 16384) / 16384; };
    const auto upper = [](int32_t c) { return std::bit_cast<int32_t>(uint32_t(c) + 16384u) / 16384; };
    return {lower(x), upper(x), lower(y), upper(y)};
}

// The x64 target retains the scalar-SSE conversion, including its invalid flag
// and the caller's exception masks. The portable branch specifies results only.
inline int32_t truncate(float value) {
#if defined(_M_X64) || defined(_M_IX86) || defined(__SSE2__)
    return _mm_cvttss_si32(_mm_set_ss(value));
#else
    if (!(value >= -2147483648.f && value < 2147483648.f))
        return std::numeric_limits<int32_t>::min();
    return static_cast<int32_t>(value);
#endif
}

inline glm::ivec2 distort(int32_t x, int32_t y) {
    const float first = noise(double(y) * 0.0005, 3423.0);
    const float dx = (first * 3.f) * 256.f;
    const float second = noise(double(x) * 0.0005, 23421.0);
    const float dy = (second * 3.f) * 256.f;
    return {truncate(float(x) + dx), truncate(float(y) + dy)};
}

inline int32_t distance(int32_t x, int32_t y, glm::ivec2 query) {
    const int64_t fixedX = (int64_t(x) - int64_t(query.x)) * 65536;
    const int64_t fixedY = (int64_t(y) - int64_t(query.y)) * 65536;
    const float dx = float(fixedX) * (1.f / 65536.f);
    const float dy = float(fixedY) * (1.f / 65536.f);
    const float xx = dx * dx;
    const float yy = dy * dy;
    return truncate(yy + xx);
}

struct Result {
    float value;
    bool positiveTotal;
};

template <class Read, class Accumulate>
Result reduce(Window bounds, glm::ivec2 query, float fallback, Read read, Accumulate accumulate) {
    std::optional<int32_t> nearest;
    for (int32_t i = bounds.firstX; i <= bounds.lastX; ++i)
        for (int32_t j = bounds.firstY; j <= bounds.lastY; ++j) {
            const auto *sample = read(i, j);
            if (!sample)
                return {fallback, false};
            const int32_t current = distance(sample->x, sample->y, query);
            if (!nearest || current < *nearest)
                nearest = current;
        }
    if (!nearest)
        return {fallback, false};

    float total = 0.f, selected = 0.f;
    for (int32_t i = bounds.firstX; i <= bounds.lastX; ++i)
        for (int32_t j = bounds.firstY; j <= bounds.lastY; ++j) {
            const auto *sample = read(i, j);
            if (!sample)
                return {fallback, false};
            const int32_t current = distance(sample->x, sample->y, query);
            const int32_t delta = wrapSubtract(current, *nearest);
            float scaled = float(delta) * std::bit_cast<float>(0x350637bdu);
            if (scaled > 1.f)
                scaled = 1.f;
            const float weight = 1.f - scaled;
            total = total + weight;
            accumulate(*sample, weight, selected);
        }
    return total > 0.f ? Result{selected / total, true} : Result{fallback, false};
}

template <class Read, class Value>
Result average(int32_t x, int32_t y, float fallback, Read read, Value value) {
    const Window bounds = window(x, y);
    // Both warp calls precede admission, including the missing-sample path.
    const glm::ivec2 query = distort(x, y);
    return reduce(bounds, query, fallback, read, [&](const auto &sample, float weight, float &sum) {
        const float product = value(sample) * weight; // evaluate even for zero weight
        sum = sum + product;
    });
}

template <class Read, class Predicate>
Result fraction(int32_t x, int32_t y, Read read, Predicate predicate) {
    const Window bounds = window(x, y);
    bool found = false;
    for (int32_t i = bounds.firstX; i <= bounds.lastX; ++i)
        for (int32_t j = bounds.firstY; j <= bounds.lastY; ++j) {
            const auto *sample = read(i, j);
            if (!sample)
                return {0.f, false};
            if (predicate(*sample)) {
                found = true;
                break; // continue with the next row, not the next pass
            }
        }
    if (!found)
        return {0.f, false};
    return reduce(bounds, distort(x, y), 0.f, read, [&](const auto &sample, float weight, float &sum) {
        if (predicate(sample))
            sum = sum + weight;
    });
}

} // namespace worldgen::scalars
