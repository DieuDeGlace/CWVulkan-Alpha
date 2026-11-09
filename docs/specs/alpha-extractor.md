# Alpha in-process extractor: where the ground truth lives

Specification for a small **32-bit mod DLL** that the Alpha mod loader `LoadLibrary`s into
`Cube.exe` (SHA-256 `84a7a132…30bf`) and that, on a hotkey, dumps the client's world state so
AlphaVulkan can be compared with it 1:1. It says where every object is, how long it lives,
which thread writes it and which lock to hold while reading. It is written from the client
decompile and assembly (`disassembled alpha cube world/`); nothing here is code from the game.

Read with: [alpha-worldgen-overview.md](alpha-worldgen-overview.md) (world object, units),
[alpha-worldgen-terrain.md](alpha-worldgen-terrain.md) §1 (zone, column, block),
[alpha-net-packets.md](alpha-net-packets.md) §1–§2 (item and creature records),
[alpha-interaction.md](alpha-interaction.md) §4.1 (statics),
[alpha-world-items.md](alpha-world-items.md) §1 (world items).

---

## 0. Conventions

* Addresses are those of `Cube.exe` at its **preferred** base 0x400000, as in every other
  spec ("0x46a8a0"). RVA = address − 0x400000.
* **The executable is relocatable.** Its optional header has DllCharacteristics 0x8140
  (DYNAMIC_BASE set) and a `.reloc` section (RVA 0x371000). Windows may load it elsewhere.
  At run time every address must be rebased: `actual = (uintptr_t)GetModuleHandleW(NULL) +
  (address − 0x400000)`. Pointers read from memory are already actual addresses.
* **G** = the game object (class `cube::GameController`), **W** = the world object
  (`cube::World`) = G + 0x2e4. "W+x" and "G+(0x2e4+x)" are the same byte.
* Containers are the Visual C++ 2012 standard library (`MSVCR110.DLL` is imported):
  * `vector<T>`: 12 bytes = begin pointer, end pointer, capacity-end pointer.
    Element count = (end − begin) / sizeof(T).
  * `list<T>`: 8 bytes = head (sentinel) node pointer, element count. Node: +0x00 next,
    +0x04 previous, +0x08 the element. Walk from head→next until back at head.
  * `map<K,V>`: 8 bytes = head (sentinel) node pointer, element count. Node: +0x00 left,
    +0x04 parent, +0x08 right, +0x0c colour (u8), +0x0d "is sentinel" (u8), +0x10 the key,
    then the value. Head→left is the smallest key, head→parent the root, head→right the
    largest. In-order walk: standard red-black successor, stopping at a node whose +0x0d
    byte is non-zero.
  * narrow `string`: 24 bytes = 16-byte inline buffer (or, when capacity > 15, a heap
    pointer in its first 4 bytes), length (+0x10), capacity (+0x14).
* Every class below has a vtable; checking `*(u32*)obj == rebased vtable` is a cheap sanity
  test before trusting a pointer (vtables from the RTTI in `symbols.csv`):

| Class | vtable |
|---|---|
| cube::GameController (G) | 0x6ffdc8 |
| cube::World (W) | 0x71a794 |
| cube::Region | 0x71dccc |
| cube::Zone | 0x71e280 |
| cube::Field (a column) | 0x6ffd90 |
| cube::Creature | 0x6fd8cc |
| cube::Spawn | 0x71dbec |
| cube::WorldMap | 0x71dfcc |

---

## 1. The root

### 1.1 The game object pointer

* **Global 0x76b1c8** (RVA 0x36b1c8, `.data`) holds G. WinMain 0x4c8ae0 allocates 0x1001018
  bytes, runs the constructor 0x459c40 on them and stores the result there (0x4c8ae0, before
  the message loop). It is null until the constructor returns. The constructor runs a long
  time (it loads every asset), so a DLL loaded early sees null for a while.
* G is never moved or freed until the game quits (WinMain deletes it after the loop ends).
* Other globals the frame function tests: 0x76b1b4 (the Direct3D device; the frame does
  nothing while it is null), 0x76b1c0 (the window).
* **W is embedded in G** at G+0x2e4; it never moves. A second world-like object, believed
  to be the `cube::WorldMap` (it has the same lock offsets and is used by the map thread),
  is embedded at G+0x800d44. The extractor does not need it.

When the DLL may start reading: G ≠ null, `*(u32*)G` = rebased 0x6ffdc8, and the byte
G+0x800584 = 1 (the "worker threads run" flag set just before the worker threads start,
0x459c40; the threads stop when it is cleared).

### 1.2 A world is loaded even on the start screen

At start-up the client seeds a world with `timeGetTime()` (G+0x800a50, 0x45a72d) and the
start screen draws its terrain behind the menu (`alpha-ui-screens.md` §3.3). So zones exist
before any character enters a world, and the **local player object exists from start-up**
(0x459c40 creates it with id 1 and the name "Wollay", G+0x8006d0). "Zones present" and
"player pointer non-null" therefore do **not** mean "in a world".

### 1.3 Safe "in a world" test

All of the following, read on any thread (they are single aligned words):

1. **Start screen closed.** Visibility of the start-screen root node G+0x800874 is 0.
   Visibility of an interface node N (as read by 0x488bd0 / 0x488ee0): let A = *(N+0x3c);
   visible = *( *(A+0x94) + 4 · *(A+0x68) ) ≠ 0.
2. **Game interface up.** The in-game root G+0x800884 is visible, **or** the map root
   G+0x8008a0 is visible (the map frame hides the in-game root, `alpha-ui-screens.md`
   §5.2.3 step 8). The in-game root is made visible only when the start-screen root, the
   character/world/connect screens and the "Please wait..." label are all hidden
   (0x4ae2f5..0x4ae356), so this also excludes character select and world loading.
3. **The world is the requested one.** W's seed (W+0x800164 = G+0x800448) equals the
   requested seed G+0x800a50, and W's name (string at W+0x94 = G+0x378) equals the
   requested name (string at G+0x800a54). While they differ the streaming thread is about
   to re-initialise the whole world (§5.2), so nothing may be dumped.
4. **The player's zone exists.** The zone containing the local player's position (§3.3,
   §2.1 lookup) is non-null.

Condition 2 is inferred from the interface specs and not observed on a live game; log the
three visibility values with every dump until it is confirmed (open question X-1).

---

## 2. Zones and blocks

### 2.1 Where zones are stored

There is no list or hash of loaded zones. Zones hang off a fixed two-level grid in W:

```
W+0x00000bc  Region*  regions[1024][1024]        index = rx*1024 + ry   (x-major!)
W+0x04000bc  Cell*    regionCells[1024][1024]    (regions spec; 0x1c-byte records)
Region (0x15a28 bytes, vtable 0x71dccc):
  +0x00018   16-byte per-zone table [64][64]     (regions/locations specs)
  +0x10018   Zone*    zones[64][64]               index = (zx mod 64)*64 + (zy mod 64)  (x-major!)
  +0x14018   location records [8][8] × 0x68
```

Lookup of zone (zx, zy), 0 ≤ zx, zy < 65536 (0x434d10 → 0x434a90): rx = zx >> 6,
ry = zy >> 6; region = W.regions[rx·1024 + ry]; if null, no zone; else
zone = region.zones[(zx & 63)·64 + (zy & 63)]. A null pointer means "not loaded".

To enumerate every loaded zone, scan the 1M region pointers (4 MB, a few milliseconds) and
each non-null region's 4096 zone pointers; or, cheaper, scan zx, zy within ±4 of the
player's zone (nothing is kept loaded further away, §5.2).

### 2.2 Zone record (200 bytes, constructor 0x6240f0, vtable 0x71e280)

| Off | Type | Meaning |
|---|---|---|
| +0x00 | ptr | vtable |
| +0x04 | list<sprite> | **sprites**: plant sprites, tree records, lanterns, cobwebs … (0x3c-byte elements, §2.6) |
| +0x0c | vector<static> | **statics**, 0x188-byte records inline (§2.6) |
| +0x18 | vector<Spawn*> | **spawn objects** (pointers, §2.6) |
| +0x24 | vector<?> | named "plant sprites" in the terrain spec; see open question X-3 |
| +0x30 | vector<item> | **world items**, 0x148-byte records inline |
| +0x3c | vector<16 bytes> | object list: (i32 n, i32 Bx, i32 By, i32 Bz) (`alpha-block-edits.md`) |
| +0x48 | vector<?> | a structure list (0x5f4ba0; `alpha-worldgen-structures.md`) |
| +0x54 | vector<?> | not traced |
| +0x60, +0x64 | i32, i32 | **zx, zy** |
| +0x68 | vector<20 bytes> | stored block edits of this zone (net §3.5 layout) |
| +0x6c | i32 | not traced (a camera distance is suspected, `alpha-audio.md` AU-3) |
| +0x74 | u8 | "dynamic data requested" (constructor sets 1) |
| +0x75 | u8 | "items changed" |
| +0x78 … +0x87 | 16 bytes | copy of the region's per-zone entry; +0x80 = zone level, +0x84 = sub-level |
| +0x88 | vector<?> | house list (structures spec §5.5.4) |
| +0x94 | vector<?> | not traced |
| +0xa0 | i32 | number of spawn objects the generator made (`alpha-missions.md`) |
| +0xa8 | Field* | **65536 columns**, 32 bytes each (§2.3) |
| +0xac | ptr | 32 × 32 cell index, 8-byte list heads (`alpha-interaction.md` §3) |
| +0xb0 | CRITICAL_SECTION | per-zone lock; initialised and deleted, **no user found**: do not rely on it |

**Generation done.** There is no flag. The generator 0x5e4850 builds the zone completely
(terrain, every post pass, stored edits, block light) and stores the zone pointer into
`region.zones[]` as its **last** act (plain 32-bit store, no lock; end of 0x5e4850 just
before 0x5ee0b8). A non-null pointer therefore means a complete zone. (After that the
streaming thread sets byte +0x28 of the WorldMap's entry for the zone to 1, 0x46acf8.)

### 2.3 Columns and blocks

Column (x, y) of a zone, with lx = x mod 256, ly = y mod 256 (0x4347a0):

    column = zone.columns + 32 · (lx + 256 · ly)          (x varies fastest — unlike regions/zones)

Column (`cube::Field`, 32 bytes; constructor at 0x450bf0, destructor 0x450c30):

| Off | Type | Meaning |
|---|---|---|
| +0x00 | ptr | vtable 0x6ffd90 (same in every column; skip) |
| +0x04 | f32 | moisture |
| +0x08 | f32 | warmth |
| +0x0c | f32 | fire share |
| +0x10 | i32 | **bottom**: z of the first stored block |
| +0x14 | i32 | surface z − 8 (use not traced) |
| +0x18 | ptr | **blocks**, 4 bytes each, z = bottom, bottom+1, … |
| +0x1c | i32 | **count** of stored blocks |

Block at height z (0x42f7e0 / 0x42f730): if z < bottom → implicit solid grey (200, 200, 200,
type 1); if z ≥ bottom + count → implicit air (z > 0) or water (z ≤ 0); otherwise the 4 bytes
at blocks + 4·(z − bottom), except that a stored type-0 block at z ≤ 0 without bit 0x40 reads
as water. Block bytes: 0 = R, 1 = G, 2 = B, 3 = type in bits 0–4, 0x20 "surface-coloured",
0x40 "dry air", 0x80 "keep" (terrain spec §1.3–§1.4). **Height range:** any i32; per column it
is [bottom, bottom + count). The extractor should dump the stored range raw (bottom, count,
count × 4 bytes) and apply the implicit rules on the comparison side; that is both smaller and
exact.

A whole zone is then 65536 × (12 bytes of climate + 8 bytes of bottom/count + 4·count). Do
not dump +0x00 or the pointer +0x18.

### 2.4 Block light

Block light is computed into the zone before publication (0x59a0e0, overview §4 step 6;
`alpha-rendering.md` §9.2). Where it is stored was not determined for this spec (open
question X-4); the four block bytes above carry no light.

### 2.5 Who writes zones

| Thread | Writes | Lock held |
|---|---|---|
| streaming thread 0x46a8a0 (§5.1) | builds new zones before publishing them; nulls and deletes far zones and regions; re-initialises the world | none while generating; W.zoneLock+W.worldLock only around the unpublish (§5.2) |
| main thread, frame update 0x488ee0 | block edits from the network or the local player (0x48f242..0x48f3d0), world items (+0x30), static states (+0x0c), spawn list (+0x18), "items changed" | W.worldLock for most of the frame; W.zoneLock in addition while applying block edits |
| chunk mesher thread 0x4690a0 | nothing in zones; reads blocks for meshes (0x49d910) | W.zoneLock only |
| map thread 0x469590 | the WorldMap object only | the WorldMap's own locks |

Whether the generator also writes blocks into neighbouring, already published zones is not
known (overview open question 2); if it does, those writes are unlocked in the game too.

### 2.6 Per-zone lists worth comparing (cheap, deterministic from the seed)

All of these are filled by the generator in a fixed order; compare them **right after
generation** (§5.4), because play changes them.

* **Sprites, zone+0x04** (`list`, element at node+0x08; nodes are 0x48 bytes). Element
  (0x3c bytes used): +0x00 i32 kind; +0x08, +0x10, +0x18 i64 position (1/65536 block); +0x20
  f32 size; +0x24 f32 rotation; +0x2c, +0x30, +0x34 f32 tint; +0x38 i32 mode. Tree records
  (kinds 60, 61) leave +0x20..+0x2b as **stale stack bytes** (`alpha-worldgen-trees.md` §5):
  never compare those bytes for kinds 60/61. Order matters: compare as a sequence.
* **Statics, zone+0x0c** (`vector`, 0x188-byte records; `alpha-interaction.md` §4.1).
  Compare +0x00..+0x47 and +0x54..+0x187. Skip +0x48..+0x53: it is a container with heap
  pointers. Fields +0x30 (state) and +0x34 (timer) change in play.
* **Spawn objects, zone+0x18** (`vector<Spawn*>`; each `cube::Spawn` is 4336 = 0x10f0
  bytes, made by 0x5adb20; `alpha-creatures-behaviour.md` §2.1): +0x08 f32, +0x10 3 × i64
  position, +0x28 i32 count/level, +0x2c i32 creature type, +0x34 i32 level, … Dump the
  pointed-to bytes from +0x08 on (skip the vtable at +0x00); treat any other 4-byte value
  that equals a heap address as unknown until the record is fully specified.
* **World items, zone+0x30** (`vector`, 0x148-byte records; `alpha-world-items.md` §1).
  Compare all but the padding 0x139..0x13b.
* **Stored block edits, zone+0x68** (`vector`, 20-byte records; net §3.5).
* **Zone header**: zx, zy, the 16-byte region entry (+0x78..+0x87), +0xa0.

---

## 3. Creatures

### 3.1 The creature map

**W+0x04 is a `map<u64 id, Creature*>`** (head pointer W+0x04 = G+0x2e8, count W+0x08).
Lookup 0x42f000 (thiscall on W, argument = pointer to the u64 id; returns the Creature* or
0): it searches from W+0x04 with 0x477d90 and returns node+0x18. Keys compare as **signed**
64-bit. Node: +0x10 u64 id, +0x18 Creature*. (W+0x0c/+0x10 is a second map and W+0x14/+0x18
a list; their contents were not traced.)

The map holds every creature this client simulates or has been told about: the local player,
NPCs and monsters made from spawn objects near the player, other players and the server's
creatures when online, pets, bombs. The world step and the network receive code insert and
delete entries; deletion frees the Creature object.

### 3.2 Creature object (`cube::Creature`, 0x1e60 bytes, constructor 0x43b690)

| Off | Size | Meaning |
|---|---|---|
| +0x00 | 4 | vtable 0x6fd8cc |
| +0x04 | 4 | not traced |
| +0x08 | 8 | **u64 id** (the map key) |
| +0x10 | 0x1168 | **the creature record**, byte-for-byte the record of `alpha-net-packets.md` §2.1 (record offset r = object offset 0x10 + r) |
| +0x1178 … +0x1e5f | | client-only state (other specs cite obj offsets: +0x1184, +0x1190, +0x130c, +0x1350, +0x13a4, +0x13e4, +0x1d28 …) |

Evidence for the record placement: the handshake (0x46fc50) receives exactly 0x1168 bytes and
copies them to player + 0x10; the streaming thread writes the player's position at
obj+0x10..+0x27 (= record field 0, 0x46a8a0). So **the 48-field table of the net spec applies
directly at obj+0x10**. Bytes of the record that the wire never carries (e.g. record
0x1ac..0x1af, padding inside fields) are not defined: compare field by field using the net
table, never the raw 0x1168 block.

### 3.3 The local player

* **G+0x8006d0** = the local player's Creature* (valid from start-up, §1.2). On joining a
  server (0x46fc50) every creature in the map is deleted, the player is looked up or
  created under the id the server assigned, inserted in the map, and **W+0xb8** (= G+0x39c)
  is set to it. Elsewhere the specs use W+0xb8 as "the step's own creature". Read both and
  report a mismatch; whether W+0xb8 is set when playing offline was not traced (X-2).
* Position: player + 0x10 (3 × i64, 1/65536 block). Zone = (x >> 24, y >> 24) for
  non-negative positions (256 blocks × 65536). The game's own "player zone" is kept at
  G+0x2bc/G+0x2c0 (`alpha-item-names.md`).

---

## 4. Clock and seed

| Where | Type | Meaning |
|---|---|---|
| W+0x80015c (G+0x800440) | i32 | time of day in ms, 0 … 86,399,999 (World constructor sets 43,200,000 = noon; packet 5 overwrites it online, 0x46cb42) |
| W+0x800160 (G+0x800444) | i32 | day number |
| W+0x800164 (G+0x800448) | u32 | **world seed** in use |
| W+0x800168 … +0x800298 | 77 × i32 | seed offsets (random spec §2.2) |
| W+0x8000f0, +0x8000f4 | f32 | home point |
| W+0x8000bc | i32 | step time accumulator used by vegetation wind (`vegetation-wind.md`) |
| W+0x94 (G+0x378) | string | world name in use (`online_<seed>` when online) |
| G+0x800a50 | u32 | **requested** seed (start-up `timeGetTime()`, then the chosen world or the server's packet 15) |
| G+0x800a54 | string | requested world name |

Dump the seed, the 77 offsets and the clock with every capture; a dump whose W seed ≠
G+0x800a50 is invalid (§1.3).

---

## 5. Threads, locks, lifetimes and the safe moment

### 5.1 Threads of the client

The constructor 0x459c40 starts three `_beginthread` workers through `std::function`
wrappers (0x450e70; bodies reached via the lambda vtables 0x702984, 0x7029a4, 0x7029c4):

| Thread body | Handle stored at | What it does |
|---|---|---|
| 0x4690a0 | G+0x800588 (priority below normal) | chunk meshes for the 3-D view |
| 0x469590 | G+0x80058c (priority below normal) | world-map tiles (WorldMap at G+0x800d44) |
| 0x46a8a0 | G+0x8005b4 | **zone streaming**: generate, unload, re-initialise |

Online, two more threads run the network (bodies 0x469c10 send and 0x46b740 receive, started
from the connect path 0x46fc50/0x47e1b0); the receive code takes W.worldLock (0x46b875).
The **main thread** runs WinMain's loop 0x4c8ae0 → frame 0x4c85f0 → G's virtual update and
draw.

### 5.2 Locks

All are Win32 CRITICAL_SECTIONs (recursive for the owning thread):

| Name here | Address | Taken by |
|---|---|---|
| **W.worldLock** | W+0x8000c0 (enter 0x601cb0, leave 0x601e90, thiscall on W) | main-thread update 0x488ee0 from 0x48902f to 0x49d2ea (nearly the whole update, including the world step 0x60c510); main-thread draw 0x4ac260 from 0x4ac37e to its end; network receive; zone/region unload; world re-initialisation |
| **W.zoneLock** | W+0x8000d8 (enter 0x59c480, leave 0x601ea0) | main thread while applying block edits (0x48f25f, 0x48f5aa) and in the world step (0x60f084); mesher 0x49d910; zone/region unload; re-initialisation |
| G.settingsLock | G+0x8005d0 | requested seed/name, the interest-point list G+0x2c4 and the reference position G+0x2ac..+0x2bb |
| G.meshLock etc. | G+0x8005b8, +0x8005e8, +0x800600, +0x800618 | renderer/mesher/map internals; not needed |

**Unload (0x5a4890, streaming thread):** takes zoneLock **then** worldLock, nulls the zone
pointer, releases both, saves the zone if byte W+0xb4 = 0 (0x5a5050), then deletes it.
Region unload (0x5a4800) and region-cell unload (0x5a4780) take worldLock then zoneLock and
delete after release. **World re-initialisation** (0x46af84..0x46afab): worldLock, zoneLock,
then 0x5a52e0 with the requested seed and name, which clears the grids and frees everything.

**When things are freed** (streaming pass, every 20 ms; the clean-up part runs at most once a
second or right after a world change):

* A zone is unloaded when its squared distance in zone units, (zx − px)² + (zy − py)², is
  ≥ 16 for **every** interest point (px, py) in the list at G+0x2c4 (list nodes carry i32
  zone x, y at node+0x08/+0x0c). So zones up to ±3 around the player survive; (4, 0) away is
  freed.
* A region is deleted when it is ≥ 3 regions away (Chebyshev, in region units) from every
  interest point; a region cell when ≥ 5 away.
* New zones: per pass, one missing zone within ±3 zones of an interest point whose centre is
  nearest the reference position (G+0x2b4, G+0x2b8, blocks), if closer than 768 blocks.
* Creatures are deleted by the world step (despawn), by packet 2 handling and on connect.

So: zone pointers, region pointers, Creature pointers and every vector buffer may become
invalid between two reads. G, W and the grids themselves never move.

### 5.3 Safe reading protocol (no code patching)

From a worker thread the DLL creates (never from DllMain itself; DllMain runs under the
loader lock — only start the thread there):

1. Poll until §1.1 and §1.3 hold.
2. `EnterCriticalSection(W+0x8000c0)` (**worldLock only**).
3. Read everything (zones, creatures, clock, seed). Copy into DLL memory; do file I/O after.
4. `LeaveCriticalSection(W+0x8000c0)`.

Why worldLock alone: every writer of zones or creatures except the mesher (read-only) and the
generator (unpublished zones) holds it — the main thread's update and draw, the network
receive, the unload paths (which need both locks to unpublish) and re-initialisation. Do **not**
also take zoneLock after worldLock from a foreign thread: 0x5a4890 takes them in the opposite
order (zoneLock, then worldLock), so that pair can deadlock. (The game itself takes worldLock
then zoneLock on the main thread; the inversion is in the original.) Holding worldLock blocks
the game's frame, so keep the hold short: a full ±3-zone dump is 49 zones × ~1.5–3 MB of
column data; copy the raw column arrays and block arrays and format later.

### 5.4 Main-thread hook points (optional)

If the dump should be taken on the main thread between frames:

* **G's update, vtable slot 10.** The frame function 0x4c85f0 calls
  `(*G->vtbl[10])(dtMs)` at 0x4c8633: `void __thiscall update(GameController* this /*ECX*/,
  int dtMs)`, implementation **0x488ee0**, returns with `RET 4`. Slot address in the vtable:
  0x6ffdc8 + 0x28 = **0x6ffdf0** (in `.rdata`; `VirtualProtect` it writable, swap the
  pointer, restore). A replacement written as `__fastcall(void* ecx, void* edx, int dt)` that
  calls the original and then dumps is the least invasive hook. After the original returns,
  the main thread holds no world lock; take worldLock as in §5.3 to exclude the network and
  streaming threads. The draw (slot 11, 0x6ffdf4 → 0x4ac260, `__thiscall`, no argument)
  follows in the same frame.
* **Detour of 0x488ee0 itself:** first instructions `55` / `8B EC` / `83 E4 F8` (push ebp;
  mov ebp,esp; and esp,−8) — **6 bytes**, position-independent; overwrite with a 5-byte
  JMP plus one NOP and copy those 6 bytes to the trampoline.
* **Detour of the frame function 0x4c85f0** (`void __cdecl frame(void)`, only caller
  0x4c9432 in WinMain): first instruction `83 3D <abs32> 00` (cmp dword [0x76b1b4], 0) —
  **7 bytes**, a single instruction with an absolute (already relocated) address, safe to copy
  as-is. The detour runs before the update and draw of that frame.
* **Fresh-zone hook: 0x5e4850** (`void __thiscall generateZone(World* this /*ECX*/, int zx,
  int zy)`, `RET 8`, called from the streaming thread at 0x46acd1). First instructions
  `55` / `8B EC` / `6A FF` — **5 bytes** exactly, position-independent. After the original
  returns, look the zone up (§2.1): if it is non-null and not yet dumped, it is the freshest
  possible copy (only the stored edits from a save and block light were applied). Read it
  while holding **zoneLock only** (W+0x8000d8) — the streaming thread never holds worldLock
  at this point and must not be made to take it before zoneLock. Use this for terrain,
  sprite, static, spawn and item comparisons against the generator; use a test world that
  has no saved zones, or delete them first, to avoid stored edits.

---

## 6. What a dump should contain (suggested)

1. Header: game base address, G, seed, 77 offsets, requested seed/name, world name, day,
   time, local player id and record (fields per net table), the §1.3 flag values.
2. Per zone (fresh-zone hook, or every loaded zone under worldLock): zx, zy, region entry,
   +0xa0; 65536 × (moisture, warmth, fire, bottom, +0x14, count, count × 4 block bytes);
   sprite list; statics; spawn objects; world items; stored edits.
3. Creatures: for every map entry, id and the 48 record fields.

---

## 7. Open questions

* **X-1** In-world test, condition 2: confirm on a live game that the in-game root or the map
  root is visible in every in-world state (inventory, system menu, map, death) and in none of
  the menu states (start, character select/creation, world select, connect, "Please wait").
* **X-2** Whether W+0xb8 is set when playing offline (only the online join path 0x46fc50
  was found writing it); log W+0xb8 and G+0x8006d0 together.
* **X-3** Which zone list holds plant sprites: the plant pass appends through 0x4862e0 (a
  `list` append), which the trees spec places at zone+0x04; the terrain and vegetation specs
  name zone+0x24 (a `vector`) and call zone+0x18 "static objects" (it is the spawn vector).
  Dump zone+0x04 and zone+0x24 and settle it.
* **X-4** Storage of block light per zone (needed only for lighting comparisons).
* **X-5** Spawn object (0x10f0 bytes) full layout, so that its pointer-valued fields can be
  excluded exactly.
* **X-6** The second map (W+0x0c) and list (W+0x14) of the world object.
* **X-7** Whether the generator writes into neighbouring published zones (overview Q2); a
  fresh-zone dump of the neighbours before and after a generation settles it.
* **X-8** Element types of zone+0x48, +0x54, +0x88, +0x94.
* **X-9** When the mod loader calls `LoadLibrary` relative to 0x459c40; the DLL must cope with
  G being null for a while (§1.1) either way.
