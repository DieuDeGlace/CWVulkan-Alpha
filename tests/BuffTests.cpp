#include "creatures/Buffs.h"
#include "creatures/Combat.h"
#include "creatures/Npcs.h"
#include "creatures/Strike.h"
#include "net/Protocol.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *description) {
    if (!okay) { std::printf("FAIL %s\n", description); failures++; }
}
}

int main() {
    {
        Combatant source, target;
        source.id = 0; // a valid resolved creature identifier
        source.action = 1;
        target.id = 72;
        target.buffs = {{1, 2.f, 0}, {1, .5f, 0}};
        glm::vec3 velocity(0);
        HitInput hit;
        hit.amount = 10;
        hit.blockable = false;
        hit.selector = 17;
        int draws = 0;
        auto produced = resolveHit(target, velocity, &source, hit, [&] { draws++; return 32767; });
        check(draws == 0 && target.buffs.size() == 3 && produced.buffs.size() == 1 &&
              produced.records.size() == 1 && produced.records[0].amount == 8.f,
              "selector-17 appends after all resident multipliers with no producer draw or change to the direct-hit snapshot");
        const auto &event = produced.buffs.front();
        check(event.source == 0 && event.target == 72 && event.record.source == 0 &&
              event.record.kind == 4 && event.record.timeMs == 3000 && event.record.value == .8f &&
              event.record.serialize() == target.buffs.back().serialize(),
              "produced record and outgoing event share semantic payload and the resolved numeric-zero attacker source");
        resolveHit(target, velocity, &source, hit, [] { return 32767; });
        check(target.buffs.size() == 4, "repeated selector production appends rather than refreshing the first periodic record");
        source.action = 17;
        hit.selector = 0;
        const auto ordinary = resolveHit(target, velocity, &source, hit, [] { return 32767; });
        check(ordinary.buffs.empty() && target.buffs.size() == 4,
              "the attacker's current action does not substitute for the explicit full-width resolver selector");
        hit.selector = 273;
        check(resolveHit(target, velocity, &source, hit, [] { return 32767; }).buffs.empty(),
              "selector comparison uses all thirty-two bits rather than its low byte");
    }
    {
        Combatant actor;
        actor.id = 41;
        actor.action = 17;
        actor.spentPower = .5f;
        const Phases timing = phases(actor, 17);
        actor.actionMs = timing.windUp + timing.active / 2;
        Body body;
        body.place({0, 0, 3});
        Npcs npcs;
        Npc target;
        target.fighter.id = 72;
        target.fighter.controller = 1;
        target.body.place({0, 1, 3});
        npcs.npcs().push_back(target);
        NpcFrame frame;
        PlayerView view{actor, body, 0, actor.actionMs};
        npcs.dispatchPlayerActions({&view, 1}, 16,
            [](int32_t, int32_t, int32_t) { return uint8_t(0); }, frame);
        check(frame.buffs.size() == 1 && npcs.all()[0].fighter.buffs.size() == 1 &&
              frame.buffs[0].source == 41 && frame.buffs[0].target == 72 &&
              frame.buffs[0].record.serialize() == npcs.all()[0].fighter.buffs[0].serialize(),
              "actual melee dispatch applies one resident and forwards one copied periodic event without double application");
    }
    {
        std::array<uint8_t, 40> bytes{};
        for (size_t index = 0; index < bytes.size(); index++) bytes[index] = (uint8_t)(index * 37 + 11);
        const uint64_t outer = 0x1122334455667788, target = 0x8877665544332211, embedded = 0xfedcba9876543210;
        const int32_t remaining = -123456789;
        const uint32_t valueBits = 0x7fc12345;
        std::memcpy(bytes.data(), &outer, 8);
        std::memcpy(bytes.data() + 8, &target, 8);
        bytes[16] = 6;
        std::memcpy(bytes.data() + 20, &valueBits, 4);
        std::memcpy(bytes.data() + 24, &remaining, 4);
        std::memcpy(bytes.data() + 32, &embedded, 8);
        const BuffEvent event = BuffEvent::parse(bytes.data());
        check(event.source == outer && event.target == target && event.record.source == embedded &&
              event.record.timeMs == remaining, "outer identities and embedded source/time remain independent");
        check(event.serialize() == bytes, "all forty bytes round-trip, including opaque alignment and float payload bits");
        Combatant receiver;
        receiver.health = 0;
        applyBuff(receiver, event.record);
        const auto payload = receiver.buffs.front().serialize();
        check(std::equal(payload.begin(), payload.end(), bytes.begin() + 16),
              "record application retains complete payload without a target-health or positive-time gate");
    }
    for (uint8_t kind : {1, 2, 3, 6, 7, 9, 10, 11}) {
        Combatant receiver;
        receiver.buffs = {{kind, 1, 100, 10}, {4, 2, 100, 20}, {kind, 3, 100, 30}};
        const Buff replacement{kind, 4, 0, 40};
        applyBuff(receiver, replacement);
        check(receiver.buffs.size() == 3 && receiver.buffs[0].source == 40 && receiver.buffs[0].timeMs == 0 &&
              receiver.buffs[1].kind == 4 && receiver.buffs[2].source == 30,
              "selected types overwrite only the first match in place and retain zero-time nodes");
    }
    for (uint8_t kind : {4, 5, 8, 12, 255}) {
        Combatant receiver;
        receiver.buffs = {{kind, 1, 100, 10}, {kind, 2, 100, 20}};
        applyBuff(receiver, {kind, 3, -10, 30});
        check(receiver.buffs.size() == 3 && receiver.buffs.back().source == 30,
              "nonreplacement and unknown types append in order even with negative time");
    }
    {
        Combatant receiver;
        const BuffEvent self{77, 77, {11, 1, 30000}};
        const auto copied = BuffEvent::parse(self.serialize().data());
        applyBuff(receiver, copied.record);
        check(receiver.buffs[0].source == 0, "self event does not synthesize an embedded source from its outer ID");
        receiver.buffs[0].timeMs = 16777217;
        tickBuffs(receiver, 1, false);
        check(receiver.buffs[0].timeMs == 16777216, "remaining time preserves integer precision beyond binary32");
    }
    for (bool authoritative : {false, true}) {
        Combatant receiver;
        receiver.buffs = {{4, 2, 800, 0}};
        int calls = 0;
        tickBuffs(receiver, 401, authoritative, [&](const Buff &record) {
            check(record.source == 0 && record.timeMs == 399, "periodic work uses preserved source and advanced signed time");
            calls++;
        });
        check(calls == (authoritative ? 1 : 0), "multiple crossed buckets cause one call only on the authoritative side");
    }
    {
        Combatant actor;
        actor.id = 41;
        actor.action = 17;
        actor.mana = .5f;
        actor.buffs = {{11, 4, 1000, 99}};
        std::vector<BuffEvent> events;
        beginActionUpdate(actor, 16, &events);
        check(actor.hasBuff(11) && actor.buffs[0].timeMs == 0 && actor.buffs[0].value == 0 &&
              actor.buffs[0].source == 0 && events.size() == 1 && events[0].source == 41 && events[0].target == 41 &&
              events[0].record.source == 0, "melee consumption replaces in place with the copied zero-time self event");
        endActionUpdate(actor, 16);
        check(!actor.hasBuff(11) && events[0].record.timeMs == 0,
              "later countdown unlinks consumed record without changing the earlier event copy");
        int draws = 0;
        const auto forced = strikeInput(actor, {}, glm::vec3(1), {0, 1, 0}, glm::vec3(1), [&] { draws++; return 32767; });
        check(actor.criticalLatched && forced.critical && draws == 1,
              "consumed critical latch survives expiry and skips the normal critical random draw");
        actor.criticalLatched = false;
        draws = 0;
        const auto ordinary = strikeInput(actor, {}, glm::vec3(1), {0, 1, 0}, glm::vec3(1), [&] { draws++; return 32767; });
        check(!ordinary.critical && draws == 2 && forced.amount == 2.f * ordinary.amount,
              "unlatched endpoint fails the chance test while the latched amount is doubled");
        actor.criticalLatched = true;
        actor.action = 31;
        actor.actionMs = 0;
        beginActionUpdate(actor, 0);
        check(actor.criticalLatched, "staff charging path bypasses the common zero-clock latch reset");
        actor.action = 1;
        beginActionUpdate(actor, 0);
        check(!actor.criticalLatched, "ordinary common zero-clock initialization resets the latch");
    }
    for (bool alive : {false, true}) {
        Combatant player;
        player.id = 1;
        player.health = alive ? 100.f : 0.f;
        Body body;
        worldgen::WorldMap map;
        Npcs npcs;
        const BuffEvent incoming{999, 1, {12, 1, 17, 123}};
        npcs.step(16, map, {{player, body, 0, 0, false, true}}, 43200000, 0,
            [](int32_t, int32_t, int32_t) { return uint8_t(0); }, {}, {}, {incoming});
        check(player.buffs.size() == 1 && player.buffs[0].timeMs == (alive ? 1 : 17),
              "server-role player receives buff input and only living admission advances its resident clock");
    }
    {
        Combatant player;
        player.id = 1;
        Body body;
        body.place({0, 0, 3});
        Npc target;
        target.fighter.id = 9;
        target.fighter.controller = 1;
        target.body.place({0, 1, 3});
        target.home = target.body.blocks();
        target.zone = {0, 0};
        Npcs npcs;
        npcs.npcs().push_back(target);
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        HitRecord hit;
        hit.attacker = 1;
        hit.target = 9;
        hit.kind = HIT_ABSORBED;
        hit.amount = 5;
        const BuffEvent incoming{999, 9, {6, 12, 17, 123}};
        npcs.step(16, map, {{player, body, 0, 0, false}}, 43200000, 0,
            [](int32_t, int32_t, int32_t z) { return (uint8_t)(z <= 0 ? 1 : 0); }, {hit}, {}, {incoming});
        const auto &buffs = npcs.all()[0].fighter.buffs;
        check(buffs.size() == 1 && buffs[0].value == 12 && buffs[0].timeMs == 1 && buffs[0].source == 123,
              "world facade applies input hits before buff records and counts down the new record in the same step");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
