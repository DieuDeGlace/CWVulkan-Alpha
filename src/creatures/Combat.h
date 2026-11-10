#pragma once
// Stats, actions and hits of a creature (docs/specs/alpha-creatures-behaviour.md §1, §3-§5):
// derived stats from level, class and equipment, the attack an input starts and how long each
// action lasts, experience and levels, and how a hit is resolved into hit records and sounds.
#include <array>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include "creatures/Particles.h"
#include "creatures/Buffs.h"
#include "creatures/Hits.h"
#include "creatures/Relationships.h"
#include "creatures/LinkedItems.h"
#include <map>
#include <vector>

namespace creatures {

struct Body;

// An item as the stat and pose rules see it (models spec §6; the full 280-byte item record
// comes with the inventory).
struct Item {
    uint8_t type = 0; // 0 = nothing, 3 weapon, 4 chest, 5 gloves, 6 boots, 7 shoulders, 8 amulet, 9 ring…
    uint8_t subtype = 0;
    uint8_t material = 0;
    uint8_t rarity = 0;
    int32_t level = 1;
    uint32_t seed = 0;
    int32_t spirits = 0; // signed native count; valid authored records keep it in sync with cubes
    // Item services §1: the attached cubes, a model cell and a material each (drawn on the
    // weapon, §3.9).
    struct Cube {
        int8_t x = 0, y = 0, z = 0;
        uint8_t material = 0;
        int32_t level = 1;
    };
    std::vector<Cube> cubes;
    uint32_t product = 0;
    uint8_t flags = 0;

    // The fields above from a 280-byte item record (behaviour §10.1).
    static Item parse(const uint8_t *record);
    // Update represented fields without overwriting padding or inactive backing
    // bytes. Full records remain necessary when replacing the source item.
    void write(ItemRecord &record) const;
};

// Weapon subtypes (models spec §6.2).
enum Weapon : uint8_t {
    DAGGER = 3,
    FIST = 4,
    LONGSWORD = 5,
    BOW = 6,
    CROSSBOW = 7,
    BOOMERANG = 8,
    STAFF = 10,
    WAND = 11,
    BRACELET = 12,
    SHIELD = 13,
    QUIVER = 14,
    GREATSWORD = 15,
    GREATAXE = 16,
    GREATMACE = 17,
    PITCHFORK = 18,
};

// Equipment slots (models spec §6.4).
enum Slot : int {
    AMULET_SLOT = 1,
    CHEST_SLOT = 2,
    BOOTS_SLOT = 3,
    GLOVES_SLOT = 4,
    SHOULDERS_SLOT = 5,
    OFF_HAND = 6,
    MAIN_HAND = 7,
    RING_LEFT = 8,
    RING_RIGHT = 9,
    SLOT_COUNT = 13,
};

// Cached spatial coordinates and descriptor ordinal, retained across movement.
struct BackingReference {
    int32_t x = -1, y = -1, index = 0;
};

// The last slot already has actor-owned backing in LinkedItemState. Retain the
// other records too, including inactive body entries and alignment bytes.
struct EquipmentSources {
    std::array<ItemRecord, SLOT_COUNT - 1> records{};
    EquipmentSources() {
        for (auto &record : records)
            setItemLevel(record, 1);
    }
};

// The record fields the combat rules read and write (§1).
struct Combatant {
    uint64_t id = 0;
    uint64_t ownerId = 0, linkedId = 0; // reverse core identifier and resident forward identifier
    BackingReference backing;
    int32_t type = 2;       // creature type
    uint8_t controller = 0; // 0 player, 1 hostile NPC, …
    uint8_t characterClass = 1, specialization = 0;
    int32_t level = 1;
    int32_t experience = 0;
    uint8_t powerTier = 0;
    float multipliers[5] = {100, 1, 1, 1, 1}; // health, attack speed, damage, armour, resistance (§8.4)
    uint16_t appearanceFlags = 0;
    std::array<Item, SLOT_COUNT> equipment{};
    LinkedItemState linkedItem; // full slot-12 source and actor-resident comparison state
    Item held; // the item being eaten or drunk (record field 43)
    std::array<int32_t, 11> skills{}; // skill levels (record field 46)

    float health = 500; // a new record's health (§8.4)
    int32_t action = 0;
    int32_t actionMs = 0;
    int32_t combo = 0;
    int32_t comboElapsedMs = 0;
    float dodgeMs = 0;
    int32_t hitStun = 0;
    int32_t actionLockMs = 0;
    float recoveryPenalty = 0;
    float charge = 0;     // field 23
    float spentCharge = 0; // the charge a ranged action took when it fired (projectiles §2.3)
    float spentPower = 0;  // what a melee blow spent (open points 6 §2.2, s)
    std::vector<uint64_t> struck; // who this swing or pulse has hit (§1.5)
    glm::vec3 aim{0};     // field 26: from the creature to what it aims at, blocks
    float mana = 0;       // field 28, 0..1 (§11.2)
    float blockGauge = 0; // field 29
    float stealth = 0;    // 0..1 (§11.3)
    bool sitting = false; // mode 0x400
    bool hitLanded = false; // common target-processing marker; set before hit admission
    bool criticalLatched = false; // consumed type-11 common-hit effect, independent of buff lifetime
    int32_t slowMs = 0, hasteMs = 0; // fields 18, 19, signed native counters
    float hitFlash = 0;
    uint32_t mode = 0;    // raw wire word in low 16 bits; canonical between actor phases
    uint32_t contact = 0; // contact flags of the physics step
    std::vector<Buff> buffs;
    std::vector<int32_t> specials; // special actions an NPC may use (§9.3, §13.2)
    std::map<int32_t, float> cooldowns; // §16.1: ability id -> remaining ms (entries stay at 0)
    Relations relations;
    EquipmentSources equipmentSources;

    ItemRecord &equipmentSource(size_t slot) {
        return slot == SLOT_COUNT - 1 ? linkedItem.source : equipmentSources.records[slot];
    }
    const ItemRecord &equipmentSource(size_t slot) const {
        return slot == SLOT_COUNT - 1 ? linkedItem.source : equipmentSources.records[slot];
    }
    ItemRecord equipmentRecord(size_t slot) const;
    void setEquipment(size_t slot, const ItemRecord &record);
    // Semantic field assignment retains each destination record's five gaps.
    void copyEquipmentFrom(const Combatant &source);

    const Item &mainHand() const { return equipment[MAIN_HAND]; }
    const Item &offHand() const { return equipment[OFF_HAND]; }
    float buffSum(uint8_t kind) const;
    bool hasBuff(uint8_t kind) const;
};

// What one item adds to each stat (§10.3). These start as Alpha's formulas; a mod may replace
// any of them.
struct ItemBonuses {
    std::function<float(const Item &)> health, damage, armour, resistance, speed, critical;
};
ItemBonuses &itemBonuses();
float itemLevelFactor(float level, int rarity); // F of §10.2
float itemHealth(const Item &item);
float itemDamage(const Item &item);
float itemArmour(const Item &item);
float itemResistance(const Item &item);
float itemSpeed(const Item &item);
float itemCritical(const Item &item);
// Screens §8.1.4 (0x4c78c0): what an item adds to regeneration.
float itemRegeneration(const Item &item);
// Screens §8.1.7 (0x4c6e10): what a consumable heals in all, the tooltip's "HP +".
float itemHeal(const Item &item);

// §4: derived stats.
float levelGrowth(int32_t level);          // g
float levelPower(int32_t level);           // P = 2^g
float basePower(const Combatant &c);       // P·2^(0.25·tier), or P·2 for players
float maxHealth(const Combatant &c);       // §4.1
// World-pass boundary after queued requests: query once, query again only for
// ordered excess health, then admit every value except ordered health <= 0.
bool admitCreatureUpdate(Combatant &c);
float damage(const Combatant &c);          // §4.2
float armour(const Combatant &c);
float resistance(const Combatant &c);
float attackSpeed(const Combatant &c);     // §4.3, S
float comboMax(const Combatant &c);        // §4.4
float criticalChance(const Combatant &c);  // §4.5
// Screens §8.1.4: the character panel's regeneration sum and its weapon and armour ratings.
float regeneration(const Combatant &c);
float weaponRating(const Combatant &c);
float armorRating(const Combatant &c);
int32_t experienceForLevel(int32_t level); // §4.6
void addExperience(Combatant &c, int32_t amount);
// Held fatal-effects contract: prepare once before regional work; convert to an
// integer only when producing each recipient record. The tier input is a byte.
float fatalCreditAmount(int32_t level, uint8_t tier, int32_t type, uint16_t appearanceFlags);
// Legacy later-corpse adapter. Its separate boss argument supplies flag 0x200.
int32_t killExperience(int32_t level, int32_t tier, int32_t type, bool boss, uint16_t appearanceFlags);

// §3: actions.
struct Phases {
    int32_t windUp = 0, active = 0, recovery = 0; // P1, P2, P3 in ms
    int32_t total() const { return windUp + active + recovery; }
};
Phases phases(const Combatant &c, int32_t action);
int32_t duration(const Combatant &c, int32_t action);
bool isMagicUser(const Combatant &c);
// Behaviour §13.2 step 9: creatures and weapons that attack from afar (they keep their distance,
// and show no weapon trails).
bool rangedUser(const Combatant &c);
// Nonnegative hit contribution, before adding the target's former threat.
float hitThreat(const Combatant &source, float amount);
int32_t primaryAttack(const Combatant &c);   // §3.1
int32_t secondaryAttack(const Combatant &c); // §3.2
// Primary returns false only for a nonzero roll lock or old action 48. A busy
// admitted attempt returns true without starting an action or changing its clock.
bool startPrimaryAttack(Combatant &c);
// Secondary returns true when its readiness, roll and mana gates start an action.
bool startSecondaryAttack(Combatant &c);
// Dispatch action effects between these common native phases. Charge growth
// belongs later, after movement; replica-other actors skip combo timeout work.
struct ActionTickRole {
    bool local = true;
    bool replica = false;
};
void beginActionUpdate(Combatant &c, int32_t dtMs, std::vector<BuffEvent> *events = nullptr);
// Periodic callbacks run before expired-node cleanup and stun progression. They
// may inspect buffs/update other state, but must not mutate the buff collection.
void endActionUpdate(Combatant &c, int32_t dtMs, ActionTickRole role = {},
                     const std::function<void(const Buff &)> &periodic = {});
// Convenience for callers with no action effects to dispatch.
void stepAction(Combatant &c, float dtMs);
// The current action's hit connected: the chain goes on (combo + 1, inferred).
void landHit(Combatant &c);
// Whether the action's wind-up ends inside (before, before + dt]: when the hit happens.
bool hitMoment(const Combatant &c, float beforeMs);
bool hitMoment(const Combatant &c, float beforeMs, float dtMs);

struct SoundEvent {
    glm::dvec3 position{0};
    int32_t sound = 0;
    float pitch = 1, volume = 1;
};

struct HitInput {
    float amount = 0;
    int32_t selector = 0; // explicit resolver argument; independent of the attacker's current action
    bool critical = false, area = false, magic = false, blockable = true;
    float power = 0; // the area value, what the blow spent: it feeds the stun (open points 6 §3.2)
    glm::vec3 knockBack{0};
    bool targetOnGround = true;
    glm::dvec3 targetPosition{0}, attackerPosition{0};
    std::array<int64_t, 3> targetRecordPosition{}; // raw target coordinates, separate from the contact point
    std::array<int64_t, 3> attackerRecordPosition{};
};

struct HitOutcome {
    bool hitAccepted = false; // native result; fully absorbed hits can return true without health damage
    std::vector<HitRecord> records;
    std::vector<SoundEvent> sounds;
    std::vector<BuffEvent> buffs; // already applied to this target; forward without applying twice
    // Step 8: when > 0, the spirit bursts of §12.1 at the target, along the hit direction.
    float sweep = 0;
    glm::vec2 sweepDirection{0};
    std::vector<Burst> bursts; // particles §5.8, §5.9
};

// Produces hit records. Stable finite native inputs leave velocity unchanged;
// response and velocity changes belong to the separate application phase.
// emit preserves producer boundaries. Mode-zero callers apply target emissions
// there so later calculation sees earlier mutations; source gains are record-only.
HitOutcome resolveHit(Combatant &target, glm::vec3 &targetVelocity, Combatant *attacker, const HitInput &hit,
                      const std::function<int()> &draw,
                      const std::function<void(const HitEmission &)> &emit = {});

struct HitApplicationContext {
    const Combatant *source = nullptr;
    bool local = true, replica = false;
    bool protectionGranted = false;
};
// Applies admitted target state. World lookup/relay and relationship propagation
// stay with the world caller; protectionGranted supplies its resolved map gate.
void applyHit(Combatant &target, Body &body, const HitRecord &hit, HitApplicationContext context = {},
              std::vector<BuffEvent> *events = nullptr);
// The world caller inserts contact/relationship work between these phases.
// Initial target admission belongs to that caller; this internal result says
// only whether health processing was reached, not whether a hit was accepted.
void prepareHitApplication(Combatant &target, const HitRecord &hit);
enum class HitApplicationPhase { stopped, health };
HitApplicationPhase applyPreparedHit(Combatant &target, Body &body, const HitRecord &hit,
                                     HitApplicationContext context, std::vector<BuffEvent> *events = nullptr);
// Incoming network consumer, after controlled-target resolution. This is a
// separate native phase from the world applier above; no source lookup occurs.
void receiveHit(Combatant &controlled, Body &body, const HitRecord &hit, std::vector<BuffEvent> *events = nullptr);

// §14: the data2.db key of a sound id, or null for ids that play nothing.
const char *soundKey(int32_t id);

// Preliminary world scan: only the living role-zero controlled actor in action
// 84 receives this addition. Call before inputs/main actor work. Zero/negative
// elapsed still performs the specified maximum queries and ordered upper bound.
void regenerate(Combatant &c, int32_t dtMs);

} // namespace creatures
