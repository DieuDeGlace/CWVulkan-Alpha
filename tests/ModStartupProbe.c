/* Synthetic SDK fixture: startup can read its own override before deciding to
 * decline, and shutdown must run while the successful layer is still present. */
#include "alphavulkan_mod.h"

static const AvHost *host;

static void shutdownProbe(void *user) {
    uint32_t size = 0;
    (void)user;
    host->log(host->context, host->readContent(host->context, "startup.txt", 0, 0, &size)
        ? "shutdown content present" : "shutdown content absent");
}

AV_EXPORT int avModLoad(const AvHost *engine, AvMod *mod) {
    char decision = 0;
    uint32_t size = 0;
    host = engine;
    if (!host->readContent(host->context, "startup.txt", &decision, 1, &size))
        return 0;
    host->log(host->context, "startup content present");
    if (decision == 'N')
        return 0;
    mod->apiVersion = AV_API_VERSION;
    mod->onShutdown = shutdownProbe;
    return 1;
}
