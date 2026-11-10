#include "creatures/Credits.h"
#include "creatures/Combat.h"
#include "creatures/ActorMap.h"
#include <bit>
#include <cfenv>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>

using namespace creatures;
static int failures = 0;
static void check(bool value, const char *message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

struct Fixture {
    ActorMap<Combatant *> actors;
    Combatant *controlled = nullptr;
    std::vector<uint64_t> lookups;
    struct Items { ItemRecord source{}, snapshot{}; };
    std::map<Combatant *, Items> items;
    int copies = 0;
    CreditWorld world(uint8_t mode) {
        return {[&](uint64_t key) {
                    lookups.push_back(key);
                    const auto found = actors.find(key);
                    return found == actors.end() ? nullptr : found->second;
                },
                [&] { return controlled; }, mode,
                [&](Combatant &actor) -> CreditItems {
                    copies++;
                    auto &storage = items[&actor];
                    return {storage.source, storage.snapshot};
                }};
    }
};

static void codecAndAdmission() {
    std::array<uint8_t, 24> bytes;
    for (size_t i = 0; i < bytes.size(); i++) bytes[i] = uint8_t(i * 71 + 3);
    check(KillRecord::parse(bytes.data()).serialize() == bytes, "all twenty-four credit bytes round-trip");
    Fixture f;
    Combatant victim;
    f.actors[2] = &victim;
    f.actors[3] = nullptr;
    auto world = f.world(1);
    for (uint64_t missing : {0ull, 3ull, 0x8000000000000000ull}) {
        f.lookups.clear();
        const auto result = receiveCredit({missing, 2, INT32_MIN, 50}, world);
        check(f.lookups == std::vector<uint64_t>{missing, 2} && !result.directAward && !result.linkedAward,
              "a missing or null first actor still performs the second lookup before rejecting");
    }
    Combatant player;
    player.id = 999;
    f.controlled = &player;
    f.actors[0] = &player;
    f.lookups.clear();
    victim.controller = 1;
    victim.health = -5;
    const auto accepted = receiveCredit({0, 2, INT32_MIN, 17}, world);
    check(accepted.directAward == &player && player.experience == 17 &&
          f.lookups == std::vector<uint64_t>{0, 2},
          "entry key zero and a dead victim are admitted; own ID and unused type do not filter credit");
    victim.controller = 0;
    receiveCredit({0, 2, 1, 50}, world);
    check(player.experience == 17, "the direct branch requires the current victim role to equal one");
}

static void designatedCleanup() {
    Fixture f;
    Combatant credited, controlled, linked;
    f.controlled = &controlled;
    f.actors[7] = &credited;
    f.actors[8] = &controlled;
    f.actors[0] = &linked;
    controlled.linkedId = 0;
    controlled.relations.contact = 123;
    linked.health = -4;
    linked.relations.threats[7] = 9;
    linked.relations.counts[7] = 11;
    auto world = f.world(1);
    const auto result = receiveCredit({7, 8, 0, 5}, world);
    check(f.lookups == std::vector<uint64_t>{7, 8, 0, 0, 0} && result.healthZeroed == &linked &&
          linked.health == 0 && linked.relations.threats.empty() && linked.relations.counts.at(7) == 11 &&
          controlled.relations.contact == 0 && controlled.linkedId == 0 && !result.directAward,
          "foreign-recipient designated death repeats key-zero lookups, zeros health, clears only threats and retains the link");
    f.actors.erase(0);
    f.lookups.clear();
    controlled.linkedId = 19;
    controlled.relations.contact = 77;
    receiveCredit({7, 8, 0, 0}, world);
    check(f.lookups == std::vector<uint64_t>{7, 8, 19} && controlled.relations.contact == 0 && controlled.linkedId == 19,
          "missing linked lookup still clears designated contact without clearing its forward identifier");

    Combatant first, second, third;
    first.health = second.health = third.health = 10;
    first.relations.threats[1] = second.relations.threats[1] = third.relations.threats[1] = 1;
    controlled.linkedId = 0;
    int zeroCalls = 0, controlledCalls = 0;
    f.lookups.clear();
    world.lookup = [&](uint64_t key) -> Combatant * {
        f.lookups.push_back(key);
        if (key == 7) return &credited;
        if (key == 8) return &controlled;
        if (key == 0) return ++zeroCalls == 1 ? &first : &second;
        return key == 9 ? &third : nullptr;
    };
    world.controlled = [&] {
        if (++controlledCalls == 2 && second.health == 0) controlled.linkedId = 9;
        return &controlled;
    };
    receiveCredit({7, 8, 0, 0}, world);
    check(f.lookups == std::vector<uint64_t>{7, 8, 0, 0, 9} && first.health == 10 && second.health == 0 &&
          first.relations.threats.size() == 1 && second.relations.threats.size() == 1 && third.relations.threats.empty(),
          "health and map writes use fresh lookup results and reload the current forward key after the health store");
}

static void independentLinkedAward() {
    Fixture f;
    Combatant credited, victim, linked;
    f.actors[4] = &credited;
    f.actors[5] = &victim;
    f.actors[0] = &linked;
    credited.health = -3;
    credited.linkedId = 0;
    victim.controller = 0;
    linked.health = 9;
    linked.level = -65535; // raw low word one; entry gate skips level arithmetic
    linked.experience = INT32_MIN;
    auto &items = f.items[&credited];
    for (size_t i = 0; i < items.source.size(); i++) items.source[i] = uint8_t(i * 13 + 7);
    items.snapshot.fill(0xa5);
    const auto before = items.source;
    auto world = f.world(0);
    const auto result = receiveCredit({4, 5, 1, -1}, world);
    check(!result.directAward && result.linkedAward == &linked && linked.experience == INT32_MAX && linked.health == 9 &&
          f.lookups == std::vector<uint64_t>{4, 5, 0} && f.copies == 1,
          "linked award is independent of designated pointer, credited life and victim role; signed addition wraps");
    auto expected = before;
    const int32_t experience = INT32_MAX;
    std::memcpy(expected.data() + 4, &experience, 4);
    setItemLevel(expected, 1);
    check(items.source == expected && credited.equipment[12].seed == uint32_t(INT32_MAX) &&
          credited.equipment[12].level == 1, "linked item receives final raw experience and only the low level word");
    bool exactCopy = true;
    for (size_t i = 0; i < items.snapshot.size(); i++) {
        const bool padding = i == 2 || i == 3 || i == 0x0f || i == 0x12 || i == 0x13;
        exactCopy &= items.snapshot[i] == (padding ? 0xa5 : items.source[i]);
    }
    check(exactCopy, "item snapshot copies selected fields while retaining destination alignment bytes");
    linked.experience = 17;
    linked.level = 0;
    receiveCredit({4, 5, 0, 0}, world);
    check(linked.experience == 17 && linked.health == 9 && f.copies == 2 && itemLevel(items.snapshot) == 0,
          "zero linked credit still stamps and copies the item without requiring a level increment");
    f.lookups.clear();
    const int copies = f.copies;
    receiveCredit({4, 5, 0, 7}, f.world(255));
    check(f.lookups == std::vector<uint64_t>{4, 5} && f.copies == copies,
          "every nonzero world mode skips linked lookup, award and item copy");
    for (float health : {0.f, -0.f, -1.f, std::numeric_limits<float>::quiet_NaN()}) {
        linked.health = health;
        receiveCredit({4, 5, 0, 0}, world);
        check(linked.experience == 17 && f.copies == copies, "linked health must compare strictly positive and ordered");
    }
}

static void aliasesAndRecordOrder() {
    Fixture f;
    Combatant actor, victim;
    f.controlled = &actor;
    actor.controller = 1;
    actor.linkedId = 0;
    actor.health = 10;
    actor.relations.threats[2] = 1;
    actor.relations.counts[2] = 2;
    actor.relations.contact = 9;
    f.actors[0] = &actor;
    f.actors[2] = &actor;
    auto world = f.world(0);
    const auto result = receiveCredit({0, 2, -1, 50}, world);
    check(result.healthZeroed == &actor && result.directAward == &actor && result.linkedAward == &actor &&
          actor.level == 2 && actor.experience == 50 && actor.health > 0 && actor.relations.threats.empty() &&
          actor.relations.counts.at(2) == 2 && actor.relations.contact == 0,
          "distinct key aliases can clean themselves, level and refill, then admit an independent second award");
    check(itemLevel(f.items[&actor].snapshot) == 2 && f.copies == 1,
          "alias item copy follows both ordered awards");
    actor.level = 1, actor.experience = 0, actor.health = 10;
    const int copies = f.copies;
    const auto zero = receiveCredit({0, 0, 123, 0}, world);
    check(zero.directAward == &actor && !zero.linkedAward && actor.health == 0 && f.copies == copies,
          "equal record IDs are admitted; zero direct credit does not restore health for the later linked gate");

    actor = Combatant{};
    victim.controller = 1;
    f.actors[9] = &victim;
    world = f.world(1);
    receiveCredit({0, 9, 0, 50}, world);
    receiveCredit({0, 9, 0, -50}, world);
    check(actor.level == 2 && actor.experience == -50 && actor.health > 0,
          "record-by-record level processing cannot be replaced with an accumulated zero credit");
    receiveCredit({0, 9, 0, 17}, world);
    receiveCredit({0, 9, 0, 17}, world);
    check(actor.level == 2 && actor.experience == -16,
          "repeated admitted records remain distinct and are not internally deduplicated");
}

static void preparedFatalAmount() {
    std::fenv_t environment;
    if (std::feholdexcept(&environment) != 0) {
        check(false, "selected masked floating controls are available");
        return;
    }
    std::fesetround(FE_TONEAREST);
    struct Anchor { int32_t level; uint8_t tier; uint32_t bits; };
    // Binary32 anchors from the held ordered arithmetic, before integer credit.
    for (const Anchor anchor : {Anchor{1, 0, 0x3f800000}, {2, 0, 0x3f800000},
             {3, 0, 0x3fe8ba34}, {5, 0, 0x40555557}, {10, 2, 0x40e318c4},
             {-20, 0, 0x43d1ffed}, {-21, 0, 0x435bfffe}, {-19, 0, 0x3f800000},
             {INT32_MAX, 255, 0x41a00000}, {INT32_MIN, 255, 0x41a00000}})
        check(std::bit_cast<uint32_t>(fatalCreditAmount(anchor.level, anchor.tier, 4, 0)) == anchor.bits,
              "prepared credit retains byte-tier addition, binary32 arithmetic and the ordered minimum");
    check(std::bit_cast<uint32_t>(fatalCreditAmount(5, 0, 101, 0x200)) == 0x4426aaac &&
          (int32_t)fatalCreditAmount(5, 0, 101, 0x200) == 666,
          "kind and flag multipliers operate on fractional credit before recipient truncation");
    for (int32_t kind : {101, 107, 108, 109, 114, 115, 116, 117, 118, 119})
        check(fatalCreditAmount(1, 0, kind, 0) == 20.f && fatalCreditAmount(1, 0, kind, 0x200) == 200.f,
              "each specified kind multiplies the already-minimum-bounded amount");
    for (int32_t kind : {0, 89, 100, 102, 106, 110, 113, 120, -1, INT32_MIN, INT32_MAX})
        check(fatalCreditAmount(1, 0, kind, 0) == 1.f,
              "neighboring and unrelated kinds do not gain the twenty-fold multiplier");
    check(fatalCreditAmount(5, 0, 4, 0x2000) == fatalCreditAmount(5, 0, 4, 0) &&
          fatalCreditAmount(1, 0, 101, 0x2800) == 0.f && fatalCreditAmount(1, 0, 101, 0xffff) == 0.f,
          "regional admission is independent of amount modifiers; flag800 finally forces zero without a second minimum");

    // Selected masked controls only: even forced-zero output retains the earlier
    // division's observable status. This is not a whole-game control claim.
    std::feclearexcept(FE_ALL_EXCEPT);
    const float zero = fatalCreditAmount(-19, 0, 4, 0x800);
    const bool divided = (std::fetestexcept(FE_DIVBYZERO) & FE_DIVBYZERO) != 0;
    std::fesetenv(&environment);
    check(zero == 0.f && divided, "forced-zero credit does not bypass earlier numeric preparation");
}

int main() {
    codecAndAdmission();
    designatedCleanup();
    independentLinkedAward();
    aliasesAndRecordOrder();
    preparedFatalAmount();
    std::puts(failures ? "FAILED" : "Credit checks passed");
    return failures ? 1 : 0;
}
