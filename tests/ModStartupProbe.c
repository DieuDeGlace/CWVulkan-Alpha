/* Synthetic SDK fixture: startup can read its own override before deciding to
 * decline, and shutdown must run while the successful layer is still present.
 * Version 10: player join/leave and taming reach it, and its calls to tell and
 * getPlayerName reach the host. */
#include "alphavulkan_mod.h"
#include <stdio.h>

static const AvHost *host;

static void shutdownProbe(void *user) {
    uint32_t size = 0;
    (void)user;
    host->log(host->context, host->readContent(host->context, "startup.txt", 0, 0, &size)
        ? "shutdown content present" : "shutdown content absent");
}

static void joinProbe(void *user, uint64_t id) {
    char name[64], small[3], line[128];
    int32_t length, cut;
    (void)user;
    length = host->getPlayerName(host->context, id, name, sizeof name);
    cut = host->getPlayerName(host->context, id, small, sizeof small);
    if (length < 0) {
        host->log(host->context, "join of an unknown player");
        return;
    }
    snprintf(line, sizeof line, "welcome %s", name);
    host->tell(host->context, id, line);
    snprintf(line, sizeof line, "name %d cut %d '%s' unknown %d", (int)length, (int)cut, small,
             (int)host->getPlayerName(host->context, id + 1000, 0, 0));
    host->log(host->context, line);
}

static void leaveProbe(void *user, uint64_t id) {
    char line[64];
    (void)user;
    snprintf(line, sizeof line, "left %llu", (unsigned long long)id);
    host->log(host->context, line);
}

static void tamedProbe(void *user, uint64_t id, int32_t type, uint64_t owner) {
    char line[96];
    (void)user;
    snprintf(line, sizeof line, "tamed %llu type %d by %llu", (unsigned long long)id, (int)type,
             (unsigned long long)owner);
    host->log(host->context, line);
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
    if (host->apiVersion >= 10 && mod->size >= sizeof(AvMod)) {
        mod->onPlayerJoin = joinProbe;
        mod->onPlayerLeave = leaveProbe;
        mod->onCreatureTamed = tamedProbe;
    }
    return 1;
}
