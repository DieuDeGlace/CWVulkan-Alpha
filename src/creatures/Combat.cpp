#include "creatures/Strike.h"
#include "creatures/Combat.h"
#include "creatures/Abilities.h"
#include "creatures/Physics.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace creatures {

float Combatant::buffSum(uint8_t kind) const {
    float s = 0;
    for (const Buff &b : buffs)
        if (b.kind == kind)
            s += b.value;
    return s;
}

bool Combatant::hasBuff(uint8_t kind) const {
    return std::any_of(buffs.begin(), buffs.end(), [&](const Buff &b) { return b.kind == kind; });
}

Item Item::parse(const uint8_t *r) {
    Item item;
    item.type = r[0];
    item.subtype = r[1];
    std::memcpy(&item.seed, r + 0x04, 4);
    item.rarity = r[0x0c];
    item.material = r[0x0d];
    int16_t level;
    std::memcpy(&level, r + 0x10, 2);
    item.level = level;
    std::memcpy(&item.spirits, r + 0x114, 4);
    // Stats consume the raw signed count; only decoding the fixed record's
    // embedded payload needs a bound.
    const int32_t storedCubes = std::clamp(item.spirits, 0, 32);
    for (int32_t k = 0; k < storedCubes; k++) {
        const uint8_t *e = r + 0x14 + 8 * k;
        int32_t level;
        std::memcpy(&level, e + 4, 4);
        item.cubes.push_back({(int8_t)e[0], (int8_t)e[1], (int8_t)e[2], e[3], level});
    }
    return item;
}

ItemBonuses &itemBonuses() {
    static ItemBonuses bonuses{itemHealth, itemDamage, itemArmour, itemResistance, itemSpeed, itemCritical};
    return bonuses;
}

// §10: item stats.

float itemLevelFactor(float v, int rarity) {
    const float g = 3.f * (1.f - 1.f / ((v - 1.f) * 0.05f + 1.f));
    return (float)std::pow(2.0, (double)g) * (float)std::pow(2.0, 0.25 * rarity);
}

namespace {

float effectiveLevel(const Item &item) { return (float)item.level + 0.1f * (float)item.spirits; }
float normalizedPower(float level, int32_t rank) {
    const float exponent = (1.f - 1.f / ((level - 1.f) * .05f + 1.f)) * 3.f;
    const float levelFactor = (float)std::pow(2.0, (double)exponent);
    const float rankExponent = (float)rank * .25f;
    const float rankFactor = (float)std::pow(2.0, (double)rankExponent);
    const float product = levelFactor * rankFactor;
    const float normalization = (float)std::pow(2.0, 3.0);
    return product / normalization;
}
float armourShare(const Item &item) { return item.type == 4 ? 1.f : 0.5f; } // chests count whole
bool armourPiece(const Item &item) { return item.type >= 4 && item.type <= 7; }

// §10.2: weapons held in both hands or shot (the set of 0x444230).
bool heavyWeapon(const Item &item) {
    if (item.type != 3)
        return false;
    switch (item.subtype) {
    case 5: case 6: case 7: case 8: case 10: case 11: case 15: case 16: case 17: case 18:
        return true;
    default:
        return false;
    }
}

} // namespace

float itemHealth(const Item &item) {
    if (item.type < 3 || item.type > 7)
        return 0;
    const uint32_t u1 = (item.seed * 8u) % 21u;
    float m = (1.f - (float)u1 / 20.f) + 1.f;
    if (item.material == 1)
        m += 1;
    else if (item.material == 26)
        m += 0.5f;
    else if (item.material == 27)
        m += 0.75f;
    float health = itemLevelFactor(effectiveLevel(item), item.rarity) * 5.f;
    health *= armourShare(item);
    return health * m;
}

float itemDamage(const Item &item) {
    if (item.type != 3)
        return 0;
    float k = 4;
    switch (item.subtype) {
    case 3: case 4: case 13: k = 2; break;
    case 6: case 7: case 8: case 10: case 11: case 15: case 16: case 17: case 18: k = 8; break;
    }
    return itemLevelFactor(effectiveLevel(item), item.rarity) * k;
}

float itemArmour(const Item &item) {
    if (!armourPiece(item))
        return 0;
    float a = 1;
    switch (item.material) {
    case 18: a = 0.8f; break;
    case 19: case 26: case 27: a = 0.85f; break;
    case 23: case 25: a = 0.75f; break;
    }
    return armourShare(item) * a * itemLevelFactor(effectiveLevel(item), item.rarity);
}

float itemResistance(const Item &item) {
    if (!armourPiece(item))
        return 0;
    float a = 1;
    switch (item.material) {
    case 1: case 19: a = 0.85f; break;
    case 26: case 27: a = 0.75f; break;
    }
    return armourShare(item) * a * itemLevelFactor(effectiveLevel(item), item.rarity);
}

float itemSpeed(const Item &item) {
    if (item.type < 3 || item.type > 9)
        return 0;
    const float k = heavyWeapon(item) || item.type == 4 ? 0.2f : 0.1f;
    float q = (float)(item.seed % 21u) / 20.f;
    if (item.material == 12)
        q += 1;
    const float s = (normalizedPower((float)item.level, item.rarity) * k) * q;
    return s < 0.001f ? 0.f : s;
}

float itemCritical(const Item &item) {
    if (item.type < 3 || item.type > 9)
        return 0;
    const float k = heavyWeapon(item) || item.type == 4 ? 0.1f : 0.05f;
    float q = 1.f - (float)(item.seed % 21u) / 20.f;
    if (item.material == 11)
        q += 1;
    const float c = (normalizedPower((float)item.level, item.rarity) * k) * q;
    return c < 0.001f ? 0.f : c;
}

namespace {

// The item type each equipment slot takes (§4.1); others in it add nothing.
bool fits(int slot, const Item &item) {
    static const uint8_t TYPE_OF_SLOT[SLOT_COUNT] = {0, 8, 4, 6, 5, 7, 3, 3, 9, 9, 24, 23, 0};
    return item.type != 0 && item.type == TYPE_OF_SLOT[slot];
}

float bonus(const std::function<float(const Item &)> &f, const Combatant &c, std::initializer_list<int> slots) {
    if (!f)
        return 0;
    float s = 0;
    for (int slot : slots)
        if (fits(slot, c.equipment[slot]))
            s += f(c.equipment[slot]);
    return s;
}

float equipmentRate(const Combatant &c, const std::function<float(const Item &)> &contribution) {
    float total = normalizedPower((float)c.level, 0) * .1f;
    if (contribution)
        for (int slot : {OFF_HAND, MAIN_HAND, CHEST_SLOT, BOOTS_SLOT, GLOVES_SLOT, SHOULDERS_SLOT,
                         AMULET_SLOT, RING_LEFT, RING_RIGHT})
            if (fits(slot, c.equipment[slot]))
                total = contribution(c.equipment[slot]) + total;
    return total;
}

// The subtype of the weapon in a hand, or -1 when the hand holds no weapon.
int weaponIn(const Item &item) { return item.type == 3 ? item.subtype : -1; }

// 0x444230: weapons held in both hands or shot.
bool twoHandedOrRanged(int m) {
    switch (m) {
    case GREATSWORD: case GREATAXE: case GREATMACE: case LONGSWORD: case STAFF: case WAND:
    case PITCHFORK: case BOOMERANG: case BOW: case CROSSBOW:
        return true;
    default:
        return false;
    }
}

float rho(const Combatant &c) { return std::min((float)c.combo / comboMax(c), 1.f); }
// These verified callers repeat the counter/helper reads in the uncapped branch.
float queriedComboRatio(const Combatant &c) {
    const float first = (float)c.combo / comboMax(c);
    return first > 1.f ? 1.f : (float)c.combo / comboMax(c);
}

bool isBlocking(const Combatant &c) {
    return c.actionLockMs <= 0 &&
        (c.action == 71 || c.action == 72 || c.action == 8 || c.action == 98 ||
         ((c.action == 59 || c.action == 63) && c.characterClass == 1 && c.specialization == 1));
}

std::array<const Item *, 2> spiritItems(const Combatant &source) {
    const Item &main = source.mainHand(), &off = source.offHand();
    const auto priority = [](const Item &item) {
        return item.type == 3 && (item.subtype == 6 || item.subtype == 7);
    };
    if (priority(main)) return {&main, nullptr};
    if (priority(off)) return {&off, nullptr};
    std::array<const Item *, 2> items{};
    size_t count = 0;
    const auto add = [&](const Item &item) {
        if (item.type == 3) items[count++] = &item;
    };
    switch (source.action) {
    case 2: case 4: case 7: case 8: case 10: case 18: case 39: case 41:
        add(off);
        break;
    case 40: case 42:
        add(main);
        break;
    case 11: case 12: case 16: case 17: case 20: case 54: case 86: case 91: case 96:
        add(off);
        add(main);
        break;
    default:
        add(main);
        if (off.subtype == 12) add(off);
        break;
    }
    return items;
}

void appendItemEffects(const Combatant &source, const HitInput &input, float ratio, HitOutcome &out) {
    const glm::vec3 supplied = input.knockBack;
    const float squared = (supplied.y * supplied.y + supplied.x * supplied.x) + supplied.z * supplied.z;
    glm::vec2 direction(supplied);
    if (squared > 0) {
        const float length = (float)std::sqrt((double)squared);
        const float reciprocal = 1.f / length;
        direction *= reciprocal;
    }
    const glm::vec3 prepared(direction.x * 4.f, direction.y * 4.f, 5.f);
    out.sweep = ratio;
    out.sweepDirection = direction;
    for (const Item *item : spiritItems(source))
        if (item) {
            // Owned decoding bounds the storage; malformed native extents have
            // no safe-memory equivalence claim. Preserve the signed count gate.
            for (int32_t index = 0; index < item->spirits && index < (int32_t)item->cubes.size(); index++) {
                const uint8_t material = item->cubes[index].material;
                if (material < 128)
                    continue;
                Burst burst;
                std::copy(input.targetRecordPosition.begin(), input.targetRecordPosition.end(), burst.position);
                burst.velocity = prepared;
                burst.count = (int32_t)(ratio * 3.f + 1.f);
                const float strength = ratio * .5f;
                if (material <= 131) {
                    const float factor = strength * .5f + 1.f;
                    const float alpha = (1.f + strength) * factor;
                    switch (material) {
                    case 128: burst.colour = {factor, factor * .5f, factor * .1f, alpha}; break;
                    case 129: burst.colour = {factor * .3f, factor, factor * .5f, alpha}; break;
                    case 130: burst.colour = {factor * .3f, factor * .5f, factor, alpha}; break;
                    case 131: burst.colour = {factor * .8f, factor * .8f, factor, alpha}; break;
                    }
                } else {
                    burst.colour = {.5f, .5f, .5f, 1.f};
                }
                if (input.area) {
                    burst.velocity *= 2.f;
                    burst.count += 10;
                }
                burst.size = .1f;
                burst.style = BRIGHT;
                burst.spread = 3.f;
                out.bursts.push_back(burst);
            }
        }
}

} // namespace

// §4

float levelGrowth(int32_t level) { return 3.f * (1.f - 1.f / (((float)level - 1.f) * 0.05f + 1.f)); }

float levelPower(int32_t level) { return (float)std::pow(2.0, (double)levelGrowth(level)); }

float basePower(const Combatant &c) {
    const float P = levelPower(c.level);
    if (c.controller == 0)
        return P * 2.f;
    return P * (float)std::pow(2.0, 0.25 * c.powerTier);
}

float maxHealth(const Combatant &c) {
    const float level = levelPower(c.level);
    const float rank = (float)std::pow(2.0, (double)((float)c.powerTier * 0.25f));
    float h = (rank * level) * c.multipliers[0];
    if (c.controller == 0)
        h = ((float)std::pow(2.0, 1.0) * level) * c.multipliers[0];
    switch (c.characterClass) {
    case 1:
        h *= 1.3f;
        if (c.specialization == 1)
            h *= 1.25f;
        break;
    case 2:
        h *= 1.1f;
        break;
    case 4:
        h *= 1.2f;
        break;
    }
    if (const auto &contribution = itemBonuses().health)
        for (int slot : {OFF_HAND, MAIN_HAND, CHEST_SLOT, BOOTS_SLOT, GLOVES_SLOT, SHOULDERS_SLOT})
            if (fits(slot, c.equipment[slot]))
                h = contribution(c.equipment[slot]) + h;
    return h;
}

bool admitCreatureUpdate(Combatant &c) {
    const float limit = maxHealth(c);
    if (c.health > limit)
        c.health = maxHealth(c);
    return !(c.health <= 0);
}

float damage(const Combatant &c) {
    const float m = c.multipliers[2];
    float d = levelPower(c.level) * m;
    const auto &f = itemBonuses().damage;
    if (f) {
        const bool shield = weaponIn(c.offHand()) == SHIELD;
        if (!shield && c.offHand().type == 3)
            d += f(c.offHand());
        if (c.mainHand().type == 3)
            d += f(c.mainHand()) * (shield ? 4.f : 1.f); // ×4 as read (open question B-1)
    }
    return d * m;
}

float armour(const Combatant &c) {
    const float base = basePower(c);
    float a = base * c.multipliers[3];
    if (c.appearanceFlags & 0x20)
        a += base;
    return a + bonus(itemBonuses().armour, c, {CHEST_SLOT, BOOTS_SLOT, GLOVES_SLOT, SHOULDERS_SLOT});
}

float resistance(const Combatant &c) {
    // The held periodic-hit stat contract keeps the rank-scaled common bonus
    // even when role zero replaces the initial base with twice level power.
    const float level = levelPower(c.level);
    const float common = (float)std::pow(2.0, (double)((float)c.powerTier * 0.25f)) * level;
    const float base = c.controller == 0 ? level * 2.f : common;
    float r = base * c.multipliers[4];
    if (c.appearanceFlags & 0x20)
        r += common;
    // Keep each slot addition in order; a separately rounded subtotal differs.
    if (const auto &contribution = itemBonuses().resistance)
        for (int slot : {CHEST_SLOT, BOOTS_SLOT, GLOVES_SLOT, SHOULDERS_SLOT})
            if (fits(slot, c.equipment[slot]))
                r += contribution(c.equipment[slot]);
    return r;
}

float attackSpeed(const Combatant &c) {
    float S = c.controller == 0 ? 1.f : (float)c.powerTier * .0625f + .75f;
    if (c.characterClass == 1 && c.specialization == 0)
        S += 0.5f * queriedComboRatio(c);
    if (c.characterClass == 3 && c.specialization == 1)
        S += queriedComboRatio(c);
    S += equipmentRate(c, itemBonuses().speed);
    if (c.hasBuff(HASTE)) {
        float n = (float)c.skills[8];
        if (c.controller != 0 && n == 0)
            n = (float)(c.level / 2 + c.powerTier + 1);
        const float skill = 1.f - 1.f / (0.1f * n + 1.f);
        S *= skill + 1.f;
    }
    return S;
}

float comboMax(const Combatant &c) {
    const int m = weaponIn(c.mainHand());
    float n = 50;
    if (m == BOW)
        n = 60;
    else if (m == BOOMERANG)
        n = 80;
    else if (m == WAND || m == BRACELET || m == GREATSWORD || m == GREATAXE || m == GREATMACE ||
             (m < 0 && (c.appearanceFlags & 0x10)))
        n = 20;
    // The native limit is an integer even though callers use it as a divisor.
    return (float)(int32_t)(c.multipliers[1] * n);
}

float criticalChance(const Combatant &c) {
    if (c.hasBuff(SURE_CRITICAL))
        return 1;
    return equipmentRate(c, itemBonuses().critical) + .15f * c.stealth;
}

int32_t experienceForLevel(int32_t level) {
    return (int32_t)((1.f - 1.f / ((float)(level - 1) * 0.05f + 1.f)) * 1000.f + 50.f);
}

int32_t killExperience(int32_t level, int32_t tier, int32_t type, bool boss, uint16_t appearanceFlags) {
    if (appearanceFlags & 0x800)
        return 0;
    const float n = (float)(level + tier);
    float e = std::max(1.f, 20.f * (1.f - 1.f / ((n - 1.f) * 0.05f + 1.f)));
    switch (type) {
    case 101: case 107: case 108: case 109: case 114: case 115: case 116: case 117: case 118: case 119:
        e *= 20;
        break;
    }
    if (boss)
        e *= 10;
    return (int32_t)e;
}

void addExperience(Combatant &c, int32_t amount) {
    c.experience += amount;
    while (c.level > 0 && c.experience >= experienceForLevel(c.level)) {
        c.experience -= experienceForLevel(c.level);
        c.level++;
        c.health = maxHealth(c);
    }
}

// §3.3: each phase is c / (S·a) unless fixed.

Phases phases(const Combatant &c, int32_t action) {
    struct Phase {
        int32_t ms;
        bool fixed;
    };
    auto scaled = [](int32_t ms) { return Phase{ms, false}; };
    auto fixed = [](int32_t ms) { return Phase{ms, true}; };
    Phase p1 = scaled(400), p2 = scaled(300), p3 = scaled(500);
    auto set = [&](Phase a, Phase b, Phase d) { p1 = a, p2 = b, p3 = d; };
    const bool bracelet = c.mainHand().subtype == BRACELET;
    switch (action) {
    case 0: set(fixed(0), fixed(0), fixed(100)); break;
    case 8: case 28: case 98: set(fixed(0), scaled(300), scaled(500)); break;
    case 11: case 60: case 61: case 104: set(fixed(0), scaled(300), scaled(100)); break;
    case 50: case 96: set(fixed(0), fixed(500), fixed(100)); break;
    case 55: set(fixed(0), scaled(500), scaled(500)); break;
    case 62: set(fixed(0), scaled(100), scaled(300)); break;
    case 1: case 9: set(scaled(300), scaled(200), scaled(500)); break;
    case 2: set(scaled(100), scaled(200), scaled(500)); break;
    case 3: case 4: set(scaled(100), scaled(100), scaled(300)); break;
    case 5: set(scaled(100), scaled(400), scaled(300)); break;
    case 6: set(scaled(50), scaled(150), scaled(500)); break;
    case 7: set(scaled(50), scaled(150), scaled(200)); break;
    case 10: set(scaled(100), fixed(200), fixed(600)); break;
    case 12: case 16: set(scaled(200), scaled(400), scaled(500)); break;
    case 13: set(scaled(200), scaled(200), scaled(500)); break;
    case 14: set(scaled(100), scaled(200), scaled(200)); break;
    case 15: set(scaled(200), scaled(200), scaled(400)); break;
    case 17: set(scaled(100), scaled(400), scaled(500)); break;
    case 18: set(scaled(100), scaled(150), scaled(200)); break;
    case 19: set(scaled(100), scaled(150), scaled(500)); break;
    case 20: case 21: set(scaled(100), scaled(400), scaled(500)); break;
    case 22: set(scaled(400), scaled(50), scaled(100)); break;
    case 23: set(scaled(50), scaled(50), scaled(10)); break;
    case 24: case 25: case 27: case 36: case 59: case 63: case 64: set(scaled(50), scaled(300), scaled(500)); break;
    case 26: set(scaled(300), scaled(1200), scaled(100)); break;
    case 30: case 32: set(scaled(800), fixed(600), scaled(100)); break;
    case 31: case 33: set(scaled(1600), fixed(1200), scaled(100)); break;
    case 34: set(scaled(1600), scaled(1200), scaled(100)); break;
    case 37: case 43: set(scaled(bracelet ? 600 : 1200), scaled(100), scaled(100)); break;
    case 38: case 44: set(scaled(500), scaled(300), scaled(100)); break;
    case 39: case 40: case 41: case 42: set(scaled(200), scaled(300), scaled(100)); break;
    case 45: case 46: set(scaled(1200), scaled(500), scaled(100)); break;
    case 48: set(fixed(100), scaled(300), fixed(0)); break;
    case 49: set(scaled(400), fixed(0), scaled(500)); break;
    case 54: set(fixed(400), fixed(100), fixed(400)); break;
    case 57: case 58: set(scaled(800), scaled(300), scaled(300)); break;
    case 65: case 66: set(scaled(300), scaled(400), scaled(200)); break;
    case 67: set(scaled(200), scaled(200), scaled(100)); break;
    case 68: case 69: set(scaled(300), fixed(1000), scaled(800)); break;
    case 70: case 74: case 75: set(scaled(300), scaled(300), scaled(500)); break;
    case 73: set(scaled(300), fixed(600), scaled(500)); break;
    case 76: case 77: case 78: set(scaled(300), fixed(500), scaled(500)); break;
    case 71: set(fixed(200), fixed(3000), fixed(200)); break;
    case 72: set(fixed(200), fixed(5000), fixed(200)); break;
    case 86: set(scaled(400), fixed(5000), scaled(500)); break;
    case 87: set(scaled(5000), scaled(300), scaled(500)); break;
    case 89: set(scaled(bracelet ? 600 : 1200), scaled(300), scaled(500)); break;
    case 91: set(scaled(1000), fixed(6000), scaled(500)); break;
    case 93: set(scaled(800), fixed(600), scaled(800)); break;
    case 94: set(scaled(500), scaled(500), scaled(100)); break;
    case 95: set(scaled(1000), fixed(2000), scaled(100)); break;
    case 101: set(fixed(100), scaled(300), scaled(500)); break;
    case 105: set(fixed(5000), scaled(300), scaled(500)); break;
    default: break;
    }
    const float speed = attackSpeed(c) * c.multipliers[1];
    auto ms = [&](Phase p) { return p.fixed ? p.ms : (int32_t)((float)p.ms / speed); };
    return {ms(p1), ms(p2), ms(p3)};
}

int32_t duration(const Combatant &c, int32_t action) { return phases(c, action).total(); }

bool isMagicUser(const Combatant &c) {
    const int m = weaponIn(c.mainHand());
    return c.characterClass == 3 || c.type == 117 || c.type == 86 || m == STAFF || m == WAND || m == BRACELET;
}

// §13.2 step 9 (0x444520).
bool rangedUser(const Combatant &c) {
    const int m = c.mainHand().type == 3 ? c.mainHand().subtype : -1;
    return c.characterClass == 3 || c.type == 117 || c.type == 86 || c.type == 104 || m == STAFF || m == WAND ||
           m == BRACELET || m == BOW || m == CROSSBOW || m == BOOMERANG;
}

float hitThreat(const Combatant &source, float amount) {
    const float multiplier = rangedUser(source) ? 1.f : source.type == 25 ? 10.f :
        source.specialization == 1 && heavyWeapon(source.mainHand()) ? 20.f : 2.f;
    return amount * multiplier + 0.01f;
}

int32_t primaryAttack(const Combatant &c) {
    // "Chain x→y": y when x is running or ended less than 200 ms ago.
    auto chain = [&](int32_t x, int32_t y) {
        return c.action == x && c.actionMs < duration(c, x) + 200 ? y : x;
    };
    const int m = weaponIn(c.mainHand()), o = weaponIn(c.offHand());
    const int raw = c.mainHand().subtype;
    if (isMagicUser(c)) {
        if (raw == WAND)
            return c.specialization == 1 ? 44 : 38;
        if (raw == STAFF)
            return c.specialization == 1 ? 32 : 30;
        return c.specialization == 1 ? chain(42, 41) : chain(40, 39);
    }
    if (c.type == 104 || m == BOW || m == CROSSBOW || o == BOW || o == CROSSBOW)
        return 22;
    if (raw == DAGGER)
        return chain(19, 18);
    if (raw == FIST || c.mainHand().type == 0)
        return chain(6, 7);
    if (raw == LONGSWORD)
        return chain(14, 13);
    if (m == BOOMERANG)
        return 26;
    if (twoHandedOrRanged(m)) {
        if (c.action == 57)
            return chain(57, 67);
        if (c.action == 67 && c.actionMs < duration(c, 67) + 200)
            return 58;
        return 57;
    }
    if (o == SHIELD)
        return chain(10, 9);
    if (c.appearanceFlags & 0x10)
        return 75;
    return chain(1, 2);
}

int32_t secondaryAttack(const Combatant &c) {
    const int m = weaponIn(c.mainHand());
    const bool odd = (c.combo & 1) != 0;
    if (m == BOW)
        return 23;
    if (isMagicUser(c)) {
        const bool spec1 = c.specialization == 1;
        if (c.mainHand().subtype == WAND)
            return spec1 ? 45 : 46;
        if (c.mainHand().subtype == STAFF)
            return spec1 ? 33 : 31;
        return spec1 ? 43 : 37;
    }
    if (m == LONGSWORD)
        return 5;
    if (twoHandedOrRanged(m))
        return odd ? 65 : 66;
    return odd ? 4 : 3;
}

bool startPrimaryAttack(Combatant &c) {
    if (c.dodgeMs != 0 || c.action == 48)
        return false;
    const Phases p = phases(c, c.action);
    int32_t ready = p.total();
    if (c.combo != 0 && c.action != 30 && c.action != 32)
        ready -= p.recovery;
    // True reports an admitted attempt, including a busy action with no writes.
    // Selection uses the old state only after its inclusive readiness gate.
    if (c.actionMs >= ready) {
        c.action = primaryAttack(c);
        c.actionMs = 0;
    }
    return true;
}
bool startSecondaryAttack(Combatant &c) {
    const Phases old = phases(c, c.action);
    const int32_t ready = old.total() - (c.combo != 0 ? old.recovery : 0);
    if (c.actionMs < ready)
        return false;
    const int32_t selected = secondaryAttack(c);
    const bool retainsRoll = c.characterClass == 4 && c.specialization == 1 &&
        (selected == 5 || selected == 17 || selected == 20);
    if ((c.dodgeMs != 0 && !retainsRoll) || manaCost(c, selected) > c.mana)
        return false;
    c.action = selected;
    c.actionMs = 0;
    if (!retainsRoll)
        c.dodgeMs = 0;
    // Action 95 is in the native gate, although this selector cannot return it.
    if ((selected == 31 || selected == 33 || selected == 37 || selected == 46 || selected == 95) && c.hasBuff(9))
        c.actionMs = phases(c, c.action).windUp;
    return true;
}

void landHit(Combatant &c) {
    if (!c.hitLanded)
        c.combo++;
    c.hitLanded = true;
    c.comboElapsedMs = 0;
}

void beginActionUpdate(Combatant &c, int32_t milliseconds, std::vector<BuffEvent> *events) {
    const float dtMs = (float)milliseconds;
    c.actionLockMs = std::max(0, c.actionLockMs - milliseconds);
    if (c.actionLockMs > 0)
        c.actionMs = duration(c, c.action) + 1;
    c.slowMs = std::max(0, std::bit_cast<int32_t>((uint32_t)c.slowMs - (uint32_t)milliseconds));
    c.hasteMs = std::max(0, std::bit_cast<int32_t>((uint32_t)c.hasteMs - (uint32_t)milliseconds));
    if (c.recoveryPenalty < 0)
        c.recoveryPenalty = std::min(0.f, c.recoveryPenalty + (dtMs * 0.001f) / 180.f);
    // Healing and projectile bodies have separate resource boundaries.
    if (c.actionMs == 0 && (c.action == 34 || c.action == 95))
        c.mana = std::max(0.f, c.mana - manaCost(c, c.action));
    if (meleeAction(c.action)) {
        if (c.actionMs == 0 && c.characterClass == 4 && c.specialization == 1 &&
            (c.action == 5 || c.action == 17 || c.action == 20))
            c.dodgeMs = 600;
        const Phases timing = phases(c, c.action);
        if (c.actionMs <= timing.windUp) {
            c.struck.clear();
            c.hitLanded = false;
        }
        const auto projectedClock = [&] {
            return std::bit_cast<int32_t>((uint32_t)c.actionMs + (uint32_t)milliseconds);
        };
        // This marker records reaching target processing, not successful damage.
        // A miss resets the combo before resource initialization and hit dispatch.
        if (!c.hitLanded && projectedClock() >= timing.windUp + timing.active &&
            c.actionMs < timing.total() && c.dodgeMs == 0) {
            const float before = attackSpeed(c);
            c.combo = 0;
            c.actionMs = (int32_t)((float)c.actionMs * (before / attackSpeed(c)));
        }
        const bool cast = c.action == 31 || c.action == 33;
        if (cast) {
            const int32_t anticipation = phases(c, c.action).windUp;
            if (c.actionMs <= anticipation && projectedClock() > anticipation) {
                c.spentPower = .1f;
                c.mana -= manaCost(c, c.action);
                if (c.mana < 0)
                    c.mana = 0;
            }
        } else if (c.actionMs == 0) {
            c.criticalLatched = false;
            c.spentPower = 0;
            if (manaCost(c, c.action) > 0) {
                c.spentPower = manaCost(c, c.action);
                c.mana -= c.spentPower;
            } else if (c.action == 17 || c.action == 20 || c.action == 5) {
                c.spentPower = c.mana;
                c.mana = 0;
                if (auto consumed = consumeBuff(c, 11)) {
                    if (events)
                        events->push_back(*consumed);
                    c.criticalLatched = true;
                }
            } else if (c.action == 21 || c.action == 54 || c.action == 87 || c.action == 88) {
                c.spentPower = 1;
            }
        }
        if (c.charge > 0) {
            c.spentPower = c.charge;
            c.mana -= c.charge;
            c.hitLanded = false;
            if (c.action == 5 || c.action == 11)
                c.spentPower *= .5f;
        }
        c.charge = 0;
        if (c.mana < 0)
            c.mana = 0;
        if (c.action == 31 && c.actionMs >= phases(c, c.action).windUp)
            if (auto consumed = consumeBuff(c, 9); consumed && events)
                events->push_back(*consumed);
    }
}

void endActionUpdate(Combatant &c, int32_t milliseconds, ActionTickRole role,
                     const std::function<void(const Buff &)> &periodic) {
    const float dtMs = (float)milliseconds;
    if (!role.replica || role.local) {
        c.comboElapsedMs += milliseconds;
        if (c.comboElapsedMs > 4000) {
            const float speed = attackSpeed(c);
            c.combo = 0;
            c.actionMs = (int32_t)((speed / attackSpeed(c)) * (float)c.actionMs);
        }
    }
    // The supported image's cooldown-refresh policy is constant false.
    for (auto &[id, ms] : c.cooldowns)
        ms = std::max(0.f, ms - dtMs);
    tickBuffs(c, milliseconds, !role.replica, periodic);
    c.hitStun -= milliseconds;
    if (c.hitStun > 0)
        c.action = 0, c.actionMs = 0;
    // The held scheduling contract keeps completed action IDs for chain decisions.
    // Controller 6 has no general clock increment; a negative active clock waits.
    const int32_t before = c.actionMs;
    if (c.controller != 6 && (c.actionMs >= 0 || c.action == 0)) {
        if (c.actionMs == 0)
            c.cooldowns[c.action] = (float)cooldownLength(c, c.action);
        c.actionMs += milliseconds;
    }
    // Item names §9: drinking (80) or eating (81) heals once per 200 ms of the action, a life
    // potion over 3 s and other food over 10 s; creatures other than players at half speed.
    if ((c.action == 80 || c.action == 81) && c.held.type == 1) {
        const float limit = c.held.subtype == 1 ? 3000.f : 10000.f;
        if (before < limit && std::floor(before / 200) != std::floor(c.actionMs / 200)) {
            const float share = 200 / limit * (c.controller != 0 ? 0.5f : 1.f);
            const float amount = itemHeal(c.held) * share;
            if (amount > 0)
                c.health = std::min(c.health + amount, maxHealth(c));
        }
    }
    bool drift = true;
    if (isMagicUser(c)) {
        switch (c.action) {
        case 28: drift = false; break;
        case 95: case 37: case 46: case 45: case 31: case 33: case 43: case 34:
            drift = c.actionMs >= (float)duration(c, c.action); break;
        default: break;
        }
        if (drift)
            c.mana += dtMs * 0.0001f;
    } else {
        switch (c.action) {
        case 24: case 25: case 27: case 59: case 63: case 64: case 8: drift = false; break;
        default: break;
        }
        if (drift)
            c.mana -= dtMs * 0.00005f;
    }
    c.mana = std::clamp(c.mana, 0.f, 1.f);
    const bool blocking = isBlocking(c);
    if (blocking) {
        if (c.blockGauge > 0)
            c.blockGauge = std::max(0.f, c.blockGauge - dtMs /
                (c.offHand().type == 3 && c.offHand().subtype == SHIELD ? 1200.f : 600.f));
    } else
        c.blockGauge += dtMs / 2000.f;
    c.blockGauge = std::min(c.blockGauge, 1.f);
    // Existing later consumption/stealth behavior is outside the common phase
    // contract and remains separate from its independently audited action effects.
    c.mana = std::min(1.f, c.mana + 0.00025f * dtMs * c.stealth);
    if (c.sitting)
        c.stealth = std::min(1.f, c.stealth + dtMs * 0.00001f); // the action-99 term waits for skills
    else
        c.stealth = std::max(0.f, c.stealth - 0.0005f * dtMs);
    c.dodgeMs = std::max(0.f, c.dodgeMs - dtMs);
    c.hitFlash = std::max(0.f, c.hitFlash - dtMs * 0.001f * 4.f); // fade rate: ours (the renderer's is unspecified)
}

void stepAction(Combatant &c, float dtMs) {
    const auto milliseconds = (int32_t)dtMs;
    beginActionUpdate(c, milliseconds);
    endActionUpdate(c, milliseconds);
}

bool hitMoment(const Combatant &c, float beforeMs) {
    const float windUp = (float)phases(c, c.action).windUp;
    return beforeMs <= windUp && windUp < c.actionMs;
}

bool hitMoment(const Combatant &c, float beforeMs, float dtMs) {
    const float windUp = (float)phases(c, c.action).windUp;
    return beforeMs <= windUp && windUp < beforeMs + dtMs;
}

// §5

HitOutcome resolveHit(Combatant &target, glm::vec3 &, Combatant *attacker, const HitInput &in,
                      const std::function<int()> &draw, const std::function<void(const HitEmission &)> &emit) {
    HitOutcome out;
    if (target.health <= 0)
        return out;
    if (attacker && (attacker->action == 91 || attacker->action == 74) && !in.targetOnGround)
        return out;
    auto r = [&] { return (float)draw() / 32767.f; };
    // Particles §5.8: a beam (action 28) throws sparks at what it hits, before anything else.
    if (attacker && attacker->action == 28)
        out.bursts.push_back(Burst::at(in.targetPosition, {0, 0, 10}, {0.6f, 0.6f, 1, 1}, 0.1f + 0.1f * r(), 8, FIRE));
    auto sound = [&](int32_t id, float pitch) { out.sounds.push_back({in.targetPosition, id, pitch, 1}); };
    const auto targetPresentation = [&](int32_t kind, float scalar) {
        glm::dvec3 at;
        for (int axis = 0; axis < 3; axis++)
            at[axis] = (double)((float)in.targetRecordPosition[axis] * (1.f / 65536.f));
        // Preserve the native payload's scalar order; playback-role labeling is
        // outside the held producer contract.
        return SoundEvent{at, kind, scalar, 1};
    };
    const auto earlyPresentation = [&](int32_t kind) {
        auto event = targetPresentation(kind, 1);
        const float scaledDraw = (float)draw() * .3f;
        event.pitch = scaledDraw / 32767.f + 1.f;
        out.sounds.push_back(event);
    };
    auto record = [&](uint8_t kind, float amount, glm::vec3 k) {
        HitRecord h;
        h.attacker = attacker ? attacker->id : 0;
        h.target = target.id;
        h.position = in.targetRecordPosition;
        h.amount = amount;
        h.knockBack = k;
        h.kind = kind;
        out.records.push_back(h);
    };
    const auto consume = [&](bool applyLocally = true) {
        if (emit)
            emit({out.records.back(), applyLocally});
    };
    float x = in.amount;
    if (!attacker) {
        draw();
        target.hitFlash = 0.5f;
    } else {
        // Immune targets.
        if (target.controller == 1 && ((target.mode & 0x80) || (target.contact & 0x20))) {
            earlyPresentation(24);
            record(HIT_IMMUNE, x, in.knockBack);
            consume();
            return out;
        }
        // Blocking.
        if (in.blockable && isBlocking(target) && target.blockGauge > 0) {
            const float q = x / damage(target);
            target.mana = std::min(1.f, target.mana + target.blockGauge);
            target.charge = std::min(target.mana, target.charge + target.blockGauge);
            target.blockGauge = std::max(-1.f, target.blockGauge - q);
            record(HIT_BLOCKED, x, in.knockBack * 0.5f);
            earlyPresentation(24);
            consume();
            return out;
        }
        // Absorbing shields, weakened by the attacker's combo.
        const float s = target.buffSum(ABSORB);
        const float c = 1.f - queriedComboRatio(*attacker);
        if (c * s > 0) {
            record(HIT_ABSORBED, x, in.knockBack * 0.5f);
            earlyPresentation(91);
            consume();
            x -= c * s;
            if (x <= 0) {
                out.hitAccepted = true;
                return out;
            }
        }
        x = std::max(0.f, x - c * (in.magic ? resistance(target) : armour(target)));
        // Step 8: with a long combo the weapons' special spirits burst at the target (§12.1,
        // visual only; drawn once spirits are drawn). The area effects of §12.2 come with
        // spirit items.
        // This fresh, cached ratio is not the upper-capped mitigation ratio.
        const float sweep = (float)attacker->combo / comboMax(*attacker);
        if (sweep > 0.25f)
            appendItemEffects(*attacker, in, sweep, out);
        if (in.area && sweep > 0) {
            const auto items = spiritItems(*attacker);
            const int count = (items[0] != nullptr) + (items[1] != nullptr);
            if (count) {
                const float scale = .1f / (float)count;
                float auxiliary = 0;
                for (const Item *item : items)
                    if (item) {
                        // The owned record has at most 32 decoded entries. Invalid
                        // native counts have no safe-memory equivalence claim.
                        const int32_t records = std::min(item->spirits, (int32_t)item->cubes.size());
                        for (int32_t index = 0; index < records; index++) {
                            const auto &cube = item->cubes[index];
                            if (cube.material < 0x80 || cube.material > 0x83)
                                continue;
                            const float level = (float)cube.level;
                            const float transformed = 1.f - 1.f / ((level - 1.f) * .05f + 1.f);
                            if (cube.material == 0x80 || cube.material == 0x81) {
                                const float weighted = transformed * scale;
                                const float contribution = weighted * sweep;
                                if (cube.material == 0x80) {
                                    x += contribution;
                                } else {
                                    x += .5f * contribution;
                                    auxiliary += 2.f * contribution;
                                }
                            } else {
                                const float quarterScale = scale * .25f;
                                const float weighted = transformed * quarterScale;
                                x += weighted * sweep;
                                const float increase = sweep * 500.f;
                                int32_t &counter = cube.material == 0x82 ? target.slowMs : attacker->hasteMs;
                                counter = (int32_t)((float)counter + increase);
                            }
                        }
                    }
                target.slowMs = std::min(target.slowMs, 5000);
                attacker->hasteMs = std::min(attacker->hasteMs, 5000);
                if (auxiliary > 0) {
                    HitRecord gain;
                    gain.attacker = gain.target = attacker->id;
                    gain.position = in.attackerRecordPosition;
                    gain.amount = std::bit_cast<float>(std::bit_cast<uint32_t>(auxiliary) ^ 0x80000000u);
                    out.records.push_back(gain);
                    consume(false);
                    attacker->health += auxiliary;
                }
            }
        }
        if (attacker->action == 28) {
            if (target.hitFlash < 0.3f)
                target.hitFlash = 0.3f;
        } else if (in.blockable) {
            target.hitFlash = 1;
        }
        // The impact sound by weapon class.
        if (in.blockable && attacker->action != 28) {
            float pitch = 0.9f + 0.2f * r();
            int sub = weaponIn(attacker->mainHand());
            if (sub < 0)
                sub = weaponIn(attacker->offHand());
            if (sub < 0)
                sub = FIST; // bare hands (inferred)
            if (attacker->action == 68)
                sub = 17;
            int id;
            if (sub <= 1) {
                id = in.critical ? 2 : 1;
                pitch += 0.1f;
            } else if (sub == 15 || sub == 16) {
                id = in.critical ? 2 : 1;
            } else if (sub == 3 || sub == 4) {
                id = in.critical ? 8 : 7;
            } else if (sub == 5) {
                id = in.critical ? 4 : 3;
            } else if (sub >= 6 && sub <= 8) {
                id = in.critical ? 10 : 9;
            } else {
                id = in.critical ? 6 : 5;
            }
            if (attacker->action == 54)
                id = 11;
            else if (attacker->action == 10)
                id = 5;
            if (in.magic) {
                id = attacker->specialization == 1 ? 42 : 39;
                pitch = attacker->specialization == 1 ? 1.f : 1.5f;
            }
            sound(id, pitch);
        }
    }
    // The native repeated-norm gate never enters its velocity-write tail for
    // stable finite operands. Preserve the supplied record vector unchanged.
    for (const Buff &b : target.buffs)
        if (b.kind == DAMAGE_TAKEN)
            x = b.value * x;
    if (in.selector == 17 && attacker) {
        // Native production leaves padding unspecified. Fresh engine records use
        // deterministic zero padding; received records retain their opaque bytes.
        const BuffEvent event{attacker->id, target.id, {DAMAGE_OVER_TIME, .1f * x, 3000, attacker->id}};
        applyBuff(target, event.record);
        out.buffs.push_back(event);
    }
    if (in.critical && target.controller != 6) {
        int voice = 0;
        switch (target.type) {
        case 0: voice = 60; break;
        case 1: voice = 61; break;
        case 2: case 43: voice = 62; break;
        case 3: case 45: voice = 63; break;
        case 4: voice = 64; break;
        case 5: voice = 65; break;
        case 7: voice = 66; break;
        case 8: voice = 67; break;
        case 9: voice = 68; break;
        case 10: voice = 69; break;
        // Audio §6.3: the natural voices of orcs, undead, frogmen and monsters.
        case 11: voice = 70; break;
        case 12: voice = 71; break;
        case 15: voice = 72; break;
        case 16: voice = 73; break;
        case 13: voice = 74; break;
        case 14: voice = 75; break;
        case 119: voice = 76; break;
        case 46: case 108: case 109: case 114: case 115: voice = 77; break;
        case 48: case 51: case 87: voice = 78; break;
        case 37: case 38: case 39: case 40: voice = 79; break;
        case 96: voice = 80; break;
        }
        const float pitch = 1.f + 0.1f * r();
        if (voice)
            sound(voice, pitch);
    }
    record(HIT_DAMAGE, x, in.knockBack);
    out.records.back().critical = in.critical;
    out.records.back().area = in.area;
    out.records.back().beam = attacker && attacker->action == 28 ? 1 : 0;
    // Open points 6 §3.2: a stun, for a target free of one for at least 3 s.
    if (attacker && target.hitStun < -3000 && target.controller != 6) {
        const auto growth = [](int32_t level) { return 1.f - 1.f / (((float)level - 1.f) * 0.05f + 1.f); };
        const bool big = bigCreature(target.type), attackerBig = bigCreature(attacker->type);
        const int32_t action = attacker->action;
        float v = in.power;
        if (action == 58 || action == 12 || action == 68 || action == 93 || action == 69)
            v = 1;
        if (big)
            v *= 0.15f;
        if (!attackerBig && attacker->controller != 0)
            v *= 0.1f;
        if (action == 91 || action == 74 || target.action == 84)
            v = 1;
        const float sourceGrowth = growth(attacker->level);
        const float targetGrowth = growth(target.level);
        const float exponent = (sourceGrowth - targetGrowth) * 10.f;
        const float scale = (float)std::pow(2.0, (double)exponent);
        v *= scale;
        if (v > 1.f)
            v = 1.f;
        float chance = v * .9f;
        if (target.hasBuff(DAMAGE_TAKEN))
            chance = 0;
        if (r() < chance) {
            out.records.back().stun = target.controller == 0 ? 1500 : 3000 - (1500 * target.powerTier >> 2);
            out.records.back().knockBack.z = 10;
            out.sounds.push_back(targetPresentation(23, 1));
        }
    }
    consume();
    out.hitAccepted = true;
    return out;
}

void applyHit(Combatant &target, Body &body, const HitRecord &hit, HitApplicationContext context,
              std::vector<BuffEvent> *events) {
    if (target.health <= 0)
        return;
    if (hit.amount > 0) {
        const uint32_t combined = (uint32_t)target.level - 1u + (uint32_t)target.powerTier;
        const float exponent = (float)std::bit_cast<int32_t>(combined) * .25f;
        const double power = std::pow(2.0, (double)exponent);
        const float numerator = hit.amount * .25f;
        const float denominator = (float)power;
        const float reduction = numerator / denominator;
        target.stealth -= reduction;
        if (target.stealth < 0)
            target.stealth = 0;
        if (target.action == 84) {
            target.action = 0;
            target.hitStun = 3000;
        }
    }
    if ((target.appearanceFlags & 0x2000) && !context.protectionGranted)
        return;
    if (hit.kind == HIT_IMMUNE)
        return;
    if (hit.amount >= 0) {
        glm::vec3 response = hit.knockBack;
        if (target.hitStun > 0 && hit.stun <= 0)
            response *= .1f;
        const auto squared = [](glm::vec3 value) {
            return (value.x * value.x + value.y * value.y) + value.z * value.z;
        };
        if (target.controller != 6 && (context.local || (!context.replica && target.controller != 0)) &&
            squared(response) >= squared(body.push))
            body.push = response;
        if (hit.stun > 0)
            body.velocity = glm::vec3(0);
    }
    if (hit.lockMs > 0)
        target.actionLockMs = std::max(target.actionLockMs, hit.lockMs);
    if (hit.kind == HIT_BLOCKED) {
        target.mana += .25f;
        if (target.mana > 1)
            target.mana = 1;
        return;
    }
    // Category 4 and other specialized consumers require separate handling.
    if (hit.kind != HIT_DAMAGE && hit.kind != HIT_ABSORBED)
        return;
    if (hit.stun > 0)
        target.hitStun = std::max(target.hitStun, hit.stun);
    if (context.replica && !context.local)
        return;
    if (hit.amount > 0 && hit.kind == HIT_ABSORBED) {
        for (Buff &shield : target.buffs)
            if (shield.kind == ABSORB) {
                shield.value -= hit.amount;
                if (shield.value <= 0) {
                    shield.value = 0;
                    shield.timeMs = 0;
                }
                if (events)
                    events->push_back({hit.attacker, hit.target, shield});
            }
    } else if (hit.amount > 0) {
        const auto eligible = [](const Combatant &actor) {
            switch (actor.controller) {
            case 0: case 1: case 3: case 5: return true;
            case 6: return actor.type != 140 && actor.type != 141 && actor.type != 142;
            default: return false;
            }
        };
        if (eligible(target) && (!context.source || eligible(*context.source)))
            target.health -= hit.amount;
    } else if (hit.amount < 0) {
        target.health -= hit.amount;
        if (target.health > maxHealth(target))
            target.health = maxHealth(target);
    }
    if (target.health <= 0) {
        target.health = 0;
        target.actionMs = 0;
    }
}

void receiveHit(Combatant &controlled, Body &body, const HitRecord &hit, std::vector<BuffEvent> *events) {
    if (hit.kind == HIT_BLOCKED) {
        if (hit.amount > 0) {
            body.velocity = hit.knockBack;
            controlled.hitFlash = 1;
        }
        body.velocity = hit.knockBack;
        const float amount = hit.amount;
        const float denominator = damage(controlled);
        const float debit = amount / denominator;
        controlled.mana += controlled.blockGauge;
        controlled.charge += controlled.blockGauge;
        if (controlled.mana > 1)
            controlled.mana = 1;
        if (controlled.charge > controlled.mana)
            controlled.charge = controlled.mana;
        controlled.blockGauge -= debit;
        if (controlled.blockGauge < -1)
            controlled.blockGauge = -1;
        return;
    }
    if (hit.kind == HIT_ABSORBED) {
        for (Buff &shield : controlled.buffs)
            if (shield.kind == ABSORB) {
                shield.value -= hit.amount;
                if (shield.value <= 0) {
                    shield.value = 0;
                    shield.timeMs = 0;
                }
                if (events)
                    events->push_back({controlled.id, controlled.id, shield});
            }
        return;
    }
    if ((hit.kind != HIT_DAMAGE && hit.kind != HIT_IMMUNE) || !(controlled.health > 0))
        return;
    if (hit.amount > 0) {
        const float quarterAmount = hit.amount * .25f;
        const int32_t priorLevel = std::bit_cast<int32_t>((uint32_t)controlled.level - 1u);
        const float exponent = (float)priorLevel * .25f;
        const float denominator = (float)std::pow(2.0, (double)exponent);
        controlled.stealth -= quarterAmount / denominator;
        if (controlled.stealth < 0)
            controlled.stealth = 0;
    }
    if (hit.stun > 0)
        controlled.hitStun = std::max(controlled.hitStun, hit.stun);
    controlled.health -= hit.amount;
    if (controlled.health < 0)
        controlled.health = 0;
    const float comparisonHealth = controlled.health;
    const float limit = maxHealth(controlled);
    if (comparisonHealth > limit)
        controlled.health = maxHealth(controlled);
}

const char *soundKey(int32_t id) {
    static const char *const KEYS[101] = {
    "hit.wav", "blade1.wav", "blade2.wav", "long-blade1.wav", "long-blade2.wav", "hit1.wav",
    "hit2.wav", "punch1.wav", "punch2.wav", "hit-arrow.wav", "hit-arrow-critical.wav", "smash1.wav",
    "slam-ground.wav", "smash-hit2.wav", "smash-jump.wav", "swing.wav", "shield-swing.wav", "swing-slow.wav",
    "swing-slow2.wav", "arrow-destroy.wav", "blade1.wav", "punch2.wav", "salvo2.wav", "sword-hit03.wav",
    "block.wav", "shield-slam.wav", "roll.wav", "destroy2.wav", "cry.wav", "levelup2.wav",
    "missioncomplete.wav", "water-splash01.wav", "step2.wav", "step-water.wav", "step-water2.wav", "step-water3.wav",
    "channel2.wav", "channel-hit.wav", "fireball.wav", "fire-hit.wav", "magic02.wav", "watersplash.wav",
    "watersplash-hit.wav", "lich-scream.wav", "drink2.wav", "pickup.wav", "disenchant2.wav", "upgrade2.wav",
    "swirl.wav", "human-voice01.wav", "human-voice02.wav", "gate.wav", "spike-trap.wav", "fire-trap.wav",
    "lever.wav", "charge2.wav", "magic02.wav", "drop.wav", "drop-coin.wav", "drop-item.wav",
    "male-groan.wav", "female-groan.wav", "male-groan.wav", "female-groan.wav", "goblin-male-groan.wav", "goblin-female-groan.wav",
    "lizard-male-groan.wav", "lizard-female-groan.wav", "dwarf-male-groan.wav", "dwarf-female-groan.wav", "orc-male-groan.wav", "orc-female-groan.wav",
    "undead-male-groan.wav", "undead-female-groan.wav", "frogman-male-groan.wav", "frogman-female-groan.wav", "monster-groan.wav", "troll-groan.wav",
    "mole-groan.wav", "slime-groan.wav", "zombie-groan.wav", "Explosion.wav", "punch2.wav", "menu-open2.wav",
    "menu-close2.wav", "menu-select.wav", "menu-tab.wav", "menu-grab-item.wav", "menu-drop-item.wav", "craft.wav",
    "craft-proc.wav", "absorb.wav", "manashield.wav", "bulwark.wav", "bird1.wav", "bird2.wav",
    "bird3.wav", "cricket1.wav", "cricket2.wav", "owl1.wav", "owl2.wav",
    };
    return id >= 0 && id <= 100 ? KEYS[id] : nullptr;
}

void regenerate(Combatant &c, int32_t dtMs) {
    if (c.controller != 0 || c.action != 84 || !(c.health > 0))
        return;
    const float maximum = maxHealth(c);
    const float seconds = (float)dtMs * .001f;
    const float fraction = seconds * .05f;
    c.health += maximum * fraction;
    const float limit = maxHealth(c);
    if (c.health > limit)
        c.health = maxHealth(c);
}

float itemRegeneration(const Item &item) {
    if (item.type < 3 || item.type > 7)
        return 0;
    const float k = item.type == 4 ? 0.2f : 0.1f;
    float q = (float)((item.seed * 8u) % 21u) / 20.f;
    if (item.material == 26)
        q += 0.5f;
    else if (item.material == 27)
        q += 1;
    return itemLevelFactor((float)item.level + 0.1f * (float)item.spirits, item.rarity) * k * q;
}

float itemHeal(const Item &item) {
    if (item.type != 1)
        return 0;
    switch (item.subtype) {
    case 1: case 2: case 4: case 5: case 6: return 200 * itemLevelFactor((float)item.level, item.rarity);
    case 8: case 9: return 100 * itemLevelFactor((float)item.level, item.rarity);
    default: return 0;
    }
}

float regeneration(const Combatant &c) {
    float r = 0;
    for (int slot : {MAIN_HAND, OFF_HAND, CHEST_SLOT, BOOTS_SLOT, GLOVES_SLOT, SHOULDERS_SLOT, AMULET_SLOT})
        if (fits(slot, c.equipment[slot]))
            r += itemRegeneration(c.equipment[slot]);
    return r;
}

// An item's level factor against the player's own (the player's rarity taken as 0, believed).
static float rating(const Combatant &c, const Item &item) {
    return itemLevelFactor((float)item.level, item.rarity) / itemLevelFactor((float)c.level, 0);
}

float weaponRating(const Combatant &c) {
    float w = 0;
    for (int slot : {OFF_HAND, MAIN_HAND})
        if (c.equipment[slot].type == 3)
            w += rating(c, c.equipment[slot]);
    return heavyWeapon(c.mainHand()) ? w : w * 0.5f; // halved unless two-handed
}

float armorRating(const Combatant &c) {
    const auto f = [&](int slot) { return fits(slot, c.equipment[slot]) ? rating(c, c.equipment[slot]) : 0.f; };
    return (f(SHOULDERS_SLOT) + 2 * f(CHEST_SLOT) + f(GLOVES_SLOT) + f(BOOTS_SLOT)) / 5;
}

} // namespace creatures
