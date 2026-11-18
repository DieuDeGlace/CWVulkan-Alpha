#include "Saves.h"
#include "worldgen/Regions.h"
#include "Bytes.h"
#include "Install.h"
#include "SqlitePath.h"
#include "ZlibStream.h"
#include <sqlite3.h>
#include <algorithm>
#include <bit>
#include <cstdio>

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

// §0: Alpha's record readers are truncation-tolerant. A field is taken only when enough bytes
// remain; otherwise reading stops and that field and every later one keep their prior values.
class FieldReader {
  public:
    explicit FieldReader(const std::vector<uint8_t> &bytes) : r(bytes) {}
    bool has(size_t n) {
        if (!stopped && r.left() >= n)
            return true;
        stopped = true;
        return false;
    }
    template <class T> bool field(T &out) {
        if (!has(sizeof(T)))
            return false;
        out = r.get<T>();
        return true;
    }
    template <size_t N> bool field(std::array<uint8_t, N> &out) {
        if (!has(N))
            return false;
        r.read(out.data(), N);
        return true;
    }
    bool string(std::string &out) {
        uint32_t n = 0;
        if (!field(n) || !has(n))
            return false;
        out = std::string(n, '\0');
        r.read(out.data(), n);
        return true;
    }
    bool bytes(std::vector<uint8_t> &out, size_t n) {
        if (!has(n))
            return false;
        out = r.bytes(n);
        return true;
    }
    size_t left() const { return r.left(); }

  private:
    ByteReader r;
    bool stopped = false;
};

constexpr uint32_t SKILLS = 11; // §3.2 field 23
// Engine bound: a stored count is not allowed to allocate more than this many entries.
constexpr uint32_t MAX_ENTRIES = 1u << 16;

} // namespace

SaveDb::~SaveDb() { close(); }

void SaveDb::close() {
    sqlite3_close(db);
    db = nullptr;
}

bool SaveDb::open(const std::filesystem::path &file, Mode mode, std::string *error) {
    close();
    int result;
    if (mode == Mode::ReadOnly) {
        const std::string uri = sqliteReadOnlyUri(file);
        result = sqlite3_open_v2(uri.c_str(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr);
    } else {
        result = sqlite3_open_v2(utf8(file).c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        // §2: the table creation runs on every open; its failure (the table exists) is ignored.
        if (result == SQLITE_OK)
            sqlite3_exec(db, "CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);", nullptr, nullptr, nullptr);
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
    if (db && !i.get()) // §2: the console line Alpha prints, only when the INSERT cannot be prepared
        std::puts("DATABASE WRITE ERROR");
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
    // §3.2 reader: the version is stored untested, every field is truncation-tolerant (§0), and
    // bytes after the last field are not read. A record therefore always results.
    FieldReader r(inflated);
    CharacterRecord c;
    uint32_t version = 0;
    r.field(version);
    for (auto &p : c.position)
        r.field(p);
    for (auto &o : c.orientation)
        r.field(o);
    r.field(c.health);
    r.field(c.experience);
    r.field(c.level);
    r.field(c.characterClass);
    r.field(c.specialization);
    r.field(c.unknown9);
    r.field(c.unknown10);
    for (auto &item : c.equipment)
        r.field(item);
    r.string(c.name);
    r.field(c.extra);
    if (uint32_t tabs = 0; r.field(tabs)) {
        c.tabs.clear();
        for (uint32_t t = 0; t < tabs && t < MAX_ENTRIES && r.has(4); t++) {
            uint32_t n = 0;
            r.field(n);
            // Each tab is resized to its stored count before its slots are copied.
            auto &tab = c.tabs.emplace_back(std::min(n, MAX_ENTRIES));
            for (auto &slot : tab)
                if (!r.field(slot.count) || !r.field(slot.item))
                    break;
        }
    }
    r.field(c.money);
    r.field(c.platinum);
    if (uint32_t known = 0; r.field(known))
        for (uint32_t i = 0; i < known && i < MAX_ENTRIES; i++) {
            ItemBytes item{};
            if (!r.field(item))
                break;
            c.recipes.push_back(item);
        }
    if (uint32_t visits = 0; r.field(visits))
        for (uint32_t i = 0; i < visits && i < MAX_ENTRIES; i++) {
            WorldVisit v;
            if (!r.field(v.seed) || !r.string(v.name) || !r.field(v.position[0]) || !r.field(v.position[1]) ||
                !r.field(v.position[2]))
                break;
            c.visits.push_back(v);
        }
    r.field(c.lastWorldSeed);
    r.string(c.lastWorldName);
    r.field(c.manaCubes);
    // Field 23: the 44 bytes are taken only when the count is exactly 11.
    if (uint32_t skills = 0; r.field(skills) && skills == SKILLS && r.has(4 * SKILLS)) {
        c.skills.assign(SKILLS, 0);
        for (auto &s : c.skills)
            r.field(s);
    }
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
    // Field 23 is always written as 11 levels (missing ones as 0, an engine default).
    w.u32(SKILLS);
    for (uint32_t i = 0; i < SKILLS; i++)
        w.i32(i < skills.size() ? skills[i] : 0);
    return std::move(w.data());
}

std::optional<std::array<int64_t, 3>> CharacterRecord::visitPosition(uint32_t seed, const std::string &world) const {
    for (const auto &visit : visits)
        if (visit.seed == seed && visit.name == world)
            return std::to_array(visit.position);
    return std::nullopt;
}

void CharacterRecord::saveVisit(uint32_t seed, const std::string &world, const std::array<int64_t, 3> &fixed) {
    std::copy(fixed.begin(), fixed.end(), position);
    lastWorldSeed = seed;
    lastWorldName = world;
    bool listed = false;
    for (auto &visit : visits)
        if (visit.seed == seed && visit.name == world) {
            std::copy(fixed.begin(), fixed.end(), visit.position);
            listed = true;
        }
    if (!listed)
        visits.push_back({seed, world, {fixed[0], fixed[1], fixed[2]}});
}

std::vector<CharacterRecord> readCharacters(const SaveDb &db) {
    // §3.1: keys 0..N-1, one record per key, so list indices stay the keys. A missing or damaged
    // value gives what the tolerant reader makes of the bytes that inflate (§0).
    std::vector<CharacterRecord> out;
    const uint32_t n = db.number("num").value_or(0);
    for (uint32_t i = 0; i < n && i < MAX_ENTRIES; i++) {
        const auto value = db.get(std::to_string(i));
        const std::vector<uint8_t> inflated = value ? zlibInflatePartial(*value) : std::vector<uint8_t>{};
        out.push_back(*CharacterRecord::parse(inflated));
    }
    return out;
}

bool saveCharacter(SaveDb &db, uint32_t index, const CharacterRecord &character) {
    return db.put(std::to_string(index), zlibCompress(character.serialize()));
}

bool addCharacter(SaveDb &db, const CharacterRecord &character) {
    const uint32_t n = db.number("num").value_or(0);
    return db.putNumber("num", n + 1) && saveCharacter(db, n, character);
}

bool deleteCharacter(SaveDb &db, uint32_t index) {
    const uint32_t n = db.number("num").value_or(0);
    if (index >= n)
        return false;
    bool ok = db.putNumber("num", n - 1) && db.remove(std::to_string(index));
    for (uint32_t k = index + 1; k < n; k++)
        ok &= db.rename(std::to_string(k), std::to_string(k - 1));
    return ok;
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
    // §4.2 with the tolerant reading of §0: an entry always results, and trailing bytes are not
    // read. A preview that does not inflate to its full size is padded with zeros to it.
    FieldReader r(value);
    WorldEntry e;
    r.string(e.name);
    r.field(e.seed);
    r.field(e.discovered);
    int32_t size[3] = {};
    if (!r.field(size[0]) || !r.field(size[1]) || !r.field(size[2]))
        return e;
    const uint64_t voxels = (uint64_t)std::max(size[0], 0) * (uint64_t)std::max(size[1], 0) * (uint64_t)std::max(size[2], 0);
    constexpr uint64_t MAX_PREVIEW = 1u << 24; // engine bound on the voxels a preview may claim
    if (voxels > MAX_PREVIEW)
        return e;
    std::copy(size, size + 3, e.previewSize);
    if (voxels > 0) {
        uint32_t c = 0;
        if (std::vector<uint8_t> packed; r.field(c) && r.bytes(packed, c))
            e.previewRgb = zlibInflatePartial(packed, (size_t)voxels * 3);
        e.previewRgb.resize((size_t)voxels * 3);
    }
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
    // §4.1: one entry per key 0..N-1, so list indices stay the keys.
    std::vector<WorldEntry> out;
    const uint32_t n = db.number("num").value_or(0);
    for (uint32_t i = 0; i < n && i < MAX_ENTRIES; i++) {
        const auto value = db.get(std::to_string(i));
        out.push_back(*WorldEntry::parse(value ? *value : std::vector<uint8_t>{}));
    }
    return out;
}

bool deleteWorldEntry(SaveDb &db, uint32_t selected) {
    // §4.1 (Alpha's own rewrite): the entry leaves the list, `num` becomes N-1, and every
    // remaining entry is written in turn under the selected key, so the last one ends up there.
    // The selection is lowered only past the new count, which cannot happen (character screens
    // §4.5): deleting the last entry writes beyond `num` and leaves the others untouched.
    // (Alpha re-serialises its loaded entries; the stored bytes are rewritten here, which is the
    // same for every entry its writer made.)
    const uint32_t n = db.number("num").value_or(0);
    if (selected >= n)
        return false;
    std::vector<std::vector<uint8_t>> remaining;
    for (uint32_t i = 0; i < n && i < MAX_ENTRIES; i++)
        if (i != selected)
            remaining.push_back(db.get(std::to_string(i)).value_or(std::vector<uint8_t>{}));
    bool ok = db.putNumber("num", n - 1);
    for (const auto &value : remaining)
        ok &= db.put(std::to_string(selected), value);
    return ok;
}

bool deleteWorldEntry(SaveDb &db, std::vector<WorldEntry> &entries, uint32_t selected) {
    // Character screens §4.5: as above, from the list held in memory, which keeps the shifted order.
    if (selected >= entries.size())
        return false;
    entries.erase(entries.begin() + selected);
    bool ok = db.putNumber("num", (uint32_t)entries.size());
    for (const WorldEntry &entry : entries)
        ok &= db.put(std::to_string(selected), entry.serialize());
    return ok;
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
