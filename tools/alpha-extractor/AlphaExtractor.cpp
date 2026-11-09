// Alpha extractor: a 32-bit mod for the original Cube World Alpha that records ground truth for
// AlphaVulkan's checks. Alpha's mod loader loads it into Cube.exe; on F9 (or when the file
// AlphaExtract\dump.now appears beside Cube.exe) it copies, under the game's own world lock,
// every loaded zone round the player (its blocks and spawn objects) and every creature record, and writes them to
// AlphaExtract\ for tools/ZoneCompare and friends; AlphaExtract\track.now records the creatures
// over 20 s instead.
//
// Where everything lives is docs/specs/alpha-extractor.md; addresses are cited from there at
// Alpha's preferred base 0x400000 and rebased at run time (Cube.exe is relocatable, §0).
// It only reads the game's memory; nothing in the game is patched. One test helper writes:
// AlphaExtract\teleport.now ("x y z" in blocks) moves the local player, for reaching a place in a copy
// of the game.
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

// §1.1, §2, §3, §4: the addresses and offsets used.
constexpr uint32_t GAME_GLOBAL = 0x76b1c8;   // holds G
constexpr uint32_t GAME_VTABLE = 0x6ffdc8;   // G's vtable, to check the pointer
constexpr uint32_t ZONE_VTABLE = 0x71e280;
constexpr uint32_t CREATURE_VTABLE = 0x6fd8cc;
constexpr uintptr_t WORLD = 0x2e4;           // W = G + 0x2e4
constexpr uintptr_t THREADS_RUN = 0x800584;  // G: set once the workers run
constexpr uintptr_t PLAYER = 0x8006d0;       // G: the local player's creature
constexpr uintptr_t REQUESTED_SEED = 0x800a50; // G
constexpr uintptr_t CREATURES = 0x04;        // W: map<u64 id, Creature*>
constexpr uintptr_t REGIONS = 0xbc;          // W: Region*[1024 * 1024], index rx * 1024 + ry
constexpr uintptr_t WORLD_LOCK = 0x8000c0;   // W: CRITICAL_SECTION
constexpr uintptr_t TIME_OF_DAY = 0x80015c, DAY = 0x800160, SEED = 0x800164; // W
constexpr uintptr_t WORLD_NAME_LENGTH = 0x94 + 0x10; // W: the world name string's length (§4)
constexpr uintptr_t REGION_ZONES = 0x10018;  // Region: Zone*[64 * 64], index (zx & 63) * 64 + (zy & 63)
constexpr uintptr_t REGION_HEADER = 0x0c;    // Region: tier, h10, style (locations spec §1)
constexpr uintptr_t REGION_TABLE = 0x18;     // Region: 16-byte per-zone entries [64][64]
constexpr uintptr_t REGION_LOCATIONS = 0x14018; // Region: 64 location records of 0x68 bytes
constexpr size_t LOCATION_SIZE = 0x68, LOCATION_KEPT = 0x2c; // (past 0x2c: stored mission data)
constexpr uintptr_t ZONE_X = 0x60, ZONE_Y = 0x64, ZONE_COLUMNS = 0xa8;
constexpr uintptr_t ZONE_SPAWNS = 0x18;      // Zone: vector<Spawn*> (§2.6)
constexpr uintptr_t ZONE_SPRITES = 0x04, ZONE_STATICS = 0x0c, ZONE_ITEMS = 0x30; // Zone: list, vectors (§2.6)
constexpr size_t SPRITE_DATA = 0x08, SPRITE_SIZE = 0x3c, STATIC_SIZE = 0x188, ITEM_SIZE = 0x148;
constexpr uint32_t SPAWN_VTABLE = 0x71dbec;
constexpr size_t SPAWN_SIZE = 0x10f0;
constexpr size_t COLUMN_SIZE = 32;           // bottom at +0x10, blocks at +0x18, count at +0x1c
constexpr uintptr_t RECORD = 0x10;           // Creature: the 0x1168-byte record
constexpr size_t RECORD_SIZE = 0x1168;
constexpr int ZONE_REACH = 3;                // zones kept loaded round the player (§5.2)

uintptr_t base = 0;
std::wstring folder; // ...\AlphaExtract\ beside Cube.exe

uintptr_t rebased(uint32_t address) { return base + (address - 0x400000); }
template <typename T> T peek(uintptr_t p) { return *(const T *)p; }

void log(const char *format, ...) {
    char line[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(line, sizeof line, format, args);
    va_end(args);
    if (FILE *f = _wfopen((folder + L"extractor.log").c_str(), L"a")) {
        SYSTEMTIME t;
        GetLocalTime(&t);
        std::fprintf(f, "%02d:%02d:%02d %s\n", t.wHour, t.wMinute, t.wSecond, line);
        std::fclose(f);
    }
}

void put(std::vector<uint8_t> &out, const void *p, size_t n) {
    out.insert(out.end(), (const uint8_t *)p, (const uint8_t *)p + n);
}

// §2.1: the zone at (zx, zy), or 0 when it is not loaded.
uintptr_t zoneAt(uintptr_t world, int32_t zx, int32_t zy) {
    if (zx < 0 || zy < 0 || zx >= 65536 || zy >= 65536)
        return 0;
    const uintptr_t region = peek<uintptr_t>(world + REGIONS + 4 * ((size_t)(zx >> 6) * 1024 + (size_t)(zy >> 6)));
    if (!region)
        return 0;
    return peek<uintptr_t>(region + REGION_ZONES + 4 * ((size_t)(zx & 63) * 64 + (size_t)(zy & 63)));
}

// One zone as a ZoneCompare recording (tools/ZoneCompare.cpp has the format): the stored
// block range of each column, raw (§2.3).
std::vector<uint8_t> recordZone(uintptr_t zone, uint32_t seed, uint32_t flags) {
    std::vector<uint8_t> out;
    out.reserve(4 << 20);
    const uint32_t version = 2;
    const int32_t zx = peek<int32_t>(zone + ZONE_X), zy = peek<int32_t>(zone + ZONE_Y);
    put(out, "AVZD", 4), put(out, &version, 4), put(out, &seed, 4), put(out, &flags, 4), put(out, &zx, 4),
        put(out, &zy, 4);
    const uintptr_t columns = peek<uintptr_t>(zone + ZONE_COLUMNS);
    for (size_t i = 0; i < 256 * 256; i++) {
        const uintptr_t column = columns + COLUMN_SIZE * i;
        const int32_t bottom = peek<int32_t>(column + 0x10);
        const uintptr_t blocks = peek<uintptr_t>(column + 0x18);
        int32_t count = peek<int32_t>(column + 0x1c);
        if (count < 0 || (count > 0 && !blocks))
            count = 0;
        const uint32_t n = (uint32_t)count;
        put(out, &bottom, 4), put(out, &n, 4);
        if (n)
            put(out, (const void *)blocks, 4 * (size_t)n);
    }
    return out;
}

// AVSP v2 adds the established native prefix count after the full list count.
// The retained suffix may have changed since generation (spawn lifecycle contract).
std::vector<uint8_t> recordSpawns(uintptr_t zone, uint32_t seed, uint32_t flags) {
    std::vector<uint8_t> out;
    const uint32_t version = 2;
    const int32_t zx = peek<int32_t>(zone + ZONE_X), zy = peek<int32_t>(zone + ZONE_Y);
    put(out, "AVSP", 4), put(out, &version, 4), put(out, &seed, 4), put(out, &flags, 4), put(out, &zx, 4),
        put(out, &zy, 4);
    const uintptr_t begin = peek<uintptr_t>(zone + ZONE_SPAWNS), end = peek<uintptr_t>(zone + ZONE_SPAWNS + 4);
    const uint32_t prefix = peek<uint32_t>(zone + 0xa0); // native prefix store, RVA 0x1edd5e
    if ((!begin && end) || end < begin || (end - begin) % 4 || (end - begin) / 4 > 100000 || prefix > (end - begin) / 4)
        return {};
    std::vector<uintptr_t> spawns;
    for (uintptr_t p = begin; p && p < end && spawns.size() < 100000; p += 4)
        if (const uintptr_t spawn = peek<uintptr_t>(p); spawn && peek<uint32_t>(spawn) == rebased(SPAWN_VTABLE))
            spawns.push_back(spawn);
        else
            return {}; // keep ordinals exact; an invalid pointer invalidates this recording
    const uint32_t count = (uint32_t)spawns.size();
    put(out, &count, 4), put(out, &prefix, 4);
    for (uintptr_t spawn : spawns)
        put(out, (const void *)spawn, SPAWN_SIZE);
    return out;
}

// A zone's other generated lists (§2.6) as "AVZC", u32 version 1, u32 seed, u32 flags, i32 zx, i32 zy,
// then three lists, each a u32 count and the raw records: sprites (0x3c bytes), statics (0x188),
// world items (0x148). Pointers in them are not followed.
std::vector<uint8_t> recordContents(uintptr_t zone, uint32_t seed, uint32_t flags) {
    std::vector<uint8_t> out;
    const uint32_t version = 1;
    const int32_t zx = peek<int32_t>(zone + ZONE_X), zy = peek<int32_t>(zone + ZONE_Y);
    put(out, "AVZC", 4), put(out, &version, 4), put(out, &seed, 4), put(out, &flags, 4), put(out, &zx, 4),
        put(out, &zy, 4);
    std::vector<uintptr_t> nodes;
    const uintptr_t head = peek<uintptr_t>(zone + ZONE_SPRITES);
    for (uintptr_t node = peek<uintptr_t>(head); node && node != head && nodes.size() < 1000000;
         node = peek<uintptr_t>(node))
        nodes.push_back(node);
    uint32_t count = (uint32_t)nodes.size();
    put(out, &count, 4);
    for (uintptr_t node : nodes)
        put(out, (const void *)(node + SPRITE_DATA), SPRITE_SIZE);
    const auto records = [&](uintptr_t field, size_t size) {
        const uintptr_t begin = peek<uintptr_t>(zone + field), end = peek<uintptr_t>(zone + field + 4);
        const uint32_t n = begin && end > begin ? (uint32_t)((end - begin) / size) : 0;
        put(out, &n, 4);
        if (n)
            put(out, (const void *)begin, (size_t)n * size);
    };
    records(ZONE_STATICS, STATIC_SIZE);
    records(ZONE_ITEMS, ITEM_SIZE);
    return out;
}

// A region's generated data (locations spec §1) as "AVRG", u32 version 2, u32 seed, u32 flags, i32 rx,
// i32 ry, i32 tier, h10, style, the 64 location records' first 0x2c bytes, then the 4096
// 16-byte per-zone entries in Alpha's order (64 zxl + zyl).
std::vector<uint8_t> recordRegion(uintptr_t region, uint32_t seed, uint32_t flags, int32_t rx, int32_t ry) {
    std::vector<uint8_t> out;
    const uint32_t version = 2;
    put(out, "AVRG", 4), put(out, &version, 4), put(out, &seed, 4), put(out, &flags, 4), put(out, &rx, 4),
        put(out, &ry, 4);
    put(out, (const void *)(region + REGION_HEADER), 12);
    for (size_t k = 0; k < 64; k++)
        put(out, (const void *)(region + REGION_LOCATIONS + LOCATION_SIZE * k), LOCATION_KEPT);
    put(out, (const void *)(region + REGION_TABLE), 16 * 64 * 64);
    return out;
}

// §3.1: every creature (in-order walk of the map's red-black tree), as "AVCR", u32 version 1,
// u32 count, then per creature u64 id and its 0x1168-byte record.
std::vector<uint8_t> recordCreatures(uintptr_t world, uint32_t &count) {
    std::vector<uint8_t> body;
    count = 0;
    const uintptr_t head = peek<uintptr_t>(world + CREATURES);
    std::vector<uintptr_t> stack;
    uintptr_t node = peek<uintptr_t>(head + 4); // the root
    const auto nil = [](uintptr_t n) { return peek<uint8_t>(n + 0x0d) != 0; };
    while ((!nil(node) || !stack.empty()) && count < 100000) {
        while (!nil(node)) {
            stack.push_back(node);
            node = peek<uintptr_t>(node); // left
        }
        node = stack.back();
        stack.pop_back();
        const uint64_t id = peek<uint64_t>(node + 0x10);
        const uintptr_t creature = peek<uintptr_t>(node + 0x18);
        if (creature && peek<uint32_t>(creature) == rebased(CREATURE_VTABLE)) {
            put(body, &id, 8);
            put(body, (const void *)(creature + RECORD), RECORD_SIZE);
            count++;
        }
        node = peek<uintptr_t>(node + 8); // right
    }
    return body;
}

void writeFile(const std::wstring &name, const std::vector<uint8_t> &bytes) {
    if (FILE *f = _wfopen((folder + name).c_str(), L"wb")) {
        std::fwrite(bytes.data(), 1, bytes.size(), f);
        std::fclose(f);
    }
}

struct Capture {
    uint32_t seed = 0, requestedSeed = 0;
    int32_t timeOfDay = 0, day = 0;
    int64_t player[3] = {};
    std::vector<std::pair<std::wstring, std::vector<uint8_t>>> zones;
    std::vector<uint8_t> creatures;
    uint32_t creatureCount = 0;
};

// Reads everything; the caller holds the world lock and catches a bad pointer.
bool read(uintptr_t game, Capture &c, const char *&why) {
    const uintptr_t world = game + WORLD;
    c.seed = peek<uint32_t>(world + SEED);
    c.requestedSeed = peek<uint32_t>(game + REQUESTED_SEED);
    c.timeOfDay = peek<int32_t>(world + TIME_OF_DAY);
    c.day = peek<int32_t>(world + DAY);
    // Recording flags, bit 0: the world is named (a named world has a town per region, locations
    // spec §4.1; the start screen's world has no name).
    const uint32_t flags = peek<uint32_t>(world + WORLD_NAME_LENGTH) > 0 ? 1u : 0u;
    const uintptr_t player = peek<uintptr_t>(game + PLAYER);
    if (c.seed != c.requestedSeed) {
        why = "the world is about to change (seed in use is not the requested one)";
        return false;
    }
    if (!player || peek<uint32_t>(player) != rebased(CREATURE_VTABLE)) {
        why = "no local player";
        return false;
    }
    for (int k = 0; k < 3; k++)
        c.player[k] = peek<int64_t>(player + RECORD + 8 * k);
    const int32_t px = (int32_t)(c.player[0] >> 24), py = (int32_t)(c.player[1] >> 24);
    if (!zoneAt(world, px, py)) {
        why = "the player's zone is not loaded";
        return false;
    }
    for (int32_t zx = px - ZONE_REACH; zx <= px + ZONE_REACH; zx++)
        for (int32_t zy = py - ZONE_REACH; zy <= py + ZONE_REACH; zy++)
            if (const uintptr_t zone = zoneAt(world, zx, zy); zone && peek<uint32_t>(zone) == rebased(ZONE_VTABLE)) {
                const std::wstring name = std::to_wstring(zx) + L"_" + std::to_wstring(zy);
                c.zones.push_back({L"zone_" + name + L".avzd", recordZone(zone, c.seed, flags)});
                c.zones.push_back({L"spawns_" + name + L".avsp", recordSpawns(zone, c.seed, flags)});
                c.zones.push_back({L"contents_" + name + L".avzc", recordContents(zone, c.seed, flags)});
            }
    for (int32_t rx = (px - ZONE_REACH) >> 6; rx <= (px + ZONE_REACH) >> 6; rx++)
        for (int32_t ry = (py - ZONE_REACH) >> 6; ry <= (py + ZONE_REACH) >> 6; ry++)
            if (const uintptr_t region = peek<uintptr_t>(world + REGIONS + 4 * ((size_t)rx * 1024 + (size_t)ry)))
                c.zones.push_back({L"region_" + std::to_wstring(rx) + L"_" + std::to_wstring(ry) + L".avrg",
                                   recordRegion(region, c.seed, flags, rx, ry)});
    c.creatures = recordCreatures(world, c.creatureCount);
    return true;
}

// read() under the world lock (§5.3), with a bad pointer caught (this function holds no
// objects of its own, as structured exception handling needs).
bool capture(uintptr_t game, Capture &c, const char *&why) {
    auto *lock = (CRITICAL_SECTION *)(game + WORLD + WORLD_LOCK);
    EnterCriticalSection(lock);
    bool ok = false;
    __try {
        ok = read(game, c, why);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        why = "a pointer could not be read";
    }
    LeaveCriticalSection(lock);
    return ok;
}

// Every creature now, under the world lock, guarded against a bad pointer (for tracking).
bool readCreatures(uintptr_t game, std::vector<uint8_t> &body, uint32_t &count, int32_t &timeOfDay) {
    const uintptr_t world = game + WORLD;
    timeOfDay = peek<int32_t>(world + TIME_OF_DAY);
    body = recordCreatures(world, count);
    return true;
}
bool sampleCreatures(uintptr_t game, std::vector<uint8_t> &body, uint32_t &count, int32_t &timeOfDay) {
    auto *lock = (CRITICAL_SECTION *)(game + WORLD + WORLD_LOCK);
    EnterCriticalSection(lock);
    bool ok = false;
    __try {
        ok = readCreatures(game, body, count, timeOfDay);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    LeaveCriticalSection(lock);
    return ok;
}

// Tracking: every creature ten times a second for 20 s, as "AVCT", u32 version 1, u32 samples,
// then per sample u32 ms since the start, i32 time of day, u32 count and the creatures as in
// creatures.avcr (u64 id and the 0x1168-byte record each).
void track(uintptr_t game) {
    std::vector<uint8_t> out;
    const uint32_t version = 1, samples = 200;
    put(out, "AVCT", 4), put(out, &version, 4), put(out, &samples, 4);
    const DWORD start = GetTickCount();
    for (uint32_t k = 0; k < samples; k++) {
        std::vector<uint8_t> body;
        uint32_t count = 0;
        int32_t timeOfDay = 0;
        if (!sampleCreatures(game, body, count, timeOfDay))
            count = 0, body.clear();
        const uint32_t ms = GetTickCount() - start;
        put(out, &ms, 4), put(out, &timeOfDay, 4), put(out, &count, 4);
        put(out, body.data(), body.size());
        Sleep(100);
    }
    writeFile(L"track.avct", out);
    log("tracked creatures for %u samples", samples);
}

void dump(uintptr_t game) {
    Capture c;
    const char *why = "";
    if (!capture(game, c, why)) {
        log("no dump: %s", why);
        return;
    }
    for (const auto &[name, bytes] : c.zones)
        writeFile(name, bytes);
    std::vector<uint8_t> creatures;
    const uint32_t version = 1;
    put(creatures, "AVCR", 4), put(creatures, &version, 4), put(creatures, &c.creatureCount, 4);
    put(creatures, c.creatures.data(), c.creatures.size());
    writeFile(L"creatures.avcr", creatures);
    if (FILE *f = _wfopen((folder + L"capture.txt").c_str(), L"w")) {
        std::fprintf(f, "seed %u\nday %d\ntime %d\nplayer %.3f %.3f %.3f\nzones %zu\ncreatures %u\n", c.seed, c.day,
                     c.timeOfDay, c.player[0] / 65536.0, c.player[1] / 65536.0, c.player[2] / 65536.0, c.zones.size(),
                     c.creatureCount);
        std::fclose(f);
    }
    log("dumped seed %u: %zu zones, %u creatures, player at (%.1f, %.1f, %.1f)", c.seed, c.zones.size(),
        c.creatureCount, c.player[0] / 65536.0, c.player[1] / 65536.0, c.player[2] / 65536.0);
}

// The game object once it is up (§1.1), or 0.
uintptr_t game() {
    __try {
        const uintptr_t g = peek<uintptr_t>(rebased(GAME_GLOBAL));
        if (g && peek<uint32_t>(g) == rebased(GAME_VTABLE) && peek<uint8_t>(g + THREADS_RUN) == 1)
            return g;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return 0;
}

bool ourWindowInFront() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

// The test helper: moves the local player, under the world lock.
void movePlayer(uintptr_t game, const double (&at)[3]) {
    auto *lock = (CRITICAL_SECTION *)(game + WORLD + WORLD_LOCK);
    EnterCriticalSection(lock);
    __try {
        if (const uintptr_t player = peek<uintptr_t>(game + PLAYER))
            for (int k = 0; k < 3; k++)
                *(int64_t *)(player + RECORD + 8 * k) = (int64_t)(at[k] * 65536.0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    LeaveCriticalSection(lock);
}

void teleport(uintptr_t game, const std::wstring &path) {
    double at[3] = {};
    FILE *file = _wfopen(path.c_str(), L"r");
    if (!file)
        return;
    const int read = fscanf(file, "%lf %lf %lf", &at[0], &at[1], &at[2]);
    fclose(file);
    DeleteFileW(path.c_str());
    if (read == 3) {
        movePlayer(game, at);
        log("teleported to (%.1f, %.1f, %.1f)", at[0], at[1], at[2]);
    }
}

DWORD WINAPI watch(void *) {
    log("extractor loaded at base %p", (void *)base);
    const std::wstring trigger = folder + L"dump.now", tracking = folder + L"track.now";
    const std::wstring moving = folder + L"teleport.now";
    bool held = false;
    for (;;) {
        Sleep(100);
        const bool f9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0 && ourWindowInFront();
        const bool file = GetFileAttributesW(trigger.c_str()) != INVALID_FILE_ATTRIBUTES;
        if ((f9 && !held) || file) {
            if (file)
                DeleteFileW(trigger.c_str());
            if (const uintptr_t g = game())
                dump(g);
            else
                log("no dump: the game is still starting");
        }
        held = f9;
        if (GetFileAttributesW(moving.c_str()) != INVALID_FILE_ATTRIBUTES)
            if (const uintptr_t g = game())
                teleport(g, moving);
        if (GetFileAttributesW(tracking.c_str()) != INVALID_FILE_ATTRIBUTES) {
            DeleteFileW(tracking.c_str());
            if (const uintptr_t g = game())
                track(g);
        }
    }
}

} // namespace

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        base = (uintptr_t)GetModuleHandleW(nullptr);
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        folder = path;
        folder = folder.substr(0, folder.find_last_of(L"\\/") + 1) + L"AlphaExtract\\";
        CreateDirectoryW(folder.c_str(), nullptr);
        // Only the thread starts here: DllMain runs under the loader lock (§5.3).
        if (HANDLE t = CreateThread(nullptr, 0, watch, nullptr, 0, nullptr))
            CloseHandle(t);
    }
    return TRUE;
}
