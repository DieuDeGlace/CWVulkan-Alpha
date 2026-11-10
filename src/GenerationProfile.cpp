#include "GenerationProfile.h"
#include "Saves.h"

std::string generationProfile(bool swimmableWater, const std::vector<ModInfo> &mods) {
    std::string result = swimmableWater ? "AlphaVulkan terrain 1\nWater=swimmable-3-v1\n" : NATIVE_GENERATION_PROFILE;
    for (const auto &mod : mods) {
        // Length prefixes avoid ambiguous identifiers/metadata. Load order matters.
        result += mod.builtin ? "Source=" : "Mod=";
        result += std::to_string(mod.folderName.size()) + ":" + mod.folderName;
        result += std::to_string(mod.version.size()) + ":" + mod.version + '\n';
        result += "Inputs=" + std::to_string(mod.fingerprint.size()) + ':' + mod.fingerprint + '\n';
    }
    return result;
}

bool ensureGenerationProfile(const std::filesystem::path &worldFile, const std::string &profile, std::string *error) {
    if (profile.empty())
        return true; // legacy library callers; production entry points supply a profile
    constexpr const char *KEY = "av_generation_profile";
    {
        SaveDb existing;
        if (existing.open(worldFile, SaveDb::Mode::ReadOnly)) {
            const auto bytes = existing.get(KEY);
            const std::string stored = bytes ? std::string(bytes->begin(), bytes->end()) : NATIVE_GENERATION_PROFILE;
            if (stored == profile)
                return true;
            if (error)
                *error = "This world uses different water or mod settings. Restore them or create a new world.";
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::create_directories(worldFile.parent_path(), ec);
    if (ec) {
        if (error)
            *error = "Cannot create the world save directory.";
        return false;
    }
    SaveDb created;
    if (!created.open(worldFile, SaveDb::Mode::ReadWrite, error))
        return false;
    if (!created.putIfAbsent(KEY, std::vector<uint8_t>(profile.begin(), profile.end()))) {
        if (error)
            *error = "Cannot record the world generation profile.";
        return false;
    }
    const auto stored = created.get(KEY);
    if (!stored || std::string(stored->begin(), stored->end()) != profile) {
        if (error)
            *error = "Another game session created this world with different water or mod settings.";
        return false;
    }
    return true;
}
