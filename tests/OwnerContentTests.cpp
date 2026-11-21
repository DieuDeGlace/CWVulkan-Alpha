// Exercises setup with private output; the owned installation stays read-only.
#include "Config.h"
#include "DataStore.h"
#include "FileHash.h"
#include "GenerationProfile.h"
#include "Install.h"
#include "OwnerContent.h"
#include "WorldSession.h"
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

void saveImportChecks(const fs::path &scratch) {
    const auto original = scratch / L"Original-%41#-\u03b1";
    const auto source = original / "Save";
    const auto client = scratch / "Engine";
    const auto target = client / "Save"; // the production executable opens this sibling
    fs::create_directories(source);
    fs::create_directory(client);
    const std::string name = "online_Trail-\xce\xb1"; // a local world whose name happens to start with online_
    const auto stateName = pathOfUtf8("world_" + name + ".db");
    const auto mapName = pathOfUtf8("map_" + name + ".db");
    CharacterRecord character;
    character.name = "Traveler";
    character.level = 12;
    character.money = 235;
    character.saveVisit(17, name, {65536, 131072, 196608});
    WorldEntry first, second, mapOnly;
    first.name = name;
    first.seed = 17;
    first.previewSize[0] = first.previewSize[1] = first.previewSize[2] = 1;
    first.previewRgb = {11, 22, 33};
    second.name = "Second world";
    second.seed = 29;
    mapOnly.name = "Map only";
    const auto secondState = fs::path("world_" + second.name + ".db");
    {
        SaveDb characters, worlds, state, map, otherState, otherMap;
        if (!characters.open(source / "characters.db", SaveDb::Mode::ReadWrite) || !addCharacter(characters, character) ||
            !worlds.open(source / "worlds.db", SaveDb::Mode::ReadWrite) || !worlds.putNumber("num", 3) ||
            !worlds.put("0", first.serialize()) || !worlds.put("1", second.serialize()) ||
            !worlds.put("2", mapOnly.serialize()) || !worlds.put("3", {7, 1}) || // inactive deletion row
            !state.open(source / stateName, SaveDb::Mode::ReadWrite) ||
            !state.put("time", WorldClock{7, 8150}.serialize()) || !state.put("zone3_4", ZoneSave{}.serialize()) ||
            !state.put("extra_note", {11, 21}) ||
            !map.open(source / mapName, SaveDb::Mode::ReadWrite) || !map.putNumber("discovered", 31) ||
            !map.put("tile3_4", TileSave{}.serialize()) || !map.put("land2_3", LandSave{}.serialize()) ||
            !map.put("reg2_3", RegionSave{}.serialize()) ||
            !otherState.open(source / secondState, SaveDb::Mode::ReadWrite) ||
            !otherMap.open(source / "map_Map only.db", SaveDb::Mode::ReadWrite) || !otherMap.putNumber("discovered", 9))
            throw std::runtime_error("Cannot create synthetic single-player saves");
    }
    std::ofstream(source / "notes.txt") << "private notes";
    std::ofstream(source / "old-copy.db") << "unrelated file";
    std::ofstream(source / "world_server_78.db") << "unrelated dedicated world";
    std::ofstream(source / "world_server_78.db-wal") << "unrelated active journal";

    std::map<fs::path, std::optional<std::string>> hashes;
    for (const auto &entry : fs::directory_iterator(source)) hashes[entry.path()] = fileSha256(entry.path());
    const auto assertSource = [&] {
        for (const auto &[file, before] : hashes)
            check(before && fileSha256(file) == before, "original save file hash preserved");
    };
    std::string error;
    {
        SaveDb openWriter;
        check(openWriter.open(source / stateName, SaveDb::Mode::ReadWrite), "opened synthetic save writer");
        check(!owner::importSaves(original, target, error) && !fs::exists(target), "open SQLite writer refused");
    }
    const auto journal = source / (stateName.native() + L"-journal");
    std::ofstream(journal) << "unfinished source transaction";
    check(!owner::importSaves(original, target, error) && fs::exists(journal) && !fs::exists(target),
          "journal refused without recovery or partial installation");
    fs::remove(journal);
    check(!owner::importSaves(original, source / "nested", error) && !fs::exists(source / "nested"),
          "nested save destination refused");
    check(!owner::importSaves(original, original / "extra", error) && !fs::exists(original / "extra"),
          "another location inside the original installation refused");
    std::error_code linkError;
    const auto alias = scratch / "linked-game";
    fs::create_directory_symlink(original, alias, linkError);
    if (!linkError)
        check(!owner::importSaves(original, alias / "extra", error) && !fs::exists(original / "extra") &&
              !owner::importSaves(alias, target, error), "linked source or destination ancestor refused");

    const bool imported = owner::importSaves(original, target, error);
    if (!imported) throw std::runtime_error("Synthetic save import: " + error);
    assertSource();
    check(!fs::exists(target / "notes.txt") && !fs::exists(target / "old-copy.db") &&
          !fs::exists(target / "world_server_78.db") && !fs::exists(target / "world_server_78.db-wal"),
          "unrelated files, dedicated worlds and unrelated sidecars excluded");
    check(!fs::exists(target / "world_Map only.db"), "map-only catalog entry retains its original file set");
    for (const fs::path &file : {fs::path("characters.db"), fs::path("worlds.db"), mapName, fs::path("map_Map only.db")})
        check(hashes[source / file] == fileSha256(target / file), "list and map copied byte-for-byte");
    const auto water = generationProfile(true, {});
    for (const auto &file : {stateName, secondState}) {
        SaveDb before, after;
        check(before.open(source / file, SaveDb::Mode::ReadOnly) && after.open(target / file, SaveDb::Mode::ReadOnly) &&
              !before.get(GENERATION_PROFILE_KEY) && after.keys().size() == before.keys().size() + 1 &&
              after.get(GENERATION_PROFILE_KEY) == std::vector<uint8_t>(water.begin(), water.end()),
              "each world gains only the required unmodded generation profile");
        for (const auto &key : before.keys())
            check(before.get(key) == after.get(key), "native world blob preserved byte-for-byte");
    }
    {
        WorldSession session;
        check(session.open(client, name, 999, 0, &error, water) && session.seed() == first.seed &&
              session.clock.day == 7 && session.clock.timeOfDayMs == 8150 && session.discoveryCount() == 31 &&
              session.playing() && session.playing()->level == 12 && session.playing()->name == character.name &&
              session.playing()->money == 235 && session.startPosition() == glm::dvec3(1, 2, 3),
              "production Save location loads the imported seed, clock, discovery and selected character");
    }
    ModInfo mod;
    mod.folderName = "SyntheticMod";
    WorldSession mismatch;
    check(!mismatch.open(client, name, 17, 0, &error, generationProfile(true, {mod})),
          "imported world rejects terrain mods on normal open");
    check(!mismatch.open(original, name, 17, 0, &error, water), "unmarked original still rejects required water");
    check(!owner::importSaves(original, target, error) && error.find("already exists") != error.npos &&
          fs::is_regular_file(target / stateName), "existing destination preserved with actionable diagnostic");
    assertSource();

    const auto retry = client / "Retry";
    const auto rejected = [&](const char *label) {
        check(!owner::importSaves(original, retry, error) && !fs::exists(retry), label);
        for (const auto &entry : fs::directory_iterator(client))
            check(!entry.path().filename().native().starts_with(L".AlphaSetup-save-"), "temporary staging cleaned after failure");
    };
    const auto put = [&](const fs::path &file, const std::string &key, const std::vector<uint8_t> &value) {
        SaveDb db;
        if (!db.open(source / file, SaveDb::Mode::ReadWrite) || !db.put(key, value))
            throw std::runtime_error("Cannot update synthetic failure fixture");
    };
    WorldEntry unsafe = second;
    unsafe.name = "../outside";
    put("worlds.db", "1", unsafe.serialize());
    rejected("unsafe file names reject entire import");
    put("worlds.db", "1", second.serialize());
    put(secondState, GENERATION_PROFILE_KEY, {1, 2, 3});
    rejected("already marked world in multi-world folder rejects entire import");
    {
        SaveDb db;
        db.open(source / secondState, SaveDb::Mode::ReadWrite);
        db.remove(GENERATION_PROFILE_KEY);
    }
    put(secondState, "av_quests", {1});
    rejected("other engine extension records refuse adoption");
    {
        SaveDb db;
        db.open(source / secondState, SaveDb::Mode::ReadWrite);
        db.remove("av_quests");
    }
    put("characters.db", "0", {1, 2, 3});
    rejected("damaged compressed character refuses installation");
}
}

int wmain(int argc, wchar_t **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    try {
    const ScratchDirectory directory;
    const auto &scratch = directory.path;
    saveImportChecks(scratch);
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
