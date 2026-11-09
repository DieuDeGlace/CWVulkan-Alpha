#include "FileHash.h"
#include "GenerationProfile.h"
#include "Saves.h"
#include "WorldSession.h"
#include <windows.h>
#include <chrono>
#include <cstdio>

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
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
