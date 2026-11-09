// Block writing helpers shared by the structure builders (docs/specs/alpha-worldgen-structures.md
// §1.10.2): the box filler, carving and plain fills, and the settle test of §2.3.
#include "worldgen/Builders.h"
#include <algorithm>

namespace worldgen {

Block StructureWriter::read(int32_t x, int32_t y, int32_t z) const {
    return zone.contains(x, y) ? zone.read(x, y, z) : IMPLICIT_SOLID;
}
bool StructureWriter::blockSolid(int32_t x, int32_t y, int32_t z) const {
    const uint8_t t = read(x, y, z).type();
    return t != AIR && t != WATER;
}
void StructureWriter::write(int32_t x, int32_t y, int32_t z, Block b) {
    if (zone.contains(x, y))
        zone.place(x, y, z, b, [&](int32_t gz) { return gapFill(x, y, gz); });
}

// Structures spec §1.10.2 item 1: moss on a block of a box.
float StructureWriter::moss(int32_t X, int32_t Y, int32_t Z) const {
    const float mo = zone.contains(X, Y) ? zone.column(X, Y).moisture : terrain.world().moisture(X, Y);
    const float n1 = noise(f(f((float)Y * 0.2f) + 534.f), f(f((float)Z * 0.2f) + 13.f));
    const float n2 = noise(0.05 * X + 4343, 0.1 * Z + 84734);
    const float inner = f(f(f(f(n1 * 0.1f) + n2) * 0.7f) + 0.2f);
    const float m = f(mo * inner);
    return m < 0 ? 0.f : (m > 1 ? 1.f : m);
}

// Structures spec §1.10.2 item 1: the box filler, 3 + sx sy sz draws.
void StructureWriter::box(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int32_t sz, Colour C, float rate, uint8_t flags) {
    int32_t t[23] = {};
    t[0] = random.draw() % 20;
    t[8] = random.draw() % 20;
    t[16] = random.draw() % 20;
    if (sx <= 0 || sy <= 0 || sz <= 0)
        return;
    auto channel = [](float c) { return (uint8_t)(int32_t)std::clamp(c, 0.f, 255.f); };
    for (int32_t i = 0; i < sx; i++)
        for (int32_t j = 0; j < sy; j++)
            for (int32_t k = sz - 1; k >= 0; k--) {
                const int32_t d = random.draw();
                const int32_t X = x + i, Y = y + j, Z = z + k;
                if (!zone.contains(X, Y))
                    continue;
                const int32_t s = t[((7 * k + i) / 2 + 7 * ((j + 7 * k) / 2)) % 23];
                const float m = moss(X, Y, Z);
                const float G = (float)(C.g + s);
                Block b;
                b.r = channel((float)(C.r + s));
                b.g = channel(f(m * (120.f - G)) + G);
                b.b = channel((float)(C.b + s));
                b.typeByte = (float)d / 32767.f > rate ? (uint8_t)(flags | DRY_AIR | ROCK) : (uint8_t)(flags | DRY_AIR | GLOWING);
                write(X, Y, Z, b);
            }
}

// Structures spec §1.10.2 item 2.
void StructureWriter::carve(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int32_t sz, bool rough) {
    const Block hole{0, 0, 0, 0xC0};
    for (int32_t i = 0; i < sx; i++)
        for (int32_t j = 0; j < sy; j++)
            for (int32_t k = sz - 1; k >= 0; k--) {
                const int32_t X = x + i, Y = y + j, Z = z + k;
                write(X, Y, Z, hole);
                if (!rough)
                    continue;
                // (Open points 10 §1: along the wall 0.1, across it 13; the y faces exchange x and y.)
                const auto frays = [](int32_t along, int32_t across, int32_t height) {
                    return noise(0.1 * along + (double)(13 * across), 0.2 * height) > 0.5f;
                };
                if (i == 0 && frays(Y, X, Z))
                    write(X - 1, Y, Z, hole);
                if (j == 0 && frays(X, Y, Z))
                    write(X, Y - 1, Z, hole);
                if (i == sx - 1 && frays(Y, X, Z))
                    write(X + 1, Y, Z, hole);
                if (j == sy - 1 && frays(X, Y, Z))
                    write(X, Y + 1, Z, hole);
            }
}

// Structures spec §1.10.2 item 3.
void StructureWriter::fill(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int32_t sz, Colour C) {
    const Block b{(uint8_t)C.r, (uint8_t)C.g, (uint8_t)C.b, DRY_AIR | ROCK};
    for (int32_t i = 0; i < sx; i++)
        for (int32_t j = 0; j < sy; j++)
            for (int32_t k = sz - 1; k >= 0; k--)
                write(x + i, y + j, z + k, b);
}

} // namespace worldgen
