#include "Mods.h"
#include "BuiltinModules.h"
#include "Config.h"
#include "DataStore.h"
#include "Install.h"
#include "ModFingerprint.h"
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <system_error>

namespace {

// What a plugin's AvHost::context points at: which mod is calling, and the engine's hooks.
struct CallerContext {
    std::string mod;
    const ModHost *host = nullptr;
};

CallerContext &caller(void *context) { return *static_cast<CallerContext *>(context); }

// The C functions of the AvHost table.
void hostLog(void *context, const char *message) {
    const CallerContext &c = caller(context);
    if (c.host->log)
        c.host->log(c.mod, message ? message : "");
}

int hostReadContent(void *context, const char *key, void *buffer, uint32_t capacity, uint32_t *size) {
    const CallerContext &c = caller(context);
    if (!key || !c.host->content)
        return 0;
    const auto bytes = c.host->content->read(key);
    if (!bytes)
        return 0;
    if (size)
        *size = (uint32_t)bytes->size();
    if (buffer)
        std::memcpy(buffer, bytes->data(), std::min<size_t>(capacity, bytes->size()));
    return 1;
}

int hostPlaySound(void *context, const char *key, float volume) {
    const CallerContext &c = caller(context);
    return key && c.host->playSound && c.host->playSound(key, volume) ? 1 : 0;
}

double hostGetTimeOfDay(void *context) {
    const CallerContext &c = caller(context);
    return c.host->getTimeOfDay ? c.host->getTimeOfDay() : 0;
}

void hostSetTimeOfDay(void *context, double milliseconds) {
    const CallerContext &c = caller(context);
    if (c.host->setTimeOfDay)
        c.host->setTimeOfDay(milliseconds);
}

int hostGetBlock(void *context, int32_t x, int32_t y, int32_t z, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *type) {
    const CallerContext &c = caller(context);
    uint8_t block[4];
    if (!c.host->getBlock || !c.host->getBlock(x, y, z, block))
        return 0;
    if (r)
        *r = block[0];
    if (g)
        *g = block[1];
    if (b)
        *b = block[2];
    if (type)
        *type = block[3];
    return 1;
}

int hostGetPlayer(void *context, AvPlayer *out) {
    const CallerContext &c = caller(context);
    AvPlayer player{};
    if (!c.host->getPlayer || !c.host->getPlayer(player))
        return 0;
    if (out)
        *out = player;
    return 1;
}

int hostShowText(void *context, double x, double y, double z, const char *text, float r, float g, float b, float size) {
    const CallerContext &c = caller(context);
    return text && c.host->showText && c.host->showText(x, y, z, text, r, g, b, size) ? 1 : 0;
}

int hostShowBanner(void *context, const char *name, const char *detail) {
    const CallerContext &c = caller(context);
    return c.host->showBanner && c.host->showBanner(name ? name : "", detail ? detail : "") ? 1 : 0;
}

uint32_t hostGetStaticCount(void *context, int32_t zx, int32_t zy) {
    const CallerContext &c = caller(context);
    const std::vector<AvStatic> *list = c.host->getStatics ? c.host->getStatics(zx, zy) : nullptr;
    return list ? (uint32_t)list->size() : 0;
}

int hostGetStatic(void *context, int32_t zx, int32_t zy, uint32_t index, AvStatic *out) {
    const CallerContext &c = caller(context);
    const std::vector<AvStatic> *list = c.host->getStatics ? c.host->getStatics(zx, zy) : nullptr;
    if (!list || index >= list->size())
        return 0;
    if (out)
        *out = (*list)[index];
    return 1;
}

int hostGetMission(void *context, int32_t cx, int32_t cy, AvMission *out) {
    const CallerContext &c = caller(context);
    AvMission mission{};
    if (!c.host->getMission || !c.host->getMission(cx, cy, mission))
        return 0;
    if (out)
        *out = mission;
    return 1;
}

int hostSetBlock(void *context, int32_t x, int32_t y, int32_t z, uint8_t r, uint8_t g, uint8_t b, uint8_t type) {
    const CallerContext &c = caller(context);
    const uint8_t block[4] = {r, g, b, type};
    return c.host->setBlock && c.host->setBlock(x, y, z, block) ? 1 : 0;
}

int hostSpawnParticles(void *context, double x, double y, double z, float vx, float vy, float vz, float r, float g,
                       float b, float size, int32_t count, int32_t style, float spread) {
    const CallerContext &c = caller(context);
    const float velocity[3] = {vx, vy, vz}, rgb[3] = {r, g, b};
    return c.host->spawnParticles && c.host->spawnParticles(x, y, z, velocity, rgb, size, count, style, spread) ? 1 : 0;
}

uint32_t hostGetQuestCount(void *context) {
    const CallerContext &c = caller(context);
    return c.host->getQuests ? (uint32_t)c.host->getQuests().size() : 0;
}

int hostGetQuest(void *context, uint32_t index, AvQuest *out) {
    const CallerContext &c = caller(context);
    if (!c.host->getQuests)
        return 0;
    const std::vector<AvQuest> list = c.host->getQuests();
    if (index >= list.size())
        return 0;
    if (out)
        *out = list[index];
    return 1;
}

int hostSay(void *context, const char *text) {
    const CallerContext &c = caller(context);
    return c.host->say && text && c.host->say(text) ? 1 : 0;
}

int hostShowChat(void *context, const char *text, float r, float g, float b) {
    const CallerContext &c = caller(context);
    return c.host->showChat && text && c.host->showChat(text, r, g, b) ? 1 : 0;
}

int hostTell(void *context, uint64_t player, const char *text) {
    const CallerContext &c = caller(context);
    return c.host->tell && text && c.host->tell(player, text) ? 1 : 0;
}

int32_t hostGetPlayerName(void *context, uint64_t id, char *buffer, uint32_t capacity) {
    const CallerContext &c = caller(context);
    const std::optional<std::string> name = c.host->playerName ? c.host->playerName(id) : std::nullopt;
    if (!name)
        return -1;
    if (buffer && capacity > 0) {
        const size_t n = std::min<size_t>(capacity - 1, name->size());
        std::memcpy(buffer, name->data(), n);
        buffer[n] = 0;
    }
    return (int32_t)std::min<size_t>(name->size(), INT32_MAX);
}

uint32_t hostGetCreatureCount(void *context) {
    const CallerContext &c = caller(context);
    return c.host->getCreatures ? (uint32_t)c.host->getCreatures().size() : 0;
}

int hostGetCreature(void *context, uint32_t index, AvCreature *out) {
    const CallerContext &c = caller(context);
    if (!c.host->getCreatures)
        return 0;
    const std::vector<AvCreature> list = c.host->getCreatures();
    if (index >= list.size())
        return 0;
    if (out)
        *out = list[index];
    return 1;
}

std::string trimmed(const std::string &s) {
    const auto first = s.find_first_not_of(" \t");
    return first == std::string::npos ? std::string() : s.substr(first, s.find_last_not_of(" \t") - first + 1);
}

} // namespace

// One loaded plugin: its DLL, the table it was given and what it gave back.
struct ModLoader::Plugin {
    CallerContext context;
    HMODULE library = nullptr;
    AvHost table{};
    AvMod mod{};
    bool initialized = false;
    Content *content = nullptr;
    Content::OverrideId overrideId = 0;
    ~Plugin() {
        if (initialized && mod.onShutdown)
            mod.onShutdown(mod.user);
        if (library)
            FreeLibrary(library);
        if (content)
            content->removeOverride(overrideId);
    }
};

std::vector<std::string> splitModList(const std::string &list) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= list.size()) {
        const size_t comma = std::min(list.find(',', start), list.size());
        const std::string name = trimmed(list.substr(start, comma - start));
        if (!name.empty())
            out.push_back(name);
        start = comma + 1;
    }
    return out;
}

bool readModInfo(const std::filesystem::path &folder, ModInfo &info, std::string *error) {
    Config ini;
    if (!ini.load(folder / "mod.ini")) {
        if (error)
            *error = "no mod.ini";
        return false;
    }
    info.folder = folder;
    info.folderName = utf8(folder.filename());
    info.name = ini.text("Mod", "Name", info.folderName);
    info.version = ini.text("Mod", "Version");
    info.description = ini.text("Mod", "Description");
    info.plugin = ini.text("Mod", "Plugin");
    // A plugin is a file name inside the mod's own folder, nothing else.
    if (info.plugin.find_first_of("/\\:") != std::string::npos) {
        if (error)
            *error = "Plugin must be a file in the mod's folder";
        return false;
    }
    return true;
}

ModLoader::ModLoader(ModHost h) : host(std::move(h)) {}

ModLoader::~ModLoader() { shutdown(); }

void ModLoader::report(const std::string &mod, const std::string &message) const {
    if (host.log)
        host.log(mod, message);
}

void ModLoader::load(const std::filesystem::path &modsFolder, const std::vector<std::string> &order, Content &content) {
    for (const std::string &folderName : order) {
        if (std::any_of(mods.begin(), mods.end(), [&](const ModInfo &mod) { return mod.folderName == folderName; })) {
            report(folderName, "not loaded: already active");
            continue;
        }
        ModInfo info;
        std::string error;
        const std::filesystem::path folder = modsFolder / pathOfUtf8(folderName);
        const bool plainFolder = !folderName.empty() && folderName.find_first_of("/\\:") == std::string::npos && folderName != "." &&
                                 folderName != "..";
        AvModLoadFunction entry = nullptr;
        for (const auto &module : builtinModules())
            if (module.id == folderName) {
                info.folderName = folderName;
                info.name = folderName;
                info.version = module.version;
                info.fingerprint = module.fingerprint;
                info.builtin = true;
                entry = module.load;
                break;
            }
        if (!plainFolder || (!entry && !readModInfo(folder, info, &error))) {
            report(folderName, "not loaded: " + (error.empty() ? std::string("bad folder name") : error));
            continue;
        }
        std::error_code ec;
        if (!info.builtin) {
            const auto fingerprint = modFingerprint(folder, info.plugin, &error);
            if (!fingerprint) {
                report(info.name, "not loaded: " + error);
                continue;
            }
            info.fingerprint = *fingerprint;
        }

        auto plugin = std::make_unique<Plugin>();
        plugin->context = {info.name, &host};
        if (entry || !info.plugin.empty()) {
            if (!entry) {
                plugin->library = LoadLibraryW((folder / pathOfUtf8(info.plugin)).wstring().c_str());
                entry = plugin->library
                            ? reinterpret_cast<AvModLoadFunction>(GetProcAddress(plugin->library, "avModLoad"))
                            : nullptr;
            }
            if (!entry) {
                report(info.name, plugin->library ? "plugin has no avModLoad" : "plugin DLL could not be loaded");
                continue;
            }
        }
        // Startup may read this layer. Its registration is rolled back with the
        // plugin on any failure and removed on shutdown, even for content-only mods.
        if (!info.builtin && std::filesystem::is_directory(folder / "content", ec)) {
            plugin->overrideId = content.addOverride(folder / "content");
            if (!plugin->overrideId) {
                report(info.name, "not loaded: content registration failed");
                continue;
            }
            plugin->content = &content;
        }
        if (entry) {
            plugin->table = {sizeof(AvHost), AV_API_VERSION, &plugin->context, hostLog, hostReadContent,
                             hostPlaySound, hostGetTimeOfDay, hostSetTimeOfDay, hostGetBlock, hostGetPlayer,
                             hostShowText, hostShowBanner, hostGetStaticCount, hostGetStatic,
                             hostGetMission, hostSetBlock, hostGetCreatureCount, hostGetCreature,
                             hostSpawnParticles, hostGetQuestCount, hostGetQuest, hostSay, hostShowChat,
                             hostTell, hostGetPlayerName};
            plugin->mod.size = sizeof(AvMod);
            if (!entry(&plugin->table, &plugin->mod)) {
                report(info.name, "plugin declined to load");
                continue;
            }
            plugin->initialized = true;
        }
        plugins.push_back(std::move(plugin));
        report(info.name, "loaded" + (info.builtin ? std::string(" (built in)") :
                    (info.version.empty() ? std::string() : " (version " + info.version + ")")));
        mods.push_back(std::move(info));
    }
}

void ModLoader::frame(double seconds) {
    for (const auto &p : plugins)
        if (p->mod.onFrame)
            p->mod.onFrame(p->mod.user, seconds);
}

void ModLoader::zoneReady(int32_t zx, int32_t zy) {
    for (const auto &p : plugins)
        if (p->mod.onZoneReady)
            p->mod.onZoneReady(p->mod.user, zx, zy);
}

void ModLoader::playerHit(uint64_t attacker, uint8_t kind, float &amount) {
    for (const auto &p : plugins)
        if (p->mod.onPlayerHit)
            p->mod.onPlayerHit(p->mod.user, attacker, kind, &amount);
}

void ModLoader::levelUp(int32_t level) {
    for (const auto &p : plugins)
        if (p->mod.onLevelUp)
            p->mod.onLevelUp(p->mod.user, level);
}

void ModLoader::hitSeen(uint64_t attacker, uint64_t target, uint8_t kind, float amount) {
    for (const auto &p : plugins)
        if (p->mod.onHitSeen)
            p->mod.onHitSeen(p->mod.user, attacker, target, kind, amount);
}

void ModLoader::missionComplete(int32_t cx, int32_t cy) {
    for (const auto &p : plugins)
        if (p->mod.onMissionComplete)
            p->mod.onMissionComplete(p->mod.user, cx, cy);
}

void ModLoader::creatureDeath(uint64_t id, int32_t type, int32_t level, uint64_t credited) {
    for (const auto &p : plugins)
        if (p->mod.onCreatureDeath)
            p->mod.onCreatureDeath(p->mod.user, id, type, level, credited);
}

void ModLoader::questComplete(uint32_t quest) {
    for (const auto &p : plugins)
        if (p->mod.onQuestComplete)
            p->mod.onQuestComplete(p->mod.user, quest);
}

void ModLoader::chat(uint64_t sender, const std::string &text) {
    for (const auto &p : plugins)
        if (p->mod.onChat)
            p->mod.onChat(p->mod.user, sender, text.c_str());
}

void ModLoader::itemReceived(int32_t type, int32_t subtype, int32_t level) {
    for (const auto &p : plugins)
        if (p->mod.onItemReceived)
            p->mod.onItemReceived(p->mod.user, type, subtype, level);
}

void ModLoader::playerJoin(uint64_t id) {
    for (const auto &p : plugins)
        if (p->mod.onPlayerJoin)
            p->mod.onPlayerJoin(p->mod.user, id);
}

void ModLoader::playerLeave(uint64_t id) {
    for (const auto &p : plugins)
        if (p->mod.onPlayerLeave)
            p->mod.onPlayerLeave(p->mod.user, id);
}

void ModLoader::creatureTamed(uint64_t id, int32_t type, uint64_t owner) {
    for (const auto &p : plugins)
        if (p->mod.onCreatureTamed)
            p->mod.onCreatureTamed(p->mod.user, id, type, owner);
}

void ModLoader::shutdown() {
    // Last loaded, first unloaded.
    for (auto it = plugins.rbegin(); it != plugins.rend(); ++it) {
        it->reset();
    }
    plugins.clear();
    mods.clear();
}
