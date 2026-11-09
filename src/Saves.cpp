#include "Saves.h"
#include "worldgen/Regions.h"
#include "Bytes.h"
#include "Install.h"
#include "SqlitePath.h"
#include "ZlibStream.h"
#include <sqlite3.h>
#include <bit>

namespace {

constexpr uint32_t FORMAT = 1; // every structured save blob starts with format version 1

// One prepared statement, finalised when it goes out of scope.
class Statement {
  public:
    Statement(sqlite3 *db, const char *sql) {
        if (db)
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    }
    ~Statement() { sqlite3_finalize(stmt); }
    Statement &text(int at, const std::string &s) {
        sqlite3_bind_text(stmt, at, s.c_str(), (int)s.size(), SQLITE_TRANSIENT);
        return *this;
    }
    Statement &blob(int at, const std::vector<uint8_t> &b) {
        sqlite3_bind_blob(stmt, at, b.data(), (int)b.size(), SQLITE_TRANSIENT);
        return *this;
    }
    int step() { return stmt ? sqlite3_step(stmt) : SQLITE_ERROR; }
    sqlite3_stmt *get() const { return stmt; }

  private:
    sqlite3_stmt *stmt = nullptr;
};

template <size_t N> std::array<uint8_t, N> readArray(ByteReader &r) {
    std::array<uint8_t, N> a{};
    r.read(a.data(), N);
    return a;
}

} // namespace

SaveDb::~SaveDb() { sqlite3_close(db); }

bool SaveDb::open(const std::filesystem::path &file, Mode mode, std::string *error) {
    sqlite3_close(db);
    db = nullptr;
    int result;
    if (mode == Mode::ReadOnly) {
        const std::string uri = sqliteReadOnlyUri(file);
        result = sqlite3_open_v2(uri.c_str(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr);
    } else {
        result = sqlite3_open_v2(utf8(file).c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        if (result == SQLITE_OK)
            result = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS blobs(key TEXT PRIMARY KEY, value BLOB);", nullptr,
                                  nullptr, nullptr);
    }
    if (result != SQLITE_OK) {
        if (error)
            *error = utf8(file) + ": " + (db ? sqlite3_errmsg(db) : "cannot open");
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    return true;
}

std::optional<std::vector<uint8_t>> SaveDb::get(const std::string &key) const {
    Statement q(db, "SELECT value FROM blobs WHERE key = ?");
    q.text(1, key);
    if (q.step() != SQLITE_ROW)
        return std::nullopt;
    const auto *data = (const uint8_t *)sqlite3_column_blob(q.get(), 0);
    return std::vector<uint8_t>(data, data + sqlite3_column_bytes(q.get(), 0));
}

bool SaveDb::put(const std::string &key, const std::vector<uint8_t> &value) {
    // §2: update if the key exists, else insert, as Alpha does.
    Statement exists(db, "SELECT 1 FROM blobs WHERE key = ?");
    exists.text(1, key);
    if (exists.step() == SQLITE_ROW) {
        Statement u(db, "UPDATE blobs SET value=? WHERE key=?");
        return u.blob(1, value).text(2, key).step() == SQLITE_DONE;
    }
    Statement i(db, "INSERT INTO blobs(key, value) VALUES(?, ?)");
    return i.text(1, key).blob(2, value).step() == SQLITE_DONE;
}

bool SaveDb::putIfAbsent(const std::string &key, const std::vector<uint8_t> &value) {
    Statement insert(db, "INSERT OR IGNORE INTO blobs(key, value) VALUES(?, ?)");
    return insert.text(1, key).blob(2, value).step() == SQLITE_DONE;
}

bool SaveDb::remove(const std::string &key) {
    Statement d(db, "DELETE FROM blobs WHERE key = ?");
    return d.text(1, key).step() == SQLITE_DONE;
}

bool SaveDb::rename(const std::string &from, const std::string &to) {
    Statement u(db, "UPDATE blobs SET key = ? WHERE key = ?");
    return u.text(1, to).text(2, from).step() == SQLITE_DONE;
}

std::vector<std::string> SaveDb::keys(const std::string &prefix) const {
    std::vector<std::string> out;
    Statement q(db, "SELECT key FROM blobs ORDER BY key");
    while (q.step() == SQLITE_ROW) {
        std::string key = (const char *)sqlite3_column_text(q.get(), 0);
        if (key.compare(0, prefix.size(), prefix) == 0)
            out.push_back(std::move(key));
    }
    return out;
}

std::optional<uint32_t> SaveDb::number(const std::string &key) const {
    const auto v = get(key);
    if (!v || v->size() != 4)
        return std::nullopt;
    return ByteReader(*v).u32();
}

bool SaveDb::putNumber(const std::string &key, uint32_t value) {
    ByteWriter w;
    w.u32(value);
    return put(key, w.data());
}

// §3.2
std::optional<CharacterRecord> CharacterRecord::parse(const std::vector<uint8_t> &inflated) {
    ByteReader r(inflated);
    if (r.u32() != FORMAT)
        return std::nullopt;
    CharacterRecord c;
    for (auto &p : c.position)
        p = r.i64();
    for (auto &o : c.orientation)
        o = r.f32();
    c.health = r.f32();
    c.experience = r.i32();
    c.level = r.i32();
    c.characterClass = r.u8();
    c.specialization = r.u8();
    c.unknown9 = r.u32();
    c.unknown10 = r.u32();
    for (auto &item : c.equipment)
        item = readArray<280>(r);
    c.name = r.string();
    c.extra = readArray<20>(r);
    const uint32_t tabs = r.u32();
    for (uint32_t t = 0; t < tabs && r.ok(); t++) {
        const uint32_t n = r.u32();
        if ((size_t)n * 284 > r.left())
            return std::nullopt;
        auto &tab = c.tabs.emplace_back(n);
        for (auto &slot : tab) {
            slot.count = r.u32();
            slot.item = readArray<280>(r);
        }
    }
    c.money = r.u32();
    c.platinum = r.u32();
    const uint32_t known = r.u32();
    if ((size_t)known * 280 > r.left())
        return std::nullopt;
    for (uint32_t i = 0; i < known; i++)
        c.recipes.push_back(readArray<280>(r));
    const uint32_t visits = r.u32();
    for (uint32_t i = 0; i < visits && r.ok(); i++) {
        WorldVisit v;
        v.seed = r.u32();
        v.name = r.string();
        for (auto &p : v.position)
            p = r.i64();
        c.visits.push_back(v);
    }
    c.lastWorldSeed = r.u32();
    c.lastWorldName = r.string();
    c.manaCubes = r.u32();
    const uint32_t skills = r.u32();
    if ((size_t)skills * 4 > r.left())
        return std::nullopt;
    for (uint32_t i = 0; i < skills; i++)
        c.skills.push_back(r.i32());
    if (!r.atEnd())
        return std::nullopt;
    return c;
}

std::vector<uint8_t> CharacterRecord::serialize() const {
    ByteWriter w;
    w.u32(FORMAT);
    for (auto p : position)
        w.i64(p);
    for (auto o : orientation)
        w.f32(o);
    w.f32(health);
    w.i32(experience);
    w.i32(level);
    w.u8(characterClass);
    w.u8(specialization);
    w.u32(unknown9);
    w.u32(unknown10);
    for (const auto &item : equipment)
        w.write(item.data(), item.size());
    w.string(name);
    w.write(extra.data(), extra.size());
    w.u32((uint32_t)tabs.size());
    for (const auto &tab : tabs) {
        w.u32((uint32_t)tab.size());
        for (const auto &slot : tab) {
            w.u32(slot.count);
            w.write(slot.item.data(), slot.item.size());
        }
    }
    w.u32(money);
    w.u32(platinum);
    w.u32((uint32_t)recipes.size());
    for (const auto &item : recipes)
        w.write(item.data(), item.size());
    w.u32((uint32_t)visits.size());
    for (const auto &v : visits) {
        w.u32(v.seed);
        w.string(v.name);
        for (auto p : v.position)
            w.i64(p);
    }
    w.u32(lastWorldSeed);
    w.string(lastWorldName);
    w.u32(manaCubes);
    w.u32((uint32_t)skills.size());
    for (auto s : skills)
        w.i32(s);
    return std::move(w.data());
}

std::vector<CharacterRecord> readCharacters(const SaveDb &db) {
    std::vector<CharacterRecord> out;
    const uint32_t n = db.number("num").value_or(0);
    for (uint32_t i = 0; i < n; i++)
        if (const auto value = db.get(std::to_string(i)))
            if (const auto inflated = zlibInflate(*value))
                if (auto c = CharacterRecord::parse(*inflated))
                    out.push_back(std::move(*c));
    return out;
}

bool writeCharacters(SaveDb &db, const std::vector<CharacterRecord> &characters) {
    const uint32_t old = db.number("num").value_or(0);
    for (size_t i = 0; i < characters.size(); i++)
        if (!db.put(std::to_string(i), zlibCompress(characters[i].serialize())))
            return false;
    for (uint32_t i = (uint32_t)characters.size(); i < old; i++)
        db.remove(std::to_string(i));
    return db.putNumber("num", (uint32_t)characters.size());
}

// §4.2
std::optional<WorldEntry> WorldEntry::parse(const std::vector<uint8_t> &value) {
    ByteReader r(value);
    WorldEntry e;
    e.name = r.string();
    e.seed = r.u32();
    e.discovered = r.u32();
    for (auto &s : e.previewSize)
        s = r.i32();
    if (e.previewSize[0] > 0 && e.previewSize[1] > 0 && e.previewSize[2] > 0) {
        const uint32_t c = r.u32();
        if (c > r.left())
            return std::nullopt;
        const auto packed = r.bytes(c);
        const auto rgb = zlibInflate(packed);
        if (!rgb || rgb->size() != (size_t)e.previewSize[0] * e.previewSize[1] * e.previewSize[2] * 3)
            return std::nullopt;
        e.previewRgb = *rgb;
    }
    if (!r.atEnd())
        return std::nullopt;
    return e;
}

std::vector<uint8_t> WorldEntry::serialize() const {
    ByteWriter w;
    w.string(name);
    w.u32(seed);
    w.u32(discovered);
    for (auto s : previewSize)
        w.i32(s);
    if (previewSize[0] > 0 && previewSize[1] > 0 && previewSize[2] > 0) {
        const auto packed = zlibCompress(previewRgb);
        w.u32((uint32_t)packed.size());
        w.bytes(packed);
    }
    return std::move(w.data());
}

std::vector<WorldEntry> readWorlds(const SaveDb &db) {
    std::vector<WorldEntry> out;
    const uint32_t n = db.number("num").value_or(0);
    for (uint32_t i = 0; i < n; i++)
        if (const auto value = db.get(std::to_string(i)))
            if (auto e = WorldEntry::parse(*value))
                out.push_back(std::move(*e));
    return out;
}

// §5.1
void WorldClock::advanceWorld(int32_t elapsedMs, bool accelerated, bool enabled) {
    timeOfDayMs += uint32_t(elapsedMs) * (accelerated ? 100u : 10u);
    if (!enabled)
        timeOfDayMs = 32400000;
    // The original comparison is signed and strict, retaining day-end equality.
    while (std::bit_cast<int32_t>(timeOfDayMs) > 86400000) {
        day++;
        timeOfDayMs -= 86400000u;
    }
}

std::optional<WorldClock> WorldClock::parse(const std::vector<uint8_t> &value) {
    if (value.size() != 8)
        return std::nullopt;
    ByteReader r(value);
    WorldClock c;
    c.day = r.u32();
    c.timeOfDayMs = r.u32();
    return c;
}

std::vector<uint8_t> WorldClock::serialize() const {
    ByteWriter w;
    w.u32(day);
    w.u32(timeOfDayMs);
    return std::move(w.data());
}

// §5.2
std::optional<ZoneSave> ZoneSave::parse(const std::vector<uint8_t> &value) {
    ByteReader r(value);
    if (r.u32() != FORMAT)
        return std::nullopt;
    ZoneSave z;
    const uint32_t n = r.u32();
    if ((size_t)n * 325 > r.left())
        return std::nullopt;
    for (uint32_t i = 0; i < n; i++)
        z.items.push_back(readArray<325>(r));
    const uint32_t e = r.u32();
    if ((size_t)e * 20 > r.left())
        return std::nullopt;
    for (uint32_t i = 0; i < e; i++)
        z.blockEdits.push_back(readArray<20>(r));
    const uint32_t s = r.u32();
    if ((size_t)s * 8 > r.left())
        return std::nullopt;
    for (uint32_t i = 0; i < s; i++)
        z.objects.push_back({r.u32(), r.u32()});
    const uint32_t t = r.u32();
    if (t > r.left())
        return std::nullopt;
    z.staticStates = r.bytes(t);
    if (!r.atEnd())
        return std::nullopt;
    return z;
}

std::vector<uint8_t> ZoneSave::serialize() const {
    ByteWriter w;
    w.u32(FORMAT);
    w.u32((uint32_t)items.size());
    for (const auto &i : items)
        w.write(i.data(), i.size());
    w.u32((uint32_t)blockEdits.size());
    for (const auto &b : blockEdits)
        w.write(b.data(), b.size());
    w.u32((uint32_t)objects.size());
    for (const auto &o : objects) {
        w.u32(o[0]);
        w.u32(o[1]);
    }
    w.u32((uint32_t)staticStates.size());
    w.bytes(staticStates);
    return std::move(w.data());
}

std::optional<TileSave> TileSave::parse(const std::vector<uint8_t> &value) {
    ByteReader r(value);
    r.u32(); // version (1)
    TileSave t;
    t.base = r.i32();
    t.sx = r.i32(), t.sy = r.i32(), t.sz = r.i32();
    if (!r.ok())
        return std::nullopt;
    if (t.sx > 0 && t.sy > 0 && t.sz > 0) {
        const uint32_t c = r.u32();
        if (c > r.left())
            return std::nullopt;
        const auto inflated = zlibInflate(r.bytes(c));
        if (!inflated || inflated->size() != (size_t)t.sx * t.sy * t.sz * 3)
            return std::nullopt;
        t.rgb = *inflated;
    }
    const uint32_t n = r.u32();
    if ((size_t)n * 12 > r.left())
        return std::nullopt;
    for (uint32_t i = 0; i < n; i++)
        t.markers.push_back({r.i32(), r.i32(), r.i32()});
    return r.ok() ? std::optional<TileSave>(t) : std::nullopt;
}

std::vector<uint8_t> TileSave::serialize() const {
    ByteWriter w;
    w.u32(1);
    w.i32(base);
    w.i32(sx), w.i32(sy), w.i32(sz);
    if (sx > 0 && sy > 0 && sz > 0) {
        const std::vector<uint8_t> packed = zlibCompress(rgb);
        w.u32((uint32_t)packed.size());
        w.bytes(packed);
    }
    w.u32((uint32_t)markers.size());
    for (const auto &m : markers)
        w.i32(m[0]), w.i32(m[1]), w.i32(m[2]);
    return std::move(w.data());
}

std::optional<LandSave> LandSave::parse(const std::vector<uint8_t> &value) {
    ByteReader r(value);
    r.u32(); // version (1)
    LandSave l;
    l.originX = r.i32(), l.originY = r.i32();
    l.visited = r.u8() != 0;
    l.sx = r.i32(), l.sy = r.i32(), l.sz = r.i32();
    if (!r.ok())
        return std::nullopt;
    if (l.sx > 0 && l.sy > 0 && l.sz > 0) {
        const uint32_t c = r.u32();
        if (c > r.left())
            return std::nullopt;
        const auto inflated = zlibInflate(r.bytes(c));
        if (!inflated || inflated->size() != (size_t)l.sx * l.sy * l.sz * 3)
            return std::nullopt;
        l.rgb = *inflated;
    }
    if (r.left() < 28)
        return std::nullopt;
    const auto cell = r.bytes(28);
    std::copy(cell.begin(), cell.end(), l.cell.begin());
    return r.ok() ? std::optional<LandSave>(l) : std::nullopt;
}

std::vector<uint8_t> LandSave::serialize() const {
    ByteWriter w;
    w.u32(1);
    w.i32(originX), w.i32(originY);
    w.u8(visited ? 1 : 0);
    w.i32(sx), w.i32(sy), w.i32(sz);
    if (sx > 0 && sy > 0 && sz > 0) {
        const std::vector<uint8_t> packed = zlibCompress(rgb);
        w.u32((uint32_t)packed.size());
        w.bytes(packed);
    }
    w.write(cell.data(), cell.size());
    return std::move(w.data());
}

RegionSave::RegionSave() {
    // A fresh record holds the defaults the game writes before its region exists: level 1.
    for (ZoneCell &z : zones)
        z.level = 1;
    const worldgen::Location none;
    for (auto &l : locations) {
        ByteWriter w;
        w.u32(0), w.u32(0), w.u32(0), w.u32((uint32_t)none.level), w.u32(0);
        w.f32(0), w.i64(0), w.i64(0), w.f32(0);
        std::copy(w.data().begin(), w.data().end(), l.begin());
    }
}

void RegionSave::refresh(const worldgen::Region &region) {
    for (size_t i = 0; i < zones.size(); i++) {
        const worldgen::ZoneEntry &E = region.zones[i];
        ZoneCell &z = zones[i];
        z.info = {E.kind, E.variant, 0, 0};
        z.id = (uint32_t)E.id, z.level = (uint32_t)E.level;
        z.subLevel = E.subLevel;
    }
    for (size_t i = 0; i < locations.size(); i++) {
        const worldgen::Location &L = region.records[i];
        ByteWriter w;
        w.u32((uint32_t)L.type), w.u32((uint32_t)L.variant), w.u32((uint32_t)L.serial);
        w.u32((uint32_t)L.level), w.u32((uint32_t)L.subLevel);
        w.f32(L.height), w.i64(L.x), w.i64(L.y), w.f32(L.radius);
        std::copy(w.data().begin(), w.data().end(), locations[i].begin());
    }
}

std::optional<RegionSave> RegionSave::parse(const std::vector<uint8_t> &value) {
    ByteReader r(value);
    r.u32(); // version (1)
    RegionSave g;
    for (ZoneCell &z : g.zones) {
        z.flags = r.u8();
        for (uint8_t &b : z.info)
            b = r.u8();
        z.id = r.u32(), z.level = r.u32();
        z.subLevel = r.u8();
    }
    for (auto &l : g.locations) {
        const auto b = r.bytes(l.size());
        std::copy(b.begin(), b.end(), l.begin());
    }
    return r.ok() ? std::optional<RegionSave>(g) : std::nullopt;
}

std::vector<uint8_t> RegionSave::serialize() const {
    ByteWriter w;
    w.u32(1);
    for (const ZoneCell &z : zones) {
        w.u8(z.flags);
        for (const uint8_t b : z.info)
            w.u8(b);
        w.u32(z.id), w.u32(z.level);
        w.u8(z.subLevel);
    }
    for (const auto &l : locations)
        w.write(l.data(), l.size());
    return std::move(w.data());
}

std::string regionKey(int32_t rx, int32_t ry) { return "reg" + std::to_string(rx) + "_" + std::to_string(ry); }

std::string landKey(int32_t lx, int32_t ly) { return "land" + std::to_string(lx) + "_" + std::to_string(ly); }

std::string tileKey(int32_t zx, int32_t zy) { return "tile" + std::to_string(zx) + "_" + std::to_string(zy); }

std::string zoneKey(int32_t zx, int32_t zy) { return "zone" + std::to_string(zx) + "_" + std::to_string(zy); }
