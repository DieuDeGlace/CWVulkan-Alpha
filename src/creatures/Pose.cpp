#include "creatures/Pose.h"
#include "creatures/Spirits.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace creatures {
namespace {

constexpr double PI = 3.141592653589793;
constexpr float PIf = 3.14159265f;

glm::mat4 T(glm::mat4 m, glm::vec3 v) { return glm::translate(m, v); }

// The weapon-attachment pose contract's rotation primitives (§2): the degree angle times
// binary32 pi/180, then the binary64 cosine and sine, each narrowed.
glm::vec2 cosSin(float degrees) {
    const float radians = degrees * std::bit_cast<float>(0x3C8EFA35u);
    return {(float)std::cos((double)radians), (float)std::sin((double)radians)};
}
// Rotation about a single axis, per row from the pre-update columns.
glm::mat4 turnColumns(glm::mat4 m, float degrees, int a, int b, bool subtractFirst) {
    const glm::vec2 cs = cosSin(degrees);
    const glm::vec4 first = m[a], second = m[b];
    m[a] = subtractFirst ? first * cs.x - second * cs.y : first * cs.x + second * cs.y;
    m[b] = subtractFirst ? second * cs.x + first * cs.y : second * cs.x - first * cs.y;
    return m;
}
glm::mat4 Rx(glm::mat4 m, float d) { return turnColumns(m, d, 1, 2, false); }
glm::mat4 Ry(glm::mat4 m, float d) { return turnColumns(m, d, 0, 2, true); }
glm::mat4 Rz(glm::mat4 m, float d) { return turnColumns(m, d, 0, 1, false); }
// Axis-angle rotation (§2): the axis divided by its binary64-rooted length, unchanged
// for a zero axis, and the current matrix multiplied by the rotation on the right.
glm::mat4 R(glm::mat4 m, float degrees, glm::vec3 axis) {
    const float length = (float)std::sqrt((double)((axis.x * axis.x + axis.y * axis.y) + axis.z * axis.z));
    if (length == 0.f)
        return m;
    const glm::vec2 cs = cosSin(degrees);
    const float c = cs.x, s = cs.y, t = 1.f - c;
    const glm::vec3 u(axis.x / length, axis.y / length, axis.z / length);
    glm::mat4 r(1.f);
    r[0][0] = (u.x * u.x) * t + c;
    r[1][1] = (u.y * u.y) * t + c;
    r[2][2] = (u.z * u.z) * t + c;
    r[0][1] = t * (u.x * u.y) + s * u.z;
    r[1][0] = t * (u.x * u.y) - s * u.z;
    r[0][2] = t * (u.x * u.z) - s * u.y;
    r[2][0] = t * (u.x * u.z) + s * u.y;
    r[1][2] = t * (u.y * u.z) + s * u.x;
    r[2][1] = t * (u.y * u.z) - s * u.x;
    return m * r;
}
glm::mat4 Rzyx(glm::mat4 m, glm::vec3 a) { return Rx(Ry(Rz(m, a.z), a.y), a.x); }
glm::mat4 S(glm::mat4 m, float s) { return glm::scale(m, glm::vec3(s)); }

// "Relax toward t at rate r per ms", dt times (§7.3).
float relax(float x, float t, float r, float dt) { return t + (x - t) * std::pow(1.f - r, dt); }
float clamp01(float v) { return std::clamp(v, 0.f, 1.f); }
float lerp(float a, float b, float q) { return (1 - q) * a + q * b; }

int subtypeIn(const Item &item) { return item.type == 3 ? item.subtype : -1; }

// 0x444230: held in both hands or shot.
bool twoHanded(int m) {
    switch (m) {
    case GREATSWORD: case GREATAXE: case GREATMACE: case LONGSWORD: case STAFF: case WAND: case PITCHFORK:
    case BOOMERANG: case BOW: case CROSSBOW:
        return true;
    default:
        return false;
    }
}

// The channel targets and per-frame values of §7.7, as a pose case sees them.
struct Frame {
    glm::vec3 dL{0}, dR{0}, rL{0}, rR{0}, T{0}, B{0}, S{0}, K{0}, H{0}, Q{0}, U{0}, E{0};
    float Y = 0, h = 0;
    glm::vec3 L{0}, Rh{0}, armL{0}, armR{0}, footL{0}, footR{0}; // base targets
    glm::vec3 lambda{0}, rho{0};                                   // hand rotations
    float legL = 0, legR = 0, F = 0, ell = 0, rate = 0.01f;
    bool turn = false;
    uint16_t mainModel = NO_MODEL, offModel = NO_MODEL;
    bool mainOnBack = false, offOnBack = false;
    bool curled = false; // pose 41: hands, feet and tail not drawn
};

} // namespace

// §6.3.1: resources (type 11) by subtype, and by material for nuggets and cubes.
static uint16_t resourceModel(const Item &item) {
    switch (item.subtype) {
    case 0:
        switch (item.material) {
        case 1: return 2337;
        case 2: return 2365;
        case 11: return 2339;
        case 12: return 2338;
        case 13: return 2340;
        case 14: return 2341;
        case 15: return 2342;
        case 16: return 2343;
        case 17: return 2364;
        default: return NO_MODEL;
        }
    case 10:
        switch (item.material) {
        case 1: return 2344;
        case 2: return 2347;
        case 11: return 2346;
        case 12: return 2345;
        default: return NO_MODEL;
        }
    case 14: return item.material == 129 ? 2350 : item.material == 130 ? 2349 : item.material == 131 ? 2351 : 2348;
    default: break;
    }
    static const uint16_t BY_SUBTYPE[28] = {0,    2365, 2366, 0,    0,    2394, 2352, 2373, 2357, 2363,
                                            0,    2160, 2389, 2336, 0,    843,  2332, 2334, 2391, 844,
                                            2097, 2392, 845,  847,  846,  848,  2390, 2091};
    return item.subtype < 28 && BY_SUBTYPE[item.subtype] ? BY_SUBTYPE[item.subtype] : NO_MODEL;
}

// §6.3.1: pet food by the creature type it tames.
static uint16_t petFoodModel(int subtype) {
    static const std::pair<int, uint16_t> FOODS[] = {
        {19, 875}, {22, 868}, {23, 869}, {25, 877}, {26, 879}, {27, 871}, {30, 859}, {33, 858}, {34, 863},
        {35, 857}, {36, 889}, {37, 853}, {38, 854}, {39, 856}, {40, 855}, {50, 870}, {53, 864}, {55, 876},
        {56, 865}, {57, 890}, {58, 891}, {59, 892}, {60, 893}, {61, 894}, {62, 895}, {63, 884}, {64, 886},
        {65, 887}, {66, 885}, {67, 874}, {74, 888}, {75, 878}, {86, 880}, {87, 862}, {88, 896}, {90, 866},
        {91, 867}, {92, 860}, {93, 861}, {98, 882}, {99, 881}, {102, 2408}, {103, 897}, {104, 872}, {105, 873},
        {106, 883}, {151, 898}};
    for (const auto &[sub, model] : FOODS)
        if (sub == subtype)
            return model;
    return 2334;
}

// §6.2, §6.3
uint16_t itemModel(const Item &item) {
    const int r = std::min<int>(item.rarity, 4);
    const int v11 = (int)((int32_t)item.seed % 11), v6 = (int)((int32_t)item.seed % 6);
    const int v5 = (int)(item.seed % 5u);
    const int m = item.material;
    switch (item.type) {
    case 3:
        switch (item.subtype) {
        case 0: return m == 5 ? 2059 : m == 7 ? 2072 : (uint16_t)(899 + 5 * v11 + r);
        case 1: return m == 5 ? NO_MODEL : m == 7 ? 2074 : (uint16_t)(5 * (v11 + 354) + r);
        case 2: return m == 2 ? (uint16_t)(664 + r) : m == 7 ? 2073 : (uint16_t)(609 + 5 * v11 + r);
        case 3: return (uint16_t)(954 + 5 * v11 + r);
        case 4: return (uint16_t)(1009 + 5 * v11 + r);
        case 5: return (uint16_t)(5 * (v11 + 376) + r);
        case 6: return (uint16_t)(674 + 5 * v11 + r);
        case 7: return (uint16_t)(729 + 5 * v11 + r);
        case 8: return (uint16_t)(784 + 5 * v11 + r);
        case 9: return 840;
        case 10: return m == 5 ? (uint16_t)(604 + r) : (uint16_t)(434 + 5 * v11 + r);
        case 11: return (uint16_t)(489 + 5 * v11 + r);
        case 12: return m == 11 ? (uint16_t)(544 + 5 * v6 + r) : (uint16_t)(574 + 5 * v6 + r);
        case 13: return m == 2 ? (uint16_t)(1119 + r) : (uint16_t)(1064 + 5 * v11 + r);
        case 14: return 839;
        case 15: return m == 5 ? 1936 : m == 7 ? 1935 : (uint16_t)(5 * (v11 + 365) + r);
        case 16: return m == 5 ? 1993 : m == 7 ? 1992 : m == 18 ? 1994 : (uint16_t)(1937 + 5 * v11 + r);
        case 17: return m == 2 ? 1995 : m == 5 ? 2053 : m == 7 ? 2052 : (uint16_t)(1996 + 5 * v11 + r);
        case 18: return 2331;
        case 19: return 2051;
        case 20: return 842;
        default: return 899;
        }
    case 4:
        switch (m) {
        case 5: return 2054;
        case 6: return item.subtype <= 6 ? (uint16_t)(1229 + item.subtype) : NO_MODEL;
        case 7: return 2067;
        case 11: return 2062;
        case 18: return 1224;
        case 19: return 2369;
        case 20: return 2375;
        case 22: return 2360;
        case 23: return 670;
        case 25: return (uint16_t)(1669 + 5 * v5 + r);
        case 26: return (uint16_t)(1359 + 5 * v5 + r);
        case 27: return (uint16_t)(1459 + 5 * v5 + r);
        default: return (uint16_t)(1124 + 5 * v5 + r);
        }
    case 5:
        switch (m) {
        case 1: return (uint16_t)(1174 + 5 * v5 + r);
        case 5: return 2057;
        case 7: return 2070;
        case 11: return 2065;
        case 18: return 1226;
        case 19: return 2368;
        case 20: return 2374;
        case 22: return 2362;
        case 23: return 672;
        case 25: return (uint16_t)(1744 + 5 * v5 + r);
        case 26: return (uint16_t)(1434 + 5 * v5 + r);
        case 27: return (uint16_t)(1534 + 5 * v5 + r);
        default: return 7;
        }
    case 6:
        switch (m) {
        case 1: return (uint16_t)(1199 + 5 * v5 + r);
        case 5: return 2056;
        case 7: return 2069;
        case 11: return 2064;
        case 18: return 1227;
        case 19: return 2370;
        case 20: return 2376;
        case 22: return 2361;
        case 23: return 673;
        case 25: return (uint16_t)(1719 + 5 * v5 + r);
        case 26: return (uint16_t)(1409 + 5 * v5 + r);
        case 27: return (uint16_t)(1509 + 5 * v5 + r);
        default: return 433;
        }
    case 7:
        switch (m) {
        case 5: return 2055;
        case 7: return 2068;
        case 11: return 2063;
        case 18: return 1225;
        case 19: return 2372;
        case 20: return 2378;
        case 22: return 2359;
        case 23: return 671;
        case 25: return (uint16_t)(1694 + 5 * v5 + r);
        case 26: return (uint16_t)(1384 + 5 * v5 + r);
        case 27: return (uint16_t)(1484 + 5 * v5 + r);
        default: return (uint16_t)(1149 + 5 * v5 + r);
        }
    case 8: return m == 12 ? (uint16_t)(1594 + 5 * v5 + r) : (uint16_t)(1564 + 5 * v5 + r);
    case 9: return m == 12 ? (uint16_t)(1644 + 5 * v5 + r) : (uint16_t)(1619 + 5 * v5 + r);
    case 1: {
        static const uint16_t CONSUMABLES[10] = {852, 849, 850, 851, 2395, 2092, 2393, 2546, 2335, 2333};
        return item.subtype < 10 ? CONSUMABLES[item.subtype] : (uint16_t)843;
    }
    case 2: return 2421;
    case 10: return 2075;
    case 11: return resourceModel(item);
    case 12: return m == 12 ? 2085 : m == 11 ? 2086 : 2084;
    case 13: return 2357;
    case 14: {
        const int k = (int)item.rarity - 1;
        return k > 3 ? 2321 : k < 0 ? 2318 : (uint16_t)(2318 + k);
    }
    case 15: return 2367;
    case 16: return (uint16_t)(2313 + (item.seed & 1));
    case 17: return (uint16_t)(2113 + (item.seed & 3));
    case 18: return (uint16_t)((item.subtype == 1 ? 2120 : 2117) + item.seed % 3u);
    case 19: return item.subtype == 24 ? 2164 : item.subtype == 25 ? 2161 : 2080;
    case 20: return petFoodModel(item.subtype);
    case 21:
        switch (item.subtype) {
        case 3: return 2422;
        case 4: case 5: return 851;
        case 6: return 2550;
        case 7: return 609;
        case 8: return 2363;
        case 9: return 2395;
        default: return (uint16_t)(2081 + (item.subtype & 3));
        }
    case 23: return item.subtype == 1 ? 2548 : 2547;
    case 24: return 2433;
    case 25: return 2566;
    default: return NO_MODEL;
    }
}

// §7.2
int32_t poseOf(const Combatant &c) {
    static const int8_t POSES[111] = {
        0,  16, 15, 18, 17, 33, 20, 19, 9,  18, 10, 28, 29, 30, 31, 32, 35, 35, 17, 18, 50, 50, 21, 21, 22, 22,
        66, 67, 11, 0,  61, 61, 61, 61, 61, 62, 27, 23, 26, 24, 25, 24, 25, 23, 26, 26, 26, 1,  -1, 61, 22, 70,
        0,  0,  1,  22, 0,  2,  3,  4,  2,  49, 18, 6,  5,  48, 49, 37, 38, 39, 40, 41, 41, 42, 43, 44, 45, 46,
        47, 54, 51, 52, 53, 57, 55, 0,  60, 61, 61, 61, 61, 63, 61, -1, 26, 26, 65, 0,  8,  0,  0,  68, 0,  68,
        69, 61, 58, 59, 71, 72, 73,
    };
    int32_t pose = c.action >= 0 && c.action < 111 ? POSES[c.action] : 0;
    if (c.action == 48) {
        const int m = subtypeIn(c.mainHand()), o = subtypeIn(c.offHand());
        pose = m == BOW || m == CROSSBOW ? 22 : o == SHIELD ? 9 : o >= 0 ? 35 : 34;
    } else if (c.action == 93) {
        pose = c.offHand().type == 0 ? 2 : 35;
    }
    return pose;
}

// Trails §1.2: ages the histories once per pose call, by the rule of the pose.
static void ageTrail(PoseState &s, int32_t pose, float t, const Phases &P, float dt, const glm::mat4 &base,
              const glm::mat4 &centre, float yawDegrees) {
    auto &h = s.trail;
    enum { MAIN_TIP, MAIN_BUTT, OFF_TIP, OFF_BUTT };
    const float P1 = (float)P.windUp, P2 = (float)P.active, P3 = (float)P.recovery, D = (float)P.total();
    // Each point moves towards the (already moved) point before it.
    const auto chain = [&](int history, float rate) {
        const float w = 1 - std::pow(1 - rate, dt);
        for (size_t i = 1; i < 16; i++)
            h[(size_t)history][i] += (h[(size_t)history][i - 1] - h[(size_t)history][i]) * w;
    };
    const auto fill = [&](int history, glm::vec3 p) { h[(size_t)history].fill(p); };
    // Rule 1, pose 69: half a ring in front of the creature, growing to 20 blocks.
    if (pose == 69 && t > P1 && t < P1 + P2) {
        const float f = 20 * (t - P1) / (P1 + P2);
        const glm::mat4 M = Rz(centre, yawDegrees);
        for (size_t i = 0; i < 16; i++) {
            const double angle = (double)i * PI / 16 - PI / 2;
            const glm::vec3 u((float)std::sin(angle), (float)std::cos(angle), 0);
            h[MAIN_TIP][i] = glm::vec3(M * glm::vec4(f * u, 1));
            h[MAIN_BUTT][i] = glm::vec3(M * glm::vec4(0.5f * f * u, 1));
        }
        return;
    }
    // Rule 2: the swings that trail with both hands.
    static const int32_t BOTH[] = {1, 2, 3, 8, 10, 12, 14, 28, 29, 30, 31, 32, 35, 48, 49};
    if (std::find(std::begin(BOTH), std::end(BOTH), pose) != std::end(BOTH) && t > P1 && t < D) {
        for (int k = MAIN_TIP; k <= OFF_BUTT; k++)
            chain(k, 0.05f);
        return;
    }
    // Rule 3: off-hand strikes; the main hand's trail stays shut.
    if ((pose == 15 || pose == 17 || pose == 19) && t > P1) {
        chain(OFF_TIP, 0.05f);
        chain(OFF_BUTT, 0.05f);
        const glm::vec3 butt = h[MAIN_BUTT][0];
        fill(MAIN_TIP, butt);
        fill(MAIN_BUTT, butt);
        return;
    }
    // Rule 4: main-hand strikes, the outer half of the blade (pose 50: two points by the right
    // foot or fist).
    if ((pose == 16 || pose == 18 || pose == 20 || pose == 50) && t > P1 && t < P1 + P2 + P3 / 2) {
        if (pose == 50) {
            h[MAIN_TIP][0] = glm::vec3(base * glm::vec4(20, 0, -12, 1));
            h[MAIN_BUTT][0] = glm::vec3(base * glm::vec4(8, 0, -12, 1));
        } else {
            h[MAIN_BUTT][0] = 0.5f * (h[MAIN_BUTT][0] + h[MAIN_TIP][0]);
        }
        chain(MAIN_TIP, 0.075f);
        chain(MAIN_BUTT, 0.075f);
        const glm::vec3 butt = h[OFF_BUTT][0];
        fill(OFF_TIP, butt);
        fill(OFF_BUTT, butt);
        return;
    }
    // Rule 5: collapse; a strip of zero area is the cleared trail.
    for (auto &history : h)
        history.fill(history[0]);
}

void trailTriangles(const PoseState &s, const Combatant &c, glm::vec3 position, std::vector<TrailCorner> &out) {
    // §1.3: not for the dead, nor for those who fight from afar, nor past the action's end.
    if (c.health <= 0 || rangedUser(c) || c.actionMs > (float)duration(c, c.action))
        return;
    // Alpha per point pair: k = (4x(1 - x))², x = i / 15; the butt edge at 50·k, the tip edge
    // at 200·k (main hand) or 255·k (off hand).
    std::array<double, 16> k;
    for (size_t i = 0; i < 16; i++) {
        const double x = (double)i / 15;
        k[i] = std::pow(4 * x * (1 - x), 2);
    }
    const auto strip = [&](const std::array<glm::vec3, 16> &tip, const std::array<glm::vec3, 16> &butt, double tipAlpha) {
        bool open = false; // a collapsed strip has no area; skip it
        for (size_t i = 1; i < 16 && !open; i++)
            open = tip[i] != tip[0] || butt[i] != butt[0];
        if (!open)
            return;
        const auto at = [&](glm::vec3 p) { return glm::vec3(position.x + p.x, position.y + p.y, s.trailHeight + p.z); };
        for (size_t i = 0; i + 1 < 16; i++) {
            const TrailCorner b0{at(butt[i]), (uint8_t)(50 * k[i])}, t0{at(tip[i]), (uint8_t)(tipAlpha * k[i])};
            const TrailCorner b1{at(butt[i + 1]), (uint8_t)(50 * k[i + 1])},
                t1{at(tip[i + 1]), (uint8_t)(tipAlpha * k[i + 1])};
            out.insert(out.end(), {b0, t0, b1, t0, t1, b1});
        }
    };
    strip(s.trail[0], s.trail[1], 200);
    if (c.offHand().type == 3)
        strip(s.trail[2], s.trail[3], 255);
}

bool stepWalk(PoseState &s, const Appearance &a, const Motion &m, float dt) {
    const float before = s.phase;
    const float dts = dt / 1000.f;
    const float speed = glm::length(m.velocity);
    const float speedXY = glm::length(glm::vec2(m.velocity));
    // k = sqrt(0.8 / S), S taken as the collision box width (open question M-4).
    const float k = std::sqrt(0.8f / std::max(a.box.x, 0.01f));
    const bool flyer = (a.flags & FLYER) != 0;
    const int32_t action = m.fighter ? m.fighter->action : 0;
    if (speed * speed > 0.5f && action != 107 && (m.onGround || m.inWater || m.climbing || flyer)) {
        if (m.climbing) {
            s.phase += k * speed * dts * 6;
            s.amplitude += 0.2f * dts;
        } else {
            bool windUp = false;
            if (m.fighter && m.onGround) {
                const Combatant &c = *m.fighter;
                const float P1 = (float)phases(c, c.action).windUp;
                switch (c.action) {
                case 48: windUp = true; break;
                case 54: windUp = c.actionMs < (float)duration(c, 54); break;
                case 6: case 7: case 10: case 17: case 18: case 19: case 20: windUp = c.actionMs < P1; break;
                }
            }
            if (windUp)
                s.phase += k * dt * 0.0001f * speedXY * 1.5f;
            else if (m.onGround)
                s.phase += k * speedXY * dts * 1.5f * (action == 79 ? 2.f : 1.f);
            else
                s.phase += k * dt * 0.002f * speedXY * 1.5f;
            s.amplitude += dts;
        }
    } else if (!m.onGround && !m.inWater && !m.climbing) {
        s.phase = 0;
        s.amplitude += k * dts;
    }
    s.amplitude = relax(std::min(s.amplitude, 1.f), 0, 0.005f, dt);
    const float speedXYNow = glm::length(glm::vec2(m.velocity));
    return m.dodgeMs <= 0 && !m.inWater && speedXYNow > 5 && std::floor(s.phase / PIf) > std::floor(before / PIf);
}

std::vector<PartDraw> poseParts(PoseState &s, const Appearance &a, const Motion &m, float dt,
                                const std::function<glm::vec3(uint16_t)> &voxels) {
    const bool quadruped = (a.flags & QUADRUPED) != 0, flyer = (a.flags & FLYER) != 0;
    const Combatant *c = m.fighter;
    // Trails §1.1: a creature that rises or falls leaves its trail where it was made.
    for (auto &history : s.trail)
        for (glm::vec3 &p : history)
            p.z += s.trailHeight - m.height;
    s.trailHeight = m.height;
    // The engine keeps an ended action's id for the chains (Combat.cpp); Alpha clears it a frame
    // after its end (phase E), so past that the pose reads it as action 0.
    int32_t action = c ? c->action : 0;
    float t = c ? c->actionMs : 0;
    if (c && t >= (float)duration(*c, action) + 50)
        action = 0, t = 0;
    int32_t pose = 0;
    if (c && action != 0) {
        Combatant now = *c;
        now.action = action;
        pose = poseOf(now);
    }
    // (§7.2: record field 17 > 0 forces pose 56, the stunned pose below; which field that is
    // is only inferred, and hit-stun is not it: landing stuns would show it. Not forced yet.)
    Phases P;
    if (c)
        P = phases(*c, action);
    const float P1 = (float)P.windUp, P12 = (float)(P.windUp + P.active), D = (float)P.total();
    const int main = c ? subtypeIn(c->mainHand()) : -1, off = c ? subtypeIn(c->offHand()) : -1;
    const bool offShield = off == SHIELD;

    // §7.4 step 1: the swing.
    // Pose contract §3: the argument and the sine are narrowed, the product formed in binary64.
    const float s0 = (float)std::sin((double)(float)((double)s.phase + PI / 2));
    float swing = (float)(((double)s.amplitude * std::bit_cast<double>(0x400197C982C286E6ull)) * (double)s0);
    if (action == 79)
        swing *= 0.5f;
    if (action == 106 || action == 107)
        swing *= 0.1f;
    if ((m.running && !m.climbing) || action == 48)
        swing *= 1.25f;
    if (flyer && !m.onGround)
        swing = 0;
    if (quadruped)
        swing *= 0.75f;
    const float swingKept = swing; // s′
    if (main == LONGSWORD && action != 0)
        swing *= 0.25f;
    if (pose == 53)
        swing *= 0.1f;
    if (action == 22 || action == 36 || action == 63 || action == 8 || action == 28 || pose == 2 || pose == 3 ||
        pose == 48 || pose == 49 || pose == 37)
        swing *= 0.5f;
    if (m.gliding)
        swing = 0;

    Frame f;
    // §7.7 step 1, §7.4 step 2: legs.
    f.legL = (float)(1.5 * swingKept * 180.0 / PI);
    f.legR = -f.legL;
    switch (pose) {
    case 57: case 52: case 58: case 59: f.legL = f.legR = 40; f.ell = 10; break;
    case 55: f.legL = f.legR = -10; break;
    }
    // Step 2: the pivot height while gliding.
    if (m.gliding && !m.onGround && m.dodgeMs <= 0)
        f.h = a.box.z / 2;
    // Step 3: base targets.
    const glm::vec3 h = a.handOffset;
    const float amp = s.amplitude;
    if (quadruped) {
        f.Rh = h;
        f.L = {-h.x, h.y, h.z};
    } else {
        const float r = h.x * (1 + amp);
        f.Rh = {std::cos(swing) * r, h.y + std::sin(swing) * r + 2.1f - 5 * amp, h.z - 4.1f + 6 * amp};
        f.L = {-std::cos(swing) * r, h.y - std::sin(swing) * r + 2.1f - 5 * amp, h.z - 4.1f + 6 * amp};
    }
    auto armOf = [&](glm::vec3 hand, float side) {
        return glm::vec3((hand.x + h.x * -0.3f * side) * 0.5f, (hand.y - 8) * 0.25f, (hand.z + h.z + 12) * 0.5f);
    };
    f.armR = armOf(f.Rh, 1);
    f.armL = armOf(f.L, -1);
    f.footR = a.footOffset;
    f.footL = {-a.footOffset.x, a.footOffset.y, a.footOffset.z};
    if (pose == 0) {
        f.rho = a.handRotation;
        f.lambda = {a.handRotation.x, -a.handRotation.y, -a.handRotation.z};
    }
    if (c) {
        f.mainModel = itemModel(c->mainHand());
        f.offModel = off == QUIVER ? NO_MODEL : itemModel(c->offHand());
        if (c->mainHand().type != 3)
            f.mainModel = NO_MODEL;
        if (c->offHand().type != 3)
            f.offModel = NO_MODEL;
    }
    // Step 5: sitting.
    if (m.sitting) {
        f.B = {-70, 0, 0};
        f.H.x = 70;
        f.dL = f.dR = {0, 3, 1};
        if (main == CROSSBOW)
            f.dR.y = 11;
        f.E.z = (a.box.y - a.box.z) / 2;
        f.K = {0, -1, 0};
        f.legL = f.legR = -20;
        if (action != 0 && t < D)
            f.dL += glm::vec3(0, -5, 6), f.dR += glm::vec3(0, -5, 6);
    }

    // §7.8: the shared blocks.
    auto G = [&](glm::vec3 g) {
        f.L = f.Rh + g;
        f.armL.y += 10;
        f.armL.z -= 4;
    };
    auto stance = [&](bool pose35) {
        f.U = glm::vec3(0);
        if (c->type != 87 && c->type != 96) { // moles and zombies keep their arms down
            f.rL.x = pose35 ? -60.f : -90.f;
            f.T.z = 20;
            f.rR = {-120, -90, 30};
        }
        if (!offShield) {
            f.rL = {-80, 40, 60};
            f.dL.y -= 4;
            f.dL.z += 2;
        }
        if (f.mainModel != NO_MODEL && f.offModel == NO_MODEL) {
            f.Rh += glm::vec3(0, 2, 3);
            G({-3, 0, -3});
            f.rR.x = 0;
            f.rR.y = 20;
            f.rL = f.rR;
        }
    };
    const bool W = t < P1, A = t >= P1 && t < P12, Rr = t >= P12 && t < D, E = t >= D;
    const float u = P.active > 0 ? clamp01((t - P1) / (float)P.active) : 1;
    auto guardIdle = [&] {
        f.T.z = 0;
        f.B.z = 0;
        f.rR = {60, -20, 20};
        f.dR = {0, 2 + std::sin(0.01f * t), 4 + std::cos(0.01f * t)};
        f.rL = {60, 20, -20};
        f.dL = {0, 2 + std::sin(0.01f * t + PIf / 2), 4 + std::cos(0.01f * t + PIf / 2)};
        f.rate = 0.03f;
    };
    auto casting = [&] { // pose 61 W
        f.T.x = 20;
        f.H.x = 20;
        f.dL = {3 * std::cos(0.005f * t) - 1, 5, 5 * std::sin(0.005f * t) + 5};
        f.dR = {3 * std::cos(0.005f * t + PIf) + 1, 5, 5 * std::sin(0.005f * t + PIf) + 5};
    };
    // Poses 23, 24, 25, 26 W and 27: the hands wobble while the spell is held.
    auto wobble = [&] {
        f.dR.x += 0.5f * std::sin(0.04f * t);
        f.dL.x += 0.5f * std::sin(0.04f * t);
        f.dR.z += 0.5f * std::cos(0.05f * t);
        f.dL.z += 0.5f * std::cos(0.05f * t);
    };
    auto spell = [&] { // pose 23 W
        f.T.z = -30;
        f.rR = {0, -45, -60};
        f.rL = {-45, -45, 0};
        f.dR.y += 4;
        f.dL.y += 4;
        wobble();
    };

    // Step 6: the pose case (Appendix C, with the cells resolved in alpha-trails-poses-schedules.md
    // §2). The few cells still open there are left at defaults.
    switch (pose) {
    case 1: {
        const bool two = twoHanded(main);
        if (t < 500) {
            const float q = clamp01(t / 500);
            if (two) {
                f.rR.x = 120 * q;
                f.dR = {-f.Rh.x, 0, 14};
                f.dL = {2 - f.dL.x, 0, 12};
            } else {
                f.dR = {1, 0, 14};
                f.dL = {-1, 0, 14};
                f.rL.x = f.rR.x = -100;
            }
            f.T.x = -40 * q;
            f.B.x = -40 * q;
            f.F = -50 * q;
        } else if (t < D) {
            f.B.x = -60 * (1 - std::min(2 * (t - 500) / 500, 1.f));
            f.T.x = -20;
            f.rR.x = -70;
            if (two) {
                f.dR = {-f.Rh.x, 4, 0};
                f.dL = {2 - f.dL.x, 4, 0};
            } else {
                f.dR = {1, 3, 8};
                f.dL = {-1, 3, 8};
                f.rL.x = -150;
            }
            f.rate = 0.1f;
        } else if (two) {
            f.Rh += glm::vec3(-8, 4, 0);
            G({-2, 0, -1});
            f.rR.x = -60;
            f.rR.z = 20;
        } else {
            f.dR = {1, 3, 2};
            f.dL = {-1, 3, 2};
            f.rL.x = f.rR.x = -250;
        }
        if (offShield)
            f.rL = {-250, -90, 0};
        break;
    }
    case 2:
        if (W) {
            const float q = clamp01(1.5f * t / P1);
            f.B.x = 5 + 10 * q;
            f.T = {10, 10, -60};
            f.rR = {lerp(90, 60, q), 0, lerp(90, 60, q)};
            f.dR = f.dL = {0, 0, 14 * q};
            G({2, 0, 0});
            f.rL = f.rR;
            f.rate = 0.01f;
        } else if (A) {
            f.B.x = -10;
            f.T.y = 10;
            f.T.z = 60;
            f.rR = {-70, 0, -60};
            f.dR = f.dL = {0, 2 * std::sin(PIf * u / 2), 0};
            G({2, -1, 0});
            f.rL = f.rR;
            f.rate = 0.04f;
        } else {
            f.Rh += glm::vec3(0, 2, 3);
            G({-3, 0, -3});
            f.rR.x = 0;
            f.rR.y = 20;
            f.rL = f.rR;
        }
        break;
    case 3:
        if (W) {
            f.B.z = 0;
            f.rR = {-90, 0, 0};
            f.dR = f.dL = {-5, 8, 0};
            G({2, 0, 0});
            f.rL = f.rR;
            f.rate = 0.005f;
        } else if (t < P1 + (float)P.active / 2) {
            f.B = {10, 0, -10};
            f.T = {30, 0, -30};
            f.rR = {0, 0, 90};
            f.dR = f.dL = {-8, -4, 20};
            G({2, 0, 0});
            f.rL = f.rR;
            f.rate = 0.005f;
        } else if (t < D) {
            f.B.z = -30;
            f.T = {0, 20, -60};
            f.rR = {70, 0, 90};
            f.dR = f.dL = {5, -2, 0};
            G({2, 0, 0});
            f.rL = {70, 0, 90};
            f.rate = 0.02f;
        } else {
            G({-2, 0, -1});
            f.rR.y = 90;
            f.T.z = -80;
            f.rL = f.rR;
        }
        break;
    case 4:
        if (W) {
            const float q = clamp01(t / P1);
            f.B.x = 5 * q;
            f.T.y = 10 * q;
            f.T.z = -20 * q;
            f.rR = {60 * q, lerp(120, 60, q), 60 * q};
            f.rate = 0.005f;
        } else if (c->characterClass == 1 && c->specialization == 1) {
            f.dR = {2, 7, 4};
            f.rR = {0, -60, 0};
            f.rate = 0.01f;
        } else {
            const float q2 = std::min(1.5f * (t - P1 / 2) / std::max(1.f, P1 / 2), 1.f); // 1 from P1 on (as read)
            f.rate = 0.005f;
            f.T = {20, 0, 0};
            f.rR = {lerp(90, 30, q2), 0, lerp(90, 0, q2)};
            f.Rh.z += 0.5f * std::cos(0.5f * t);
            f.dR = {-6, 0, 16 * q2};
        }
        if (!W)
            f.dL = f.dR;
        G({2, 0, 0});
        f.rL = f.rR;
        break;
    case 5:
        f.rR = {-90, 0, -40};
        f.rL = {-90, 0, 40};
        f.Rh.z += 0.5f * std::cos(0.5f * t);
        f.L.z += 0.5f * std::cos(0.5f * t);
        f.dR = {1, -7, 0};
        f.dL = {-1, -7, 0};
        f.T.z = -40;
        f.rate = 0.005f;
        break;
    case 6:
        f.Rh.z += 0.5f * std::cos(0.5f * t);
        f.L.z += 0.5f * std::cos(0.5f * t);
        f.rate = 0.005f;
        if (c->specialization == 1) {
            f.rR = {-50, -60, 40};
            f.rL = {-90, 90, -50};
            f.dR = {-3, 7, 3};
            f.dL = {-2, 2, 0};
            f.T.z = -20;
        } else {
            f.rR = {-90, 0, -40};
            f.rL = {-90, 90, -40};
            f.dR = {1, -7, 0};
            f.dL = {-1, -7, 0};
            f.T.z = -40;
        }
        break;
    case 8:
        if (f.offModel != NO_MODEL) {
            if (t < 100)
                f.T.z = 80, f.rL = {0, 0, 90}, f.rR = {-45, 45, 0};
            else if (t < 500)
                f.T.z = -90, f.rL = {0, 60, 90}, f.L.z += 5, f.rR = {-90, 0, 45};
            else
                f.T.z = -30, f.rL = {0, 100, 60}, f.rR = {-45, 45, 0};
        } else if (t < 100) {
            f.T.z = -80, f.rR = {0, 0, 90};
        } else if (t < 500) {
            f.T.z = 90, f.rR = {0, 60, -85};
            f.dL += glm::vec3(2, 3, 3);
            f.dR += glm::vec3(2, 3, 3);
        } else {
            f.Rh += glm::vec3(0, 2, 3);
            f.rR.x = 0;
            f.rR.y = 20;
        }
        G({-3, 0, -3});
        f.rL = f.rR;
        break;
    case 9:
        if (W) {
            f.T.z = 80;
            f.rL = {0, 0, 90};
            f.rR = {-45, 45, 0};
            if (action == 48)
                f.B.z = 360 * t / 300;
        } else {
            f.T.z = -90;
            f.rL = {0, 0, 90};
            f.rR = {-90, 0, 45};
        }
        break;
    case 10:
        if (W) {
            f.T.z = 80;
            f.rL = {0, 0, 90};
            f.rR = {-45, 45, 0};
        } else if (!E) {
            f.T.z = -90;
            f.rL = {0, 0, 90};
            f.rR = {-90, 0, 45};
            f.L.x += std::sin(0.1f * (t - 600));
            f.rate = 0.03f;
        } else {
            stance(false);
        }
        break;
    case 11:
        if (!(a.flags & 0x0004)) {
            f.T.z = 60;
            f.rR = {0, -45, -60};
            f.rL = {-45, -45, 0};
        }
        break;
    case 15:
        if (W) {
            f.T.z = 40;
            f.rL = {0, -90, 30};
            f.dL.y -= 4;
            f.dL.z += 4;
            f.rR = {0, -90, -120};
            f.dR.y -= 6;
        } else if (!E) {
            f.T.z = -90;
            f.E.x = -0.1f;
            f.rL = {-90, -90, -30};
            f.rR = {0, -90, -120};
            f.dR.y -= 6;
        } else {
            stance(false);
        }
        break;
    case 16:
        if (W) {
            f.T.z = 90;
            f.rR = {0, -90, 0};
            f.rL = {-90, 0, 90};
            f.dR.y += 6;
            f.dR.z += 4;
        } else if (!E) {
            f.T.z = 20;
            f.rL = {-90, -60, 90};
            f.dL.y -= 4;
            f.dL.z += 4;
            f.rR = {0, -90, -220};
            f.dR.y -= 6;
            f.E.x = 0.1f;
            f.footL.y -= 4;
            f.footR.y += 4;
            f.turn = true;
        } else {
            stance(false);
        }
        if (offShield)
            f.rL = {0, 0, 90};
        break;
    case 17:
        if (W) {
            f.T.z = 40;
            f.rR = {-90, 90, 30};
            f.rL = {-90, 0, -20};
            f.dR.y -= 6;
        } else if (!E) {
            f.T.z = -90;
            f.rR = {-40, 0, 30};
            f.dR = {3, 0, 4};
            f.rL = {-90, 90, 90};
            f.turn = true;
            f.rate = 0.05f;
        } else {
            stance(false);
        }
        break;
    case 18:
        if (W) {
            f.T.z = -40;
            f.rL = {-90, -90, -30};
            f.rR = {-90, 0, 20};
            f.dR.y -= 6;
        } else if (!E) {
            f.T.z = 90;
            f.rL = offShield ? glm::vec3(0, 0, 40) : glm::vec3(-40, 0, -30);
            f.dL = {-3, 0, 4};
            f.rR = {-90, -90, -90};
            f.dR = {3, -1, 0};
            f.turn = true;
            f.rate = 0.05f;
        } else {
            stance(false);
        }
        break;
    case 19:
    case 20:
        if (t > P1 && t < D) {
            if (pose == 19) {
                f.T.z = -20;
                f.B.z = -20;
                f.rL = {0, 90, 40};
                f.dL = {0, 8, 4};
                f.rR = {0, 90, 30};
            } else {
                f.T.z = 45;
                f.B.z = 45;
                f.rR = {0, -90, -90};
                f.dR = {8, 0, 4};
                f.rL = {0, -90, -30};
            }
            f.H.x = -10;
            f.S.y = 3;
            f.rate = 0.03f;
        } else {
            guardIdle();
            if (pose == 19)
                f.S.z += 0.5f * std::sin(0.005f * t);
        }
        break;
    case 21:
    case 22: {
        const bool mainShoots = main == BOW || main == CROSSBOW;
        if (pose == 22) {
            if (!mainShoots) {
                f.dL += glm::vec3(-4, 3, 2);
                f.dR += glm::vec3(-10, 2, 1);
                f.T.z = -60;
                f.rL.z = 60;
                f.rL.y = -90;
                f.rR.z = 60;
                if (c->offHand().type == 3)
                    f.mainModel = 840;
                if (t < 300)
                    f.rL.x += 20;
            } else {
                f.dR += glm::vec3(4, 3, 2);
                f.dL += glm::vec3(10, 2, 1);
                f.T.z = 60;
                f.rR.z = -60;
                f.rR.y = -90;
                f.rL.z = -60;
                if (c->mainHand().type == 3)
                    f.offModel = 840;
                if (t < 300)
                    f.rR.x += 20;
            }
        } else if (mainShoots) {
            if (main == CROSSBOW)
                f.rR.y = -90;
            if (t < 2 * D) { // (as read: the release is reached only past 2·D)
                f.dR += main == BOW ? glm::vec3(7, 5, 4) : glm::vec3(3, 2, 4); // (open points 3 §5)
                f.dL += glm::vec3(10, 3, 3);
                f.T.z = 70;
                f.rR.z = f.rL.z = -70;
                if (t < D / 2) {
                    f.dR.z -= 2;
                    f.rR.x = -5;
                    f.dL = {5, 1, 0};
                    f.T.z = 50;
                    f.offModel = NO_MODEL;
                }
            } else {
                f.dL.x += 1;
                f.rL.x = f.rR.x = -30;
                f.rL.y = -45;
            }
            if (c->mainHand().type == 3)
                f.offModel = 840;
        } else {
            if (off == CROSSBOW)
                f.rL.y = -90;
            if (t < 2 * D) {
                f.dL += off == BOW ? glm::vec3(-7, 5, 4) : glm::vec3(-3, 2, 4); // (open points 3 §5)
                f.dR += glm::vec3(-10, 3, 3);
                f.T.z -= 70;
                f.rL.z = 70;
                f.rR.z = 70;
                if (t < D / 2) {
                    f.dL.z -= 2;
                    f.rL.x = -5;
                    f.dR = {-5, 1, 0};
                    f.T.z = -50;
                    f.mainModel = NO_MODEL;
                }
            } else {
                f.dR.x -= 1;
                f.rL.x = f.rR.x = -30;
                f.rR.y = 45;
                if (off == CROSSBOW)
                    f.rL.z = 50;
            }
            if (c->offHand().type == 3)
                f.mainModel = 840;
        }
        break;
    }
    case 23:
        spell();
        if (!W)
            f.T.z = 60;
        break;
    case 24:
    case 25:
        if (W) {
            f.dR.y += 4;
            f.dL.y += 4;
            f.T.z = pose == 24 ? 20.f : -20.f;
            wobble();
        } else if (pose == 24) {
            f.E.x = 0.5f;
            f.T.z = -60;
            f.B.z = -30;
            f.rL = {0, 45, 60};
            f.rR = {-45, 45, 0};
            f.dL = {-2, 0, 0};
            f.turn = true;
        } else {
            f.E.x = -0.5f;
            f.T.z = 60;
            f.B.z = 30;
            f.rR = {0, -45, -60};
            f.rL = {-45, -45, 0};
            f.dR = {2, 0, 0};
            f.turn = true;
        }
        break;
    case 26:
        if (W) {
            spell();
        } else if (!E) {
            f.T.z = 90;
            f.rR = {-90, 0, -90};
            f.rL = {-45, -45, 0};
            f.dR = {3, 0, 1};
            f.rate = 0.03f;
        } else {
            f.rR = {-60, 0, 0};
            f.dR = {3, 0, 1};
        }
        break;
    case 27:
        spell();
        break;
    case 28:
        if (W) {
            f.T.z = -80;
            f.rR = {-90, 90, -90};
            f.rL = {20, 90, 90};
        } else if (A) {
            f.T.z = -90;
            f.B.z = 360 * u;
            f.rL = {0, 100, 90};
            f.rR = {0, 90, 0};
            f.rate = 0.2f;
        } else {
            f.T.z = 80;
            f.rL = {90, 90, 270};
            f.rR = {0, 90, 90};
        }
        break;
    case 29:
        if (W) {
            f.rR = {-80, -90, 60};
            f.rL = {-80, 90, -60};
            f.dR.y += 3;
            f.dR.x -= 5;
            f.dL.x += 5;
        } else if (!E) {
            f.T = {0, 0, -30};
            f.dR.y -= 5;
            f.dL.y -= 5;
            f.rL = {-90, 90, 110};
            f.rR = {-90, -90, -110};
            f.E.y = -0.5f;
            f.rate = 0.04f;
        } else {
            stance(false);
        }
        break;
    case 30: case 31: case 32: case 33:
        if (W) {
            const float q = clamp01(t / P1);
            if (pose == 30)
                f.rR = {-80, 90, -90}, f.dR = {-10, 8, 8}, f.T.z = -45, f.B.z = -20 * q;
            else if (pose == 31)
                f.rR = {0, -90, 0}, f.dR = {-8, 6, 6}, f.H.x = -10, f.H.z = 30, f.T.z = 45, f.B.z = 20,
                f.E.x = 0, f.rate = 0.05f;
            else if (pose == 32)
                f.dR = {2, 2, 14}, f.rR = {50, 0, 0}, f.T.z = -45, f.B.z = -20 * q, f.E.x = 0.5f * q, f.rate = 0.04f;
            else
                f.dR = {4, 2, 8}, f.rR = {-90, -180, 90}, f.T.z = -90, f.E.y = -1;
        } else if (t < D) {
            const float q2 = clamp01(1.2f * u);
            const auto blend = [&](glm::vec3 to, glm::vec3 from) { return q2 * to + (1 - q2) * from; };
            if (pose == 30)
                f.E.x = -0.8f * q2, f.H.x = -10, f.T.z = 90 * q2 - 45 * (1 - q2), f.B.z = 30 * q2, f.ell = 10 * q2,
                f.rR = blend({-110, 60, 0}, {-90, 90, 48}), f.dR = blend({-12, 0, -1}, {-10, 8, 8}), f.rate = 0.05f;
            else if (pose == 31)
                f.E.x = 0.8f * q2, f.H.z = -30, f.T.z = 45 * (1 - q2) - 45 * q2, f.B.z = -30 * q2, f.B.x = -20 * q2,
                f.ell = 10 * q2, f.rR = blend({0, -90, -130}, {0, -90, 20}), f.dR = blend({0, 2, 0}, {-8, 6, 6}),
                f.rate = 0.05f;
            else if (pose == 32)
                f.B.x = 20 * (1 - clamp01(2 * u)) - 10 * clamp01(2 * u), f.T.z = 45 * clamp01(2 * u) - 45 * (1 - clamp01(2 * u)),
                f.rR = blend({-90, 0, -45}, {90, 0, 45}), f.dR = blend({0, 9, 0}, {2, 2, 14}), f.rate = 0.05f;
            else
                f.dR = {6, 0, 0}, f.E.y = 0.5f, f.T.z = 90, f.rR = {-90, 0, -90}, f.turn = true;
        } else {
            f.rate = 0.03f;
            f.dR = {2, 0, 0};
            f.rR = glm::vec3(0);
            f.T.z = -30;
        }
        f.dL = f.dR;
        f.rL = f.rR;
        f.L = f.Rh + glm::vec3(-0.2f, -0.2f, -1);
        break;
    case 34:
        f.rR = {-90, -90, 80};
        f.T.z = -80;
        break;
    case 35:
        if (W) {
            f.rR = {-90, 0, 0};
            f.rL = {-90, 0, 40};
            f.dR = {1, -7, 0};
            f.dL = {-1, -7, 0};
            f.T.z = -40;
            f.E.y = -0.5f;
            f.rate = 0.005f;
            f.turn = true;
        } else if (!E) {
            f.rR = {-90, -60, -30};
            f.rL = {-90, 60, -50};
            f.dR = {3, 5, 2};
            f.dL = {6, 8, 2};
            f.T.z = 40;
            f.rate = 0.05f;
            f.turn = true;
        } else {
            stance(true);
        }
        break;
    case 37:
        if (W) {
            f.rR = {-90, -90, 80};
            f.T.z = -80;
        } else if (!E) {
            f.rR = {-90, 0, -90};
            f.T.z = 90;
            f.Rh.x += 4;
            f.footL.y -= 4;
            f.footR.y += 4;
            f.turn = true;
            f.rate = 0.05f;
        }
        if (!E) {
            G({2, 0, 0});
            f.rL = f.rR;
        } else {
            f.Rh += glm::vec3(-8, 4, 0);
            G({-2, 0, -1});
            f.rR.x = -60;
            f.rR.z = 20;
            f.T.z = -20;
        }
        break;
    case 38:
        if (t < P1 + (float)P.active / 2)
            f.H = {-30, 10, 90}, f.T = {0, 0, 60}, f.K = {-3, -2, 0};
        else if (t < D)
            f.H = {30, -10, -30}, f.T = {10, 0, -20}, f.K = {3, 0, 2};
        break;
    case 39:
        if (t < P1 + (float)P.active / 2)
            f.H = {-70, 0, 0}, f.T = {-10, 0, 0}, f.K = {0, -3, -1}, f.rL.x = f.rR.x = 20;
        else if (t < D)
            f.H = {50, 0, 0}, f.T = {20, 0, 0}, f.S = {0, -3, 1}, f.K = {0, 0, 2}, f.rR.x = f.rL.x = -20,
            f.Q = {-10, 0, 0};
        break;
    case 40:
        if (W)
            f.B.z = 90, f.H = {0, 10, -60}, f.Q = {0, 0, -70}, f.rate = 0.002f;
        else
            f.B.z = 200, f.H = {0, -20, 60}, f.K = {-3, 0, -2}, f.Q = {0, 0, 70};
        break;
    case 41: {
        // The creature curls up toward its body and uncurls.
        float k = 0;
        if (W)
            k = P1 > 0 ? t / P1 : 1;
        else if (A)
            k = 1, f.curled = true, f.S = {0, 0, -3};
        else if (Rr)
            k = 1 - (t - P12) / std::max(1.f, (float)P.recovery);
        const glm::vec3 centre = a.bodyOffset;
        f.Rh = glm::mix(f.Rh, centre, k);
        f.L = glm::mix(f.L, centre, k);
        f.footR = glm::mix(f.footR, centre, k);
        f.footL = glm::mix(f.footL, centre, k);
        break;
    }
    case 42:
        if (W)
            f.H = {-10, 10, 30}, f.T = {-10, 0, 0}, f.K = {-3, 0, -1}, f.rL.x = f.rR.x = 20;
        else
            f.H = {50, t < P12 ? -30.f : 30.f, 0}, f.T = {30, 0, 0}, f.S = {0, -1, 2}, f.K = {0, 0, 2}, f.rR.x = -60,
            f.rL.x = -20, f.Q = {-20, 0, 0};
        f.rate = 0.005f;
        break;
    case 43:
        if (W)
            f.H = {-20, 0, 0}, f.T = {50, 0, 0}, f.S = {0, -2, 2}, f.K = {0, -6, -2}, f.rL.x = 20, f.rR.x = 30,
            f.Q = {-50, 0, 0};
        else
            f.H = {-10, 0, 0}, f.K = {0, 0, 1}, f.rate = 0.02f;
        break;
    case 44:
    case 73:
    {
        // Five steps: the wind-up, three thirds of the strike, then back to the wind-up.
        const float P2 = (float)P.active;
        const auto windUp = [&] {
            f.H = {10, 0, 0}, f.T = {-20, 0, 0}, f.S = {0, -1, 0}, f.K = {0, 1, 1}, f.rL.x = f.rR.x = 20;
            f.dL.z = f.dR.z = 2, f.Q = {30, 0, 10};
        };
        if (t < P1)
            windUp();
        else if (t < P1 + P2 / 3)
            f.H = {-30, 0, 0}, f.T = {50, 0, 0}, f.S = {0, 0, 1}, f.K = {0, -4, 2}, f.rL.x = 60, f.rR.x = 70,
            f.Q = {-30, 0, 0};
        else if (t < P1 + 2 * P2 / 3)
            f.H = {-30, 0, 0}, f.T = {40, 0, 0}, f.S = {0, 1, 1}, f.K = {0, -2, 2}, f.rL.x = 70, f.rR.x = -30,
            f.dR = {0, 3, -2}, f.Q = {-30, 0, -10};
        else if (t < P1 + P2)
            f.H = {-30, 0, 0}, f.T = {10, 0, 0}, f.S = {0, 1, 1}, f.K = {0, -2, 2}, f.rL.x = -30, f.rR.x = -40,
            f.dL = {0, 3, -2}, f.Q = {-30, 0, -10};
        else if (t < D)
            windUp(), f.H = {10, 0, -10};
        break;
    }
    case 45:
        if (W)
            f.T.x = -10, f.H.x = 10, f.Q.x = -60;
        else if (A)
            f.rL.x = f.rR.x = 80, f.H.x = -60, f.B.x = -360 * u, f.K = {0, -3, 0};
        else
            f.T.x = 20, f.H.x = -10, f.K = {0, -2, 0};
        break;
    case 46:
    case 47: {
        const float side = pose == 46 ? 1.f : -1.f;
        if (W)
            f.T.z = -10 * side, f.T.y = 30 * side, f.rL.y = f.rR.y = -30 * side, f.H.z = 60 * side;
        else if (A)
            f.rL.y = f.rR.y = 80 * side, f.B.y = -360 * u * side;
        else
            f.T.y = -30 * side, f.rL.y = f.rR.y = 30 * side;
        break;
    }
    case 48:
        if (W) {
            const float q = clamp01(t / P1);
            f.T.y = 10 * q;
            f.T.z = -80 * q;
            f.rR = {-90, 90 * q, -30 * q};
            G({2, 0, 0});
        } else {
            if (A)
                f.T = {10, 0, -90}, f.rR = {0, 110, -20}, f.L = f.Rh;
            else if (Rr)
                f.B.z = 20, f.T = {0, 0, 90}, f.rR = {0, 90, 90}, f.L = f.Rh;
            else
                G({-2, 0, -1}), f.rR.y = 90, f.T.z = -80;
            f.armL.y += 10;
            f.armL.z -= 4;
            if (!E)
                f.B.z = 360 * clamp01((t - P1) / std::max(1.f, D - P1));
        }
        f.rL = f.rR;
        break;
    case 49:
        if (W) {
            f.B.z = 30;
            f.T = {0, 0, 90};
            f.rR = {-90, -90, 90};
            G({2, 0, 0});
            f.rate = 0.005f;
        } else if (!E) {
            f.rR = {-90, -90, 0};
            if (t < P12)
                f.E.x = 0.25f;
            G({-2, 0, 0});
            f.B.z = -360 * clamp01((t - P1) / std::max(1.f, D - P1));
            f.rate = 0.03f;
        } else {
            G({-2, 0, -1});
            f.rR = {-90, 90, 0};
        }
        f.rL = f.rR;
        break;
    case 50:
        f.rR = {60, -20, 20};
        f.dR = {0, 2, 4};
        f.rL = {60, 20, -20};
        f.dL = {0, 2, 4};
        if (W || A) {
            f.T.z = 50;
            f.rL.x = -90;
            f.rL.z = 90;
            f.rR.x = -90;
            f.dL = {-4, -4, -2};
            f.dR = {0, 4, 1};
            if (A) {
                const float q = clamp01(1.5f * u);
                f.B.z = -270 * q;
                f.B.y = -40 * q;
                f.S.z += 4;
                f.ell = 40 * q;
                f.H.x = -30;
                f.H.z = -60;
                f.rate = 0.03f;
            }
        } else {
            guardIdle();
        }
        break;
    case 51:
    case 52:
        if (pose == 52)
            f.E.z = -0.14f, f.T.x = 10, f.K = {0, 2, -2};
        if (t < 3000) { // the held item's use limit (§6.5); consumables' own limits come with items
            f.H = {45 + 5 * std::cos(0.02f * t), 0, 0};
            f.T = {10, 0, 0};
            f.B = {10, 0, 0};
            f.dL = {1, 7, 8};
            f.dR = {-1, 7, 8};
            f.rL = f.rR = {100, 0, 0};
        }
        break;
    case 53:
        f.T = {-30, 0, 0};
        f.dL = {4, 6, 6};
        f.dR = {-4, 6, 6};
        f.S.y = 3;
        break;
    case 54:
        if (glm::dot(m.intent, m.intent) > 0) {
            f.B.x = 20;
            f.T.x = -10;
            f.H.x = -20;
            f.rL.x = f.rR.x = -20;
            f.dL.y += 4;
            f.dR.y += 4;
            f.F = -40;
            f.footL += glm::vec3(0, 4, 3);
            f.footR += glm::vec3(0, 4, 3);
        }
        break;
    case 56: // stunned
        f.T.x += 30;
        f.dL = {-2, 0, 0};
        f.dR = {2, 0, 0};
        f.rL = {-10, -10, 90};
        f.rR = {-10, 10, -90};
        break;
    case 55:
        f.B = {-90, 0, 0};
        f.H.x = 20;
        f.E.z = (a.box.y - a.box.z) / 2 - 0.15f;
        f.K = {0, -1, 0};
        f.dL = f.dR = {0, 0, 3};
        break;
    case 57:
        f.E.z = -0.14f;
        f.T.x = 10;
        f.dL = f.dR = {0, 4, 0};
        f.K = {0, 2, -2};
        break;
    case 59:
        f.T.x = 20;
        f.E.z += 1.25f;
        f.B.z = -50;
        f.dL = {4, 6, -3};
        f.dR = {-4, 6, -3};
        f.K = {0, 1, 0};
        f.H = {0, 0, 30};
        f.rL = {60, 90, 60};
        f.rR = {60, -90, -60};
        break;
    case 60:
        if (t < 600)
            f.T.z = -80, f.rR = {-90, 90, -90}, f.rL = {20, 90, 90};
        else if (t < 5000)
            f.B.z = 360 * t / 300, f.rL = {0, -90, 0}, f.rR = {0, 90, 0}, f.rate = 0.2f;
        else
            f.T.z = 80, f.rL = {90, 90, 270}, f.rR = {0, 90, 90};
        break;
    case 61:
        if (W) {
            casting();
        } else if (A) {
            f.T.x = -20;
            f.T.z = 10;
            f.dL = {2, 4, 0};
            f.dR = {-2, 6, 0};
            f.rL = {0, 45, 0};
            f.rR = {0, -45, 0};
        } else {
            f.dL = {-3, -6, 0};
            f.dR = {3, -6, 0};
            f.rL = {-90, 0, 130};
            f.rR = {-90, 0, -130};
            f.rate = 0.03f;
        }
        break;
    case 62:
        casting();
        break;
    case 63:
        // Alternating 500 ms windows: right, both, left, both, ...
        if (t < P12 + 400) {
            const int64_t k500 = (int64_t)std::floor((t - P1 + 100) / 500), k1000 = (int64_t)std::floor((t - P1 + 100) / 1000);
            if (t <= P1 || (k500 & 1))
                f.dL = {2, -2, 12}, f.dR = {-2, -2, 12}, f.T.x = 20, f.B.x = 10;
            else if (k1000 & 1)
                f.dL = {2, -2, 12}, f.dR = {0, 0, -5}, f.T = {-10, 0, 20}, f.B = {-5, 0, 10}, f.rate = 0.03f;
            else
                f.dL = {0, 0, -5}, f.dR = {-2, -2, 12}, f.T = {-10, 0, -20}, f.B = {-5, 0, -10}, f.rate = 0.03f;
        }
        break;
    case 65:
        if (t < D)
            f.T.z = 90 * std::sin(0.03f * t), f.dL = f.dR = glm::vec3(0), f.rate = 0.04f;
        break;
    case 66:
        if (c->spentCharge > 0) { // open points 3 §6: a charged throw, a boomerang in each hand
            if (W) {
                f.T.z = -90;
                f.dR = {2, -2, 1};
                f.rR = {0, 10, 0};
                f.offModel = f.mainModel;
            } else if (A) {
                f.T.z = 10;
                f.dL = {-2, -2, 1};
                f.rL = {0, -10, 0};
                f.offModel = f.mainModel;
                f.mainModel = NO_MODEL;
            } else {
                f.T.z = 90;
                f.mainModel = f.offModel = NO_MODEL;
            }
            if (!W)
                f.B.z = std::min(360 * (t - P1) / 200, 700.f);
        } else if (W) {
            f.T.z = -90;
            f.dR = {2, -2, 1};
            f.rR = {0, 10, 0};
        } else {
            f.T.z = 90;
            f.mainModel = NO_MODEL; // the boomerang is in flight
        }
        break;
    case 67:
        f.T.z = -90 * c->charge;
        f.dR = {2, -2, 1};
        f.rR = {0, 10, 0};
        f.Rh.z += 0.5f * std::cos(0.5f * t);
        f.L.z += 0.5f * std::cos(0.5f * t);
        if (c->charge > 0)
            f.offModel = f.mainModel; // a second boomerang in the off hand
        break;
    case 7:
        // (Its U channel blends record fields 24 and 25, which the engine does not keep yet.)
        f.rR = {0, 90, 0};
        if (twoHanded(main))
            G({2, 0, 0});
        f.rate = t < D / 2 ? 0.005f : 0.03f;
        break;
    case 68:
        if (W)
            f.T.x = -20, f.rL = f.rR = {-90, 0, 0}, f.dR = {1, -2, 0}, f.dL = {-1, -2, 0};
        else
            f.rL = {0, 45, 0}, f.rR = {0, -45, 0}, f.dR = {-2, 5, 0}, f.dL = {2, 6, 0};
        break;
    case 69:
        if (t < P12)
            f.rR = {-90, -90, -90}, f.dR = {4, 3, 0}, f.T.z = -60 * t / std::max(1.f, P12), f.rL = {0, 0, 90},
            f.B.z = -360 * t / std::max(1.f, P12), f.rate = 0.03f;
        else
            f.rL = {0, 0, 90}, f.rR = {-60, -60, -60};
        break;
    case 70:
        f.B.x = W ? -30.f : 120.f;
        break;
    case 71:
        if (W)
            f.dR = {-3, -2, 10}, f.dL = {3, -2, 10}, f.T.x = 40, f.B.x = 10;
        else if (A)
            f.dR = f.dL = {0, 5, 0}, f.T.x = -20, f.B.x = -40;
        else
            f.dR = {2, -1, 0}, f.dL = {-2, -1, 0};
        break;
    case 72:
        if (t < 5000)
            f.T.x = 20, f.H.x = 30, f.dL = f.dR = {0, 0, 16};
        break;
    default:
        break;
    }
    // Step 8: arm targets.
    f.armL = 0.5f * (f.armL + f.L);
    f.armR = 0.5f * (f.armR + f.Rh);
    // Steps 9–12.
    if (m.knockedDown) {
        if (!quadruped)
            f.B = {90, 0, 0}, f.E.z = (a.box.x - a.box.z) / 2;
        else
            f.B = {0, 90, 0}, f.E.z = (a.box.y - a.box.z) / 2;
    }
    if (m.climbing) {
        f.dL.z += 7;
        f.dR.z += 7;
        f.rL.x += 90;
        f.rR.x += 90;
    }
    if (off == 20) { // a torch in the off hand
        f.rL.x = 0;
        f.dL.y += 4;
        f.dL.z += 4;
    }
    if (!quadruped && action != 106 && action != 107) {
        const glm::vec2 vxy(m.velocity);
        if (m.running && !m.climbing)
            f.B.x -= 25, f.H.x += 10;
        else if (glm::dot(vxy, vxy) > 0.1f && glm::dot(m.intent, m.velocity) > 0)
            f.B.x -= 10, f.H.x += 5;
    }
    // Weapons go on the back between fights (§6.5).
    if (c) {
        switch (action) {
        case 0: case 21: case 79: case 80: case 81: case 82: case 83: case 84: case 85: case 91: case 96: case 105:
        case 106: case 107: case 109:
            f.mainOnBack = main != FIST && main != BRACELET;
            f.offOnBack = off != 20 && off != FIST && off != BRACELET;
            break;
        }
        if (m.gliding && main != FIST && main != BRACELET)
            f.mainOnBack = true;
    }

    // Step 13: smoothing, one weight for every channel.
    const float w = s.fresh ? 1.f : 1.f - std::pow(1.f - f.rate, dt);
    s.fresh = false;
    auto move = [&](glm::vec3 &p, glm::vec3 target) { p += (target - p) * w; };
    auto turn = [&](glm::vec3 &p, glm::vec3 target) {
        for (int k = 0; k < 3; k++)
            p[k] += (float)(std::asin(std::sin(glm::radians((double)target[k] - p[k]))) * 180.0 / PI) * w;
    };
    move(s.turnR, f.rR);
    move(s.turnL, f.rL);
    move(s.handR, f.dR);
    move(s.handL, f.dL);
    move(s.torso, f.T);
    move(s.lean, f.E);
    s.pivot += (f.h - s.pivot) * w;
    turn(s.tilt, f.B);
    turn(s.head, f.H);
    turn(s.tail, f.Q);
    s.aim += (f.Y - s.aim) * w;
    move(s.roll, f.U);
    move(s.shift, f.S);
    move(s.headShift, f.K);
    // Step 14: composition.
    const glm::vec3 rho = f.rho + s.turnR, lambda = f.lambda + s.turnL;
    const glm::vec3 Rhand = f.Rh + s.handR, Lhand = f.L + s.handL;
    const glm::vec3 armR = f.armR + 0.4f * s.handR, armL = f.armL + 0.4f * s.handL;
    s.rightHand = Rhand;
    s.leftHand = Lhand;

    // §7.4 step 7: the base frame (dodge roll, displayed rotation, pivot height, lean, tilt).
    glm::mat4 base = T(glm::mat4(1), {0, 0, s.lean.z});
    if (m.dodgeMs > 0) {
        glm::vec3 axis(m.velocity.y, -m.velocity.x, 0);
        if (c && c->type == 101)
            axis = m.velocity;
        // Pose contract §6 step 3: a squared length above 0.01, the axis first scaled by the
        // reciprocal of its narrowed root.
        if ((axis.x * axis.x + axis.y * axis.y) + axis.z * axis.z > 0.01f) {
            const float inverse = 1.f / (float)std::sqrt((double)((axis.x * axis.x + axis.y * axis.y) + axis.z * axis.z));
            base = R(base, (1 - m.dodgeMs / 600) * -360, axis * inverse);
        }
    }
    base = Rz(base, m.yawDegrees);
    base = T(base, {0, 0, s.pivot});
    base = T(base, {s.lean.x, s.lean.y, 0});
    base = Rzyx(base, s.tilt);
    base = T(base, {0, 0, -s.pivot});
    base = S(base, a.box.x / 11.2f);
    const float bob = 4 * std::fabs(swingKept) * (action == 79 ? 4.f : 1.f);
    // §7.9: the body frame and the arm frame.
    const glm::mat4 lifted = T(T(base, {0, 0, bob}), s.shift);
    const glm::mat4 body = Rx(Ry(Rz(lifted, s.torso.z), s.torso.y), a.bodyPitch + s.torso.x);
    glm::mat4 arm = lifted;
    // Head-pitch-and-pick §1.4: the arm matrix pitches by twice the head pitch, unless climbing
    // (flags bit 0x0001 with +0x5C bit 0x04) or the pose selector is 2, 3 or 0x25.
    if (!m.climbing && pose != 2 && pose != 3 && pose != 0x25)
        arm = Rx(arm, m.headPitch * 2.f);
    if (m.sitting)
        arm = Rx(arm, 60);
    arm = T(Ry(T(arm, {0, 0, -4}), s.roll.y), {0, 0, 4});
    arm = Rx(Ry(Rz(arm, s.torso.z), s.torso.y), s.torso.x);

    std::vector<PartDraw> out;
    // The spec's centring translations take the model's corner (voxel 0) as origin; meshes
    // here are centred in x and y with their base at z = 0, so (sx/2, sy/2, 0) is added.
    // `cubes` is the item whose attached cubes go on the model (item services §3.9): each sits
    // at its model cell, one voxel big.
    // `item` is the equipment the model shows: it shimmers by its material (materials §1.2), and
    // a weapon carries its attached cubes, each at its model cell, one voxel big (§3.9).
    auto part = [&](uint16_t model, glm::mat4 at, glm::vec3 scale, std::function<glm::vec3(glm::vec3)> corner,
                    bool mirrored = false, bool tint = false, const Item *item = nullptr) {
        if (model == NO_MODEL || model >= MODEL_SLOTS)
            return;
        const glm::vec3 size = voxels(model);
        at = glm::scale(at, scale);
        const bool shine = item && item->type != 0 &&
                           (item->material == 1 || item->material == 11 || item->material == 12 || item->material == 22);
        if (item && item->type == 3)
            for (const Item::Cube &k : item->cubes)
                out.push_back({model, T(at, corner(size) + glm::vec3(k.x + 0.5f, k.y + 0.5f, k.z)), false, false, true,
                               spiritColour(k.material), shine});
        at = T(at, corner(size) + glm::vec3(size.x / 2, size.y / 2, 0));
        out.push_back({model, at, mirrored, tint, false, glm::vec3(1), shine});
    };
    const auto centred = [](glm::vec3 sz) { return -0.5f * sz; };
    const float sDeg = (float)((double)swing * 180.0 / PI);
    const uint16_t chestModel = c && c->equipment[CHEST_SLOT].type == 4 && itemModel(c->equipment[CHEST_SLOT]) != NO_MODEL
                                    ? itemModel(c->equipment[CHEST_SLOT])
                                    : a.model(Part::Body);
    const uint16_t handModel = c && c->equipment[GLOVES_SLOT].type == 5 ? itemModel(c->equipment[GLOVES_SLOT])
                                                                        : a.model(Part::Hand);
    const uint16_t footModel = c && c->equipment[BOOTS_SLOT].type == 6 ? itemModel(c->equipment[BOOTS_SLOT])
                                                                       : a.model(Part::Foot);
    const float b = a.bodyScale;

    // §7.5 in drawing order; §6.5 for the equipment.
    part(chestModel, Rz(T(body, a.bodyOffset), 0.5f * s.aim + sDeg / 2), glm::vec3(b), centred, false, false,
         c && chestModel != a.model(Part::Body) ? &c->equipment[CHEST_SLOT] : nullptr);
    if (c && c->equipment[AMULET_SLOT].type == 8)
        part(itemModel(c->equipment[AMULET_SLOT]),
             Rx(Rz(T(body, a.bodyOffset + glm::vec3(0, 3, 6.05f)), sDeg / 2 + 0.5f * s.aim),
                0.7f * swing * swing * 180.f / PIf),
             glm::vec3(b), [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, -1.5f, -sz.z); });
    if (c && c->equipment[SHOULDERS_SLOT].type == 7)
        part(itemModel(c->equipment[SHOULDERS_SLOT]), Rz(T(T(body, {0, 0, 4 + 2 * amp}), a.bodyOffset), sDeg / 2),
             glm::vec3(a.shoulderScale * (b + 0.1f)), centred);
    if (!f.curled)
        part(a.model(Part::Tail),
             Rzyx(Rz(Rx(T(T(body, a.bodyOffset), a.tailOffset), a.tailAngle), -sDeg), s.tail),
             glm::vec3(a.tailScale * b), [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, -sz.y, -sz.z / 2); });
    glm::mat4 head = T(body, a.headOffset + s.headShift + glm::vec3(0, 0, -bob / 2));
    head = T(head, {0, 0, -3});
    head = Rx(head, -s.torso.x - a.bodyPitch);
    head = Ry(head, -s.torso.y);
    head = Rz(head, s.head.z - s.torso.z);
    head = Ry(head, s.head.y);
    // Head-pitch-and-pick §1.3 step 8: (pitch - drawn X rotation) + head angle; the engine's
    // drawn X rotation is zero (see Motion::headPitch).
    head = Rx(head, m.headPitch + s.head.x);
    head = T(head, {0, 0, 3});
    part(a.model(Part::Head), head, glm::vec3(a.headScale), centred);
    part(a.model(Part::Hair), head, glm::vec3(a.headScale), centred, false, true);
    if (!f.curled) {
        part(a.model(Part::Arm), T(arm, armR), glm::vec3(a.armScale), centred);
        part(a.model(Part::Arm), T(arm, armL), glm::vec3(a.armScale), centred);
    }
    // The weapons.
    const float wscale = a.weaponScale;
    auto mainGrip = [&](uint16_t model) -> std::function<glm::vec3(glm::vec3)> {
        if (model == 840)
            return [](glm::vec3 sz) { return glm::vec3(-sz.x / 2 - 1, 0.5f, 1); };
        switch (main) {
        case FIST: return [](glm::vec3 sz) { return -0.5f * sz; };
        case BOW: return [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 1.5f - sz.y, -sz.z / 2); };
        case CROSSBOW: return [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 0, -sz.z / 2); };
        case BOOMERANG: return [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, -2, -4); };
        case STAFF: return [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, -sz.y / 2, -10); };
        case BRACELET: return [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 1 - sz.y, -sz.z / 2); };
        default: return [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, -sz.y / 2, -4); };
        }
    };
    const float bracelet = main == BRACELET ? 0.6f : 1.f;
    // Trails §1.1: after a weapon is drawn, its butt (voxel z = 0) and tip (z = size) become
    // point 0 of that hand's histories.
    const auto newest = [&](size_t before, int history) {
        if (out.size() == before)
            return;
        const PartDraw &weapon = out.back();
        const float length = voxels(weapon.model).z;
        s.trail[(size_t)history][0] = glm::vec3(weapon.matrix * glm::vec4(0, 0, length, 1));
        s.trail[(size_t)history + 1][0] = glm::vec3(weapon.matrix * glm::vec4(0, 0, 0, 1));
    };
    // Pose contract §8: the carried swing turn, ((q × 0.25) × 180) ÷ pi with a binary64 divide.
    const float carriedTurn = (float)((double)((swing * 0.25f) * 180.f) / PI);
    const size_t beforeMain = out.size();
    if (f.mainModel != NO_MODEL) {
        // Carried (§8): -45 about y for a staff, -135 otherwise, then centred; no model exception.
        if (f.mainOnBack)
            part(f.mainModel,
                 R(R(T(Rz(body, carriedTurn), {0, -7.1f, 0.1f}), main == STAFF ? -45.f : -135.f, {0, 1, 0}), 90,
                   {0, 0, 1}),
                 glm::vec3(wscale), centred, false, false, c ? &c->mainHand() : nullptr);
        else if (main == BRACELET && f.mainModel != 840)
            // The hand scale, then the bracelet's own 0.6, as two scalings (§8).
            part(f.mainModel, S(R(Rzyx(T(arm, Rhand), rho), m.headPitch * 0.5f, {1, 0, 0}), wscale), glm::vec3(bracelet),
                 mainGrip(f.mainModel), false, false, c ? &c->mainHand() : nullptr);
        else // §1.5: the held branch adds half the head pitch about x
            part(f.mainModel, R(Rzyx(T(arm, Rhand), rho), m.headPitch * 0.5f, {1, 0, 0}), glm::vec3(wscale),
                 mainGrip(f.mainModel), false, false, c ? &c->mainHand() : nullptr);
    }
    newest(beforeMain, 0);
    const size_t beforeOff = out.size();
    if (f.offModel != NO_MODEL) {
        std::function<glm::vec3(glm::vec3)> grip;
        float extra = 1;
        if (f.offModel == 840)
            grip = [](glm::vec3 sz) { return glm::vec3(1 - sz.x / 2, 0.5f, 1); };
        else if (pose == 0x47)
            grip = centred;
        else if (off == BOW)
            grip = [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 1.5f - sz.y, -sz.z / 2); };
        else if (off == CROSSBOW)
            grip = [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 0, -sz.z / 2); };
        else if (off == BRACELET)
            grip = [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 1 - sz.y, -sz.z / 2); }, extra = 0.6f;
        else if (off == SHIELD)
            grip = [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 3, -sz.z / 2); };
        else if (main == BOOMERANG)
            grip = [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, -2, -4); };
        else if (off == FIST)
            grip = centred; // after the scale and R_y(180), below
        else
            grip = [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, -sz.y / 2, -4); };
        if (f.offOnBack) {
            // Carried (§9), with no model exception.
            glm::mat4 back = Rz(T(body, {0.5f, -0.65f, 0.5f}), carriedTurn);
            if (off == SHIELD)
                back = R(T(back, {0, -8.2f, 0}), 180, {0, 0, 1});
            else if (off == BOW || off == CROSSBOW)
                back = R(R(T(back, {0, -6.2f, 0}), -45, {0, 1, 0}), 90, {0, 0, 1});
            else
                back = R(R(T(back, {0, -7.2f, 0}), -225, {0, 1, 0}), 90, {0, 0, 1});
            part(f.offModel, back, glm::vec3(wscale), centred, false, false, c ? &c->offHand() : nullptr);
        } else {
            // Held (§9): selector 0x47 holds the item at the hands' midpoint, 8 higher.
            const glm::vec3 grasp = pose == 0x47 ? 0.5f * (Rhand + Lhand) + glm::vec3(0, 0, 8) : Lhand;
            // §1.5: the held branch adds half the head pitch about x (the hand turns are zero).
            glm::mat4 at = R(Rzyx(T(arm, grasp), lambda), m.headPitch * 0.5f, {1, 0, 0});
            const bool special = f.offModel == 840 || pose == 0x47;
            if (!special && off == FIST && main != BOOMERANG)
                part(f.offModel, Ry(S(at, wscale), 180), glm::vec3(1), grip, false, false, c ? &c->offHand() : nullptr);
            else if (!special && off == BRACELET)
                part(f.offModel, S(at, wscale), glm::vec3(extra), grip, false, false, c ? &c->offHand() : nullptr);
            else
                part(f.offModel, at, glm::vec3(wscale), grip, false, false, c ? &c->offHand() : nullptr);
        }
    }
    newest(beforeOff, 2);
    ageTrail(s, pose, t, P, dt, base, T(glm::mat4(1), {0, 0, s.lean.z}), m.yawDegrees);
    // Quivers on the back (§6.5).
    if (c && (off == QUIVER))
        part(839, Rz(Ry(T(Rz(T(body, {0.65f, -0.5f, 0.3f}), sDeg / 4), {0, -6.2f, 0}), -45), 90), glm::vec3(wscale),
             centred);
    if (off == BOW || off == CROSSBOW)
        part(839, Rz(Ry(T(Rz(body, sDeg / 4), {0, -7.1f, 0.1f}), 45), 90), glm::vec3(wscale + 0.01f), centred);
    if (main == BOW || main == CROSSBOW)
        part(839, Rz(Ry(T(Rz(body, sDeg / 4), {0, -7.1f, 0.1f}), -45), 90), glm::vec3(wscale + 0.01f), centred);
    // §7.9.4: the hands; the right (+x) hand is drawn mirrored.
    if (!f.curled) {
        auto hand = [&](glm::vec3 at, glm::vec3 rot, float leg) {
            glm::mat4 mtx = arm;
            if (quadruped)
                mtx = Rx(mtx, leg);
            mtx = Ry(Rz(T(mtx, at), rot.z), rot.y);
            if (quadruped)
                mtx = Rx(mtx, leg);
            return Rx(mtx, rot.x);
        };
        const Item *gloves = c && handModel != a.model(Part::Hand) ? &c->equipment[GLOVES_SLOT] : nullptr;
        part(handModel, hand(Lhand, lambda, f.legR), glm::vec3(a.handScale), centred, false, false, gloves);
        part(handModel, hand(Rhand, rho, f.legL), glm::vec3(a.handScale), centred, true, false, gloves);
    }
    // §6.5: between the hands, the item being drunk or eaten (actions 80, 81, within its use
    // limit), the pet item or food held out (action 82, standing on the hands' midpoint) and
    // the mana cube while it is used (pose 72, first 5 s).
    if (c) {
        const glm::vec3 between = 0.5f * (Lhand + Rhand);
        const glm::mat4 held = Rx(T(arm, between), lambda.x);
        const float limit = c->held.type == 1 ? (c->held.subtype == 1 ? 3000.f : 10000.f) : 0.f;
        if ((action == 80 || action == 81) && c->actionMs < limit)
            part(itemModel(c->held), held, glm::vec3(a.handScale), centred);
        if (action == 82 && c->equipment[12].type != 0)
            part(itemModel(c->equipment[12]), held, glm::vec3(a.handScale),
                 [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, -sz.y / 2, 0); });
        if (pose == 72 && c->actionMs < 5000)
            part(2566, Rx(Ry(Rz(T(arm, between + glm::vec3(0, 0, 8)), 0.02f * c->actionMs), 45), 45), glm::vec3(1),
                 centred);
    }
    // The lantern hangs at the left hand while lit.
    if (m.lantern && c && c->equipment[10].type == 24)
        part(2433, T(arm, Lhand + glm::vec3(0, 2, -2)), glm::vec3(0.8f),
             [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 1 - sz.y / 2, 2 - sz.z); });
    // §6.5: the glider above the body while it is open, the boat under it while sailing; both
    // stay level against the body's tilt.
    if (c && c->equipment[11].type == 23) {
        const float up = a.box.z / std::max(a.box.x, 0.01f);
        if (m.gliding && c->equipment[11].subtype != 1)
            part(2547, Rx(T(body, a.bodyOffset + glm::vec3(0, 0, 8 * up)), -s.tilt.x), glm::vec3(2 * b), centred);
        if (action == 107 && c->equipment[11].subtype == 1)
            part(2548, Rz(Rx(T(body, a.bodyOffset + glm::vec3(0, 0, -9 * up)), -s.tilt.x), -s.tilt.z), glm::vec3(2 * b),
                 [](glm::vec3 sz) { return glm::vec3(1 - sz.x / 2, 2 - sz.y / 2, 0); });
    }
    // §7.9.6: wings.
    if (a.model(Part::Wing) != NO_MODEL) {
        const float flap = std::cos(s.phase / 2) * amp * 90 - 45;
        for (float side : {-1.f, 1.f}) {
            glm::mat4 at = T(arm, {side * a.wingOffset.x, a.wingOffset.y, a.wingOffset.z});
            at = Rx(Rz(Ry(Rx(at, a.wingAngle), -side * flap), -90), 90);
            part(a.model(Part::Wing), at, glm::vec3(a.wingScale), [](glm::vec3 sz) { return glm::vec3(-sz.x / 2, 0, -sz.z / 2); });
        }
    }
    // §7.9.5: feet, from the base frame.
    if (!f.curled) {
        const bool lifts = !(flyer && !m.onGround);
        auto foot = [&](glm::vec3 target, float a1, float a2, float ell, float lift, bool turned) {
            glm::mat4 at = Ry(Rx(Rz(base, s.aim), a1), ell);
            at = T(at, target + glm::vec3(0, 0, 3 * std::fabs(swingKept) + (lifts ? lift : 0)));
            at = glm::scale(at, glm::vec3(a.footScale));
            at = turned ? Rz(at, 60) : Rx(at, a2);
            return at;
        };
        part(footModel, foot(f.footL, a.footAngle + f.F + f.legL, f.F + f.legL, f.ell, 4 * amp * s0, f.turn),
             glm::vec3(1), centred);
        part(footModel, foot(f.footR, f.F + f.legR, a.footAngle + f.F + f.legR, -f.ell, 4 * amp * (1 - s0), false),
             glm::vec3(1), centred);
    }
    return out;
}

} // namespace creatures
