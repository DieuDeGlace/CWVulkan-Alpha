#include "creatures/Npcs.h"
#include "Missions.h"
#include "worldgen/Items.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

using namespace worldgen;

namespace {

template <class T> void put(std::vector<uint8_t> &out, T v) {
    const size_t at = out.size();
    out.resize(at + sizeof v);
    std::memcpy(out.data() + at, &v, sizeof v);
}
template <class T> T get(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}

// §0: the level curve c(x).
float curve(float x) { return 1.f - 1.f / ((x - 1.f) * 0.05f + 1.f); }

// §2.4: "take a random entry": one draw, and the entry leaves the list.
void take(std::vector<std::pair<int32_t, int32_t>> &list, Random &random, std::pair<int32_t, int32_t> &out) {
    const size_t i = (size_t)random.draw() % list.size();
    out = list[i];
    list.erase(list.begin() + (ptrdiff_t)i);
}

int64_t fixed(double blocks) { return (int64_t)(blocks * 65536.0); }

} // namespace

std::array<uint8_t, 56> MissionArea::record(int32_t cx, int32_t cy) const {
    std::array<uint8_t, 56> r{};
    std::memcpy(&r[0], &cx, 4);
    std::memcpy(&r[4], &cy, 4);
    std::memcpy(&r[16], &unused, 4);
    std::memcpy(&r[20], &seed, 4);
    std::memcpy(&r[24], &kind, 4);
    std::memcpy(&r[28], &targetType, 4);
    std::memcpy(&r[32], &level, 4);
    r[36] = tier, r[37] = state;
    std::memcpy(&r[40], &count, 4);
    std::memcpy(&r[44], &required, 4);
    std::memcpy(&r[48], &targetZoneX, 4);
    std::memcpy(&r[52], &targetZoneY, 4);
    return r;
}

MissionArea MissionArea::fromRecord(const uint8_t *p, int32_t &cx, int32_t &cy) {
    cx = get<int32_t>(p, 0), cy = get<int32_t>(p, 4);
    MissionArea m;
    m.unused = get<uint32_t>(p, 16);
    m.seed = get<uint32_t>(p, 20);
    m.kind = get<int32_t>(p, 24);
    m.targetType = get<int32_t>(p, 28);
    m.level = get<int32_t>(p, 32);
    m.tier = p[36], m.state = p[37];
    m.count = get<int32_t>(p, 40);
    m.required = get<int32_t>(p, 44);
    m.targetZoneX = get<int32_t>(p, 48);
    m.targetZoneY = get<int32_t>(p, 52);
    return m;
}

std::vector<uint8_t> MissionArea::save() const {
    std::vector<uint8_t> out;
    put<uint32_t>(out, 1);
    put(out, seed), put(out, kind), put(out, targetType), put(out, level);
    put(out, tier), put(out, state);
    put(out, count), put(out, required), put(out, targetZoneX), put(out, targetZoneY);
    return out;
}

std::optional<MissionArea> MissionArea::parse(const std::vector<uint8_t> &v) {
    if (v.size() < 38 || get<uint32_t>(v.data(), 0) != 1)
        return std::nullopt;
    MissionArea m;
    m.seed = get<uint32_t>(v.data(), 4);
    m.kind = get<int32_t>(v.data(), 8);
    m.targetType = get<int32_t>(v.data(), 12);
    m.level = get<int32_t>(v.data(), 16);
    m.tier = v[20], m.state = v[21];
    m.count = get<int32_t>(v.data(), 22);
    m.required = get<int32_t>(v.data(), 26);
    m.targetZoneX = get<int32_t>(v.data(), 30);
    m.targetZoneY = get<int32_t>(v.data(), 34);
    return m;
}

std::vector<uint8_t> MonsterArea::save() const {
    std::vector<uint8_t> out;
    put<uint32_t>(out, 1);
    put(out, type), put(out, level), put(out, tier), put(out, zoneX), put(out, zoneY);
    return out;
}

std::optional<MonsterArea> MonsterArea::parse(const std::vector<uint8_t> &v) {
    if (v.size() < 21 || get<uint32_t>(v.data(), 0) != 1)
        return std::nullopt;
    MonsterArea m;
    m.type = get<int32_t>(v.data(), 4);
    m.level = get<int32_t>(v.data(), 8);
    m.tier = v[12];
    m.zoneX = get<int32_t>(v.data(), 13);
    m.zoneY = get<int32_t>(v.data(), 17);
    return m;
}

const char *objectiveKey(int32_t kind) {
    switch (kind) {
    case 1: return "monster";
    case 2: return "rareboss";
    case 3: return "villagemonster";
    case 4: return "villagerareboss";
    case 5: return "dungeon";
    case 7: return "invasion";
    case 8: return "sceneryinvasion";
    case 9: return "villageinvasion";
    case 10: return "caveinvasion";
    case 11: return "riverinvasion";
    case 12: return "gang";
    case 13: return "gangboss";
    default: return nullptr;
    }
}

int32_t monsterForLevel(const RegionCell *C, int32_t level, Random &random) {
    if (!C)
        return 108; // the fallback, without a draw
    std::vector<int32_t> list;
    list.push_back(C->warmth <= 0.2f ? 114 : 108); // yeti or troll
    list.push_back(C->warmth <= 0.2f ? 116 : 119); // mammoth or reptile monster
    if (level > 10)
        list.push_back(115); // cyclops
    if (level > 20)
        list.push_back(117); // lich
    if (level > 50) {
        list.push_back(C->warmth < 0.2f ? 113 : C->fire ? 112 : 111); // snow, ember or plain golem
        list.push_back(109);                                             // dark troll
    }
    if (level > 70)
        list.push_back(110); // hell demon
    return list[(size_t)random.draw() % list.size()];
}

int32_t missionCreature(Random &random) {
    static const int32_t types[9] = {48, 51, 96, 80, 76, 40, 43, 45, 52};
    return types[random.draw() % 9];
}

int32_t rewardPlatinumAmount(int32_t level, int32_t tier) {
    const int32_t combined = std::bit_cast<int32_t>(uint32_t(level) + uint32_t(tier));
    return (int32_t)(50.f * curve((float)combined) + 1.f);
}

ItemRecord missionRewardGear(const MissionArea &m, int32_t characterClass, Random &random) {
    const bool armour = random.draw() % 2 == 0;
    return armour ? armourItem(random, m.level, m.tier, characterClass)
                  : weaponItem(random, m.level, m.tier, characterClass);
}

std::vector<ItemRecord> missionRewards(const MissionArea &m, int32_t characterClass, Random &random) {
    ItemRecord platinum;
    platinum.type = 13;
    platinum.level = std::bit_cast<int16_t>(uint16_t(rewardPlatinumAmount(m.level, m.tier)));
    return {platinum, missionRewardGear(m, characterClass, random)};
}

const Location *cellLocation(const World &world, int32_t cx, int32_t cy) {
    const Region *region = world.region(floorDiv(cx, 8), floorDiv(cy, 8));
    return region ? &region->records[8 * (size_t)(cx & 7) + (size_t)(cy & 7)] : nullptr;
}

float locationBump(const World &world, const Location &L, double x, double y) {
    return bump(world.locationDistanceFx(L, fixed(x), fixed(y)));
}

void Missions::dailyPass(const World &world, int32_t rx, int32_t ry, const std::vector<Player> &players, Random &random) {
    const Region *region = world.region(rx, ry);
    if (!region || !world.named)
        return;
    const RegionCell *C = world.makeCell(rx, ry);
    int32_t pMax = 1;
    for (size_t k = 0; k < players.size(); k++)
        pMax = k == 0 ? players[k].level : std::max(pMax, players[k].level);
    std::vector<Key> A, B, M;
    // §2.3: the cell scan, x outer.
    for (int32_t i = 0; i < 8; i++)
        for (int32_t j = 0; j < 8; j++) {
            const int32_t cx = 8 * rx + i, cy = 8 * ry + j;
            const Location &L = region->records[8 * (size_t)i + (size_t)j];
            if (world.owner((int32_t)(L.x >> 16), (int32_t)(L.y >> 16)) != C)
                continue; // not this region's land
            Cell &c = cells[{cx, cy}];
            if (c.mission.kind != 0) {
                c.mission.kind = 0;
                c.mission.count = 0;
                changed.insert({cx, cy});
            }
            if (c.monster.type != 0)
                c.monster.type = c.monster.level = 0, c.monster.tier = 0;
            if (L.type == 0) {
                int32_t lo = 1, hi = 10;
                if (C->warmth < 0.2f)
                    lo = 10, hi = 20;
                if (C->moisture < 0.2f && C->warmth > 0.8f)
                    lo = 15, hi = 25;
                if (C->moisture > 0.8f && C->warmth > 0.8f)
                    lo = 10, hi = 20;
                if (C->fire)
                    lo = 20, hi = 30;
                c.monster.level = lo + random.draw() % (hi - lo + 1);
                c.monster.tier = 0;
                c.monster.type = monsterForLevel(C, c.monster.level, random);
                c.monster.zoneY = 8 * cy + 2 + random.draw() % 4;
                c.monster.zoneX = 8 * cx + 2 + random.draw() % 4;
                M.push_back({cx, cy});
            }
            if (L.type != 0 && L.type != 10 && L.level <= pMax + 2 && (L.type != 1 || random.draw() % 50 == 0))
                (L.level >= pMax - 2 ? A : B).push_back({cx, cy});
        }
    // §2.4: two location rounds, then up to two monster hunts.
    Key picked;
    for (int round = 0; round < 2; round++) {
        const int r = random.draw();
        if (r % 6 == 0 && !B.empty())
            take(B, random, picked);
        else if (!A.empty())
            take(A, random, picked);
        else if (!B.empty())
            take(B, random, picked);
        else
            continue;
        build(world, picked.first, picked.second, pMax, random);
    }
    for (int round = 0; round < 2 && !M.empty(); round++) {
        take(M, random, picked);
        build(world, picked.first, picked.second, pMax, random);
    }
    done.insert({rx, ry});
    passed.insert({rx, ry});
}

void Missions::build(const World &world, int32_t cx, int32_t cy, int32_t pMax, Random &random) {
    const Location *L = cellLocation(world, cx, cy);
    if (!L)
        return;
    Cell &c = cells[{cx, cy}];
    MissionArea &m = c.mission;
    m.kind = 0, m.state = 0, m.count = 0, m.required = 0;
    if (L->type == 10)
        return;
    // §2.5 step 3: the kind, by the location's type.
    int32_t kind;
    if (L->type == 1) {
        static const int32_t town[3] = {9, 3, 4};
        kind = town[random.draw() % 3];
    } else {
        random.draw(); // (a one-entry list still costs a draw)
        kind = L->type == 0 ? 1 : 5;
    }
    // Step 5: the target.
    if (kind == 1) {
        m.targetType = c.monster.type;
        m.targetZoneX = c.monster.zoneX, m.targetZoneY = c.monster.zoneY;
    } else {
        struct Weighted {
            float w;
            int32_t zx, zy;
        };
        std::vector<Weighted> zones;
        for (int32_t zx = 8 * cx; zx < 8 * cx + 8; zx++)
            for (int32_t zy = 8 * cy; zy < 8 * cy + 8; zy++) {
                const float w = locationBump(world, *L, 256.0 * zx + 128, 256.0 * zy + 128);
                if (w > 0)
                    zones.push_back({w, zx, zy});
            }
        if (zones.empty())
            return; // no mission for this cell
        std::stable_sort(zones.begin(), zones.end(), [](const Weighted &a, const Weighted &b) { return a.w > b.w; });
        m.targetZoneX = zones[0].zx, m.targetZoneY = zones[0].zy;
        if (kind == 3) {
            m.targetType = monsterForLevel(nullptr, pMax, random); // (always the troll, as read)
        } else if (kind == 4) { // the wildlife picker at the target zone's centre, L's height
            m.targetType = wildlife ? wildlife(256 * m.targetZoneX + 128, 256 * m.targetZoneY + 128, (int32_t)L->height, random)
                                    : missionCreature(random);
        } else {
            m.targetType = missionCreature(random);
            m.required = 10 + random.draw() % 10;
        }
    }
    // Step 6.
    m.seed = (uint32_t)random.draw();
    m.tier = (uint8_t)(L->subLevel & 0xff);
    m.level = pMax;
    m.kind = kind;
    changed.insert({cx, cy});
}

std::vector<Spawn> Missions::zonePass(const World &world, int32_t zx, int32_t zy,
                                      const std::function<float(int32_t, int32_t)> &ground,
                                      const std::vector<Player> &players, Random &random) const {
    std::vector<Spawn> out;
    const auto [cx, cy] = cellOfZone(zx, zy);
    const int32_t centreX = 256 * zx + 128, centreY = 256 * zy + 128;
    const float groundZ = ground(centreX, centreY);
    const auto at = [&](Spawn &s, double x, double y) {
        s.x = fixed(x), s.y = fixed(y), s.z = fixed(groundZ);
    };
    const Cell *c = cell(cx, cy);
    if (c && c->monster.type != 0 && c->monster.zoneX == zx && c->monster.zoneY == zy) {
        // §3.1: the cell's monster, a mission target only for a hunt (kind 1).
        Spawn s;
        s.controller = 1;
        at(s, centreX, centreY);
        s.type = c->monster.type;
        s.level = c->monster.level;
        s.powerTier = c->monster.tier;
        s.appearanceFlags = c->mission.kind == 1 ? 0x2000 : 0;
        s.behaviour = 1; // Combat, RandomWalk
        int32_t weaponRarity = c->monster.tier + 1;
        weaponRarity += random.draw() % 20 == 0;
        weaponRarity += random.draw() % 100 == 0;
        s.inventory.push_back(weaponItem(random, s.level, std::min(weaponRarity, 4), -1));
        int32_t armourRarity = c->monster.tier + 1;
        armourRarity += random.draw() % 100 == 0;
        armourRarity += random.draw() % 1000 == 0;
        s.inventory.push_back(armourItem(random, s.level, std::min(armourRarity, 4), -1));
        out.push_back(std::move(s));
        return out;
    }
    // §3.2: a wandering group, in plain wilderness away from home and from the players.
    const ZoneEntry *entry = world.zoneEntry(zx, zy);
    if (!entry || entry->kind != 0 || (zx == HOME / ZONE && zy == HOME / ZONE))
        return out;
    for (const Player &p : players) {
        const glm::dvec3 d = p.position - glm::dvec3(centreX, centreY, groundZ);
        if (glm::dot(d, d) < 10000.0)
            return out;
    }
    if (!world.owner(centreX, centreY))
        return out;
    int32_t pMin = 1, pMax = 1;
    for (size_t k = 0; k < players.size(); k++) {
        pMin = k == 0 ? players[k].level : std::min(pMin, players[k].level);
        pMax = k == 0 ? players[k].level : std::max(pMax, players[k].level);
    }
    const int32_t level = pMin + random.draw() % (pMax - pMin + 1);
    static const int32_t species[13] = {0, 9, 11, 4, 7, 15, 2, 13, 51, 48, 76, 45, 43};
    const int32_t first = species[random.draw() % 13];
    const auto variant = [&] {
        const bool paired = first == 0 || first == 2 || first == 4 || first == 7 || first == 9 || first == 11 ||
                            first == 13 || first == 15;
        return paired ? first + random.draw() % 2 : first;
    };
    const auto potions = [&](Spawn &s) {
        for (int n = random.draw() % 2; n > 0; n--) {
            ItemRecord potion;
            potion.type = 1, potion.subtype = 1, potion.level = (int16_t)level;
            s.inventory.push_back(potion);
        }
    };
    Spawn leader;
    leader.controller = random.draw() % 2 ? 1 : 3;
    at(leader, centreX, centreY);
    leader.type = variant();
    leader.level = level;
    // The leader's 20-point route, each point on the ground of its column.
    leader.behaviour = 2;
    for (int k = 0; k < 20; k++) {
        const int32_t x = 256 * zx + 16 + random.draw() % 224;
        const int32_t y = 256 * zy + 16 + random.draw() % 224;
        leader.route.push_back({x, y, (int32_t)ground(x, y)});
    }
    potions(leader);
    const float u = (float)random.draw() / 32767.f;
    const int32_t n = (int32_t)(u * u * 4);
    std::vector<Spawn> members;
    for (int32_t k = 1; k <= n; k++) {
        Spawn m;
        m.controller = leader.controller;
        m.x = leader.x, m.y = leader.y, m.z = leader.z;
        m.level = leader.level;
        m.powerTier = leader.powerTier;
        leader.x += (int64_t)(k % 2) << 16; // (the leader drifts, as read)
        leader.y += (int64_t)(k / 2) << 16;
        m.type = variant();
        m.behaviour = 3; // follows the leader (spawn index base + 0) and walks a copy of its route
        m.route = leader.route;
        m.leader = 0;
        potions(m);
        members.push_back(std::move(m));
    }
    out.push_back(std::move(leader));
    out.insert(out.end(), members.begin(), members.end());
    return out;
}

void Missions::discover(const World &world, glm::dvec3 p) {
    const int32_t cx = floorDiv((int32_t)std::floor(p.x), SUBCELL), cy = floorDiv((int32_t)std::floor(p.y), SUBCELL);
    const Location *L = cellLocation(world, cx, cy);
    const auto it = cells.find({cx, cy});
    if (!L || it == cells.end())
        return;
    MissionArea &m = it->second.mission;
    if (m.kind != 0 && m.state == 0 && locationBump(world, *L, p.x, p.y) > 0) {
        m.state = 1;
        changed.insert({cx, cy});
    }
}

std::optional<Missions::Key> Missions::kill(std::pair<int32_t, int32_t> homeZone, uint16_t flags, bool boss) {
    const Key key = cellOfZone(homeZone.first, homeZone.second);
    const auto it = cells.find(key);
    if (it == cells.end())
        return std::nullopt;
    MissionArea &m = it->second.mission;
    if (m.state == 2 || !((flags & 0x2000) || (m.kind == 5 && boss)))
        return std::nullopt;
    switch (m.kind) {
    case 1: case 2: case 3: case 4: case 5: case 13:
        m.state = 2;
        break;
    case 7: case 8: case 9: case 10: case 11: case 12:
        m.count = std::bit_cast<int32_t>(uint32_t(m.count) + 1u);
        m.state = m.count >= m.required ? 2 : 1;
        break;
    default:
        return std::nullopt;
    }
    changed.insert(key);
    return m.state == 2 ? std::optional<Key>(key) : std::nullopt;
}

std::optional<MissionProgress> Missions::fatal(const creatures::Combatant &target, uint8_t marker, uint8_t mode,
    const std::function<bool(int32_t, int32_t)> &regionAvailable,
    const std::function<creatures::Combatant *(uint64_t)> &lookup) {
    if (mode != 0)
        return std::nullopt;
    const int32_t x = target.backing.x / 8, y = target.backing.y / 8;
    if (x < 0 || y < 0 || x >= 8192 || y >= 8192 || !regionAvailable(x / 8, y / 8))
        return std::nullopt;
    const auto found = cells.find({x, y});
    if (found == cells.end())
        return std::nullopt; // absent engine storage is not an allocated native record
    auto &mission = found->second.mission;
    if (mission.state == 2 || (!(target.appearanceFlags & 0x2000) && !(mission.kind == 5 && marker != 0)))
        return std::nullopt;
    for (const auto &[key, value] : target.relations.threats) {
        if (!(value > 0) || !lookup(key))
            continue;
        const auto *contributor = lookup(key);
        if (!contributor || contributor->controller != 0)
            continue;
        switch (mission.kind) {
        case 1: case 2: case 3: case 4: case 5: case 13:
            mission.state = 2;
            break;
        case 7: case 8: case 9: case 10: case 11: case 12:
            mission.count = std::bit_cast<int32_t>(uint32_t(mission.count) + 1u);
            mission.state = 1;
            if (mission.count >= mission.required)
                mission.state = 2;
            break;
        default:
            return std::nullopt;
        }
        changed.insert({x, y});
        return MissionProgress{x, y, mission, key, target.controller};
    }
    return std::nullopt;
}

void visitMissionCells(const creatures::Combatant &target, const std::function<void(int32_t, int32_t)> &visit) {
    const auto upper = [](int32_t coordinate) {
        return std::bit_cast<int32_t>(uint32_t((coordinate / 8) * 8) + 8u);
    };
    for (int32_t x = (target.backing.x / 8) * 8; x < upper(target.backing.x); x++)
        for (int32_t y = (target.backing.y / 8) * 8; y < upper(target.backing.y); y++)
            if (x >= 0 && x < 65536 && y >= 0 && y < 65536)
                visit(x, y);
}

void Missions::appendUpdates(std::span<const MissionProgress> immediate, const std::set<Key> &changedCells,
    const std::set<Key> &requestedCells, std::vector<std::vector<uint8_t>> &output) const {
    std::map<Key, const MissionProgress *> latest;
    for (const auto &event : immediate) {
        const auto record = event.record();
        output.emplace_back(record.begin(), record.end());
        latest[{event.x, event.y}] = &event;
    }
    auto finalCells = changedCells;
    finalCells.insert(requestedCells.begin(), requestedCells.end());
    for (const auto &[x, y] : finalCells) {
        const auto *entry = cell(x, y);
        const auto record = (entry ? entry->mission : MissionArea{}).record(x, y);
        if (!requestedCells.contains({x, y})) {
            const auto previous = latest.find({x, y});
            if (previous != latest.end() && previous->second->record() == record)
                continue;
        }
        output.emplace_back(record.begin(), record.end());
    }
}

void Missions::wake(creatures::Npc &n, creatures::Npcs &npcs) const {
    const auto [cx, cy] = cellOfZone(n.zone.first, n.zone.second);
    const Cell *c = cell(cx, cy);
    if (!c || c->mission.kind != 5 || !(n.looks.flags & 0x1000) || n.fighter.controller == 6)
        return;
    n.fighter.level = c->mission.level;
    n.fighter.powerTier = c->mission.tier;
    npcs.reseed(c->mission.seed + (uint32_t)n.fighter.id);
    n.regear = true;
    if ((n.looks.flags & 0x200) && n.boss)
        n.looks.flags |= 0x2000;
}

const Missions::Cell *Missions::cell(int32_t cx, int32_t cy) const {
    const auto it = cells.find({cx, cy});
    return it == cells.end() ? nullptr : &it->second;
}

void Missions::apply(const uint8_t *record) {
    int32_t cx, cy;
    const MissionArea m = MissionArea::fromRecord(record, cx, cy);
    cells[{cx, cy}].mission = m;
}

void Missions::restore(int32_t cx, int32_t cy, const std::optional<MissionArea> &mission,
                       const std::optional<MonsterArea> &monster) {
    Cell &c = cells[{cx, cy}];
    if (mission) {
        c.mission = *mission;
        done.insert({floorDiv(cx, 8), floorDiv(cy, 8)});
    }
    if (monster)
        c.monster = *monster;
}
