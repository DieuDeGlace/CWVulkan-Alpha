// Exercises setup with private output; the owned installation stays read-only.
#include "Config.h"
#include "DataStore.h"
#include "FileHash.h"
#include "Install.h"
#include "OwnerContent.h"
#include <windows.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>

namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}
}

int wmain(int argc, wchar_t **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    try {
    const auto scratch = fs::current_path() / (L"OwnerContentTests-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
        std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(scratch);
    const auto known = scratch / "abc.txt";
    std::ofstream(known, std::ios::binary) << "abc";
    check(fileSha256(known) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "streaming SHA-256 matches standard vector");
    check(!fileSha256(scratch / "missing"), "missing hash input fails");
    check(!owner::inspect(scratch, false).ready(), "incomplete install rejected");
    const auto fake = scratch / "fake";
    fs::create_directory(fake);
    for (const auto &name : owner::requiredFiles())
        std::ofstream(fake / name, std::ios::binary) << "not original content";
    check(!owner::inspect(fake, false).ready(), "file presence alone does not identify supported image");

    const auto configuration = scratch / "settings.ini";
    std::ofstream(configuration, std::ios::binary) << "\xEF\xBB\xBF; [Game] mentioned in a comment\r\n  [Game] ; real section\r\n  Path = old\r\n[Video]\r\nWidth=1234\r\n";
    std::string error;
    const auto unicodePath = scratch / L"owner-\u03b1;%41#";
    check(owner::remember(configuration, unicodePath, error), "installation registration succeeds");
    Config config;
    check(config.load(configuration) && config.text("Game", "Path") == utf8(fs::absolute(unicodePath)) &&
          config.text("Video", "Width") == "1234", "UTF-8 path saved without changing other settings");
    std::ofstream(configuration, std::ios::binary) << "[Game]";
    check(owner::remember(configuration, unicodePath, error) && config.load(configuration) &&
          config.text("Game", "Path") == utf8(fs::absolute(unicodePath)), "section without newline handled");
    check(config.text("Video", "Width").empty(), "configuration reload clears removed settings");

    const auto installation = argc > 1 ? fs::path(argv[1]) : install::locate("");
    if (installation.empty()) {
        std::printf("Owner extraction checks SKIPPED: no installation supplied/found.\n");
    } else {
        std::map<std::string, std::optional<std::string>> before;
        for (const auto &name : owner::requiredFiles())
            before[name] = fileSha256(installation / name);
        const auto report = owner::inspect(installation, true);
        check(report.ready(), "supported owner executable and every asset decode successfully");
        for (const auto &problem : report.problems)
            std::printf("  %s\n", problem.c_str());
        const auto cache = scratch / L"private-\u03b1";
        const bool extractedOk = owner::extract(installation, cache, error);
        check(extractedOk, "private Unicode cache extracted");
        if (!extractedOk)
            throw std::runtime_error(error);
        if (!error.empty())
            std::printf("  %s\n", error.c_str());
        Content content;
        check(content.open(installation), "owner content opened read-only");
        const auto expected = content.read("airship.cub");
        std::ifstream extracted(cache / "airship.cub", std::ios::binary);
        const std::vector<uint8_t> actual((std::istreambuf_iterator<char>(extracted)), {});
        check(expected && actual == *expected, "decoded model matches direct content reader");
        check(fileSha256(cache / "resource1.dat") == before["resource1.dat"], "loose font copied exactly");
        check(fs::is_regular_file(cache / "OWNER-CONTENT.txt") && !fs::exists(cache / "Cube.exe") &&
              !fs::exists(cache / "data1.db"), "complete private manifest without executable or stores");
        size_t entries = 0;
        for (const auto &entry : fs::directory_iterator(cache)) {
            (void)entry;
            entries++;
        }
        size_t records = 0;
        for (size_t count : report.storeEntries)
            records += count;
        check(entries == records + 8, "every store asset plus seven loose files and manifest extracted");
        check(!owner::extract(installation, cache, error), "existing private output never overwritten");
        check(!owner::extract(installation, installation / "should-not-exist", error), "output inside original rejected");
        for (const auto &[name, hash] : before)
            check(hash && fileSha256(installation / name) == hash, "original required file unchanged");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
    } catch (const std::exception &error) {
        std::printf("FAIL unexpected setup exception: %s\n", error.what());
        return 1;
    }
}
