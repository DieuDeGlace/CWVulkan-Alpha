#pragma once
// A single-player world kept in Alpha's save files (docs/specs/alpha-saves.md): its entry in
// worlds.db, its clock in world_<name>.db, and the character who plays in it from
// characters.db. The files live in AlphaVulkan's own Save folder, never in the game's install.
#include "Saves.h"
#include <glm/glm.hpp>
#include <map>

class WorldSession {
  public:
    // Opens world `name` in `folder`/Save, adding it to the world list with `seed` when it is
    // new (§4.1); an existing world keeps its own seed. `character` is an index into
    // characters.db, or -1 to play without one.
    bool open(const std::filesystem::path &folder, const std::string &name, uint32_t seed, int character,
              std::string *error = nullptr, const std::string &generationProfile = {});

    uint32_t seed() const { return entry.seed; }
    // The character playing, if one was chosen.
    const std::optional<CharacterRecord> &playing() const { return character; }
    std::optional<CharacterRecord> &playing() { return character; } // written back on leaving
    const std::string &name() const { return entry.name; }
    // Where the character last stood in this world, in blocks (none for a new character).
    std::optional<glm::dvec3> startPosition() const;

    // §5.1: the world clock, run as the server runs it (ten game milliseconds per real one).
    WorldClock clock;
    void advance(double seconds);

    // Screens §5.1.6: writes a zone's map tile into map_<name>.db, as Alpha's map worker does.
    void saveTile(int32_t zx, int32_t zy, const TileSave &tile);
    void saveLand(int32_t lx, int32_t ly, const LandSave &land);
    // Screens §5.1.6: region records (zone flags and copies) and saved tiles read back.
    std::optional<RegionSave> loadRegion(int32_t rx, int32_t ry);
    void saveRegion(int32_t rx, int32_t ry, const RegionSave &region);
    std::optional<TileSave> loadTile(int32_t zx, int32_t zy);

    // §5.2: a zone's stored changes, and writing back its world items (the rest of the record
    // is kept as it was).
    std::optional<ZoneSave> loadZone(int32_t zx, int32_t zy);
    void saveZoneItems(int32_t zx, int32_t zy, std::vector<std::array<uint8_t, 325>> items);
    void saveZoneStates(int32_t zx, int32_t zy, std::vector<uint8_t> states); // one byte per static
    void saveZoneEdits(int32_t zx, int32_t zy, std::vector<std::array<uint8_t, 20>> edits); // block edits §12

    // Plain values of world_<name>.db by key, read or written together (missions §7: the
    // `mission<cx>_<cy>` and `monster<cx>_<cy>` values of a region's cells).
    std::map<std::string, std::vector<uint8_t>> values(const std::vector<std::string> &keys);
    void setValues(const std::map<std::string, std::vector<uint8_t>> &values);

    // Writes the clock (Alpha does so from its frame update, the engine every few seconds).
    void saveClock();
    // On leaving the world (§3.1, §4.1): the clock, the character where it stands, and the
    // world entry.
    void leave(const glm::dvec3 &position);
    // Saves §4.1: the world list's preview, the map-tile model of the zone the player is in
    // (written with the entry on leaving).
    void setPreview(int32_t sx, int32_t sy, int32_t sz, std::vector<uint8_t> rgb);

  private:
    std::filesystem::path saveFolder;
    SaveDb mapDb; // map_<name>.db, opened on the first tile
    bool mapOpen = false;
    bool openMap(); // opens map_<name>.db on first use
    WorldEntry entry;
    int entryIndex = -1;
    int characterIndex = -1;
    std::optional<CharacterRecord> character;
    double pendingMs = 0;
};
