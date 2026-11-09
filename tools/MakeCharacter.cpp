// Adds a new character to a characters.db, as Create Character would (for trying the character
// screens without clicking through them).
// Usage: MakeCharacter <save folder> <name> [race 0..7] [class 1..4]
#include "NewCharacter.h"
#include "Saves.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>

int main(int argc, char **argv) {
    if (argc < 3) {
        std::printf("usage: MakeCharacter <save folder> <name> [race 0..7] [class 1..4]\n");
        return 1;
    }
    std::filesystem::create_directories(argv[1]);
    SaveDb db;
    if (!db.open(std::filesystem::path(argv[1]) / "characters.db", SaveDb::Mode::ReadWrite)) {
        std::printf("cannot open characters.db\n");
        return 1;
    }
    creatures::CharacterChoice choice;
    choice.race = argc > 3 ? std::atoi(argv[3]) : 0;
    choice.hairColour[0] = 200, choice.hairColour[1] = 120, choice.hairColour[2] = 40;
    std::vector<CharacterRecord> characters = readCharacters(db);
    characters.push_back(newCharacter(argv[2], choice, argc > 4 ? std::atoi(argv[4]) : 1, [] { return std::rand() & 0x7fff; }));
    if (!writeCharacters(db, characters)) {
        std::printf("could not write\n");
        return 1;
    }
    std::printf("%zu characters\n", characters.size());
    return 0;
}
