// The structure passes after the boulders and before the tree grid
// (docs/specs/alpha-worldgen-structures.md): camps, the third boulder pass and the type-10
// stone circles, with the settle test that stands static objects on the ground. Section
// numbers point at that spec.
//
// The dungeon builder (§1) is in Dungeon.cpp, the town builder (§5) in Town.cpp and the prefab
// placer (§6.2) in Prefab.cpp.
#include "worldgen/Builders.h"
#include <cmath>

namespace worldgen {
namespace {

constexpr int64_t ONE = 65536; // one block in 1/65536 blocks
constexpr double PI = 3.141592653589793;

Block toBlock(glm::vec3 c, uint8_t typeByte) {
    return {(uint8_t)(int32_t)c.r, (uint8_t)(int32_t)c.g, (uint8_t)(int32_t)c.b, typeByte};
}

// Reads within the zone being generated; a column outside it counts as solid (caves §0).
Block cell(const Zone &zone, int32_t x, int32_t y, int32_t z) {
    return zone.contains(x, y) ? zone.read(x, y, z) : IMPLICIT_SOLID;
}
bool solidAt(const Zone &zone, int32_t x, int32_t y, int32_t z) { return !clear(cell(zone, x, y, z).type()); }


StaticObject makeStatic(int32_t kind, int64_t x, int64_t y, int64_t z, float sx, float sy, float sz) {
    StaticObject o;
    o.kind = kind;
    o.x = x;
    o.y = y;
    o.z = z;
    o.scale[0] = sx;
    o.scale[1] = sy;
    o.scale[2] = sz;
    return o;
}

} // namespace

// §2.3.
bool settle(StaticObject &o, const Zone &zone) {
    const bool turned = o.rotation & 1;
    const float sx = turned ? o.scale[1] : o.scale[0], sy = turned ? o.scale[0] : o.scale[1];
    const int64_t hx = (int64_t)f(f(sx * 0.5f) * 65536.f), hy = (int64_t)f(f(sy * 0.5f) * 65536.f);
    const int32_t X0 = (int32_t)((o.x - hx) / ONE), X1 = (int32_t)((o.x + hx) / ONE);
    const int32_t Y0 = (int32_t)((o.y - hy) / ONE), Y1 = (int32_t)((o.y + hy) / ONE);
    auto anySolid = [&](int32_t z) {
        for (int32_t X = X0; X <= X1; X++)
            for (int32_t Y = Y0; Y <= Y1; Y++)
                if (solidAt(zone, X, Y, z))
                    return true;
        return false;
    };
    auto allSolid = [&](int32_t z) {
        for (int32_t X = X0; X <= X1; X++)
            for (int32_t Y = Y0; Y <= Y1; Y++)
                if (!solidAt(zone, X, Y, z))
                    return false;
        return true;
    };
    for (int i = 0; i < 50 && !anySolid((int32_t)(o.z / ONE)); i++)
        o.z -= ONE;
    for (int i = 0; i < 50 && anySolid((int32_t)(o.z / ONE)); i++)
        o.z += ONE;
    if (o.z <= 0 || !allSolid((int32_t)((o.z - ONE) / ONE)))
        return false;
    return cell(zone, blockOfFixed(o.x), blockOfFixed(o.y), blockOfFixed(o.z)).type() !=
           WATER;
}

// §2.2: a camp round fixed-point position P; false when the ground does not suit one.
bool Terrain::camp(Zone &zone, Random &random, int64_t px, int64_t py, int64_t pz) const {
    const int32_t bx = (int32_t)(px / ONE), by = (int32_t)(py / ONE);
    if (1.f - 50.f * roughness(bx, by) >= 0)
        return false; // camps need rough ground
    StaticObject fire = makeStatic(65, px, py, pz, 2.4f, 2.4f, 0.5f);
    fire.rotation = random.draw() & 3;
    bool placed = false;
    for (int32_t i = 0; i < 3 && !placed; i++)
        for (int32_t j = 0; j < 3 && !placed; j++) {
            StaticObject at = fire;
            at.x = px + ONE * i;
            at.y = py + ONE * j;
            if (settle(at, zone)) {
                zone.statics.push_back(at);
                placed = true;
            }
        }
    if (!placed)
        return false;
    // The ring: a tent, stool, table or mat at each corner, 3.5 blocks out.
    for (int32_t i : {0, 7})
        for (int32_t j : {0, 7}) {
            const int64_t x = px + ONE * i - 229376, y = py + ONE * j - 229376;
            StaticObject o;
            switch (random.draw() & 3) {
            case 0:
                o = makeStatic(66, x, y, pz, 4, 4, 3); // tent
                break;
            case 1:
                o = makeStatic(16, x, y, pz, 1, 1, 0.5f); // stool
                break;
            case 2:
                o = makeStatic(12, x, y, pz, 3, 3, 1); // table
                break;
            default:
                o = makeStatic(69, x, y, pz, 2, 2, 0.1f); // wood mat
                break;
            }
            o.rotation = random.draw() & 3;
            if (settle(o, zone))
                zone.statics.push_back(o);
        }
    return true;
}

void Terrain::structurePasses(Zone &zone, Random &random, const ZoneEntry &E, const Location *Lz,
                              bool climateBonus) const {
    const int32_t zx = zone.zx, zy = zone.zy;
    const int32_t x0 = zone.originX(), y0 = zone.originY();

    // §6.1: the home building, in unnamed worlds only (the start screen's world): the character
    // platform stamped on the home column. Placer mode 0 reads no markers, so it draws nothing.
    if (!w.named && zone.contains(HOME, HOME) && prefabs)
        if (const std::shared_ptr<const Prefab> platform = prefabs("character-platform.cub")) {
            Placement at;
            at.x = HOME - platform->sx / 2;
            at.y = HOME - platform->sy / 2;
            at.z = zone.column(HOME, HOME).top();
            at.typeByte = 6;
            at.mode = 0;
            at.fillDown = true;
            PlacerWorld around;
            around.warmth = [this](int32_t x, int32_t y) { return w.warmth(x, y); };
            around.level = [](int32_t, int32_t) { return 1; }; // (mode 0 places no creatures)
            around.gapFill = [&](int32_t x, int32_t y, int32_t z) { return rockBlock(x, y, z, zone); };
            placePrefab(zone, random, around, *platform, at);
        }

    // §2.1: camp attempts on zones with an odd coordinate sum.
    if ((zx + zy) & 1)
        for (int attempt = 0; attempt < 10; attempt++) {
            const int32_t py = y0 + 48 + random.draw() % 160;
            const int32_t px = x0 + 48 + random.draw() % 160;
            const int64_t X = (int64_t)px * ONE + 32768, Y = (int64_t)py * ONE + 32768;
            if (Lz && (Lz->type == 1 || Lz->type == 5)) {
                const float v = 1.f - w.locationDistanceFx(*Lz, X, Y);
                if (v > 0 && v * v > 0)
                    continue; // not inside a town or ruin
            }
            const int64_t Z = (int64_t)zone.column(px, py).top() * ONE;
            if (camp(zone, random, X, Y, Z)) {
                zone.objectList.push_back({X, Y, Z});
                break;
            }
        }

    // §3: boulder pass 3.
    if (E.kind != 1 && E.kind != 3 && E.kind != 4) {
        int32_t n = random.draw() % 10;
        if (climateBonus)
            n += 10;
        for (int32_t b = 0; b < n; b++) {
            const int32_t ax = (random.draw() & 7) + 3;
            const int32_t ay = (random.draw() & 7) + 3;
            const int32_t az = (random.draw() & 7) + 3;
            const int32_t x = 2 * (128 * zx + ax) + random.draw() % (4 * (64 - ax));
            const int32_t y = 2 * (128 * zy + ay) + random.draw() % (4 * (64 - ay));
            const float hy = (float)y - (float)HOME, hx = (float)x - (float)HOME;
            if (f(f(hy * hy) + f(hx * hx)) < 400.f)
                continue;
            bool nearObject = false;
            for (const ObjectMark &o : zone.objectList) {
                const float ey = f((float)((int64_t)y * ONE - o.y) * (1.f / 65536.f));
                const float ex = f((float)((int64_t)x * ONE - o.x) * (1.f / 65536.f));
                nearObject = nearObject || ey * ey + ex * ex < 1600.f;
            }
            if (nearObject || townness(x, y) > 0.25f)
                continue;
            // The ground: just above the highest stored block that is neither air nor water.
            const Column &column = zone.column(x, y);
            int32_t G = column.bottom;
            for (int32_t i = (int32_t)column.blocks.size() - 1; i >= 0; i--)
                if (!clear(column.blocks[i].type())) {
                    G = column.bottom + i + 1;
                    break;
                }
            const Block ground = zone.read(x, y, G - 1);
            const uint8_t gt = ground.type();
            if (gt == TOWN_GROUND || gt == LEAVES_TYPE || gt == WOOD_TYPE || (ground.typeByte & DRY_AIR))
                continue;
            for (int32_t X = x - ax; X <= x + ax; X++) {
                const float u = f((float)(X - x) / (float)ax);
                for (int32_t Y = y - ay; Y <= y + ay; Y++) {
                    if (!zone.contains(X, Y))
                        continue;
                    const float v = f((float)(Y - y) / (float)ay);
                    const float s2 = f(v * v + u * u);
                    const float k = f(noise(0.05 * X, 0.05 * Y) * 0.8f);
                    for (int32_t Z = G + 2 * az; Z >= G - 2 * az; Z--) {
                        const float wz = f(f((float)(Z - G) / (float)az) + k);
                        if (f(wz * wz + s2) <= 1.f && !(zone.read(X, Y, Z).typeByte & DRY_AIR))
                            zone.place(X, Y, Z, toBlock(rock(X, Y, Z, &zone), ROCK | SURFACE_COLOURED),
                                       [&](int32_t gz) { return rockBlock(X, Y, gz, zone); });
                    }
                }
            }
        }
    }

    // §5: towns and ruins.
    if (Lz && (Lz->type == 1 || Lz->type == 5))
        town(zone, random, *Lz, E.variant);

    // §4: the type-10 site, a ring of rock pillars round a runestone.
    if (E.kind == 4) {
        const int32_t cx = x0 + 128, cy = y0 + 128;
        const int32_t T = zone.column(cx, cy).top();
        zone.objectList.push_back({(int64_t)cx * ONE, (int64_t)cy * ONE, (int64_t)T * ONE});
        const int32_t n = random.draw() % 3 + 6;
        for (int32_t k = 0; k < n; k++) {
            const float a = f((double)(2 * k) * PI / (double)n);
            const float s = f(std::sin((double)a)), c = f(std::cos((double)a));
            const int32_t px = cx + (int32_t)f(c * 25.f), py = cy + (int32_t)f(s * 25.f);
            const int32_t zp = (zone.contains(px, py) ? zone.column(px, py).top() : T) + 4;
            const int32_t a1 = (random.draw() & 3) + 3;
            const int32_t a2 = (random.draw() & 3) + 3;
            for (int32_t X = px - 2 * a1; X <= px + 2 * a1; X++)
                for (int32_t Y = py - 2 * a2; Y <= py + 2 * a2; Y++) {
                    if (!zone.contains(X, Y))
                        continue;
                    const float ny = f(noise(0.05 * Y, 0.05 * Y) * 0.3f); // (both arguments are 0.05 Y)
                    const float v = f(f((float)(Y - py) / (float)a2) + ny);
                    for (int32_t Z = zp + 20; Z >= zp - 20; Z--) {
                        const float wz = f((float)(Z - zp) / 10.f);
                        const float u = f(f(noise(0.05 * X, 0.05 * Z) * 0.3f) + f((float)(X - px) / (float)a1));
                        if (f(f(u * u + v * v) + wz * wz) <= 1.f && !(zone.read(X, Y, Z).typeByte & DRY_AIR))
                            zone.place(X, Y, Z, rockBlock(X, Y, Z, zone),
                                       [&](int32_t gz) { return rockBlock(X, Y, gz, zone); });
                    }
                }
        }
        // The runestone, 3.5 blocks off the centre, settled straight down then up.
        StaticObject stone = makeStatic(45, (int64_t)cx * ONE + 229376, (int64_t)cy * ONE + 229376, (int64_t)T * ONE, 4, 4, 5);
        auto here = [&] {
            return cell(zone, blockOfFixed(stone.x), blockOfFixed(stone.y),
                        blockOfFixed(stone.z));
        };
        for (int guard = 0; guard < 4096 && clear(here().type()); guard++)
            stone.z -= ONE;
        for (int guard = 0; guard < 4096 && !clear(here().type()); guard++)
            stone.z += ONE;
        stone.rotation = random.draw() & 3;
        zone.statics.push_back(stone);
    }
}

// Structures spec §4 test helper: a pillar's offset from the circle's centre.
glm::ivec2 pillarOffset(int32_t k, int32_t n) {
    const float a = f((double)(2 * k) * PI / (double)n);
    return {(int32_t)f(f(std::cos((double)a)) * 25.f), (int32_t)f(f(std::sin((double)a)) * 25.f)};
}

} // namespace worldgen
