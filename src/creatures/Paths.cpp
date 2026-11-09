#include "creatures/Paths.h"
#include <algorithm>
#include <cmath>

namespace creatures {

namespace {

bool solid(uint8_t type) { return type != 0 && type != 2; }

int32_t cellOf(double v) { return (int32_t)std::floor(v); }

// The cells a box with centre c covers (physics §0).
struct Range {
    int32_t lo[3], hi[3];
};
Range rangeOf(glm::dvec3 c, glm::vec3 size) {
    Range r;
    for (int k = 0; k < 3; k++) {
        r.lo[k] = cellOf(c[k] - size[k] * 0.5);
        r.hi[k] = cellOf(c[k] + size[k] * 0.5);
    }
    return r;
}

bool anySolid(const Range &r, const BlockType &block) {
    for (int32_t x = r.lo[0]; x <= r.hi[0]; x++)
        for (int32_t y = r.lo[1]; y <= r.hi[1]; y++)
            for (int32_t z = r.lo[2]; z <= r.hi[2]; z++)
                if (solid(block(x, y, z)))
                    return true;
    return false;
}

// "Relax x toward t at rate r per ms", dt times.
float relax(float x, float t, float r, int32_t dt) { return t + (x - t) * std::pow(1.f - r, (float)dt); }

} // namespace

bool clearRoute(glm::vec3 size, bool swimmer, glm::dvec3 P, glm::dvec3 T, float R, const BlockType &block,
                const ObjectTest &objects, const IgnoredStatics *ignored) {
    const auto reached = [&](glm::dvec3 Q, bool strict) {
        const double d = glm::length(glm::dvec2(T - Q));
        return (strict ? d < R : d <= R) && std::abs(Q.z - T.z) < size.z + R;
    };
    if (reached(P, true))
        return true;
    const double D = std::min(glm::length(glm::dvec2(T - P)), 50.0);
    const int32_t n = (int32_t)(std::sqrt(D * D + 0.5625) + 1);
    glm::dvec3 Q = P;
    for (int32_t k = 0; k < n; k++) {
        if (reached(Q, false))
            return true;
        glm::dvec2 u = glm::dvec2(T - Q);
        if (glm::dot(u, u) > 0)
            u = glm::normalize(u);
        const glm::dvec3 step(u.x, u.y, -0.71);
        for (int axis = 0; axis < 3; axis++) {
            Q[axis] += step[axis];
            if (!anySolid(rangeOf(Q, size), block)) {
                // A static in the way stops this axis, with no step-up.
                const glm::dvec3 h = glm::dvec3(size) * 0.5;
                if (objects && objects(Q - h, Q + h, -1, 0, ignored))
                    Q[axis] -= step[axis];
                continue;
            }
            if (axis < 2 && !swimmer) { // a one-block step-up (the bottom ends 2.02 up, as read)
                Range up = rangeOf(Q + glm::dvec3(0, 0, 1.01), size);
                if (!anySolid(up, block)) {
                    const double bottom = std::floor(Q.z - size.z * 0.5) + 2.02;
                    Q.z = bottom + size.z * 0.5;
                    continue;
                }
            }
            Q[axis] -= step[axis];
        }
    }
    return glm::length(Q - P) > 25;
}

void PathSearch::clear() {
    nodes.clear();
    open.clear();
    path.clear();
    ignored.clear();
    ignoredFound = false;
}

int32_t PathSearch::heuristic(const Cell &c) const {
    const int32_t a = std::abs(c[0] - goal[0]), b = std::abs(c[1] - goal[1]), e = std::abs(c[2] - goal[2]);
    return 2 * (10 * std::max(a, b) + 4 * std::min(a, b) + 10 * e);
}

void PathSearch::start(glm::dvec3 from, glm::dvec3 to, float r) {
    clear();
    startCell = {cellOf(from.x), cellOf(from.y), cellOf(from.z)};
    goal = {cellOf(to.x), cellOf(to.y), cellOf(to.z)};
    goalPosition = to;
    radius = r;
    timerMs = 0;
    Node n;
    n.g = 1; // (every later g is one more, as read)
    n.h = heuristic(startCell);
    n.f = n.g + n.h;
    n.parent = startCell;
    nodes[startCell] = n;
    open.insert(startCell);
    best = startCell;
}

void PathSearch::rebuild() {
    path.clear();
    if (nodes.empty()) {
        path.push_back(goal);
        return;
    }
    Cell c = best;
    for (size_t guard = 0; guard <= nodes.size(); guard++) {
        path.push_back(c);
        const auto it = nodes.find(c);
        if (c == startCell || it == nodes.end() || it->second.parent == c)
            break;
        c = it->second.parent;
    }
    std::reverse(path.begin(), path.end());
}

void PathSearch::round(glm::vec3 size, const BlockType &block) {
    if (nodes.empty())
        return;
    if (open.size() >= 501 || nodes.size() >= 20001) { // abandoned
        clear();
        rebuild();
        return;
    }
    if (open.empty())
        return;
    // §4.2: the open cell with the smallest f (the first on ties).
    Cell here = *open.begin();
    for (const Cell &c : open)
        if (nodes[c].f < nodes[here].f)
            here = c;
    open.erase(here);
    if (here == goal) {
        rebuild();
        return;
    }
    // §4.1 step 3: the statics a box (1, 1, h) at the start and at the goal touches.
    if (!ignoredFound && statics) {
        ignoredFound = true;
        for (const Cell &c : {startCell, goal}) {
            const glm::dvec3 lo(c[0], c[1], c[2]), hi(c[0] + 1.0, c[1] + 1.0, c[2] + (double)size.z);
            const auto floorDiv8 = [](int32_t v) { return v >= 0 ? v / 8 : -((-v + 7) / 8); };
            statics(floorDiv8(c[0] - 1), floorDiv8(c[0] + 1), floorDiv8(c[1] - 1), floorDiv8(c[1] + 1), [&](const SearchStatic &s) {
                if (s.passable)
                    return;
                if (lo.x < s.at.x + s.half.x && hi.x > s.at.x - s.half.x && lo.y < s.at.y + s.half.y &&
                    hi.y > s.at.y - s.half.y && lo.z < s.at.z + s.height && hi.z > s.at.z)
                    ignored.insert(s.ref);
            });
        }
    }
    const Node node = nodes[here];
    // §4.3: the surroundings of the box standing in this cell.
    const glm::dvec3 centre(here[0] + 0.5, here[1] + 0.5, here[2] + size.z * 0.5 + 0.01);
    const Range r = rangeOf(centre, size);
    const auto any = [&](int32_t x0, int32_t x1, int32_t y0, int32_t y1, int32_t z0, int32_t z1) {
        for (int32_t x = x0; x <= x1; x++)
            for (int32_t y = y0; y <= y1; y++)
                for (int32_t z = z0; z <= z1; z++)
                    if (solid(block(x, y, z)))
                        return true;
        return false;
    };
    const int32_t x0 = r.lo[0], x1 = r.hi[0], y0 = r.lo[1], y1 = r.hi[1], z0 = r.lo[2], z1 = r.hi[2];
    const bool floor = any(x0, x1, y0, y1, z0 - 1, z0 - 1);
    const bool ceiling = any(x0, x1, y0, y1, z1 + 1, z1 + 1);
    const bool wallXm = any(x0 - 1, x0 - 1, y0, y1, z0, z1), wallXp = any(x1 + 1, x1 + 1, y0, y1, z0, z1);
    const bool wallYm = any(x0, x1, y0 - 1, y0 - 1, z0, z1), wallYp = any(x0, x1, y1 + 1, y1 + 1, z0, z1);
    const bool ledgeXm = any(x0 - 1, x0 - 1, y0, y1, z0 - 1, z0 - 1), ledgeXp = any(x1 + 1, x1 + 1, y0, y1, z0 - 1, z0 - 1);
    const bool ledgeYm = any(x0, x1, y0 - 1, y0 - 1, z0 - 1, z0 - 1), ledgeYp = any(x0, x1, y1 + 1, y1 + 1, z0 - 1, z0 - 1);
    const auto post = [&](int32_t x, int32_t y) { return any(x, x, y, y, z0, z1); };
    const auto cornerFloor = [&](int32_t x, int32_t y) { return any(x, x, y, y, z0 - 1, z0 - 1); };
    const bool water = block(here[0], here[1], cellOf(here[2] + size.z * 0.5 + 0.001)) == 2;
    // Open points 2 §1: the statics round the node: one under or in it supports it; one beside
    // it blocks that way when it is 3 blocks tall or the head is closed in.
    bool onStatic = false, staticXm = false, staticXp = false, staticYm = false, staticYp = false;
    if (statics) {
        const double nx0 = here[0] + 0.5 - size.x * 0.5, nx1 = here[0] + 0.5 + size.x * 0.5;
        const double ny0 = here[1] + 0.5 - size.y * 0.5, ny1 = here[1] + 0.5 + size.y * 0.5;
        const double nz0 = here[2], nz1 = here[2] + size.z;
        // (The fourth "ceiling" flag tests the floor height on +y, as Alpha does: the ledge.)
        const bool closedIn = ceiling || any(x0 - 1, x0 - 1, y0, y1, z1 + 1, z1 + 1) ||
                              any(x1 + 1, x1 + 1, y0, y1, z1 + 1, z1 + 1) || any(x0, x1, y0 - 1, y0 - 1, z1 + 1, z1 + 1) ||
                              ledgeYp;
        const auto floorDiv8 = [](int32_t v) { return v >= 0 ? v / 8 : -((-v + 7) / 8); };
        statics(floorDiv8(x0 - 1), floorDiv8(x1 + 1), floorDiv8(y0 - 1), floorDiv8(y1 + 1), [&](const SearchStatic &s) {
            if (ignored.count(s.ref)) // (open points 2 §1.2: neither support nor wall)
                return;
            const double X = s.at.x, Y = s.at.y, Z = s.at.z, hx = s.half.x, hy = s.half.y, H = s.height;
            const auto overX = [&](double shift) { return X - hx <= nx1 + shift && nx0 + shift < X + hx; };
            const auto overY = [&](double shift) { return Y - hy <= ny1 + shift && ny0 + shift < Y + hy; };
            if (overX(0) && overY(0) && Z <= nz1 - 1 && nz0 - 1 < Z + H)
                onStatic = true;
            if (!(H >= 3 || closedIn) || s.kind == 1 || s.kind == 2 || (s.kind >= 6 && s.kind <= 8) || s.ref == seatTarget)
                return;
            if (!(Z <= nz1 && nz0 < Z + H))
                return;
            staticXm = staticXm || (overX(-1) && overY(0));
            staticXp = staticXp || (overX(1) && overY(0));
            staticYm = staticYm || (overX(0) && overY(-1));
            staticYp = staticYp || (overX(0) && overY(1));
        });
    }
    const bool supported = floor || onStatic || water;
    // §4.4: try a neighbour; true when it was inserted or improved.
    const auto add = [&](Cell c, int32_t cost) {
        const int32_t g = node.g + cost;
        auto it = nodes.find(c);
        if (it == nodes.end()) {
            Node n;
            n.g = g, n.h = heuristic(c), n.f = g + n.h, n.parent = here;
            nodes[c] = n;
            open.insert(c);
            if (n.h < nodes[best].h)
                best = c;
            return true;
        }
        if (open.count(c) && g < it->second.g) {
            it->second.g = g, it->second.f = g + it->second.h, it->second.parent = here;
            if (it->second.h < nodes[best].h)
                best = c;
            return true;
        }
        return false;
    };
    const int32_t x = here[0], y = here[1], z = here[2];
    const int32_t straight = floor ? 10 : 20, diagonal = floor ? 14 : 24;
    bool xm = false, xp = false, ym = false, yp = false;
    if (!wallXm && !staticXm && (floor || ledgeXm || wallYm || wallYp || post(x0 - 1, y0 - 1) || post(x0 - 1, y1 + 1) || water))
        xm = add({x - 1, y, z}, straight);
    if (!wallXp && !staticXp && (supported || ledgeXp || wallYm || wallYp || post(x1 + 1, y0 - 1) || post(x1 + 1, y1 + 1)))
        xp = add({x + 1, y, z}, straight);
    if (!wallYm && !staticYm && (supported || ledgeYm || wallXm || wallXp || post(x0 - 1, y0 - 1) || post(x1 + 1, y0 - 1)))
        ym = add({x, y - 1, z}, !floor && !nodes.count({x, y - 1, z}) ? 30 : straight);
    if (!wallYp && !staticYp && (supported || ledgeYp || wallXm || wallXp || post(x0 - 1, y1 + 1) || post(x1 + 1, y1 + 1)))
        yp = add({x, y + 1, z}, straight);
    if (xm && ym && !post(x0 - 1, y0 - 1) && (supported || cornerFloor(x0 - 1, y0 - 1)))
        add({x - 1, y - 1, z}, diagonal);
    if (xp && ym && !post(x1 + 1, y0 - 1) && (supported || cornerFloor(x1 + 1, y0 - 1)))
        add({x + 1, y - 1, z}, diagonal);
    if (xm && yp && !post(x0 - 1, y1 + 1) && (supported || cornerFloor(x0 - 1, y1 + 1)))
        add({x - 1, y + 1, z}, diagonal);
    if (xp && yp && !post(x1 + 1, y1 + 1) && (supported || cornerFloor(x1 + 1, y1 + 1)))
        add({x + 1, y + 1, z}, diagonal);
    if (!floor && !onStatic)
        add({x, y, z - 1}, 0); // drop
    if (!ceiling && (wallXm || wallXp || wallYm || wallYp || water))
        add({x, y, z + 1}, floor ? 0 : 40); // climb
    rebuild();
}

void PathSearch::reroot(const Cell &N) {
    startCell = N;
    // The open set cut to the 70 cells nearest the goal by h (ties: first in order).
    std::vector<Cell> kept(open.begin(), open.end());
    std::stable_sort(kept.begin(), kept.end(), [&](const Cell &a, const Cell &b) { return nodes[a].h < nodes[b].h; });
    if (kept.size() > 70)
        kept.resize(70);
    // Keep the open cells whose parent chain reaches N, with their chains; forget the rest.
    std::map<Cell, Node> chained;
    std::set<Cell> stillOpen;
    for (const Cell &c : kept) {
        std::vector<Cell> chain;
        Cell at = c;
        bool reaches = false;
        for (size_t guard = 0; guard <= nodes.size(); guard++) {
            chain.push_back(at);
            if (at == N) {
                reaches = true;
                break;
            }
            const auto it = nodes.find(at);
            if (it == nodes.end() || it->second.parent == at)
                break;
            at = it->second.parent;
        }
        if (!reaches)
            continue;
        stillOpen.insert(c);
        for (const Cell &k : chain)
            chained[k] = nodes[k];
    }
    if (!chained.count(N))
        chained[N] = nodes[N];
    chained[N].parent = N;
    nodes = std::move(chained);
    open = std::move(stillOpen);
    best = nodes.begin()->first;
    for (const auto &[c, node] : nodes) // (map order is the cells' order: the first on ties)
        if (node.h < nodes[best].h)
            best = c;
}

void PathSearch::rounds(glm::vec3 size, float stop, const BlockType &block) {
    for (int k = 0; k < 10 && !nodes.empty() && path.size() <= 50; k++) {
        if (!path.empty()) {
            const Cell &last = path.back();
            const double dx = last[0] - goal[0], dy = last[1] - goal[1], dz = last[2] - goal[2];
            if (dx * dx + dy * dy + dz * dz <= (double)stop * stop)
                break;
        }
        round(size, block);
    }
}

bool PathSearch::follow(Body &b, int32_t dt, const BlockType &block) {
    b.intent = glm::vec3(0);
    if (path.empty())
        return false;
    const glm::dvec3 p = b.blocks();
    const glm::vec3 size = b.size;
    const auto within = [&](const Cell &c, float r) {
        const double dx = c[0] - goal[0], dy = c[1] - goal[1], dz = c[2] - goal[2];
        return dx * dx + dy * dy + dz * dz <= (double)r * r;
    };
    // §6.2: arrived, or re-plan after 3 s off the cells.
    const Cell feetHalf = {cellOf(p.x), cellOf(p.y), cellOf(p.z - size.z * 0.5 + 0.5)};
    if (within(feetHalf, radius)) {
        clear();
        return false;
    }
    if (timerMs > 3000) {
        start(p - glm::dvec3(0, 0, size.z * 0.5 - 0.5), goalPosition, radius);
        return false;
    }
    // §6.3: the nearest path cell to the feet cell (the later one on ties).
    const Cell feet = {cellOf(p.x), cellOf(p.y), cellOf(p.z - size.z * 0.5)};
    size_t nearest = 0;
    double q = 1e18;
    for (size_t i = 0; i < path.size(); i++) {
        const double dx = path[i][0] - feet[0], dy = path[i][1] - feet[1], dz = path[i][2] - feet[2];
        const double d = dx * dx + dy * dy + dz * dz;
        if (d <= q)
            q = d, nearest = i;
    }
    if (q == 0)
        timerMs = 0;
    const bool partial = !(b.contact & 0x40) && path.size() < 30 && !within(path.back(), radius);
    if (!partial && q == 0 && path[nearest] != startCell)
        reroot(path[nearest]);
    // §6.4: the target cell and its ground.
    const auto ground = [&](const Cell &c) {
        const glm::dvec3 at(c[0] + 0.5, c[1] + 0.5, c[2] + size.z * 0.5);
        Range r = rangeOf(at, size);
        r.lo[2] = r.hi[2] = cellOf(at.z - size.z * 0.5 - 0.2);
        return anySolid(r, block);
    };
    const Cell N = path[nearest];
    const bool g0 = ground(N);
    Cell target = N;
    bool targetIsN2 = false, g1 = false, g2 = false;
    if (!partial && nearest + 1 < path.size()) {
        target = path[nearest + 1];
        g1 = ground(target);
        if (target[2] != N[2] && (g0 || g1) && nearest + 2 < path.size()) {
            target = path[nearest + 2];
            g2 = ground(target);
            targetIsN2 = true;
        }
    }
    if (q < 4 && (b.contact & ON_WALL) && !g0)
        b.mode |= CLIMB;
    else
        b.mode &= ~CLIMB;
    glm::vec3 u = glm::vec3(glm::dvec3(target[0] + 0.5, target[1] + 0.5, target[2] + size.z * 0.5) - p);
    const float rise = u.z;
    if (glm::dot(u, u) <= 0.001f)
        return true;
    if (glm::length(u) > 1)
        u = glm::normalize(u);
    if (u.z > 0)
        u.z = 1;
    else if (u.z < 0 && (b.contact & IN_WATER))
        u.z = -2;
    if (b.contact & 0x40)
        u.z = 3;
    if (q >= 4) { // off the path
        b.intent = 40.f * u;
        b.push = glm::vec3(0);
        return true;
    }
    if (partial && !(b.mode & CLIMB))
        return true; // waits for the search to reach farther
    const bool lift = rise > 1 || (b.contact & 0x40) || !g0 || (targetIsN2 && !g2);
    const bool loose = target == N || (!g1 && !(targetIsN2 && g2));
    if (partial && u.z >= 0) {
        b.velocity = 8.f * u;
    } else {
        if (!partial && u.z >= 0 && lift)
            b.velocity.z = relax(b.velocity.z, 4 * u.z, 0.05f, dt);
        const float speed = loose ? 8.f : (b.mode & RUN) ? 12.f : 6.f;
        const float rate = loose ? 0.01f : 0.001f;
        b.velocity.x = relax(b.velocity.x, speed * u.x, rate, dt);
        b.velocity.y = relax(b.velocity.y, speed * u.y, rate, dt);
    }
    b.intent = 20.f * u;
    b.push = glm::vec3(0);
    return true;
}

} // namespace creatures
