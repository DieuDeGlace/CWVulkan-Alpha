#include "creatures/Npcs.h"
#include <cstdio>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
Npc source(uint64_t id, uint8_t role = 1) {
    Npc npc;
    npc.fighter.id = id;
    npc.fighter.controller = role;
    npc.fighter.health = 100;
    npc.body.place({0, 0, 3});
    npc.home = npc.body.blocks();
    return npc;
}
}

int main() {
    {
        Npcs world;
        Combatant local;
        local.id = 0;
        local.health = 100;
        Body body;
        PlayerView view{local, body, 0, 0, false, false, true};
        NpcFrame frame;
        HitRecord hit;
        hit.attacker = hit.target = 0;
        hit.amount = 10;
        int callbacks = 0;
        world.onControlledHit = [&](HitRecord &applied) { callbacks++; applied.amount *= .5f; };
        world.applyWorldHit(hit, {&view, 1}, frame);
        check(local.health == 95 && hit.amount == 10 && callbacks == 1 && frame.playerHits.empty(),
              "zero/self world keys apply synchronously through one SDK copy without queuing a replay");
        hit.target = 999;
        world.applyWorldHit(hit, {&view, 1}, frame);
        local.health = 0;
        hit.target = 0;
        world.applyWorldHit(hit, {&view, 1}, frame);
        check(callbacks == 1 && local.health == 0 && frame.playerHits.empty(),
              "missing and dead targets reject before SDK or relay and cannot start another fatal application");
        local.health = 100;
        view.local = false;
        world.applyWorldHit(hit, {&view, 1}, frame);
        check(local.health == 100 && callbacks == 1 && frame.playerHits.size() == 1 &&
              frame.playerHits[0].record.serialize() == hit.serialize(),
              "a world without a controlled actor relays a role-zero target without source lookup or local mutation");
        frame.reset();
        view.local = true;
        world.onControlledHit = {};
        hit.attacker = 999;
        world.applyWorldHit(hit, {&view, 1}, frame);
        check(local.health == 90, "a missing source bypasses its positive-health predicate without replacing it with the target");
        world.npcs().push_back(source(99));
        world.npcs().back().fighter.health = 0;
        hit.attacker = 99;
        world.applyWorldHit(hit, {&view, 1}, frame);
        check(local.health == 80, "resolved source health does not gate ordinary world-hit application");
    }
    {
        Npcs world;
        world.npcs().push_back(source(UINT64_MAX, 2));
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::WorldMap map;
        map.add(zone);
        Combatant local;
        local.id = 0;
        local.health = 100;
        local.stealth = 1;
        local.action = 84;
        Body body;
        body.place({0, 1, 3});
        PlayerView view{local, body, 0, 0, false, false, true};
        NpcFrame frame;
        world.stepInto(20, map, {view}, 1000, 0,
            [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); }, frame, {}, {},
            [&](int32_t) {
                world.npcs().front().fighter.id = 33;
                local.id = 77;
                HitRecord hit;
                hit.attacker = UINT64_MAX;
                hit.target = 0;
                hit.amount = 1;
                world.applyWorldHit(hit, {&view, 1}, frame);
            });
        check(local.health == 100 && local.stealth < 1 && local.action == 0 && local.hitStun == 3000,
              "in-pass application resolves entry keys despite changed own IDs and retains prefix effects before source-role rejection");
    }
    {
        Npcs world;
        Npc attacker = source(UINT64_MAX);
        attacker.fighter.action = 1;
        attacker.fighter.multipliers[2] = 10;
        const Phases timing = phases(attacker.fighter, 1);
        attacker.fighter.actionMs = timing.windUp + timing.active / 2;
        world.npcs().push_back(attacker);
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::WorldMap map;
        map.add(zone);
        Combatant local;
        local.id = 1;
        local.health = 100;
        Body body;
        body.place({0, 1, 3});
        NpcFrame frame;
        int callbacks = 0, actorCalls = 0;
        world.onControlledHit = [&](HitRecord &hit) { callbacks++; hit.amount = 1; };
        world.stepInto(16, map, {{local, body, 0, 0, false, false, true}}, 1000, 0,
            [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); }, frame, {}, {},
            [&](int32_t elapsed) {
                actorCalls++;
                check(local.health == 99 && callbacks == 1,
                      "a real earlier-key NPC melee hit is applied before the later controlled common phase");
                beginActionUpdate(local, elapsed);
                endActionUpdate(local, elapsed, {true, false});
            });
        check(actorCalls == 1 && callbacks == 1 && local.health == 99 && frame.playerHits.empty() &&
              frame.emittedHits.size() == 1 && frame.emittedHits[0].amount != 1,
              "production melee preserves the original emitted payload and cannot replay the applied SDK copy after the pass");
    }
    for (const float lock : {-1.f, 0.f, 1.f}) {
        Combatant target;
        target.id = 72;
        target.health = 100;
        target.characterClass = 4;
        target.specialization = 1;
        target.dodgeMs = lock;
        target.mana = .9f;
        target.buffs = {{SURE_CRITICAL, 7, 1, 99}, {3, 0, 1000}, {SURE_CRITICAL, 8, 2, 88}};
        Body body;
        body.velocity = {1, 2, 3};
        HitRecord hit;
        hit.target = 999; // the helper's self event uses the target object's own ID
        hit.kind = 4;
        hit.amount = 1;
        hit.stun = 50;
        hit.lockMs = 70;
        std::vector<BuffEvent> events;
        applyHit(target, body, hit, {nullptr, false, true}, &events);
        check(target.health == 100 && target.hitStun == 0 && target.actionLockMs == 70 && body.velocity == glm::vec3(0),
              "category four retains preceding motion/lock effects and returns before ordinary stun, ownership and health");
        check(lock == 0 ? events.empty() && target.mana == .9f && target.buffs[0].source == 99 :
              events.size() == 1 && target.mana == 1 && events[0].source == 72 && events[0].target == 72 &&
              events[0].record.kind == SURE_CRITICAL && events[0].record.source == 0 &&
              target.buffs.size() == 3 && target.buffs[0].timeMs == 30000 && target.buffs[0].value == 0 &&
              target.buffs[2].source == 88,
              "nonzero roll lock admits the assassin buff, replaces the first match and emits a self event after application");
    }
    for (const uint8_t category : {uint8_t(0), uint8_t(2), uint8_t(6), uint8_t(255)}) {
        Combatant target;
        target.health = 100;
        Body body;
        HitRecord hit;
        hit.amount = 10;
        hit.kind = category;
        applyHit(target, body, hit);
        check(target.health == 90, "remaining positive categories use health admission rather than a named-category whitelist");
    }
    for (const auto &[characterClass, specialization] :
         std::array<std::pair<uint8_t, uint8_t>, 3>{{{3, 1}, {4, 0}, {4, 2}}}) {
        Combatant target;
        target.characterClass = characterClass;
        target.specialization = specialization;
        target.dodgeMs = -1;
        target.mana = 0;
        Body body;
        HitRecord hit;
        hit.kind = 4;
        std::vector<BuffEvent> events;
        applyHit(target, body, hit, {}, &events);
        check(target.mana == 0 && target.buffs.empty() && events.empty(),
              "category four requires class four and specialization exactly one, including noncanonical specialization rejection");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
