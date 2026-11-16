// Checks the dictionary reader and the sound decoder: first on small made-up inputs, then on the
// owner's install if one is found. Usage: ContentTests [game folder]
#include "Audio.h"
#include "Config.h"
#include "DataStore.h"
#include "Dictionary.h"
#include "ItemNames.h"
#include "Speech.h"
#include <cstring>
#include "Install.h"
#include <cstdio>

static int failures = 0;
static void check(bool ok, const std::string &what) {
    if (!ok) {
        failures++;
        std::printf("FAIL %s\n", what.c_str());
    }
}

static void syntheticChecks() {
    Dictionary d;
    std::string error;
    check(d.parse("\xEF\xBB\xBF<?xml version=\"1.0\"?>\n<root>\n<!-- note -->\n"
                  "<name key=\"Cat\" tags=\"f an\"><singular>Cat &amp; co</singular><plural>Cats</plural></name>\n"
                  "<speech key=\"hello\">\n  Hi [there $stress]\n  %\n  Bye\n</speech>\n<empty key=\"e\"/>\n</root>",
                  &error),
          "parse synthetic: " + error);
    const auto *cat = d.find("name", "Cat");
    check(cat && cat->attributes.at("tags") == "f an", "attributes kept");
    check(cat && cat->field("singular") && *cat->field("singular") == "Cat & co", "entities decoded");
    check(cat && cat->field("plural") && *cat->field("plural") == "Cats", "second field");
    const auto *hello = d.find("speech", "hello");
    check(hello && hello->text == "Hi [there $stress]\n  %\n  Bye", "text trimmed, markup kept");
    check(d.find("empty", "e") != nullptr, "self-closing entry");
    check(!d.parse("<root><name key=\"x\"><singular>a</plural></name></root>", &error), "mismatched tags rejected");

    // Item names §3 step 6 (re-certified), on a made-up dictionary: a matched tag whose text has
    // no "@" falls through to the first child in element-name order, and "#" is replaced in both
    // branches; §3 step 4: a missing word just removes "@"; §7: the coin quantity is signed.
    Dictionary names;
    check(names.parse("<root>"
                      "<name key=\"MaterialIron\"><singular>Iron</singular><item>Iron# @</item></name>"
                      "<name key=\"MaterialCopper\"><singular>Copper</singular><item>Copper @</item></name>"
                      "<name key=\"Sword\" tags=\"t\"><singular>Sword</singular></name>"
                      "<name key=\"item:plain\"><t>no placeholder</t><b>B @</b><a>A @</a></name>"
                      "<name key=\"item:used\"><t>T @</t><a>A @</a></name>"
                      "</root>", &error),
          "parse made-up names: " + error);
    const auto record = [](uint8_t type, uint8_t sub, uint8_t material, uint32_t seed, uint8_t rarity) {
        creatures::ItemRecord r{};
        r[0] = type, r[1] = sub, r[0x0c] = rarity, r[0x0d] = material;
        std::memcpy(&r[4], &seed, 4);
        return r;
    };
    const std::string g1 = seedName(1), g5 = seedName(5);
    std::string got = itemName(record(3, 0, 1, 1, 0), names); // d 1: item:plain, tag t has no "@"
    check(got == "A Iron" + g1 + " Sword", "tag without @ falls back to the first child (got " + got + ")");
    got = itemName(record(3, 0, 1, 5, 0), names); // d 5: item:used, tag t with "@"
    check(got == "T Iron" + g5 + " Sword", "matched tag with @, then # (got " + got + ")");
    got = itemName(record(3, 18, 1, 0, 0), names);
    check(got == "Iron# ", "a missing word removes @ (got " + got + ")");
    creatures::ItemRecord coins = record(12, 0, 10, 0, 0);
    creatures::setItemLevel(coins, -5);
    got = receivedText(coins, names);
    check(got == "-5 Copper", "coin chat part: signed quantity and material word (got " + got + ")");
}

int main(int argc, char **argv) {
    syntheticChecks();

    Config config;
    if (!config.load("AlphaVulkan.ini"))
        config.load("config/AlphaVulkan.ini");
    const fs::path game = install::locate(argc > 1 ? argv[1] : config.text("Game", "Path"));
    Content content;
    std::string error;
    if (game.empty() || !content.open(game, &error)) {
        std::printf("no Alpha install found; only the synthetic checks ran\n");
        std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
        return failures ? 1 : 0;
    }

    // --list EXT prints every key with that extension and stops (a lookup aid).
    if (argc > 3 && std::string(argv[2]) == "--list") {
        for (const std::string &key : content.list(argv[3]))
            std::printf("%s\n", key.c_str());
        return 0;
    }

    for (const char *language : {"dict_en.xml", "dict_de.xml"}) {
        const auto bytes = content.read(language);
        Dictionary d;
        check(bytes && d.parse(std::string(bytes->begin(), bytes->end()), &error), std::string(language) + ": " + error);
        std::printf("%s: %zu entries\n", language, d.entries().size());
        check(d.entries().size() > 400, std::string(language) + " has its entries");
    }
    Dictionary english;
    const auto en = content.read("dict_en.xml");
    english.parse(std::string(en->begin(), en->end()));
    const auto *crow = english.find("name", "Crow");
    check(crow && crow->field("plural") && *crow->field("plural") == "Crows", "Crow's plural");
    const auto *hills = english.find("landscape", "Hills");
    check(hills && hills->field("normal") && *hills->field("normal") == "@ Hills", "Hills landscape");

    // Item names §8: the worked examples.
    {
        const auto item = [](uint8_t type, uint8_t sub, uint8_t material, uint32_t seed, uint8_t rarity) {
            creatures::ItemRecord r{};
            r[0] = type, r[1] = sub, r[0x0c] = rarity, r[0x0d] = material;
            std::memcpy(&r[4], &seed, 4);
            return r;
        };
        const auto named = [&](const creatures::ItemRecord &r, const std::string &want) {
            const std::string got = itemName(r, english);
            check(got == want, "item name " + want + " (got " + got + ")");
        };
        named(item(3, 0, 1, 0, 0), "Iron Sword");
        named(item(3, 0, 1, 10, 0), "shabby Iron Sword");
        named(item(3, 0, 1, 10, 3), "Xenax's exceptional Iron Sword");
        named(item(3, 0, 1, 3, 4), "sublime Iron Sword of Xorhild");
        named(item(3, 0, 1, 12345, 4), "glorious Iron Sword of Grimika");
        named(item(3, 0, 1, 12, 4), "magnificent Iron Sword von Razkor");
        named(item(3, 0, 6, 55, 2), "Sword");
        named(item(4, 0, 25, 0, 0), "Silk Chest Armor");
        named(item(9, 0, 11, 7, 1), "clean Gold Ring");
        named(item(1, 1, 3, 9, 2), "Life Potion");
        named(item(11, 0, 13, 51, 1), "balanced Emerald Nugget");
        named(item(11, 15, 0, 9, 3), "Mushroom");
        creatures::ItemRecord formula = item(2, 0, 1, 42, 1);
        formula[8] = 3;
        named(formula, "Formula: battle-tested Iron Sword");
        named(item(19, 19, 0, 0, 0), "Pet: Collie");
        named(item(20, 19, 0, 0, 0), "Bubble Gum");
        creatures::ItemRecord quest = item(21, 4, 0, 4, 0);
        const uint32_t giver = 1000;
        std::memcpy(&quest[8], &giver, 4);
        named(quest, "Urzic's Medicine");
        creatures::ItemRecord coins = item(12, 0, 10, 0, 0);
        creatures::setItemLevel(coins, 37);
        named(coins, "Copper Coin");
        check(receivedText(coins, english) == "37 Copper", "coins in chat");
    }

    // Talk §5.1-§5.3: the innkeeper, random talk for id 1, pet-food talk for id 1000.
    {
        const auto words = [](const std::vector<SpeechPage> &pages) {
            std::string out;
            for (const SpeechPage &p : pages) {
                for (const SpeechSegment &s : p)
                    out += s.text + " ";
                out += "| ";
            }
            return out;
        };
        // The NPC reseeds the single shared stream (srand) and draws from it; its later users
        // continue from that state (re-certified talk contract §0, §2.4).
        const auto nextAfter = [](uint32_t seed, int draws) {
            worldgen::Random expected(seed);
            for (int i = 0; i < draws; i++)
                expected.draw();
            return expected.draw();
        };
        worldgen::Random shared(777);
        const auto inn = npcSpeech(english, 5, 132, 0, nullptr, shared);
        check(inn.size() == 1 && inn[0].size() == 21, "the innkeeper's 21 segments: " + words(inn));
        check(shared.count() == 0 && shared.draw() == nextAfter(777, 0), "the innkeeper neither reseeds nor draws");
        const auto random = npcSpeech(english, 1, 136, 0, nullptr, shared);
        check(words(random) == "My Slime is adorable ! | ", "random:11 for id 1: " + words(random));
        check(!random.empty() && random[0].size() == 5 && random[0][1].colour == glm::vec4(1, 0.3f, 0.3f, 1),
              "the pet in creature red");
        check(shared.draw() == nextAfter(1, 4), "random talk leaves the shared stream four draws past srand(1)");
        const auto food = npcSpeech(english, 1000, 136, 3, nullptr, shared);
        check(words(food).find("Slimes") != std::string::npos && words(food).find("Pink Jellies") != std::string::npos,
              "pet-food talk for id 1000: " + words(food));
        check(shared.draw() == nextAfter(1000, 1), "pet-food talk leaves the stream one draw past srand(1000)");
        // §5.4: a resident speaks of Ruins of N (N = the name of serial 525240, "Duraron").
        worldgen::Location ruins;
        ruins.type = 5, ruins.serial = 525240;
        const auto explored = npcSpeech(english, 1000, 136, 4, &ruins, shared);
        check(words(explored).rfind("Have you already explored the Ruins of Duraron ?", 0) == 0 && explored.size() == 2,
              "explored talk: " + words(explored));
        check(shared.draw() == nextAfter(1000, 0), "explored talk reseeds but makes no draw");
    }

    size_t sounds = 0;
    double seconds = 0;
    for (const std::string &key : content.list(".wav")) {
        const auto bytes = content.read(key);
        SoundClip clip;
        check(bytes && decodeSound(bytes->data(), bytes->size(), clip) && clip.frames() > 0, key + " decodes");
        sounds++;
        seconds += clip.seconds();
    }
    std::printf("%zu sounds, %.1f s in all\n", sounds, seconds);
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
