#include "SoundRules.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace sound {
namespace {
int64_t subtract(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); }
int32_t add(int32_t a, int32_t b) { return std::bit_cast<int32_t>(uint32_t(a) + uint32_t(b)); }
// A fixed-point difference as float blocks: nearest float, then x 1/65536 in float.
float blocksBetween(int64_t a, int64_t b) { return float(subtract(a, b)) * (1.f / 65536.f); }

// §3 table rows.
constexpr int32_t SIX = 21600000, NOON = 43200000, NINE_PM = 75600000, MIDNIGHT = 86400000;
} // namespace

Fixed recordPosition(const glm::vec3 &blocks) {
    return {int64_t(blocks.x * 65536.f), int64_t(blocks.y * 65536.f), int64_t(blocks.z * 65536.f)};
}

Fixed fixedOf(const glm::dvec3 &blocks) {
    return {int64_t(blocks.x * 65536.0), int64_t(blocks.y * 65536.0), int64_t(blocks.z * 65536.0)};
}

std::optional<float> attenuation(const Fixed &at, const Fixed &listener, float volume) {
    const float dx = blocksBetween(at[0], listener[0]);
    const float dy = blocksBetween(at[1], listener[1]);
    const float dz = blocksBetween(at[2], listener[2]);
    const float squared = (dy * dy + dx * dx) + dz * dz;
    if (squared > 10000.f)
        return std::nullopt;
    const float d = float(std::sqrt(double(squared)));
    return (1.f - d / 100.f) * volume;
}

float playedPitch(float pitch) { return std::clamp(pitch, 1.f / 1024.f, 2.f); }

bool ambienceTicks(int64_t x, int64_t y, int32_t period, int32_t clock, int32_t dt) {
    const int64_t shiftedY = std::bit_cast<int64_t>(uint64_t(y) + 333ull * 65536);
    const int32_t bx = int32_t(x / 65536), by = int32_t(shiftedY / 65536);
    const int32_t phase = bx % period + by % period;
    const int32_t before = add(phase, clock), after = add(before, dt);
    return before / period != after / period;
}

std::optional<Request> AmbienceFrame::request(const AmbientRecord &record) const {
    const int32_t kind = record.kind;
    // Draws in order: the chance, then (only when it passes) the pitch, then an owl's file.
    const auto call = [&](int32_t period, int32_t odds) {
        return ambienceTicks(record.position[0], record.position[1], period, worldClock, elapsed) && draw() % odds == 0;
    };
    if (dayClock > SIX && dayClock < NINE_PM && kind >= 61 && kind <= 63) { // birds 94, 95, 96 by day
        if (call(1000, 10))
            return Request{94 + (kind - 61), record.position, 0.1f, randomPitch(draw(), 0.2f, 0.9f)};
        return std::nullopt;
    }
    if (kind == 61) { // owls by night
        if (call(1000, 10)) {
            const float pitch = randomPitch(draw(), 0.2f, 0.5f);
            return Request{99 + draw() % 2, record.position, 0.1f, pitch};
        }
        return std::nullopt;
    }
    if (dayClock > NOON && dayClock < MIDNIGHT && kind >= 0 && kind <= 4 && call(5000, 70)) // crickets
        return Request{kind <= 1 ? 98 : 97, record.position, 0.05f, randomPitch(draw(), 0.2f, 0.9f)};
    return std::nullopt;
}

} // namespace sound
