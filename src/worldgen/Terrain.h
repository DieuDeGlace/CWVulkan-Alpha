#pragma once
// Alpha's terrain (docs/specs/alpha-worldgen-terrain.md): the shaping fields, the base height,
// the colour rules, and the zone generator that turns them into voxel columns, followed by the
// plant pass (alpha-worldgen-vegetation.md) and the block light (alpha-rendering.md §9.2).
// Section numbers point at the terrain spec unless they name another.
//
// The passes between the surface and the trees are in PostPasses.cpp (rock formations, roads,
// terraces, boulders; alpha-worldgen-caves.md) and Structures.cpp (camps, the third boulder
// pass, stone circles; alpha-worldgen-structures.md). The dungeon builder is in Dungeon.cpp.
// Not in yet: the town builder's building pass. Every zone draws from its own random stream,
// so town zones line up 1:1 only once it exists.
#include "worldgen/Prefab.h"
#include "worldgen/Regions.h"
#include "worldgen/Zone.h"
#include <glm/glm.hpp>
#include <memory>

namespace worldgen {

// A column's climate, read from the zone being generated when it holds the column (the stored
// values equal the computed ones, §0).
struct Climate {
    float moisture, warmth, fire;
};

// Caves spec §6: where a boulder is built.
struct BoulderSeed {
    int32_t x, y, z;
};

// Caves spec §1, §2: which inside test of a formation.
enum class FormationTest { Mound, MesaGuard, MesaFill };
int32_t formationTop(int32_t X, int32_t Y, int32_t z, int32_t rz, bool mesa);
glm::vec2 formationUV(FormationTest test, int32_t X, int32_t Y, int32_t Z, int32_t x, int32_t y, int32_t rx, int32_t ry);
// Caves spec §5.1: the road shade, before the town mix.
float roadShade(int32_t x, int32_t y);
// Caves spec §5.2: a column's terrace from its height.
struct Terrace {
    int32_t zb = 0;
    float t = 0, a = 0;
    int32_t zlow = 0, zw = 0;
};
Terrace terraceAt(float h);
// Structures spec §1.1: the dungeon builder's arguments.
struct DungeonCall {
    int32_t x0 = 0, y0 = 0; // the grid's corner, blocks (256 zx + 32, 256 zy + 32)
    int32_t variant = 0;    // style 0..5 (E.variant)
    int32_t flag = 0;       // 1 for the kind-5 call, which fresh worlds never make
    int32_t level = 1, subLevel = 0;
};

// Structures spec §4: the offset of pillar k of n from a stone circle's centre.
glm::ivec2 pillarOffset(int32_t k, int32_t n);

// Wildlife spec §2.2: the creature types that may live where the climate and height are these
// (mode 1: the people list), in the original's order.
std::vector<int32_t> wildlifeList(float warmth, float moisture, float fire, float mountain, int32_t h, int32_t mode);

class Terrain {
  public:
    explicit Terrain(const World &world) : w(world) {}

    // §3: the shaping fields of a block column.
    float townness(int32_t x, int32_t y) const;  // q
    float flatness(int32_t x, int32_t y) const;  // W
    float roughness(int32_t x, int32_t y) const; // R
    float mountains(int32_t x, int32_t y) const; // M
    // §4: the base height H of a column, in blocks.
    float height(int32_t x, int32_t y) const;

    // §6: colours on a 0..255 scale.
    glm::vec3 grass(int32_t x, int32_t y) const;
    glm::vec3 sand(int32_t x, int32_t y) const;
    glm::vec3 beach(int32_t x, int32_t y) const;
    glm::vec3 snow(int32_t x, int32_t y) const;
    glm::vec3 rock(int32_t x, int32_t y, int32_t z, const Zone *zone = nullptr) const;
    glm::vec3 cliff(int32_t x, int32_t y, int32_t z, const Zone *zone = nullptr) const;
    // §6.1: the surface colour and its type byte.
    Block surface(int32_t x, int32_t y, int32_t z, const Zone *zone = nullptr) const;
    // The same with a given climate (structures spec §5.5.8 colours a plaza this way).
    Block surface(int32_t x, int32_t y, int32_t z, const Climate &climate, const Zone *zone) const;
    // Structures spec §5.5.8: St, the paving stone colour (0x5fa170).
    glm::vec3 stone(int32_t x, int32_t y, int32_t z) const;

    // §5: generates zone (zx, zy), first preparing the 3 x 3 regions round its own (safe from
    // several threads at once).
    std::unique_ptr<Zone> generate(int32_t zx, int32_t zy) const;
    // Locations spec: prepares region (rx, ry) with this terrain's heights.
    void prepareRegion(int32_t rx, int32_t ry) const;

    Climate climate(int32_t x, int32_t y, const Zone *zone) const;
    // Caves spec §1 (mound) and §2 (mesa): a rock formation on top of the ground.
    void formation(Zone &zone, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz, bool mesa) const;
    // Structures spec §1: builds a dungeon into the zone.
    void dungeon(Zone &zone, Random &random, const DungeonCall &call) const;
    // Structures spec §5: builds the part of a town or ruin (L of type 1 or 5) in this zone;
    // `quarter` is the zone entry's variant (1..4 in a town's quarter zones).
    void town(Zone &zone, Random &random, const Location &L, int32_t quarter) const;
    const World &world() const { return w; }
    // Wildlife spec §2: the species of creature that lives at column (x, y) at height h, by
    // climate (mode 0) or the people list (mode 1); one draw.
    int32_t wildlifeSpecies(int32_t x, int32_t y, int32_t h, int32_t mode, Random &random) const;
    // Block edits §4: the rock that fills a gap under a column, and the light flood run again
    // over a zone whose blocks changed.
    Block gapBlock(int32_t x, int32_t y, int32_t z, const Zone &zone) const { return rockBlock(x, y, z, zone); }
    void relight(Zone &zone) const {
        zone.shadedLight.clear();
        blockLight(zone);
    }

    // The models the structure builders place (dungeon arches); without it they are left out,
    // which changes no draws.
    PrefabSource prefabs;
    // Explicit runtime extension; oracle tools use the native generator by default.
    bool swimmableWater = false;
    // Spawn objects spec §3.2: the finishing pass's work on one spawn (appearance, equipment,
    // boss spirit, rarities, levels), which needs the creature rules; from the zone's own
    // stream. Without it the spawns are left unfinished and only bosses get their spirit.
    // `whole` false: only the appearance and equipment (a location's ring creatures, made up
    // as they are placed and again in the finishing pass).
    std::function<void(Spawn &, Random &, bool whole)> finishSpawn;

  private:
    void surfacePass(Zone &zone, const std::vector<float> &heights, Random &random,
                     std::vector<BoulderSeed> &ravineBoulders) const;
    void postPasses(Zone &zone, const std::vector<float> &heights, Random &random,
                    const std::vector<BoulderSeed> &ravineBoulders) const;
    void boulder(Zone &zone, const BoulderSeed &seed, int32_t ax, int32_t ay, int32_t az, bool fromRavine) const;
    // Structures.cpp
    void structurePasses(Zone &zone, Random &random, const ZoneEntry &E, const Location *Lz, bool climateBonus) const;
    bool camp(Zone &zone, Random &random, int64_t px, int64_t py, int64_t pz) const;
    void treeGrid(Zone &zone, Random &random) const;
    void plantPass(Zone &zone, Random &random) const;
    // Wildlife.cpp: the animals and wild plants of a zone (wildlife spec §3).
    // Spawn objects spec §6.1, §6.2: the statue of even zones and the location candidate grid,
    // between the tree grid and the wildlife pass.
    void statuePass(Zone &zone, Random &random) const;
    void locationGrid(Zone &zone, Random &random) const;
    // Location creatures spec (0x5dc6e0): camps, rings and groups at the grid's candidates
    // (fixed-point ground points).
    void locationCreatures(Zone &zone, Random &random, const Location &L,
                           const std::vector<std::array<int64_t, 3>> &candidates) const;
    void wildlifePass(Zone &zone, Random &random) const;
    void blockLight(Zone &zone) const;
    Block rockBlock(int32_t x, int32_t y, int32_t z, const Zone &zone) const;
    float noiseAt(uint32_t ox, uint32_t oy, double k, int32_t x, int32_t y) const;

    const World &w;
};

} // namespace worldgen
