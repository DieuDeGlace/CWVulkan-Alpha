#include "creatures/Npcs.h"
#include "net/RemoteFighter.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
Npc actor(uint64_t id, uint8_t role = 1) {
    Npc npc;
    npc.fighter.id = id;
    npc.fighter.controller = role;
    npc.fighter.health = 100;
    npc.body.place({0, 0, 3});
    return npc;
}
Combatant &fighter(Npcs &world, uint64_t id) {
    return std::find_if(world.npcs().begin(), world.npcs().end(),
        [&](const Npc &npc) { return npc.fighter.id == id; })->fighter;
}
}

int main() {
    {
        Npcs world;
        constexpr int64_t base = int64_t(1) << 56;
        for (const auto &[id, offset] : std::vector<std::pair<uint64_t, int64_t>>{
                 {9, 8 * 65536 - 1}, {7, 8 * 65536}, {8, 8 * 65536 + 1},
                 {uint64_t(-2), 2 * 65536}, {uint64_t(-3), 65536},
                 {uint64_t(-4), 3 * 65536}, {uint64_t(-5), 4 * 65536}, {72, 0}}) {
            Npc npc = actor(id);
            npc.body.position[0] = base + offset;
            world.npcs().push_back(npc);
        }
        fighter(world, uint64_t(-2)).health = 0;
        fighter(world, uint64_t(-3)).relations.threats[0] = -0.f;
        fighter(world, uint64_t(-4)).relations.threats[0] = std::numeric_limits<float>::quiet_NaN();
        fighter(world, uint64_t(-5)).relations.threats[0] = -1;
        auto &target = fighter(world, 72);
        target.health = 1000000000.f;
        target.relations.contact = 88;
        target.relations.threats[0] = -100000000.f;
        Combatant source;
        source.id = 0;
        source.characterClass = 3;
        source.relations.contact = 99;
        source.relations.counts[72] = 16777216.f;
        Body body;
        body.position[0] = base + 100 * 65536;
        PlayerView local{source, body, 0, 0, false, false, true};
        NpcFrame frame;
        HitRecord hit;
        hit.attacker = 0;
        hit.target = 72;
        hit.amount = 100000000.f;
        world.applyWorldHit(hit, {&local, 1}, frame);
        check(source.relations.contact == 72 && target.relations.contact == 0 &&
              source.relations.counts.at(72) == 16777216.f && target.relations.threats.at(0) == 0,
              "contact keys include zero, counts use binary32 and weighted-plus-constant precedes the former threat value");
        std::vector<uint64_t> notified;
        for (const auto &record : frame.relationships)
            notified.push_back(record.target);
        check(notified == std::vector<uint64_t>{72, uint64_t(-3), uint64_t(-2), 9} &&
              frame.relationships[0].source == 0 && frame.relationships[0].value == 0 &&
              frame.relationships[1].value == .5f && frame.relationships[2].value == .5f,
              "fanout notifies the target first then signed-key candidates, including dead actors and negative zero");
        check(!fighter(world, 7).relations.threats.contains(0) && !fighter(world, 8).relations.threats.contains(0) &&
              fighter(world, uint64_t(-5)).relations.threats.at(0) == -1 &&
              std::isnan(fighter(world, uint64_t(-4)).relations.threats.at(0)),
              "raw fixed differences retain strict eight-unit equality beyond binary64 precision and preserve nonzero/unordered threat");
        NpcFrame::Cursor cursor;
        check(frame.addedSince(cursor).relationships.size() == 4 && frame.addedSince(cursor).relationships.empty(),
              "retained relationship outputs have one new-notification view without replay");
        const auto capacity = frame.relationships.capacity();
        frame.reset();
        check(frame.relationships.empty() && frame.relationships.capacity() == capacity,
              "next-frame relationship reset retains the output allocation");
    }
    {
        Npcs world;
        Npc target = actor(72);
        target.fighter.appearanceFlags = 0x2000;
        target.fighter.action = 84;
        target.fighter.stealth = 1;
        target.body.velocity = {1, 2, 3};
        world.npcs().push_back(target);
        world.npcs().push_back(actor(41));
        fighter(world, 41).health = 0;
        NpcFrame frame;
        HitRecord hit;
        hit.attacker = 41;
        hit.target = 72;
        hit.amount = 10;
        hit.stun = 50;
        hit.lockMs = 17;
        world.applyWorldHit(hit, {}, frame);
        const auto &blocked = fighter(world, 72);
        check(blocked.health == 100 && blocked.action == 0 && blocked.hitStun == 3000 && blocked.stealth == 0 &&
              blocked.actionLockMs == 0 && blocked.relations.contact == 41 &&
              blocked.relations.threats.at(41) == 20.01f && fighter(world, 41).relations.counts.at(72) == 1 &&
              frame.relationships.size() == 1 && world.all()[0].body.velocity == glm::vec3(1, 2, 3),
              "protection rejection retains the positive prefix/contact/notification but prevents later response, lock and health");
        fighter(world, 41).controller = 0;
        fighter(world, 72).relations.threats.clear();
        hit.amount = 0;
        world.applyWorldHit(hit, {}, frame);
        check(fighter(world, 72).relations.threats.at(41) == .01f && fighter(world, 72).actionLockMs == 17,
              "a new zero-amount contribution from a dead noncontrolled role-zero actor admits the same invocation");
        fighter(world, 41).controller = 1;
        Npc contributor = actor(99, 0);
        contributor.fighter.health = 0;
        contributor.body.place({100000, 100000, 3});
        world.npcs().push_back(contributor);
        for (const float value : {0.f, -0.f, -1.f, std::numeric_limits<float>::quiet_NaN(), 1.f}) {
            auto &state = fighter(world, 72);
            state.health = 100;
            state.relations.threats[99] = value;
            hit.amount = 1;
            world.applyWorldHit(hit, {}, frame);
            check(state.health == (value > 0 ? 99 : 100),
                  "protection needs an ordered-positive resolving role-zero entry, with no contributor life or distance restriction");
        }
        auto &state = fighter(world, 72);
        state.health = 100;
        state.relations.threats[99] = 0;
        state.relations.threats[999] = 1;
        world.applyWorldHit(hit, {}, frame);
        check(state.health == 100, "a positive entry whose key does not resolve grants no protection admission");
    }
    {
        Npcs world;
        world.npcs().push_back(actor(41, 5));
        world.npcs().push_back(actor(UINT64_MAX));
        world.npcs().push_back(actor(9, 0));
        fighter(world, UINT64_MAX).health = 0;
        fighter(world, UINT64_MAX).relations.threats[1] = 0;
        fighter(world, 9).mode = 0x20;
        fighter(world, 9).relations.threats[1] = -7;
        Combatant local;
        local.id = 1;
        local.health = 50;
        Body body;
        body.place({100, 0, 3});
        PlayerView view{local, body, 0, 0, false, false, true};
        NpcFrame frame;
        HitRecord hit;
        hit.attacker = 41;
        hit.target = 1;
        hit.amount = -10;
        world.applyWorldHit(hit, {&view, 1}, frame);
        check(local.health == 60 && local.relations.contact == 0 && fighter(world, 41).relations.counts.empty() &&
              fighter(world, UINT64_MAX).relations.threats.at(41) == 1 &&
              fighter(world, 9).relations.threats.at(41) == .5f && frame.relationships.size() == 2 &&
              frame.relationships[0].target == UINT64_MAX && frame.relationships[1].target == 9,
              "healing uses source-first relationship admission and existing-key membership, then candidate-first fanout with live maps");
        for (const uint8_t category : {uint8_t(1), uint8_t(3), uint8_t(4)}) {
            frame.reset();
            local.health = 50;
            hit.kind = category;
            world.applyWorldHit(hit, {&view, 1}, frame);
            check(local.health == 50 && frame.relationships.empty(),
                  "category early exits do not reach healing or its relationship continuation");
        }
    }
    {
        Combatant first, second;
        first.controller = second.controller = 1;
        first.type = second.type = 2;
        check(!hostile(first, second), "equal role-one classifications without status do not relate");
        second.mode = 0x20;
        check(hostile(first, second), "equal role-one classifications still reach the later status-bit gate");
        first.controller = 5;
        second.controller = 0;
        check(!hostile(first, second) && hostile(second, first), "role-five then role-zero exclusion is directional and precedes status");
        first.controller = 6;
        check(!hostile(first, second) && !hostile(second, first), "either role six excludes the relationship before status");
        first.controller = second.controller = 1;
        first.type = 19;
        second.mode = 0;
        check(hostile(first, second), "different role-one classification results admit without status");
        check(harmless(89, 0) && harmless(89, 0x0400) && !harmless(0x159, 0),
              "independently rechecked kind 89 is admitted by its complete kind value, without truncation or an unrelated flag gate");
        for (const uint16_t flags : {uint16_t(0x0200), uint16_t(0x0800), uint16_t(0x1000), uint16_t(0x1a00)})
            check(!harmless(89, flags), "each native 0x1a00 bit rejects kind 89 before its kind admission");
    }
    {
        net::RemoteFighter remote;
        remote.fighter.relations.contact = 123;
        remote.fighter.relations.counts[0] = 16777216.f;
        remote.fighter.relations.threats[UINT64_MAX] = -0.f;
        Combatant snapshot;
        snapshot.health = 17;
        snapshot.relations.contact = 999;
        snapshot.relations.threats[8] = 100;
        remote.refresh(snapshot, 20);
        check(remote.fighter.health == 17 && remote.fighter.relations.contact == 123 &&
              remote.fighter.relations.counts.at(0) == 16777216.f &&
              std::bit_cast<uint32_t>(remote.fighter.relations.threats.at(UINT64_MAX)) == 0x80000000 &&
              !remote.fighter.relations.threats.contains(8),
              "core presentation refresh preserves resident relation maps and exact float bits separately");
    }
    {
        RelationshipRecord record{0x8000000000000000ull, UINT64_MAX, -0.f, {1, 2, 3, 4}};
        const auto bytes = record.serialize();
        const auto decoded = RelationshipRecord::parse(bytes.data());
        check(decoded.serialize() == bytes && bytes[7] == 0x80 && bytes[8] == 0xff &&
              bytes[19] == 0x80 && bytes[20] == 1 && bytes[23] == 4,
              "the complete 24-byte codec preserves both raw keys, negative zero and the four opaque trailing bytes");
        Combatant first, second;
        first.id = 999;
        first.health = second.health = 0;
        first.controller = second.controller = 6;
        first.relations.contact = 123;
        first.relations.counts[19] = 8;
        ActorMap<Combatant *> table{{0, &first}, {19, &second}, {77, &first}, {9, nullptr}};
        std::vector<uint64_t> lookups;
        const auto lookup = [&](uint64_t key) -> Combatant * {
            lookups.push_back(key);
            const auto found = table.find(key);
            return found == table.end() ? nullptr : found->second;
        };
        const std::array<RelationshipRecord, 1> alias{{{77, 0, -7}}};
        receiveRelationships(alias, lookup);
        check(lookups == std::vector<uint64_t>{77, 0, 77} && first.relations.threats.at(0) == -7,
              "receiver looks up target/source/target, compares IDs rather than pointers and accepts distinct aliases");
        const std::array<RelationshipRecord, 7> incoming{{
            {5, 0, 100}, {0, 9, 101}, {0, 0, 102}, {19, 19, 103},
            {0, 19, -2}, {0, 19, -0.f}, {19, 0, 11}}};
        receiveRelationships(incoming, lookup);
        check(first.relations.threats.size() == 2 && first.relations.threats.at(0) == -7 &&
              std::bit_cast<uint32_t>(first.relations.threats.at(19)) == 0x80000000 &&
              second.relations.threats.size() == 1 && second.relations.threats.at(0) == 11 &&
              first.relations.counts.at(19) == 8 && first.relations.contact == 123 &&
              !table.contains(5) && table.at(9) == nullptr,
              "later assignment skips missing/null/equal IDs, accepts dead/out-of-role actors, retains last value and changes no contact/count state");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
