#include "Loot.h"
#include "creatures/Strike.h"
#include <algorithm>
#include <array>
#include <cstdio>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
const BlockType floorBlock = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
Npc victim(uint64_t id, glm::dvec3 position = {10, 10, 3}) {
    Npc result;
    result.fighter.id = id;
    result.fighter.type = 4;
    result.fighter.controller = 1;
    result.fighter.health = 1;
    result.fighter.actionMs = 77;
    result.spawn = Npc::NO_SPAWN;
    result.body.place(position);
    result.home = position;
    ItemRecord feather{};
    feather[0] = 11;
    feather[1] = 2;
    setItemLevel(feather, 1);
    result.inventory.push_back(feather); // guaranteed admitted inventory drop
    return result;
}
std::array<uint8_t, 172> appearance(const Appearance &looks) {
    std::array<uint8_t, 172> bytes{};
    looks.serialize(bytes.data());
    return bytes;
}
}

int main() {
    {
        worldgen::Random shared(123), reference(123);
        Npcs first(shared), second(shared);
        const auto nextReference = [&] {
            Appearance expected;
            int32_t kind = 144;
            generate(kind, expected, nullptr, [&] { return reference.draw(); });
            return appearance(expected);
        };
        const auto firstExpected = nextReference();
        first.placeBomb({10, 10, 3}, 1);
        check(appearance(first.all()[0].looks) == firstExpected && shared.count() == reference.count(),
              "an actual actor producer uses its supplied world stream without an extra draw");
        check(shared.draw() == reference.draw(), "presentation observes the producer's advanced state");
        const auto secondExpected = nextReference();
        second.placeBomb({10, 10, 3}, 1);
        check(appearance(second.all()[0].looks) == secondExpected && shared.count() == reference.count(),
              "another actor subsystem continues that stream after intervening presentation");
        first.reseed(0xfffffff0u);
        reference.seed(0xfffffff0u);
        const auto reseededExpected = nextReference();
        second.placeBomb({11, 10, 3}, 1);
        check(appearance(second.all()[1].looks) == reseededExpected && shared.count() == reference.count() &&
              shared.draw() == reference.draw(),
              "one actor subsystem's reseed affects subsequent users without restoring the old state");
    }
    {
        Combatant fighter;
        fighter.type = 4;
        fighter.controller = 1;
        fighter.appearanceFlags = 0x200;
        fighter.level = 17;
        fighter.powerTier = 3;
        fighter.equipment[MAIN_HAND] = Item{3, 1, 2, 4, 7, 123};
        Body body;
        body.position[0] = (int64_t(1) << 56) + 3;
        body.position[1] = -(int64_t(1) << 56) - 5;
        body.position[2] = 17;
        ItemRecord item{};
        item.fill(0xcc);
        item[0] = 11;
        const auto live = lootVictim(fighter, body, {&item, 1});
        NpcFrame::Death death;
        death.fighter = fighter;
        death.appearanceFlags = fighter.appearanceFlags;
        std::copy_n(body.position, 3, death.position);
        death.inventory = {item};
        const auto delayed = lootVictim(death);
        check(std::equal(std::begin(live.position), std::end(live.position), body.position) &&
              live.inventory.size() == 1 && live.inventory[0].item == item &&
              live.equipment == delayed.equipment && live.appearanceFlags == delayed.appearanceFlags &&
              live.level == delayed.level && live.tier == delayed.tier,
              "live loot retains raw integer positions/full inventory bytes and the established equipment adapter");
    }
    {
        worldgen::Random shared(123);
        Npcs world(shared);
        world.npcs().push_back(victim(72));
        world.npcs().push_back(victim(73));
        world.npcs()[0].fighter.linkedId = 73;
        world.npcs()[1].fighter.health = 17;
        worldgen::WorldMap map;
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        map.add(zone);
        WorldItems items;
        items.generated(*zone);
        NpcFrame frame;
        int callbacks = 0;
        world.onFatalLoot = [&](const Combatant &fighter, const Body &body, std::span<const ItemRecord> inventory) {
            callbacks++;
            check(fighter.health == 0 && fighter.actionMs == 0 && frame.sounds.size() == 1 &&
                  frame.sounds[0].sound == 27 && world.all()[1].fighter.health == 17,
                  "loot sees first presentation/clock reset while the linked actor remains alive");
            dropLoot(lootVictim(fighter, body, inventory), items, 9, floorBlock, shared);
        };
        HitRecord hit;
        hit.attacker = 999;
        hit.target = 72;
        hit.amount = 1;
        world.applyWorldHit(hit, {}, frame);
        const uint32_t draws = shared.count();
        check(callbacks == 1 && draws > 0 && !items.zone(0, 0)->empty() && frame.deaths.empty() &&
              world.all()[1].fighter.health == 0 && frame.sounds.size() == 1,
              "drops are resident before return and linked health is zeroed afterward without recursive fatal work");
        world.applyWorldHit(hit, {}, frame);
        check(callbacks == 1 && shared.count() == draws, "dead-target repetition cannot replay loot or draws");
        world.stepInto(0, map, {}, 1000, 9, floorBlock, frame);
        check(callbacks == 1 && shared.count() == draws && frame.deaths.size() == 2 &&
              frame.deaths[0].lootApplied && !frame.deaths[1].lootApplied,
              "death outputs distinguish completed fatal loot from the linked actor's non-hit adapter");
    }
    for (const uint64_t link : {uint64_t(0), uint64_t(999), uint64_t(72)}) {
        Npcs world;
        world.npcs().push_back(victim(72));
        world.npcs()[0].fighter.linkedId = link;
        world.npcs().push_back(victim(0));
        world.npcs()[1].fighter.health = 17;
        HitRecord hit;
        hit.attacker = 999;
        hit.target = 72;
        hit.amount = 1;
        NpcFrame frame;
        world.applyWorldHit(hit, {}, frame);
        check(world.all()[0].fighter.health == 0 && world.all()[1].fighter.health == 17 && frame.sounds.size() == 1,
              "fatal links use a zero sentinel, tolerate missing/self keys and never substitute key zero");
    }
    {
        worldgen::Random shared(123);
        Npcs world(shared);
        worldgen::WorldMap map;
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        map.add(zone);
        WorldItems items;
        items.generated(*zone);
        world.npcs().push_back(victim(72, {10, 11, 3}));
        world.npcs().push_back(victim(73, {10, 12, 3}));
        Combatant attacker;
        attacker.id = 1;
        attacker.health = 100;
        attacker.action = 1;
        attacker.multipliers[2] = 1000;
        const auto timing = phases(attacker, 1);
        attacker.actionMs = timing.windUp + timing.active / 2;
        Body body;
        body.place({10, 10, 3});
        std::vector<PlayerView> players{{attacker, body, 0, attacker.actionMs, true, false, true}};
        std::vector<uint64_t> fatalities;
        std::vector<uint32_t> drawCounts;
        world.onFatalLoot = [&](const Combatant &fighter, const Body &targetBody, std::span<const ItemRecord> inventory) {
            if (!fatalities.empty())
                check(!items.zone(0, 0)->empty() && shared.count() > drawCounts.back(),
                      "the next melee target sees prior drops and the continued random stream");
            fatalities.push_back(fighter.id);
            dropLoot(lootVictim(fighter, targetBody, inventory), items, 0, floorBlock, shared);
            drawCounts.push_back(shared.count());
        };
        NpcFrame frame;
        world.dispatchPlayerActions(players, 16, floorBlock, frame);
        check(fatalities == std::vector<uint64_t>{72, 73} && frame.deaths.empty() && items.zone(0, 0)->size() >= 2,
              "one production melee pass creates both targets' loot before the corpse phase");
        const uint32_t afterStrike = shared.count();
        players[0].strikes = false;
        world.stepInto(0, map, players, 1000, 0, floorBlock, frame);
        int delayedCalls = 0;
        for (const auto &death : frame.deaths)
            if (!death.lootApplied) {
                delayedCalls++;
                dropLoot(lootVictim(death), items, 0, floorBlock, shared);
            }
        check(frame.deaths.size() == 2 && delayedCalls == 0 && shared.count() == afterStrike,
              "client/server death-output policy does not duplicate synchronous drops or consume draws again");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
