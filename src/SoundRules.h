#pragma once
// Positional playback and pre-sort terrain ambience from the held audio and terrain-slot contracts.
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace sound {

using Fixed = std::array<int64_t, 3>; // 1/65536 block

// §0: the random pitch a + k·u, evaluated in single precision as float(r) · k / 32767 + a, in
// that order (r one C-runtime rand() result).
inline float randomPitch(int32_t r, float k, float a) { return float(r) * k / 32767.f + a; }

// §1.3: a sound record carries its position as float blocks; the frame converts each to fixed
// point (x 65536, truncated toward zero) before playing. A double block position is first
// rounded to the record's floats.
Fixed recordPosition(const glm::vec3 &blocks);
inline Fixed recordPosition(const glm::dvec3 &blocks) { return recordPosition(glm::vec3(blocks)); }
// Engine adapter: an engine position held in double blocks (not a record), truncated toward
// zero to fixed point.
Fixed fixedOf(const glm::dvec3 &blocks);

// §1.2 steps 1-3: (1 - d/100)·v for a sound at `at` heard at the listener, or nothing past the
// cut-off d² > 10 000 (d² = 10 000 exactly continues and ends at 0). The per-axis 64-bit
// difference is converted to the nearest float, scaled by 1/65536, summed as (Δy² + Δx²) + Δz²
// in float; d is the double square root rounded to float.
std::optional<float> attenuation(const Fixed &at, const Fixed &listener, float volume);
// §1.2 step 6: the voice volume (soundVolume · a) · 0.01, `soundVolume` the integer option.
inline float voiceVolume(int32_t soundVolume, float a) { return (float(soundVolume) * a) * 0.01f; }
// §1.1: the frequency ratio XAudio2 plays for a requested pitch, clamped to [1/1024, 2].
float playedPitch(float pitch);

// One play request of the play routine (§1.2): id, position, volume, pitch.
struct Request {
    int32_t id = 0;
    Fixed position{};
    float volume = 1, pitch = 1;
};

// §3 tick test: whether a record at fixed (x, y) ticks this frame for period P, world clock T
// and frame step dt (64-bit truncating block division, its low 32 bits, C remainders and 32-bit
// signed truncating division).
bool ambienceTicks(int64_t x, int64_t y, int32_t period, int32_t clock, int32_t dt);

// A record of a chunk's published list that can sound: its kind and fixed position.
struct AmbientRecord {
    int32_t kind = 0;
    Fixed position{};
};

// A world step updates worldClock independently of dayClock. The renderer compares this
// clock against one full frame increment ahead, once per published decoration.
struct AmbienceFrame {
    int32_t dayClock = 0, worldClock = 0, elapsed = 0;
    std::function<int32_t()> draw;
    std::function<void(const Request &)> dispatch;

    std::optional<Request> request(const AmbientRecord &record) const;
};

} // namespace sound
