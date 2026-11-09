// The prefab placer (docs/specs/alpha-worldgen-structures.md §6.2).
#include "worldgen/Prefab.h"
#include <glm/glm.hpp>
#include <algorithm>

namespace worldgen {

uint32_t Prefab::colour(int32_t i, int32_t j, int32_t k) const {
    if (i < 0 || j < 0 || k < 0 || i >= sx || j >= sy || k >= sz)
        return 0;
    const uint8_t *c = &rgb[3 * ((size_t)(k * sy + j) * sx + i)];
    return (uint32_t)c[0] << 16 | (uint32_t)c[1] << 8 | c[2];
}

namespace {

constexpr int64_t ONE = 65536;
constexpr int64_t HALF = 32768;

constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) { return (uint32_t)r << 16 | (uint32_t)g << 8 | b; }

// The marker colours of §6.2, by row number.
enum Marker : uint32_t {
    DOOR = rgb(255, 0, 0),
    WINDOW = rgb(0, 255, 0),
    WALL_PLANT = rgb(0, 127, 127),
    BED = rgb(255, 127, 0),
    SHELF = rgb(255, 127, 127),
    TABLE_SET = rgb(127, 127, 0),
    LANTERN = rgb(255, 255, 0),
    CREST = rgb(63, 0, 0),
    SIGN = rgb(0, 0, 255),
    FLOWER_BOX = rgb(255, 0, 255),
    CRAFT_OR_GUARD = rgb(0, 255, 255),
    BENCH = rgb(0, 0, 127),
    CRAFT_TOOL = rgb(0, 255, 127),
    VASE = rgb(0, 127, 0),
    VILLAGER = rgb(127, 0, 127),
};

// The modes whose markers are interpreted (0 and 8 only make plain blocks).
bool readsMarkers(int32_t mode) { return mode >= 1 && mode <= 15 && mode != 8; }

bool solidType(const Block &b) { return b.type() != AIR && b.type() != WATER; }

Block colourBlock(uint32_t c, uint8_t typeByte) {
    return {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c, typeByte};
}

StaticObject makeStatic(int32_t kind, int64_t x, int64_t y, int64_t z, int32_t rotation, float sx, float sy, float sz) {
    StaticObject o;
    o.kind = kind;
    o.x = x;
    o.y = y;
    o.z = z;
    o.rotation = rotation;
    o.scale[0] = sx;
    o.scale[1] = sy;
    o.scale[2] = sz;
    return o;
}

Sprite makeSprite(int32_t kind, int64_t x, int64_t y, int64_t z, float size, float angle, int32_t flags) {
    Sprite s;
    s.kind = kind;
    s.x = x;
    s.y = y;
    s.z = z;
    s.size = size;
    s.angle = angle;
    s.flags = flags;
    return s;
}

class Placer {
  public:
    Placer(Zone &zone, Random &random, const PlacerWorld &world, const Prefab &model, const Placement &at)
        : zone(zone), random(random), world(world), model(model), at(at), r(at.rotation & 3) {}

    void run() {
        // Zone clip: nothing at all when the model's footprint misses the zone.
        const int32_t wx = (r & 1) ? model.sy : model.sx, wy = (r & 1) ? model.sx : model.sy;
        if (at.x + wx <= zone.originX() || at.y + wy <= zone.originY() || at.x >= zone.originX() + Zone::SIZE ||
            at.y >= zone.originY() + Zone::SIZE)
            return;
        wm = zone.contains(at.x, at.y) ? zone.column(at.x, at.y).warmth : world.warmth(at.x, at.y);

        // Crop, from world directions (-x, -y, +x, +y) to model columns.
        const int32_t *e = at.crop;
        int32_t i0, i1, j0, j1;
        switch (r) {
        case 0: i0 = e[0], i1 = e[2], j0 = e[1], j1 = e[3]; break;
        case 1: i0 = e[3], i1 = e[1], j0 = e[0], j1 = e[2]; break;
        case 2: i0 = e[2], i1 = e[0], j0 = e[3], j1 = e[1]; break;
        default: i0 = e[1], i1 = e[3], j0 = e[2], j1 = e[0]; break;
        }
        for (int32_t i = i0; i < model.sx - i1; i++)
            for (int32_t j = j0; j < model.sy - j1; j++)
                column(i, j);
    }

  private:
    // The rotation table without the origin.
    glm::ivec2 turn(int32_t i, int32_t j) const {
        switch (r) {
        case 0: return {i, j};
        case 1: return {j, model.sx - 1 - i};
        case 2: return {model.sx - 1 - i, model.sy - 1 - j};
        default: return {model.sy - 1 - j, i};
        }
    }

    Block read(int32_t x, int32_t y, int32_t z) const {
        return zone.contains(x, y) ? zone.read(x, y, z) : IMPLICIT_SOLID;
    }

    void write(int32_t x, int32_t y, int32_t z, Block b, bool listed = true) {
        if (zone.contains(x, y))
            zone.place(x, y, z, b, [&](int32_t gz) { return world.gapFill(x, y, gz); });
        if (listed && at.written)
            at.written->push_back({x, y, z, b});
    }

    void column(int32_t i, int32_t j) {
        int32_t zlo = model.sz, zhi = 0;
        for (int32_t k = 0; k < model.sz; k++)
            if (model.colour(i, j, k)) {
                zlo = std::min(zlo, k);
                zhi = std::max(zhi, k);
            }
        const glm::ivec2 t = turn(i, j);
        const int32_t X = at.x + t.x, Y = at.y + t.y;

        // Fill-down: the bottom voxel's colour runs down to the ground.
        if (at.fillDown && model.colour(i, j, 0) && zone.contains(X, Y)) {
            const Block fill = colourBlock(model.colour(i, j, 0), ROCK);
            for (int32_t z = at.z - 1; z > zone.column(X, Y).bottom; z--) {
                if (solidType(read(X, Y, z)))
                    break;
                write(X, Y, z, fill, false);
            }
        }

        for (int32_t k = model.sz - 1; k >= 0; k--) {
            const int32_t Z = at.z + k;
            const uint32_t C = model.colour(i, j, k);
            const bool inside = zlo < k && k < zhi;
            if (!C) {
                if (inside)
                    write(X, Y, Z, {255, 255, 255, 0xC0});
                continue;
            }
            const Block E = read(X, Y, Z);
            if ((E.typeByte & DRY_AIR) && solidType(E))
                continue; // structure blocks already there win
            if (!readsMarkers(at.mode) || !marker(C, i, j, k, X, Y, Z, inside, E))
                plain(C, X, Y, Z);
        }
    }

    void plain(uint32_t C, int32_t X, int32_t Y, int32_t Z) {
        if (at.mode == 8 && C == SIGN)
            write(X, Y, Z, {255, 255, 255, WATER}); // a well's blue voxels are water
        else
            write(X, Y, Z, colourBlock(C, at.typeByte | DRY_AIR));
    }

    void air(int32_t X, int32_t Y, int32_t Z) { write(X, Y, Z, {0, 0, 0, DRY_AIR}); }
    void airIfInside(bool inside, int32_t X, int32_t Y, int32_t Z) {
        if (inside)
            air(X, Y, Z);
    }

    // A blob of one marker colour: true at its origin voxel, with the static it makes.
    bool blob(uint32_t C, int32_t i, int32_t j, int32_t k, int32_t kind, StaticObject &o) const {
        if (model.colour(i - 1, j, k) == C || model.colour(i, j - 1, k) == C || model.colour(i, j, k - 1) == C)
            return false;
        int32_t i2 = i, j2 = j, k2 = k;
        while (model.colour(i2 + 1, j, k) == C)
            i2++;
        while (model.colour(i, j2 + 1, k) == C)
            j2++;
        while (model.colour(i, j, k2 + 1) == C)
            k2++;
        const glm::ivec2 a = turn(i, j), b = turn(i2, j2);
        const glm::ivec2 lo = glm::min(a, b), hi = glm::max(a, b) + 1;
        const int32_t zlo = k, zhi = k2 + 1;
        const int32_t lx = hi.x - lo.x, ly = hi.y - lo.y;
        o = makeStatic(kind, (int64_t)(lo.x + hi.x) * HALF + (int64_t)at.x * ONE,
                       (int64_t)(lo.y + hi.y) * HALF + (int64_t)at.y * ONE, (int64_t)(zlo + at.z) * ONE, lx > ly ? 0 : 1,
                       (float)std::max(lx, ly), (float)std::min(lx, ly), (float)(zhi - zlo));
        o.state = 1;
        return true;
    }

    Spawn creature(int64_t px, int64_t py, int64_t pz, int32_t X, int32_t Y) const {
        Spawn s;
        s.x = px + HALF;
        s.y = py + HALF;
        s.z = pz + HALF;
        s.yaw = (float)((2 - r) * 90);
        s.level = world.level ? world.level(X, Y) : 1;
        return s;
    }

    // The NPC marker's gear by mode (§6.2, the mode table).
    void villagerGear(Spawn &s) {
        auto slot = [&](int32_t index, uint8_t type, uint8_t material, bool level20) {
            ItemRecord &item = s.equipment[index];
            item.type = type;
            item.rarity = (uint8_t)(random.draw() % 5);
            item.material = material;
            if (level20)
                item.level = 20;
        };
        // Modes 9..12: guards in full gear. Mode 9 draws the weapon among 15..17, mode 10
        // among 6..7; 11 and 12 have a fixed one.
        auto armed = [&](uint8_t fixedSubtype, uint8_t weaponMaterial, uint8_t armourMaterial) {
            s.appearanceFlags = 0x40;
            s.level = 20;
            s.type = 2;
            ItemRecord &weapon = s.equipment[7];
            weapon.type = 3;
            if (at.mode == 9)
                weapon.subtype = (uint8_t)(random.draw() % 3 + 15);
            else if (at.mode == 10)
                weapon.subtype = (uint8_t)((random.draw() & 1) + 6);
            else
                weapon.subtype = fixedSubtype;
            weapon.rarity = (uint8_t)(random.draw() % 5);
            weapon.level = 20;
            weapon.material = weaponMaterial;
            slot(5, 7, armourMaterial, true); // chest
            slot(2, 4, armourMaterial, false);
            slot(3, 6, armourMaterial, false);
            slot(4, 5, armourMaterial, false);
        };
        switch (at.mode) {
        case 6: s.characterClass = 0x83; break;
        case 7:
            s.characterClass = 0x89;
            s.level = 20;
            s.type = (random.draw() & 1) + 2;
            break;
        case 9: s.characterClass = 1; armed(0, 1, 1); break;
        case 10: s.characterClass = 2; armed(0, 2, 26); break;
        case 11: s.characterClass = 4; armed(5, 1, 27); break;
        case 12: s.characterClass = 3; armed(10, 2, 25); break;
        case 13: s.characterClass = 0x85; break;
        case 14: s.characterClass = 0x86; break;
        case 15: s.characterClass = 0x87; break;
        default: break;
        }
    }

    // The marker table; false when C is not a marker (the voxel then gets the plain write).
    bool marker(uint32_t C, int32_t i, int32_t j, int32_t k, int32_t X, int32_t Y, int32_t Z, bool inside, const Block &E) {
        const int64_t px = (int64_t)X * ONE, py = (int64_t)Y * ONE, pz = (int64_t)Z * ONE;
        const int32_t M = at.mode;
        StaticObject o;
        switch (C) {
        case DOOR:
            if (blob(C, i, j, k, M == 7 ? 2 : 1, o))
                zone.statics.push_back(o);
            air(X, Y, Z);
            return true;
        case WINDOW:
            if (blob(C, i, j, k, 3, o))
                zone.statics.push_back(o);
            air(X, Y, Z);
            return true;
        case WALL_PLANT:
            if (wm <= 0.5f || solidType(E))
                return true;
            if (random.draw() % 5 == 0) {
                Sprite s = makeSprite(41 + random.draw() % 3, px + 32964, py + 32964, pz + 196, 0.15f,
                                      (float)(-90 * r), 0);
                zone.sprites.push_back(s);
            }
            air(X, Y, Z);
            return true;
        case BED:
            zone.statics.push_back(makeStatic(19, px + HALF, py + HALF, pz, 0, 2, 3, 1));
            zone.statics.push_back(makeStatic(20, px + 147456, py + HALF, pz, 0, 1, 1, 1));
            airIfInside(inside, X, Y, Z);
            return true;
        case SHELF:
            zone.statics.push_back(makeStatic(32 + random.draw() % 3, px + HALF, py + HALF, pz, at.rotation, 2, 3, 1));
            airIfInside(inside, X, Y, Z);
            return true;
        case TABLE_SET: {
            zone.statics.push_back(makeStatic(12, px + HALF, py + HALF, pz, random.draw() & 3, 3, 3, 1));
            const int32_t turn = random.draw() & 3;
            zone.statics.push_back(makeStatic(16, px + 176947, py + HALF, pz, turn, 1, 1, 0.5f));
            zone.statics.push_back(makeStatic(16, px - 111411, py + HALF, pz, turn, 1, 1, 0.5f));
            random.draw(); // (its value is not used)
            airIfInside(inside, X, Y, Z);
            return true;
        }
        case LANTERN: {
            Sprite s = makeSprite(13, px + HALF, py + HALF, pz, 0.1f, 0, 1);
            s.tint[0] = 0.6f, s.tint[1] = 0.5f, s.tint[2] = 0.4f;
            zone.sprites.push_back(s);
            airIfInside(inside, X, Y, Z);
            return true;
        }
        case CREST:
            zone.sprites.push_back(makeSprite(48, px + HALF, py + HALF, pz + HALF, 0.065f, (float)(-90 * r), 0));
            airIfInside(inside, X, Y, Z);
            return true;
        case SIGN: {
            int32_t kind = -1;
            switch (M) {
            case 2: kind = 33; break;
            case 3: kind = 34; break;
            case 4: kind = 35; break;
            case 5: kind = 36; break;
            case 6: kind = 37; break;
            case 13: kind = 38; break;
            case 14: kind = 39; break;
            case 15: kind = 40; break;
            default: break;
            }
            if (kind >= 0)
                zone.sprites.push_back(makeSprite(kind, px + HALF, py + HALF, pz - HALF, 0.1f, (float)((-1 - r) * 90), 0));
            airIfInside(inside, X, Y, Z);
            return true;
        }
        case FLOWER_BOX:
            if (wm <= 0.5f)
                return true;
            if (blob(C, i, j, k, 0, o)) {
                o.kind = 47 + random.draw() % 3;
                zone.statics.push_back(o);
            }
            airIfInside(inside, X, Y, Z);
            return true;
        case CRAFT_OR_GUARD:
            if (M == 13 || M == 14 || M == 15) {
                if (M == 13)
                    zone.statics.push_back(makeStatic(71, px + HALF, py + HALF, pz, -at.rotation, 3, 3, 6));
                else if (M == 14)
                    zone.statics.push_back(makeStatic(75, px + HALF, py + HALF, pz, -at.rotation, 1.5f, 1.5f, 1));
                else
                    zone.statics.push_back(makeStatic(73, px + HALF, py + HALF, pz, -at.rotation, 1.5f, 1.5f, 1));
                airIfInside(inside, X, Y, Z);
            } else {
                Spawn s = creature(px, py, pz, X, Y);
                s.controller = 6;
                s.type = M == 10 ? 141 : 142;
                zone.spawns.push_back(s);
                air(X, Y, Z);
            }
            return true;
        case BENCH:
            if (M == 13) {
                zone.statics.push_back(makeStatic(77, px + HALF, py + HALF, pz, -at.rotation, 1.8f, 1.8f, 1.2f));
                air(X, Y, Z);
            }
            return true;
        case CRAFT_TOOL:
            if (M == 13)
                zone.statics.push_back(makeStatic(72, px + HALF, py + HALF, pz, -at.rotation, 1, 1, 1));
            else if (M == 14)
                zone.statics.push_back(makeStatic(76, px + HALF, py + HALF, pz, -at.rotation, 1.8f, 1.8f, 1.2f));
            else if (M == 15)
                zone.statics.push_back(makeStatic(74, px + HALF, py + HALF, pz, -at.rotation, 1.5f, 1.5f, 3));
            else
                return true;
            airIfInside(inside, X, Y, Z);
            return true;
        case VASE: {
            const int32_t kind = 56 + random.draw() % 9;
            const float s = kind < 60 ? 0.5f : 1.f;
            zone.statics.push_back(makeStatic(kind, px + HALF, py + HALF, pz, random.draw() & 3, s, s, s));
            airIfInside(inside, X, Y, Z);
            return true;
        }
        case VILLAGER: {
            Spawn s = creature(px, py, pz, X, Y);
            s.controller = 3;
            s.type = random.draw() & 3;
            s.equipment[2].type = 4;
            s.equipment[2].subtype = 0;
            s.equipment[2].material = 6;
            villagerGear(s);
            zone.spawns.push_back(s);
            airIfInside(inside, X, Y, Z);
            return true;
        }
        default:
            return false;
        }
    }

    Zone &zone;
    Random &random;
    const PlacerWorld &world;
    const Prefab &model;
    const Placement &at;
    const int32_t r;
    float wm = 0;
};

} // namespace

void placePrefab(Zone &zone, Random &random, const PlacerWorld &world, const Prefab &model, const Placement &at) {
    Placer(zone, random, world, model, at).run();
}

} // namespace worldgen
