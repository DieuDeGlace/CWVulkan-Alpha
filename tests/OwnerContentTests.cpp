// Exercises setup with private output; the owned installation stays read-only.
#include "Config.h"
#include "DataStore.h"
#include "FileHash.h"
#include "Install.h"
#include "OwnerContent.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
#include <stdexcept>

namespace {
int failures = 0;
void check(bool value, const char *description) {
    if (!value) {
        std::printf("FAIL %s\n", description);
        failures++;
    }
}

class ScratchDirectory {
  public:
    ScratchDirectory()
        : path(fs::canonical(fs::temp_directory_path()) /
              (L"OwnerContentTests-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
               std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        if (!fs::create_directory(path))
            throw std::runtime_error("Could not create a unique owner-content test directory");
    }

    ~ScratchDirectory() {
        std::error_code error;
        fs::remove_all(path, error);
    }

    const fs::path path;
};
}

int wmain(int argc, wchar_t **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    try {
    const ScratchDirectory directory;
    const auto &scratch = directory.path;
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

    // Extractor §A1: identifying headers and the reference inventory, on synthetic files.
    {
        const auto headers = scratch / "headers";
        fs::create_directory(headers);
        const auto write = [&](const char *name, const std::string &bytes) {
            std::ofstream(headers / name, std::ios::binary) << bytes;
            return headers / name;
        };
        const std::string plx = std::string("\0\0\0\0\x16\0\0\0\x01\0\0\0\x0e\0\0\0", 16) + "PlasmaGraphics" + "rest";
        check(owner::hasIdentifyingHeader("gui.plx", write("gui.plx", plx)), "PlasmaGraphics header recognised");
        check(!owner::hasIdentifyingHeader("gui.plx", write("bad.plx", "PlasmaGraphics")), "other .plx bytes rejected");
        check(owner::hasIdentifyingHeader("data1.db", write("data1.db", std::string("SQLite format 3\0more", 20))),
              "SQLite 3 header recognised");
        check(!owner::hasIdentifyingHeader("data1.db", write("data2.db", "SQLite format 2")), "other .db bytes rejected");
        check(owner::hasIdentifyingHeader("resource1.dat", write("resource1.dat", std::string("\0\x01\0\0\0\x0d", 6))),
              "TrueType sfnt version recognised");
        check(!owner::hasIdentifyingHeader("resource2.dat", write("resource2.dat", "OTTO")), "other font bytes rejected");
        check(owner::hasIdentifyingHeader("db.dat", write("db.dat", "PLACEHOLDER")) &&
                  !owner::hasIdentifyingHeader("db.dat", write("empty.dat", "")), "db.dat needs only content");
        const auto &required = owner::requiredFiles();
        check(std::find(required.begin(), required.end(), "db.dat") != required.end() &&
                  required.size() == owner::referenceFiles().size(), "db.dat is a required start-up file");
        bool sized = true;
        for (const auto &file : owner::referenceFiles())
            sized = sized && file.size > 0 && std::string(file.sha256).size() == 64;
        check(sized && owner::referenceFiles().back().size == 32, "every reference file records a size and SHA-256");

        // Missing stores are reported and never created by the check.
        const auto partial = scratch / "partial";
        fs::create_directory(partial);
        for (const auto &name : required)
            if (!name.ends_with(".db"))
                std::ofstream(partial / name, std::ios::binary) << "not original content";
        const auto report = owner::inspect(partial, true);
        check(!report.ready() && !fs::exists(partial / "data1.db") && !fs::exists(partial / "data4.db"),
              "missing stores rejected without creating them");
    }

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
        check(entries == records + 9, "every store asset plus eight loose files and manifest extracted");
        for (const auto &note : report.notes)
            std::printf("  note: %s\n", note.c_str());
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
