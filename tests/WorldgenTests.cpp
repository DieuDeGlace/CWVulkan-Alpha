// Checks the world generator against the test values written in the world-generation specs
// (docs/specs/alpha-worldgen-*.md). They were computed from the specs' formulas, so a pass means
// the implementation follows the spec; matching the running game is checked separately.
#include <limits>
#include <memory>
#include "BlockEdits.h"
#include "Missions.h"
#include "worldgen/Builders.h"
#include "worldgen/Items.h"
#include "worldgen/MapTile.h"
#include "worldgen/Prefab.h"
#include "worldgen/Random.h"
#include "worldgen/Regions.h"
#include "worldgen/Terrain.h"
#include "worldgen/Trees.h"
#include "creatures/InventoryValue.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace worldgen;

static int failures = 0;

static void same(double got, double want, const std::string &what, double tolerance = 0) {
    const bool ok = tolerance == 0 ? got == want : std::abs(got - want) <= tolerance;
    if (!ok) {
        failures++;
        std::printf("FAIL %s: got %.17g, want %.17g\n", what.c_str(), got, want);
    }
}

int main() {
    // Random spec §1.3 and §2.2 (seed 123).
    Random r(123);
    same(r.draw(), 440, "draw 1");
    same(r.draw(), 19053, "draw 2");
    same(r.draw(), 23075, "draw 3");
    const SeedOffsets o = makeSeedOffsets(123);
    same(o.A(), 440, "A");
    const int32_t o168[] = {19053, 23075, 13104, 32363};
    for (int i = 0; i < 4; i++)
        same(o[0x168 + 4 * i], o168[i], "O168 group " + std::to_string(i));
    const int32_t o18c[] = {3265,  30749, 32678, 9760,  28064, 20429, 750,  4480, 3863, 30429,
                            18586, 30758, 31595, 31567, 24630, 16059, 2734, 703,  3966, 12383};
    for (int i = 0; i < 20; i++)
        same(o[0x18c + 4 * i], o18c[i], "O18c group " + std::to_string(i));
    same(o[0x1dc], 18065, "O1dc");
    same(o[0x1e0], 28392, "O1e0");
    same(o[0x1e4], 29902, "O1e4");
    same(o[0x1f8], 10421, "O1f8");
    same(o[0x274], 26554, "O274");
    same(o[0x280], 10611, "O280");
    same(o[0x178], 25656, "O178");
    same(o[0x17c], 17603, "O17c");

    // Random spec §3.3: the noise kernel.
    same(noise(0, 0), -0.28179097175598145, "N(0, 0)");
    same(noise(0.5, 0), -0.4591194987297058, "N(0.5, 0)");
    same(noise(0, 0.5), -0.43308860063552856, "N(0, 0.5)");
    same(noise(1.25, 2.75), 0.14833128452301025, "N(1.25, 2.75)");
    same(noise(-0.3, 0), -0.3548884093761444, "N(-0.3, 0)");
    same(noise(100.5, 200.25), 0.39360496401786804, "N(100.5, 200.25)");
    same(noise(4198.464, 3423), 0.5244963765144348, "N(4198.464, 3423)");

    // Regions spec §2.3: region cells for seed 123.
    World world(123);
    struct CellCase {
        int rx, ry, x, y;
        double m, w;
        int e;
    } cells[] = {
        {512, 512, 8397824, 8397824, 0.4915097653865814, 0.5601367354393005, 74},
        {513, 512, 8408064, 8395776, 0.5406934022903442, 0.7547471523284912, -100},
        {512, 513, 8403968, 8412160, 0.9370158910751343, 0.9595354795455933, -100},
        {511, 511, 8377344, 8381440, 0.4052278399467468, 0.6399243474006653, 56},
        {510, 512, 8371200, 8397824, 0.9375194311141968, 0.9097689390182495, -100},
        {514, 514, 8426496, 8428544, 0.664085865020752, 0.40054935216903687, 45},
        {509, 509, 8342528, 8354816, 0.9998779296875, 0.09554429352283478, 58},
    };
    for (const auto &c : cells) {
        const RegionCell *cell = world.makeCell(c.rx, c.ry);
        const std::string name = "cell (" + std::to_string(c.rx) + ", " + std::to_string(c.ry) + ")";
        if (!cell) {
            failures++;
            std::printf("FAIL %s missing\n", name.c_str());
            continue;
        }
        same(cell->x, c.x, name + " x");
        same(cell->y, c.y, name + " y");
        same(cell->moisture, c.m, name + " moisture");
        same(cell->warmth, c.w, name + " warmth");
        same(cell->elevation, c.e, name + " elevation");
        same(cell->fire, 0, name + " fire");
    }

    // Regions spec §3.1 and §4.
    world.makeCells(512, 512);
    const glm::vec2 d = world.displacement(HOME, HOME);
    same(d.x, 402.813232421875, "displacement x at home");
    same(d.y, 92.43751525878906, "displacement y at home");
    struct BlendCase {
        int x, y;
        double warmth, moisture;
    } blends[] = {
        {8396928, 8396928, 0.5601367354393005, 0.4915097653865814},
        {8402400, 8396800, 0.6633611917495728, 0.5175975561141968},
        {8402440, 8396800, 0.6896752715110779, 0.5242478847503662},
        {8402480, 8396800, 0.733677864074707, 0.5353685617446899},
        {8402520, 8396800, 0.7547471523284912, 0.5406934022903442},
    };
    for (const auto &b : blends) {
        const std::string at = " at (" + std::to_string(b.x) + ", " + std::to_string(b.y) + ")";
        same(world.warmth(b.x, b.y), b.warmth, "warmth" + at);
        same(world.moisture(b.x, b.y), b.moisture, "moisture" + at);
    }

    // Terrain spec §6.2 and §6.5: colours.
    const Terrain terrain(world);
    auto colour = [&](glm::vec3 got, glm::vec3 want, const std::string &what) {
        for (int i = 0; i < 3; i++)
            same(got[i], want[i], what + "[" + std::to_string(i) + "]");
    };
    colour(terrain.grass(0, 0), {109.60842895507812f, 207.68994140625f, 0}, "grass(0, 0)");
    colour(terrain.grass(HOME, HOME), {163.84413146972656f, 224.4915313720703f, 0}, "grass at home");
    colour(terrain.snow(0, 0), {132.31939697265625f, 194.3641815185547f, 255}, "snow(0, 0)");
    colour(terrain.snow(1000, 2000), {138.51609802246094f, 197.11827087402344f, 255}, "snow(1000, 2000)");

    // Locations spec §12.1: the home region of seed 123, a named world.
    {
        World fresh(123);
        const Terrain heights(fresh);
        heights.prepareRegion(512, 512);
        const Region *home = fresh.region(512, 512);
        if (!home) {
            failures++;
            std::printf("FAIL home region not prepared\n");
        } else {
            same(home->draws, 308, "home region draws");
            same(home->tier, 1, "tier");
            same(home->style, 3, "style");
            struct RecordCase {
                int index, type;
                int64_t x, y;
                float r, h;
                int variant, serial, level, subLevel;
            } records[] = {
                {5, 12, 549814534144LL, 550485622784LL, 128, 142.8507537841797f, 0, 764, 26, 2},
                {6, 4, 549822681812LL, 550628568518LL, 706, 190.33050537109375f, 0, 901, 87, 3},
                {7, 0, 549822854785LL, 550762547407LL, 766, 213.23287963867188f, 0, 0, 1, 0},
                {12, 10, 549956491734LL, 550360442409LL, 729, 152.62640380859375f, 0, 25782, 1, 0},
                {18, 5, 550086636472LL, 550079029984LL, 316, 237.089599609375f, 0, 574, 10, 0},
                {20, 14, 550082969600LL, 550351405056LL, 150, 187.16929626464844f, 1, 1044, 621, 0},
                {28, 5, 550227107042LL, 550359015900LL, 398, 179.47958374023438f, 0, 274, 1, 0},
                {36, 1, 550359793664LL, 550359793664LL, 516, 187.8577880859375f, 3, 525240, 1, 0},
                {53, 6, 550628858773LL, 550494084720LL, 667, 170.71372985839844f, 0, 320, 2, 0},
                {55, 14, 550636617728LL, 550754058240LL, 150, 245.3907470703125f, 2, 909, 109, 0},
            };
            for (const auto &c : records) {
                const Location &L = home->records[c.index];
                const std::string name = "record " + std::to_string(c.index);
                same(L.type, c.type, name + " type");
                same((double)L.x, (double)c.x, name + " x");
                same((double)L.y, (double)c.y, name + " y");
                same(L.radius, c.r, name + " r");
                same(L.height, c.h, name + " h", 1e-3); // (last bits may differ: spec open question 4)
                same(L.variant, c.variant, name + " variant");
                same(L.serial, c.serial, name + " serial");
                same(L.level, c.level, name + " level");
                same(L.subLevel, c.subLevel, name + " sub-level");
            }
            struct EntryCase {
                int zxl, zyl, kind, variant, id, level, subLevel;
            } entries[] = {
                {19, 12, 3, 0, 25357, 18, 1}, {19, 35, 3, 1, 26548, 621, 4}, {52, 59, 3, 2, 31418, 109, 4},
                {11, 36, 4, 0, 25782, 1, 0},  {36, 35, 1, 1, 0, 1, 0},       {35, 36, 1, 4, 0, 1, 0},
                {3, 43, 0, 0, 0, 26, 0},
            };
            for (const auto &e : entries) {
                const ZoneEntry &E = home->zones[64 * e.zxl + e.zyl];
                const std::string name = "zone (" + std::to_string(e.zxl) + ", " + std::to_string(e.zyl) + ")";
                same(E.kind, e.kind, name + " kind");
                same(E.variant, e.variant, name + " variant");
                same(E.id, e.id, name + " id");
                same(E.level, e.level, name + " level");
                same(E.subLevel, e.subLevel, name + " sub-level");
            }
        }
    }

    // Trees spec §3.1 and §8: moss, and the worked kind-0 tree.
    {
        Zone zone;
        for (auto &c : zone.columns) {
            c.warmth = 0.5f;
            c.moisture = 1.f;
            c.bottom = 1;
            c.blocks.assign(100, Block{255, 255, 255, 0});
        }
        Random unused(1);
        TreeBuilder probe(terrain, zone, unused);
        same(probe.moss(0, 0, 0), 0.6660673022270203, "moss(0, 0, 0)");
        same(probe.moss(10, 20, 30), 0.07561349868774414, "moss(10, 20, 30)");
        same(probe.moss(100, 200, 50), 0.2500426769256592, "moss(100, 200, 50)");
        same(probe.moss(900, 1900, 55), 0, "moss below zero is 0");
        for (auto &c : zone.columns)
            c.moisture = 0.45f;
        Random random(123);
        TreeBuilder(terrain, zone, random).build(100, 200, 50, 8, 10, 0);
        same(random.draw(), 27840, "57 draws, then the next one");
        auto cell = [&](int x, int y, int z, int r, int g, int b, int type, const std::string &what) {
            const Block k = zone.read(x, y, z);
            same(k.r, r, what + " r");
            same(k.g, g, what + " g");
            same(k.b, b, what + " b");
            same(k.type(), type, what + " type");
        };
        cell(100, 200, 50, 218, 177, 98, WOOD, "trunk foot");
        cell(100, 200, 58, 220, 173, 98, WOOD, "trunk at 58");
        cell(100, 200, 61, 222, 174, 98, WOOD, "branch ball at 61");
        cell(104, 204, 62, 53, 220, 60, LEAVES, "leaf blob");
        cell(109, 197, 59, 47, 217, 60, LEAVES, "last leaves");
        same(zone.read(100, 200, 65).type(), AIR, "nothing above the crown");
    }

    // Caves spec §10: formations, roads and terraces.
    same(formationTop(8397000, 8397100, 180, 25, false), 211, "mound top");
    same(formationTop(8397010, 8397100, 180, 25, false), 211, "mound top at X + 10");
    {
        const glm::vec2 uv = formationUV(FormationTest::Mound, 8397010, 8397100, 190, 8397000, 8397100, 24, 22);
        same(uv.x, 0.39192742109298706, "mound u");
        same(uv.y, 0.073123499751091, "mound v");
        same((float)(uv.y * uv.y + uv.x * uv.x), 0.15895414352416992, "mound u2 + v2");
    }
    same(formationTop(8397000, 8397100, 180, 12, true), 195, "mesa top");
    {
        const glm::vec2 uv = formationUV(FormationTest::MesaFill, 8397010, 8397100, 190, 8397000, 8397100, 24, 22);
        same(uv.x, 0.8065943717956543, "mesa u");
        same(uv.y, -0.09695065766572952, "mesa v");
        same((float)(uv.y * uv.y + uv.x * uv.x), 0.6599938869476318, "mesa u2 + v2");
    }
    same(roadShade(8397000, 8397100), 144.53866577148438, "road shade with cobble");
    same(roadShade(8397002, 8397101), 160.3988800048828, "road shade without cobble");
    same(roadShade(1000, 2000), 126.07621002197266, "road shade at (1000, 2000)");
    struct TerraceCase {
        float h;
        int zb;
        double t, a;
        int zlow, zw;
    } terraces[] = {
        {12.0f, 10, 0.4000000059604645, 0.800000011920929, 8, 10},
        {13.0f, 10, 0.6000000238418579, 0.5999999046325684, 9, 10},
        {13.9f, 10, 0.7799999117851257, -0.22559940814971924, 13, 11},
        {14.6f, 10, 0.9200000762939453, -0.8976001739501953, 16, 14},
        {17.2f, 15, 0.44000014662742615, 0.8800002932548523, 12, 15},
    };
    for (const auto &c : terraces) {
        const Terrace T = terraceAt(c.h);
        const std::string name = "terrace h " + std::to_string(c.h);
        same(T.zb, c.zb, name + " zb");
        same(T.t, c.t, name + " t");
        same(T.a, c.a, name + " a");
        same(T.zlow, c.zlow, name + " zlow");
        same(T.zw, c.zw, name + " zw");
    }
    same(noise(0.02 * 8397000 + 55432.0, 0.02 * 8397100 + 974.0), 0.11888830363750458, "clearance noise");

    // Structures spec §3 and §4: boulder noise and the stone circle's pillars.
    same(noise(0.05 * 8397000, 0.05 * 8397100), -0.4843904376029968, "boulder noise");
    same((float)(noise(0.05 * 8397000, 0.05 * 8397100) * 0.8f), -0.38751235604286194, "boulder k");
    const int pillars6[][2] = {{25, 0}, {12, 21}, {-12, 21}, {-25, 0}, {-12, -21}, {12, -21}};
    const int pillars7[][2] = {{25, 0}, {15, 19}, {-5, 24}, {-22, 10}, {-22, -10}, {-5, -24}, {15, -19}};
    const int pillars8[][2] = {{25, 0}, {17, 17}, {0, 25}, {-17, 17}, {-25, 0}, {-17, -17}, {0, -25}, {17, -17}};
    auto pillars = [&](int n, const int (*want)[2]) {
        for (int k = 0; k < n; k++) {
            const std::string name = "pillar " + std::to_string(k) + " of " + std::to_string(n);
            same(pillarOffset(k, n).x, want[k][0], name + " x");
            same(pillarOffset(k, n).y, want[k][1], name + " y");
        }
    };
    pillars(6, pillars6);
    pillars(7, pillars7);
    pillars(8, pillars8);
    {
        // §4 inside test at (8397060, 8397095, 205), pillar (8397058, 8397096), zp 200, a1 4, a2 5.
        const float ny = (float)(noise(0.05 * 8397095, 0.05 * 8397095) * 0.3f);
        const float v = (float)((float)(8397095 - 8397096) / 5.f + ny);
        const float u = (float)((float)(noise(0.05 * 8397060, 0.05 * 205) * 0.3f) + (float)(8397060 - 8397058) / 4.f);
        same(noise(0.05 * 8397095, 0.05 * 8397095), 0.2656570374965668, "pillar noise y");
        same(noise(0.05 * 8397060, 0.05 * 205), -0.6990781426429749, "pillar noise x");
        same(u, 0.29027652740478516, "pillar u");
        same(v, -0.12030288577079773, "pillar v");
    }

    // Items spec §7: the generators from the srand(123) stream.
    {
        Random s(123);
        const ItemRecord w = weaponItem(s, 5, 1, -1);
        same(s.count(), 16, "weapon draws");
        same(w.type, 3, "weapon type");
        same(w.subtype, 8, "weapon subtype");
        same(w.material, 2, "weapon material");
        same(w.seed, 29, "weapon seed");
        same(w.rarity, 1, "weapon rarity");
    }
    {
        Random s(123);
        const ItemRecord a = armourItem(s, 5, 1, -1);
        same(s.count(), 24, "armour draws");
        same(a.type, 6, "armour type");
        same(a.material, 27, "armour material");
        same(a.seed, 67, "armour seed");
    }
    {
        Random s(123);
        const ItemRecord r = recipeItem(s, 5, 0);
        same(s.count(), 25, "recipe draws");
        same(r.type, 2, "recipe type");
        same(r.product, 7, "recipe product");
        same(r.material, 27, "recipe material");
        same(r.seed, 86, "recipe seed");
        same(r.level, 5, "recipe level");
    }
    {
        Random s(123);
        const ItemRecord l = lootItem(s, 5, 1);
        same(s.count(), 136, "loot draws");
        same(l.type, 5, "loot type");
        same(l.material, 25, "loot material");
        same(l.rarity, 1, "loot rarity");
        same(l.seed, 0, "loot seed");
        Random s1(1);
        const ItemRecord l1 = lootItem(s1, 5, 1);
        same(s1.count(), 128, "loot draws, srand(1)");
        same(l1.type * 100 + l1.subtype, 302, "loot item, srand(1)");
        Random s37(37);
        const ItemRecord l37 = lootItem(s37, 5, 1);
        same(l37.type * 100 + l37.product, 203, "loot recipe, srand(37)");
        same(l37.subtype, 4, "loot recipe subtype, srand(37)");
        same(l37.seed, 55, "loot recipe seed, srand(37)");
    }

    // Structures spec §6.2: the prefab placer on a small model in an empty zone.
    {
        Zone zone;
        zone.zx = zone.zy = 0;
        for (Column &c : zone.columns) {
            c.bottom = 0;
            c.blocks.assign(10, Block{90, 90, 90, GRASS});
            c.warmth = 1;
        }
        Prefab model;
        model.sx = 4, model.sy = 2, model.sz = 3;
        model.rgb.assign(4 * 2 * 3 * 3, 0);
        auto put = [&](int i, int j, int k, uint8_t r, uint8_t g, uint8_t b) {
            uint8_t *c = &model.rgb[3 * ((k * 2 + j) * 4 + i)];
            c[0] = r, c[1] = g, c[2] = b;
        };
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 2; j++) {
                put(i, j, 0, 120, 100, 80); // floor
                put(i, j, 2, 120, 100, 80); // roof
            }
        put(0, 0, 1, 255, 0, 0); // a door two voxels wide along x
        put(1, 0, 1, 255, 0, 0);
        put(3, 1, 1, 0, 127, 0); // a vase: 2 draws
        PlacerWorld pw;
        pw.warmth = [](int32_t, int32_t) { return 1.f; };
        pw.level = [](int32_t, int32_t) { return 3; };
        pw.gapFill = [](int32_t, int32_t, int32_t) { return IMPLICIT_SOLID; };
        Placement at;
        at.x = 100, at.y = 100, at.z = 10;
        at.rotation = 1;
        at.mode = 1;
        Random s(123);
        placePrefab(zone, s, pw, model, at);
        same(s.count(), 2, "placer draws");
        same(zone.statics.size(), 2, "placer statics");
        // The door: model x 0..1 at j = 0 turns (r = 1) to world y = 103..102 at x = 100.
        same(zone.statics[0].kind, 1, "door kind");
        same((double)zone.statics[0].x / 65536, 100.5, "door x");
        same((double)zone.statics[0].y / 65536, 103, "door y");
        same(zone.statics[0].rotation, 1, "door turned");
        same(zone.statics[0].scale[0], 2, "door width");
        same(zone.read(100, 103, 11).typeByte, DRY_AIR, "door air");
        same(zone.read(100, 103, 10).typeByte, ROCK | DRY_AIR, "floor block");
        same(zone.statics[1].kind, 56 + 440 % 9, "vase kind");
    }

    // Held column-value contract: an index outside 0..1023 in the 3 x 3 window gives 0.5.
    {
        World cw(123);
        same(cw.warmth(0, 8000000) == 0.5f && cw.warmth(8000000, 0) == 0.5f && cw.warmth(1023 * 16384, 8000000) == 0.5f, 1,
             "column value falls back to one half at the world index edges");
    }

    // Held template-stamping contract: guard gear levels, raw orientation, ordered column value.
    {
        auto run = [&](int mode, int rotation, float warmth, uint8_t r, uint8_t g, uint8_t b) {
            auto zone = std::make_shared<Zone>();
            zone->zx = zone->zy = 0;
            for (Column &c : zone->columns) {
                c.bottom = 0;
                c.blocks.assign(10, Block{90, 90, 90, GRASS});
                c.warmth = warmth;
            }
            Prefab model;
            model.sx = model.sy = 1, model.sz = 3;
            model.rgb = {120, 100, 80, r, g, b, 120, 100, 80};
            PlacerWorld pw;
            pw.warmth = [&](int32_t, int32_t) { return warmth; };
            pw.level = [](int32_t, int32_t) { return 3; };
            pw.gapFill = [](int32_t, int32_t, int32_t) { return IMPLICIT_SOLID; };
            Placement at;
            at.x = at.y = 50, at.z = 10;
            at.rotation = rotation;
            at.mode = mode;
            Random s(123);
            placePrefab(*zone, s, pw, model, at);
            return zone;
        };
        const auto guard = run(9, 0, 1, 127, 0, 127);
        bool levels = guard->spawns.size() == 1;
        for (int slot : {2, 3, 4, 5, 7})
            levels &= levels && guard->spawns[0].equipment[slot].level == 20;
        same(levels, 1, "every guard equipment block takes level 20");
        same(run(9, 5, 1, 127, 0, 127)->spawns[0].yaw, (2 - 5) * 90.f, "creature yaw uses the raw orientation");
        same(run(1, 5, 1, 63, 0, 0)->sprites[0].angle, -90.f * 5, "crest angle uses the raw orientation");
        // A lone wall-plant voxel in open air above the column.
        auto plant = [&](float warmth) {
            auto zone = std::make_shared<Zone>();
            zone->zx = zone->zy = 0;
            for (Column &c : zone->columns) {
                c.bottom = 0;
                c.blocks.assign(10, Block{90, 90, 90, GRASS});
                c.warmth = warmth;
            }
            Prefab model;
            model.sx = model.sy = model.sz = 1;
            model.rgb = {0, 127, 127};
            PlacerWorld pw;
            pw.warmth = [&](int32_t, int32_t) { return warmth; };
            pw.level = [](int32_t, int32_t) { return 3; };
            pw.gapFill = [](int32_t, int32_t, int32_t) { return IMPLICIT_SOLID; };
            Placement at;
            at.x = at.y = 50, at.z = 10;
            at.mode = 1;
            Random s(123);
            placePrefab(*zone, s, pw, model, at);
            return std::make_pair(s.count(), zone->read(50, 50, 10).typeByte);
        };
        const auto warm = plant(1), unordered = plant(std::numeric_limits<float>::quiet_NaN());
        same(warm.first >= 1 && warm.second == DRY_AIR, 1, "a warm column draws and clear-writes the wall-plant cell");
        same(unordered.first == 0 && unordered.second != DRY_AIR, 1,
             "an unordered column value skips the wall-plant marker without a draw or write");
    }

    // Screens §5.1.6: the map tile of a flat zone of grass up to z = 70, floor level 62.
    {
        World tw(123);
        Terrain tt(tw);
        Zone zone;
        zone.zx = zone.zy = 32800;
        for (Column &c : zone.columns) {
            c.bottom = 0;
            c.blocks.assign(71, Block{60, 160, 40, GRASS});
            c.belowSurface = 62;
        }
        const MapTile tile = buildMapTile(zone, tt);
        same(tile.base, 7, "tile base (lo 61)");
        same(tile.sz, 2, "tile layers ((71 - 61) / 8 + 1)");
        auto voxel = [&](int x, int y, int k) {
            const uint8_t *p = &tile.rgb[3 * ((k * 32 + y) * 32 + x)];
            return glm::ivec3(p[0], p[1], p[2]);
        };
        same(voxel(3, 4, 1) == glm::ivec3(60, 160, 40), 1, "tile top voxel (70 / 8 - 7 = 1)");
        same(voxel(3, 4, 0) == glm::ivec3(60, 160, 40), 1, "tile body voxel");
    }

    // Structures spec §1: whole dungeons, one per style, in a real world.
    {
        World dw(123);
        Terrain dt(dw);
        const int32_t rx = HOME / 16384, ry = HOME / 16384;
        dt.prepareRegion(rx, ry);
        const Region *region = dw.region(rx, ry);
        bool seen[6] = {};
        int built = 0;
        for (int32_t n = 0; region && n < 64 * 64 && built < 2; n++) {
            const ZoneEntry &E = region->zones[n];
            if (E.kind != 3 || E.variant > 5 || seen[E.variant])
                continue;
            seen[E.variant] = true;
            const int32_t zx = rx * 64 + n / 64, zy = ry * 64 + n % 64;
            const auto zone = dt.generate(zx, zy);
            bool exit = false;
            for (const ZoneMarker &m : zone->markers)
                exit = exit || m.type == 5;
            const std::string name = "dungeon v" + std::to_string(E.variant);
            for (const ZoneMarker &m : zone->markers)
                if (m.type == 5)
                    std::printf("%s: zone (%d, %d), exit at (%lld, %lld, %lld), %zu creatures\n", name.c_str(), zx, zy,
                                (long long)(m.x >> 16), (long long)(m.y >> 16), (long long)(m.z >> 16), zone->spawns.size());
            same(exit, 1, name + " exit marker");
            same(!zone->spawns.empty(), 1, name + " creatures");
            same(!zone->statics.empty(), 1, name + " furniture");
            built++;
        }
        same(built > 0, 1, "a dungeon zone found");
    }

    // Structures spec §5: the four quarter zones of a town, which hold its market, class houses,
    // tower and well.
    {
        World tw(123);
        Terrain tt(tw);
        const int32_t rx = HOME / 16384, ry = HOME / 16384;
        tt.prepareRegion(rx, ry);
        const Region *region = tw.region(rx, ry);
        int quarters = 0;
        size_t statics = 0, houses = 0, seats = 0;
        bool seatsRight = true;
        bool stockValues = true;
        size_t vendors = 0;
        for (int32_t n = 0; region && n < 64 * 64 && quarters < 4; n++) {
            const ZoneEntry &E = region->zones[n];
            if (E.kind != 1)
                continue;
            const int32_t zx = rx * 64 + n / 64, zy = ry * 64 + n % 64;
            const auto zone = tt.generate(zx, zy);
            std::printf("town quarter %d: zone (%d, %d), %zu statics, %zu spawns, %zu sprites\n", (int)E.variant, zx, zy,
                        zone->statics.size(), zone->spawns.size(), zone->sprites.size());
            statics += zone->statics.size();
            for (const auto &spawn : zone->spawns)
                if (spawn.characterClass >= 0x80 && spawn.characterClass <= 0x82) {
                    ++vendors;
                    stockValues &= spawn.inventory.empty() && spawn.inventoryValue &&
                                   spawn.inventoryValue->groups.size() == 1 && !spawn.inventoryValue->groups[0].empty();
                    if (spawn.inventoryValue)
                        for (const auto &group : spawn.inventoryValue->groups)
                            for (const auto &entry : group) stockValues &= entry.count > 0;
                }
            for (const worldgen::House &h : zone->houses) { // structures §5.5.4 step 16
                houses++;
                seats += h.seats.size() + h.beds.size();
                for (int32_t k : h.seats)
                    seatsRight &= zone->statics[(size_t)k].kind == 16 || zone->statics[(size_t)k].kind == 18;
                for (int32_t k : h.beds)
                    seatsRight &= zone->statics[(size_t)k].kind == 19;
            }
            quarters++;
        }
        same(quarters > 0, 1, "a town found");
        same(statics > 0, 1, "town objects");
        std::printf("%zu furnished houses, %zu seats and beds%c", houses, seats, 10);
        same(houses > 0, 1, "furnished houses kept");
        same(seatsRight, 1, "seats are stools and benches, beds are beds");
        same(stockValues, 1, "generated vendors retain owned grouped stock instead of a flat shadow");
        std::printf("%zu vendor descriptors checked for full grouped stock\n", vendors);
    }

    // Block edits §13.1: a bomb on flat natural ground (top block z 64) removes 955 blocks.
    {
        auto ground = [](int32_t, int32_t, int32_t z) {
            return z <= 64 && z >= 40 ? Block{90, 140, 40, (uint8_t)(GRASS | SURFACE_COLOURED)} : Block{255, 255, 255, AIR};
        };
        const auto broken = breakCells(rawBlocks({8398402.5, 8397472.5, 65.4}), 8, CellRounding::FloorAdjusted, 5, ground);
        const auto crater = removals(broken);
        same((double)crater.size(), 955, "bomb crater records");
        int layer64 = 0, layer58 = 0;
        for (const BlockEdit &e : crater)
            layer64 += e.z == 64, layer58 += e.z == 58;
        same(layer64, 193, "crater top layer");
        same(layer58, 45, "crater bottom layer");
        const std::array<uint8_t, 20> first = {0x3b, 0x26, 0x80, 0, 0x9d, 0x22, 0x80, 0, 0x3f, 0, 0, 0,
                                               0xff, 0xff, 0xff, 0, 5, 0, 0, 0};
        same(crater.front().bytes() == first, 1, "first crater record bytes");
        same(crater.back().x == 8398409 && crater.back().y == 8397475 && crater.back().z == 64, 1, "last crater record");
        same(BlockEdit::parse(first.data()) == crater.front(), 1, "record round trip");
        // A structure block (no flag 0x20) survives.
        auto built = [](int32_t, int32_t, int32_t z) { return z <= 64 ? Block{9, 9, 9, PLACED_BLOCK} : Block{255, 255, 255, AIR}; };
        same((double)breakCells(rawBlocks({8398402.5, 8397472.5, 65.4}), 8, CellRounding::FloorAdjusted, 5, built).size(), 0,
             "bombs spare structures");
        // Strike-cell-helpers §8.1 step 6: the burst is at the unsnapped cell position, in the
        // tested cell's colour.
        const std::array<int64_t, 3> centre = rawBlocks({8398402.5, 8397472.5, 65.4});
        same(broken.front().at[0] == centre[0] - 7 * 65536 && broken.front().at[1] == centre[1] - 3 * 65536 &&
                 broken.front().at[2] == centre[2] - 2 * 65536,
             1,
             "first burst at the unsnapped offset position");
        same(broken.front().colour.g, 140, "burst colour from the tested cell");
    }
    // Strike-cell-helpers §2.1, §5.3: the test cell uses floor-adjusted division (an exact
    // negative multiple goes one lower); the strike records by truncation, the blast by the
    // floor-adjusted index.
    {
        std::vector<glm::ivec3> asked;
        auto natural = [&](int32_t x, int32_t y, int32_t z) {
            asked.push_back({x, y, z});
            return Block{1, 2, 3, (uint8_t)(GRASS | SURFACE_COLOURED)};
        };
        const std::array<int64_t, 3> exact{10 * 65536, 10 * 65536, -65536}, half{10 * 65536, 10 * 65536, -32768};
        const auto strikeExact = breakCells(exact, 1, CellRounding::Truncate, 0, natural);
        same(asked.size() == 1 && asked[0].z == -2, 1, "exact -65536 is tested at -2");
        same(strikeExact.size() == 1 && strikeExact[0].removal.z == -1, 1, "the strike records it at -1");
        same(breakCells(exact, 1, CellRounding::FloorAdjusted, 0, natural)[0].removal.z, -2, "the blast records -2");
        const auto strikeHalf = breakCells(half, 1, CellRounding::Truncate, 0, natural);
        same(strikeHalf[0].removal.z, 0, "raw -32768: the strike records 0 (tested at -1)");
        same(breakCells(half, 1, CellRounding::FloorAdjusted, 0, natural)[0].removal.z, -1, "the blast records -1");
        same((double)breakCells(half, 0, CellRounding::Truncate, 0, natural).size(), 0, "radius 0 breaks nothing");
    }
    // §13.2: blueprint 0 (10 x 10 x 6: five grey layers under wood) turned once.
    {
        CubModel slab;
        slab.sx = slab.sy = 10, slab.sz = 6;
        for (int k = 0; k < 6; k++)
            for (int n = 0; n < 100; n++)
                for (int c = 0; c < 3; c++)
                    slab.rgb.push_back(k < 5 ? 165 : (uint8_t)(c == 0 ? 101 : c == 1 ? 40 : 0));
        const int64_t P[3] = {(int64_t)(8398402.3 * 65536), (int64_t)(8397472.8 * 65536), (int64_t)(65.35 * 65536)};
        const auto base = placementBase(P, {3.4f, -0.2f, -1.2f});
        same(base[0] == 8398406 && base[1] == 8397473 && base[2] == 64, 1, "placement base block");
        auto grass = [](int32_t, int32_t, int32_t) { return Block{90, 140, 40, (uint8_t)(GRASS | SURFACE_COLOURED)}; };
        const auto placed = placeBlueprint(slab, base, 1, grass);
        same((double)placed.size(), 600, "slab records");
        auto record = [](const BlueprintBlock &p) {
            return BlockEdit{p.x, p.y, p.z, p.block.r, p.block.g, p.block.b, p.block.typeByte, 5}.bytes();
        };
        const std::array<uint8_t, 20> first = {0x4a, 0x26, 0x80, 0, 0x9c, 0x22, 0x80, 0, 0x40, 0, 0, 0,
                                               0xa5, 0xa5, 0xa5, 0x46, 5, 0, 0, 0};
        const std::array<uint8_t, 20> last = {0x41, 0x26, 0x80, 0, 0xa5, 0x22, 0x80, 0, 0x45, 0, 0, 0,
                                              0x65, 0x28, 0x00, 0x46, 5, 0, 0, 0};
        same(record(placed.front()) == first, 1, "first slab record bytes");
        same(record(placed.back()) == last, 1, "last slab record bytes");
        auto taken = [](int32_t, int32_t, int32_t) { return Block{1, 1, 1, PLACED_BLOCK}; };
        same((double)placeBlueprint(slab, base, 1, taken).size(), 0, "placement skips protected blocks");
    }
    // §5: an edit of day 5 comes back on day 8, not on day 9.
    same(editKept(5, 8), 1, "edit kept on day 8");
    same(editKept(5, 9), 0, "edit dropped on day 9");
    same(editKept(-1, 900), 1, "negative days never expire");

    // Wildlife §2.5: the picker's lists and picks.
    {
        auto pick = [](float w, float m, float F, float M, int32_t h, int d) {
            const auto list = wildlifeList(w, m, F, M, h, 0);
            return list[(size_t)d % list.size()];
        };
        same((double)wildlifeList(0.5f, 0.5f, 0, 0.1f, 40, 0).size(), 38, "base + temperate");
        same(pick(0.5f, 0.5f, 0, 0.1f, 40, 1000), 69, "plant-creature");
        same(pick(0.5f, 0.5f, 0, 0.4f, 40, 1000), 105, "lemonbeetle with the rockling");
        same(pick(0.9f, 0.1f, 0, 0, 40, 7), 66, "desertrunner");
        same(pick(0.9f, 0.4f, 0, 0, 40, 7), 46, "ogre: base only");
        same(pick(0.1f, 0.5f, 0, 0, 2, 13), 46, "shore: ogre");
        same(pick(0.5f, 0.5f, 0.3f, 0, 40, 32767), 73, "fire: devourer");
        same(pick(0.5f, 0.5f, 0, 0, -5, 100), 152, "water: lantern-fish");
    }
    // Missions §10.1: with the stream of srand(1), level 1 + 41 mod 10 = 2 and, in a mild
    // climate, the reptile monster (18467 mod 2 = 1).
    {
        Random r(1);
        same(1 + r.draw() % 10, 2, "monster level");
        RegionCell mild;
        mild.warmth = 0.5f, mild.moisture = 0.5f;
        same(monsterForLevel(&mild, 2, r), 119, "monster for level 2");
        same(monsterForLevel(nullptr, 5, r), 108, "kind 3 always targets the troll");
        RegionCell cold;
        cold.warmth = 0.1f;
        Random r2(1);
        r2.draw();
        same(monsterForLevel(&cold, 2, r2), 116, "cold land: the mammoth");
    }
    // §10.3: the hunt's section-13 record and its saves.
    {
        MissionArea m;
        m.seed = 11478, m.kind = 1, m.targetType = 119, m.level = 5;
        m.targetZoneX = 32778, m.targetZoneY = 32804;
        const auto r = m.record(4097, 4100);
        const uint8_t want[56] = {0x01, 0x10, 0, 0, 0x04, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0xd6, 0x2c, 0, 0, 1, 0, 0, 0, 0x77, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0x0a, 0x80, 0, 0, 0x24, 0x80, 0, 0};
        same(std::equal(r.begin(), r.end(), want), 1, "section-13 record bytes");
        int32_t cx, cy;
        same(MissionArea::fromRecord(r.data(), cx, cy) == m && cx == 4097 && cy == 4100, 1, "record round trip");
        const std::vector<uint8_t> saved = m.save();
        const std::vector<uint8_t> wantSave = {1, 0, 0, 0, 0xd6, 0x2c, 0, 0, 1, 0, 0, 0, 0x77, 0, 0, 0, 5, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0a, 0x80, 0, 0, 0x24, 0x80, 0, 0};
        same(saved == wantSave, 1, "mission save bytes");
        same(MissionArea::parse(saved) && *MissionArea::parse(saved) == m, 1, "mission save round trip");
        MonsterArea monster{119, 2, 0, 32778, 32804};
        const std::vector<uint8_t> wantMonster = {1, 0, 0, 0, 0x77, 0, 0, 0, 2, 0, 0, 0, 0, 0x0a, 0x80, 0, 0, 0x24, 0x80, 0, 0};
        same(monster.save() == wantMonster, 1, "monster save bytes");
        // §10.6: the kill completes it; its Platinum Coins quantity is 9.
        Missions missions;
        missions.edit(4097, 4100).mission = m;
        missions.edit(4097, 4100).mission.state = 1;
        same(missions.kill({32778, 32804}, 0, false).has_value(), 0, "an unflagged creature does not count");
        same(missions.kill({32778, 32804}, 0x2000, false).has_value(), 1, "the flagged monster completes the hunt");
        same(missions.cell(4097, 4100)->mission.state, 2, "state complete");
        same(rewardPlatinumAmount(5, 0), 9, "Platinum Coins quantity");
        // §10.7: the counting rule.
        Missions town;
        MissionArea &v = town.edit(4100, 4100).mission;
        v.kind = 9, v.required = 12;
        town.kill({32800, 32800}, 0x2000, false);
        same(town.cell(4100, 4100)->mission.count == 1 && town.cell(4100, 4100)->mission.state == 1, 1, "first counted kill");
    }

    {
        // Open points 10 §11: a whole negative height reads one block lower.
        same(blockOfFixed(0), 0, "block of 0");
        same(blockOfFixed(65536), 1, "block of 1");
        same(blockOfFixed(-1), -1, "block just below 0");
        same(blockOfFixed(-5 * 65536), -6, "block of -5");
        same(blockOfFixed(-5 * 65536 - 32768), -6, "block of -5.5");
        // Open points 11 §3: the order of the products in smooth shows in the last bit.
        same(smooth(0.5f), 0.5, "smooth of a half");
        const float x = 0.3f;
        same(smooth(x), (double)((float)((float)(3.f * x) * x) - (float)((float)((float)(2.f * x) * x) * x)), "smooth order");
    }

    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}

