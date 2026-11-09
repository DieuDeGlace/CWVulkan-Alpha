// Checks the content-store loader against the owner's install: every stored value must
// unscramble and scramble back to the same bytes, and every .cub must have a size matching its
// dimensions. Usage: StoreTests [game folder]
#include "Config.h"
#include "CubModel.h"
#include "DataStore.h"
#include "Install.h"
#include <cstdio>
#include <cstring>

static int failures = 0;
static void check(bool ok, const std::string &what) {
    if (!ok) {
        failures++;
        std::printf("FAIL %s\n", what.c_str());
    }
}

int main(int argc, char **argv) {
    // The transform on its own: a known short value, both directions.
    std::vector<uint8_t> sample = {'h', 'e', 'l', 'l', 'o', '!', 0, 255};
    const auto original = sample;
    scramble(sample);
    check(sample != original, "scramble changes bytes");
    unscramble(sample);
    check(sample == original, "unscramble undoes scramble");

    Config config;
    if (!config.load("AlphaVulkan.ini"))
        config.load("config/AlphaVulkan.ini");
    const fs::path game = install::locate(argc > 1 ? argv[1] : config.text("Game", "Path"));
    if (game.empty()) {
        std::printf("no Alpha install found; only the transform was checked\n");
        return failures ? 1 : 0;
    }
    Content content;
    std::string error;
    if (!content.open(game, &error)) {
        std::printf("FAIL %s\n", error.c_str());
        return 1;
    }

    const char *kinds[Content::COUNT] = {".cub", ".wav", ".png", ".xml"};
    for (int i = 0; i < Content::COUNT; i++) {
        const DataStore &store = content.store(i);
        size_t rows = 0, bytes = 0;
        for (const std::string &key : store.keys()) {
            const auto raw = store.raw(key);
            auto plain = *raw;
            unscramble(plain);
            auto back = plain;
            scramble(back);
            check(back == *raw, key + " round-trips");
            rows++;
            bytes += raw->size();
            if (key.ends_with(".cub")) {
                CubModel model;
                std::string why;
                check(model.load(plain.data(), plain.size(), &why), key + ": " + why);
            } else if (key.ends_with(".wav")) {
                check(plain.size() > 12 && std::memcmp(plain.data(), "RIFF", 4) == 0, key + " is RIFF");
            } else if (key.ends_with(".png")) {
                check(plain.size() > 8 && std::memcmp(plain.data(), "\x89PNG", 4) == 0, key + " is PNG");
            }
        }
        std::printf("data%d.db: %zu rows, %zu bytes (%s)\n", i + 1, rows, bytes, kinds[i]);
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
