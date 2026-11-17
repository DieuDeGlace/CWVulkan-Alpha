#include "SoundRules.h"
#include "TerrainLights.h"
#include "worldgen/Zone.h"
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
constexpr double SIX = 21600000, NOON = 43200000, NINE_PM = 75600000, MIDNIGHT = 86400000;
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
    const int32_t bx = int32_t(x / 65536), by = int32_t((y + 333ll * 65536) / 65536); // low 32 bits
    const int32_t phase = bx % period + by % period;
    const int32_t before = add(phase, clock), after = add(before, dt);
    return before / period != after / period;
}

void ambientRecord(const AmbientRecord &record, double t, int32_t clock, int32_t dt,
                   const std::function<int32_t()> &draw, std::vector<Request> &out) {
    const int32_t kind = record.kind;
    // Draws in order: the chance, then (only when it passes) the pitch, then an owl's file.
    const auto call = [&](int32_t period, int32_t odds) {
        return ambienceTicks(record.position[0], record.position[1], period, clock, dt) && draw() % odds == 0;
    };
    if (t > SIX && t < NINE_PM && kind >= 61 && kind <= 63) { // birds 94, 95, 96 by day
        if (call(1000, 10))
            out.push_back({94 + (kind - 61), record.position, 0.1f, randomPitch(draw(), 0.2f, 0.9f)});
        return;
    }
    if (kind == 61) { // owls by night
        if (call(1000, 10)) {
            const float pitch = randomPitch(draw(), 0.2f, 0.5f);
            out.push_back({99 + draw() % 2, record.position, 0.1f, pitch});
        }
        return;
    }
    if (t > NOON && t < MIDNIGHT && kind >= 0 && kind <= 4 && call(5000, 70)) // crickets
        out.push_back({kind <= 1 ? 98 : 97, record.position, 0.05f, randomPitch(draw(), 0.2f, 0.9f)});
}

// The same fixed-point dot product and float scaling as the light-priority contract's.
float chunkSquaredDistance(const Fixed &midpoint, const Fixed &listener) {
    return terrain_lights::priority(midpoint, listener);
}

ChunkShape chunkShape(glm::ivec2 index, const Fixed &low, const Fixed &high) {
    ChunkShape shape;
    shape.index = index;
    for (size_t k = 0; k < 3; k++)
        shape.midpoint[k] = std::bit_cast<int64_t>(uint64_t(low[k]) + uint64_t(high[k])) / 2;
    const float dx = blocksBetween(high[0], shape.midpoint[0]);
    const float dy = blocksBetween(high[1], shape.midpoint[1]);
    const float dz = blocksBetween(high[2], shape.midpoint[2]);
    shape.radius = 1.5f * float(std::sqrt(double((dx * dx + dy * dy) + dz * dz)));
    return shape;
}

void AmbienceGrid::walk(glm::ivec2 origin, int32_t w, const Fixed &listener, const Source &source) {
    if (w <= 0)
        return;
    if (w != width) {
        width = w;
        slots.assign(size_t(w) * size_t(w), Slot{});
    }
    for (int32_t x = origin.x; x < origin.x + w; x++)
        for (int32_t y = origin.y; y < origin.y + w; y++) {
            if (x < 0 || y < 0 || x >= 0x80000 || y >= 0x80000)
                continue;
            Slot &slot = slots[size_t((y % w) * w + (x % w))];
            const glm::ivec2 here(x, y);
            const bool built = source.built(x, y);
            // Engine adapter: a chunk the engine has built for this position is published into
            // its slot when the walk reaches it (the native publication key is unresolved).
            if (built && (!slot.chunk || slot.chunk->index != here))
                slot.chunk = source.shape(x, y);
            if (built && source.visible(*slot.chunk))
                slot.squared = chunkSquaredDistance(slot.chunk->midpoint, listener);
            if (slot.chunk && withinAmbience(slot.squared))
                source.records(slot.chunk->index);
        }
}

void ZoneAmbience::records(const worldgen::Zone &zone, glm::ivec2 chunk, std::vector<AmbientRecord> &out) {
    Entry &e = zones[{zone.zx, zone.zy}];
    if (e.zone != &zone || e.posts != zone.songPosts.size() || e.plants != zone.plants.size()) {
        e.zone = &zone;
        e.posts = zone.songPosts.size();
        e.plants = zone.plants.size();
        for (auto &list : e.byChunk)
            list.clear();
        const int32_t first = zone.originX() / 32, firstY = zone.originY() / 32;
        const auto place = [&](int32_t kind, const Fixed &at) {
            // Ruling 1 §1.1: a record belongs to chunk trunc(trunc(x · 0.03125) / 65536).
            const int32_t cx = int32_t(int64_t(double(at[0]) * 0.03125) / 65536) - first;
            const int32_t cy = int32_t(int64_t(double(at[1]) * 0.03125) / 65536) - firstY;
            if (cx >= 0 && cx < 8 && cy >= 0 && cy < 8)
                e.byChunk[size_t(cx + 8 * cy)].push_back({kind, at});
        };
        for (const auto &post : zone.songPosts)
            place(61, post);
        // Vegetation §1.3: a plant stands at the centre of its column, (x + 0.5, y + 0.5, z).
        for (const worldgen::Plant &p : zone.plants)
            if (p.kind >= 0 && p.kind <= 4)
                place(p.kind, {int64_t(p.x) * 65536 + 32768, int64_t(p.y) * 65536 + 32768, int64_t(p.z) * 65536});
    }
    const int32_t i = chunk.x - zone.originX() / 32, j = chunk.y - zone.originY() / 32;
    if (i >= 0 && i < 8 && j >= 0 && j < 8) {
        const auto &list = e.byChunk[size_t(i + 8 * j)];
        out.insert(out.end(), list.begin(), list.end());
    }
}

void ZoneAmbience::retain(const std::function<bool(int32_t zx, int32_t zy)> &held) {
    std::erase_if(zones, [&](const auto &entry) { return !held(entry.first.first, entry.first.second); });
}

} // namespace sound
