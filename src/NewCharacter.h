#pragma once
// A new character as Alpha makes one (docs/specs/alpha-character-screens.md §3.9, §3.10): the
// creature type of a race and gender, how many faces and haircuts it has, and the starting kit,
// inventory and recipes of each class.
#include "Saves.h"
#include "creatures/Appearance.h"
#include <functional>
#include <string>

// §3.9: the creature type of a race (0 human … 7 frogman) and gender.
int32_t creatureTypeOf(int32_t race, bool female);
// §3.9: the face and haircut counts of a creature type (the wrap of the arrows).
int32_t faceCount(int32_t type);
int32_t haircutCount(int32_t type);
// §3.10: the record Create Character writes. `characterClass` is 1 warrior … 4 rogue; `draw`
// is rand() (the level-1 recipe grants spend draws).
CharacterRecord newCharacter(const std::string &name, const creatures::CharacterChoice &choice, int32_t characterClass,
                             const std::function<int()> &draw);
// The choice record as the save keeps it (field 13, 20 bytes).
std::array<uint8_t, 20> choiceBytes(const creatures::CharacterChoice &choice);
