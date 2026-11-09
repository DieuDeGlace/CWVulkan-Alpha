#include "EngineModules.h"
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace av::modules {
namespace {
struct Pass {
    Registration registration;
    std::string name;
    TerrainPass run;
};
std::shared_mutex passMutex;
std::vector<Pass> passes;
Registration nextRegistration = 1;
}

Registration registerTerrainPass(std::string name, TerrainPass pass) {
    if (name.empty() || !pass)
        return 0;
    std::unique_lock lock(passMutex);
    if (std::any_of(passes.begin(), passes.end(), [&](const Pass &entry) { return entry.name == name; }))
        return 0;
    const Registration result = nextRegistration++;
    passes.push_back({result, std::move(name), std::move(pass)});
    return result;
}

bool unregisterTerrainPass(Registration registration) {
    std::unique_lock lock(passMutex);
    const auto entry = std::find_if(passes.begin(), passes.end(), [&](const Pass &pass) {
        return pass.registration == registration;
    });
    if (entry == passes.end())
        return false;
    passes.erase(entry);
    return true;
}

void applyTerrainPasses(worldgen::Zone &zone, const worldgen::Terrain &terrain) {
    // Reader lock permits parallel generation and makes unregister wait for all
    // in-flight callbacks, including their captured state, to finish.
    std::shared_lock lock(passMutex);
    for (const auto &pass : passes)
        pass.run(zone, terrain);
}
}
