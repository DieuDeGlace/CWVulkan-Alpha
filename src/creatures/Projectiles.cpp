#include "creatures/Projectiles.h"
#include "creatures/Names.h"
#include "creatures/Npcs.h"
#include "creatures/Strike.h"
#include "creatures/Visibility.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace creatures {

std::array<uint8_t, 112> Projectile::serialize() const {
    std::array<uint8_t, 112> r{};
    const auto put = [&](size_t at, const void *v, size_t n) { std::memcpy(r.data() + at, v, n); };
    put(0x00, &shooter, 8);
    put(0x08, home.data(), 12);
    put(0x18, position, 24);
    put(0x3c, &velocity, 12);
    put(0x48, &damage, 4);
    put(0x4c, &radius, 4);
    put(0x50, &scale, 4);
    put(0x54, &strength, 4);
    put(0x58, &cost, 4);
    r[0x5c] = special;
    put(0x60, &kind, 4);
    r[0x64] = element;
    put(0x68, &age, 4);
    put(0x6c, &slow, 4);
    return r;
}

Projectile Projectile::parse(const uint8_t *r) {
    Projectile p;
    const auto get = [&](size_t at, void *v, size_t n) { std::memcpy(v, r + at, n); };
    get(0x00, &p.shooter, 8);
    get(0x08, p.home.data(), 12);
    get(0x18, p.position, 24);
    get(0x3c, &p.velocity, 12);
    get(0x48, &p.damage, 4);
    get(0x4c, &p.radius, 4);
    get(0x50, &p.scale, 4);
    get(0x54, &p.strength, 4);
    get(0x58, &p.cost, 4);
    p.special = r[0x5c];
    get(0x60, &p.kind, 4);
    p.element = r[0x64];
    get(0x68, &p.age, 4);
    get(0x6c, &p.slow, 4);
    return p;
}

namespace {

constexpr float WU = 65536.f;

bool hasChargeCue(int32_t action) {
    return action == 24 || action == 25 || action == 27 || action == 36 ||
           action == 59 || action == 63 || action == 64;
}

// §2.2 (0x3df60): the weapons W sums, in order. A bow or crossbow (subtype 6, 7) counts alone,
// the main hand's first; otherwise action 96 takes the off hand then the main hand, and the
// other actions the main hand then an off-hand bracelet (subtype 12).
struct WeaponSet {
    std::array<const Item *, 2> items{};
    int count = 0;
    void add(const Item &item) { items[count++] = &item; }
};
WeaponSet weaponSet(const Combatant &c) {
    WeaponSet set;
    const Item &main = c.mainHand(), &off = c.offHand();
    const auto ranged = [](const Item &item) { return item.type == 3 && (item.subtype == BOW || item.subtype == CROSSBOW); };
    if (ranged(main)) {
        set.add(main);
    } else if (ranged(off)) {
        set.add(off);
    } else if (c.action == 96) {
        if (off.type == 3)
            set.add(off);
        if (main.type == 3)
            set.add(main);
    } else {
        if (main.type == 3)
            set.add(main);
        if (off.type == 3 && off.subtype == BRACELET)
            set.add(off);
    }
    return set;
}

// §2.2: W, the weapon damage, with the per-action factor; M is the same without it.
float weaponDamage(const Combatant &c, bool actionFactor) {
    const float power = levelPower(c.level); // 2^g in double, rounded to single
    float w = c.multipliers[2] * power;
    const WeaponSet set = weaponSet(c);
    if (itemBonuses().damage)
        for (int k = 0; k < set.count; k++)
            w += itemBonuses().damage(*set.items[k]);
    if (set.count == 0) {
        const float bare = power * (float)std::pow(2.0, 0.25 * (double)c.powerTier);
        if (c.appearanceFlags & 0x08)
            w += bare * 2;
        if (c.appearanceFlags & 0x10)
            w += bare * 8;
    }
    if (actionFactor)
        switch (c.action) {
        case 87: w *= 50; break;
        case 91: w *= 2; break;
        case 93: w *= 5; break;
        case 96: w *= 3; break;
        default: break;
        }
    return w;
}

// §2.2: a(id).
float actionShare(int32_t id) {
    switch (id) {
    case 5: case 26: return 0.5f;
    case 21: case 72: return 0.1f;
    case 30: case 32: return 0.4f;
    case 31: case 33: return 0.6f;
    case 37: case 43: case 57: case 58: case 68: return 2;
    case 55: return 0.25f;
    default: return 1;
    }
}

glm::vec3 unit(glm::vec3 v) {
    const float l = glm::length(v);
    return l > 0 ? v / l : glm::vec3(0);
}

// Spends a proc buff (kind 10 for charged shots, 9 for big spells): it goes as a zero-length
// "event 40" record.
void spendBuff(Combatant &c, uint8_t kind, ShotEffects &out) {
    if (auto event = consumeBuff(c, kind))
        out.buffs.push_back(*event);
}

void gainBuff(Combatant &c, uint8_t kind, glm::dvec3 at, ShotEffects &out) {
    const Buff record{kind, 0, 30000};
    applyBuff(c, record);
    out.buffs.push_back({c.id, c.id, record});
    out.sounds.push_back({at, 47, 1, 1}); // upgrade2.wav
}

// §4 step 4: the chain ends; the action clock is rescaled by Sold / Snew (the ratio first,
// single precision, then truncated toward zero).
void endChain(Combatant &c) {
    const float before = attackSpeed(c);
    c.combo = 0;
    const float scaled = (before / attackSpeed(c)) * (float)c.actionMs;
    c.actionMs = scaled >= -2147483648.f && scaled < 2147483648.f ? (int32_t)scaled : INT32_MIN;
}

// §5.2 step 6, §6.4: a landed hit raises the shooter's combo and restarts its combo timer.
void continueChain(Combatant &c) {
    c.combo += 1;
    c.comboElapsedMs = 0;
}

bool solidAt(const BlockType &block, glm::dvec3 p) {
    const uint8_t t = block((int32_t)std::floor(p.x), (int32_t)std::floor(p.y), (int32_t)std::floor(p.z)) & 0x1f;
    return t != 0 && t != 2;
}

visibility::Position raw(const int64_t position[3]) { return {position[0], position[1], position[2]}; }

} // namespace

Projectile healingBurst(const Combatant &healer, glm::dvec3 at) {
    Projectile p;
    p.shooter = healer.id;
    p.kind = BURST;
    p.element = 2;
    p.damage = 0.1f * weaponDamage(healer, false);
    p.radius = 4;
    p.strength = 0;
    for (int k = 0; k < 3; k++)
        p.position[k] = (int64_t)(at[k] * 65536.0);
    return p;
}

Projectile rayCloud(const Combatant &healer, glm::dvec3 at) {
    Projectile p = healingBurst(healer, at);
    p.damage = magicBase(healer) * 0.1f;
    return p;
}

bool shootsProjectile(int32_t action) {
    switch (action) {
    case 22: case 23: case 26: case 50: case 55: case 96: case 89: case 90: case 108: return true;
    default: return action >= 37 && action <= 46;
    }
}

void chargeSound(const Combatant &c, int32_t before, int32_t dtMs, glm::dvec3 at, ShotEffects &out) {
    // alpha-ranged-action-scheduling.md: the dispatch sound reads the incoming
    // charge, while charging itself uses the already advanced action clock.
    if (hasChargeCue(c.action) && before / 200 != (before + dtMs) / 200)
        out.sounds.push_back({at, 55, 1 + 0.5f * c.charge, 1});
}

void advanceCharge(Combatant &c, float dtMs) {
    if (c.dodgeMs != 0) {
        const bool volley = (c.action == 50 || c.action == 96) && c.actionMs <= (float)duration(c, c.action);
        const bool assassin = c.characterClass == 4 && c.specialization == 1;
        const bool retained = assassin && (c.action == 5 || c.action == 20 || c.action == 17);
        if (!volley && !retained) {
            c.action = 0;
            if (!assassin)
                c.charge = 0;
        }
    }
    const float S = attackSpeed(c);
    if (c.action == 36) {
        c.charge += (dtMs * 0.0005f) * S;
    }
    switch (c.action) {
    case 8: case 24: case 25: case 27: case 59: case 63: case 64:
        if (c.actionMs >= (float)phases(c, c.action).windUp) {
            float q = (dtMs * 0.00075f) * S;
            if (c.characterClass == 2 && c.specialization == 0)
                q *= 1 + std::min((float)c.combo / comboMax(c), 4.f);
            if (c.hasBuff(2))
                q *= 1 + 9 * abilityValue(c, 102);
            c.charge += q;
            if (c.hasBuff(10))
                c.charge = c.mana;
        }
        break;
    default:
        break;
    }
    c.charge = std::min(c.charge, c.mana);
    // Movement's later gate observes the remaining stun, after this tick's gain.
    if (c.hitStun > 0)
        c.charge = 0, c.mode &= ~0x11u;
}

void stepCharge(Combatant &c, int32_t before, int32_t dtMs, glm::dvec3 at, ShotEffects &out) {
    chargeSound(c, before, dtMs, at, out);
    advanceCharge(c, (float)dtMs);
}

void actionSounds(const Combatant &c, float t, float dt, glm::dvec3 at, const std::function<int()> &draw,
                  std::vector<SoundEvent> &out) {
    const int32_t action = c.action;
    // These dispatch branches own their audio. The native melee lift/swing
    // branch does not surround ranged dispatch or its input-start helpers.
    if (shootsProjectile(action) || hasChargeCue(action))
        return;
    // (Using, sitting, riding and sailing are skipped here: ours, the spec's filter is not given.)
    if (action == 0 || (action >= 80 && action <= 84) || action == 106 || action == 107)
        return;
    const auto u = [&] { return (float)draw() / 32767.f; };
    const auto crosses = [&](float x) { return t <= x && x < t + dt; };
    const Phases P = phases(c, action);
    const float P1 = (float)P.windUp, P2 = (float)P.active;
    const Item &m = c.mainHand();
    const int s = m.subtype;
    const bool twoHanded = m.type == 3 && ((s >= 5 && s <= 8) || s == 10 || s == 11 || (s >= 15 && s <= 18));
    if (t <= 0 && twoHanded && action != 91 && !(action >= 30 && action <= 33) && action != 87 && action != 88)
        out.push_back({at, 17, 1 + 0.1f * u(), 1}); // the heavy weapon is lifted
    if (crosses(P1)) {
        if (action == 10)
            out.push_back({at, 16, 0.9f + 0.2f * u(), 1});
        else if (action == 11)
            out.push_back({at, 48, 0.9f + 0.2f * u(), 1});
        else if (action == 54)
            out.push_back({at, 14, 0.9f + 0.2f * u(), 1});
        else if (action != 91) {
            float pitch = (0.9f + 0.2f * u()) * 1.5f;
            if (twoHanded || action == 68 || action == 69 || action == 93)
                pitch *= 0.5f;
            out.push_back({at, 15, pitch, 1});
        }
        if (action == 94)
            out.push_back({at, 40, 1 + 0.1f * u(), 1});
        // (Action 104 is served by the default handler: its beam-block sounds 5/16 never play,
        // open points 3 §2.2.)
    }
    // Open points 3 §2.1: multi-hit actions swing again at every hit interval inside P2 (91 with
    // a punch too; 5 is silent).
    float interval = 0;
    switch (action) {
    case 72: case 86: interval = 250; break;
    case 91: interval = 1000; break;
    case 11: interval = std::floor(P2 / 3); break;
    case 30: case 32: interval = std::floor(P2 / 6); break;
    case 31: case 33: interval = std::floor(P2 / 12); break;
    default: break;
    }
    if (interval > 0 && P1 <= t && t < P1 + P2) {
        const auto k = [&](float x) { return std::trunc((x - P1) / interval); };
        if (t - dt < P1 || k(t + dt) != k(t)) {
            out.push_back({at, 15, 1 + 0.25f * u(), 1});
            if (action == 91)
                out.push_back({at, 82, 0.5f + 0.1f * u(), 1});
        }
    }
    if ((action == 57 || action == 58 || action == 60 || action == 74) && crosses(P1 + P2 / 2)) {
        out.push_back({at, 12, 1 + 0.25f * u(), 1});
        if (action == 74)
            out.push_back({at, 82, 0.5f + 0.1f * u(), 1});
    }
}

void launch(Combatant &c, const Body &body, glm::vec3 aim, float t, float dt, const std::function<int()> &draw,
            std::vector<Projectile> &shots, ShotEffects &out, const BlockType &block) {
    const auto u = [&] { return (float)draw() / 32767.f; };
    const auto crosses = [&](float x) { return t <= x && x < t + dt; };
    const Phases P = phases(c, c.action);
    const float P1 = (float)P.windUp, P2 = (float)P.active;
    const glm::dvec3 at = body.blocks();
    const bool aiming = glm::dot(aim, aim) > 0;
    Projectile p;
    p.shooter = c.id;
    std::memcpy(p.position, body.position, sizeof p.position);
    const int32_t action = c.action;

    // Particles §5.3, §5.4: a staff's channelling (actions 30-33) and a fire explosion's charge
    // (87, 88) hum every 200 ms and end, as the action's time passes its duration, in a burst:
    // at the aim point for a staff (water or fire), around the creature for the explosion.
    const bool staff = action >= 30 && action <= 33, explosion = action == 87 || action == 88;
    if (staff || explosion) {
        const float D = (float)duration(c, action);
        if (t < D && std::floor(t / 200) != std::floor((t + dt) / 200))
            out.sounds.push_back({at, 36, 1 + 0.25f * u(), 1});
        if (explosion && t - dt < D && D <= t) { // §5.4's own window, one step behind the staff's
            Burst b = Burst::at(at, {0, 0, 2}, glm::vec4(0), 0.25f, 80, FIRE);
            b.spread = 20;
            out.bursts.push_back(b);
            out.sounds.push_back({at, 81, 1, 1});
        } else if (staff && t < D && D <= t + dt) {
            // §5.3: at the creature plus the float vector at +0x160 (x 65536, truncated), spread 8.
            const bool water = c.specialization == 1;
            Burst b = Burst::at(glm::dvec3(0), {0, 0, 2}, water ? glm::vec4(0, 0.2f, 1, 1) : glm::vec4(0), 0.25f, 20,
                                water ? BRIGHT : FIRE);
            for (int k = 0; k < 3; k++)
                b.position[k] = body.position[k] + (int64_t)(c.aim[k] * 65536.f);
            b.spread = 8;
            out.bursts.push_back(b);
            out.sounds.push_back({at, water ? 41 : 38, 1, 1});
        }
    }

    // §2.6: the handler of actions 28, 94, 95 gives a healer (class 3, specialization 1) a
    // healing burst at its reach point when P1 is crossed: the attack origin plus the block ray
    // (limit 200) along the aim. Actions 28 and 95 place it in the beam tick (Npcs::beamTick).
    if (action == 94 && c.characterClass == 3 && c.specialization == 1 && crosses(P1)) {
        const visibility::Position origin = attackOrigin(body);
        const glm::vec3 dir = unit(aim);
        const float reach = aiming && block ? visibility::trace(origin, {dir.x, dir.y, dir.z}, 200.f, 0, 1, {block, {}}) : 0.f;
        Projectile heal = healingBurst(c, {});
        for (int k = 0; k < 3; k++)
            heal.position[k] = std::bit_cast<int64_t>(uint64_t(origin[k]) + uint64_t(int64_t(dir[k] * reach * WU)));
        shots.push_back(heal);
    }

    switch (action) {
    case 22: case 23: case 26: { // §2.3: bow, crossbow, boomerang
        if (crosses(P1)) {
            const float cost = manaCost(c, action);
            c.mana = std::max(0.f, c.mana - (cost > 0 ? cost : c.charge));
            c.spentCharge = c.charge;
            c.charge = 0;
        }
        const float s = c.spentCharge;
        if (!crosses(P1) && !(action == 26 && s > 0 && crosses(P1 + 300)))
            return;
        p.velocity = 100.f * unit(aim);
        p.kind = action == 26 ? BOOMERANG_SHOT : ARROW;
        if (action == 26) {
            p.radius = 2 * s + 2;
            p.strength = 0.2f * s;
            p.scale = 1; // the caller sets the thrower's weapon scale (appearance +0x38)
        } else {
            p.radius = 0.5f;
            p.strength = s;
        }
        const float cost = manaCost(c, action);
        p.cost = cost > 0 ? cost : p.strength;
        if (cost > 0)
            p.strength = cost;
        p.special = s > 0 || action == 23;
        if (p.special)
            spendBuff(c, 10, out);
        // σ × ((a × W) × (s² × K + B)); B reads the record's strength.
        const float K = action == 26 ? 1.f : 5.f;
        float B = p.strength == 0 ? 1.f : 0.1f;
        if (action == 23)
            B = 0;
        const float sigma = 1.25f - 0.5f * u();
        p.damage = sigma * ((actionShare(action) * weaponDamage(c, true)) * (s * s * K + B));
        shots.push_back(p);
        if (action == 26)
            out.sounds.push_back({at, 15, (float)(((double)draw() * 0.51) / 32767.0 + (double)0.7f), 1});
        else if (p.special)
            out.sounds.push_back({at, 22, 1 + 0.5f * u(), 1});
        else
            out.sounds.push_back({at, 15, 2 + 0.5f * u(), 1});
        return;
    }
    case 50: case 55: case 96: { // §2.4: retreat, salvo, shuriken
        if (crosses(P1)) {
            c.spentCharge = c.charge;
            c.mana -= c.charge; // the later common resource phase supplies its floor
            c.charge = 0;
            spendBuff(c, 10, out);
        }
        const float N = std::floor(P2 / 4);
        if (N <= 0 || t < P1 || t >= P1 + P2 || !aiming || std::floor((t + dt) / N) == std::floor(t / N))
            return;
        const float s = c.spentCharge;
        p.velocity = 150.f * unit(aim);
        p.strength = 0.25f * s;
        p.cost = 0;
        p.special = 1;
        // The volley spends the pitch draw before its damage draw. Single
        // physical shots use the opposite order in their separate branch.
        out.sounds.push_back({at, 22, 1 + 0.5f * u(), 1});
        const float sigma = 1.25f - 0.5f * u();
        p.damage = sigma * ((actionShare(action) * weaponDamage(c, true)) * (s * s * 5.f + (action == 55 ? 0.1f : 1.f)));
        shots.push_back(p);
        return;
    }
    case 89: case 90: { // §2.6: stationary bursts
        if (t < P1 && std::trunc(t / 200) != std::trunc((t + dt) / 200))
            out.sounds.push_back({at, 36, 1 + 0.25f * u(), 1});
        if (!crosses(P1))
            return;
        out.sounds.push_back({at, 38, 1 + 0.25f * u(), 1});
        p.kind = BURST;
        p.element = action == 89 ? 1 : 2;
        p.damage = actionShare(action) * weaponDamage(c, false) * 2;
        p.radius = 8;
        p.strength = 0;
        shots.push_back(p);
        return;
    }
    default:
        break;
    }
    if (!((action >= 37 && action <= 46) || action == 108))
        return;
    // §2.5: spells and the thrown rock.
    if (t <= P1 - 200 && std::trunc(t / 200) != std::trunc((t + dt) / 200))
        out.sounds.push_back({at, 36, 1 + 0.25f * u(), 1});
    // Anticipation spending is independent of candidate selection for repeated
    // magic: a selected boundary can reject emission after this cost is paid.
    if (crosses(P1) && aiming)
        c.mana = std::max(0.f, c.mana - manaCost(c, action));
    float F = P1;
    if (action == 45 || action == 46) {
        const float third = std::floor(P2 / 3);
        if (third > 0)
            F = std::max(P1, P1 + std::trunc((t + dt - P1) / third) * third);
    }
    if (!(t < P1 + P2 && crosses(F) && aiming))
        return;
    const float ch = c.charge;
    const bool bracelet = c.mainHand().type == 3 && c.mainHand().subtype == BRACELET;
    p.velocity = unit(aim) * (bracelet ? 100.f : 50.f);
    p.radius = 4 * ch + 0.5f;
    p.strength = ch;
    p.scale = ch + 0.5f;
    if (c.type == 101) { // raised by the box's y extent and half its z extent
        p.position[1] = std::bit_cast<int64_t>(uint64_t(p.position[1]) + uint64_t(int64_t(body.size.y * WU)));
        p.position[2] = std::bit_cast<int64_t>(uint64_t(p.position[2]) + uint64_t(int64_t(0.5f * body.size.z * WU)));
    }
    // Element for spells and the rock alike: class 3 water (specialization 1) or fire.
    if (c.characterClass == 3)
        p.element = c.specialization == 1 ? 2 : 1;
    const float sigma = 1.25f - 0.5f * u();
    if (action == 108) {
        p.kind = ROCK;
        p.velocity = 100.f * unit(aim);
        p.damage = actionShare(action) * weaponDamage(c, true) * sigma * 5;
        p.scale = 4;
        p.strength = 1;
    } else {
        p.kind = SPELL;
        p.damage = actionShare(action) * weaponDamage(c, false) * sigma;
        if (action == 45 || action == 46)
            p.strength = 0.25f, p.scale = ch + 1;
        else if (action == 37 || action == 43)
            p.strength = 1, p.scale = ch + 1.5f;
        p.special = action == 37 || action == 43 || action == 45 || action == 46;
    }
    shots.push_back(p);
    if (ch > 0) {
        c.mana = std::max(0.f, c.mana - ch);
        c.charge = 0;
    }
    if (p.special)
        spendBuff(c, 9, out);
    if (p.element == 2)
        out.sounds.push_back({at, 41, action == 41 || action == 42 || action == 44 ? 1.25f : 1.f, 0.9f});
    else
        out.sounds.push_back({at, 38, action == 38 || action == 39 || action == 40 ? 2.f : 1.f, 0.9f});
}

void Projectiles::step(int32_t dt, const std::vector<ShotTarget> &creatures,
                       const std::function<Combatant *(uint64_t)> &lookup, const BlockType &block,
                       const std::function<int()> &draw, ShotEffects &out, ProjectileContext context) {
    const auto u = [&] { return (float)draw() / 32767.f; };
    const float tau = (float)dt * 0.001f;
    const auto find = [&](uint64_t id) -> const ShotTarget * {
        for (const ShotTarget &t : creatures)
            if (t.fighter->id == id)
                return &t;
        return nullptr;
    };
    std::vector<bool> removed(list.size(), false);

    for (size_t index = 0; index < list.size(); index++) {
        Projectile &p = list[index];
        const ShotTarget *shooter = find(p.shooter);
        Combatant *mine = lookup ? lookup(p.shooter) : nullptr;
        Combatant *from = mine ? mine : shooter ? shooter->fighter : nullptr;
        const bool buffAuthority = mine && (mine == context.controlled || (!context.networkMode && mine->controller != 0));
        const bool resourceAuthority = mine && (!context.networkMode || mine == context.controlled);
        const int32_t ageBefore = p.age;

        // §4 step 1: velocity.
        if (p.kind == BOOMERANG_SHOT) {
            if (!shooter || p.age > 1200) {
                removed[index] = true;
                continue;
            }
            // Aim-triplet contract §3.6: fixed-point differences, each converted to binary32
            // (one rounding) and times 2^-16; outbound, the owner's aim is an offset point.
            const auto separation = [&](const int64_t offset[3]) {
                glm::vec3 out;
                for (int k = 0; k < 3; k++) {
                    const uint64_t to = uint64_t(shooter->body->position[k]) + uint64_t(offset[k]);
                    out[k] = float(std::bit_cast<int64_t>(to - uint64_t(p.position[k]))) * (1.f / 65536.f);
                }
                return out;
            };
            if (p.age > 1100) {
                // §4 step 1: the catch, scaled twice by F = trunc(f x 65536) in 16.16 fixed point
                // (each product divided by 65536 toward zero).
                const float f = (float)(p.age - 1100) / 100.f;
                const int64_t F = (int64_t)(f * 65536.f);
                for (int k = 0; k < 3; k++) {
                    const int64_t gap = std::bit_cast<int64_t>(uint64_t(shooter->body->position[k]) - uint64_t(p.position[k]));
                    p.position[k] += (gap * F / 65536) * F / 65536;
                }
            } else if (p.age > 800) {
                const int64_t none[3] = {0, 0, 0};
                p.velocity = separation(none) * 10.f;
            } else {
                const glm::vec3 aim = shooter->fighter->aim;
                const int64_t offset[3] = {int64_t(aim.x * 65536.f), int64_t(aim.y * 65536.f), int64_t(aim.z * 65536.f)};
                const glm::vec3 target = separation(offset) * 5.f;
                for (int32_t i = 0; i < dt; i++)
                    p.velocity += (target - p.velocity) * 0.05f;
            }
        } else if (p.kind != SPELL) {
            p.velocity.z -= tau * 30 * 0.25f; // gravity, 7.5 blocks/s²
        }
        // Steps 2-4: displacement, age, timeout.
        const glm::vec3 d = tau * p.velocity;
        p.age = std::bit_cast<int32_t>((uint32_t)p.age + (uint32_t)dt);
        if (p.kind != BOOMERANG_SHOT && p.age > 5000) {
            if (p.kind != BURST && mine)
                endChain(*mine);
            removed[index] = true;
            continue;
        }

        // §5: the flight in sub-steps (kind 3 too: n grows with its gravity-fed velocity).
        const int n = (int)(glm::length(d) * 2.f + 1.f);
        const glm::vec3 ds = d / (float)n;
        // k: the velocity, normalised only when not still, times 5c; then k.z := 3c, so a
        // still projectile pushes (0, 0, 3c).
        glm::vec3 knock = p.velocity;
        if (glm::dot(knock, knock) > 0)
            knock = unit(knock);
        knock *= 5.f * p.strength;
        knock.z = 3 * p.strength;
        const visibility::Scene scene{block, {}};
        // §5.1 step 2: the overlap in world units, half-sizes converted separately; all three
        // lower tests, then the strict upper ones.
        const auto overlaps = [&](const Body &b, float half) {
            const int64_t r = (int64_t)(half * WU);
            int64_t h[3];
            for (int k = 0; k < 3; k++)
                h[k] = (int64_t)(b.size[k] * 0.5f * WU);
            for (int k = 0; k < 3; k++)
                if (!(p.position[k] + r >= b.position[k] - h[k]))
                    return false;
            for (int k = 0; k < 3; k++)
                if (!(p.position[k] - r < b.position[k] + h[k]))
                    return false;
            return true;
        };
        // §5.1 step 4: a dodging creature joins the shooter's "evaded by" set (+0x11B4).
        const auto evaded = [&](uint64_t id) {
            if (from && std::find(from->deferred.begin(), from->deferred.end(), id) == from->deferred.end())
                from->deferred.push_back(id);
        };
        const bool secondTick = ageBefore / 1000 != p.age / 1000, tenthTick = ageBefore / 100 != p.age / 100;

        enum class HitPath { Contact, Impact };
        const auto hitCreature = [&](const ShotTarget &C, HitPath path) {
            const bool contact = path == HitPath::Contact;
            // The gain draws precede ownership of the eventual resource write.
            if (contact && from && (p.kind == ARROW || p.kind == BOOMERANG_SHOT) && !p.special) {
                const float twiceDraw = (float)draw() * 2.f;
                const float offset = 1.f - twiceDraw / 32767.f;
                float g = offset * .05f + .1f;
                if (p.kind == BOOMERANG_SHOT)
                    g *= 0.2f;
                g *= (float)duration(*from, from->action) / 300.f;
                if (draw() % 8 == 0)
                    g *= 2;
                if (resourceAuthority) {
                    mine->mana += g;
                    if (mine->mana > 1)
                        mine->mana = 1;
                }
            }
            const bool critical = from && u() < criticalChance(*from);
            float x = p.damage;
            if (critical)
                x *= 2.f;
            if (buffAuthority) {
                // Chance precedes the producer-only flag/class/element checks and
                // generic hit admission. Neither a failed producer nor a rejected
                // generic hit refunds these draws.
                if (contact) {
                    if (procChance(*mine, .15f, draw) && !p.special &&
                        mine->characterClass == 2 && mine->specialization == 1)
                        gainBuff(*mine, 10, shooter ? shooter->body->blocks() : p.blocks(), out);
                } else if (procChance(*mine, .25f, draw) && !p.special && p.kind == SPELL && p.element == 1) {
                    gainBuff(*mine, 9, shooter ? shooter->body->blocks() : p.blocks(), out);
                }
            }
            if (contact && x < 0)
                x = 0;
            if (contact && p.element == 2) {
                if (from && !hostile(*from, *C.fighter) && C.fighter->controller != 6) {
                    HitRecord h;
                    h.attacker = p.shooter;
                    h.target = C.fighter->id;
                    std::copy_n(C.body->position, h.position.size(), h.position.begin());
                    h.amount = std::bit_cast<float>(std::bit_cast<uint32_t>(x) ^ 0x80000000u);
                    if (from->controller == 1)
                        h.amount *= .5f;
                    h.critical = critical;
                    out.hits.push_back(h);
                }
                return false;
            }
            // The generic callee rejects this target after caller-side draws and
            // producer events have already happened.
            if (context.networkMode && C.fighter == context.controlled)
                return false;
            Combatant target = *C.fighter;
            glm::vec3 velocity = C.body->velocity;
            HitInput in;
            in.amount = x;
            in.selector = 0; // both native projectile call sites pass the full-width zero selector
            in.critical = critical;
            in.area = p.special != 0;
            in.magic = p.kind == SPELL;
            in.knockBack = knock;
            in.targetOnGround = (C.body->contact & ON_GROUND) != 0;
            in.targetPosition = p.blocks();
            std::copy_n(C.body->position, in.targetRecordPosition.size(), in.targetRecordPosition.begin());
            in.attackerPosition = shooter ? shooter->body->blocks() : p.blocks();
            if (shooter)
                std::copy_n(shooter->body->position, in.attackerRecordPosition.size(), in.attackerRecordPosition.begin());
            HitOutcome result = resolveHit(target, velocity, from, in, draw,
                [&](const HitEmission &emission) { out.hits.push_back(emission); });
            out.sounds.insert(out.sounds.end(), result.sounds.begin(), result.sounds.end());
            out.bursts.insert(out.bursts.end(), result.bursts.begin(), result.bursts.end());
            return result.hitAccepted;
        };
        // §5.1 step 1, §6.4: eligibility. With no shooter (or a water burst's contact) no test
        // runs; otherwise not the shooter and "can damage" (0x1988a0, owner first).
        const auto eligible = [&](const ShotTarget &C, HitPath path) {
            if (!from || (path == HitPath::Contact && p.kind == BURST && p.element == 2))
                return true;
            return C.fighter->id != p.shooter && hostilityTest(*from, *C.fighter);
        };
        // §5.1 step 3: line of sight 0x19ee90 to the creature's position, option 1, limit 200.
        const auto sight = [&](const ShotTarget &C) {
            return visibility::visible(raw(p.position), raw(C.body->position), 1, 200.f, scene);
        };

        for (int i = 0; i < n; i++) {
            bool landed = false, destroy = false, wall = false; // reset every sub-step
            for (const ShotTarget &C : creatures) {
                if (C.fighter->health <= 0 || !eligible(C, HitPath::Contact) || !overlaps(*C.body, p.radius) ||
                    !sight(C))
                    continue;
                if (C.fighter->dodgeMs != 0) {
                    evaded(C.fighter->id);
                    continue;
                }
                bool gate;
                switch (p.kind) {
                case BURST: gate = secondTick; break;
                case BOOMERANG_SHOT: gate = p.age > 300 && tenthTick; break;
                case ARROW: case SPELL: case ROCK: gate = !p.special; break;
                default: gate = false; break;
                }
                if (gate && hitCreature(C, HitPath::Contact))
                    landed = true;
                if (p.kind != BOOMERANG_SHOT && p.kind != BURST) {
                    destroy = true;
                    break;
                }
            }
            if (p.kind == BURST) // §5.2: a burst never moves; its sub-steps only scan creatures
                continue;
            // §5.2: movement and blocks.
            if (p.kind == BOOMERANG_SHOT && ageBefore / 200 != p.age / 200)
                out.sounds.push_back({p.blocks(), 15, 1 + 0.25f * u(), 1});
            const auto move = [&](float sign) {
                for (int k = 0; k < 3; k++)
                    p.position[k] += (int64_t)(sign * ds[k] * WU);
            };
            move(1);
            if (solidAt(block, p.blocks())) {
                const glm::dvec3 hit = p.blocks();
                move(-1);
                wall = true;
                if (p.kind == ARROW) // particles §5.7: only an arrow inside a solid block
                    out.struck.push_back({p.blocks(), glm::ivec3(glm::floor(hit))});
            } else {
                // Step 4: the block ray 0x1a35d0 along the velocity, limit |velocity| x tau.
                const glm::vec3 dir = unit(p.velocity);
                const float limit = glm::length(p.velocity) * tau;
                if (limit > visibility::trace(raw(p.position), {dir.x, dir.y, dir.z}, limit, 0, 0, scene)) {
                    move(-1);
                    wall = true;
                }
            }
            if (wall && !destroy && mine)
                endChain(*mine);
            if (p.kind == BOOMERANG_SHOT) {
                if (landed && mine)
                    continueChain(*mine);
                if (wall)
                    p.velocity = glm::vec3(0);
                continue;
            }
            if (!destroy && !wall)
                continue;

            // §6.4: the end of the flight.
            if (p.kind == SPELL || p.special) {
                if (!destroy) {
                    const int32_t sound = p.kind == SPELL ? (p.element == 2 ? 42 : 39) : 20;
                    const float pitch = (float)draw() * .4f / 32767.f + 1.f;
                    out.sounds.push_back({p.blocks(), sound, pitch, 1});
                }
                const bool skipCandidates = from && !context.networkMode && from->controller == 0 &&
                                            from != context.controlled;
                // The burst: a 5-block half-size cube; health and dodging count only with a shooter.
                for (const ShotTarget &C : creatures) {
                    if (skipCandidates)
                        break;
                    if ((from && C.fighter->health <= 0) || !eligible(C, HitPath::Impact) || !overlaps(*C.body, 5.f) ||
                        !sight(C))
                        continue;
                    if (from && C.fighter->dodgeMs != 0) {
                        evaded(C.fighter->id);
                        continue;
                    }
                    if (hitCreature(C, HitPath::Impact))
                        landed = true;
                }
            }
            if (landed && mine)
                continueChain(*mine);
            if (p.kind == SPELL && p.element == 2) { // a water spell leaves a puddle
                p.kind = BURST;
                p.age = p.special ? 0 : 3000;
                p.damage *= 0.05f;
                p.radius = p.strength * 5.f + 5.f;
                p.strength = 0;
                p.special = 0;
            } else {
                removed[index] = true;
            }
            break;
        }
    }
    size_t w = 0;
    for (size_t i = 0; i < list.size(); i++)
        if (!removed[i])
            list[w++] = list[i];
    list.resize(w);
}

} // namespace creatures
