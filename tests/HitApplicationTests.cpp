#include "creatures/Combat.h"
#include "creatures/Npcs.h"
#include "creatures/Physics.h"
#include "creatures/Strike.h"
#include <bit>
#include <cstdio>
#include <limits>

using namespace creatures;
namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) { std::printf("FAIL %s\n", description); failures++; }
}
Combatant targetActor() {
    Combatant actor;
    actor.id = 72;
    actor.controller = 1;
    actor.health = 100;
    return actor;
}
}

int main() {
    {
        const auto saved = itemBonuses().health;
        for (float health : {-1.f, -0.f, 10.f, 11.f, std::numeric_limits<float>::quiet_NaN()}) {
            Combatant actor;
            actor.controller = 1;
            actor.characterClass = 0;
            actor.multipliers[0] = 0;
            actor.equipment[MAIN_HAND].type = 3;
            actor.health = health;
            int queries = 0;
            itemBonuses().health = [&](const Item &) { return ++queries == 1 ? 10.f : 20.f; };
            const bool admitted = admitCreatureUpdate(actor);
            check(queries == (health > 10 ? 2 : 1) && admitted == !(health <= 0) &&
                  (health > 10 ? actor.health == 20 :
                      std::bit_cast<uint32_t>(actor.health) == std::bit_cast<uint32_t>(health)),
                  "health admission queries before testing life, keeps equality/unordered values, and stores a fresh second maximum");
        }
        Combatant actor;
        actor.controller = 1;
        actor.characterClass = 0;
        actor.multipliers[0] = 0;
        actor.equipment[MAIN_HAND].type = 3;
        actor.health = 11;
        int queries = 0;
        itemBonuses().health = [&](const Item &) { return ++queries == 1 ? 10.f : -7.f; };
        check(!admitCreatureUpdate(actor) && actor.health == -7 && queries == 2,
              "admission rereads health after the second maximum result without another clamp or query");
        queries = 0;
        actor.health = 5;
        itemBonuses().health = [&](const Item &) { queries++; return std::numeric_limits<float>::quiet_NaN(); };
        check(admitCreatureUpdate(actor) && actor.health == 5 && queries == 1,
              "unordered comparison maximum does not replace an ordinary current health value");
        itemBonuses().health = saved;
    }
    {
        Npcs world;
        Npc target;
        target.fighter = targetActor();
        target.fighter.health = 0;
        target.fighter.action = 22;
        target.fighter.actionMs = 17;
        target.fighter.actionLockMs = 9;
        target.fighter.hitStun = 100;
        target.fighter.buffs.push_back({4, 1, 1000, 41});
        target.body.place({0, 1, 3});
        target.body.velocity = {2, 3, 4};
        target.home = target.body.blocks();
        target.zone = {0, 0};
        target.tameMs = 44;
        world.npcs().push_back(target);
        Combatant source;
        source.id = 41;
        Body body;
        body.place({0, 0, 3});
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        world.step(10, map, {{source, body}}, 43200000, 0,
            [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); });
        check(world.all().size() == 1 && world.all()[0].fighter.actionMs == 17 &&
              world.all()[0].fighter.actionLockMs == 9 && world.all()[0].fighter.hitStun == 100 &&
              world.all()[0].fighter.buffs[0].timeMs == 1000 && world.all()[0].thinkMs == 0 &&
              world.all()[0].tameMs == 44,
              "actual NPC health rejection skips behavior, timers and resident countdown");
    }
    {
        Combatant source;
        source.id = 41;
        source.action = 58;
        auto target = targetActor();
        target.action = 84;
        target.hitStun = -3001;
        target.buffs.push_back({6, 10, 1000, source.id});
        Body body;
        HitInput input;
        input.amount = 30;
        input.magic = true;
        input.blockable = false;
        int draws = 0;
        std::vector<uint8_t> consumed;
        std::vector<BuffEvent> buffs;
        const auto result = resolveHit(target, body.velocity, &source, input,
            [&] { draws++; return 0; }, [&](const HitEmission &emission) {
                const auto &record = emission.record;
                consumed.push_back(record.kind);
                if (emission.applyLocally)
                    applyHit(target, body, record, {&source}, &buffs);
            });
        check(result.hitAccepted && result.records.size() == 2 &&
              consumed == std::vector<uint8_t>{HIT_ABSORBED, HIT_DAMAGE} && buffs.size() == 1 &&
              target.buffs[0].value == 0 && target.buffs[0].timeMs == 0 && target.action == 0 &&
              target.hitStun == 3000 && target.health == 81 && result.records.back().stun == 0 && draws == 1,
              "shield application precedes the main-record status gate and suppresses its otherwise admitted draw");
    }
    {
        Combatant source;
        source.action = 28;
        auto target = targetActor();
        target.health = 0;
        Body body;
        int draws = 0, applications = 0;
        const auto result = resolveHit(target, body.velocity, &source, {},
            [&] { draws++; return 0; }, [&](const HitEmission &) { applications++; });
        check(result.records.empty() && result.bursts.empty() && draws == 0 && applications == 0,
              "dead-target admission precedes beam presentation and all application callbacks");
    }
    {
        Combatant source;
        source.id = 41;
        source.action = 1;
        source.multipliers[2] = 10;
        const auto timing = phases(source, source.action);
        source.actionMs = timing.windUp + timing.active / 2;
        Body body;
        body.place({0, 0, 3});
        Npc target;
        target.fighter = targetActor();
        target.fighter.action = 84;
        target.fighter.hitStun = -3001;
        target.fighter.buffs.push_back({6, .25f, 1000, 99});
        target.body.place({0, 1, 3});
        Npcs world;
        world.npcs().push_back(target);
        PlayerView player{source, body, 0, source.actionMs};
        NpcFrame frame;
        world.dispatchPlayerActions({&player, 1}, 16,
            [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); }, frame);
        check(frame.seen.size() == 2 && frame.seen[0].first.kind == HIT_ABSORBED &&
              frame.seen[1].first.kind == HIT_DAMAGE && frame.seen[1].first.stun == 0 &&
              world.all()[0].fighter.hitStun == 3000 && frame.buffs.size() == 1 &&
              frame.buffs[0].record.source == 99 && frame.buffs[0].record.timeMs == 0,
              "actual NPC melee facade consumes shield state before constructing the main hit");
    }
    {
        auto target = targetActor();
        target.action = 84;
        target.stealth = .75f;
        Body body;
        body.velocity = {2, 0, 0};
        body.push = {1, 0, 0};
        HitRecord hit;
        hit.amount = 10;
        hit.kind = HIT_IMMUNE;
        hit.stun = 100;
        hit.lockMs = 700;
        hit.knockBack = {0, 3, 0};
        applyHit(target, body, hit);
        check(target.stealth == 0 && target.action == 0 && target.hitStun == 3000 && target.health == 100 &&
              target.actionLockMs == 0 && body.velocity.x == 2 && body.push.x == 1,
              "category three retains positive-hit scalar/cancellation prefix and skips later state changes");
    }
    for (float amount : {0.f, -1.f}) {
        auto target = targetActor();
        target.mana = -1;
        target.charge = .2f;
        target.blockGauge = .6f;
        target.hitStun = -10;
        Body body;
        body.velocity = {2, 0, 0};
        body.push = {1, 0, 0};
        HitRecord hit;
        hit.amount = amount;
        hit.kind = HIT_BLOCKED;
        hit.stun = 100;
        hit.lockMs = 200;
        hit.knockBack = {0, 3, 0};
        applyHit(target, body, hit);
        check(target.mana == -.75f && target.charge == .2f && target.blockGauge == .6f &&
              target.health == 100 && target.hitStun == -10 && target.actionLockMs == 200 &&
              body.velocity.x == (amount < 0 ? 2.f : 0.f) && body.push.y == (amount < 0 ? 0.f : 3.f),
              "blocked record adds a quarter resource, keeps charge/gauge, and handles nonnegative response before returning");
    }
    {
        auto target = targetActor();
        target.level = 2;
        target.powerTier = 2;
        target.stealth = .9f;
        Body body;
        HitRecord hit;
        hit.kind = HIT_IMMUNE;
        hit.amount = .7f;
        applyHit(target, body, hit);
        check(std::bit_cast<uint32_t>(target.stealth) == 0x3f4bc302,
              "scalar reduction uses quartered amount divided by narrowed power after the integer exponent sum");
    }
    {
        auto target = targetActor();
        target.buffs = {{6, 12, 900, 11}, {4, 99, 400, 7}, {6, 5, 1000, 22}, {6, 0, 0, 33}};
        target.buffs[2].typePadding = {3, 7, 11};
        target.buffs[2].timePadding = {13, 17, 19, 23};
        const auto untouched = target.buffs[1].serialize();
        Body body;
        HitRecord hit;
        hit.attacker = 888;
        hit.target = 777;
        hit.amount = 7;
        hit.kind = HIT_ABSORBED;
        std::vector<BuffEvent> events;
        applyHit(target, body, hit, {}, &events);
        check(target.health == 100 && target.buffs.size() == 4 && events.size() == 3 &&
              target.buffs[0].value == 5 && target.buffs[0].timeMs == 900 &&
              target.buffs[2].value == 0 && target.buffs[2].timeMs == 0 &&
              target.buffs[3].value == 0 && target.buffs[3].timeMs == 0 && target.buffs[1].serialize() == untouched,
              "shield damage subtracts the entire amount from every matching resident without unlink or health overflow");
        check(events[0].source == 888 && events[0].target == 777 && events[0].record.source == 11 &&
              events[1].record.source == 22 && events[2].record.source == 33 &&
              events[1].record.serialize() == target.buffs[2].serialize(),
              "shield events keep input header IDs, resident embedded sources, opaque bytes and traversal order");
    }
    for (uint8_t role : {uint8_t(0), uint8_t(1), uint8_t(6)})
        for (bool local : {false, true})
            for (bool replica : {false, true}) {
                auto target = targetActor();
                target.controller = role;
                Body body;
                body.velocity = {2, 0, 0};
                body.push = {1, 0, 0};
                HitRecord hit;
                hit.amount = 10;
                hit.stun = 1;
                hit.lockMs = 7;
                hit.knockBack = {0, 3, 0};
                applyHit(target, body, hit, {nullptr, local, replica});
                const bool responseAllowed = role != 6 && (local || (!replica && role != 0));
                check(body.velocity == glm::vec3(0) && body.push.y == (responseAllowed ? 3.f : 0.f) &&
                      target.hitStun == 1 && target.actionLockMs == 7 &&
                      target.health == ((!replica || local) ? 90.f : 100.f),
                      "response ownership, velocity clearing, timers and final health ownership have independent gates");
            }
    {
        auto target = targetActor();
        target.hitStun = 100;
        Body body;
        body.velocity = {2, 0, 0};
        body.push = {1, 0, 0};
        HitRecord hit;
        hit.amount = 1;
        hit.knockBack = {3, 4, 0};
        applyHit(target, body, hit);
        check(body.push == glm::vec3(1, 0, 0) && body.velocity.x == 2,
              "existing stun attenuates a nonstunning response before comparing squared magnitudes");
    }
    const auto eligible = [](uint8_t role) { return role == 0 || role == 1 || role == 3 || role == 5 || role == 6; };
    for (uint8_t sourceRole = 0; sourceRole <= 6; sourceRole++)
        for (uint8_t targetRole = 0; targetRole <= 6; targetRole++) {
            Combatant source;
            source.controller = sourceRole;
            auto target = targetActor();
            target.controller = targetRole;
            Body body;
            HitRecord hit;
            hit.amount = 10;
            applyHit(target, body, hit, {&source});
            check(target.health == (eligible(sourceRole) && eligible(targetRole) ? 90.f : 100.f),
                  "positive ordinary damage checks both numeric health predicates");
        }
    for (int kind : {139, 140, 141, 142, 143}) {
        auto target = targetActor();
        target.controller = 6;
        target.type = kind;
        Body body;
        HitRecord hit;
        hit.amount = 10;
        applyHit(target, body, hit);
        check(target.health == (kind >= 140 && kind <= 142 ? 100.f : 90.f),
              "role-six kinds 140 through 142 are excluded from positive ordinary health subtraction");
    }
    {
        Combatant source;
        source.controller = 2;
        auto target = targetActor();
        target.controller = 2;
        Body body;
        HitRecord hit;
        hit.amount = -5;
        hit.kind = HIT_ABSORBED;
        applyHit(target, body, hit, {&source});
        check(target.health == 105, "negative shield-category amounts heal without positive-damage eligibility");
    }
    for (bool granted : {false, true}) {
        auto target = targetActor();
        target.appearanceFlags = 0x2000;
        target.stealth = 1;
        Body body;
        HitRecord hit;
        hit.amount = 1;
        applyHit(target, body, hit, {nullptr, true, false, granted});
        check(target.stealth == .75f && target.health == (granted ? 99.f : 100.f),
              "protected-target admission follows the scalar prefix and controls the later body");
    }
    {
        auto target = targetActor();
        Body body;
        body.place({0, 0, 20});
        body.velocity = {2, 0, 0};
        HitRecord hit;
        hit.amount = 1;
        hit.stun = 100;
        hit.knockBack = {3, 0, 0};
        applyHit(target, body, hit);
        body.stunMs = target.hitStun;
        step(body, 1, [](int32_t, int32_t, int32_t) { return uint8_t(0); });
        check(body.position[0] == 196 && body.push.x == 3,
              "real physics consumes retained response after hit application has cleared velocity");
    }
    {
        Body body;
        body.place({0, 0, 20});
        body.push = {.7f, 0, 0};
        body.pathActive = true;
        step(body, 3, [](int32_t, int32_t, int32_t) { return uint8_t(0); });
        check(std::bit_cast<uint32_t>(body.push.x) == 0x3f308681,
              "navigation admits response decay with per-millisecond subtract/multiply/add rounding");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
