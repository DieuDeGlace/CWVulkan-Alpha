// Light-write trace. Every RVA, register, frame offset and instruction length below is cited from
// docs/specs/alpha-native-light-write-observation-points.md (the held observation-point contract).
//
// Each observation point is a detour: the point's first whole instructions (all position
// independent, with no branch target after the first byte, per the contract's instruction table)
// are copied into a stub that saves every register and the flags, records one event, restores
// them, runs the copied instructions and jumps back. The handler never blocks, takes no game lock
// and calls no game routine (contract, "Safety limits"); events get a global sequence number from
// one interlocked counter, so the order across the mesh worker, streaming and main threads is the
// order in which the points were reached.
#define _CRT_SECURE_NO_WARNINGS
#include "LightTrace.h"
#include <windows.h>
#include <cstdio>
#include <vector>

namespace light_trace {
namespace {

// Supported image ("Supported image").
constexpr uint32_t IMAGE_TIMESTAMP = 0x51EA955E, IMAGE_SIZE = 0x003BC000;
constexpr uint32_t CLIENT_POINTER = 0x36B1C8; // RVA holding the client once constructed
constexpr uintptr_t WORLD = 0x2E4;            // world embedded in the client

constexpr int MAX_PROBES = 8, MAX_SETS = 16;
constexpr uint32_t CAPACITY = 1u << 18;      // events kept; later ones are counted as dropped
constexpr int32_t UNKNOWN = INT32_MIN;

#pragma pack(push, 1)
struct Event {                 // 88 bytes, little endian, as written to AVLT version 2
    uint32_t seq;              // 1-based global order; 0 while still being written
    uint16_t kind;
    uint16_t probeMask;        // bit k: probe k was stored at this moment and its bytes are valid
    uint32_t thread;
    uint32_t probeSet;         // which probe list the probe bytes belong to
    int64_t qpc;
    int32_t v[8];              // per-kind values (tools/alpha-extractor/README.md lists them)
    uint8_t probe[MAX_PROBES][4];
};
#pragma pack(pop)
static_assert(sizeof(Event) == 88, "AVLT event layout");

struct Regs { // pushad then pushfd, from low to high addresses
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax, eflags;
    uint32_t sp() const { return esp + 4; } // the point's own ESP (pushfd preceded pushad)
};

struct Point {
    uint32_t rva;
    uint8_t lengths[4]; // whole instructions relocated into the stub (sum at least 5)
    Kind kind;
    int32_t sub;        // world-step route for StepTest: 0 bomb, 1 building, 2 variable radius
};

// "Instruction properties at each point".
const Point POINTS[] = {
    {0x19A0E0, {1, 2, 3}, LightEnter, 0},
    {0x19A772, {3, 3}, LightPreCommit, 0},
    {0x19A7EE, {1, 1, 1, 2}, LightExit, 0},
    {0x06F5FC, {1, 1, 2, 1}, AdmitYes, 0},
    {0x06F607, {1, 1, 2, 1}, AdmitNo, 0},
    {0x09DB10, {7}, BuildLocked, 0},
    {0x09DB55, {2, 4}, RelightChosen, 0},
    {0x09DB89, {4, 2}, BuildLightDone, 0},
    {0x20EFDE, {6}, StepTest, 0},
    {0x2117FD, {6}, StepTest, 1},
    {0x213EF5, {6}, StepTest, 2},
    {0x04E7A0, {1, 2, 3}, CellWriteEnter, 0},
    {0x04E877, {3, 1, 1}, CellWriteExit, 0},
    {0x071920, {1, 2, 1, 3}, ClearEnter, 0},
    {0x0719D0, {1, 1, 1, 3}, ClearStored, 0},
};
constexpr size_t POINT_COUNT = sizeof POINTS / sizeof POINTS[0];

uintptr_t base = 0;
bool active = false;
Event *events = nullptr;
volatile LONG next = 0;    // events claimed so far (may exceed CAPACITY: the excess is dropped)
// Probe lists are written once and then published by index, so a handler never sees a partial list.
struct ProbeSet {
    int count;
    int32_t cells[MAX_PROBES][3];
};
ProbeSet sets[MAX_SETS];
volatile LONG setCount = 0, currentSet = 0;

bool readProbeSet(const std::wstring &path, ProbeSet &out) {
    FILE *list = _wfopen(path.c_str(), L"r");
    if (!list)
        return false;
    out.count = 0;
    while (out.count < MAX_PROBES &&
           std::fscanf(list, "%d %d %d", &out.cells[out.count][0], &out.cells[out.count][1], &out.cells[out.count][2]) == 3)
        out.count++;
    std::fclose(list);
    return true;
}
__declspec(thread) bool skippedWrite = false; // this thread's cell write targets an unpublished zone

template <typename T> T peek(uintptr_t p) { return *(const T *)p; }
uint32_t rva(uint32_t address) { return address - (uint32_t)base; }

uintptr_t world() {
    const uintptr_t client = peek<uintptr_t>(base + CLIENT_POINTER);
    return client ? client + WORLD : 0;
}

// "Cell bytes and addressing": the zone, or 0 when it is not resident. Unlocked, as the contract's
// safety limits require; the reads describe what was resident when this event ran.
uintptr_t zoneAt(uintptr_t w, int32_t zx, int32_t zy) {
    if (zx < 0 || zy < 0 || zx >= 65536 || zy >= 65536)
        return 0;
    const uintptr_t region = peek<uintptr_t>(w + 0xBC + 4 * (1024 * (size_t)(zx / 64) + (size_t)(zy / 64)));
    return region ? peek<uintptr_t>(region + 0x10018 + 4 * (64 * (size_t)(zx % 64) + (size_t)(zy % 64))) : 0;
}

bool cell(uintptr_t w, const int32_t (&at)[3], uint8_t (&out)[4]) {
    if (at[0] < 0 || at[1] < 0 || at[0] > 16777215 || at[1] > 16777215)
        return false;
    const uintptr_t zone = zoneAt(w, at[0] / 256, at[1] / 256);
    if (!zone)
        return false;
    const uintptr_t column = peek<uintptr_t>(zone + 0xA8) + 0x20 * ((size_t)(at[0] % 256) + 256 * (size_t)(at[1] % 256));
    const int32_t index = at[2] - peek<int32_t>(column + 0x10);
    const uintptr_t cells = peek<uintptr_t>(column + 0x18);
    if (!cells || index < 0 || index >= peek<int32_t>(column + 0x1C))
        return false;
    *(uint32_t *)out = peek<uint32_t>(cells + 4 * (size_t)index);
    return true;
}

// Guarded reads: a stale pointer gives UNKNOWN instead of a fault in the game's thread.
int32_t read32(uintptr_t p) {
    __try {
        return peek<int32_t>(p);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return UNKNOWN;
    }
}

uint16_t readProbes(Event &e) {
    uint16_t mask = 0;
    e.probeSet = (uint32_t)currentSet;
    const ProbeSet &set = sets[e.probeSet];
    __try {
        if (const uintptr_t w = world())
            for (int k = 0; k < set.count; k++)
                if (cell(w, set.cells[k], e.probe[k]))
                    mask |= (uint16_t)(1u << k);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return mask;
}

// Zones resident in the 3 x 3 block round the containing zone (bit 3 * (dx + 1) + (dy + 1)).
int32_t presence(int32_t zx, int32_t zy) {
    int32_t bits = 0;
    __try {
        if (const uintptr_t w = world())
            for (int dx = -1; dx <= 1; dx++)
                for (int dy = -1; dy <= 1; dy++)
                    if (zoneAt(w, zx + dx, zy + dy))
                        bits |= 1 << (3 * (dx + 1) + (dy + 1));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        bits = UNKNOWN;
    }
    return bits;
}

void record(Kind kind, const int32_t (&v)[8]) {
    const LONG index = InterlockedIncrement(&next) - 1;
    if ((uint32_t)index >= CAPACITY)
        return;
    Event &e = events[index];
    e.kind = (uint16_t)kind;
    e.thread = GetCurrentThreadId();
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    e.qpc = t.QuadPart;
    for (int k = 0; k < 8; k++)
        e.v[k] = v[k];
    e.probeMask = readProbes(e);
    InterlockedExchange((volatile LONG *)&e.seq, index + 1); // published last
}

// Signed truncating division, as the admission routine finds the containing zone.
int32_t zoneOf(int32_t chunk) { return (int32_t)(32 * (int64_t)chunk / 256); }

// The worker re-asks admission for every waiting candidate on each pass. Only the mesh worker
// runs admission, so this table needs no lock. A denial is recorded when a chunk's resident 3 x 3
// block differs from its last recorded denial; v[5] then counts the identical denials skipped.
struct Denial {
    int32_t x, y, presence;
    uint32_t repeats;
    bool used;
};
Denial denials[1 << 16];

Denial &denialFor(int32_t x, int32_t y) {
    uint32_t h = ((uint32_t)x * 0x9E3779B1u) ^ ((uint32_t)y * 0x85EBCA77u);
    for (uint32_t probe = 0;; probe++) {
        Denial &d = denials[(h + probe) & 0xFFFF];
        if (!d.used || (d.x == x && d.y == y) || probe == 64) {
            if (!d.used || d.x != x || d.y != y)
                d = {x, y, INT32_MIN, 0, true};
            return d;
        }
    }
}

void __cdecl observe(uint32_t point, Regs *r) {
    if (!active)
        return;
    const Point &p = POINTS[point];
    const uint32_t sp = r->sp(), bp = r->ebp;
    int32_t v[8] = {UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN};
    switch (p.kind) {
    case LightEnter: // "The light routine as a common point": ECX world, caller, six arguments
        v[0] = (int32_t)rva((uint32_t)read32(sp));
        for (int k = 1; k <= 6; k++)
            v[k] = read32(sp + 4 * k);
        break;
    case LightPreCommit:
    case LightExit: // low X is no longer an argument at the exit; pair with this thread's entry
        v[0] = (int32_t)rva((uint32_t)read32(bp + 4));
        for (int k = 2; k <= 6; k++)
            v[k] = read32(bp + 0xC + 4 * (k - 2));
        break;
    case AdmitYes:
    case AdmitNo: // "Admission": [EBP+8] chunk X, [EBP+0xC] chunk Y; presence re-read unlocked
        v[0] = read32(bp + 8), v[1] = read32(bp + 0xC);
        if (v[0] != UNKNOWN && v[1] != UNKNOWN) {
            v[3] = zoneOf(v[0]), v[4] = zoneOf(v[1]);
            v[2] = presence(v[3], v[4]);
            Denial &d = denialFor(v[0], v[1]);
            if (p.kind == AdmitNo && d.presence == v[2]) {
                d.repeats++;
                return;
            }
            v[5] = (int32_t)d.repeats;
            d.presence = p.kind == AdmitNo ? v[2] : INT32_MIN;
            d.repeats = 0;
        }
        break;
    case BuildLocked: { // cache slot at [ESP+0xB0]; its dirty byte (+0x74) can still change
        v[0] = read32(bp + 8), v[1] = read32(bp + 0xC);
        const int32_t slot = read32(sp + 0xB0);
        v[2] = slot;
        if (slot != UNKNOWN && slot)
            v[3] = read32((uint32_t)slot + 0x74) & 0xFF;
        break;
    }
    case RelightChosen: // DL is the dirty byte the builder used; origin X [ESP+0x78], Y in EDI
        v[0] = read32(bp + 8), v[1] = read32(bp + 0xC);
        v[2] = (int32_t)(r->edx & 0xFF);
        v[3] = read32(sp + 0x78), v[4] = (int32_t)r->edi;
        break;
    case BuildLightDone:
        v[0] = read32(bp + 8), v[1] = read32(bp + 0xC);
        break;
    case StepTest: { // "World-step decision points": world at [EBP-0x2B18], test of world +0xB8
        v[0] = p.sub;
        const int32_t w = read32(bp - 0x2B18);
        v[1] = w == UNKNOWN || !w ? UNKNOWN : read32((uint32_t)w + 0xB8);
        if (p.sub == 1)
            v[2] = read32(bp - 0x2B50);
        break;
    }
    case CellWriteEnter: { // "Edit writers": X, Y, Z, record pointer, explicit zone
        const int32_t zone = read32(sp + 0x14);
        skippedWrite = zone != 0; // generation into an unpublished zone: not a resident write
        if (skippedWrite)
            return;
        for (int k = 0; k < 3; k++)
            v[k] = read32(sp + 4 + 4 * k);
        const int32_t source = read32(sp + 0x10);
        v[3] = source == UNKNOWN || !source ? UNKNOWN : read32((uint32_t)source);
        v[4] = zone;
        v[5] = (int32_t)rva((uint32_t)read32(sp));
        break;
    }
    case CellWriteExit:
        if (skippedWrite)
            return;
        break;
    case ClearEnter:
        for (int k = 0; k < 3; k++)
            v[k] = read32(sp + 4 + 4 * k);
        v[5] = (int32_t)rva((uint32_t)read32(sp));
        break;
    default:
        break;
    }
    record(p.kind, v);
}

// Opcodes that would make a relocated instruction position dependent or a branch.
bool relocatable(const uint8_t *code) {
    const uint8_t op = code[0];
    if (op == 0xE8 || op == 0xE9 || op == 0xEB || (op >= 0x70 && op <= 0x7F) || (op >= 0xE0 && op <= 0xE3))
        return false;
    return !(op == 0x0F && code[1] >= 0x80 && code[1] <= 0x8F);
}

void emit(std::vector<uint8_t> &out, std::initializer_list<uint8_t> bytes) { out.insert(out.end(), bytes); }
void emit32(std::vector<uint8_t> &out, uint32_t value) {
    for (int k = 0; k < 4; k++)
        out.push_back((uint8_t)(value >> (8 * k)));
}

} // namespace

bool installed() { return active; }

bool install(uintptr_t moduleBase, const std::wstring &folder, const char *&why) {
    base = moduleBase;
    const auto *dos = (const IMAGE_DOS_HEADER *)base;
    const auto *nt = (const IMAGE_NT_HEADERS32 *)(base + dos->e_lfanew);
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || nt->FileHeader.TimeDateStamp != IMAGE_TIMESTAMP ||
        nt->OptionalHeader.SizeOfImage != IMAGE_SIZE) {
        why = "Cube.exe is not the supported Alpha image";
        return false;
    }
    if (peek<uintptr_t>(base + CLIENT_POINTER)) {
        why = "the game was already running; the trace installs only at load";
        return false;
    }
    if (!readProbeSet(folder + L"lighttrace.txt", sets[0])) {
        why = "no lighttrace.txt";
        return false;
    }
    setCount = 1;

    events = (Event *)VirtualAlloc(nullptr, sizeof(Event) * CAPACITY, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    auto *stubs = (uint8_t *)VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!events || !stubs) {
        why = "no memory for the trace";
        return false;
    }
    for (size_t i = 0; i < POINT_COUNT; i++) {
        uint8_t *target = (uint8_t *)(base + POINTS[i].rva);
        size_t length = 0;
        for (uint8_t n : POINTS[i].lengths) {
            if (n && !relocatable(target + length)) {
                why = "an observation point does not hold the expected instructions";
                return false;
            }
            length += n;
        }
        if (length < 5) {
            why = "an observation point is shorter than a jump";
            return false;
        }
    }
    // Build every stub first, then patch, so a refusal leaves the game untouched.
    std::vector<std::vector<uint8_t>> code(POINT_COUNT);
    uint8_t *cursor = stubs;
    std::vector<uint8_t *> stubAt(POINT_COUNT);
    for (size_t i = 0; i < POINT_COUNT; i++) {
        const uintptr_t target = base + POINTS[i].rva;
        size_t length = 0;
        for (uint8_t n : POINTS[i].lengths)
            length += n;
        auto &c = code[i];
        stubAt[i] = cursor;
        emit(c, {0x9C, 0x60, 0x54, 0x68}); // pushfd; pushad; push esp; push point
        emit32(c, (uint32_t)i);
        c.push_back(0xE8); // call observe
        emit32(c, (uint32_t)((uintptr_t)&observe - ((uintptr_t)cursor + c.size() + 4)));
        emit(c, {0x83, 0xC4, 0x08, 0x61, 0x9D}); // add esp, 8; popad; popfd
        c.insert(c.end(), (const uint8_t *)target, (const uint8_t *)target + length);
        c.push_back(0xE9); // jmp back past the relocated instructions
        emit32(c, (uint32_t)((target + length) - ((uintptr_t)cursor + c.size() + 4)));
        if (cursor + c.size() > stubs + 4096) {
            why = "stub space exhausted";
            return false;
        }
        memcpy(cursor, c.data(), c.size());
        cursor += (c.size() + 15) & ~size_t(15);
    }
    FlushInstructionCache(GetCurrentProcess(), stubs, 4096);
    active = true;
    for (size_t i = 0; i < POINT_COUNT; i++) {
        uint8_t *target = (uint8_t *)(base + POINTS[i].rva);
        size_t length = 0;
        for (uint8_t n : POINTS[i].lengths)
            length += n;
        DWORD old;
        VirtualProtect(target, length, PAGE_EXECUTE_READWRITE, &old);
        target[0] = 0xE9;
        *(uint32_t *)(target + 1) = (uint32_t)((uintptr_t)stubAt[i] - ((uintptr_t)target + 5));
        for (size_t k = 5; k < length; k++)
            target[k] = 0x90;
        VirtualProtect(target, length, old, &old);
    }
    FlushInstructionCache(GetCurrentProcess(), (void *)base, IMAGE_SIZE);
    return true;
}

void mark(Kind kind) {
    if (!active)
        return;
    int32_t v[8] = {UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN};
    if (kind == ProbesChanged)
        v[0] = (int32_t)currentSet;
    record(kind, v);
}

bool replaceProbes(const std::wstring &path) {
    if (!active || setCount >= MAX_SETS)
        return false;
    const LONG index = setCount; // only the extractor's thread adds sets
    if (!readProbeSet(path, sets[index]))
        return false;
    setCount = index + 1;
    InterlockedExchange(&currentSet, index);
    mark(ProbesChanged);
    return true;
}

// AVLT version 2: "AVLT", u32 version, u32 image timestamp, u32 module base, u32 set count, then
// each probe set as u32 count and that many (i32 X, Y, Z), i64 performance-counter frequency,
// u32 claimed, u32 dropped, u32 incomplete, u32 written, then the written events in sequence order.
bool save(const std::wstring &path) {
    if (!active)
        return false;
    const uint32_t claimed = (uint32_t)next;
    const uint32_t kept = claimed < CAPACITY ? claimed : CAPACITY;
    std::vector<Event> done;
    done.reserve(kept);
    for (uint32_t i = 0; i < kept; i++)
        if (events[i].seq)
            done.push_back(events[i]);
    FILE *f = _wfopen(path.c_str(), L"wb");
    if (!f)
        return false;
    const uint32_t version = 2, stamp = IMAGE_TIMESTAMP, moduleBase = (uint32_t)base, count = (uint32_t)setCount;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    const uint32_t dropped = claimed - kept, incomplete = kept - (uint32_t)done.size(),
                   written = (uint32_t)done.size();
    std::fwrite("AVLT", 1, 4, f);
    for (uint32_t value : {version, stamp, moduleBase, count})
        std::fwrite(&value, 4, 1, f);
    for (uint32_t s = 0; s < count; s++) {
        const uint32_t n = (uint32_t)sets[s].count;
        std::fwrite(&n, 4, 1, f);
        std::fwrite(sets[s].cells, sizeof sets[s].cells[0], n, f);
    }
    std::fwrite(&frequency.QuadPart, 8, 1, f);
    for (uint32_t value : {claimed, dropped, incomplete, written})
        std::fwrite(&value, 4, 1, f);
    std::fwrite(done.data(), sizeof(Event), done.size(), f);
    std::fclose(f);
    return true;
}

} // namespace light_trace
