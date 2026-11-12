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

// §9.2 steps 1-5 and §9.4: the creature becomes the pet of `owner` as the Pet slot item says.
void Npcs::makePet(Npc &pet, const Item &item, const Combatant &owner) {
    pet.path.clear();
    pet.path.state.reference = {-1, -1, 0};
    Combatant c;
    c.id = pet.fighter.id;
    c.health = pet.fighter.health;
    c.type = item.subtype;
    c.level = std::max(1, item.level);
    c.experience = (int32_t)item.seed;
    c.controller = 5;
    c.characterClass = 0;
    int32_t type = c.type;
    generate(type, pet.looks, nullptr, [&] { return draw(); });
    c.appearanceFlags = pet.looks.flags;
    // §9.4: the behaviour builder's multipliers, per-type changes, level cap and health.
    float m[5] = {100, 1, 1, 1, 1};
    if (c.type == 25)
        m[0] = 300, m[2] = 0.1f, m[3] = 5, m[4] = 5;
    if (c.type == 86)
        c.characterClass = 3, c.specialization = 1;
    if (c.type == 104)
        c.characterClass = 2, c.specialization = 1;
    std::copy(m, m + 5, c.multipliers);
    c.level = std::min(c.level, owner.level);
    c.multipliers[0] *= 1 + 0.5f * skillFactor(owner.skills[0]);
    c.actionMs = 1000000;
    c.linkedItem.source.fill(0); // native reset differs from fresh item construction
    c.equipment[PET_SLOT].level = 0;
    c.linkedItem.retained = pet.fighter.linkedItem.retained;
    c.behaviorReference = pet.fighter.behaviorReference; // the held body reset preserves field40
    pet.fighter = c;
    pet.body.mode = c.mode;
    pet.fighter.ownerId = owner.id;
    pet.spawn = Npc::NO_SPAWN;
    pet.fighter.relations.threats.clear();
    pet.target = 0;
    pet.threat = 0;
    pet.body.size = pet.looks.box;
    pet.deadMs = 0;
    pet.fatalPresented = false;
    pet.fatalLootApplied = false;
    pet.fatalRegionApplied = false;
    pet.fatalRecipientsApplied = false;
    pet.fatalBackingApplied = false;
    if (Combatant *mappedOwner = lookupActor(owner.id, {}).combat())
        mappedOwner->linkedId = c.id;
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
                    child.zone = {worldgen::floorDiv((int32_t)std::floor(p.body.blocks().x), worldgen::ZONE),
                                  worldgen::floorDiv((int32_t)std::floor(p.body.blocks().y), worldgen::ZONE)};
                    child.home = p.body.blocks();
                    Combatant plain;
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
                // This category route deliberately looks up key zero too.
                const ActorEntry linked = lookupActor(owner.linkedId, players);
                const Combatant *state = linked.combat();
                // Keep the preexisting engine feeding grace until its full
                // selector-seven producer is connected; resident presence wins.
                const bool feedingGrace = linked.npc && linked.npc->tameMs > 0;
                if (!state || (!state->hasBuff(7) && !feedingGrace)) {
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
                current->backing = {};
                current->linkedId = 0;
                current->health = 0;
                current->controller = 3;
                current->ownerId = 0;
                if (actor.npc)
                    actor.npc->deadMs = 5000; // retained engine corpse-disposal adapter
            }
        }
    }
}

// §7.3: one bite of a creature being fed (the action-110 effect at its hit moment).
void Npcs::bite(Npc &n, std::vector<PlayerView> &players, NpcFrame &frame) {
    for (PlayerView &p : players) {
        const Item &food = p.fighter.equipment[PET_SLOT];
        if (p.fighter.id != n.target || food.type != 20 || food.subtype != n.fighter.type)
            continue;
        frame.sounds.push_back({n.body.blocks(), 44, 0.9f + 0.2f * (float)draw() / 32767.f, 1}); // drink2.wav
        HitRecord signal;
        signal.attacker = n.fighter.id, signal.target = p.fighter.id, signal.kind = 6; // not damage
        frame.playerHits.push_back(signal);
        frame.emittedHits.push_back(signal);
        float progress = (n.tameMs > 0 ? n.tameProgress : 0.f) + 0.2f;
        n.bites++;
        if (progress >= 1) { // tamed: the player's pet from now on
            Item asItem;
            asItem.type = 19, asItem.subtype = (uint8_t)n.fighter.type, asItem.level = n.fighter.level;
            asItem.seed = (uint32_t)n.fighter.experience;
            const float health = n.fighter.health;
            makePet(n, asItem, p.fighter);
            n.fighter.health = health;
            frame.tamed.push_back({n.fighter.id, p.fighter.id, n.fighter.type, n.fighter.level, n.fighter.experience});
            progress = 1;
        }
        n.tameProgress = progress;
        n.tameMs = 5000;
        n.tamer = p.fighter.id;
        return;
    }
}

void Npcs::callPet(Combatant &owner, glm::dvec3 at) {
    owner.relations.counts.clear(); // §13 step 2: the pet's Combat has nothing left to fight
    if (Npc *pet = findNpc(owner.linkedId); pet && pet->fighter.controller == 5 && pet->fighter.ownerId == owner.id) {
        pet->fighter.relations.threats.clear();
        pet->target = 0;
        const glm::dvec3 d = pet->body.blocks() - at;
        if (glm::dot(d, d) > 40000.0)
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
