#pragma once
// The mod loader (docs/modding.md). Mods live in folders under Mods/; only the ones named in
// AlphaVulkan.ini [Mods] Load are used, in that order. Each may bring
//
//   mod.ini     [Mod] Name, Version, Description, Plugin (a DLL in the folder)
//   content/    files that replace or add content keys (Content::addOverride)
//   the plugin  a DLL speaking the C interface of sdk/alphavulkan_mod.h
//
// The engine side of the plugin interface is a ModHost: the loader turns its calls into the
// AvHost function table, so plugins never see engine types.
#include "alphavulkan_mod.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Content;

// What the engine lends to mods. Each hook may be left empty.
struct ModHost {
    std::function<void(const std::string &mod, const std::string &message)> log;
    const Content *content = nullptr;
    std::function<bool(const std::string &key, float volume)> playSound;
    std::function<double()> getTimeOfDay;
    std::function<void(double)> setTimeOfDay;
    // (version 2) a block of a held zone: false when the zone is not held.
    std::function<bool(int32_t x, int32_t y, int32_t z, uint8_t rgbType[4])> getBlock;
    // (version 3) the local player: false when nobody is playing.
    std::function<bool(AvPlayer &)> getPlayer;
    // (version 4) floating text and the banner: false when no game is showing.
    std::function<bool(double x, double y, double z, const std::string &text, float r, float g, float b, float size)> showText;
    std::function<bool(const std::string &name, const std::string &detail)> showBanner;
    // (version 4) a held zone's statics: null when the zone is not held.
    std::function<const std::vector<AvStatic> *(int32_t zx, int32_t zy)> getStatics;
    // (version 5) a map cell's mission: false when there is none.
    std::function<bool(int32_t cx, int32_t cy, AvMission &)> getMission;
    // (version 5) a block edit: false when the zone is not held or blocks cannot change here.
    std::function<bool(int32_t x, int32_t y, int32_t z, const uint8_t rgbType[4])> setBlock;
    // (version 6) the creatures held now.
    std::function<std::vector<AvCreature>()> getCreatures;
    // (version 7) a local particle burst: false when no world is showing.
    std::function<bool(double x, double y, double z, const float velocity[3], const float rgb[3], float size,
                       int32_t count, int32_t style, float spread)>
        spawnParticles;
    // (version 8) the local player's quests.
    std::function<std::vector<AvQuest>()> getQuests;
    // (version 9) says a chat line, or shows one here only: false when no game is showing.
    std::function<bool(const std::string &text)> say;
    std::function<bool(const std::string &text, float r, float g, float b)> showChat;
};

struct ModInfo {
    std::string folderName; // as listed in [Mods] Load
    std::string name, version, description, plugin;
    std::filesystem::path folder;
    bool builtin = false; // source module compiled into this engine
    std::string fingerprint; // SHA-256 of the declared source inputs or loaded mod files
};

// Reads `folder`/mod.ini; false (with a reason) if the folder is not a mod.
bool readModInfo(const std::filesystem::path &folder, ModInfo &info, std::string *error = nullptr);

class ModLoader {
  public:
    explicit ModLoader(ModHost host);
    ~ModLoader();
    ModLoader(const ModLoader &) = delete;
    ModLoader &operator=(const ModLoader &) = delete;

    // Loads compiled source modules or mod folders in the explicit configured order.
    // Problems are reported through the host's log and skip that mod.
    void load(const std::filesystem::path &modsFolder, const std::vector<std::string> &order, Content &content);
    // Calls every plugin's onFrame.
    void frame(double seconds);
    // Calls every plugin's onZoneReady.
    void zoneReady(int32_t zx, int32_t zy);
    // Calls every plugin's onPlayerHit, which may change `amount`.
    void playerHit(uint64_t attacker, uint8_t kind, float &amount);
    // Calls every plugin's onLevelUp.
    void levelUp(int32_t level);
    // Calls every plugin's onHitSeen.
    void hitSeen(uint64_t attacker, uint64_t target, uint8_t kind, float amount);
    // Calls every plugin's onMissionComplete.
    void missionComplete(int32_t cx, int32_t cy);
    // Calls every plugin's onCreatureDeath.
    void creatureDeath(uint64_t id, int32_t type, int32_t level, uint64_t credited);
    // Calls every plugin's onQuestComplete.
    void questComplete(uint32_t quest);
    // Calls every plugin's onChat.
    void chat(uint64_t sender, const std::string &text);
    // Calls every plugin's onItemReceived.
    void itemReceived(int32_t type, int32_t subtype, int32_t level);
    // Calls every plugin's onShutdown and unloads them (also done by the destructor).
    void shutdown();

    const std::vector<ModInfo> &loaded() const { return mods; }
    // The host's hooks, for a running game to fill in (and empty again when it ends).
    ModHost &hooks() { return host; }

  private:
    struct Plugin; // a loaded DLL and its AvMod
    void report(const std::string &mod, const std::string &message) const;

    ModHost host;
    std::vector<ModInfo> mods;
    std::vector<std::unique_ptr<Plugin>> plugins;
};

// "A, B ,C" -> {"A", "B", "C"}: the [Mods] Load list.
std::vector<std::string> splitModList(const std::string &list);
