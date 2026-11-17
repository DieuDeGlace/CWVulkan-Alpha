#include "DataStore.h"
#include <algorithm>
#include "Install.h"
#include "SqlitePath.h"
#include <sqlite3.h>
#include <fstream>
#include <set>
#include <utility>

// The 44-entry key table (alpha-extractor.md §A4).
static constexpr uint32_t SWAPS[44] = {
    4242, 9551, 840, 84800, 9242, 9846, 127, 9, 9483, 394, 123,
    4834, 32444, 24355, 2433, 17, 34234, 42342, 4243, 14, 184934, 1987,
    3094, 1901, 89409, 4813, 37, 143, 3490, 19483, 1343, 432, 84732,
    9184, 9612, 1233, 3434, 1839, 2984, 1993, 2984, 4895, 816583, 13};

// The exchange partner of byte i: (table[i mod 44] + i) as unsigned 32 bits, modulo n.
static size_t partner(size_t i, size_t n) { return (uint32_t)(SWAPS[i % 44] + (uint32_t)i) % n; }

// Decoding: from the last byte down to the first, exchange byte i with its partner; then
// complement every byte. Encoding is the exact inverse.
void unscramble(std::vector<uint8_t> &bytes) {
    const size_t n = bytes.size();
    for (size_t i = n; i-- > 0;)
        std::swap(bytes[i], bytes[partner(i, n)]);
    for (uint8_t &b : bytes)
        b = (uint8_t)(255 - b);
}

void scramble(std::vector<uint8_t> &bytes) {
    const size_t n = bytes.size();
    for (uint8_t &b : bytes)
        b = (uint8_t)(255 - b);
    for (size_t i = 0; i < n; i++)
        std::swap(bytes[i], bytes[partner(i, n)]);
}

// One prepared statement, finalised when it goes out of scope.
class Query {
  public:
    Query(sqlite3 *db, const char *sql) {
        if (db)
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    }
    ~Query() { sqlite3_finalize(stmt); }
    sqlite3_stmt *get() const { return stmt; }
    bool step() const { return stmt && sqlite3_step(stmt) == SQLITE_ROW; }

  private:
    sqlite3_stmt *stmt = nullptr;
};

DataStore::~DataStore() { sqlite3_close(db); }

bool hasSqliteHeader(const std::filesystem::path &file) {
    char header[sizeof SQLITE_HEADER] = {};
    std::ifstream in(file, std::ios::binary);
    return in.read(header, sizeof header) && std::equal(header, header + sizeof header, SQLITE_HEADER);
}

bool DataStore::open(const std::filesystem::path &file, std::string *error) {
    sqlite3_close(db);
    db = nullptr;
    const auto fail = [&](const std::string &why) {
        if (error)
            *error = utf8(file) + ": " + why;
        sqlite3_close(db);
        db = nullptr;
        return false;
    };
    // The original client's open creates a missing store as an empty database (§A2), so a
    // successful open would prove nothing. Here a store must already be an SQLite 3 file, and
    // it is opened read-only and immutable without SQLITE_OPEN_CREATE: nothing in the owner's
    // installation is ever created or written, not even a journal.
    if (!hasSqliteHeader(file))
        return fail("missing, or not an SQLite 3 database");
    const std::string uri = sqliteReadOnlyUri(file);
    if (sqlite3_open_v2(uri.c_str(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr) != SQLITE_OK)
        return fail(db ? sqlite3_errmsg(db) : "cannot open");
    // The blob table, blobs(key TEXT PRIMARY KEY, value BLOB), with both columns readable.
    Query check(db, "SELECT key, value FROM blobs LIMIT 0");
    if (!check.get())
        return fail("no blobs(key, value) table");
    return true;
}

bool DataStore::empty() const {
    Query q(db, "SELECT 1 FROM blobs LIMIT 1");
    return !q.step();
}

std::vector<std::string> DataStore::keys() const {
    std::vector<std::string> out;
    Query q(db, "SELECT key FROM blobs ORDER BY key");
    while (q.step())
        out.emplace_back((const char *)sqlite3_column_text(q.get(), 0));
    return out;
}

std::optional<std::vector<uint8_t>> DataStore::raw(const std::string &key) const {
    Query q(db, "SELECT value FROM blobs WHERE key = ?");
    if (!q.get())
        return std::nullopt;
    sqlite3_bind_text(q.get(), 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
    if (!q.step())
        return std::nullopt;
    const auto *data = (const uint8_t *)sqlite3_column_blob(q.get(), 0);
    const int size = sqlite3_column_bytes(q.get(), 0);
    return std::vector<uint8_t>(data, data + size);
}

std::optional<std::vector<uint8_t>> DataStore::read(const std::string &key) const {
    auto bytes = raw(key);
    if (bytes)
        unscramble(*bytes);
    return bytes;
}

bool Content::open(const std::filesystem::path &gameDir, std::string *error) {
    for (int i = 0; i < COUNT; i++) {
        const auto file = gameDir / ("data" + std::to_string(i + 1) + ".db");
        if (!stores[i].open(file, error))
            return false;
        // An empty store is what the original client leaves behind for a missing file; every
        // lookup in it would fail later, so it is refused here instead.
        if (stores[i].empty()) {
            if (error)
                *error = utf8(file) + ": the content store holds no records";
            return false;
        }
    }
    return true;
}

Content::OverrideId Content::addOverride(const std::filesystem::path &folder) {
    if (nextOverride == 0)
        return 0;
    const auto id = nextOverride++;
    overrides.push_back({id, folder});
    return id;
}

bool Content::removeOverride(OverrideId id) {
    const auto entry = std::find_if(overrides.begin(), overrides.end(), [&](const Override &value) { return value.id == id; });
    if (entry == overrides.end())
        return false;
    overrides.erase(entry);
    return true;
}

// A key is a plain file name: no folders, so an override can never reach outside its folder.
static bool plainName(const std::string &key) {
    return !key.empty() && key.find_first_of("/\\:") == std::string::npos && key != "." && key != "..";
}

std::optional<std::vector<uint8_t>> Content::read(const std::string &key) const {
    if (plainName(key))
        for (auto folder = overrides.rbegin(); folder != overrides.rend(); ++folder) {
            std::ifstream in(folder->folder / pathOfUtf8(key), std::ios::binary);
            if (in)
                return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), {});
        }
    for (const DataStore &s : stores)
        if (auto bytes = s.read(key))
            return bytes;
    return std::nullopt;
}

std::vector<std::string> Content::list(const std::string &extension) const {
    auto matches = [&](const std::string &key) {
        return key.size() >= extension.size() &&
               key.compare(key.size() - extension.size(), extension.size(), extension) == 0;
    };
    std::set<std::string> keys;
    for (const DataStore &s : stores)
        for (std::string &key : s.keys())
            if (matches(key))
                keys.insert(std::move(key));
    for (const auto &folder : overrides) {
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(folder.folder, ec))
            if (entry.is_regular_file(ec) && matches(utf8(entry.path().filename())))
                keys.insert(utf8(entry.path().filename()));
    }
    return {keys.begin(), keys.end()};
}
