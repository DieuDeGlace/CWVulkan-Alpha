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
int64_t wrapSub(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); }
glm::vec3 difference(const int64_t a[3], const int64_t b[3]) {
    return {toFloat(wrapSub(a[0], b[0])), toFloat(wrapSub(a[1], b[1])), toFloat(wrapSub(a[2], b[2]))};
}

constexpr float DEGREES = std::bit_cast<float>(0x3C8EFA35u); // 0.017453292
constexpr float NEAR_SHIP = 4096.f;                           // 64 blocks, squared
constexpr float NEAR_PLAYER = 262144.f;                       // 512 blocks, squared

// The rotation about z by binary32 angle `degrees`, each operation in binary32.
struct Turn {
    float c, s;
    explicit Turn(float degrees) {
        const float angle = degrees * DEGREES;
        c = float(std::cos(double(angle)));
        s = float(std::sin(double(angle)));
    }
    glm::vec3 operator()(glm::vec3 v) const { return {c * v.x + (-s) * v.y, s * v.x + c * v.y, v.z}; }
};

} // namespace

void Airships::setModel(const worldgen::Prefab &model) {
    sx = model.sx, sy = model.sy, sz = model.sz;
    hx = float(sx) * 0.5f, hy = float(sy) * 0.5f, hz = float(sz) * 0.5f;
    voxels.assign(size_t(sx) * size_t(sy) * size_t(sz), 0);
    for (int32_t z = 0; z < sz; z++)
        for (int32_t y = 0; y < sy; y++)
            for (int32_t x = 0; x < sx; x++) {
                const uint32_t colour = model.colour(x, y, z);
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

bool Airships::touches(glm::vec3 centre, glm::vec3 size) const {
    const glm::vec3 half = size * 0.5f;
    const glm::vec3 low = centre - half, high = centre + half;
    const glm::ivec3 lo(blockIndex(toRaw(low.x)), blockIndex(toRaw(low.y)), blockIndex(toRaw(low.z)));
    const glm::ivec3 hi(blockIndex(toRaw(high.x)), blockIndex(toRaw(high.y)), blockIndex(toRaw(high.z)));
    for (int32_t x = lo.x; x <= hi.x; x++)
        for (int32_t y = lo.y; y <= hi.y; y++)
            for (int32_t z = lo.z; z <= hi.z; z++)
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

void Airships::step(int32_t dtMs, const std::function<bool(int64_t key, Carried &out)> &lookup) {
    const float seconds = float(dtMs) * 0.001f;
    // §6.2 step 2: the follow factor approaches 1 once per millisecond.
    float follow = 0.f;
    for (int32_t ms = 0; ms < dtMs; ms++)
        follow = (1.f - follow) * 0.01f + follow;
    const int64_t share = toRaw(follow);
    for (auto &[id, ship] : list) {
        const Raw before = ship.position;
        const float yawBefore = ship.yaw;
        for (size_t k = 0; k < 3; k++) {
            ship.state.target[k] += toRaw(seconds * ship.velocity[int(k)]);
            const uint64_t product = uint64_t(wrapSub(ship.state.target[k], ship.position[k])) * uint64_t(share);
            ship.position[k] += std::bit_cast<int64_t>(product) / 65536;
        }
        // §6.3: carried creatures follow the pose change; those gone or beyond 64 blocks of
        // the target leave the set.
        const float turnBy = ship.yaw - yawBefore;
        const Turn turn(turnBy);
        const auto carry = [&](int64_t *at) {
            const glm::vec3 moved = turn(difference(at, before.data()));
            for (size_t k = 0; k < 3; k++)
                at[k] = ship.position[k] + toRaw(moved[int(k)]);
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

} // namespace creatures
