Status: FINAL / STABLE / HELD

# Alpha saves: characters, world list, worlds and maps

Behavioural description of the save databases of Cube World **Alpha**: the files the client
(`Cube.exe`) keeps for characters, the world list, per-world state and the world map, and the
per-world file the dedicated server (`Server.exe`) keeps. Every statement below was checked
against the original images only. Addresses are RVAs (image base 0x400000 in both images) and
always name the image. The asset databases `data1.db`–`data4.db` are a separate subject
(`alpha-data-stores.md`).

---

## 0. Conventions

* Integers are little-endian; fields follow one another with no alignment padding. Widths:
  u8/u32 unsigned, i32/i64 signed, f32 an IEEE-754 single, "word" a 4-byte value whose type
  has not been established.
* "string" means a u32 byte count *n* followed by *n* bytes of 8-bit text with no terminator.
* "zlib" means a stream produced by zlib's one-shot compression (`Cube.exe` RVA 0x1fc0d0,
  with an output buffer slightly larger than the input) and read back by streaming inflation
  (`Cube.exe` RVA 0x49540) in 128,000-byte steps. The client imports these from `ZLIB1.DLL`
  and initialises inflation with the version string "1.2.3".
* Every serialised record that carries a **format version** word gets it from the record
  writer's initial state, which is fixed at 1 (`Cube.exe` RVA 0x4a8a0); the map-database
  records take theirs from a map-object field that the map constructor sets to 1
  (`Cube.exe` RVA 0x1fae40). The character reader stores the version without testing it.
* The readers examined here (character record, world entry, `time`, `discovered`) are
  truncation-tolerant: a field is taken only when enough bytes remain;
  otherwise the read position is moved to the end and that field (and all later ones) keep
  the value they had before the read.
* Coordinates used in keys:
  * a world position is three i64 values; a position divided by 65,536 gives a block
    coordinate, and a block coordinate divided by 256 gives a **zone** index (both divisions
    truncate toward zero, `Cube.exe` RVA 0x878a0);
  * a **region** is 64 × 64 zones (region = zone ÷ 64, zone-within-region = zone mod 64,
    `Cube.exe` RVA 0x1a4890); region indices are bounded to 0–1023 by the region routines;
  * a **map cell** is 8 × 8 zones; the 8 × 8 map cells of region (*rx*, *ry*) are numbered
    *rx*·8 + *i*, *ry*·8 + *j* for *i*, *j* in 0–7 (`Cube.exe` RVA 0x1a4af0).
* Records that are copied whole from memory (items, inventory slots, the 28-byte land
  trailer) carry whatever padding bytes the in-memory structure held.

---

## 1. Files

| File | Written by | Content |
|---|---|---|
| `Save/characters.db` | client | the character list |
| `Save/worlds.db` | client | the world list shown in the world menu |
| `Save/world_<name>.db` | client for its local worlds; server as `Save/world_server_<seed>.db` | per-world state: clock, zones, map-cell mission and monster state |
| `Save/map_<name>.db` | client only | the world-map knowledge for that world |

* `<name>` is the world's name. The client builds the open paths with a forward slash
  (`Save/world_` at `Cube.exe` RVA 0x1a52e0, `Save/map_` at RVA 0x1fbc90, the list files at
  RVA 0x64efc and 0x65014 inside RVA 0x59c40). A backslash form (`Save\world_`, `Save\map_`)
  appears only in the world-deletion routine, which removes both files with the Windows
  file-delete call (RVA 0x81d30). The suffix is `.db` in every case.
* The server names its world `server_` followed by its seed in decimal (`Server.exe` RVA
  0x149c50), so its file is `Save/world_server_<seed>.db` (path built at `Server.exe` RVA
  0xd83a0). The server image contains no characters, world-list or map database path.
* The world list separates entries whose name begins with the seven characters `online_`
  from the others (`Cube.exe` RVA 0xa23d0). No code that builds such a name was found; see
  U-1.
* When the client is connected to a server it sets its world object's network-client flag
  (world object +0xb4; set at `Cube.exe` RVA 0x6fc50, cleared on disconnect at RVA 0x719f0).
  While that flag is set the client does not open `world_<name>.db`, does not save zones on
  unload, and does not save missions or monsters. The map database is opened whenever the
  world name is non-empty, regardless of that flag.

---

## 2. The database format (all four kinds)

Each file is a SQLite 3 database; both images embed SQLite 3.7.15.2. The game uses one table:

    CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);

and only these statements (`Cube.exe` RVA 0x49720–0x49b6b; `Server.exe` RVA 0x13010–0x133bb
for the first three):

* **open**: the file is opened (and created if absent), then the table-creation statement
  above is executed every time; its failure when the table already exists is ignored
  (`Cube.exe` RVA 0x497b0, `Server.exe` RVA 0x13010).
* **read**: `SELECT value FROM blobs WHERE key = ?`; the value bytes are copied out. A key
  that is absent yields "not found" (`Cube.exe` RVA 0x49810, `Server.exe` RVA 0x13070).
* **write**: `SELECT 1 FROM blobs WHERE key = ?`; if a row exists,
  `UPDATE blobs SET value=? WHERE key=?`, otherwise `INSERT INTO blobs(key, value) VALUES(?, ?)`.
  A statement is retried while finalisation reports a schema change. The console text
  "DATABASE WRITE ERROR" is printed only when the INSERT statement cannot be prepared
  (`Cube.exe` RVA 0x499f0, `Server.exe` RVA 0x13240).
* **delete** `DELETE FROM blobs WHERE key = ?` and **rename**
  `UPDATE blobs SET key = ? WHERE key = ?` exist in the client only (`Cube.exe` RVA 0x49720,
  0x49920); the server image does not contain these statements.

Values are stored exactly as serialised; no byte-swapping or other transform of the asset
stores is applied. Compression is per record kind, as stated below.

Keys are ASCII: a prefix followed by decimal integers, with `_` between two coordinates (for
example `zone<x>_<y>`), or a bare decimal index in the two list databases.

---

## 3. `characters.db`

### 3.1 Keys and list maintenance

| Key | Value |
|---|---|
| `num` | u32: number of characters *N* |
| `0` … `N−1` | zlib(character record, §3.2) |

* At start-up the client opens `characters.db`, reads `num` (zero characters if the key is
  absent) and loads keys 0 … N−1 (`Cube.exe` RVA 0x64efc–0x65012 inside RVA 0x59c40).
* Creating a character (`Cube.exe` RVA 0x821a0) copies the typed name into the creature,
  sets level 1, writes `num` = N+1, then saves the new character under key N.
* Deleting character *k* (`Cube.exe` RVA 0x816f0) writes `num` = N−1 first, deletes key *k*,
  then renames keys *k*+1 … N−1 to *k* … N−2 in ascending order.
* A character is saved (`Cube.exe` RVA 0x87520, which serialises with RVA 0x4d790 and
  compresses the whole record) on: creation; an automatic save every 60,000 ms while the
  current world name is non-empty and a further in-game state flag is set (loop at
  `Cube.exe` RVA 0x690a0); return to the main menu from the in-game menu (RVA 0x7ddd0);
  disconnection from a server (RVA 0x719f0); and client shutdown (RVA 0x66d90). Before
  serialising, when the current world name is non-empty, the save routine stores the current
  position in the character's per-world position table (field 19) under the current world's
  seed and name.
* Loading key *k* (`Cube.exe` RVA 0x806c0) inflates and parses the record (RVA 0x4be40),
  then **restores the position the creature had before the load**, so the position stored
  in field 2 is not applied by this load. It then copies that position into a second
  position triple at creature +0x1350 and copies level and experience into two tracking
  words at creature +0x1d3c and +0x1d40.

### 3.2 Character record

Writer `Cube.exe` RVA 0x4d790, reader RVA 0x4be40. "Creature +x" is an offset into the
in-memory creature; "extra +x" is an offset into a 64-byte per-character data object that
the creature points to at creature +0x1d28.

| # | Type | Source | Meaning |
|---|---|---|---|
| 1 | u32 | — | format version (1) |
| 2 | 3 × i64 | creature +0x10 | position (see §3.1: not applied on load) |
| 3 | 3 × word | creature +0x28 | not established (see U-2) |
| 4 | f32 | creature +0x16c | health (refilled to the maximum on level-up, `Cube.exe` RVA 0x47b00) |
| 5 | i32 | creature +0x194 | experience (converted into levels at RVA 0x47b00) |
| 6 | i32 | creature +0x190 | level |
| 7 | u8 | creature +0x140 | class |
| 8 | u8 | creature +0x141 | specialization (changed by a paid action that also rewrites the skill levels, `Cube.exe` RVA 0x7b600) |
| 9 | word | creature +0x1198 | unknown; preserved across the connect-time character copy (RVA 0x6fc50) |
| 10 | word | creature +0x119c | unknown; preserved likewise |
| 11 | 13 × item | creature +0x300 | equipment slots 0–12, 280 bytes each, stride 0x118 (item layout: `alpha-net-packets.md` §1.1) |
| 12 | string | creature +0x1168 | character name (written as the length of the zero-terminated text; the reader copies the bytes and terminates them) |
| 13 | 20 bytes | extra +0x00 | character style (§3.3) |
| 14 | u32 *T* | creature +0x11dc | number of inventory tabs |
| 15 | *T* × (u32 *n*, *n* × 284 bytes) | creature +0x11dc | each tab: *n* slots; a slot is a u32 count followed by a 280-byte item |
| 16 | i32 | creature +0x1304 | primary money balance (tested and deducted for several paid actions, including the specialization change, `Cube.exe` RVA 0x7b600) |
| 17 | i32 | creature +0x1308 | second spendable balance (tested and deducted for the action that rewrites an item's 16-bit field at item +0x10 to the character's level, RVA 0x7b600) |
| 18 | u32 *k*, *k* × item | extra +0x14 (count at extra +0x18) | an item list; meaning not established (U-2) |
| 19 | u32 *m*, *m* × (u32 seed, string name, 3 × i64 position) | extra +0x1c (count at extra +0x20) | per-world position table keyed by the (seed, name) pair |
| 20 | u32 | extra +0x24 | seed of the character's last world |
| 21 | string | extra +0x28 | name of that world (fields 20–21 are compared with a world entry's seed and name by the world-preview routine, `Cube.exe` RVA 0x205ae0) |
| 22 | i32 | creature +0x1164 | a counter; the skill points available are one quarter of it plus two per level above level 1 (RVA 0x7b600) |
| 23 | u32 = 11, then 11 × i32 | creature +0x1138 | skill-level count and skill levels; the reader takes the 44 bytes only when the count is exactly 11 |

Reader behaviour (`Cube.exe` RVA 0x4be40) beyond the layout: before parsing it resets the
character's inventory to four tabs and its two balances to template values, replaces the
64-byte per-character object with a fresh one, and clears creature byte +0x60. The 20 style
bytes are parsed into the fresh object and the creature type and appearance are immediately
re-derived from them (§3.3). Each tab is resized to its stored slot count before its slots
are copied. Item-list entries are read into a default item and appended.

### 3.3 Character style (field 13) and derived appearance

The creature type and appearance are **not stored** as such; after reading field 13 the
reader calls a derivation routine (`Cube.exe` RVA 0x3f7c0) that sets the creature type
(creature +0x64) and the appearance block (creature +0x78) from the 20 style bytes:

| Off | Type | Use in the derivation |
|---|---|---|
| 0 | u32 | race selector 0–7; any other value leaves the creature type unchanged |
| 4 | u8 | its lowest bit selects one of the two creature types of the race |
| 5 | 3 bytes | not used by the derivation |
| 8 | i32 | head/face choice (reduced by a per-race modulus and added to a per-race base model) |
| 12 | i32 | hair choice (reduced and offset likewise) |
| 16 | 3 × u8 | hair colour R, G, B (copied into appearance +2…+4) |
| 19 | u8 | not used by the derivation |

Creature type from race selector *r* and the low bit *b*: *r* 0 → 2 + *b*, 1 → 0 + *b*,
2 → 9 + *b*, 3 → 11 + *b*, 4 → 4 + *b*, 5 → 7 + *b*, 6 → 15 + *b*, 7 → 13 + *b*. The
routine also sets fixed appearance scale values; the per-race model bases and moduli are
outside this document.

---

## 4. `worlds.db`

### 4.1 Keys and list maintenance

| Key | Value |
|---|---|
| `num` | u32: number of worlds *N* |
| `0` … `N−1` | world entry, §4.2 (not compressed as a whole) |

* At start-up the client reads `num` and loads entries 0 … N−1 (`Cube.exe` RVA 0x65014–
  0x6510f inside RVA 0x59c40; entry reader RVA 0x809a0).
* Creating a world from the menu (`Cube.exe` RVA 0x82530) writes `num` = N+1, appends the
  entry (seed parsed from the typed text, name from the name field) and writes it under
  key N.
* Entering a world for which no entry with the same seed and name exists (`Cube.exe` RVA
  0x6a8a0) likewise writes `num` = N+1 and writes a new entry under key N with a
  discovered counter of 0.
* An entry is rewritten (writer `Cube.exe` RVA 0x878a0) on return to the main menu
  (RVA 0x7ddd0) and at client shutdown (RVA 0x66d90; only when the selected entry matches
  the current world's seed and name).
* Deleting a world (`Cube.exe` RVA 0x81d30) deletes its two database files, removes the
  entry from the in-memory list, adjusts the selected index if it is now past the end,
  writes `num` = N−1, and then writes **every remaining entry in turn under the single key
  of the selected index** (the last write wins). No other key is deleted or renamed by this
  routine.
* The client copies the map's current discovered counter into the selected entry (inside
  `Cube.exe` RVA 0x88ee0, at RVA 0x8c1d2); the writer prints "wrote" and the seed to the
  console.

### 4.2 World entry

| Type | Meaning |
|---|---|
| string | world name |
| u32 | world seed |
| u32 | discovered counter (the map database's `discovered` value, §6.1) |
| 3 × i32 | preview model size X, Y, Z |
| u32 *c* + *c* bytes | present only when all three sizes are > 0: zlib of X·Y·Z × 3 bytes (R, G, B per voxel) |

When the entry being written is the current world (same name and seed), the writer first
refreshes the preview from the map-tile model (§6.4) of the zone that contains the player.

---

## 5. `world_<name>.db`

Opened by the world-setup routine (`Cube.exe` RVA 0x1a52e0, `Server.exe` RVA 0xd83a0) only
when the world name is non-empty and the network-client flag is clear. Keys:

| Key | Grid | Value |
|---|---|---|
| `time` | — | §5.1 (8 bytes, no version word) |
| `zone<x>_<y>` | zone | §5.2 |
| `mission<x>_<y>` | map cell | §5.3 (38 bytes) |
| `monster<x>_<y>` | map cell | §5.4 (21 bytes) |

### 5.1 `time`

    u32 day number
    u32 time of day in milliseconds

* The world constructors set day 0 and 43,200,000 ms (`Cube.exe` RVA 0x18eb00,
  `Server.exe` RVA 0xc8570). The setup routine does not reset the clock; it only replaces
  day and time with the stored values when the key is present.
* The time of day advances during the world update; a day rolls over (day + 1, time −
  86,400,000) while the time is **greater than** 86,400,000, so 86,400,000 itself is a
  value the clock can hold. With an empty world name the time is held at 32,400,000.
* The update writes the key whenever its elapsed-time accumulator crosses a multiple of
  10,000 ms, the database is open and the name is non-empty (`Cube.exe` within RVA
  0x20c510 at 0x20c6e4–0x20c7f4; `Server.exe` within RVA 0x1322d0 at 0x1324a4–0x1325b4).

### 5.2 `zone<x>_<y>`

Serialiser `Cube.exe` RVA 0x4e340 (`Server.exe` RVA 0x1faa0). A zone is saved
(`Cube.exe` RVA 0x1a5050, `Server.exe` RVA 0xd81b0) only when the world name is non-empty and
either the zone's modified flag (zone +0x75) is set or its block-edit list is non-empty. In
order:

1. u32 format version (1).
2. u32 *n*, then *n* **world items** of 325 bytes each: item (280) · 24 bytes (three 8-byte
   values) · word · word · u8 · word · word · word. In memory these are 328-byte entries;
   the writer copies the fields without the 3 bytes of internal padding. When the modified
   flag is clear the writer stores *n* = 0.
3. u32 *e*, then *e* **block edits** of 20 bytes each (copied whole).
4. u32 *s*, then *s* pairs of words: two words per **dynamic object** of the zone, taken from
   object offsets 0x38 and 0x3c (U-3).
5. u32 *t*, then *t* bytes: one byte per **static entity** of the zone, in the zone's
   static-entity order (byte 0x30 of each 392-byte entity).

Zone saves happen when a zone is unloaded (`Cube.exe` RVA 0x1a4890), for every loaded zone
when the world is set up again (RVA 0x1a52e0) or destroyed (RVA 0x193ed0), and in the server
equivalents (`Server.exe` RVA 0xd83a0 and its callers of RVA 0xd81b0). The client's unload
and destroy paths skip zone saves while the network-client flag is set. Readers: `Cube.exe`
RVA 0x1e4850, `Server.exe` RVA 0x118630.

### 5.3 `mission<x>_<y>`

Serialiser `Cube.exe` RVA 0x4e120 (`Server.exe` RVA 0x1f880); 38 bytes:

| Off | Type | Meaning |
|---|---|---|
| 0 | u32 | format version (1) |
| 4 | word | unknown |
| 8 | word | unknown |
| 12 | word | unknown |
| 16 | word | unknown |
| 20 | u8 | unknown |
| 21 | u8 | unknown |
| 22 | word | unknown |
| 26 | word | unknown |
| 30 | 8 bytes | unknown |

### 5.4 `monster<x>_<y>`

Serialiser `Cube.exe` RVA 0x4e280 (`Server.exe` RVA 0x1f9e0); 21 bytes:

| Off | Type | Meaning |
|---|---|---|
| 0 | u32 | format version (1) |
| 4 | word | unknown |
| 8 | word | unknown |
| 12 | u8 | unknown |
| 13 | 8 bytes | unknown |

Missions and monsters are written for all 8 × 8 map cells of a region at once, mission then
monster for each cell (`Cube.exe` RVA 0x1a4af0, `Server.exe` RVA 0xd7c50), only when the
network-client flag is clear, the name is non-empty, the region index is within 0–1023 and
the region's record-state flag (region +0x15a18) is set. The client does this when a region
is unloaded (RVA 0x1a4800) and when the world is destroyed (RVA 0x193ed0). When a region is
generated, a stored mission record for a cell replaces that cell's generated mission state
and sets the record-state flag (`Cube.exe` RVA 0x1da280; server reader `Server.exe` RVA
0x10e080), and the monster record is read in the same pass.

---

## 6. `map_<name>.db`

Client only. Opened when a world with a non-empty name is set up for the map (`Cube.exe`
RVA 0x1fbc90), which first flushes and closes the previous map (RVA 0x201f80).

| Key | Grid | Value |
|---|---|---|
| `discovered` | — | §6.1 (bare u32, no version word) |
| `land<x>_<y>` | region | §6.2 |
| `reg<x>_<y>` | region | §6.3 (60,164 bytes) |
| `tile<x>_<y>` | zone | §6.4 |

### 6.1 `discovered`

The number of zones whose map cell has had bit 0 of its flag byte (cell +0x30) set; the
counter is incremented only when that bit is set for the first time (`Cube.exe` RVA
0x1fc160). It is read when the map opens (RVA 0x1fbc90) and written after every tile write
(RVA 0x203a00), when a map region is unloaded (RVA 0x202160) and when the map is closed
(RVA 0x201f80), which then resets it to 0.

### 6.2 `land<x>_<y>`

Writer `Cube.exe` RVA 0x2050b0 (reader references the key at RVA 0x2024d0). Written only
when the land object has a picture model:

| Type | Meaning |
|---|---|
| u32 | format version (1) |
| 8 bytes | land object +0x20 (two words, unknown) |
| u8 | land object +0x28 (unknown) |
| 3 × i32 | picture size X, Y, Z |
| u32 *c* + *c* bytes | only when all sizes are > 0: zlib of X·Y·Z × 3 bytes RGB |
| 28 bytes | land object +0x04 … +0x1f, copied whole (unknown) |

Land records are written when the map is closed (RVA 0x201f80) and when land data is
unloaded (from RVA 0x1fbed0).

### 6.3 `reg<x>_<y>`

Writer `Cube.exe` RVA 0x205420 (reader references the key at RVA 0x203230). Written when a
map region is unloaded (RVA 0x202160) and for every map region when the map is closed
(RVA 0x201f80).

| Part | Size | Content |
|---|---|---|
| version | 4 | u32 1 |
| 64 × 64 zone cells | 4096 × 14 | per 52-byte zone cell, in the region's storage order: u8, u8, u8, u8, u8, word, word, u8 from cell bytes 0x30, 0x10, 0x11, 0x12, 0x13, words 0x14 and 0x18, byte 0x1c |
| 8 × 8 map-cell records | 64 × 44 | per 104-byte record: word × 5, word, 16 bytes, word, from record offsets 0x18, 0x1c, 0x20, 0x24, 0x28, 0x14, 0x00–0x0f, 0x10 |

4 + 57,344 + 2,816 = 60,164 bytes. The first byte of each zone cell (cell +0x30) is the flag
byte of §6.1. Other field meanings: U-4.

### 6.4 `tile<x>_<y>`

The voxel model of one zone shown on the world map; generated, read and written by
`Cube.exe` RVA 0x203a00 for zone indices 0–65535 (the generation pass covers a 32 × 32
column grid):

| Type | Meaning |
|---|---|
| u32 | format version (1) |
| word | zone cell +0x04 (unknown) |
| 3 × i32 | model size X, Y, Z |
| u32 *c* + *c* bytes | only when all sizes are > 0: zlib of X·Y·Z × 3 bytes RGB |
| u32 *k* | number of trailing entries (zone cell +0x24) |
| *k* × 12 bytes | trailing entries, from a list held by the zone cell (8 bytes + 4 bytes each; meaning U-4) |

When a stored tile is read, the model is rebuilt from it, bit 1 of the cell flag byte is set,
and each of the *k* entries is handed to a map routine (RVA 0x201eb0).

---

## 7. `options.cfg` and `server.cfg`

* `options.cfg` (client, working directory) is text with one `key value` pair per line, read
  at `Cube.exe` RVA 0xce6e0 and written at RVA 0xcef80. Its keys are `fullscreen`,
  `resolutionX`, `resolutionY`, `antiAliasing`, `renderDistance`, `soundVolume`,
  `musicVolume`, `cameraSpeed`, `cameraSmoothness`, `language`, `invertY` and
  `minTimeStep`. None of them names a port, address, player name or save folder.
* `server.cfg` (server, working directory) holds the world seed as one decimal integer. If
  it cannot be read, the server takes a C-runtime random value modulo 1,000,000 and writes it
  to `server.cfg` (`Server.exe` RVA 0x149c50). See `alpha-net-server.md` §1.1.

---

## 8. Unresolved

* **U-1** The world name the client uses while connected to a server. Only a test for the
  prefix `online_` (RVA 0xa23d0) was found, no code that builds such a name, so whether and
  under which name `map_<name>.db` is written online is open. (`world_<name>.db` is not
  opened while connected; §1.)
* **U-2** Character fields 3 (three words at creature +0x28), 9, 10 and 18 (meaning of the
  item list), style bytes 5–7 and 19, the per-race model bases and moduli of §3.3, the
  meaning of creature byte +0x60 cleared on load, and any length limit on the name.
* **U-3** Meaning of the two words per dynamic object in zone records.
* **U-4** Meanings of the `reg` cell bytes and map-cell words, the `land` header and
  trailer, the `tile` header word and trailing entries; field-by-field symmetry of the `reg`
  and `land` readers (RVA 0x203230, 0x2024d0) with their writers.
* **U-5** Meanings and types of all `mission` and `monster` fields; what sets the region
  record-state flag apart from loading a stored mission.
* **U-6** Types of the world-item fields after the item; meaning of the zone modified flag
  beyond its save effect; meaning of the static-entity byte.
* **U-7** Whether `time`, zone, mission/monster and static-entity data are identical to
  records the server sends over the network (companion network specs); not re-verified.
* **U-8** Whether any reader other than the character reader tests the version word.

---

## Re-certification (2026-11-14)

Re-established against `Cube.exe` and `Server.exe` image bytes and their disassembly and
decompilation only; no save file was opened.

**Confirmed:** the single `blobs` table and its statement texts; SQLite embedded in both
images; per-kind compression (whole character record; preview, land and tile pictures);
character record order, widths and creature/extra offsets; equipment 13 × 280 bytes;
inventory 284-byte slots (u32 count + item); skill count gate of exactly 11; world entry
layout (no version, preview only when all sizes > 0); key prefixes and grids; zone record
order with 325-byte world items, 20-byte block edits, two words per dynamic object and one
byte per 392-byte static entity; mission 38 bytes and monster 21 bytes with their field
order; `reg` layout and 60,164-byte size; `discovered` as a bare u32; noon default clock;
character deletion by delete-and-rename; client-only map database; no server character
storage; `server.cfg` as a decimal seed; `options.cfg` key set.

**Corrected (old → new):**
1. Addresses were absolute virtual addresses labelled as addresses → RVAs with the image
   named.
2. "Client spells world paths with a backslash" → the open paths use `/`; `\` appears only
   in world-file deletion.
3. Table creation "when the file is new" → executed on every open, failure ignored.
4. "The same statements in the server" → the server has no delete or rename statements.
5. "DATABASE WRITE ERROR on a failed write" → printed only when the INSERT cannot be
   prepared.
6. Mission "target creature type" and "two coordinates", monster "creature type" and
   "zone x, y of the monster's place", and character field 3 "orientation" → unknown words
   (those labels rested on save contents or unverified network mapping).
7. **Reversal:** "appearance and creature type are not saved / open question" → they are
   derived on load from the 20 style bytes (field 13), whose race, sex bit, face, hair and
   hair-colour bytes are now specified.
8. "On load the client copies the position to its interpolation state and resets the
   controller byte to 0 (player)" → the list load discards the stored position, restoring
   the pre-load position; byte +0x60 is cleared by the reader (meaning unresolved).
9. Save triggers: added the 60-second automatic save, return-to-menu, character creation;
   "from the character screens" narrowed to creation.
10. Field meanings: field 16 confirmed as the money balance, field 17 as a second spendable
    balance, field 22 as a counter feeding skill points, fields 20–21 as last-world seed and
    name; field 4/5/6 semantics tied to level-up evidence.
11. Time range "0 … 86,399,999" → rollover only above 86,400,000; time written on
    10,000 ms accumulator boundaries; clock held at 32,400,000 for an unnamed world; setup
    does not reset the clock.
12. Tile trailer "u32 unknown" → u32 count followed by that many 12-byte entries.
13. `discovered` "increases each time something new is discovered" → counts first-time
    setting of bit 0 of a zone cell's flag byte.
14. World deletion: added the native behaviour of rewriting all remaining entries under
    the selected index's key and deleting both world files.
15. Zone save condition "changed or holds items" → modified flag set or block-edit list
    non-empty; items are written only with the modified flag.
16. World-item field types "f32 · f32 · u8 · 3 × i32" → widths only (types unresolved).
17. Online world name "`online_<seed>`" → not confirmed (U-1); confirmed instead that the
    world database is not opened while connected.

**Removed:** every value, count and size taken from save files (sample positions, names,
seeds, sizes, colour bytes, counts and "verified byte-for-byte" claims); network-bit numbers
and network-record equivalence claims (now U-7); the section addressed to a replacement and
all advice to readers or writers; the verification paragraph about save copies and recorded
sessions; the zone-number "middle of the world" remark and the claim that negative
coordinates never occur.

**Unresolved:** U-1 to U-8 above.
