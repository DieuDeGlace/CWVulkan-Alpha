#pragma once
// Source-module SDK: C++20, rebuilt with this engine and its headers/toolchain.
// Unlike the DLL SDK, engine-owned Zone/Terrain types are available directly.
#include <cstdint>
#include <functional>
#include <string>

namespace worldgen {
struct Zone;
class Terrain;
}

namespace av::modules {
inline constexpr uint32_t SOURCE_API_VERSION = 1;
using TerrainPass = std::function<void(worldgen::Zone &, const worldgen::Terrain &)>;
using Registration = uint64_t;

// Call at module load/shutdown, before/after world workers. Names are unique and
// passes run in registration order after native generation, before water/light.
// A pass may run concurrently for different zones. It owns only its supplied
// zone, must be deterministic, and must not register/unregister from a pass.
// Returns zero for an empty callback/name or a duplicate name.
Registration registerTerrainPass(std::string name, TerrainPass pass);
// Waits for active passes before releasing their state. Zero/unknown is a no-op.
bool unregisterTerrainPass(Registration registration);
}
