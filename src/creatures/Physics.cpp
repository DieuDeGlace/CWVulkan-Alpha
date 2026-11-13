#include "creatures/Physics.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace creatures {
namespace {

constexpr double ONE = 65536.0;

bool solid(uint8_t type) { return type != 0 && type != 2; }
int32_t blockOf(int64_t fx) { return (int32_t)(fx >= 0 ? fx / 65536 : -((-fx + 65535) / 65536)); }
int64_t fixedOf(double blocks) { return (int64_t)(blocks * ONE); } // trunc toward zero

// Drawn-height terms (alpha-beam-height-leftovers.md, terms and §1). T: a binary32 times
// 65536 in binary32, truncated. F: a fixed value as binary32, times 2^-16. snapLayer: the
// truncating quotient by 65536, one less when negative (so exact negatives go one lower).
int64_t fixedT(float blocks) { return (int64_t)(blocks * 65536.f); }
float floatF(int64_t fixed) { return (float)fixed * (1.f / 65536.f); }
int64_t snapLayer(int64_t fixed) { return fixed / 65536 - (fixed < 0 ? 1 : 0); }
constexpr int64_t SNAP_LIFT = 66191; // -trunc(1.01 * -65536.0), both snaps' lift above the layer

// "Relax x toward t at rate r per ms", dt times.
float relax(float x, float t, float r, int32_t dt) { return t + (x - t) * std::pow(1.f - r, (float)dt); }
glm::vec3 relax(glm::vec3 x, glm::vec3 t, float r, int32_t dt) {
    for (int32_t elapsed = 0; elapsed < dt; elapsed++) {
        const glm::vec3 difference = t - x;
        const glm::vec3 change = difference * r;
        x += change;
    }
    return x;
}

// §0: the cells a box covers, inclusive. A face exactly on a boundary takes the block beyond
// it on the max side only.
struct Cells {
    int32_t lo[3], hi[3];
};
Cells cellsOf(const int64_t p[3], glm::vec3 size, glm::vec3 lift = glm::vec3(0)) {
    Cells c;
    for (int k = 0; k < 3; k++) {
        const int64_t half = fixedOf(size[k] * 0.5);
        const int64_t raise = fixedOf(lift[k]);
        c.lo[k] = blockOf(p[k] - half + raise);
        c.hi[k] = blockOf(p[k] + half + raise);
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

// Travel §3.5: the noise's value at a lattice point.
double corner(int32_t a, int32_t b) {
    uint32_t n = (uint32_t)a + 57u * (uint32_t)b;
    n ^= n << 13;
    const uint32_t q = (n * (n * n * 60493u + 19990303u) + 1376312589u) & 0x7fffffffu;
    return 1.0 - (double)q / 1073741824.0;
}

int64_t trunc64(double x) { return (int64_t)x; }

} // namespace

float skillFactor(int32_t n) { return 1.f - 1.f / (0.1f * (float)n + 1.f); }

bool gliding(const Body &b) {
    return (b.mode & GLIDER) && b.velocity.z < 0 && !(b.contact & ON_GROUND) && b.stunMs < 1 && b.dodgeMs < 1;
}

float valueNoise(double x, double y) {
    const int32_t i = (int32_t)x, j = (int32_t)y;
    const double fx = x - i, fy = y - j;
    const double pi = (double)3.1415927f;
    const double wx = (1 - std::cos(pi * fx)) / 2, wy = (1 - std::cos(pi * fy)) / 2;
    return (float)((corner(i, j) * (1 - wx) + corner(i + 1, j) * wx) * (1 - wy) +
                   (corner(i, j + 1) * (1 - wx) + corner(i + 1, j + 1) * wx) * wy);
}

glm::vec3 wind(const int64_t p[3]) {
    const double x = (double)p[0], y = (double)p[1];
    return {2 * valueNoise((double)trunc64(0.05 * x) / ONE, (double)trunc64(0.05 * y) / ONE),
            2 * valueNoise((double)(0x0D7F0000LL - trunc64(-0.05 * x)) / ONE, (double)(0x20F60000LL - trunc64(-0.05 * y)) / ONE),
            valueNoise((double)(0x108A0000LL - trunc64(-0.02 * x)) / ONE, (double)(0x14E10000LL - trunc64(-0.02 * y)) / ONE) +
                0.5f};
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

void worldStep(Body &body, int32_t dt, const BlockType &block, const ObjectTest &objects) {
    for (int32_t piece : stepPieces(dt))
        if (piece > 0) {
            step(body, piece, block, objects);
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

glm::vec3 separation(glm::vec3 size, glm::vec3 other, glm::dvec3 self, glm::dvec3 them, bool stunned) {
    const glm::dvec3 d = self - them;
    for (int k = 0; k < 3; k++)
        if (std::abs(d[k]) >= other[k] * 0.5 + size[k] * 0.5)
            return glm::vec3(0);
    glm::vec2 u(d.x, d.y);
    if (glm::dot(u, u) > 0)
        u = glm::normalize(u);
    if (stunned)
        u *= 0.1f;
    return glm::vec3(u * 5.f * (other.x / std::max(size.x, 0.01f)), 0);
}

void step(Body &b, int32_t dt, const BlockType &block, const ObjectTest &objects) {
    const float dts = (float)dt * 0.001f;
    auto blocks = [&](int32_t x, int32_t y, int32_t z) {
        const uint8_t t = block(x, y, z);
        return solid(t) || (b.swimmer && t != 2);
    };

    // The previous step's contact flags and wall.
    const uint32_t before = b.contact;
    const bool onGround = before & ON_GROUND, inWater = before & IN_WATER, onWall = before & ON_WALL;
    const glm::vec3 oldNormal = b.wallNormal;
    // §2 step 3: any action folds the glider.
    if (b.action != 0)
        b.mode &= ~GLIDER;
    // §2 step 4: intent is an acceleration; a swimmer does not climb out of the water unless
    // holding on (Ctrl).
    glm::vec3 a = b.stunMs > 0 ? glm::vec3(0) : b.intent * dts;
    if (!(b.mode & CLIMB) && (before & NOT_SUBMERGED) && a.z > 0)
        a.z = 0;
    // §2 step 2: the ground slam drives down between P1 and P1 + X.
    if (b.action == 54 && b.actionMs > b.windUpMs && b.actionMs < b.windUpMs + b.activeMs)
        b.velocity.z = -60;
    // §3.4: the bank angle.
    {
        const glm::vec2 ah(a), vh(b.velocity);
        float beta = 0;
        if (glm::dot(ah, ah) > 0.01f && glm::dot(vh, vh) > 0.01f) {
            const glm::vec2 an = glm::normalize(ah), vn = glm::normalize(vh);
            beta = (float)(std::asin(std::clamp(an.x * vn.y - an.y * vn.x, -1.f, 1.f)) * (180.0 / 3.141592653589793)) * 0.5f;
        }
        if (b.size.x > 2)
            beta *= 2.f / b.size.x;
        if (gliding(b))
            b.bank = relax(b.bank, std::clamp(beta * 40, -90.f, 90.f), 0.001f, dt);
        else
            b.bank = relax(b.bank, beta, 0.005f, dt);
    }
    // §3.2: the speed limit, with travel's skill factors (travel §0).
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
    if (b.local && b.action == 107 && onGround && !inWater)
        b.action = 0; // travel §4.1: a beached boat is left
    if (b.action == 107 && inWater)
        S *= b.skills[5] >= 1 ? 1 + 3 * skillFactor(b.skills[5]) : 0.f; // sailing
    else if (inWater)
        S *= b.skills[4] >= 1 ? 0.5f + 0.5f * skillFactor(b.skills[4]) : 0.5f; // swimming
    if (b.local && b.action == 106) { // travel §5.3: riding
        if (!b.mountValid) {
            b.action = 0;
        } else {
            const float k = b.skills[1] >= 1 ? 1 + skillFactor(b.skills[1]) : 0.f;
            S *= k;
            if (k == 0)
                b.action = 0;
        }
        if (b.action == 106 && glm::dot(b.intent, b.intent) > 0) {
            b.ridingEnergy -= 0.002f * dts;
            if (b.ridingEnergy <= 0)
                b.ridingEnergy = 0, b.action = 0;
        }
        if (b.action == 106 && (b.mode & CLIMB))
            b.action = 0; // Ctrl dismounts
    }
    if (b.mode & GLIDER) {
        const float g = b.skills[3] >= 1 ? 1.5f + 3 * skillFactor(b.skills[3]) : 0.f;
        if (g <= 0)
            b.mode &= ~GLIDER;
        else if (!onGround)
            S *= g;
    }
    if (b.action == 79) // sneaking
        S *= 0.5f + 0.4f * skillFactor(b.skills[7]);
    const bool climbingOnWall = (b.mode & CLIMB) && onWall;
    const bool gliderInAir = (b.mode & GLIDER) && !onGround;
    if ((b.mode & RUN) && !climbingOnWall && !gliderInAir)
        S = (float)((double)S * 2.0);
    S *= b.haste;
    // Charges move at 40.
    const float t = b.actionMs, P1 = b.windUpMs, X = b.activeMs;
    if (b.action == 48 || (b.action == 54 && t >= P1 + X) || ((b.action == 50 || b.action == 96) && t < 500))
        S = 40;
    // §3.3: dashes replace the acceleration with 20 along the facing (or to a side).
    {
        const float yaw = glm::radians(b.yaw);
        const glm::vec3 facing(-std::sin(yaw), std::cos(yaw), 0), left(-std::cos(yaw), -std::sin(yaw), 0);
        switch (b.action) {
        case 68: case 69: case 93:
            if (t <= P1)
                S = 0;
            else if (t < P1 + 0.75f * X)
                a = 20.f * facing, S = 50;
            break;
        case 76:
            if (t > P1 && t < P1 + X)
                a = 20.f * facing, S = 60;
            break;
        case 77:
            if (t > P1 && t < P1 + X)
                a = 20.f * left, S = 60;
            break;
        case 78:
            if (t > P1 && t < P1 + X)
                a = -20.f * left, S = 60;
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
    }
    if (b.controller == 4)
        S = 30;
    if ((b.action == 57 || b.action == 74) && t > P1 && t < b.durationMs)
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
    // §3.3: a melee lunge with nobody in reach surges along the facing (no dt factor); the cap
    // then holds it at S.
    if (lunges(b.action, t) && !b.targetInContact) {
        const float yaw = glm::radians(b.yaw);
        a += 2 * S * glm::vec3(-std::sin(yaw), std::cos(yaw), 0);
    }
    // §3.6: stamina. Climbing with a key held costs it by the climbing skill; lifting the
    // glider costs it; hanging on a wall or gliding without the lift keeps it; anything else
    // refills it.
    if ((b.mode & CLIMB) && !(b.mode & GLIDER) && onWall && !onGround) {
        if (glm::dot(b.intent, b.intent) > 0) {
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
    // Step 7: integrate, capping the horizontal speed.
    b.velocity += a;
    const float h2 = b.velocity.x * b.velocity.x + b.velocity.y * b.velocity.y;
    if (h2 > S * S) {
        const float k = S / std::sqrt(h2);
        b.velocity.x *= k;
        b.velocity.y *= k;
    }
    // §3.5: climbing holds the creature, flyers and gliders sink at a tenth of gravity, the
    // rest fall.
    const float z = (float)(b.position[2] / ONE);
    if ((b.mode & CLIMB) && !gliding(b) && onWall) {
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
        const auto crash = [&] {
            b.crashed = true;
            b.mode &= ~GLIDER;
        };
        if (inWater) {
            b.mode &= ~GLIDER;
        } else if (onGround) {
            if (std::abs(b.bank) > 40)
                crash();
        } else if (onWall && glm::dot(b.velocity, b.velocity) > 64 && glm::dot(oldNormal, b.velocity) < -4) {
            crash();
        } else {
            if ((b.mode & RUN) && b.velocity.z < 0)
                b.velocity.z += 30.f * dts; // the lift
            b.velocity += wind(b.position) * dts;
        }
    }
    // §2 step 10: a stunned creature lets go of the wall and the glider.
    if (b.stunMs > 0)
        b.mode &= ~(GLIDER | CLIMB);

    // §4.1 step 1, open points 4 §9: unstick first, once a step. A box overlapping any solid
    // block, even at head height, has its bottom set 1.1 blocks above the floor of the block
    // layer it is in, whether or not that is free; deep inside, it climbs a block a step. (Never
    // for inert objects, controller 6. Nothing like this exists for static objects.)
    if (b.controller != 6) {
        // (Cells of zones not loaded are passed over.)
        const auto solidHere = [&](int32_t x, int32_t y, int32_t z) {
            const uint8_t type = block(x, y, z);
            return solid(type) && type != UNLOADED_CELL;
        };
        if (any(cellsOf(b.position, b.size), solidHere)) {
            constexpr int64_t LIFT = 72089; // 1.1 in fixed point, as the game has it
            const int64_t half = fixedOf(b.size.z * 0.5);
            const int64_t bottom = b.position[2] - half;
            const int64_t layer = bottom >= 0 ? bottom / ONE : bottom / ONE - 1; // (as read: exact negatives go lower)
            b.position[2] = layer * ONE + half + LIFT;
            b.bob = 0;
        }
    }

    // Step 11: the displacement, then §4: substeps under one block, x, y, z each.
    glm::vec3 d = (b.velocity + b.push + b.separate) * dts;
    const int64_t start[3] = {b.position[0], b.position[1], b.position[2]};
    const float squared = (d.x * d.x + d.y * d.y) + d.z * d.z;
    const float length = (float)std::sqrt((double)squared);
    const int n = (int)(length + 1.f);
    const float reciprocal = 1.f / (float)n;
    d *= reciprocal;
    const uint32_t old = b.contact;
    bool touchedObject = false, onObject = false;
    // Height §1.2 step 1: a static-object hit withdraws the step-up (and step-down) for the
    // rest of this update.
    bool stepAllowed = true;
    const int64_t halfHeight = fixedT(b.size.z * 0.5f);
    const auto objectHere = [&](int axis, int64_t move) {
        if (!objects)
            return false;
        const glm::dvec3 c = b.blocks(), h = glm::dvec3(b.size) * 0.5;
        return objects(c - h, c + h, axis, (double)move / ONE, b.ignoredStatics);
    };
    for (int s = 0; s < n; s++)
        for (int k = 0; k < 3; k++) {
            const int64_t move = fixedOf(d[k]);
            b.position[k] += move;
            if (!any(cellsOf(b.position, b.size), blocks)) {
                // §4.5: a static object stops this axis too (no step-up onto it); landing on
                // one counts as standing.
                if (move != 0 && objectHere(k, move)) {
                    b.position[k] -= move;
                    touchedObject = true;
                    stepAllowed = false;
                    if (k == 2 && move < 0) {
                        onObject = true;
                        b.velocity.z = 0;
                        b.fallStart = (float)(b.position[2] / ONE);
                    }
                }
                continue;
            }
            if (k == 2) {
                // §4.2: stop vertically. The player's hard landing out of the water hurts
                // (travel §8); the game turns fallHurt into damage, stun and sound.
                b.position[2] -= move;
                if (b.local && b.velocity.z < -2)
                    b.thudded = true;
                const float h = b.fallStart - (float)(b.position[2] / ONE);
                if (b.local && b.velocity.z < -5 && !inWater && h > 20) {
                    b.fallHurt += (h / 20 - 1) * 0.5f;
                    b.mode &= ~CLIMB;
                }
                b.velocity.z = 0;
                b.push.z = 0;
                b.fallStart = (float)(b.position[2] / ONE);
                continue;
            }
            // §4.3: step up onto a ledge, else undo this axis only (velocity kept).
            // Height §1.2 step 8: the snap keeps the drawn height (z - offset) continuous. The
            // object base term is 0 (the engine has no world-object voxel models).
            if (stepAllowed && !b.swimmer && !any(cellsOf(b.position, b.size, {0, 0, 1.01f}), blocks)) {
                const int64_t zBefore = b.position[2];
                const int64_t raised = 65536 * snapLayer(zBefore - halfHeight) + halfHeight + SNAP_LIFT;
                b.bob = floatF(raised - (zBefore - fixedT(b.bob)));
                b.position[2] = raised;
            } else {
                b.position[k] -= move;
            }
        }

    // §5.1: contact flags.
    b.contact = touchedObject ? ON_OBJECT : 0;
    if (onObject)
        b.contact |= ON_GROUND;
    const Cells box = cellsOf(b.position, b.size);
    {
        glm::vec3 moved((float)((b.position[0] - start[0]) / ONE), (float)((b.position[1] - start[1]) / ONE),
                        (float)((b.position[2] - start[2]) / ONE));
        const glm::vec2 vh(b.velocity), ih(b.intent);
        if (glm::dot(vh, vh) > 16 && glm::dot(ih, ih) > 16 && glm::dot(moved, moved) < 0.01f * glm::dot(d, d))
            b.contact |= STUCK;
    }
    int32_t waterTop = INT32_MIN;
    bool wet = false;
    for (int32_t x = box.lo[0]; x <= box.hi[0]; x++)
        for (int32_t y = box.lo[1]; y <= box.hi[1]; y++)
            for (int32_t zc = box.lo[2]; zc <= box.hi[2]; zc++) {
                if (block(x, y, zc) == 2) {
                    wet = true;
                    waterTop = std::max(waterTop, zc + 1);
                } else {
                    b.contact |= NOT_SUBMERGED;
                }
            }
    if (wet) {
        if (!(old & IN_WATER) && b.velocity.z < -3)
            b.splashed = true;
        b.contact |= IN_WATER;
        waterTop = std::max(waterTop, box.lo[2]);
        // §5.2: buoyancy.
        const float r = ((float)waterTop - (float)(b.position[2] / ONE)) / (b.size.z * 0.5f);
        const float lift = r <= 0 ? 0.f : (r < 1 || b.action == 107) ? r + 1 : 1.f; // a boat has no cap
        b.velocity.z += 30.f * dts * lift;
    }
    // §5.4: ground, then the step-down snap.
    if (!wet) {
        Cells g = box;
        g.lo[2] = g.hi[2] = (int32_t)snapLayer(b.position[2] - halfHeight - fixedT(0.2f));
        if (any(g, [&](int32_t x, int32_t y, int32_t zc) { return solid(block(x, y, zc)); }))
            b.contact |= ON_GROUND;
    }
    const glm::vec2 dh(d);
    // Height §1.3: on the terrain grid only, at most once an update, and not after a static hit.
    if ((old & ON_GROUND) && stepAllowed && glm::dot(dh, dh) > 0 && !(b.contact & (ON_GROUND | IN_WATER)) &&
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
    // §6: friction, then the push.
    const float R = glm::dot(b.intent, b.intent) > 0 ? 0.0025f : 0.005f;
    if (b.contact & ON_GROUND)
        b.velocity = relax(b.velocity, glm::vec3(0), R, dt);
    if ((b.contact & (ON_GROUND | IN_WATER)) || (b.mode & GLIDER) || b.pathActive)
        b.push = relax(b.push, glm::vec3(0), R, dt);
    // §5.4: wall probes along the world axes.
    {
        const float m = 0.2f;
        const glm::dvec3 p = b.blocks();
        auto column = [&](int32_t x0, int32_t x1, int32_t y0, int32_t y1) {
            for (int32_t x = x0; x <= x1; x++)
                for (int32_t y = y0; y <= y1; y++)
                    for (int32_t zc = box.lo[2]; zc <= box.hi[2]; zc++)
                        if (solid(block(x, y, zc)))
                            return true;
            return false;
        };
        const int32_t yLo = (int32_t)std::floor(p.y - b.size.y / 2 - m), yHi = (int32_t)std::floor(p.y + b.size.y / 2 + m);
        const int32_t xLo = (int32_t)std::floor(p.x - b.size.x / 2 - m), xHi = (int32_t)std::floor(p.x + b.size.x / 2 + m);
        // (The diagonal corner probes, chosen by facing, are not in yet.)
        const struct {
            bool hit;
            glm::vec3 normal;
        } probes[] = {
            {column(box.lo[0], box.hi[0], yLo, yLo), {0, 1, 0}},
            {column(box.lo[0], box.hi[0], yHi, yHi), {0, -1, 0}},
            {column(xLo, xLo, box.lo[1], box.hi[1]), {1, 0, 0}},
            {column(xHi, xHi, box.lo[1], box.hi[1]), {-1, 0, 0}},
        };
        for (const auto &probe : probes)
            if (probe.hit) {
                b.wallNormal = probe.normal;
                b.contact |= ON_WALL;
                break;
            }
    }
    // §5.3: water drag and the swim pitch; a boat glides with a tenth of the friction
    // sideways (travel §4.2) and sits level.
    if (wet && b.action == 107) {
        const float keep = std::pow(1.f - 0.1f * R, (float)dt);
        b.velocity.x *= keep;
        b.velocity.y *= keep;
        b.velocity.z = relax(b.velocity.z, 0.f, R, dt);
        b.swimPitch = relax(b.swimPitch, 0.f, 0.01f, dt);
    } else if (wet) {
        b.velocity = relax(b.velocity, glm::vec3(0), 0.005f, dt);
        b.swimPitch = relax(b.swimPitch, -60.f, 0.01f, dt);
    } else {
        b.swimPitch = relax(b.swimPitch, 0.f, 0.01f, dt);
    }
    // §3.8: over the top of a wall, the climber is shoved onto the ledge.
    if (b.local && (b.mode & CLIMB) && !gliding(b) && onWall && !(b.contact & ON_WALL) &&
        glm::dot(glm::vec2(b.intent), glm::vec2(b.intent)) > 0) {
        const float yaw = glm::radians(b.yaw);
        if (glm::dot(glm::vec2(-std::sin(yaw), std::cos(yaw)), glm::vec2(oldNormal)) <= 0)
            b.velocity = -5.f * oldNormal;
    }
    // Step 15: dodge time and the landing bob.
    if (b.local && b.dodgeMs > 500 && b.dodgeMs - dt <= 500)
        b.rolled = true;
    b.dodgeMs = std::max(0, b.dodgeMs - dt);
    // Height §1.5: the offset approaches 0 once per millisecond of the step, each operation in
    // binary32; 0.01 when below zero, 0.02 otherwise (NaN included).
    const float bobRate = 0.f > b.bob ? 0.01f : 0.02f;
    for (int32_t elapsed = 0; elapsed < dt; elapsed++)
        b.bob = (0.f - b.bob) * bobRate + b.bob;
}

} // namespace creatures
