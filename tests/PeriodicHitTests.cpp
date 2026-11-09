#include "creatures/Npcs.h"
#include <bit>
#include <cstdio>

using namespace creatures;
namespace {
int failures = 0;
void check(bool condition, const char *message) {
    if (!condition) { std::printf("FAIL %s\n", message); failures++; }
}
Npc actor(uint64_t id, uint8_t role = 1) {
    Npc npc;
    npc.fighter.id = id;
    npc.fighter.controller = role;
    npc.fighter.health = 100;
    npc.fighter.multipliers[4] = 0;
    npc.body.place({0, 1, 3});
    npc.body.contact = ON_GROUND;
    npc.home = npc.body.blocks();
    return npc;
}
void countdown(Npcs &world, int32_t elapsed, NpcFrame &frame) {
    Npc &target = world.npcs()[0];
    endActionUpdate(target.fighter, elapsed, {false, false}, [&](const Buff &buff) {
        world.periodicHit(target.fighter, target.body, buff, {}, frame);
    });
}
}

int main() {
    {
        Npcs world;
        world.npcs().push_back(actor(71));
        auto &target = world.npcs()[0];
        target.body.position[0] = 0x123456789abcdefLL;
        target.fighter.buffs = {{DAMAGE_OVER_TIME, 3, 1201, 999}};
        NpcFrame frame;
        countdown(world, 810, frame);
        check(target.fighter.health == 97 && target.fighter.buffs[0].timeMs == 391 &&
              frame.emittedHits.size() == 1 && frame.seen.size() == 1 && frame.emittedHits[0].attacker == 0 &&
              frame.emittedHits[0].position[0] == target.body.position[0] &&
              frame.emittedHits[0].stun == 0 && frame.emittedHits[0].knockBack == glm::vec3(0) &&
              frame.sounds.empty() && frame.buffs.empty(),
              "one surviving crossing produces/applies one raw-coordinate null-source hit even across several buckets");
        countdown(world, 391, frame);
        check(target.fighter.health == 97 && target.fighter.buffs.empty() && frame.emittedHits.size() == 1,
              "expiration at zero skips periodic production and unlinks only during cleanup");
    }
    {
        Npcs world;
        world.npcs().push_back(actor(71));
        auto source = actor(41);
        source.fighter.health = 0;
        source.fighter.combo = 25;
        source.fighter.action = 17;
        world.npcs().push_back(source);
        auto &target = world.npcs()[0];
        target.fighter.multipliers[4] = 3;
        target.fighter.combo = 0;
        target.fighter.buffs = {{ABSORB, 10, 1000, 41}, {DAMAGE_OVER_TIME, 20, 801, 41}};
        NpcFrame frame;
        countdown(world, 10, frame);
        check(frame.emittedHits.size() == 2 && frame.emittedHits[0].kind == HIT_ABSORBED &&
              frame.emittedHits[0].amount == 20 && frame.emittedHits[1].amount == 13.5f &&
              target.fighter.health == 86.5f && target.fighter.buffs.size() == 2 &&
              target.fighter.buffs[0].value == 0 && target.fighter.buffs[0].timeMs == 0 &&
              target.fighter.buffs[1].timeMs == 791 && frame.buffs.size() == 1,
              "dead found source supplies its own ratio, fixed target stat, and selector zero without action-17 reinsertion");
    }
    {
        Npcs world;
        world.npcs().push_back(actor(71));
        auto source = actor(41);
        source.fighter.action = 58;
        world.npcs().push_back(source);
        auto &target = world.npcs()[0];
        target.fighter.action = 84;
        target.fighter.hitStun = -4000;
        target.fighter.buffs = {{ABSORB, 5, 1000, 41}, {DAMAGE_TAKEN, 2, 1, 41},
                               {DAMAGE_OVER_TIME, 9, 801, 41}, {ABSORB, 3, 1000, 41}};
        NpcFrame frame;
        countdown(world, 10, frame);
        check(frame.emittedHits.size() == 2 && frame.emittedHits[0].kind == HIT_ABSORBED &&
              frame.emittedHits[0].amount == 9 && frame.emittedHits[1].amount == 2 &&
              frame.emittedHits[1].stun == 0 && target.fighter.health == 98 &&
              target.fighter.action == 0 && target.fighter.hitStun == 2990 && frame.buffs.size() == 2 &&
              frame.buffs[0].record.value == 0 && frame.buffs[1].record.value == 0 &&
              target.fighter.buffs.size() == 2 && target.fighter.buffs[0].kind == ABSORB &&
              target.fighter.buffs[0].timeMs == 0 && target.fighter.buffs[1].kind == DAMAGE_OVER_TIME,
              "shield application precedes stun admission, expired multipliers stay resident, and depletion respects visit order");
        countdown(world, 0, frame);
        check(target.fighter.buffs.size() == 1 && target.fighter.buffs[0].kind == DAMAGE_OVER_TIME &&
              frame.emittedHits.size() == 2, "next zero countdown removes the previously visited depleted shield without replaying a hit");
    }
    {
        Npcs world;
        world.npcs().push_back(actor(0));
        auto &target = world.npcs()[0];
        target.fighter.multipliers[4] = 5;
        target.fighter.action = 17;
        target.fighter.buffs = {{DAMAGE_OVER_TIME, 10, 801, 0}};
        NpcFrame frame;
        countdown(world, 10, frame);
        check(frame.emittedHits.size() == 1 && frame.emittedHits[0].attacker == 0 &&
              frame.emittedHits[0].amount == 5 && target.fighter.health == 95 && target.fighter.buffs.size() == 1,
              "identifier zero resolves a resident self source and applies found-source mitigation without recursive DOT insertion");
    }
    {
        Npcs world;
        world.npcs().push_back(actor(71));
        world.npcs().push_back(actor(0, 2));
        auto &target = world.npcs()[0];
        target.fighter.buffs = {{DAMAGE_OVER_TIME, 10, 801, 999}};
        NpcFrame frame;
        countdown(world, 10, frame);
        check(frame.emittedHits.size() == 1 && frame.emittedHits[0].amount == 10 && target.fighter.health == 100,
              "application resolves the emitted zero source independently and retains its role-dependent health admission");
    }
    {
        Npcs world;
        world.npcs().push_back(actor(71));
        world.npcs().push_back(actor(41));
        auto &target = world.npcs()[0];
        target.body.contact = STUCK;
        target.fighter.contact = 0;
        target.fighter.buffs = {{DAMAGE_OVER_TIME, 10, 801, 41}};
        NpcFrame frame;
        countdown(world, 10, frame);
        check(frame.emittedHits.size() == 1 && frame.emittedHits[0].kind == HIT_IMMUNE &&
              target.fighter.health == 100 && target.fighter.contact == STUCK &&
              frame.sounds.size() == 1 && frame.sounds[0].sound == 24,
              "periodic interception sees the current engine body contact before selected stat and health processing");
    }
    for (bool grounded : {false, true}) {
        Npcs world;
        world.npcs().push_back(actor(71));
        auto source = actor(41, 0);
        source.fighter.action = 74;
        world.npcs().push_back(source);
        auto &target = world.npcs()[0];
        target.body.contact = grounded ? ON_GROUND : 0;
        target.body.velocity = {1, 2, 3};
        target.body.push = {2, 3, 4};
        target.fighter.hitStun = -4000;
        target.fighter.buffs = {{DAMAGE_OVER_TIME, 10, 801, 41}};
        NpcFrame frame;
        countdown(world, 10, frame);
        check(grounded ? (frame.emittedHits.size() == 1 && frame.emittedHits[0].stun == 3000 &&
                         target.body.velocity == glm::vec3(0) && target.body.push == glm::vec3(0, 0, 10) &&
                         target.fighter.health == 90 && target.fighter.hitStun == 2990 &&
                         frame.sounds.size() == 1 && frame.sounds[0].sound == 23)
                      : (frame.emittedHits.empty() && target.fighter.health == 100 &&
                         target.body.velocity == glm::vec3(1, 2, 3) && target.body.push == glm::vec3(2, 3, 4)),
              "source action ground gate precedes the zero-base stun override and separate velocity/response application");
    }
    {
        Npcs world;
        Combatant player;
        player.id = 1;
        player.health = 100;
        player.buffs = {{DAMAGE_OVER_TIME, 10, 801, 999}};
        Body body;
        PlayerView view{player, body, 0, 0, false, false, true};
        NpcFrame frame;
        int localApplications = 0;
        endActionUpdate(player, 10, {true, false}, [&](const Buff &buff) {
            world.periodicHit(player, body, buff, {&view, 1}, frame,
                              [&](HitRecord &hit) { localApplications++; hit.amount *= .5f; });
        });
        check(player.health == 95 && frame.emittedHits.size() == 1 && frame.emittedHits[0].amount == 10 &&
              frame.playerHits.empty() && localApplications == 1,
              "controlled countdown runs the SDK once before local application, preserving the producer record and avoiding a second application");
        endActionUpdate(player, 400, {true, true}, [&](const Buff &buff) {
            world.periodicHit(player, body, buff, {&view, 1}, frame);
        });
        check(player.health == 95 && player.buffs[0].timeMs == 391 && frame.emittedHits.size() == 1,
              "replica controlled countdown advances time while suppressing authoritative periodic production");
    }
    for (int action : {1, 28})
        for (float initial : {0.f, .3f, .8f})
            for (bool presentation : {false, true}) {
                auto target = actor(71).fighter;
                target.hitFlash = initial;
                auto source = actor(41).fighter;
                source.action = action;
                HitInput input;
                input.magic = true;
                input.amount = 10;
                input.blockable = presentation;
                int draws = 0;
                resolveHit(target, target.aim, &source, input, [&] { draws++; return 32767; });
                const float expected = action == 28 ? (initial < .3f ? .3f : initial) : presentation ? 1.f : initial;
                check(target.hitFlash == expected && draws == (action == 28 || presentation ? 1 : 0),
                      "late flash stores distinguish beam minimum from the ordinary presentation flag");
            }
    {
        auto target = actor(71).fighter;
        target.hitFlash = .8f;
        int draws = 0;
        resolveHit(target, target.aim, nullptr, {}, [&] { draws++; return 0; });
        check(target.hitFlash == .5f && draws == 1, "null-source flash overwrites a larger old value after one discarded draw");
    }
    {
        Npcs world;
        Combatant player;
        player.id = 1;
        player.health = 100;
        player.action = 84;
        player.stealth = 1;
        player.buffs = {{DAMAGE_OVER_TIME, 10, 801, 999}};
        Body body;
        worldgen::WorldMap map;
        const auto frame = world.step(10, map, {{player, body, 0, 0, false, true}}, 43200000, 0,
            [](int32_t, int32_t, int32_t) { return uint8_t(0); });
        check(player.health == 100 && player.action == 84 && player.stealth == 1 &&
              player.buffs[0].timeMs == 791 && frame.emittedHits.size() == 1 && frame.playerHits.size() == 1,
              "actual dedicated-world player countdown relays role-zero output before scalar/action/health mutation");
    }
    {
        Npcs world;
        world.npcs().push_back(actor(71));
        world.npcs()[0].fighter.buffs = {{DAMAGE_OVER_TIME, 3, 801, 999}};
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        const auto frame = world.step(10, map, {}, 43200000, 0,
            [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); });
        check(world.all().size() == 1 && world.all()[0].fighter.health == 97 &&
              world.all()[0].fighter.buffs[0].timeMs == 791 && frame.emittedHits.size() == 1,
              "actual NPC world phase reaches periodic production/application before movement");
    }
    for (bool variant : {false, true}) {
        auto source = actor(41).fighter;
        source.action = 17;
        source.combo = 50;
        source.equipment[MAIN_HAND] = {3, 0};
        source.equipment[MAIN_HAND].spirits = 3;
        source.equipment[MAIN_HAND].cubes = {{0, 0, 0, 128, 0}, {0, 0, 0, 127, 100},
                                            {0, 0, 0, 255, -1}, {0, 0, 0, 129, 100}};
        source.equipment[OFF_HAND] = {3, 12};
        source.equipment[OFF_HAND].spirits = 1;
        source.equipment[OFF_HAND].cubes = {{0, 0, 0, 131, 0}};
        auto target = actor(71).fighter;
        HitInput input;
        input.amount = 0;
        input.magic = true;
        input.blockable = false;
        input.area = variant;
        input.knockBack = {0, 3, 4};
        input.targetPosition = {-1000, -1000, -1000};
        input.attackerPosition = {1000, 1000, 1000};
        input.targetRecordPosition = {INT64_MIN + 17, -1, INT64_MAX - 7};
        int draws = 0;
        const auto out = resolveHit(target, target.aim, &source, input, [&] { draws++; return 0; });
        const float horizontal = (3.f * (1.f / 5.f)) * 4.f;
        bool payloads = out.bursts.size() == 3;
        for (const auto &burst : out.bursts)
            payloads &= burst.position[0] == input.targetRecordPosition[0] && burst.position[1] == -1 &&
                        burst.position[2] == input.targetRecordPosition[2] &&
                        burst.velocity == glm::vec3(0, variant ? horizontal * 2.f : horizontal, variant ? 10.f : 5.f) &&
                        burst.count == (variant ? 14 : 4) && burst.style == BRIGHT && burst.size == .1f && burst.spread == 3;
        check(payloads && draws == 0 && out.bursts[0].colour == glm::vec4(1, 1, 1.25f, 1.875f) &&
              out.bursts[1].colour == glm::vec4(1.25f, .625f, .125f, 1.875f) &&
              out.bursts[2].colour == glm::vec4(.5f, .5f, .5f, 1),
              "item effects use the full argument-vector length, raw position, selected-item/record order and grey default without draws");
    }
    {
        Npcs world;
        world.npcs().push_back(actor(71));
        auto source = actor(41);
        source.fighter.combo = 50;
        source.fighter.equipment[MAIN_HAND] = {3, 0};
        source.fighter.equipment[MAIN_HAND].spirits = 2;
        source.fighter.equipment[MAIN_HAND].cubes = {{0, 0, 0, 128, 0}, {0, 0, 0, 200, -1}};
        source.body.place({20, 30, 40});
        world.npcs().push_back(source);
        auto &target = world.npcs()[0];
        target.fighter.buffs = {{DAMAGE_OVER_TIME, 3, 801, 41}};
        NpcFrame frame;
        countdown(world, 10, frame);
        check(target.fighter.health == 97 && frame.bursts.size() == 2 &&
              frame.bursts[0].velocity == glm::vec3(0, 0, 5) && frame.bursts[1].velocity == glm::vec3(0, 0, 5) &&
              frame.bursts[0].count == 4 && frame.bursts[1].count == 4 &&
              frame.bursts[1].colour == glm::vec4(.5f, .5f, .5f, 1) && frame.sounds.empty(),
              "actual periodic scheduler retains item presentation at zero impulse without ordinary presentation or affix variants");
    }
    for (float impulse : {-0.f, 1.e-30f}) {
        auto source = actor(41).fighter;
        source.combo = 50;
        source.equipment[MAIN_HAND] = {3, 0};
        source.equipment[MAIN_HAND].spirits = 1;
        source.equipment[MAIN_HAND].cubes = {{0, 0, 0, 128, 1}};
        auto target = actor(71).fighter;
        HitInput input;
        input.knockBack = {impulse, 0, 0};
        input.blockable = false;
        const auto out = resolveHit(target, target.aim, &source, input, [] { return 0; });
        check(out.bursts.size() == 1 &&
              std::bit_cast<uint32_t>(out.bursts[0].velocity.x) == std::bit_cast<uint32_t>(impulse * 4.f) &&
              out.bursts[0].velocity.z == 5,
              "zero or underflowed squared length skips normalization and preserves the scaled original horizontal component");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
