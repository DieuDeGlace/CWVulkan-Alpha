#pragma once
// The land-name banner's texts (docs/specs/alpha-ui-screens.md §5.9.1): the name of the land,
// location or sea the player stands in, and the district or portal of its zone. Names are made
// from numbers the world generator stored (region cell ids, location serials), without draws.
#include "Dictionary.h"
#include "Missions.h"
#include "worldgen/Regions.h"
#include <map>
#include <set>
#include <string>

namespace ui {

struct LandText {
    std::string name, detail;
};

// §5.9.1: N(n), the name made from a number ("Duraron" for 525240).
std::string landNameOf(uint32_t n);
// §5.9.1: a location's name ("N City", "Ruins of N"...), empty when its kind has none.
std::string locationName(const worldgen::Location &L, const Dictionary &dictionary);
// §5.9.1: a zone's detail ("Trade District", "Portal of N"...), empty for most zones.
std::string zoneDetail(const worldgen::World &world, int32_t zx, int32_t zy, const Dictionary &dictionary);
// §5.9.1: what the banner shows for block column (x, y).
LandText landText(const worldgen::World &world, int32_t x, int32_t y, const Dictionary &dictionary);
// Talk §2.3: the tags and variables of location L under `prefix` (its `name` entry, each text
// with the first "@" replaced by the location's name).
void locationVariables(const worldgen::Location &L, const Dictionary &dictionary, const std::string &prefix,
                       std::set<std::string> &tags, std::map<std::string, std::string> &variables);
// Missions §8.3: the objective text of a mission at location L, as one line (no text for
// kinds without an `objective:` entry).
std::u16string objectiveText(const MissionArea &mission, const worldgen::Location &L, const Dictionary &dictionary);

} // namespace ui
