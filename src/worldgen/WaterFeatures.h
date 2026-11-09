#pragma once
// User-requested water extensions. The native generator remains independently
// testable; runtime profiles opt into real liquid at shallow wet-ground faces.
#include "worldgen/WorldMap.h"

namespace worldgen {
constexpr int SWIMMING_DEPTH = 3;
void makeWaterSwimmable(Zone &zone);
std::vector<Plant> bankReeds(const WorldMap &map, const Zone &zone, uint32_t seed);
}
