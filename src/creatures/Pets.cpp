// Pets (docs/specs/alpha-pets-bombs.md Part B): taming by feeding, the pet pass that keeps each
// player's pet in step with the Pet slot, the pet's Combat and Companion behaviours, its return
// after death, sharing the owner's kill experience, T to call it back and the pet under its
// rider. These run where the world is decided: offline and on the server.
#include "creatures/Npcs.h"
#include "creatures/Physics.h"
#include "worldgen/Regions.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace creatures {

namespace {

constexpr int PET_SLOT = 12;

} // namespace

bool rideable(int32_t type) {
    return type == 19 || type == 20 || type == 22 || type == 23 || (type >= 25 && type <= 40) ||
           (type >= 63 && type <= 67) || type == 74 || type == 75 || (type >= 98 && type <= 100) ||
           (type >= 102 && type <= 105) || type == 151;
}

Npc *Npcs::findNpc(uint64_t id) {
    for (Npc &n : list)
        if (n.fighter.id == id)
            return &n;
    return nullptr;
}

const Npc *Npcs::petOf(const Combatant &owner) const {
    for (const Npc &n : list)
        if (n.fighter.id == owner.linkedId && n.fighter.controller == 5 && n.fighter.ownerId == owner.id)
            return &n;
    return nullptr;
}

uint64_t Npcs::freeId() const {
    uint64_t smallest = 0;
    if (actorLookupActive) {
        for (const auto &[key, actor] : actors)
            if ((actor.npc || actor.player) && ActorKeyLess{}(key, smallest))
                smallest = key;
    } else {
        for (const Npc &n : list)
            if (ActorKeyLess{}(n.fighter.id, smallest))
                smallest = n.fighter.id;
    }
    return smallest - uint64_t(1); // defined low-64-bit wrap, including the signed minimum
}

// §9.2 step 1, re-certification correction 17: the record reset (level 1, experience 0,
// motion, mode, action, action time, combo, class, tier, equipment, owner and skills cleared,
// hit-stun -3000, multipliers 100, 1, 1 with armour and resistance kept, health untouched),
// then the transient reset (behaviour, stealth, mount, target and pet ids, the threat,
// hit-counter and third maps, the path and the whole buff list, a kind-7 taming buff included).
// The controller and type are not part of either reset; the caller writes what follows.
void Npcs::resetCreature(Npc &n) {
    Combatant c;
    c.id = n.fighter.id;
    c.controller = n.fighter.controller;
    c.type = n.fighter.type;
    c.health = n.fighter.health;
    c.multipliers[3] = n.fighter.multipliers[3];
    c.multipliers[4] = n.fighter.multipliers[4];
    c.characterClass = 0;
    c.hitStun = -3000;
    c.linkedItem.source.fill(0); // native reset differs from fresh item construction
    c.equipment[PET_SLOT].level = 0;
    c.linkedItem.retained = n.fighter.linkedItem.retained;
    c.behaviorReference = n.fighter.behaviorReference; // the held body reset preserves field40
    n.fighter = std::move(c);
    n.body.mode = n.fighter.mode; // the reset leaves the reverse ID zero (alpha-fatal-context-field-producers.md)
    n.looks = Appearance{};       // the default appearance
    n.path.clear();
    n.path.state.reference = {-1, -1, 0};
    n.spawn = Npc::NO_SPAWN;
    n.target = 0; // legacy continuation cache of the cleared target
    n.threat = 0;
    n.deathPassStarted = false;
    n.fatalPresented = false;
    n.fatalLootApplied = false;
    n.fatalRegionApplied = false;
    n.fatalRecipientsApplied = false;
    n.fatalBackingApplied = false;
}

// §9.4: multipliers 100, 1, 1, 1, 1 and the per-type changes; then, only for a found owner
// (alpha-forward-link-zero-gates.md, "Owner helper": its own unguarded search by the owner's
// ID), the level cap, the owner field, the taming-skill health factor and the owner's link.
// The AI it installs (Combat, then Companion) is selected by controller 5 in think().
void Npcs::buildPet(Npc &pet, uint64_t ownerId) {
    Combatant &c = pet.fighter;
    float m[5] = {100, 1, 1, 1, 1};
    if (c.type == 25)
        m[0] = 300, m[2] = 0.1f, m[3] = 5, m[4] = 5;
    if (c.type == 86)
        c.characterClass = 3, c.specialization = 1;
    if (c.type == 104)
        c.characterClass = 2, c.specialization = 1;
    std::copy(m, m + 5, c.multipliers);
    if (Combatant *mappedOwner = lookupActor(ownerId, {}).combat()) {
        c.level = std::min(c.level, mappedOwner->level);
        c.ownerId = ownerId;
        c.multipliers[0] *= 1 + 0.5f * skillFactor(mappedOwner->skills[0]);
        mappedOwner->linkedId = c.id;
    }
}

// §9.2 steps 1-6 (and §9.3 steps 6-8): reset, then type, level and experience from the item,
// the owner, the default appearance and the generator with no choice record (its five
// choice-record draws plus the type's own), then the behaviour builder. Health is kept.
void Npcs::makePet(Npc &pet, const Item &item, const Combatant &owner) {
    resetCreature(pet);
    Combatant &c = pet.fighter;
    c.type = item.subtype;
    c.level = std::max(1, item.level);
    c.experience = (int32_t)item.seed;
    // Step 3, the item's name characters: the engine creature record carries no name.
    c.ownerId = owner.id; // step 4
    int32_t type = c.type;
    generate(type, pet.looks, nullptr, [&] { return draw(); });
    c.appearanceFlags = pet.looks.flags;
    pet.body.size = pet.looks.box;
    buildPet(pet, owner.id);
}

// §9: each player's pet, found, rebuilt, made or removed by the Pet slot; §11.2 stale pets.
void Npcs::petPass(std::vector<PlayerView> &players, NpcFrame &frame) {
    if (namedWorld)
        for (PlayerView &p : players) {
            auto &owner = p.fighter;
            if (owner.controller != 0)
                continue;
            const Item &slot = owner.equipment[PET_SLOT];
            slot.write(owner.linkedItem.source);
            if (slot.type == 19) {
                // The first and post-recovery forward lookups each exclude zero.
                ActorEntry linked = owner.linkedId ? lookupActor(owner.linkedId, players) : ActorEntry{};
                if (!linked.combat()) {
                    owner.linkedId = 0;
                    for (const auto &[key, actor] : actors)
                        if (actor.combat() && actor.combat()->controller == 5 && actor.combat()->ownerId == owner.id) {
                            owner.linkedId = actor.combat()->id;
                            break;
                        }
                }
                linked = owner.linkedId ? lookupActor(owner.linkedId, players) : ActorEntry{};
                bool refreshed = false;
                if (linked.npc && !owner.linkedItem.matches(uint32_t(owner.skills[0]))) {
                    refreshed = true;
                    makePet(*linked.npc, slot, owner);
                }
                owner.linkedItem.publish(uint32_t(owner.skills[0])); // before possible construction, even with no link
                if (!refreshed && owner.linkedId == 0 && !(p.body.mode & GLIDER)) {
                    const uint64_t id = freeId();
                    owner.linkedId = id;
                    Npc child;
                    child.fighter.id = id;
                    child.fighter.controller = 5; // §9.3 step 3
                    child.zone = {worldgen::floorDiv((int32_t)std::floor(p.body.blocks().x), worldgen::ZONE),
                                  worldgen::floorDiv((int32_t)std::floor(p.body.blocks().y), worldgen::ZONE)};
                    child.home = p.body.blocks();
                    // §9.3 step 5: the maximum health of the reset controller-5 record with the
                    // item's type and level, before the builder caps the level and raises the
                    // health multiplier.
                    Combatant plain;
                    plain.controller = 5;
                    plain.type = slot.subtype;
                    plain.level = std::max(1, slot.level);
                    plain.characterClass = 0;
                    child.fighter.health = maxHealth(plain);
                    makePet(child, slot, owner);
                    child.body.place(p.body.blocks());
                    child.yaw = p.yaw;
                    appendNpc(std::move(child));
                }
            } else {
                // This category route deliberately looks up key zero too. §9.5: a pet still
                // carrying its kind-7 taming buff (the 5-s grace of §7.4) is kept.
                const ActorEntry linked = lookupActor(owner.linkedId, players);
                const Combatant *state = linked.combat();
                if (!state || !state->hasBuff(7)) {
                    owner.linkedId = 0;
                    if (linked.combat())
                        linked.combat()->health = 0;
                }
            }
        }

    // This later pass is reached even when the category/name gate skipped work.
    // Actor storage/key traversal remains valid throughout these state writes.
    for (const auto &[key, actor] : actors) {
        Combatant *current = actor.combat();
        if (!current)
            continue;
        const ActorEntry linked = lookupActor(current->linkedId, players);
        if (current == controlledActor && linked.combat() && current->equipment[PET_SLOT].type == 19) {
            Item &slot = current->equipment[PET_SLOT];
            const int32_t level = std::bit_cast<int16_t>(uint16_t(linked.combat()->level));
            const uint32_t experience = uint32_t(linked.combat()->experience);
            const bool changed = slot.level != level || slot.seed != experience;
            slot.level = level;
            slot.seed = experience;
            slot.write(current->linkedItem.source);
            if (changed) {
                frame.petItems.push_back({current->id, level, linked.combat()->experience});
            }
        }
        if (current->controller == 5) {
            const ActorEntry owner = lookupActor(current->ownerId, players);
            if (owner.combat() && owner.combat()->linkedId != current->id) {
                // §11.2, correction 17: the record and transient reset of §9.2 step 1 first.
                if (actor.npc)
                    resetCreature(*actor.npc);
                current->backing = {};
                current->linkedId = 0;
                current->health = 0;
                current->controller = 3;
                current->actionMs = 5000; // "Later pass over all creatures": +0x6C before the reverse clear
                current->ownerId = 0;
                if (actor.npc)
                    actor.npc->deathPassStarted = true; // dispose of the stale pet without death notifications
            }
        }
    }
}

// §7.3, ruling knockback-taming-charge B.2: one bite, the action-110 effect at its P1
// crossing. The creature's target (+0x11d0) must still exist and hold this creature's food in
// its Pet slot; otherwise nothing happens and nothing is drawn.
void Npcs::bite(Npc &n, std::vector<PlayerView> &players, NpcFrame &frame) {
    Combatant &c = n.fighter;
    const Combatant *feeder = lookupActor(c.relations.contact, players).combat();
    if (!feeder)
        return;
    const Item &food = feeder->equipment[PET_SLOT];
    if (food.type != 20 || int32_t(food.subtype) != c.type)
        return;
    const uint64_t feederId = feeder->id;
    const float r = (float)draw() / 32767.f; // the bite's only draw: the sound's pitch
    frame.sounds.push_back({n.body.blocks(), 44, 0.9f + 0.2f * r, 1}); // drink2.wav
    // A kind-6 signal at the creature's position, straight into section 2 (not the hit routine).
    HitRecord signal;
    signal.attacker = c.id, signal.target = feederId, signal.kind = 6;
    std::copy_n(n.body.position, signal.position.size(), signal.position.begin());
    frame.playerHits.push_back(signal);
    frame.emittedHits.push_back(signal);
    float progress = 0; // the last kind-7 entry's value, 0 if none
    for (const Buff &b : c.buffs)
        if (b.kind == 7)
            progress = b.value;
    progress += 0.2f; // binary32: five bites sum to exactly 1.0
    // Bite counter (field 11, the combo) + 1 and field 12 (its elapsed time) := 0.
    c.combo = std::bit_cast<int32_t>(uint32_t(c.combo) + 1u);
    c.comboElapsedMs = 0;
    if (progress >= 1.f) { // tamed: controller 5 and the pet behaviour with the feeder as owner
        c.controller = 5;
        buildPet(n, feederId);
        frame.tamed.push_back({c.id, feederId, c.type, c.level, c.experience});
        progress = 1.f;
    }
    // The kind-7 progress buff (5000 ms, carrying the owner field: 0 until the last bite)
    // replaces any earlier one and is announced in section 12.
    Buff record;
    record.kind = 7, record.value = progress, record.timeMs = 5000, record.source = c.ownerId;
    applyBuff(c, record);
    frame.buffs.push_back({c.id, c.id, record});
}

void Npcs::callPet(Combatant &owner, glm::dvec3 at) {
    // §13 step 2, server simulation §8 kind 8: the player's hit-counter map and target are
    // cleared, so the pet's Combat has nothing left to fight.
    owner.relations.counts.clear();
    owner.relations.contact = 0;
    if (Npc *pet = findNpc(owner.linkedId); pet && pet->fighter.controller == 5 && pet->fighter.ownerId == owner.id) {
        pet->fighter.relations.threats.clear();
        pet->target = 0;
        // Correction 24: the 3-D squared distance in blocks, single precision, above 40,000.
        float d[3];
        for (int k = 0; k < 3; k++)
            d[k] = float(std::bit_cast<int64_t>(uint64_t(pet->body.position[k]) - uint64_t(int64_t(at[k] * 65536.0)))) *
                   (1.f / 65536.f);
        if ((d[0] * d[0] + d[1] * d[1]) + d[2] * d[2] > 40000.f)
            pet->body.place(at);
    }
}

// §12: the owner's kill experience goes to a living pet, whose item follows.
void Npcs::sharePetExperience(Combatant &owner, int32_t experience, NpcFrame &frame) {
    Npc *pet = findNpc(owner.linkedId);
    if (!pet || pet->fighter.controller != 5 || pet->fighter.ownerId != owner.id || pet->fighter.health <= 0)
        return;
    addExperience(pet->fighter, experience);
    stampLinkedCredit(owner, pet->fighter, actorCreditItems(owner)); // retained legacy admission, shared copy boundary
    frame.petItems.push_back({owner.id, pet->fighter.level, pet->fighter.experience});
}

} // namespace creatures
