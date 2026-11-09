#include "ItemNames.h"
#include "creatures/Names.h"
#include <map>

namespace {

// D(K, c) of §0: a child's text of a `name` entry, "" when the key or child is missing.
std::string text(const Dictionary &d, const std::string &key, const char *child) {
    if (key.empty())
        return {};
    const DictionaryEntry *e = d.find("name", key);
    const std::string *t = e ? e->field(child) : nullptr;
    return t ? *t : std::string();
}

// Replaces the first `mark` in `pattern` by `with`.
std::string fill(std::string pattern, char mark, const std::string &with) {
    if (const size_t at = pattern.find(mark); at != std::string::npos)
        pattern.replace(at, 1, with);
    return pattern;
}

// §1.3, in code order: rarity 0, 1, 2 (rarity 3 adds "2" to rarity 2's), 4.
const char *const ADJECTIVES[4][10] = {
    {"shabby", "plain", "battered", "artless", "unwieldy", "used", "dusty", "scratched", "worn", "common"},
    {"adjusted", "balanced", "battletested", "good", "handmade", "fair", "neat", "clean", "undamaged", "flawless"},
    {"exceptional", "polished", "extraordinary", "exquisite", "superb", "unique", "handsome", "grand", "magic",
     "decorated"},
    {"brilliant", "shining", "magnificent", "sublime", "pompous", "glorious", "splendid", "famous", "legendary",
     "fabulous"},
};

std::string adjectiveKey(uint8_t rarity, uint32_t seed) {
    const uint32_t d = seed % 10;
    switch (rarity) {
    case 0: case 1: case 2: return std::string("item:") + ADJECTIVES[rarity][d];
    case 3: return std::string("item:") + ADJECTIVES[2][d] + "2";
    default: return std::string("item:") + ADJECTIVES[3][d];
    }
}

// §3 step 6: the adjective entry's child matching one of the word's tags, else its first.
std::string adjective(const Dictionary &d, const std::string &key, const std::string &wordKey) {
    const DictionaryEntry *e = d.find("name", key);
    if (!e || e->fields.empty())
        return {};
    if (const DictionaryEntry *word = d.find("name", wordKey))
        if (const auto tags = word->attributes.find("tags"); tags != word->attributes.end()) {
            size_t from = 0;
            while (from <= tags->second.size()) {
                size_t end = tags->second.find_first_of(" ,", from);
                if (end == std::string::npos)
                    end = tags->second.size();
                if (const std::string *t = e->field(tags->second.substr(from, end - from)))
                    return *t;
                from = end + 1;
            }
        }
    // The first child in element-name order.
    const std::pair<std::string, std::string> *first = &e->fields.front();
    for (const auto &f : e->fields)
        if (f.first < first->first)
            first = &f;
    return first->second;
}

uint32_t u32At(const creatures::ItemRecord &item, size_t at) {
    return (uint32_t)item[at] | (uint32_t)item[at + 1] << 8 | (uint32_t)item[at + 2] << 16 | (uint32_t)item[at + 3] << 24;
}

} // namespace

std::string itemKey(uint8_t type, uint8_t sub) {
    static const std::map<std::pair<int, int>, const char *> KEYS = [] {
        std::map<std::pair<int, int>, const char *> k;
        const char *weapons[] = {"Sword", "Axe", "Mace", "Dagger", "Fist", "Longsword", "Bow", "Crossbow", "Boomerang",
                                 "Arrow", "Staff", "Wand", "Bracelet", "Shield", "Arrows", "Greatsword", "Greataxe",
                                 "Greatmace"};
        for (int s = 0; s < 18; s++)
            k[{3, s}] = weapons[s];
        k[{2, 0}] = "Formula", k[{3, 20}] = "Torch", k[{4, 0}] = "ChestArmor", k[{5, 0}] = "Gloves";
        k[{6, 0}] = "Boots", k[{7, 0}] = "ShoulderArmor", k[{8, 0}] = "Amulet", k[{9, 0}] = "Ring";
        k[{12, 0}] = "Coin", k[{13, 0}] = "PlatinumCoin", k[{14, 0}] = "Leftovers", k[{15, 0}] = "Beak";
        k[{16, 0}] = "Painting", k[{18, 0}] = k[{18, 1}] = "Candle", k[{19, 0}] = "Pet";
        k[{23, 0}] = "HangGlider", k[{23, 1}] = "Boat", k[{24, 0}] = "Lamp", k[{25, 0}] = "ManaCube";
        const char *consumables[] = {"Cookie", "LifePotion", "CactusPotion", "ManaPotion", "GinsengSoup",
                                     "SnowBerryMash", "MushroomSpit", "Bomb", "PineappleSlice", "PumpkinMuffin"};
        for (int s = 0; s < 10; s++)
            k[{1, s}] = consumables[s];
        const char *ingredients[] = {"Nugget", "Log", "Feather", "Horn", "Claw", "Fiber", "Cobweb", "Hair",
                                     "Crystal", "Yarn", "Cube", "Capsule", "Flask", "Orb", "Spirit", "Mushroom",
                                     "Pumpkin", "Pineapple", "RadishSlice", "ShimmerMushroom", "GinsengRoot",
                                     "OnionSlice", "Heartflower", "PricklyPear", "FrozenHeartflower", "Soulflower",
                                     "WaterFlask", "SnowBerry"};
        for (int s = 0; s < 28; s++)
            k[{11, s}] = ingredients[s];
        // Pet food: every subtype is bait, then the foods of the tameable creatures.
        for (int s = 0; s <= 155; s++)
            k[{20, s}] = "Bait";
        const std::pair<int, const char *> foods[] = {
            {19, "BubbleGum"}, {22, "VanillaCupcake"}, {23, "ChocolateCupcake"}, {25, "CinnamonRole"},
            {26, "Waffle"}, {27, "Croissant"}, {30, "Candy"}, {33, "PumpkinMash"}, {34, "CottonCandy"},
            {35, "Carrot"}, {36, "BlackberryMarmelade"}, {37, "GreenJelly"}, {38, "PinkJelly"},
            {39, "YellowJelly"}, {40, "BlueJelly"}, {50, "BananaSplit"}, {53, "Popcorn"}, {55, "LicoriceCandy"},
            {56, "CerealBar"}, {57, "SaltedCaramel"}, {58, "GingerTartlet"}, {59, "MangoJuice"},
            {60, "FruitBasket"}, {61, "MelonIceCream"}, {62, "BloodOrangeJuice"}, {63, "MilkChocolateBar"},
            {64, "MintChocolateBar"}, {65, "WhiteChocolateBar"}, {66, "CaramelChocolateBar"},
            {67, "ChocolateCookie"}, {74, "SugarCandy"}, {75, "AppleRing"}, {86, "WaterIce"},
            {87, "ChocolateDonut"}, {88, "Pancakes"}, {90, "StrawberryCake"}, {91, "ChocolateCake"},
            {92, "Lollipop"}, {93, "Softice"}, {98, "CandiedApple"}, {99, "DateCookie"}, {102, "Bread"},
            {103, "Curry"}, {104, "Lolly"}, {105, "LemonTart"}, {106, "StrawberryCocktail"}, {151, "BiscuitRole"}};
        for (const auto &[s, key] : foods)
            k[{20, s}] = key;
        const char *quest[] = {"Amulet1", "Amulet2", "JewelCase", "Key", "Medicine",
                               "Antivenom", "BandAid", "Crutch", "Bandage", "Salve"};
        for (int s = 0; s < 10; s++)
            k[{21, s}] = quest[s];
        return k;
    }();
    const auto it = KEYS.find({type, sub});
    return it == KEYS.end() ? std::string() : it->second;
}

std::string materialKey(uint8_t material) {
    switch (material) {
    case 1: return "MaterialIron";
    case 2: return "MaterialWood";
    case 5: return "MaterialObsidian";
    case 7: return "MaterialBone";
    case 10: return "MaterialCopper";
    case 11: return "MaterialGold";
    case 12: return "MaterialSilver";
    case 13: return "MaterialEmerald";
    case 14: return "MaterialSapphire";
    case 15: return "MaterialRuby";
    case 16: return "MaterialDiamond";
    case 17: return "MaterialSandstone";
    case 18: return "MaterialSaurian";
    case 19: return "MaterialParrot";
    case 20: return "MaterialMammoth";
    case 21: return "MaterialPlant";
    case 22: case 130: return "MaterialIce";
    case 23: return "MaterialLight";
    case 24: return "MaterialGlass";
    case 25: return "MaterialSilk";
    case 26: return "MaterialLinen";
    case 27: return "MaterialCotton";
    case 128: return "MaterialFire";
    case 129: return "MaterialUnholy";
    case 131: return "MaterialWind";
    default: return "Material";
    }
}

std::string seedName(uint32_t seed) { return creatures::generatedTitle(seed, (int32_t)((7u * seed) % 11u)); }

std::string itemName(const creatures::ItemRecord &item, const Dictionary &d) {
    using namespace creatures;
    const uint8_t type = itemType(item), sub = itemSubtype(item);
    switch (type) {
    case 2: { // §4: the taught item's name
        ItemRecord taught = item;
        taught[0x00] = item[0x08];
        std::fill(taught.begin() + 0x08, taught.begin() + 0x0c, 0);
        return "Formula: " + itemName(taught, d);
    }
    case 19: { // §5: the creature key as it is
        const char *key = speciesKey(sub);
        return std::string("Pet: ") + (key ? key : "");
    }
    case 21: // §6: the quest giver's title
        return generatedTitle(u32At(item, 0x08), (int32_t)u32At(item, 0x04)) + "'s " +
               text(d, itemKey(type, sub), "singular");
    case 1: case 14: case 18:
        return text(d, itemKey(type, sub), "singular");
    default:
        break;
    }
    // §3: material pattern, word, adjective.
    const std::string wordKey = itemKey(type, sub), word = text(d, wordKey, "singular");
    const std::string pattern = text(d, materialKey(itemMaterial(item)), "item");
    if (pattern.find('@') == std::string::npos)
        return word;
    std::string name = fill(pattern, '@', word);
    const uint32_t seed = u32At(item, 0x04);
    if (seed != 0 && itemRarity(item) <= 4) {
        const std::string a = adjective(d, adjectiveKey(itemRarity(item), seed), wordKey);
        if (!a.empty()) {
            name = fill(a, '@', name);
            name = fill(name, '#', seedName(seed));
        }
    }
    return name;
}

std::string receivedText(const creatures::ItemRecord &item, const Dictionary &d) {
    using namespace creatures;
    if (itemType(item) == 12)
        return std::to_string(itemLevel(item)) + " " + text(d, materialKey(itemMaterial(item)), "singular");
    if (quantityType(item))
        return std::to_string(itemLevel(item)) + " X " + itemName(item, d);
    return itemName(item, d) + " +" + std::to_string(powerOf(itemLevel(item)));
}
