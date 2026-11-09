// Checks the mod loader against a made-up Mods folder: content overrides (order, new keys,
// names that try to leave the folder), mod.ini handling, and the TimeLapse example plugin
// built next to this test. Needs no game install.
#include "DataStore.h"
#include "Install.h"
#include "Mods.h"
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

    check(splitModList(" A, B ,,C ") == std::vector<std::string>{"A", "B", "C"}, "load list parsing");

    std::vector<std::string> log;
    double time = 6 * 3600000.0;
    ModHost host;
    host.log = [&](const std::string &mod, const std::string &message) { log.push_back(mod + ": " + message); };
    host.getTimeOfDay = [&] { return time; };
    host.setTimeOfDay = [&](double t) { time = t; };
    Content content; // no stores opened: only the overrides answer
    host.content = &content;
    {
        ModLoader loader(host);
        loader.load(mods, {"First", "Second", "Escape", "NotAMod", "Missing", "..", "TimeLapse"}, content);
        check(loader.loaded().size() == 3, "three mods load (First, Second, TimeLapse)");

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
    for (const auto &l : log)
        std::printf("  log  %s\n", l.c_str());

    fs::remove_all(mods, ec);
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
