#pragma once
// The prefab placer (docs/specs/alpha-worldgen-structures.md §6.2): stamps a voxel model into
// a zone, block by block, and turns the model's marker colours into doors, windows, beds,
// signs, NPCs and the like.
#include "worldgen/Random.h"
#include "worldgen/Zone.h"
#include <functional>
#include <memory>
#include <string>

namespace worldgen {

// A voxel model as the placer reads it: x varies fastest, then y, then z; (0, 0, 0) is empty.
struct Prefab {
    int32_t sx = 0, sy = 0, sz = 0;
    std::vector<uint8_t> rgb; // 3 bytes per voxel

    // The colour of voxel (i, j, k) packed as 0xRRGGBB; 0 (empty) outside the model.
    uint32_t colour(int32_t i, int32_t j, int32_t k) const;
};

// Looks a model up by its key in the game's data (for example "castle-arc.cub"); null when the
// install does not have it.
using PrefabSource = std::function<std::shared_ptr<const Prefab>(const std::string &key)>;

// A block the placer wrote (the output list of §6.2).
struct PlacedBlock {
    int32_t x, y, z;
    Block block;
};

struct Placement {
    int32_t x = 0, y = 0, z = 0; // origin, blocks
    int32_t rotation = 0;        // quarter turns, 0..3
    uint8_t typeByte = 6;        // T: the type byte of plain model blocks (bit 0x40 is added)
    int32_t mode = 0;            // M: which markers count and what they make
    bool fillDown = false;       // F: extend the bottom layer down to the ground
    int32_t crop[4] = {0, 0, 0, 0}; // model columns cut off at -x, -y, +x, +y (world directions)
    std::vector<PlacedBlock> *written = nullptr;
};

// What the placer needs from the world around the zone.
struct PlacerWorld {
    // The warmth blend of a column outside the zone (regions spec §4).
    std::function<float(int32_t x, int32_t y)> warmth;
    // The level of a creature placed at (x, y) (0x59a9b0; its rule is not specified yet).
    std::function<int32_t(int32_t x, int32_t y)> level;
    // The rock-coloured block that fills a gap under a column (terrain §1.5).
    std::function<Block(int32_t x, int32_t y, int32_t z)> gapFill;
};

// §6.2. Places `model` into `zone`, drawing from `random` only at marker voxels.
void placePrefab(Zone &zone, Random &random, const PlacerWorld &world, const Prefab &model, const Placement &at);

} // namespace worldgen
