#pragma once
#include "Mods.h"
#include <filesystem>
#include <string>
#include <vector>

// Engine extension metadata, independent of the native save record formats.
inline constexpr const char *NATIVE_GENERATION_PROFILE = "AlphaVulkan terrain 1\nWater=native\n";
std::string generationProfile(bool swimmableWater, const std::vector<ModInfo> &mods);
// Pins new world files; existing unmarked files are treated as native. A rejected
// profile is checked read-only, before world state or the world list is changed.
bool ensureGenerationProfile(const std::filesystem::path &worldFile, const std::string &profile,
                             std::string *error = nullptr);
