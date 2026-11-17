#pragma once
// Alpha's content stores, data1.db .. data4.db (docs/specs/alpha-extractor.md Part A): SQLite 3
// files with one `blobs(key TEXT PRIMARY KEY, value BLOB)` table whose values are scrambled.
// data1 holds the .cub models, data2 the .wav sounds, data3 the .png images and data4 the
// dictionaries. Opened read-only; nothing is ever created in the owner's installation.
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

// Undo (or apply) the stores' byte scrambling in place (§A4).
void unscramble(std::vector<uint8_t> &bytes);
void scramble(std::vector<uint8_t> &bytes);

// The 16 bytes every SQLite 3 database begins with: "SQLite format 3" and a zero byte.
inline constexpr char SQLITE_HEADER[16] = "SQLite format 3";
// Whether `file` exists and begins with SQLITE_HEADER.
bool hasSqliteHeader(const std::filesystem::path &file);

class DataStore {
  public:
    DataStore() = default;
    DataStore(const DataStore &) = delete;
    DataStore &operator=(const DataStore &) = delete;
    ~DataStore();

    // Opens an existing SQLite 3 store with a readable blob table; false for a missing file,
    // another kind of file or another schema (never creating anything).
    bool open(const std::filesystem::path &file, std::string *error = nullptr);
    // True when the store holds no record (or is not open).
    bool empty() const;
    // Every key in the store, in key order.
    std::vector<std::string> keys() const;
    // The stored bytes of `key` exactly as kept (still scrambled).
    std::optional<std::vector<uint8_t>> raw(const std::string &key) const;
    // The plain file bytes of `key`.
    std::optional<std::vector<uint8_t>> read(const std::string &key) const;

  private:
    sqlite3 *db = nullptr;
};

// All four stores of an install, looked up by key (keys do not repeat across stores), with
// mods' content folders over them: a plain file named like a key (e.g. `airship.cub`) in an
// override folder replaces that key, and new names add keys. Later folders win (docs/modding.md).
class Content {
  public:
    // Opens data1.db .. data4.db of `gameDir`; false when any is missing, invalid or empty.
    bool open(const std::filesystem::path &gameDir, std::string *error = nullptr);
    // Adds a folder of override files on top of everything added before it.
    using OverrideId = uint64_t;
    OverrideId addOverride(const std::filesystem::path &folder);
    // Remove only this registration, preserving other loaders' layers. Change
    // registrations before/after content workers, never during their reads.
    bool removeOverride(OverrideId id);
    std::optional<std::vector<uint8_t>> read(const std::string &key) const;
    // Keys from every store and override ending in `extension` (e.g. ".cub"), each once,
    // sorted.
    std::vector<std::string> list(const std::string &extension) const;
    const DataStore &store(int index) const { return stores[index]; }
    static constexpr int COUNT = 4;

  private:
    DataStore stores[COUNT];
    struct Override { OverrideId id; std::filesystem::path folder; };
    std::vector<Override> overrides;
    OverrideId nextOverride = 1;
};
