// Bounded scheduling contracts; no reference-game instructions or assets.
#include "creatures/Projectiles.h"
#include "creatures/Npcs.h"
#include <cmath>
#include <cstdio>

using namespace creatures;
namespace {
int failures = 0;
void check(bool passed, const char *description) {
    if (!passed) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}

Combatant ranger(int action) {
    Combatant result;
    result.characterClass = 2;
    result.equipment[MAIN_HAND] = {3, BOW, 1, 0, 1, 0};
    result.action = action;
    result.mana = 1;
    return result;
}

struct Collision {
    Combatant source, target;
    Body sourceBody, targetBody;
    Projectiles world;
    ShotEffects effects;

    Collision() {
        source.id = 41;
        source.characterClass = 3;
        sourceBody.place({20, 0, 0});
        target.id = 72;
        target.controller = 1;
        Projectile shot;
        shot.shooter = source.id;
        shot.kind = SPELL;
        shot.element = 1;
        shot.damage = 10;
        world.list.push_back(shot);
    }
    void step(const std::function<int()> &draw, ProjectileContext context) {
        world.step(1, {{&source, &sourceBody}, {&target, &targetBody}},
            [&](uint64_t id) { return id == source.id ? &source : nullptr; },
            [](int32_t, int32_t, int32_t) { return uint8_t(0); }, draw, effects, context);
    }
};
}

int main() {
    {
        Collision c;
        c.source.action = 17;
        const int values[] = {32767, 0, 12345, 32767, 0, 32767};
        int draws = 0;
        c.step([&] {
            if (draws == 5)
                check(c.source.hasBuff(9) && c.effects.buffs.size() == 1,
                      "impact proc is resident and queued before generic hit draws");
            return draws < 6 ? values[draws++] : (++draws, 32767);
        }, {false, &c.source});
        check(draws == 6 && c.effects.hits.size() == 2 && c.effects.buffs.size() == 1 &&
              c.effects.buffs[0].record.kind == 9,
              "contact and impact each draw critical then their own producer roll before generic resolution");
        check(c.source.buffs.size() == 1 && c.source.buffs[0].kind == 9,
              "a wrong-class contact still rolls, and a current action 17 does not replace projectile selector zero");
    }
    for (uint8_t subtype : {uint8_t(0), uint8_t(1), uint8_t(2), uint8_t(BRACELET),
                            uint8_t(BOOMERANG), uint8_t(STAFF), uint8_t(DAGGER)}) {
        Collision c;
        c.source.equipment[MAIN_HAND] = {3, subtype};
        const bool half = subtype <= 2 || subtype == BRACELET;
        const int procDraw = half || subtype == DAGGER ? 6000 : 4000;
        int draws = 0;
        c.step([&] { return draws++ == 4 ? procDraw : 32767; }, {false, &c.source});
        check(draws == 6 && c.source.hasBuff(9) == (subtype == DAGGER),
              "producer probability applies its weapon-pair half or three-tenths scale before the draw");
    }
    {
        Collision c;
        c.source.equipment[MAIN_HAND] = {4, STAFF};
        int draws = 0;
        c.step([&] { return draws++ == 4 ? 4000 : 32767; }, {false, &c.source});
        check(c.source.hasBuff(9), "nonweapon raw staff subtype does not scale the proc probability");
    }
    {
        Collision c;
        c.source.action = 91;
        int draws = 0;
        c.step([&] { return draws++ == 3 ? 0 : 32767; }, {false, &c.source});
        check(draws == 4 && c.effects.hits.empty() && c.source.hasBuff(9) && c.effects.buffs.size() == 1,
              "impact producer survives a later generic ground-target rejection");
    }
    {
        Collision c;
        c.world.list[0].element = 2;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {false, &c.source});
        check(draws == 5 && c.effects.hits.size() == 1 && c.effects.hits[0].record.amount > 0,
              "hostile water contact skips its separate event while the later impact still enters generic resolution");
    }
    for (uint8_t role : {uint8_t(0), uint8_t(6)}) {
        Collision c;
        c.world.list[0].kind = BURST;
        c.world.list[0].element = 2;
        c.world.list[0].age = 999;
        c.target.controller = role;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {false, &c.source});
        check(draws == 2 && c.effects.hits.size() == (role == 0 ? 1u : 0u) &&
              (role != 0 || c.effects.hits[0].record.amount == -10.f),
              "water burst's separate event requires a false relationship result and then excludes role six");
    }
    {
        Collision c;
        auto &shot = c.world.list[0];
        shot.element = 2;
        shot.special = 2;
        shot.strength = .7f;
        shot.velocity = {1, 0, 0};
        c.step([] { return 32767; }, {false, &c.source});
        check(c.world.list.size() == 1 && shot.kind == BURST && shot.age == 0 && shot.damage == .5f &&
              shot.radius == 8.5f && shot.strength == 0 && shot.special == 0 && shot.velocity.x == 1,
              "water conversion uses the old flag and strength and retains its existing velocity");
    }
    {
        Projectiles world;
        Projectile shot;
        shot.kind = BURST;
        shot.age = INT32_MAX;
        world.list.push_back(shot);
        ShotEffects effects;
        world.step(1, {}, {}, [](int32_t, int32_t, int32_t) { return uint8_t(0); }, [] { return 0; }, effects);
        check(world.list.size() == 1 && world.list[0].age == INT32_MIN,
              "projectile clock addition retains the native signed 32-bit wrap before its lifetime comparison");
    }
    {
        Collision c;
        c.world.list[0].kind = ARROW;
        c.source.mana = .2f;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {true, nullptr});
        check(draws == 4 && c.source.mana == .2f && c.effects.buffs.empty(),
              "nonlocal network resource-gain draws occur before suppression of the resource write");
    }
    {
        Collision c;
        c.source.characterClass = 2;
        c.source.specialization = 1;
        c.world.list[0].kind = ARROW;
        int draws = 0;
        c.step([&] { return ++draws == 4 ? 0 : 32767; }, {false, &c.source});
        check(draws == 5 && c.source.hasBuff(10) && c.effects.buffs.size() == 1 &&
              c.effects.buffs[0].source == 41 && c.effects.buffs[0].target == 41 &&
              c.effects.buffs[0].record.source == 0 && c.effects.buffs[0].record.timeMs == 30000,
              "owned ranger contact appends its fresh self buff after gain and critical draws");
    }
    {
        Collision c;
        c.source.mana = .2f;
        int draws = 0;
        c.step([&] { draws++; return 32767; }, {false, nullptr});
        check(draws == 2 && c.effects.hits.size() == 1 && c.effects.buffs.empty(),
              "mode-zero nonlocal role-zero source reaches contact but leaves the later impact scan before its draws");
    }
    {
        Collision c;
        c.source.controller = 1;
        c.source.characterClass = 2;
        c.source.specialization = 1;
        c.target.controller = 0;
        c.world.list[0].kind = ARROW;
        int draws = 0;
        c.step([&] { return ++draws == 4 ? 0 : 32767; }, {true, &c.target});
        check(draws == 3 && c.effects.hits.empty() && c.effects.buffs.empty(),
              "controlled network target is rejected only after the preceding contact resource and critical draws");
    }
    for (bool impact : {false, true}) {
        Collision c;
        c.world.list[0].special = impact ? 1 : 0;
        c.world.list[0].kind = impact ? SPELL : ARROW;
        c.world.list[0].damage = -10;
        c.world.list[0].element = 0;
        c.target.dodgeMs = impact ? 1.f : 0.f;
        int draws = 0;
        c.world.step(1, {{&c.target, &c.targetBody}}, {},
            [=](int32_t, int32_t, int32_t) { return uint8_t(impact ? 1 : 0); },
            [&] { draws++; return 32767; }, c.effects);
        check(!c.effects.hits.empty() && c.effects.hits[0].record.amount == (impact ? -10.f : 0.f) &&
              draws == (impact ? 2 : 1),
              "null source admits ordinary contacts and locked impact candidates; only contact clamps negative input");
    }
    {
        auto actor = ranger(24);
        ShotEffects effects;
        chargeSound(actor, 16777399, 1, {}, effects);
        check(effects.sounds.size() == 1 && effects.sounds[0].sound == 55,
              "held-action sound retains one-millisecond bucket crossings beyond binary32 integer precision");
    }
    for (uint8_t weapon : {uint8_t(STAFF), uint8_t(WAND), uint8_t(BRACELET)})
        for (uint8_t specialization : {uint8_t(0), uint8_t(1)}) {
            Combatant actor;
            actor.characterClass = 3;
            actor.specialization = specialization;
            actor.equipment[MAIN_HAND] = {3, weapon};
            actor.actionMs = 1000;
            actor.buffs.push_back({9, 1, 1000});
            check(startSecondaryAttack(actor), "powered selected secondary attack starts after idle recovery");
            const bool skip = actor.action == 31 || actor.action == 33 || actor.action == 37 || actor.action == 46;
            check(actor.actionMs == (skip ? (float)phases(actor, actor.action).windUp : 0.f),
                  "buff nine skips staff and selected projectile anticipation, excluding 45 and 43");
            const float before = actor.actionMs;
            check(!startSecondaryAttack(actor) && actor.actionMs == before,
                  "busy powered secondary start leaves its current clock unchanged");
        }
    {
        Combatant actor;
        actor.actionMs = 1000;
        actor.dodgeMs = -1;
        actor.mana = 1;
        check(!startSecondaryAttack(actor) && actor.action == 0, "a negative nonzero roll timer rejects an ordinary secondary start");
        actor.dodgeMs = 600;
        actor.characterClass = 4;
        actor.specialization = 1;
        actor.equipment[MAIN_HAND] = {3, LONGSWORD};
        actor.criticalLatched = true;
        check(startSecondaryAttack(actor) && actor.action == 5 && actor.dodgeMs == 600 && actor.criticalLatched,
              "assassin selection preserves the roll exception without mutating the later hit latch");
        actor = {};
        actor.actionMs = 1000;
        actor.mana = .099f;
        check(!startSecondaryAttack(actor), "secondary helper rejects mana below its native requirement");
        actor.mana = .1f;
        check(startSecondaryAttack(actor), "secondary helper accepts mana equality");
    }
    for (int action : {22, 23, 26, 50, 55, 96, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 89, 90, 108,
                       24, 25, 27, 36, 59, 63, 64}) {
        auto actor = ranger(action);
        std::vector<SoundEvent> sounds;
        int draws = 0;
        actionSounds(actor, 0, 2000, {}, [&] { draws++; return 0; }, sounds);
        check(sounds.empty() && draws == 0, "ranged dispatch has no extra melee lift/swing layer or pitch draws");
    }
    {
        auto actor = ranger(89);
        const float anticipation = (float)phases(actor, 89).windUp;
        Body body;
        ShotEffects effects;
        std::vector<Projectile> shots;
        int draws = 0;
        launch(actor, body, {}, anticipation - 50, 300, [&] { draws++; return 0; }, shots, effects);
        check(shots.size() == 1 && effects.sounds.size() == 2 && effects.sounds[0].sound == 36 &&
              effects.sounds[1].sound == 38 && draws == 2,
              "area projectile can channel in its last wind-up interval and release with zero direction");
    }
    {
        auto actor = ranger(22);
        actor.actionMs = -1;
        stepAction(actor, 32);
        check(actor.actionMs == -1, "negative active action clock does not advance through the general path");
        actor.action = 0;
        stepAction(actor, 32);
        check(actor.actionMs == 31, "idle action permits a negative clock to advance");
        actor.controller = 6;
        actor.actionMs = 0;
        stepAction(actor, 32);
        check(actor.actionMs == 0, "control discriminator six suppresses the general clock increment");
        actor.controller = 1;
        actor.actionMs = 100000;
        stepAction(actor, 32);
        check(actor.actionMs == 100032, "completed action clock keeps advancing without an invented duration cap");
    }
    for (int remainingStun : {-1, 0, 1}) {
        auto actor = ranger(24);
        actor.charge = .2f;
        actor.actionMs = 200;
        actor.hitStun = remainingStun;
        ShotEffects effects;
        stepCharge(actor, 190, 10, {}, effects);
        check(remainingStun > 0 ? actor.charge == 0 : actor.charge > .2f,
              "late charge gate clears only a still-positive stun");
    }
    {
        auto zone = std::make_shared<worldgen::Zone>();
        zone->zx = zone->zy = 0;
        worldgen::Spawn spawn;
        spawn.type = 44;
        spawn.controller = 1;
        spawn.x = spawn.y = 10 * 65536;
        spawn.z = 3 * 65536;
        zone->spawns.push_back(spawn);
        worldgen::WorldMap map;
        map.add(zone);
        Combatant player;
        player.id = 1;
        Body body;
        body.place({15, 10, 3});
        const BlockType floor = [](int32_t, int32_t, int32_t z) { return (uint8_t)(z <= 0 ? 1 : 0); };
        Npcs npcs;
        npcs.step(0, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
        check(npcs.all().size() == 1, "synthetic creature is activated through the actual world facade");
        if (!npcs.all().empty()) {
            npcs.npcs()[0].thinkMs = 32;
            npcs.npcs()[0].fighter.hitStun = 32;
            npcs.step(32, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
            check(!npcs.all().empty() && npcs.all()[0].thinkMs == 64,
                  "stun expiring exactly this update still suppresses early behavior and retains accumulated time");
            auto &npc = npcs.npcs()[0];
            npc.fighter.characterClass = 2;
            npc.fighter.equipment[MAIN_HAND] = {3, BOW};
            npc.fighter.action = 22;
            npc.fighter.actionMs = (float)phases(npc.fighter, 22).windUp;
            npc.fighter.hitStun = 100;
            npc.fighter.mana = 1;
            const auto emitted = npcs.step(32, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
            check(emitted.shots.size() == 1 && npc.fighter.action == 0 && npc.fighter.actionMs == 32 &&
                  npc.fighter.hitStun == 68, "real NPC emits its pending shot before the surviving stun interrupts its action");
            npc.fighter.action = 22;
            npc.fighter.actionMs = (float)phases(npc.fighter, 22).windUp;
            npc.fighter.hitStun = 32;
            npc.fighter.actionLockMs = 33;
            const auto locked = npcs.step(32, map, {{player, body, 0, 0, false}}, 43200000, 0, floor);
            check(locked.shots.empty() && npc.fighter.actionLockMs == 1,
                  "real NPC's surviving action lock suppresses a would-be anticipation crossing");
        }
    }
    {
        auto actor = ranger(25);
        const float threshold = (float)phases(actor, 25).windUp;
        actor.actionMs = threshold;
        ShotEffects effects;
        stepCharge(actor, threshold - 1, 1, {}, effects);
        check(actor.charge > 0, "charging begins when the advanced clock reaches anticipation");
    }
    {
        auto actor = ranger(25);
        actor.charge = .2f;
        actor.actionMs = 210;
        ShotEffects effects;
        stepCharge(actor, 190, 20, {}, effects);
        check(effects.sounds.size() == 1 && std::abs(effects.sounds[0].pitch - 1.1f) < 1e-6f,
              "charge sound pitch reads charge before the gain");
        check(actor.charge > .2f, "the later charge gain still occurs");
        actor.actionMs = 199;
        effects.sounds.clear();
        stepCharge(actor, -1, 200, {}, effects);
        check(effects.sounds.empty(), "signed sound quotients truncate toward zero");
    }
    {
        auto actor = ranger(8);
        actor.actionMs = 210;
        ShotEffects effects;
        stepCharge(actor, 190, 20, {}, effects);
        check(effects.sounds.empty(), "action eight charges without entering the ranged sound branch");
    }
    {
        auto actor = ranger(22);
        Body body;
        const float threshold = (float)phases(actor, 22).windUp;
        std::vector<Projectile> shots;
        ShotEffects effects;
        int draws = 0;
        auto draw = [&] { draws++; return 0; };
        launch(actor, body, {1, 0, 0}, threshold - 1, 1, draw, shots, effects);
        check(shots.empty() && draws == 0, "landing exactly at a single-shot threshold does not emit");
        launch(actor, body, {1, 0, 0}, threshold, 1, draw, shots, effects);
        check(shots.size() == 1 && draws == 2, "a single shot includes equality only on the old side");
    }
    {
        auto actor = ranger(26);
        actor.charge = .5f;
        Body body;
        const float threshold = (float)phases(actor, 26).windUp;
        std::vector<Projectile> shots;
        ShotEffects effects;
        launch(actor, body, {1, 0, 0}, threshold - 1, 400, [] { return 0; }, shots, effects);
        check(shots.size() == 1, "a large boomerang update emits once even across both thresholds");
    }
    {
        auto actor = ranger(55);
        actor.charge = .5f;
        Body body;
        const Phases timing = phases(actor, 55);
        const float period = (float)(timing.active / 4);
        std::vector<Projectile> shots;
        ShotEffects effects;
        int draws = 0;
        const auto draw = [&] { return draws++ == 0 ? 0 : 32767; };
        launch(actor, body, {1, 0, 0}, 0, period, draw, shots, effects);
        check(shots.size() == 1 && draws == 2, "a volley emits exactly once at its pulse boundary");
        check(effects.sounds.size() == 1 && effects.sounds[0].pitch == 1,
              "the volley pitch uses the first random draw");
        check(!shots.empty() && std::abs(shots[0].damage - 2.278125f) < 1e-5f,
              "volley damage uses the second draw's spread");
        shots.clear();
        launch(actor, body, {1, 0, 0}, period, (float)timing.active * 2, draw, shots, effects);
        check(shots.size() == 1, "a large volley update does not catch up every skipped pulse");
    }
    {
        auto actor = ranger(45);
        actor.characterClass = 3;
        Body body;
        const Phases timing = phases(actor, 45);
        const float first = (float)timing.windUp, period = (float)(timing.active / 3);
        std::vector<Projectile> shots;
        ShotEffects effects;
        launch(actor, body, {1, 0, 0}, first, period, [] { return 0; }, shots, effects);
        check(shots.empty(), "a repeated spell landing on the selected boundary defers emission");
        check(std::abs(actor.mana - .7f) < 1e-6f, "anticipation magic cost is paid even when the selected candidate defers emission");
        launch(actor, body, {1, 0, 0}, first + period, 1, [] { return 0; }, shots, effects);
        check(shots.size() == 1, "the following update can emit from equality at that boundary");
        check(std::abs(actor.mana - .7f) < 1e-6f, "later repeated emission does not repeat the anticipation cost");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
