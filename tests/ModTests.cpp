// Checks the mod loader against a made-up Mods folder: content overrides (order, new keys,
// names that try to leave the folder), mod.ini handling, and the TimeLapse example plugin
// built next to this test. Needs no game install.
#include "DataStore.h"
#include "Install.h"
#include "FileHash.h"
#include "ModFingerprint.h"
#include "Mods.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <windows.h>

static int failures = 0;
static void check(bool ok, const std::string &what) {
    if (!ok) {
        failures++;
        std::printf("FAIL %s\n", what.c_str());
    }
}

static void write(const fs::path &file, const std::string &text) {
    fs::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << text;
}

static std::string text(const std::optional<std::vector<uint8_t>> &bytes) {
    return bytes ? std::string(bytes->begin(), bytes->end()) : "<none>";
}

int main() {
    // A scratch Mods folder next to the test.
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    const fs::path here = fs::path(self).parent_path();
    const fs::path mods = here / "ModTests-scratch";
    std::error_code ec;
    fs::remove_all(mods, ec);
    write(mods / "First" / "mod.ini", "[Mod]\nName=First\nVersion=2\n");
    write(mods / "First" / "content" / "thing.cub", "first");
    write(mods / "First" / "content" / "only-first.wav", "f");
    write(mods / "Second" / "mod.ini", "[Mod]\nName=Second\n");
    write(mods / "Second" / "content" / "thing.cub", "second");
    write(mods / "Escape" / "mod.ini", "[Mod]\nPlugin=..\\..\\evil.dll\n");
    fs::create_directories(mods / "NotAMod");
    // The example plugin, copied in as a mod.
    fs::create_directories(mods / "TimeLapse");
    fs::copy_file(here / "Mods" / "TimeLapse" / "TimeLapse.dll", mods / "TimeLapse" / "TimeLapse.dll", ec);
    check(!ec, "example plugin built at build/Mods/TimeLapse");
    write(mods / "TimeLapse" / "mod.ini", "[Mod]\nName=TimeLapse\nPlugin=TimeLapse.dll\n");
    fs::create_directories(mods / "Startup");
    fs::copy_file(here / "test-plugins" / "ModStartupProbe.dll", mods / "Startup" / "Probe.dll", ec);
    check(!ec, "startup probe plugin built outside the package");
    write(mods / "Startup" / "mod.ini", "[Mod]\nName=Startup\nPlugin=Probe.dll\n");
    write(mods / "Startup" / "content" / "startup.txt", "N");
    write(mods / "Broken" / "mod.ini", "[Mod]\nPlugin=missing.dll\n");
    write(mods / "Broken" / "content" / "leaked.cub", "must not load");

    check(splitModList(" A, B ,,C ") == std::vector<std::string>{"A", "B", "C"}, "load list parsing");

    std::vector<std::string> log;
    double time = 6 * 3600000.0;
    ModHost host;
    host.log = [&](const std::string &mod, const std::string &message) { log.push_back(mod + ": " + message); };
    host.getTimeOfDay = [&] { return time; };
    host.setTimeOfDay = [&](double t) { time = t; };
    Content content; // no stores opened: only the overrides answer
    host.content = &content;
    const auto externalLayer = content.addOverride(mods / "Second" / "content");
    {
        ModLoader loader(host);
        loader.load(mods, {"First", "Second", "Escape", "NotAMod", "Missing", "..", "TimeLapse", "Startup", "Broken"}, content);
        check(loader.loaded().size() == 3, "three mods load (First, Second, TimeLapse)");
        check(!content.read("startup.txt") && !content.read("leaked.cub"), "declined and missing DLL mods leave no content layer");
        check(std::all_of(loader.loaded().begin(), loader.loaded().end(), [](const ModInfo &mod) {
            return mod.fingerprint.size() == 64;
        }), "every active mod has an input fingerprint");

        check(text(content.read("thing.cub")) == "second", "later mod wins");
        check(text(content.read("only-first.wav")) == "f", "earlier mod's other files stay");
        check(!content.read("../First/mod.ini"), "keys cannot leave the content folder");
        check(content.list(".cub") == std::vector<std::string>{"thing.cub"}, "list shows override keys once");

        // SDK version 2: the zone hook reaches the plugin.
        loader.zoneReady(32800, 32801);
        loader.zoneReady(32800, 32802);

        // TimeLapse: one game hour per real second.
        loader.frame(0.5);
        check(std::abs(time - 6.5 * 3600000.0) < 1, "plugin moved the clock half an hour");
        time = 86400000.0 - 1000;
        loader.frame(1);
        check(time >= 0 && time < 3600000.0, "plugin wraps the clock past midnight");
    }
    check(!content.read("only-first.wav") && text(content.read("thing.cub")) == "second",
          "unload removes owned layers and preserves another registration of the same folder");
    check(content.removeOverride(externalLayer) && !content.removeOverride(externalLayer) && !content.read("thing.cub"),
          "external override lifetime remains independent");
    write(mods / "Startup" / "content" / "startup.txt", "Y");
    std::vector<std::pair<uint64_t, std::string>> told;
    host.tell = [&](uint64_t player, const std::string &line) {
        told.push_back({player, line});
        return true;
    };
    host.playerName = [](uint64_t id) { return id == 7 ? std::optional<std::string>("Ada") : std::nullopt; };
    {
        ModLoader loader(host);
        loader.load(mods, {"Startup"}, content);
        check(loader.loaded().size() == 1 && text(content.read("startup.txt")) == "Y", "successful startup retains its content");
        // SDK version 10: join, leave and taming reach the plugin; tell and getPlayerName reach the host.
        loader.playerJoin(7);
        loader.playerLeave(7);
        loader.creatureTamed(5, 44, 7);
        check(told.size() == 1 && told[0].first == 7 && told[0].second == "welcome Ada", "v10 join hook tells the player");
        loader.shutdown();
        check(!content.read("startup.txt"), "successful shutdown removes its content");
    }
    const auto originalFingerprint = modFingerprint(mods / "First", "");
    write(mods / "First" / "content" / "thing.cub", "changed without a version bump");
    check(originalFingerprint && originalFingerprint != modFingerprint(mods / "First", ""), "content bytes change the fingerprint");
    const auto changedFingerprint = modFingerprint(mods / "First", "");
    write(mods / "First" / "content" / "renamed.cub", "f");
    fs::remove(mods / "First" / "content" / "only-first.wav");
    check(changedFingerprint != modFingerprint(mods / "First", ""), "content key set changes the fingerprint");
    fs::copy(mods / "First", mods / "Relocated", fs::copy_options::recursive, ec);
    check(!ec && modFingerprint(mods / "First", "") == modFingerprint(mods / "Relocated", ""), "fingerprints do not depend on install paths");
    const auto pluginFingerprint = modFingerprint(mods / "TimeLapse", "TimeLapse.dll");
    std::ofstream(mods / "TimeLapse" / "TimeLapse.dll", std::ios::binary | std::ios::app) << "test overlay";
    check(pluginFingerprint && pluginFingerprint != modFingerprint(mods / "TimeLapse", "TimeLapse.dll"),
          "DLL bytes change the fingerprint without executing the modified file");
    check(!modFingerprint(mods / "Broken", "missing.dll"), "missing declared input fails fingerprinting");
    const std::string abc = "abc";
    check(bytesSha256({reinterpret_cast<const uint8_t *>(abc.data()), abc.size()}) ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "byte hashing matches the standard SHA-256 vector");
    check(bytesSha256({}) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
          "empty byte hashing matches the standard SHA-256 vector");
    const std::vector<uint8_t> million(1000000, 'a');
    check(bytesSha256(million) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
          "multi-block byte hashing matches the standard million-byte vector");
    auto logged = [&](const std::string &line) {
        for (const auto &l : log)
            if (l.find(line) != std::string::npos)
                return true;
        return false;
    };
    check(logged("First: loaded (version 2)"), "load reported with version");
    check(logged("Escape: not loaded"), "plugin path outside the folder refused");
    check(logged("NotAMod: not loaded: no mod.ini"), "folder without mod.ini skipped");
    check(logged("TimeLapse: hello"), "plugin logged on load");
    check(logged("TimeLapse: goodbye (2 zones seen)"), "plugin shut down on unload, having seen two zones");
    check(logged("Startup: startup content present") && logged("Startup: shutdown content present"),
          "DLL lifecycle sees its layer before decline and through shutdown");
    check(logged("Startup: name 3 cut 3 'Ad' unknown -1"), "v10 getPlayerName: full length, terminated cut, unknown -1");
    check(logged("Startup: left 7") && logged("Startup: tamed 5 type 44 by 7"), "v10 leave and taming hooks");
    for (const auto &l : log)
        std::printf("  log  %s\n", l.c_str());

    fs::remove_all(mods, ec);
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
