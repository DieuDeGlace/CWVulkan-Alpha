#include "creatures/Airships.h"
#include <bit>
#include <cmath>

namespace creatures {
namespace {

// World objects terms: F (raw to binary32 blocks), T (binary32 blocks to raw, truncating),
// the block index (truncating quotient, one less when negative) and Q (squared length).
float toFloat(int64_t raw) { return float(raw) * (1.f / 65536.f); }
int64_t toRaw(float blocks) { return int64_t(blocks * 65536.f); }
int32_t blockIndex(int64_t raw) { return int32_t(raw / 65536 - (raw < 0 ? 1 : 0)); }
float squaredLength(glm::vec3 v) { return (v.x * v.x + v.y * v.y) + v.z * v.z; }
int64_t wrapAdd(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b)); }
int64_t wrapSub(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); }
glm::vec3 difference(const int64_t a[3], const int64_t b[3]) {
    return {toFloat(wrapSub(a[0], b[0])), toFloat(wrapSub(a[1], b[1])), toFloat(wrapSub(a[2], b[2]))};
}
// Steering N: scaled by the binary32 reciprocal of the binary32-rounded double root.
glm::vec3 normalised(glm::vec3 v) {
    const float r = 1.f / float(std::sqrt(double(squaredLength(v))));
    return {v.x * r, v.y * r, v.z * r};
}
// Steering Approach: n times p = ((g - p) x r) + p, each operation in binary32.
void approach(float &p, float g, int32_t n, float r) {
    for (int32_t i = 0; i < n; i++)
        p = (g - p) * r + p;
}

constexpr double PI = 3.141592653589793; // binary64 0x400921FB54442D18
constexpr float DEGREES = std::bit_cast<float>(0x3C8EFA35u); // 0.017453292
constexpr float NEAR_SHIP = 4096.f;                           // 64 blocks, squared
constexpr float NEAR_PLAYER = 262144.f;                       // 512 blocks, squared
constexpr float ARRIVED = std::bit_cast<float>(0x3A83126Fu);  // 0.001

float cosF(float x) { return float(std::cos(double(x))); }
float sinF(float x) { return float(std::sin(double(x))); }

// The rotation about z by binary32 angle `degrees`, each operation in binary32.
struct Turn {
    float c, s;
    explicit Turn(float degrees) : c(cosF(degrees * DEGREES)), s(sinF(degrees * DEGREES)) {}
    glm::vec3 operator()(glm::vec3 v) const { return {c * v.x + (-s) * v.y, s * v.x + c * v.y, v.z}; }
};

// Steering §2.1: a + the arcsine of sin(b - a), in degrees, scaled by t.
float angleToward(float a, float b, float t) {
    const auto radians = [](float degrees) { return float(double(degrees / 180.f) * PI); };
    const float alpha = radians(a), beta = radians(b);
    const double ca = cosF(alpha), sa = sinF(alpha), cb = cosF(beta), sb = sinF(beta);
    float s = float(sb * ca - cb * sa);
    if (s > 1.f)
        s = 1.f;
    else if (-1.f > s)
        s = -1.f;
    const double r = float(std::asin(double(s)));
    const double k = (double(t) * 180.0) / PI;
    return float(r * k) + a;
}

} // namespace

void Airships::setModel(const worldgen::Prefab &model) {
    sx = model.sx, sy = model.sy, sz = model.sz;
    hx = float(sx) * 0.5f, hy = float(sy) * 0.5f, hz = float(sz) * 0.5f;
    voxels.assign(size_t(sx) * size_t(sy) * size_t(sz), 0);
    marker.reset();
    // World objects §5 / steering §3.2: the loader visits x outermost, then y, then z, empties
    // each pure red voxel and lists it; the first listed is the companion's marker.
    for (int32_t x = 0; x < sx; x++)
        for (int32_t y = 0; y < sy; y++)
            for (int32_t z = 0; z < sz; z++) {
                const uint32_t colour = model.colour(x, y, z);
                if (colour == 0xFF0000 && !marker)
                    marker = glm::ivec3(x, y, z);
                voxels[(size_t(z) * size_t(sy) + size_t(y)) * size_t(sx) + size_t(x)] = colour != 0 && colour != 0xFF0000;
            }
}

bool Airships::solid(int32_t x, int32_t y, int32_t z) const {
    // §5: outside the grid the shared out-of-grid cell, empty unless a client store wrote it
    // (not traced; treated as empty).
    if (x < 0 || y < 0 || z < 0 || x >= sx || y >= sy || z >= sz)
        return false;
    return voxels[(size_t(z) * size_t(sy) + size_t(y)) * size_t(sx) + size_t(x)] != 0;
}

bool Airships::modelFrame(const Ship &ship, const int64_t position[3], glm::vec3 &centre) const {
    const glm::vec3 d = difference(position, ship.position.data());
    if (squaredLength(d) > NEAR_SHIP)
        return false;
    const glm::vec3 turned = Turn(-ship.yaw)(d);
    centre = {turned.x + hx, turned.y + hy, turned.z + hz};
    return true;
}

namespace {
struct Box {
    glm::ivec3 lo, hi;
    Box(glm::vec3 centre, glm::vec3 size) {
        const glm::vec3 half = size * 0.5f;
        const glm::vec3 low = centre - half, high = centre + half;
        lo = {blockIndex(toRaw(low.x)), blockIndex(toRaw(low.y)), blockIndex(toRaw(low.z))};
        hi = {blockIndex(toRaw(high.x)), blockIndex(toRaw(high.y)), blockIndex(toRaw(high.z))};
    }
};
} // namespace

bool Airships::touches(glm::vec3 centre, glm::vec3 size) const {
    const Box box(centre, size);
    for (int32_t x = box.lo.x; x <= box.hi.x; x++)
        for (int32_t y = box.lo.y; y <= box.hi.y; y++)
            for (int32_t z = box.lo.z; z <= box.hi.z; z++)
                if (solid(x, y, z))
                    return true;
    return false;
}

bool Airships::collide(const int64_t position[3], glm::vec3 size, uint64_t key, int64_t &base) {
    if (!hasModel())
        return false;
    // §3.2 step 6: every ship with a solid voxel in the box takes the key and sets the base;
    // the last in key order decides the base.
    bool hit = false;
    for (auto &[id, ship] : list) {
        glm::vec3 centre;
        if (!modelFrame(ship, position, centre) || !touches(centre, size))
            continue;
        hit = true;
        base = ship.position[2] - toRaw(hz);
        ship.carried.insert(std::bit_cast<int64_t>(key));
    }
    return hit;
}

bool Airships::blocksRaised(const int64_t position[3], glm::vec3 size, uint64_t key) {
    if (!hasModel())
        return false;
    // §4 step 2: the box raised by 1.01 in the model frame; the first ship hit ends the test.
    for (auto &[id, ship] : list) {
        glm::vec3 centre;
        if (!modelFrame(ship, position, centre))
            continue;
        centre.z = 1.01f + centre.z;
        if (touches(centre, size)) {
            ship.carried.insert(std::bit_cast<int64_t>(key));
            return true;
        }
    }
    return false;
}

void Airships::unstick(int64_t position[3], glm::vec3 size) {
    if (!hasModel())
        return;
    // Steering §5.2, per ship in key order with the position as earlier ships left it.
    for (const auto &[id, ship] : list) {
        glm::vec3 P;
        if (!modelFrame(ship, position, P))
            continue;
        const Box box(P, size);
        const auto emptyShifted = [&](glm::ivec3 by) {
            for (int32_t x = box.lo.x + by.x; x <= box.hi.x + by.x; x++)
                for (int32_t y = box.lo.y + by.y; y <= box.hi.y + by.y; y++)
                    for (int32_t z = box.lo.z; z <= box.hi.z; z++)
                        if (solid(x, y, z))
                            return false;
            return true;
        };
        if (emptyShifted({0, 0, 0}))
            continue;
        // Step 8: the four horizontal escapes in order; the first free one, then only a
        // strictly nearer one.
        const float halfX = size.x * 0.5f, halfY = size.y * 0.5f;
        const struct {
            glm::ivec3 by;
            glm::vec3 centre;
        } escapes[] = {
            {{1, 0, 0}, {(float(box.lo.x) + 1.05f) + halfX, P.y, P.z}},
            {{-1, 0, 0}, {(float(box.hi.x) - 0.05f) - halfX, P.y, P.z}},
            {{0, 1, 0}, {P.x, (float(box.lo.y) + 1.05f) + halfY, P.z}},
            {{0, -1, 0}, {P.x, (float(box.hi.y) - 0.05f) - halfY, P.z}},
        };
        float best = -1.f;
        glm::vec3 chosen{0};
        for (const auto &escape : escapes) {
            if (!emptyShifted(escape.by))
                continue;
            const glm::vec3 d = escape.centre - P;
            const float distance = (d.y * d.y + d.x * d.x) + d.z * d.z;
            if (best < 0.f || distance < best)
                best = distance, chosen = escape.centre;
        }
        if (best < 0.f)
            continue;
        // Step 10: back to the world by +yaw.
        const glm::vec3 u = Turn(ship.yaw)(chosen - P);
        for (size_t k = 0; k < 3; k++)
            position[k] = wrapAdd(position[k], toRaw(u[int(k)]));
    }
}

void Airships::refresh(std::span<const Raw> players, const worldgen::WorldMap &map) {
    // §2.3: each player's zone (truncating block, then zone, divisions) and its eight
    // neighbours, as far as loaded. (The original's set is ordered by address; the order
    // cannot change the result, since identifiers of different zones differ.)
    std::set<const worldgen::Zone *> zones;
    for (const Raw &p : players) {
        const int32_t bx = int32_t(p[0] / 65536) / 256, by = int32_t(p[1] / 65536) / 256;
        for (int32_t i = -1; i <= 1; i++)
            for (int32_t j = -1; j <= 1; j++)
                if (const worldgen::Zone *zone = map.zone(bx + i, by + j))
                    zones.insert(zone);
    }
    const auto nearPlayer = [&](const Raw &at) {
        for (const Raw &p : players)
            if (NEAR_PLAYER > squaredLength(difference(at.data(), p.data())))
                return true;
        return false;
    };
    // Removal: ships whose target is beyond 512 blocks of every player.
    std::erase_if(list, [&](const auto &entry) { return !nearPlayer(entry.second.state.target); });
    // Copies: each template of a player zone not yet listed, once a player is within 512
    // blocks of its target; the pose starts at the target.
    for (const worldgen::Zone *zone : zones)
        for (const worldgen::AirshipTemplate &t : zone->airships) {
            if (list.contains(t.id) || !nearPlayer(t.target))
                continue;
            Ship &ship = list[t.id];
            ship.state = t;
            ship.position = t.target;
            ship.yaw = t.targetYaw;
        }
}

void Airships::steerShip(Ship &ship, int32_t dtMs, const worldgen::WorldMap &map) const {
    worldgen::AirshipTemplate &s = ship.state;
    const auto toward = [&](const Raw &goal) { return difference(goal.data(), s.target.data()); };
    // §1.2: the waypoint 50 blocks along +-(sin h, cos h) from home, 20 up.
    const auto waypoint = [&](float reach) {
        const float h = float((double(-s.heading) * PI) / 180.0);
        const glm::vec3 offset(sinF(h) * reach, cosF(h) * reach, 20.f);
        return Raw{wrapAdd(s.home[0], toRaw(offset.x)), wrapAdd(s.home[1], toRaw(offset.y)),
                   wrapAdd(s.home[2], toRaw(offset.z))};
    };
    // §1.6: states 0-2 turn their target yaw toward the heading.
    const auto turnToHeading = [&] { approach(s.targetYaw, angleToward(s.targetYaw, s.heading, 1.f), dtMs, 0.005f); };
    switch (ship.course) {
    case 1: { // §1.3: to home, slowing within 10 blocks
        const glm::vec3 d = toward(s.home);
        const float q = squaredLength(d);
        if (ARRIVED > q)
            ship.velocity = glm::vec3(0), ship.course = 2;
        else
            ship.velocity = q > 100.f ? normalised(d) * 10.f : d;
        turnToHeading();
        break;
    }
    case 0:
    case 2: { // §1.4, §1.5: behind or ahead of home at 10 blocks a second
        const glm::vec3 d = toward(waypoint(ship.course == 0 ? -50.f : 50.f));
        if (ARRIVED > squaredLength(d))
            ship.velocity = glm::vec3(0), ship.course = ship.course == 0 ? 1 : 3;
        else
            ship.velocity = normalised(d) * 10.f;
        turnToHeading();
        break;
    }
    case 3: { // §1.7: to the destination, holding 100 to 150 blocks above the ground
        glm::vec3 d = toward(s.destination);
        d.z = 0.f;
        const float q = squaredLength(d);
        if (ARRIVED > q) {
            ship.velocity = glm::vec3(0), ship.course = 0;
        } else {
            if (q > 900.f)
                d = normalised(d) * 30.f;
            approach(ship.velocity.x, d.x, dtMs, ARRIVED);
            approach(ship.velocity.y, d.y, dtMs, ARRIVED);
        }
        // Step 4: the column height above the stored run (0 when absent or negative).
        const int32_t cx = int32_t(s.target[0] / 65536), cy = int32_t(s.target[1] / 65536);
        int32_t ground = 0;
        if (cx >= 0 && cy >= 0 && cx < (1 << 24) && cy < (1 << 24))
            if (const worldgen::Zone *zone = map.zone(cx / 256, cy / 256)) {
                const worldgen::Column &column = zone->column(cx, cy);
                ground = std::bit_cast<int32_t>(uint32_t(column.bottom) + uint32_t(column.blocks.size()));
                ground = std::max(ground, 0);
            }
        const auto level = [&](int32_t k) { return int64_t(std::bit_cast<int32_t>(uint32_t(ground) + uint32_t(k))) * 65536; };
        const float rise = s.target[2] < level(100) ? 10.f : s.target[2] > level(150) ? -10.f : 0.f;
        approach(ship.velocity.z, rise, dtMs, ARRIVED);
        // Step 6: the target yaw follows the horizontal velocity.
        glm::vec3 e(ship.velocity.x, ship.velocity.y, 0.f);
        if (!(squaredLength(e) > 0.f))
            break;
        e = normalised(e);
        if (e.y > 1.f)
            e.y = 1.f;
        if (-1.f > e.y)
            e.y = -1.f;
        const float arc = float(std::acos(double(e.y)));
        const float goal = float((double(e.x > 0.f ? -arc : arc) / PI) * 180.0);
        approach(s.targetYaw, angleToward(s.targetYaw, goal, 1.f), dtMs, ARRIVED);
        break;
    }
    default: break; // other states leave the ship as it is
    }
}

void Airships::step(int32_t dtMs, bool steer, const worldgen::WorldMap &map,
                    const std::function<bool(int64_t key, Carried &out)> &lookup) {
    const float seconds = float(dtMs) * 0.001f;
    // World objects §6.2 step 2: the follow factor approaches 1 once per millisecond.
    float follow = 0.f;
    approach(follow, 1.f, dtMs, 0.01f);
    const int64_t share = toRaw(follow);
    for (auto &[id, ship] : list) {
        const Raw before = ship.position;
        const float yawBefore = ship.yaw;
        if (steer)
            steerShip(ship, dtMs, map);
        for (size_t k = 0; k < 3; k++) {
            ship.state.target[k] = wrapAdd(ship.state.target[k], toRaw(seconds * ship.velocity[int(k)]));
            const uint64_t product = uint64_t(wrapSub(ship.state.target[k], ship.position[k])) * uint64_t(share);
            ship.position[k] = wrapAdd(ship.position[k], std::bit_cast<int64_t>(product) / 65536);
        }
        // Steering §2.2: the yaw follows the target yaw through the angle helper.
        approach(ship.yaw, angleToward(ship.yaw, ship.state.targetYaw, 1.f), dtMs, 0.01f);
        // §6.3: carried creatures follow the pose change; those gone or beyond 64 blocks of
        // the target leave the set.
        const float turnBy = ship.yaw - yawBefore;
        const Turn turn(turnBy);
        const auto carry = [&](int64_t *at) {
            const glm::vec3 moved = turn(difference(at, before.data()));
            for (size_t k = 0; k < 3; k++)
                at[k] = wrapAdd(ship.position[k], toRaw(moved[int(k)]));
        };
        std::erase_if(ship.carried, [&](int64_t key) {
            Carried c;
            if (!lookup(key, c) || squaredLength(difference(c.position, ship.state.target.data())) > NEAR_SHIP)
                return true;
            if (c.follow)
                carry(c.follow);
            carry(c.position);
            if (c.yaw)
                *c.yaw = turnBy + *c.yaw;
            return false;
        });
    }
}

bool Airships::place(int64_t key, Body &body, float &yaw) const {
    const auto it = list.find(key);
    if (it == list.end() || !hasModel())
        return false;
    const Ship &ship = it->second;
    // Steering §3.3: the marker turned by +yaw about the grid's centre, a hair above it.
    if (marker) {
        const Turn turn(ship.yaw);
        const float tx = float(sx) * -0.5f, ty = float(sy) * -0.5f, tz = float(sz) * -0.5f;
        const glm::vec3 p(*marker);
        const float X = ((-turn.s * p.y) + (turn.c * p.x)) + ((-turn.s * ty) + (turn.c * tx));
        const float Y = ((turn.c * p.y) + (turn.s * p.x)) + ((turn.c * ty) + (turn.s * tx));
        const float Z = p.z + tz;
        const int64_t lift = toRaw(body.size.z * 0.5f + ARRIVED);
        const int64_t spot[3] = {wrapAdd(ship.position[0], toRaw(X)), wrapAdd(ship.position[1], toRaw(Y)),
                                 wrapAdd(wrapAdd(ship.position[2], toRaw(Z)), lift)};
        if (squaredLength(difference(body.position, spot)) > 1.f)
            std::copy(spot, spot + 3, body.position);
    }
    yaw = ship.yaw;
    return true;
}

} // namespace creatures
