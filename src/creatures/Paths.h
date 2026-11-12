#pragma once
#include "creatures/RouteFollowing.h"

namespace creatures {

// Engine services around the held supplied-input search and route consumer.
class PathSearch {
  public:
    using Cell = search::Cell;
    search::State state;
    route::RecordLookup records;
    route::ColumnLookup columns;
    std::function<void(const route::Record &)> activate;
    // Physics still addresses objects by owner/ordinal. This derived adapter is
    // populated from the same endpoint scans, never a second search authority.
    IgnoredStatics ignored;

    void start(glm::dvec3 from, glm::dvec3 to, float radius, glm::vec3 size);
    void setup(glm::vec3 size);
    void round(glm::vec3 size, const BlockType &block);
    void rounds(glm::vec3 size, float stop, const BlockType &block);
    void clear();
    bool empty() const { return state.path.empty(); }
    bool searching() const { return !state.records.empty(); }
    int32_t nodeCount() const { return int32_t(state.records.size()); }
    bool follow(Body &body, int32_t elapsed, const BlockType &block);
    // Original world caller's false/nonempty composition; no normalization.
    glm::vec3 fallback(const Body &body) const;
};
} // namespace creatures
