#include "FileHash.h"
#include "DataStore.h"
#include "GenerationProfile.h"
#include "Saves.h"
#include "WorldSession.h"
#include <windows.h>
#include <chrono>
#include <cstdio>
#include <fstream>

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
    const auto scratch = std::filesystem::current_path() /
        (L"GenerationProfileTests-\u03b1%41#-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
         std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto saves = scratch / "Save";
    std::filesystem::create_directories(saves);
    const auto native = generationProfile(false, {});
    const auto swimming = generationProfile(true, {});
    check(native == NATIVE_GENERATION_PROFILE && swimming != native, "native and swimming profiles are distinct");
    ModInfo first, second;
    first.folderName = "First";
    first.version = "1";
    first.builtin = true;
    second.folderName = "Second";
    check(generationProfile(true, {first, second}) != generationProfile(true, {second, first}), "module order is part of profile");
    second.version = "2";
    check(generationProfile(true, {second}) != generationProfile(true, {}), "mod identity/version is part of profile");
    std::string error;
    {
        WorldSession session;
        check(session.open(scratch, "swimming", 41, -1, &error, swimming), "new world pins swimming profile");
        session.clock.day = 3;
        session.saveClock();
    }
    const auto worldFile = saves / "world_swimming.db";
    const auto worldHash = fileSha256(worldFile), listHash = fileSha256(saves / "worlds.db");
    {
        WorldSession wrong;
        check(!wrong.open(scratch, "swimming", 41, -1, &error, native), "different terrain profile rejected before opening world");
    }
    check(worldHash && worldHash == fileSha256(worldFile) && listHash == fileSha256(saves / "worlds.db"),
          "rejected world leaves both world and world list unchanged");
    {
        WorldSession matching;
        check(matching.open(scratch, "swimming", 99, -1, &error, swimming) && matching.clock.day == 3 && matching.seed() == 41,
              "matching profile reopens saved state with original seed");
    }
    const auto legacyFile = saves / "world_legacy.db";
    {
        SaveDb legacy;
        check(legacy.open(legacyFile, SaveDb::Mode::ReadWrite), "legacy world created");
        legacy.put("time", WorldClock{}.serialize());
    }
    const auto legacyHash = fileSha256(legacyFile);
    check(!ensureGenerationProfile(legacyFile, swimming, &error), "unmarked legacy save cannot silently adopt swimming terrain");
    check(ensureGenerationProfile(legacyFile, native, &error), "unmarked legacy save accepted with native profile");
    check(legacyHash && legacyHash == fileSha256(legacyFile), "legacy checks are read-only, including percent/hash/Unicode paths");
    const auto serverFile = saves / "world_server_41.db";
    check(ensureGenerationProfile(serverFile, swimming, &error) && ensureGenerationProfile(serverFile, swimming, &error) &&
          !ensureGenerationProfile(serverFile, native, &error), "dedicated-server save path pins the same policy");
    const auto modFolder = scratch / "Mods" / "UnchangedVersion";
    std::filesystem::create_directories(modFolder / "content");
    std::ofstream(modFolder / "mod.ini") << "[Mod]\nVersion=1\n";
    const auto asset = modFolder / "content" / "sample.cub";
    std::ofstream(asset, std::ios::binary) << "original";
    Content content;
    ModLoader loader({});
    loader.load(scratch / "Mods", {"UnchangedVersion"}, content);
    const auto originalProfile = generationProfile(true, loader.loaded());
    check(loader.loaded().size() == 1 && loader.loaded().front().fingerprint.size() == 64, "actual content mod has a pinned identity");
    {
        WorldSession session;
        check(session.open(scratch, "modded", 17, -1, &error, originalProfile), "actual mod profile pins a new world");
        session.clock.day = 7;
        session.saveClock();
    }
    const auto moddedFile = saves / "world_modded.db";
    const auto moddedHash = fileSha256(moddedFile), moddedListHash = fileSha256(saves / "worlds.db");
    loader.shutdown();
    std::ofstream(asset, std::ios::binary) << "modified";
    loader.load(scratch / "Mods", {"UnchangedVersion"}, content);
    const auto changedProfile = generationProfile(true, loader.loaded());
    check(originalProfile != changedProfile && loader.loaded().front().version == "1", "same version with changed bytes changes the profile");
    {
        WorldSession wrong;
        check(!wrong.open(scratch, "modded", 99, -1, &error, changedProfile), "changed code/content identity is rejected at the real world boundary");
    }
    check(moddedHash && moddedHash == fileSha256(moddedFile) && moddedListHash == fileSha256(saves / "worlds.db"),
          "changed-input rejection leaves world and world list unchanged");
    loader.shutdown();
    std::ofstream(asset, std::ios::binary) << "original";
    loader.load(scratch / "Mods", {"UnchangedVersion"}, content);
    {
        WorldSession restored;
        check(restored.open(scratch, "modded", 99, -1, &error, generationProfile(true, loader.loaded())) &&
              restored.seed() == 17 && restored.clock.day == 7, "restoring exact inputs reopens the original saved world");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
