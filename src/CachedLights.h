#pragma once
// Bounded current/pending sprite-light state from the held lifecycle contract.
// Engine job and geometry readiness adapters do not certify native scheduling.
#include "TerrainLights.h"
#include <map>

namespace worldgen { struct Sprite; }

namespace terrain_lights {

struct CachedSource {
    FixedPosition position{};
    glm::vec3 colour{0};
    int32_t kind = 0;
    uint32_t flags = 0;
};

// Preserve the supplied sprite sequence, including unlit entries, after the
// native two-stage truncating chunk membership calculation. Unused presentation
// metadata and native sector-list construction remain separate dependencies.
std::vector<CachedSource> chunkSources(std::span<const worldgen::Sprite> sprites, glm::ivec2 chunk);

class CachedSources {
  public:
    void stage(glm::ivec2 chunk, std::vector<CachedSource> sources);
    // Both gates are explicit. Copying pending to current does not consume it.
    bool refresh(glm::ivec2 chunk, bool cacheMiss, size_t pendingGeometryCount);
    void reset(glm::ivec2 chunk);
    void append(std::vector<Candidate> &out, const ViewAdmission &view, int32_t clockMs, float daylight) const;

  private:
    struct State { std::vector<CachedSource> current, pending; };
    std::map<std::pair<int32_t, int32_t>, State> chunks;
};

} // namespace terrain_lights
