#include "BuiltinModules.h"
#include "DataStore.h"
#include "EngineModules.h"
#include "Mods.h"
#include "worldgen/Terrain.h"
#include <algorithm>
#include <cstdio>
#include <memory>

namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}
}

int main() {
    using namespace av::modules;
    worldgen::World world(41);
    worldgen::Terrain terrain(world);
    auto zone = std::make_unique<worldgen::Zone>();
    zone->column(0, 0).blocks.push_back({100, 100, 100, worldgen::ROCK});
    zone->column(1, 0).blocks.push_back({100, 100, 100, worldgen::ROCK | worldgen::KEEP});
    Content content;
    std::vector<std::string> messages;
    ModHost host;
    host.log = [&](const std::string &, const std::string &message) { messages.push_back(message); };
    const bool hasExample = std::any_of(builtinModules().begin(), builtinModules().end(), [](const BuiltinModule &module) {
        return module.id == "StoneTint";
    });
    if (hasExample) {
        ModLoader loader(host);
        loader.load("nonexistent-mod-folder", {"StoneTint", "StoneTint"}, content);
        check(loader.loaded().size() == 1 && loader.loaded().front().builtin && loader.loaded().front().version == "1",
              "source module loads once without a DLL or mod folder");
        applyTerrainPasses(*zone, terrain);
        check(zone->column(0, 0).blocks.back().r == 75 && zone->column(0, 0).blocks.back().b == 140,
              "source module modifies engine-owned terrain");
        check(zone->column(1, 0).blocks.back().r == 100, "example preserves protected material");
        loader.shutdown();
        check(loader.loaded().empty(), "shutdown clears active module list");
        applyTerrainPasses(*zone, terrain);
        check(zone->column(0, 0).blocks.back().r == 75, "shutdown unregisters terrain pass");
        loader.load("nonexistent-mod-folder", {"StoneTint"}, content);
        check(loader.loaded().size() == 1, "source module reloads after shutdown");
        loader.shutdown();
    }

    std::vector<int> order;
    const auto first = registerTerrainPass("test.first", [&](worldgen::Zone &, const worldgen::Terrain &) { order.push_back(1); });
    const auto second = registerTerrainPass("test.second", [&](worldgen::Zone &, const worldgen::Terrain &) { order.push_back(2); });
    check(first && second && !registerTerrainPass("test.first", [](auto &, const auto &) {}), "duplicate pass names rejected");
    // This calls the actual terrain pipeline, not only the registry facade.
    const auto generated = terrain.generate(32802, 32803);
    check(generated && order == std::vector<int>{1, 2}, "generation invokes source passes once in registration order");
    check(unregisterTerrainPass(first) && unregisterTerrainPass(second) && !unregisterTerrainPass(first),
          "registrations can be removed exactly once");
    check(!registerTerrainPass("", [](auto &, const auto &) {}) && !registerTerrainPass("empty", {}), "invalid passes rejected");
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
