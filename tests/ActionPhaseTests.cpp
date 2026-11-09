// Common phase ordering from the held original-only update contract.
#include "creatures/Abilities.h"
#include "creatures/Strike.h"
#include "creatures/Projectiles.h"
#include "creatures/Npcs.h"
#include "net/Protocol.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace creatures;
namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}
}

int main() {
    {
        Combatant first, second;
        second.controller = 1;
        check(hostile(first, second) && hostile(second, first),
              "one role-one operand makes the relationship predicate true in either direction");
        first.controller = 5;
        second.controller = 0;
        first.mode = 0x20;
        check(!hostile(first, second) && hostile(second, first),
              "ordered role-five to role-zero exclusion precedes the final flag test");
        second.controller = 6;
        check(!hostile(first, second) && !hostile(second, first),
              "either role-six operand excludes the relationship before flags");
    }
    {
        Combatant actor;
        actor.equipment[MAIN_HAND] = {3, FIST};
        const auto saved = itemBonuses().speed;
        itemBonuses().speed = [](const Item &) { return .001f; };
        check(std::bit_cast<uint32_t>(attackSpeed(actor)) == 0x3f81ba5e,
              "item haste accumulates with its normalized base before adding the class/combo speed term");
        itemBonuses().speed = saved;
    }
    {
        Combatant actor;
        actor.equipment[OFF_HAND] = actor.equipment[MAIN_HAND] = {3, FIST};
        actor.equipment[MAIN_HAND].seed = 1;
        const auto saved = itemBonuses().critical;
        std::vector<uint32_t> order;
        itemBonuses().critical = [&](const Item &item) {
            order.push_back(item.seed);
            return item.seed == 0 ? 100000000.f : -100000000.f;
        };
        check(criticalChance(actor) == 0 && order == std::vector<uint32_t>({0, 1}),
              "critical stat adds ordered contributions against its initial base instead of rounding a separate subtotal");
        itemBonuses().critical = saved;
    }
    for (int offset : {-1, 0, 1}) {
        Combatant actor;
        actor.action = 1;
        actor.actionMs = phases(actor, 1).windUp + offset;
        actor.struck = {12};
        actor.hitLanded = true;
        beginActionUpdate(actor, 0);
        check(actor.hitLanded == (offset > 0) && actor.struck.empty() == (offset <= 0),
              "common processing clears its target collection and marker through inclusive anticipation");
    }
    for (int lock : {-1, 0, 1}) {
        Combatant actor;
        actor.action = 1;
        actor.combo = 4;
        const Phases timing = phases(actor, 1);
        actor.actionMs = timing.windUp + timing.active + 1;
        actor.dodgeMs = (float)lock;
        beginActionUpdate(actor, 0);
        check(actor.combo == (lock == 0 ? 0 : 4),
              "unprocessed common action resets its combo before dispatch even after the middle boundary, only with zero lock");
    }
    {
        Combatant actor;
        actor.action = 3;
        actor.mana = 1;
        actor.charge = .2f;
        actor.hitLanded = true;
        beginActionUpdate(actor, 0);
        check(std::abs(actor.mana - .7f) < 1e-6f && actor.spentPower == .2f && actor.charge == 0 && !actor.hitLanded,
              "positive pending amount overwrites the recorded nominal cost while debiting both amounts");
        actor.action = 5;
        actor.actionMs = 0;
        actor.mana = .7f;
        actor.charge = .4f;
        beginActionUpdate(actor, 0);
        check(actor.mana == 0 && actor.spentPower == .2f && actor.charge == 0,
              "action five copies mana first, then pending finalization records half the full pending debit");
        actor.action = 1;
        actor.actionMs = phases(actor, 1).windUp + 1;
        actor.mana = -0.f;
        actor.charge = -.5f;
        actor.spentPower = .8f;
        actor.hitLanded = true;
        beginActionUpdate(actor, 0);
        check(actor.charge == 0 && !std::signbit(actor.charge) && actor.spentPower == .8f &&
              actor.hitLanded && std::signbit(actor.mana),
              "nonpositive pending amount is cleared without spending, overwriting or clearing state; equal negative-zero mana survives");
    }
    {
        Combatant actor;
        actor.action = 33;
        actor.mana = .9f;
        actor.spentPower = .8f;
        actor.criticalLatched = true;
        beginActionUpdate(actor, 0);
        check(actor.mana == .9f && actor.spentPower == .8f && actor.criticalLatched,
              "charge action at zero clock preserves prior recorded amount and latch without an anticipation crossing");
        const int anticipation = phases(actor, 33).windUp;
        actor.actionMs = anticipation - 1;
        beginActionUpdate(actor, 1);
        check(actor.mana == .9f && actor.spentPower == .8f,
              "projected equality at charge anticipation does not debit mana");
        actor.actionMs = anticipation;
        beginActionUpdate(actor, 1);
        check(std::abs(actor.mana - .6f) < 1e-6f && actor.spentPower == .1f && actor.criticalLatched,
              "crossing out of inclusive charge anticipation records point-one and debits its distinct nominal cost");
        actor.mana = .9f;
        actor.buffs.push_back({9, 0, 0});
        beginActionUpdate(actor, 1);
        check(actor.mana == .9f && actor.spentPower == .1f && actor.hasBuff(9),
              "resident type nine makes the charge debit zero without changing its recorded point-one amount");
    }
    check(meleeAction(72) && !meleeAction(73) && meleeAction(74),
          "common-action membership excludes action 73 between its neighboring dispatch cases");
    {
        Combatant actor;
        actor.action = 1;
        actor.spentPower = .7f;
        Body body, target;
        target.position[0] = 65536;
        int draws = 0;
        const auto hit = strikeInput(actor, body, target, [&] { draws++; return 32767; });
        check(draws == 2 && !hit.critical && std::bit_cast<uint32_t>(hit.amount) == 0x40259999,
              "common physical damage squares charge before multiplying by five at the exact binary32 boundary");
        actor.action = 87;
        const auto explosion = strikeInput(actor, body, target, [] { return 32767; });
        check(explosion.amount == .75f && !explosion.magic && strikeBase(actor) == 50.f,
              "explosion selects magic-stat damage without forwarding the 30-33 magic flag or physical multiplier");
    }
    {
        Combatant actor;
        actor.action = 1;
        Body body, target;
        body.position[0] = (INT64_C(1) << 62) + 1;
        target.position[0] = body.position[0] + 201;
        const auto hit = strikeInput(actor, body, target, [] { return 32767; });
        const float expected = (201.f / 65536.f) * (6.f / attackSpeed(actor));
        check(hit.knockBack.x == expected && hit.knockBack.x > 0,
              "nearby raw offsets survive subtraction at coordinates beyond binary64 unit precision");
        body.position[0] = INT64_MAX - 100;
        target.position[0] = INT64_MIN + 100;
        const auto wrapped = strikeInput(actor, body, target, [] { return 32767; });
        check(wrapped.knockBack.x == expected,
              "fixed-point displacement preserves native 64-bit subtraction bits without signed overflow");
    }
    {
        Combatant actor;
        actor.action = 17;
        actor.equipment[OFF_HAND] = actor.equipment[MAIN_HAND] = {3, FIST};
        const auto saved = itemBonuses().damage;
        std::vector<int> order;
        itemBonuses().damage = [&](const Item &) { order.push_back(2); return 1.f; };
        int count = 0;
        strikeInput(actor, Body{}, Body{}, [&] { order.push_back(++count == 1 ? 1 : 3); return 32767; });
        check(order == std::vector<int>({1, 2, 2, 3}),
              "critical decision precedes both selected item-stat queries and the final variation draw");
        actor.multipliers[2] = 16777216.f;
        check(magicBase(actor) == 16777216.f,
              "selected equipment contributions round sequentially against the accumulated creature damage");
        itemBonuses().damage = saved;
    }
    {
        Combatant actor;
        actor.controller = 1;
        actor.level = 3;
        const float base = attackSpeed(actor);
        actor.buffs.push_back({12, 0, 0});
        const float rankTwoFactor = std::bit_cast<float>(uint32_t(0x3f955556));
        check(attackSpeed(actor) == base * rankTwoFactor,
              "resident haste uses integer level-half fallback and the separately rounded skill-plus-one factor");
    }
    {
        Combatant actor;
        actor.action = 1;
        actor.combo = 1;
        const int ready = phases(actor, 1).windUp + phases(actor, 1).active;
        actor.actionMs = ready - 1;
        actor.mana = -.25f;
        actor.hitLanded = actor.criticalLatched = true;
        actor.buffs.push_back({9, 2, 0, 91});
        check(startPrimaryAttack(actor) && actor.action == 1 && actor.actionMs == ready - 1 &&
              actor.combo == 1 && actor.mana == -.25f && actor.hitLanded && actor.criticalLatched &&
              actor.buffs[0].timeMs == 0 && actor.buffs[0].source == 91,
              "busy primary acceptance leaves action, resources, bookkeeping and resident records unchanged");
        actor.actionMs = ready;
        check(startPrimaryAttack(actor) && actor.action == 6 && actor.actionMs == 0 &&
              actor.combo == 1 && actor.mana == -.25f && actor.hitLanded && actor.criticalLatched &&
              actor.buffs[0].timeMs == 0 && actor.buffs[0].source == 91,
              "primary readiness equality writes only the selected action and zero clock without a mana gate");
    }
    for (int lock : {-600, -1, 1, 600}) {
        Combatant actor;
        actor.characterClass = 4;
        actor.specialization = 1;
        actor.actionMs = 10000;
        actor.dodgeMs = (float)lock;
        actor.equipment[MAIN_HAND] = {3, LONGSWORD};
        check(!startPrimaryAttack(actor) && actor.action == 0 && actor.actionMs == 10000 && actor.dodgeMs == lock,
              "every nonzero roll rejects primary input with no assassin exception or mutation");
    }
    {
        Combatant actor;
        actor.action = 48;
        actor.actionMs = 10000;
        check(!startPrimaryAttack(actor) && actor.action == 48 && actor.actionMs == 10000,
              "old action 48 rejects primary input even after its duration");
    }
    for (int action : {30, 32}) {
        Combatant actor;
        actor.action = action;
        actor.combo = -1;
        const int full = duration(actor, action);
        actor.actionMs = full - 1;
        check(startPrimaryAttack(actor) && actor.action == action && actor.actionMs == full - 1,
              "old primary staff actions retain full recovery despite a nonzero combo counter");
        actor.actionMs = full;
        check(startPrimaryAttack(actor) && actor.action == 6 && actor.actionMs == 0,
              "old primary staff action becomes ready at its inclusive full duration");
    }
    {
        Combatant actor;
        actor.equipment[MAIN_HAND] = {4, DAGGER};
        check(primaryAttack(actor) == 19, "raw dagger subtype wins without a weapon category");
        actor.equipment[MAIN_HAND].type = 0;
        check(primaryAttack(actor) == 19, "raw dagger precedes the empty-category punch rule");
        actor.equipment[MAIN_HAND] = {4, FIST};
        check(primaryAttack(actor) == 6, "raw fist subtype selects punching with a nonweapon category");
        actor.equipment[MAIN_HAND] = {4, LONGSWORD};
        check(primaryAttack(actor) == 14, "raw longsword subtype selects its chain without a weapon category");
        actor.equipment[MAIN_HAND].type = 0;
        check(primaryAttack(actor) == 6, "empty category precedes raw longsword selection");
        actor.equipment[MAIN_HAND] = {4, 0};
        check(primaryAttack(actor) == 1, "a nonempty nonweapon category reaches the default chain");
        actor.equipment[OFF_HAND] = {3, SHIELD};
        check(primaryAttack(actor) == 10, "off-hand shield precedes the default chain");
        actor.equipment[MAIN_HAND] = {4, DAGGER};
        actor.equipment[OFF_HAND] = {3, BOW};
        check(primaryAttack(actor) == 22, "a valid off-hand bow precedes raw dagger selection");
        actor.characterClass = 3;
        actor.equipment[MAIN_HAND] = {4, WAND};
        check(primaryAttack(actor) == 38, "class-based magic selection uses the raw wand subtype");
        actor.specialization = 1;
        check(primaryAttack(actor) == 44, "magic specialization applies to a raw nonweapon wand subtype");
        actor.equipment[MAIN_HAND].subtype = STAFF;
        check(primaryAttack(actor) == 32, "class-based magic selection uses the raw staff subtype");
    }
    {
        Combatant actor;
        actor.equipment[MAIN_HAND] = {3, GREATSWORD};
        actor.action = 57;
        const int end = duration(actor, 57) + 200;
        actor.actionMs = end - 1;
        check(primaryAttack(actor) == 67, "primary combo remains open one millisecond before its boundary");
        actor.actionMs = end;
        check(primaryAttack(actor) == 57, "primary combo window excludes equality");
        check(startPrimaryAttack(actor) && actor.action == 57 && actor.actionMs == 0,
              "a ready same-action result restarts its clock");
    }
    {
        Combatant actor;
        actor.equipment[MAIN_HAND] = {4, BRACELET};
        const float speed = attackSpeed(actor) * actor.multipliers[1];
        for (int action : {37, 43, 89})
            check(phases(actor, action).windUp == (int)(600.f / speed),
                  "bracelet anticipation depends on the raw subtype without a weapon-category test");
        const Phases staff = phases(actor, 30);
        check(staff.windUp == (int)(800.f / speed) && staff.active == 600 &&
              staff.recovery == (int)(100.f / speed) &&
              staff.total() == (int)(800.f / speed) + 600 + (int)(100.f / speed),
              "timing truncates each scaled component separately and retains the fixed middle contribution");
    }
    {
        Combatant actor;
        actor.controller = 1;
        actor.characterClass = 0;
        actor.multipliers[0] = 16777216.f;
        actor.equipment[OFF_HAND].type = actor.equipment[MAIN_HAND].type = 3;
        actor.equipment[CHEST_SLOT].type = 4;
        actor.equipment[BOOTS_SLOT].type = 6;
        actor.equipment[GLOVES_SLOT].type = 5;
        actor.equipment[SHOULDERS_SLOT].type = 7;
        check(std::bit_cast<uint32_t>(maxHealth(actor)) == 0x4b800010,
              "maximum health rounds each of six equipment additions, rather than adding a rounded subtotal");
    }
    for (int32_t count : {-5, 33}) {
        std::array<uint8_t, 280> record{};
        record[0] = 3;
        std::memcpy(record.data() + 0x114, &count, sizeof(count));
        const Item item = Item::parse(record.data());
        check(item.spirits == count && item.cubes.size() == (count < 0 ? 0u : 32u),
              "item stats retain the raw signed count while payload decoding stays inside the fixed record");
    }
    for (int scenario = 0; scenario < 6; scenario++) {
        Combatant source;
        source.id = scenario == 4 ? 0 : 42;
        source.type = scenario < 2 ? 25 : 2;
        source.characterClass = scenario == 0 || scenario == 4 ? 3 : 1;
        source.specialization = scenario == 2 || scenario == 3 ? 1 : 0;
        if (scenario == 2)
            source.equipment[MAIN_HAND] = {3, GREATSWORD};
        if (scenario == 3)
            source.equipment[OFF_HAND] = {3, SHIELD};
        Body body;
        body.place({0, 0, 3});
        Npcs npcs;
        Npc target;
        target.fighter.id = 2;
        target.fighter.controller = 1;
        target.body.place({0, 1, 3});
        target.home = target.body.blocks();
        target.zone = {0, 0};
        npcs.npcs().push_back(target);
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        HitRecord hit;
        hit.attacker = scenario == 5 ? 99 : source.id;
        hit.target = 2;
        hit.amount = 1;
        npcs.step(0, map, {{source, body, 0, 0, false}}, 43200000, 0,
            [](int32_t, int32_t, int32_t z) { return (uint8_t)(z <= 0 ? 1 : 0); }, {hit});
        const auto &threats = npcs.all()[0].threats;
        if (scenario == 5)
            check(threats.empty(), "an unresolved source creates no hit threat entry");
        else {
            const float expected = scenario == 0 || scenario == 4 ? 1.01f : scenario == 1 ? 10.01f :
                scenario == 2 ? 20.01f : 2.01f;
            check(threats.at(source.id) == expected,
                  "incoming hit uses resolved player class, weapon and numeric source-zero identity in native precedence");
        }
    }
    {
        Combatant actor;
        actor.actionMs = 16777216;
        stepAction(actor, 1);
        check((int64_t)actor.actionMs == 16777217,
              "stored action time keeps one-millisecond increments beyond the binary32 integer limit");
        net::CreatureRecord record;
        record.setFighter(actor);
        check((int64_t)record.fighter().actionMs == 16777217,
              "large action clocks preserve their native signed integer value on the wire");
    }
    {
        Combatant source, target;
        source.id = 77;
        source.action = 17;
        glm::vec3 velocity(0);
        HitInput input;
        input.amount = 10;
        input.selector = 17;
        input.blockable = false;
        resolveHit(target, velocity, &source, input, [] { return 0; });
        check(target.buffs.size() == 1 && target.buffs[0].kind == DAMAGE_OVER_TIME && target.buffs[0].source == 77,
              "a generated periodic buff retains the attacker's source identifier");
    }
    {
        Combatant actor;
        actor.powerTier = 8;
        actor.appearanceFlags = 0x20;
        check(resistance(actor) == 6.f,
              "role-zero resistance replaces only its base, retaining the rank-scaled flag bonus");
        actor.controller = 1;
        check(resistance(actor) == 8.f, "other roles use the rank-scaled base and flag bonus");
    }
    {
        Combatant actor;
        actor.multipliers[1] = .999f;
        check(comboMax(actor) == 49, "combo divisor truncates the scaled native maximum to an integer");
        actor.multipliers[1] = 1;
        actor.appearanceFlags = 0x10;
        check(comboMax(actor) == 20, "unarmed combo maximum uses the appearance flag");
        actor.equipment[MAIN_HAND] = {3, STAFF};
        check(comboMax(actor) == 50, "a weapon's default maximum overrides the unarmed flag rule");
        actor.equipment[MAIN_HAND].subtype = BOW;
        check(comboMax(actor) == 60, "the bow maximum retains its explicit weapon rule");
    }
    for (int action : {1, 28}) {
        Combatant actor;
        actor.id = 1;
        actor.action = action;
        actor.multipliers[2] = 10; // Keep the ordering fixture above target mitigation for every roll.
        const Phases timing = phases(actor, action);
        actor.actionMs = (float)(timing.windUp + (action == 28 ? 199 : timing.active / 2));
        actor.hitStun = 100;
        actor.aim = {0, 1, 0};
        Body body;
        body.place({0, 0, 3});
        Npcs npcs;
        Npc target;
        target.fighter.id = 2;
        target.fighter.controller = 1;
        target.fighter.health = maxHealth(target.fighter);
        target.body.place({0, 1, 3});
        target.home = target.body.blocks();
        target.zone = {0, 0};
        npcs.npcs().push_back(target);
        PlayerView player{actor, body, 0, actor.actionMs};
        const BlockType floor = [](int32_t, int32_t, int32_t z) { return (uint8_t)(z <= 0 ? 1 : 0); };
        NpcFrame frame;
        beginActionUpdate(actor, 16);
        npcs.dispatchPlayerActions({&player, 1}, 16, floor, frame);
        if (frame.seen.empty() || npcs.all()[0].fighter.health >= target.fighter.health) {
            std::printf("action %d: %zu hits, health %.9g -> %.9g\n", action, frame.seen.size(),
                        target.fighter.health, npcs.all()[0].fighter.health);
            for (const auto &event : frame.seen)
                std::printf("  kind %u, amount %.9g\n", (unsigned)event.first.kind, event.first.amount);
        }
        check(!frame.seen.empty() && npcs.all()[0].fighter.health < target.fighter.health,
              "local melee and beam dispatch hit before a surviving stun interrupts the action");
        endActionUpdate(actor, 16);
        check(actor.action == 0 && actor.hitStun == 84, "common tail follows local hit effects");
        const size_t events = frame.seen.size();
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        player.strikes = false;
        frame = npcs.step(0, map, {player}, 43200000, 0, floor, {}, std::move(frame));
        check(frame.seen.size() == events, "later NPC simulation carries earlier events without repeating local attacks");
    }
    for (int lock : {9, 10, 11}) {
        Combatant actor;
        actor.action = 22;
        actor.actionMs = 17;
        actor.actionLockMs = lock;
        beginActionUpdate(actor, 10);
        check(actor.actionLockMs == std::max(0, lock - 10) &&
              actor.actionMs == (lock > 10 ? (float)duration(actor, 22) + 1 : 17),
              "only a strictly surviving action lock overrides the dispatch clock");
    }
    {
        Combatant actor;
        actor.action = 37;
        actor.characterClass = 3;
        actor.mana = .7f;
        actor.actionMs = 0;
        beginActionUpdate(actor, 16);
        check(actor.mana == .7f, "projectile mana is not spent by generic zero-clock initialization");
        endActionUpdate(actor, 16);
        check(actor.mana == .7f && actor.actionMs == 16, "active caster resource suppression uses the advanced clock");
    }
    for (int stun : {15, 16, 17}) {
        Combatant actor;
        actor.action = 22;
        actor.actionMs = 200;
        actor.hitStun = stun;
        beginActionUpdate(actor, 16);
        check(actor.action == 22 && actor.hitStun == stun, "pending action dispatch sees the incoming stun");
        endActionUpdate(actor, 16);
        check(actor.hitStun == stun - 16 && actor.action == (stun > 16 ? 0 : 22) &&
              actor.actionMs == (stun > 16 ? 16 : 216), "stun equality preserves action; positive remainder interrupts before increment");
    }
    for (int lifetime : {16, 17}) {
        Combatant actor;
        actor.action = 22;
        actor.actionMs = 200;
        actor.hitStun = 100;
        actor.buffs.push_back({1, .5f, lifetime});
        stepAction(actor, 16);
        check(actor.hitStun == (lifetime > 16 ? -16 : 84) && actor.action == (lifetime > 16 ? 22 : 0),
              "only a surviving type-one buff clears stun before its countdown");
    }
    {
        Combatant actor;
        actor.action = 54;
        actor.actionMs = 0;
        actor.skills[6] = 2;
        actor.cooldowns[54] = 10;
        stepAction(actor, 16);
        check(actor.cooldowns[54] == cooldownLength(actor, 54), "new cooldown initialization follows existing-entry decay");
        actor.actionMs = 100;
        stepAction(actor, 16);
        check(actor.cooldowns[54] == cooldownLength(actor, 54) - 16, "existing cooldown entry subsequently decays");
    }
    for (int lifetime : {1, 400, 401}) {
        Combatant actor;
        actor.actionMs = 100;
        actor.buffs = {{1, .5f, 1}, {4, 2, lifetime, 77}};
        int pulses = 0;
        endActionUpdate(actor, 1, {}, [&](const Buff &buff) {
            pulses++;
            check(buff.source == 77 && actor.hasBuff(1), "periodic callback retains source and can observe pending expired nodes");
        });
        check(pulses == (lifetime == 400 ? 1 : 0), "periodic quotient boundary requires the buff to survive");
        check(!actor.hasBuff(1), "expired nodes are removed before leaving the phase");
    }
    {
        Combatant actor;
        actor.actionMs = 100;
        actor.combo = 3;
        actor.comboElapsedMs = 3999;
        endActionUpdate(actor, 1);
        check(actor.combo == 3, "combo timeout excludes equality at four seconds");
        endActionUpdate(actor, 1);
        check(actor.combo == 0, "combo timeout clears only beyond four seconds");
        actor.combo = 3;
        actor.comboElapsedMs = 5000;
        endActionUpdate(actor, 1, {false, true});
        check(actor.combo == 3 && actor.comboElapsedMs == 5000, "replica-other skips common combo timeout");
    }
    for (bool shieldFirst : {false, true}) {
        Combatant actor;
        actor.actionMs = 100;
        const Buff shield{6, 2, 1000}, periodic{4, 2, 400};
        actor.buffs = shieldFirst ? std::vector<Buff>{shield, periodic} : std::vector<Buff>{periodic, shield};
        endActionUpdate(actor, 1, {}, [&](const Buff &) {
            for (Buff &buff : actor.buffs)
                if (buff.kind == 6)
                    buff.value = buff.timeMs = 0;
        });
        check(actor.hasBuff(6) == shieldFirst,
              "payload depletion removes a shield this tick only when expiration traversal has not visited it yet");
        endActionUpdate(actor, 1);
        check(!actor.hasBuff(6), "a retained depleted shield expires on the following tick");
    }
    {
        Combatant actor;
        actor.action = 8;
        actor.actionMs = 100;
        actor.blockGauge = 1;
        actor.equipment[OFF_HAND] = {3, SHIELD};
        stepAction(actor, 600);
        check(std::abs(actor.blockGauge - .5f) < 1e-6f, "shield blocking drains over twelve hundred milliseconds");
        actor.actionLockMs = 601;
        stepAction(actor, 600);
        check(std::abs(actor.blockGauge - .8f) < 1e-6f, "surviving action lock disables blocking and permits recovery");
    }
    {
        Combatant actor;
        actor.action = 24;
        actor.actionMs = 200;
        actor.dodgeMs = 10;
        actor.charge = .4f;
        actor.mana = 1;
        advanceCharge(actor, 16);
        check(actor.action == 0 && actor.charge == 0 && actor.actionMs == 200, "ordinary roll cancellation preserves clock and clears action/charge");
        actor.characterClass = 4;
        actor.specialization = 1;
        actor.action = 24;
        actor.charge = .4f;
        advanceCharge(actor, 16);
        check(actor.action == 0 && actor.charge == .4f, "assassin cancellation retains charge");
        actor.action = 5;
        advanceCharge(actor, 16);
        check(actor.action == 5 && actor.charge == .4f, "assassin's exempt action survives the roll gate");
    }
    {
        Combatant actor;
        actor.comboElapsedMs = 123;
        actor.actionLockMs = 456;
        actor.slowMs = 789;
        actor.hasteMs = 321;
        actor.recoveryPenalty = -.5f;
        net::CreatureRecord record;
        record.setFighter(actor);
        const auto parsed = record.fighter();
        check(parsed.comboElapsedMs == 123 && parsed.actionLockMs == 456 && parsed.slowMs == 789 &&
              parsed.hasteMs == 321 && parsed.recoveryPenalty == -.5f, "common timers retain their existing wire field widths");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
