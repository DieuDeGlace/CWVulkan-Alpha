#include "net/CreaturePublication.h"
#include <cstdio>
#include <map>
#include <thread>

using namespace net;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
static bool available(std::mutex &lock) {
    bool result = false;
    std::thread probe([&] { if (lock.try_lock()) { result = true; lock.unlock(); } });
    probe.join();
    return result;
}

static void snapshotAndLookup() {
    std::mutex lock;
    CreatureRecord first, replacement, update;
    first.setName("snapshot");
    first.setHealth(75);
    update.setName("wire");
    update.setHealth(41);
    auto fighter = update.fighter();
    fighter.mode = 0xa620;
    update.setFighter(fighter);
    // Use actual codec field masks; identity is a separate eight-byte prefix.
    const auto payload = encodeDelta(0x1122334455667788, update,
                                     (uint64_t(1) << 14) | (uint64_t(1) << 27));
    CreatureRecord *current = &first;
    std::vector<std::string> order;
    uint64_t firstKey = 0, lastKey = 0;
    CreaturePublication services{
        lock,
        [&](uint64_t key) -> CreatureRecord * {
            check(!available(lock), "lookup executes while the world mutex is held");
            order.push_back("lookup");
            if (order.size() == 1) firstKey = key;
            lastKey = key;
            return current;
        },
        [&](CreatureRecord &resident) {
            check(!available(lock) && &resident == &replacement, "post-assignment engine admission runs on the new resident under lock");
            order.push_back("admit");
            resident.bytes[0x50] = 0;
        }
    };
    check(publishCreatureUpdate(0, payload, services, [&](const auto &bytes, CreatureRecord &temporary) {
        order.push_back("decode");
        check(available(lock), "decode releases the world lock");
        check(temporary.name() == "snapshot" && temporary.health() == 75, "decode starts from the first resident snapshot");
        {
            std::lock_guard guard(lock);
            current = &replacement;
            replacement.setName("intervening");
            first.setName("later first");
        }
        return decodeCreatureUpdate(bytes, temporary);
    }), "valid publication succeeds");
    check(firstKey == 0 && lastKey == 0 && order == std::vector<std::string>{"lookup", "decode", "lookup", "admit"},
          "both lookups accept context key zero and ignore a different wire identity");
    check(replacement.name() == "snapshot" && replacement.fighter().mode == 0xa620 && replacement.health() == 41 &&
          first.name() == "later first" && first.health() == 75,
          "final relookup receives the whole decoded snapshot with absent fields retained and raw mode bits preserved");
}

static void rawModesAndFailures() {
    std::mutex lock;
    std::map<uint64_t, CreatureRecord> actors;
    const uint64_t key = 0xfedcba9876543210;
    actors[key].setName("resident");
    CreaturePublication services{lock, [&](uint64_t id) -> CreatureRecord * {
        const auto found = actors.find(id);
        return found == actors.end() ? nullptr : &found->second;
    }, {}};
    for (uint16_t mode : {uint16_t(0xffff), uint16_t(0xa620), uint16_t(0), uint16_t(0x20)}) {
        CreatureRecord update;
        auto fighter = update.fighter(); fighter.mode = mode; update.setFighter(fighter);
        check(publishCreatureUpdate(key, encodeDelta(1, update, uint64_t(1) << 14), services) &&
              actors[key].fighter().mode == mode && actors[key].name() == "resident",
              "actual codec publication retains all sixteen mode bits without an identity or ownership exclusion");
        update.setHealth(19);
        check(publishCreatureUpdate(key, encodeDelta(2, update, uint64_t(1) << 27), services) &&
              actors[key].fighter().mode == mode && actors[key].health() == 19,
              "absent mode field retains the earlier snapshot word");
    }
    const auto before = actors[key].bytes;
    check(!publishCreatureUpdate(key, {1, 2, 3}, services) && actors[key].bytes == before,
          "engine malformed-payload policy discards the temporary without partial resident mutation");
    bool decoded = false;
    check(!publishCreatureUpdate(123, {}, services, [&](const auto &, auto &) { decoded = true; return true; }) && !decoded,
          "missing initial storage is rejected without decode or invented actor creation");
    check(!publishCreatureUpdate(key, {}, services, [&](const auto &, auto &) {
        std::lock_guard guard(lock); actors.erase(key); return true;
    }) && !actors.contains(key), "missing final storage is rejected after relookup without retaining an old pointer");
    check(available(lock), "failure paths release the world mutex");
}

int main() {
    snapshotAndLookup(); rawModesAndFailures();
    if (!failures) std::puts("PASS: separate snapshot/decode/publication locks, context relookup and raw mode updates");
    return failures ? 1 : 0;
}
