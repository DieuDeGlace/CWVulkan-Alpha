#include "creatures/Combat.h"
#include "creatures/Npcs.h"
#include "creatures/Projectiles.h"
#include "creatures/Strike.h"
#include <bit>
#include <cstdio>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
Combatant sourceActor() {
    Combatant source;
    source.id = 41;
    source.combo = 50;
    source.health = 500;
    source.equipment[MAIN_HAND] = {3, 0};
    source.equipment[MAIN_HAND].spirits = 1;
    source.equipment[MAIN_HAND].cubes = {{0, 0, 0, 0x81, 21}};
    source.equipment[OFF_HAND] = {3, 0};
    source.equipment[OFF_HAND].spirits = 1;
    source.equipment[OFF_HAND].cubes = {{0, 0, 0, 0x81, 61}};
    return source;
}
Combatant targetActor() {
    Combatant target;
    target.id = 72;
    target.controller = 1;
    target.multipliers[4] = 0;
    return target;
}
HitInput input() {
    HitInput hit;
    hit.amount = 10;
    hit.area = hit.magic = true;
    hit.blockable = false;
    hit.attackerRecordPosition = {INT64_MIN + 17, -1, INT64_MAX - 7};
    hit.targetRecordPosition = {7, 11, 13};
    return hit;
}
}

int main() {
    struct Selection { int action, mainType, mainSubtype, offSubtype; float gain; };
    for (const auto &sample : {
        Selection{0, 3, 0, 0, .1f}, {2, 3, 0, 0, .15f}, {40, 3, 0, 0, .1f},
        {11, 3, 0, 0, .125f}, {17, 3, 0, 0, .125f}, {0, 3, 0, 12, .125f},
        {11, 3, 6, 7, .1f}, {11, 3, 0, 6, .15f}, {0, 4, 0, 0, 0.f},
        {11, 4, 0, 0, .15f}}) {
        auto source = sourceActor(), target = targetActor();
        source.action = sample.action;
        source.equipment[MAIN_HAND].type = (uint8_t)sample.mainType;
        source.equipment[MAIN_HAND].subtype = (uint8_t)sample.mainSubtype;
        source.equipment[OFF_HAND].subtype = (uint8_t)sample.offSubtype;
        // Keep ratio one even for a selected subtype with a different divisor.
        source.combo = (int32_t)comboMax(source);
        glm::vec3 velocity(0);
        int draws = 0;
        const auto result = resolveHit(target, velocity, &source, input(), [&] { draws++; return 32767; });
        const float gain = result.records.size() == 2 ? -result.records[0].amount : 0;
        check(gain == sample.gain && draws == 0 && result.hitAccepted,
              "action/item priority selects the contributing resident items without extra draws");
    }
    {
        auto source = sourceActor(), target = targetActor();
        glm::vec3 velocity(0);
        std::vector<HitEmission> emitted;
        std::vector<float> healthAtEmission;
        const auto request = input();
        const auto result = resolveHit(target, velocity, &source, request, [] { return 32767; },
            [&](const HitEmission &event) {
                emitted.push_back(event);
                healthAtEmission.push_back(source.health);
            });
        const float expectedHealth = 500.f + .1f;
        check(emitted.size() == 2 && !emitted[0].applyLocally && emitted[1].applyLocally &&
              healthAtEmission == std::vector<float>{500.f, expectedHealth} && source.health == expectedHealth &&
              source.health > maxHealth(source),
              "self record emission precedes direct uncapped source healing, which precedes main target emission");
        const auto &gain = result.records.front();
        check(gain.attacker == 41 && gain.target == 41 && gain.position == request.attackerRecordPosition &&
              gain.kind == 0 && gain.critical == 0 && gain.area == 0 && gain.beam == 0 && gain.stun == 0 &&
              gain.lockMs == 0 && gain.knockBack == glm::vec3(0) &&
              std::bit_cast<uint32_t>(gain.amount) == 0xbdcccccd &&
              result.records.back().position == request.targetRecordPosition,
              "source/self payload retains source raw coordinates, sign-toggled amount and initialized semantic fields");
    }
    {
        auto source = sourceActor(), target = targetActor();
        source.action = 11;
        source.equipment[OFF_HAND].spirits = 0;
        glm::vec3 velocity(0);
        const auto result = resolveHit(target, velocity, &source, input(), [] { return 32767; });
        check(result.records.size() == 2 && result.records[0].amount == -.05f,
              "an empty selected item still counts in the common per-item divisor");
        source = sourceActor();
        source.combo = 100;
        const auto uncapped = resolveHit(target, velocity, &source, input(), [] { return 32767; });
        check(uncapped.records.size() == 2 && uncapped.records[0].amount == -.2f,
              "source auxiliary uses its fresh ratio above one without the mitigation cap");
    }
    for (bool selected : {false, true}) {
        auto source = sourceActor(), target = targetActor();
        source.equipment[MAIN_HAND].type = selected ? 3 : 0;
        source.equipment[MAIN_HAND].spirits = 0;
        source.hasteMs = 7000;
        target.slowMs = 6000;
        glm::vec3 velocity(0);
        resolveHit(target, velocity, &source, input(), [] { return 32767; });
        check(source.hasteMs == (selected ? 5000 : 7000) && target.slowMs == (selected ? 5000 : 6000),
              "nonempty selection caps both counters even without a contributing record; empty selection skips caps");
    }
    {
        auto source = sourceActor(), target = targetActor();
        source.controller = 2;
        source.health = -.05f;
        glm::vec3 velocity(0);
        const auto result = resolveHit(target, velocity, &source, input(), [] { return 32767; });
        check(result.records.size() == 2 && source.health == -.05f + .1f,
              "direct auxiliary source gain has no source-life or numeric health-eligibility predicate");
        source = sourceActor();
        source.equipment[MAIN_HAND].spirits = 2;
        source.equipment[MAIN_HAND].cubes = {{0, 0, 0, 0x81, 21}, {0, 0, 0, 0x81, -9}};
        const auto cancelled = resolveHit(target, velocity, &source, input(), [] { return 32767; });
        check(cancelled.records.size() == 1 && source.health == 500,
              "negative transformed contributions participate before the final auxiliary positivity test");
    }
    {
        auto source = sourceActor(), target = targetActor();
        source.combo = 1;
        source.multipliers[1] = .999f; // truncated divisor 49, giving a fractional counter increase
        source.hasteMs = target.slowMs = -11;
        source.equipment[MAIN_HAND].spirits = 2;
        source.equipment[MAIN_HAND].cubes = {{0, 0, 0, 0x82, 21}, {0, 0, 0, 0x83, 21}};
        glm::vec3 velocity(0);
        const auto result = resolveHit(target, velocity, &source, input(), [] { return 32767; });
        check(source.hasteMs == 0 && target.slowMs == 0 && result.records.size() == 1,
              "counter conversion truncates the floating sum, rather than flooring the increase before integer addition");
    }
    {
        auto source = sourceActor();
        source.action = 11;
        source.spentPower = .5f;
        source.multipliers[2] = 10;
        const auto timing = phases(source, source.action);
        source.actionMs = timing.windUp + timing.active / 2;
        Body body;
        body.place({0, 0, 3});
        Npc target;
        target.fighter = targetActor();
        target.body.place({0, 1, 3});
        Npcs world;
        world.npcs().push_back(target);
        PlayerView player{source, body, 0, source.actionMs};
        NpcFrame frame;
        world.dispatchPlayerActions({&player, 1}, 16,
            [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); }, frame);
        check(frame.seen.size() == 2 && frame.emittedHits.size() == 2 &&
              frame.emittedHits[0].serialize() == frame.seen[0].first.serialize() &&
              frame.emittedHits[1].serialize() == frame.seen[1].first.serialize() &&
              frame.seen[0].first.target == source.id &&
              frame.seen[1].first.target == target.fighter.id && frame.playerHits.size() == 1 &&
              !frame.playerHits[0].applyLocally && source.health == 500.f + .125f,
              "NPC melee facade preserves self-before-target output and marks the already performed local source gain");
    }
    {
        auto source = sourceActor(), target = targetActor();
        source.equipment[MAIN_HAND].subtype = BOW;
        source.combo = 60;
        Body sourceBody, targetBody;
        sourceBody.place({20, 0, 0});
        Projectiles world;
        Projectile shot;
        shot.shooter = source.id;
        shot.kind = BURST;
        shot.special = 1;
        shot.age = 999;
        shot.damage = 10;
        world.list.push_back(shot);
        ShotEffects effects;
        world.step(1, {{&source, &sourceBody}, {&target, &targetBody}},
            [&](uint64_t id) { return id == source.id ? &source : nullptr; },
            [](int32_t, int32_t, int32_t) { return uint8_t(0); }, [] { return 32767; }, effects,
            {false, &source});
        check(effects.hits.size() == 2 && !effects.hits[0].applyLocally && effects.hits[1].applyLocally &&
              effects.hits[0].record.position[0] == sourceBody.position[0] && source.health == 500.f + .1f,
              "actual projectile producer forwards self payload without requesting a duplicate local gain");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
