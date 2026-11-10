#include "creatures/Strike.h"
#include "creatures/Npcs.h"
#include "creatures/FatalRecipients.h"
#include "creatures/TargetSelection.h"
#include "creatures/CreatedBehavior.h"
#include "creatures/DescriptorEquipment.h"
#include <cassert>
#include <bit>
#include "BlockEdits.h"
#include "Loot.h"
#include "worldgen/Random.h"
#include "worldgen/Regions.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <set>
#include <utility>

namespace creatures {

namespace {

constexpr float PI = 3.14159265358979f;

struct FlagScope {
    bool &flag;
    bool previous;
    explicit FlagScope(bool &value) : flag(value), previous(std::exchange(value, true)) {}
    ~FlagScope() { flag = previous; }
};

glm::vec2 facing(float yawDegrees) {
    const float a = yawDegrees * PI / 180.f;
    return {-std::sin(a), std::cos(a)};
}

float yawToward(glm::vec2 d) { return std::atan2(-d.x, d.y) * 180.f / PI; }

// Open points 4 §5.2: who fights by the ranged rule. (Only the subtype counts, not the item type.)
bool shooter(const Combatant &c) {
    const int m = c.mainHand().subtype;
    return c.type == 104 || m == BOW || m == CROSSBOW || m == BOOMERANG;
}

} // namespace

// §13.2 step 7: whether `a` would fight `b` (0x596ca0).
bool hostile(const Combatant &a, const Combatant &b) {
    if (a.controller == 6 || b.controller == 6)
        return false;
    if (a.controller == 5 && b.controller == 0)
        return false;
    if ((a.controller == 1) != (b.controller == 1))
        return true;
    if (a.controller == 1 && b.controller == 1 &&
        harmless(a.type, a.appearanceFlags) != harmless(b.type, b.appearanceFlags))
        return true;
    return (a.mode & 0x20) || (b.mode & 0x20);
}

namespace {

// Legacy caller positions are engine block coordinates. The query itself keeps
// raw fixed positions and the held visibility/ray numeric boundaries.
bool sight(glm::dvec3 a, glm::dvec3 b, const BlockType &block, const visibility::RecordLookup &records) {
    visibility::Position start, end;
    for (size_t axis = 0; axis < 3; axis++) {
        start[axis] = int64_t(a[axis] * 65536.0);
        end[axis] = int64_t(b[axis] * 65536.0);
    }
    return visibility::visible(start, end, {block, records});
}

// (open points 4 §5, §7: strictly past its end; only the dodge accepts the exact moment)
bool actionOver(const Combatant &c) { return c.actionMs > (float)duration(c, c.action); }

} // namespace

bool harmless(int32_t type, uint16_t appearanceFlags) {
    if (appearanceFlags & 0x1a00)
        return false;
    switch (type) {
    case 19: case 20: case 22: case 23: case 25: case 26: case 27: case 30: case 31: case 32: case 33: case 34:
    case 35: case 53: case 55: case 56: case 57: case 58: case 67: case 68: case 74: case 87: case 89: case 92:
    case 93: case 98: case 100: case 106: case 107: case 145: case 146: case 147:
        return true;
    default:
        return false;
    }
}

// The older actor setup entry is an engine projection onto the held descriptor
// kernel. Complete item backing survives each partial header/scalar mutation.
void equip(Combatant &c, const Appearance &looks, const std::function<int()> &draw, bool generating,
           int32_t descriptorRole, creation::Configuration &configuration) {
    if (looks.flags & 0x40)
        return;
    std::array<ItemRecord, SLOT_COUNT> records;
    for (size_t slot = 0; slot < records.size(); slot++)
        records[slot] = c.equipmentRecord(slot);
    equipment::setup({c.type, descriptorRole, uint16_t(c.level), looks.flags, c.characterClass, c.specialization,
                      records, c.multipliers, c.specials, configuration.choice, configuration.kinds},
                     generating ? 0 : 1, draw);
    for (size_t slot = 0; slot < records.size(); slot++)
        c.setEquipment(slot, records[slot]);
}

namespace {
void equipDescriptor(worldgen::Spawn &s, const Appearance &looks, const std::function<int()> &draw,
                     bool generating) {
    if (looks.flags & 0x40)
        return;
    // The generator currently owns compact item headers. Preserve every
    // represented field; this bridge does not claim full native payload storage.
    std::array<ItemRecord, SLOT_COUNT> records;
    for (size_t slot = 0; slot < records.size(); slot++)
        records[slot] = toRecord(s.equipment[slot]);
    equipment::setup({s.type, s.controller, uint16_t(s.level), looks.flags, s.characterClass, s.specialization,
                      records, s.multipliers, s.specials, s.creationChoice, s.creationKinds},
                     generating ? 0 : 1, draw);
    for (size_t slot = 0; slot < records.size(); slot++) {
        const auto &record = records[slot];
        auto &item = s.equipment[slot];
        item.type = record[0]; item.subtype = record[1];
        std::memcpy(&item.seed, record.data() + 4, 4);
        std::memcpy(&item.product, record.data() + 8, 4);
        item.rarity = record[0x0c]; item.material = record[0x0d]; item.byte0e = record[0x0e];
        item.level = itemLevel(record);
    }
}
void refreshDescriptor(worldgen::Spawn &s, uint64_t identifier, const activation::Context &context,
                       worldgen::Random &random) {
    Appearance looks;
    if (s.finished)
        looks = Appearance::parse(s.appearance.data());
    looks.flags |= s.appearanceFlags;
    activation::Services services;
    services.seed = [&](uint32_t seed) { random.seed(seed); };
    services.setup = [&](int32_t argument) {
        assert(argument == 1);
        equipDescriptor(s, looks, [&] { return random.draw(); }, false);
    };
    services.equipmentLevel = [&](size_t slot) -> int16_t & { return s.equipment[slot].level; };
    services.inventory = [&](const activation::InventoryVisitor &visit) {
        if (s.inventoryValue) {
            // Generated snapshots stay immutable. Publish the refreshed owned
            // descriptor value after the ordered synchronous visitor ends.
            auto refreshed = std::make_shared<InventoryValue>(*s.inventoryValue);
            for (auto &group : refreshed->groups)
                for (auto &entry : group) {
                    int16_t level = itemLevel(entry.item);
                    uint8_t rarity = itemRarity(entry.item);
                    visit({entry.count, itemType(entry.item), itemSubtype(entry.item), level, rarity});
                    setItemLevel(entry.item, level);
                    entry.item[0x0c] = rarity;
                }
            s.inventoryValue = std::move(refreshed);
            return;
        }
        // Legacy generator rows remain an explicit one-unit compact adapter.
        for (auto &item : s.inventory)
            visit({1, item.type, item.subtype, item.level, item.rarity});
    };
    services.draw = [&] { return random.draw(); };
    activation::refresh({identifier, s.controller, s.level, s.powerTier, s.appearanceFlags, s.bossLoot},
                        &context, services);
    if (!s.finished) {
        looks.flags = s.appearanceFlags;
        looks.serialize(s.appearance.data());
        s.finished = true;
    } else {
        std::memcpy(s.appearance.data() + 6, &s.appearanceFlags, 2);
    }
}
} // namespace

void finishSpawn(worldgen::Spawn &s, worldgen::Random &random, bool whole) {
    const auto draw = [&random] { return random.draw(); };
    // Step 2: the appearance, its flags OR-ed into what the creators set.
    Appearance looks;
    int32_t type = s.type;
    generate(type, looks, nullptr, draw);
    looks.flags |= s.appearanceFlags;
    // Step 3: mode-0 equipment, on a working creature made from the spawn.
    equipDescriptor(s, looks, draw, true);
    s.appearanceFlags = looks.flags;
    looks.serialize(s.appearance.data());
    s.finished = true;
    if (!whole)
        return;
    // Step 4: a boss carries a spirit (item services §3.4): fire, unholy, ice or wind.
    const bool boss = (looks.flags & 0x200) != 0;
    if (boss) {
        worldgen::ItemRecord spirit;
        spirit.type = 11, spirit.subtype = 14;
        spirit.level = (int16_t)s.level;
        spirit.rarity = 2;
        spirit.material = (uint8_t)(128 + random.draw() % 4);
        s.inventory.push_back(spirit);
    }
    // Steps 5-7: rarities of seven slots (forced for a boss), levels re-rolled down (not for
    // a boss), and one discarded draw for a creature that is not a boss.
    for (int slot : {7, 6, 5, 2, 4, 3, 1})
        s.equipment[slot].rarity = (uint8_t)rarityRoll(random, s.powerTier, boss);
    if (!boss)
        for (int slot : {7, 6, 5, 2, 4, 3, 1, 8, 9})
            s.equipment[slot].level = rerollLevel(random, s.equipment[slot].level, false);
    if (s.controller == 1 && !boss)
        random.draw();
}

int Npcs::draw() {
    return random().draw();
}

// §9.2 (server §5.2): the spawns of the active zones, the 3 x 3 round each player's zone,
// come alive when any player is within their radius.
void Npcs::spawnCreature(const worldgen::Spawn &s, std::pair<int32_t, int32_t> key, size_t index, size_t base,
                         uint64_t id, const std::array<int64_t, 3> &position, bool descriptorBacked) {
    const glm::dvec3 p = glm::dvec3(double(position[0]), double(position[1]), double(position[2])) / 65536.0;
    Npc npc;
    npc.zone = key;
    npc.spawn = descriptorBacked ? index : Npc::NO_SPAWN;
    npc.home = p;
    npc.despawnRadius = s.radius;
    Combatant &c = npc.fighter;
    c.id = id;
    if (descriptorBacked)
        c.backing = {key.first, key.second, (int32_t)index};
    if (descriptorBacked)
        c.behaviorReference = position;
    c.type = s.type;
    c.controller = (uint8_t)s.controller;
    c.characterClass = s.characterClass;
    npc.talkKind = s.field42;
    npc.talkTarget = {s.field41[0], s.field41[1]};
    c.specialization = 0;
    c.level = s.level;
    c.powerTier = s.powerTier;
    std::copy(s.multipliers, s.multipliers + 5, c.multipliers);
    if (s.finished) { // §9.2 step 7: the looks the zone generator gave it
        npc.looks = Appearance::parse(s.appearance.data());
        c.specialization = s.specialization;
        c.specials = s.specials;
    } else {
        int32_t type = s.type;
        generate(type, npc.looks, nullptr, [&] { return draw(); });
    }
    npc.creationConfig.choice = s.creationChoice;
    npc.creationConfig.kinds = s.creationKinds;
    // §9.1: the spawn's own appearance flags and gear (a town guard keeps its gear, 0x40).
    npc.looks.flags |= s.appearanceFlags;
    npc.boss = s.bossLoot;
    npc.behaviour = s.behaviour;
    if (s.characterClass >= 0x80 && s.characterClass <= 0x84) { // schedules §3.6: back to the counter
        npc.behaviour = 4;
        npc.route = {p};
        npc.routeRadius = 2;
    }
    for (const worldgen::ScheduleStop &stop : s.schedule)
        npc.schedule.push_back({glm::dvec3((double)stop.x, (double)stop.y, (double)stop.z) / 65536.0, stop.timeMs});
    for (const auto &point : s.route)
        npc.route.push_back(glm::dvec3(point[0], point[1], point[2]));
    if (s.behaviour == 5 || s.behaviour == 6)
        npc.routeRadius = 2; // (location spec §4.3.2, §4.4: WalkPath radius 2)
    if (s.leader >= 0) {
        npc.follows = spawnId(key.first, key.second, s.leader + (index >= base ? base : 0));
    }
    if (onActivate)
        onActivate(npc);
    for (int slot = 0; slot < SLOT_COUNT - 1; slot++) {
        const worldgen::ItemRecord &r = s.equipment[slot];
        if (s.finished) {
            c.setEquipment(slot, toRecord(r));
            continue;
        }
        c.equipmentSource(slot) = toRecord(r);
        if (r.type) {
            c.equipment[slot].type = r.type;
            c.equipment[slot].subtype = r.subtype;
            c.equipment[slot].material = r.material;
            c.equipment[slot].rarity = r.rarity;
            c.equipment[slot].level = r.level ? r.level : c.level;
            c.equipment[slot].seed = r.seed;
        }
    }
    c.appearanceFlags = npc.looks.flags;
    c.linkedItem.source = toRecord(s.equipment[12]);
    c.equipment[12] = Item::parse(c.linkedItem.source.data());
    if (!s.finished)
        equip(c, npc.looks, [&] { return draw(); }, false, s.controller, npc.creationConfig);
    if (s.inventoryValue) {
        npc.inventory.copyFrom(*s.inventoryValue);
    } else if (!s.inventory.empty()) {
        // Direct one-unit rows do not run automatic counted-type normalization.
        for (const auto &item : s.inventory)
            npc.inventory.appendUnit(toRecord(item));
    }
    c.health = maxHealth(c);
    c.actionMs = 1000000;
    if (npc.looks.flags & 0x200) // (the recorded bosses are all twice the size of their kind)
        npc.looks.box *= 2.f;
    npc.body.size = npc.looks.box;
    npc.body.swimmer = (npc.looks.flags & SWIMMER) != 0;
    npc.body.flyer = (npc.looks.flags & 0x0002) != 0; // models §3.3: flyers fall slowly
    npc.body.place({p.x, p.y, p.z + npc.looks.box.z * 0.5 + 0.01}); // §9.2 step 7: at the spawn's position
    npc.body.fallStart = (float)p.z;
    npc.yaw = s.yaw;
    npc.quest = s.quest;
    npc.questRole = s.questRole;
    npc.path.records = routeRecords;
    npc.path.columns = routeColumns;
    npc.path.activate = routeActivate;
    if (s.questRole == 2) { // a quest's prisoner sits tied up until freed
        c.action = 83;
        c.mode |= 0x400;
    }
    npc.body.mode = c.mode;
    appendNpc(std::move(npc));
}

void Npcs::activate(const worldgen::WorldMap &map, const std::vector<PlayerView> &players, double timeOfDayMs,
                    int32_t day, float dtMs) {
    // The production engine clock supplies an in-range integer millisecond
    // value. Keep the native two-query caller relation explicit in the kernel.
    const std::function<int32_t()> readClock = [&] { return int32_t(timeOfDayMs); };
    std::set<std::pair<int32_t, int32_t>> active;
    for (const PlayerView &p : players) {
        const glm::dvec3 at = p.body.blocks();
        const int32_t zx = worldgen::floorDiv((int32_t)std::floor(at.x), worldgen::ZONE);
        const int32_t zy = worldgen::floorDiv((int32_t)std::floor(at.y), worldgen::ZONE);
        for (int32_t i = -1; i <= 1; i++)
            for (int32_t j = -1; j <= 1; j++)
                if (map.zones().count({zx + i, zy + j}))
                    active.insert({zx + i, zy + j});
    }
    for (const auto &key : active) {
        const auto &zone = map.zones().at(key);
        const auto extra = extras.find(key);
        const size_t base = zone->spawns.size();
        const size_t total = base + (extra == extras.end() || extra->second.first != base ? 0 : extra->second.second.size());
        const auto context = descriptorContext ? descriptorContext(key.first, key.second) :
                                                std::optional<activation::Context>{};
        for (size_t i = 0; i < total; i++) {
            SpawnState &state = spawns[{key.first, key.second, i}];
            if (state.sourceZone.lock() != zone) {
                state.refreshed.reset();
                state.sourceZone = zone;
            }
            const worldgen::Spawn &s = state.refreshed ? *state.refreshed :
                i < base ? zone->spawns[i] : extra->second.second[i - base];
            state.countdownMs -= dtMs;
            if (!activation::inWindow(s.windowStart, s.windowEnd, readClock))
                continue;
            const uint64_t id = spawnId(key.first, key.second, i);
            if (lookupActor(id, {}).combat())
                continue;
            if (state.countdownMs < 0)
                state.countdownMs = 0;
            if (state.countdownMs != 0 && state.deathDay == day)
                continue;
            std::array<int64_t, 3> position{s.x, s.y, s.z};
            // Effective descriptor position, retaining its raw coordinates through schedule selection.
            for (const worldgen::ScheduleStop &stop : s.schedule)
                if (timeOfDayMs >= stop.timeMs)
                    position = {stop.x, stop.y, stop.z};
            const bool near = std::any_of(players.begin(), players.end(), [&](const PlayerView &v) {
                return activation::inRange(v.body.position, position, s.radius);
            });
            if (!near)
                continue;
            if (context && context->selector == 5 && (s.appearanceFlags & 0x1000)) {
                if (!state.refreshed)
                    state.refreshed = std::make_unique<worldgen::Spawn>(s);
                refreshDescriptor(*state.refreshed, id, *context, random());
            }
            spawnCreature(state.refreshed ? *state.refreshed : s, key, i, base, id, position);
        }
    }
    // Quests: their creatures, near any player, until dead or gone.
    for (const auto &[quest, creatures] : questSpawns)
        for (size_t i = 0; i < creatures.size(); i++) {
            const worldgen::Spawn &s = creatures[i];
            const uint64_t id = questCreatureId(quest, i);
            if (questGone.count(id) || lookupActor(id, {}).combat())
                continue;
            const glm::dvec3 p = glm::dvec3((double)s.x, (double)s.y, (double)s.z) / 65536.0;
            const bool near = std::any_of(players.begin(), players.end(), [&](const PlayerView &v) {
                const glm::dvec3 d = p - v.body.blocks();
                return glm::dot(d, d) < (double)s.radius * s.radius;
            });
            if (near) {
                const int32_t zx = worldgen::floorDiv((int32_t)std::floor(p.x), worldgen::ZONE);
                const int32_t zy = worldgen::floorDiv((int32_t)std::floor(p.y), worldgen::ZONE);
                spawnCreature(s, {zx, zy}, i, 0, id, {s.x, s.y, s.z}, false); // engine quests have no native descriptor backing
            }
        }
}

void Npcs::setQuestCreatures(uint32_t quest, std::vector<worldgen::Spawn> creatures) {
    questSpawns[quest] = std::move(creatures);
}

void Npcs::removeQuest(uint32_t quest) {
    questSpawns.erase(quest);
    list.erase(std::remove_if(list.begin(), list.end(), [&](const Npc &n) { return n.quest == quest && n.leaveMs < 0; }),
               list.end());
}

uint32_t Npcs::free(uint64_t id, glm::dvec3 home) {
    for (Npc &n : list)
        if (n.fighter.id == id && n.questRole == 2 && n.fighter.controller == 6 && n.fighter.health > 0) {
            n.fighter.controller = 3;
            n.fighter.action = 0, n.fighter.actionMs = 0;
            n.fighter.mode &= ~0x400u;
            n.body.mode = n.fighter.mode;
            n.body.intent = {0, 0, 1}; // (stands up)
            n.behaviour = 2;           // walks off toward home along a one-point route
            n.route = {home};
            n.routeIndex = 0;
            n.leaveMs = 60000;
            return n.quest;
        }
    return 0;
}

bool Npcs::guarded(uint32_t quest, glm::dvec3 at, double radius) const {
    for (const Npc &n : list)
        if (n.quest == quest && n.questRole == 1 && n.fighter.health > 0) {
            const glm::dvec3 d = n.body.blocks() - at;
            if (glm::dot(d, d) < radius * radius)
                return true;
        }
    return false;
}

void Npcs::setExtraSpawns(int32_t zx, int32_t zy, size_t base, std::vector<worldgen::Spawn> extra,
                          bool removeCreatures) {
    if (removeCreatures)
        if (const auto previous = extras.find({zx, zy}); previous != extras.end()) {
            const uint64_t first = spawnId(zx, zy, previous->second.first);
            const uint64_t count = previous->second.second.size();
            // Current engine descriptor IDs form this wrapped range. Match
            // those keys, including a tamed actor, rather than its current role
            // or descriptor association. An already empty tail removes nobody.
            std::erase_if(list, [&](const Npc &n) { return uint64_t(n.fighter.id - first) < count; });
        }
    for (auto it = spawns.lower_bound({zx, zy, base}); it != spawns.end() && std::get<0>(it->first) == zx &&
                                                        std::get<1>(it->first) == zy;)
        it = spawns.erase(it);
    extras[{zx, zy}] = {base, std::move(extra)};
}

void Npcs::trimMissionSpawns(const Combatant &target, const worldgen::WorldMap &map) {
    visitMissionCells(target, [&](int32_t x, int32_t y) {
        const auto *zone = map.zone(x, y);
        if (!zone)
            return;
        const auto extra = extras.find({x, y});
        const size_t base = extra == extras.end() ? zone->spawns.size() : extra->second.first;
        if (base != zone->spawns.size())
            return; // mismatched engine prefix storage is outside the valid tail contract
        for (auto state = spawns.lower_bound({x, y, base}); state != spawns.end() &&
                std::get<0>(state->first) == x && std::get<1>(state->first) == y;)
            state = spawns.erase(state);
        if (extra != extras.end())
            extra->second.second.clear(); // destroy owned tail values, retain vector capacity and prefix boundary
    });
}

bool Npcs::markFatalBacking(const Combatant &target, const worldgen::WorldMap &map, int32_t day) {
    const auto &backing = target.backing;
    if (backing.x < 0 || backing.y < 0 || backing.x >= 65536 || backing.y >= 65536 || backing.index < 0)
        return false;
    const auto *zone = map.zone(backing.x, backing.y);
    if (!zone)
        return false;
    size_t count = zone->spawns.size();
    if (const auto extra = extras.find({backing.x, backing.y}); extra != extras.end()) {
        if (extra->second.first != count)
            return false; // invalid engine prefix ownership is outside the retained-vector contract
        count += extra->second.second.size();
    }
    if (size_t(backing.index) >= count)
        return false;
    auto &state = spawns[{backing.x, backing.y, size_t(backing.index)}];
    state.countdownMs = 1200000;
    state.deathDay = day;
    return true;
}

uint64_t Npcs::placeBomb(glm::dvec3 at, int32_t level) {
    Npc npc;
    npc.spawn = Npc::NO_SPAWN;
    npc.zone = {worldgen::floorDiv((int32_t)std::floor(at.x), worldgen::ZONE),
                worldgen::floorDiv((int32_t)std::floor(at.y), worldgen::ZONE)};
    npc.home = at;
    Combatant &c = npc.fighter;
    c.id = freeId();
    c.type = 144;
    c.characterClass = 0; // (no class: the plain NPC health 100 P(L))
    c.controller = 6;
    c.level = std::max(level, 1);
    int32_t type = 144;
    generate(type, npc.looks, nullptr, [&] { return draw(); });
    c.appearanceFlags = npc.looks.flags;
    c.health = 500; // the new-creature default; the fuse holds it to the maximum
    c.actionMs = 1000000;
    npc.body.size = npc.looks.box;
    npc.body.place(at);
    npc.body.fallStart = (float)at.z;
    const uint64_t id = c.id;
    appendNpc(std::move(npc));
    return id;
}

void Npcs::appendNpc(Npc npc) {
    list.push_back(std::move(npc));
    Npc &added = list.back();
    actors[added.fighter.id] = {&added, nullptr};
}

void Npcs::createActor(const ActorEntry &current, int32_t step) {
    const Combatant *creator = current.combat();
    if (!creator || uint8_t(creator->action) != 92)
        return;
    const auto view = [](const ActorEntry &entry) {
        return creation::Actor{entry.combat(), entry.body(), entry.npc ? &entry.npc->looks :
            entry.player ? entry.player->appearance : nullptr};
    };
    const creation::Actor source = view(current);
    if (!source.looks)
        return; // a supplied player view needs appearance state for this engine binding
    const creation::Configuration defaults;
    const auto &configuration = current.npc ? current.npc->creationConfig :
        current.player->creationConfig ? *current.player->creationConfig : defaults;
    creation::World world;
    // This NPC world runs offline and on the engine's authoritative server.
    // Network-client creation/authority is a separate integration boundary.
    world.designated = controlledActor;
    world.visit = [&](const creation::Visitor &visit) {
        for (const auto &[key, entry] : actors)
            visit(key, entry.combat());
    };
    world.lookup = [&](uint64_t key) {
        const auto entry = actors.find(key);
        return entry == actors.end() ? creation::Actor{} : view(entry->second);
    };
    creation::Services services;
    services.anticipation = [&](int32_t action) {
        return phases(*creator, action < 0 ? uint8_t(creator->action) : action).windUp;
    };
    services.maximumHealth = maxHealth;
    services.draw = [&] { return draw(); };
    auto result = creation::create(source, configuration, step, world, services);
    if (!result.actor)
        return;
    auto &created = *result.actor;
    Npc npc;
    npc.fighter = std::move(created.fighter);
    npc.body = created.body;
    npc.looks = created.looks;
    npc.spawn = Npc::NO_SPAWN;
    npc.home = npc.body.blocks(); // existing engine navigation/unload adapter
    npc.zone = {worldgen::floorDiv(int32_t(std::floor(npc.home.x)), worldgen::ZONE),
                worldgen::floorDiv(int32_t(std::floor(npc.home.y)), worldgen::ZONE)};
    npc.behaviour = 7;
    npc.follows = created.setup.companionKey;
    npc.specialMs = created.setup.specialMs;
    npc.targetRange = created.setup.range;
    npc.chaseMs = created.setup.chaseMs;
    npc.cooldownMs = created.setup.cooldownMs;
    npc.createdWalkMs = created.setup.walkMs;
    appendNpc(std::move(npc)); // live predecessor publication is last
}

void Npcs::refreshActors(std::span<PlayerView> players) {
    controlledActor = nullptr;
    for (auto &[key, actor] : actors)
        actor = {};
    for (Npc &npc : list)
        actors[npc.fighter.id] = {&npc, nullptr};
    for (PlayerView &player : players) {
        actors[player.fighter.id] = {nullptr, &player};
        if (player.local)
            controlledActor = &player.fighter;
    }
    std::erase_if(actors, [](const auto &entry) { return !entry.second.combat(); });
}

Npcs::ActorEntry Npcs::lookupActor(uint64_t key, std::span<PlayerView> players) {
    if (actorLookupActive) {
        const auto entry = actors.find(key);
        return entry == actors.end() ? ActorEntry{} : entry->second;
    }
    if (Npc *npc = findNpc(key))
        return {npc, nullptr};
    for (PlayerView &player : players)
        if (player.fighter.id == key)
            return {nullptr, &player};
    return {};
}

void Npcs::blast(Npc &bomb, std::vector<PlayerView> &players, NpcFrame &frame) {
    if (!actorLookupActive)
        refreshActors(players);
    FlagScope indexScope{actorLookupActive};
    const glm::dvec3 P = bomb.body.blocks();
    frame.bursts.push_back(bombBurst(P)); // particles §5.1
    auto hit = [&](Combatant &target, Body &body, glm::vec3 box) {
        if (&target == &bomb.fighter || target.health <= 0 || target.dodgeMs > 0 || body.dodgeMs > 0)
            return;
        const glm::dvec3 d = body.blocks() - P;
        const double reach = 8 + 0.7 * box.x;
        if (d.x * d.x + d.y * d.y >= reach * reach || std::abs(d.z) >= 8 + bomb.looks.box.z + 0.5 * box.z)
            return;
        HitInput in;
        in.critical = draw() % 4 == 0;
        in.amount = 25 * levelPower(bomb.fighter.level) * (in.critical ? 2 : 1);
        in.area = in.magic = true;
        glm::vec2 u(d.x, d.y);
        if (glm::dot(u, u) > 0.01f)
            u = glm::normalize(u);
        in.knockBack = 10.f * glm::vec3(u, 0.25f);
        in.targetOnGround = (body.contact & ON_GROUND) != 0;
        in.targetPosition = body.blocks();
        std::copy_n(body.position, in.targetRecordPosition.size(), in.targetRecordPosition.begin());
        std::copy_n(bomb.body.position, in.attackerRecordPosition.size(), in.attackerRecordPosition.begin());
        in.attackerPosition = P;
        HitOutcome out = resolveHit(target, body.velocity, &bomb.fighter, in, [&] { return draw(); },
            [&](const HitEmission &emission) {
                const HitRecord &record = emission.record;
                frame.emittedHits.push_back(record);
                if (!emission.applyLocally) {
                    frame.seen.push_back({record, bomb.body.blocks()});
                    return;
                }
                frame.seen.push_back({record, body.blocks()});
                applyWorldHit(record, players, frame);
            });
        frame.buffs.insert(frame.buffs.end(), out.buffs.begin(), out.buffs.end());
        frame.sounds.insert(frame.sounds.end(), out.sounds.begin(), out.sounds.end());
        frame.bursts.insert(frame.bursts.end(), out.bursts.begin(), out.bursts.end());
    };
    for (PlayerView &p : players)
        hit(p.fighter, p.body, p.body.size);
    for (Npc &n : list)
        hit(n.fighter, n.body, n.looks.box);
}

// Schedules §3.2: the last entry (in list order) whose time has come, else the first.
int32_t currentStop(const std::vector<std::pair<glm::dvec3, int32_t>> &schedule, double clockMs) {
    int32_t current = 0;
    for (int32_t k = 0; k < (int32_t)schedule.size(); k++)
        if (schedule[(size_t)k].second <= clockMs)
            current = k;
    return current;
}

bool Npcs::spawnLocation(Npc &n, const BlockType &block) {
    if (n.schedule.empty())
        return false;
    const int32_t current = currentStop(n.schedule, clockMs);
    const auto solid = [&](int32_t x, int32_t y, int32_t z) { return block(x, y, z) != 0 && block(x, y, z) != 2; };
    if (current != n.scheduleStop) { // §3.3 step 3: a new stop
        n.path.clear();
        n.scheduleStop = current;
        glm::dvec3 G = n.schedule[(size_t)current].first;
        const int32_t x = (int32_t)std::floor(G.x), y = (int32_t)std::floor(G.y);
        int k = 0;
        while (k < 11 && solid(x, y, (int32_t)std::floor(G.z)))
            G.z += 1, k++;
        if (solid(x, y, (int32_t)std::floor(G.z)))
            return false;
        bool ground = false;
        for (k = 0; k < 11 && !ground; k++) {
            if (solid(x, y, (int32_t)std::floor(G.z) - 1))
                ground = true;
            else
                G.z -= 1;
        }
        if (!ground)
            return false;
        n.path.start(n.body.blocks() + glm::dvec3(0, 0, 0.5 - n.body.size.z * 0.5), G, 4, n.body.size);
        n.path.round(n.body.size, block);
        n.body.intent = glm::vec3(0);
        return true;
    }
    if (n.scheduleArrived == n.scheduleStop)
        return false;
    if (n.path.searching())
        n.path.rounds(n.body.size, 4, block);
    const bool arrived = n.path.empty() || (n.path.state.path.back() == search::coordinate(n.path.state.goal) && n.path.state.path.size() <= 2);
    if (!arrived)
        return true; // walking (the follower moves it)
    n.path.clear();
    n.body.intent = glm::vec3(0);
    n.scheduleArrived = n.scheduleStop;
    n.home = n.schedule[(size_t)n.scheduleStop].first; // its random walk now keeps near the stop
    return true;
}

bool Npcs::randomInteraction(Npc &n, float dtMs, const BlockType &block) {
    if (!seats || !seatFree || !sit)
        return false;
    Combatant &c = n.fighter;
    const bool seated = c.action == 83 || c.action == 84;
    // §4.3 steps 1-3: sit out the timer, then stand up and cool down for 20 s.
    n.interactMs = std::max(n.interactMs - dtMs, 0.f);
    if (n.interactMs == 0) {
        if (seated) {
            n.path.clear();
            n.seat.index = -1;
            c.action = 0, c.actionMs = 0;
            n.body.intent = {0, 0, 1}; // (also ends the sitting in the physics step)
            n.interactMs = 20000;
            return false;
        }
        c.action = 0;
    }
    if (seated)
        return true;
    // Steps 4-5 (outdoors, §4.4): keep a free target still among the seats round the
    // creature, else draw one in sight while the timer is out.
    const glm::dvec3 centre = n.body.blocks();
    const Seats near = seats(glm::ivec3(glm::floor(centre)), clockMs);
    const auto same = [&](const Seat &s) { return s.zx == n.seat.zx && s.zy == n.seat.zy && s.index == n.seat.index; };
    const bool keep = n.seat.index >= 0 && std::any_of(near.keep.begin(), near.keep.end(), same) && seatFree(n.seat);
    if (!keep) {
        std::vector<Seat> candidates;
        if (n.interactMs == 0)
            for (const Seat &s : near.pick)
                if (near.house || sight(centre, s.at, block, sightRecords)) // outdoors only those in sight
                    candidates.push_back(s);
        const Seat *picked = candidates.empty() ? nullptr : &candidates[(uint32_t)draw() % candidates.size()];
        if (!picked || !seatFree(*picked)) {
            n.path.clear();
            n.seat.index = -1;
            return false;
        }
        n.interactMs = 20000;
        n.seat = *picked;
        n.path.state.reference = {picked->zx, picked->zy, picked->index}; // (it never blocks the way to it)
        // The goal: in a house the seat's own position; outdoors fitted to the ground block by
        // block.
        glm::dvec3 goal = picked->at;
        const auto solid = [&](glm::dvec3 p) {
            const uint8_t t = block((int32_t)std::floor(p.x), (int32_t)std::floor(p.y), (int32_t)std::floor(p.z)) & 31;
            return t != 0 && t != 2;
        };
        for (int k = 0; !near.house && k < 64 && solid(goal); k++) // (no limit in Alpha)
            goal.z += 1;
        for (int k = 0; !near.house && k < 64 && !solid(goal - glm::dvec3(0, 0, 1)); k++)
            goal.z -= 1;
        n.path.start(centre + glm::dvec3(0, 0, 0.5 - n.body.size.z * 0.5), goal, 2, n.body.size);
    }
    // Steps 6-7: walk, and take the seat within 3 blocks of the creature's centre.
    if (n.path.searching())
        n.path.rounds(n.body.size, 2, block);
    const glm::dvec3 d = centre - n.seat.at;
    if (glm::dot(d, d) < 9) {
        sit(n, n.seat);
        n.path.clear();
        n.seat.index = -1;
        n.body.intent = glm::vec3(0);
        n.body.velocity = glm::vec3(0);
        n.interactMs = 20000;
    }
    return true;
}

bool Npcs::walkPath(Npc &n, float dtMs, const BlockType &block) {
    if (n.route.empty())
        return false;
    n.waitMs = std::max(0.f, n.waitMs - dtMs);
    if (n.waitMs > 0)
        return true;
    n.routeIndex %= n.route.size();
    n.body.intent = glm::vec3(0);
    const float h = n.body.size.z;
    // Fit the point to the ground: down while free, then up while colliding (21 tests each).
    glm::dvec3 T = n.route[n.routeIndex];
    const auto collides = [&](glm::dvec3 at) {
        const glm::dvec3 centre = at + glm::dvec3(0, 0, h * 0.5);
        for (int32_t x = (int32_t)std::floor(centre.x - n.body.size.x * 0.5); x <= (int32_t)std::floor(centre.x + n.body.size.x * 0.5); x++)
            for (int32_t y = (int32_t)std::floor(centre.y - n.body.size.y * 0.5); y <= (int32_t)std::floor(centre.y + n.body.size.y * 0.5); y++)
                for (int32_t z = (int32_t)std::floor(centre.z - h * 0.5); z <= (int32_t)std::floor(centre.z + h * 0.5); z++)
                    if (block(x, y, z) != 0 && block(x, y, z) != 2)
                        return true;
        return false;
    };
    int k = 0;
    while (k < 21 && !collides(T))
        T.z -= 1, k++;
    if (k == 21) {
        n.routeIndex++;
        return false;
    }
    k = 0;
    while (k < 21 && collides(T))
        T.z += 1, k++;
    if (k == 21) {
        n.routeIndex++;
        return false;
    }
    const glm::dvec3 feet = n.body.blocks() - glm::dvec3(0, 0, h * 0.5);
    if (glm::length(feet - T) < n.routeRadius) { // arrived: stand 2-6 s, then the next point
        n.path.clear();
        n.waitMs = 2000.f + (float)(draw() % 4000);
        n.routeIndex++;
        return false;
    }
    if (!n.path.searching()) {
        n.path.start(n.body.blocks() + glm::dvec3(0, 0, 0.1 - h * 0.5), T, n.routeRadius, n.body.size);
        n.path.round(n.body.size, block);
    }
    n.path.rounds(n.body.size, n.routeRadius, block);
    n.body.mode &= ~RUN;
    return true;
}

bool Npcs::hostileInContact(const Combatant &fighter, const Body &body) const {
    for (const Npc &o : list)
        if (o.fighter.health > 0 && hostile(fighter, o.fighter) && inContact(body.size, body.blocks(), o.body.size, o.body.blocks()))
            return true;
    return false;
}

glm::vec3 Npcs::pushOnPlayer(const Body &player) const {
    glm::vec3 total(0);
    for (const Npc &o : list)
        if (o.fighter.health > 0 && (o.fighter.controller == 1 || o.fighter.controller == 6)) {
            const glm::vec3 grown = o.body.size + (o.fighter.controller == 6 ? glm::vec3(0) : glm::vec3(1, 1, 0));
            total += separation(player.size, grown, player.blocks(), o.body.blocks(), false);
        }
    return total;
}

void Npcs::removeTargets(const std::set<std::pair<int32_t, int32_t>> &zones) {
    std::erase_if(list, [&](const Npc &n) { return (n.looks.flags & 0x2000) && zones.count(n.zone); });
}

// §13.1: the sequence Combat, LookAtPlayer, RandomWalk; the first that handles the moment
// wins (which of them report "handled" is open, A-1: here Combat with a target, LookAtPlayer
// with a player near, RandomWalk always).
void Npcs::think(Npc &npc, float dtMs, std::vector<PlayerView> &players, const BlockType &block,
                 NpcFrame &frame, const worldgen::WorldMap &map) {
    if (npc.fighter.controller == 6)
        return;
    if (npc.fighter.controller == 5) { // pets §10: Combat, then Companion
        if (!combat(npc, dtMs, players, block, frame))
            companion(npc, npc.fighter.ownerId, players, dtMs, block);
        return;
    }
    if (npc.behaviour == 7) { // created root: retry Combat, Companion, Wander in that order
        if (!combat(npc, dtMs, players, block, frame) && !createdCompanion(npc, players, int32_t(dtMs), block, map)) {
            npc.fighter.mode = npc.body.mode;
            created::wander(npc.fighter, npc.body, npc.fighter.behaviorReference, npc.createdWalkMs,
                            int32_t(dtMs), [&] { return draw(); });
            npc.body.mode = npc.fighter.mode;
        }
        return;
    }
    // Wildlife §4.9: the cell monster, the group leader and its members.
    if (npc.behaviour == 1) {
        if (!combat(npc, dtMs, players, block, frame))
            randomWalk(npc, dtMs);
        return;
    }
    if (npc.behaviour == 5) { // a location's ring creature: Combat, WalkPath, RandomInteraction
        if (!combat(npc, dtMs, players, block, frame) && !walkPath(npc, dtMs, block) && !randomInteraction(npc, dtMs, block))
            randomWalk(npc, dtMs);
        return;
    }
    if (npc.behaviour == 6) { // a location's group leader: Combat, WalkPath, RandomWalk
        if (!combat(npc, dtMs, players, block, frame) && !walkPath(npc, dtMs, block))
            randomWalk(npc, dtMs);
        return;
    }
    if (npc.behaviour == 4) { // a shopkeeper: Combat, LookAtPlayer, WalkPath to its counter
        if (!combat(npc, dtMs, players, block, frame) && !lookAtPlayer(npc, players))
            walkPath(npc, dtMs, block);
        return;
    }
    if (npc.behaviour == 2 || npc.behaviour == 3) {
        if (combat(npc, dtMs, players, block, frame) || lookAtPlayer(npc, players))
            return;
        if (npc.behaviour == 3 && companion(npc, npc.follows, players, dtMs, block))
            return;
        if (!walkPath(npc, dtMs, block))
            randomWalk(npc, dtMs);
        return;
    }
    if (combat(npc, dtMs, players, block, frame) || lookAtPlayer(npc, players) || spawnLocation(npc, block))
        return;
    if (npc.fighter.characterClass == 0x88 && randomInteraction(npc, dtMs, block)) // trails spec §3.6
        return;
    const uint8_t cls = npc.fighter.characterClass;
    if ((cls >= 0x80 && cls <= 0x87) || (npc.looks.flags & 0x40)) {
        npc.body.intent = glm::vec3(0);
        return;
    }
    randomWalk(npc, dtMs);
}

bool Npcs::createdCompanion(Npc &npc, std::span<PlayerView> players, int32_t step, const BlockType &block,
                            const worldgen::WorldMap &map) {
    created::CompanionServices services;
    services.lookup = [&](uint64_t key) { return lookupActor(key, players).body(); };
    services.maximumHealth = [&] { return maxHealth(npc.fighter); };
    services.directRoute = [&](const Body &from, const Body &to, float width) {
        const route::Dimensions size{from.size.x, from.size.y, from.size.z};
        const route::Subject subject{from.position, size, npc.fighter.appearanceFlags,
            [&](uintptr_t identity) { return npc.path.state.exclusions.contains(identity); }};
        route::Scene scene;
        scene.column = [&](int32_t x, int32_t y) -> std::optional<route::Column> {
            const auto *zone = map.zoneAt(x, y);
            if (!zone)
                return std::nullopt;
            const auto &column = zone->column(x, y);
            static_assert(sizeof(worldgen::Block) == 4);
            return route::Column{column.bottom, int32_t(column.blocks.size()), std::as_bytes(std::span(column.blocks))};
        };
        scene.spatial = [&](const route::Position &position, int axis, const route::Dimensions &direction, bool terrain) {
            return route::scanRecords(subject, position, axis, direction, terrain, routeRecords);
        };
        return route::direct(subject, to.position, width, scene);
    };
    services.voxel = [&](const created::Position &position) {
        const auto coordinate = [](int64_t value) {
            if (value < 0)
                value = std::bit_cast<int64_t>(uint64_t(value) - uint64_t(65536));
            return std::bit_cast<int32_t>(uint32_t(value / 65536));
        };
        const int32_t z = coordinate(position[2]);
        const int32_t y = coordinate(position[1]);
        const int32_t x = coordinate(position[0]);
        if (x < 0 || y < 0 || x >= 0x1000000 || y >= 0x1000000)
            return uint8_t(1);
        return block(x, y, z);
    };
    services.clearPath = [&] { npc.path.clear(); };
    services.cellPresent = [&](int32_t x, int32_t y) {
        return x >= 0 && y >= 0 && x < 65536 && y < 65536 && map.zone(x, y) != nullptr;
    };
    services.initializePath = [&] { npc.path.setup(npc.body.size); };
    services.pathStep = [&] {
        npc.path.round(npc.body.size, block);
    };
    services.pathStatus = [&] {
        created::PathStatus status{npc.path.nodeCount(), int32_t(npc.path.state.path.size()), {}};
        if (!npc.path.empty())
            status.tail = npc.path.state.path.back();
        return status;
    };
    npc.fighter.mode = npc.body.mode;
    const bool handled = created::companion(npc.fighter, npc.body, npc.follows, step, npc.path.state, services);
    npc.body.mode = npc.fighter.mode;
    return handled;
}

// §13.2
bool Npcs::combat(Npc &npc, float dtMs, std::vector<PlayerView> &players, const BlockType &block, NpcFrame &frame) {
    Combatant &c = npc.fighter;
    selection::Services services;
    services.lookup = [&](uint64_t key) {
        const ActorEntry entry = lookupActor(key, players);
        return selection::Actor{entry.combat(), entry.body()};
    };
    services.maximumHealth = [&] { return maxHealth(c); };
    services.duration = [&] { return duration(c, uint8_t(c.action)); };
    services.timing = [&](selection::Timing query, int32_t action) {
        // Existing engine phase tables supply these distinct native query sites;
        // complete table/stat equivalence remains a separate dependency.
        const Phases p = phases(c, action < 0 ? uint8_t(c.action) : action);
        switch (query) {
        case selection::Timing::Recovery: return p.recovery;
        case selection::Timing::Anticipation: return p.windUp;
        case selection::Timing::Active: return p.active;
        }
        return int32_t(0);
    };
    services.firstPotion = [&] {
        if (auto *entry = npc.inventory.firstHealthItem())
            return selection::Potion{&entry->count, &entry->item};
        return selection::Potion{};
    };
    services.draw = [&] { return draw(); };
    services.power = [](double exponent) { return std::pow(1.5, exponent); };
    services.related = hostile;
    services.classified = [](const Combatant &actor) { return harmless(actor.type, actor.appearanceFlags); };
    services.visible = [&](selection::Actor from, selection::Actor to) {
        const visibility::Position start{from.body->position[0], from.body->position[1], from.body->position[2]};
        const visibility::Position end{to.body->position[0], to.body->position[1], to.body->position[2]};
        return visibility::visible(start, end, {block, sightRecords});
    };
    services.acquired = [&](selection::Actor target) { propagateThreat(*target.fighter, {&npc, nullptr}, frame); };
    services.extensionAdmission = [&](selection::Actor target) {
        // Required engine quests: allies fight hostile non-animal creatures.
        return npc.questRole == 3 && target.fighter->controller == 1 &&
               !harmless(target.fighter->type, target.fighter->appearanceFlags);
    };
    selection::TargetSelection selector{{&c, &npc.body},
        {npc.specialMs, npc.chaseMs, npc.cooldownMs, npc.targetRange}, int32_t(dtMs), services};
    selector.begin();
    if (selector.scanning())
        for (const auto &[key, entry] : actors)
            selector.consider({entry.combat(), entry.body()});
    const selection::Result selected = selector.finish();
    npc.body.mode = c.mode; // publish prefix writes to this phase's movement view
    if (selected.outcome != selection::Outcome::ContinueWithTarget) {
        if (selected.outcome == selection::Outcome::ReturnFalse)
            npc.target = 0, npc.threat = 0;
        return selected.outcome == selection::Outcome::ReturnTrue;
    }
    const selection::Actor target = selected.target;
    npc.target = target.fighter->id; // legacy continuation cache, not a selection sentinel
    const auto threat = c.relations.threats.find(npc.target);
    npc.threat = threat == c.relations.threats.end() ? 0 : threat->second;
    const bool ownedPet = c.controller == 5;
    const bool pacified = c.hasBuff(8);
    const auto tempting = [&](const Combatant &other) {
        return uint8_t(other.action) == 82 && other.equipment[12].type == 20 &&
               int32_t(other.equipment[12].subtype) == c.type;
    };
    const Phases p = phases(c, c.action);
    const glm::dvec3 self = npc.body.blocks();
    const glm::dvec3 there = target.body->blocks();
    const glm::dvec3 toTarget = there - self;
    const bool seen = sight(self, there, block, sightRecords);
    // Open points 4 §2: the range, each rule over the last. A special is rolled afresh on every
    // update while its timer is out; it is kept only if the creature is in range for that roll.
    const float own = npc.looks.box.x, theirs = target.body->size.x;
    int32_t special = 0;
    if (npc.specialMs <= 0 && !c.specials.empty())
        special = c.specials[(uint32_t)draw() % c.specials.size()];
    float R = 2.f * own + theirs;
    const bool windingUp = (c.action == 86 || c.action == 87 || c.action == 89) && c.actionMs < (float)p.windUp;
    if (rangedUser(c) && seen && !windingUp)
        R = 50;
    if (c.type == 47 || (c.type >= 111 && c.type <= 113))
        R = 20;
    if (c.action == 95 || c.action == 28 || c.action == 108)
        R = 50;
    if (c.action == 72)
        R = own + theirs;
    if (special == 93 || special == 91 || special == 68 || special == 69)
        R = 20;
    else if (special == 86 || special == 89)
        R = 2.f * own + theirs;
    if (!seen)
        R = 1;
    const double distance = glm::length(toTarget);

    if (distance > R || !seen) {
        // Chase, for up to 20 s. Giving up changes neither the action nor the intent, so the
        // walk toward the target goes on until the next behaviour sets another (§8.3).
        npc.chaseMs = std::bit_cast<int32_t>(uint32_t(npc.chaseMs) + uint32_t(int32_t(dtMs)));
        if (npc.chaseMs > 20000) {
            npc.target = 0;
            npc.threat = 0;
            npc.fighter.relations.threats.clear();
            npc.path.clear();
            npc.cooldownMs = 3000;
            npc.chaseMs = 0;
            return false;
        }
        if (c.action == 95 || c.action == 28)
            c.action = 0; // (beams end; anything else runs on while it walks)
        // Path search §3: with the target in sight and the route clear (and not stuck) the
        // chaser walks straight; otherwise it searches toward the target's feet.
        const bool clear = seen && !(npc.body.contact & STUCK) &&
                           clearRoute(npc.body.size, npc.body.swimmer, self, there, R, block, objects);
        if (clear) {
            npc.path.clear();
        } else {
            const glm::dvec3 feet = self - glm::dvec3(0, 0, npc.body.size.z * 0.5 - 0.1);
            const glm::dvec3 goal = there - glm::dvec3(0, 0, target.body->size.z * 0.5 - 0.1);
            if (npc.path.empty() && !npc.path.searching() && (target.body->contact & 7))
                npc.path.start(feet, goal, npc.body.size.x + target.body->size.x, npc.body.size);
            if (feet != goal)
                npc.path.rounds(npc.body.size, npc.body.size.x, block);
            npc.threat *= 0.9f;
            if (const auto it = npc.fighter.relations.threats.find(npc.target); it != npc.fighter.relations.threats.end())
                it->second *= 0.9f;
        }
        glm::vec3 u = glm::vec3(toTarget);
        if (!(npc.body.contact & IN_WATER))
            u.z = 0;
        if (glm::dot(u, u) > 0)
            u = glm::normalize(u);
        npc.body.intent = (R <= 0.5f ? 80.f : 40.f) * u;
        npc.yaw = yawToward(glm::vec2(toTarget));
        return true;
    }

    // In range: it faces the target and aims at it (flag 0x04, field 26).
    npc.chaseMs = 0;
    npc.yaw = yawToward(glm::vec2(toTarget));
    npc.body.intent = glm::vec3(0);
    // Pets §7.3: fed by a player holding out its food: up to five bites (action 110), then, if
    // still not tamed, pacified for 20 s.
    if (!pacified && !ownedPet && tempting(*target.fighter)) {
        if (actionOver(c)) {
            if (c.action != 110)
                npc.bites = 0;
            if (npc.bites < 5) {
                c.action = 110;
                c.actionMs = 0;
            } else {
                npc.tameProgress = 0;
                npc.tameMs = 0;
                c.buffs.push_back({8, 0, 20000});
                npc.target = 0;
            }
        }
        return true;
    }
    npc.body.mode |= AIMING;
    npc.body.look = glm::vec3(toTarget);
    const auto start = [&](int32_t action) {
        c.action = action;
        c.actionMs = 0;
        c.hitLanded = false;
    };
    // A special interrupts whatever runs.
    if (special != 0) {
        npc.specialMs = 20000;
        start(special);
        return true;
    }
    // Open points 3 §1: the action pools of some big creatures. A new action is drawn only when
    // the last has ended; until then they face the target and wait (they never dodge or use
    // the weapon rules).
    const bool golem = c.type == 47 || (c.type >= 111 && c.type <= 113);
    if (c.type == 116 || c.type == 118 || c.type == 119 || golem) {
        if (!actionOver(c))
            return true;
        std::vector<int32_t> pool;
        if (c.type == 116) {
            pool = {69, 74, 73};
        } else if (c.type == 118) { // the laser head beams (95), any other head stomps (86)
            pool = {7, 6, 20, npc.looks.model(Part::Head) == 2154 ? 95 : 86};
        } else if (golem) {
            pool = {108, 54, 76};
        } else { // §1.6: the saurian's moves come from its parts
            const uint16_t head = npc.looks.model(Part::Head), hand = npc.looks.model(Part::Hand);
            const uint16_t body = npc.looks.model(Part::Body), tail = npc.looks.model(Part::Tail);
            pool.push_back(head >= 2147 && head <= 2149 ? 69 : 68);
            if (tail == 2151 || tail == 2152)
                pool.push_back(70);
            pool.push_back(hand == 2143 || hand == 2144 ? 75 : 74);
            if (body == 2140 || body == 2141)
                pool.push_back(77), pool.push_back(78);
            if (body >= 2135 && body <= 2139)
                pool.push_back(76);
            const bool curled = c.action == 71 || c.action == 72;
            if (!curled && body == 2133 && (draw() & 1))
                pool.push_back(71);
            if (!curled && body == 2134 && (draw() & 1))
                pool.push_back(72);
            if (c.action != 73)
                pool.push_back(73);
        }
        start(pool[(uint32_t)draw() % pool.size()]);
        if (golem && c.action == 54) { // the smash leaps at the target
            npc.body.velocity = 2.f * glm::vec3(toTarget);
            npc.body.velocity.z = 15;
        }
        return true;
    }
    // §1.7: a dragon (no magic user) casts between 8 and 60 blocks; closer it leaps or lunges.
    if (c.type == 101 && !isMagicUser(c)) {
        const double d2 = glm::dot(toTarget, toTarget);
        if (d2 >= 64 && d2 < 3600) {
            if (actionOver(c))
                start(37);
            return true;
        }
        if (d2 < 64) {
            npc.body.mode |= AIMING;
            if (c.actionMs > 2000) {
                if ((draw() & 1) && self.z - there.z <= 8) {
                    npc.body.velocity.z = 30;
                } else {
                    start(51);
                    if (glm::dot(toTarget, toTarget) > 0)
                        npc.body.velocity = 10.f * glm::normalize(glm::vec3(toTarget));
                }
            }
            return true;
        }
    }
    // Magic users within 60 blocks: the primary attack at low mana or on an even draw, else
    // the secondary.
    if (isMagicUser(c) && distance < 60) {
        if (c.mana <= 0.5f || draw() % 2 == 0)
            startPrimaryAttack(c);
        else
            startSecondaryAttack(c);
        return true;
    }
    // Open points 4 §5.1: claws and jaws together hop. A hop needs only 2 s of the current action,
    // not its end, and every update after that draws again until it lunges.
    const double distance2 = glm::dot(toTarget, toTarget);
    const uint16_t natural = npc.looks.flags;
    if ((natural & 0x02) && (natural & 0x10)) {
        if (distance2 >= 64) {
            npc.body.mode &= ~AIMING;
            return true;
        }
        if (c.actionMs > 2000) {
            if ((draw() & 1) && self.z - there.z <= 2) {
                npc.body.velocity.z = 5;
            } else {
                start(51);
                if (distance2 > 0)
                    npc.body.velocity = 10.f * glm::normalize(glm::vec3(toTarget));
            }
        }
        return true;
    }

    // Open points 4 §6.1: the dodge, tried after its one-in-ten draw. False when it cannot be
    // made; the update then goes on to the attack.
    const auto dodge = [&] {
        if (bigCreature(c.type) || own >= 2 || c.actionMs < (float)duration(c, c.action) ||
            !(npc.body.contact & (ON_GROUND | IN_WATER)))
            return false;
        const glm::vec2 e(toTarget);
        if (glm::dot(e, e) == 0)
            return true;
        const glm::vec2 u = glm::normalize(e);
        const glm::vec2 right(u.y, -u.x);
        const glm::vec2 ways[4] = {u, -u, right, -right};
        npc.body.velocity = glm::vec3(20.f * ways[(uint32_t)draw() % 4], 5);
        npc.body.dodgeMs = 600;
        c.dodgeMs = 600;
        return true;
    };
    // §7.1: release a charge once it has reached the whole of the mana, or start it.
    // (The step takes the charge as the new action starts, as it does for a player.)
    const auto release = [&](int32_t action, int32_t atMs) {
        start(action);
        c.actionMs = atMs;
    };
    const auto charging = [&](int32_t action) { return c.action == action && c.mana <= c.charge; };
    const auto charge = [&](int32_t action, int32_t released) {
        if (charging(action))
            release(released, 0);
        else if (c.action != action)
            start(action);
    };
    const auto jump = [&] {
        npc.body.velocity.z += 20;
        start(54);
    };

    if (shooter(c)) {
        // §6.2: the ranged rule. (Beyond 60 blocks nothing happens; R keeps them nearer.)
        if (distance2 >= 3600)
            return true;
        const bool held = c.action == 24 || c.action == 25 || c.action == 27;
        if (!held && (uint32_t)draw() % 10 == 0 && dodge())
            return true;
        if (c.mana < 0.8f && !held) {
            startPrimaryAttack(c);
            return true;
        }
        if (actionOver(c)) {
            const bool armed = c.mainHand().type != 0;
            const int m = c.mainHand().subtype;
            if (armed && m == CROSSBOW)
                charge(24, 22);
            else if (armed && m == BOOMERANG)
                charge(27, 26);
            else if (m == BOW || c.type == 104)
                charge(25, 55);
        }
        return true;
    }
    // §5.3: jaws alone bite (a turtle shells when it can), when the last action has ended.
    if (natural & 0x10) {
        if (actionOver(c))
            start(c.type == 25 && usableNow(c, 72) ? 72 : 75);
        return true;
    }
    // §5.4: claws alone strike.
    if (natural & 0x08) {
        startPrimaryAttack(c);
        return true;
    }
    // §6.1: melee.
    const float reach = 3.f * (own + theirs);
    const bool held = c.action == 59 || c.action == 63 || c.action == 8;
    if (distance2 < (double)reach * reach && !held && (uint32_t)draw() % 10 == 0 && dodge())
        return true;
    if (c.mana < 0.8f && !held) {
        startPrimaryAttack(c);
        return true;
    }
    if (!actionOver(c))
        return true;
    // §7.1: the mana ability of the weapons held. Those who charge jump first while they can.
    const bool main = c.mainHand().type != 0, off = c.offHand().type != 0;
    const int m = c.mainHand().subtype, o = c.offHand().subtype;
    if (m == LONGSWORD) {
        start(5);
    } else if (main && !off) {
        if (charging(59))
            release(c.specialization == 1 ? 61 : 60, c.specialization == 1 ? 0.f : (float)phases(c, 60).windUp);
        else if (usableNow(c, 54))
            jump();
        else if (c.action != 59)
            start(59);
    } else if (o == SHIELD) {
        charge(8, 104);
    } else if (o == DAGGER) {
        start(17);
    } else if (o == FIST) {
        start(20);
    } else if (!main) {
        if (usableNow(c, 54))
            jump();
        else
            startSecondaryAttack(c);
    } else if (charging(63)) {
        release(11, (float)phases(c, 11).windUp);
    } else if (usableNow(c, 54)) {
        jump();
    } else if (c.action != 63) {
        start(63);
    }
    return true;
}

// §13.3: face the nearest living player within 8 blocks.
bool Npcs::lookAtPlayer(Npc &npc, std::vector<PlayerView> &players) {
    const PlayerView *nearest = nullptr;
    double best = 64;
    for (const PlayerView &p : players) {
        if (p.fighter.health < 0)
            continue;
        const glm::dvec3 d = p.body.blocks() - npc.body.blocks();
        if (glm::dot(d, d) < best)
            best = glm::dot(d, d), nearest = &p;
    }
    if (!nearest)
        return false;
    if (npc.fighter.action == 83 || npc.fighter.action == 84)
        return false;
    npc.yaw = yawToward(glm::vec2(nearest->body.blocks() - npc.body.blocks()));
    npc.body.mode |= AIMING;
    npc.body.look = glm::vec3(nearest->body.blocks() - npc.body.blocks());
    npc.body.intent = glm::vec3(0);
    npc.path.clear(); // (wildlife §4.5: it resumes with a fresh search)
    return true;
}

// §13.4
void Npcs::randomWalk(Npc &npc, float dtMs) {
    npc.body.mode &= ~0x40u;
    npc.walkMs -= dtMs;
    if (npc.walkMs > 0)
        return;
    const glm::dvec3 self = npc.body.blocks();
    const glm::vec2 home = glm::vec2(npc.home - self);
    if (glm::dot(home, home) <= 100) {
        // A random direction at strength 10. (The spec reads the two draws as the intent
        // itself; Alpha's recorded wanderers walk at the one speed a length of 10 gives,
        // 3.9 blocks/s, half of the time and hardly ever faster, so the direction is scaled.)
        const float a = (float)draw() / 32767.f, b = (float)draw() / 32767.f;
        const glm::vec2 direction(2 * a - 1, 2 * b - 1);
        npc.body.intent = glm::dot(direction, direction) > 0 ? glm::vec3(10.f * glm::normalize(direction), 0)
                                                             : glm::vec3(0);
    } else {
        npc.body.intent = glm::vec3(10.f * glm::normalize(home), 0);
    }
    npc.walkMs = 3000.f + (float)(draw() % 5000);
    npc.fighter.action = 0;
    npc.fighter.actionMs = 1000000;
}

void Npcs::propagateThreat(Combatant &source, const ActorEntry &recipient, NpcFrame &frame) {
    assert(actorLookupActive);
    Combatant &target = *recipient.combat();
    const float current = target.relations.threats[source.id];
    frame.relationships.push_back({target.id, source.id, current});
    for (const auto &[key, actor] : actors) {
        Combatant *candidate = actor.combat();
        if (!candidate || candidate == &source || candidate == &target || !hostile(*candidate, source))
            continue;
        float distance[3];
        for (size_t axis = 0; axis < 3; axis++) {
            const uint64_t difference = uint64_t(actor.body()->position[axis]) - uint64_t(recipient.body()->position[axis]);
            distance[axis] = (float)std::bit_cast<int64_t>(difference) * (1.f / 65536.f);
        }
        const float x2 = distance[0] * distance[0];
        const float y2 = distance[1] * distance[1];
        const float z2 = distance[2] * distance[2];
        const float squared = (y2 + x2) + z2;
        if (!(squared < 64.f))
            continue;
        if (candidate->relations.threats[source.id] == 0) {
            candidate->relations.threats[source.id] = .5f;
            frame.relationships.push_back({candidate->id, source.id, candidate->relations.threats[source.id]});
        }
    }
}

bool Npcs::protectionGranted(const Combatant &target, std::span<PlayerView> players) {
    if (!(target.appearanceFlags & 0x2000))
        return true;
    for (const auto &[key, value] : target.relations.threats)
        if (value > 0) {
            const Combatant *contributor = lookupActor(key, players).combat();
            if (contributor && contributor->controller == 0)
                return true;
        }
    return false;
}

void Npcs::applyWorldHit(const HitRecord &hit, std::span<PlayerView> players, NpcFrame &frame,
                         const std::function<void(HitRecord &)> &localHit) {
    if (!actorLookupActive)
        refreshActors(players);
    FlagScope indexScope{actorLookupActive};
    const ActorEntry recipient = lookupActor(hit.target, players);
    Combatant *target = recipient.combat();
    if (!target || !(target->health > 0))
        return;
    if (!controlledActor && target->controller == 0) {
        frame.playerHits.push_back(hit);
        return;
    }
    const ActorEntry source = lookupActor(hit.attacker, players);
    Combatant *by = source.combat();
    auto applied = hit;
    const bool local = target == controlledActor;
    if (local) {
        if (localHit)
            localHit(applied);
        else if (onControlledHit)
            onControlledHit(applied);
    }
    prepareHitApplication(*target, applied);
    if (by && applied.amount >= 0) {
        by->relations.contact = applied.target;
        target->relations.contact = applied.attacker;
        by->relations.counts[applied.target] += 1.f;
        float &value = target->relations.threats[applied.attacker];
        value = hitThreat(*by, applied.amount) + value;
        propagateThreat(*by, recipient, frame);
        // Preserve the existing engine AI acquisition adapter independently
        // from the exact contact identifier and resident relationship maps.
        if (recipient.npc && !recipient.npc->target) {
            recipient.npc->target = applied.attacker;
            recipient.npc->threat = 1;
        }
    }
    const auto phase = applyPreparedHit(*target, *recipient.body(), applied,
        {by, local, false, protectionGranted(*target, players)}, &frame.buffs);
    if (phase != HitApplicationPhase::health)
        return;
    // These engine world calls use mode zero. Missing-source negative input
    // retains an explicit engine omission policy for the native unsafe fanout.
    if (by && applied.amount < 0)
        for (const auto &[key, actor] : actors) {
            Combatant *candidate = actor.combat();
            if (candidate && hostile(*by, *candidate) && candidate->relations.threats.contains(target->id)) {
                const float adjustment = applied.amount * .1f;
                candidate->relations.threats[by->id] -= adjustment;
                propagateThreat(*by, actor, frame);
            }
        }
    // Prepare once, update the region, then visit current recipients in signed
    // key order. Later corpse processing must not repeat these effects.
    if (target->health <= 0) {
        target->health = 0;
        glm::vec3 position;
        for (int axis = 0; axis < 3; axis++)
            position[axis] = (float)recipient.body()->position[axis] * (1.f / 65536.f);
        frame.sounds.push_back({glm::dvec3(position), 27, 1, 1});
        target->actionMs = 0;
        if (recipient.npc)
            recipient.npc->fatalPresented = true;
        if (onFatalObject)
            onFatalObject(*target, frame);
        if (onFatalLoot) {
            const auto inventory = recipient.npc ? recipient.npc->inventory.entries() : std::vector<CountedItem>{};
            onFatalLoot(*target, *recipient.body(), inventory);
            if (recipient.npc)
                recipient.npc->fatalLootApplied = true;
        }
        // Zero is a sentinel at this specific fatal link boundary. This is a
        // direct store, without recursively generating another fatal sequence.
        if (target->linkedId && lookupActor(target->linkedId, players).combat())
            if (Combatant *linked = lookupActor(target->linkedId, players).combat())
                linked->health = 0;
        const float credit = fatalCreditAmount(target->level, target->powerTier, target->type, target->appearanceFlags);
        const size_t progressStart = frame.missions.size();
        if (onFatalRegion) {
            onFatalRegion(*target, recipient.npc ? recipient.npc->boss : 0,
                [&](uint64_t key) { return lookupActor(key, players).combat(); }, frame);
            if (recipient.npc)
                recipient.npc->fatalRegionApplied = true;
        }
        // The regional service emits at most one snapshot for this invocation.
        std::optional<MissionArea> completedMission;
        if (frame.missions.size() == progressStart + 1 && frame.missions.back().completed()) {
            completedMission = frame.missions.back().mission;
            frame.missions.back().rewardsApplied = true;
        }
        int32_t previousLevel = 0;
        FatalRecipientWorld fatalWorld;
        fatalWorld.visit = [&](const auto &visit) {
            for (const auto &[key, actor] : actors)
                visit(key, actor.combat());
        };
        fatalWorld.credit = {[&](uint64_t key) { return lookupActor(key, players).combat(); },
                             [&] { return controlledActor; }, 0, creditItems};
        fatalWorld.appendCredit = [&](const KillRecord &record) {
            frame.kills.push_back(record);
            previousLevel = controlledActor ? controlledActor->level : 0;
        };
        fatalWorld.consumed = [&](const KillRecord &record, const CreditResult &result) {
            if (controlledActor && (result.directAward == controlledActor || controlledActor->level != previousLevel))
                frame.appliedCredits.push_back({record, result.directAward == controlledActor,
                                               previousLevel, controlledActor->level});
        };
        fatalWorld.completionReward = [&](uint64_t key, Combatant &candidate) {
            const auto deliver = [&](const ItemRecord &item) {
                frame.grants.push_back({key, item});
                if (&candidate == controlledActor && onFatalItem)
                    onFatalItem(item);
            };
            ItemRecord platinum{};
            platinum[0] = 13;
            setItemLevel(platinum, std::bit_cast<int16_t>(uint16_t(
                rewardPlatinumAmount(completedMission->level, completedMission->tier))));
            deliver(platinum);
            // The first append/local inventory call completes before this draw.
            deliver(toRecord(missionRewardGear(*completedMission, candidate.characterClass,
                                               sharedRandom ? *sharedRandom : privateRandom)));
        };
        applyFatalRecipients(*target, applied.target, credit, completedMission.has_value(), fatalWorld);
        if (recipient.npc)
            recipient.npc->fatalRecipientsApplied = true;
        if (onFatalBacking) {
            onFatalBacking(*target);
            if (recipient.npc)
                recipient.npc->fatalBackingApplied = true;
        }
    }
}

// §5 for one target of a strike or a beam; true when the hit was not blocked or refused.
bool Npcs::landBlow(Combatant &attacker, const Body &attackerBody, Combatant &target, Body &body, NpcFrame &frame,
                    HitInput in, std::span<PlayerView> players) {
    const glm::dvec3 from = attackerBody.blocks();
    in.targetOnGround = (body.contact & ON_GROUND) != 0;
    in.targetPosition = body.blocks();
    std::copy_n(body.position, in.targetRecordPosition.size(), in.targetRecordPosition.begin());
    std::copy_n(attackerBody.position, in.attackerRecordPosition.size(), in.attackerRecordPosition.begin());
    in.attackerPosition = from;
    // Local application happens at each producer boundary. A shield event can
    // cancel an action or change a status gate before the main record is built.
    HitOutcome out = resolveHit(target, body.velocity, &attacker, in, [&] { return draw(); },
        [&](const HitEmission &emission) {
            const HitRecord &record = emission.record;
            frame.emittedHits.push_back(record);
            if (!emission.applyLocally) {
                frame.seen.push_back({record, from});
                for (const PlayerView &player : players)
                    if (&player.fighter == &attacker) {
                        frame.playerHits.push_back(emission);
                        break;
                    }
                return;
            }
            frame.seen.push_back({record, body.blocks()});
            applyWorldHit(record, players, frame);
        });
    frame.buffs.insert(frame.buffs.end(), out.buffs.begin(), out.buffs.end());
    frame.sounds.insert(frame.sounds.end(), out.sounds.begin(), out.sounds.end());
    frame.bursts.insert(frame.bursts.end(), out.bursts.begin(), out.bursts.end());
    // Projectiles §2.6: a healer's swing heals round itself once more for each creature it hits.
    if (attacker.characterClass == 3 && attacker.specialization == 1 && !out.records.empty())
        frame.shots.push_back(healingBurst(attacker, from));
    return out.hitAccepted;
}

void Npcs::beamTick(Combatant &attacker, const Body &attackerBody, float t, float dt, std::span<PlayerView> players,
                    NpcFrame &frame, const BlockType &block) {
    const glm::dvec3 from = attackerBody.blocks();
    if (attacker.action != 28 && attacker.action != 95)
        return;
    const float D = (float)phases(attacker, attacker.action).windUp;
    // (Open points 3 §2.3: every 200 ms, not 100.)
    if (!(t + dt > D) || std::trunc((t + dt - D) / 200) == std::trunc((t - D) / 200))
        return;
    glm::vec3 u = attacker.aim;
    if (glm::dot(u, u) > 0)
        u = glm::normalize(u);
    // The ray's length: to the first solid block, 200 at most.
    float length = 0;
    const auto solid = [&](glm::dvec3 p) {
        const uint8_t type = block((int32_t)std::floor(p.x), (int32_t)std::floor(p.y), (int32_t)std::floor(p.z)) & 31;
        return type != 0 && type != 2;
    };
    while (length < 200 && !solid(from + glm::dvec3(u * length)))
        length += 0.25f;
    std::set<uint64_t> touched;
    bool hit = false;
    for (int i = 0; (float)i < length; i++) {
        const glm::dvec3 p = from + glm::dvec3(u * (float)i);
        const auto test = [&](Combatant &c, Body &body) {
            if (c.id == attacker.id || c.health <= 0 || touched.count(c.id) || c.dodgeMs > 0)
                return;
            const glm::dvec3 d = body.blocks() - p;
            const double r = 0.5 * body.size.x + 1;
            if (d.x * d.x + d.y * d.y >= r * r || std::abs(d.z) >= 0.5 * body.size.z + 1)
                return;
            touched.insert(c.id); // (friend or foe: either stops the sparks)
            if (hostile(attacker, c) || hostile(c, attacker)) {
                landBlow(attacker, attackerBody, c, body, frame, beamInput(attacker, [&] { return draw(); }), players);
                hit = true;
            }
        };
        for (PlayerView &v : players)
            test(v.fighter, v.body);
        for (Npc &n : list)
            test(n.fighter, n.body);
    }
    // Open points 3 §2.2: the channel sound on the 200-ms grid of the action's own time: 37 when
    // this tick hit an enemy, else 36.
    if (std::trunc((t + dt) / 200) != std::trunc(t / 200))
        frame.sounds.push_back({from, hit ? 37 : 36, 1 + 0.25f * (float)draw() / 32767.f, 1});
    if (touched.empty()) {
        Burst b = Burst::at(from + glm::dvec3(u * length), -u, {1, 0.2f, 0.5f, 1}, 0.1f, 5, FIRE);
        frame.bursts.push_back(b);
        attacker.combo = 0;
    } else if (hit) {
        attacker.combo += 1;
    }
}

void Npcs::strike(Combatant &attacker, const Body &attackerBody, float yaw, std::span<PlayerView> players,
                  bool byPlayer, NpcFrame &frame, const BlockType &block, uint64_t npcTarget, bool attackerAlly) {
    const glm::dvec3 from = attackerBody.blocks();
    const glm::vec3 box = attackerBody.size;
    // Open points 6 §1.5: every creature in reach, each once per swing or pulse.
    auto hit = [&](Combatant &target, Body &body) {
        if (std::find(attacker.struck.begin(), attacker.struck.end(), target.id) != attacker.struck.end())
            return;
        glm::dvec3 sightFrom;
        if (!reachesTarget(attacker, from, yaw, box, body.blocks(), body.size, &sightFrom) ||
            !sight(sightFrom, body.blocks(), block, sightRecords) || target.dodgeMs > 0)
            return;
        const bool first = attacker.struck.empty();
        attacker.struck.push_back(target.id);
        attacker.hitLanded = true;
        const auto rolls = [&] { return draw(); };
        const HitInput in = strikeInput(attacker, attackerBody, body, rolls);
        if (landBlow(attacker, attackerBody, target, body, frame, in, players) && first)
            afterFirstTarget(attacker, in.area, rolls);
    };
    if (byPlayer) {
        for (Npc &n : list)
            if (n.fighter.controller != 6 && n.fighter.health > 0)
                hit(n.fighter, n.body);
    } else {
        for (PlayerView &p : players)
            if (p.fighter.health > 0)
                hit(p.fighter, p.body);
        // Quests: a battle's two sides strike each other.
        if (npcTarget)
            for (Npc &n : list)
                if (n.fighter.id == npcTarget && n.fighter.health > 0 && (attackerAlly || n.questRole == 3))
                    hit(n.fighter, n.body);
    }
}

void Npcs::periodicHit(Combatant &target, Body &body, const Buff &buff, std::span<PlayerView> players, NpcFrame &frame,
                      const std::function<void(HitRecord &)> &localHit) {
    if (!actorLookupActive)
        refreshActors(players);
    FlagScope indexScope{actorLookupActive};
    // Exact lookup includes zero, dead sources and self references. A missing
    // source has its own producer draw/amount path; never substitute the target.
    const ActorEntry source = lookupActor(buff.source, players);
    // The engine stores physics contact separately; present its current value
    // to both producer gates, including the role-one stuck-contact response.
    target.contact = body.contact;
    HitInput input;
    input.amount = buff.value;
    input.magic = true;
    input.blockable = false;
    input.targetOnGround = (body.contact & ON_GROUND) != 0;
    input.targetPosition = body.blocks();
    std::copy_n(body.position, input.targetRecordPosition.size(), input.targetRecordPosition.begin());
    if (const Body *sourceBody = source.body()) {
        input.attackerPosition = sourceBody->blocks();
        std::copy_n(sourceBody->position, input.attackerRecordPosition.size(), input.attackerRecordPosition.begin());
    }
    // Selector stays zero even when the source is still performing action 17.
    // No critical, affix, ordinary presentation, base stun or initial impulse.
    const HitOutcome out = resolveHit(target, body.velocity, source.combat(), input, [&] { return draw(); },
        [&](const HitEmission &emission) {
            const HitRecord &record = emission.record;
            frame.emittedHits.push_back(record);
            const ActorEntry recipient = lookupActor(record.target, players);
            if (!recipient.combat() || !(recipient.combat()->health > 0))
                return;
            frame.seen.push_back({record, recipient.body()->blocks()});
            applyWorldHit(record, players, frame, localHit);
        });
    frame.buffs.insert(frame.buffs.end(), out.buffs.begin(), out.buffs.end());
    frame.sounds.insert(frame.sounds.end(), out.sounds.begin(), out.sounds.end());
    frame.bursts.insert(frame.bursts.end(), out.bursts.begin(), out.bursts.end());
}

void Npcs::dispatchPlayerActions(std::span<PlayerView> players, int32_t dtMs, const BlockType &block, NpcFrame &frame) {
    if (actorLookupActive)
        for (PlayerView &player : players)
            if (player.strikes && player.fighter.health > 0)
                createActor({nullptr, &player}, dtMs);
    for (PlayerView &p : players)
        if (p.strikes && p.fighter.health > 0 && meleeAction(p.fighter.action) &&
            strikeWindow(p.fighter, p.actionMsBefore, (float)dtMs))
            strike(p.fighter, p.body, p.yaw, players, true, frame, block);
    for (PlayerView &p : players)
        if (p.strikes && p.fighter.health > 0)
            beamTick(p.fighter, p.body, p.actionMsBefore, (float)dtMs, players, frame, block);
}

NpcFrame Npcs::step(float dtMs, const worldgen::WorldMap &map, std::vector<PlayerView> players, double timeOfDayMs,
                    int32_t day, const BlockType &block, const std::vector<HitRecord> &incoming, NpcFrame frame,
                    const std::vector<BuffEvent> &buffInput) {
    stepInto(dtMs, map, std::move(players), timeOfDayMs, day, block, frame, incoming, buffInput);
    return frame;
}

void Npcs::stepInto(float dtMs, const worldgen::WorldMap &map, std::vector<PlayerView> players, double timeOfDayMs,
                    int32_t day, const BlockType &block, NpcFrame &frame, const std::vector<HitRecord> &incoming,
                    const std::vector<BuffEvent> &buffInput,
                    const std::function<void(int32_t)> &advanceControlled) {
    leftoverMs += dtMs;
    const int32_t milliseconds = (int32_t)leftoverMs;
    leftoverMs -= (float)milliseconds;
    dtMs = (float)milliseconds;
    clockMs = timeOfDayMs;
    refreshActors(players);
    FlagScope indexScope{actorLookupActive};
    // Server §3 step 3: hits sent by players' clients land on the NPCs they name.
    for (const HitRecord &hit : incoming) {
        const ActorEntry recipient = lookupActor(hit.target, players);
        if (recipient.npc && recipient.combat()->health > 0) {
            const glm::dvec3 at = recipient.body()->blocks();
            applyWorldHit(hit, players, frame);
            frame.seen.push_back({hit, at});
        }
    }
    // Input hits precede buff application; resident countdown is later in this
    // same update. Neither source identity nor target health gates a buff record.
    for (const BuffEvent &event : buffInput) {
        if (Npc *target = findNpc(event.target))
            applyBuff(target->fighter, event.record);
        else
            for (PlayerView &player : players)
                if (player.fighter.id == event.target) {
                    applyBuff(player.fighter, event.record);
                    break;
                }
    }
    for (PlayerView &player : players)
        if (player.advanceBuffs && player.fighter.health > 0)
            tickBuffs(player.fighter, milliseconds, true, [&](const Buff &buff) {
                periodicHit(player.fighter, player.body, buff, players, frame);
            });
    petPass(players, frame); // retained link maintenance precedes descriptor activation
    activate(map, players, timeOfDayMs, day, dtMs);
    // Creatures whose zone is let go go with it (inferred); pets stay with their owners.
    list.erase(std::remove_if(list.begin(), list.end(),
                              [&](const Npc &n) { return !map.zones().count(n.zone) && n.fighter.controller != 5; }),
               list.end());

    // Server §5.4: an NPC no player is fighting goes when every player is further than its
    // radius + 20 blocks; one a player has threat on stays while within 512 blocks of home.
    auto despawns = [&](const Npc &n) {
        if (n.fighter.health <= 0 || n.fighter.controller == 5) // (pets never despawn)
            return false;
        const glm::dvec3 h = n.body.blocks() - n.home;
        for (const PlayerView &p : players)
            if (const auto t = n.fighter.relations.threats.find(p.fighter.id);
                t != n.fighter.relations.threats.end() && t->second > 0 && glm::dot(h, h) < 262144.0)
                return false;
        double m = -1;
        for (const PlayerView &p : players) {
            const glm::dvec3 d = n.body.blocks() - p.body.blocks();
            m = m < 0 ? glm::dot(d, d) : std::min(m, glm::dot(d, d));
        }
        const double r = n.despawnRadius + 20.0;
        return m > r * r;
    };
    list.erase(std::remove_if(list.begin(), list.end(), despawns), list.end());
    // Quests: a freed prisoner is gone after its minute.
    for (Npc &n : list)
        if (n.leaveMs >= 0 && (n.leaveMs -= dtMs) < 0)
            questGone.insert(n.fighter.id);
    list.erase(std::remove_if(list.begin(), list.end(), [&](const Npc &n) { return n.quest && questGone.count(n.fighter.id); }),
               list.end());

    // Engine producers currently use an actor's ID as its entry key. Refresh
    // pointers after pre-pass removals while retaining nodes for surviving keys.
    // The live walk below uses these keys, never a mutable actor ID or role.
    refreshActors(players);

    struct ActorPassScope {
        bool &active;
        ~ActorPassScope() { active = false; }
    } actorPassScope{actorPassActive};
    actorPassActive = true;
    for (auto entry = actors.begin(); entry != actors.end(); ++entry) {
        const ActorEntry actor = entry->second;
        if (actor.player) {
            if (actor.player->local && advanceControlled)
                advanceControlled(milliseconds);
            // Library players dispatch at their own key; the actual client
            // dispatches inside its common update and supplies strikes=false.
            dispatchPlayerActions(std::span(actor.player, 1), milliseconds, block, frame);
            continue;
        }
        if (!actor.npc)
            continue;
        Npc &npc = *actor.npc;
        npc.path.records = routeRecords;
        npc.path.columns = routeColumns;
        npc.path.activate = routeActivate;
        Combatant &c = npc.fighter;
        if (!admitCreatureUpdate(c))
            continue;
        // The body is a phase-local view of the full actor mode word. Refresh
        // before behavior so received/copied flags survive movement edits.
        npc.body.mode = c.mode;
        npc.fatalPresented = false;
        npc.fatalLootApplied = false;
        npc.fatalRegionApplied = false;
        npc.fatalRecipientsApplied = false;
        npc.fatalBackingApplied = false;
        npc.tameMs = std::max(0.f, npc.tameMs - dtMs); // pets §7.3: the taming progress lasts 5 s
        // §13.1: decide every 32 ms of accumulated time while not stunned.
        npc.thinkMs += dtMs;
        // The native behavior gate observes the incoming stun. Expiring this
        // update permits later movement, but must not admit an early decision.
        const bool canThink = c.hitStun <= 0;
        if (canThink && npc.thinkMs >= 32) {
            think(npc, npc.thinkMs, players, block, frame, map);
            npc.thinkMs = 0;
        }
        // The authoritative path phase sees incoming stun, before lock/action
        // dispatch and the later common countdown.
        npc.body.mode &= ~0x80u;
        c.mode = npc.body.mode;
        // The admitted bomb body can reach zero here and still completes this
        // pass. Its health gate has already run, before behavior/steering.
        if (c.type == 144 && c.controller == 6)
            c.health = std::max(0.f, c.health - 0.025f * dtMs);
        if (!npc.path.empty() && c.hitStun <= 0) {
            npc.body.mode &= ~4u;
            if (npc.path.follow(npc.body, milliseconds, block)) {
                npc.path.state.rebuild();
            } else {
                if (!npc.path.empty()) {
                    npc.body.mode |= 4;
                    npc.pathFallback = npc.path.fallback(npc.body);
                }
                npc.body.mode |= 0x80;
            }
        }
        c.mode = npc.body.mode;
        c.dodgeMs = (float)npc.body.dodgeMs;
        beginActionUpdate(c, milliseconds, &frame.buffs);
        npc.body.dodgeMs = (int32_t)c.dodgeMs;
        const int32_t before = c.actionMs;
        // Projectiles §2: ranged actions charge and launch; the rest strike at their moment.
        for (const PlayerView &p : players)
            if (p.fighter.id == npc.target)
                c.aim = glm::vec3(p.body.blocks() - npc.body.blocks());
        createActor(actor, milliseconds);
        ShotEffects shot;
        chargeSound(c, before, milliseconds, npc.body.blocks(), shot);
        actionSounds(c, before, dtMs, npc.body.blocks(), [&] { return draw(); }, shot.sounds);
        const size_t launched = frame.shots.size();
        launch(c, npc.body, c.aim, before, dtMs, [&] { return draw(); }, frame.shots, shot, block);
        for (size_t k = launched; k < frame.shots.size(); k++) // §2.3: a boomerang has its thrower's weapon scale
            if (frame.shots[k].kind == BOOMERANG_SHOT)
                frame.shots[k].scale = npc.looks.weaponScale;
        frame.sounds.insert(frame.sounds.end(), shot.sounds.begin(), shot.sounds.end());
        frame.buffs.insert(frame.buffs.end(), shot.buffs.begin(), shot.buffs.end());
        frame.bursts.insert(frame.bursts.end(), shot.bursts.begin(), shot.bursts.end());
        if (c.action == 110 && hitMoment(c, before, dtMs)) {
            bite(npc, players, frame); // pets §7.3: a bite of the food held out
        } else if (c.action == 28 || c.action == 95) {
            beamTick(c, npc.body, before, dtMs, players, frame, block);
        } else if (meleeAction(c.action) && strikeWindow(c, before, dtMs)) {
            strike(c, npc.body, npc.yaw, players, false, frame, block, npc.target,
                   npc.questRole == 3);
        }
        if (c.action != 0 && !shootsProjectile(c.action) && c.action != 110 && hitMoment(c, before, dtMs)) {
            // Block edits §9: the impact point, in front of the creature by its reach.
            if (const int32_t radius = craterRadius(c.action, npc.looks.box.x)) {
                const float f = impactReach(c.action, npc.looks.box.x).second;
                const float yaw = glm::radians(npc.yaw);
                glm::dvec3 centre = npc.body.blocks() + glm::dvec3(-std::sin(yaw) * f, std::cos(yaw) * f, 0);
                if (c.action >= 30 && c.action <= 33)
                    centre = npc.body.blocks() + glm::dvec3(c.aim);
                if (c.action == 69 || c.action == 75)
                    centre.z += (npc.looks.box.z - npc.looks.box.x) / 2;
                if (c.action == 11 || c.action == 54 || c.action == 61)
                    centre = npc.body.blocks();
                frame.craters.push_back({centre, radius});
            }
        }
        endActionUpdate(c, milliseconds, {false, false}, [&](const Buff &buff) {
            periodicHit(c, npc.body, buff, players, frame);
        });
        npc.body.mode = c.mode;
        // Physics §4.4: overlapping creatures push each other apart (not bombs, nor creatures
        // in actions 83 and 84); boxes grow by (1, 1, 0) except inert ones.
        if (c.controller != 6 && c.action != 83 && c.action != 84) {
            npc.body.separate = glm::vec3(0);
            const auto push = [&](const Body &o, uint8_t kind) {
                const glm::vec3 grown = o.size + (kind == 6 ? glm::vec3(0) : glm::vec3(1, 1, 0));
                npc.body.separate += separation(npc.body.size, grown, npc.body.blocks(), o.blocks(), c.hitStun > 0);
            };
            for (const Npc &o : list)
                if (&o != &npc && o.fighter.health > 0)
                    push(o.body, o.fighter.controller);
            for (const PlayerView &p : players)
                if (p.fighter.health > 0)
                    push(p.body, 0);
        }
        // Physics §3.2, §3.3: what the speed limit and the action motions read.
        {
            const Phases timing = phases(c, c.action);
            Body &b = npc.body;
            b.action = c.action, b.actionMs = c.actionMs, b.windUpMs = (float)timing.windUp, b.activeMs = (float)timing.active;
            b.durationMs = (float)duration(c, c.action);
            b.controller = c.controller, b.type = c.type, b.appearanceFlags = npc.looks.flags;
            b.stunMs = c.hitStun;
            b.yaw = npc.yaw;
            b.haste = c.hasBuff(12) ? 1 + skillFactor(c.level / 2 + c.powerTier + 1) : 1.f;
            b.slowed = c.slowMs > 0, b.hastened = c.hasteMs > 0;
            b.targetInContact = false;
            if (lunges(c.action, c.actionMs)) {
                for (const PlayerView &p : players)
                    b.targetInContact = b.targetInContact ||
                                        (p.fighter.health > 0 && hostile(c, p.fighter) &&
                                         inContact(b.size, b.blocks(), p.body.size, p.body.blocks()));
                b.targetInContact = b.targetInContact || hostileInContact(c, b);
                // Found, or past 500 ms: the intercept ends (and a pending 47 becomes the smash).
                if (c.action == 48 && (b.targetInContact || c.actionMs > 500)) {
                    const uint8_t m = c.mainHand().type == 3 ? c.mainHand().subtype : 0;
                    c.action = m == LONGSWORD ? 5 : m == DAGGER ? 17 : m == FIST ? 20 : 0;
                    c.actionMs = 0;
                    c.mana = 1; // (field 28 := 1, as read)
                }
            }
        }
        npc.body.ignoredStatics = npc.path.searching() ? &npc.path.ignored : nullptr; // (path search §4.1 step 3)
        npc.body.pathActive = !npc.path.empty();
        worldStep(npc.body, (int32_t)dtMs, block, objects);
        npc.path.state.counter = std::bit_cast<int32_t>(uint32_t(npc.path.state.counter) + uint32_t(milliseconds));
        c.mode = npc.body.mode;
        advanceCharge(c, dtMs);
        npc.body.mode = c.mode;
        if (std::exchange(npc.body.splashed, false)) { // physics §5.3
            frame.bursts.push_back(splashBurst(npc.body.blocks()));
            frame.sounds.push_back({npc.body.blocks(), 31, 0.9f + (float)draw() / 32767.f, 1});
        }
        if (c.action == 72)
            npc.yaw = npc.body.yaw;
        if (glm::dot(glm::vec2(npc.body.intent), glm::vec2(npc.body.intent)) > 0 && npc.target == 0)
            npc.yaw = yawToward(glm::vec2(npc.body.intent));
        // Travel §5.4: a pet under its riding owner stands where the owner stands, 0.01 higher
        // than their shared floor, turned as the owner.
        if (c.controller == 5)
            for (const PlayerView &p : players)
                if (p.fighter.id == npc.fighter.ownerId && p.fighter.action == 106) {
                    npc.body.place(p.body.blocks() + glm::dvec3(0, 0, 0.5 * npc.body.size.z - 0.5 * p.body.size.z + 0.01));
                    npc.body.velocity = npc.body.intent = glm::vec3(0);
                    npc.yaw = p.yaw;
                }
    }

    actorPassActive = false;
    actorLookupActive = false; // later corpse removal can move deque elements
    // §9.4, §12.3, server §6.2: deaths. At the moment of death the spawn's countdown starts and
    // every player in the threat map is credited (hostile creatures only); the body is
    // removed 1000 ms later.
    for (auto it = list.begin(); it != list.end();) {
        Npc &n = *it;
        if (n.fighter.health > 0) {
            ++it;
            continue;
        }
        // Pets §11.1: a dead pet lies until 20 s have passed and its owner is here and not
        // dying, then comes back to its owner, healed and calm.
        if (n.fighter.controller == 5) {
            if (n.deadMs == 0) {
                n.fighter.health = 0;
                if (!n.fatalPresented)
                    frame.sounds.push_back({n.body.blocks(), 27, 1, 1}); // non-hit engine death adapter
                for (Npc &o : list)
                    if (o.target == n.fighter.id)
                        o.target = 0, o.threat = 0;
            }
            n.deadMs += std::max(dtMs, 0.001f);
            for (const PlayerView &p : players)
                if (p.fighter.id == n.fighter.ownerId && n.deadMs >= 20000 && p.fighter.health >= 0) {
                    n.fighter.health = maxHealth(n.fighter);
                    n.body.place(p.body.blocks());
                    n.target = 0;
                    n.fighter.relations.threats.clear();
                    p.fighter.relations.counts.clear();
                    n.deadMs = 0;
                }
            ++it;
            continue;
        }
        if (n.deadMs == 0) {
            n.fighter.health = 0;
            if (!n.fatalBackingApplied && n.spawn != Npc::NO_SPAWN) {
                SpawnState &state = spawns[{n.zone.first, n.zone.second, n.spawn}];
                state.countdownMs = 1200000;
                state.deathDay = day;
            }
            if (n.fighter.type == 144 && n.fighter.controller == 6) {
                frame.explosions.push_back({n.body.blocks(), n.fighter.id, n.fighter.level});
                blast(n, players, frame);
            }
            if (!n.fatalPresented)
                frame.sounds.push_back({n.body.blocks(), 27, 1, 1}); // non-hit engine death adapter
            NpcFrame::Death death{n.fighter, n.looks.flags, {}, n.inventory.entries(), n.zone, n.boss};
            death.quest = n.quest, death.questRole = n.questRole;
            death.lootApplied = n.fatalLootApplied;
            death.regionApplied = n.fatalRegionApplied;
            if (n.quest)
                questGone.insert(n.fighter.id); // a quest creature is not made again
            std::copy(n.body.position, n.body.position + 3, death.position);
            for (const auto &[who, threat] : n.fighter.relations.threats)
                if (threat > 0 && !death.credited)
                    for (const PlayerView &p : players)
                        if (p.fighter.id == who)
                            death.credited = who;
            if (n.fighter.controller != 6) // (a bomb leaves nothing)
                frame.deaths.push_back(death);
            if (!n.fatalRecipientsApplied && n.fighter.controller == 1) {
                const int32_t xp = killExperience(n.fighter.level, n.fighter.powerTier, n.fighter.type,
                                                 (n.looks.flags & 0x200) != 0, n.looks.flags);
                for (size_t k = 0; k < players.size(); k++)
                    if (n.fighter.relations.threats.count(players[k].fighter.id) || n.target == players[k].fighter.id) {
                        frame.kills.push_back({players[k].fighter.id, n.fighter.id, n.fighter.type, xp});
                        if (k == 0)
                            frame.experience = std::bit_cast<int32_t>(uint32_t(frame.experience) + uint32_t(xp));
                        sharePetExperience(players[k].fighter, xp, frame); // pets §12
                    }
            }
            for (Npc &o : list) // creatures that targeted it forget it
                if (o.target == n.fighter.id)
                    o.target = 0, o.threat = 0;
        }
        n.deadMs += std::max(dtMs, 0.001f);
        n.body.intent = glm::vec3(0);
        worldStep(n.body, (int32_t)dtMs, block, objects);
        it = n.deadMs >= 1000 ? list.erase(it) : std::next(it);
    }
}

NpcFrame::Added NpcFrame::addedSince(Cursor &cursor) const {
    const auto take = []<class T>(const std::vector<T> &records, size_t &position) {
        assert(position <= records.size());
        const auto added = std::span<const T>(records).subspan(position);
        position = records.size();
        return added;
    };
    Added added{take(emittedHits, cursor.emittedHits), take(playerHits, cursor.playerHits),
                take(sounds, cursor.sounds), take(buffs, cursor.buffs), take(kills, cursor.kills),
                take(shots, cursor.shots), take(bursts, cursor.bursts), take(deaths, cursor.deaths),
                take(craters, cursor.craters), take(explosions, cursor.explosions), take(tamed, cursor.tamed),
                take(petItems, cursor.petItems), std::bit_cast<int32_t>(uint32_t(experience) - uint32_t(cursor.experience)),
                take(seen, cursor.seen), take(relationships, cursor.relationships), take(staticStates, cursor.staticStates),
                take(missions, cursor.missions), take(grants, cursor.grants), take(appliedCredits, cursor.appliedCredits)};
    cursor.experience = experience;
    return added;
}

void NpcFrame::reset() {
    emittedHits.clear();
    playerHits.clear();
    sounds.clear();
    buffs.clear();
    relationships.clear();
    staticStates.clear();
    missions.clear();
    grants.clear();
    appliedCredits.clear();
    kills.clear();
    shots.clear();
    bursts.clear();
    deaths.clear();
    craters.clear();
    explosions.clear();
    tamed.clear();
    petItems.clear();
    seen.clear();
    experience = 0;
}

} // namespace creatures
