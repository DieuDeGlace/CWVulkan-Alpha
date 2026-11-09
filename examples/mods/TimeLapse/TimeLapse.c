/* TimeLapse: an example AlphaVulkan code mod. Each real second it moves the time of day on by
 * one game hour, so a whole day passes in 24 seconds. Built by the engine's CMake into
 * build/Mods/TimeLapse; enable it with AlphaVulkan.ini [Mods] Load=TimeLapse. */
#include "alphavulkan_mod.h"
#include <stdio.h>

#define HOUR_MS 3600000.0
#define DAY_MS 86400000.0

static const AvHost *host;
static int zones;

static void onFrame(void *user, double seconds) {
    (void)user;
    double t = host->getTimeOfDay(host->context) + seconds * HOUR_MS;
    while (t >= DAY_MS)
        t -= DAY_MS;
    host->setTimeOfDay(host->context, t);
}

static void onShutdown(void *user) {
    (void)user;
    char line[64];
    snprintf(line, sizeof line, "goodbye (%d zones seen)", zones);
    host->log(host->context, line);
}

/* SDK version 2: hear of each zone, and look at the block at its corner's ground. */
static void onZoneReady(void *user, int32_t zx, int32_t zy) {
    (void)user;
    zones++;
    if (host->size >= sizeof(AvHost) && zones == 1) {
        uint8_t r, g, b, type;
        if (host->getBlock(host->context, zx * 256, zy * 256, 0, &r, &g, &b, &type)) {
            char line[96];
            snprintf(line, sizeof line, "first zone %d,%d: block at sea level has type %u", zx, zy, (unsigned)type);
            host->log(host->context, line);
        }
    }
}

AV_EXPORT int avModLoad(const AvHost *engine, AvMod *mod) {
    if (engine->apiVersion < 1)
        return 0;
    host = engine;
    mod->apiVersion = AV_API_VERSION;
    mod->onFrame = onFrame;
    mod->onShutdown = onShutdown;
    if (engine->apiVersion >= 2)
        mod->onZoneReady = onZoneReady;

    /* Show that content can be read: the size of the English dictionary. */
    uint32_t size = 0;
    char line[96];
    if (host->readContent(host->context, "dict_en.xml", NULL, 0, &size))
        snprintf(line, sizeof line, "hello: one game hour per second (dict_en.xml is %u bytes)", (unsigned)size);
    else
        snprintf(line, sizeof line, "hello: one game hour per second");
    host->log(host->context, line);
    return 1;
}
