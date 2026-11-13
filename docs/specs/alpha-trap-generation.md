Status: FINAL / STABLE / HELD. Full author readback completed.

# Type-7 zone-object producer: the dungeon generation pass and its trap draws

This independently authored, code-free contract identifies the world-generation routine that produces the type-7 zone objects (server `0x100300` / client `0x1CC510`), places it in the zone-generation order, describes its arguments, frame state and cell grid, and lists its random-draw sequence in order so that the trap draws (modulo 40, rotation modulo 4, phase modulo 4000) can be inserted at the exact point of each cell iteration. The held contract `alpha-hazards-blast-ticks.md` was used only to locate the routine and for terms (zone-object record layout, object-append routine, activity fields); its section 2 summary of the type-7 producer is confirmed and extended here. The Infinite-Cubes reconstruction (`alpha/src/cube/WorldGeneration.cpp`, `alpha/include/cube/Dungeon.h`) was used only as a lead for where the passes are. Every fact below was re-established from the original images; where only decompiler output of the original image was read (not instruction listings), this is said.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`. Every address is an RVA; where one address is given it is the server RVA. Inside the dungeon routine the client lies `0xCC210` above the server; inside the zone-generation entry routine it lies `0xCC220` above (section 9). Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method and conventions

The dungeon routine and the zone-generation entry routine have no first-pass decompiler output (both timed out); the longer-budget retries `decompiled/server/retry-default-180/functions/00500300.c` and `.../00518630.c` exist and were read as decompiler output of the original server image. Instruction listings (`decompiled/server/assembly-functions/00500300.asm`, `.../00518630.asm`, and the matching client files `005cc510.asm`, `005e4850.asm`) were used to confirm the trap window, the call sites, the argument pushes, the draw call sites and their order. The ordered list of draw call sites and drawing callees inside the routine was taken from `decompiled/server/calls.csv`, and the order of draws stated in section 5 agrees with that address order. Helper routines named below were read in decompiler output and, where marked, in instruction listings.

**Draw.** One call of the C runtime's random function, imported from MSVCR110 through the import slot server `0x1582F4` / client `0x2FC468` (symbol name `rand` in the import table). Its value is non-negative, so every signed remainder below is an ordinary remainder. "Draw mod n" means that remainder. "Even" means draw mod 2 is 0. Where a draw is turned into a binary32 value, the order of the multiplication by a constant and the division by 32767 (binary32 constant at server `0x158834`) is stated at the site, because it differs between sites.

**Seed.** The C runtime's seeding function, slot server `0x15832C` / client `0x2FC4B4` (symbol name `srand`).

**Block coordinates** below are integer world block coordinates; "raw" means the 64-bit fixed-point form with 65536 units per block (as in the held contract). Positions given to zone objects are integer block coordinates converted to raw by the integer-to-raw helper server `0x0CDE40`.

**Fill-box draws.** Many draws of the dungeon routine are made inside the box-fill routine server `0x113400` / client `0x1DF600` (instruction-identical, section 7). Its contract for draws, read from its instruction listing:
1. It always makes exactly **three draws** first, before looking at the size: it builds a 23-entry jitter table whose entries 0, 8 and 16 are each a draw mod 20 (in that order) and whose other entries are 0.
2. Then, when all three box sizes are positive, it visits every block of the box with the x offset outermost (ascending), the y offset in the middle (ascending) and the z offset innermost (descending, from size z − 1 to 0), and makes **exactly one draw per block**. That draw, converted to binary32 and divided by 32767, is compared with the routine's roughness argument; the comparison selects one of two block type codes (type byte 6 or 13, each with bit `0x40`, plus the caller's flag byte); the block colour comes from the jitter table and the block is written by the set-block routine server `0x01FF00`, which makes no draw.
3. When any size is zero or negative, no per-block draws are made; the three table draws are still made.

So a fill of size (a, b, c) makes **3 + a·b·c draws** when a, b and c are all positive and **3 draws** otherwise. In the rest of this document "fill (a, b, c) at (x, y, z)" means one call of this routine with that size and origin, and the draw count follows from this rule. The roughness and colour arguments do not change the draw count.

**Non-drawing helpers.** The following make no draw (no random call in their decompiler output or any callee's): the cell accessor server `0x0F84A0`, its coordinate transform `0x12DDE0`, the rotated-extent queries `0x12D820`, `0x12D840`, `0x12D860`, the closed-cell test `0x122820`, the cut-box routine `0x0D2500`, the carpet fill `0x0FF340`, the terrain-block colour routine `0x0F9450` and its column-value helpers `0x0FC140` and `0x0FC100`, the set-block routine `0x01FF00`, the block and column lookups `0x005FD0` and `0x006100`, the zone-object initialiser `0x0C84B0`, the object-append routine `0x0D6670`, and the list appends `0x128450` (zone `+0x04`), `0x1284A0` (zone `+0x48`) and `0x0F2BE0` (zone `+0x18`). Two runtime helpers that the call graph links to the random function (the security-cookie check `0x14A8B8` and an array-construction helper `0x0016C0`, which here calls the trivial routine `0x01CBA0`) were checked and make no draw on the paths used here.

**Drawing callees not audited here.** The dungeon routine also calls these routines, each of which makes its own draws; their internal draw sequences are outside this contract: the room carve `0x1236D0` and the corridor link `0x0F9010` (both summarised in section 5.2), the torch builder `0x12C370`, the wall-object builder `0x12A830`, the container-inventory filler `0x0FDD80` (through `0x12C160`), the item generator `0x12B470`, the recipe-item generator `0x12A760`, the rarity draw `0x12BF40` and the sprite-placement routine `0x124540`.

## 2. Identity, caller chain and position in zone generation

### 2.1 What the routine builds

The routine constructs a frame-local object whose virtual table the image's run-time type information names `cube::Dungeon` (server vftable `0x173944`, type descriptor string `.?AVDungeon@cube@@` at `0x183A5C`), lays out rooms and corridors in that object's three-dimensional cell grid, and then builds the result as blocks, zone objects, plants, items, creatures and generation records of a zone. It is the **dungeon generation pass**. The type-7 zone objects (the hazards whose world-update behaviour is described in the held contract) are placed on cell floors inside this pass. "Dungeon", "room", "corridor", "entrance", "stair" and similar words are descriptive labels for observed roles only.

### 2.2 Caller chain

1. **Generation worker** (server `0x149550`, started through the thread entry thunk `0x149B90`). While its running flag is set, it walks its list of positions in list order. For each position it considers zone coordinates around it (x outer, y inner, each clamped to 0–65535) whose squared zone distance from the position is below the current best (initially 9), and accepts a coordinate when its sector record is missing or the sector's zone slot for it is empty. The first coordinate with the strictly smallest squared distance wins. If one was accepted, it calls the zone-generation entry (at `0x1497D4`) with that zone's x and y. The client's equivalent worker is client `0x06A8A0` (thunk `0x06DBA0`), the only caller of the client entry in the client call graph; it was located but not audited.
2. **Zone-generation entry** (server `0x118630` / client `0x1E4850`). Receiver: the world. Arguments: zone x, zone y (signed 32-bit, both required to be in 0–65535, else it returns at once). Its passes are listed in 2.4.
3. **Dungeon pass** (server `0x100300` / client `0x1CC510`), called from the entry at server `0x11C2A3` and `0x11C2D7` (client `0x1E84C3` and `0x1E84F7`). These are the only two call sites in either image (`calls.csv` of both images).

### 2.3 When it is called

The entry routine locates the **zone-cell record** of the zone: the 16-byte record at sector `+0x18` + 16 × ((zone x mod 64) × 64 + (zone y mod 64)), where the sector is the pointer at world `+0xBC` + 4 × ((zone x / 64) × 1024 + zone y / 64). At server `0x11A94C` it copies the record's 16 bytes into the zone at `+0x78`–`+0x87`. Then, after the passes listed below:
- At server `0x11C282`: if the record's first byte (zone `+0x78`) is **3**, the dungeon pass is called with variant = the record's second byte (zone `+0x79`) and the large-layout byte **0**.
- At server `0x11C2B4`: if the record's first byte is **5**, the dungeon pass is called with variant **3** and the large-layout byte **1**.

Because the first byte cannot be both 3 and 5, a zone runs the dungeon pass at most once. Who writes the zone-cell records was not traced.

### 2.4 Order of passes in the zone-generation entry (server RVAs; client = server + `0xCC220`)

1. `0x1186A2`–`0x11870E`: for each of the 3 × 3 sectors around the zone's sector, the sector-preparation routine `0x10E080` (not audited).
2. `0x118732`: the existing-zone lookup `0x006290`; if the zone exists the entry returns.
3. `0x11873F`–`0x118776`: a 200-byte zone is allocated and constructed (`0x148B60`).
4. **Seed** at `0x118795`: the seeding function is called with (world `+0x800188`, 32-bit) + zone y × 65536 + zone x, in 32-bit wrapping arithmetic. Every draw listed in this contract for this zone comes after this seed.
5. `0x1187A7`–`0x118801`: zone `+0x60` and `+0x64` are set to zone x and y; two helpers (`0x02E090`, `0x0D5A80`) are called with the zone centre block (x × 256 + 128, y × 256 + 128). Not audited.
6. From `0x118806` to the buffer release at `0x11A93E`: **terrain column pass** over the zone's 256 × 256 block columns. Direct draw sites at `0x11A214` and `0x11A229`; its callees were not audited for draws. A position list created at `0x1188B0` and used by pass 13 is filled here.
7. `0x11A94C`–`0x11A9A0`: the zone-cell record copy (2.3).
8. From `0x11A9B2`: **region-kind feature pass**, steered by the 104-byte region record at sector `+0x14018` + 104 × ((zone y / 8 mod 8) + 8 × (zone x / 8 mod 8)) (kind at record `+0x18`). Draw sites: `0x11AA80`, `0x11AC47`, `0x11AC5E`, `0x11AC75` (region kind 6 grid), `0x11AECC` (kind 11), `0x11AFD6` (kind 12, discarded draw), and, when the zone-cell first byte is not 4, 1 or 3, `0x11B088`, `0x11B136`, `0x11B149`, `0x11B15C`, `0x11B186`, `0x11B1A9`, `0x11B3D6`; plus the draws of the feature builders `0x0FFBF0`, `0x0FF3F0` and `0x113760` (not audited). A zone whose zone-cell first byte is 3 (the small dungeon case) therefore skips the last group.
9. A surface column pass over the zone's columns (its calls lie at `0x11B489`–`0x11B9A2`); no draw sites.
10. A second surface column pass (its list is created at `0x11BC0D`). For each column where it writes a type-3 surface block, it makes **one draw** (`0x11C094`); when that draw mod 200 is 0 the column position is appended to a list (`0x0D9B70` at `0x11C0E0`).
11. `0x11C282`–`0x11C2D7`: **the dungeon pass** (2.3).
12. From `0x11C2DC` to `0x11C66E`: for each position of the list from pass 10, three draws (each mod 3, plus 2) give ellipsoid radii, and an ellipsoid of type-6 blocks is written (`0x11C30D`, `0x11C324`, `0x11C33B`).
13. From `0x11C66E`: for each position of the list from pass 6, three draws (mod 4 plus 4, mod 4 plus 4, mod 6 plus 4) and an ellipsoid of type-38 blocks (`0x11C69F`, `0x11C6BA`, `0x11C6D5`).
14. Later passes: start-location placement (`0x124540` at `0x11CB5B`, only for the zone holding the world's start position), a parity-gated placement with up to ten attempts of two draws each (`0x11CBBB`, `0x11CBF6`), a feature count draw when the zone-cell first byte is not 4, 1 or 3 (`0x11CD95`), and later passes (draw sites from `0x11CE95` onward). Not audited.

So the dungeon pass runs **after** the terrain, region-feature and surface passes and **before** the two ellipsoid passes; its draws follow the type-3-surface 1-in-200 draws of pass 10 and precede the three-per-entry draws of pass 12.

## 3. Arguments and frame state of the dungeon routine

The receiver is the world. The stack arguments are:

| Frame offset | Value from the callers | Use |
| --- | --- | --- |
| `+0x08` | the zone being generated | zone fields `+0x80` (32-bit, call it **level L**), `+0x84` (byte, call it **rarity Q**), `+0x79` (style byte), and the lists below |
| `+0x0C` | the sector pointer | never read |
| `+0x10` | zone x × 256 + 32 | **x base X0** (block x of cell column 0) |
| `+0x14` | zone y × 256 + 32 | **y base Y0** |
| `+0x18` | variant V: zone-cell second byte (call at `0x11C2A3`) or 3 (call at `0x11C2D7`) | style, layout and building choices |
| `+0x1C` | byte **G**: 0 (call at `0x11C2A3`) or 1 (call at `0x11C2D7`) | large-layout selector; read at `0x1008CF`, `0x1058E3`, `0x108BF7`, `0x1091DB`, `0x109668`, `0x109AC3` |

L and Q are bytes `+0x08`–`+0x0B` and `+0x0C` of the zone-cell record (copied to the zone). The **z base D** is computed inside the routine (5.4). The zone lists written are: zone objects `+0x0C` (append routine `0x0D6670`), plants `+0x04`, creatures `+0x18`, items `+0x30`, and generation records `+0x48`.

In the cell loop the frame holds the cell indices x at `EBP−0x2B50`, y at `EBP−0x2B4C` and z at `EBP−0x2B48`, the bases X0 at `EBP−0x2B38`, Y0 at `EBP−0x2B34` and D at `EBP−0x2B54`, and the current cell pointer at `EBP−0x2B60` (server instruction listing around `0x1058CA`).

## 4. The dungeon cell grid

**Object.** The frame-local dungeon object is constructed by `0x0F7370` with sizes 22, 22, 22. Fields: `+0x04` rotation r (32-bit, initially 0), `+0x08` reflect byte (initially 0), `+0x0C`, `+0x10`, `+0x14` the x, y and z sizes, `+0x18` the cell array. All cells are zeroed at construction.

**Cell record.** Two bytes. Byte 0 is the **kind**; byte 1 holds **flags**. Cell (i, j, k) of the stored array is at array + 2 × ((size y × k + j) × size x + i).

| Kind | Producer |
| --- | --- |
| 0 | initial (empty) |
| 1 | never stored by the producers found; it is the kind of the out-of-range default record (below) |
| 2 | raised platform inside a room (room carve) and raised columns (column-raising step of 5.3) |
| 3 | open cell: room interior (room carve) and corridor path (corridor line routine `0x1234B0`) |
| 4 | the single entrance cell (5.2, step 6) |

| Flag bit of byte 1 | Producer |
| --- | --- |
| `0x01` | vertical step: set by the corridor line routine `0x1234B0` on the lower of two vertically adjacent path cells wherever the path changes z (both cells are also made kind 3) |
| `0x02` | carpet area: set by the room carve on a random sub-rectangle of a room's floor layer |
| `0x04` | guard: set on the centre cell of each room marked guard (5.2, step 4) |

**Rotated access.** Every cell read in the build passes, and every write after layout, goes through the accessor `0x0F84A0`, which first maps the requested (i, j) through the object's rotation and reflection (`0x12DDE0`): with r taken mod 4, r = 1 maps (i, j) to (size x − 1 − j, i); r = 2 to (size x − 1 − i, size y − 1 − j); r = 3 to (j, size y − 1 − i); r = 0 leaves it; then, when the reflect byte is nonzero, j becomes size y − 1 − j. k is not transformed. If the mapped cell is outside the grid, the accessor returns a single shared static record whose kind byte is set to 1 on every such access and whose flag byte is initialised to 0 once. The rotated extents used by the build loops are `0x12D820` (size x when r is even, else size y), `0x12D840` (the other) and `0x12D860` (size z); here all are 22.

**What the trap gate reads.** The "first byte value 4" is the entrance kind and the "bit 0 of the second byte" is the vertical-step flag of the cell being visited.

**Rooms.** The routine also keeps a frame array of 27 room records of 28 bytes. Room index n = 9·a + 3·b + c for lattice coordinates a, b, c in 0–2. Record bytes 0–3: **used**, **guard**, **no-population** and **fixed** flags; then three 32-bit **offsets** (ox, oy, oz) and three 32-bit **size adjustments** (sx, sy, sz). A room occupies the cell box with origin (7a + 2 + ox, 7b + 2 + oy, 7c + 3 + oz) and size (3 + sx, 3 + sy, 1 + sz). A fixed room's offsets are replaced by (0, 0, 0) and its adjustments by (0, 0, 1) before carving (values read from decompiler output only).

## 5. The draw sequence of the dungeon routine

All draws use the C runtime generator of section 1, in the state left by the seed of 2.4 step 4 and the draws of the passes before the dungeon pass. Server draw sites are given; client sites are server + `0xCC210`.

### 5.1 Style (before the room lattice)

1. Variants 1 and 2 only: **one draw** mod 3 (`0x1005F0`), choosing an extra creature-race style. Other variants: none. (The variant dispatch at `0x1003CA` uses a five-entry table for variants 1–5; variant 0, 4 and any other value take the default branch.)

### 5.2 Room lattice, layout, carving and corridors

2. **Room lattice** (`0x1007F8`–`0x100852`): for room index n from 0 to 26 in order, **six draws**: draw 1 mod 3 is sz; draw 2 mod 3 minus 1 is sy; draw 3 mod 3 minus 1 is sx; draw 4 mod 3 minus 1 is oz; draw 5 mod 2 is oy; draw 6 mod 2 is ox. 162 draws.
3. **Layout** (when G is 0; when G is nonzero no draw is made and rooms 13, 16, 4, 22, 1 and 19 are marked used, none guard):
   - Variant 2: **one discarded draw**, then **one draw** (room 4 used when even), then **one draw** (room 6 used when even) (`0x100B19`, `0x100B42`, `0x100B65`). Rooms 11, 16, 25, 22, 19, 7, 0 and 3 are used; room 12 is used and guard.
   - Variants 4 and 5: **one discarded draw**, then **one draw** (room 4), then **one draw** (room 6), each used when even (`0x10195E`, `0x101990`, `0x1019B3`). Rooms 16, 25, 22, 19, 7, 0 and 3 are used; room 14 is used, no-population and fixed; room 12 is used and guard.
   - Every other variant: **one draw** mod 3 selects a layout (`0x100E94`); room 10 is used; then three draws, each marking a room used when even:
     - layout 1 (`0x1015F0`, `0x101613`, `0x10163C`): rooms 19, 1, 22; rooms 13, 4, 7 and 16 used; room 25 used and guard;
     - layout 2 (`0x101221`, `0x101244`, `0x101267`): rooms 19, 7, 25; rooms 1, 4, 13 and 22 used; room 16 used and guard;
     - layout 0 (`0x100EB3`, `0x100ED6`, `0x100EF2`): rooms 19, 7, 16; rooms 1, 4, 13 and 22 used; room 25 used and guard.

     (The layout draw's remainder 1 branches to `0x1015F0`, remainder 2 to `0x101221`, and remainder 0 falls through to `0x100EB3`; instruction listing.)

   Each layout also builds an ordered link list of room pairs (appends through `0x0F3BA0` and `0x028590`). Its contents are fixed per layout and not reproduced here.
4. **Carving** (after the grid construction at `0x101DC5`): rooms in index order. For each used room: when it is neither guard nor fixed, **one draw** mod 3 (`0x101F29`; the room gets a raised platform when it is 0); then the room carve `0x1236D0` (`0x101F52`) marks the room's cells kind 3 and makes its own draws: **one draw** (carpet when odd); when odd, **two draws** (carpet width = draw mod room size x, plus 1; depth = draw mod room size y, plus 1) and up to **two more** (an x offset, draw mod (size x − width + 1), only when size x − width is positive; then a y offset likewise), and the flag `0x02` is set on the carpet cells of the room's lowest layer; then, when the room has a raised platform and size x and size y both exceed 2, **two draws** (width = draw mod (size x − 2), plus 1; depth likewise) and up to **two more** (an x offset, draw mod (size x − width − 1), only when size x − width exceeds 1; then y likewise), and the platform cells over the room's full height become kind 2. (The room carve makes no draw at all when any room size is not positive.) Then, for a guard room, flag `0x04` is set on the cell at the room box origin plus half its size on each axis.
5. **Corridors**: for each link in list order whose two rooms are both used, the corridor link `0x0F9010` (`0x1021CE`) draws a cell line (`0x1234B0`) and makes **exactly two draws** (each deciding a parallel side line when odd) when the two rooms' x ranges overlap; otherwise exactly two draws when their y ranges overlap; otherwise **no draw**.
6. **Entrance**: the cell at (7a + 2 + ox, 7b + 1 + oy, 7c + 3 + oz) of the first link's first room becomes kind 4. No draw.
7. **Orientation** (`0x1022D1`, `0x1022EB`): **one draw** mod 4 is the initial rotation r0, stored in the object; **one draw**: reflect is set when even.
8. **Colours** (`0x102310`–`0x1025B1`): roof colour **three draws**, each mod 50 plus 20, assigned to the third, second and first component in draw order; brightness **one draw** mod 50; variant 3 only, **one more draw** mod 50 plus 100 replaces the brightness; wall colour **three draws**, each mod 50 plus 50 plus brightness (third, second, first); floor colour **three draws** likewise. Then variants 1 and 2: **six draws** replacing wall and floor colours (for each: third mod 50 plus 50, second mod 50 plus 50, first mod 20 plus 50); variant 4: **six draws** (third mod 50 plus 20, second mod 50 plus 80, first mod 50 plus 100, twice); variant 5: fixed colours, no draw. The roughness used by the fills is a constant per variant group (server `0x1586A4`, variants 1 and 2 `0x173954`, variant 4 `0x173958`); it does not change any draw count.

### 5.3 Column raising (variants 0, 1, 2 and 3 only)

9. For each column of the rotated extents (outer index along the first extent, inner along the second; extents swap with the parity of r0): unless the variant is 2, **one draw** mod 6 (`0x102709`), and when it is 0 **one more draw** mod 3 (`0x102717`) giving a height of that plus 2 (else height 1; variant 2 always height 1, no draw). Then, scanning k from 21 down to 0 through the rotated accessor and skipping kind-4 cells, the first nonzero cell found has the cells above it, up to height cells but not beyond k = 21, set to kind 2. No other draw.

### 5.4 Depth selection (no draws)

10. Four trials: r becomes (r + 1) mod 4 and is stored. The entrance cell is the last kind-4 cell found scanning k from 21 down to 0, then the first extent, then the second (so the lowest such k); if none, (0, 0, 0). The ground g is found at block column (X0 + 10·ex + 5, Y0 + 10·ey + 5): if that column does not exist, **the routine ends at once** (no further draw, nothing more written). Otherwise g starts at the column's base plus height and decreases while the block's type (low five bits of block byte 3) is 0, then is increased by 1. The trial is wet when any block in the 10 × 10 × 10 box with origin (X0 + 10·ex, Y0 + 10·ey, g − 5) has type 2 or 3. Its score is g + (100 when wet) − 10·ez. The first trial, and any later trial with a strictly smaller score, is kept (score, rotation, entrance cell, wet).
11. Variants 4 and 5 add 50 to the kept score; a wet result subtracts 100; if the score plus 10 × kept ez is negative, the score becomes −10 × kept ez. The rotation is set to the kept rotation. **D** is this score. Cell (x, y, z) occupies blocks X0 + 10x … + 9, Y0 + 10y … + 9, D + 10z … + 9.

### 5.5 Carpet colours

12. (`0x102C09`–`0x102C66`) carpet colour **three draws** and carpet border colour **three draws**, each mod 180 plus 50 (third, second, first component in draw order); then variants 0 and 3 only, **one discarded draw** (`0x102CA3`).

### 5.6 Column build pass (draws only through fills)

13. (fill calls from `0x102EB7` to `0x1044AD`) For each column (x′ over the first rotated extent outer, y′ over the second inner) with origin bx = X0 + 10x′, by = Y0 + 10y′: the lowest top T₀ of the 100 block columns (bx + i, by + j), i, j in 0–9, is found. Then k runs from 21 down to 0 over the cell (x′, y′, k):
    - kind 0: nothing;
    - the first nonempty cell sets the column top Tc = D + 10(k + 1), minus 10 when it is kind 4;
    - kind 4: marks the column as above the entrance; no fill;
    - any other kind: if it is the first nonempty cell and T₀ < D + 10k + 5, the variant's top structure is built first: variant 2: fill (10, 10, 3) at (bx, by, D + 10k + 11); variants 0, 1 and 3: for each side in the order x−1, x+1, y−1, y+1 whose neighbour cells at k and at k + 1 both have kind 0, 4 or 1, a fill (2, 16, 3) for x sides or (16, 2, 3) for y sides along that side at height D + 10(k + 1), then eight merlons each consisting of a conditional single-block write (no draw) and a fill (1, 1, 1); then variant 3 only, when all four side neighbours at k have kind 0, 4 or 1, a fill (14, 14, 4) at (bx − 2, by − 2, D + 10(k + 1)) followed by seven fills (s, s, 2) for s = 14, 12, …, 2 at (bx − 2 + m, by − 2 + m, D + 10k + 14 + 2m), m = 0…6. Then, for every such cell (first or not), the **shell fill** (14, 14, 14) at (bx − 2, by − 2, D + 10k − 2).

    After the scan, when a nonempty cell was found or the variant is 4 or 5, and the block column (bx, by) exists: the lowest base B of the existing columns (bx + i, by + j), i, j in −2…11, is found. Variants 0, 1, 2 and 3: the **foundation fill** (14, 14, Tc − B) at (bx − 2, by − 2, B) (flag byte `0x80` when above the entrance). Variant 4: the stepped-temple fills: a fill (14, 14, t − B − 4) at (bx − 2, by − 2, B) where t is D + 10 × (kept ez + step − 5), plus 2 near the centre, step being 10 minus the column's Chebyshev distance from the centre column (10, 10), minus one more at the centre; when t − B is positive, fills (12, 12, 3) at (bx − 1, by − 1, t − 4) and (10, 10, 3) at (bx, by, t − 1) and a rim of terrain blocks (no draws); and on the centre row or column at distance 2 or more, three runs of eleven stair fills ((14, 1, m + 3), (2, 1, m + 5), (2, 1, m + 5) for the y-direction, or the transposed sizes for the x-direction, m = −1…9). Variant 5: nothing here.
14. Variant 5 only (fill call at `0x104674`): for every block column (bx over X0 … X0 + 219 outer, by over Y0 … Y0 + 219 inner) that exists, one fill (1, 1, h) at (bx, by, base) forming a pyramid, h being D + 10 × kept ez − Chebyshev distance from the apex (X0 + 105, Y0 + 105) − 51 + 110 − base. These are 48 400 or fewer fills of at least three draws each.

### 5.7 The cell loop (this is where the trap draws sit)

15. **Loop** (from `0x10477A`; exit at `0x107B92`): x from 0 to the first rotated extent − 1 (outer, ascending), y from 0 to the second − 1 (middle), z from 0 to 21 (inner, ascending). Each cell is read through the rotated accessor. Only cells of kind **3** or **4** are processed; cells of kind 0, 1 or 2 are skipped with no draw. For a processed cell, in this order:

    1. **Entrance record** (kind 4 only, `0x1048C7`): a generation record of kind 5 with block position (X0 + 10x + 5, Y0 + 10y + 5, D + 10z) is appended to zone `+0x48`. No draw.
    2. **Cut** (`0x10495A`, all variants 0–5): the cut-box routine clears the 10 × 10 × 10 block box at (X0 + 10x, Y0 + 10y, D + 10z) (its flag argument is 1 for variants 1, 2 and 4). No draw. For kind 4 a zone object is then initialised and destroyed unused (no effect).
    3. **Openings** (variants 0, 1 and 3 only; kind 3 only; vertical-step flag clear): **four draws**, for the sides in the order y−1, y+1, x−1, x+1 (`0x1049B5`, `0x104B6C`, `0x104D10`, `0x104EBC`). Each draw is made first; when it is odd, the neighbour cell on that side has kind 0, and the probe block at height D + 10z + 2 is not solid (`0x0061F0` returns 0; probes at (X0 + 10x + 5, Y0 + 10y − 3), (X0 + 10x + 5, Y0 + 10(y + 1) + 3), (X0 + 10x − 3, Y0 + 10y + 5) and (X0 + 10(x + 1) + 3, Y0 + 10y + 5) for the four sides), a 2 × 2 × 4 box is cut through the wall (no draw) and a **type-4** zone object is appended: size (2, 0.2, 4), rotation 0, 2, 3 or 1 for the four sides respectively, position (X0 + 10x + 5, Y0 + 10y − 1), (X0 + 10x + 5, Y0 + 10(y + 1) + 1), (X0 + 10x − 1, Y0 + 10y + 5) or (X0 + 10(x + 1) + 1, Y0 + 10y + 5), all at height D + 10z + 2. These four draws happen whatever the outcome.
    4. **Stair fills** (all variants; vertical-step flag set): up to four runs, each of eleven fills of sizes (11, 10, 1), (10, 10, 1), …, (1, 10, 1) (x-direction runs) or (10, 11, 1) … (10, 1, 1) (y-direction runs), each run gated by the kinds of the two side neighbours and of the neighbour above (`0x1051AD`, `0x1052DD`, `0x10540D`, `0x10553D`). Cells with this flag never reach the trap gate.
    5. **Floor** (skipped together with steps 6 and 7 when the cell below, (x, y, z − 1), has kind 3; also skipped when the variant is 4 and the kind is 4): the **floor fill** (10, 10, 1) at (X0 + 10x, Y0 + 10y, D + 10z) with the floor colour (`0x105641`): **3 + 100 = 103 draws**. Then, for variants 0 and 3 when the cell's carpet flag is set, a carpet is laid on the floor layer with the carpet colours, inset by two blocks on each side whose neighbour lacks the carpet flag, plus border strips (`0x0FF340`, no draws).
    6. **Trap gate** (`0x1058CA`–`0x1058F9`), reached directly after step 5. In order:
       1. If the cell kind is 4, the rest of this step and steps 7 and 8 are skipped (control goes to step 9).
       2. If the vertical-step flag is set, the rest of this step and step 7 are skipped (control goes to step 8).
       3. If G (frame `+0x1C`) is nonzero, **no draw** is made here and control goes to step 7.
       4. Otherwise **one draw** (`0x1058ED`). If the draw mod 40 is nonzero, control goes to step 7.
       5. Otherwise the **trap** is produced (`0x105905`–`0x1059F1`): a zone-object record is initialised (`0x0C84B0`, as in the held contract's table); its position is set to the blocks (X0 + 10x + 5, Y0 + 10y + 5, D + 10z + 1) converted to raw (converted in the order z, y, x; no draw); its size to (10, 10, 10); then **one draw** mod 4 is stored as the rotation `+0x20` (`0x1059B9`); the type `+0x00` becomes 7; then **one draw** mod 4000 is stored as `+0x34` (`0x1059D7`); the record is appended to zone `+0x0C` (`0x1059F1`). Control then goes to step 8 (`0x105A05` jumps to `0x1073FB`), so step 7 is not run for this cell.
    7. **Furnishing** (from `0x105A0A`; when the gate did not produce a trap, including every eligible cell when G is nonzero). Its draws, in order:
       - the current zone-object count is noted;
       - **ceiling**: only when the cell above, (x, y, z + 1), is closed (kind 0, 1 or 2 per `0x122820`): **one draw** mod 3 (`0x105A54`); when 0 and the variant is 4 or 2, a hanging plant of type 55 is appended to zone `+0x04` with **four draws**: rotation (draw / 32767 × 360), scale (draw × 0.04 / 32767 + 0.08), y offset (draw mod 5), x offset (draw mod 5). Then, variants 2 and 1 only, for the sides x−1, x+1, y+1, y−1 in that order: **one draw** mod 3; when 0 and that neighbour is closed, a wall plant (rotation 0, 180, 270 or 90) is appended to zone `+0x04` with **three draws**: scale (draw × 0.05 / 32767 + 0.08), lateral offset (draw mod 5), type (draw mod 2, plus 57). (Constants 360, 0.04, 0.05 and 0.08 read from the server image at `0x158780`, `0x1586B4`, `0x1586B8`, `0x173970`; expression order from decompiler output.)
       - **walls**, for the sides x−1, x+1, y−1, y+1 in that order: **one draw** mod 3; when 0 and that neighbour is closed: for the first three sides, **one draw** (a torch when odd: **one draw** mod 4 for its lateral offset, a solidity probe, and when solid the torch builder `0x12C370` with its own draws, appended to zone `+0x04`), then **one draw** mod 8: when 0 a creature record (allocated 0x10F0 bytes, built by `0x0E0F40`; fields `+0x28` = 6, `+0x2C` = 143, `+0x34` = L, `+0xF58` = 10.0) with **one draw** mod 4 for its lateral offset and the inventory filler `0x0FDD80` (own draws), appended to zone `+0x18`; otherwise **one draw** mod 4 for the lateral offset and the wall-object builder `0x12A830` (own draws; side code 1, 3 or 2; style zone `+0x79`), appended to zone `+0x0C`. For the fourth side the torch draw comes first and, only when it is even, the mod-8 draw and its branch follow (side code 0);
       - **stocking** of every zone object appended during this step: type 10: **one draw** mod 4 plus 1 items, each from the item generator `0x12B470` (level L, rarity Q + 1), the object's activity byte set to 2; types 13, 12, 35, 36 and 37: for each integer position inside the object's x and y size, **one draw** mod 10, and when 0 an item with further draws (kind choice mod 6, then either mod 4 with an optional mod-3 draw, or mod 6 with zero to two further draws or a generator call) and **one draw** for its rotation (draw × 360 / 32767), appended to zone `+0x30`.
    8. **Arches and chandelier** (`0x107401`–`0x1078AD`; cells of kind 3 only; also reached directly from step 5 when the cell below is kind 3, and after a trap): when the cell above is not kind 3: if the variant's arch sprite exists and the block at (X0 + 10x, Y0 + 10y, D + 10(z + 1)) is solid, for each side x−1, x+1, y−1, y+1 whose neighbour is not kind 3, the sprite-placement routine `0x124540` (own draws; rotation codes 0, 2, 3, 1); then, variant 3 only, **one draw** mod 10 (`0x107760`) and when 0 a chandelier plant of type 56 appended to zone `+0x04`.
    9. **Guard** (`0x1078B3`, any processed cell whose guard flag is set): a creature record is built (fields `+0x28` = 1, bit `0x1000` of `+0x7A`, `+0x34` = L but at least 1, byte `+0x58` = Q) at block height D + 10z + 1 and horizontal position offset by 4.5 blocks from the cell's low corner (constant `0x173AC8`; the y term is (Y0 + 10y) + 4.5 in decompiler output, the x term was not fully legible there); **one draw** modulo the size of the routine's race list chooses its race (`0x1079C0`); the rarity routine `0x12BF40` and the item generator `0x12B470` make their own draws; a kind-6 generation record is appended to zone `+0x48`; **one draw** mod 4 gives a potion count (`0x107AA5`); the creature is appended to zone `+0x18`.

**Summary for one cell that reaches the trap gate.** The cell is kind 3, its vertical-step flag is clear and the cell below is not kind 3. Before the gate, within the same iteration, the draws are: four opening draws (variants 0, 1 and 3 only) and the 103 floor-fill draws. Then comes the gate draw (when G is 0). On a trap, the rotation draw and the phase draw follow immediately, and the next draws are those of step 8 and step 9 of the same cell, then the next cell. With no trap, the furnishing draws of step 7 follow.

### 5.8 After the cell loop

16. Variant 4 (after the loop exit at `0x107B92`; fill calls `0x107C84`, `0x107DEB`, `0x1080A6`): the tower top over the centre column: fill (14, 14, 16), two cut boxes (10 × 14 × 10 and 14 × 10 × 10), a shaft cut, a pillar fill (2, 2, h), then for each level of the spiral one fill chosen by the level mod 8 (sizes (5, 5, 1), (1, 5, 1) or (5, 1, 1) at positions that depend on the residue). Variant 5 (fill calls `0x1081B3`, `0x108326`, `0x1085E6`): the analogous top with fill (14, 14, 14), two long cuts (6 × 74 × 6 and 74 × 6 × 6), a shaft cut, a pillar fill and the same spiral fills. Variants 1 and 2: a weathering pass over the 220 × 220 block area driven by noise (no draws). Draws only through fills.
17. **Room population** (draw sites `0x108AC1`–`0x109C56`): rooms in index order; for each used room without the no-population flag, draws for a leader and patrol (style choice, corner, race or hostility choice depending on G, path shape mod 3, potion count) and followers, and groups of stationary creatures (group count, corner, member counts, races or hostility depending on G, potion counts). Not audited in detail; all these draws come after every trap draw of the zone.
18. The routine ends (`0x109D35`).

## 6. Objects and records written per cell (for placing the trap draws)

| Step | Written | Draws |
| --- | --- | --- |
| 15.1 | kind-5 generation record (kind-4 cells) | none |
| 15.2 | cut 10 × 10 × 10 | none |
| 15.3 | up to four type-4 objects, size (2, 0.2, 4) | four, before any of them |
| 15.4 | stair fills | 3 + volume each |
| 15.5 | floor fill (10, 10, 1); carpet | 103; none |
| 15.6 | type-7 object, size (10, 10, 10), active 1, rotation draw mod 4, phase draw mod 4000 | gate 1, then 2 on a trap |
| 15.7 | plants, torches, wall objects, containers, items | as listed |
| 15.8 | arch sprites; type-56 plant (variant 3) | sprite routine; one |
| 15.9 | guard creature and kind-6 record | as listed |

## 7. Agreement between client and server

The complete dungeon routine was compared instruction by instruction: 9 454 listed instructions in each image, every instruction at the same offset from the routine start, and identical text once addresses are paired. The only differing operands are absolute addresses: the exception-handler record, the seven jump-table bases (each table read from the image bytes; all 42 entries give the same routine-relative targets), the global block constant pushed at `0x1087FD` (server `0x1842C0` / client `0x36DDF8`) and a constructor helper pointer. Every external callee and constant pairs one to one (section 9). The fill-box routine, cell accessor, coordinate transform, dungeon initialiser, room carve, corridor link and corridor line routines were compared the same way and are instruction-identical apart from paired addresses.

The zone-generation entry routines (server `0x118630`, client `0x1E4850`) are instruction-identical, with every instruction at the same offset, from the start through server `0x11DC37` / client `0x1E9E57`. This covers the seed, every pass before the dungeon pass, both call sites with their conditions and argument pushes, and the first passes after it. At that point the client listing omits a 33-byte run that the server listing shows; the remainder was not compared. The server generation worker and the client worker were not compared.

Therefore the dungeon routine, its trap draws and its position in zone generation agree in the two images.

## 8. Unresolved

- Game meaning of every kind, flag, type, variant and race value. The labels used here (dungeon, room, entrance, stair, carpet, guard, trap, torch, container) are descriptive only.
- The internal draw sequences of the drawing callees listed in section 1 (room carve and corridor link are summarised in 5.2 but were read in decompiler output only; the others are not audited). The link-list contents of each layout. The full draw sequence of the room-population pass (5.8 step 17) and of the stocking sub-step (5.7 step 7) are outlined, not audited.
- The draws of the passes before the dungeon pass inside callees (terrain, feature builders), the meaning of world `+0x800188` (seed base), who writes the zone-cell records, and the zone-cell variant byte's possible values (behaviour for a variant outside 0–5 was not followed; the steps whose conditions list variants would not select it).
- The C runtime generator's internal state handling (the MSVCR110 module was not examined); this contract assumes that only the generation thread draws between the seed and the end of the zone's generation, as the synchronous call chain suggests.
- The writes made through the shared out-of-range cell record (4) by the guard-flag write or corridor routines when they address cells outside the grid.
- The client generation worker `0x06A8A0` and the remainder of the client entry routine after client `0x1E9E57`.
- Steps 13, 14, 16 and 17 and the furnishing branches were read in decompiler output of the original server image and checked against the ordered call-site list; only the trap window, the openings, the floor fill, the gate and the call sites in the entry routine were also read instruction by instruction.
- Floating-point control state.

## 9. Paired addresses (client / server RVA)

| Item | Client / server |
| --- | --- |
| dungeon routine | `0x1CC510` / `0x100300` |
| zone-generation entry | `0x1E4850` / `0x118630` |
| dungeon call sites (G = 0, G = 1) | `0x1E84C3`, `0x1E84F7` / `0x11C2A3`, `0x11C2D7` |
| kind tests before the calls | `0x1E84A2`, `0x1E84D4` / `0x11C282`, `0x11C2B4` |
| seed call | `0x1E49B5` / `0x118795` |
| zone-cell record copy | `0x1E6B6C` / `0x11A94C` |
| random import slot | `0x2FC468` / `0x1582F4` |
| seed import slot | `0x2FC4B4` / `0x15832C` |
| trap window | `0x1D1ADA`–`0x1D1C15` / `0x1058CA`–`0x105A05` |
| trap gate draw, rotation draw, phase draw | `0x1D1AFD`, `0x1D1BC9`, `0x1D1BE7` / `0x1058ED`, `0x1059B9`, `0x1059D7` |
| trap append | `0x1D1C01` / `0x1059F1` |
| floor fill call | `0x1D1851` / `0x105641` |
| opening draws | `0x1D0BC5`, `0x1D0D7C`, `0x1D0F20`, `0x1D10CC` / `0x1049B5`, `0x104B6C`, `0x104D10`, `0x104EBC` |
| furnishing start, ceiling draw | `0x1D1C1A`, `0x1D1C64` / `0x105A0A`, `0x105A54` |
| fill-box | `0x1DF600` / `0x113400` |
| cell accessor, transform | `0x1C46F0`, `0x1FA3C0` / `0x0F84A0`, `0x12DDE0` |
| dungeon initialiser | `0x1C38C0` / `0x0F7370` |
| room carve, corridor link, corridor line | `0x1EF8F0`, `0x1C52A0`, `0x1EF6D0` / `0x1236D0`, `0x0F9010`, `0x1234B0` |
| cut box, carpet fill | `0x19ABA0`, `0x1CB550` / `0x0D2500`, `0x0FF340` |
| object initialiser, object append | `0x18EAA0`, `0x1A0E00` / `0x0C84B0`, `0x0D6670` |
| torch, wall object, inventory, item, sprite placement, rarity | `0x1F8960`, `0x1F6E20`, `0x1C9F90`, `0x1F7A60`, `0x1F0CE0`, `0x1F8530` / `0x12C370`, `0x12A830`, `0x0FDD80`, `0x12B470`, `0x124540`, `0x12BF40` |
| set block, column lookup, block lookup | `0x04E7A0`, `0x0347A0`, `0x02F7E0` / `0x01FF00`, `0x006100`, `0x005FD0` |
| unit-draw divisor 32767 | `0x2FD338` / `0x158834` |
