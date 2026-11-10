#pragma once
#include "creatures/RouteGeometry.h"
#include <map>
#include <set>
#include <vector>

namespace creatures::search {

using Cell = std::array<int32_t, 3>;

// Labels describe the held classifier's geometric tests, not terrain types.
// Pair order is minus/plus; corner order is --, +-, -+, ++.
struct Classification {
    bool lower = false, upper = false, sampleBit = false;
    std::array<bool, 2> faceX{}, faceY{}, lowerX{}, lowerY{}, upperX{}, upperY{};
    std::array<bool, 4> lowerCorner{}, verticalCorner{}, recordSide{};
    bool recordBelow = false;

    // Candidate order: X-/X+/Y-/Y+, four corners, Z-/Z+.
    // Diagonals require actual cardinal insertions/improvements this step.
    bool allows(size_t candidate, const std::array<bool, 4> &updated) const;
};
using ClassificationLookup = std::function<Classification(const Cell &)>;

Cell coordinate(std::span<const int64_t, 3> raw);
int32_t destinationScore(const Cell &from, const Cell &to);

struct Node {
    int32_t accumulated = 0, destination = 0, selection = 0;
    Cell parent{};
    bool operator==(const Node &) const = default;
};

// Independently owned supplied-input search state. No native allocation layout,
// pointer lifetime, whole-frame schedule or server equivalence is implied.
struct State {
    route::Position start{}, goal{};
    Cell anchor{}, reference{-1, -1, 0};
    float radius = 0;
    int32_t counter = 0;
    std::map<Cell, Node> records;
    std::set<Cell> pending;
    std::optional<Cell> remembered;
    std::vector<Cell> path;
    std::set<uintptr_t> exclusions;

    // Retains endpoints, radius, counter, anchor and reference.
    void clear();
    void setup(std::span<const float, 3> size, const route::RecordLookup &records = {});
    // Exactly one selection/expansion. False also covers a popped exact goal.
    // Geometry is called only for an admitted non-goal selection.
    bool step(const ClassificationLookup &classify);
    // Retains partial chains and the specified count guard, including repeats.
    void rebuild();

  private:
    void publish(const Cell &key, const Node &value);
};

} // namespace creatures::search
