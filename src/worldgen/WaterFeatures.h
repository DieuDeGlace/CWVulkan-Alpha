#pragma once
// User-requested water extensions. The native generator remains independently
// testable; runtime profiles opt into real liquid at shallow wet-ground faces.
#include "worldgen/WorldMap.h"
#include <functional>

namespace worldgen {
constexpr int SWIMMING_DEPTH = 3;
// The block stored under water carved below a column's lowest stored cell (the
// terrain's gap rock); empty: the implicit solid block.
using BedFill = std::function<Block(int32_t x, int32_t y, int32_t z)>;
void makeWaterSwimmable(Zone &zone, const BedFill &bed = {});
std::vector<Plant> bankReeds(const WorldMap &map, const Zone &zone, uint32_t seed);
}
