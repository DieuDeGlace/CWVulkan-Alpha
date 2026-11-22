#include "CachedLights.h"
#include "worldgen/WorldMap.h"
#include <bit>
#include <cmath>

namespace terrain_lights {
namespace {
float radius(int32_t clockMs, int32_t phase) {
    const float clock = float(clockMs) * .01f;
    const float angle = clock + float(phase);
    // Preserve the imported-call/narrowing boundaries. The platform cosine is
    // an explicit engine adapter; original runtime/ambient controls stay open.
    const float wave = float(std::cos(double(angle)));
    const float half = wave * .5f;
    return half + 15.f;
}
void appendSource(std::vector<Candidate> &out, const CachedSource &source, const ViewAdmission &view,
                  int32_t clockMs, float dayFactor, int32_t &phase) {
    if (!(source.flags & 1) || !view.accepts(source.position, 16))
        return;
    phase = std::bit_cast<int32_t>(uint32_t(phase) + 33u);
    const glm::vec3 colour = source.kind == 13 ? source.colour * dayFactor : source.colour;
    out.push_back({source.position, colour, radius(clockMs, phase), 0});
}
} // namespace

std::vector<CachedSource> chunkSources(std::span<const worldgen::Sprite> sprites, glm::ivec2 chunk) {
    std::vector<CachedSource> sources;
    for (const auto &sprite : sprites)
        if (worldgen::decorationChunkOf(sprite.x) == chunk.x && worldgen::decorationChunkOf(sprite.y) == chunk.y)
            sources.push_back({{sprite.x, sprite.y, sprite.z}, {sprite.tint[0], sprite.tint[1], sprite.tint[2]},
                                sprite.kind, uint32_t(sprite.flags)});
    return sources;
}

void visitDecorations(std::vector<Candidate> &lights, std::span<const worldgen::PositionedDecoration> records,
                      const ViewAdmission &view, float daylight, std::optional<float> slotSquared,
                      const sound::AmbienceFrame &frame) {
    const bool ambience = slotSquared && *slotSquared < 4096.f && frame.draw && frame.dispatch;
    int32_t phase = 0;
    const float dayFactor = 1.f - daylight * .5f;
    for (const auto &record : records) {
        const auto &s = record.sprite;
        if (s.flags & 1)
            appendSource(lights, {{s.x, s.y, s.z}, {s.tint[0], s.tint[1], s.tint[2]}, s.kind, uint32_t(s.flags)},
                         view, frame.worldClock, dayFactor, phase);
        if (ambience)
            if (const auto request = frame.request({s.kind, {s.x, s.y, s.z}}))
                frame.dispatch(*request);
    }
}

void CachedSources::stage(glm::ivec2 chunk, std::vector<CachedSource> sources) {
    chunks[{chunk.x, chunk.y}].pending = std::move(sources);
}
bool CachedSources::refresh(glm::ivec2 chunk, bool cacheMiss, size_t pendingGeometryCount) {
    const auto entry = chunks.find({chunk.x, chunk.y});
    if (entry == chunks.end() || !cacheMiss || pendingGeometryCount == 0)
        return false;
    entry->second.current = entry->second.pending;
    return true;
}
void CachedSources::reset(glm::ivec2 chunk) { chunks.erase({chunk.x, chunk.y}); }

void CachedSources::append(std::vector<Candidate> &out, const ViewAdmission &view, int32_t clockMs, float daylight) const {
    const float halfDay = daylight * .5f;
    const float dayFactor = 1.f - halfDay;
    for (const auto &[chunk, state] : chunks) {
        if (uint32_t(chunk.first) >= 0x80000u || uint32_t(chunk.second) >= 0x80000u)
            continue;
        int32_t phase = 0;
        for (const auto &source : state.current)
            appendSource(out, source, view, clockMs, dayFactor, phase);
    }
}

} // namespace terrain_lights
