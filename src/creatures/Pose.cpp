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
// Pose selector contracts §1: the binary32 sine and cosine wrappers (argument widened, result
// narrowed), the ordered clamps (a NaN passes) and the 32-bit wrapping clock arithmetic.
float sinw(float x) { return (float)std::sin((double)x); }
float cosw(float x) { return (float)std::cos((double)x); }
float clampHigh(float v) { return v > 1.f ? 1.f : v; }
float clampUnit(float v) { return v > 1.f ? 1.f : 0.f > v ? 0.f : v; }
int32_t add32(int32_t a, int32_t b) { return (int32_t)((uint32_t)a + (uint32_t)b); }
int32_t sub32(int32_t a, int32_t b) { return (int32_t)((uint32_t)a - (uint32_t)b); }

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
    float offScale = 1; // the off-hand weapon scale local (weapon pose contract §3)
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
    int32_t clock = c ? c->actionMs : 0; // the pose clock C +0x4 (owner +0x6C)
    if (c && (float)clock >= (float)duration(*c, action) + 50)
        action = 0, clock = 0;
    float t = (float)clock;
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
    // Combat helper tables: B (wind-up), Cq (active), Ar (recovery) and R0, their wrapping sum.
    const int32_t B = P.windUp, Cq = P.active, Ar = P.recovery, R0 = add32(add32(B, Cq), Ar);
    // Pose selectors 31-49 §8 (0x0C6B80): the use limit of the item being eaten or drunk.
    const int32_t heldLimit = c && c->held.type == 1 ? (c->held.subtype == 1 ? 3000 : 10000) : 0;
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
    if ((pose == 0x33 || pose == 0x34) && clock < heldLimit)
        swing *= 0.1f;
    if (pose == 0x35)
        swing *= 0.1f;
    if (action == 22 || action == 36 || action == 63 || action == 8 || action == 28 || pose == 2 || pose == 3 ||
        pose == 48 || pose == 49 || pose == 37)
        swing *= 0.5f;
    if (m.gliding)
        swing = 0;

    Frame f;
    f.offScale = a.weaponScale;
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
    // Step 3: base targets. Weapon pose contract §3: the rest hand positions.
    const glm::vec3 h = a.handOffset;
    const float amp = s.amplitude;
    if (quadruped) {
        f.Rh = h;
        f.L = {-h.x, h.y, h.z};
    } else {
        const float reach = h.x * amp + h.x;
        const float cq = cosw(swing), sq = sinw(swing);
        const float z = (h.z - 4.1f) + amp * 6;
        f.Rh = {cq * reach, h.y + sq * reach, z};
        f.L = {-(cq * reach), h.y + -(sq * reach), z};
        f.Rh.y = (2.1f - amp * 5) + f.Rh.y;
        f.L.y = (2.1f - amp * 5) + f.L.y;
    }
    // Pose selectors 01-18 §1.2: the auxiliary mid-points OM (armL) and MM (armR).
    f.armL = {(h.x * -0.3f + f.L.x) * 0.5f, ((f.L.y - 8) * 0.5f) * 0.5f, (f.L.z + (h.z + 12)) * 0.5f};
    f.armR = {(f.Rh.x + h.x * 0.3f) * 0.5f, ((f.Rh.y - 8) * 0.5f) * 0.5f, (f.Rh.z + (h.z + 12)) * 0.5f};
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
        if (action != 0 && clock < R0)
            f.dL += glm::vec3(0, -5, 6), f.dR += glm::vec3(0, -5, 6);
    }

    // The per-selector pose evaluator. Storage of the selector contracts (alpha-pose-selectors-*.md,
    // weapon-attachment-poses §3-§5) as the frame fields hold it:
    //   rR, rL       main-hand / off-hand angle targets (MA, OA; L134, L164)
    //   dR, dL       main-hand / off-hand offset targets (MO, OO; L14C, L170)
    //   Rh, L        main-hand / off-hand rest hand positions (MP, OP; L17C, L158)
    //   armR, armL   main / off auxiliary mid-points (MM, OM; L23C, L188)
    //   T  body-angle target (BA, L140)          B  creature-angle target (CA, L194)
    //   H  head-angle target (HA, L1A0)          Q  tail-angle target (L200)
    //   E  creature-offset target (CO, L1AC)     S  body-offset target (BO, L1F4)
    //   K  C +0x78 target (L1B8)                 U  hand-turn target (HT, L1C4)
    //   h  lift target (L60)                     F  auxiliary scalar (L6C)
    //   footR, footL  auxiliary points FA / FB (owner +0x100, X-flipped copy; L1D0, L1DC)
    //   turn  flag byte (L27)      rate  blend-rate slot (L10)      offScale  off-hand weapon scale
    //   mainModel, offModel  C +0xDC / C +0xE0      cleared  selector C +0x8 := 0
    // B, Cq, Ar and R0 are the wind-up, active, recovery and total helpers; P is twoHanded(main).
    // Owner +0x148 / +0x154 (hand-turn sources) and +0x1314 (spin variants) are not kept by the
    // engine: those hand turns stay zero and the spin variants are never taken (open).
    bool cleared = false;
    const int mainSub = c ? c->mainHand().subtype : -1, offSub = c ? c->offHand().subtype : -1;
    const auto upperArm = [&] { f.armL.y += 10; f.armL.z -= 4; };
    const auto offFromMain = [&](float dx) { f.L = f.Rh; f.L.x += dx; };
    // 01-18 §3 block G.
    const auto blockG = [&] {
        offFromMain(2);
        upperArm();
        f.rate = 0.005f;
        f.rL = f.rR;
    };
    // 01-18 §3 block R: the two-hand grip.
    const auto twoHandGrip = [&](bool copyAngles) {
        f.Rh.y += 2;
        f.Rh.z += 3;
        f.L = f.Rh;
        f.L.x -= 3;
        f.L.z -= 3;
        upperArm();
        f.rR.x = 0;
        f.rR.y = 20;
        if (copyAngles)
            f.rL = f.rR;
    };
    // 01-18 §3 block T (19-24 §3.5 block S).
    const auto blockT = [&](glm::vec3 offAngles) {
        f.rL = offAngles;
        f.dL.y -= 4;
        f.dL.z += 2;
        if (f.mainModel != NO_MODEL && f.offModel == NO_MODEL)
            twoHandGrip(true);
    };
    // 01-18 §3 block Q: the late branch; false when the type gate stops it.
    const auto late = [&](float offX, bool typeGate) {
        f.U = glm::vec3(0);
        cleared = true;
        if (typeGate && (c->type == 0x60 || c->type == 0x57))
            return false;
        f.rL.x = offX;
        f.T.z = 20;
        f.rR = {-120, -90, 30};
        return true;
    };
    // Selectors 5, 6, 0x43: the rest hands bob.
    const auto handBob = [&] {
        f.Rh.z = cosw(t * 0.5f) * 0.5f + f.Rh.z;
        f.L.z = cosw(t * 0.5f) * 0.5f + f.L.z;
    };
    // 19-24 §3.1 oscillation O (01-18 selectors 0x17, 0x18).
    const auto sway = [&] {
        f.dR.y += 4;
        f.dL.y += 4;
        f.dR.x = sinw(t * 0.04f) * 0.5f + f.dR.x;
        f.dR.z = cosw(t * 0.05f) * 0.5f + f.dR.z;
        f.dL.x = sinw(t * 0.04f) * 0.5f + f.dL.x;
        f.dL.z = cosw(t * 0.05f) * 0.5f + f.dL.z;
    };
    // 19-24 §3.2 block F.
    const auto armsForward = [&] {
        f.T.z = -30;
        f.rR = {0, -45, -60};
        f.rL = {-45, -45, 0};
        sway();
    };
    // 01-18 selectors 0x13, 0x14 and 31-49 selector 0x32, late paths: the guard idle.
    const auto guard = [&](float bodyRate) {
        f.T.z = 0;
        f.B.z = 0;
        f.rR = {60, -20, 20};
        const float p = t * 0.01f;
        f.dR = {0, sinw(p) + 2, cosw(p) + 4};
        f.rL = {60, 20, -20};
        const float p2 = (float)((double)(t * 0.01f) + PI / 2);
        f.dL = {0, sinw(p2) + 2, cosw(p2) + 4};
        f.S.z = sinw(t * bodyRate) * 0.5f + f.S.z;
    };
    // 31-49 §5 orbit offsets O.
    const auto orbit = [&] {
        f.T.x = 20;
        f.H.x = 20;
        const float p = t * 0.005f;
        f.dL = {cosw(p) * 3 - 1, 5, sinw(p) * 5 + 5};
        const float p2 = (float)((double)(t * 0.005f) + PI);
        f.dR = {cosw(p2) * 3 + 1, 5, sinw(p2) * 5 + 5};
    };
    // 19-24 §3.3 mirror tail M, and §3.4 rest block E.
    const auto mirror = [&] {
        f.dL = f.dR;
        f.rL = f.rR;
        f.L = f.Rh;
        f.L.x -= 0.2f;
        f.L.y -= 0.2f;
        f.L.z -= 1;
    };
    const auto restE = [&] {
        f.dR = {2, 0, 0};
        f.rR = glm::vec3(0);
        f.T.z = -30;
        f.rate = 0.03f;
    };
    const auto mix = [](glm::vec3 to, float p, glm::vec3 from, float q) { return to * p + from * q; };
    // 31-49 §5 constant set K.
    const auto setK = [&] {
        f.E.z = -0.14f;
        f.T.x = 10;
        f.dL = glm::vec3(0, 4, 0);
        f.dR = glm::vec3(0, 4, 0);
        f.K = {0, 2, -2};
    };

    switch (pose) {
    case 0x01: { // 01-18 §4
        const bool two = twoHanded(main);
        if (t < 500) {
            const float r = clampHigh(t / 500);
            if (two) {
                f.rR.x = r * 120;
                f.dR.y = 0, f.dR.z = 14, f.dR.x = -f.Rh.x;
                f.dL.y = 0, f.dL.z = 12, f.dL.x = 2 - f.dL.x;
            } else {
                f.dR = {1, 0, 14};
                f.dL = {-1, 0, 14};
                f.rL.x = f.rR.x = -100;
            }
            f.T.x = r * -40;
            f.B.x = r * -40;
            if (action == 0x30) {
                const float spin = (t / 500) * 360;
                f.B.x = r * -40 - (spin > 360 ? 360.f : spin);
            }
            s.amplitude = 0; // owner +0x1188 := +0.0, seen by later refreshes
        } else if (clock < R0) {
            const float r = clampHigh((float)sub32(clock, 500) / 500 * 2);
            f.B.x = (1 - r) * -60;
            f.T.x = -20;
            f.rR.x = -70;
            if (two) {
                f.dR.y = 4, f.dR.z = 0, f.dR.x = -f.Rh.x;
                f.dL.y = 4, f.dL.z = 0, f.dL.x = 2 - f.dL.x;
            } else {
                f.dR = {1, 3, 8};
                f.dL = {-1, 3, 8};
                f.rL.x = f.rR.x = -150;
            }
            f.rate = 0.1f;
        } else if (two) {
            f.Rh.y += 4;
            f.Rh.x -= 8;
            offFromMain(-2);
            f.L.z -= 1;
            upperArm();
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
    case 0x02: // 01-18 §4
        if (clock < B) {
            const float p = clampHigh(t / (float)B * 1.5f), q = 1 - p;
            f.B.x = p * 10 + 5;
            f.T = {10, 10, -60};
            f.rR = {q * 90 + p * 60, q * 0.f + p * 0.f, q * 90 + p * 60};
            f.dR = {0, 0, p * 14};
            f.dL = f.dR;
            offFromMain(2);
            upperArm();
            f.rate = 0.01f;
            f.rL = f.rR;
        } else if (clock < R0) {
            const float p = clampHigh((float)sub32(clock, B) / (float)Cq);
            f.rate = 0.04f;
            f.B.x = -10;
            f.T.y = 10;
            f.T.z = 60;
            f.rR = {-70, 0, -60};
            const float k = (float)std::sin((double)(float)(((double)p * PI) * 0.5)) * 2;
            f.dR = {0, k, 0};
            f.dL = f.dR;
            offFromMain(2);
            f.L.y -= 1;
            f.rL = f.rR;
        } else {
            cleared = true;
            twoHandGrip(true);
        }
        break;
    case 0x03: // 01-18 §4
        if (clock < B) {
            f.B.z = 0;
            f.rR = {-90, 0, 0};
            f.dR = {-5, 8, 0};
            f.dL = f.dR;
            blockG();
        } else if (clock < add32(B, Cq / 2)) {
            f.B = {10, 0, -10};
            f.T = {30, 0, -30};
            f.rR = {0, 0, 90};
            f.dR = {-8, -4, 20};
            f.dL = f.dR;
            blockG();
        } else if (clock < R0) {
            f.B.z = -30;
            f.T = {0, 20, -60};
            f.rR = {70, 0, 90};
            f.dR = {5, -2, 0};
            f.dL = f.dR;
            offFromMain(2);
            upperArm();
            f.rate = 0.02f;
            f.rL = f.rR;
        } else {
            cleared = true;
            offFromMain(-2);
            f.L.z -= 1;
            upperArm();
            f.rR.y = 90;
            f.T.z = -80;
            f.rL = f.rR;
        }
        break;
    case 0x04: { // 01-18 §4
        const int32_t halfB = B / 2;
        if (clock < halfB) {
            const float p = clampHigh(t / (float)halfB), q = 1 - p;
            f.B.x = p * 5;
            f.T.y = p * 10;
            f.T.z = p * -20;
            f.rR = {q * 0.f + p * 60, q * 120 + p * 60, q * 0.f + p * 60};
            blockG();
            break;
        }
        if (c->characterClass == 1 && c->specialization == 1) {
            f.dR = {2, 7, 4};
            f.rR = {0, -60, 0};
            f.rate = 0.01f;
        } else {
            const float p = clampHigh((float)sub32(clock, halfB) / (float)halfB * 1.5f), q = 1 - p;
            f.B.x = 0;
            f.T = {20, 0, 0};
            f.rR = {q * 90 + p * 30, q * 0.f + p * 0.f, q * 90 + p * 0.f};
            f.Rh.z = cosw(t * 0.5f) * 0.5f + f.Rh.z;
            f.dR = {-6, 0, p * 16};
            f.rate = 0.005f;
        }
        f.dL = f.dR;
        offFromMain(2);
        upperArm();
        f.rL = f.rR;
        break;
    }
    case 0x05: // 01-18 §5
        f.rR = {-90, 0, -40};
        f.rL = {-90, 0, 40};
        handBob();
        f.dR = {1, -7, 0};
        f.dL = {-1, -7, 0};
        f.T.z = -40;
        f.rate = 0.005f;
        break;
    case 0x06: // 01-18 §5
        if (c->specialization == 1) {
            f.rR = {-50, -60, 40};
            f.rL = {-90, 90, -50};
            handBob();
            f.dR = {-3, 7, 3};
            f.dL = {-2, 2, 0};
            f.T.z = -20;
        } else {
            f.rR = {-90, 0, -40};
            f.rL = {-90, 90, -40};
            handBob();
            f.dR = {1, -7, 0};
            f.dL = {-1, 0, 1};
            f.T.z = -40;
        }
        f.rate = 0.005f;
        break;
    case 0x07: // 01-18 §5; its hand-turn target blends owner +0x148 and +0x154 (open: not kept)
        if (clock < R0 / 2)
            f.rate = 0.005f;
        else if (clock < R0)
            f.rate = 0.03f;
        f.rR = {0, 90, 0};
        if (twoHanded(main)) {
            offFromMain(2);
            upperArm();
        }
        break;
    case 0x08: // 01-18 §5: the clock against 100 and 500
        if (f.offModel != NO_MODEL) {
            if (clock < 100)
                f.T.z = 80, f.rL = {0, 0, 90}, f.rR = {-45, 45, 0};
            else if (clock < 500)
                f.T.z = -90, f.rL = {0, 60, 90}, f.dL.z += 5, f.rR = {-90, 0, 45};
            else
                f.T.z = -30, f.rL = {0, 100, 60}, f.rR = {-45, 45, 0};
            break;
        }
        if (clock < 100) {
            f.T.z = -80;
            f.rR = {0, 0, 90};
        } else if (clock < 500) {
            f.T.z = 90;
            f.rR = {0, 60, -90};
            f.rR.z += 5;
            f.dL.x += 2, f.dR.x += 2, f.dL.y += 3, f.dR.y += 3, f.dL.z += 3, f.dR.z += 3;
        } else {
            twoHandGrip(false);
        }
        offFromMain(-3);
        f.L.z -= 3;
        f.rL = f.rR;
        break;
    case 0x09: // 01-18 §5
    case 0x0A:
        if (clock < B) {
            f.T.z = 80;
            f.rL = {0, 0, 90};
            f.rR = {-45, 45, 0};
            if (action == 0x30)
                f.B.z = (float)sub32(0, clock) / 300 * 360;
        } else if (pose == 0x09 || clock < R0) {
            f.T.z = -90;
            f.rL = {0, 0, 90};
            f.rR = {-90, 0, 45};
            if (pose == 0x0A) {
                f.L.x = sinw((float)sub32(clock, 600) * 0.1f) + f.L.x;
                f.rate = 0.03f;
            }
        } else if (late(-90, true)) {
            blockT(offSub == SHIELD ? glm::vec3(0, 0, 90) : glm::vec3(-80, 40, 60));
        }
        break;
    case 0x0B: // 01-18 §5
        if (!(a.flags & 0x0004)) {
            f.T.z = 60;
            f.rR = {0, -45, -60};
            f.rL = {-45, -45, 0};
        }
        break;
    case 0x0F: // 01-18 §6 (the spin variant, remainder 2, is open; its CA.Z is then 0)
        f.B.z = 0;
        if (clock < B) {
            f.T.z = 40;
            f.rL = {0, -90, 30};
            f.dL.y -= 4;
            f.dL.z += 4;
            f.rR = {0, -90, -120};
            f.dR.y -= 6;
        } else if (clock < R0) {
            f.T.z = -90;
            f.E.x = -0.1f;
            f.rL = {-90, -90, -30};
            f.rR = {0, -90, -120};
            f.dR.y -= 6;
        } else if (late(-90, true)) {
            blockT({-80, 40, 60});
        }
        if (offSub == SHIELD)
            f.rL = {0, 0, 90};
        break;
    case 0x10: // 01-18 §6 (the spin variant, remainder 1, is open; its CA.Z is then 0)
        f.B.z = 0;
        if (clock < B) {
            f.T.z = 90;
            f.rR = {0, -90, 0};
            f.rL = {-90, 0, 90};
            f.dR.y += 6;
            f.dR.z += 4;
        } else if (clock < R0) {
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
        } else if (late(-60, true)) {
            blockT({-80, 40, 60});
        }
        if (offSub == SHIELD)
            f.rL = {0, 0, 90};
        break;
    case 0x11: // 01-18 §6
        if (clock < B) {
            f.T.z = 40;
            f.rR = {-90, 90, 30};
            f.rL = {-90, 0, -20};
            f.dL.y -= 6;
        } else if (clock < R0) {
            f.T.z = -90;
            f.rR = {-40, 0, 30};
            f.dR = {3, 0, 4};
            f.rL = {-90, 90, 90};
            f.dL = {-3, -1, 0};
            f.turn = true;
            f.rate = 0.05f;
        } else if (late(-90, true)) {
            blockT({-80, 40, 60});
        }
        break;
    case 0x12: // 01-18 §6
        if (clock < B) {
            f.T.z = -40;
            f.rL = {-90, -90, -30};
            f.rR = {-90, 0, 20};
            f.dR.y -= 6;
        } else if (clock < R0) {
            f.T.z = 90;
            f.rL = offSub == SHIELD ? glm::vec3(0, 0, 40) : glm::vec3(-40, 0, -30);
            f.dL = {-3, 0, 4};
            f.rR = {-90, -90, -90};
            f.dR = {3, -1, 0};
            f.turn = true;
            f.rate = 0.05f;
        } else if (late(-90, true)) {
            blockT(offSub == SHIELD ? glm::vec3(0, 0, 90) : glm::vec3(-80, 40, 60));
        }
        break;
    case 0x13: // 01-18 §6
    case 0x14:
        if (clock > B && clock < R0) {
            if (pose == 0x13) {
                f.T.z = -20;
                f.B.z = -20;
                f.rL = {0, 90, 40};
                f.dL = {0, 8, 4};
                f.rR = {0, 90, 30};
                f.dR = {1, -4, -1};
            } else {
                f.T.z = 45;
                f.B.z = 45;
                f.rR = {0, -90, -90};
                f.dR = {8, 0, 4};
                f.rL = {0, -90, -30};
                f.dL = {-1, -4, -1};
            }
            f.H.x = -10;
            f.S.y = 3;
        } else {
            guard(pose == 0x13 ? 0.005f : 0.01f);
        }
        f.rate = 0.03f;
        break;
    case 0x15: // 01-18 §7: path M works the off hand, path N (a bow or crossbow) the main hand
        if (mainSub != BOW && mainSub != CROSSBOW) {
            if (offSub == CROSSBOW)
                f.rL.y = -90;
            if (clock < add32(R0, R0)) {
                if (offSub == BOW)
                    f.dL.y += 5, f.dL.x -= 7;
                else
                    f.dL.y += 2, f.dL.x -= 3;
                f.dL.z += 4;
                f.dR.y += 3, f.dR.x -= 10, f.dR.z += 3;
                f.T.z -= 70;
                f.rL.z = 70;
                f.rR.z = 70;
                if (clock < R0 / 2) {
                    f.dL.z -= 2;
                    f.rL.x = -5;
                    f.dR = {-5, 1, 0};
                    f.mainModel = NO_MODEL;
                    f.T.z = -50;
                    f.rL.z = 70;
                    break;
                }
            } else {
                f.dL.x -= 1;
                f.rL.x = -30;
                f.rR.x = -30;
                f.rR.y = 45;
                if (offSub == CROSSBOW)
                    f.rL.z = 50;
            }
            if (c->offHand().type == 3)
                f.mainModel = 840;
        } else {
            if (mainSub == CROSSBOW)
                f.rR.y = -90;
            if (clock < add32(R0, R0)) {
                if (mainSub == BOW)
                    f.dR.y += 5, f.dR.x += 7;
                else
                    f.dR.y += 2, f.dR.x += 3;
                f.dR.z += 4;
                f.dL.y += 3, f.dL.x += 10, f.dL.z += 3;
                f.T.z = 70;
                f.rR.z = -70;
                f.rL.z = -70;
                if (clock < R0 / 2) {
                    f.dR.z -= 2;
                    f.rR.x = -5;
                    f.dL = {5, 1, 0};
                    f.offModel = NO_MODEL;
                    f.T.z = 50;
                    f.rR.z = -70;
                    break;
                }
            } else {
                f.dR.x += 1;
                f.rR.x = -30;
                f.rL.x = -30;
                f.rL.y = -45;
            }
            if (c->mainHand().type == 3)
                f.offModel = 840;
        }
        break;
    case 0x16: // 01-18 §7
        if (mainSub != BOW && mainSub != CROSSBOW) {
            f.dL.y += 3, f.dL.x -= 4, f.dL.z += 2;
            f.dR.y += 2, f.dR.x -= 10, f.dR.z += 1;
            f.T.z = -60;
            f.rL.z = 60;
            f.rL.y = -90;
            f.rR.z = 60;
            if (c->offHand().type == 3)
                f.mainModel = 840;
            if (clock < 300)
                f.dL.z -= 1, f.rL.x += 20, f.rL.y -= 20;
        } else {
            f.dR.y += 3, f.dR.x += 4, f.dR.z += 2;
            f.dL.y += 2, f.dL.x += 10, f.dL.z += 1;
            f.T.z = 60;
            f.rR.z = -60;
            f.rR.y = -90;
            f.rL.z = -60;
            if (c->mainHand().type == 3)
                f.offModel = 840;
            if (clock < 300)
                f.dR.z -= 1, f.rR.x += 20, f.rR.y += 20;
        }
        break;
    case 0x17: // 01-18 §7
        if (clock < B) {
            armsForward();
        } else {
            f.T.z = 60;
            f.rR = {0, -45, -60};
            f.rL = {-45, -45, 0};
        }
        break;
    case 0x18: // 01-18 §7
        if (clock < B) {
            sway();
            f.T.z = 20;
        } else if (clock < R0) {
            f.E.x = 0.5f;
            f.T.z = -60;
            f.B.z = -30;
            f.rL = {0, 45, 60};
            f.rR = {-45, 45, 0};
            f.dL = {-2, 0, 0};
            f.turn = true;
        }
        break;
    case 0x19: // 19-24 §5.1
        if (clock < B) {
            sway();
            f.T.z = -20;
        } else if (clock < R0) {
            f.E.x = -0.5f;
            f.T.z = 60;
            f.B.z = 30;
            f.rR = {0, -45, -60};
            f.rL = {-45, -45, 0};
            f.dR = {2, 0, 0};
            f.turn = true;
        }
        break;
    case 0x1A: // 19-24 §5.2
        if (clock < B) {
            armsForward();
        } else if (clock < R0) {
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
    case 0x1B: // 19-24 §5.3
        armsForward();
        break;
    case 0x1C: // 19-24 §5.4: no clamp on the turn
        if (clock < B) {
            f.T.z = -80;
            f.B.z = 0;
            f.rR = {-90, 90, -90};
            f.rL = {20, 90, 90};
        } else if (clock < add32(B, Cq)) {
            f.T.z = -90;
            f.B.z = (float)sub32(clock, B) * 360 / (float)Cq;
            f.rL = {0, 100, 90};
            f.rR = {0, 90, 0};
            f.rate = 0.2f;
        } else {
            f.T.z = 80;
            f.B.z = 0;
            f.rL = {90, 90, 270};
            f.rR = {0, 90, 90};
        }
        break;
    case 0x1D: // 19-24 §5.5
        if (clock < B) {
            f.rR = {-80, -90, 60};
            f.rL = {-80, 90, -60};
            f.dR.y += 3;
            f.dR.x -= 5;
            f.dL.x += 5;
        } else if (clock < R0) {
            f.T = {0, 0, -30};
            f.dR.y -= 5;
            f.dL.y -= 5;
            f.rL = {-90, 90, 110};
            f.rR = {-90, -90, -110};
            f.rate = 0.04f;
            f.E.y = -0.5f;
        } else {
            late(-90, false);
            blockT({-80, 40, 60});
        }
        break;
    // 19-24 §5.6-§5.9. Their p × 10 (stack +0x30) has no traced reader (open); the engine keeps
    // it as the foot spread. The spin terms on owner +0x1314 are open (never taken).
    case 0x1E:
        if (clock < B) {
            const float r = t / (float)B;
            f.rR = {-80, 90, -90};
            f.dR = {-10, 8, 8};
            f.T.z = -45;
            f.B.z = r * -20;
            f.E.x = r * 0.f;
            f.rate = 0.04f;
        } else if (clock < add32(add32(B, Cq), Ar)) {
            const float p = clampHigh((float)sub32(clock, B) * 1.2f / (float)Cq), q = 1 - p;
            f.E.x = q * 0.f - p * 0.8f;
            f.rR = mix({-110, 60, 0}, p, {-90, 90, 48}, q);
            f.dR = mix({-12, 0, -1}, p, {-10, 8, 8}, q);
            f.H.x = -10;
            f.T.z = p * 90 - q * 45;
            f.B.z = p * 30;
            f.rate = 0.05f;
            f.ell = p * 10;
        } else {
            restE();
        }
        mirror();
        break;
    case 0x1F:
        if (clock < B) {
            f.rR = {0, -90, 0};
            f.dR = {-8, 6, 6};
            f.H.x = -10;
            f.H.z = 30;
            f.T.z = 45;
            f.rate = 0.05f;
            f.E.x = -0.f;
            f.B.z = 20;
        } else if (clock < R0) {
            const float p = clampHigh((float)sub32(clock, B) * 1.2f / (float)Cq), q = 1 - p;
            f.E.x = p * 0.8f + q * -0.f;
            f.H.z = -30;
            f.rR = mix({0, -90, -130}, p, {0, -90, 20}, q);
            f.dR = mix({0, 2, 0}, p, {-8, 6, 6}, q);
            f.T.z = q * 45 - p * 45;
            f.B.z = p * -30;
            f.B.x = p * -20;
            f.rate = 0.05f;
            f.ell = p * 10;
        } else {
            restE();
        }
        mirror();
        break;
    case 0x20:
        if (clock < B) {
            const float r = t / (float)B;
            f.dR = {2, 2, 14};
            f.rR = {50, 0, 0};
            f.T.z = -45;
            f.B.z = r * -20;
            f.E.x = r * 0.5f;
            f.rate = 0.04f;
        } else if (clock < add32(add32(B, Cq), Ar)) {
            const float p = clampHigh((float)sub32(clock, B) * 2 / (float)Cq), q = 1 - p;
            f.B.x = q * 20 - p * 10;
            f.rR = mix({-90, 0, -45}, p, {90, 0, 45}, q);
            f.dR = mix({0, 9, 0}, p, {2, 2, 14}, q);
            f.T.z = p * 45 - q * 45;
            f.rate = 0.05f;
        } else {
            restE();
        }
        mirror();
        break;
    case 0x21:
        if (clock < B) {
            f.dR = {4, 2, 8};
            f.rR = {-90, -180, 90};
            f.T.z = -90;
            f.E.y = -1;
            f.rate = 0.02f;
        } else if (clock < add32(B, Cq)) {
            f.dR = {6, 0, 0};
            f.E.y = 0.5f;
            f.T.z = 90;
            f.rR = {-90, 0, -90};
            f.turn = true;
            f.rate = 0.05f;
        } else {
            restE();
        }
        mirror();
        break;
    case 0x22: // 19-24 §5.10
        f.rR = {-90, -90, 80};
        f.T.z = -80;
        break;
    case 0x23: // 19-24 §5.11
        if (clock < B) {
            f.rR = {-90, 0, 0};
            f.rL = {-90, 0, 40};
            f.dR = {1, -7, 0};
            f.dL = {-1, -7, 0};
            f.T.z = -40;
            f.E.y = -0.5f;
            f.rate = 0.005f;
            f.turn = true;
        } else if (clock < R0) {
            f.rR = {-90, -60, -30};
            f.rL = {-90, 60, -50};
            f.dR = {3, 5, 2};
            f.dL = {6, 8, 2};
            f.T.z = 40;
            f.rate = 0.05f;
            f.turn = true;
        } else if (late(-60, true)) {
            blockT({-80, 40, 60});
        }
        break;
    case 0x25: { // 25-30 §5.1: the mid-point gets its common +10 / -4 on top of phase 3's own
        float armY = f.armL.y, armZ = f.armL.z;
        if (clock < B) {
            f.rR = {-90, -90, 80};
            f.T.z = -80;
        } else if (clock < R0) {
            f.rR = {-90, 0, -90};
            f.Rh.x += 4;
            f.T.z = 90;
            f.turn = true;
            f.footL.y -= 4;
            f.footR.y += 4;
            f.rate = 0.05f;
        } else {
            cleared = true;
            f.Rh.y += 4;
            f.Rh.x -= 8;
            f.rR.x = -60;
            f.rR.z = 20;
            f.T.z = -20;
            armY += 10;
            armZ -= 4;
        }
        offFromMain(2);
        f.armL.y = armY + 10;
        f.armL.z = armZ - 4;
        f.rL = f.rR;
        break;
    }
    case 0x26: // 25-30 §5.2
        if (clock < add32(B, Cq / 2))
            f.H = {-30, 10, 90}, f.T = {0, 0, 60}, f.K = {-3, -2, 0};
        else if (clock < R0)
            f.H = {30, -10, -30}, f.T = {10, 0, -20}, f.K = {3, 0, 2};
        break;
    case 0x27: // 25-30 §5.3
        if (clock < add32(B, Cq / 2))
            f.H = {-70, 0, 0}, f.T = {-10, 0, 0}, f.K = {0, -3, -1}, f.rL.x = 20, f.rR.x = 20;
        else if (clock < R0)
            f.H = {50, 0, 0}, f.T = {20, 0, 0}, f.S = {0, -3, 1}, f.K = {0, 0, 2}, f.rR.x = -20, f.rL.x = -20,
            f.Q = {-10, 0, 0};
        break;
    case 0x28: // 25-30 §5.4
        if (clock < B)
            f.B.z = 90, f.rate = 0.002f, f.H = {0, 10, -60}, f.Q = {0, 0, -70};
        else if (clock < R0)
            f.B.z = 200, f.H = {0, -20, 60}, f.K = {-3, 0, -2}, f.Q = {0, 0, 70};
        break;
    case 0x29: { // 25-30 §6.1: the rest hands and feet curl toward the body offset P and back
        // (The head and tail offset locals curl too; their consumers are not traced, so the engine
        // keeps those parts in place: open.)
        const auto curl = [&](float k) {
            for (glm::vec3 *v : {&f.L, &f.Rh, &f.footL, &f.footR})
                *v = (a.bodyOffset - *v) * k + *v;
        };
        if (clock < B) {
            curl(t / (float)B);
        } else if (clock < add32(B, Cq)) {
            f.curled = true; // C +0xBC, +0xCC, +0xD4, +0xD8 := 0, read here as hands, feet and tail hidden
            f.S = {0, 0, -3};
        } else if (clock < R0) {
            curl(1 - (float)sub32(sub32(clock, B), Cq) / (float)Ar);
        }
        break;
    }
    case 0x2A: // 25-30 §6.2
        if (clock < add32(B, Cq / 2)) {
            f.H = {-10, 10, 30};
            f.T = {-10, 0, 0};
            f.K = {-3, 0, -1};
            f.rL.x = 20;
            f.rR.x = 20;
            f.rate = 0.005f;
        } else if (clock < R0) {
            f.H = {50, 30, 0};
            if (clock < add32(B, Cq))
                f.H.y = -30;
            f.T = {30, 0, 0};
            f.S = {0, -1, 2};
            f.K = {0, 0, 2};
            f.rR.x = -60;
            f.rate = 0.005f;
            f.rL.x = -20;
            f.Q = {-20, 0, 0};
        }
        break;
    case 0x2B: // 25-30 §6.3
        if (clock < add32(B, Cq / 2))
            f.H = {-20, 0, 0}, f.T = {50, 0, 0}, f.S = {0, -2, 2}, f.K = {0, -6, -2}, f.rL.x = 20, f.rR.x = 30,
            f.Q = {-50, 0, 0};
        else if (clock < R0)
            f.H = {-10, 0, 0}, f.T = glm::vec3(0), f.K = {0, 0, 1}, f.rL.x = 0, f.rR.x = 0, f.rate = 0.02f;
        break;
    case 0x2C: // 25-30 §6.4, 31-49 §6 0x49: the wind-up, three thirds of the strike, the return
    case 0x49: {
        const auto ends = [&](glm::vec3 head) {
            f.H = head, f.T = {-20, 0, 0}, f.S = {0, -1, 0}, f.K = {0, 1, 1}, f.rL.x = 20, f.rR.x = 20;
            f.dL.z = 2, f.dR.z = 2, f.Q = {30, 0, 10};
        };
        if (clock < B)
            ends({10, 0, 0});
        else if (clock < add32(B, Cq / 3))
            f.H = {-30, 0, 0}, f.T = {50, 0, 0}, f.S = {0, 0, 1}, f.K = {0, -4, 2}, f.rL.x = 60, f.rR.x = 70,
            f.Q = {-30, 0, 0};
        else if (clock < add32(B, add32(Cq, Cq) / 3))
            f.H = {-30, 0, 0}, f.T = {40, 0, 0}, f.S = {0, 1, 1}, f.K = {0, -2, 2}, f.rL.x = 70, f.rR.x = -30,
            f.dR = {0, 3, -2}, f.Q = {-30, 0, -10};
        else if (clock < add32(B, Cq))
            f.H = {-30, 0, 0}, f.T = {10, 0, 0}, f.S = {0, 1, 1}, f.K = {0, -2, 2}, f.rL.x = -30, f.rR.x = -40,
            f.dL = {0, 3, -2}, f.Q = {-30, 0, -10};
        else if (clock < R0)
            ends({10, 0, -10});
        break;
    }
    case 0x2D: // 25-30 §7.1: no clamp on the roll
        if (clock < B)
            f.T.x = -10, f.H.x = 10, f.Q.x = -60;
        else if (clock < add32(B, Cq))
            f.rL.x = 80, f.rR.x = 80, f.H.x = -60, f.B.x = (float)sub32(clock, B) / (float)Cq * -360,
            f.K = {0, -3, 0};
        else if (clock < R0)
            f.T.x = 20, f.H.x = -10, f.K = {0, -2, 0};
        break;
    case 0x2E: // 25-30 §7.2, §7.3 (mirrored)
    case 0x2F: {
        const float side = pose == 0x2E ? 1.f : -1.f;
        if (clock < B)
            f.T.z = -10 * side, f.T.y = 30 * side, f.rL.y = f.rR.y = -30 * side, f.H.z = 60 * side;
        else if (clock < add32(B, Cq))
            f.rL.y = f.rR.y = 80 * side, f.B.y = (float)sub32(clock, B) / (float)Cq * (-360 * side);
        else if (clock < R0)
            f.T.y = -30 * side, f.rL.y = f.rR.y = 30 * side;
        break;
    }
    case 0x30: { // 25-30 §7.4 (its hand-turn target is owner +0x148: open, not kept)
        const int32_t halfB = B / 2;
        const auto stepR = [&](float dx) {
            offFromMain(dx);
            upperArm();
            f.rate = 0.01f;
        };
        if (clock < halfB) {
            const float p = clampHigh(t / (float)halfB), q = 1 - p;
            f.T.y = p * 10;
            f.T.z = p * -80;
            f.rR = {q * -90 + p * -90, q * 0.f + p * 90, q * 0.f + p * -30};
            stepR(2);
        } else if (clock < B) {
            f.T = {10, 0, -90};
            f.rR = {0, 110, -20};
            stepR(2);
        } else if (clock < R0) {
            f.T = {0, 0, 90};
            f.rR = {0, 90, 90};
            stepR(-2);
        } else {
            cleared = true;
            offFromMain(-2);
            f.L.z -= 1;
            upperArm();
            f.rR.y = 90;
            f.T.z = -80;
        }
        // Step W, every phase.
        f.B.z = clampUnit((float)sub32(clock, B) / (float)Cq) * 360;
        f.rL = f.rR;
        break;
    }
    case 0x31: // 31-49 §6 (its hand-turn target is owner +0x148: open, not kept)
        if (clock < B) {
            f.B.z = 30;
            f.T = {0, 0, 90};
            f.rR = {-90, -90, 90};
            f.Rh = f.L;
            f.L.x += 2;
            upperArm();
            f.rate = 0.005f;
        } else if (clock < R0) {
            f.T = glm::vec3(0);
            f.rR = {-90, -90, 0};
            if (clock < add32(B, Cq))
                f.E.x = 0.25f;
            offFromMain(-2);
            upperArm();
            f.B.z = clampHigh((float)sub32(clock, B) / (float)Cq) * -360;
            f.rate = 0.03f;
        } else {
            cleared = true;
            offFromMain(-2);
            f.L.z -= 1;
            upperArm();
            f.rR = {-90, 90, 0};
            f.T.z = 0;
        }
        f.rL = f.rR;
        break;
    case 0x32: { // 31-49 §6 (its p × 40 scratch is never read)
        f.rR = {60, -20, 20};
        f.dR = {0, 2, 4};
        f.rL = {60, 20, -20};
        f.dL = {0, 2, 4};
        const auto kick = [&] {
            f.rL.x = -90, f.rL.z = 90, f.rR.x = -90;
            f.dL = {-4, -4, -2}, f.dR = {0, 4, 1};
        };
        if (clock < B) {
            f.T.z = 50;
            kick();
        } else if (clock < add32(B, Cq)) {
            const float p = clampUnit((float)sub32(clock, B) / (float)Cq * 1.5f);
            f.B.z = p * -270;
            f.B.y = p * -40;
            f.S.z += 4;
            kick();
            f.H.x = -30;
            f.H.z = -60;
            f.rate = 0.03f;
        } else {
            guard(0.005f);
        }
        break;
    }
    case 0x34: // 31-49 §6: constant set K, then selector 0x33's body
        setK();
        [[fallthrough]];
    case 0x33:
        if (clock < heldLimit) {
            f.H = {cosw(t * 0.02f) * 5 + 45, 0, 0};
            f.T = {10, 0, 0};
            f.B = {10, 0, 0};
            f.dL = {1, 7, 8};
            f.dR = {-1, 7, 8};
            f.rL = {100, 0, 0};
            f.rR = {100, 0, 0};
        }
        break;
    case 0x35: // 31-49 §6
        f.T = {-30, 0, 0};
        f.dL = {4, 6, 6};
        f.dR = {-4, 6, 6};
        f.S.y = 3;
        break;
    case 0x36: // 31-49 §6: owner +0x40 read here as the movement intent
        if ((m.intent.x * m.intent.x + m.intent.y * m.intent.y) + m.intent.z * m.intent.z > 0) {
            f.dL.y += 4;
            f.B.x = 20;
            f.T.x = -10;
            f.H.x = -20;
            f.rL.x = -20;
            f.rR.x = -20;
            f.dR.y += 4;
            f.F = -40;
            f.footL.z += 3, f.footR.z += 3;
            f.footR.y += 4, f.footL.y += 4;
        }
        break;
    case 0x37: // 31-49 §6
        f.B = {-90, 0, 0};
        f.H.x = 20;
        f.E.z = (a.box.y - a.box.z) * 0.5f - 0.15f;
        f.K = {0, -1, 0};
        f.dL = f.dR = {0, 0, 3};
        break;
    case 0x38: // 31-49 §6
        f.T.x += 30;
        f.dL = {-2, 0, 0};
        f.dR = {2, 0, 0};
        f.rL = {-10, -10, 90};
        f.rR = {-10, 10, -90};
        break;
    case 0x39: // 31-49 §6
        setK();
        break;
    case 0x3A: // 31-49 §6 (the mount terms are open: the engine passes no mount)
        f.T.x = 20;
        f.dL = {4, 6, -3};
        f.dR = {-4, 6, -3};
        f.K = {0, 2, -2};
        f.rL = {60, 90, 60};
        f.rR = {60, -90, -60};
        break;
    case 0x3B: // 31-49 §6
        f.E.z += 1.25f;
        f.T.x = 20;
        f.B.z = -50;
        f.dL = {4, 6, -3};
        f.dR = {-4, 6, -3};
        f.K = {0, 1, 0};
        f.H = {0, 0, 30};
        f.rL = {60, 90, 60};
        f.rR = {60, -90, -60};
        break;
    case 0x3C: // 31-49 §6: the clock against 600 and 5000
        if (clock < 600)
            f.T.z = -80, f.B.z = 0, f.rR = {-90, 90, -90}, f.rL = {20, 90, 90};
        else if (clock < 5000)
            f.T.z = 0, f.B.z = (float)sub32(clock, 600) * 360 / 300, f.rL = {0, -90, 0}, f.rR = {0, 90, 0},
            f.rate = 0.2f;
        else
            f.T.z = 80, f.B.z = 0, f.rL = {90, 90, 270}, f.rR = {0, 90, 90};
        break;
    case 0x3D: // 31-49 §6
        if (clock < B) {
            orbit();
        } else if (clock < add32(B, Cq)) {
            f.T.x = -20;
            f.T.z = 10;
            f.dL = {2, 4, 0};
            f.dR = {-2, 6, 0};
            f.rL = {0, 45, 0};
            f.rR = {0, -45, 0};
        } else if (clock < R0) {
            f.dL = {-3, -6, 0};
            f.dR = {3, -6, 0};
            f.rL = {-90, 0, 130};
            f.rR = {-90, 0, -130};
            f.rate = 0.03f;
        }
        break;
    case 0x3E: // 31-49 §6
        orbit();
        break;
    case 0x3F: { // 31-49 §6: 500 ms windows, divided toward zero
        if (clock >= add32(add32(Cq, 400), B))
            break;
        const int32_t e = add32(sub32(clock, B), 100), q1 = e / 500, q2 = e / 1000;
        if (clock <= B || q1 % 2 == 1)
            f.dL = {2, -2, 12}, f.dR = {-2, -2, 12}, f.T.x = 20, f.B.x = 10;
        else if (q2 % 2 != 0)
            f.dL = {2, -2, 12}, f.dR = {0, 0, -5}, f.rate = 0.03f, f.T = {-10, 0, 20}, f.B = {-5, 0, 10};
        else
            f.dL = {0, 0, -5}, f.dR = {-2, -2, 12}, f.rate = 0.03f, f.T = {-10, 0, -20}, f.B = {-5, 0, -10};
        break;
    }
    case 0x41: // 31-49 §6
        if (clock < R0)
            f.T = {0, 0, sinw(t * 0.03f) * 90}, f.dR = glm::vec3(0), f.dL = glm::vec3(0), f.rate = 0.04f;
        break;
    case 0x42: // 31-49 §6: owner +0x13B4, read as the spent charge (a boomerang in each hand)
        if (c->spentCharge > 0) {
            if (clock < B) {
                f.T.z = -90;
                f.dR = {2, -2, 1};
                f.rR = {0, 10, 0};
                f.offModel = f.mainModel;
            } else if (clock < add32(B, 300)) {
                f.T.z = 10;
                f.dL = {-2, -2, 1};
                f.rL = {0, -10, 0};
                f.offModel = f.mainModel;
                f.mainModel = NO_MODEL;
            } else if (clock < R0) {
                f.mainModel = NO_MODEL;
                f.T.z = 90;
                f.offModel = NO_MODEL;
            }
            f.rate = 0.03f;
            if (clock > B && clock < R0) {
                const float spin = (float)sub32(clock, B) * 360 / 200;
                f.B.z = spin > 700 ? 700.f : spin;
            }
        } else if (clock < B) {
            f.T.z = -90;
            f.dR = {2, -2, 1};
            f.rR = {0, 10, 0};
        } else if (clock < R0) {
            f.T.z = 90;
            f.mainModel = NO_MODEL; // the boomerang is in flight
        }
        break;
    case 0x43: // 31-49 §6: owner +0x144 is the charge
        f.T.z = c->charge * -90;
        f.dR = {2, -2, 1};
        f.rR = {0, 10, 0};
        handBob();
        if (c->charge > 0)
            f.offModel = f.mainModel; // a second boomerang in the off hand
        break;
    case 0x44: // 31-49 §6
        if (clock < B)
            f.T.x = -20, f.rL = f.rR = {-90, 0, 0}, f.dR = {1, -2, 0}, f.dL = {-1, -2, 0};
        else
            f.T.x = 0, f.rL = {0, 45, 0}, f.rR = {0, -45, 0}, f.dR = {-2, 5, 0}, f.dL = {2, 6, 0};
        break;
    case 0x45: { // 31-49 §6
        const int32_t strike = add32(B, Cq);
        if (clock < strike)
            f.rate = 0.03f, f.B.z = t * -360 / (float)strike, f.rR = {-90, -90, -90}, f.dR = {4, 3, 0},
            f.T.z = t * -60 / (float)strike, f.rL = {0, 0, 90};
        else
            f.rL = {0, 0, 90}, f.rR = {-60, -60, -60};
        break;
    }
    case 0x46: // 31-49 §6
        if (clock < B)
            f.B.x = -30;
        else if (clock < add32(B, Cq))
            f.B.x = 120;
        break;
    case 0x47: // 31-49 §6: model 2101 between the hands at one and a half the weapon scale
        if (clock < B) {
            f.dR = {-3, -2, 10};
            f.dL = {3, -2, 10};
            f.offModel = 2101;
            f.offScale = f.offScale * 1.5f;
            f.T.x = 40;
            f.B.x = 10;
        } else if (clock < add32(Cq, B)) {
            f.rate = 0.01f;
            f.dR = f.dL = {0, 5, 0};
            f.T.x = -20;
            f.B.x = -40;
        } else {
            f.dR = {2, -1, 0};
            f.dL = {-2, -1, 0};
        }
        break;
    case 0x48: // 31-49 §6
        if (clock < 5000)
            f.T.x = 20, f.H.x = 30, f.dL = f.dR = {0, 0, 16};
        break;
    default: // 0x0C, 0x0D, 0x0E, 0x24, 0x40 and selectors past 0x49: no case body
        break;
    }
    // Weapon pose contract §5: a zero selector also zeroes the clock; later selector tests see 0.
    if (cleared)
        pose = 0, clock = 0, t = 0;
    // Step 8 (selectors 01-18 §1.2): the auxiliary mid-points average with the rest hands.
    f.armL = (f.L + f.armL) * 0.5f;
    f.armR = (f.Rh + f.armR) * 0.5f;
    // Steps 9–12. Weapon pose contract §5: knocked down, a quadruped (+0x7E bit 0) takes box x
    // (+0x80) for its offset and any other creature box y (+0x84).
    if (m.knockedDown) {
        if (quadruped)
            f.B = {0, 90, 0}, f.E.z = (a.box.x - a.box.z) * 0.5f;
        else
            f.B = {90, 0, 0}, f.E.z = (a.box.y - a.box.z) * 0.5f;
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
            part(f.offModel, back, glm::vec3(f.offScale), centred, false, false, c ? &c->offHand() : nullptr);
        } else {
            // Held (§9): selector 0x47 holds the item at the hands' midpoint, 8 higher.
            const glm::vec3 grasp = pose == 0x47 ? 0.5f * (Rhand + Lhand) + glm::vec3(0, 0, 8) : Lhand;
            // §1.5: the held branch adds half the head pitch about x (the hand turns are zero).
            glm::mat4 at = R(Rzyx(T(arm, grasp), lambda), m.headPitch * 0.5f, {1, 0, 0});
            const bool special = f.offModel == 840 || pose == 0x47;
            if (!special && off == FIST && main != BOOMERANG)
                part(f.offModel, Ry(S(at, f.offScale), 180), glm::vec3(1), grip, false, false, c ? &c->offHand() : nullptr);
            else if (!special && off == BRACELET)
                part(f.offModel, S(at, f.offScale), glm::vec3(extra), grip, false, false, c ? &c->offHand() : nullptr);
            else
                part(f.offModel, at, glm::vec3(f.offScale), grip, false, false, c ? &c->offHand() : nullptr);
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
