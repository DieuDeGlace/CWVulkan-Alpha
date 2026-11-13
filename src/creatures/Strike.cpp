#include "creatures/Strike.h"
#include "creatures/Abilities.h"
#include "creatures/Physics.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace creatures {

namespace {

constexpr float PI = 3.14159265358979f;

// The bases share everything but the action's own factor.
bool bowLike(const Item &item) { return item.type == 3 && (item.subtype == BOW || item.subtype == CROSSBOW); }

// §2.4: the weapons a blow counts; the second is empty when one is enough.
void weaponsOf(const Combatant &c, const Item *(&first), const Item *(&second)) {
    first = second = nullptr;
    const Item &main = c.mainHand(), &off = c.offHand();
    const Item *mainWeapon = main.type == 3 ? &main : nullptr, *offWeapon = off.type == 3 ? &off : nullptr;
    if (bowLike(main)) {
        first = &main;
        return;
    }
    if (bowLike(off)) {
        first = &off;
        return;
    }
    switch (c.action) {
    case 2: case 4: case 7: case 8: case 10: case 18: case 39: case 41:
        first = offWeapon;
        return;
    case 11: case 12: case 16: case 17: case 20: case 54: case 86: case 91: case 96:
        first = offWeapon, second = mainWeapon;
        return;
    case 40: case 42:
        first = mainWeapon;
        return;
    default:
        first = mainWeapon;
        if (offWeapon && off.subtype == 12)
            second = offWeapon;
        return;
    }
}

// §2.2: the action's own share of the base.
float shareOf(int32_t action) {
    switch (action) {
    case 5: case 26: return 0.5f;
    case 21: case 72: return 0.1f;
    case 30: case 32: return 0.4f;
    case 31: case 33: return 0.6f;
    case 37: case 43: case 57: case 58: case 68: return 2.f;
    case 55: return 0.25f;
    default: return 1.f;
    }
}

// §3.1: the strength of the push.
float pushOf(const Combatant &c) {
    if (c.controller == 3 || c.controller == 5)
        return 0;
    const float S = attackSpeed(c);
    switch (c.action) {
    case 30: case 31: case 32: case 33: return 0;
    case 1: case 2: case 9: case 13: case 14: case 15: case 60: case 61: return 6.f / S;
    case 3: case 4: case 6: case 7: case 18: case 19: case 62: return 3.f / S;
    case 65: case 66: return 10.f / S;
    case 5: return 4;
    case 11: case 87: return 10;
    case 17: case 20: return 6;
    case 54: return 5;
    case 21: case 88: return 30.f * abilityValue(c, 21) + 5.f;
    default: return 2.f / S;
    }
}

struct Pulse {
    bool pulsing = false;
    int32_t length = 0;
};

// §1.3: the actions that strike again and again.
Pulse pulseOf(const Phases &p, int32_t action) {
    switch (action) {
    case 72: case 86: return {true, 250};
    case 91: return {true, 1000};
    case 5: case 11: return {true, p.active / 3};
    case 30: case 32: return {true, p.active / 6};
    case 31: case 33: return {true, p.active / 12};
    default: return {};
    }
}

} // namespace

bool bigCreature(int32_t type) {
    switch (type) {
    case 101: case 107: case 108: case 109: case 114: case 115: case 116: case 117: case 118: case 119:
        return true;
    default:
        return false;
    }
}

bool meleeAction(int32_t a) {
    if ((a >= 1 && a <= 7) || (a >= 9 && a <= 21) || (a >= 30 && a <= 33))
        return true;
    switch (a) {
    case 51: case 54: case 57: case 58: case 60: case 61: case 62: case 65: case 66: case 67: case 68: case 69:
    case 70: case 72: case 74: case 75: case 76: case 77: case 78: case 86: case 87: case 88: case 91:
    case 93: case 104:
        return true;
    default:
        return false;
    }
}

void strikingWeapons(const Combatant &c, const Item *(&first), const Item *(&second)) { weaponsOf(c, first, second); }

float magicBase(const Combatant &c) {
    const float P = levelPower(c.level);
    const float rank = (float)std::pow(2.0, 0.0);
    float base = c.multipliers[2] * (rank * P);
    const Item *first, *second;
    weaponsOf(c, first, second);
    const auto &worth = itemBonuses().damage;
    if (first || second) {
        if (worth && first)
            base = worth(*first) + base;
        if (worth && second)
            base = worth(*second) + base;
        return base;
    }
    // No weapon for this blow: claws and jaws. These terms take the tier, not the multiplier.
    const float natural = P * (float)std::pow(2.0, 0.25 * c.powerTier);
    if (c.appearanceFlags & 0x08)
        base += natural * 2.f;
    if (c.appearanceFlags & 0x10)
        base += natural * 8.f;
    return base;
}

float strikeBase(const Combatant &c) {
    const float base = magicBase(c);
    switch (c.action) {
    case 87: return base * 50.f;
    case 91: return base * 2.f;
    case 93: return base * 5.f;
    case 96: return base * 3.f;
    default: return base;
    }
}

StrikeWindow strikeWindow(Combatant &c, float tMs, float dtMs) {
    // Strike-call-sites contract S5-S9 (integer clock arithmetic, truncating division).
    const Phases p = phases(c, c.action);
    const int32_t B = p.windUp, C = p.active, t = int32_t(tMs), dt = int32_t(dtMs);
    StrikeWindow out;
    // S5: the window, with the variants of actions 0x44/0x45 and 0x5D.
    int32_t start = std::max(C / 2 - 100 + B, B), end = C / 2 + B;
    if (end >= B + C)
        end = B + C - 1;
    if (c.action == 0x44 || c.action == 0x45)
        start = C / 3 + B;
    if (c.action == 0x5D)
        start = C / 5 + B, end = B + C;
    // S6: the periodic actions strike once per interval, in a 100-unit window.
    if (const Pulse pulse = pulseOf(p, c.action); pulse.pulsing) {
        const int32_t I = pulse.length;
        start = end = B;
        // A zero interval (division by zero) is outside the contract; the engine never strikes.
        if (B <= t && t < B + C && I > 0) {
            start = B + ((t - B) / I) * I;
            end = start + 100;
            const int32_t next = B + ((t + dt - B) / I) * I;
            if (next != start && next < B + C) {
                out.areaHeal = true;
                c.struck.clear();
                c.deferred.clear();
            }
            if (t + dt > end)
                out.areaHeal = true;
        }
    }
    // S7: the wind-up runs the deferred-key pass and clears both sets.
    if (t <= B) {
        deferredPass(c);
        c.deferred.clear();
        c.struck.clear();
        c.hitLanded = false;
    }
    // S9: at the window's end the deferred-key pass runs and only the deferred set is cleared.
    if (t >= end) {
        deferredPass(c);
        c.deferred.clear();
    }
    out.strikes = start < t && t <= end;
    return out;
}

StrikeGeometry strikeGeometry(const Combatant &a, const Body &body, float yaw) {
    // Strike-call-sites contract K2.
    const float x = body.size.x;
    float R = x * 1.5f, F = 1.5f;
    switch (a.action) {
    case 0x0D: case 0x0E: case 0x02: case 0x01: case 0x09: case 0x04: case 0x03: case 0x07: case 0x06:
    case 0x12: case 0x13:
        R = x * 2.f, F = 1.f;
        break;
    default: break;
    }
    switch (a.action) {
    case 0x14: case 0x15: R *= 2.f; break;
    case 0x44: case 0x46: F = 0; break;
    case 0x56: case 0x5D: case 0x68: R *= 2.f, F = 0; break;
    case 0x57: case 0x58: R = 8, F = 0; break;
    case 0x1E: case 0x20: R = 3, F = 0; break;
    case 0x1F: case 0x21: R = 5, F = 0; break;
    case 0x5B: case 0x4A: R = 20, F = 0; break;
    case 0x48: case 0x4E: case 0x4D: case 0x4C: R *= 1.2f, F = 0; break;
    default: break;
    }
    const auto fixed = [](float v) { return int64_t(v * 65536.f); };
    const auto add = [](int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b)); };
    // Step 4: the rotation of (0, d, 0) about the vertical axis by +0x30 degrees.
    const float d = x * F;
    const float angle = yaw * 0.0174532924f;
    const float cosine = float(std::cos(double(angle))), sine = float(std::sin(double(angle)));
    StrikeGeometry g;
    g.reach = R;
    const float offset[3] = {-sine * d, cosine * d, 0.f};
    for (size_t k = 0; k < 3; k++)
        g.centre[k] = add(body.position[k], fixed(offset[k]));
    g.start = {body.position[0], body.position[1], body.position[2]};
    if (a.action >= 0x1E && a.action <= 0x21) { // step 5, and the geometric test's start point
        for (size_t k = 0; k < 3; k++)
            g.centre[k] = add(body.position[k], fixed(a.aim[int(k)]));
        g.start = g.centre;
        g.start[2] = add(g.start[2], fixed(R));
    }
    if (a.action == 0x4B || a.action == 0x45) // step 6: subtracted from the centre's z
        g.centre[2] = std::bit_cast<int64_t>(uint64_t(g.centre[2]) - uint64_t(fixed((body.size.z - x) * 0.5f)));
    if (a.action == 0x0B || a.action == 0x3D || a.action == 0x36) { // step 7
        g.centre = {body.position[0], body.position[1], body.position[2]};
        g.reach = R * 3.f;
    }
    return g;
}

bool strikeReaches(const StrikeGeometry &g, const Body &striker, const Body &target) {
    // K5 steps 2 and 3: the fixed-point horizontal reach and the binary32 vertical test.
    const float r = target.size.x * 0.7f + g.reach;
    int64_t horizontal = 0;
    for (size_t k = 0; k < 2; k++) {
        const uint64_t d = uint64_t(g.centre[k]) - uint64_t(target.position[k]);
        horizontal = std::bit_cast<int64_t>(uint64_t(horizontal) + uint64_t(std::bit_cast<int64_t>(d * d) / 65536));
    }
    if (!(horizontal < int64_t((r * r) * 65536.f)))
        return false;
    const float dz = float(std::bit_cast<int64_t>(uint64_t(g.centre[2]) - uint64_t(target.position[2]))) *
                     (1.f / 65536.f);
    return ((target.size.z * 0.5f) + striker.size.z) + g.reach > float(std::fabs(double(dz)));
}

bool reachesTarget(const Combatant &a, glm::dvec3 position, float yaw, glm::vec3 box, glm::dvec3 target,
                   glm::vec3 targetBox, glm::dvec3 *sightFrom) {
    Body striker, struck;
    for (int k = 0; k < 3; k++) {
        striker.position[k] = int64_t(position[k] * 65536.0);
        struck.position[k] = int64_t(target[k] * 65536.0);
    }
    striker.size = box;
    struck.size = targetBox;
    const StrikeGeometry g = strikeGeometry(a, striker, yaw);
    if (sightFrom)
        *sightFrom = glm::dvec3(double(g.start[0]), double(g.start[1]), double(g.start[2])) / 65536.0;
    return strikeReaches(g, striker, struck);
}

bool procChance(const Combatant &source, float probability, const std::function<int()> &draw) {
    // Roll Cw (0x0441A0 / 0x00F220): scaled by the main-hand weapon's subtype.
    if (source.mainHand().type == 3)
        switch (source.mainHand().subtype) {
        case 0: case 1: case 2: case BRACELET: probability *= .5f; break;
        case BOOMERANG: case STAFF: probability *= .3f; break;
        default: break;
        }
    return (float)draw() / 32767.f < probability;
}

namespace {

HitInput commonHitInput(const Combatant &a, glm::vec3 displacement, glm::dvec3 position, glm::vec3 box,
                        glm::dvec3 target, glm::vec3 targetBox, const std::function<int()> &draw) {
    HitInput in;
    in.critical = a.criticalLatched || (float)draw() / 32767.f < criticalChance(a);
    displacement.z = 0;
    const float squared = (displacement.x * displacement.x + displacement.y * displacement.y) +
                          displacement.z * displacement.z;
    if (squared > .01f) {
        const float root = (float)std::sqrt((double)squared);
        displacement *= 1.f / root;
    }
    displacement.z = .25f;
    const float sizeRatio = box.x / targetBox.x;
    displacement *= (float)std::sqrt((double)sizeRatio);
    displacement *= pushOf(a);
    in.knockBack = displacement;

    const float s = a.spentPower;
    const bool magicStat = (a.action >= 30 && a.action <= 33) || a.action == 87 || a.action == 88;
    float prepared = shareOf(a.action) * (magicStat ? magicBase(a) : strikeBase(a));
    if (!magicStat) {
        const float baseline = a.action == 60 || a.action == 11 || a.action == 62 ? 0.f : 1.f;
        const float squaredCharge = s * s;
        const float chargeFactor = squaredCharge * 5.f + baseline;
        prepared *= chargeFactor;
    }
    const float scaledDraw = (float)draw() * .5f;
    const float variation = 1.25f - scaledDraw / 32767.f;
    in.amount = magicStat ? (variation * prepared) * (0.f + 1.f) : (variation + 0.f) * prepared;
    if (in.critical)
        in.amount *= 2;
    in.area = s > 0 || a.action == 11;
    in.selector = a.action;
    in.power = s;
    in.magic = a.action >= 30 && a.action <= 33;
    in.targetPosition = target;
    in.attackerPosition = position;
    return in;
}

} // namespace

HitInput strikeInput(const Combatant &a, glm::dvec3 position, glm::vec3 box, glm::dvec3 target, glm::vec3 targetBox,
                     const std::function<int()> &draw) {
    auto hit = commonHitInput(a, glm::vec3(target - position), position, box, target, targetBox, draw);
    for (int axis = 0; axis < 3; axis++) {
        hit.targetRecordPosition[axis] = (int64_t)(target[axis] * 65536.0);
        hit.attackerRecordPosition[axis] = (int64_t)(position[axis] * 65536.0);
    }
    return hit;
}

HitInput strikeInput(const Combatant &a, const Body &body, const Body &target, const std::function<int()> &draw) {
    glm::vec3 displacement;
    for (int axis = 0; axis < 3; axis++) {
        // Subtract the fixed-point bit patterns before converting to binary32;
        // subtracting two rounded world-space positions loses nearby offsets.
        const uint64_t difference = (uint64_t)target.position[axis] - (uint64_t)body.position[axis];
        displacement[axis] = (float)std::bit_cast<int64_t>(difference);
        displacement[axis] *= 1.f / 65536.f;
    }
    auto hit = commonHitInput(a, displacement, body.blocks(), body.size, target.blocks(), target.size, draw);
    std::copy_n(target.position, hit.targetRecordPosition.size(), hit.targetRecordPosition.begin());
    std::copy_n(body.position, hit.attackerRecordPosition.size(), hit.attackerRecordPosition.begin());
    return hit;
}

HitInput beamInput(const Combatant &a, const std::function<int()> &draw) {
    const bool critical = (float)draw() / 32767.f < criticalChance(a);
    return beamInput(a, critical, draw);
}

HitInput beamInput(const Combatant &a, bool critical, const std::function<int()> &draw) {
    // Strike-call-sites B4 steps 6-7: m = scaled unit draw (0.05) + 1, S4 = (heal power x 1.5) x m.
    HitInput in;
    in.critical = critical;
    const float m = ((float)draw() * 0.05f) / 32767.f + 1.f;
    in.amount = (magicBase(a) * 1.5f) * m;
    if (in.critical)
        in.amount *= 2;
    in.magic = true;
    in.area = a.action == 95;
    in.power = a.action == 95 ? 0.1f : 0.f;
    return in;
}

StrikeAftermath afterStrike(Combatant &a, bool first, bool &hitAny, const std::function<int()> &draw) {
    // Strike-call-sites §5, only when the attack routine returned 1.
    StrikeAftermath out;
    // 1. Action 0x1E: Cw(0.25); class pair (_, 0) gains a kind-9 effect and record 47.
    if (a.action == 0x1E && procChance(a, .25f, draw) && a.specialization == 0) {
        const Buff record{9, 0, 30000, 0};
        applyBuff(a, record);
        out.buff = BuffEvent{a.id, a.id, record};
        out.upgradeSound = true;
    }
    // 2. The swing count, when this was the swing's first target.
    if (first) {
        a.combo += 1;
        a.comboElapsedMs = 0;
    }
    // 3. The crit charge (+0x1190) of class pair (4, 0).
    const bool B3 = a.spentPower > 0.f || a.action == 0x0B;
    if (B3 && a.characterClass == 4 && a.specialization == 0) {
        a.stealth = a.stealth + a.spentPower;
        if (a.stealth > 1.f)
            a.stealth = 1.f;
    }
    // 4. Mana on the frame's first success, unless charged or a heal action.
    if (!hitAny && !B3 && a.action != 0x1E && a.action != 0x20) {
        float g = (1.f - ((float)draw() * 2.f) / 32767.f) * 0.05f + 0.1f;
        g = ((float)duration(a, a.action) / 500.f) * g;
        if (draw() % 8 == 0)
            g *= 1.5f;
        a.mana = a.mana + g;
        if (a.mana > 1.f)
            a.mana = 1.f;
    }
    // 5.
    hitAny = true;
    return out;
}

HitRecord areaHealRecord(const Combatant &a, uint64_t target, const std::array<int64_t, 3> &position,
                         const std::function<int()> &draw) {
    // Strike-call-sites §6 steps 2-5: the roll, the heal power, then the critical query's draw.
    const float roll = 1.25f - ((float)draw() * 0.5f) / 32767.f;
    HitRecord heal;
    heal.attacker = a.id;
    heal.target = target;
    heal.amount = ((magicBase(a) * roll) * (0.f + 1.f)) * -0.1f;
    heal.critical = (float)draw() / 32767.f < criticalChance(a);
    heal.position = position;
    return heal;
}

} // namespace creatures
