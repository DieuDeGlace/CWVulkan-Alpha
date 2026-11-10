#include "creatures/Npcs.h"
#include "net/Protocol.h"
#include "net/RemoteFighter.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>

using namespace creatures;
static int failures = 0;
static void check(bool value, const char *message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); failures++; }
}
static bool gap(size_t offset) { return offset == 2 || offset == 3 || offset == 15 || offset == 18 || offset == 19; }
static ItemRecord sourceItem(int32_t count = 2) {
    ItemRecord item;
    for (size_t i = 0; i < item.size(); i++) item[i] = uint8_t(i * 31 + 7);
    item[0] = 19;
    item[1] = 19;
    setItemLevel(item, 3);
    std::memcpy(item.data() + 0x114, &count, 4);
    return item;
}
static void equip(Combatant &actor, const ItemRecord &source) {
    actor.linkedItem.source = source;
    actor.equipment[12] = Item::parse(source.data());
}

static void storageAndWire() {
    LinkedItemState fresh;
    check(itemLevel(fresh.source) == 1 && itemLevel(fresh.retained.item) == 1 && fresh.retained.word == 0 && fresh.matches(0),
          "construction establishes empty level-one current/snapshot values and a zero comparison word");
    fresh.source = sourceItem();
    fresh.retained.item.fill(0xa5);
    fresh.publish(0x80000000);
    for (size_t i = 0; i < fresh.source.size(); i++)
        check(fresh.retained.item[i] == (gap(i) ? 0xa5 : fresh.source[i]),
              "publication copies all selected bytes and retains destination alignment");
    check(fresh.matches(0x80000000) && !fresh.matches(0), "the raw comparison word independently gates equality");
    fresh.source[0x18] ^= 0xff;
    fresh.source[0x80] ^= 0xff;
    check(fresh.matches(0x80000000), "entry levels and inactive backing data alone do not invalidate comparison");
    fresh.source[0x14] ^= 0xff;
    check(!fresh.matches(0x80000000), "active entry coordinates participate in comparison");

    for (int32_t count : {0, -7, 2, 32, 33}) {
        const auto source = sourceItem(count);
        net::CreatureRecord record;
        record.setLinkedItem(source);
        auto actor = record.fighter();
        check(actor.linkedItem.source == source && actor.equipment[12].product == 0x5c3d1eff &&
              actor.equipment[12].flags == source[14], "core decode retains the full source and previously omitted header fields");
        actor.linkedItem.retained.item.fill(0x22);
        actor.linkedItem.retained.word = 0xffffffff;
        net::CreatureRecord encoded;
        encoded.setFighter(actor);
        check(encoded.fighter().linkedItem.source == source,
              "the actual codec round-trips every source byte across signed, full and malformed counts without serializing the snapshot");
        net::RemoteFighter remote;
        remote.fighter.linkedItem.source.fill(0xa7);
        remote.fighter.linkedItem.retained.item.fill(0xa6);
        remote.fighter.linkedItem.retained.word = 0x12345678;
        remote.refresh(actor, 0);
        for (size_t i = 0; i < source.size(); i++)
            check(remote.fighter.linkedItem.source[i] == (gap(i) ? 0xa7 : source[i]),
                  "core assignment copies every selected source byte and retains the destination's five source alignment bytes");
        check(remote.fighter.linkedItem.retained.word == 0x12345678 &&
              std::all_of(remote.fighter.linkedItem.retained.item.begin(), remote.fighter.linkedItem.retained.item.end(),
                          [](uint8_t byte) { return byte == 0xa6; }),
              "core assignment preserves the destination actor's separate resident snapshot/word");
    }
}

static void actualCreditStorage() {
    Combatant owner, victim, linked;
    owner.id = 1;
    owner.linkedId = 3;
    auto original = sourceItem();
    original[0] = 7; // linked credit has no pet-category requirement
    equip(owner, original);
    owner.linkedItem.retained.item.fill(0xa5);
    owner.linkedItem.retained.word = 99;
    linked.health = 4;
    linked.level = 0;
    linked.experience = 17;
    ActorMap<Combatant *> actors{{1, &owner}, {2, &victim}, {3, &linked}};
    CreditWorld world{[&](uint64_t key) -> Combatant * {
        const auto found = actors.find(key);
        return found == actors.end() ? nullptr : found->second;
    }, []() -> Combatant * { return nullptr; }, 0};
    const auto result = receiveCredit({1, 2, 0, 7}, world);
    const uint32_t experience = 24;
    std::memcpy(original.data() + 4, &experience, 4);
    setItemLevel(original, 0);
    check(result.linkedAward == &linked && linked.experience == 24 && owner.linkedItem.source == original &&
          owner.equipment[12].seed == 24 && owner.equipment[12].level == 0 && owner.linkedItem.retained.word == 99,
          "the real credit consumer uses actor-owned full storage, stamps final XP/level and leaves the comparison word alone");
    for (size_t i = 0; i < original.size(); i++)
        check(owner.linkedItem.retained.item[i] == (gap(i) ? 0xa5 : original[i]),
              "default linked credit copies all selected source bytes, including inactive entries");
}

struct PetFixture {
    worldgen::Random random{123};
    Npcs npcs{random};
    worldgen::WorldMap map;
    Combatant owner;
    Body body;
    NpcFrame frame;
    PetFixture() {
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        owner.id = 1;
        owner.level = 10;
        owner.health = 0; // selected role-zero actors do not need positive health
        equip(owner, sourceItem());
        body.place({20, 20, 3});
        npcs.setNamedWorld(true);
    }
    void step(bool local = false) {
        npcs.stepInto(0, map, {{owner, body, 0, 0, false, false, local}}, 1000, 0,
            [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); }, frame);
    }
};

static void maintenancePublication() {
    PetFixture f;
    f.owner.linkedItem.retained.item.fill(0xa5);
    f.owner.skills[0] = 3;
    f.step();
    check(f.npcs.all().size() == 1 && f.owner.linkedId != 0 && f.owner.linkedItem.retained.word == 3 &&
          sameItem(f.owner.linkedItem.source, f.owner.linkedItem.retained.item),
          "a dead role-zero owner still reaches publication and creates its missing linked actor");
    if (f.npcs.all().empty()) return;
    auto &child = f.npcs.npcs()[0];
    check(std::all_of(child.fighter.linkedItem.source.begin(), child.fighter.linkedItem.source.end(),
                     [](uint8_t byte) { return byte == 0; }) && itemLevel(child.fighter.linkedItem.retained.item) == 1,
          "child reset clears the source level to zero while retaining its separately constructed snapshot");
    child.fighter.cooldowns[1234] = 55;
    f.owner.equipment[12].cubes[0].level ^= 0x12345678;
    f.owner.linkedItem.source[0x80] ^= 0x55;
    f.step();
    check(child.fighter.cooldowns.contains(1234) && f.owner.linkedItem.retained.item[0x80] == f.owner.linkedItem.source[0x80],
          "ignored byte changes do not reconstruct but still publish all backing entries");
    child.fighter.linkedItem.retained.item.fill(0x77);
    child.fighter.linkedItem.retained.word = 123;
    child.fighter.mode = child.body.mode = 0xa620;
    const std::array<int64_t, 3> reference{0x123456789abcdef, -65537, 999};
    child.fighter.behaviorReference = reference;
    child.creationConfig = {0x12345601, {96, 17, 96}};
    f.owner.equipment[12].cubes[0].x++;
    f.step();
    check(!child.fighter.cooldowns.contains(1234) && child.fighter.linkedItem.retained.word == 123 &&
          child.fighter.linkedItem.retained.item[0] == 0x77 && itemLevel(child.fighter.linkedItem.source) == 0,
          "a compared coordinate reconstructs the real child without discarding its resident snapshot");
    check(child.fighter.mode == 0 && child.body.mode == 0,
          "actual pet reconstruction resets the complete mode word without reviving flags from the old body");
    check(child.fighter.behaviorReference == reference,
          "actual linked reconstruction preserves the reference field skipped by the native body reset");
    check(child.creationConfig.choice == 0x12345601 && child.creationConfig.kinds == std::vector<int32_t>{96, 17, 96},
          "actual linked reconstruction preserves resident creation configuration outside the replaced body");
    child.fighter.cooldowns[1234] = 55;
    f.owner.skills[0]++;
    f.step();
    check(!child.fighter.cooldowns.contains(1234), "comparison-word change alone reconstructs an otherwise equal item");
    const auto retained = f.owner.linkedItem.retained;
    f.owner.equipment[12] = Item{};
    f.step();
    check(f.owner.linkedItem.retained.word == retained.word && f.owner.linkedItem.retained.item == retained.item,
          "the other-category removal route retains both snapshot and comparison word");

    PetFixture blocked;
    blocked.body.mode |= GLIDER;
    blocked.owner.skills[0] = 8;
    blocked.step();
    check(blocked.npcs.all().empty() && blocked.owner.linkedItem.retained.word == 8 &&
          sameItem(blocked.owner.linkedItem.source, blocked.owner.linkedItem.retained.item),
          "snapshot publication does not require successful child allocation/admission");
    PetFixture excluded;
    excluded.owner.controller = 2;
    const auto before = excluded.owner.linkedItem.retained;
    excluded.step();
    check(excluded.npcs.all().empty() && excluded.owner.linkedItem.retained.item == before.item &&
          excluded.owner.linkedItem.retained.word == before.word, "non-role-zero views do not enter selected maintenance");
}

static void lateMirror() {
    PetFixture f;
    f.npcs.setNamedWorld(false);
    Npc child;
    child.fighter.id = UINT64_MAX;
    child.fighter.controller = 5;
    child.fighter.ownerId = f.owner.id;
    child.fighter.health = 17;
    child.fighter.level = 9;
    child.fighter.experience = 1234;
    child.spawn = Npc::NO_SPAWN;
    child.body.place({20, 20, 3});
    f.npcs.npcs().push_back(child);
    f.owner.linkedId = child.fighter.id;
    f.owner.linkedItem.retained.word = 99;
    const auto before = f.owner.linkedItem.retained;
    f.step(true);
    check(f.owner.equipment[12].level == 9 && f.owner.equipment[12].seed == 1234 &&
          itemLevel(f.owner.linkedItem.source) == 9 && f.owner.linkedItem.retained.word == before.word &&
          f.owner.linkedItem.retained.item == before.item && f.frame.petItems.size() == 1,
          "later controlled mirroring updates current source even with maintenance skipped, without refreshing comparison state");
}

int main() {
    storageAndWire();
    actualCreditStorage();
    maintenancePublication();
    lateMirror();
    if (!failures) std::puts("LinkedItemTests passed");
    return failures ? 1 : 0;
}
