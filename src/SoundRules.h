#pragma once
// The rules that turn a sound request into what is heard, and the ambient calls, from the held
// docs/specs/alpha-audio.md (re-certified 2026-11-16) with the held rulings
// alpha-rulings-audio-ui-sounds.md and alpha-rulings-zone-fields.md (ruling 1). Pure functions
// and a small slot table; playback itself is Audio's.
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace worldgen {
struct Zone;
}

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

// §3: one record's decision order, tests and draws (chance, then pitch, then the owl's file;
// nothing for a record that does not tick). `timeOfDay` in ms of the day; appends at most one
// request.
void ambientRecord(const AmbientRecord &record, double timeOfDay, int32_t clock, int32_t dt,
                   const std::function<int32_t()> &draw, std::vector<Request> &out);

// Zone-fields ruling 1 §1.3: |chunk midpoint - L|² in blocks² (fixed-point dot product with
// per-axis truncating /65536, the sum converted to float and scaled by 2^-16). No square root.
float chunkSquaredDistance(const Fixed &midpoint, const Fixed &listener);
// Ruling 1 §1.4: the record gate, strictly below 4096 blocks² (64 blocks); NaN fails.
inline bool withinAmbience(float squared) { return squared < 4096.f; }

// A built chunk as the ambience slot keeps it: its indices (32-block units), the midpoint of
// its bounds and its bounding radius 1.5·|midpoint - bound| (ruling 1 §1.2).
struct ChunkShape {
    glm::ivec2 index{-1};
    Fixed midpoint{};
    float radius = 0;
};
// Ruling 1 §1.2: the shape of a chunk with the two bound corners `low` and `high` (fixed point):
// midpoint (low + high) / 2 truncated toward zero, radius 1.5 times its float distance to `high`.
ChunkShape chunkShape(glm::ivec2 index, const Fixed &low, const Fixed &high);

// Ruling 1 §1.3-§1.4: the render routine's walk over the chunk slot grid. Each slot keeps the
// squared camera distance of its last refresh, which can be stale, and the chunk last published
// into it.
class AmbienceGrid {
  public:
    struct Source {
        // Whether the engine holds built geometry for chunk (x, y): the slot's "populated" test.
        std::function<bool(int32_t x, int32_t y)> built;
        // The shape of the built chunk (x, y), asked when a slot takes it in.
        std::function<ChunkShape(int32_t x, int32_t y)> shape;
        // The view test of a slot's chunk.
        std::function<bool(const ChunkShape &)> visible;
        // Walks the records of chunk `index` (a slot whose distance passes the gate).
        std::function<void(glm::ivec2 index)> records;
    };
    // One frame: x outer from `origin.x`, y inner from `origin.y`, each over `width` positions;
    // positions outside 0 <= x, y < 0x80000 are skipped; slot (y rem w)·w + (x rem w).
    void walk(glm::ivec2 origin, int32_t width, const Fixed &listener, const Source &source);

  private:
    struct Slot {
        std::optional<ChunkShape> chunk;
        // Engine adapter: the native value before a slot's first refresh is indeterminate; NaN
        // (which the gate rejects) stands in for it.
        float squared = std::numeric_limits<float>::quiet_NaN();
    };
    std::vector<Slot> slots;
    int32_t width = 0;
};

// The sounding records of held zones by chunk, in zone-list order: the tree pass's kind-61
// records (the tree grid runs before the plant pass), then plants of kinds 0-4 at their column
// centres. Other kinds never sound and are left out. Rebuilt when a zone's lists change.
class ZoneAmbience {
  public:
    void records(const worldgen::Zone &zone, glm::ivec2 chunk, std::vector<AmbientRecord> &out);
    // Lets go of zones no longer held (`held` answers for a zone's coordinates).
    void retain(const std::function<bool(int32_t zx, int32_t zy)> &held);

  private:
    struct Entry {
        const worldgen::Zone *zone = nullptr;
        size_t posts = 0, plants = 0;
        std::array<std::vector<AmbientRecord>, 64> byChunk; // 8 x 8 chunks, index i + 8 j
    };
    std::map<std::pair<int32_t, int32_t>, Entry> zones;
};

} // namespace sound
