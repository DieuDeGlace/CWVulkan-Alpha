// The town builder (docs/specs/alpha-worldgen-structures.md §5): every zone of a town's or a
// ruin's sub-cell is cut into plots, the plots get roles (houses, a market, gardens, class
// houses, a tower, a well, ruins), and each role is built. Section numbers point at that spec.
//
// A town zone's draw count also depends on the marker voxels of the models the placer visits
// (§8 items 6, 10), so it lines up 1:1 only with the game's own models.
#include "creatures/CreatureTables.h"
#include "creatures/VendorStock.h"
#include "worldgen/Builders.h"
#include "worldgen/Items.h"
#include "worldgen/Prefab.h"
#include "worldgen/Trees.h"
#include <algorithm>
#include <cmath>

namespace worldgen {
namespace {

constexpr int64_t ONE = 65536;
constexpr int64_t HALF = 32768;

// §5.2: one plot's record.
struct Plot {
    int32_t lo = INT32_MAX, hi = INT32_MIN, inner = -10000;
    int32_t kind = 0, sub = 0, face = 0;
    float wc = 0;
    int32_t ox = 0, oy = 0;
};

enum PlotKind {
    NOTHING = 0,
    HOUSE = 2,
    TOWER = 3,
    LANDING_PLACE = 4,
    WELL = 5,
    GARDEN = 6,
    STREET_HOUSE = 7, // a house plot crossed by a street: listed with the houses, builds nothing
    PLINTH = 8,
    MARKET = 9,
    WARRIOR_HOUSE = 10,
    RANGER_HOUSE = 11,
    ROGUE_HOUSE = 12,
    MAGE_HOUSE = 13,
    SMITHY = 14,
    WEAVING_MILL = 15,
    CARPENTER = 16,
    SPHINX = 17,
    ANTIQUE_BUILDING = 18,
    OBELISK = 19,
    CENTRE_RUIN = 20,
};

class TownBuilder : StructureWriter {
  public:
    TownBuilder(const Terrain &terrain, Zone &zone, Random &random, const Location &L, int32_t quarter,
                const std::function<Block(int32_t, int32_t, int32_t)> &gapFill)
        : StructureWriter{terrain, zone, random, gapFill}, L(L), Q(quarter), g(L.type == 1 ? 5 : 4), s(256 / g) {}

    void run() {
        survey();
        facing();
        if (!assignRoles())
            return;
        for (int32_t r = 0; r < g * g; r++)
            zone.townPlots.push_back({plots[r].ox, plots[r].oy, s, plots[r].kind, plots[r].sub, plots[r].face});
        for (int32_t i = 0; i < g; i++)
            for (int32_t j = 0; j < g; j++)
                build(plots[i + g * j]);
        // §5.5.8: plazas on the open plots, then §5.5.9: the people.
        for (int32_t i = 0; i < g; i++)
            for (int32_t j = 0; j < g; j++) {
                const Plot &p = plots[i + g * j];
                if (p.kind == NOTHING || p.kind == STREET_HOUSE)
                    plaza(p);
            }
        if (L.type == 1)
            townsfolk();
        else
            ruinCreatures();
    }

  private:
    // 0x5fa4c0: how much a point belongs to the town (q of terrain §3.1 at an fx point).
    float weight(int64_t X, int64_t Y) const { return bump(terrain.world().locationDistanceFx(L, X, Y)); }

    static bool stores(const Column &c, uint8_t type) {
        for (const Block &b : c.blocks)
            if (b.type() == type)
                return true;
        return false;
    }

    // §5.2.
    void survey() {
        for (int32_t i = 0; i < g; i++)
            for (int32_t j = 0; j < g; j++) {
                Plot &p = plots[i + g * j];
                p.ox = zone.originX() + Zone::SIZE * i / g;
                p.oy = zone.originY() + Zone::SIZE * j / g;
                bool busy = false, bad = false, road = false;
                for (int32_t a = 0; a < s; a++)
                    for (int32_t b = 0; b < s; b++) {
                        const int32_t X = p.ox + a, Y = p.oy + b;
                        const Column &c = zone.column(X, Y);
                        const float wv = weight((int64_t)X * ONE, (int64_t)Y * ONE);
                        busy = busy || wv > 0.1f;
                        bad = bad || c.top() < 1 || stores(c, WATER) || stores(c, LAKE_BED);
                        if (a > 7 && a < s - 7 && b > 7 && b < s - 7 && wv < 0.65f)
                            road = road || stores(c, TOWN_GROUND);
                        const int32_t h = c.top() - 1;
                        p.lo = std::min(p.lo, h);
                        p.hi = std::max(p.hi, h);
                        if (a > 7 && a < s - 8 && b > 7 && b < s - 8 && p.inner != 0)
                            p.inner = h;
                    }
                p.face = random.draw() & 3;
                p.wc = weight((int64_t)(p.ox + s / 2) * ONE, (int64_t)(p.oy + s / 2) * ONE);
                p.kind = NOTHING;
                if (!bad && busy) {
                    if (road) {
                        p.kind = STREET_HOUSE; // (a steep street plot stays one)
                    } else {
                        p.kind = (float)random.draw() / 32767.f < p.wc + 0.25f ? HOUSE : NOTHING;
                        if (p.hi - p.lo > 16)
                            p.kind = NOTHING;
                    }
                }
            }
    }

    // §5.3: houses turn their fronts towards a plot that is not a house.
    void facing() {
        for (int32_t i = 0; i < g; i++)
            for (int32_t j = 0; j < g; j++) {
                Plot &p = plots[i + g * j];
                for (int32_t n = 0; n < 3; n++) {
                    static const int32_t DI[4] = {0, -1, 0, 1}, DJ[4] = {-1, 0, 1, 0};
                    const int32_t ni = i + DI[p.face & 3], nj = j + DJ[p.face & 3];
                    if (ni >= 0 && nj >= 0 && ni < g && nj < g && plots[ni + g * nj].kind != HOUSE)
                        break;
                    p.face++;
                }
            }
    }

    // §5.4: gardens, then the special plots taken from the house list. False when the zone has
    // no house plots, which ends the builder.
    bool assignRoles() {
        if (L.type == 1)
            for (int32_t r = 0; r < g * g; r++) {
                Plot &p = plots[r];
                if (p.kind == NOTHING && p.hi - p.lo < 16 && (random.draw() & 1) == 0 && p.wc > 0)
                    p.kind = GARDEN;
            }
        std::vector<int32_t> B;
        for (int32_t r = 0; r < g * g; r++)
            if (plots[r].kind == HOUSE) // (street plots are listed apart and never used)
                B.push_back(r);
        if (B.empty())
            return false;
        std::stable_sort(B.begin(), B.end(), [&](int32_t a, int32_t b) { return plots[a].wc < plots[b].wc; });
        const size_t original = B.size();
        auto pop = [&]() -> Plot & {
            Plot &p = plots[B.back()];
            B.pop_back();
            return p;
        };
        auto pick = [&]() -> Plot & {
            const size_t n = (uint32_t)random.draw() % B.size();
            Plot &p = plots[B[n]];
            B.erase(B.begin() + (ptrdiff_t)n);
            return p;
        };
        if (L.type == 1) {
            if (Q == 1 && B.size() > 1)
                pop().kind = MARKET;
            if (Q == 3)
                for (int32_t kind : {WARRIOR_HOUSE, RANGER_HOUSE, ROGUE_HOUSE, MAGE_HOUSE})
                    if (!B.empty())
                        pick().kind = kind;
            if (Q == 2)
                for (int32_t kind : {SMITHY, CARPENTER, WEAVING_MILL})
                    if (!B.empty())
                        pick().kind = kind;
            if (Q == 3 && B.size() > 1)
                pop().kind = TOWER;
            if ((Q == 0 || Q == 3) && B.size() > 1)
                pop().sub = 1;
            if (Q == 1)
                for (int32_t sub = 2; sub <= 5 && !B.empty(); sub++)
                    pop().sub = sub;
            if (Q == 0 && !B.empty())
                pick().kind = WELL;
            for (int32_t n = 0; n < 6; n++)
                if (B.size() > 3)
                    pick().kind = NOTHING;
        } else {
            const bool centreHere = (L.x / ONE) >> 8 == zone.zx && (L.y / ONE) >> 8 == zone.zy;
            if (centreHere && original > 1)
                pop().kind = CENTRE_RUIN;
            for (int32_t n = 0; n < 2; n++)
                if (B.size() > 3)
                    pick().kind = NOTHING;
            if (L.variant == 0) {
                for (int32_t n = 0; n < 2; n++)
                    if (B.size() > 3) {
                        Plot &p = pick();
                        p.kind = ANTIQUE_BUILDING;
                        p.face = random.draw() & 3;
                    }
            } else if (L.variant == 2) {
                for (int32_t kind : {SPHINX, OBELISK})
                    if ((random.draw() & 1) && B.size() > 3) {
                        Plot &p = pick();
                        p.kind = kind;
                        p.face = random.draw() & 3;
                    }
            }
        }
        return true;
    }

    PlacerWorld placerWorld() const {
        PlacerWorld p;
        p.warmth = [this](int32_t x, int32_t y) { return terrain.world().warmth(x, y); };
        p.level = [this](int32_t x, int32_t y) { return terrain.world().levelAt(x, y); };
        p.gapFill = gapFill;
        return p;
    }

    std::shared_ptr<const Prefab> model(const char *key) const {
        return terrain.prefabs ? terrain.prefabs(key) : nullptr;
    }

    // §5.5: a plot's own block; every plot but a house also gets its open-plot creatures
    // (§5.5.10), after the blocks of kinds 8, 9, 6 and before the prefabs.
    void build(Plot &p) {
        switch (p.kind) {
        case PLINTH: plinth(p); break;
        case MARKET: market(p); break;
        case GARDEN: garden(p); break;
        case HOUSE: house(p); break;
        default: break;
        }
        if (p.kind != HOUSE)
            openPlotCreatures(p);
        switch (p.kind) {
        case TOWER:
        case LANDING_PLACE:
        case WELL:
        case WARRIOR_HOUSE:
        case RANGER_HOUSE:
        case ROGUE_HOUSE:
        case MAGE_HOUSE:
        case SMITHY:
        case WEAVING_MILL:
        case CARPENTER:
        case SPHINX:
        case OBELISK:
        case CENTRE_RUIN:
        case ANTIQUE_BUILDING: special(p); break;
        default: break;
        }
    }

    int32_t cx(const Plot &p) const { return p.ox + s / 2; }
    int32_t cy(const Plot &p) const { return p.oy + s / 2; }
    int32_t top(int32_t x, int32_t y) const { return zone.column(x, y).top(); }

    // §5.5.1: a stepped stone plinth with a goddess statue (no writer of this kind is known).
    void plinth(const Plot &p) {
        const int32_t T = top(cx(p), cy(p)) + 1;
        for (int32_t n = 4; n <= 23; n++)
            box(cx(p) - 2 - 2 * (n - 4), cy(p) - 2 - 2 * (n - 4), T - (n - 4), n, n, 1, {220, 220, 220}, 0, 0);
        StaticObject statue;
        statue.kind = 0;
        statue.x = (int64_t)cx(p) * ONE, statue.y = (int64_t)cy(p) * ONE, statue.z = (int64_t)T * ONE;
        statue.scale[0] = statue.scale[1] = 3, statue.scale[2] = 12;
        for (int guard = 0; guard < 4096 && blockSolid(cx(p), cy(p), blockOfFixed(statue.z)); guard++)
            statue.z += ONE;
        statue.rotation = random.draw() & 3;
        zone.statics.push_back(statue);
    }

    // §5.5.2: market stands round the edge of the plot, goods inside them.
    void market(const Plot &p) {
        markets.push_back({cx(p), cy(p), top(cx(p), cy(p))});
        struct Slot {
            bool stand;
            int32_t side; // 0: low y edge, 1: high y, 2: high x, 3: low x
            int32_t along;
            int32_t rotation;
        };
        static const Slot SLOTS[20] = {
            {true, 0, -7, 2}, {true, 0, 0, 2},  {true, 0, 7, 2},  {true, 1, -7, 0},  {true, 1, 0, 0},
            {true, 1, 7, 0},  {true, 2, -7, 3}, {true, 2, 0, 3},  {true, 2, 7, 3},   {true, 3, -7, 1},
            {true, 3, 0, 1},  {true, 3, 7, 1},  {false, 0, -3, 2}, {false, 0, 3, 2}, {false, 1, -3, 0},
            {false, 1, 3, 0}, {false, 2, -3, 3}, {false, 2, 3, 3}, {false, 3, -3, 1}, {false, 3, 3, 1},
        };
        for (const Slot &slot : SLOTS) {
            if (random.draw() % 5 == 0)
                continue;
            const int32_t q = random.draw() % 3;
            int32_t x, y;
            switch (slot.side) {
            case 0: x = cx(p) + slot.along, y = p.oy + 6 + q; break;
            case 1: x = cx(p) + slot.along, y = p.oy + s - 6 - q; break;
            case 2: x = p.ox + s - 6 - q, y = cy(p) + slot.along; break;
            default: x = p.ox + 6 + q, y = cy(p) + slot.along; break;
            }
            StaticObject o;
            o.x = (int64_t)x * ONE, o.y = (int64_t)y * ONE, o.z = (int64_t)p.hi * ONE;
            o.rotation = slot.rotation;
            auto scale = [&](float sx, float sy, float sz) { o.scale[0] = sx, o.scale[1] = sy, o.scale[2] = sz; };
            if (slot.stand) {
                o.kind = 21 + random.draw() % 3;
                scale(3.5f, 2, 3);
            } else {
                switch (random.draw() & 3) {
                case 0: o.kind = 24, scale(2, 2, 2); break;
                case 1: {
                    o.kind = 25;
                    const float k = f(f(f(f(random.draw() * 0.5) / 32767.f) + 1.f) * 1.5f);
                    scale(k, k, k);
                    break;
                }
                case 2: {
                    o.kind = 26;
                    const float e = f(f(f(random.draw() * 0.5) / 32767.f) + 1.f);
                    scale(1.5f * e, 1.5f * e, 0.75f * e);
                    break;
                }
                default: o.kind = 27, scale(1.5f, 1.5f, 1.4f); break;
                }
            }
            if (settle(o, zone))
                zone.statics.push_back(o);
        }
    }

    // §5.5.3: a striped crop field with a scarecrow and fences on two sides.
    void garden(const Plot &p) {
        const int32_t stripe = random.draw() & 1;
        const int32_t crop = random.draw() % 6;
        struct Crop {
            int32_t plant, item, flags;
            float size;
        };
        static const Crop CROPS[6] = {{23, 16, 0, 0.1f}, {25, 0, 4, 0.2f}, {26, 0, 0, 0.2f},
                                      {29, 0, 4, 0.2f},  {30, 0, 4, 0.2f}, {24, 17, 0, 0.15f}};
        for (int32_t u = 0; u < s; u++)
            for (int32_t w = 0; w < s; w++) {
                const int32_t x = p.ox + u, y = p.oy + w;
                Crop c = CROPS[crop];
                if (crop == 3 && random.draw() % 10 == 0)
                    c.plant = 21, c.size = 0.05f; // a cornflower among the wheat
                Column &column = zone.column(x, y);
                if (column.blocks.empty() || column.blocks.back().type() != GRASS)
                    continue;
                // The field block: the cliff colour at 0.7, every other pair of rows lifted by
                // (10, 10, 0); the old keep bit stays.
                Block &b = column.blocks.back();
                const float sel = (float)(((stripe ? w : u) >> 1) & 1);
                const glm::vec3 C = terrain.cliff(x, y, column.top() - 1, &zone);
                auto channel = [&](float c, float lift) {
                    return (uint8_t)(int32_t)std::clamp(f(f(c * 0.7f) + f(lift * sel)), 0.f, 255.f);
                };
                b.r = channel(C.r, 10), b.g = channel(C.g, 10), b.b = channel(C.b, 0);
                b.typeByte = (uint8_t)((b.typeByte & KEEP) | FIELD);
                const int32_t T = column.top();
                if (u == s / 2 && w == s / 2) {
                    Spawn scarecrow;
                    scarecrow.controller = 6;
                    scarecrow.type = 140;
                    scarecrow.level = terrain.world().levelAt(x, y);
                    scarecrow.x = (int64_t)x * ONE + HALF, scarecrow.y = (int64_t)y * ONE + HALF;
                    scarecrow.z = (int64_t)T * ONE;
                    scarecrow.yaw = (float)((random.draw() & 3) * 90);
                    zone.spawns.push_back(scarecrow);
                    continue;
                }
                if (random.draw() % 10 != 0)
                    continue;
                const int32_t k = random.draw() & 3;
                if (k != 0 || c.item == 0) {
                    Sprite plant;
                    plant.kind = c.plant;
                    plant.flags = c.flags;
                    plant.angle = c.plant == 25 ? 0.f : f((float)random.draw() * 360.f / 32767.f);
                    plant.x = (int64_t)x * ONE + HALF, plant.y = (int64_t)y * ONE + HALF, plant.z = (int64_t)T * ONE;
                    plant.size = c.size;
                    zone.sprites.push_back(plant);
                } else {
                    ItemDrop drop;
                    drop.item.type = 11;
                    drop.item.subtype = (uint8_t)c.item;
                    drop.x = (int64_t)x * ONE + HALF, drop.y = (int64_t)y * ONE + HALF, drop.z = (int64_t)T * ONE;
                    drop.rotation = f((float)random.draw() * 360.f / 32767.f);
                    drop.scale = 0.1f;
                    zone.drops.push_back(drop);
                }
            }
        // Fences along the low-y and low-x edges.
        for (int32_t run = 0; run < 2; run++) {
            if (!(random.draw() & 1))
                continue;
            for (int32_t k = 0; k < s / 2; k++) {
                const int32_t x = run == 0 ? p.ox + 2 * k + 1 : p.ox, y = run == 0 ? p.oy : p.oy + 2 * k + 1;
                if (random.draw() % 10 == 0)
                    continue;
                const Column &column = zone.column(x, y);
                if (column.blocks.empty() || column.blocks.back().type() == AIR)
                    continue;
                StaticObject fence;
                fence.kind = 52 + (random.draw() & 3);
                fence.x = (int64_t)x * ONE;
                fence.y = (int64_t)y * ONE + HALF;
                fence.z = (int64_t)column.top() * ONE;
                fence.rotation = run;
                fence.scale[0] = 2, fence.scale[1] = 1, fence.scale[2] = 1.5f;
                zone.statics.push_back(fence);
            }
        }
    }

    // §5.5.5: the buildings made of one prefab.
    void special(const Plot &p) {
        const char *key = nullptr;
        int32_t mode = 0;
        bool centred = true; // placed with the model's middle on the plot's centre
        switch (p.kind) {
        case TOWER: key = "frame-tower.cub", mode = 7, centred = false; break;
        case LANDING_PLACE: key = "landingplace.cub", mode = 7, centred = false; break;
        case WELL: key = "building-stone-well.cub", mode = 8; break;
        case WARRIOR_HOUSE: key = "building-warrior.cub", mode = 9; break;
        case RANGER_HOUSE: key = "building-ranger.cub", mode = 10; break;
        case ROGUE_HOUSE: key = "building-rogue.cub", mode = 11; break;
        case MAGE_HOUSE: key = "building-mage.cub", mode = 12; break;
        case SMITHY: key = "building-smithy.cub", mode = 13; break;
        case WEAVING_MILL: key = "building-weavingmill.cub", mode = 15; break;
        case CARPENTER: key = "building-carpentersshop.cub", mode = 14; break;
        case SPHINX: key = "sphinx01.cub"; break;
        case OBELISK: key = "obelisk.cub"; break;
        case ANTIQUE_BUILDING: { // (open points 5 §5: four models)
            static const char *const FOUR[4] = {"antique-building1.cub", "antique-building2.cub",
                                                "antique-building3.cub", "antique-building4.cub"};
            key = FOUR[random.draw() & 3];
            break;
        }
        default: key = (random.draw() & 1) ? "antique-building2.cub" : "antique-building1.cub"; break;
        }
        const std::shared_ptr<const Prefab> m = model(key);
        Placement at;
        at.rotation = p.face & 3;
        at.typeByte = ROCK;
        at.mode = mode;
        at.fillDown = true;
        if (!centred) {
            at.x = cx(p) - 16, at.y = cy(p) - 16, at.z = p.hi - 7;
        } else if (m && p.kind >= WARRIOR_HOUSE && p.kind <= CARPENTER) {
            // Class and craft houses: on the highest ground under the unturned footprint.
            int32_t Hm = 0;
            for (int32_t u = 0; u < m->sx; u++)
                for (int32_t v = 0; v < m->sy; v++)
                    if (zone.contains(cx(p) - 16 + u, cy(p) - 16 + v))
                        Hm = std::max(Hm, top(cx(p) - 16 + u, cy(p) - 16 + v));
            at.x = cx(p) - 16, at.y = cy(p) - 16, at.z = Hm - 7;
        } else if (m) {
            const bool turned = p.kind != WELL && (p.face & 1);
            const int32_t sx = turned ? m->sy : m->sx, sy = turned ? m->sx : m->sy;
            at.x = cx(p) - sx / 2, at.y = cy(p) - sy / 2;
            at.z = p.hi;
            // (The ruins' pieces settle as the well does, open points 5 §5.)
            if (p.kind == WELL || (p.kind >= SPHINX && p.kind <= CENTRE_RUIN))
                while (at.z > -1000 && !blockSolid(at.x, at.y, at.z))
                    at.z--;
        }
        if (m)
            placePrefab(zone, random, placerWorld(), *m, at);
        if (p.kind == LANDING_PLACE)
            landingAirship(p);
        // Open points 5 §3: a mana cube on the footprint's low corner column, above whatever
        // tops it. Its number decides who can see it (a player with that many cubes).
        if (p.kind == SPHINX || p.kind == OBELISK) {
            const int32_t number = random.draw() % 200;
            if (m && zone.contains(at.x, at.y)) {
                int32_t z = zone.column(at.x, at.y).top();
                while (z > -1000 && !blockSolid(at.x, at.y, z))
                    z--;
                GroundItem cube{at.x, at.y, z + 1, 25, 0, 0, 1};
                cube.seed = (uint32_t)number;
                zone.items.push_back(cube);
            }
        }
    }

    // World objects §2.1: after the landing place, an airship template when any of the eight
    // region cells round the zone's own is land (elevation > 0). Airship steering §4.1: first
    // its companion spawn record, keyed to the template, joins the zone's spawn list (the
    // finishing pass then treats it like every other spawn). No draws here.
    void landingAirship(const Plot &p) {
        bool land = false;
        for (int32_t i = -1; i <= 1; i++)
            for (int32_t j = -1; j <= 1; j++)
                if (i != 0 || j != 0)
                    if (const RegionCell *c = terrain.world().cell(zone.zx / 64 + i, zone.zy / 64 + j))
                        land = land || c->elevation > 0;
        if (!land)
            return;
        const AirshipTemplate ship =
            airshipTemplate(zone.zx, zone.zy, int32_t(zone.airships.size()), p.face, p.hi, cx(p), cy(p));
        Spawn companion;
        companion.radius = 512.f;
        companion.x = ship.target[0], companion.y = ship.target[1], companion.z = ship.target[2];
        companion.controller = 3;
        companion.type = 4;
        companion.characterClass = 0x88;
        companion.level = L.level; // +0x34 from +0x24 of the layout's record: the location's level
        companion.yaw = ship.targetYaw;
        companion.objectKey = ship.id;
        zone.spawns.push_back(companion);
        zone.airships.push_back(ship);
    }

    // ---- §5.5.10: creatures on open plots --------------------------------------------------

    // The first open z at or above a column's surface − 8 (column +0x14).
    int32_t openAbove(int32_t x, int32_t y) const {
        int32_t z = zone.column(x, y).belowSurface;
        while (blockSolid(x, y, z))
            z++;
        return z;
    }
    // "Lowest open z": from the column's bottom up while solid.
    int32_t lowestOpen(int32_t x, int32_t y) const {
        int32_t z = zone.column(x, y).bottom;
        while (blockSolid(x, y, z))
            z++;
        return z;
    }

    void openPlotCreatures(const Plot &p) {
        if (!(p.wc > 0.2f))
            return;
        for (int32_t pp : {0, s})
            for (int32_t q : {0, s}) {
                if ((random.draw() & 7) != 0)
                    continue;
                const int32_t X = p.ox + s / 4 + pp / 2, Y = p.oy + s / 4 + q / 2;
                if (!zone.contains(X, Y))
                    continue;
                Spawn c;
                c.x = (int64_t)X * ONE, c.y = (int64_t)Y * ONE, c.z = (int64_t)(openAbove(X, Y) + 1) * ONE;
                c.yaw = f(f((float)random.draw() * 360.f) / 32767.f);
                static const int32_t KINDS[5] = {34, 30, 19, 26, 33};
                c.type = KINDS[(uint32_t)random.draw() % 5];
                const bool small = c.type == 33 || c.type == 34;
                const int32_t lo = small ? 2 : 3, hi = small ? 4 : 6;
                c.level = lo + random.draw() % (hi - lo + 1);
                c.controller = 5;
                c.powerTier = (uint8_t)L.subLevel;
                zone.spawns.push_back(c);
            }
    }

    // ---- §5.5.4: modular houses -------------------------------------------------------------

    struct Cell {
        uint8_t role = 0, r = 0;
        bool entrance = false, stairs = false;
        int32_t height = 0, room = 0;
    };
    enum Role : uint8_t { NONE = 0, STOREY = 1, GROUND = 2, ROOF = 3, ARC = 5 };

    struct House {
        Cell cells[3][3][4];
        int32_t face = 0;
        bool mirror = false;
        glm::ivec3 O{0};
        int32_t sub = 0;
        std::vector<glm::ivec3> spawn, entrance, counter;
        Cell dummy;

        // 0x5a5b40: view (a, b, z) to storage; outside, a cell that reads empty.
        Cell &at(int32_t a, int32_t b, int32_t z) {
            if (a < 0 || b < 0 || z < 0 || a > 2 || b > 2 || z > 3) {
                dummy = Cell{};
                return dummy;
            }
            int32_t x, y;
            switch (face & 3) {
            case 1: x = 2 - b, y = a; break;
            case 2: x = 2 - a, y = 2 - b; break;
            case 3: x = b, y = 2 - a; break;
            default: x = a, y = b; break;
            }
            if (mirror)
                y = 2 - y;
            return cells[x][y][z];
        }
    };
    std::vector<House> houses;

    // §5.5.4 step 1: the module slots of the town's or ruin's set.
    struct ModuleSet {
        int32_t stairs, base, floor, floorStairs, wall, window, door, indoor, balcony, lamp, roof[3], arc;
    };
    ModuleSet moduleSet() const {
        auto run = [](int32_t S, int32_t first) {
            return ModuleSet{S, first, first + 1, first + 2, first + 3, first + 4, first + 5, first + 6, first + 7,
                             first + 8, {first + 9, first + 10, first + 11}, first + 12};
        };
        if (L.type == 1) {
            switch (L.variant) {
            case 1: return run(2185, 2287);
            case 2: return run(2185, 2215);
            case 3: return run(2185, 2202);
            case 4: return run(2241, 2228);
            case 5: return run(2242, 2243);
            default: return run(2185, 2189);
            }
        }
        if (L.variant == 2)
            return {2276, 2277, 2278, 2279, 2280, 2280, 2281, 2282, 2281, 2281, {2283, 2284, 2285}, 2286};
        if (L.variant == 3)
            return {2265, 2266, 2267, 2268, 2269, 2269, 2270, 2271, 2269, 2269, {2272, 2273, 2274}, 2275};
        return {2256, 2257, 2258, 2259, 2260, 2260, 2260, 2260, 2260, 2260, {2261, 2262, 2263}, 2264};
    }

    std::shared_ptr<const Prefab> slot(int32_t index) const {
        if (index < 0 || index >= creatures::MODEL_SLOTS || !creatures::MODEL_KEYS[index])
            return nullptr;
        return model((std::string(creatures::MODEL_KEYS[index]) + ".cub").c_str());
    }

    // "Place M (r, T, mode, F)" at the cell centre of view cell (a, b), Z0 given. A missing
    // model is skipped (it would make no draws but its markers').
    void placeAt(House &h, int32_t modelSlot, int32_t a, int32_t b, int32_t Z0, int32_t r, uint8_t T, int32_t mode,
                 bool fill, const int32_t *crop = nullptr, int32_t dx = 0, int32_t dy = 0) {
        const std::shared_ptr<const Prefab> m = slot(modelSlot);
        if (!m)
            return;
        Placement at;
        at.x = h.O.x + 7 + 13 * a - m->sx / 2 + dx;
        at.y = h.O.y + 7 + 13 * b - m->sy / 2 + dy;
        at.z = Z0;
        at.rotation = r; // (the statics keep it whole; the placer takes it modulo 4)
        at.typeByte = T;
        at.mode = mode;
        at.fillDown = fill;
        if (crop)
            std::copy(crop, crop + 4, at.crop);
        placePrefab(zone, random, placerWorld(), *m, at);
    }

    int32_t modelHeight(int32_t modelSlot) const {
        const std::shared_ptr<const Prefab> m = slot(modelSlot);
        return m ? m->sz : 0;
    }

    static int64_t fxOf(double v) { return (int64_t)(v * 65536.0); }

    void set(House &h, int32_t x, int32_t y, int32_t z, uint8_t role, int32_t room = 0, bool stairs = false) {
        Cell &c = h.at(x, y, z);
        c.role = role;
        if (room)
            c.room = room;
        if (stairs)
            c.stairs = true;
    }

    // §5.5.4 step 3.
    void layout(House &h) {
        if (h.sub >= 2 && h.sub <= 5) {
            set(h, 0, 0, 0, GROUND), set(h, 0, 1, 0, GROUND), set(h, 1, 1, 0, GROUND), set(h, 1, 0, 0, GROUND);
            set(h, 0, 0, 1, STOREY), set(h, 0, 1, 1, STOREY, 3), set(h, 1, 1, 1, STOREY), set(h, 1, 0, 1, STOREY);
            set(h, 0, 0, 2, ROOF), set(h, 0, 1, 2, ROOF), set(h, 1, 1, 2, ROOF), set(h, 1, 0, 2, ROOF);
        } else if (h.sub == 1) {
            set(h, 0, 0, 0, GROUND), set(h, 0, 0, 1, STOREY, 2), set(h, 0, 0, 2, STOREY, 0, true), set(h, 0, 0, 3, ROOF);
            set(h, 1, 0, 0, GROUND), set(h, 1, 0, 1, STOREY, 2), set(h, 1, 0, 2, STOREY, 1), set(h, 1, 0, 3, ROOF);
            set(h, 2, 0, 0, GROUND), set(h, 2, 0, 1, STOREY, 2), set(h, 2, 0, 2, STOREY, 1), set(h, 2, 0, 3, ROOF);
            set(h, 0, 1, 0, GROUND), set(h, 0, 1, 1, STOREY, 2), set(h, 0, 1, 2, ROOF);
            if (random.draw() & 1)
                set(h, 2, 1, 0, GROUND), set(h, 2, 1, 1, STOREY, 2), set(h, 2, 1, 2, ROOF);
            set(h, 1, 1, 0, GROUND), set(h, 1, 1, 1, STOREY, 3), set(h, 1, 1, 2, ROOF);
            set(h, 1, 2, 0, GROUND), set(h, 1, 2, 1, STOREY), set(h, 1, 2, 2, ROOF);
        } else {
            switch (random.draw() % 3) {
            case 0: // two towers
                set(h, 0, 0, 0, GROUND), set(h, 0, 0, 1, STOREY), set(h, 0, 0, 2, STOREY, 1, true), set(h, 0, 0, 3, ROOF);
                set(h, 2, 0, 0, GROUND), set(h, 2, 0, 1, STOREY), set(h, 2, 0, 2, STOREY, 0, true), set(h, 2, 0, 3, ROOF);
                set(h, 0, 1, 0, GROUND), set(h, 0, 1, 1, STOREY);
                if ((random.draw() & 1) == 0)
                    set(h, 0, 1, 2, STOREY), set(h, 0, 1, 3, ROOF);
                else
                    set(h, 0, 1, 2, ROOF);
                if (random.draw() & 1)
                    set(h, 2, 1, 0, GROUND), set(h, 2, 1, 1, STOREY), set(h, 2, 1, 2, ROOF);
                if ((random.draw() & 1) == 0)
                    set(h, 1, 0, 0, GROUND), set(h, 1, 0, 1, STOREY), set(h, 1, 0, 2, ROOF);
                else
                    set(h, 1, 0, 1, ARC);
                break;
            case 1: // an L
                set(h, 0, 0, 0, GROUND), set(h, 0, 0, 1, STOREY), set(h, 0, 0, 2, ROOF);
                set(h, 1, 0, 0, GROUND), set(h, 1, 0, 1, STOREY), set(h, 1, 0, 2, ROOF);
                set(h, 0, 1, 0, GROUND), set(h, 0, 1, 1, STOREY, 1), set(h, 0, 1, 2, ROOF);
                set(h, 1, 1, 0, GROUND), set(h, 1, 1, 1, STOREY);
                if (random.draw() & 1)
                    set(h, 1, 1, 2, STOREY, 0, true), set(h, 1, 1, 3, ROOF);
                else
                    set(h, 1, 1, 2, ROOF);
                if (random.draw() & 1)
                    set(h, 0, 2, 0, GROUND), set(h, 0, 2, 1, STOREY), set(h, 0, 2, 2, ROOF);
                break;
            default: // a bar with wings
                if ((random.draw() & 1) == 0) {
                    set(h, 0, 0, 0, GROUND), set(h, 0, 0, 1, STOREY), set(h, 0, 0, 2, STOREY, 0, true), set(h, 0, 0, 3, ROOF);
                    set(h, 1, 0, 0, GROUND), set(h, 1, 0, 1, STOREY), set(h, 1, 0, 2, STOREY), set(h, 1, 0, 3, ROOF);
                    set(h, 2, 0, 0, GROUND), set(h, 2, 0, 1, STOREY, 1), set(h, 2, 0, 2, STOREY), set(h, 2, 0, 3, ROOF);
                } else {
                    set(h, 0, 0, 0, GROUND), set(h, 0, 0, 1, STOREY), set(h, 0, 0, 2, ROOF);
                    set(h, 1, 0, 0, GROUND), set(h, 1, 0, 1, STOREY, 1), set(h, 1, 0, 2, ROOF);
                    set(h, 2, 0, 0, GROUND), set(h, 2, 0, 1, STOREY), set(h, 2, 0, 2, ROOF);
                }
                if (random.draw() & 1)
                    set(h, 0, 1, 0, GROUND), set(h, 0, 1, 1, STOREY), set(h, 0, 1, 2, ROOF);
                if (random.draw() & 1)
                    set(h, 2, 1, 0, GROUND), set(h, 2, 1, 1, STOREY), set(h, 2, 1, 2, ROOF);
                set(h, 1, 1, 0, GROUND), set(h, 1, 1, 1, STOREY), set(h, 1, 1, 2, ROOF);
                break;
            }
        }
    }

    // Helper W (0x5bf7c0): a piece of furniture against a wall, facing r.
    StaticObject wallPiece(int64_t x, int64_t y, int64_t z, int32_t r, int32_t room) {
        StaticObject o;
        o.x = x, o.y = y, o.z = z;
        o.rotation = r;
        auto scale = [&](float sx, float sy, float sz) { o.scale[0] = sx, o.scale[1] = sy, o.scale[2] = sz; };
        auto shift = [&](int64_t by) {
            switch (r & 3) {
            case 0: o.y -= by; break;
            case 1: o.x += by; break;
            case 2: o.y += by; break;
            default: o.x -= by; break;
            }
        };
        if (random.draw() % 50 == 0) {
            o.kind = 10, scale(1.2f, 0.8f, 0.8f);
            shift(HALF);
            return o;
        }
        if (room == 1) {
            if (random.draw() & 1)
                o.kind = 20, scale(1, 1, 1);
            else
                o.kind = 32 + random.draw() % 3, scale(2, 1, 1.57f);
            return o;
        }
        switch (random.draw() % 6) {
        case 0: o.kind = 32 + random.draw() % 3, scale(2, 1, 1.57f); break;
        case 1: o.kind = 18, scale(3, 1, 0.4f); break;
        case 2:
            o.kind = 16, scale(1, 1, 0.5f);
            shift((int64_t)f(f((float)random.draw() / 32767.f) * 65536.f));
            break;
        case 3: o.kind = 30, scale(3, 1, 1), shift(13107); break;
        case 4: o.kind = 29, scale(2.5f, 1, 3), shift(19660); break;
        default: o.kind = 56 + random.draw() % 9, scale(1, 1, 1); break;
        }
        return o;
    }

    // Helper G (0x5bf5b0): goods outside a house.
    StaticObject goods(int64_t x, int64_t y, int64_t z, int32_t r) {
        StaticObject o;
        o.x = x, o.y = y, o.z = z;
        o.rotation = r;
        auto scale = [&](float sx, float sy, float sz) { o.scale[0] = sx, o.scale[1] = sy, o.scale[2] = sz; };
        switch (random.draw() % 7) {
        case 0: o.kind = 24, scale(2, 2, 2); break;
        case 1: {
            o.kind = 25;
            const float k = f(f(f(f(random.draw() * 0.5) / 32767.f) + 1.f) * 1.5f);
            scale(k, k, k);
            break;
        }
        case 2: {
            o.kind = 26;
            const float e = f(f(f(random.draw() * 0.5) / 32767.f) + 1.f);
            scale(1.5f * e, 1.5f * e, 0.75f * e);
            break;
        }
        case 3: o.kind = 27, scale(1.5f, 1.5f, 1.4f); break;
        case 4: o.kind = 18, scale(3, 1, 0.4f); break;
        case 5: o.kind = 16, scale(1, 1, 0.5f); break;
        default: o.kind = 28, scale(3, 3, 2.5f); break;
        }
        return o;
    }

    // Inserted only when the block under its foot is solid ("supported").
    void supported(const StaticObject &o, int32_t floor) {
        if (blockSolid(blockOfFixed(o.x), blockOfFixed(o.y), floor - 1))
            zone.statics.push_back(o);
    }

    void house(const Plot &p) {
        const ModuleSet set = moduleSet();
        houses.emplace_back();
        House &h = houses.back();
        h.sub = p.sub;
        layout(h);
        h.face = p.face;
        h.mirror = (random.draw() & 1) == 0;

        // Step 4: storey heights; M the highest.
        int32_t M = 0;
        auto eligible = [&](int32_t a, int32_t b) {
            const Cell &c = h.at(a, b, 1);
            return c.role == STOREY && !h.at(a, b, 2).stairs && c.room != 3;
        };
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++) {
                if (!eligible(a, b))
                    continue;
                int32_t height = 0;
                for (int32_t u = 0; u < 13; u++)
                    for (int32_t v = 0; v < 13; v++) {
                        const int32_t x = p.ox + 7 + 13 * a + u, y = p.oy + 7 + 13 * b + v;
                        if (zone.contains(x, y))
                            height = std::max(height, top(x, y));
                    }
                h.at(a, b, 1).height = height;
                M = std::max(M, height);
            }
        // Step 5: the entrance.
        std::vector<glm::ivec2> highest;
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                if (eligible(a, b) && h.at(a, b, 1).height == M)
                    highest.push_back({a, b});
        if (!highest.empty()) {
            const glm::ivec2 e = highest[(uint32_t)random.draw() % highest.size()];
            h.at(e.x, e.y, 1).entrance = true;
        }
        // Step 6: rotations.
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t z = 0; z < 4; z++) {
                    Cell &c = h.at(a, b, z);
                    c.r = (uint8_t)(c.role == ARC ? (h.face & 1) : (random.draw() & 3));
                }
        // Step 7: the origin, from the ground round the entrance.
        int32_t h0 = 0;
        bool first = true;
        for (int32_t z = 3; z >= 0; z--)
            for (int32_t a = 0; a < 3; a++)
                for (int32_t b = 0; b < 3; b++) {
                    if (!h.at(a, b, z).entrance)
                        continue;
                    for (int32_t u = 0; u < 15; u++)
                        for (int32_t v = 0; v < 15; v++) {
                            const int32_t x = p.ox + 6 + 13 * a + u, y = p.oy + 6 + 13 * b + v;
                            if (!zone.contains(x, y))
                                continue;
                            const int32_t d = top(x, y) - M;
                            if (first || d > h0)
                                h0 = d, first = false;
                        }
                }
        h.O = {p.ox + 7, p.oy + 7, M + h0 - 6};
        const glm::ivec3 O = h.O;

        // Step 8: the street under the house, paved up to the ground floor.
        if (L.type == 1 && L.variant != 4 && L.variant != 5)
            pave(h);

        const int32_t Mh = h.sub >= 1 && h.sub <= 5 ? h.sub + 1 : 1;
        // Step 10: bases, arcs and walls.
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t z = 0; z < 4; z++) {
                    Cell c = h.at(a, b, z);
                    const int32_t Z0 = O.z + 7 * z;
                    if (c.role == ARC) {
                        placeAt(h, set.arc, a, b, Z0, c.r, ROCK, 1, false);
                    } else if (c.role == GROUND) {
                        placeAt(h, set.base, a, b, Z0, c.r, ROCK, 1, true);
                    } else if (c.role == STOREY) {
                        static const int32_t SIDES[4][3] = {{0, -1, 0}, {0, 1, 2}, {-1, 0, 1}, {1, 0, 3}};
                        for (const auto &side : SIDES) {
                            const uint8_t n = h.at(a + side[0], b + side[1], z).role;
                            const uint8_t below = h.at(a + side[0], b + side[1], z - 1).role;
                            if (n == NONE) {
                                if (c.entrance || (below != NONE && below != ROOF)) {
                                    placeAt(h, set.door, a, b, Z0, side[2], ROCK, Mh, false);
                                    if (c.entrance) {
                                        // The entrance stairs, centred 13 blocks out.
                                        const std::shared_ptr<const Prefab> S = slot(set.stairs);
                                        if (S) {
                                            int32_t dx = 0, dy = 0;
                                            if (side[1] < 0)
                                                dy = -13;
                                            else if (side[1] > 0)
                                                dy = 13;
                                            else if (side[0] < 0)
                                                dx = -13;
                                            else
                                                dx = 13;
                                            placeAt(h, set.stairs, a, b, Z0 - 4, side[2], ROCK, Mh, true, nullptr, dx, dy);
                                        }
                                    }
                                } else {
                                    int32_t piece = set.window;
                                    if (z == 1)
                                        piece = random.draw() % 6 == 0 ? set.lamp : set.window;
                                    else if (z >= 2)
                                        piece = random.draw() % 6 == 0 ? set.balcony : set.window;
                                    placeAt(h, piece, a, b, Z0, side[2], ROCK, 1, false);
                                }
                            } else if (n != STOREY) {
                                placeAt(h, set.wall, a, b, Z0, side[2], ROCK, 1, false);
                            }
                        }
                    }
                }
        // Step 11: roofs, two halves each.
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t z = 0; z < 4; z++) {
                    const Cell c = h.at(a, b, z);
                    if (c.role != ROOF)
                        continue;
                    const int32_t crop[4] = {h.at(a - 1, b, z).role != NONE, h.at(a, b - 1, z).role != NONE,
                                             h.at(a + 1, b, z).role != NONE, h.at(a, b + 1, z).role != NONE};
                    for (int32_t half = 0; half < 2; half++) {
                        const int32_t r = half == 0 ? c.r : (c.r + 2) & 3;
                        static const int32_t FRONT[4][2] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
                        const Cell &n = h.at(a + FRONT[r][0], b + FRONT[r][1], z);
                        int32_t roof = set.roof[0];
                        if (n.role == ROOF)
                            roof = (n.r & 1) == (c.r & 1) ? set.roof[2] : set.roof[1];
                        placeAt(h, roof, a, b, O.z + 7 * z, r, 14, 1, false, crop);
                    }
                }
        // Step 12: floors and carpets.
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t z = 0; z < 4; z++) {
                    const Cell c = h.at(a, b, z);
                    const int32_t top7 = O.z + 7 * z + 1;
                    if (c.role == ROOF && h.at(a, b, z - 1).role == STOREY) {
                        placeAt(h, set.floor, a, b, top7 - modelHeight(set.floor), c.r, ROCK, 1, false);
                    } else if (c.role == STOREY && c.stairs) {
                        placeAt(h, set.floorStairs, a, b, top7 - modelHeight(set.floorStairs), c.r, ROCK, 1, false);
                    } else if (c.role == STOREY) {
                        if (L.type == 1 && (random.draw() & 1) == 0 && !h.at(a, b, z + 1).stairs) {
                            const int32_t carpet = 2186 + random.draw() % 3;
                            const int32_t r = random.draw() & 3;
                            placeAt(h, carpet, a, b, top7 - modelHeight(carpet), r, ROCK, 1, false);
                        }
                        placeAt(h, set.floor, a, b, top7 - modelHeight(set.floor), c.r, ROCK, 1, false);
                    }
                }
        // Step 13: indoor walls round the bedrooms.
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t z = 0; z < 4; z++) {
                    const Cell c = h.at(a, b, z);
                    if (c.role != STOREY || c.room != 1)
                        continue;
                    static const int32_t SIDES[4][3] = {{0, -1, 0}, {-1, 0, 1}, {0, 1, 2}, {1, 0, 3}};
                    for (const auto &side : SIDES)
                        if (h.at(a + side[0], b + side[1], z).role == STOREY)
                            placeAt(h, set.indoor, a, b, O.z + 7 * z, side[2], ROCK, 1, false);
                }
        // Step 14: rooms cleared to protected air, and the house's points.
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t z = 0; z < 4; z++) {
                    const Cell c = h.at(a, b, z);
                    if (c.role != STOREY)
                        continue;
                    for (int32_t u = 0; u < 14; u++)
                        for (int32_t v = 0; v < 14; v++)
                            for (int32_t w = 5; w >= 0; w--) {
                                const int32_t x = O.x + 13 * a + u, y = O.y + 13 * b + v, zz = O.z + 7 * z + w;
                                if (!zone.contains(x, y))
                                    continue;
                                Block bl = zone.read(x, y, zz);
                                if (!(bl.typeByte & DRY_AIR)) {
                                    bl.typeByte = DRY_AIR;
                                    write(x, y, zz, bl);
                                }
                            }
                    if (h.at(a, b, z - 1).role == GROUND) {
                        const glm::ivec3 pc{O.x + 7 + 13 * a, O.y + 7 + 13 * b, O.z + 7 * z + 1};
                        if (c.entrance)
                            h.entrance.push_back(pc);
                        if (c.room == 3)
                            h.counter.push_back(pc);
                        h.spawn.push_back(pc);
                    }
                }
        if (L.type != 1)
            return;
        // Step 15: lanterns and furniture.
        const size_t furnished = zone.statics.size();
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t z = 0; z < 4; z++) {
                    const Cell c = h.at(a, b, z);
                    if (c.role != STOREY)
                        continue;
                    const int64_t baseX = (int64_t)(O.x + 13 * a) * ONE, baseY = (int64_t)(O.y + 13 * b) * ONE;
                    if ((a + b) & 1) {
                        Sprite lantern;
                        lantern.kind = 13;
                        lantern.x = baseX + 7 * ONE, lantern.y = baseY + 7 * ONE;
                        lantern.z = (int64_t)(O.z + 7 * z) * ONE + fxOf(5.3);
                        lantern.size = 0.075f;
                        lantern.angle = 0;
                        lantern.flags = 1;
                        lantern.tint[0] = 0.6f, lantern.tint[1] = 0.5f, lantern.tint[2] = 0.4f;
                        zone.sprites.push_back(lantern);
                    }
                    if (h.at(a, b, z + 1).stairs)
                        continue;
                    const int32_t Zf = O.z + 7 * z + 1;
                    const int64_t zf = (int64_t)Zf * ONE;
                    auto piece = [&](double dx, double dy, int32_t r) {
                        supported(wallPiece(baseX + fxOf(dx), baseY + fxOf(dy), zf, r, c.room), Zf);
                    };
                    auto gated = [&](double dx, double dy, int32_t r) {
                        if (random.draw() % 3 != 0)
                            piece(dx, dy, r);
                    };
                    if (h.at(a - 1, b, z).role != STOREY)
                        gated(1.5, 4.5, 1), gated(1.5, 9.5, 1);
                    if (h.at(a + 1, b, z).role != STOREY)
                        gated(12.5, 4.5, 3), gated(12.5, 9.5, 3);
                    if (h.at(a, b - 1, z).role != STOREY)
                        piece(4.5, 1.5, 2);
                    if (h.at(a, b + 1, z).role != STOREY)
                        gated(4.5, 12.5, 0), gated(9.5, 12.5, 0);
                    auto object = [&](int32_t kind, int64_t x, int64_t y, int32_t r, float sx, float sy, float sz) {
                        StaticObject o;
                        o.kind = kind;
                        o.x = x, o.y = y, o.z = zf;
                        o.rotation = r;
                        o.scale[0] = sx, o.scale[1] = sy, o.scale[2] = sz;
                        supported(o, Zf);
                    };
                    const int64_t mx = baseX + 7 * ONE, my = baseY + 7 * ONE;
                    switch (c.room) {
                    case 1:
                        for (const auto &bed : {glm::ivec2(4, 4), glm::ivec2(10, 4), glm::ivec2(4, 10), glm::ivec2(10, 10)})
                            object(19, baseX + bed.x * ONE, baseY + bed.y * ONE, random.draw() & 3, 2, 3, 1);
                        break;
                    case 2:
                        object(12, mx, my, random.draw() & 3, 3, 3, 1);
                        object(16, baseX + fxOf(4.5), my, 1, 1, 1, 0.5f);
                        object(16, baseX + fxOf(9.5), my, 3, 1, 1, 0.5f);
                        object(16, mx, baseY + fxOf(4.5), 2, 1, 1, 0.5f);
                        object(16, mx, baseY + fxOf(9.5), 0, 1, 1, 0.5f);
                        break;
                    case 3:
                        object(31, baseX + fxOf(3.5), my, 3, 4, 1, 1);
                        object(31, baseX + fxOf(10.5), my, 1, 4, 1, 1);
                        object(31, mx, baseY + fxOf(3.5), 0, 4, 1, 1);
                        object(31, mx, baseY + fxOf(10.5), 2, 4, 1, 1);
                        break;
                    default:
                        if (random.draw() % 5 == 0)
                            object(12, mx, my, random.draw() & 3, 3, 3, 1);
                        break;
                    }
                }
        // Step 16: the house's seats and beds (no draws).
        worldgen::House record;
        record.ox = h.O.x, record.oy = h.O.y, record.oz = h.O.z;
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t z = 0; z < 4; z++)
                    if (h.at(a, b, z).role == STOREY)
                        record.storeys.push_back({a, b, z});
        for (size_t k = furnished; k < zone.statics.size(); k++) {
            const int32_t kind = zone.statics[k].kind;
            if (kind == 16 || kind == 18)
                record.seats.push_back((int32_t)k);
            else if (kind == 19)
                record.beds.push_back((int32_t)k);
        }
        zone.houses.push_back(std::move(record));
        // Step 17: goods outside the ground floor.
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++) {
                if (h.at(a, b, 0).role != GROUND)
                    continue;
                const int64_t baseX = (int64_t)(O.x + 13 * a) * ONE, baseY = (int64_t)(O.y + 13 * b) * ONE;
                const int64_t z1 = (int64_t)(O.z + 1) * ONE;
                struct Slots {
                    int32_t da, db;
                    double x1, y1, x2, y2;
                    int32_t r;
                };
                static const Slots SIDES[4] = {{-1, 0, -1.5, 4, -1.5, 10, 3},
                                               {1, 0, 15.5, 4, 15.5, 10, 1},
                                               {0, -1, 4, -1.5, 10, -1.5, 0},
                                               {0, 1, 4, 15.5, 10, 15.5, 2}};
                for (const Slots &sd : SIDES) {
                    if (h.at(a + sd.da, b + sd.db, 0).role != NONE)
                        continue;
                    for (int32_t k = 0; k < 2; k++) {
                        if (random.draw() % 6 != 0)
                            continue;
                        StaticObject o = goods(baseX + fxOf(k ? sd.x2 : sd.x1), baseY + fxOf(k ? sd.y2 : sd.y1), z1, sd.r);
                        if (settle(o, zone))
                            zone.statics.push_back(o);
                    }
                }
            }
    }

    // §5.5.4 step 8.
    void pave(const House &h) {
        House &hh = const_cast<House &>(h);
        const glm::ivec3 O = h.O;
        int32_t x1 = O.x - 2, y1 = O.y - 2;
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                if (hh.at(a, b, 0).role != NONE) {
                    x1 = std::max(x1, O.x + 13 * a + 13);
                    y1 = std::max(y1, O.y + 13 * b + 13);
                }
        x1 += 2, y1 += 2;
        const int32_t x0 = O.x - 2, y0 = O.y - 2;
        auto ground = [&](int32_t x, int32_t y) {
            int32_t z = top(x, y) - 1;
            while (!blockSolid(x, y, z))
                z--;
            return z;
        };
        for (int32_t x = x0; x <= x1; x++)
            for (int32_t y = y0; y <= y1; y++) {
                if (!zone.contains(x, y))
                    continue;
                const int32_t z = ground(x, y);
                if (read(x, y, z).type() != TOWN_GROUND || z < O.z)
                    return;
            }
        for (int32_t x = x0; x <= x1; x++)
            for (int32_t y = y0; y <= y1; y++) {
                if (!zone.contains(x, y))
                    continue;
                const int32_t z = ground(x, y);
                const Block found = read(x, y, z);
                if (found.type() != TOWN_GROUND)
                    continue;
                glm::vec3 C(found.r, found.g, found.b);
                if (x == x0 || x == x1 || y == y0 || y == y1) {
                    const float n = noise(0.05 * x + 843, 0.05 * y + 984);
                    C = glm::vec3(f(f(f(40.f * n) + 140.f) * 0.7f));
                }
                const Block paving{(uint8_t)(int32_t)C.r, (uint8_t)(int32_t)C.g, (uint8_t)(int32_t)C.b, TOWN_GROUND};
                for (int32_t zz = z + 1; zz <= O.z + 5; zz++)
                    write(x, y, zz, paving);
            }
    }

    // ---- §5.5.8: plazas -------------------------------------------------------------------

    std::vector<glm::ivec3> plazas, markets;

    void plaza(const Plot &p) {
        bool any = false;
        const int32_t h = (256 / g) / 2, k = s / 4;
        for (int32_t pp : {0, h})
            for (int32_t q : {0, h}) {
                const int32_t X = p.ox + k + pp, Y = p.oy + k + q;
                if ((double)weight((int64_t)X * ONE, (int64_t)Y * ONE) < 0.72)
                    continue;
                int32_t H = lowestOpen(X, Y);
                if (L.type == 1) {
                    if (read(X, Y, H - 1).type() != TOWN_GROUND)
                        continue;
                    auto inDisc = [&](int32_t x, int32_t y) {
                        const double dx = X + 0.5 - x, dy = Y + 0.5 - y;
                        return dx * dx + dy * dy;
                    };
                    for (int32_t x = X - 8; x <= X + 8; x++)
                        for (int32_t y = Y - 8; y <= Y + 8; y++)
                            if (inDisc(x, y) <= 64 && zone.contains(x, y))
                                H = std::max(H, lowestOpen(x, y));
                    for (int32_t x = X - 8; x <= X + 8; x++)
                        for (int32_t y = Y - 8; y <= Y + 8; y++) {
                            const double r2 = inDisc(x, y);
                            if (r2 > 64 || !zone.contains(x, y))
                                continue;
                            const int32_t Hx = lowestOpen(x, y);
                            Climate cl{terrain.world().moisture(x, y), terrain.world().warmth(x, y),
                                       terrain.climate(X, Y, &zone).fire};
                            for (int32_t z = Hx - 1; z <= H; z++) {
                                if (r2 < 49) {
                                    write(x, y, z, terrain.surface(X, Y, H, cl, &zone));
                                } else {
                                    const float m = f(f(f((float)random.draw() * 0.1f) / 32767.f) + 0.75f);
                                    const glm::vec3 St = terrain.stone(x, y, z) * m;
                                    write(x, y, z, {(uint8_t)(int32_t)St.r, (uint8_t)(int32_t)St.g, (uint8_t)(int32_t)St.b, ROCK});
                                }
                            }
                        }
                }
                any = true;
                const float wm = terrain.climate(X, Y, &zone).warmth;
                const int32_t kind = wm > 0.8f ? 3 : 5;
                if (H > 1) {
                    const int32_t d1 = random.draw(), d2 = random.draw();
                    TreeBuilder(terrain, zone, random).build(X, Y, H + 1, d2 % 6 + 7, (d1 & 7) + 13, kind);
                }
                if (L.type == 1) {
                    static const double SPOTS[4][3] = {{0.5, -7.5, 0}, {0.5, 9.5, 2}, {-7.5, 0.5, 3}, {9.5, 0.5, 1}};
                    for (const auto &spot : SPOTS) {
                        if ((random.draw() & 7) == 0)
                            continue;
                        StaticObject bench;
                        bench.kind = 18;
                        bench.x = fxOf(X + spot[0]), bench.y = fxOf(Y + spot[1]), bench.z = (int64_t)H * ONE;
                        bench.rotation = (int32_t)spot[2];
                        bench.scale[0] = 3, bench.scale[1] = 0.4f, bench.scale[2] = 0.4f;
                        if (settle(bench, zone))
                            zone.statics.push_back(bench);
                    }
                }
            }
        if (any)
            plazas.push_back({cx(p), cy(p), lowestOpen(cx(p), cy(p))});
    }

    // ---- §5.5.9: townsfolk and ruin creatures ---------------------------------------------

    static Spawn person(glm::ivec3 at) {
        Spawn s;
        s.x = (int64_t)at.x * ONE, s.y = (int64_t)at.y * ONE, s.z = (int64_t)at.z * ONE;
        return s;
    }

    // Full stock owns its grouped value; upstream town admission remains separate.
    void vendorStock(Spawn &shop, int32_t which) {
        using creatures::vendor_stock::Route;
        const Route route = which == 1 ? Route::General : which == 2 ? Route::Armour : Route::Weapons;
        auto inventory = std::make_shared<creatures::InventoryValue>();
        creatures::vendor_stock::generate(*inventory, route, [&] { return random.draw(); });
        shop.inventoryValue = std::move(inventory);
    }

    // §5.5.9, town: innkeepers, shopkeepers and residents with their day.
    void townsfolk() {
        std::vector<glm::ivec3> poi;
        if (const Region *region = terrain.world().region(zone.zx >> 6, zone.zy >> 6))
            for (const Location &rec : region->records)
                if (rec.type != 0 && rec.type != 1 && rec.type != 10)
                    poi.push_back({(int32_t)(rec.x / ONE), (int32_t)(rec.y / ONE), 0});
        std::vector<House *> inns, shops;
        for (House &h : houses) {
            if (h.sub == 1)
                inns.push_back(&h);
            else if (h.sub >= 2 && h.sub <= 5)
                shops.push_back(&h);
        }
        for (House &h : houses) {
            if (h.spawn.empty())
                continue;
            if (h.sub >= 1 && h.sub <= 5) {
                if (h.counter.empty())
                    continue;
                Spawn keeper = person(h.counter[(uint32_t)random.draw() % h.counter.size()]);
                keeper.controller = 3;
                keeper.type = (random.draw() & 1) + 2;
                keeper.level = L.level;
                keeper.characterClass = (uint8_t)(h.sub == 1 ? 0x84 : 0x80 + h.sub - 2);
                if (h.sub == 2)
                    vendorStock(keeper, 1);
                else if (h.sub == 4)
                    vendorStock(keeper, 2);
                else if (h.sub == 3 || h.sub == 5)
                    vendorStock(keeper, 3);
                zone.spawns.push_back(keeper);
                continue;
            }
            if (h.sub != 0)
                continue;
            const int32_t n = (random.draw() & 1) + 1;
            for (int32_t i = 0; i < n; i++) {
                const glm::ivec3 home = h.spawn[(uint32_t)random.draw() % h.spawn.size()];
                Spawn r = person(home);
                r.controller = 3;
                r.characterClass = 0x88;
                r.type = (random.draw() & 1) + 2;
                r.level = L.level;
                if (!poi.empty() && (random.draw() & 1) == 0) {
                    const glm::ivec3 place = poi[poiCounter++ % poi.size()];
                    r.field41[0] = place.x, r.field41[1] = place.y, r.field41[2] = 0;
                    r.field42 = 4;
                }
                if (random.draw() % 10 == 0)
                    r.field42 = 3;
                // The day: out to the plaza, the market, a shop, the inn, and home again.
                int32_t T = 0;
                auto stop = [&](glm::ivec3 at, int32_t ms) {
                    r.schedule.push_back({(int64_t)at.x * ONE, (int64_t)at.y * ONE, (int64_t)at.z * ONE, ms});
                };
                auto then = [&](int32_t minutes) { T = r.schedule.back().timeMs + minutes * 60000; };
                stop(home, 0);
                T = (random.draw() % 180 + 420) * 60000;
                auto visit = [&](const std::vector<glm::ivec3> &list, int32_t mod, int32_t add) {
                    if (list.empty())
                        return;
                    stop(list[(uint32_t)random.draw() % list.size()], T);
                    then(random.draw() % mod + add);
                };
                auto visitHouse = [&](const std::vector<House *> &list, int32_t mod, int32_t add) {
                    if (list.empty())
                        return;
                    const House *other = list[(uint32_t)random.draw() % list.size()];
                    if (other->entrance.empty())
                        return;
                    stop(other->entrance[(uint32_t)random.draw() % other->entrance.size()], T);
                    then(random.draw() % mod + add);
                };
                visit(plazas, 60, 60);
                visit(markets, 180, 60);
                visitHouse(shops, 20, 0);
                visit(plazas, 60, 60);
                visitHouse(inns, 180, 60);
                visitHouse(shops, 30, 0);
                visitHouse(shops, 30, 0);
                visit(plazas, 60, 60);
                if (!h.entrance.empty()) {
                    random.draw(); // (its value is not used)
                    stop(home, T);
                    then(random.draw() % 60 + 60);
                }
                stop(home, T + 1800000);
                zone.spawns.push_back(r);
            }
        }
    }

    // §5.5.9, ruins: the guardians of the antique building, creatures on open plots, rings
    // and groups round the ruined houses.
    struct RuinGroup {
        std::vector<int32_t> a, b;
    };

    void ruinCreatures() {
        std::vector<int32_t> K;
        std::vector<RuinGroup> GL;
        switch (L.variant) {
        case 0: K = {41, 17, 96, 96}, GL = {{{41, 17}, {59, 37, 38, 40, 39}}}; break;
        case 1: K = {9, 10}, GL = {{{9, 10}, {22, 55}}}; break;
        case 2: K = {83, 84}, GL = {{{83, 84}, {53, 60}}, {{81}, {}}}; break;
        case 3: K = {76}, GL = {{{76}, {58, 60}}, {{81}, {}}}; break;
        case 4: {
            const int32_t m = L.serial % 3;
            K = {15, 16}, GL = {{{15, 16}, {96}}, {{m == 1 ? 94 : m == 2 ? 17 : 97}, {}}};
            break;
        }
        default: K = {11, 12}, GL = {{{46}, {19}}}; break;
        }
        auto hostile = [&](int64_t x, int64_t y, int64_t z, const std::vector<int32_t> &kinds) {
            Spawn c;
            c.x = x, c.y = y, c.z = z;
            c.yaw = f(f((float)random.draw() * 360.f) / 32767.f);
            c.level = L.level;
            c.controller = 1;
            c.appearanceFlags |= 0x1000;
            c.type = kinds[(uint32_t)random.draw() % kinds.size()];
            c.powerTier = (uint8_t)L.subLevel;
            return c;
        };
        for (int32_t i = 0; i < g; i++)
            for (int32_t j = 0; j < g; j++) {
                const Plot &p = plots[i + g * j];
                if (p.kind == CENTRE_RUIN) {
                    Spawn boss = hostile((int64_t)cx(p) * ONE, (int64_t)cy(p) * ONE,
                                         (int64_t)(openAbove(cx(p), cy(p)) + 1) * ONE, K);
                    boss.appearanceFlags |= 0x200;
                    boss.bossLoot = true;
                    zone.spawns.push_back(boss);
                } else if (p.kind != HOUSE && p.wc > 0.2f) {
                    for (int32_t pp : {0, s})
                        for (int32_t q : {0, s}) {
                            const int32_t X = p.ox + s / 4 + pp / 2, Y = p.oy + s / 4 + q / 2;
                            if (random.draw() % 5 != 0 || !zone.contains(X, Y))
                                continue;
                            zone.spawns.push_back(
                                hostile((int64_t)X * ONE, (int64_t)Y * ONE, (int64_t)(openAbove(X, Y) + 1) * ONE, K));
                        }
                }
            }
        constexpr double PI = 3.141592653589793;
        auto potions = [&](Spawn &c) {
            const int32_t e = random.draw() & 1;
            for (int32_t n = 0; n < e; n++) {
                ItemRecord potion;
                potion.type = 1, potion.subtype = 1, potion.level = (int16_t)c.level;
                c.inventory.push_back(potion);
            }
        };
        for (House &h : houses) {
            if (h.spawn.empty())
                continue;
            std::vector<glm::ivec3> P = h.spawn;
            auto take = [&] {
                const size_t i = (uint32_t)random.draw() % P.size();
                const glm::ivec3 at = P[i];
                P.erase(P.begin() + (ptrdiff_t)i);
                return at;
            };
            const int32_t rings = random.draw() % 3;
            for (int32_t m = 0; m < rings && !P.empty(); m++) {
                const glm::ivec3 centre = take();
                const int32_t n = random.draw() % 3 + 2;
                for (int32_t t = 0; t < n; t++) {
                    const float a = f((double)(2 * t) * PI / (double)n);
                    const float c2 = f(f(std::cos((double)a)) * 2.f), s2 = f(f(std::sin((double)a)) * 2.f);
                    Spawn c;
                    c.x = (int64_t)centre.x * ONE + (int64_t)f(c2 * 65536.f);
                    c.y = (int64_t)centre.y * ONE + (int64_t)f(s2 * 65536.f);
                    c.z = (int64_t)centre.z * ONE + ONE;
                    c.yaw = f((double)a / PI * 180.0 + 90.0);
                    c.level = L.level;
                    c.controller = 1;
                    c.appearanceFlags |= 0x1000;
                    c.type = K[(uint32_t)random.draw() % K.size()];
                    c.powerTier = (uint8_t)L.subLevel;
                    potions(c);
                    zone.spawns.push_back(c);
                }
            }
            if (GL.empty())
                continue;
            const int32_t groups = random.draw() & 1;
            for (int32_t m = 0; m < groups && !P.empty(); m++) {
                const glm::ivec3 at = take();
                const RuinGroup &group = GL[(uint32_t)random.draw() % GL.size()];
                if (group.a.empty())
                    continue;
                Spawn c = hostile((int64_t)at.x * ONE, (int64_t)at.y * ONE, (int64_t)(at.z + 1) * ONE, group.a);
                if (!houses.empty()) {
                    const House &other = houses[(uint32_t)random.draw() % houses.size()];
                    if (!other.spawn.empty())
                        random.draw(); // a point of that house joins the creature's route (not kept here)
                }
                potions(c);
                zone.spawns.push_back(c);
            }
        }
    }

    size_t poiCounter = 0;

    const Location &L;
    const int32_t Q, g, s;
    Plot plots[25];
};

} // namespace

void Terrain::town(Zone &zone, Random &random, const Location &L, int32_t quarter) const {
    TownBuilder(*this, zone, random, L, quarter, [&](int32_t x, int32_t y, int32_t z) { return rockBlock(x, y, z, zone); })
        .run();
}

} // namespace worldgen
