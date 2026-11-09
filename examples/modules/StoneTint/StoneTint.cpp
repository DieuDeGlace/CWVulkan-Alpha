// An engine-integrated source module. No DLL, detours or original assets.
#include "alphavulkan_engine_module.h"
#include "alphavulkan_mod.h"
#include "worldgen/Terrain.h"
#include "worldgen/Zone.h"
#include <algorithm>

namespace {
av::modules::Registration registration = 0;

void tint(worldgen::Zone &zone, const worldgen::Terrain &) {
    for (auto &column : zone.columns) {
        // Work on exposed natural rock only, keeping material and flags intact.
        for (auto at = column.blocks.rbegin(); at != column.blocks.rend(); ++at) {
            if (worldgen::clear(at->type()))
                continue;
            if (at->type() == worldgen::ROCK && !(at->typeByte & worldgen::KEEP)) {
                at->r = (uint8_t)(at->r * 3 / 4);
                at->b = (uint8_t)std::min(255, at->b + 40);
            }
            break;
        }
    }
}

void shutdown(void *) {
    av::modules::unregisterTerrainPass(registration);
    registration = 0;
}
}

extern "C" int AV_MODULE_ENTRY(const AvHost *host, AvMod *mod) {
    if (!host || !mod || host->apiVersion < AV_API_VERSION || mod->size < sizeof(AvMod))
        return 0;
    const auto added = av::modules::registerTerrainPass("StoneTint.terrain", tint);
    if (!added)
        return 0;
    registration = added;
    mod->apiVersion = AV_API_VERSION;
    mod->onShutdown = shutdown;
    if (host->log)
        host->log(host->context, "registered engine terrain pass");
    return 1;
}
