// Pets (docs/specs/alpha-pets-bombs.md Part B): taming by feeding, the pet pass that keeps each
// player's pet in step with the Pet slot, the pet's Combat and Companion behaviours, its return
// after death, sharing the owner's kill experience, T to call it back and the pet under its
// rider. These run where the world is decided: offline and on the server.
#include "creatures/Npcs.h"
#include "creatures/Physics.h"
#include "worldgen/Regions.h"
#include <algorithm>
#include <cmath>

namespace creatures {

namespace {

constexpr int PET_SLOT = 12;

// §9.2: what of the Pet slot item the pass compares (its type, creature, level, experience
// and name).
std::vector<uint8_t> itemKey(const Item &item) {
    std::vector<uint8_t> key = {item.type, item.subtype, item.material, item.rarity};
    for (int32_t v : {item.level, (int32_t)item.seed, item.spirits})
        for (int k = 0; k < 4; k++)
            key.push_back((uint8_t)(v >> (8 * k)));
    for (const Item::Cube &c : item.cubes)
        key.push_back(c.material);
    return key;
}

float yawTo(glm::vec2 d) { return (float)(std::atan2(-d.x, d.y) * 180.0 / 3.141592653589793); }

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

const Npc *Npcs::petOf(uint64_t owner) const {
    const auto it = pets.find(owner);
    if (it == pets.end())
        return nullptr;
    for (const Npc &n : list)
        if (n.fighter.id == it->second.pet)
            return &n;
    return nullptr;
}

uint64_t Npcs::freeId() const {
    uint64_t smallest = 0;
    if (actorPassActive) {
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
    pet.fighter = c;
    pet.owner = owner.id;
    pet.spawn = Npc::NO_SPAWN;
    pet.threats.clear();
    pet.target = 0;
    pet.threat = 0;
    pet.body.size = pet.looks.box;
    pet.deadMs = 0;
    pets[owner.id].pet = c.id;
}

// §9: each player's pet, found, rebuilt, made or removed by the Pet slot; §11.2 stale pets.
void Npcs::petPass(std::vector<PlayerView> &players) {
    for (PlayerView &p : players) {
        PetLink &link = pets[p.fighter.id];
        const Item &slot = p.fighter.equipment[PET_SLOT];
        Npc *pet = link.pet ? findNpc(link.pet) : nullptr;
        if (slot.type == 19) {
            if (!pet) { // §9.1: re-attach a pet that lost its link
                link.pet = 0;
                for (Npc &n : list)
                    if (n.fighter.controller == 5 && n.owner == p.fighter.id) {
                        pet = &n;
                        link.pet = n.fighter.id;
                        break;
                    }
            }
            const std::vector<uint8_t> key = itemKey(slot);
            if (pet && (p.fighter.skills[0] != link.skill || key != link.item)) {
                makePet(*pet, slot, p.fighter); // §9.2 (id, position and health kept)
            } else if (!pet && !(p.body.mode & GLIDER)) { // §9.3
                Npc n;
                n.fighter.id = freeId();
                n.zone = {worldgen::floorDiv((int32_t)std::floor(p.body.blocks().x), worldgen::ZONE),
                          worldgen::floorDiv((int32_t)std::floor(p.body.blocks().y), worldgen::ZONE)};
                n.home = p.body.blocks();
                // (The health of the creature as made, before the builder raises its maximum.)
                Combatant plain;
                plain.type = slot.subtype, plain.level = std::max(1, slot.level), plain.characterClass = 0;
                n.fighter.health = maxHealth(plain);
                makePet(n, slot, p.fighter);
                n.body.place(p.body.blocks());
                n.yaw = p.yaw;
                appendNpc(std::move(n));
            }
        } else if (pet) { // §9.5: the slot no longer holds a pet item
            if (pet->tameMs <= 0) {
                link.pet = 0;
                pet->fighter.health = 0;
            }
        } else {
            link.pet = 0;
        }
        link.skill = p.fighter.skills[0];
        link.item = itemKey(slot);
    }
    // §11.2: a pet whose owner is here but no longer names it is put away.
    for (Npc &n : list) {
        if (n.fighter.controller != 5 || !n.owner)
            continue;
        const bool ownerHere = std::any_of(players.begin(), players.end(), [&](const PlayerView &p) {
            return p.fighter.id == n.owner;
        });
        if (ownerHere && pets[n.owner].pet != n.fighter.id) {
            n.fighter.health = 0;
            n.fighter.controller = 3;
            n.owner = 0;
            n.deadMs = 5000;
        }
    }
}

// §10.1: the pet's target: the creature its owner has hit most, still alive.
uint64_t Npcs::petTarget(const Npc &pet, std::vector<PlayerView> &players) {
    const PlayerView *owner = nullptr;
    for (const PlayerView &p : players)
        if (p.fighter.id == pet.owner)
            owner = &p;
    if (!owner || owner->fighter.action == 106)
        return 0;
    uint64_t best = 0;
    int32_t most = 0;
    for (const auto &[id, count] : hitCounts[pet.owner]) {
        const Npc *n = findNpc(id);
        if (count > most && n && n->fighter.health > 0)
            best = id, most = count;
    }
    return best;
}

// §10.2: Companion: run when far, heal 10 % a second out of combat, walk up to the owner.
bool Npcs::companion(Npc &pet, uint64_t followed, std::vector<PlayerView> &players, float dtMs, const BlockType &block) {
    const Body *owner = nullptr;
    for (const PlayerView &p : players)
        if (p.fighter.id == followed)
            owner = &p.body;
    if (const Npc *n = owner ? nullptr : findNpc(followed); n && n->fighter.health > 0)
        owner = &n->body;
    if (!owner)
        return false;
    const glm::dvec3 d = owner->blocks() - pet.body.blocks();
    const double distance = glm::length(d);
    pet.body.mode = distance > 8 ? pet.body.mode | RUN : pet.body.mode & ~RUN;
    if (pet.fighter.controller == 5) // a pet heals out of combat
        pet.fighter.health = std::min(maxHealth(pet.fighter), pet.fighter.health + maxHealth(pet.fighter) * 0.1f * dtMs / 1000);
    pet.body.intent = glm::vec3(0);
    if (distance < 4)
        return false;
    // Path search §3: when the straight route is blocked, search toward the ground under the
    // owner (the first solid block within 21 down, then the first open cell within 21 up).
    if (!clearRoute(pet.body.size, pet.body.swimmer, pet.body.blocks(), owner->blocks(), pet.body.size.x + owner->size.x,
                    block, objects)) {
        const glm::dvec3 o = owner->blocks();
        const int32_t gx = (int32_t)std::floor(o.x), gy = (int32_t)std::floor(o.y);
        int32_t gz = (int32_t)std::floor(o.z);
        for (int k = 0; k < 21 && !(block(gx, gy, gz) != 0 && block(gx, gy, gz) != 2); k++)
            gz--;
        for (int k = 0; k < 21 && block(gx, gy, gz) != 0 && block(gx, gy, gz) != 2; k++)
            gz++;
        const glm::dvec3 G(gx + 0.5, gy + 0.5, gz);
        if (glm::length(G - (pet.body.blocks() - glm::dvec3(0, 0, pet.body.size.z * 0.5))) <= 4) {
            pet.path.clear();
            return false;
        }
        if (pet.path.empty() && !pet.path.searching())
            pet.path.start(pet.body.blocks() - glm::dvec3(0, 0, pet.body.size.z * 0.5 - 0.1), G, 2);
        pet.path.rounds(pet.body.size, 2, block);
        return true;
    }
    pet.path.clear();
    glm::vec3 intent(5 * (float)d.x, 5 * (float)d.y, 0);
    if (glm::length(intent) > 30)
        intent = 30.f * glm::normalize(intent);
    pet.body.intent = intent;
    pet.yaw = yawTo(glm::vec2(d));
    return true;
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

void Npcs::callPet(uint64_t owner, glm::dvec3 at) {
    hitCounts[owner].clear(); // §13 step 2: the pet's Combat has nothing left to fight
    const auto it = pets.find(owner);
    if (it == pets.end())
        return;
    if (Npc *pet = findNpc(it->second.pet)) {
        pet->threats.clear();
        pet->target = 0;
        const glm::dvec3 d = pet->body.blocks() - at;
        if (glm::dot(d, d) > 40000.0)
            pet->body.place(at);
    }
}

// §12: the owner's kill experience goes to a living pet, whose item follows.
void Npcs::sharePetExperience(uint64_t owner, int32_t experience, NpcFrame &frame) {
    const auto it = pets.find(owner);
    Npc *pet = it == pets.end() ? nullptr : findNpc(it->second.pet);
    if (!pet || pet->fighter.health <= 0)
        return;
    addExperience(pet->fighter, experience);
    frame.petItems.push_back({owner, pet->fighter.level, pet->fighter.experience});
    // (The item's copy follows too, so the pass does not rebuild the pet for it.)
    if (it->second.item.size() >= 12) {
        for (int k = 0; k < 4; k++) {
            it->second.item[4 + k] = (uint8_t)(pet->fighter.level >> (8 * k));
            it->second.item[8 + k] = (uint8_t)(pet->fighter.experience >> (8 * k));
        }
    }
}

} // namespace creatures
