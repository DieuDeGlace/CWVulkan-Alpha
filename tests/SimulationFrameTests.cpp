#include "SimulationFrame.h"
#include "Saves.h"
#include "StaticStates.h"
#include "WorldItems.h"
#include "creatures/Npcs.h"
#include <algorithm>
#include <bit>
#include <cstdio>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
}

int main() {
    for (const auto &[duration, expected] : std::vector<std::pair<int32_t, std::vector<int32_t>>>{
             {0, {0}}, {-7, {-7}}, {1, {1}}, {20, {20}}, {21, {20, 1}}, {39, {20, 19}},
             {40, {20, 20}}, {41, {20, 20, 1}}, {60, {20, 20, 20}}, {101, {20, 20, 20, 20, 20, 1}}}) {
        struct Aggregate {
            std::vector<int32_t> values{77};
            Aggregate() = default;
            Aggregate(const Aggregate &) = delete;
            Aggregate(Aggregate &&) = delete;
        } events;
        std::vector<int32_t> seen;
        int inputs = 0;
        simulation::clientFrame(duration, true, events,
            [&](int32_t elapsed, Aggregate &output) {
                check(&output == &events && output.values.front() == 77 && inputs == 0,
                      "one prepared aggregate survives every world call before input");
                seen.push_back(elapsed);
                output.values.push_back(elapsed);
            },
            [&](Aggregate &output) {
                inputs++;
                check(&output == &events && seen == expected && output.values.size() == expected.size() + 1,
                      "one input phase sees every completed world piece and retained prepared outputs");
            });
        check(seen == expected && inputs == 1, "signed durations retain exact piece count/order and final-call boundary");
    }
    {
        std::vector<int> events{3};
        int calls = 0, inputs = 0;
        simulation::clientFrame(41, false, events, [&](int32_t, auto &) { calls++; },
            [&](auto &output) { inputs++; output.push_back(4); });
        check(calls == 0 && inputs == 1 && events == std::vector<int>{3, 4},
              "failed supplied world admission preserves prepared records and still reaches later input");
    }
    {
        NpcFrame frame;
        NpcFrame::Cursor cursor;
        const auto append = [&] {
            HitRecord record;
            record.amount = (float)frame.emittedHits.size();
            frame.emittedHits.push_back(record);
            frame.playerHits.push_back(record);
            frame.sounds.push_back({});
            frame.buffs.push_back({5, 6, {6, 2, 10, 7, {1, 2, 3}, {4, 5, 6, 7}}});
            frame.kills.push_back({});
            frame.shots.push_back({});
            frame.bursts.push_back({});
            frame.deaths.push_back({});
            frame.craters.push_back({});
            frame.explosions.push_back({});
            frame.tamed.push_back({});
            frame.petItems.push_back({});
            frame.seen.push_back({record, {}});
            frame.experience += 5;
        };
        for (int index = 0; index < 2; index++) {
            append();
            const auto added = frame.addedSince(cursor);
            check(added.emittedHits.size() == 1 && added.playerHits.size() == 1 && added.sounds.size() == 1 &&
                  added.buffs.size() == 1 && added.kills.size() == 1 && added.shots.size() == 1 && added.bursts.size() == 1 &&
                  added.deaths.size() == 1 && added.craters.size() == 1 && added.explosions.size() == 1 &&
                  added.tamed.size() == 1 && added.petItems.size() == 1 && added.seen.size() == 1 && added.experience == 5 &&
                  added.emittedHits[0].amount == (float)index && added.buffs[0].record.typePadding[2] == 3 &&
                  frame.emittedHits.size() == (size_t)index + 1,
                  "new-output views preserve all retained lists, opaque buff bytes and per-piece notifications without replay");
        }
        const auto none = frame.addedSince(cursor);
        check(none.emittedHits.empty() && none.deaths.empty() && none.buffs.empty() && none.experience == 0,
              "reading an unchanged aggregate emits no duplicate application, loot or experience notification");
        const auto capacity = frame.emittedHits.capacity();
        frame.reset();
        cursor = {};
        const auto empty = frame.addedSince(cursor);
        check(frame.emittedHits.capacity() == capacity && empty.emittedHits.empty() && empty.seen.empty() &&
              empty.playerHits.empty() && empty.experience == 0, "next-frame reset clears values while retaining owned buffers");
    }
    {
        check(simulation::clockWriteDue(9980, 20) && !simulation::clockWriteDue(10000, 20) &&
              !simulation::clockWriteDue(10000, 0) && simulation::clockWriteDue(uint32_t(-10000), 1) &&
              simulation::clockWriteDue(uint32_t(INT32_MAX - 5), 10),
              "clock persistence compares signed current/future quotients after the counter increment, including wrap");
        WorldClock clock{7, 86400000};
        clock.advanceWorld(0, false);
        check(clock.day == 7 && clock.timeOfDayMs == 86400000, "day-end equality survives a zero world call");
        clock.advanceWorld(1, false);
        check(clock.day == 8 && clock.timeOfDayMs == 10, "strictly excessive clock advances its period before retaining remainder");
        clock = {UINT32_MAX, 86399990};
        clock.advanceWorld(1, false);
        clock.advanceWorld(1, false);
        check(clock.day == 0 && clock.timeOfDayMs == 10, "period counter preserves four-byte wrapping");
        clock = {4, 10};
        clock.advanceWorld(-2, false);
        check(clock.day == 4 && std::bit_cast<int32_t>(clock.timeOfDayMs) == -10, "negative elapsed has no invented lower-period wrap");
        clock = {4, 172800000};
        clock.advanceWorld(0, false);
        check(clock.day == 5 && clock.timeOfDayMs == 86400000, "repeated strict wrap retains the last period equality");
        clock = {4, 0};
        clock.advanceWorld(INT32_MIN, false);
        check(clock.day == 4 && clock.timeOfDayMs == 0, "elapsed multiplication retains low 32 bits before the signed comparison");
        clock.advanceWorld(0, true, false);
        check(clock.day == 4 && clock.timeOfDayMs == 32400000, "disabled supplied clock gate forces nine hours even at zero elapsed");
        clock = {0, 1000};
        clock.advanceWorld(20, true);
        check(clock.timeOfDayMs == 3000, "qualifying preliminary actor selects the hundred-rate clock");
    }
    {
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        zone->statics.emplace_back();
        zone->statics.back().kind = 1;
        worldgen::WorldMap map;
        map.add(zone);
        Npcs npcs;
        Npc npc;
        npc.fighter.id = 71;
        npc.fighter.controller = 1;
        npc.fighter.health = 100;
        npc.fighter.buffs = {{DAMAGE_OVER_TIME, 3, 801, 999}};
        npc.body.place({10, 10, 3});
        npc.home = npc.body.blocks();
        npcs.npcs().push_back(npc);
        Combatant local;
        local.id = 1;
        local.health = 100;
        local.actionMs = 100;
        Body body;
        body.place({0, 0, 3});
        WorldItems items;
        WorldItem item;
        item.landingMs = item.hideMs = 100;
        items.replace(0, 0, {item});
        StaticStates statics;
        statics.generated(*zone);
        Projectiles shots;
        Projectile shot;
        shot.kind = SPELL;
        shot.position[0] = shot.position[1] = int64_t(500) * 65536;
        shot.position[2] = 3 * 65536;
        shot.velocity = {1, 0, 0};
        shots.list.push_back(shot);
        NpcFrame events;
        NpcFrame::Cursor cursor;
        int calls = 0, notifications = 0, inputs = 0;
        WorldClock clock{0, 1000};
        const BlockType floor = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
        simulation::clientFrame(41, true, events,
            [&](int32_t elapsed, NpcFrame &aggregate) {
                calls++;
                regenerate(local, elapsed);
                clock.advanceWorld(elapsed, false);
                beginActionUpdate(local, elapsed);
                endActionUpdate(local, elapsed);
                items.step(elapsed, true, floor, [] { return 0; }, aggregate.sounds);
                statics.step(elapsed, [](uint64_t, glm::dvec3 &, glm::vec3 &, int32_t &) { return false; }, {{{0, 0}, zone.get()}});
                npcs.stepInto((float)elapsed, map, {{local, body, 0, 0, false, false, true}}, clock.timeOfDayMs, 0, floor, aggregate);
                const auto added = aggregate.addedSince(cursor);
                notifications += (int)added.emittedHits.size();
                ShotEffects effects;
                shots.step(elapsed, {}, {}, floor, [] { return 0; }, effects);
                check(inputs == 0 && local.action == 0, "every real world subsystem advances before current-frame primary input");
            },
            [&](NpcFrame &) {
                inputs++;
                check(local.actionMs == 141, "all common actor pieces finish before primary readiness");
                useAbilities(local, body, {true, false, false, false, false, false}, {});
            });
        check(calls == 3 && inputs == 1 && local.action == 6 && local.actionMs == 0 &&
              npcs.all()[0].fighter.health == 97 && npcs.all()[0].fighter.buffs[0].timeMs == 760 &&
              notifications == 1 && events.emittedHits.size() == 1 && events.seen.size() == 1 &&
              items.zone(0, 0)->front().landingMs == 59 && items.zone(0, 0)->front().hideMs == 59 &&
              statics.zone(0, 0)->front().timerMs == 41 && clock.timeOfDayMs == 1410 &&
              shots.list.size() == 1 && shots.list[0].age == 41 &&
              shots.list[0].position[0] == int64_t(500) * 65536 + 2685,
              "41-ms frame advances actors, periodic hits, items, statics, projectiles and clock in 20/20/1 with no replay");
    }
    {
        Combatant local;
        local.id = 1;
        local.health = 100;
        local.specialization = 1;
        local.skills[8] = 1;
        local.hitStun = 60;
        Body body;
        NpcFrame events;
        simulation::clientFrame(41, true, events,
            [&](int32_t elapsed, NpcFrame &) { beginActionUpdate(local, elapsed); endActionUpdate(local, elapsed); },
            [&](NpcFrame &aggregate) {
                const auto effect = useAbilities(local, body, {false, false, false, false, true, false}, {});
                aggregate.buffs.insert(aggregate.buffs.end(), effect.buffs.begin(), effect.buffs.end());
            });
        check(local.hitStun == 0 && local.buffs.size() == 1 && local.buffs[0].timeMs == 10000 &&
              events.buffs.size() == 1, "post-world instant buff clears stun and retains its full duration until the next world call");
        const auto speed = itemBonuses().speed;
        int queries = 0;
        itemBonuses().speed = [&](const Item &) { queries++; return 0.f; };
        local.action = 1;
        local.charge = 1;
        local.hitStun = 100;
        local.equipment[MAIN_HAND].type = 3;
        local.cooldowns.clear();
        useAbilities(local, body, {true, false, false, false, true, false}, {});
        check(queries > 0 && local.action == 1 && local.hitStun == 0 && !(body.mode & CLIMB),
              "explicit held slots still query readiness before ignoring its result and suppress primary on admission");
        itemBonuses().speed = speed;
    }
    {
        Combatant actor;
        actor.characterClass = 3;
        actor.specialization = 1;
        actor.action = 28;
        actor.actionMs = 16777217;
        Body body;
        const std::array<int32_t, 2> bindings{29, 28};
        useBoundAbilities(actor, body, bindings, {}, {});
        check(actor.action == 0 && actor.actionMs == 16777217, "released secondary beam clears only its action byte");
        actor.characterClass = 4;
        actor.action = 29;
        useBoundAbilities(actor, body, bindings, {}, {});
        check(actor.action == 0 && actor.actionMs == 16777217, "released primary channel clears only its action byte");
    }
    {
        Combatant actor;
        actor.actionMs = 100;
        actor.cooldowns[1] = 100;
        Body body;
        body.mode = CLIMB;
        const std::array<int32_t, 2> bindings{6, 257};
        useBoundAbilities(actor, body, bindings, {true, true, false, false, false, false}, {});
        check(actor.action == 1 && actor.actionMs == 0 && !(body.mode & CLIMB),
              "configured full-width binding uses its own cooldown key before its low byte becomes the action");
        actor.action = 0;
        actor.actionMs = 100;
        actor.cooldowns[257] = -1;
        body.mode = CLIMB;
        useBoundAbilities(actor, body, bindings, {true, true, false, false, false, false}, {});
        check(actor.action == 0 && actor.actionMs == 100 && !(body.mode & CLIMB),
              "negative cooldown rejects a configured held slot while retaining primary suppression and final mode clearing");
    }
    {
        Combatant actor;
        AbilityBindings bindings;
        check(bindings.values().empty(), "binding construction has no logical entries");
        for (const int32_t characterClass : {0, 1, 2, 3, 4, 5, 255}) {
            actor.characterClass = (uint8_t)characterClass;
            actor.skills[6] = 0;
            actor.skills[7] = -1;
            actor.skills[8] = INT32_MIN;
            bindings.refresh(actor);
            const auto values = bindings.values();
            const bool hasClass = characterClass >= 1 && characterClass <= 4;
            check(values.size() == (hasClass ? 5 : 2) && (!hasClass ||
                  (values[2] == 0 && values[3] == 0 && values[4] == 0)),
                  "refresh preserves two/five entries and masks nonpositive skills without removing slots");
        }
        actor.skills[6] = actor.skills[7] = actor.skills[8] = 1;
        for (const int32_t specialization : {0, 1, 2, 255}) {
            actor.specialization = (uint8_t)specialization;
            for (const auto &[characterClass, expected] :
                 std::array<std::pair<uint8_t, std::array<int32_t, 3>>, 4>{{
                     {1, {54, 86, specialization == 0 ? 102 : 101}},
                     {2, {21, 50, specialization == 1 ? 100 : 99}},
                     {3, {specialization == 1 ? 34 : 88, 103, 49}},
                     {4, {48, 79, specialization == 0 ? 97 : 96}}}}) {
                actor.characterClass = characterClass;
                bindings.refresh(actor);
                check(std::equal(expected.begin(), expected.end(), bindings.values().begin() + 2),
                      "class suffix distinguishes equality-to-one from zero/nonzero specialization");
            }
        }
        actor.characterClass = 0;
        actor.equipment[MAIN_HAND].type = 0;
        actor.action = 42;
        actor.actionMs = 0;
        for (const auto &[subtype, pair] :
             std::array<std::pair<uint8_t, std::array<int32_t, 2>>, 3>{{
                 {10, {30, 31}}, {11, {38, 46}}, {12, {40, 37}}}}) {
            actor.equipment[MAIN_HAND].subtype = subtype;
            for (const int32_t specialization : {1, 2}) {
                actor.specialization = (uint8_t)specialization;
                bindings.refresh(actor);
                const std::array<int32_t, 2> expected = specialization == 1 ?
                    (subtype == 10 ? std::array<int32_t, 2>{32, 33} : subtype == 11 ?
                     std::array<int32_t, 2>{44, 45} : std::array<int32_t, 2>{42, 43}) : pair;
                check(std::equal(expected.begin(), expected.end(), bindings.values().begin()),
                      "direct raw subtype pairs bypass category, class and primary continuation tests");
            }
        }
        actor.equipment[MAIN_HAND].subtype = 5;
        actor.equipment[OFF_HAND].type = 0;
        actor.equipment[OFF_HAND].subtype = 6;
        bindings.refresh(actor);
        check(bindings.values()[0] == 6 && bindings.values()[1] == 5,
              "empty main category precedes primary subtype five while secondary subtype five precedes off-hand six");
        actor.equipment[MAIN_HAND].type = 3;
        actor.equipment[MAIN_HAND].subtype = 8;
        bindings.refresh(actor);
        check(bindings.values()[0] == 26 && bindings.values()[1] == 25,
              "secondary uses raw off-hand six before main eight without requiring an off-hand category");
        actor.equipment[MAIN_HAND].subtype = 15;
        actor.equipment[OFF_HAND] = {};
        for (const uint8_t action : {uint8_t(57), uint8_t(67)}) {
            actor.action = action;
            const int32_t boundary = duration(actor, action) + 200;
            for (const int32_t offset : {-1, 0, 1}) {
                actor.actionMs = boundary + offset;
                bindings.refresh(actor);
                check(bindings.values()[0] == (offset < 0 ? (action == 57 ? 67 : 58) : 57),
                      "heavy chain has strict continuation boundaries and returns to 57 at equality and later");
            }
        }
    }
    {
        Combatant actor;
        actor.characterClass = 1;
        actor.specialization = 1;
        actor.skills[8] = 1;
        actor.hitStun = 200;
        AbilityBindings bindings;
        bindings.refresh(actor);
        Body body;
        NpcFrame events;
        simulation::clientFrame(41, true, events,
            [&](int32_t elapsed, NpcFrame &) {
                beginActionUpdate(actor, elapsed);
                endActionUpdate(actor, elapsed);
                actor.characterClass = 0;
                actor.skills[8] = 0;
                actor.equipment[MAIN_HAND].subtype = 10;
            },
            [&](NpcFrame &) {
                useBoundAbilities(actor, body, bindings.values(), {false, false, false, false, true, false}, {});
            });
        check(bindings.values().size() == 5 && bindings.values()[4] == 101 && actor.hasBuff(1) && actor.hitStun == 0,
              "later input executes captured binding/count despite world changes to class, equipment and skill");
        bindings.refresh(actor);
        check(bindings.values().size() == 2 && bindings.values()[0] == 32,
              "the following refresh replaces logical values and count from current state");
        actor.equipment[MAIN_HAND] = {};
        bindings.refresh(actor);
        actor.equipment[MAIN_HAND].type = 3;
        actor.equipment[MAIN_HAND].subtype = 6;
        actor.action = 0;
        actor.actionMs = 10000;
        useBoundAbilities(actor, body, bindings.values(), {true, false, false, false, false, false}, {});
        check(bindings.values()[0] == 6 && actor.action == 22,
              "ordinary primary fallback selects from current state rather than the captured bar entry");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
