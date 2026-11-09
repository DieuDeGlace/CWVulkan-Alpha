#pragma once
// How creatures find their way (docs/specs/alpha-path-search.md): the clear-route test that
// walks a copy of the box toward a target (§2), the weighted grid search over feet cells (§4)
// whose state lives in the creature between updates, and the follower that steers the
// creature along the found path every step (§6).
//
// Static objects block the clear-route walker when the caller gives them; the creature step
// opens doors (§6.5). Not in yet: statics in the search's own classification (§4.3).
#include "creatures/Physics.h"
#include <array>
#include <glm/glm.hpp>
#include <map>
#include <set>
#include <vector>

namespace creatures {

// §2: whether a creature with box `size` can walk from P to T (box centres, blocks), arriving
// within R; a swimmer does not step up.
bool clearRoute(glm::vec3 size, bool swimmer, glm::dvec3 P, glm::dvec3 T, float R, const BlockType &block,
                const ObjectTest &objects = {}, const IgnoredStatics *ignored = nullptr);

// Open points 2 §1: a static object as the search sees it: its centre on x and y and base on
// z (blocks), half its footprint, its height, its kind and its place (zone x, zone y, index).
struct SearchStatic {
    glm::dvec3 at{0};
    glm::dvec2 half{0};
    double height = 0;
    int32_t kind = 0;
    std::array<int32_t, 3> ref{};
    bool passable = false; // §0: traps, levers, and open doors, windows and gates
};
// The statics listed in the 8 x 8-block index cells ix0..ix1 x iy0..iy1, each handed to `visit`.
using StaticLookup = std::function<void(int32_t ix0, int32_t ix1, int32_t iy0, int32_t iy1,
                                        const std::function<void(const SearchStatic &)> &visit)>;

class PathSearch {
  public:
    // Open points 2 §1: the statics the search stands on and is blocked by (none when empty),
    // and the seat the creature walks to (RandomInteraction), which never blocks it.
    StaticLookup statics;
    std::array<int32_t, 3> seatTarget{-1, -1, 0};
    // §4.1 step 3: the statics touching the start and goal boxes, walked through by the search,
    // the clear-route test and the physics until the search state is emptied.
    IgnoredStatics ignored;
    using Cell = std::array<int32_t, 3>; // ordered by x, then y, then z
    struct Node {
        int32_t g = 0, h = 0, f = 0;
        Cell parent{};
    };

    // §4.1: a new search from `from` toward `to` (feet points in blocks) with arrival radius r.
    void start(glm::dvec3 from, glm::dvec3 to, float r);
    // §4.2, §4.5: one round (one cell expanded), then the path rebuilt. `size` is the box.
    void round(glm::vec3 size, const BlockType &block);
    // §4.6: up to ten rounds while the path is short and does not reach within `stop` of the
    // goal (blocks).
    void rounds(glm::vec3 size, float stop, const BlockType &block);
    // Empties the whole state (server 0x405330).
    void clear();
    bool empty() const { return path.empty(); }
    bool searching() const { return !nodes.empty(); }

    // §6: steers `body` along the path for a step of dt ms: sets intent (and velocity) and
    // the climbing flag. Returns false when there is nothing to follow.
    bool follow(Body &body, int32_t dtMs, const BlockType &block);

    std::vector<Cell> path; // start cell first, best node last
    Cell goal{}, startCell{};
    glm::dvec3 goalPosition{0};
    float radius = 0;
    float timerMs = 0;

  private:
    int32_t heuristic(const Cell &c) const;
    void rebuild();
    // §6.3 step 8: the search re-rooted at cell N where the creature stands.
    void reroot(const Cell &N);

    bool ignoredFound = false; // (worked out on the first round, when the box is known)
    std::map<Cell, Node> nodes;
    std::set<Cell> open;
    Cell best{};
};

} // namespace creatures
