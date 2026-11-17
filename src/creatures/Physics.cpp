#include "creatures/Physics.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace creatures {
namespace {

constexpr double ONE = 65536.0;

bool solid(uint8_t type) { return type != 0 && type != 2; }
int64_t fixedOf(double blocks) { return (int64_t)(blocks * ONE); } // trunc toward zero

// Drawn-height terms (alpha-beam-height-leftovers.md, terms and §1). T: a binary32 times
// 65536 in binary32, truncated. F: a fixed value as binary32, times 2^-16. snapLayer: the
// truncating quotient by 65536, one less when negative (so exact negatives go one lower).
int64_t fixedT(float blocks) { return (int64_t)(blocks * 65536.f); }
float floatF(int64_t fixed) { return (float)fixed * (1.f / 65536.f); }
int64_t snapLayer(int64_t fixed) { return fixed / 65536 - (fixed < 0 ? 1 : 0); }
// Physics §0, §5.4: the plain conversion, truncating toward zero (wall probes).
int32_t plainBlock(int64_t fixed) { return (int32_t)(fixed / 65536); }
constexpr int64_t SNAP_LIFT = 66191; // -trunc(1.01 * -65536.0), both snaps' lift above the layer

// "Relax x toward t at rate r per ms", dt times: x + (t - x) r, rounded each time (player
// control §0, scalar helper 0x411d40); nothing for a count that is not positive.
float relax(float x, float t, float r, int32_t dt) {
    for (int32_t elapsed = 0; elapsed < dt; elapsed++)
        x += (t - x) * r;
    return x;
}

// Player control §8.3: ground friction is withheld in action 48, in action 54 until the
// time passes P1 + P2, and in actions 6, 7, 10 and 17-20 until it reaches P1.
bool groundFriction(const Body &b) {
    switch (b.action) {
    case 48: return false;
    case 54: return b.actionMs > b.windUpMs + b.activeMs;
    case 6: case 7: case 10: case 17: case 18: case 19: case 20: return b.actionMs >= b.windUpMs;
    default: return true;
    }
}
glm::vec3 relax(glm::vec3 x, glm::vec3 t, float r, int32_t dt) {
    for (int32_t elapsed = 0; elapsed < dt; elapsed++) {
        const glm::vec3 difference = t - x;
        const glm::vec3 change = difference * r;
        x += change;
    }
    return x;
}

// Physics §3.3: Rz(ψ)(x, y, 0), ψ turned to radians in single precision, the sine and cosine
// in double precision.
glm::vec3 turned(float degrees, float x, float y) {
    const float radians = degrees * (float)(3.141592653589793 / 180.0);
    const double c = std::cos((double)radians), s = std::sin((double)radians);
    return {(float)(x * c - y * s), (float)(x * s + y * c), 0.f};
}
// §3.9: the creature facing Rz(ψc)(0, 1, 0) = (−sin ψc, cos ψc, 0).
glm::vec3 facing(float yaw) { return turned(yaw, 0.f, 1.f); }

// §0: the cells a box covers, inclusive, by the box conversion of P ∓ trunc((s/2)·65536).
struct Cells {
    int32_t lo[3], hi[3];
};
Cells cellsOf(const int64_t p[3], glm::vec3 size, glm::vec3 lift = glm::vec3(0)) {
    Cells c;
    for (int k = 0; k < 3; k++) {
        const int64_t half = fixedOf(size[k] * 0.5);
        const int64_t raise = fixedOf(lift[k]);
        c.lo[k] = boxBlock(p[k] - half + raise);
        c.hi[k] = boxBlock(p[k] + half + raise);
    }
    return c;
}

template <class F> bool any(const Cells &c, F test) {
    for (int32_t x = c.lo[0]; x <= c.hi[0]; x++)
        for (int32_t y = c.lo[1]; y <= c.hi[1]; y++)
            for (int32_t z = c.lo[2]; z <= c.hi[2]; z++)
                if (test(x, y, z))
                    return true;
    return false;
}

// §4.2 step 2: (h/20 − 1 block) × maxHealth × 0.5, each stage narrowed to binary32 and
// truncated back to fixed point; the result as a float.
float fallDamage(int64_t drop, float maxHealth) {
    const int64_t share = fixedT(floatF(drop) / 20.f);
    const int64_t over = share - 65536;
    const int64_t scaled = fixedT(floatF(over) * maxHealth);
    const int64_t halved = fixedT(floatF(scaled) * 0.5f);
    return floatF(halved);
}

// §4.4: the overlap test on `box` (my max ≥ its min, my min < its max, in fixed point on every
// axis), then the horizontal push from it scaled by `scaleX` / my x. A zero-length u skips.
glm::vec3 push(glm::vec3 size, const int64_t self[3], glm::vec3 box, float scaleX, const int64_t them[3], bool stunned) {
    for (int k = 0; k < 3; k++) {
        const int64_t mine = fixedOf(size[k] * 0.5), theirs = fixedOf(box[k] * 0.5);
        if (!(self[k] + mine >= them[k] - theirs && self[k] - mine < them[k] + theirs))
            return glm::vec3(0);
    }
    glm::vec2 u(floatF(self[0] - them[0]), floatF(self[1] - them[1]));
    const float squared = u.x * u.x + u.y * u.y;
    if (squared == 0)
        return glm::vec3(0);
    u *= 1.f / std::sqrt(squared);
    if (stunned)
        u *= 0.1f;
    return glm::vec3(u * (5.f * scaleX / size.x), 0);
}

// Travel §3.5: the noise's value at a lattice point.
double corner(int32_t a, int32_t b) {
    uint32_t n = (uint32_t)a + 57u * (uint32_t)b;
    n ^= n << 13;
    const uint32_t q = (n * (n * n * 60493u + 19990303u) + 1376312589u) & 0x7fffffffu;
    return 1.0 - (double)q / 1073741824.0;
}

int64_t trunc64(double x) { return (int64_t)x; }

} // namespace

int32_t boxBlock(int64_t fixed) { return (int32_t)snapLayer(fixed); }

float skillFactor(int32_t n) { return 1.f - 1.f / (0.1f * (float)n + 1.f); }

bool gliding(const Body &b) {
    return (b.mode & GLIDER) && b.velocity.z < 0 && !(b.contact & ON_GROUND) && b.stunMs < 1 && b.dodgeMs < 1;
}

float valueNoise(double x, double y) {
    const int32_t i = (int32_t)x, j = (int32_t)y;
    const double fx = x - i, fy = y - j;
    const double pi = 3.1415927; // the seven-digit double constant, not π nor a float
    const double wx = (1 - std::cos(pi * fx)) / 2, wy = (1 - std::cos(pi * fy)) / 2;
    return (float)((corner(i, j) * (1 - wx) + corner(i + 1, j) * wx) * (1 - wy) +
                   (corner(i, j + 1) * (1 - wx) + corner(i + 1, j + 1) * wx) * wy);
}

// §3.7: one noise call per component, from the horizontal position only.
glm::vec3 wind(const int64_t p[3]) {
    const double x = (double)p[0], y = (double)p[1];
    return {2 * valueNoise((double)trunc64(0.05 * x) / ONE, (double)trunc64(0.05 * y) / ONE),
            2 * valueNoise((double)(0x0D7F0000LL - trunc64(-0.05 * x)) / ONE, (double)(0x20F60000LL - trunc64(-0.05 * y)) / ONE),
            valueNoise((double)(0x108A0000LL - trunc64(-0.02 * x)) / ONE, (double)(0x14E10000LL - trunc64(-0.02 * y)) / ONE) +
                0.5f};
}

float blendWeight(double rate, int32_t dt) {
    double w = 0;
    for (int32_t elapsed = 0; elapsed < dt; elapsed++)
        w += (1 - w) * rate;
    return (float)w;
}

float stepHeadPitch(float pitch, bool climbing, int32_t dt) {
    if (climbing)
        return relax(pitch, 45.f, 0.005f, dt);
    // Rulings walk-flag-pitch §2.3: x = float(dt) × −0.01f; f = float(precise double exp(x)).
    const float x = (float)dt * -0.01f;
    const float f = (float)std::exp((double)x);
    return f * pitch;
}

std::vector<int32_t> stepPieces(int32_t dt) {
    std::vector<int32_t> out;
    if (dt > 20) {
        const int32_t n = (dt - 21) / 20 + 1;
        out.assign(n, 20);
        out.push_back(dt - 20 * n);
    } else {
        out.push_back(dt);
    }
    return out;
}

void worldStep(Body &body, int32_t dt, const BlockType &block, const ObjectTest &objects, float *riderEnergy) {
    for (int32_t piece : stepPieces(dt))
        if (piece > 0) {
            step(body, piece, block, objects, riderEnergy);
            body.separate = glm::vec3(0); // (worked out for one step: the first piece only)
        }
}

bool lunges(int32_t action, float ms) {
    if (action == 48)
        return true;
    const bool swing = (action >= 1 && action <= 7) || (action >= 9 && action <= 20) || action == 58 || action == 62 ||
                       (action >= 65 && action <= 67);
    return swing && ms < 200;
}

bool inContact(glm::vec3 size, glm::dvec3 self, glm::vec3 other, glm::dvec3 them) {
    const glm::dvec3 d = them - self;
    const double reach = 0.7 * other.x + size.x + 1;
    return d.x * d.x + d.y * d.y < reach * reach && std::abs(d.z) < other.z * 0.5 + size.z + 1;
}

glm::vec3 separationPush(glm::vec3 size, const int64_t self[3], glm::vec3 other, bool enlarge, const int64_t them[3],
                         bool stunned) {
    const glm::vec3 box = enlarge ? other + glm::vec3(1, 1, 0) : other;
    return push(size, self, box, other.x, them, stunned);
}

void step(Body &b, int32_t dt, const BlockType &block, const ObjectTest &objects, float *riderEnergy) {
    const float dts = (float)dt * 0.001f;
    auto blocks = [&](int32_t x, int32_t y, int32_t z) {
        const uint8_t t = block(x, y, z);
        return solid(t) || (b.swimmer && t != 2);
    };

    // The previous step's contact flags and wall.
    const uint32_t before = b.contact;
    const bool onGround = before & ON_GROUND, inWater = before & IN_WATER, onWall = before & ON_WALL;
    const glm::vec3 oldNormal = b.wallNormal;
    const float t = b.actionMs, P1 = b.windUpMs, X = b.activeMs;
    const bool intending = glm::dot(b.intent, b.intent) > 0;
    // §2 item 2: the ground slam drives down while P1 < time < duration and time < P1 + X.
    if (b.action == 54 && t > P1 && t < b.durationMs && t < P1 + X)
        b.velocity.z = -60;
    // §2 item 4: any action folds the glider; any intent clears the lens bit (0x400) and,
    // separately, ends sitting or sleeping (actions 83, 84).
    if (b.action != 0)
        b.mode &= ~GLIDER;
    if (intending) {
        b.mode &= ~LENS;
        if (b.action == 83 || b.action == 84)
            b.action = 0;
    }
    // §2 item 6: intent is an acceleration; a swimmer does not climb out of the water unless
    // holding on (Ctrl).
    glm::vec3 a = b.stunMs > 0 ? glm::vec3(0) : b.intent * dts;
    if (!(b.mode & CLIMB) && (before & NOT_SUBMERGED) && a.z > 0)
        a.z = 0;
    // §3.4: the bank angle; the thresholds, the asin and its scaling in double precision.
    {
        const glm::vec2 ah(a), vh(b.velocity);
        float beta = 0;
        if ((double)glm::dot(ah, ah) > 0.01 && (double)glm::dot(vh, vh) > 0.01) {
            const glm::vec2 an = glm::normalize(ah), vn = glm::normalize(vh);
            const float sine = std::clamp(an.x * vn.y - an.y * vn.x, -1.f, 1.f);
            beta = (float)(std::asin((double)sine) * (180.0 / 3.141592653589793) * 0.5);
        }
        if (b.size.x > 2)
            beta *= 2.f / b.size.x;
        if (gliding(b))
            b.bank = relax(b.bank, std::clamp(beta * 40, -90.f, 90.f), 0.001f, dt);
        else
            b.bank = relax(b.bank, beta, 0.005f, dt);
    }
    // §3.2: the speed limit, row by row in the table's order, with travel's skill factors
    // (travel §0) and the action motions of §3.3 where the table places them.
    float S = 6;
    switch (b.type) { // the fast types
    case 101: case 107: case 108: case 109: case 114: case 115: case 116: case 117: case 118: case 119:
        S = 8;
        break;
    default:
        if (b.appearanceFlags & 0x0200)
            S = 8;
        break;
    }
    if (b.controller == 5)
        S = 12;
    if (b.action == 107 && inWater)
        S *= b.skills[5] >= 1 ? 1 + 3 * skillFactor(b.skills[5]) : 0.f; // sailing
    else if (inWater)
        S *= b.skills[4] >= 1 ? 0.5f + 0.5f * skillFactor(b.skills[4]) : 0.5f; // swimming
    if (b.local && b.action == 107 && onGround && !inWater)
        b.action = 0; // travel §4.1: a beached boat is left
    if (b.local && b.action == 106) { // riding actor step §2
        if (b.mountValid && b.skills[1] >= 1)
            S *= 1.f + skillFactor(b.skills[1]);
        else
            b.action = 0;
        if (intending)
            b.ridingEnergy -= 0.002f * dts;
        if (b.ridingEnergy <= 0)
            b.ridingEnergy = 0, b.action = 0;
        if (b.mode & CLIMB)
            b.action = 0; // Ctrl dismounts
    }
    if (b.mode & GLIDER) {
        const float g = b.skills[3] >= 1 ? 1.5f + 3 * skillFactor(b.skills[3]) : 0.f;
        if (g <= 0)
            b.mode &= ~GLIDER;
        else if (!onGround)
            S *= g;
    }
    if (b.action == 79) { // sneaking; a non-player without the skill uses its fallback
        const int32_t n = b.skills[7] == 0 && b.controller != 0 ? b.skillFallback : b.skills[7];
        S *= 0.5f + 0.4f * skillFactor(n);
    }
    const bool climbingOnWall = (b.mode & CLIMB) && onWall;
    const bool gliderInAir = (b.mode & GLIDER) && !onGround;
    if ((b.mode & RUN) && !climbingOnWall && !gliderInAir)
        S = (float)((double)S * 2.0);
    S *= b.haste;
    // Charges move at 40; action 54 only before P1 + X.
    if (b.action == 48 || (b.action == 54 && t < P1 + X) || ((b.action == 50 || b.action == 96) && t < 500))
        S = 40;
    // §3.3: a melee lunge with nobody in reach surges along the facing at 2S (this row's S, no
    // dt factor); the cap then holds it at the final S. The local player, or any creature that
    // is not a received replica (game byte +0xb4).
    if ((b.local || !b.replica) && lunges(b.action, t) && !b.targetInContact)
        a += turned(b.yaw, 0.f, 2 * S);
    // Action 81 (eating) stands still for its use length, then becomes 83.
    if (b.action == 81 && b.useLimitMs >= 0) {
        if (t <= b.useLimitMs)
            S = 0;
        else
            b.action = 83;
    }
    // A dodge cancels the action, except a dodge attack (50, 96) within its duration. (The
    // exception for bytes obj +0x140/+0x141 = 4/1 keeping actions 5, 17 and 20 is unresolved
    // and not modelled.)
    if (b.dodgeMs != 0 && !((b.action == 50 || b.action == 96) && t <= b.durationMs))
        b.action = 0;
    // §3.3: dashes replace the acceleration with 20 along the facing (or to a side).
    switch (b.action) {
    case 68: case 69: case 93:
        if (t <= P1)
            S = 0;
        else if (t < P1 + (float)((int32_t)X * 3 / 4))
            a = 20.f * facing(b.yaw), S = 50;
        break;
    case 76:
        if (t > P1 && t < P1 + X)
            a = 20.f * facing(b.yaw), S = 60;
        break;
    case 77:
        if (t > P1 && t < P1 + X)
            a = 20.f * turned(b.yaw, -1.f, 0.f), S = 60;
        break;
    case 78:
        if (t > P1 && t < P1 + X)
            a = 20.f * turned(b.yaw, 1.f, 0.f), S = 60;
        break;
    case 71:
        if (t < b.durationMs)
            S = 0;
        break;
    case 72: // the spin
        if (t < b.durationMs) {
            S = 12;
            b.yaw += 720.f * dts;
        }
        break;
    default:
        break;
    }
    if (b.controller == 4)
        S = 30;
    if ((b.action == 57 || b.action == 74) && t >= P1 && t < b.durationMs)
        S = 0.1f; // channelled
    if (b.slowed)
        S *= 0.5f;
    if (b.hastened)
        S *= 1.5f;
    // Aim-triplet contract §3.9: walking away from the aim slows a fighter (straight back
    // halves the limit). Both vectors are normalized precisely in two dimensions.
    if ((b.mode & AIMING) && b.dodgeMs < 1 && !((b.action == 50 || b.action == 96) && t <= 1000)) {
        glm::vec2 aim(b.look), v(b.velocity);
        if (v.y * aim.y + v.x * aim.x < 0.f) {
            const auto normalize = [](glm::vec2 &value) {
                const float squared = value.x * value.x + value.y * value.y;
                const float inverse = 1.f / float(std::sqrt(double(squared)));
                value = {value.x * inverse, value.y * inverse};
            };
            normalize(aim);
            normalize(v);
            const float dot = v.y * aim.y + v.x * aim.x;
            S *= dot * 0.5f + 1.f;
        }
    }
    // §3.6: stamina. Climbing with a key held costs it by the climbing skill; lifting the
    // glider costs it; hanging on a wall or gliding without the lift keeps it; anything else
    // refills it.
    if ((b.mode & CLIMB) && !(b.mode & GLIDER) && onWall && !onGround) {
        if (intending) {
            const float k = 1.f - 1.f / ((float)(b.skills[2] + 1) * 0.1f + 1.f);
            b.stamina = std::max(0.f, b.stamina - (1 - k) * 0.0005f * (float)dt);
            if (b.stamina < 0.01f)
                b.mode &= ~CLIMB;
        }
    } else if (gliderInAir) {
        if (b.mode & RUN)
            b.stamina -= 0.0002f * (float)dt;
    } else {
        b.stamina += 0.0001f * (float)dt;
    }
    b.stamina = std::clamp(b.stamina, 0.f, 1.f);
    // Step 9: integrate, capping the horizontal speed.
    b.velocity += a;
    const float h2 = b.velocity.x * b.velocity.x + b.velocity.y * b.velocity.y;
    if (h2 > S * S) {
        const float k = S / std::sqrt(h2);
        b.velocity.x *= k;
        b.velocity.y *= k;
    }
    // §3.5: climbing (mode 0x01, mode 0x10 clear, on a wall) holds the creature, flyers and
    // gliders sink at a tenth of gravity, the rest fall.
    const float z = floatF(b.position[2]);
    if ((b.mode & CLIMB) && !(b.mode & GLIDER) && onWall) {
        b.velocity = relax(b.velocity, glm::vec3(0), 0.0025f, dt);
        if (b.velocity.z > -10)
            b.fallStart = z;
    } else if ((b.flyer && b.stunMs <= 0) || gliding(b)) {
        b.velocity.z -= 3.f * dts;
        b.fallStart = z;
    } else {
        if (b.velocity.z >= 0)
            b.fallStart = z;
        b.velocity.z -= 30.f * dts;
    }
    // §3.7 / travel §3.4: the player's glider control.
    if (b.local && (b.mode & GLIDER)) {
        // Travel §3.6: the stun (1500 ms for the player, controller kind 0) takes effect in this
        // step, so the stun rule below also lets go of the wall.
        const auto crash = [&] {
            b.crashed = true;
            b.stunMs = 1500;
            b.mode &= ~GLIDER;
        };
        if (inWater) {
            b.mode &= ~GLIDER;
        } else if (onGround) {
            if (std::abs(b.bank) > 40)
                crash();
        } else {
            // A wall crash still lifts and blows the creature this step (ground and water do not).
            if (onWall && glm::dot(b.velocity, b.velocity) > 64 && glm::dot(oldNormal, b.velocity) < -4)
                crash();
            if ((b.mode & RUN) && b.velocity.z < 0)
                b.velocity.z += 30.f * dts; // the lift
            b.velocity += wind(b.position) * dts;
        }
    }
    // §2 item 12: a stunned creature, or one inside a running action, lets go of the wall and
    // the glider.
    if (b.stunMs > 0)
        b.mode &= ~(GLIDER | CLIMB);
    if (b.action != 0 && t < b.durationMs)
        b.mode &= ~(GLIDER | CLIMB);

    // §4.1 step 1, open points 4 §9: unstick first, once a step. A box overlapping any solid
    // block, even at head height, has its bottom set 1.1 blocks above the floor of the block
    // layer it is in, whether or not that is free; deep inside, it climbs a block a step. (Never
    // for inert objects, controller 6. Nothing like this exists for static objects.)
    if (b.controller != 6 && !b.replica) {
        // (Cells of zones not loaded are passed over.)
        const auto solidHere = [&](int32_t x, int32_t y, int32_t z) {
            const uint8_t type = block(x, y, z);
            return solid(type) && type != UNLOADED_CELL;
        };
        if (any(cellsOf(b.position, b.size), solidHere)) {
            constexpr int64_t LIFT = 72089; // 1.1 in fixed point, as the game has it
            const int64_t half = fixedOf(b.size.z * 0.5);
            const int64_t layer = snapLayer(b.position[2] - half); // (exact negatives go lower)
            b.position[2] = layer * 65536 + half + LIFT;
            b.bob = 0;
        }
        // Airship steering §5: then, once, out of any airship's voxels.
        if (b.obstacles)
            b.obstacles->unstick(b.position, b.size);
    }

    // Step 13: the displacement, with step 14's separation (not for kind 6 nor actions 83 and
    // 84), then §4: substeps under one block, d × the single-precision 1/n, x, y, z each.
    const bool separates = b.controller != 6 && b.action != 83 && b.action != 84;
    glm::vec3 d = (b.velocity + b.push + (separates ? b.separate : glm::vec3(0))) * dts;
    const int64_t start[3] = {b.position[0], b.position[1], b.position[2]};
    const float squared = (d.x * d.x + d.y * d.y) + d.z * d.z;
    const float length = (float)std::sqrt((double)squared);
    const int n = (int)(length + 1.f);
    const float reciprocal = 1.f / (float)n;
    d *= reciprocal;
    bool touchedObject = false, onObject = false;
    bool onVoxelObject = false; // world objects §3.1: a z-axis airship hit this update
    // Height §1.2 step 1: a static-object hit withdraws the step-up (and step-down) for the
    // rest of this update.
    bool stepAllowed = true;
    const int64_t halfHeight = fixedT(b.size.z * 0.5f);
    const auto objectHere = [&](int axis, int64_t move) {
        if (!objects || move == 0) // §4.5: a d[k] of zero never hits
            return false;
        const glm::dvec3 c = b.blocks(), h = glm::dvec3(b.size) * 0.5;
        return objects(c - h, c + h, axis, (double)move / ONE, b.ignoredStatics);
    };
    // §4.5: a static hit sets the object result and ends step-up; on z while falling it is
    // standing on the object.
    const auto staticHit = [&](int axis) {
        touchedObject = true;
        stepAllowed = false;
        if (axis == 2 && b.velocity.z < 0)
            onObject = true;
    };
    for (int s = 0; s < n; s++) {
        // World objects §3.1: the base height of the last airship hit, reset each sub-step.
        int64_t base = 0;
        for (int k = 0; k < 3; k++) {
            const int64_t move = fixedOf(d[k]);
            b.position[k] += move;
            bool hit = any(cellsOf(b.position, b.size), blocks);
            if (hit) {
                // §4.1 step 3.3: on z the statics are also tested after a cell hit, unless the
                // creature already stands on an object this update; x and y respond directly.
                if (k == 2 && !onObject && objectHere(k, move))
                    staticHit(k);
            } else if (objectHere(k, move)) {
                hit = true;
                staticHit(k);
            } else if (b.obstacles && b.obstacles->collide(b.position, b.size, b.key, base)) {
                // World objects §3.2: the airships, voxel by voxel; a hit acts as a cell.
                hit = true;
                onVoxelObject = onVoxelObject || k == 2;
            }
            if (!hit)
                continue;
            if (k == 2) {
                // §4.2, at the moved position: the landing sound (any creature), the local
                // player's fall damage out of the water, then the undo.
                if (b.velocity.z < -2)
                    b.thudded = true;
                const int64_t drop = fixedT(b.fallStart) - b.position[2];
                if (b.local && !inWater && b.velocity.z < -5 && drop > fixedT(20.f)) {
                    b.fallHurt += fallDamage(drop, b.maxHealth);
                    b.mode &= ~CLIMB;
                }
                b.position[2] -= move;
                b.velocity.z = 0;
                b.push.z = 0;
                b.fallStart = floatF(b.position[2]);
                continue;
            }
            // §4.3: step up onto a ledge, else undo this axis only (velocity kept). The raised
            // box must clear the terrain, then every airship (world objects §4).
            // Height §1.2 step 8: the snap, relative to the sub-step's airship base, keeps the
            // drawn height (z - offset) continuous.
            if (stepAllowed && !b.swimmer && !any(cellsOf(b.position, b.size, {0, 0, 1.01f}), blocks) &&
                !(b.obstacles && b.obstacles->blocksRaised(b.position, b.size, b.key))) {
                const int64_t relative = b.position[2] - base;
                const int64_t raised = 65536 * snapLayer(relative - halfHeight) + halfHeight + SNAP_LIFT;
                b.bob = floatF(raised - (relative - fixedT(b.bob)));
                b.position[2] = raised + base;
            } else {
                b.position[k] -= move;
            }
        }
    }

    // §5.1: contact flags.
    b.contact = touchedObject ? ON_OBJECT : 0;
    if (onObject || onVoxelObject) // world objects §3.4: either landing flag stands
        b.contact |= ON_GROUND;
    const Cells box = cellsOf(b.position, b.size);
    {
        const glm::vec3 moved(floatF(b.position[0] - start[0]), floatF(b.position[1] - start[1]),
                              floatF(b.position[2] - start[2]));
        const glm::vec2 vh(b.velocity), ih(b.intent);
        if (b.stunMs <= 0 && glm::dot(vh, vh) > 16 && glm::dot(ih, ih) > 16 &&
            glm::dot(moved, moved) < 0.01f * glm::dot(d, d))
            b.contact |= STUCK;
    }
    int32_t waterTop = INT32_MIN;
    bool water = false;
    for (int32_t x = box.lo[0]; x <= box.hi[0]; x++)
        for (int32_t y = box.lo[1]; y <= box.hi[1]; y++)
            for (int32_t zc = box.lo[2]; zc <= box.hi[2]; zc++) {
                if (block(x, y, zc) == 2) {
                    water = true;
                    waterTop = std::max(waterTop, zc + 1);
                } else {
                    b.contact |= NOT_SUBMERGED;
                }
            }
    const bool wet = water && b.controller != 6; // §5.1: never in water as controller kind 6
    if (wet) {
        b.contact |= IN_WATER;
        waterTop = std::max(waterTop, box.lo[2]);
        // §5.2: buoyancy, the numerator converted from fixed point.
        const float r = floatF((int64_t)waterTop * 65536 - b.position[2]) / (b.size.z * 0.5f);
        const float lift = r <= 0 ? 0.f : (r < 1 || b.action == 107) ? r + 1 : 1.f; // a boat has no cap
        b.velocity.z += 30.f * dts * lift;
    }
    // Riding actor step §3: liquid and visited type-3 ground cells share one
    // refill mark. A dry probe visits the full Y span of its first solid column.
    bool refill = wet;
    if (!wet) {
        const int32_t z = (int32_t)snapLayer(b.position[2] - halfHeight - fixedT(0.2f));
        for (int32_t x = box.lo[0]; x <= box.hi[0]; x++) {
            for (int32_t y = box.lo[1]; y <= box.hi[1]; y++) {
                const uint8_t type = block(x, y, z);
                if (solid(type))
                    b.contact |= ON_GROUND;
                refill |= type == 3;
            }
            if (b.contact & ON_GROUND)
                break;
        }
    }
    if (refill && b.controller == 5 && riderEnergy && *riderEnergy < 1.f)
        *riderEnergy = std::min(1.f, *riderEnergy + 0.05f * dts);
    const glm::vec2 dh(d);
    // Height §1.3: on the terrain grid only, at most once an update, and not after a static hit.
    if ((before & ON_GROUND) && stepAllowed && glm::dot(dh, dh) > 0 && !(b.contact & (ON_GROUND | IN_WATER)) &&
        d.z <= 0) {
        Cells g = box;
        const int64_t zBefore = b.position[2];
        const int64_t layer = snapLayer(zBefore - halfHeight - fixedT(1.2f));
        g.lo[2] = g.hi[2] = (int32_t)layer;
        if (any(g, [&](int32_t x, int32_t y, int32_t zc) { return solid(block(x, y, zc)); })) {
            b.contact |= ON_GROUND;
            const int64_t lowered = 65536 * layer + SNAP_LIFT + halfHeight;
            b.bob = floatF(lowered - (zBefore - fixedT(b.bob)));
            b.position[2] = lowered;
        }
    }
    // §6: friction (the boat first, then the ground), then the push.
    const float R = intending ? 0.0025f : 0.005f;
    if (wet && b.action == 107) {
        // Player control §8.3: x and y relax toward 0 at 0.1 R, one rounded step per ms.
        b.velocity.x = relax(b.velocity.x, 0.f, 0.1f * R, dt);
        b.velocity.y = relax(b.velocity.y, 0.f, 0.1f * R, dt);
        b.velocity.z = relax(b.velocity.z, 0.f, R, dt);
    }
    if ((b.contact & ON_GROUND) && groundFriction(b))
        b.velocity = relax(b.velocity, glm::vec3(0), R, dt);
    if ((b.contact & (ON_GROUND | IN_WATER)) || (b.mode & GLIDER) || b.pathActive)
        b.push = relax(b.push, glm::vec3(0), R, dt);
    // §5.4: wall probes along the world axes, with the plain (truncating) conversion.
    {
        // The probe sits s/2 + 0.2 from the centre for controller kind 0, otherwise at
        // trunc(s/2 + 0.5) + 0.5 + 0.05.
        const auto reach = [&](float extent) {
            const float half = extent * 0.5f;
            return fixedT(b.controller == 0 ? half + 0.2f : std::trunc(half + 0.5f) + 0.5f + 0.05f);
        };
        const int64_t rx = reach(b.size.x), ry = reach(b.size.y);
        const int64_t px = b.position[0], py = b.position[1];
        auto column = [&](int32_t x0, int32_t x1, int32_t y0, int32_t y1) {
            for (int32_t x = x0; x <= x1; x++)
                for (int32_t y = y0; y <= y1; y++)
                    for (int32_t zc = box.lo[2]; zc <= box.hi[2]; zc++)
                        if (solid(block(x, y, zc)))
                            return true;
            return false;
        };
        const int32_t yLo = plainBlock(py - ry), yHi = plainBlock(py + ry);
        const int32_t xLo = plainBlock(px - rx), xHi = plainBlock(px + rx);
        // All four axis probes run in this order, each hit writing one component (a later hit
        // overwrites the same one), so N may be diagonal and is not normalised.
        const struct {
            bool hit;
            int axis;
            float value;
        } probes[] = {
            {column(box.lo[0], box.hi[0], yLo, yLo), 1, 1},
            {column(box.lo[0], box.hi[0], yHi, yHi), 1, -1},
            {column(xLo, xLo, box.lo[1], box.hi[1]), 0, 1},
            {column(xHi, xHi, box.lo[1], box.hi[1]), 0, -1},
        };
        b.wallNormal = glm::vec3(0);
        for (const auto &probe : probes)
            if (probe.hit) {
                b.wallNormal[probe.axis] = probe.value;
                b.contact |= ON_WALL;
            }
        // Only when none hit: the corner columns (−,−), (+,−), (−,+), (+,+) the creature does
        // not face away from; the first hit points N away from that corner.
        if (!(b.contact & ON_WALL)) {
            const glm::vec3 ahead = facing(b.yaw);
            bool found = false;
            for (int cy = -1; cy <= 1 && !found; cy += 2)
                for (int cx = -1; cx <= 1 && !found; cx += 2) {
                    if (ahead.x * (0.7f * (float)cx) + ahead.y * (0.7f * (float)cy) < -0.1f)
                        continue;
                    const int32_t x = plainBlock(px + cx * rx), y = plainBlock(py + cy * ry);
                    if (column(x, x, y, y)) {
                        found = true;
                        b.contact |= ON_WALL;
                        b.wallNormal = glm::vec3((float)-cx, (float)-cy, 0);
                    }
                }
        }
    }
    // §5.3: water drag and the swim pitch (not for controller kind 6 nor appearance flag
    // 0x0001); a boat or a creature out of the water levels out. Then the splash.
    if (wet && b.action != 107) {
        b.velocity = relax(b.velocity, glm::vec3(0), 0.005f, dt);
        if (b.controller != 6 && !(b.appearanceFlags & 0x0001))
            b.swimPitch = relax(b.swimPitch, -60.f, 0.01f, dt);
    } else {
        b.swimPitch = relax(b.swimPitch, 0.f, 0.01f, dt);
    }
    if (wet && !(before & IN_WATER) && b.velocity.z < -3)
        b.splashed = true;
    // §3.8: over the top of a wall, the climber (mode 0x01, mode 0x10 clear) is shoved onto the
    // ledge.
    if (b.local && (b.mode & CLIMB) && !(b.mode & GLIDER) && onWall && !(b.contact & ON_WALL) &&
        glm::dot(glm::vec2(b.intent), glm::vec2(b.intent)) > 0) {
        if (glm::dot(glm::vec2(facing(b.yaw)), glm::vec2(oldNormal)) <= 0)
            b.velocity = -5.f * oldNormal;
    }
    // §5.3: creature type 101 pitches up during the first second of action 51.
    if (b.type == 101)
        b.swimPitch = relax(b.swimPitch, b.action == 51 && t < 1000 ? 60.f : 0.f, 0.01f, dt);
    // §2 item 18, in order: the glider and lantern equipment checks, the landing bob, the dodge.
    if (!b.gliderEquipped)
        b.mode &= ~GLIDER;
    if (!b.lanternEquipped)
        b.mode &= ~LANTERN;
    // Height §1.5: the offset approaches 0 once per millisecond of the step, each operation in
    // binary32; 0.01 when below zero, 0.02 otherwise (NaN included).
    const float bobRate = 0.f > b.bob ? 0.01f : 0.02f;
    for (int32_t elapsed = 0; elapsed < dt; elapsed++)
        b.bob = (0.f - b.bob) * bobRate + b.bob;
    if (b.dodgeMs > 500 && b.dodgeMs - dt <= 500)
        b.rolled = true;
    b.dodgeMs = std::max(0, b.dodgeMs - dt);
}

} // namespace creatures
