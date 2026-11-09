#pragma once
// Alpha's content stores, data1.db .. data4.db (docs/specs/alpha-data-stores.md): SQLite files
// with one `blobs(key, value)` table whose values are scrambled. Opened read-only.
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

// Undo (or apply) the stores' byte scrambling in place (spec §2).
void unscramble(std::vector<uint8_t> &bytes);
void scramble(std::vector<uint8_t> &bytes);

class DataStore {
  public:
    DataStore() = default;
    DataStore(const DataStore &) = delete;
    DataStore &operator=(const DataStore &) = delete;
    ~DataStore();

    bool open(const std::filesystem::path &file, std::string *error = nullptr);
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
    bool open(const std::filesystem::path &gameDir, std::string *error = nullptr);
    // Adds a folder of override files on top of everything added before it.
    void addOverride(const std::filesystem::path &folder);
    std::optional<std::vector<uint8_t>> read(const std::string &key) const;
    // Keys from every store and override ending in `extension` (e.g. ".cub"), each once,
    // sorted.
    std::vector<std::string> list(const std::string &extension) const;
    const DataStore &store(int index) const { return stores[index]; }
    static constexpr int COUNT = 4;

  private:
    DataStore stores[COUNT];
    std::vector<std::filesystem::path> overrides;
};
