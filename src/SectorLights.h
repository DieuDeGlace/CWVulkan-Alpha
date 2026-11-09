#pragma once
// Sector object/item source rules from alpha-terrain-light-source-lifecycle.md.
// Cached chunk sources and creature attachments have separate lifecycle gates.
#include "TerrainLights.h"
#include <map>

class WorldItems;
namespace worldgen { struct Zone; }

namespace terrain_lights {

struct SectorRange {
    glm::ivec2 first{0}, last{-1};
    bool contains(int32_t x, int32_t y) const {
        return x >= first.x && x <= last.x && y >= first.y && y <= last.y;
    }
};
SectorRange sectorRange(const FixedPosition &controlled, float reach);

std::optional<Candidate> staticSource(int32_t kind, const FixedPosition &base, float height, float daylight,
                                      const ViewAdmission &view);
std::optional<Candidate> itemSource(uint8_t category, uint8_t subtype, const FixedPosition &base, int32_t hidden);

class SectorSources {
  public:
    // Retain a key even without static lights: it identifies a loaded sector.
    void update(const worldgen::Zone &zone);
    void remove(int32_t x, int32_t y) { sectors.erase({x, y}); }
    // Append into the common collection after earlier source groups. Read live
    // item state once, independent of model availability or draw distance.
    void append(std::vector<Candidate> &out, const WorldItems &items, const ViewAdmission &view,
                const FixedPosition &controlled, float daylight) const;

  private:
    struct StaticLight {
        FixedPosition position;
        float height;
        int32_t kind;
    };
    std::map<std::pair<int32_t, int32_t>, std::vector<StaticLight>> sectors;
};

} // namespace terrain_lights
