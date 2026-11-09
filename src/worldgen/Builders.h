#pragma once
// Block writing helpers shared by the structure builders (docs/specs/alpha-worldgen-structures.md
// §1.10.2): reads and writes inside the zone being generated, the box filler with its moss and
// glow, carving and plain fills.
#include "worldgen/Terrain.h"
#include <functional>

namespace worldgen {

inline float f(double v) { return (float)v; }
// A float block coordinate in 1/65536 blocks (0x42c580).
inline int64_t fx(float v) { return (int64_t)f(v * 65536.f); }
// A fixed-point coordinate (1/65536 block) as a block, as the game reads one (0x42f860): below zero
// it takes one block off and then truncates, so a whole negative height reads one lower
// (open points 10 §11).
inline int32_t blockOfFixed(int64_t v) { return (int32_t)((v < 0 ? v - 65536 : v) / 65536); }

// A box colour; channels may run past 255 before the box filler clamps them.
struct Colour {
    int32_t r = 0, g = 0, b = 0;
};

struct StructureWriter {
    const Terrain &terrain;
    Zone &zone;
    Random &random;
    const std::function<Block(int32_t, int32_t, int32_t)> &gapFill; // rock under a column (terrain §1.5)

    // A block with the implicit rules; columns outside the zone read solid.
    Block read(int32_t x, int32_t y, int32_t z) const;
    // A block that is neither air nor water (0x43b480).
    bool blockSolid(int32_t x, int32_t y, int32_t z) const;
    // The block writer; columns outside the zone ignore it.
    void write(int32_t x, int32_t y, int32_t z, Block b);
    // §1.10.2 item 1: how mossy a box block is, 0..1.
    float moss(int32_t X, int32_t Y, int32_t Z) const;
    // §1.10.2 item 1: the box filler, 3 + sx sy sz draws (3 when a size is not positive).
    void box(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int32_t sz, Colour C, float rate, uint8_t flags);
    // §1.10.2 item 2: dry air through a box, frayed at its sides when rough.
    void carve(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int32_t sz, bool rough);
    // §1.10.2 item 3: one colour through a box, no draws.
    void fill(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int32_t sz, Colour C);
};

// §2.3: drops a static object onto the ground under its footprint; false when it cannot stand
// there.
bool settle(StaticObject &o, const Zone &zone);

} // namespace worldgen
