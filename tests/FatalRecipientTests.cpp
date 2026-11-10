#include "creatures/FatalRecipients.h"
#include "creatures/Npcs.h"
#include <cstdio>
#include <limits>
#include <string>

using namespace creatures;
static int failures = 0;
static void check(bool value, const char *message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); failures++; }
}

struct Fixture {
    Combatant target;
    Combatant *controlled = nullptr;
    ActorMap<Combatant *> actors;
    std::vector<uint64_t> visits, grants;
    std::vector<KillRecord> credits;
    std::vector<std::string> order;
    Fixture() {
        target.id = 999;
        target.type = 89;
        target.controller = 1;
        target.health = 0;
        actors[77] = &target;
    }
    FatalRecipientWorld world() {
        FatalRecipientWorld result;
        result.visit = [&](const auto &visit) {
            for (const auto &[key, actor] : actors) {
                visits.push_back(key);
                visit(key, actor);
            }
        };
        result.credit = {[&](uint64_t key) -> Combatant * {
            const auto found = actors.find(key);
            return found == actors.end() ? nullptr : found->second;
        }, [&] { return controlled; }, 1};
        result.appendCredit = [&](const KillRecord &record) {
            credits.push_back(record);
            order.push_back("append" + std::to_string(record.player));
        };
        result.consumed = [&](const KillRecord &record, const CreditResult &) {
            order.push_back("consume" + std::to_string(record.player));
        };
        result.completionReward = [&](uint64_t key, Combatant &) {
            grants.push_back(key);
            order.push_back("grant" + std::to_string(key));
        };
        return result;
    }
};

static void admissionAndCleanup() {
    Fixture f;
    Combatant negative, zero, unordered, absent, other;
    negative.id = 123;
    negative.health = -4;
    negative.level = 0;
    negative.experience = 5;
    zero.health = 0;
    unordered.health = std::numeric_limits<float>::quiet_NaN();
    other.controller = 2;
    f.controlled = &negative;
    const uint64_t first = uint64_t(-5);
    f.actors[first] = &negative;
    f.actors[0] = &zero;
    f.actors[3] = &unordered;
    f.actors[6] = &absent;
    f.actors[uint64_t(-9)] = &other;
    f.actors[99] = nullptr;
    f.target.relations.threats = {{first, 0}, {0, -1}, {3, std::numeric_limits<float>::quiet_NaN()}};
    for (auto *actor : {&negative, &zero, &unordered, &absent, &other}) {
        actor->relations.contact = actor == &zero ? 0 : 77;
        actor->relations.threats = {{0, -4}, {77, std::numeric_limits<float>::quiet_NaN()}, {9, 3}};
        actor->relations.counts = {{0, 4}, {77, 5}};
    }
    auto world = f.world();
    world.consumed = [&](const KillRecord &record, const CreditResult &result) {
        f.order.push_back("consume" + std::to_string(record.player));
        if (record.player == first)
            check(result.directAward == &negative && negative.experience == 12 && negative.health == -4,
                  "the real consumer immediately awards the mapped dead recipient without substituting its own ID");
    };
    applyFatalRecipients(f.target, 77, 7.9f, true, world);
    check(f.credits.size() == 3 && f.credits[0].player == first && f.credits[1].player == 0 && f.credits[2].player == 3,
          "signed traversal credits zero, negative and unordered membership values without health or key-zero filtering");
    for (const auto &record : f.credits)
        check(record.victim == 77 && record.victimType == 89 && record.experience == 7,
              "payload uses the incoming target key, current target kind and per-record truncation of the prepared float");
    check(f.grants == std::vector<uint64_t>{first, 0, 3, 6},
          "completed rewards include role-zero recipients without contribution membership");
    for (auto *actor : {&negative, &zero, &unordered, &absent, &other})
        check(actor->relations.contact == 0 && actor->relations.threats.size() == 1 && actor->relations.threats.contains(9) &&
              actor->relations.counts.size() == 2 && actor->relations.counts.at(0) == 4 && actor->relations.counts.at(77) == 5,
              "all nonnull roles receive I/zero-link cleanup while their unrelated threats and counts remain intact");
    check(f.order == std::vector<std::string>{"append" + std::to_string(first), "consume" + std::to_string(first),
              "grant" + std::to_string(first), "append0", "consume0", "grant0", "append3", "consume3", "grant3", "grant6"},
          "each append, actual consumer and completion branch finishes before the next recipient");
}

static void liveAliasesAndMutations() {
    Fixture f;
    Combatant zero, credited, late, inserted, behind;
    credited.level = 0;
    late.controller = 2;
    f.controlled = &credited;
    f.actors[uint64_t(-1)] = &f.target;
    f.actors[0] = &zero;
    f.actors[2] = &credited;
    f.actors[3] = &late;
    f.target.relations.threats = {{0, 1}, {2, -1}, {77, 9}};
    late.relations.contact = 55;
    late.relations.threats = {{0, 3}, {55, 8}, {77, 9}};
    auto world = f.world();
    world.consumed = [&](const KillRecord &record, const CreditResult &) {
        f.order.push_back("consume" + std::to_string(record.player));
        if (record.player == 2) {
            f.target.linkedId = 55;
            f.target.type = 117;
            f.target.relations.threats[4] = -1;
            f.actors[4] = &inserted;
            f.actors[uint64_t(-10)] = &behind;
        }
    };
    applyFatalRecipients(f.target, 77, 5.5f, true, world);
    check(f.credits.size() == 2 && f.credits[0].player == 2 && f.credits[0].victimType == 89 &&
          f.credits[1].player == 4 && f.credits[1].victimType == 117 && credited.experience == 5,
          "earlier target aliases remove later membership and actual consumers can affect subsequent eligibility and kind reads");
    check(f.grants == std::vector<uint64_t>{0, 2, 4} && f.visits == std::vector<uint64_t>{uint64_t(-1), 0, 2, 3, 4, 77},
          "live successors added after the current key are visited; earlier insertions are not replayed or globally snapshotted");
    check(late.relations.contact == 0 && late.relations.threats.size() == 1 && late.relations.threats.contains(0),
          "each later recipient cleans against the current forward key rather than a frozen pre-traversal link");
}

static void repeatedPointersAndZeroCredit() {
    Fixture f;
    Combatant recipient;
    recipient.id = 400;
    recipient.health = 0;
    recipient.experience = 50;
    f.controlled = &recipient;
    f.target.linkedId = 9;
    f.actors[uint64_t(-3)] = &recipient;
    f.actors[5] = &recipient;
    f.target.relations.threats = {{uint64_t(-3), 1}, {5, 1}};
    auto world = f.world();
    int consumed = 0;
    world.consumed = [&](const KillRecord &record, const CreditResult &result) {
        consumed++;
        check(record.experience == 0 && result.directAward == &recipient && recipient.level == 2 &&
              recipient.experience == 0 && recipient.health > 0,
              "zero credit still invokes the actual level consumer before continuing to the next alias");
    };
    applyFatalRecipients(f.target, 77, 0, false, world);
    check(f.credits.size() == 2 && consumed == 2 && f.grants.empty(),
          "distinct world entries with the same pointer are not deduplicated; incomplete regions emit no completion grants");
    f.target.controller = 2;
    applyFatalRecipients(f.target, 77, 5, true, world);
    check(f.credits.size() == 2 && f.grants.empty(), "non-role-one targets still clean relationships but produce no credit or completion grants");
}

static void codecAndFrameViews() {
    std::array<uint8_t, 288> bytes;
    for (size_t i = 0; i < bytes.size(); i++) bytes[i] = uint8_t(i * 71 + 3);
    check(ItemGrant::parse(bytes.data()).serialize() == bytes, "all 288 grant bytes, including opaque item data, round-trip");
    NpcFrame frame;
    NpcFrame::Cursor cursor;
    frame.grants.push_back(ItemGrant::parse(bytes.data()));
    frame.appliedCredits.push_back({{1, 2, 3, 7}, true, 1, 2});
    auto added = frame.addedSince(cursor);
    check(added.grants.size() == 1 && added.grants[0].serialize() == bytes && added.appliedCredits.size() == 1 &&
          added.appliedCredits[0].levelBefore == 1 && added.appliedCredits[0].levelAfter == 2 &&
          frame.addedSince(cursor).grants.empty() && frame.addedSince(cursor).appliedCredits.empty(),
          "typed grants and already-applied credit metadata remain ordered and drain once through retained frame views");
    const auto grantsCapacity = frame.grants.capacity(), creditsCapacity = frame.appliedCredits.capacity();
    frame.reset();
    check(frame.grants.empty() && frame.appliedCredits.empty() && frame.grants.capacity() == grantsCapacity &&
          frame.appliedCredits.capacity() == creditsCapacity, "frame reset retains both new output capacities");
}

static void retainedBackingAdmission() {
    auto zone = std::make_shared<worldgen::Zone>(1, 2);
    worldgen::Spawn spawn;
    spawn.x = 266ll * 65536;
    spawn.y = 522ll * 65536;
    spawn.z = 3 * 65536;
    spawn.type = 19;
    spawn.radius = 1000;
    zone->spawns.push_back(spawn);
    worldgen::WorldMap map;
    map.add(zone);
    Npcs npcs;
    Combatant owner, target;
    owner.id = 1;
    Body body;
    body.place({266, 522, 3});
    target.backing = {1, 2, 0};
    check(npcs.markFatalBacking(target, map, 7), "a held retained descriptor admits the immediate backing write");
    NpcFrame frame;
    const auto step = [&](int32_t day) {
        npcs.stepInto(0, map, {{owner, body, 0, 0, false}}, 1000, day,
            [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); }, frame);
    };
    step(7);
    check(npcs.all().empty(), "the actual engine backing timer prevents activation on the written day without waiting for a corpse pass");
    step(8);
    check(npcs.all().size() == 1, "the actual engine activation adapter observes the separately written day");
    npcs.setExtraSpawns(1, 2, 1, {spawn}, false);
    target.backing.index = 1;
    check(npcs.markFatalBacking(target, map, 8), "a retained extra descriptor can admit a backing write");
    npcs.trimMissionSpawns(target, map);
    check(!npcs.markFatalBacking(target, map, 8), "completion tail disposal removes final backing index admission before the write");
    for (const BackingReference invalid : {BackingReference{-1, 2, 0}, {1, -1, 0}, {65536, 2, 0},
             {1, 65536, 0}, {1, 2, -1}, {1, 2, 2}, {7, 7, 0}}) {
        target.backing = invalid;
        check(!npcs.markFatalBacking(target, map, 8), "negative/outside coordinates, invalid indices and missing held cells reject backing writes");
    }
}

int main() {
    admissionAndCleanup();
    liveAliasesAndMutations();
    repeatedPointersAndZeroCredit();
    codecAndFrameViews();
    retainedBackingAdmission();
    if (!failures) std::puts("FatalRecipientTests passed");
    return failures ? 1 : 0;
}
