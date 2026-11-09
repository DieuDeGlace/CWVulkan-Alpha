#include "TerrainLights.h"
#include <algorithm>
#include <bit>

namespace terrain_lights {
namespace {
int64_t add(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b)); }
int64_t subtract(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); }
uint64_t key(int32_t x, int32_t y) { return uint64_t(uint32_t(x)) << 32 | uint32_t(y); }
bool inWorld(glm::ivec2 chunk) {
    return uint32_t(chunk.x) < 0x80000u && uint32_t(chunk.y) < 0x80000u;
}
} // namespace

std::optional<FixedPosition> fromBlocks(glm::dvec3 position) {
    FixedPosition result;
    for (int axis = 0; axis < 3; ++axis) {
        const double scaled = position[axis] * 65536.0;
        if (!(scaled >= -0x1p63 && scaled < 0x1p63))
            return std::nullopt;
        result[axis] = static_cast<int64_t>(scaled);
    }
    return result;
}

glm::vec3 renderPosition(const FixedPosition &position, const FixedPosition &shift) {
    glm::vec3 result;
    for (int axis = 0; axis < 3; ++axis)
        result[axis] = float(add(position[axis], shift[axis])) * (1.f / 65536.f);
    return result;
}

float priority(const FixedPosition &position, const FixedPosition &reference) {
    int64_t sum = 0;
    for (size_t axis = 0; axis < 3; ++axis) {
        const int64_t delta = subtract(position[axis], reference[axis]);
        const int64_t squared = std::bit_cast<int64_t>(uint64_t(delta) * uint64_t(delta));
        sum = add(sum, squared / 65536);
    }
    return float(sum) * (1.f / 65536.f);
}

void order(std::vector<Candidate> &candidates) {
    // Compared separately with original finite-key runtime observations. Larger
    // mixed ties remain a bounded verification claim, not an assumed stable sort.
    std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
        return a.priority < b.priority;
    });
}

ShaderLights shaderPrefix(const Selection &selection) {
    ShaderLights upload;
    std::copy_n(selection.positions.begin(), UPLOADED, upload.positions.begin());
    std::copy_n(selection.colours.begin(), UPLOADED, upload.colours.begin());
    return upload;
}

glm::vec3 FadingSource::fadedColour() const {
    const float fraction = float(ageMs) / float(durationMs);
    const float fade = 1.f - fraction;
    return colour * (fade < 0.f ? 0.f : fade);
}

void FadingSources::advance(int32_t elapsedMs) {
    for (auto &source : sources)
        source.ageMs = std::bit_cast<int32_t>(uint32_t(source.ageMs) + uint32_t(elapsedMs));
    // Equality survives, including its zero-colour slot. Defined integer wrap is
    // an engine safety policy; native wrap/exceptional states are not certified.
    std::erase_if(sources, [](const FadingSource &source) { return source.ageMs > source.durationMs; });
}

const std::vector<Selection> &Selector::rebuild(std::span<const Candidate> ordered, std::span<const Chunk> chunks,
                                              const FixedPosition &shift) {
    if (cachedChunks.size() != chunks.size() || !std::equal(cachedChunks.begin(), cachedChunks.end(), chunks.begin())) {
        cachedChunks.assign(chunks.begin(), chunks.end());
        index.clear();
        index.reserve(chunks.size());
        first = {0x80000, 0x80000};
        last = {-1, -1};
        for (size_t i = 0; i < chunks.size(); ++i)
            if (inWorld(chunks[i].coordinate)) {
                const auto at = chunks[i].coordinate;
                index.emplace_back(key(at.x, at.y), i);
                first = glm::min(first, at);
                last = glm::max(last, at);
            }
        std::sort(index.begin(), index.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    }
    selections.resize(chunks.size());
    for (size_t i = 0; i < chunks.size(); ++i) {
        selections[i].mesh = chunks[i].mesh;
        selections[i].count = 0;
        selections[i].positions.fill(glm::vec4(0));
        selections[i].colours.fill(glm::vec4(0));
    }
    if (index.empty())
        return selections;

    for (const Candidate &candidate : ordered) {
        const float scaledRadius = candidate.radius * 65536.f;
        // Exceptional radius conversion is outside the native ordinary-input
        // contract. Reject it safely at this engine interface.
        if (!(double(scaledRadius) >= -0x1p63 && double(scaledRadius) < 0x1p63))
            continue;
        const int64_t radius = static_cast<int64_t>(scaledRadius);
        const int64_t x0 = std::max<int64_t>(first.x, subtract(candidate.position[0], radius) / 65536 / 32);
        const int64_t x1 = std::min<int64_t>(last.x, add(candidate.position[0], radius) / 65536 / 32);
        const int64_t y0 = std::max<int64_t>(first.y, subtract(candidate.position[1], radius) / 65536 / 32);
        const int64_t y1 = std::min<int64_t>(last.y, add(candidate.position[1], radius) / 65536 / 32);
        for (int64_t x = x0; x <= x1; ++x)
            for (int64_t y = y0; y <= y1; ++y) {
                const uint64_t coordinate = key(int32_t(x), int32_t(y));
                const auto found = std::lower_bound(index.begin(), index.end(), coordinate,
                    [](const auto &entry, uint64_t value) { return entry.first < value; });
                if (found == index.end() || found->first != coordinate)
                    continue;
                Selection &selection = selections[found->second];
                if (selection.count >= CAPACITY)
                    continue;
                const glm::vec4 position(renderPosition(candidate.position, shift), candidate.radius);
                const size_t slot = selection.count++;
                selection.positions[slot] = position;
                selection.colours[slot] = glm::vec4(candidate.colour, 0);
            }
    }
    return selections;
}

} // namespace terrain_lights
