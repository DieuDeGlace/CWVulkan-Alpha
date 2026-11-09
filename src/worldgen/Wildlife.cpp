// The animals and wild plants a zone is generated with (docs/specs/alpha-materials-wildlife-routes.md
// §2, §3): the species picker by climate and height, and the zone's 3 x 3 candidate pass that
// places a plant object or an animal (with its companions) at each kept candidate. Before it, the
// two passes that follow the tree grid (docs/specs/alpha-spawn-objects.md §6): the statue and the
// location candidate grid.
#include "worldgen/Terrain.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace worldgen {

namespace {

// §2.2: the lists, in their exact order.
const std::vector<int32_t> SEA_LIFE = {145, 146, 147, 150, 152, 153, 155, 154};
const std::vector<int32_t> PEOPLE = {2, 4, 7, 9, 11, 15, 51, 48, 76};
const std::vector<int32_t> FIRE = {73, 82, 85, 112, 103, 62, 110};
const std::vector<int32_t> SHORE = {106, 57, 86, 25};
const std::vector<int32_t> TEMPERATE = {54, 74, 28, 56, 25, 53, 55, 60, 63, 67, 69, 70, 71, 87, 88, 100, 90, 91,
                                        92, 35, 102, 104, 105, 34, 33, 30, 31, 32, 19, 20, 26, 27, 98, 22, 23, 151};
const std::vector<int32_t> HOT_DRY = {54, 61, 72, 42, 73, 66, 83, 84, 102, 103, 88, 62, 36, 99};
const std::vector<int32_t> HOT_WET = {54, 89, 75, 74, 71, 64, 68, 60, 58, 67, 83,
                                      84, 25, 102, 104, 105, 88, 62, 36, 50, 22, 23};
const std::vector<int32_t> COLD = {28, 56, 65, 90, 93, 21, 49, 22, 23, 35};

// §3.4: a type's wild level range.
std::pair<int32_t, int32_t> levelRange(int32_t t) {
    auto in = [&](std::initializer_list<int32_t> ids) { return std::find(ids.begin(), ids.end(), t) != ids.end(); };
    if (in({19, 20, 25, 26, 30, 31, 32, 60, 63, 67, 69, 70}))
        return {3, 6};
    if (in({98, 100}))
        return {6, 9};
    if (in({37, 38, 39, 40, 53, 59, 88, 105, 106, 151}))
        return {9, 14};
    if (in({28, 61, 90, 155}))
        return {14, 21};
    if (in({36, 54, 64, 65, 66, 72, 73, 91, 99, 152, 153}))
        return {21, 31};
    if (in({21, 46, 47, 50, 58, 75, 80, 86, 89, 102, 104, 150}))
        return {31, 47};
    if (in({17, 49, 103, 113, 114, 154}))
        return {47, 81};
    if (in({81, 83, 84, 94}))
        return {81, 180};
    if (in({62, 82, 85, 97, 110, 112}))
        return {180, 19981}; // (as read)
    return {2, 4};
}

// §3.4: how many of a type go about together.
std::pair<int32_t, int32_t> groupRange(int32_t t) {
    auto in = [&](std::initializer_list<int32_t> ids) { return std::find(ids.begin(), ids.end(), t) != ids.end(); };
    if (in({21, 28, 42, 50, 55, 63, 69, 70, 87, 88}))
        return {1, 3};
    if (in({22, 35, 36, 54, 56, 60, 71, 72, 73, 98, 99, 100, 102, 103, 104, 105, 154}))
        return {1, 5};
    return {1, 1};
}

// §3.4: a companion's type, redrawn within a paired species.
int32_t variant(int32_t t, Random &random) {
    if (t == 0 || t == 1)
        return random.draw() % 2;
    for (int32_t first : {2, 4, 7, 9, 11, 13, 15, 22, 83})
        if (t == first || t == first + 1)
            return first + random.draw() % 2;
    if (t >= 37 && t <= 40)
        return 37 + random.draw() % 4;
    return t;
}

} // namespace

std::vector<int32_t> wildlifeList(float warm, float moist, float fire, float mountain, int32_t h, int32_t mode) {
    if (h < 0)
        return SEA_LIFE;
    if (mode != 0)
        return PEOPLE;
    if (fire > 0.1f)
        return FIRE;
    std::vector<int32_t> list = {21, 46};
    if (mountain > 0.3f)
        list.push_back(47);
    const std::vector<int32_t> *more = nullptr;
    if (h <= 3)
        more = &SHORE;
    else if (warm < 0.2f)
        more = &COLD;
    else if (warm < 0.8f)
        more = &TEMPERATE;
    else if (moist < 0.2f)
        more = &HOT_DRY;
    else if (moist >= 0.6f)
        more = &HOT_WET;
    if (more)
        list.insert(list.end(), more->begin(), more->end());
    return list;
}

// Spawn objects §6.1: zones with an even coordinate sum get a goddess statue (static kind 0)
// at a drawn column, standing on its highest solid block.
void Terrain::statuePass(Zone &zone, Random &random) const {
    if ((zone.zx + zone.zy) % 2 != 0)
        return;
    const int32_t x = 256 * zone.zx + 16 + random.draw() % 224;
    const int32_t y = 256 * zone.zy + 16 + random.draw() % 224;
    if (!zone.contains(x, y))
        return;
    const Column &column = zone.column(x, y);
    int32_t z = column.top();
    while (z >= column.bottom && clear(zone.read(x, y, z).type()))
        z--;
    StaticObject statue;
    statue.kind = 0;
    statue.x = ((int64_t)x << 16) + 32768, statue.y = ((int64_t)y << 16) + 32768, statue.z = (int64_t)(z + 1) << 16;
    statue.scale[0] = statue.scale[1] = 2, statue.scale[2] = 8;
    statue.rotation = random.draw() % 4;
    zone.statics.push_back(statue);
}

// Spawn objects §6.2: in zones of most location types, every fifth cell of a 14 x 14 grid draws
// whether it is a candidate for the location's creatures, more likely deep inside the location;
// then the creatures (LocationCreatures.cpp).
void Terrain::locationGrid(Zone &zone, Random &random) const {
    const int32_t zx = zone.zx, zy = zone.zy;
    const Region *region = w.region(floorDiv(zx, 64), floorDiv(zy, 64));
    const Location *Lz = region ? &region->records[8 * (size_t)((zx >> 3) & 7) + (size_t)((zy >> 3) & 7)] : nullptr;
    if (!Lz || Lz->type == 0 || Lz->type == 1 || Lz->type == 5 || Lz->type == 10 || Lz->type == 14)
        return;
    std::vector<std::array<int64_t, 3>> candidates;
    for (int32_t i = 0; i < 14; i++)
        for (int32_t j = 0; j < 14; j++) {
            if ((i + 3 * j) % 5 != 0)
                continue;
            const int32_t x = 256 * zx + 18 * i + 4, y = 256 * zy + 18 * j + 4;
            const float v = 1.f - w.locationDistanceFx(*Lz, (int64_t)x << 16, (int64_t)y << 16);
            const float weight = v > 0 ? v * v : 0.f;
            if ((float)random.draw() / 32767.f > 0.75f * weight || !zone.contains(x, y))
                continue;
            int32_t z = zone.column(x, y).belowSurface; // the first open cell from there up
            while (!clear(zone.read(x, y, z).type()))
                z++;
            candidates.push_back({((int64_t)x << 16) + 32768, ((int64_t)y << 16) + 32768, (int64_t)z << 16});
        }
    locationCreatures(zone, random, *Lz, candidates);
}

int32_t Terrain::wildlifeSpecies(int32_t x, int32_t y, int32_t h, int32_t mode, Random &random) const {
    const std::vector<int32_t> list = wildlifeList(w.warmth(x, y), w.moisture(x, y), w.fireShare(x, y), mountains(x, y), h, mode);
    return list[(size_t)random.draw() % list.size()];
}

void Terrain::wildlifePass(Zone &zone, Random &random) const {
    const int32_t zx = zone.zx, zy = zone.zy;
    const Region *region = w.region(floorDiv(zx, 64), floorDiv(zy, 64));
    const Location *Lz = region ? &region->records[8 * (size_t)((zx >> 3) & 7) + (size_t)((zy >> 3) & 7)] : nullptr;
    const auto inside = [&](const Location &L, int32_t x, int32_t y) {
        const float v = 1 - w.locationDistanceFx(L, (int64_t)x << 16, (int64_t)y << 16);
        return v;
    };
    // (Before the pass, believed a location guardian: a type-9 location centred in this zone
    // gets one creature of type 107 at its centre.)
    if (Lz && Lz->type == 9 && zone.contains((int32_t)(Lz->x >> 16), (int32_t)(Lz->y >> 16))) {
        Spawn g;
        g.x = Lz->x, g.y = Lz->y, g.z = (int64_t)(Lz->height * 65536.0);
        g.type = 107;
        g.level = Lz->level;
        g.powerTier = (uint8_t)Lz->subLevel;
        zone.spawns.push_back(g);
    }
    for (int32_t i = 0; i < 3; i++)
        for (int32_t j = 0; j < 3; j++) {
            // §3.1: a candidate column.
            if (random.draw() % 4 == 0)
                continue;
            const int32_t x = 256 * zx + 24 + 85 * i + random.draw() % 10;
            const int32_t y = 256 * zy + 24 + 85 * j + random.draw() % 10;
            if (Lz && Lz->type != 0 && Lz->type != 10) {
                const float v = inside(*Lz, x, y);
                if (v > 0 && v * v > 0.3f)
                    continue;
            }
            const Column &column = zone.column(x, y);
            const float warm = column.warmth, moist = column.moisture;
            if (warm < 0.2f && random.draw() % 4 == 0)
                continue;
            if (moist < 0.2f && random.draw() % 4 == 0)
                continue;
            bool nearObject = false;
            for (const ObjectMark &p : zone.objectList) {
                const float dx = (float)x - (float)((double)p.x / 65536.0), dy = (float)y - (float)((double)p.y / 65536.0);
                nearObject = nearObject || dx * dx + dy * dy < 400.f;
            }
            if (nearObject)
                continue;
            int32_t z = column.belowSurface;
            while (true) {
                const uint8_t t = zone.read(x, y, z).type();
                if (t == AIR || t == WATER)
                    break;
                z++;
            }
            const uint8_t ground = zone.read(x, y, z - 1).type();
            if (townness(x, y) > 0 || flatness(x, y) < 1)
                continue;
            Spawn s;
            s.x = ((int64_t)x << 16) + 32768, s.y = ((int64_t)y << 16) + 32768, s.z = (int64_t)z << 16;
            s.yaw = (float)random.draw() * 360.f / 32767.f;
            s.level = 1;
            s.type = wildlifeSpecies(x, y, z, 0, random);
            // §3.2: wild plants (inert, controller 6) on fire ground, snow and grass.
            int32_t plant = -1;
            bool hardy = false; // health multiplier 25
            if (ground == FIRE_GROUND) {
                plant = random.draw() % 2 ? 126 : 130;
            } else if (z >= 0 && ground == SNOW) {
                if (random.draw() % 4 == 0) {
                    static const int32_t SNOWY[4] = {121, 123, 125, 122};
                    plant = SNOWY[random.draw() % 4];
                    hardy = true;
                }
            } else if (z >= 0 && (ground == GRASS || ground == FIELD || ground == SAND)) {
                if (random.draw() % 3 != 0 && warm > 0.8f && moist < 0.1f) {
                    plant = random.draw() % 2 == 0 ? 124 : 128;
                } else if (ground == GRASS && random.draw() % 3 != 0 && moist > 0.1f) {
                    static const int32_t GREEN[4] = {120, 123, 127, 125};
                    plant = GREEN[random.draw() % 4];
                    hardy = true;
                }
            }
            if (plant >= 0) {
                s.type = plant;
                s.controller = 6;
                if (hardy)
                    s.multipliers[0] = 25;
                zone.spawns.push_back(s);
                continue;
            }
            // §3.3: an animal, its level and tier, its active hours and its companions.
            s.controller = 1;
            if (random.draw() % 100 == 0)
                s.appearanceFlags |= 0x200;
            const auto [lo, hi] = levelRange(s.type);
            s.level = lo + random.draw() % (hi - lo + 1);
            if (Lz && lo <= Lz->level && Lz->level <= hi && inside(*Lz, x, y) > 0)
                s.powerTier = (uint8_t)Lz->subLevel;
            s.windowStart = 21600000, s.windowEnd = 86400000;
            zone.spawns.push_back(s);
            const auto [glo, ghi] = groupRange(s.type);
            const int32_t k = glo - 1 + random.draw() % (ghi - glo + 1);
            for (int32_t c = 0; c < k; c++) {
                Spawn m;
                m.controller = 1;
                const float theta = (float)(c * 2 * 3.141592653589793 / k);
                m.x = s.x + (int64_t)(std::cos(theta) * 8 * 65536.0f);
                m.y = s.y + (int64_t)(std::sin(theta) * 8 * 65536.0f);
                m.z = s.z;
                m.type = variant(s.type, random);
                const auto [mlo, mhi] = levelRange(m.type);
                m.level = mlo + random.draw() % (mhi - mlo + 1);
                m.powerTier = s.powerTier;
                m.yaw = (float)random.draw() * 360.f / 32767.f;
                zone.spawns.push_back(m);
            }
        }
}

} // namespace worldgen
