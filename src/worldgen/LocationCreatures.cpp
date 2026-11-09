// The creatures of a location (docs/specs/alpha-location-creatures.md, 0x5dc6e0): in the zones
// of hills, oases, lakes, valleys, great rocks and giant trees, the candidates of the location
// grid get rings of creatures round a camp fire, or a leader with followers on a patrol. Section
// numbers point at that spec.
#include "worldgen/Builders.h"
#include "worldgen/Terrain.h"
#include <cmath>
#include <vector>

namespace worldgen {

namespace {

constexpr int64_t ONE = 65536; // one block in 1/65536 blocks
constexpr double PI = 3.141592653589793;
constexpr float TWO_POW_MINUS_16 = 1.f / 65536.f;

// §2.2: what a creature set holds.
struct Group {
    std::vector<int32_t> leaders, followers;
};
struct CreatureSet {
    bool camp = false;
    std::vector<int32_t> ring;
    std::vector<Group> groups;
};

// §2.1, §2.2, §2.3: the location's set, with its set-up draws.
CreatureSet creatureSet(const Location &L, Random &random) {
    std::vector<int32_t> list;
    if (L.type == 4) {
        list = {9};
    } else {
        list = {0, 1, 2, 3, 4, 5, 1, 7, 8}; // (1 twice and no 6 here, as read)
        if (L.type != 13 && L.type != 2)
            list.push_back(6);
        if (L.type == 3)
            list.push_back(10);
    }
    const int32_t number = list[(uint32_t)L.serial % (uint32_t)list.size()];
    CreatureSet s;
    switch (number) {
    case 1: { // undead
        s.camp = true, s.ring = {15, 16};
        Group first;
        static const int32_t LEADERS[3] = {17, 41, 97};
        if (const int d = random.draw() % 4; d < 3)
            first.leaders = {LEADERS[d]}; // (3: none, so the group makes nothing)
        switch (random.draw() % 4) {
        case 0: first.followers = {40, 37, 38, 39}; break;
        case 1: first.followers = {59}; break;
        case 2: first.followers = {41}; break;
        default: first.followers = {96}; break;
        }
        s.groups = {first, {{15, 16}, {15, 16}}};
        break;
    }
    case 2: { // humans
        s.camp = true, s.ring = {2, 3};
        static const int32_t LEADERS[3] = {94, 79, 82}, PETS[4] = {30, 26, 19, 33};
        const int32_t leader = LEADERS[random.draw() % 3];
        const int32_t pet = PETS[random.draw() % 4];
        s.groups = {{{leader}, {pet}}, {{2, 3}, {2, 3}}};
        break;
    }
    case 3: { // dwarves
        s.camp = true, s.ring = {9, 10};
        static const int32_t LEADERS[2] = {47, 88}, PETS[3] = {87, 91, 33};
        const int32_t leader = LEADERS[random.draw() % 2];
        const int32_t pet = PETS[random.draw() % 3];
        s.groups = {{{leader}, {pet}}, {{9, 10}, {9, 10}}};
        break;
    }
    case 4: { // lizardmen
        s.camp = true, s.ring = {7, 8};
        static const int32_t PETS[3] = {60, 53, 58};
        s.groups = {{{7, 8}, {PETS[random.draw() % 3]}}, {{7, 8}, {7, 8}}};
        break;
    }
    case 5: // goblins
        s.camp = true, s.ring = {4, 5};
        s.groups = {{{88}, {87, 55, 60}}, {{4, 5}, {4, 5}}};
        break;
    case 6: // plant creatures and onionlings
        s.ring = {70, 69};
        s.groups = {{{71}, {71}}, {{70, 69}, {70, 69}}};
        break;
    case 7: // insectoids (the third group keeps the second's 60, as read)
        s.ring = {52};
        s.groups = {{{62}, {62}}, {{60}, {60}}, {{60, 52}, {52}}};
        break;
    case 8: // beetles
        s.ring = {102, 105, 104};
        s.groups = {{{104}, {104}}, {{102}, {102}}, {{105}, {105}}};
        break;
    case 9: // squid and crabs
        s.ring = {86, 106};
        s.groups = {{{86}, {86}}, {{106}, {106}}};
        break;
    case 10: // bunnies, squirrels and raccoons
        s.ring = {35, 90, 91};
        s.groups = {{{35}, {35}}, {{90}, {90}}, {{91}, {91}}};
        break;
    default: // 0: orcs with an ogre and dogs
        s.camp = true, s.ring = {11, 12};
        s.groups = {{{46}, {19, 33, 26}}, {{11, 12}, {11, 12}}};
        break;
    }
    return s;
}

// A spawn of the location: controller 1, flag 0x1000 (it takes its map cell's level when the
// cell holds a mission), the location's level and tier.
Spawn locationSpawn(const Location &L, int64_t x, int64_t y, int64_t z) {
    Spawn s;
    s.x = x, s.y = y, s.z = z;
    s.controller = 1;
    s.appearanceFlags |= 0x1000;
    s.level = L.level;
    s.powerTier = (uint8_t)L.subLevel;
    return s;
}

std::array<int32_t, 3> blocksOf(int64_t x, int64_t y, int64_t z) {
    return {(int32_t)(x / ONE), (int32_t)(y / ONE), (int32_t)(z / ONE)};
}

} // namespace

void Terrain::locationCreatures(Zone &zone, Random &random, const Location &L,
                                const std::vector<std::array<int64_t, 3>> &candidates) const {
    if (L.type == 0)
        return; // §1.2
    const CreatureSet set = creatureSet(L, random);

    // §3: the candidate deepest inside the location, in the zone of its centre.
    int marked = -1;
    if ((int32_t)(L.x / ONE / 256) == zone.zx && (int32_t)(L.y / ONE / 256) == zone.zy) {
        float best = 0;
        for (size_t i = 0; i < candidates.size(); i++) {
            const float v = (float)(1.f - (float)w.locationDistanceFx(L, candidates[i][0], candidates[i][1]));
            const float weight = v > 0 ? v * v : 0.f;
            if (weight > best)
                best = weight, marked = (int)i;
        }
    }

    for (size_t i = 0; i < candidates.size(); i++) {
        const auto [X, Y, Z] = candidates[i];
        // §4.1: the other candidates 5 to 128 blocks away.
        std::vector<std::array<int64_t, 3>> near;
        for (const auto &P : candidates) {
            const float dx = (float)(P[0] - X) * TWO_POW_MINUS_16, dy = (float)(P[1] - Y) * TWO_POW_MINUS_16;
            const float d2 = dy * dy + dx * dx;
            if (d2 > 25.f && d2 < 16384.f)
                near.push_back(P);
        }
        // §4.2
        bool ring = (int)i == marked;
        if (!ring) {
            if (random.draw() & 1)
                continue;
            ring = (random.draw() & 1) != 0;
        }

        if (ring) {
            // §4.3.1: the camp fire, settled on the first of nine spots, and four props round it.
            std::array<int64_t, 3> centre = candidates[i];
            if (set.camp) {
                StaticObject fire;
                fire.kind = 65;
                fire.scale[0] = fire.scale[1] = 2.4f, fire.scale[2] = 0.5f;
                fire.rotation = random.draw() % 4;
                bool placed = false;
                for (int64_t a = 0; a < 3 && !placed; a++)
                    for (int64_t b = 0; b < 3 && !placed; b++) {
                        StaticObject at = fire;
                        at.x = X + ONE * a, at.y = Y + ONE * b, at.z = Z;
                        if (settle(at, zone)) {
                            zone.statics.push_back(at);
                            centre = {at.x, at.y, at.z};
                            placed = true;
                        }
                    }
                for (int64_t a : {0, 7})
                    for (int64_t b : {0, 7}) {
                        static const int32_t KINDS[4] = {66, 16, 12, 69}; // tent, stool, table, mat
                        static const float SCALES[4][3] = {{4, 4, 3}, {1, 1, 0.5f}, {3, 3, 1}, {2, 2, 0.1f}};
                        const int k = random.draw() % 4;
                        StaticObject prop;
                        prop.kind = KINDS[k];
                        for (int axis = 0; axis < 3; axis++)
                            prop.scale[axis] = SCALES[k][axis];
                        prop.x = centre[0] + ONE * a - 229376, prop.y = centre[1] + ONE * b - 229376, prop.z = centre[2];
                        prop.rotation = random.draw() % 4;
                        if (settle(prop, zone))
                            zone.statics.push_back(prop);
                    }
            }
            // §4.3.2: one to three creatures 3 blocks round the centre, facing out, made up at
            // once (and again in the finishing pass); the marked candidate's first is a boss.
            const float phi = (float)(random.draw() * 6.283185307179586 / 32767.0);
            const int k = random.draw() % 3 + 1;
            for (int c = 0; c < k; c++) {
                const float theta = (float)((double)(2 * c) * PI / (double)k + phi);
                const int64_t y = centre[1] + (int64_t)(float)((float)((float)std::sin((double)theta) * 3.f) * 65536.f);
                const int64_t x = centre[0] + (int64_t)(float)((float)((float)std::cos((double)theta) * 3.f) * 65536.f);
                Spawn s = locationSpawn(L, x, y, centre[2]);
                s.yaw = (float)((double)(float)(theta * 180.f) / PI + 90.0);
                s.type = set.ring[(size_t)random.draw() % set.ring.size()];
                s.behaviour = 5;
                s.route = {blocksOf(x, y, centre[2])};
                if ((int)i == marked && c == 0) {
                    s.appearanceFlags |= 0x200;
                    s.bossLoot = true;
                }
                if (finishSpawn)
                    finishSpawn(s, random, false);
                zone.spawns.push_back(s);
            }
            continue;
        }

        // §4.4: a leader on a patrol through up to three neighbouring candidates, and followers.
        const Group &group = set.groups[(uint32_t)random.draw() % (uint32_t)set.groups.size()];
        if (group.leaders.empty())
            continue;
        Spawn leader = locationSpawn(L, X, Y, Z);
        leader.yaw = (float)(random.draw() % 360);
        leader.type = group.leaders[(size_t)random.draw() % group.leaders.size()];
        leader.behaviour = 6;
        leader.route = {blocksOf(X, Y, Z)};
        if (!near.empty())
            for (int n = 0; n < 3; n++) {
                const auto &P = near[(uint32_t)random.draw() % (uint32_t)near.size()];
                const int32_t px = (int32_t)(P[0] / ONE), py = (int32_t)(P[1] / ONE);
                if (!zone.contains(px, py))
                    continue;
                int32_t z = zone.column(px, py).belowSurface;
                while (!clear(zone.read(px, py, z).type()))
                    z++;
                leader.route.push_back({px, py, z});
            }
        zone.spawns.push_back(leader);
        // The followers stand on the leader. They copy its id before it has one (as read,
        // open question LC-2), so they follow nobody: Combat, then RandomWalk.
        const int n = random.draw() % 3 + 1;
        for (int f = 0; f < n; f++) {
            Spawn follower = locationSpawn(L, X, Y, Z);
            follower.type = group.followers[(size_t)random.draw() % group.followers.size()];
            follower.behaviour = 1;
            zone.spawns.push_back(follower);
        }
    }
}

} // namespace worldgen
