// Checks the save readers and writers (docs/specs/alpha-saves.md): every record in the owner's
// Alpha saves must parse and serialize back to the same bytes, and a written save must read
// back. The owner's files are opened read-only and never written; writing uses a scratch file.
// Usage: SaveTests [game folder]
#include "Config.h"
#include "CubModel.h"
#include "DataStore.h"
#include "Install.h"
#include "Saves.h"
#include "WorldSession.h"
#include "ZlibStream.h"
#include "creatures/Npcs.h"
#include "worldgen/MapTile.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <windows.h>

static int failures = 0;
static void check(bool ok, const std::string &what) {
    if (!ok) {
        failures++;
        std::printf("FAIL %s\n", what.c_str());
    }
}

int main(int argc, char **argv) {
    // Writing and reading back, in a scratch file beside the test.
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    const fs::path scratch = fs::path(self).parent_path() / "SaveTests-scratch.db";
    std::error_code ec;
    fs::remove(scratch, ec);
    {
        SaveDb db;
        std::string error;
        check(db.open(scratch, SaveDb::Mode::ReadWrite, &error), "create scratch: " + error);
        CharacterRecord c;
        c.name = "Scratch";
        c.level = 7;
        c.position[0] = 550398629740LL;
        c.tabs.assign(4, std::vector<CharacterRecord::Slot>(50));
        c.skills.assign(11, 2);
        c.visits.push_back({123, "test", {1, 2, 3}});
        check(writeCharacters(db, {c, c}), "write two characters");
        check(writeCharacters(db, {c}), "rewrite as one");
        const auto back = readCharacters(db);
        check(back.size() == 1 && back[0].name == "Scratch" && back[0].level == 7 && back[0].visits.size() == 1 &&
                  back[0].serialize() == c.serialize(),
              "characters read back");
        check(!db.get("1"), "removed entry is gone");
        WorldClock clock{5, 14615510};
        check(db.put("time", clock.serialize()) && WorldClock::parse(*db.get("time"))->timeOfDayMs == 14615510, "clock");
    }
    fs::remove(scratch, ec);

    // A single-player session in a scratch folder: a new world, a character, leaving, coming back.
    {
        const fs::path folder = fs::path(self).parent_path() / "SaveTests-session";
        fs::remove_all(folder, ec);
        fs::create_directories(folder / "Save", ec);
        {
            SaveDb db;
            check(db.open(folder / "Save" / "characters.db", SaveDb::Mode::ReadWrite), "session characters");
            CharacterRecord c;
            c.name = "Walker";
            c.tabs.assign(4, std::vector<CharacterRecord::Slot>(50));
            c.skills.assign(11, 0);
            check(writeCharacters(db, {c}), "session character written");
        }
        std::string error;
        WorldSession first;
        check(first.open(folder, "scratch", 123, 0, &error), "session open: " + error);
        check(first.clock.day == 0 && first.clock.timeOfDayMs == 43200000, "a new world starts at noon");
        check(!first.startPosition(), "a new character has no position here");
        first.advance(86400.5); // ten game days and five game seconds
        first.leave({8396928.5, 8396930.25, 120});
        WorldSession again;
        check(again.open(folder, "scratch", 999, 0, &error), "session reopen: " + error);
        check(again.seed() == 123, "the world keeps its seed");
        check(again.clock.day == 10 && again.clock.timeOfDayMs == 43205000, "the clock comes back");
        const auto at = again.startPosition();
        check(at && at->x == 8396928.5 && at->y == 8396930.25 && at->z == 120, "the character comes back where it left");
        SaveDb worlds;
        check(worlds.open(folder / "Save" / "worlds.db", SaveDb::Mode::ReadOnly) && readWorlds(worlds).size() == 1,
              "one world entry");
        fs::remove_all(folder, ec);
    }

    // Screens §5.1.6 / saves §6.4: a map tile round-trips.
    {
        TileSave t;
        t.base = 7, t.sx = 2, t.sy = 2, t.sz = 1;
        t.rgb = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
        t.markers = {{8396800, 8396832, 64}};
        const auto back = TileSave::parse(t.serialize());
        check(back && back->base == 7 && back->sz == 1 && back->rgb == t.rgb && back->markers == t.markers,
              "map tile round-trips");
    }

    Config config;
    if (!config.load("AlphaVulkan.ini"))
        config.load("config/AlphaVulkan.ini");
    const fs::path game = install::locate(argc > 1 ? argv[1] : config.text("Game", "Path"));
    if (game.empty() || !fs::is_directory(game / "Save")) {
        std::printf("no Alpha saves found; only the scratch checks ran\n");
        std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
        return failures ? 1 : 0;
    }

    SaveDb characters;
    std::string error;
    if (characters.open(game / "Save" / "characters.db", SaveDb::Mode::ReadOnly, &error)) {
        const uint32_t n = characters.number("num").value_or(0);
        for (uint32_t i = 0; i < n; i++) {
            const auto inflated = zlibInflate(*characters.get(std::to_string(i)));
            const auto c = inflated ? CharacterRecord::parse(*inflated) : std::nullopt;
            check(c.has_value(), "character " + std::to_string(i) + " parses");
            if (c) {
                check(c->serialize() == *inflated, "character " + std::to_string(i) + " round-trips byte for byte");
                std::printf("character %u: \"%s\" level %d class %u, %zu tabs, %zu worlds visited\n", i, c->name.c_str(),
                            c->level, c->characterClass, c->tabs.size(), c->visits.size());
            }
        }
    } else
        std::printf("(no characters.db: %s)\n", error.c_str());

    SaveDb worlds;
    if (worlds.open(game / "Save" / "worlds.db", SaveDb::Mode::ReadOnly)) {
        const uint32_t n = worlds.number("num").value_or(0);
        for (uint32_t i = 0; i < n; i++) {
            const auto e = WorldEntry::parse(*worlds.get(std::to_string(i)));
            check(e.has_value(), "world entry " + std::to_string(i) + " parses");
            if (!e)
                continue;
            const auto again = WorldEntry::parse(e->serialize());
            check(again && again->name == e->name && again->seed == e->seed && again->previewRgb == e->previewRgb,
                  "world entry " + std::to_string(i) + " survives a rewrite");
            std::printf("world %u: \"%s\" seed %u, discovered %u\n", i, e->name.c_str(), e->seed, e->discovered);

            SaveDb state;
            if (!state.open(game / "Save" / ("world_" + e->name + ".db"), SaveDb::Mode::ReadOnly))
                continue;
            if (const auto t = state.get("time"))
                check(WorldClock::parse(*t).has_value(), e->name + " clock parses");
            size_t zones = 0;
            for (const std::string &key : state.keys("zone")) {
                const auto value = state.get(key);
                const auto z = ZoneSave::parse(*value);
                check(z && z->serialize() == *value, key + " round-trips byte for byte");
                zones++;
            }
            std::printf("  %zu zone records\n", zones);

            // The owner's map tiles parse, and re-encode to records that parse the same.
            SaveDb map;
            if (map.open(game / "Save" / ("map_" + e->name + ".db"), SaveDb::Mode::ReadOnly)) {
                size_t tiles = 0;
                for (const std::string &key : map.keys("tile")) {
                    const auto t = TileSave::parse(*map.get(key));
                    check(t.has_value(), key + " parses");
                    if (!t)
                        continue;
                    const auto again = TileSave::parse(t->serialize());
                    check(again && again->rgb == t->rgb && again->base == t->base && again->markers == t->markers,
                          key + " survives a rewrite");
                    // The first few against our own world generator and tile builder.
                    const char *only = std::getenv("ONLY_TILES");
                    if ((tiles < 4 && !only) || std::getenv("ALL_TILES") || (only && std::strstr(only, key.c_str()))) {
                        int zx = 0, zy = 0;
                        std::sscanf(key.c_str(), "tile%d_%d", &zx, &zy);
                        worldgen::World w(e->seed);
                        worldgen::Terrain terrain(w);
                        // The game's building models, for towns and dungeons.
                        static Content content;
                        static bool opened = content.open(game);
                        terrain.finishSpawn = creatures::finishSpawn;
                        if (opened)
                            terrain.prefabs = [&](const std::string &k) -> std::shared_ptr<const worldgen::Prefab> {
                                static std::map<std::string, std::shared_ptr<const worldgen::Prefab>> cache;
                                if (const auto it = cache.find(k); it != cache.end())
                                    return it->second;
                                std::shared_ptr<worldgen::Prefab> prefab;
                                CubModel model;
                                if (const auto bytes = content.read(k); bytes && model.load(bytes->data(), bytes->size())) {
                                    prefab = std::make_shared<worldgen::Prefab>();
                                    prefab->sx = model.sx, prefab->sy = model.sy, prefab->sz = model.sz;
                                    prefab->rgb = model.rgb;
                                }
                                return cache[k] = prefab;
                            };
                        const auto zone = terrain.generate(zx, zy);
                        if (std::getenv("NO_TREES"))
                            for (worldgen::Column &c : zone->columns)
                                for (worldgen::Block &b : c.blocks)
                                    if (b.type() == 7 || b.type() == 8)
                                        b = worldgen::Block{255, 255, 255, 0};
                        const worldgen::MapTile ours = worldgen::buildMapTile(*zone, terrain);
                        // Voxels both filled and within 3 per channel; filled only in theirs / ours;
                        // both filled but different colours; per layer the mismatches.
                        size_t close = 0, onlyTheirs = 0, onlyOurs = 0, differ = 0;
                        std::vector<int> byLayer(std::max(t->sz, 1), 0);
                        if (ours.sz == t->sz && ours.rgb.size() == t->rgb.size())
                            for (size_t i = 0; i < t->rgb.size(); i += 3) {
                                const bool a = t->rgb[i] | t->rgb[i + 1] | t->rgb[i + 2], b = ours.rgb[i] | ours.rgb[i + 1] | ours.rgb[i + 2];
                                int d = 0;
                                for (int c = 0; c < 3; c++)
                                    d = std::max(d, std::abs((int)t->rgb[i + c] - (int)ours.rgb[i + c]));
                                const int layer = (int)(i / 3 / (32 * 32));
                                if (a && b && d <= 3)
                                    close++;
                                else if (a && !b)
                                    onlyTheirs++, byLayer[layer]++;
                                else if (b && !a)
                                    onlyOurs++, byLayer[layer]++;
                                else if (a && b)
                                    differ++, byLayer[layer]++;
                            }
                        std::printf("  %s: base %d/%d, layers %d/%d; %zu close, %zu differ, %zu only Alpha's, %zu only ours; by layer",
                                    key.c_str(), t->base, ours.base, t->sz, ours.sz, close, differ, onlyTheirs, onlyOurs);
                        for (int n : byLayer)
                            std::printf(" %d", n);
                        std::printf("\n");
                        // Per voxel column: the top filled layer, and the top colour where it agrees.
                        if (ours.sz == t->sz && ours.rgb.size() == t->rgb.size()) {
                            auto topOf = [&](const std::vector<uint8_t> &rgb, int x, int y) {
                                for (int k = t->sz - 1; k >= 0; k--) {
                                    const size_t i = 3 * (((size_t)k * 32 + y) * 32 + x);
                                    if (rgb[i] | rgb[i + 1] | rgb[i + 2])
                                        return k;
                                }
                                return -1;
                            };
                            int equal = 0, off1 = 0, more = 0, colourClose = 0, higher = 0;
                            std::string diffs;
                            for (int x = 0; x < 32; x++)
                                for (int y = 0; y < 32; y++) {
                                    const int ka = topOf(t->rgb, x, y), kb = topOf(ours.rgb, x, y);
                                    if (ka == kb) {
                                        equal++;
                                        const size_t i = 3 * (((size_t)ka * 32 + y) * 32 + x);
                                        int d = 0;
                                        for (int c = 0; c < 3; c++)
                                            d = std::max(d, std::abs((int)t->rgb[i + c] - (int)ours.rgb[i + c]));
                                        if (d <= 3)
                                            colourClose++;
                                        else if (diffs.size() < 400)
                                            diffs += " (" + std::to_string(x) + "," + std::to_string(y) + ": " +
                                                     std::to_string(t->rgb[i]) + "/" + std::to_string(t->rgb[i + 1]) + "/" +
                                                     std::to_string(t->rgb[i + 2]) + " vs " + std::to_string(ours.rgb[i]) + "/" +
                                                     std::to_string(ours.rgb[i + 1]) + "/" + std::to_string(ours.rgb[i + 2]) + ")";
                                    } else {
                                        (std::abs(ka - kb) == 1 ? off1 : more)++;
                                        higher += kb > ka;
                                    }
                                }
                            std::printf("    tops: %d equal (%d same colour), %d one layer off, %d more; ours higher in %d\n", equal,
                                        colourClose, off1, more, higher);
                            if (tiles == 0)
                                std::printf("    colour differences:%s\n", diffs.c_str());
                            // Where trees stand: columns two or more layers above the bare
                            // ground ('A' Alpha's only, 'o' ours only, '#' both).
                            // Per town plot: our role, and how many voxel columns stand two or more
                            // layers above the bare ground in ours and in Alpha's tile
                            // ("kind:ours/Alpha"; the top row is the high y edge).
                            if (std::getenv("PLOT_MAP") && !zone->townPlots.empty()) {
                                worldgen::Zone bare = *zone;
                                for (worldgen::Column &c : bare.columns)
                                    for (worldgen::Block &b : c.blocks)
                                        if (b.typeByte & 0x40)
                                            b = worldgen::Block{255, 255, 255, 0};
                                const worldgen::MapTile ground = worldgen::buildMapTile(bare, terrain);
                                const int g = (int)std::lround(std::sqrt((double)zone->townPlots.size()));
                                for (int j = g - 1; j >= 0; j--) {
                                    std::printf("    ");
                                    for (int i = 0; i < g; i++) {
                                        const auto &p = zone->townPlots[i + g * j];
                                        int mine = 0, theirs = 0;
                                        for (int x = 0; x < 32; x++)
                                            for (int y = 0; y < 32; y++) {
                                                const int bx = zone->originX() + 8 * x + 4, by = zone->originY() + 8 * y + 4;
                                                if (bx < p.ox || bx >= p.ox + p.size || by < p.oy || by >= p.oy + p.size)
                                                    continue;
                                                const int base = topOf(ground.rgb, x, y);
                                                mine += topOf(ours.rgb, x, y) >= base + 2;
                                                theirs += topOf(t->rgb, x, y) >= base + 2;
                                            }
                                        std::printf("%2d.%d:%2d/%2d  ", p.kind, p.sub, mine, theirs);
                                    }
                                    std::printf("\n");
                                }
                            }
                            // Top layers compared per column: '.' the same, '+' ours higher, '-' lower
                            // (top row is the high y edge).
                            if (std::getenv("DIFF_MAP"))
                                for (int y = 31; y >= 0; y--) {
                                    std::printf("    ");
                                    for (int x = 0; x < 32; x++) {
                                        const int d = topOf(ours.rgb, x, y) - topOf(t->rgb, x, y);
                                        std::printf("%c", d == 0 ? '.' : d > 0 ? (d > 9 ? '#' : (char)('0' + d)) : (d < -9 ? '=' : (char)('a' - d - 1)));
                                    }
                                    std::printf("\n");
                                }
                            if (std::getenv("TREE_MAP") && tiles == 2) {
                                worldgen::Zone bare = *zone;
                                for (worldgen::Column &c : bare.columns)
                                    for (worldgen::Block &b : c.blocks)
                                        if (b.type() == 7 || b.type() == 8)
                                            b = worldgen::Block{255, 255, 255, 0};
                                const worldgen::MapTile ground = worldgen::buildMapTile(bare, terrain);
                                for (int y = 31; y >= 0; y--) {
                                    std::printf("    ");
                                    for (int x = 0; x < 32; x++) {
                                        const int g = topOf(ground.rgb, x, y);
                                        const bool a = topOf(t->rgb, x, y) >= g + 2, b = topOf(ours.rgb, x, y) >= g + 2;
                                        std::printf("%c", a && b ? '#' : a ? 'A' : b ? 'o' : '.');
                                    }
                                    std::printf("\n");
                                }
                            }
                        }
                    }
                    tiles++;
                }
                std::printf("  %zu map tiles\n", tiles);
                // Land pictures: parse, survive a rewrite, and the picture's box of zones agrees
                // with our region-cell ownership (screens §5.1.6).
                worldgen::World lw(e->seed);
                for (const std::string &key : map.keys("land")) {
                    const auto l = LandSave::parse(*map.get(key));
                    check(l.has_value(), key + " parses");
                    if (!l)
                        continue;
                    const auto again = LandSave::parse(l->serialize());
                    check(again && again->rgb == l->rgb && again->cell == l->cell, key + " survives a rewrite");
                    int lx = 0, ly = 0;
                    std::sscanf(key.c_str(), "land%d_%d", &lx, &ly);
                    lw.makeCells(lx, ly);
                    const worldgen::RegionCell *cell = lw.cell(lx, ly);
                    int x0 = INT32_MAX, y0 = INT32_MAX, x1 = INT32_MIN, y1 = INT32_MIN;
                    for (int zx = 64 * lx - 64; zx <= 64 * lx + 128; zx++)
                        for (int zy = 64 * ly - 64; zy <= 64 * ly + 128; zy++)
                            if (cell && lw.landOwnsZone(lx, ly, zx, zy))
                                x0 = std::min(x0, zx), y0 = std::min(y0, zy), x1 = std::max(x1, zx), y1 = std::max(y1, zy);
                    if (cell) {
                        int32_t sx, sy, sid, sel;
                        std::memcpy(&sx, &l->cell[0], 4), std::memcpy(&sy, &l->cell[4], 4);
                        std::memcpy(&sid, &l->cell[20], 4), std::memcpy(&sel, &l->cell[24], 4);
                        if (std::getenv("LAND_DIFF"))
                            for (int32_t vy = 0; vy < l->sy; vy++)
                                for (int32_t vx = 0; vx < l->sx; vx++) {
                                    const uint8_t *p = &l->rgb[3 * ((size_t)vy * l->sx + vx)];
                                    const int32_t zx = l->originX + vx, zy = l->originY + vy;
                                    if ((p[0] | p[1] | p[2]) && !lw.landOwnsZone(lx, ly, zx, zy)) {
                                        int32_t oi = 0, oj = 0;
                                        lw.ownerIndex(zx * 256, zy * 256, oi, oj);
                                        std::string beaten;
                                        for (int32_t i = lx - 1; i <= lx + 1; i++)
                                            for (int32_t j = ly - 1; j <= ly + 1; j++)
                                                if ((i != lx || j != ly) && !lw.landOwnsZone(i, j, zx, zy) == false)
                                                    beaten += " " + std::to_string(i - lx) + "," + std::to_string(j - ly);
                                        std::printf("    %s zone %d,%d owner %+d,%+d; neighbours owning:%s\n", key.c_str(), zx, zy,
                                                    oi - lx, oj - ly, beaten.c_str());
                                    }
                                }
                        if (sx != cell->x || sy != cell->y || sel != cell->elevation)
                            std::printf("  %s cell: saved %d,%d id %d el %d; ours %d,%d id %d el %d\n", key.c_str(), sx, sy, sid,
                                        sel, cell->x, cell->y, cell->id, cell->elevation);
                    }
                    std::printf("  %s: origin %d,%d size %dx%d; ours %d,%d size %dx%d\n", key.c_str(), l->originX, l->originY,
                                l->sx, l->sy, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
                    // Open point (roadmap): 6 of the 25 sample pictures take in zones a diagonal
                    // neighbour is closer to; the rule itself is confirmed (screens §5.1.6 step 3).
                    if (x0 != l->originX || y0 != l->originY || x1 - x0 + 1 != l->sx || y1 - y0 + 1 != l->sy)
                        std::printf("  (open point) %s covers other zones\n", key.c_str());
                }
                // Region records: byte-exact rewrite, and the zone table copies agree with ours.
                worldgen::Terrain rt(lw);
                for (const std::string &key : map.keys("reg")) {
                    const auto raw = *map.get(key);
                    const auto g = RegionSave::parse(raw);
                    check(g.has_value(), key + " parses");
                    if (!g)
                        continue;
                    check(g->serialize() == raw, key + " rewrites byte for byte");
                    int rx = 0, ry = 0;
                    std::sscanf(key.c_str(), "reg%d_%d", &rx, &ry);
                    rt.prepareRegion(rx, ry);
                    const worldgen::Region *region = lw.region(rx, ry);
                    if (!region)
                        continue;
                    int same = 0, flagged = 0, shown = 0;
                    for (int i = 0; i < 64 * 64; i++) {
                        const RegionSave::ZoneCell &z = g->zones[i];
                        const worldgen::ZoneEntry &E = region->zones[i];
                        flagged += z.flags != 0;
                        if (z.info[0] == E.kind && z.info[1] == E.variant && (int32_t)z.id == E.id &&
                            (int32_t)z.level == E.level && z.subLevel == E.subLevel)
                            same++;
                        else if (shown++ < 4 && std::getenv("REG_DIFF"))
                            std::printf("    %s %d: saved %d %d %d %d id %u lv %u sub %d; ours %d %d id %d lv %d sub %d\n",
                                        key.c_str(), i, z.info[0], z.info[1], z.info[2], z.info[3], z.id, z.level,
                                        z.subLevel, E.kind, E.variant, E.id, E.level, E.subLevel);
                    }
                    // A region the game never generated keeps the defaults (kind 0, level 1).
                    const bool refreshed = std::any_of(g->zones.begin(), g->zones.end(),
                                                       [](const RegionSave::ZoneCell &z) { return z.info[0] != 0 || z.level != 1; });
                    if (!refreshed)
                        continue;
                    std::printf("  %s: %d of 4096 zone entries agree, %d flagged\n", key.c_str(), same, flagged);
                    check(same == 4096, key + " zone table agrees");
                    RegionSave ours = *g;
                    ours.refresh(*region);
                    // The ground height may differ by a few float steps (the game rounds through
                    // x87); everything else must match exactly.
                    int copies = 0;
                    for (int i = 0; i < 64; i++) {
                        auto a = g->locations[i], b = ours.locations[i];
                        float ha, hb;
                        std::memcpy(&ha, &a[20], 4), std::memcpy(&hb, &b[20], 4);
                        std::fill(a.begin() + 20, a.begin() + 24, 0), std::fill(b.begin() + 20, b.begin() + 24, 0);
                        copies += a == b && std::abs(ha - hb) <= 1e-5f * std::max(1.f, std::abs(ha));
                    }
                    check(copies == 64, key + " location copies agree (" + std::to_string(copies) + " of 64)");
                    if (std::getenv("REG_DIFF"))
                        for (int i = 0; i < 64; i++) {
                            const worldgen::Location &L = region->records[i];
                            std::printf("    loc %d type %d var %d serial %d lv %d sub %d x %lld y %lld r %g h %g:", i, L.type,
                                        L.variant, L.serial, L.level, L.subLevel, (long long)L.x, (long long)L.y, L.radius,
                                        L.height);
                            for (int b = 0; b < 44; b += 4) {
                                uint32_t v;
                                std::memcpy(&v, &g->locations[i][b], 4);
                                std::printf(" %08x", v);
                            }
                            std::printf("\n");
                        }
                }
            }
        }
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
