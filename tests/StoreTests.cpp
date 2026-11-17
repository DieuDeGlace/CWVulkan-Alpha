// Checks the content-store loader against the owner's install: every stored value must
// unscramble and scramble back to the same bytes, and every .cub must have a size matching its
// dimensions. Usage: StoreTests [game folder]
#include "Config.h"
#include "CubModel.h"
#include "DataStore.h"
#include "Install.h"
#include <sqlite3.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>

static int failures = 0;
static void check(bool ok, const std::string &what) {
    if (!ok) {
        failures++;
        std::printf("FAIL %s\n", what.c_str());
    }
}

// A synthetic store with the §A2 schema, holding `records` (key, plain value) scrambled.
static bool writeStore(const fs::path &file, const std::vector<std::pair<std::string, std::vector<uint8_t>>> &records,
                       const char *schema = "CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);") {
    sqlite3 *db = nullptr;
    bool ok = sqlite3_open(utf8(file).c_str(), &db) == SQLITE_OK && sqlite3_exec(db, schema, nullptr, nullptr, nullptr) == SQLITE_OK;
    for (const auto &[key, plain] : records) {
        sqlite3_stmt *insert = nullptr;
        auto value = plain;
        scramble(value);
        ok = ok && sqlite3_prepare_v2(db, "INSERT INTO blobs(key, value) VALUES(?, ?)", -1, &insert, nullptr) == SQLITE_OK &&
             sqlite3_bind_text(insert, 1, key.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK &&
             sqlite3_bind_blob(insert, 2, value.data(), (int)value.size(), SQLITE_TRANSIENT) == SQLITE_OK &&
             sqlite3_step(insert) == SQLITE_DONE;
        sqlite3_finalize(insert);
    }
    sqlite3_close(db);
    return ok;
}

static std::vector<uint8_t> cub(int32_t x, int32_t y, int32_t z, const std::vector<uint8_t> &cells) {
    std::vector<uint8_t> out(12);
    std::memcpy(out.data(), &x, 4);
    std::memcpy(out.data() + 4, &y, 4);
    std::memcpy(out.data() + 8, &z, 4);
    out.insert(out.end(), cells.begin(), cells.end());
    return out;
}

static void syntheticChecks() {
    // §A4 decoding, worked by hand from the key table: for n = 3 only i = 1 exchanges (with 0),
    // then every byte is complemented.
    std::vector<uint8_t> three = {1, 2, 3};
    unscramble(three);
    check(three == std::vector<uint8_t>{253, 254, 252}, "three-byte decode exchanges bytes 1 and 0, then complements");
    // n = 5: i = 4 with 1, i = 1 with 2, i = 0 with 2 (i = 3 and 2 stay).
    std::vector<uint8_t> five = {0, 1, 2, 3, 4};
    unscramble(five);
    check(five == std::vector<uint8_t>{251, 253, 255, 252, 254}, "five-byte decode follows the key table");
    std::vector<uint8_t> none;
    unscramble(none);
    check(none.empty(), "an empty value is left unchanged");
    std::vector<uint8_t> long_(1000);
    for (size_t i = 0; i < long_.size(); i++)
        long_[i] = (uint8_t)(i * 7 + 3);
    auto coded = long_;
    scramble(coded);
    check(coded.size() == long_.size() && coded != long_, "encoding keeps the length");
    unscramble(coded);
    check(coded == long_, "decoding inverts encoding beyond the 44-entry table");

    // §A5.1: three sizes, then cells with x fastest; pure red/green/blue cells are markers.
    {
        std::vector<uint8_t> cells(2 * 3 * 2 * 3, 7);
        const auto set = [&](int x, int y, int z, uint8_t r, uint8_t g, uint8_t b) {
            uint8_t *p = &cells[3 * (x + 2 * (y + 3 * z))];
            p[0] = r, p[1] = g, p[2] = b;
        };
        set(1, 0, 0, 255, 0, 0);
        set(0, 2, 1, 255, 0, 0);
        set(1, 1, 0, 0, 255, 0);
        set(0, 1, 1, 0, 0, 255);
        set(1, 2, 1, 255, 0, 1); // not exactly pure
        CubModel model;
        check(model.load(cub(2, 3, 2, cells).data(), 12 + cells.size()) && model.complete, "synthetic model loads");
        check(model.colour(1, 1, 0) == glm::u8vec3(0, 255, 0) && model.colour(1, 2, 1) == glm::u8vec3(255, 0, 1),
              "cell (x, y, z) is at 12 + 3 (x + X (y + Y z))");
        check(model.markers[CubModel::RED] == std::vector<glm::ivec3>{{1, 0, 0}, {0, 2, 1}} &&
                  model.markers[CubModel::GREEN] == std::vector<glm::ivec3>{{1, 1, 0}} &&
                  model.markers[CubModel::BLUE] == std::vector<glm::ivec3>{{0, 1, 1}},
              "pure red, green and blue cells become markers in cell order");
        auto longer = cub(2, 3, 2, cells);
        longer.push_back(9);
        check(model.load(longer.data(), longer.size()) && model.complete && model.rgb.size() == cells.size(),
              "bytes past the last cell are ignored");
        const auto shorter = cub(2, 3, 2, std::vector<uint8_t>(cells.begin(), cells.begin() + 8));
        check(model.load(shorter.data(), shorter.size()) && !model.complete && model.rgb.size() == cells.size() &&
                  model.colour(0, 0, 0) == glm::u8vec3(7) && model.colour(1, 0, 0) == glm::u8vec3(255, 0, 0) &&
                  model.colour(0, 1, 0) == glm::u8vec3(0) &&
                  model.markers[CubModel::RED] == std::vector<glm::ivec3>{{1, 0, 0}} && model.markers[CubModel::GREEN].empty(),
              "a short value copies only whole cells and leaves the rest zero");
        const auto header = cub(2, 3, 2, {});
        check(model.load(header.data(), 8, nullptr, {1, 1, 4}) && model.sx == 2 && model.sy == 3 && model.sz == 4,
              "a truncated header takes the caller's default for the missing sizes");
        check(!model.load(header.data(), 8), "a missing size without a positive default is refused");
        check(!model.load(cub(2, 0, 2, {}).data(), 12) && !model.load(cub(-1, 1, 1, {}).data(), 12),
              "every size must be positive");
    }

    // §A2: stores open read-only; a missing one is never created, and an empty one is refused.
    const fs::path scratch = fs::current_path() /
        ("StoreTests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(scratch);
    {
        DataStore store;
        std::string error;
        check(!store.open(scratch / "data9.db", &error) && !fs::exists(scratch / "data9.db"),
              "opening a missing store fails without creating it");
        std::ofstream(scratch / "not.db", std::ios::binary) << "not a database at all, just text";
        check(!store.open(scratch / "not.db", &error), "a file without the SQLite 3 header is refused");
        check(writeStore(scratch / "other.db", {}, "CREATE TABLE things(name TEXT);") && !store.open(scratch / "other.db", &error),
              "a database without the blob table is refused");

        const auto model = cub(1, 1, 1, {255, 0, 0});
        const std::vector<uint8_t> wave = {'R', 'I', 'F', 'F', 4, 0, 0, 0, 'W', 'A', 'V', 'E'};
        const std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
        const std::string xml = "\xEF\xBB\xBF<?xml version=\"1.0\"?><tags/>";
        check(writeStore(scratch / "data1.db", {{"aim.cub", model}}) && writeStore(scratch / "data2.db", {{"a.wav", wave}}) &&
                  writeStore(scratch / "data3.db", {{"a.png", png}}) &&
                  writeStore(scratch / "data4.db", {{"dict_en.xml", std::vector<uint8_t>(xml.begin(), xml.end())}}),
              "synthetic stores written");
        check(hasSqliteHeader(scratch / "data1.db"), "a store begins with the SQLite 3 header");
        check(store.open(scratch / "data1.db", &error) && !store.empty() && store.keys() == std::vector<std::string>{"aim.cub"},
              "synthetic store opens: " + error);
        const auto raw = store.raw("aim.cub");
        check(raw && *raw != model && store.read("aim.cub") == model, "values are stored scrambled and read decoded");
        check(!store.read("AIM.cub"), "keys compare exactly");
        {
            Content content;
            check(content.open(scratch, &error) && content.read("dict_en.xml") &&
                      content.read("dict_en.xml")->size() == xml.size(), "four synthetic stores open as content");
        }
        check(writeStore(scratch / "empty.db", {}) && store.open(scratch / "empty.db", &error) && store.empty(),
              "an empty store opens but holds no record");
        fs::remove(scratch / "data3.db");
        {
            Content content;
            check(writeStore(scratch / "data3.db", {}) && !content.open(scratch, &error),
                  "content with an empty store is refused");
        }
        fs::remove(scratch / "data3.db");
        {
            Content content;
            check(!content.open(scratch, &error) && !fs::exists(scratch / "data3.db"),
                  "content with a missing store is refused and the store is not created");
        }
    }
    std::error_code ignored;
    fs::remove_all(scratch, ignored);
}

int main(int argc, char **argv) {
    syntheticChecks();
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
                check(model.load(plain.data(), plain.size(), &why) && model.complete && plain.size() == 12 + model.rgb.size(),
                      key + ": " + why);
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
