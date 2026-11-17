# Alpha extractor reference: native content store, data formats and in-process world state

Status: FINAL / STABLE / HELD (re-certified 2026-11-16 against the original Alpha evidence)

Supported image: `Cube.exe` SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`
(PE32, machine 0x14c, 3,885,568 bytes, timestamp 0x51ea955e). Where `Server.exe` is mentioned it
is SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

Scope. Part A describes the files of an owned Alpha installation, how the original client
identifies, opens and decodes them, and the identifiers by which an installation can be
recognised. Part B describes where the running client keeps its world state in memory, which
thread writes it and which lock covers it. Everything is taken from the original client binary,
its decompile/assembly and the original installation's files; it is code-free prose.

Read with: [alpha-worldgen-overview.md](alpha-worldgen-overview.md) (world object, units),
[alpha-worldgen-terrain.md](alpha-worldgen-terrain.md) §1 (zone, column, block),
[alpha-net-packets.md](alpha-net-packets.md) §1–§2 (item and creature records),
[alpha-interaction.md](alpha-interaction.md) §4.1 (statics),
[alpha-world-items.md](alpha-world-items.md) §1 (world items).

---

## 0. Conventions

* Addresses are those of `Cube.exe` at its preferred base 0x400000, as in every other spec
  ("0x46a8a0"). RVA = address − 0x400000.
* The image is relocatable: optional-header DllCharacteristics = 0x8140 (dynamic base, NX
  compatible, terminal-server aware) and a `.reloc` section at RVA 0x371000. At run time an
  address from this document lies at (actual module base + RVA). Pointer values read from
  memory are already actual addresses.
* Section map (needed to turn an address into a file offset): `.text` RVA 0x1000, file offset
  0x400; `.rdata` RVA 0x2fc000, file offset 0x2fb000; `.data` RVA 0x365000, file offset
  0x363200 (virtual size 37,624, raw size 24,576: the tail is zero-filled at load); `_RDATA`
  RVA 0x36f000; `.rsrc` RVA 0x370000; `.reloc` RVA 0x371000. Size of image 0x3bc000, entry RVA
  0x28e1e0.
* All multi-byte integers in files and memory are little-endian.
* **G** = the game object (RTTI class `cube::GameController`), **W** = the world object
  (`cube::World`) = G + 0x2e4. "W+x" and "G+(0x2e4+x)" are the same byte.
* Containers are those of the Visual C++ 2012 runtime (`MSVCR110.DLL`/`MSVCP110.DLL` are
  imported):
  * vector: 12 bytes = begin pointer, end pointer, capacity-end pointer; element count =
    (end − begin) / element size.
  * list: 8 bytes = sentinel node pointer, element count. Node: +0x00 next, +0x04 previous,
    +0x08 element. Appending links the new node before the sentinel (tail).
  * ordered map: 8 bytes = sentinel node pointer, element count. Node: +0x00 left, +0x04
    parent, +0x08 right, +0x0c colour byte, +0x0d "is sentinel" byte, +0x10 key, then value.
    Sentinel→left = smallest key, sentinel→parent = root, sentinel→right = largest key; an
    in-order walk stops at a node whose +0x0d byte is non-zero.
  * narrow string: 24 bytes = 16-byte inline buffer (when capacity > 15 its first 4 bytes are
    a heap pointer instead), length at +0x10, capacity at +0x14. Wide strings use the same
    shape with an 8-character inline buffer (capacity > 7 means heap).
* RTTI vtables (from `symbols.csv`, each confirmed by a constructor store):

| Class | vtable |
|---|---|
| cube::GameController (G) | 0x6ffdc8 |
| cube::World (W) | 0x71a794 (stored by constructor 0x58eb00) |
| cube::Region | 0x71dccc |
| cube::Zone | 0x71e280 (stored by constructor 0x6240f0) |
| cube::Field (a column) | 0x6ffd90 (stored by constructor 0x450bf0) |
| cube::Creature | 0x6fd8cc |
| cube::Spawn | 0x71dbec |
| cube::WorldMap | 0x71dfcc (stored by constructor 0x5fae40) |
| cube::Database | stored by constructor 0x449380 |

---

# Part A — Installation and content store

## A1. Installation inventory and identifiers

The reference installation contains the files below (sizes in bytes; SHA-256 of the owner's
original files). Kinds were established from the file headers and the client's loaders.

| File | Size | SHA-256 | Kind / consumer |
|---|---|---|---|
| Cube.exe | 3,885,568 | 84a7a132…6dc30bf (full value above) | client |
| Server.exe | 1,718,784 | 1ffbcbce…70d34fb1 (full value above) | dedicated server |
| data1.db | 20,941,824 | 0e482f6d628414db017d983f62843e5dbdc52c5b54fd478f6f0e06c786639b9d | SQLite store, voxel models (§A3) |
| data2.db | 39,565,312 | e2231a7478d882d6382a4146ee70f1dae67662b58c8a9188fbcc294061b550fb | SQLite store, sounds |
| data3.db | 100,352 | e41dbde87bd58142d73bc22e0d3e9fd61fdb340e80f1e1e84b674edc63430438 | SQLite store, images |
| data4.db | 172,032 | d859dd14eb855aa8b08dc5fae2062d7b722060b9204682ac5cabbc4af971599e | SQLite store, text dictionaries |
| gui.plx | 822,138 | 615106d59e5ef3af43157f3c019f7a8bdb35d407aee73e51fe1a4f9c3caa0695 | interface scene (§A6) |
| start.plx | 1,534,507 | be1efe6b5a1cb6b6984495994b10eb830a7f187583eaf44b7d0eae6504059c53 | interface scene |
| help.plx | 258,161 | 1af05e8d119d6a268b71bfbf19b6ab428936e959727fa7a1332b0aa03295f5a5 | interface scene |
| quest-tag.plx | 75,421 | 79923577ef8f6d90413ab7dbb0fb97f6c94df132a8a076b9ce1b74caa3a31161 | interface scene |
| cursor.plx | 11,609 | a82d79abd362a0b0c2117dfa75f92ab37dc133e96ba7915041ecd090ebc429b9 | interface scene |
| resource1.dat | 19,388 | f2d473a996288f7d3d0f4a4540bbb71608593bfe40965efa7b02e8c18cc45785 | TrueType font (§A7) |
| resource2.dat | 15,864 | 1fb8573f8a39590ecc39725b365ef9c29a3f81b839e70df1e9e6d6cd6db7f173 | TrueType font |
| db.dat | 32 | c4dfd81a97b4b53fd287222cf0610c4f262dc70550b9ec2461eb2b8863ac413e | start-up check file (§A8) |
| options.cfg | 198 (varies) | — (user settings, written by the client) | §A9 |
| FreeImage.dll | 2,084,864 | 5f24cb61deafe4955631d4ff69a9fe0717679a959dba8dd5a289961c1f6b5e6f | third-party image library (imported) |
| zlib1.dll | 75,264 | e96b397b499d9eaa3f52eaf496ca8941e80c0ad1544879ccadf02bf2c6a1ecfc | third-party (imported by ordinal) |
| XAudio2_8.dll | 339,456 | 17beef7ea65da3037b6dce97a6ac8d10e2afabc4204c93f4a2c1cb7e4767eba1 | audio runtime |
| msvcp110.dll / msvcr110.dll / vccorlib110.dll | 535,008 / 875,472 / 252,400 | c8d5572c… / b30160e7… / 0f4a9f69… | VC++ 2012 runtime |
| freeimage-license.txt, freetype-license.txt | 17,436 / 6,743 | — | licence texts |

Identifying header bytes:

* `*.db`: the 16-byte SQLite 3 header "SQLite format 3" followed by a zero byte.
* `*.plx`: u32 0, u32 0x16, u32 1, u32 14, then the 14 ASCII bytes "PlasmaGraphics" (identical
  first 30 bytes in all five files).
* `resource1.dat`, `resource2.dat`: sfnt version 0x00010000 (big-endian), 13 tables, first table
  tag "FFTM".
* `db.dat`: the 32 ASCII bytes "PLACEHOLDER" followed by 21 "0" characters (reference copy).

File names the client opens (string addresses in `Cube.exe`): "data1.db" 0x703f14,
"data2.db" 0x71e260, "data3.db" 0x7003b8, "data4.db" 0x703b74, "db.dat" 0x6ffe18,
L"gui.plx" 0x6ffefc, L"quest-tag.plx" 0x6fff0c, L"help.plx" 0x6fff28, L"start.plx" 0x6fff6c,
L"cursor.plx" 0x700c34, L"resource1.dat" 0x6fcd24, L"resource2.dat" 0x70027c,
"options.cfg" 0x703194, "Save/characters.db" 0x700e54, "Save/worlds.db" 0x700e6c,
"Save/world_" 0x71a7a4, "Save/map_" 0x71e05c. The string "interface.plx" (0x71a2f8) is only
stored as a name by 0x584300; no file of that name exists in the reference installation and no
load of it was found. `Server.exe` references "data1.db" (0x55b534) and "data4.db" (0x55b2c8)
and the same blob-table SQL; the server's loaders were not re-examined here.

## A2. Database container

Each `dataN.db` is an SQLite 3 database with exactly one user table and no views or triggers:
table `blobs` with columns `key` (declared TEXT, primary key) and `value` (declared BLOB), plus
SQLite's automatic primary-key index. The client's database object (`cube::Database`, 8 bytes:
vtable, connection handle at +0x04; constructor 0x449380) uses these statements, all present in
`.rdata`:

| Address | Statement | Used by |
|---|---|---|
| 0x6ffb20 | CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB); | open 0x4497b0 |
| 0x6ffbe8 | SELECT value FROM blobs WHERE key = ? | fetch 0x449810 |
| 0x6ffb58 | SELECT 1 FROM blobs WHERE key = ? | existence test |
| 0x6ffb7c / 0x6ffba4 | UPDATE … SET value / INSERT INTO blobs(key, value) … | writes (save stores) |
| 0x6ffc10 / 0x6ffc30 | DELETE FROM blobs … / UPDATE blobs SET key = ? … | save stores |

* **Open (0x4497b0).** Closes any previous connection, opens the named file through the
  embedded SQLite (0x525590), and on success executes the CREATE TABLE statement, ignoring its
  result; returns true. A missing file is therefore created as an empty database and every
  later lookup in it fails: opening does not by itself prove the content is present.
* **Fetch (0x449810).** Prepares the SELECT, binds the key as text parameter 1, steps; on a
  row (SQLite code 100) copies the whole value into a newly allocated buffer and returns true.
  When the statement must be retried (code 17) it prepares again; any other result returns
  false (key absent). Wrapper 0x4498d0 moves the bytes into a byte vector.
* Keys are ASCII file names with extension (for example "aim.cub", "dict_en.xml"), compared
  exactly by SQLite's default text collation.

## A3. Store roles (reference installation)

| Store | Opened by | Keys (rows) | Payload after decoding |
|---|---|---|---|
| data1.db | 0x4ee230 (path built by 0x4515f0) | `*.cub`, 2,857 | voxel model (§A5.1) |
| data2.db | 0x622da0 | `*.wav`, 93 | RIFF/WAVE sound (§A5.2) |
| data3.db | constructor 0x459c40 at 0x45e119 | `*.png`, 104 | PNG image (§A5.3) |
| data4.db | 0x4e1970 | `dict_de.xml`, `dict_en.xml`, 2 | UTF-8 XML (§A5.4) |

Every asset load fetches by key and then applies the descrambler of §A4 to the fetched bytes
before parsing: images 0x486a20 (call 0x486a8f), voxel models 0x4e7290 (0x4e730b), dictionary
0x4e1970 (0x4e1b96), sounds 0x623a60 (0x623b34). The client contains no encoding counterpart
and never writes the asset stores. The save stores under `Save/` use the same table and
statements but no descrambler call was found on their paths (inferred unscrambled; not checked
against a save file).

## A4. Value scrambling

**Key table.** 44 signed 32-bit integers at address 0x6ffa68 (RVA 0x2ffa68, file offset
0x2fea68, 176 bytes, inside `.rdata`), in order:
4242, 9551, 840, 84800, 9242, 9846, 127, 9, 9483, 394, 123, 4834, 32444, 24355, 2433, 17,
34234, 42342, 4243, 14, 184934, 1987, 3094, 1901, 89409, 4813, 37, 143, 3490, 19483, 1343,
432, 84732, 9184, 9612, 1233, 3434, 1839, 2984, 1993, 2984, 4895, 816583, 13.

**Decoding (0x4496a0, applied in place to a buffer of n bytes).**

1. For i from n − 1 down to 0: let j = (table[i mod 44] + i) taken as an unsigned 32-bit
   value, modulo n (unsigned). Exchange byte i and byte j.
2. Then replace every byte b by 255 − b (bitwise complement).

An empty buffer is left unchanged. Encoding is the exact inverse: complement every byte, then
for i from 0 up to n − 1 exchange byte i and byte j with the same j. The transform does not
change the length and carries no header, checksum or compression.

Verification by this author: decoding the stored value of `aim.cub` (data1, 13,836 bytes)
yields dimensions 16 × 16 × 18 and exactly 12 + 16·16·18·3 bytes; `dict_en.xml` (data4) begins
with a UTF-8 byte-order mark and "<?xml"; the first data3 row begins with the PNG signature;
the first data2 row begins "RIFF", length, "WAVE".

## A5. Decoded payload formats

### A5.1 Voxel model (`.cub`) — loader 0x4e7290, builder 0x4e7650

* Header: three i32 values, size X, size Y, size Z (in that order). Then X·Y·Z colour cells of
  3 bytes (R, G, B). Cell (x, y, z) is at byte 12 + 3·(x + X·(y + Y·z)): x varies fastest,
  then y, then z.
* The loader copies only whole 3-byte cells that are present; a value shorter than its header
  promises leaves the missing cells as allocated (zero) and a truncated header falls back to
  the caller-supplied default for the missing size fields. The builder accepts the model only
  when all three sizes are positive.
* While building (unless the caller's "skip markers" flag is set) every cell whose colour is
  exactly (255, 0, 0), (0, 255, 0) or (0, 0, 255) is appended, as a 3 × i32 position, to one of
  three marker lists of the model (red at model +0x04, green at +0x10, blue at +0x1c). Sizes are
  kept at model +0x44/+0x48/+0x4c and the cell copy at +0x30.
* Which colour marks an empty cell is decided in the mesher 0x4e7870 and was not re-derived
  for this document (open question A-2).
* The same loader can read the same layout from a plain file when called with a zero
  database argument; the constructor calls it that way for "build-cursor.cub" (0x4634c6),
  which is not present in the reference installation (no failure is raised).

### A5.2 Sound (`.wav`) — loader 0x623a60

The decoded value is parsed as RIFF: chunk "RIFF" (0x46464952) must hold the form type
"WAVE" (0x45564157); the "fmt " chunk (0x20746d66) is copied into the sound's format block and
the whole "data" chunk (0x61746164) into a new buffer. Other chunks are skipped. Rows whose form
type is not "WAVE" are ignored.

### A5.3 Image (`.png`) — loader 0x486a20

The decoded bytes are handed, with their length, to the texture-from-memory path 0x658fa0
(the client imports FreeImage's memory-stream functions; that this path uses them is inferred).
When the key is absent the loader falls back to 0x658c40 with the same name.

### A5.4 Dictionary (`.xml`) — loader 0x4e1970

The key "dict_en.xml" (string 0x703b80) is fetched from data4.db, decoded and parsed as XML;
element/attribute names referenced by the loader include "tags", "sentence", "normal" and
"name". The selection of `dict_de.xml` (for example by the language option) was not traced
(open question A-3).

## A6. Interface scenes (`.plx`)

The constructor 0x459c40 loads gui.plx, quest-tag.plx, help.plx and start.plx (around
0x45c120..0x45c2cd) and cursor.plx (0x461e46), each through the scene loader 0x653770 into a
previously created interface node. Files are read directly from disk (not from a database) and
begin with the "PlasmaGraphics" header of §A1; further records include a "Seal" record and
length-prefixed names, some UTF-16 (for example "Mesh"). The internal layout was not specified
in this document (open question A-1); recognition is by name, size, hash and header.

## A7. Fonts (`resource1.dat`, `resource2.dat`)

Both are TrueType fonts loaded from disk by 0x639800 (resource1 before resource2, around
0x45ddff). If either load fails the constructor sets the byte G+0x1a0 to 1. The main loop of
WinMain 0x4c8ae0 tests that byte on every iteration and posts a quit message when it is set, so
the client exits right after start-up. The wide string L"resource1.dat" is also passed by
hundreds of interface-text calls as their font name.

## A8. Start-up check file (`db.dat`)

The constructor opens db.dat as a binary input stream (0x45abe0), measures it by seeking to the
end, and:

* if the measured size is 0, takes the constructor's failure path (0x45ac44 → 0x465aa5);
* otherwise reads the whole file into a string and passes it, with the name "db" (0x6ffe20),
  to 0x4574f0 on an internal object; a zero result sets the quit byte G+0x1a0 (0x45ad8d), with
  the effect described in §A7.

The meaning of 0x4574f0 (a named evaluation/lookup through 0x457600) is unresolved (open
question A-4); the reference content is the 32-byte placeholder text of §A1.

## A9. Files the client creates

* WinMain creates the directory "Save" (`_mkdir`) before constructing G.
* options.cfg: plain text, one "name value" pair per line, read by 0x4ce6e0 and written by
  0x4cef80. Names in the reference file: fullscreen, resolutionX, resolutionY, antiAliasing,
  renderDistance, soundVolume, musicVolume, cameraSpeed, cameraSmoothness, language, invertY,
  minTimeStep. It is user state, not content.
* Save/characters.db, Save/worlds.db, Save/world_<…> (opened by world initialisation 0x5a52e0
  at 0x5a5725) and Save/map_<…> (WorldMap 0x5fbc90 at 0x5fbdd1) are blob-table databases of the
  §A2 schema. Zones are saved through 0x5a5050 (see §B5.2).

---

# Part B — In-process world state

## B1. The root

### B1.1 The game object

* Global 0x76b1c8 (RVA 0x36b1c8, `.data`) holds G. WinMain 0x4c8ae0 creates "Save", allocates
  0x1001018 bytes, clears the global, runs the constructor 0x459c40 and stores its result in the
  global only after the constructor returns. The constructor loads every asset (Part A), so the
  global stays null for a long time after process start.
* G is not moved or freed until WinMain leaves its loop (it then calls the destructor through
  vtable slot 0 with argument 1).
* Other globals: 0x76b1b4 = the Direct3D 9 device, 0x76b1c0 = the window. The frame function
  0x4c85f0 does nothing unless both 0x76b1b4 and 0x76b1c8 are non-null.
* W is embedded at G+0x2e4 (constructor 0x58eb00, called at 0x459f57); the `cube::WorldMap` is
  embedded at G+0x800d44 (constructor 0x5fae40, called at 0x45a5b2). Neither moves.
* Thread run flags in G: byte G+0x800584 is set to 1 at 0x463512 (before the mesher and map
  threads start) and is the loop condition of those two threads; byte G+0x8005b0 is set to 1
  at 0x4635c6 and is the loop condition of the streaming thread (tested at 0x46a8e4 and at the
  loop end 0x46b70f). Both are cleared by the destructor 0x466d90 (0x466f9d, 0x466fed).
* Byte G+0x1a0 is the quit request (§A7).

### B1.2 A world exists from start-up

The constructor seeds a world with `timeGetTime()` (stored to G+0x800a50 at 0x45a73b, then
world initialisation 0x5a52e0) and the start screen draws its terrain (`alpha-ui-screens.md`
§3.3). The local player object also exists from start-up: the constructor allocates a 0x1e60-
byte creature with id 1, names it "Wollay" (name field at object +0x1168), stores it at
G+0x8006d0, inserts it into the creature map under id 1 and sets W+0xb8 to it (0x46222c,
setter 0x487e90). "Zones present" or "player non-null" therefore does not mean "in a world".

### B1.3 Native indicators of the in-world state

Visibility of an interface node N: let A = the pointer at N+0x3c; the node is visible when the
u32 at (pointer at A+0x94) + 4·(i32 at A+0x68) is non-zero.

1. Start-screen root: node pointer at G+0x800874.
2. In-game root G+0x800884: the frame code 0x4ae27e..0x4ae354 makes it visible exactly when the
   nodes at G+0x800874, +0x800880, +0x800888, +0x800894, +0x80088c, +0x800890 and +0x800898
   are all hidden. The map root is at G+0x8008a0 (the map frame hides the in-game root,
   `alpha-ui-screens.md` §5.2.3 step 8).
3. Requested versus loaded world: W's seed W+0x800164 (= G+0x800448) and name (string at
   W+0x94 = G+0x378) against the requested seed G+0x800a50 and name (string at G+0x800a54).
   While they differ, the next streaming pass unloads everything and re-initialises the world
   (§B5.2).
4. The zone containing the local player's position (§B3.3, §B2.1).

The identity of the nodes other than the start-screen, in-game and map roots, and the
indicator values in each menu state, were not observed on a running client (open question X-1).

## B2. Zones and blocks

### B2.1 Zone storage

There is no list of loaded zones. Zones hang off a fixed two-level grid in W:

| Location | Content |
|---|---|
| W+0xbc | Region pointers, 1024 × 1024, index rx·1024 + ry (x-major) |
| W+0x4000bc | Region-cell pointers, 1024 × 1024, same indexing (0x1c-byte records per the regions spec) |
| Region +0x18 | per-zone table, 64 × 64 entries of 16 bytes |
| Region +0x10018 | Zone pointers, 64 × 64, index (zx mod 64)·64 + (zy mod 64) (x-major) |
| Region +0x14018 | location records, 8 × 8 × 0x68 bytes |

Lookup of zone (zx, zy) (0x434d10 → 0x434a90): defined for 0 ≤ zx, zy < 65536 (otherwise no
zone); rx = zx div 64, ry = zy div 64 (region indices must be 0..1023); null region → no zone;
else the pointer at region+0x10018 + 4·((zx mod 64)·64 + (zy mod 64)). Null means not loaded.

### B2.2 Zone record (200 = 0xc8 bytes, allocated at 0x5e495f, constructor 0x6240f0)

| Off | Type | Meaning |
|---|---|---|
| +0x00 | ptr | vtable 0x71e280 |
| +0x04 | list | sprites (plant sprites, tree records, lanterns, cobwebs …), nodes 0x48 bytes (§B2.6) |
| +0x0c | vector | statics, 0x188-byte records inline |
| +0x18 | vector | Spawn pointers |
| +0x24 | vector | named "plant sprites" in the terrain spec; see X-3 |
| +0x30 | vector | world items, 0x148-byte records inline |
| +0x3c | vector | object list, 16-byte (i32 n, i32 Bx, i32 By, i32 Bz) records (`alpha-block-edits.md`) |
| +0x48 | vector | structure list (`alpha-worldgen-structures.md`) |
| +0x54 | vector | not traced |
| +0x60, +0x64 | i32, i32 | zx, zy |
| +0x68 | vector | stored block edits, 20-byte records (net §3.5); occupies +0x68..+0x73 |
| +0x74 | u8 | "dynamic data requested" (constructor 1) |
| +0x75 | u8 | "items changed" (constructor 0) |
| +0x76 | u8 | constructor 0; not traced |
| +0x78 … +0x87 | 16 bytes | copy of the region's per-zone entry; +0x80 zone level (constructor 1), +0x84 sub-level byte (constructor 0) |
| +0x88 | vector | house list (structures spec §5.5.4) |
| +0x94 | vector | not traced |
| +0xa0 | i32 | number of spawn objects the generator made (written at 0x5edd5e) |
| +0xa4 | i32 | constructor 0; not traced |
| +0xa8 | ptr | 65,536 columns of 32 bytes (array allocation with a 4-byte count prefix) |
| +0xac | ptr | 32 × 32 cell index of 8-byte list heads (`alpha-interaction.md` §3) |
| +0xb0 | CRITICAL_SECTION | per-zone lock (24 bytes); initialised and deleted, no user found |

Publication. The generator 0x5e4850 (thiscall on W, arguments zx, zy, `RET 8`) returns null
for coordinates outside 0..65535. Otherwise it builds the whole zone — terrain, post passes,
stored edits, block light (0x59a0e0 at 0x5ede84), then a final pass over the statics that adds
entries to the sprite list (0x5edf83..0x5ee048) — and as its last act stores the zone pointer
into the region's zone array (plain store at 0x5ee05a, no lock) and returns the zone pointer.
A non-null zone pointer therefore designates a complete zone. Afterwards the streaming thread
takes the WorldMap's zone lock, sets byte +0x28 of the WorldMap entry for that zone to 1
(0x46acf8) and releases it.

### B2.3 Columns and blocks

Column (x, y) in block units, 0 ≤ x, y < 2^24 (0x4347a0): zone = (x div 256, y div 256) unless
a zone is given, in which case the block must lie inside it; column address = zone columns +
32·((x mod 256) + 256·(y mod 256)) (x varies fastest, unlike the region and zone grids).

Column (`cube::Field`, 32 bytes; constructor 0x450bf0 zeroes all fields):

| Off | Type | Meaning |
|---|---|---|
| +0x00 | ptr | vtable 0x6ffd90 |
| +0x04 | f32 | moisture |
| +0x08 | f32 | warmth |
| +0x0c | f32 | fire share |
| +0x10 | i32 | bottom: z of the first stored block |
| +0x14 | i32 | surface z − 8 (use not traced) |
| +0x18 | ptr | stored blocks, 4 bytes each, z = bottom, bottom + 1, … |
| +0x1c | i32 | count of stored blocks |

Block at (x, y, z) (0x42f7e0 with 0x42f730):

* no column (coordinates out of range or zone not loaded), or z < bottom → solid grey
  R = G = B = 200, byte 3 = 0x01 (constant at 0x76afc4, set by static initialiser 0x6f9750);
* z ≥ bottom + count → if z > 0, air R = G = B = 255, byte 3 = 0x00 (0x76afc0, initialiser
  0x6f9720); if z ≤ 0, water R = G = B = 255, byte 3 = 0x82, i.e. type 2 with bit 0x80
  (0x76afbc, initialiser 0x6f9780);
* otherwise the stored 4 bytes at blocks + 4·(z − bottom), except that a stored block whose
  type bits (0–4) are 0, at z ≤ 0 and without bit 0x40, reads as the water constant above.

Block bytes: 0 = R, 1 = G, 2 = B, 3 = type in bits 0–4, 0x20 "surface-coloured", 0x40 "dry
air", 0x80 "keep" (terrain spec §1.3–§1.4). The stored range per column is exactly
[bottom, bottom + count); a raw column is fully described by its climate floats, bottom, the
+0x14 value, count and the count × 4 stored bytes.

### B2.4 Block light

Computed into the zone before publication (0x59a0e0). Its storage was not determined (X-4);
the four block bytes carry no light.

### B2.5 Writers of zones

| Thread | Writes | Locks held |
|---|---|---|
| streaming 0x46a8a0 | builds zones before publishing; nulls and deletes far zones, regions and region cells; re-initialises the world | none while generating; both world locks around each unpublish and the re-initialisation (§B5.2) |
| main thread, update 0x488ee0 | block edits from the network or the local player (0x48f242..0x48f3d0), world items (+0x30), statics (+0x0c), spawn list (+0x18), "items changed" | W.worldLock from 0x48902f to 0x49d2ea; W.zoneLock in addition while applying edits (0x48f25f, 0x48f5aa) and inside the world step 0x60c510 (0x60f084) |
| mesher 0x4690a0 | nothing in zones; reads blocks (0x49d910) | W.zoneLock only |
| map 0x469590 | the WorldMap only | the WorldMap's own locks |

Whether the generator writes blocks into neighbouring published zones is not known (X-7).

### B2.6 Per-zone lists filled by the generator (fixed order)

* Sprites, zone+0x04: list nodes are 0x48 bytes (maximum-size constant 0x38e38e2 in the append
  0x4862e0), element at node+0x08 (0x3c bytes used): +0x00 i32 kind; +0x08, +0x10, +0x18 i64
  position (1/65536 block); +0x20 f32 size; +0x24 f32 rotation; +0x2c, +0x30, +0x34 f32 tint;
  +0x38 i32 mode. Tree records (kinds 60, 61) leave +0x20..+0x2b as stale stack bytes
  (`alpha-worldgen-trees.md` §5). Appends are at the tail; order is significant.
* Statics, zone+0x0c: 0x188-byte records (`alpha-interaction.md` §4.1); +0x48..+0x53 is a
  container holding heap pointers; +0x30 (state) and +0x34 (timer) change in play.
* Spawn objects, zone+0x18: pointers to `cube::Spawn`, 0x10f0 bytes each (allocated with that
  size immediately before constructor 0x5adb20 in 0x5af170): +0x08 f32, +0x10 3 × i64
  position, +0x28 i32 count/level, +0x2c i32 creature type, +0x34 i32 level, … (layout X-5).
* World items, zone+0x30: 0x148-byte records (`alpha-world-items.md` §1); 0x139..0x13b padding.
* Stored block edits, zone+0x68: 20-byte records (net §3.5).
* Zone header: zx, zy, the 16-byte region entry (+0x78..+0x87), +0xa0.

## B3. Creatures

### B3.1 The creature map

W+0x04 is an ordered map from u64 id to Creature pointer (sentinel pointer at W+0x04 =
G+0x2e8, count at W+0x08). Lookup 0x42f000 (thiscall on W, argument = pointer to the u64 id)
uses the lower-bound search 0x477d90 and returns the node's value (node+0x18) or null. Keys
order as signed 64-bit values (high words compared signed, low words unsigned). Node: +0x10 u64
id, +0x18 Creature pointer. W+0x0c/+0x10 (a second map) and W+0x14/+0x18 (a list) were not
traced (X-6).

The map holds every creature the client simulates or was told about: the local player, NPCs and
monsters made from nearby spawn objects, other players and server creatures online, pets,
bombs. The world step and the network receive code insert and delete entries; deletion frees
the Creature.

### B3.2 Creature object (`cube::Creature`, 0x1e60 bytes, constructor 0x43b690)

| Off | Size | Meaning |
|---|---|---|
| +0x00 | 4 | vtable 0x6fd8cc |
| +0x04 | 4 | not traced |
| +0x08 | 8 | u64 id (the map key) |
| +0x10 | 0x1168 | the creature record of `alpha-net-packets.md` §2.1 (record offset r = object offset 0x10 + r) |
| +0x1178 … +0x1e5f | | client-only state (other specs cite +0x1184, +0x1190, +0x1198/+0x119c (connection), +0x11dc, +0x130c, +0x1350, +0x13a4, +0x13e4, +0x1d28 …) |

Evidence: the join path 0x46fc50 receives a 0x1168-byte record for the player at object +0x10;
the streaming thread writes the player's position at +0x10..+0x27 and its copy at
+0x1350..+0x1367. Bytes the wire never carries (padding inside fields, record 0x1ac..0x1af) are
undefined; field-by-field reading per the net table is the meaningful view.

### B3.3 The local player

* G+0x8006d0 = the local player's Creature pointer (valid from start-up, §B1.2).
* W+0xb8 (= G+0x39c) = the step's own creature. Set at start-up to the start-up player
  (0x46222c) and on joining a server to the player looked up or created under the
  server-assigned id (0x470408). On joining, every creature in the map is deleted first.
* Position: player +0x10, 3 × i64 in 1/65536 block. Zone = (x >> 24, y >> 24) for
  non-negative positions. The game's own "player zone" is kept at G+0x2bc/G+0x2c0
  (`alpha-item-names.md`).

## B4. Clock and seed

| Where | Type | Meaning |
|---|---|---|
| W+0x80015c (G+0x800440) | i32 | time of day in ms, 0 … 86,399,999; World constructor sets 43,200,000 (0x58ede5); re-initialisation sets it from its arguments (0x5a57db); the network receive thread writes it under W.worldLock (0x46cbd2) |
| W+0x800160 (G+0x800444) | i32 | day number (receive thread write at 0x46cbcc) |
| W+0x800164 (G+0x800448) | u32 | world seed in use (written by re-initialisation, 0x5a5324) |
| W+0x800168 … +0x800298 | 77 × i32 | seed offsets (random spec §2.2) |
| W+0x8000f0, +0x8000f4 | f32 | home point; re-initialisation sets x to 0x4b002080 (8,396,928.0) |
| W+0x8000bc | i32 | step time accumulator (World constructor clears it; read by 0x488b80) |
| W+0x94 (G+0x378) | string | world name in use |
| G+0x800a50 | u32 | requested seed (start-up `timeGetTime()`, then the chosen world or the server's choice) |
| G+0x800a54 | string | requested world name |

## B5. Threads, locks and lifetimes

### B5.1 Threads

The constructor starts three `_beginthread` workers through `std::function` wrappers
(0x450e70; thread entry 0x450e00 calls the functor once). Functor vtables 0x702984, 0x7029a4,
0x7029c4; their call slots 0x46db70, 0x46db80, 0x46dba0 jump to the bodies:

| Body | Handle at | Priority | Loop condition | Work |
|---|---|---|---|---|
| 0x4690a0 | G+0x800588 | below normal | G+0x800584 | chunk meshes |
| 0x469590 | G+0x80058c | below normal | G+0x800584 | world-map tiles (WorldMap at G+0x800d44) |
| 0x46a8a0 | G+0x8005b4 | normal | G+0x8005b0 | zone streaming |

Online, the send (0x469c10) and receive (0x46b740) threads run; the receive thread takes
W.worldLock (0x46b875, 0x46cbb9). The main thread runs WinMain's loop 0x4c8ae0 → frame 0x4c85f0
→ G's update (vtable slot 10) and draw (slot 11).

### B5.2 Locks

All are Win32 critical sections (recursive for the owning thread):

| Name | Address | Enter / leave | Taken by |
|---|---|---|---|
| W.worldLock | W+0x8000c0 | 0x601cb0 / 0x601e90 (register-passed W) | main update 0x48902f..0x49d2ea (including the world step 0x60c510); main draw 0x4ac260 from 0x4ac37e to its end; network receive; zone, region and cell unload; world re-initialisation |
| W.zoneLock | W+0x8000d8 | 0x59c480 / 0x601ea0 | main thread while applying edits and in the world step; mesher 0x49d910; unloads; re-initialisation |
| G.settingsLock | G+0x8005d0 | direct | requested seed/name, interest-point list G+0x2c4, reference position G+0x2ac..+0x2bb |
| other G locks | G+0x8005b8, +0x8005e8, +0x800600, +0x800618 | direct | renderer, mesher and map internals |

Lock order in the original:

* zone unload 0x5a4890: zoneLock, then worldLock; null the pointer; release worldLock, then
  zoneLock; save the zone through 0x5a5050 when byte W+0xb4 is 0 (the save is skipped unless
  W+0xa4 is non-zero and the zone has "items changed" set or a non-empty edit vector); then
  destroy the zone (vtable slot 0, argument 1);
* region unload 0x5a4800: first unloads the region's zones (0x5a4af0), then worldLock,
  zoneLock, null, release in reverse, destroy; region-cell unload 0x5a4780: worldLock,
  zoneLock, null, release, free;
* re-initialisation 0x46af84..0x46afab: worldLock, zoneLock, then 0x5a52e0 with the requested
  seed and name and WorldMap reset 0x5fbc90; both released at the end of the pass;
* the main thread takes worldLock then zoneLock.

The streaming thread's zone unload therefore takes the two world locks in the opposite order
from every other path (zoneLock → worldLock versus worldLock → zoneLock).

**Streaming pass** (body of the loop, then `Sleep(20)`):

1. Under settingsLock: decide "world changed" (requested seed ≠ W seed, or requested name ≠ W
   name), copy the reference position (G+0x2ac 8 bytes, G+0x2b4, G+0x2b8 in blocks) and the
   interest-point list (nodes carry i32 zone x, y at node+0x08/+0x0c).
2. Unless the world changed: for each interest point, consider zones zx, zy within ±3 of it,
   clamped to 0..65535; among zones not yet loaded choose the one whose centre
   (zx·256 + 128, zy·256 + 128) has the smallest squared block distance to the reference
   position, provided it is below 768² = 589,824 (strict); generate at most that one zone.
3. When more than 1000 ms passed since the last clean-up (or the world changed): every loaded
   zone whose squared zone distance (zx − px)² + (zy − py)² is ≥ 16 for every interest point is
   unloaded — all zones when the world changed. A region at Chebyshev region distance ≥ 3 from
   every interest point's region is unloaded, a region cell at ≥ 5 (all of them when the world
   changed).
4. If the world changed: re-initialise (above), place the player from the new world's start
   data, copy that position to player +0x1350 and clear player +0x34..+0x3f.

Consequences: the kept area is a disc of squared radius < 16 zones around each interest point
(for example (3, 2) is kept, (3, 3) and (4, 0) are freed). Zone, region and Creature pointers
and every vector buffer can become invalid between two reads; G, W and the grids never move.

**Lock coverage.** Every writer of zones or creatures except the mesher (read-only) and the
generator (unpublished zones) holds W.worldLock: the main update and draw, the network receive,
the unpublish steps (which hold both locks) and re-initialisation. While the streaming thread
runs the generator it holds no world lock.

### B5.3 Native entry points

| Routine | Convention | First instructions (instruction boundary) |
|---|---|---|
| frame 0x4c85f0 | no arguments, only caller 0x4c9432 in WinMain | 7-byte compare of the global 0x76b1b4 with 0 (absolute address operand) |
| G update 0x488ee0 (vtable 0x6ffdc8 slot 10 at 0x6ffdf0) | this in ECX, one stack argument = ms since the previous frame (0 on the first), `RET 4`; called at 0x4c8633 | push ebp; mov ebp, esp; and esp, −8 — 6 bytes |
| G draw 0x4ac260 (slot 11 at 0x6ffdf4) | this in ECX, no argument | — |
| generator 0x5e4850 | this = W in ECX, stack zx, zy, `RET 8`, returns Zone pointer or null; called by the streaming thread at 0x46acd1 | push ebp; mov ebp, esp; push −1 — 5 bytes |

Between update and draw the frame compares the four i32 at G+0x170..+0x17f with saved copies;
when any differs it saves G+0x170..+0x19f and calls 0x4c8940 with argument 0.

---

## 7. Open questions

* **A-1** Internal layout of the `.plx` scene format (loader 0x653770).
* **A-2** The empty-cell rule of `.cub` models (mesher 0x4e7870).
* **A-3** Whether and when `dict_de.xml` is used instead of `dict_en.xml`.
* **A-4** Semantics of the db.dat check (0x4574f0 / 0x457600) and which contents pass it.
* **A-5** Path base used for the content files (0x4515f0 joins a base string with the name;
  data2.db is opened with a bare name). Working directory versus executable directory not
  settled.
* **A-6** The server's own content loaders (Server.exe) were not re-examined.
* **X-1** Interface indicator values in every in-world and menu state, observed on a running
  client; identities of the nodes G+0x800880, +0x800888, +0x80088c, +0x800890, +0x800894,
  +0x800898.
* **X-3** Which zone list holds plant sprites: the plant pass appends through 0x4862e0 (a list
  append, zone+0x04 per the trees spec); the terrain and vegetation specs name zone+0x24.
* **X-4** Storage of block light per zone.
* **X-5** Full Spawn (0x10f0-byte) layout, so that pointer-valued fields can be identified.
* **X-6** The second map (W+0x0c) and list (W+0x14) of the world object.
* **X-7** Whether the generator writes into neighbouring published zones (overview Q2).
* **X-8** Element types of zone+0x48, +0x54, +0x88, +0x94.
* **X-9** The receive-thread clock write (0x46cbcc/0x46cbd2) lies in code the evidence
  disassembler did not reach (read from raw bytes); its packet number was not re-derived.

(X-2 of the previous revision is resolved, correction 9.)

---

## Re-certification (2026-11-16)

Re-checked against `Cube.exe` (SHA-256 above): its decompile and assembly, raw image bytes and
the original installation's files. Infinite-Cubes/alpha was not needed. Every retained claim
was confirmed at the cited address unless listed below or kept as an open question.

Corrections:

1. **Scope and title.** The document now covers the native content store and data formats
   (Part A: inventory, identifiers, SQLite blob schema, scramble table and algorithm, payload
   formats, fonts, db.dat, client-created files), which the previous revision omitted.
   Evidence: SQL strings 0x6ffb20..0x6ffc30, open 0x4497b0, fetch 0x449810, decoder 0x4496a0,
   table 0x6ffa68; installation file headers.
2. **Non-native material removed.** Advice about a loadable module, hotkeys, code patching,
   page protection, capture contents, comparison procedure and a project name was removed;
   native facts it relied on are kept as neutral descriptions (§B5.2 lock coverage, §B5.3 entry
   points). The address-rebasing formula and the pseudo-structure listing of §2.1 were replaced
   by prose and a table.
3. **Streaming thread run flag** is byte G+0x8005b0 (set 0x4635c6, tested 0x46a8e4, loop
   back-edge 0x46b70f → 0x46a8f1, cleared 0x466fed), not G+0x800584, which governs only the
   mesher and map threads (0x4690d8, 0x4695b1; set 0x463512).
4. **Kept zone area** is a disc: zones with squared zone distance < 16 survive; "±3 zones
   survive" was wrong for (3, 3) (distance² 18). Evidence 0x46a8a0 clean-up loop (test < 0x10).
   Also: the clean-up runs when more than 1000 ms have passed (comparison with 0x3e9), and on a
   world change all zones, regions and cells are unloaded without distance tests.
5. **New-zone choice** detail: candidate ±3 window clamped to 0..65535, strict comparison with
   the running best squared distance initialised to 0x90000 (768²). Evidence 0x46a8a0.
6. **Zone +0x6c** is the end pointer of the +0x68 edit vector, not an independent i32: the save
   test in 0x5a5050 compares +0x68 with +0x6c. Row removed; +0x76 and +0xa4 rows added
   (constructor 0x6240f0).
7. **Implicit block values** given exactly: solid grey 200/200/200 type byte 0x01 (0x76afc4),
   air 255/255/255 byte 0x00 (0x76afc0), water 255/255/255 byte 0x82 (0x76afbc), from static
   initialisers 0x6f9720/0x6f9750/0x6f9780. Out-of-range coordinates and unloaded zones also
   read as solid grey (0x42f7e0).
8. **Generator** 0x5e4850 returns the zone pointer (null for out-of-range coordinates) rather
   than nothing, and after block light it runs a statics-to-sprites pass (0x5edf83..0x5ee048)
   before the publishing store at 0x5ee05a.
9. **W+0xb8 offline (old X-2 resolved).** The constructor sets W+0xb8 to the start-up player
   through setter 0x487e90 (call 0x46222c) after inserting it into the creature map under id 1;
   the join path writes it again at 0x470408.
10. **WorldMap at G+0x800d44** is confirmed (constructor 0x5fae40 called with that address at
    0x45a5b2), no longer "believed".
11. **Clock writer address.** The receive thread writes day (G+0x800444) and time (G+0x800440)
    at 0x46cbcc/0x46cbd2 after entering W.worldLock at 0x46cbb9; the cited 0x46cb42 is not that
    write. Packet association not re-derived (X-9).
12. **In-game root condition** spans 0x4ae27e..0x4ae354 and requires seven nodes hidden
    (G+0x800874, +0x800880, +0x800888, +0x800894, +0x80088c, +0x800890, +0x800898), not the
    narrower range 0x4ae2f5..0x4ae356. 0x488bd0 toggles a node's visibility; it is not a
    reader.
13. **Region unload** 0x5a4800 first unloads the region's zones (0x5a4af0) before taking its
    own locks; region-cell unload frees with `operator delete`.
14. **Zone save condition** added: 0x5a5050 saves only when W+0xa4 is non-zero and the zone has
    "items changed" or a non-empty edit vector.
15. **Thread mapping evidence** made explicit: functor call slots 0x46db70/0x46db80/0x46dba0
    jump to 0x4690a0/0x469590/0x46a8a0; the streaming thread keeps normal priority (only
    0x800588 and 0x80058c are lowered).
16. **Frame details** added: the frame does nothing unless both the device and G are non-null;
    the update argument is the ms since the previous frame (0 first); 0x4c8940 is called between
    update and draw when one of the four i32 at G+0x170..+0x17f changes.
17. **Quit flag** G+0x1a0 documented (set by font-load failure and by the db.dat check; WinMain
    posts a quit message when set).
18. **Column lookup range** is 0 ≤ x, y < 2^24 blocks; a given zone must contain the block
    (0x4347a0).

Remaining uncertainties: A-1 … A-6, X-1, X-3 … X-9 above. Counts of rows per store are those of
the reference installation, not limits of the format. The save stores' lack of scrambling is
inferred from the absence of decoder calls, not checked against a save file.

Cross-spec conflicts (other specs not edited):

* `alpha-audio.md` §3 and AU-3 use "zone field +0x6c" as a camera distance (gate +0x6c < 4096).
  In `cube::Zone` +0x6c is the end pointer of the edit vector (0x5a5050 compares +0x68 and
  +0x6c; constructor 0x6240f0). Either the audio field belongs to a different draw-list object
  or that claim needs re-certification.
* Terrain/vegetation specs name zone+0x24 as the plant-sprite list and zone+0x18 as "static
  objects"; the trees spec and the list append 0x4862e0 place sprites at zone+0x04, and
  zone+0x18 is the Spawn-pointer vector (X-3, unchanged).
* `alpha-net-packets.md` associates the clock write with packet 5 at 0x46cb42 (as cited by the
  previous revision); the write found here is at 0x46cbcc/0x46cbd2 (correction 11).
