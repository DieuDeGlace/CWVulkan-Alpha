#pragma once
#include "alphavulkan_engine_module.h"

namespace av::modules {
// Engine pipeline entry; modules use registration, never call this recursively.
void applyTerrainPasses(worldgen::Zone &zone, const worldgen::Terrain &terrain);
}
