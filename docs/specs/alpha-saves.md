# Alpha saves: characters, world list, worlds and maps

Behavioural specification of the save files of Cube World **Alpha** (`Cube.exe`, and the
world database that `Server.exe` writes), so that AlphaVulkan can read and write them
interchangeably with the original game. Written from the Alpha decompile and checked
byte-for-byte against the user's own Alpha saves (read-only):
`Cube World Alpha\Save\characters.db`, `worlds.db`, `world_test.db`, `map_test.db`. Every
layout marked "verified" below parses those files exactly to their last byte.

Routines are cited by address in `Cube.exe` unless marked "server". The asset databases
`data1.db`–`data4.db` are a different matter, described in `alpha-data-stores.md`.

---

## 0. Conventions

* Little-endian integers, IEEE-754 floats, no alignment: fields follow each other. "u32",
  "i32", "u64", "i64", "f32", "u8" as in `alpha-net-transport.md`.
* "string" means u32 byte length *n* followed by *n* bytes of text (no terminator); text is
  the game's narrow (8-bit) encoding.
* "zlib" means an RFC 1950 stream made by zlib `compress()` (client 0x5fc0d0) and read with
  `inflate()` (client 0x449540).
* Records copied whole from memory (items, inventory slots) keep their in-memory padding.
  Padding bytes in real saves contain leftover memory (for example the bytes "EG" in
  inventory items); readers must ignore them and writers should write zero.
* Coordinates in keys use three grid sizes, all derived from the block position
  (i64 / 65,536):
  * **zone** = block ÷ 256 (zone 32,806 ≈ the middle of the world);
  * **map cell** = zone ÷ 8 (a cell is 8 × 8 zones);
  * **region** = zone ÷ 64 (a region is 64 × 64 zones = 8 × 8 map cells).
  See the world-generation specs for what zones, cells and regions contain.
* Every structured blob begins with a u32 **format version**; all observed blobs have 1.
  Readers should accept 1 and refuse anything else.

---

## 1. Files

| File | Written by | Content |
|---|---|---|
| `Save/characters.db` | client | the character list |
| `Save/worlds.db` | client | the world list shown in the world menu |
| `Save/world_<name>.db` | client; server as `Save/world_server_<seed>.db` | per-world dynamic state: clock, zones, map-cell quest and monster state |
| `Save/map_<name>.db` | client only | the player's world-map knowledge for that world: discovered count, map tiles, land pictures, region map data |

* `<name>` is the world's name as typed in the menu (e.g. `test`). For a multiplayer world
  the client uses the name `online_<seed>` (decimal seed), so its local files would be
  `world_online_<seed>.db` and `map_online_<seed>.db`; whether the client actually writes
  them while online is **open question V-1**.
* Paths are relative to the game's working directory. The client spells world paths with a
  backslash (`Save\world_`, `Save\map_`) and the list files with a slash; both work on
  Windows.
* There is no save for the server's players: characters live only on each player's
  machine.

---

## 2. The database format (all four kinds)

Each file is an ordinary **SQLite 3** database with exactly one table:

    CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);

(plus SQLite's automatic index for the primary key). SQLite is linked into both binaries;
no other tables, triggers or views exist. The game uses only these statements (client
0x4497b0 … 0x4499f0, the same text in the server):

* create: `CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);` when the file is new;
* read: `SELECT value FROM blobs WHERE key = ?`;
* write: `SELECT 1 FROM blobs WHERE key = ?`, then `UPDATE blobs SET value=? WHERE key=?`
  if the key exists, otherwise `INSERT INTO blobs(key, value) VALUES(?, ?)`; a failed
  write prints "DATABASE WRITE ERROR";
* delete and rename (used when a character is deleted): `DELETE FROM blobs WHERE key = ?`,
  `UPDATE blobs SET key = ? WHERE key = ?`.

Values are **not scrambled** (the byte-swap transform of the asset stores is never applied
to saves). Compression is per kind, as stated in each section.

Keys are plain ASCII built from a prefix and decimal numbers, with `_` between two
coordinates: `zone32801_32799`, `mission4096_4100`, `tile32800_32800`. Negative numbers
never occur in practice (all coordinates are near 2¹⁵ zones).

---

## 3. `characters.db`

### 3.1 Keys

| Key | Value |
|---|---|
| `num` | u32: number of characters *N* |
| `0` … `N−1` | zlib(character record), §3.2 |

Deleting character *k* (client 0x4816f0) deletes key *k*, renames keys *k*+1 … *N*−1 down
by one, and rewrites `num`. Characters are saved (client 0x487520 → 0x44d790) when the
player leaves a world, disconnects, quits, and from the character screens.

### 3.2 Character record (verified)

Writer client 0x44d790, reader client 0x44be40. The record is the inflated value. Offsets in
the "From" column are into the in-memory creature (creature object offset; the network
record of `alpha-net-packets.md` §2.1 starts at creature +0x10, so network offset =
creature offset − 0x10).

| # | Type | From | Meaning |
|---|---|---|---|
| 1 | u32 | — | format version, 1 |
| 2 | 3 × i64 | +0x10 | position (network bit 0) |
| 3 | 3 × f32 | +0x28 | orientation (network bit 1) |
| 4 | f32 | +0x16c | health (network bit 27) |
| 5 | i32 | +0x194 | experience (network bit 34) |
| 6 | i32 | +0x190 | level (network bit 33) |
| 7 | u8 | +0x140 | class (network bit 21) |
| 8 | u8 | +0x141 | specialization (network bit 22) |
| 9 | u32 | +0x1198 | *unknown*, not on the network (1.0 as f32 in the sample) |
| 10 | u32 | +0x119c | *unknown*, not on the network (0 in the sample) |
| 11 | 13 × item | +0x300 | equipment slots 0–12 (280 bytes each, `alpha-net-packets.md` §1.1) |
| 12 | string | +0x1168 | name (the network name field, max 15 characters) |
| 13 | 20 bytes | extra data +0x00 | *unknown* (the last bytes look like an RGB colour, ff c8 32, in the sample) |
| 14 | u32 *T* | +0x11dc | number of inventory tabs (4 in the sample) |
| 15 | *T* × (u32 *n*, *n* × 284 bytes) | +0x11dc | each tab: *n* slots (50 in the sample), each slot = u32 stack count + item |
| 16 | u32 | +0x1304 | *unknown* (21 in the sample; money is likely) |
| 17 | u32 | +0x1308 | *unknown* (0 in the sample) |
| 18 | u32 *k*, *k* × item | extra data +0x14 | an item list (15 in the sample: one of each kind of potion, food and ingredient; the crafting recipes known is likely) |
| 19 | u32 *m*, *m* × (u32 world seed, string world name, 3 × i64 position) | extra data +0x1c | the character's last position in each world it has visited |
| 20 | u32 | extra data +0x24 | seed of the world the character was last in |
| 21 | string | extra data +0x28 | name of that world |
| 22 | u32 | +0x1164 | *unknown* (network bit 47) |
| 23 | u32 = 11, then 11 × i32 | +0x1138 | skill count and skill levels (network bit 46); the reader only accepts the levels when the count is exactly 11 |

"Extra data" is a 64-byte object the creature points to (creature +0x1d28) that holds
per-character persistent data; field 13 is its first 20 bytes.

Sample (`characters.db` key `0`, 1,530 bytes compressed, 64,866 inflated): position
(550,398,629,740, 550,344,041,405, 4,199,551), health 136.7, level 1, experience 3, class 1,
equipment in slots 7–10, name "test", 4 tabs × 50 slots, one world entry (seed 2,412,123,
"test", same position), current world seed 2,412,123 "test", skills all 0.

Notes:

* The **appearance and creature type are not saved** in fields 1–23. Either they are fully
  derived from field 13 or from fixed values when a character is loaded, or they live in a
  part of the record not yet named; **open question V-2**. AlphaVulkan must preserve field 13
  byte for byte.
* On load (client 0x4806c0) the client also copies the position to its interpolation state
  and resets the creature's controller byte to 0 (player).

---

## 4. `worlds.db`

### 4.1 Keys

| Key | Value |
|---|---|
| `num` | u32: number of worlds *N* |
| `0` … `N−1` | world entry, §4.2 (**not** compressed as a whole) |

The client appends an entry when a new world is entered for the first time (client
0x46a8a0) and rewrites an entry (client 0x4878a0) when the player leaves the world (called
from client 0x466d90) and from the world menus.

### 4.2 World entry (verified)

| Type | Meaning |
|---|---|
| string | world name |
| u32 | world seed |
| u32 | the world's "discovered" counter (the same number as the `discovered` key of its map database, §6.1) |
| 3 × i32 | preview model size X, Y, Z (voxels) |
| u32 *c* + *c* bytes | only when all three sizes are > 0: zlib of X·Y·Z × 3 bytes (R, G, B per voxel) |

The preview is the map-tile model (§6.4) of the zone the player was in when the entry was
written. Sample: "test", seed 2,412,123, discovered 38, size 32 × 32 × 11, 9,547
compressed bytes (33,792 inflated).

---

## 5. `world_<name>.db`

Written by the client for single-player worlds and by the server for its world. Keys:

| Key | Grid | Value |
|---|---|---|
| `time` | — | §5.1 (8 bytes, raw) |
| `zone<x>_<y>` | zone | §5.2 (raw) |
| `mission<x>_<y>` | map cell | §5.3 (raw, 38 bytes) |
| `monster<x>_<y>` | map cell | §5.4 (raw, 21 bytes) |

### 5.1 `time` (verified)

    u32 day number
    u32 time of day in milliseconds (0 … 86,399,999)

Exactly the two values of network packet 5. Sample: day 5, 14,615,510 ms. A new world
starts at day 0, 43,200,000 ms (noon; client 0x58eb00). Written by the frame update
(client 0x60c510); read when the world opens (server 0x4d83a0, client 0x5a52e0).

### 5.2 `zone<x>_<y>` (verified)

Dynamic state of one zone, written when the zone is saved (client 0x5a5050 → 0x44e340) and
only if it has been changed or holds items. In order:

1. u32 format version (1).
2. u32 *n*, then *n* **world items**, each 325 bytes:
   item (280) · 3 × i64 position · f32 · f32 · u8 · 3 × i32
   (the same fields as network section 7 records, `alpha-net-packets.md` §3.7, without the
   3 padding bytes). If the zone's "has items" flag is clear the writer stores *n* = 0.
3. u32 *e*, then *e* **block edits** of 20 bytes each (network section 1 layout,
   `alpha-net-packets.md` §3.5).
4. u32 *s*, then *s* pairs of u32: two words of state per **dynamic object** of the zone
   (taken from object offsets 0x38 and 0x3c; meaning **open question V-3**).
5. u32 *t*, then *t* bytes: one state byte per **static entity** of the zone, in the zone's
   static-entity order (the byte at offset 0x30 of each 392-byte entity; 1 in every observed
   save). This is the byte the server sends in network section 6.

Sample `zone32801_32799`: 9 world items (type 11 plants lying in the world), no block
edits, 15 dynamic objects, 1 static entity.

### 5.3 `mission<x>_<y>` (verified size)

Quest state of one map cell, 38 bytes (client 0x5a4af0 → 0x44e120):

| Off | Type | Meaning |
|---|---|---|
| 0 | u32 | format version (1) |
| 4 | u32 | *unknown* (17,607 in a cell with a quest) |
| 8 | u32 | *unknown* |
| 12 | u32 | creature type of the quest target (matches the cell's `monster` record) |
| 16 | u32 | *unknown* (1 in a cell with a quest) |
| 20 | u8 | *unknown* |
| 21 | u8 | *unknown* |
| 22 | u32 | *unknown* |
| 26 | u32 | *unknown* |
| 30 | 8 bytes | *unknown* (two i32 coordinates are likely) |

### 5.4 `monster<x>_<y>` (verified size)

Monster state of one map cell, 21 bytes (client 0x44e280):

| Off | Type | Meaning |
|---|---|---|
| 0 | u32 | format version (1) |
| 4 | u32 | creature type (0 = no monster in this cell) |
| 8 | u32 | *unknown* (4 seen) |
| 12 | u8 | *unknown* |
| 13 | 2 × i32 | zone x, zone y of the monster's place (seen: 32,773 / 32,794); leftover bytes when the type is 0 |

Missions and monsters are written for all 8 × 8 cells of a region at once (the sample has
cells 4096–4103 × 4096–4103, i.e. region 512, 512), when the region is saved (client
0x5a4800, server 0x4d7c50). The same data is what the server sends in network section 13.

---

## 6. `map_<name>.db`

The player's map knowledge, written only by the client (client 0x601f80, 0x602160,
0x603a00, 0x6050b0, 0x605420), when map regions are unloaded and when the world is left.

| Key | Grid | Value |
|---|---|---|
| `discovered` | — | u32: the discovered counter (38 in the sample; it increases each time something new is discovered, client 0x5fc160) |
| `land<x>_<y>` | region | §6.2 |
| `reg<x>_<y>` | region | §6.3 (raw, 60,164 bytes) |
| `tile<x>_<y>` | zone | §6.4 |

Note that the `discovered` value is a **bare u32** without a version word.

### 6.2 `land<x>_<y>` (verified)

| Type | Meaning |
|---|---|
| u32 | format version (1) |
| 2 × i32 | *unknown* (values near 32,600–32,800) |
| u8 | *unknown* flag |
| 3 × i32 | picture size X, Y, Z (95 × 98 × 1 in the sample) |
| u32 *c* + *c* bytes | only when all sizes are > 0: zlib of X·Y·Z × 3 bytes RGB |
| 28 bytes | *unknown* (copied whole from the land object) |

### 6.3 `reg<x>_<y>` (verified size)

| Part | Size | Content |
|---|---|---|
| version | 4 | u32 1 |
| 64 × 64 cells | 4096 × 14 | per cell, in row order: u8, u8, u8, u8, u8, u32, u32, u8 (from the cell struct's bytes 0x30, 0x10, 0x11, 0x12, 0x13, words 0x14 and 0x18, byte 0x1c) |
| 8 × 8 records | 64 × 44 | per map cell: u32 ×5, u32, 16 bytes, u32 (from the 104-byte map-cell struct's offsets 0x18, 0x1c, 0x20, 0x24, 0x28, 0x14, 0x00–0x0f, 0x10) |

4 + 57,344 + 2,816 = 60,164 bytes, the size of every `reg` value in the sample. The grid of
64 × 64 is per zone of the region; the 8 × 8 records are per map cell (the same 104-byte
structs whose words feed network section 13). Field meanings: **open question V-4**.

### 6.4 `tile<x>_<y>` (verified)

The voxel model drawn for one zone on the world map and the minimap:

| Type | Meaning |
|---|---|
| u32 | format version (1) |
| i32 | *unknown* (6–10 seen; a base height is likely) |
| 3 × i32 | model size X, Y, Z (32 × 32 × 8–12 seen) |
| u32 *c* + *c* bytes | only when all sizes are > 0: zlib of X·Y·Z × 3 bytes RGB |
| u32 | *unknown* (0 in all 105 samples) |

---

## 7. `options.cfg` and `server.cfg`

* `options.cfg` (client, working directory) is text, one `key value` pair per line. The
  observed file holds `fullscreen 0`, `resolutionX 1920`, `resolutionY 1080`,
  `antiAliasing 0`, `renderDistance 100`, `soundVolume 100`, `musicVolume 100`,
  `cameraSpeed 50`, `cameraSmoothness 100`, `language 0`, `invertY 0`, `minTimeStep 16`.
  **None of these affects networking or saves**; there is no port, address, player-name or
  save-folder option.
* `server.cfg` (server, working directory) is the world seed as one decimal integer
  (`alpha-net-server.md` §1.1).

---

## 8. Reading and writing them in AlphaVulkan

1. Open the databases with any SQLite 3 library; create the `blobs` table when creating a
   file; never add other tables (Alpha would ignore them, but keeping the files identical
   in shape keeps them portable).
2. Read and write the values exactly as in sections 3–6; keep every *unknown* field and
   padding run byte for byte when rewriting a value that came from Alpha.
3. Compress only what Alpha compresses: the whole character record; the preview, land and
   tile pictures inside their records. Everything else is raw.
4. Keys are case-sensitive ASCII; use Alpha's prefixes and the coordinate grids of §0.
5. Write `num` after adding or removing list entries, as Alpha does.

---

## 9. Open questions (and how to settle them)

Verification uses copies of the save files (never the originals) and sessions recorded
with the user's Alpha mod DLLs.

* **V-1** Does the client write `world_online_<seed>.db`/`map_online_<seed>.db` while
  connected? Check the `Save` folder of a copy of the game after a short online session
  against a test server.
* **V-2** Where the character's appearance and race are saved (field 13 is the main
  suspect). Create two characters differing only in hair colour on a copy of the game and
  diff the inflated records.
* **V-3** Meaning of the two words per dynamic object in zone records.
* **V-4** Meanings of the `reg` cell bytes and map-cell words, of the `land` header and
  trailer, and of the `tile` header word (open the world map in a session and diff
  `map_*.db` before and after).
* **V-5** Meanings of the `mission` and `monster` fields (accept and finish a quest, then
  diff the cell).
* **V-6** Character fields 9, 10, 16, 17 and 22 (spend money, gain mana or stamina, and
  diff).
