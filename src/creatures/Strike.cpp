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

bool strikeWindow(Combatant &c, float t, float dtMs) {
    const Phases p = phases(c, c.action);
    const float P1 = (float)p.windUp, P2 = (float)p.active;
    if (t <= P1)
        c.struck.clear();
    float from, to;
    if (const Pulse pulse = pulseOf(p, c.action); pulse.pulsing && pulse.length > 0) {
        const float T = (float)pulse.length;
        const bool running = P1 <= t && t < P1 + P2;
        from = to = P1;
        if (running) {
            from = P1 + std::trunc((t - P1) / T) * T;
            to = from + 100;
        }
        const float next = P1 + std::trunc((t + dtMs - P1) / T) * T;
        if (running && next != from && next < P1 + P2)
            c.struck.clear();
    } else if (c.action == 93) {
        from = P1 + (float)(p.active / 5);
        to = P1 + P2;
    } else {
        const float middle = P1 + (float)(p.active / 2);
        from = c.action == 68 || c.action == 69 ? P1 + (float)(p.active / 3) : std::max(P1, middle - 100);
        to = middle >= P1 + P2 ? P1 + P2 - 1 : middle;
    }
    return from < t && t <= to;
}

bool reachesTarget(const Combatant &a, glm::dvec3 position, float yaw, glm::vec3 box, glm::dvec3 target,
                   glm::vec3 targetBox, glm::dvec3 *sightFrom) {
    float r = 1.5f * box.x, f = 1.5f;
    switch (a.action) {
    case 1: case 2: case 3: case 4: case 6: case 7: case 9: case 13: case 14: case 18: case 19:
        r = 2.f * box.x, f = 1;
        break;
    case 20: case 21: r *= 2; break;
    case 68: case 70: f = 0; break;
    case 86: case 93: case 104: r *= 2, f = 0; break;
    case 87: case 88: r = 8, f = 0; break;
    case 30: case 32: r = 3, f = 0; break;
    case 31: case 33: r = 5, f = 0; break;
    case 72: case 76: case 77: case 78: r *= 1.2f, f = 0; break;
    case 74: case 91: r = 20, f = 0; break;
    case 11: case 54: case 61: r *= 3; break;
    default: break;
    }
    const float turn = yaw * PI / 180.f;
    glm::dvec3 origin = position + glm::dvec3(-std::sin(turn) * box.x * f, std::cos(turn) * box.x * f, 0);
    glm::dvec3 sightPoint = position;
    if (a.action >= 30 && a.action <= 33) {
        origin = position + glm::dvec3(a.aim);
        sightPoint = origin + glm::dvec3(0, 0, r);
    }
    if (a.action == 69 || a.action == 75)
        origin.z += 0.5f * (box.z - box.x);
    if (a.action == 11 || a.action == 54 || a.action == 61)
        origin = position;
    if (sightFrom)
        *sightFrom = sightPoint;
    const glm::dvec3 d = origin - target;
    const double reach = 0.7 * targetBox.x + r;
    return d.x * d.x + d.y * d.y < reach * reach && std::abs(d.z) < 0.5 * targetBox.z + box.z + r;
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
    HitInput in;
    in.critical = critical;
    in.amount = 1.5f * magicBase(a) * (1.f + 0.05f * ((float)draw() / 32767.f));
    if (in.critical)
        in.amount *= 2;
    in.magic = true;
    in.area = a.action == 95;
    in.power = a.action == 95 ? 0.1f : 0.f;
    return in;
}

void afterFirstTarget(Combatant &a, bool area, const std::function<int()> &draw) {
    if (a.hitStun <= 0)
        a.combo += 1;
    if (area && a.characterClass == 4 && a.specialization == 0)
        a.stealth = std::min(1.f, a.stealth + a.spentPower);
    if (area || a.action == 30 || a.action == 32)
        return;
    float gain = 0.1f + 0.05f * (1.f - 2.f * ((float)draw() / 32767.f));
    gain *= (float)duration(a, a.action) / 500.f;
    if (draw() % 8 == 0)
        gain *= 1.5f;
    a.mana = std::min(1.f, a.mana + gain);
}

} // namespace creatures
