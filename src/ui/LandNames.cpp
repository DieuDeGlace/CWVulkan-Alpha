// The land-name banner's texts (docs/specs/alpha-ui-screens.md §5.9.1).
#include "ui/LandNames.h"
#include "Speech.h"
#include "creatures/Names.h"
#include "ui/Painter.h"
#include <map>

namespace ui {
namespace {

// A dictionary pattern with its first "@" replaced; `keepWithout` keeps a pattern with none.
std::string fill(const std::string &pattern, const std::string &name, bool keepWithout) {
    const size_t at = pattern.find('@');
    if (at == std::string::npos)
        return keepWithout ? pattern : std::string();
    return pattern.substr(0, at) + name + pattern.substr(at + 1);
}

std::string field(const Dictionary &d, const char *kind, const std::string &key, const char *child) {
    const DictionaryEntry *e = d.find(kind, key);
    const std::string *s = e ? e->field(child) : nullptr;
    return s ? *s : std::string();
}

// The location-key table (W+0x800124), by (type, variant).
const char *locationKey(int32_t type, int32_t variant) {
    static const std::map<std::pair<int32_t, int32_t>, const char *> KEYS = {
        {{1, 0}, "Village"},   {{2, 0}, "Mountain"}, {{3, 0}, "Forest"},     {{4, 0}, "Lake"},
        {{5, 0}, "Ruins"},     {{5, 1}, "Ruins"},    {{5, 2}, "Ruins"},      {{5, 3}, "Ruins"},
        {{5, 4}, "Gravesite"}, {{6, 0}, "Canyon"},   {{7, 0}, "Valley"},     {{8, 0}, "Crater"},
        {{9, 0}, "Cave"},      {{10, 0}, "Portal"},  {{11, 0}, "Rock"},      {{12, 0}, "Tree"},
        {{13, 0}, "Peak"},     {{14, 0}, "Castle"},  {{14, 1}, "Ruins"},     {{14, 2}, "Catacombs"},
        {{14, 3}, "Palace"},   {{14, 4}, "Temple"},  {{14, 5}, "Pyramid"},   {{15, 0}, "Island"},
    };
    const auto it = KEYS.find({type, variant});
    return it == KEYS.end() ? nullptr : it->second;
}

// The zone-key table (W+0x80012c), by the zone entry's (kind, variant).
const char *zoneKey(int32_t kind, int32_t variant) {
    static const std::map<std::pair<int32_t, int32_t>, const char *> KEYS = {
        {{1, 1}, "Trade Quarter"}, {{1, 2}, "Crafting Quarter"}, {{1, 3}, "Class Quarter"},
        {{1, 4}, "Pet Quarter"},   {{4, 0}, "Portal"},           {{5, 0}, "Palace"},
    };
    const auto it = KEYS.find({kind, variant});
    return it == KEYS.end() ? nullptr : it->second;
}

} // namespace

std::string landNameOf(uint32_t n) { return creatures::generatedTitle(n, -1); }

std::string locationName(const worldgen::Location &L, const Dictionary &dictionary) {
    const char *key = locationKey(L.type, L.variant);
    return key ? fill(field(dictionary, "name", key, "singular"), landNameOf((uint32_t)L.serial), true) : std::string();
}

void locationVariables(const worldgen::Location &L, const Dictionary &dictionary, const std::string &prefix,
                       std::set<std::string> &tags, std::map<std::string, std::string> &variables) {
    const char *key = locationKey(L.type, L.variant);
    if (!key)
        return;
    speechNameVariables(dictionary, key, prefix, tags, variables);
    const std::string name = landNameOf((uint32_t)L.serial);
    for (auto &[variable, text] : variables)
        if (variable.rfind("@" + prefix + "_", 0) == 0)
            text = fill(text, name, true);
}

std::u16string objectiveText(const MissionArea &m, const worldgen::Location &L, const Dictionary &dictionary) {
    const char *kind = objectiveKey(m.kind);
    const DictionaryEntry *e = kind ? dictionary.find("speech", std::string("objective:") + kind) : nullptr;
    if (!e)
        return {};
    std::set<std::string> tags;
    std::map<std::string, std::string> variables;
    variables["@name"] = creatures::generatedTitle(m.seed, m.targetType);
    if (const char *species = creatures::speciesKey(m.targetType))
        speechNameVariables(dictionary, species, "creature", tags, variables);
    locationVariables(L, dictionary, "scenery", tags, variables);
    std::string line;
    for (const SpeechPage &page : SpeechText(e->text).render(tags, variables))
        for (const SpeechSegment &word : page)
            line += (line.empty() || word.text == "." ? "" : " ") + word.text;
    return utf16(line);
}

std::string zoneDetail(const worldgen::World &world, int32_t zx, int32_t zy, const Dictionary &dictionary) {
    const worldgen::ZoneEntry *E = zx >= 0 && zy >= 0 ? world.zoneEntry(zx, zy) : nullptr;
    const char *key = E ? zoneKey(E->kind, E->variant) : nullptr;
    if (!key)
        return {};
    const std::string pattern = field(dictionary, "name", key, "singular");
    // (Without an id the original takes a grid name, which fresh worlds never reach.)
    return pattern.find('@') == std::string::npos || (E->id != 0 && E->kind != 0)
               ? fill(pattern, landNameOf((uint32_t)E->id), true)
               : std::string();
}

LandText landText(const worldgen::World &world, int32_t x, int32_t y, const Dictionary &dictionary) {
    LandText out;
    // The name: a location's within about three quarters of its radius, else the region's.
    const worldgen::Location *L = world.location(x, y);
    float w = 0;
    if (L && L->type != 0 && L->type != 10) {
        const float f = world.locationDistanceFx(*L, (int64_t)x << 16, (int64_t)y << 16);
        w = f < 1 ? (1 - f) * (1 - f) : 0.f;
    }
    if (L && L->type != 0 && L->type != 10 && !(0.2f > w)) {
        out.name = locationName(*L, dictionary);
    } else if (const worldgen::RegionCell *C = world.owner(x, y)) {
        const std::string pattern = field(dictionary, "landscape", C->elevation < 0 ? "Ocean" : "Lands of", "normal");
        out.name = fill(pattern, landNameOf((uint32_t)C->id), false);
    }
    // The detail: the zone's district or portal.
    if (x >= 0 && y >= 0)
        out.detail = zoneDetail(world, x / 256, y / 256, dictionary);
    return out;
}

} // namespace ui
