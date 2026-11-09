#include "SectorLights.h"
#include "WorldItems.h"
#include "worldgen/Zone.h"
#include <bit>
#include <limits>

namespace terrain_lights {
namespace {
FixedPosition raised(FixedPosition at, int64_t height) {
    at[2] = std::bit_cast<int64_t>(uint64_t(at[2]) + uint64_t(height));
    return at;
}
bool index(float value, int32_t &out) {
    if (!(double(value) >= -0x1p31 && double(value) < 0x1p31))
        return false;
    out = int32_t(value);
    return true;
}
} // namespace

SectorRange sectorRange(const FixedPosition &controlled, float reach) {
    SectorRange result;
    for (int axis = 0; axis < 2; ++axis) {
        const int32_t column = std::bit_cast<int32_t>(uint32_t(controlled[axis] / 65536));
        const float centre = float(column);
        const float low = (centre - reach) * (1.f / 256.f);
        const float high = (centre + reach) * (1.f / 256.f);
        if (!index(low, result.first[axis]) || !index(high, result.last[axis]))
            return {};
    }
    return result;
}

std::optional<Candidate> staticSource(int32_t kind, const FixedPosition &base, float height, float daylight,
                                       const ViewAdmission &view) {
    if (kind != 50 && kind != 51 && kind != 65)
        return std::nullopt;
    if (kind != 50 && !view.accepts(base, 32))
        return std::nullopt;
    const auto offset = fromBlocks({0, 0, double(height)});
    if (!offset)
        return std::nullopt;
    const FixedPosition position = raised(base, (*offset)[2]);
    if (kind == 50 && !view.accepts(position, 20))
        return std::nullopt;
    const glm::vec3 colour = kind == 50 ? glm::vec3(1.2f, 1, .8f) : glm::vec3(1.2f, .8f, .4f);
    return Candidate{position, colour * (1.f - daylight), 20, 0};
}

std::optional<Candidate> itemSource(uint8_t category, uint8_t subtype, const FixedPosition &base, int32_t hidden) {
    if (hidden != 0)
        return std::nullopt;
    glm::vec3 colour;
    if (category == 11 && subtype == 19)
        colour = {.1f, .3f, .3f};
    else if (category == 18 && subtype == 0)
        colour = {.6f, .6f, .2f};
    else if (category == 18 && subtype == 1)
        colour = {0, .6f, .2f};
    else
        return std::nullopt;
    return Candidate{raised(base, 3 * 65536), colour, 16, 0};
}

void SectorSources::update(const worldgen::Zone &zone) {
    auto &sources = sectors[{zone.zx, zone.zy}];
    sources.clear();
    for (const auto &object : zone.statics)
        if (object.kind == 50 || object.kind == 51 || object.kind == 65)
            sources.push_back({{object.x, object.y, object.z}, object.scale[2], object.kind});
}

void SectorSources::append(std::vector<Candidate> &out, const WorldItems &items, const ViewAdmission &view,
                           const FixedPosition &controlled, float daylight) const {
    const SectorRange range = sectorRange(controlled, view.reach());
    // Iterating present keys in lexicographic order skips missing lookups while
    // retaining the native X-then-Y visit order, without scanning empty space.
    for (auto it = sectors.lower_bound({range.first.x, std::numeric_limits<int32_t>::min()});
         it != sectors.end() && it->first.first <= range.last.x; ++it) {
        const auto [x, y] = it->first;
        if (!range.contains(x, y))
            continue;
        for (const auto &source : it->second)
            if (auto light = staticSource(source.kind, source.position, source.height, daylight, view))
                out.push_back(*light);
        if (const auto *list = items.zone(x, y))
            for (const auto &item : *list) {
                const FixedPosition at{item.position[0], item.position[1], item.position[2]};
                if (auto light = itemSource(item.item[0], item.item[1], at, item.hideMs);
                    light && view.accepts(light->position, 16))
                    out.push_back(*light);
            }
    }
}

} // namespace terrain_lights
