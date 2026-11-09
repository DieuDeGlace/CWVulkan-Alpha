#pragma once
// What the interface calls a creature (docs/specs/alpha-ui-screens.md §5.4.1): its species
// name key in the dictionary, the title made from its id, whether it is a peaceful animal (the
// green bar), and the level curve the name colour compares.
#include <cstdint>
#include <string>

namespace creatures {

// §5.4.1 item 1: the dictionary key (a `<name>` entry) of a creature type, or null.
const char *speciesKey(int32_t type);
// §5.4.1 item 3: the title of a boss or an NPC, from its id and type (no draws).
std::string generatedTitle(uint64_t id, int32_t type);
// §5.4.1 item 5: a peaceful animal (gets the neutral bar when hostile-controlled).
bool peacefulAnimal(int32_t type, uint16_t appearanceFlags);
// §5.4.1 item 4: p(L) = 1 - 1 / ((L - 1) 0.05 + 1), in float.
float levelCurve(int32_t level);

} // namespace creatures
