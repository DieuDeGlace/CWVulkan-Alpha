// What the interface calls a creature (docs/specs/alpha-ui-screens.md §5.4.1).
#include "creatures/Names.h"
#include <vector>

namespace creatures {
namespace {

const char *const KEYS[156] = {
    "ElfMale" /* 0 */,
    "ElfFemale" /* 1 */,
    "HumanMale" /* 2 */,
    "HumanFemale" /* 3 */,
    "GoblinMale" /* 4 */,
    "GoblinFemale" /* 5 */,
    "Bullterrier" /* 6 */,
    "LizardmanMale" /* 7 */,
    "LizardmanFemale" /* 8 */,
    "DwarfMale" /* 9 */,
    "DwarfFemale" /* 10 */,
    "OrcMale" /* 11 */,
    "OrcFemale" /* 12 */,
    "FrogmanMale" /* 13 */,
    "FrogmanFemale" /* 14 */,
    "UndeadMale" /* 15 */,
    "UndeadFemale" /* 16 */,
    "Skeleton" /* 17 */,
    "OldMan" /* 18 */,
    "Collie" /* 19 */,
    "ShepherdDog" /* 20 */,
    "SkullBull" /* 21 */,
    "Alpaca" /* 22 */,
    "BrownAlpaca" /* 23 */,
    "Egg" /* 24 */,
    "Turtle" /* 25 */,
    "Terrier" /* 26 */,
    "ScottishTerrier" /* 27 */,
    "Wolf" /* 28 */,
    "Panther" /* 29 */,
    "Cat" /* 30 */,
    "BrownCat" /* 31 */,
    "WhiteCat" /* 32 */,
    "Pig" /* 33 */,
    "Sheep" /* 34 */,
    "Bunny" /* 35 */,
    "Porcupine" /* 36 */,
    "GreenSlime" /* 37 */,
    "PinkSlime" /* 38 */,
    "YellowSlime" /* 39 */,
    "BlueSlime" /* 40 */,
    "Frightener" /* 41 */,
    "SandHorror" /* 42 */,
    "Wizard" /* 43 */,
    "Bandit" /* 44 */,
    "Witch" /* 45 */,
    "Ogre" /* 46 */,
    "Rockling" /* 47 */,
    "Gnoll" /* 48 */,
    "PolarGnoll" /* 49 */,
    "Monkey" /* 50 */,
    "Gnobold" /* 51 */,
    "Insectoid" /* 52 */,
    "Hornet" /* 53 */,
    "InsectGuard" /* 54 */,
    "Crow" /* 55 */,
    "Chicken" /* 56 */,
    "Seagull" /* 57 */,
    "Parrot" /* 58 */,
    "Bat" /* 59 */,
    "Fly" /* 60 */,
    "Midge" /* 61 */,
    "Mosquito" /* 62 */,
    "PlainRunner" /* 63 */,
    "LeafRunner" /* 64 */,
    "SnowRunner" /* 65 */,
    "DesertRunner" /* 66 */,
    "Peacock" /* 67 */,
    "Frog" /* 68 */,
    "PlantCreature" /* 69 */,
    "RadishCreature" /* 70 */,
    "Onionling" /* 71 */,
    "DesertOnionling" /* 72 */,
    "Devourer" /* 73 */,
    "Duckbill" /* 74 */,
    "Crocodile" /* 75 */,
    "SpikeCreature" /* 76 */,
    "Anubis" /* 77 */,
    "Horus" /* 78 */,
    "Jester" /* 79 */,
    "Spectrino" /* 80 */,
    "Djinn" /* 81 */,
    "Minotaur" /* 82 */,
    "NomadMale" /* 83 */,
    "NomadFemale" /* 84 */,
    "Imp" /* 85 */,
    "Spitter" /* 86 */,
    "Mole" /* 87 */,
    "Biter" /* 88 */,
    "Koala" /* 89 */,
    "Squirrel" /* 90 */,
    "Raccoon" /* 91 */,
    "Owl" /* 92 */,
    "Penguin" /* 93 */,
    "Werewolf" /* 94 */,
    nullptr /* 95 */,
    "Zombie" /* 96 */,
    "Vampire" /* 97 */,
    "Horse" /* 98 */,
    "Camel" /* 99 */,
    "Cow" /* 100 */,
    "Dragon" /* 101 */,
    "BarkBeetle" /* 102 */,
    "FireBeetle" /* 103 */,
    "SnoutBeetle" /* 104 */,
    "LemonBeetle" /* 105 */,
    "Crab" /* 106 */,
    "SeaCrab" /* 107 */,
    "Troll" /* 108 */,
    "DarkTroll" /* 109 */,
    "HellDemon" /* 110 */,
    "Golem" /* 111 */,
    "EmberGolem" /* 112 */,
    "SnowGolem" /* 113 */,
    "Yeti" /* 114 */,
    "Cyclops" /* 115 */,
    "Mammoth" /* 116 */,
    "Lich" /* 117 */,
    "RuneGiant" /* 118 */,
    "Saurian" /* 119 */,
    "Bush" /* 120 */,
    "SnowBush" /* 121 */,
    "SnowBerryBush" /* 122 */,
    "CottonPlant" /* 123 */,
    "Scrub" /* 124 */,
    "CobwebScrub" /* 125 */,
    "FireScrub" /* 126 */,
    "Ginseng" /* 127 */,
    "Cactus" /* 128 */,
    nullptr /* 129 */,
    "ThornTree" /* 130 */,
    "GoldDeposit" /* 131 */,
    "IronDeposit" /* 132 */,
    "SilverDeposit" /* 133 */,
    "SandstoneDeposit" /* 134 */,
    "EmeraldDeposit" /* 135 */,
    "SapphireDeposit" /* 136 */,
    "RubyDeposit" /* 137 */,
    "DiamondDeposit" /* 138 */,
    "IceCrystalDeposit" /* 139 */,
    "Scarecrow" /* 140 */,
    "Aim" /* 141 */,
    "Dummy" /* 142 */,
    "Vase" /* 143 */,
    "Bomb" /* 144 */,
    "SapphireFish" /* 145 */,
    "LemonFish" /* 146 */,
    "Seahorse" /* 147 */,
    "Mermaid" /* 148 */,
    "Merman" /* 149 */,
    "Shark" /* 150 */,
    "Bumblebee" /* 151 */,
    "LanternFish" /* 152 */,
    "MawFish" /* 153 */,
    "Piranha" /* 154 */,
    "Blowfish" /* 155 */,
};

struct TitleParts {
    std::vector<const char *> first, second;
};

// §5.4.1 item 3: the two syllable lists of a type.
TitleParts partsOf(int32_t type) {
    switch (type) {
    case 0: return {{"El", "Li", "Dri", "Elan", "Le", "Ly", "A", "Tho", "Da", "Les"}, {"ric", "kun", "zic", "dor", "las", "sander", "reon", "reas", "lundra", "andor"}};
    case 1: return {{"Ar", "A", "Si", "Mer", "Lu", "Ari", "Su", "Sira", "Hia", "Li"}, {"weya", "luna", "laya", "leya", "ma", "lia", "matra", "vy", "zyna", "ly"}};
    case 2: return {{"De", "Ro", "As", "Ge", "An", "Pat", "Wolf", "Ale", "Mi", "Ben"}, {"rek", "man", "gram", "rald", "rick", "ram", "sander", "kal", "ton", "ny"}};
    case 3: return {{"Sa", "Jen", "Kla", "Mi", "La", "Auri", "Mela", "Alu", "Ve", "Est"}, {"rah", "ny", "ria", "elle", "na", "ga", "ma", "riana", "rona", "una"}};
    case 4: return {{"Gra", "Xe", "Ur", "Bel", "Chu", "Ki", "Go", "Zim", "Kubo", "Raz"}, {"zic", "nax", "tik", "tuk", "ruk", "bo", "rix", "mi", "tok", "nor"}};
    case 5: return {{"Kur", "Xe", "Ki", "Am", "Zifa", "Zu", "Ma", "Chi", "Zi", "Mia"}, {"ra", "lia", "bi", "ba", "ly", "ki", "bara", "mi", "zy", "xa"}};
    case 7: return {{"Cho", "Zer", "Kaz", "Kraz", "Zel", "Drak", "Lo", "Raz", "Spi", "Zen"}, {"rux", "rek", "zic", "tac", "mec", "kor", "rax", "zor", "ro", "go"}};
    case 8: return {{"Zi", "Zer", "Az", "Ki", "Iza", "Drak", "Li", "May", "Spi", "Zan"}, {"rya", "ri", "zia", "ki", "mah", "ira", "maya", "ana", "ra", "ya"}};
    case 9: return {{"Tor", "Gem", "Bar", "Me", "As", "Tem", "Arak", "Ur", "Grim", "Xor"}, {"lox", "bart", "kor", "thos", "kun", "bur", "thor", "lok", "li", "bor"}};
    case 10: return {{"Mum", "Grun", "Brun", "Hei", "Mo", "Hil", "Ur", "Lum", "Grim", "Xor"}, {"pie", "hild", "di", "muna", "ki", "trud", "sa", "axa", "ira", "ika"}};
    case 11: return {{"Uz", "Ur", "Chu", "Ku", "Mor", "Ura", "Ak", "Ur", "Or", "Gor"}, {"ku", "ruk", "muk", "thak", "dok", "tor", "rorok", "chak", "kaz", "rack"}};
    case 12: return {{"Uz", "Ur", "Chu", "Ku", "Mor", "Ura", "Ak", "Ur", "Or", "Gor"}, {"ka", "rua", "mua", "thara", "daka", "tah", "rorah", "chaka", "kaya", "rana"}};
    case 13: return {{"Qua", "Rib", "Quib", "Zib", "Il", "Wok", "Wib", "Moko", "Sli", "Bul"}, {"rik", "bit", "ble", "bik", "wak", "wok", "wib", "luk", "mey", "bak"}};
    case 14: return {{"Qua", "Rib", "Quib", "Zib", "Il", "Wok", "Wib", "Moko", "Sli", "Bul"}, {"ria", "bia", "bla", "bia", "waka", "woka", "wibba", "lua", "maya", "ba"}};
    case 15: return {{"Chu", "Dro", "Al", "Dem", "Mor", "Mur", "Cro", "Zul", "Dra", "The"}, {"lu", "kor", "card", "morius", "enius", "tus", "demar", "lus", "ruul", "zad"}};
    case 16: return {{"Xu", "Dra", "Al", "Dem", "Mor", "Myr", "Cra", "Zul", "Dri", "Thu"}, {"la", "kira", "cara", "moria", "ena", "tana", "diria", "laza", "rah", "zazah"}};
    case 18: return {{"De", "Ro", "As", "Ge", "An", "Pat", "Wolf", "Ale", "Mi", "Ben"}, {"rek", "man", "gram", "rald", "rick", "ram", "sander", "kal", "ton", "ny"}};
    case 43: return {{"De", "Ro", "As", "Ge", "An", "Pat", "Wolf", "Ale", "Mi", "Ben"}, {"rek", "man", "gram", "rald", "rick", "ram", "sander", "kal", "ton", "ny"}};
    case 45: return {{"Sa", "Jen", "Kla", "Mi", "La", "Auri", "Mela", "Alu", "Ve", "Est"}, {"rah", "ny", "ria", "elle", "na", "ga", "ma", "riana", "rona", "una"}};
    case 83: return {{"De", "Ro", "As", "Ge", "An", "Pat", "Wolf", "Ale", "Mi", "Ben"}, {"rek", "man", "gram", "rald", "rick", "ram", "sander", "kal", "ton", "ny"}};
    case 84: return {{"Sa", "Jen", "Kla", "Mi", "La", "Auri", "Mela", "Alu", "Ve", "Est"}, {"rah", "ny", "ria", "elle", "na", "ga", "ma", "riana", "rona", "una"}};
    default: return {{"Kro", "Aru", "As", "Ge", "Kur", "Lugo", "Iko", "Liku", "Tero", "Var", "An", "The", "Ga", "Lan", "Dura", "Dama", "Se", "Thal", "Nar", "San"}, {"no", "mi", "la", "sel", "ron", "ion", "kor", "lon", "rok", "tar", "den", "gar", "dar", "dara", "lan", "ka", "gor", "rior", "ria", "mor"}};
    }
}

} // namespace

const char *speciesKey(int32_t type) { return type >= 0 && type < 156 ? KEYS[type] : nullptr; }

std::string generatedTitle(uint64_t id, int32_t type) {
    const uint32_t s = (uint32_t)id;
    const TitleParts parts = partsOf(type);
    const uint32_t a = s / 7 + 13u * s, b = 7u * s + s / 10;
    return std::string(parts.first[a % parts.first.size()]) + parts.second[b % parts.second.size()];
}

bool peacefulAnimal(int32_t type, uint16_t appearanceFlags) {
    if (appearanceFlags & (0x200 | 0x800 | 0x1000))
        return false;
    switch (type) {
    case 19: case 20: case 22: case 23: case 25: case 26: case 27: case 30: case 31: case 32: case 33:
    case 34: case 35: case 53: case 55: case 56: case 57: case 58: case 67: case 68: case 74: case 87:
    case 89: case 92: case 93: case 98: case 100: case 106: case 107: case 145: case 146: case 147:
        return true;
    default:
        return false;
    }
}

float levelCurve(int32_t level) {
    const float L = (float)level;
    return 1.f - 1.f / ((L - 1.f) * 0.05f + 1.f);
}

} // namespace creatures
