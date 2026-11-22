#include "WorldSession.h"
#include "BlockEdits.h"
#include "StaticStates.h"
#include "WorldItems.h"
#include "GenerationProfile.h"
#include "AlphaLook.h"

namespace fs = std::filesystem;

namespace {

constexpr double FIXED = 65536.0; // positions are kept in 1/65536 blocks

} // namespace

bool WorldSession::openMap() {
    if (!mapOpen)
        mapOpen = mapDb.open(worldSavePath(saveFolder, entry.name, WorldSaveFile::Map), SaveDb::Mode::ReadWrite);
    return mapOpen;
}

void WorldSession::saveTile(int32_t zx, int32_t zy, const TileSave &tile) {
    if (openMap())
        mapDb.put(tileKey(zx, zy), tile.serialize());
}

void WorldSession::saveLand(int32_t lx, int32_t ly, const LandSave &land) {
    if (openMap())
        mapDb.put(landKey(lx, ly), land.serialize());
}

std::optional<RegionSave> WorldSession::loadRegion(int32_t rx, int32_t ry) {
    if (!openMap())
        return std::nullopt;
    const auto value = mapDb.get(regionKey(rx, ry));
    return value ? RegionSave::parse(*value) : std::nullopt;
}

void WorldSession::saveRegion(int32_t rx, int32_t ry, const RegionSave &region) {
    if (openMap())
        mapDb.put(regionKey(rx, ry), region.serialize());
}

std::optional<TileSave> WorldSession::loadTile(int32_t zx, int32_t zy) {
    if (!openMap())
        return std::nullopt;
    const auto value = mapDb.get(tileKey(zx, zy));
    return value ? TileSave::parse(*value) : std::nullopt;
}

uint32_t WorldSession::discoveryCount() {
    return openMap() ? mapDb.number("discovered").value_or(0) : 0;
}

void WorldSession::saveDiscoveryCount(uint32_t count) {
    if (openMap())
        mapDb.putNumber("discovered", count);
}

bool WorldSession::selectEntry(const std::string &name, uint32_t seed, bool matchSeed, std::string *error) {
    SaveDb worlds;
    if (!worlds.open(saveFolder / "worlds.db", SaveDb::Mode::ReadWrite, error))
        return false;
    const std::vector<WorldEntry> list = readWorlds(worlds);
    entryIndex = -1;
    for (size_t i = 0; i < list.size(); i++)
        if (list[i].name == name && (!matchSeed || list[i].seed == seed)) {
            entry = list[i];
            entryIndex = (int)i;
        }
    if (entryIndex < 0) {
        entry = WorldEntry{};
        entry.name = name;
        entry.seed = seed;
        entryIndex = (int)list.size();
        if (!worlds.put(std::to_string(entryIndex), entry.serialize()) ||
            !worlds.putNumber("num", (uint32_t)list.size() + 1)) {
            if (error)
                *error = "cannot add the world to worlds.db";
            return false;
        }
    }
    return true;
}

bool WorldSession::openRemoteMap(const fs::path &folder, const std::string &name, uint32_t seed, std::string *error) {
    mapDb.close();
    mapOpen = false;
    saveFolder = folder / "Save";
    std::error_code ec;
    fs::create_directories(saveFolder, ec);
    if (!selectEntry(name, seed, true, error))
        return false;
    mapOpen = mapDb.open(worldSavePath(saveFolder, entry.name, WorldSaveFile::Map), SaveDb::Mode::ReadWrite, error);
    return mapOpen;
}

bool WorldSession::open(const fs::path &folder, const std::string &name, uint32_t seed, int characterNumber,
                        std::string *error, const std::string &generationProfile) {
    mapDb.close();
    mapOpen = false;
    saveFolder = folder / "Save";
    std::error_code ec;
    fs::create_directories(saveFolder, ec);
    if (!ensureGenerationProfile(worldSavePath(saveFolder, name), generationProfile, error))
        return false;
    // Menus pass the selected entry's seed. Keep the existing name lookup for
    // the offline command-line path as well.
    if (!selectEntry(name, seed, false, error))
        return false;

    // §5.1: the clock; a new world starts at noon of day 0. (Alpha's setup keeps whatever clock
    // its one world object holds when `time` is absent; each session here is a fresh world.)
    clock = WorldClock{};
    SaveDb world;
    if (world.open(worldSavePath(saveFolder, name), SaveDb::Mode::ReadWrite, error))
        if (const auto value = world.get("time"))
            if (const auto stored = WorldClock::parse(*value))
                clock = *stored;

    // §3: the character.
    characterIndex = -1;
    character.reset();
    if (characterNumber >= 0) {
        SaveDb characters;
        if (!characters.open(saveFolder / "characters.db", SaveDb::Mode::ReadWrite, error))
            return false;
        character = loadCharacter(characters, uint32_t(characterNumber));
        if (!character) {
            if (error)
                *error = "there is no character " + std::to_string(characterNumber) + " in characters.db";
            return false;
        }
        characterIndex = characterNumber;
    }
    return true;
}

std::optional<std::array<int64_t, 3>> WorldSession::startFixedPosition() const {
    // Saves §3.1: the per-world table (field 19) determines where to enter this world.
    return character ? character->visitPosition(entry.seed, entry.name) : std::nullopt;
}

std::optional<glm::dvec3> WorldSession::startPosition() const {
    if (const auto pos = startFixedPosition())
        return glm::dvec3((*pos)[0] / FIXED, (*pos)[1] / FIXED, (*pos)[2] / FIXED);
    return std::nullopt;
}

void WorldSession::advance(double seconds) {
    pendingMs += seconds * 1000.0 * 10.0;
    const uint64_t whole = (uint64_t)pendingMs;
    pendingMs -= (double)whole;
    uint64_t ms = clock.timeOfDayMs + whole;
    while (ms > (uint64_t)alpha::DAY_MS) { // saves §5.1: only above 86,400,000 does the day roll
        ms -= (uint64_t)alpha::DAY_MS;
        clock.day++;
    }
    clock.timeOfDayMs = (uint32_t)ms;
}

std::optional<ZoneSave> WorldSession::loadZone(int32_t zx, int32_t zy) {
    SaveDb world;
    if (!world.open(worldSavePath(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return std::nullopt;
    const auto bytes = world.get(zoneKey(zx, zy));
    return bytes ? ZoneSave::parse(*bytes) : std::nullopt;
}

bool WorldSession::saveResidentZone(int32_t zx, int32_t zy, const WorldItems &items, const StaticStates &statics,
                                    const BlockEditLists &edits) {
    const bool modified = items.modified(zx, zy);
    const auto *history = edits.zone(zx, zy);
    if (!modified && (!history || history->empty()))
        return true;
    const auto *list = items.zone(zx, zy);
    if (!list)
        return false;
    SaveDb world;
    if (!world.open(worldSavePath(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return false;
    const auto key = zoneKey(zx, zy);
    ZoneSave zone;
    if (const auto old = world.get(key)) {
        auto saved = ZoneSave::parse(*old);
        if (!saved)
            return false;
        zone.objects = std::move(saved->objects);
        zone.staticStates = std::move(saved->staticStates);
    }
    if (modified) {
        zone.items.reserve(list->size());
        for (const WorldItem &item : *list)
            zone.items.push_back(item.save());
    }
    zone.blockEdits = edits.saved(zx, zy);
    if (statics.zone(zx, zy))
        zone.staticStates = statics.stateBytes(zx, zy);
    return world.put(key, zone.serialize());
}

std::map<std::string, std::vector<uint8_t>> WorldSession::values(const std::vector<std::string> &keys) {
    std::map<std::string, std::vector<uint8_t>> out;
    SaveDb world;
    if (!world.open(worldSavePath(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return out;
    for (const std::string &key : keys)
        if (auto bytes = world.get(key))
            out[key] = std::move(*bytes);
    return out;
}

void WorldSession::setValues(const std::map<std::string, std::vector<uint8_t>> &values) {
    SaveDb world;
    if (!world.open(worldSavePath(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return;
    for (const auto &[key, bytes] : values)
        world.put(key, bytes);
}

void WorldSession::saveClock() {
    SaveDb world;
    if (world.open(worldSavePath(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        world.put("time", clock.serialize());
}

void WorldSession::setPreview(int32_t sx, int32_t sy, int32_t sz, std::vector<uint8_t> rgb) {
    entry.previewSize[0] = sx, entry.previewSize[1] = sy, entry.previewSize[2] = sz;
    entry.previewRgb = std::move(rgb);
}

void WorldSession::leave(const glm::dvec3 &position) {
    std::array<int64_t, 3> fixed{};
    if (character) {
        for (int axis = 0; axis < 3; ++axis)
            fixed[axis] = int64_t(position[axis] * FIXED);
    }
    leaveAtFixedPosition(fixed);
}

void WorldSession::leaveAtFixedPosition(const std::array<int64_t, 3> &position) {
    saveClock();
    SaveDb worlds;
    if (worlds.open(saveFolder / "worlds.db", SaveDb::Mode::ReadWrite))
        worlds.put(std::to_string(entryIndex), entry.serialize());
    if (!character)
        return;
    character->saveVisit(entry.seed, entry.name, position);
    // Saves §3.1: the character is saved under its own key only.
    SaveDb characters;
    if (characterIndex >= 0 && characters.open(saveFolder / "characters.db", SaveDb::Mode::ReadWrite) &&
        (uint32_t)characterIndex < characters.number("num").value_or(0))
        saveCharacter(characters, (uint32_t)characterIndex, *character);
}
