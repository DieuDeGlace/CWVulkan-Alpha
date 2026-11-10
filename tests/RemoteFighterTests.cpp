#include "net/RemoteFighter.h"
#include "net/Protocol.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>

using namespace creatures;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { ++failures; std::printf("FAIL %s\n", message); }
}

static void currentState() {
    net::RemoteFighter remote;
    Buff resident{9, .25f, -3, 42};
    resident.typePadding = {7, 8, 9};
    resident.timePadding = {1, 2, 3, 4};
    remote.fighter.buffs.push_back(resident);
    Combatant snapshot;
    snapshot.id = 42;
    snapshot.action = 37;
    snapshot.actionMs = 100;
    snapshot.health = 90;
    remote.refresh(snapshot, 20);
    check(remote.fighter.actionMs == 100 && remote.fighter.health == 90,
          "first snapshot starts at the received clock without an extra frame increment");
    snapshot.health = 17;
    snapshot.combo = 23;
    snapshot.multipliers[1] = .75f;
    snapshot.equipment[MAIN_HAND].type = 3;
    snapshot.equipment[MAIN_HAND].subtype = WAND;
    snapshot.equipment[MAIN_HAND].spirits = 1;
    snapshot.equipment[MAIN_HAND].cubes = {{1, 2, 3, 129}};
    snapshot.mode = 0x0200;
    snapshot.appearanceFlags = 0x0210;
    snapshot.buffs = {{255, 9, 1000, 99}};
    remote.refresh(snapshot, 20);
    check(remote.fighter.actionMs == 120 && remote.fighter.health == 17 && remote.fighter.combo == 23 &&
              remote.fighter.multipliers[1] == .75f && remote.fighter.mode == 0x0200 &&
              remote.fighter.appearanceFlags == 0x0210 && remote.fighter.mainHand().subtype == WAND &&
              remote.fighter.mainHand().cubes.size() == 1 && remote.fighter.mainHand().cubes[0].material == 129,
          "unchanged action/clock still publishes current health, counters, stats, equipment and flags");
    check(remote.fighter.buffs.size() == 1 && remote.fighter.buffs[0].serialize() == resident.serialize(),
          "core refresh preserves every resident buff byte without consuming, expiring or accepting snapshot buffs");
    snapshot.actionMs = 101;
    remote.refresh(snapshot, 20);
    check(remote.fighter.actionMs == 101, "a changed received clock resets the presentation clock");
    snapshot.action = 38;
    snapshot.actionMs = 101;
    remote.refresh(snapshot, 20);
    check(remote.fighter.actionMs == 101 && remote.fighter.action == 38,
          "a changed action resets even when the received clock is equal");
    snapshot.health = 0;
    remote.refresh(snapshot, 0);
    check(remote.fighter.health == 0 && remote.fighter.actionMs == 101,
          "death and zero-duration frames still refresh received core fields");
    snapshot.actionMs = std::numeric_limits<int32_t>::max();
    remote.refresh(snapshot, 0);
    remote.refresh(snapshot, 1);
    check(remote.fighter.actionMs == std::numeric_limits<int32_t>::min(),
          "the engine presentation counter has defined native-width wrap instead of signed overflow");
}

static void wireMode() {
    net::CreatureRecord record;
    record.bytes[0x116] = 0xa5;
    record.bytes[0x117] = 0x5a;
    Combatant snapshot;
    snapshot.mode = 0x12340200;
    snapshot.actionMs = 9;
    record.setFighter(snapshot);
    check(record.bytes[0x114] == 0 && record.bytes[0x115] == 2 &&
              record.bytes[0x116] == 0xa5 && record.bytes[0x117] == 0x5a && record.fighter().mode == 0x0200,
          "semantic mode transfer uses exactly the existing two-byte property and preserves neighboring opaque bytes");
    net::RemoteFighter remote;
    remote.refresh(record.fighter(), 0);
    net::CreatureRecord changed = record;
    snapshot = changed.fighter();
    snapshot.mode = 0x8041;
    changed.setFighter(snapshot);
    const uint64_t mask = net::changedFields(changed, &record);
    check(mask == (uint64_t(1) << 14), "a mode-only edit changes only existing wire field14");
    check(net::applyDelta(net::encodeDelta(42, changed, mask), record) == 42, "a mode-only partial delta decodes");
    remote.refresh(record.fighter(), 20);
    check(remote.fighter.mode == 0x8041 && remote.fighter.actionMs == 29 &&
              record.bytes[0x116] == 0xa5 && record.bytes[0x117] == 0x5a,
          "mode-only reception reaches presentation without changing the action clock policy or padding");
    changed = record;
    snapshot = changed.fighter();
    snapshot.health = 7;
    changed.setFighter(snapshot);
    check(net::applyDelta(net::encodeDelta(42, changed, uint64_t(1) << 27), record) == 42,
          "a health-only partial delta decodes");
    remote.refresh(record.fighter(), 1);
    check(remote.fighter.health == 7 && remote.fighter.mode == 0x8041 && remote.fighter.actionMs == 30,
          "omitted mode retains its previous value while health refreshes during an unchanged action");
}

static void equipmentStorage() {
    constexpr size_t equipmentOffset = 0x2f0;
    constexpr std::array<size_t, 5> gaps{2, 3, 15, 18, 19};
    constexpr std::array<int32_t, 5> counts{-7, 0, 2, 32, 33};
    std::array<ItemRecord, SLOT_COUNT> originals;
    net::CreatureRecord wire;
    for (size_t slot = 0; slot < originals.size(); slot++) {
        auto &item = originals[slot];
        for (size_t byte = 0; byte < item.size(); byte++)
            item[byte] = uint8_t(byte * 29 + slot * 17 + 3);
        const int32_t count = counts[slot % counts.size()];
        std::memcpy(item.data() + 0x114, &count, 4);
        std::copy(item.begin(), item.end(), wire.bytes.begin() + equipmentOffset + slot * item.size());
    }
    auto snapshot = wire.fighter();
    net::CreatureRecord roundTrip;
    roundTrip.setFighter(snapshot);
    for (size_t slot = 0; slot < originals.size(); slot++) {
        check(snapshot.equipmentSource(slot) == originals[slot],
              "all thirteen decoded equipment slots retain every source byte across signed/full/oversized counts");
        check(std::equal(originals[slot].begin(), originals[slot].end(),
                         roundTrip.bytes.begin() + equipmentOffset + slot * originals[slot].size()),
              "the real outgoing codec retains inactive entries and all source gaps in every slot");
    }

    net::RemoteFighter remote;
    Combatant copied;
    remote.fighter.linkedItem.retained.word = copied.linkedItem.retained.word = 99;
    copied.health = 17;
    copied.type = 999;
    for (size_t slot = 0; slot < originals.size(); slot++) {
        remote.fighter.equipmentSource(slot).fill(uint8_t(0xe0 + slot));
        copied.equipmentSource(slot).fill(uint8_t(0xc0 + slot));
    }
    remote.refresh(snapshot, 0);
    copied.copyEquipmentFrom(snapshot);
    for (size_t slot = 0; slot < originals.size(); slot++) {
        auto expectedRemote = originals[slot], expectedCopy = originals[slot];
        for (const size_t gap : gaps) {
            expectedRemote[gap] = uint8_t(0xe0 + slot);
            expectedCopy[gap] = uint8_t(0xc0 + slot);
        }
        check(remote.fighter.equipmentRecord(slot) == expectedRemote && copied.equipmentRecord(slot) == expectedCopy,
              "real core refresh and equipment assignment copy all body entries while retaining each destination's five gaps");
    }
    check(copied.health == 17 && copied.type == 999 && copied.linkedItem.retained.word == 99 &&
          remote.fighter.linkedItem.retained.word == 99,
          "equipment copying changes no other actor state or resident item comparison word");
    const auto before = copied.equipmentRecord(1);
    copied.copyEquipmentFrom(copied);
    check(copied.equipmentRecord(1) == before, "self equipment assignment retains the full record");

    snapshot.equipmentSource(1)[0x113] ^= 0x5a; // count zero: no semantic cube represents this byte
    net::CreatureRecord changed = roundTrip;
    changed.setFighter(snapshot);
    const auto mask = net::changedFields(changed, &roundTrip);
    check(mask == (uint64_t(1) << 44), "an inactive nonlinked equipment edit changes only the existing equipment wire group");
    check(net::applyDelta(net::encodeDelta(77, changed, mask), roundTrip) == 77,
          "the real partial-delta reader adopts the equipment group");
    remote.refresh(roundTrip.fighter(), 20);
    check(remote.fighter.equipment[1].cubes.empty() &&
          remote.fighter.equipmentRecord(1)[0x113] == snapshot.equipmentSource(1)[0x113],
          "an inactive nonlinked byte survives a wire delta and core refresh without becoming an active cube");
    snapshot.equipment[1].seed = 0x12345678;
    check(snapshot.equipmentRecord(1)[4] == 0x78 &&
          snapshot.equipmentRecord(1)[0x113] == remote.fighter.equipmentRecord(1)[0x113],
          "a direct semantic item edit overlays its represented field without losing inactive backing");
}

int main() {
    currentState();
    wireMode();
    equipmentStorage();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
