#include "WorldSession.h"
#include "GenerationProfile.h"
#include "AlphaLook.h"

namespace fs = std::filesystem;

namespace {

constexpr double FIXED = 65536.0; // positions are kept in 1/65536 blocks

fs::path worldFile(const fs::path &folder, const std::string &name) { return folder / ("world_" + name + ".db"); }

} // namespace

bool WorldSession::openMap() {
    if (!mapOpen)
        mapOpen = mapDb.open(saveFolder / ("map_" + entry.name + ".db"), SaveDb::Mode::ReadWrite);
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

bool WorldSession::open(const fs::path &folder, const std::string &name, uint32_t seed, int characterNumber,
                        std::string *error, const std::string &generationProfile) {
    saveFolder = folder / "Save";
    std::error_code ec;
    fs::create_directories(saveFolder, ec);
    if (!ensureGenerationProfile(worldFile(saveFolder, name), generationProfile, error))
        return false;

    // §4: find the world in the list, or add it.
    SaveDb worlds;
    if (!worlds.open(saveFolder / "worlds.db", SaveDb::Mode::ReadWrite, error))
        return false;
    const std::vector<WorldEntry> list = readWorlds(worlds);
    entryIndex = -1;
    for (size_t i = 0; i < list.size(); i++)
        if (list[i].name == name) {
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

    // §5.1: the clock; a new world starts at noon of day 0.
    clock = WorldClock{};
    SaveDb world;
    if (world.open(worldFile(saveFolder, name), SaveDb::Mode::ReadWrite, error))
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
        std::vector<CharacterRecord> all = readCharacters(characters);
        if (characterNumber >= (int)all.size()) {
            if (error)
                *error = "there is no character " + std::to_string(characterNumber) + " in characters.db";
            return false;
        }
        characterIndex = characterNumber;
        character = std::move(all[characterNumber]);
    }
    return true;
}

std::optional<glm::dvec3> WorldSession::startPosition() const {
    if (!character)
        return std::nullopt;
    auto toBlocks = [](const int64_t p[3]) { return glm::dvec3(p[0] / FIXED, p[1] / FIXED, p[2] / FIXED); };
    if (character->lastWorldSeed == entry.seed && character->lastWorldName == entry.name)
        return toBlocks(character->position);
    for (const auto &visit : character->visits)
        if (visit.seed == entry.seed && visit.name == entry.name)
            return toBlocks(visit.position);
    return std::nullopt;
}

void WorldSession::advance(double seconds) {
    pendingMs += seconds * 1000.0 * 10.0;
    const uint64_t whole = (uint64_t)pendingMs;
    pendingMs -= (double)whole;
    uint64_t ms = clock.timeOfDayMs + whole;
    while (ms >= (uint64_t)alpha::DAY_MS) {
        ms -= (uint64_t)alpha::DAY_MS;
        clock.day++;
    }
    clock.timeOfDayMs = (uint32_t)ms;
}

std::optional<ZoneSave> WorldSession::loadZone(int32_t zx, int32_t zy) {
    SaveDb world;
    if (!world.open(worldFile(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return std::nullopt;
    const auto bytes = world.get(zoneKey(zx, zy));
    return bytes ? ZoneSave::parse(*bytes) : std::nullopt;
}

void WorldSession::saveZoneItems(int32_t zx, int32_t zy, std::vector<std::array<uint8_t, 325>> items) {
    SaveDb world;
    if (!world.open(worldFile(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return;
    ZoneSave zone;
    if (const auto bytes = world.get(zoneKey(zx, zy)))
        if (auto old = ZoneSave::parse(*bytes))
            zone = std::move(*old);
    zone.items = std::move(items);
    world.put(zoneKey(zx, zy), zone.serialize());
}

void WorldSession::saveZoneStates(int32_t zx, int32_t zy, std::vector<uint8_t> states) {
    SaveDb world;
    if (!world.open(worldFile(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return;
    ZoneSave zone;
    if (const auto bytes = world.get(zoneKey(zx, zy)))
        if (auto old = ZoneSave::parse(*bytes))
            zone = std::move(*old);
    zone.staticStates = std::move(states);
    world.put(zoneKey(zx, zy), zone.serialize());
}

void WorldSession::saveZoneEdits(int32_t zx, int32_t zy, std::vector<std::array<uint8_t, 20>> edits) {
    SaveDb world;
    if (!world.open(worldFile(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return;
    ZoneSave zone;
    if (const auto bytes = world.get(zoneKey(zx, zy)))
        if (auto old = ZoneSave::parse(*bytes))
            zone = std::move(*old);
    zone.blockEdits = std::move(edits);
    world.put(zoneKey(zx, zy), zone.serialize());
}

std::map<std::string, std::vector<uint8_t>> WorldSession::values(const std::vector<std::string> &keys) {
    std::map<std::string, std::vector<uint8_t>> out;
    SaveDb world;
    if (!world.open(worldFile(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return out;
    for (const std::string &key : keys)
        if (auto bytes = world.get(key))
            out[key] = std::move(*bytes);
    return out;
}

void WorldSession::setValues(const std::map<std::string, std::vector<uint8_t>> &values) {
    SaveDb world;
    if (!world.open(worldFile(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        return;
    for (const auto &[key, bytes] : values)
        world.put(key, bytes);
}

void WorldSession::saveClock() {
    SaveDb world;
    if (world.open(worldFile(saveFolder, entry.name), SaveDb::Mode::ReadWrite))
        world.put("time", clock.serialize());
}

void WorldSession::setPreview(int32_t sx, int32_t sy, int32_t sz, std::vector<uint8_t> rgb) {
    entry.previewSize[0] = sx, entry.previewSize[1] = sy, entry.previewSize[2] = sz;
    entry.previewRgb = std::move(rgb);
}

void WorldSession::leave(const glm::dvec3 &position) {
    saveClock();
    SaveDb worlds;
    if (worlds.open(saveFolder / "worlds.db", SaveDb::Mode::ReadWrite))
        worlds.put(std::to_string(entryIndex), entry.serialize());
    if (!character)
        return;
    int64_t fixed[3];
    for (int axis = 0; axis < 3; axis++)
        fixed[axis] = (int64_t)(position[axis] * FIXED);
    std::copy(fixed, fixed + 3, character->position);
    character->lastWorldSeed = entry.seed;
    character->lastWorldName = entry.name;
    bool listed = false;
    for (auto &visit : character->visits)
        if (visit.seed == entry.seed && visit.name == entry.name) {
            std::copy(fixed, fixed + 3, visit.position);
            listed = true;
        }
    if (!listed) {
        CharacterRecord::WorldVisit visit;
        visit.seed = entry.seed;
        visit.name = entry.name;
        std::copy(fixed, fixed + 3, visit.position);
        character->visits.push_back(visit);
    }
    SaveDb characters;
    if (!characters.open(saveFolder / "characters.db", SaveDb::Mode::ReadWrite))
        return;
    std::vector<CharacterRecord> all = readCharacters(characters);
    if (characterIndex < (int)all.size()) {
        all[characterIndex] = *character;
        writeCharacters(characters, all);
    }
}
