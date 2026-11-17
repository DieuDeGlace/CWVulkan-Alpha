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

// §1.6 with alpha-rulings-corridor-third-line.md: the bounds of the third (m + 1) corridor line
// for rooms A = (PA, SA) and B = (PB, SB), m on axis `u` (0: the corridor runs along y; 1: along
// x). Along y both bounds use x extents; along x, B's x extent always and A's y extent only when
// B is lower (PB.x < PA.x). The odd-draw gate is separate and is drawn first in every case.
bool corridorThirdLine(int32_t u, int32_t m, glm::ivec3 PA, glm::ivec3 SA, glm::ivec3 PB, glm::ivec3 SB);

// §5.5.9 Ruins: a hostile creature of location L at (x, y, z) in 1/65536 blocks (controller 1,
// flag 0x1000, the location's level and sub-level), with a facing draw, then a kind draw from
// `kinds`.
Spawn ruinCreature(int64_t x, int64_t y, int64_t z, const std::vector<int32_t> &kinds, const Location &L,
                   const std::function<int32_t()> &draw);
// §5.5.9 Ruins step 3, second bullet, with alpha-rulings-spawn-rounds-ruin-routes.md §2: one
// group iteration of a ruin house. `P` holds the house's remaining spawn points (not empty; one
// is drawn and removed), `groupKinds` the lists a of the location's groups and `houseSpawns`
// the original spawn lists of the zone's houses in list order. The creature stands one block
// above its point and runs Combat 20, WalkPath 2 over [its own position] or [its own position,
// a spawn point of a drawn house (whole blocks, not raised)]. Draws: point, group (an empty list
// a ends the iteration with no creature), facing, kind, house, a point of that house (only if
// it has any), the item count e = d & 1.
std::optional<Spawn> ruinGroupCreature(std::vector<glm::ivec3> &P, const std::vector<std::vector<int32_t>> &groupKinds,
                                       const std::vector<const std::vector<glm::ivec3> *> &houseSpawns,
                                       const Location &L, const std::function<int32_t()> &draw);

} // namespace worldgen
