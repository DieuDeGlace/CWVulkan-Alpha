// The player's abilities (docs/specs/alpha-creatures-behaviour.md §16).
#include "creatures/Abilities.h"
#include <algorithm>
#include <cmath>

namespace creatures {

int32_t skillOf(int32_t id) {
    switch (id) {
    case 21: case 34: case 48: case 54: case 88: return 6;
    case 50: case 79: case 86: case 103: return 7;
    case 49: case 96: case 97: case 99: case 100: case 101: case 102: return 8;
    default: return -1;
    }
}

float abilityValue(const Combatant &c, int32_t id) {
    const int32_t skill = skillOf(id);
    if (skill < 0)
        return 1;
    int32_t n = c.skills[skill];
    if (n == 0 && c.controller != 0)
        n = c.level / 2 + c.powerTier + 1;
    return 1.f - 1.f / (0.1f * (float)n + 1.f);
}

float manaCost(const Combatant &c, int32_t id) {
    switch (id) {
    case 3: case 4: return 0.1f;
    case 31: case 33: case 37: case 43: case 45: case 46: case 95: return c.hasBuff(9) ? 0.f : 0.3f;
    case 34: return (1.f - 0.75f * abilityValue(c, 34)) * 0.125f;
    default: return 0;
    }
}

int32_t cooldownLength(const Combatant &c, int32_t id) {
    const float v = abilityValue(c, id);
    auto len = [&](float base, float factor) { return (int32_t)(base - factor * v); };
    switch (id) {
    case 21: return len(8000, 8000);
    case 48: return len(20000, 12000);
    case 49: return len(16000, 10000);
    case 50: return len(16000, 10000);
    case 54: return len(20000, 14000);
    case 72: return 15000;
    case 86: return len(60000, 40000);
    case 88: return len(8000, 8000);
    case 96: return len(20000, 14000);
    case 97: return len(60000, 30000);
    case 99: return len(12000, 10000);
    case 100: case 101: case 102: return len(60000, 30000);
    case 103: return len(20000, 10000);
    default: return 0;
    }
}

bool usableNow(const Combatant &c, int32_t id) {
    if (!(c.hitStun < 1 || id == 101))
        return false;
    if (manaCost(c, id) > c.mana)
        return false;
    if (const auto it = c.cooldowns.find(id); it != c.cooldowns.end() && it->second != 0)
        return false;
    return id != 28 || c.mana > 0;
}

bool freeToAct(const Combatant &c) {
    if (c.action == 48)
        return false;
    bool ready = c.actionMs >= duration(c, c.action);
    if (!ready) {
        switch (c.action) {
        case 0: case 1: case 2: case 3: case 4: case 6: case 7: case 9: case 10: case 12: case 13: case 14: case 15:
        case 16: case 18: case 19: case 22: case 26: case 28: case 29: case 31: case 33: case 34: case 37: case 45:
        case 46: case 57: case 58: case 65: case 66: case 67: case 86: case 98: case 101:
            ready = true;
            break;
        default: break;
        }
    }
    return ready && c.charge <= 0;
}

namespace {

// The number of slots the list holds: five for the four classes, two otherwise.
int32_t slotCount(const Combatant &c) { return c.characterClass >= 1 && c.characterClass <= 4 ? 5 : 2; }

bool twoHanded(const Item &main) {
    if (main.type != 3)
        return false;
    switch (main.subtype) {
    case 15: case 16: case 17: case 5: case 10: case 11: case 18: case 8: case 6: case 7: return true;
    default: return false;
    }
}

} // namespace

std::array<int32_t, 6> abilitySlots(const Combatant &c) {
    std::array<int32_t, 6> s{};
    const uint8_t M = c.mainHand().subtype, O = c.offHand().subtype;
    const bool spec1 = c.specialization == 1;
    if (M == 10) {
        s[0] = spec1 ? 32 : 30, s[1] = spec1 ? 33 : 31;
    } else if (M == 12) {
        s[0] = spec1 ? 42 : 40, s[1] = spec1 ? 43 : 37;
    } else if (M == 11) {
        s[0] = spec1 ? 44 : 38, s[1] = spec1 ? 45 : 46;
    } else {
        s[0] = primaryAttack(c);
        if (M == 5)
            s[1] = 5;
        else if (M == 3)
            s[1] = 17;
        else if (M == 4)
            s[1] = 20;
        else if (M == 6 || O == 6)
            s[1] = 25;
        else if (M == 7 || O == 7)
            s[1] = 24;
        else if (M == 8)
            s[1] = 27;
        else if (twoHanded(c.mainHand()))
            s[1] = 59;
        else if (O == 13)
            s[1] = 8;
        else
            s[1] = 63;
    }
    switch (c.characterClass) {
    case 1: s[2] = 54, s[3] = 86, s[4] = c.specialization == 0 ? 102 : 101; break;
    case 2: s[2] = 21, s[3] = 50, s[4] = spec1 ? 100 : 99; break;
    case 3: s[2] = spec1 ? 34 : 88, s[3] = 103, s[4] = 49; break;
    case 4: s[2] = 48, s[3] = 79, s[4] = c.specialization == 0 ? 97 : 96; break;
    default: break;
    }
    // The class abilities need their skill (slot i <-> skill i + 4).
    for (int32_t i = 2; i < slotCount(c); i++)
        if (c.skills[i + 4] <= 0)
            s[i] = 0;
    return s;
}

void AbilityBindings::refresh(const Combatant &c) {
    storage = abilitySlots(c);
    count = slotCount(c);
}

namespace {

void start(Combatant &c, Body &body, int32_t id) {
    c.action = id;
    c.actionMs = 0;
    c.dodgeMs = 0;
    body.dodgeMs = 0;
}

// §16.6: the buff abilities.
void buff(Combatant &c, int32_t id, AbilityEffects &out) {
    if (const auto it = c.cooldowns.find(id); it != c.cooldowns.end() && it->second > 0)
        return;
    const float v = abilityValue(c, id);
    Buff b;
    switch (id) {
    case 97: b = {3, 0, (int32_t)(12000.f * v + 8000.f)}; break;
    case 100: b = {12, 1 + v, 10000}; break;
    case 101:
        b = {1, 0.75f - 0.3f * v, 10000};
        break;
    case 102: b = {2, 0, 10000}; break;
    case 103: {
        // q is the stat 0x446e80, not traced (§16.10); the player's base power stands in.
        const float q = basePower(c);
        b = {6, 2 * (1 + v) * q, 30000};
        break;
    }
    default: return;
    }
    applyBuff(c, b);
    if (id == 101 && c.hitStun > 0)
        c.hitStun = 0;
    out.buffs.push_back({c.id, c.id, b});
    c.cooldowns[id] = (float)cooldownLength(c, id);
}

void press(Combatant &c, Body &body, int32_t id, glm::vec3 aim, AbilityEffects &out) {
    switch (id) {
    case 3: case 4: case 23: case 31: case 33: case 37: case 45: case 46: case 65: case 66: case 94: case 95:
        if (startSecondaryAttack(c))
            body.dodgeMs = (int32_t)c.dodgeMs;
        break;
    case 5: case 17: case 20:
        if (c.mana > 0)
            start(c, body, id);
        break;
    case 8: case 24: case 25: case 27: case 59: case 63: case 64: case 10:
        if (c.action != id)
            start(c, body, id);
        break;
    case 11: start(c, body, id); break;
    case 28:
        if (c.mana > 0 && c.action != 28)
            c.action = 28, c.actionMs = 0;
        break;
    case 34:
        if (c.actionMs >= duration(c, c.action))
            c.action = 34, c.actionMs = 0; // aimed at the player itself (field 36)
        break;
    case 48: {
        start(c, body, 48);
        glm::vec3 v = aim;
        if (glm::dot(v, v) > 0) {
            v = 50.f * glm::normalize(v);
            v.z = std::min(v.z, 5.f);
        }
        body.velocity = v;
        break;
    }
    case 50: {
        glm::vec3 push(-aim.x, -aim.y, 0);
        if (glm::dot(push, push) > 0) {
            const float v = abilityValue(c, 50);
            push = (15.f * v + 20.f) * glm::normalize(push);
            push.z = 12.f * v + 5.f;
        }
        body.push = push;
        c.cooldowns[50] = (float)cooldownLength(c, 50);
        break;
    }
    case 54: {
        start(c, body, 54);
        glm::vec3 v = 2.f * aim;
        if (glm::dot(v, v) > 2500)
            v = 50.f * glm::normalize(v);
        v.z = 15;
        body.velocity = v;
        break;
    }
    case 96: {
        start(c, body, 96);
        glm::vec3 v(-aim.x, -aim.y, 0);
        if (glm::dot(v, v) > 0) {
            v = 30.f * glm::normalize(v);
            v.z = 20;
        }
        body.velocity = v;
        c.dodgeMs = 600, body.dodgeMs = 600;
        break;
    }
    case 97: case 100: case 101: case 102: case 103:
        buff(c, id, out);
        if (id == 101)
            out.sounds.push_back(0x5d);
        if (id == 103)
            out.sounds.push_back(0x5c);
        break;
    case 99:
        body.mode |= 0x400;
        c.cooldowns[99] = (float)cooldownLength(c, 99);
        break;
    default:
        if (c.mana >= manaCost(c, id))
            start(c, body, uint8_t(id));
        break;
    }
}

void release(Combatant &c, int32_t id) {
    auto become = [&](int32_t action, int32_t ms) { c.action = action, c.actionMs = ms; };
    switch (id) {
    case 8: become(104, 0); break;
    case 24: become(23, 0); break;
    case 25: become(55, 0); break;
    case 27: become(26, 0); break;
    case 36: become(37, 0); break;
    case 35: become(0, 0); break;
    case 59:
        if (c.specialization == 1)
            become(61, 0);
        else
            become(60, phases(c, 60).windUp);
        break;
    case 63: become(11, phases(c, 11).windUp); break;
    case 64: become(16, phases(c, 16).windUp); break;
    case 10:
        if (c.actionMs > duration(c, 10))
            become(3, duration(c, 3));
        break;
    default: break;
    }
}

} // namespace

AbilityEffects useAbilities(Combatant &c, Body &body, const std::array<bool, 6> &held, glm::vec3 aim) {
    AbilityBindings bindings;
    bindings.refresh(c);
    return useBoundAbilities(c, body, bindings.values(), held, aim);
}

AbilityEffects useBoundAbilities(Combatant &c, Body &body, std::span<const int32_t> slots,
                                const std::array<bool, 6> &held, glm::vec3 aim) {
    AbilityEffects out;
    const size_t count = std::min(slots.size(), held.size());
    bool used = false;
    for (size_t i = 1; i < count; i++) {
        const int32_t id = slots[i];
        if (id == 0)
            continue;
        if (held[i]) {
            used = true;
            const bool ready = freeToAct(c);
            if (i == 1 && !ready)
                continue;
            if (!usableNow(c, id))
                continue;
            press(c, body, id, aim, out);
        } else if (c.action == id) {
            release(c, id);
        }
    }
    if (!used && c.action != 48 && held[0])
        startPrimaryAttack(c); // the chaining rule; no mana, cooldown or free-to-act test
    if (count > 1 && !held[1] && slots[1] == 28 && c.action == 28)
        c.action = 0;
    if (count > 0 && !held[0] && slots[0] == 29 && c.action == 29)
        c.action = 0;
    if (used)
        body.mode &= ~1u;
    return out;
}

} // namespace creatures
