// Checks our world generator against zones recorded from Alpha itself by the extractor mod
// (tools/alpha-extractor). Each recording is one zone, block for block, as Alpha holds it in
// memory; this generates the same zone and reports where the two differ.
// Usage: ZoneCompare [--strict] [--grid] [--list] [--edges] file.avzd|file.avrg|file.avsp|file.avzc [more ...]
//        (--strict: fail on any compared mismatch, including light, or trailing data)
//        (--list: every spawn of both sides)   (--grid: a map of where they differ)
//        (--edges: light as flooded again at zone edges, rendering §9.2)
//        ZoneCompare --record seed zx zy out.avzd [unnamed]   (writes our own zone in the same form, to
//        check the tool and the extractor's format)
//
// The recording (.avzd, little endian):
//   "AVZD", u32 version (2), u32 world seed, u32 flags (bit 0: the world is named; version 1
//   has no flags and means a named world), i32 zone x, i32 zone y,
//   then the zone's 256 x 256 columns, x fastest: i32 bottom z, u32 count, count blocks of
//   4 bytes (r, g, b, type byte), from the bottom up.
#include "Config.h"
#include "CubModel.h"
#include "DataStore.h"
#include "Install.h"
#include "creatures/Npcs.h"
#include "worldgen/EdgeLight.h"
#include "worldgen/Terrain.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <tuple>
#include <set>
#include <string>
#include <optional>
#include <vector>

using namespace worldgen;

namespace {

// The game's building models (towns, dungeons, the home platform), read from the Alpha install
// the engine is set up with; without them those structures are left out.
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

struct Reader {
    std::vector<uint8_t> bytes;
    size_t at = 0;
    bool read(void *p, size_t n) {
        if (n > bytes.size() - at)
            return false;
        std::memcpy(p, &bytes[at], n);
        at += n;
        return true;
    }
    template <typename T> bool get(T &v) {
        if (sizeof v > bytes.size() - at)
            return false;
        std::memcpy(&v, &bytes[at], sizeof v);
        at += sizeof v;
        return true;
    }
};

bool sameBlock(const Block &a, const Block &b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.type() == b.type();
}

// Compares one recording; returns false when it cannot be read.
// The light Alpha's meshing leaves in the chunks along a zone's edges: the zone and its eight
// neighbours are generated and each edge chunk flooded again (rendering §9.2).
class EdgeFloods {
  public:
    EdgeFloods(Terrain &terrain, std::shared_ptr<const Zone> centre) {
        map.add(centre);
        for (int32_t i = -1; i <= 1; i++)
            for (int32_t j = -1; j <= 1; j++)
                if (i || j)
                    map.add(terrain.generate(centre->zx + i, centre->zy + j));
        for (int32_t cx = 0; cx < Zone::SIZE / CHUNK_SIDE; cx++)
            for (int32_t cy = 0; cy < Zone::SIZE / CHUNK_SIDE; cy++) {
                const int32_t x0 = centre->originX() + cx * CHUNK_SIDE, y0 = centre->originY() + cy * CHUNK_SIDE;
                if (atZoneEdge(x0, y0, CHUNK_SIDE))
                    floods.emplace(std::make_pair(cx, cy), EdgeLight(map, x0, y0, CHUNK_SIDE));
            }
        originX = centre->originX(), originY = centre->originY();
    }

    int light(int32_t x, int32_t y, int32_t z, int fallback) const {
        const auto it = floods.find({(x - originX) / CHUNK_SIDE, (y - originY) / CHUNK_SIDE});
        return it != floods.end() && it->second.covers(x, y, z) ? it->second.at(x, y, z) : fallback;
    }

  private:
    static constexpr int32_t CHUNK_SIDE = 32;
    WorldMap map;
    std::map<std::pair<int32_t, int32_t>, EdgeLight> floods;
    int32_t originX = 0, originY = 0;
};

bool compare(const char *path, bool grid, bool edges, bool strict) {
    std::ifstream file(path, std::ios::binary);
    Reader in{std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {})};
    char magic[4];
    uint32_t version = 0, seed = 0, flags = 1;
    int32_t zx = 0, zy = 0;
    if (!in.get(magic) || std::memcmp(magic, "AVZD", 4) || !in.get(version) || (version != 1 && version != 2) ||
        !in.get(seed) || (version >= 2 && !in.get(flags)) || !in.get(zx) || !in.get(zy)) {
        std::printf("%s: not a zone recording\n", path);
        return false;
    }
    World world(seed);
    world.named = flags & 1;
    Terrain terrain(world);
    terrain.prefabs = gameModels();
    terrain.finishSpawn = creatures::finishSpawn;
    const std::shared_ptr<const Zone> ours = terrain.generate(zx, zy);
    std::optional<EdgeFloods> floods;
    if (edges)
        floods.emplace(terrain, ours);

    int sameColumns = 0, shown = 0;
    long long blocks = 0, sameBlocks = 0;
    long long lit = 0, sameLight = 0; // air and water cells in Alpha's stored range: their light
    std::map<std::pair<int, int>, long long> lights; // (Alpha's light, ours) where they differ
    int cells[256] = {};
    std::map<std::pair<int, int>, long long> kinds; // (Alpha type, our type) of differing blocks
    for (int32_t i = 0; i < Zone::SIZE * Zone::SIZE; i++) {
        int32_t bottom = 0;
        uint32_t count = 0;
        if (!in.get(bottom) || !in.get(count) || count > 4096) {
            std::printf("%s: cut short at column %d\n", path, i);
            return false;
        }
        std::vector<Block> theirs(count);
        for (Block &b : theirs)
            if (!in.get(b.r) || !in.get(b.g) || !in.get(b.b) || !in.get(b.typeByte))
                return false;
        const Column &mine = ours->columns[(size_t)i];
        // Block by block over the z range either side holds (air outside a column's blocks).
        const int32_t lo = std::min(bottom, mine.bottom), hi = std::max(bottom + (int32_t)count, mine.top());
        bool same = true;
        int32_t firstDiff = 0;
        for (int32_t z = lo; z < hi; z++) {
            const Block a = z >= bottom && z < bottom + (int32_t)count ? theirs[(size_t)(z - bottom)] : Block{};
            const Block b = z >= mine.bottom && z < mine.top() ? mine.blocks[(size_t)(z - mine.bottom)] : Block{};
            // Air and water: Alpha keeps the cell's light in its colour bytes (rendering §9.2), the
            // first byte for air and the third for water; ours is kept apart (Zone::light).
            if (clear(a.type()) && a.type() == b.type()) {
                if (z >= bottom && z < bottom + (int32_t)count) {
                    const int theirLight = a.type() == WATER ? a.b : a.r;
                    const int32_t x = ours->originX() + i % Zone::SIZE, y = ours->originY() + i / Zone::SIZE;
                    int ourLight = ours->light(x, y, z);
                    if (floods)
                        ourLight = floods->light(x, y, z, ourLight);
                    lit++;
                    if (theirLight == ourLight)
                        sameLight++;
                    else
                        lights[{theirLight, ourLight}]++;
                }
                if (a.type() == AIR)
                    continue;
                blocks++, sameBlocks++; // water in both
                continue;
            }
            blocks++;
            if (sameBlock(a, b)) {
                sameBlocks++;
            } else {
                kinds[{a.type(), b.type()}]++;
                if (same) {
                    same = false;
                    firstDiff = z;
                }
            }
        }
        sameColumns += same;
        if (!same)
            cells[(i / Zone::SIZE / 16) * 16 + (i % Zone::SIZE) / 16]++;
        if (!same && shown < 10) {
            shown++;
            const int32_t x = i % Zone::SIZE, y = i / Zone::SIZE, z = firstDiff;
            const auto pick = [z](int32_t bottom, const std::vector<Block> &list) {
                return z >= bottom && z < bottom + (int32_t)list.size() ? list[(size_t)(z - bottom)] : Block{};
            };
            const Block a = pick(bottom, theirs), b = pick(mine.bottom, mine.blocks);
            std::printf("  column (%d, %d): Alpha %d..%d, ours %d..%d; first difference z %d: Alpha type %d "
                        "(%d,%d,%d), ours type %d (%d,%d,%d)\n",
                        x, y, bottom, bottom + (int32_t)count, mine.bottom, mine.top(), z, a.type(), a.r, a.g, a.b,
                        b.type(), b.r, b.g, b.b);
        }
    }
    std::printf("%s: zone (%d, %d) seed %u: %d of %d columns the same, %lld of %lld solid blocks (%.3f%%)\n", path, zx,
                zy, seed, sameColumns, Zone::SIZE * Zone::SIZE, sameBlocks, blocks,
                blocks ? 100.0 * (double)sameBlocks / (double)blocks : 100.0);
    if (grid) { // where the differences are: 16 x 16 cells of 16 x 16 columns, north up
        std::printf("  differing columns per 16 x 16 cell (. none, 1-9 tenths, # all):\n");
        for (int cy = 15; cy >= 0; cy--) {
            std::printf("  ");
            for (int cx = 0; cx < 16; cx++) {
                const int n = cells[cy * 16 + cx];
                std::printf("%c", n == 0 ? '.' : n == 256 ? '#' : (char)('1' + std::min(n * 9 / 256, 8)));
            }
            std::printf("\n");
        }
    }
    if (lit)
        std::printf("  light of air and water: %lld of %lld cells the same (%.3f%%)\n", sameLight, lit,
                    100.0 * (double)sameLight / (double)lit);
    {
        std::vector<std::pair<long long, std::pair<int, int>>> worst;
        for (const auto &[pair, n] : lights)
            worst.push_back({n, pair});
        std::sort(worst.rbegin(), worst.rend());
        for (size_t k = 0; k < worst.size() && k < 5; k++)
            std::printf("  light: Alpha %d where ours is %d: %lld cells\n", worst[k].second.first,
                        worst[k].second.second, worst[k].first);
    }
    for (const auto &[pair, n] : kinds)
        if (n * 200 >= blocks - sameBlocks) // the kinds of difference worth naming (0.5% or more)
            std::printf("  Alpha type %d where ours is type %d: %lld blocks\n", pair.first, pair.second, n);
    return !strict || (sameColumns == Zone::SIZE * Zone::SIZE && sameLight == lit && in.at == in.bytes.size());
}

// Writes our generated zone as a recording.
bool record(uint32_t seed, bool named, int32_t zx, int32_t zy, const char *path) {
    World world(seed);
    world.named = named;
    Terrain terrain(world);
    terrain.prefabs = gameModels();
    terrain.finishSpawn = creatures::finishSpawn;
    const auto zone = terrain.generate(zx, zy);
    std::vector<uint8_t> out;
    const auto put = [&out](const void *p, size_t n) {
        out.insert(out.end(), (const uint8_t *)p, (const uint8_t *)p + n);
    };
    const uint32_t version = 2, flags = named ? 1u : 0u;
    if (std::string(path).ends_with(".avsp")) {
        // A generated diagnostic fixture, with no retained native suffix. Only
        // fields consumed by this comparator are populated; it is not a save.
        const uint32_t count = (uint32_t)zone->spawns.size();
        put("AVSP", 4), put(&version, 4), put(&seed, 4), put(&flags, 4), put(&zx, 4), put(&zy, 4);
        put(&count, 4), put(&count, 4); // full count and established prefix count
        for (const Spawn &spawn : zone->spawns) {
            std::array<uint8_t, 0x10f0> row{};
            const auto field = [&](size_t offset, auto value) { std::memcpy(row.data() + offset, &value, sizeof value); };
            field(0x10, spawn.x), field(0x18, spawn.y), field(0x20, spawn.z);
            field(0x28, (int32_t)spawn.controller), field(0x2c, (int32_t)spawn.type);
            field(0x30, (uint8_t)spawn.characterClass), field(0x34, (int32_t)spawn.level);
            field(0x54, spawn.yaw), field(0x7a, (uint16_t)spawn.appearanceFlags);
            std::memcpy(row.data() + 0xf58, spawn.multipliers, sizeof spawn.multipliers);
            put(row.data(), row.size());
        }
        std::ofstream file(path, std::ios::binary);
        file.write((const char *)out.data(), (std::streamsize)out.size());
        return static_cast<bool>(file);
    }
    put("AVZD", 4), put(&version, 4), put(&seed, 4), put(&flags, 4), put(&zx, 4), put(&zy, 4);
    for (size_t i = 0; i < zone->columns.size(); i++) {
        const Column &c = zone->columns[i];
        const uint32_t count = (uint32_t)c.blocks.size();
        put(&c.bottom, 4), put(&count, 4);
        for (size_t k = 0; k < c.blocks.size(); k++) {
            Block b = c.blocks[k];
            if (clear(b.type())) {
                const uint8_t light = zone->light(zone->originX() + (int32_t)(i % Zone::SIZE),
                                                 zone->originY() + (int32_t)(i / Zone::SIZE),
                                                 c.bottom + (int32_t)k);
                if (b.type() == AIR)
                    b.r = light;
                else
                    b.b = light;
            }
            put(&b, 4);
        }
    }
    std::ofstream file(path, std::ios::binary);
    file.write((const char *)out.data(), (std::streamsize)out.size());
    return static_cast<bool>(file);
}

// Compares a region recording ("AVRG", from the extractor): its header, its 64 location
// records and its per-zone table (locations spec Ã‚Â§1).
bool compareRegion(const char *path, bool strict) {
    std::ifstream file(path, std::ios::binary);
    Reader in{std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {})};
    char magic[4];
    uint32_t version = 0, seed = 0, flags = 1;
    int32_t rx = 0, ry = 0, header[3] = {};
    if (!in.get(magic) || std::memcmp(magic, "AVRG", 4) || !in.get(version) || (version != 1 && version != 2) ||
        !in.get(seed) || (version >= 2 && !in.get(flags)) || !in.get(rx) || !in.get(ry) || !in.get(header)) {
        std::printf("%s: not a region recording\n", path);
        return false;
    }
    World world(seed);
    world.named = flags & 1;
    Terrain terrain(world);
    terrain.prefabs = gameModels();
    terrain.finishSpawn = creatures::finishSpawn;
    terrain.prepareRegion(rx, ry);
    const Region *ours = world.region(rx, ry);
    if (!ours)
        return false;
    int differences = 0;
    const auto report = [&](const std::string &what) {
        if (differences++ < 40)
            std::printf("  %s\n", what.c_str());
    };
    if (header[0] != ours->tier || header[1] != ours->h10 || header[2] != ours->style)
        report("header: Alpha tier " + std::to_string(header[0]) + " h10 " + std::to_string(header[1]) + " style " +
               std::to_string(header[2]) + ", ours " + std::to_string(ours->tier) + " " + std::to_string(ours->h10) +
               " " + std::to_string(ours->style));
    for (int k = 0; k < 64; k++) {
        uint8_t raw[0x2c];
        if (!in.get(raw))
            return false;
        Location a;
        std::memcpy(&a.x, raw + 0x00, 8), std::memcpy(&a.y, raw + 0x08, 8), std::memcpy(&a.radius, raw + 0x10, 4);
        std::memcpy(&a.height, raw + 0x14, 4), std::memcpy(&a.type, raw + 0x18, 4), std::memcpy(&a.variant, raw + 0x1c, 4);
        std::memcpy(&a.serial, raw + 0x20, 4), std::memcpy(&a.level, raw + 0x24, 4), std::memcpy(&a.subLevel, raw + 0x28, 4);
        const Location &b = ours->records[(size_t)k];
        if (a.type == 0 && b.type == 0)
            continue;
        if (a.x != b.x || a.y != b.y || a.radius != b.radius || a.height != b.height || a.type != b.type ||
            a.variant != b.variant || a.serial != b.serial || a.level != b.level || a.subLevel != b.subLevel) {
            char line[512];
            std::snprintf(line, sizeof line,
                          "location %d: Alpha type %d at (%.2f, %.2f) r %.2f h %.2f var %d serial %d level %d/%d; "
                          "ours type %d at (%.2f, %.2f) r %.2f h %.2f var %d serial %d level %d/%d",
                          k, a.type, a.x / 65536.0, a.y / 65536.0, a.radius, a.height, a.variant, a.serial, a.level,
                          a.subLevel, b.type, b.x / 65536.0, b.y / 65536.0, b.radius, b.height, b.variant, b.serial,
                          b.level, b.subLevel);
            report(line);
        }
    }
    int entryDifferences = 0;
    for (int k = 0; k < 64 * 64; k++) {
        uint8_t raw[16];
        if (!in.get(raw))
            return false;
        const ZoneEntry &b = ours->zones[(size_t)k];
        int32_t id, level;
        std::memcpy(&id, raw + 4, 4), std::memcpy(&level, raw + 8, 4);
        if (raw[0] != b.kind || raw[1] != b.variant || id != b.id || level != b.level || raw[12] != b.subLevel) {
            entryDifferences++;
            report("zone entry (" + std::to_string(k / 64) + ", " + std::to_string(k % 64) + "): Alpha kind " +
                   std::to_string(raw[0]) + " var " + std::to_string(raw[1]) + " id " + std::to_string(id) + " level " +
                   std::to_string(level) + "; ours kind " + std::to_string(b.kind) + " var " +
                   std::to_string(b.variant) + " id " + std::to_string(b.id) + " level " + std::to_string(b.level));
        }
    }
    std::printf("%s: region (%d, %d) seed %u: %d differences (%d of 4096 zone entries differ)\n", path, rx, ry, seed, differences,
                entryDifferences);
    return !strict || (differences == 0 && in.at == in.bytes.size());
}

// Compares a zone's spawn objects ("AVSP", from the extractor) with ours, in order: position,
// controller (+0x28), creature type (+0x2c), class (+0x30), level (+0x34), yaw (+0x54),
// appearance flags (+0x7a) and the five stat multipliers (+0xf58).
bool compareSpawns(const char *path, bool list, bool strict, bool prefixOnly) {
    std::ifstream file(path, std::ios::binary);
    Reader in{std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {})};
    char magic[4];
    uint32_t version = 0, seed = 0, flags = 1, count = 0, prefix = 0;
    int32_t zx = 0, zy = 0;
    if (!in.get(magic) || std::memcmp(magic, "AVSP", 4) || !in.get(version) || (version != 1 && version != 2) || !in.get(seed) ||
        !in.get(flags) || !in.get(zx) || !in.get(zy) || !in.get(count) ||
        (version == 2 && (!in.get(prefix) || prefix > count))) {
        std::printf("%s: not a spawn recording\n", path);
        return false;
    }
    if (prefixOnly && version != 2) {
        std::fprintf(stderr, "%s: prefix comparison requires AVSP v2 metadata; legacy counts do not identify it\n", path);
        return false;
    }
    const uint32_t comparedCount = prefixOnly ? prefix : count;
    World world(seed);
    world.named = flags & 1;
    Terrain terrain(world);
    terrain.prefabs = gameModels();
    terrain.finishSpawn = creatures::finishSpawn;
    const auto ours = terrain.generate(zx, zy);
    int same = 0, shown = 0;
    std::map<std::string, int> fields; // which fields differ, and how often
    for (uint32_t k = 0; k < count; k++) {
        std::vector<uint8_t> raw(0x10f0);
        if (!in.read(raw.data(), raw.size()))
            return false;
        if (k >= comparedCount)
            continue; // still read and validate the complete retained recording
        const auto at = [&](size_t offset, auto v) {
            std::memcpy(&v, &raw[offset], sizeof v);
            return v;
        };
        if (list)
            std::printf("  Alpha %2u: type %3d at (%.2f, %.2f, %.2f) controller %d class %d level %d flags %04x yaw %.1f\n", k,
                        at(0x2c, int32_t()), at(0x10, int64_t()) / 65536.0, at(0x18, int64_t()) / 65536.0,
                        at(0x20, int64_t()) / 65536.0, at(0x28, int32_t()), at(0x30, uint8_t()), at(0x34, int32_t()),
                        at(0x7a, uint16_t()), at(0x54, float()));
        if (k >= ours->spawns.size())
            continue;
        const Spawn &b = ours->spawns[k];
        std::string wrong;
        const auto check = [&](const char *name, bool ok) {
            if (!ok) {
                wrong += std::string(wrong.empty() ? "" : ", ") + name;
                fields[name]++;
            }
        };
        check("position", at(0x10, int64_t()) == b.x && at(0x18, int64_t()) == b.y && at(0x20, int64_t()) == b.z);
        check("controller", at(0x28, int32_t()) == b.controller);
        check("type", at(0x2c, int32_t()) == b.type);
        check("class", at(0x30, uint8_t()) == b.characterClass);
        check("level", at(0x34, int32_t()) == b.level);
        check("yaw", at(0x54, float()) == b.yaw);
        check("flags", at(0x7a, uint16_t()) == b.appearanceFlags);
        bool multipliers = true;
        for (int m = 0; m < 5; m++)
            multipliers &= at(0xf58 + 4 * (size_t)m, float()) == b.multipliers[m];
        check("multipliers", multipliers);
        if (wrong.empty()) {
            same++;
        } else if (shown++ < 10) {
            std::printf("  spawn %u: %s differ; Alpha type %d at (%.2f, %.2f, %.2f) level %d, ours type %d at "
                        "(%.2f, %.2f, %.2f) level %d\n",
                        k, wrong.c_str(), at(0x2c, int32_t()), at(0x10, int64_t()) / 65536.0,
                        at(0x18, int64_t()) / 65536.0, at(0x20, int64_t()) / 65536.0, at(0x34, int32_t()), b.type,
                        b.x / 65536.0, b.y / 65536.0, b.z / 65536.0, b.level);
        }
    }
    if (list)
        for (size_t k = 0; k < ours->spawns.size(); k++) {
            const Spawn &b = ours->spawns[k];
            std::printf("  ours  %2zu: type %3d at (%.2f, %.2f, %.2f) controller %d class %d level %d flags %04x yaw %.1f\n",
                        k, b.type, b.x / 65536.0, b.y / 65536.0, b.z / 65536.0, b.controller, b.characterClass, b.level,
                        b.appearanceFlags, b.yaw);
        }
    std::printf("%s: zone (%d, %d) seed %u: Alpha %u spawns, ours %zu; %d the same\n", path, zx, zy, seed, count,
                ours->spawns.size(), same);
    for (const auto &[name, n] : fields)
        std::printf("  %s differs in %d\n", name.c_str(), n);
    if (version == 2)
        std::printf("  native prefix %u; compared %u %s records\n", prefix, comparedCount, prefixOnly ? "prefix" : "retained");
    return !strict || (same == comparedCount && ours->spawns.size() == comparedCount && in.at == in.bytes.size());
}

} // namespace

// Compares a zone's sprites, statics and world items ("AVZC", from the extractor) with ours. The
// game keeps plants, trees and structure sprites in one list; ours in several.
bool compareContents(const char *path, bool list, bool strict) {
    std::ifstream file(path, std::ios::binary);
    Reader in{std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {})};
    char magic[4];
    uint32_t version = 0, seed = 0, flags = 1, count = 0;
    int32_t zx = 0, zy = 0;
    if (!in.get(magic) || std::memcmp(magic, "AVZC", 4) || !in.get(version) || version != 1 || !in.get(seed) ||
        !in.get(flags) || !in.get(zx) || !in.get(zy)) {
        std::printf("%s: not a contents recording\n", path);
        return false;
    }
    World world(seed);
    world.named = flags & 1;
    Terrain terrain(world);
    terrain.prefabs = gameModels();
    terrain.finishSpawn = creatures::finishSpawn;
    const auto zone = terrain.generate(zx, zy);
    const auto value = [](const std::vector<uint8_t> &raw, size_t offset, auto v) {
        std::memcpy(&v, &raw[offset], sizeof v);
        return v;
    };
    using Place = std::tuple<int32_t, int64_t, int64_t, int64_t>;

    // Sprites, by kind and place.
    std::multiset<Place> ours;
    for (const Plant &p : zone->plants)
        ours.insert({p.kind, ((int64_t)p.x << 16) + 32768, ((int64_t)p.y << 16) + 32768,
                     ((int64_t)p.z << 16) - (p.sink ? 6553 : 0)});
    for (const Sprite &p : zone->sprites)
        ours.insert({p.kind, p.x, p.y, p.z});
    for (const LeafSource &p : zone->leafSources)
        ours.insert({60, p.x, p.y, p.z});
    for (const std::array<int64_t, 3> &p : zone->songPosts)
        ours.insert({61, p[0], p[1], p[2]});
    std::map<int32_t, std::array<int, 3>> kinds; // Alpha's, ours, the same
    for (const Place &p : ours)
        kinds[std::get<0>(p)][1]++;
    const size_t ourSprites = ours.size();
    if (!in.get(count))
        return false;
    int matched = 0;
    for (uint32_t k = 0; k < count; k++) {
        std::vector<uint8_t> raw(0x3c);
        if (!in.read(raw.data(), raw.size()))
            return false;
        const Place p{value(raw, 0, int32_t()), value(raw, 8, int64_t()), value(raw, 16, int64_t()), value(raw, 24, int64_t())};
        kinds[std::get<0>(p)][0]++;
        if (const auto it = ours.find(p); it != ours.end()) {
            ours.erase(it);
            kinds[std::get<0>(p)][2]++;
            matched++;
        } else if (list) {
            std::printf("  sprite in Alpha only: kind %d at (%.2f, %.2f, %.2f)\n", std::get<0>(p), std::get<1>(p) / 65536.0,
                        std::get<2>(p) / 65536.0, std::get<3>(p) / 65536.0);
        }
    }
    if (list)
        for (const Place &p : ours)
            std::printf("  sprite in ours only: kind %d at (%.2f, %.2f, %.2f)\n", std::get<0>(p), std::get<1>(p) / 65536.0,
                        std::get<2>(p) / 65536.0, std::get<3>(p) / 65536.0);
    std::string differing;
    for (const auto &[kind, n] : kinds)
        if (n[0] != n[2] || n[1] != n[2])
            differing += " " + std::to_string(kind) + ":" + std::to_string(n[0]) + "/" + std::to_string(n[1]);
    std::printf("%s: zone (%d, %d): sprites Alpha %u, ours %zu, the same %d;%s%s\n", path, zx, zy, count, ourSprites,
                matched, differing.empty() ? "" : " kinds Alpha/ours", differing.c_str());
    const bool spritesMatch = matched == count && ourSprites == count;

    // Statics, in order.
    if (!in.get(count))
        return false;
    int same = 0;
    std::map<std::string, int> fields;
    for (uint32_t k = 0; k < count; k++) {
        std::vector<uint8_t> raw(0x188);
        if (!in.read(raw.data(), raw.size()))
            return false;
        if (k >= zone->statics.size())
            continue;
        const StaticObject &b = zone->statics[k];
        std::string wrong;
        const auto check = [&](const char *name, bool ok) {
            if (!ok)
                fields[name]++, wrong += std::string(" ") + name;
        };
        check("kind", value(raw, 0, int32_t()) == b.kind);
        check("position", value(raw, 8, int64_t()) == b.x && value(raw, 16, int64_t()) == b.y && value(raw, 24, int64_t()) == b.z);
        check("rotation", value(raw, 0x20, int32_t()) == b.rotation);
        check("scale", value(raw, 0x24, float()) == b.scale[0] && value(raw, 0x28, float()) == b.scale[1] &&
                           value(raw, 0x2c, float()) == b.scale[2]);
        same += wrong.empty();
        if (list && !wrong.empty())
            std::printf("  static %u:%s; Alpha kind %d at (%.2f, %.2f, %.2f) rot %d, ours kind %d at (%.2f, %.2f, %.2f) rot %d\n", k,
                        wrong.c_str(), value(raw, 0, int32_t()), value(raw, 8, int64_t()) / 65536.0,
                        value(raw, 16, int64_t()) / 65536.0, value(raw, 24, int64_t()) / 65536.0, value(raw, 0x20, int32_t()),
                        b.kind, b.x / 65536.0, b.y / 65536.0, b.z / 65536.0, b.rotation);
    }
    std::string fieldText;
    for (const auto &[name, n] : fields)
        fieldText += " " + name + " " + std::to_string(n);
    std::printf("  statics Alpha %u, ours %zu, the same %d;%s\n", count, zone->statics.size(), same, fieldText.c_str());
    const bool staticsMatch = same == count && zone->statics.size() == count;

    // World items: ours are in two lists, so by category, subtype and place.
    using Thing = std::tuple<int32_t, int32_t, int64_t, int64_t, int64_t>;
    std::multiset<Thing> things;
    for (const GroundItem &i : zone->items)
        things.insert({i.category, i.subtype, ((int64_t)i.x << 16) + 32768, ((int64_t)i.y << 16) + 32768, (int64_t)i.z << 16});
    for (const ItemDrop &d : zone->drops)
        things.insert({d.item.type, d.item.subtype, d.x, d.y, d.z});
    const size_t ourItems = things.size();
    if (!in.get(count))
        return false;
    int items = 0;
    for (uint32_t k = 0; k < count; k++) {
        std::vector<uint8_t> raw(0x148);
        if (!in.read(raw.data(), raw.size()))
            return false;
        const Thing t{raw[0], raw[1], value(raw, 0x118, int64_t()), value(raw, 0x120, int64_t()), value(raw, 0x128, int64_t())};
        if (const auto it = things.find(t); it != things.end()) {
            things.erase(it);
            items++;
        } else if (list) {
            std::printf("  item in Alpha only: %d/%d at (%.2f, %.2f, %.2f)\n", raw[0], raw[1], std::get<2>(t) / 65536.0,
                        std::get<3>(t) / 65536.0, std::get<4>(t) / 65536.0);
        }
    }
    if (list)
        for (const Thing &t : things)
            std::printf("  item in ours only: %d/%d at (%.2f, %.2f, %.2f)\n", std::get<0>(t), std::get<1>(t),
                        std::get<2>(t) / 65536.0, std::get<3>(t) / 65536.0, std::get<4>(t) / 65536.0);
    std::printf("  items Alpha %u, ours %zu, the same %d\n", count, ourItems, items);
    return !strict || (spritesMatch && staticsMatch && items == count && ourItems == count && in.at == in.bytes.size());
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::printf("usage: ZoneCompare [--strict] [--grid] [--list] [--edges] recording [more recordings ...]\n");
        return 1;
    }
    if (std::string(argv[1]) == "--record" && (argc == 6 || argc == 7))
        return record((uint32_t)std::strtoul(argv[2], nullptr, 10), argc == 6, std::atoi(argv[3]), std::atoi(argv[4]),
                      argv[5])
                   ? 0
                   : 1;
    bool ok = true, grid = false, list = false, edges = false, strict = false, prefixOnly = false;
    std::vector<const char *> recordings;
    for (int i = 1; i < argc; i++) {
        const std::string argument = argv[i];
        if (argument == "--strict")
            strict = true;
        else if (argument == "--prefix")
            prefixOnly = true;
        else if (argument == "--grid")
            grid = true;
        else if (argument == "--list")
            list = true;
        else if (argument == "--edges")
            edges = true;
        else if (argument.starts_with("--")) {
            std::fprintf(stderr, "unknown option: %s\n", argv[i]);
            return 1;
        } else
            recordings.push_back(argv[i]);
    }
    if (recordings.empty()) {
        std::fprintf(stderr, "no recordings supplied\n");
        return 1;
    }
    for (const char *path : recordings) {
        const std::string name = path;
        bool matched;
        if (prefixOnly && !name.ends_with(".avsp")) {
            std::fprintf(stderr, "%s: --prefix applies only to AVSP spawn recordings\n", path);
            ok = false;
            continue;
        }
        if (name.ends_with(".avsp"))
            matched = compareSpawns(path, list, strict, prefixOnly);
        else if (name.ends_with(".avzc"))
            matched = compareContents(path, list, strict);
        else if (name.ends_with(".avrg"))
            matched = compareRegion(path, strict);
        else
            matched = compare(path, grid, edges, strict);
        if (!matched)
            std::fprintf(stderr, "%s: %s\n", path, strict ? "strict comparison failed or recording unreadable" : "recording unreadable");
        ok &= matched;
    }
    return ok ? 0 : 1;
}
