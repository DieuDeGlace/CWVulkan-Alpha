#include "worldgen/Terrain.h"
#include "EngineModules.h"
#include "worldgen/Trees.h"
#include "worldgen/WaterFeatures.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace worldgen {
namespace {

constexpr float TWO_POW_MINUS_16 = 1.f / 65536.f;

float squared(float v) { return v * v; }

// A spawn object at world column (x, y) (behaviour spec §2.2: on the ravine floor).
// Caves §5.5 step 10: a ravine spawn stands at the middle of its column, on the whole block
// of the floor height.
Spawn spawnAt(Spawn s, int32_t x, int32_t y, float z) {
    s.x = ((int64_t)x << 16) + 32768;
    s.y = ((int64_t)y << 16) + 32768;
    s.z = (int64_t)(int32_t)z << 16;
    return s;
}
// 1 - (1 - t^2)^2, a recurring ease.
float ease(float t) { return 1.f - squared(1.f - t * t); }
glm::vec3 clamp255(glm::vec3 c) { return glm::clamp(c, glm::vec3(0), glm::vec3(255)); }
// Block bytes from float colour channels (trunc, §6).
Block toBlock(glm::vec3 c, uint8_t typeByte) {
    return {(uint8_t)(int32_t)c.r, (uint8_t)(int32_t)c.g, (uint8_t)(int32_t)c.b, typeByte};
}
glm::vec3 colourOf(Block b) { return {(float)b.r, (float)b.g, (float)b.b}; }

} // namespace

float Terrain::noiseAt(uint32_t ox, uint32_t oy, double k, int32_t x, int32_t y) const {
    return noise((double)w.offsets()[ox] + k * x, (double)w.offsets()[oy] + k * y);
}

Climate Terrain::climate(int32_t x, int32_t y, const Zone *zone) const {
    if (zone && uint32_t(x) < 0x01000000u && uint32_t(y) < 0x01000000u && zone->contains(x, y)) {
        const Column &c = zone->column(x, y);
        return {c.moisture, c.warmth, c.fire};
    }
    return {w.moisture(x, y), w.warmth(x, y), w.fireShare(x, y)};
}

// §3.1
float Terrain::townness(int32_t x, int32_t y) const {
    const Location *L = w.location(x, y);
    return L && L->type == 1 ? bump(w.locationDistance(*L, x, y)) : 0.f;
}

// §3.2
float Terrain::flatness(int32_t x, int32_t y) const {
    float value = w.channel(x, y);
    const float q = townness(x, y);
    if (q > 0) {
        // A street grid through towns: cos of 360 radians per grid unit.
        const glm::dvec2 g = w.grid(x, y);
        const float a = std::min(3 * q, 1.f);
        const float a2 = ease(a);
        for (double gc : {g.x, g.y})
            value = std::min(value, (float)(std::cos(gc * 360.0) * (double)a2 + 1.0));
    }
    if (q > 0.65f)
        value = (float)(value * std::max((0.7 - q) / 0.05000000074505806, 0.0));
    if (const Location *L = w.location(x, y); L && (L->type == 2 || L->type == 4))
        value += 2 * bump(w.locationDistance(*L, x, y));
    return value;
}

// §3.3
float Terrain::roughness(int32_t x, int32_t y) const {
    float v = std::fabs(noiseAt(0x170, 0x174, 0.01, x, y) * 0.1f + noiseAt(0x168, 0x16c, 0.001, x, y));
    v *= (noise(0.001 * x, 0.001 * y) + 1.f) * 0.1f + 0.8f;
    const float t = 1.f - 0.75f * w.channel(x, y);
    if (t > 0)
        v += 0.05f * t * t;
    if (const Location *L = w.location(x, y)) {
        const int32_t type = L->type;
        if (type == 1 || type == 2 || type == 4 || type == 13)
            v += bump(w.locationDistance(*L, x, y));
        else if (type == 6 || type == 7)
            v += 0.5f * bump(w.locationDistance(*L, x, y));
    }
    return v + w.fireShare(x, y);
}

// §3.4
float Terrain::mountains(int32_t x, int32_t y) const {
    const auto &o = w.offsets();
    const float a = noise(0.01 * x, 0.01 * y);
    const float a2 = 1.f - a * a;
    const float b = noise((double)o[0x1dc] + 0.01 * x + 843.0, (double)o[0x1e0] + 0.01 * y + 984.0) * 0.1f;
    const float c = noise(0.0025 * x + o[0x1dc], 0.0025 * y + o[0x1e0]);
    float M = 1.f - std::fabs(b + c) * ((1.f - a2 * a2) * 1.3f + 2.f);
    M *= 0.4f * noise(0.005 * x + 94.0, 0.005 * y + 874.0) + 0.6f;

    const Location *L = w.location(x, y);
    const float f = L ? w.locationDistance(*L, x, y) : 0.f;
    if (L && (L->type == 6 || L->type == 7)) {
        // Valleys keep mountain cover round their rim.
        const float s = M >= 1 ? 0.f : (M + 0.5f > 1 ? 1.f - M : 0.5f);
        if (f <= 0.36f) {
            const float k = std::min((1.f - f / 0.36f) * 1.5f, 1.f);
            M = (M + s) * (1.f - k * k);
        } else if (f < 1) {
            const float r = (float)std::sqrt((double)f);
            const float g = (r - 0.6f) / 0.3999999761581421f; // (1 - 0.6f, open points 11 §2)
            M = squared(1.f - g * g) * s + M;
        }
    }
    if (M < 0) {
        M = 0;
    } else {
        M *= smooth(std::min(2 * roughness(x, y), 1.f));
        M *= smooth(std::min(flatness(x, y), 1.f));
        const float town = 1.f - smooth(std::min(2 * townness(x, y), 1.f));
        M *= town * town * town;
    }
    if (L) {
        if (L->type == 2)
            return smooth(M) + bump(f);
        // Type 10: a round hole of radius r in the mountain cover round the centre (locations
        // spec §11.1).
        if (L->type == 10 && L->radius > 0) {
            const float ex = (float)(L->x - ((int64_t)x << 16)) * TWO_POW_MINUS_16;
            const float ey = (float)(L->y - ((int64_t)y << 16)) * TWO_POW_MINUS_16;
            const float d = (ey * ey + ex * ex) / (L->radius * L->radius);
            if (d < 1)
                M *= 1.f - (1.f - d) * (1.f - d);
        }
        if (L->type == 4 || L->type == 5) {
            if (f <= 0.25f)
                M = 0;
            else if (f < 1) {
                const float g = ((float)std::sqrt((double)f) - 0.5f) * 2.f;
                M = (1.f - squared(1.f - g * g)) * M;
            }
        }
    }
    return smooth(M);
}

// §4
float Terrain::height(int32_t x, int32_t y) const {
    // §4.1: large-scale terms.
    const float s1 = squared(unit(noiseAt(0x18c, 0x190, 0.0001, x, y)));
    const float s2 = squared(unit(noiseAt(0x194, 0x198, 0.0001, x, y)));
    const float s3 = squared(unit(noiseAt(0x19c, 0x1a0, 0.001, x, y)));
    const float s4 = squared(unit(noiseAt(0x1a4, 0x1a8, 0.001, x, y)));
    const float s5 = squared(unit(noiseAt(0x1ac, 0x1b0, 0.002, x, y)));
    const float g = squared(smooth(std::min(4 * roughness(x, y), 1.f)));
    const float m3 = g * s3, m4 = g * s4;
    float m5 = g * s5;

    // §4.2: the elevation of the region cells, blended over the displaced sample point.
    const glm::vec2 d = w.displacement(x, y);
    const int64_t Px = ((int64_t)x << 16) + (int64_t)(d.x * 65536.f);
    const int64_t Py = ((int64_t)y << 16) + (int64_t)(d.y * 65536.f);
    struct Candidate {
        float d2;
        int32_t elevation;
    };
    Candidate candidates[9];
    int count = 0;
    float nearest = 0;
    for (int32_t i = floorDiv(x - REGION, REGION); i <= floorDiv(x + REGION, REGION); i++)
        for (int32_t j = floorDiv(y - REGION, REGION); j <= floorDiv(y + REGION, REGION); j++) {
            const RegionCell *c = w.cell(i, j);
            if (!c)
                continue; // missing cells are skipped here
            const float ex = (float)(((int64_t)c->x << 16) - Px) * TWO_POW_MINUS_16;
            const float ey = (float)(((int64_t)c->y << 16) - Py) * TWO_POW_MINUS_16;
            const float d2 = ey * ey + ex * ex;
            if (count == 0 || d2 < nearest)
                nearest = d2;
            candidates[count++] = {d2, c->elevation};
        }
    float E = 0, land = 1;
    if (count) {
        float weights = 0, elevations = 0, lands = 0;
        for (int k = 0; k < count; k++) {
            const float t = std::min((candidates[k].d2 - nearest) * 5.000000058e-08f, 1.f);
            const float wc = squared(1.f - t);
            weights += wc;
            elevations += (float)candidates[k].elevation * wc;
            if (candidates[k].elevation > 0)
                lands += wc;
        }
        E = elevations / weights;
        land = lands / weights;
    }

    // §4.3: assembly.
    const float n6 = noiseAt(0x1bc, 0x1c0, 0.0002, x, y), n7 = noiseAt(0x1b4, 0x1b8, 0.0002, x, y);
    float H = ((n6 + 1.f) * 100.f * s2 + (n7 + 1.f) * 100.f * s1) * land + E;
    const float q = townness(x, y);
    if (q > 0.5f)
        m5 *= 1.f - smooth(std::min((q - 0.5f) * 2.f, 1.f));
    const float c = std::min(std::max(flatness(x, y), 0.02f) * 2.f, 1.f);
    const float u = 1.f - c;
    const float g2 = 1.f - u * u * u * u;
    float A = (0.1f * g2 + 0.9f) * m3, Bm = (0.1f * g2 + 0.9f) * m4, C = (0.5f * g2 + 0.5f) * m5;
    const Location *L = w.location(x, y);
    if (L && L->type == 1) {
        const float k = 1.f - 0.5f * bump(w.locationDistance(*L, x, y));
        A *= k;
        Bm *= k;
        C *= k;
    }
    H = ((noiseAt(0x1cc, 0x1d0, 0.002, x, y) + 1.f) * 50.f * Bm + (noiseAt(0x1c4, 0x1c8, 0.002, x, y) + 1.f) * 50.f * A) +
        (noiseAt(0x1d4, 0x1d8, 0.01, x, y) + 1.f) * 20.f * C + H;

    if (!L || L->type == 0)
        return H;
    // §4.3 step 6: location rules, in order.
    if (const RegionCell *O = w.owner((int32_t)(L->x / 65536), (int32_t)(L->y / 65536));
        O && O->elevation < 0 && L->type != 11) {
        // Islands rise out of sea cells round a location.
        const double ax = 0.0025 * x, ay = 0.0025 * y;
        const int64_t DX = ((int64_t)x << 16) + (int64_t)(noise(ax + 8432984.0, ay + 90493.0) * 100.f * 65536.f) - L->x;
        const int64_t DY = (int64_t)((noise(ax + 3423.0, ay + 112.0) * 100.f + (float)y) * 65536.f) - L->y;
        const float ex = (float)DX * TWO_POW_MINUS_16, ey = (float)DY * TWO_POW_MINUS_16;
        const float rho = std::min((ex * ex + ey * ey) / squared(L->radius + 256.f), 1.f);
        if (1.f - rho > 0)
            H -= smooth(std::min((1.f - rho) * 1.1f, 1.f)) * (float)O->elevation;
    }
    const float f = w.locationDistance(*L, x, y);
    const float lakeLevel = L->height - 25.f;
    auto rim = [&] { return squared(1.f - squared(((float)std::sqrt((double)f) - 0.5f) * 2.f)); };
    switch (L->type) {
    case 4: // lake basin
        if (f <= 0.25f) {
            const float u4 = squared(1.f - 4.f * f);
            H = (1.f - u4) * lakeLevel + u4 * (L->height - 50.f);
        } else if (f < 1)
            H = (lakeLevel - H) * rim() + H;
        break;
    case 6:
    case 7: // valley
        if (f <= 0.25f) {
            const float u4 = squared(1.f - 4.f * f);
            H = (1.f - u4) * (H + 10.f) + u4 * (H - 30.f);
        } else if (f < 1)
            H = rim() * 10.f + H;
        H = std::max(H, E);
        break;
    case 13: // spire
        if (f <= 0.01f)
            return H + 150.f;
        if (f < 1) {
            const float wv = ((float)std::sqrt((double)f) - 0.1f) / 0.9f;
            return H + 150.f * squared(squared(1.f - wv * wv));
        }
        break;
    }
    return H;
}

// §6.2
glm::vec3 Terrain::grass(int32_t x, int32_t y) const {
    const float a = unit(noise(0.04 * x, 0.04 * y));
    const float p = 120.f * noise(0.005 * x + 45645.0, 0.005 * y + 456456.0);
    const float k = 80.f * noise(0.02 * x + 89648.0, 0.02 * y + 1649.0) + 1.f;
    const float e = 30.f * noise(0.005 * x + 342.0, 0.005 * y + 23423.0);
    return clamp255({(k * a + 200.f * (1.f - a)) - std::max(p, 0.f), (120.f * a + 230.f * (1.f - a)) + e,
                     (k * a + k * (1.f - a)) + p});
}

// §6.3
glm::vec3 Terrain::sand(int32_t x, int32_t y) const {
    const float a80 = 80.f * unit(noiseAt(0x274, 0x278, 0.03, x, y));
    const float b = unit(noiseAt(0x27c, 0x280, 0.003, x, y));
    const float g = 40.f * noise(0.01 * x + 493.0, 0.01 * y + 789.0) + 200.f;
    return clamp255({((1.f - b) * 255.f + 255.f * b) + a80, ((1.f - b) * g + 150.f * b) + a80,
                     ((1.f - b) * 100.f + 50.f * b) + a80});
}

// §6.4
glm::vec3 Terrain::beach(int32_t x, int32_t y) const {
    const float a80 = 80.f * unit(noiseAt(0x274, 0x278, 0.03, x, y));
    const float b = unit(noiseAt(0x27c, 0x280, 0.01, x, y));
    return clamp255({((1.f - b) * 240.f + 240.f * b) + a80, ((1.f - b) * 240.f + 180.f * b) + a80,
                     ((1.f - b) * 100.f + 50.f * b) + a80});
}

// §6.5
glm::vec3 Terrain::snow(int32_t x, int32_t y) const {
    const float a = unit(noise(0.04 * x, 0.04 * y));
    return clamp255({190.f * a + 100.f * (1.f - a), 220.f * a + 180.f * (1.f - a), 255.f * a + 255.f * (1.f - a)});
}

// §6.6
glm::vec3 Terrain::rock(int32_t x, int32_t y, int32_t z, const Zone *zone) const {
    const Climate cl = climate(x, y, zone);
    const float v = unit((noise(0.1 * y + 98984.0, 0.4 * z) + noise(0.1 * x, 0.4 * z)) * 0.5f);
    glm::vec3 base(120, 120, 130);
    const float wm = cl.warmth;
    if (wm < 0.5f) {
        const float t = 2.f * wm;
        base = {10.f * (1.f - t) + 160.f * t, 20.f * (1.f - t) + 160.f * t, 50.f * (1.f - t) + 170.f * t};
    } else if (wm > 0.75f) {
        const float t = 4.f * (wm - 0.75f);
        const float n = noise(0.001 * t + 6544.0, 0.001 * y + 123.0); // (the first argument does use t)
        base = {120.f * (1.f - t) + 200.f * t, 120.f * (1.f - t) + (20.f * n + 130.f) * t,
                130.f * (1.f - t) + 100.f * t};
    }
    glm::vec3 C = base + 160.f * v;
    C.b += 10.f * (noiseAt(0x22c, 0x230, 0.005, x, y) + 1.f);
    C = clamp255(C);
    if (const float F = cl.fire; F > 0) {
        const float s = std::min(0.1f * noise(0.05 * x, 0.05 * y) + noise(0.02 * x, 0.02 * y), 1.f);
        const float u = 1.f - std::fabs(s);
        const float u5 = u * u * u * u * u;
        C.r = C.r * (1.f - F) + (200.f * u5 + 10.f) * F;
        C.g = C.g * (1.f - F) + ((noise(0.1 * x, 0.1 * y) + 1.f) * u5 * 50.f + 10.f) * F;
        C.b = C.b * (1.f - F) + 10.f * F;
    }
    return C;
}

// §6.7
glm::vec3 Terrain::cliff(int32_t x, int32_t y, int32_t z, const Zone *zone) const {
    const Climate cl = climate(x, y, zone);
    const float v = unit((noise(0.1 * y + 98984.0, 0.4 * z + 8437.0) + noise(0.1 * x, 0.4 * z)) * 0.5f);
    const float n25c = noiseAt(0x25c, 0x260, 0.01, x, y), n264 = noiseAt(0x264, 0x268, 0.01, x, y);
    const float n26c = noiseAt(0x26c, 0x270, 0.01, x, y), n24c = noiseAt(0x24c, 0x250, 0.01, x, y);
    const float n254 = noiseAt(0x254, 0x258, 0.01, x, y);
    glm::vec3 C = clamp255({60.f * v + (20.f * n24c + 180.f) + 20.f * n25c, 60.f * v + (20.f * n254 + 100.f) + 20.f * n264,
                            (0.f * v + 60.f) + 20.f * n26c});
    const float wm = cl.warmth;
    if (wm > 0.75f) {
        const float t = 4.f * (wm - 0.75f);
        C = C * (1.f - t) + rock(x, y, z, zone) * t * 0.9f;
    }
    if (wm < 0.2f) {
        const float s = 1.f - 4.f * wm;
        C = C * (1.f - s) + (rock(x, y, z, zone) + snow(x, y)) * (0.5f * s);
    }
    if (cl.fire > 0)
        C = C * (1.f - cl.fire) + rock(x, y, z, zone) * cl.fire;
    return C;
}

// §6.1
Block Terrain::surface(int32_t x, int32_t y, int32_t z, const Zone *zone) const {
    return surface(x, y, z, climate(x, y, zone), zone);
}

glm::vec3 Terrain::stone(int32_t x, int32_t y, int32_t z) const {
    const float v = (float)((float)((noise(0.01 * y + 98984.0, 0.3 * z + 8437.0) + noise(0.01 * x, 0.3 * z)) * 0.5f) + 1.f) * 0.5f;
    const float base = 200.f * v + 50.f;
    return clamp255({base + 20.f * noiseAt(0x234, 0x238, 0.005, x, y), base + 20.f * noiseAt(0x23c, 0x240, 0.005, x, y),
                     base + 20.f * noiseAt(0x244, 0x248, 0.005, x, y)});
}

Block Terrain::surface(int32_t x, int32_t y, int32_t z, const Climate &cl, const Zone *zone) const {
    const float mo = cl.moisture, wm = cl.warmth;
    uint8_t type = GRASS;
    glm::vec3 C = grass(x, y);
    const float rk = sat((10.f * roughness(x, y) - 0.3f) * 1.5f);
    if (wm > 0.8f) { // hot rock
        const float a = std::min((wm - 0.8f) / 0.1f, 1.f);
        const float n = sat(noise(0.03 * x, 0.03 * y));
        const float b = ease(n) * 0.8f * a;
        C = C * (1.f - b) + rock(x, y, z, zone) * b;
    }
    if (mo < 0.2f && wm > 0.75f) { // desert
        type = SAND;
        float d = sat((1.f - mo / 0.2f) * (wm - 0.75f) * 4.f * 10.f);
        if (const Location *L = w.location(x, y); L && L->type == 4) {
            const float g = sat(w.locationDistance(*L, x, y) - 0.5f);
            d *= 1.f - squared(1.f - g * g);
        }
        d *= rk;
        if (d > 1)
            d = 1;
        else if (d < 0.5f)
            type = GRASS;
        C = C * (1.f - d) + sand(x, y) * d;
    }
    { // beach
        const float g = sat(noise(0.01 * x + 854.0, 0.01 * y + 985.0) + (float)(15 - z) / 10.f - 0.5f);
        float g2 = ease(g);
        if (wm < 0.2f)
            g2 *= wm / 0.2f;
        if (g2 > 0) {
            C = C * (1.f - g2) + beach(x, y) * g2;
            type = SAND;
        }
    }
    if (wm < 0.3f) { // snow
        const float k = std::min(1.f - (wm - 0.29f) / 0.01f, 1.f);
        type = SNOW;
        C = C * (1.f - k) + snow(x, y) * k;
    }
    if (const float F = cl.fire; F > 0) {
        C = C * (1.f - F) + rock(x, y, z, zone) * F;
        if (F > 0.5f)
            type = FIRE_GROUND;
    }
    return toBlock(C, type | SURFACE_COLOURED);
}

Block Terrain::rockBlock(int32_t x, int32_t y, int32_t z, const Zone &zone) const {
    return toBlock(rock(x, y, z, &zone), ROCK);
}

void Terrain::prepareRegion(int32_t rx, int32_t ry) const {
    w.prepareRegion(rx, ry, [this](int32_t x, int32_t y) { return height(x, y); });
}

std::unique_ptr<Zone> Terrain::generate(int32_t zx, int32_t zy) const {
    // §5.1 step 1: the regions round the zone's own.
    for (int32_t i = -1; i <= 1; i++)
        for (int32_t j = -1; j <= 1; j++)
            prepareRegion((zx >> 6) + i, (zy >> 6) + j);
    auto zone = std::make_unique<Zone>();
    zone->zx = zx;
    zone->zy = zy;
    // §5.1 step 2: the zone's own random stream.
    Random random((uint32_t)w.A() + 65536u * (uint32_t)zy + (uint32_t)zx);

    // §5.2: climate per column.
    const int32_t x0 = zone->originX(), y0 = zone->originY();
    for (int32_t x = x0; x < x0 + Zone::SIZE; x++)
        for (int32_t y = y0; y < y0 + Zone::SIZE; y++) {
            Column &c = zone->column(x, y);
            c.moisture = w.moisture(x, y);
            c.warmth = w.warmth(x, y);
            c.fire = w.fireShare(x, y);
        }
    // §5.3: base heights, one row and column past the zone.
    std::vector<float> heights(257 * 257);
    for (int32_t i = 0; i <= 256; i++)
        for (int32_t j = 0; j <= 256; j++)
            heights[i * 257 + j] = height(x0 + i, y0 + j);

    std::vector<BoulderSeed> ravineBoulders;
    surfacePass(*zone, heights, random, ravineBoulders);
    postPasses(*zone, heights, random, ravineBoulders);
    treeGrid(*zone, random);
    statuePass(*zone, random);
    locationGrid(*zone, random);
    wildlifePass(*zone, random);
    plantPass(*zone, random);
    // Spawn objects §3: the finishing pass, the last user of the zone's stream, over every spawn
    // in list order.
    if (finishSpawn)
        for (size_t i = 0; i < zone->spawns.size(); i++)
            finishSpawn(zone->spawns[i], random, true);
    // Without it, item services §3.4 (0x5ed35c) alone: every boss spawn carries one spirit, which
    // it drops when it dies: fire, unholy, ice or wind by one draw, rarity 2, the spawn's level.
    for (Spawn &s : zone->spawns)
        if (!finishSpawn && (s.appearanceFlags & 0x200)) {
            ItemRecord spirit;
            spirit.type = 11, spirit.subtype = 14;
            spirit.material = (uint8_t)(128 + random.draw() % 4);
            spirit.rarity = 2;
            spirit.level = (int16_t)s.level;
            s.inventory.push_back(spirit);
        }
    // Open points 5 §4.1: generated items sink to what is under them, through water too.
    for (GroundItem &item : zone->items)
        while (clear(zone->read(item.x, item.y, item.z - 1).type()))
            item.z--;
    av::modules::applyTerrainPasses(*zone, *this);
    if (swimmableWater)
        makeWaterSwimmable(*zone);
    blockLight(*zone);
    return zone;
}

// §5.4 - §5.6
void Terrain::surfacePass(Zone &zone, const std::vector<float> &heights, Random &random,
                          std::vector<BoulderSeed> &ravineBoulders) const {
    const int32_t x0 = zone.originX(), y0 = zone.originY();
    const Location *Lz = w.location(x0, y0); // the zone's own location (sub-cell of its corner)
    auto rockAt = [&](int32_t x, int32_t y) { return [&, x, y](int32_t z) { return rockBlock(x, y, z, zone); }; };

    for (int32_t i = 0; i < Zone::SIZE; i++)
        for (int32_t j = 0; j < Zone::SIZE; j++) {
            const int32_t x = x0 + i, y = y0 + j;
            const float h = heights[i * 257 + j];
            // §5.4
            const bool steep = std::fabs((double)(h - heights[(i + 1) * 257 + j])) > 0.3 ||
                               std::fabs((double)(h - heights[i * 257 + j + 1])) > 0.3;
            const float P = mountains(x, y);
            const double n1 = noise(0.04 * x + 432.0, 0.04 * y + 432.0), n2 = noise(0.08 * x + 432.0, 0.08 * y + 432.0);
            const float v1 = (float)(1.0 - std::fabs(n1 + 0.05 * n2));
            const double n3 = noise(0.08 * x + 4234.0, 0.08 * y + 234.0), n4 = noise(0.05 * x + 423432.0, 0.05 * y + 54352.0);
            const float v2 = (float)(1.0 - std::fabs(0.05 * n3 + n4));
            const float T = squared(1.f - v2 * v2 * v2) + (squared(1.f - v1 * v1 * v1) + 0.f);
            float P2 = P;
            if (Lz && (Lz->type == 6 || Lz->type == 13)) {
                const float k = std::min(2.f * bump(w.locationDistance(*Lz, x, y)), 1.f);
                const float m = sat(1.3f * noise(0.01 * x + 985.0, 0.01 * y + 98584.0) + 1.f);
                P2 = smooth(m) * ease(k) * 0.4f + P;
            }
            const float G = P2 * T;
            const float cover = smooth(sat(1.25f * G));
            const float S = (float)((noise(0.01 * x + 34432.0, 0.01 * y + 8992.0) + 1.5) * 60.0) * P + 8.f * G + h;
            const int32_t zs = (int32_t)S;

            Block top = surface(x, y, zs, &zone);
            uint8_t type = top.typeByte;
            if (cover > 0.5f)
                type = ROCK;
            const glm::vec3 rk = rock(x, y, zs, &zone);
            const glm::vec3 c2 = glm::trunc(colourOf(top) * (1.f - cover) + rk * cover);
            Column &column = zone.column(x, y);
            column.bottom = zs;
            const float r8 = std::min(8.f * P2, 1.f);
            const Block upper = toBlock(c2, type);
            const Block lower =
                steep ? upper : toBlock(cliff(x, y, zs, &zone) * (1.f - r8) + (c2 + rk) * (0.5f * r8), ROCK);
            column.blocks = {lower, upper};
            column.belowSurface = zs - 8;

            // §5.5: ravines.
            const float Lr = (noise(0.01 * x + 435.0, 0.01 * y + 847.0) + 1.f) * 20.f + h;
            bool lakeFill = true;
            if (S > Lr) {
                const auto &o = w.offsets();
                const float v = 1.f - std::fabs(noise(0.005 * x + o[0x178], 0.005 * y + o[0x17c]) +
                                                noise(o[0x178] + 0.01 * x, o[0x17c] + 0.01 * y)) * 4.f;
                if (v < 0)
                    continue; // (the lake fill is skipped too)
                const float u = 1.f - v * v;
                const float w1 = 1.f - u * u, w2 = 1.f - u * u * u * u;
                const float t = sat((10.f * noise(0.03 * x + 7635.0, 0.03 * y + 123847.0) + ((S - Lr) - 20.f)) / 10.f);
                const float k = ease(t);
                const float a = 1.f - t;
                const float a7 = a * a * a * a * a * a * a;
                const float topZ = ((20.f * P + 4.f) * w1 + Lr) * (1.f - a7) + (S + 2.f) * a7;
                const float mfl = 6.f * T * P + Lr;
                const float floorZ = (1.f - w2) * S + w2 * mfl + 2.f;
                if (mfl < S - 2.f)
                    for (int32_t k2 = 0; k2 < 2; k2++) { // the ravine's walls darken towards rock
                        const Block b = zone.read(x, y, zs + k2);
                        const glm::vec3 c = colourOf(b) * k + rock(x, y, (int32_t)((float)k2 + S), &zone) * (1.f - k);
                        zone.place(x, y, zs + k2, toBlock(c, b.typeByte), rockAt(x, y));
                    }
                if (topZ <= floorZ)
                    continue;
                for (int32_t z = (int32_t)floorZ; (float)z < topZ; z++)
                    zone.place(x, y, z, z < 1 ? Block{0, 0, 0, WATER} : Block{0, 0, 0, (uint8_t)(topZ < S ? DRY_AIR : AIR)},
                               rockAt(x, y));
                const int32_t zf = (int32_t)(floorZ - 1.f);
                const Block floorSurface = surface(x, y, zf, &zone);
                const uint8_t floorType = cover > 0.5f || k > 0.5f ? (uint8_t)ROCK : floorSurface.typeByte;
                const glm::vec3 rf = rock(x, y, zf, &zone);
                const glm::vec3 c3 =
                    glm::trunc(colourOf(floorSurface) * (1.f - k) + (rf + cliff(x, y, zf, &zone)) * (0.4f * k));
                zone.place(x, y, zf, toBlock(c3 * (1.f - cover) + rf * cover, floorType), rockAt(x, y));
                // Ravine spawns (caves spec §7, behaviour spec §2.2).
                if (topZ - floorZ > 2.f && (x + 3 * y) % 7 == 0 && random.draw() % 30 == 0) {
                    switch (random.draw() & 3) {
                    case 0: // a glowing mushroom; its angle is in radians (open points 12 §7.3)
                        if (topZ <= S && (random.draw() & 3) == 0)
                            zone.items.push_back({x, y, (int32_t)floorZ, 11, 19,
                                                  (float)((double)random.draw() * 6.283185307179586 / 32767.0), 0.1f});
                        break;
                    case 1: // a mineral deposit
                        if (topZ <= S) {
                            Spawn deposit;
                            switch (random.draw() % 10) {
                            case 0: deposit.type = 131; break; // gold
                            case 1: deposit.type = 133; break; // silver
                            case 3: {
                                const int32_t d = random.draw() % 100;
                                deposit.type = d == 0 ? 138 : d <= 3 ? 137 : d <= 8 ? 136 : 135;
                                break;
                            }
                            default: deposit.type = 132; break; // iron
                            }
                            deposit.controller = 6;
                            deposit.level = 1;
                            deposit.multipliers[0] = 25;
                            zone.spawns.push_back(spawnAt(deposit, x, y, floorZ));
                        }
                        break;
                    case 2: // a bat
                        if (topZ <= S) {
                            Spawn bat;
                            bat.type = 59;
                            bat.level = w.levelAt(x, y); // (the block corner, open points 10 §4)
                            zone.spawns.push_back(spawnAt(bat, x, y, floorZ));
                        }
                        break;
                    case 3:
                        ravineBoulders.push_back({x, y, (int32_t)(floorZ + 2.f)});
                        break;
                    }
                }
                lakeFill = true;
            }
            // §5.6: lake fill.
            if (lakeFill && Lz && Lz->type == 4 && w.locationDistance(*Lz, x, y) < 0.25f)
                for (int32_t z = (int32_t)(Lz->height - 25.f); z >= column.bottom; z--) {
                    const uint8_t t = zone.read(x, y, z).type();
                    zone.place(x, y, z, clear(t) ? Block{255, 255, 255, 0x82} : toBlock(rock(x, y, z, &zone), LAKE_BED),
                               rockAt(x, y));
                }
        }
}

// Vegetation §5: the tree grid, which builds each tree it places (trees spec).
void Terrain::treeGrid(Zone &zone, Random &random) const {
    const int32_t x0 = zone.originX(), y0 = zone.originY();
    const Location *Lz = w.location(x0, y0);
    for (int32_t i = 0; i < 14; i++)
        for (int32_t j = 0; j < 14; j++) {
            const int32_t gx = x0 + 18 * i, gy = y0 + 8 + 18 * j;
            const float bonus = Lz && Lz->type == 3 ? bump(w.locationDistance(*Lz, gx + 8, gy)) : 0.f;
            const float mo = climate(gx + 8, gy, &zone).moisture;
            int32_t s = (int32_t)((float)(random.draw() % 5) + 2.f * mo + 6.f + 4.f * bonus);
            int32_t t = (int32_t)(((float)random.draw() * 8.f / 32767.f + 6.f * bonus + 8.f) * (0.5f * mo + 1.f));
            int32_t x = gx + s, y = gy - 8 + s; // (the sample y less 8: 256 zy + 18 j + s)
            const float hx = (float)(x - HOME), hy = (float)(y - HOME);
            if (hx * hx + hy * hy < 400.f)
                continue; // keep the home point clear
            x = std::min(x, x0 + 256 - s / 2);
            y = std::min(y, y0 + 256 - s / 2);
            bool nearObject = false; // §5 step 8: keep clear of camps and stone circles
            for (const ObjectMark &o : zone.objectList) {
                const float ey = (float)((int64_t)y * 65536 - o.y) * TWO_POW_MINUS_16;
                const float ex = (float)((int64_t)x * 65536 - o.x) * TWO_POW_MINUS_16;
                nearObject = nearObject || ey * ey + ex * ex < 1600.f;
            }
            if (nearObject)
                continue;

            const float n = noiseAt(0x294, 0x298, 0.001, x, y);
            int32_t kind = (random.draw() & 1) ? 5 : 0;
            if (n <= 0.3f) {
                if (random.draw() % 10 == 0)
                    kind = 1;
            } else if (random.draw() % 10 != 0)
                kind = 1;
            if (random.draw() % 10 == 0)
                kind = 2;
            const float wm = climate(x, y, &zone).warmth;
            if (wm > 0.8f && mo > 0.7f) {
                kind = 4 + ((random.draw() & 1) != 0);
                if ((random.draw() & 3) == 0)
                    kind = 3;
            }
            if (wm >= 0.3f && wm < 0.7f) {
                if (noise(0.001 * x + 8473.0, 0.001 * y + 9438.0) > 0.8f && random.draw() % 5 != 0)
                    kind = 2;
            } else if (wm < 0.3f) {
                if (wm > 0.2f) {
                    if (noise(0.001 * x + 8473.0, 0.001 * y + 9438.0) > 0.6f && random.draw() % 5 != 0)
                        kind = 2;
                } else
                    kind = 1;
            }
            if (kind == 2)
                t += random.draw() % std::max(t / 2, 1);
            else if (kind == 1)
                s = std::max(std::min(s, (int32_t)(0.5f * (float)t)), 1);
            if (x + s > x0 + 256)
                x = x0 + 256 - s;
            if (y + s > y0 + 256)
                y = y0 + 256 - s;

            const float p = std::min(sat(1.2f * noiseAt(0x1f4, 0x1f8, 0.008, x, y)) + 0.2f * mo * mo + 0.02f, 1.f) + bonus;
            if ((float)random.draw() / 32767.f > p)
                continue;
            if (!zone.contains(x, y))
                continue;
            // The ground: down from the top to the first block that is not air or water.
            const Column &c = zone.column(x, y);
            int32_t ground = c.bottom - 1;
            for (int32_t z = c.top() - 1; z >= c.bottom; z--)
                if (!clear(zone.read(x, y, z).type())) {
                    ground = z;
                    break;
                }
            if (ground < c.bottom)
                continue;
            // Trees spec §1.2: hot, dry ground grows palms.
            const Climate here = climate(x, y, &zone);
            if (here.warmth > 0.8f && here.moisture < 0.2f)
                kind = 3;
            const uint8_t type = zone.read(x, y, ground).type();
            if (ground >= 0 && (type == GRASS || type == SNOW || (type == SAND && ground + 1 < 3))) {
                zone.trees.push_back({x, y, ground + 1, s, t, kind});
                TreeBuilder(*this, zone, random).build(x, y, ground + 1, std::max(s, 1), t, kind);
            }
        }
}

// Vegetation §2 - §4.
void Terrain::plantPass(Zone &zone, Random &random) const {
    const int32_t x0 = zone.originX(), y0 = zone.originY();
    auto draw = [&] { return random.draw(); };
    auto spin = [&] { return (float)draw() * 360.f / 32767.f; };
    auto floatArgNoise = [](float a, float b) { return noise((double)a, (double)b); };

    for (int32_t x = x0; x < x0 + Zone::SIZE; x++)
        for (int32_t y = y0; y < y0 + Zone::SIZE; y++) {
            const Column &column = zone.column(x, y);
            const int32_t count = (int32_t)column.blocks.size();
            const Climate cl = climate(x, y, &zone);
            for (int32_t i = 0; i < count; i++) {
                // (The stored array, open points 12 §2: past the top it is type 0 at any height,
                // never the implicit water.)
                const int32_t zt = column.bottom + i + 1;
                const uint8_t above = i + 1 < count ? column.blocks[i + 1].type() : (uint8_t)AIR;
                if (i < count - 1 && !clear(above))
                    continue;
                const uint8_t here = column.blocks[i].type();

                if (here == TOWN_GROUND && above == AIR) { // §3.1: street lights
                    if (townness(x, y) > 0.75f && (90 * y + x) % 470 == 0 && (draw() & 15) == 0) {
                        bool room = true;
                        for (int32_t z = zt; z < zt + 7; z++)
                            room = room && clear(zone.read(x, y, z).type());
                        if (room) {
                            StaticObject light;
                            const bool hot = cl.warmth > 0.8f;
                            light.kind = hot ? 51 : 50;
                            light.x = (int64_t)x * 65536;
                            light.y = (int64_t)y * 65536;
                            light.z = (int64_t)zt * 65536;
                            light.rotation = draw() & 3;
                            light.scale[0] = light.scale[1] = hot ? 1.f : 2.f;
                            light.scale[2] = 8;
                            zone.statics.push_back(light);
                        }
                    }
                    continue;
                }
                if (here == LAKE_BED && above == AIR) { // §3.2: reeds
                    if (cl.warmth > 0.2f &&
                        std::fabs(floatArgNoise(0.05f * (float)x + 9843.f, 0.05f * (float)y + 8437.f)) > 0.5f) {
                        if ((draw() & 7) == 0)
                            zone.plants.push_back({x, y, zt, 0.09f, spin(), 22, 4});
                    }
                    continue;
                }
                if (here == WATER && above == AIR) { // §3.3: water lilies
                    if (cl.warmth > 0.2f && zt > 0 &&
                        std::fabs(floatArgNoise(0.05f * (float)x + 24234.f, 0.05f * (float)y + 53565.f)) > 0.7f) {
                        if (draw() % 10 == 0) {
                            Plant lily{x, y, zt, 0.09f, spin()};
                            lily.kind = 31 + (draw() & 1);
                            lily.sink = true;
                            zone.plants.push_back(lily);
                        }
                    }
                    continue;
                }
                const bool ground = here == GRASS || here == SAND || here == SNOW || here == FIRE_GROUND;
                if (!ground || !clear(above))
                    continue;
                // §3.4: ground cover.
                if (std::fabs(floatArgNoise(0.05f * (float)x + 9843.f, 0.05f * (float)y + 8437.f)) <= 0.6f)
                    continue;
                if ((draw() & 7) != 0)
                    continue;
                const float nA = noise(0.01 * x + 9843.0, 0.01 * y + 8437.0);
                if (zt <= -5) {
                    Plant p{x, y, zt, 0.1f, (float)((draw() & 3) * 90)};
                    if (nA <= 0) {
                        p.kind = 7;
                        p.flags = 4;
                    } else {
                        p.kind = 5 + (draw() & 1);
                        if (p.kind == 5) {
                            p.scale = 0.075f;
                            p.flags = 4;
                        }
                    }
                    p.scale *= (float)draw() / 32767.f + 1.f;
                    zone.plants.push_back(p);
                    continue;
                }
                if (zt < 1)
                    continue;
                if (here == FIRE_GROUND) {
                    Plant p{x, y, zt, 0.075f, (float)((draw() & 3) * 90)};
                    if (nA > 0) {
                        p.kind = 9;
                        p.flags = 4;
                    } else {
                        p.kind = 10;
                        p.scale = (float)(draw() * 0.02 / 32767.0 + 0.03);
                    }
                    zone.plants.push_back(p);
                    continue;
                }
                if (here == GRASS || here == SAND) {
                    const float fineFloat = floatArgNoise(0.01f * (float)x + 34234.f, 0.01f * (float)y + 234234.f);
                    const float fineDouble = noise(0.01 * x + 34234.0, 0.01 * y + 234234.0);
                    if (cl.warmth > 0.75f) {
                        if (cl.moisture <= 0.25f) {
                            const int r = draw();
                            if (r % 100 == 0) {
                                Plant p{x, y, zt, 0.075f, (float)((draw() & 3) * 90)};
                                p.kind = 28 - ((draw() & 1) != 0 ? 1 : 0);
                                zone.plants.push_back(p);
                            }
                        } else {
                            Plant p{x, y, zt, 0.075f, spin()};
                            if (nA > 0)
                                p.kind = fineFloat > 0 ? 4 : 3;
                            else
                                p.kind = fineDouble <= 0.5f ? 12 : 11;
                            if (p.kind == 2 || p.kind == 3)
                                p.flags = 4;
                            else if (p.kind == 11) {
                                p.flags = 4;
                                p.scale = (float)draw() * 0.05f / 32767.f + 0.05f;
                            } else if (p.kind == 12) {
                                p.flags = 4;
                                p.scale = (float)draw() * 0.02f / 32767.f + 0.1f;
                            }
                            if (here == GRASS || p.kind == 2 || p.kind == 3)
                                zone.plants.push_back(p);
                        }
                    } else {
                        Plant p{x, y, zt, 0.075f, (float)((draw() & 3) * 90)};
                        if (nA > 0)
                            p.kind = cl.moisture > 0.5f ? (fineFloat > 0 ? 12 : 4) : (fineFloat > 0 ? 2 : 3);
                        else
                            p.kind = fineDouble <= 0 ? 1 : 0;
                        if (p.kind == 2 || p.kind == 3 || p.kind == 4 || p.kind == 12)
                            p.flags = 4;
                        if (p.kind == 12)
                            p.scale = (float)draw() * 0.02f / 32767.f + 0.1f;
                        if (here == GRASS || p.kind == 2 || p.kind == 3 || p.kind == 4)
                            zone.plants.push_back(p);
                    }
                }
                // §4: ground items.
                if (here == GRASS) {
                    const int r = draw();
                    if (r % 150 == 0) {
                        GroundItem item{x, y, zt, 11, 15, (float)draw() / 32767.f * 360.f, 1};
                        if (draw() & 1) {
                            item.subtype = 22;
                            item.scale = 0.1f;
                        }
                        zone.items.push_back(item);
                    }
                } else if (here == SNOW) {
                    if (draw() % 300 == 0) {
                        GroundItem item{x, y, zt, 11, 24, (float)draw() / 32767.f * 360.f, 0.1f};
                        draw(); // (its value always selects subtype 24)
                        zone.items.push_back(item);
                    }
                }
            }
        }
}

// alpha-light-propagation-lifecycle.md: reset stored clear cells, run sixteen
// synchronous rounds, then retain the shaded results. A new zone is isolated
// from its neighbors until its initial lighting is complete.
void Terrain::blockLight(Zone &zone) const {
    const int32_t x0 = zone.originX(), y0 = zone.originY();
    std::unordered_map<uint64_t, uint8_t> working;
    std::vector<uint64_t> cells;
    std::vector<int32_t> skyline(Zone::SIZE * Zone::SIZE);
    for (int32_t i = 0; i < Zone::SIZE; i++)
        for (int32_t j = 0; j < Zone::SIZE; j++) {
            const int32_t x = x0 + i, y = y0 + j;
            const Column &c = zone.column(x, y);
            int32_t sky = c.bottom; // the first z that is sky-lit
            for (int32_t z = c.top() - 1; z >= c.bottom; z--)
                if (!clear(c.blocks[(size_t)(z - c.bottom)].type())) {
                    sky = z + 1;
                    break;
                }
            skyline[i + Zone::SIZE * j] = sky;
            for (int32_t z = c.bottom; z < sky; z++)
                if (clear(c.blocks[(size_t)(z - c.bottom)].type())) {
                    working[lightKey(x, y, z)] = 0;
                    cells.push_back(lightKey(x, y, z));
                }
        }
    if (cells.empty())
        return;
    auto valueAt = [&](int32_t x, int32_t y, int32_t z) -> int {
        if (!zone.contains(x, y))
            return 0; // the generation pass cannot sample an adjacent zone
        const Column &column = zone.column(x, y);
        if (z < column.bottom)
            return 0;
        if (z >= column.top())
            return 255;
        const Block b = column.blocks[(size_t)(z - column.bottom)];
        // Only neighbor reads apply Alpha's full-bright implicit water rule.
        // The initialization and target passes still use the stored air cell.
        if (b.type() == AIR && z <= 0 && !(b.typeByte & DRY_AIR))
            return 255;
        if (b.type() == GLOWING)
            return 255;
        if (!clear(b.type()))
            return 0;
        if (z >= skyline[(x - x0) + Zone::SIZE * (y - y0)])
            return 255;
        const auto it = working.find(lightKey(x, y, z));
        return std::max(it == working.end() ? 255 : (int)it->second, 5);
    };
    std::vector<uint8_t> next(cells.size());
    for (int sweep = 0; sweep < 16; sweep++) {
        for (size_t n = 0; n < cells.size(); n++) {
            const uint64_t key = cells[n];
            const int32_t x = (int32_t)(key >> 40), y = (int32_t)((key >> 16) & 0xffffff), z = (int16_t)(key & 0xffff);
            int best = 0;
            const int32_t steps[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
            for (const auto &s : steps) {
                best = std::max(best, valueAt(x + s[0], y + s[1], z + s[2]));
                if (best == 255)
                    break;
            }
            next[n] = (uint8_t)(best * 85 / 100);
        }
        for (size_t n = 0; n < cells.size(); n++)
            working[cells[n]] = next[n];
    }
    zone.shadedLight.reserve(cells.size());
    for (uint64_t key : cells)
        zone.shadedLight.emplace_back(key, working[key]);
    std::sort(zone.shadedLight.begin(), zone.shadedLight.end());
}

} // namespace worldgen
