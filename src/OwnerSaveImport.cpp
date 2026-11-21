#include "OwnerContent.h"
#include "Bytes.h"
#include "GenerationProfile.h"
#include "Install.h"
#include "Saves.h"
#include "SqlitePath.h"
#include "ZlibStream.h"
#include <windows.h>
#include <sqlite3.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstring>
#include <memory>
#include <set>
#include <stdexcept>
#include <string_view>

namespace {
[[noreturn]] void fail(const std::string &message) { throw std::runtime_error(message); }

struct CloseHandleDeleter {
    void operator()(HANDLE h) const { CloseHandle(h); }
};
using Handle = std::unique_ptr<void, CloseHandleDeleter>;

[[noreturn]] void failWin32(const std::string &operation, const fs::path &path, DWORD code) {
    fail(operation + " " + utf8(path) + " (Win32 " + std::to_string(code) + ": " +
         std::system_category().message(static_cast<int>(code)) + ")");
}

Handle lockPath(const fs::path &path, bool directory) {
    // Deny deletion only on the directories and source databases used for the copy.
    // No write sharing on a source file: a SQLite writer already holding it also blocks this open.
    HANDLE h = CreateFileW(path.c_str(), directory ? FILE_READ_ATTRIBUTES : GENERIC_READ,
        directory ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | (directory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_FLAG_SEQUENTIAL_SCAN), nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const DWORD code = GetLastError();
        if (!directory && code == ERROR_SHARING_VIOLATION)
            fail("A selected save is open for writing. Close both games and any save editor, then retry.");
        failWin32("Cannot open", path, code);
    }
    Handle locked(h);
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(h, &info) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        bool(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != directory || GetFileType(h) != FILE_TYPE_DISK)
        fail("Save import requires ordinary local directories and files without links or junctions: " + utf8(path));
    return locked;
}

fs::path localPath(const fs::path &input) {
    auto path = fs::absolute(input).lexically_normal();
    if (path != path.root_path() && path.filename().empty()) path = path.parent_path();
    const auto drive = path.root_name().native();
    if (drive.size() != 2 || drive[1] != L':' || !path.has_root_directory())
        fail("Choose a local drive path for the save import.");
    for (const auto &part : path.relative_path()) {
        const auto &s = part.native();
        if (s.find_first_of(L"<>:\"|?*") != s.npos || (!s.empty() && (s.back() == L' ' || s.back() == L'.')))
            fail("Invalid local directory name for save import.");
    }
    return path;
}

Handle pinDirectory(const fs::path &directory) {
    // A user can traverse a profile ancestor without permission to open a handle on it.
    // Read its attributes to reject junctions. Only the operational leaf needs a handle.
    auto at = directory.root_path();
    for (const auto &part : directory.relative_path()) {
        if (!part.empty()) {
            at /= part;
            const DWORD attributes = GetFileAttributesW(at.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES)
                failWin32("Cannot inspect directory", at, GetLastError());
            if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
                fail("Save import requires directories without links or junctions: " + utf8(at));
        }
    }
    return lockPath(directory, true);
}

fs::path finalPath(HANDLE handle, DWORD volume = VOLUME_NAME_DOS) {
    const DWORD needed = GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_NORMALIZED | volume);
    if (!needed) fail("Cannot resolve a selected save path (Win32 " + std::to_string(GetLastError()) + ").");
    std::wstring result(needed, L'\0');
    const DWORD size = GetFinalPathNameByHandleW(handle, result.data(), needed, FILE_NAME_NORMALIZED | volume);
    if (!size || size >= needed) fail("Cannot resolve a stable local save path.");
    result.resize(size);
    if (volume == VOLUME_NAME_GUID) return fs::path(result);
    if (!result.starts_with(L"\\\\?\\") || result.size() < 7 || result[5] != L':' || result[6] != L'\\')
        fail("Save import requires an ordinary local drive path.");
    return fs::path(result.substr(4));
}

bool samePath(const fs::path &a, const fs::path &b) {
    return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_EQUAL;
}

struct NameOrder {
    bool operator()(const fs::path &a, const fs::path &b) const {
        return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_LESS_THAN;
    }
};
using Names = std::set<fs::path, NameOrder>;

enum class Kind { Characters, Worlds, State, Map };

bool present(const fs::path &file) {
    return fs::symlink_status(file).type() != fs::file_type::not_found;
}

void checkJournals(const fs::path &file) {
    for (const wchar_t *suffix : {L"-journal", L"-wal", L"-shm"})
        if (present(file.native() + suffix))
            fail("A selected database has SQLite journal files. Close Alpha cleanly before importing.");
}

class Stage {
  public:
    explicit Stage(const fs::path &parent) : path(parent / (L".AlphaSetup-save-" +
        std::to_wstring(GetCurrentProcessId()) + L"-" +
        std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        if (!fs::create_directory(path))
            fail("Cannot create a new temporary save directory.");
        try {
            directory = lockPath(path, true);
            if (!samePath(finalPath(directory.get()), path))
                fail("The staging directory changed during save import.");
        } catch (...) {
            std::error_code ignored;
            fs::remove(path, ignored);
            throw;
        }
    }
    Stage(const Stage &) = delete;
    ~Stage() {
        if (published) return;
        std::error_code ignored;
        for (const auto &file : files)
            fs::remove(file, ignored);
        directory.reset();
        fs::remove(path, ignored);
    }
    void copy(const fs::path &name, HANDLE input) {
        files.push_back(path / name);
        HANDLE h = CreateFileW(files.back().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            files.pop_back();
            fail("Cannot create the staged save file.");
        }
        Handle output(h);
        std::array<uint8_t, 65536> buffer;
        bool first = true;
        for (;;) {
            DWORD n = 0, written = 0;
            if (!ReadFile(input, buffer.data(), DWORD(buffer.size()), &n, nullptr))
                fail("Cannot read the closed source database.");
            if (first && (n < 100 || std::memcmp(buffer.data(), "SQLite format 3\0", 16) ||
                          buffer[18] != 1 || buffer[19] != 1))
                fail("Save import requires a closed SQLite rollback-mode database: " + utf8(name));
            first = false;
            if (!n) break;
            if (!WriteFile(h, buffer.data(), n, &written, nullptr) || written != n)
                fail("Cannot write the staged database. Check free disk space.");
        }
        if (!FlushFileBuffers(h)) fail("Cannot finish the staged database.");
    }
    void publish(const fs::path &target) {
        directory.reset();
        if (!MoveFileExW(path.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH))
            fail("Cannot publish the save copy. Choose a new destination with an existing parent directory.");
        published = true;
    }
    const fs::path path;
  private:
    Handle directory;
    std::vector<fs::path> files;
    bool published = false;
};

using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
Statement query(sqlite3 *db, const char *sql) {
    sqlite3_stmt *q = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &q, nullptr) != SQLITE_OK)
        fail("Cannot read the expected Alpha save schema.");
    return Statement(q, sqlite3_finalize);
}

bool integerKey(std::string_view s, int32_t &number) {
    if (s.empty()) return false;
    const auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), number);
    return error == std::errc{} && end == s.data() + s.size() && s == std::to_string(number);
}

bool coordinateKey(std::string_view key, std::string_view prefix) {
    if (!key.starts_with(prefix)) return false;
    key.remove_prefix(prefix.size());
    const auto split = key.find('_');
    int32_t x, y;
    return split != key.npos && integerKey(key.substr(0, split), x) && integerKey(key.substr(split + 1), y);
}

// Require a complete entry before its name is used in a path. The gameplay reader
// deliberately tolerates truncated fields and pads a partial preview for display.
std::string worldName(const std::vector<uint8_t> &bytes) {
    ByteReader r(bytes);
    auto name = r.string();
    r.u32(); // seed
    r.u32(); // discovered
    const int32_t x = r.i32(), y = r.i32(), z = r.i32();
    if (x > 0 && y > 0 && z > 0) {
        constexpr uint64_t maxVoxels = 1u << 24; // same maximum as the menu
        const uint64_t xy = uint64_t(x) * y;
        const auto packed = r.u32();
        if (xy > maxVoxels / z || packed > r.left()) fail("Invalid world preview size.");
        const auto pixels = zlibInflate(r.bytes(packed));
        if (!pixels || pixels->size() != xy * z * 3) fail("Incomplete world preview.");
    }
    if (!r.atEnd() || name.empty() || name.back() == '.' || name.back() == ' ' ||
        name.find_first_of("<>:\"/\\|?*") != name.npos ||
        std::any_of(name.begin(), name.end(), [](unsigned char c) { return c < 32; }))
        fail("The world list contains an incomplete entry or unsafe world name.");
    return name;
}

bool validStateRecord(Kind kind, const std::string &key, const std::vector<uint8_t> &value) {
    if (kind == Kind::State && key == "time") return WorldClock::parse(value).has_value();
    if (kind == Kind::Map && key == "discovered") return value.size() == 4;
    const bool versionOne = value.size() >= 4 && ByteReader(value).u32() == 1;
    if (kind == Kind::State) {
        if (coordinateKey(key, "zone")) return ZoneSave::parse(value).has_value();
        if (coordinateKey(key, "mission")) return versionOne && value.size() == 38;
        if (coordinateKey(key, "monster")) return versionOne && value.size() == 21;
    } else {
        if (coordinateKey(key, "reg")) return versionOne && value.size() == 60164;
        if (coordinateKey(key, "land")) return versionOne && LandSave::parse(value).has_value();
        if (coordinateKey(key, "tile")) return versionOne && TileSave::parse(value).has_value();
    }
    return true; // Keep additional opaque metadata exactly as supplied.
}

void validate(const fs::path &file, Kind kind, std::vector<std::string> &worlds) {
    sqlite3 *raw = nullptr;
    const int opened = sqlite3_open_v2(sqliteReadOnlyUri(file).c_str(), &raw, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr);
    std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db(raw, sqlite3_close);
    if (opened != SQLITE_OK) fail("Cannot open the staged SQLite database.");
    sqlite3_limit(raw, SQLITE_LIMIT_LENGTH, 64 << 20);
    sqlite3_db_config(raw, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, nullptr);
    auto schema = query(raw,
        "SELECT 1 WHERE (SELECT count(*) FROM sqlite_schema)=2 "
        "AND EXISTS(SELECT 1 FROM sqlite_schema WHERE type='table' AND name='blobs') "
        "AND EXISTS(SELECT 1 FROM sqlite_schema WHERE type='index' AND name='sqlite_autoindex_blobs_1' AND sql IS NULL) "
        "AND (SELECT count(*) FROM pragma_table_xinfo('blobs'))=2 "
        "AND EXISTS(SELECT 1 FROM pragma_table_xinfo('blobs') WHERE name='key' AND upper(type)='TEXT' AND pk=1 AND hidden=0) "
        "AND EXISTS(SELECT 1 FROM pragma_table_xinfo('blobs') WHERE name='value' AND upper(type)='BLOB' AND pk=0 AND hidden=0)");
    if (sqlite3_step(schema.get()) != SQLITE_ROW) fail("Unsupported save schema or extra SQLite objects.");
    auto integrity = query(raw, "PRAGMA quick_check(1)");
    if (sqlite3_step(integrity.get()) != SQLITE_ROW || !sqlite3_column_text(integrity.get(), 0) ||
        std::strcmp(reinterpret_cast<const char *>(sqlite3_column_text(integrity.get(), 0)), "ok"))
        fail("SQLite integrity check failed.");

    const bool list = kind == Kind::Worlds || kind == Kind::Characters;
    uint32_t count = 0;
    if (list) {
        auto num = query(raw, "SELECT value FROM blobs WHERE key='num'");
        int rc = sqlite3_step(num.get());
        if (rc == SQLITE_ROW && sqlite3_column_bytes(num.get(), 0) == sizeof(count))
            std::memcpy(&count, sqlite3_column_blob(num.get(), 0), sizeof(count));
        else if (rc != SQLITE_DONE)
            fail("Invalid save list count.");
        if (count > (1u << 16)) fail("Save list exceeds the engine's entry limit.");
    }
    uint32_t active = 0;
    auto records = query(raw, "SELECT key, value FROM blobs");
    int rc;
    while ((rc = sqlite3_step(records.get())) == SQLITE_ROW) {
        if (sqlite3_column_type(records.get(), 0) != SQLITE_TEXT || sqlite3_column_type(records.get(), 1) != SQLITE_BLOB)
            fail("Expected a native text key and blob.");
        const std::string key(reinterpret_cast<const char *>(sqlite3_column_text(records.get(), 0)),
                              sqlite3_column_bytes(records.get(), 0));
        if (key.starts_with("av_")) fail("This save already contains AlphaVulkan extension records.");
        const auto *data = static_cast<const uint8_t *>(sqlite3_column_blob(records.get(), 1));
        const auto size = sqlite3_column_bytes(records.get(), 1);
        std::vector<uint8_t> value;
        if (size) value.assign(data, data + size);
        if (!list) {
            if (!validStateRecord(kind, key, value)) fail("Incomplete world or map record.");
            continue;
        }
        if (key == "num") continue;
        int32_t index;
        if (!integerKey(key, index) || index < 0 || uint32_t(index) >= count)
            continue; // Extra metadata and inactive native deletion rows never supply filenames.
        if (kind == Kind::Worlds) {
            worlds.push_back(worldName(value));
        } else {
            const auto plain = zlibInflate(value);
            const auto character = plain ? CharacterRecord::parse(*plain) : std::nullopt;
            if (!character || character->serialize() != *plain)
                fail("Invalid or incomplete character record.");
        }
        ++active;
    }
    if (rc != SQLITE_DONE || (list && active != count)) fail("Incomplete save database or missing list entries.");
}
} // namespace

bool owner::importSaves(const fs::path &installation, const fs::path &destination, std::string &error) {
    try {
        const auto requested = localPath(destination);
        if (requested.filename().empty()) fail("Choose a new Save directory by name.");
        const auto sourceDirectory = pinDirectory(localPath(installation) / "Save");
        const auto parentDirectory = pinDirectory(requested.parent_path());
        const auto source = finalPath(sourceDirectory.get());
        const auto parent = finalPath(parentDirectory.get());
        const auto target = parent / requested.filename();
        // Alternate drive letters can name a single volume. Compare its physical identity.
        const auto game = finalPath(sourceDirectory.get(), VOLUME_NAME_GUID).parent_path();
        for (auto p = finalPath(parentDirectory.get(), VOLUME_NAME_GUID);; p = p.parent_path()) {
            if (samePath(p, game)) fail("The destination must be outside the owned installation.");
            if (p == p.root_path()) break;
        }
        if (present(target))
            fail("The destination already exists. Import into a new Save directory beside a fresh copy of AlphaVulkan.exe.");

        Stage stage(target.parent_path());
        std::vector<Handle> databases;
        std::vector<std::string> worlds;
        const auto copy = [&](const fs::path &file, Kind kind) {
            databases.push_back(lockPath(file, false));
            if (!samePath(finalPath(databases.back().get()).parent_path(), source))
                fail("The original save database resolved outside the selected Save directory.");
            checkJournals(file); // The read handle now excludes all source writers.
            const auto name = file.filename();
            stage.copy(name, databases.back().get());
            try {
                validate(stage.path / name, kind, worlds);
            } catch (const std::exception &e) {
                fail(utf8(name) + ": " + e.what());
            }
        };
        copy(source / "characters.db", Kind::Characters);
        copy(source / "worlds.db", Kind::Worlds);
        Names planned, states, absent;
        for (const auto &world : worlds) {
            for (const auto type : {WorldSaveFile::State, WorldSaveFile::Map}) {
                const auto file = worldSavePath(source, world, type);
                if (!planned.insert(file).second) continue;
                if (!present(file)) {
                    checkJournals(file);
                    absent.insert(file);
                    continue; // A map-only or never-entered catalog entry has no local world to pin.
                }
                copy(file, type == WorldSaveFile::State ? Kind::State : Kind::Map);
                if (type == WorldSaveFile::State) states.insert(file.filename());
            }
        }
        for (const auto &file : absent)
            if (present(file)) fail("The original Save changed during import. Close Alpha and retry.");
        const auto profile = generationProfile(true, {});
        const std::vector<uint8_t> marker(profile.begin(), profile.end());
        for (const auto &name : states) {
            SaveDb db;
            if (!db.open(stage.path / name, SaveDb::Mode::ReadWrite) ||
                !db.putIfAbsent(GENERATION_PROFILE_KEY, marker) || db.get(GENERATION_PROFILE_KEY) != marker)
                fail("Cannot record the unmodded water profile on the staged world.");
        }
        stage.publish(target);
        error.clear();
        return true;
    } catch (const std::exception &e) {
        error = e.what();
        return false;
    }
}
