// Checks creatures as they come alive against Alpha's, from a creature recording of the
// extractor mod (tools/alpha-extractor): for every creature that came from a zone's spawn (its id
// says which), this generates the zone, lets our creature system wake that spawn with a player
// beside it, and compares what Alpha made of it with what we make: type, level, tier, class,
// health, stat multipliers, appearance and equipment.
// Usage: CreatureCompare seed named|unnamed creatures.avcr
//        CreatureCompare seed named|unnamed --hits track.avct (our damage for a recorded fight)
//        CreatureCompare seed named|unnamed --simulate x y z   (our creatures round a player there
//        for 20 s, summed up like the extractor's tracking; add --runs N for N random streams; with
//        AV_SPEEDS set, the distribution of walking speeds too)
//
// The recording (.avcr): "AVCR", u32 version (1), u32 count, then per creature its u64 id and
// its 0x1168-byte creature record (alpha-net-packets.md §2.1).
#include "Config.h"
#include "CubModel.h"
#include "DataStore.h"
#include "Install.h"
#include "creatures/Npcs.h"
#include "creatures/Strike.h"
#include "net/Protocol.h"
#include "worldgen/Terrain.h"
#include "worldgen/WorldMap.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <iterator>
#include <map>
#include <memory>
#include <string>

using namespace worldgen;

namespace {

// The game's building models, as ZoneCompare reads them.
PrefabSource gameModels() {
    static Content content;
    static const bool opened = [] {
        Config config;
        if (!config.load("AlphaVulkan.ini"))
            config.load("config/AlphaVulkan.ini");
        return content.open(install::locate(config.text("Game", "Path")));
    }();
    if (!opened)
        return {};
    return [](const std::string &key) -> std::shared_ptr<const Prefab> {
        static std::map<std::string, std::shared_ptr<const Prefab>> cache;
        if (const auto it = cache.find(key); it != cache.end())
            return it->second;
        std::shared_ptr<Prefab> prefab;
        CubModel model;
        if (const auto bytes = content.read(key); bytes && model.load(bytes->data(), bytes->size())) {
            prefab = std::make_shared<Prefab>();
            prefab->sx = model.sx, prefab->sy = model.sy, prefab->sz = model.sz;
            prefab->rgb = model.rgb;
        }
        return cache[key] = prefab;
    };
}

std::string itemText(const creatures::Item &i) {
    char s[96];
    std::snprintf(s, sizeof s, "(%d/%d mat %d rar %d lvl %d seed %u)", i.type, i.subtype, i.material, i.rarity,
                  i.level, i.seed);
    return s;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 4) {
        std::printf("usage: CreatureCompare seed named|unnamed creatures.avcr\n");
        return 1;
    }
    World world((uint32_t)std::strtoul(argv[1], nullptr, 10));
    world.named = std::string(argv[2]) == "named";
    Terrain terrain(world);
    terrain.prefabs = gameModels();
    terrain.finishSpawn = creatures::finishSpawn;

    if (std::string(argv[3]) == "--hits" && argc >= 5) {
        // Our damage for the attackers of a recorded fight (track.avct): for each creature and
        // action, the base, what one unit of damage costs the player, and the range of a hit.
        std::ifstream track(argv[4], std::ios::binary);
        const std::vector<uint8_t> data((std::istreambuf_iterator<char>(track)), {});
        uint32_t samples = 0;
        std::memcpy(&samples, &data[8], 4);
        std::map<uint64_t, net::CreatureRecord> firstOf; // (creature << 8 | action): where it was first seen
        net::CreatureRecord player;
        size_t at = 12;
        for (uint32_t k = 0; k < samples && at + 12 <= data.size(); k++) {
            uint32_t count = 0;
            std::memcpy(&count, &data[at + 8], 4);
            at += 12;
            for (uint32_t n = 0; n < count; n++, at += 8 + net::CreatureRecord::SIZE) {
                uint64_t id;
                std::memcpy(&id, &data[at], 8);
                net::CreatureRecord record;
                std::memcpy(record.bytes.data(), &data[at + 8], record.bytes.size());
                const creatures::Combatant c = record.fighter();
                if (c.controller == 0 && k == 0)
                    player = record;
                if (c.controller != 0 && c.action != 0)
                    firstOf.try_emplace(id << 8 | (uint8_t)c.action, record);
            }
        }
        const creatures::Combatant me = player.fighter();
        std::mt19937 rng(1);
        const auto draw = [&] { return (int)(rng() % 32768); };
        std::printf("player: level %d health %.1f of %.1f armour %.1f resist %.1f\n", me.level, me.health,
                    creatures::maxHealth(me), creatures::armour(me), creatures::resistance(me));
        for (const auto &[key, record] : firstOf) {
            creatures::Combatant a = record.fighter();
            float low = 1e9f, high = 0;
            for (int n = 0; n < 2000; n++) {
                creatures::HitInput in =
                    creatures::strikeInput(a, {0, 0, 0}, {1, 1, 2}, {0, 1, 0}, {1, 1, 2}, draw);
                if (in.critical)
                    continue;
                creatures::Combatant target = me;
                target.health = 1e6f;
                glm::vec3 velocity{0};
                in.blockable = false;
                float lost = 0;
                for (const creatures::HitRecord &hit : creatures::resolveHit(target, velocity, &a, in, draw).records)
                    lost += hit.amount;
                low = std::min(low, lost), high = std::max(high, lost);
            }
            const creatures::Item &m = a.mainHand(), &o = a.offHand();
            std::printf("%llx type %d action %d: level %d tier %d class %d/%d mult2 %.3f flags %x charge %.2f spent %.2f "
                        "main %d/%d lv%d off %d/%d lv%d; base %.2f: a hit costs %.2f..%.2f\n",
                        (unsigned long long)(key >> 8), a.type, a.action, a.level, a.powerTier, a.characterClass,
                        a.specialization, a.multipliers[2], a.appearanceFlags, a.charge, a.spentPower, m.type, m.subtype,
                        m.level, o.type, o.subtype, o.level, creatures::strikeBase(a), low, high);
        }
        return 0;
    }
    if (std::string(argv[3]) == "--simulate" && argc >= 7) {
        // Our creatures round a player standing at (x, y, z) for 20 s of noon, sampled ten
        // times a second, summed up as tools/alpha-extractor's tracking is: path length, net
        // distance, speeds and the share of samples spent moving.
        const glm::dvec3 at(std::atof(argv[4]), std::atof(argv[5]), std::atof(argv[6]));
        WorldMap map;
        const int32_t pzx = (int32_t)std::floor(at.x / 256), pzy = (int32_t)std::floor(at.y / 256);
        for (int32_t i = -2; i <= 2; i++)
            for (int32_t j = -2; j <= 2; j++)
                map.add(std::shared_ptr<Zone>(terrain.generate(pzx + i, pzy + j)));
        const auto block = [&](int32_t x, int32_t y, int32_t z) -> uint8_t { return map.read(x, y, z).type(); };
        // `--runs N` after the position: N runs with different random streams, each summed up
        // in one line (the animals' mean speed while moving and mean path), to see how much
        // 20 s of wandering varies.
        const int runs = argc >= 9 && std::string(argv[7]) == "--runs" ? std::atoi(argv[8]) : 1;
        struct Track {
            int32_t type = 0;
            glm::dvec3 first{0}, last{0};
            double path = 0;
            float maxSpeed = 0, movingSum = 0;
            int samples = 0, moving = 0;
        };
        std::map<uint64_t, Track> tracks;
        std::map<int, int> speeds; // walking speeds in quarter blocks/s, all runs together
        for (int run = 0; run < runs; run++) {
        tracks.clear();
        creatures::Npcs npcs;
        npcs.reseed(12345u + 7919u * (uint32_t)run);
        creatures::Combatant me;
        me.id = 1;
        me.health = creatures::maxHealth(me);
        creatures::Body body;
        body.place(at);
        for (int frame = 0; frame <= 1250; frame++) { // 16 ms frames
            npcs.step(16, map, {{me, body, 0, me.actionMs}}, 43200000, 0, block);
            if (frame % 6 != 0)
                continue; // (about every 100 ms)
            for (const creatures::Npc &n : npcs.all()) {
                Track &t = tracks[n.fighter.id];
                const glm::dvec3 p = n.body.blocks();
                const float speed = glm::length(glm::vec2(n.body.velocity));
                if (t.samples == 0)
                    t.first = p, t.type = n.fighter.type;
                else
                    t.path += glm::length(glm::dvec2(p - t.last));
                t.last = p;
                t.samples++;
                t.maxSpeed = std::max(t.maxSpeed, speed);
                if (n.fighter.type < 120 && n.fighter.type != 55) // walkers on the ground (as the
                    speeds[(int)std::lround(speed * 4)]++;          // extractor notes compare them)
                if (speed > 0.05f)
                    t.moving++, t.movingSum += speed;
            }
        }
        if (runs > 1) {
            double path = 0, speed = 0;
            int animals = 0;
            for (const auto &[id, t] : tracks)
                if (t.moving > 0)
                    animals++, path += t.path, speed += t.movingSum / (float)t.moving;
            std::printf("run %2d: %d animals, mean path %.1f blocks, mean speed while moving %.2f\n", run, animals,
                        animals ? path / animals : 0.0, animals ? speed / animals : 0.0);
        }
        }
        if (std::getenv("AV_SPEEDS")) { // the distribution of walking speeds
            int all = 0;
            for (const auto &[k, n] : speeds)
                all += n;
            for (const auto &[k, n] : speeds)
                std::printf("%.2f %s %d\n", k / 4.0, std::string((size_t)(n * 60 / std::max(all, 1)), '#').c_str(), n);
        }
        if (runs > 1)
            return 0;
        for (int32_t i = -2; i <= 2; i++) // the spawns in reach, and whether they woke
            for (int32_t j = -2; j <= 2; j++)
                if (const Zone *z = map.zone(pzx + i, pzy + j))
                    for (size_t k = 0; k < z->spawns.size(); k++) {
                        const Spawn &s = z->spawns[k];
                        const glm::dvec3 p = glm::dvec3((double)s.x, (double)s.y, (double)s.z) / 65536.0;
                        const double d = glm::length(glm::dvec2(p - at));
                        const uint64_t id = creatures::Npcs::spawnId(pzx + i, pzy + j, k);
                        if (d < 250)
                            std::printf("spawn %llx type %3d at %.0f blocks, window %d-%d: %s\n", (unsigned long long)id,
                                        s.type, d, s.windowStart, s.windowEnd, tracks.count(id) ? "woke" : "asleep");
                    }
        for (const auto &[id, t] : tracks)
            std::printf("%llx type %3d: %d samples, path %.1f blocks, net %.1f; velocity max %.2f, mean while moving "
                        "%.2f (%d%% moving)\n",
                        (unsigned long long)id, t.type, t.samples, t.path, glm::length(glm::dvec2(t.last - t.first)),
                        t.maxSpeed, t.moving ? t.movingSum / (float)t.moving : 0.f, 100 * t.moving / std::max(t.samples, 1));
        return 0;
    }

    std::ifstream file(argv[3], std::ios::binary);
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
    uint32_t version = 0, count = 0;
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "AVCR", 4)) {
        std::printf("%s: not a creature recording\n", argv[3]);
        return 1;
    }
    std::memcpy(&version, &bytes[4], 4), std::memcpy(&count, &bytes[8], 4);

    std::map<std::pair<int32_t, int32_t>, std::shared_ptr<Zone>> zones;
    int compared = 0, same = 0;
    std::map<std::string, int> fields;
    for (uint32_t k = 0; k < count; k++) {
        const size_t at = 12 + (size_t)k * (8 + net::CreatureRecord::SIZE);
        if (at + 8 + net::CreatureRecord::SIZE > bytes.size())
            break;
        uint64_t id;
        std::memcpy(&id, &bytes[at], 8);
        net::CreatureRecord record;
        std::memcpy(record.bytes.data(), &bytes[at + 8], record.bytes.size());
        const creatures::Combatant theirs = record.fighter();
        if (theirs.controller == 0)
            continue; // a player
        // Server §5.3: the id of a spawn's creature is (zy * 65536 + zx) * 256 + index.
        const size_t index = id & 255;
        const int32_t zx = (int32_t)((id >> 8) & 0xffff), zy = (int32_t)((id >> 24) & 0xffff);
        auto &zone = zones[{zx, zy}];
        if (!zone)
            zone = std::shared_ptr<Zone>(terrain.generate(zx, zy));
        if (index >= zone->spawns.size()) {
            std::printf("  creature %llx: not from a spawn of zone (%d, %d)\n", (unsigned long long)id, zx, zy);
            continue;
        }
        const Spawn &spawn = zone->spawns[index];
        // Wake that one spawn: a world map with its zone alone, and a player on the spawn at
        // noon (inside every active window).
        WorldMap map;
        map.add(zone);
        creatures::Npcs npcs;
        creatures::Combatant me;
        me.id = 1;
        me.health = creatures::maxHealth(me);
        creatures::Body body;
        body.place(glm::dvec3((double)spawn.x, (double)spawn.y, (double)spawn.z) / 65536.0 + glm::dvec3(0, 0, 3));
        const auto block = [&](int32_t x, int32_t y, int32_t z) -> uint8_t {
            return zone->contains(x, y) ? zone->read(x, y, z).type() : 1;
        };
        npcs.step(0, map, {{me, body, 0, me.actionMs}}, 43200000, 0, block);
        const creatures::Npc *ours = nullptr;
        for (const creatures::Npc &n : npcs.all())
            if (n.fighter.id == id)
                ours = &n;
        if (!ours) {
            std::printf("  creature %llx (type %d): ours did not wake\n", (unsigned long long)id, theirs.type);
            continue;
        }
        compared++;
        const creatures::Combatant &c = ours->fighter;
        std::string wrong;
        const auto check = [&](const std::string &name, bool ok, const std::string &detail = {}) {
            if (ok)
                return;
            fields[name]++;
            wrong += "  " + name + (detail.empty() ? "" : " " + detail);
        };
        check("type", c.type == theirs.type);
        check("level", c.level == theirs.level, std::to_string(theirs.level) + "/" + std::to_string(c.level));
        check("tier", c.powerTier == theirs.powerTier);
        check("class", c.characterClass == theirs.characterClass && c.specialization == theirs.specialization);
        check("health", std::abs(c.health - theirs.health) < 0.01f,
              std::to_string(theirs.health) + "/" + std::to_string(c.health));
        bool multipliers = true;
        for (int m = 0; m < 5; m++)
            multipliers &= c.multipliers[m] == theirs.multipliers[m];
        check("multipliers", multipliers);
        uint8_t ourLooks[172], theirLooks[172];
        ours->looks.serialize(ourLooks);
        std::memcpy(theirLooks, &record.bytes[0x68], 172);
        std::string looksDetail; // the differing appearance bytes, offset: Alpha's/ours
        for (int b = 0; b < 172; b++)
            if (b != 5 && ourLooks[b] != theirLooks[b]) { // (byte 5: written by no one, spawn §5)
                char one[32];
                std::snprintf(one, sizeof one, " +%02x:%02x/%02x", b, theirLooks[b], ourLooks[b]);
                looksDetail += one;
            }
        check("appearance", looksDetail.empty(), looksDetail);
        for (size_t slot = 0; slot < c.equipment.size(); slot++) {
            const creatures::Item &a = theirs.equipment[slot], &b = c.equipment[slot];
            if (a.type == 0 && b.type == 0)
                continue;
            check("slot " + std::to_string(slot),
                  a.type == b.type && a.subtype == b.subtype && a.material == b.material && a.rarity == b.rarity &&
                      a.level == b.level && a.seed == b.seed,
                  itemText(a) + " vs " + itemText(b));
        }
        if (wrong.empty())
            same++;
        else
            std::printf("  creature %llx type %d (zone %d, %d spawn %zu):%s\n", (unsigned long long)id, theirs.type, zx,
                        zy, index, wrong.c_str());
    }
    std::printf("%s: %d creatures compared, %d the same\n", argv[3], compared, same);
    for (const auto &[name, n] : fields)
        std::printf("  %s differs in %d\n", name.c_str(), n);
    return 0;
}
