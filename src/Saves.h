#pragma once
// Alpha's save files (docs/specs/alpha-saves.md): SQLite files with one blobs(key, value) table,
// holding the character list, the world list, each world's state and each world's map. Records
// are read into plain structs and written back the same way; fields whose meaning is not known
// yet are kept byte for byte, so a save that AlphaVulkan rewrites still loads in Alpha.
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace worldgen {
struct Region;
}

struct sqlite3;

// §2: one save database.
class SaveDb {
  public:
    enum class Mode {
        ReadOnly, // never writes, not even a journal (for the owner's own saves in tests)
        ReadWrite // creates the file and its table if missing
    };
    SaveDb() = default;
    SaveDb(const SaveDb &) = delete;
    SaveDb &operator=(const SaveDb &) = delete;
    ~SaveDb();

    bool open(const std::filesystem::path &file, Mode mode, std::string *error = nullptr);
    std::optional<std::vector<uint8_t>> get(const std::string &key) const;
    bool put(const std::string &key, const std::vector<uint8_t> &value);
    // Atomic insert that preserves an existing value; true also means already present.
    bool putIfAbsent(const std::string &key, const std::vector<uint8_t> &value);
    bool remove(const std::string &key);
    bool rename(const std::string &from, const std::string &to);
    // Every key starting with `prefix`, in key order.
    std::vector<std::string> keys(const std::string &prefix = {}) const;
    // A bare u32 value (the `num` and `discovered` keys).
    std::optional<uint32_t> number(const std::string &key) const;
    bool putNumber(const std::string &key, uint32_t value);

  private:
    sqlite3 *db = nullptr;
};

// alpha-net-packets.md §1.1: an item record, 280 bytes. Kept whole until its fields are used.
using ItemBytes = std::array<uint8_t, 280>;

// §3.2: one character.
struct CharacterRecord {
    int64_t position[3] = {};
    float orientation[3] = {};
    float health = 0;
    int32_t experience = 0, level = 1;
    uint8_t characterClass = 0, specialization = 0;
    uint32_t unknown9 = 0, unknown10 = 0;
    std::array<ItemBytes, 13> equipment = {};
    std::string name;
    std::array<uint8_t, 20> extra = {}; // field 13: keep as is (open question V-2)
    struct Slot {
        uint32_t count = 0;
        ItemBytes item = {};
    };
    std::vector<std::vector<Slot>> tabs; // inventory tabs, each a list of slots
    uint32_t money = 0, platinum = 0; // fields 16, 17 (screens §8.1.1): copper and platinum coins
    std::vector<ItemBytes> recipes; // field 18: the formulas learnt
    struct WorldVisit {
        uint32_t seed = 0;
        std::string name;
        int64_t position[3] = {};
    };
    std::vector<WorldVisit> visits;
    uint32_t lastWorldSeed = 0;
    std::string lastWorldName;
    uint32_t manaCubes = 0; // field 22: mana cubes found (skills §6), a skill point per four
    std::vector<int32_t> skills; // 11 levels

    static std::optional<CharacterRecord> parse(const std::vector<uint8_t> &inflated);
    std::vector<uint8_t> serialize() const;
};

// §3: the character list.
std::vector<CharacterRecord> readCharacters(const SaveDb &db);
bool writeCharacters(SaveDb &db, const std::vector<CharacterRecord> &characters);

// §4.2: one entry of the world list.
struct WorldEntry {
    std::string name;
    uint32_t seed = 0;
    uint32_t discovered = 0;
    int32_t previewSize[3] = {};
    std::vector<uint8_t> previewRgb; // X*Y*Z*3 bytes, inflated; empty when there is no preview

    static std::optional<WorldEntry> parse(const std::vector<uint8_t> &value);
    std::vector<uint8_t> serialize() const;
};
std::vector<WorldEntry> readWorlds(const SaveDb &db);

// §5.1: a world's clock.
struct WorldClock {
    uint32_t day = 0;
    uint32_t timeOfDayMs = 43200000; // a new world starts at noon
    static std::optional<WorldClock> parse(const std::vector<uint8_t> &value);
    std::vector<uint8_t> serialize() const;
};

// §5.2: the stored changes of one zone.
struct ZoneSave {
    std::vector<std::array<uint8_t, 325>> items; // world items
    std::vector<std::array<uint8_t, 20>> blockEdits;
    std::vector<std::array<uint32_t, 2>> objects; // two words per dynamic object (V-3)
    std::vector<uint8_t> staticStates;            // one byte per static entity
    static std::optional<ZoneSave> parse(const std::vector<uint8_t> &value);
    std::vector<uint8_t> serialize() const;
};

// §6.4 with the corrections of screens §5.1.6: a zone's map tile in map_<name>.db.
struct TileSave {
    int32_t base = 0;              // B, map units of 8 blocks
    int32_t sx = 0, sy = 0, sz = 0;
    std::vector<uint8_t> rgb;      // sx * sy * sz voxels, 3 bytes each
    std::vector<std::array<int32_t, 3>> markers; // land-border points, blocks
    static std::optional<TileSave> parse(const std::vector<uint8_t> &value);
    std::vector<uint8_t> serialize() const;
};
std::string tileKey(int32_t zx, int32_t zy);

// §6.2 with the corrections of screens §5.1.6: a land's picture in map_<name>.db.
struct LandSave {
    int32_t originX = 0, originY = 0; // zones
    bool visited = false;
    int32_t sx = 0, sy = 0, sz = 0;
    std::vector<uint8_t> rgb;
    std::array<uint8_t, 28> cell{}; // the region cell: point, fire, moisture, warmth, id, elevation
    static std::optional<LandSave> parse(const std::vector<uint8_t> &value);
    std::vector<uint8_t> serialize() const;
};
std::string landKey(int32_t lx, int32_t ly);

// §6.3 with screens §5.1.6: a region's zone flags and copies in map_<name>.db.
struct RegionSave {
    struct ZoneCell {
        uint8_t flags = 0;               // bit 0 discovered, bit 1 has a saved tile
        std::array<uint8_t, 4> info{};   // the zone's table entry: kind, variant, ...
        uint32_t id = 0, level = 0;
        uint8_t subLevel = 0;
    };
    std::array<ZoneCell, 64 * 64> zones{}; // row order: index 64 * x + y (V-4)
    // The 8 x 8 location copies: type, variant, serial, level, sub-level (u32 each), ground
    // height (f32), centre x and y (i64, 1/65536 blocks), radius (f32).
    std::array<std::array<uint8_t, 44>, 64> locations{};
    RegionSave();
    // Screens §5.1.6: copy the world region's zone table and location records in, keeping the
    // zone flags.
    void refresh(const worldgen::Region &region);
    static std::optional<RegionSave> parse(const std::vector<uint8_t> &value);
    std::vector<uint8_t> serialize() const;
};
std::string regionKey(int32_t rx, int32_t ry);

// Save keys (§0, §5): zone<x>_<y> and so on.
std::string zoneKey(int32_t zx, int32_t zy);
