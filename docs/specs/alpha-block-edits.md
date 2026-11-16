Status: FINAL / STABLE / HELD

# Alpha block edits: how the world's blocks change at run time

Behavioural specification of every way Cube World **Alpha** changes the blocks of an
already generated world, and of how such a change is recorded, stored, expired and
applied: the 20-byte block-edit record, the zone edit list and its save, the "place a voxel
object" request (packet 6 kind 7) with its blueprint table and the client's hidden build
mode, the bomb explosion, the craters that big creatures dig with their attacks, how a
client applies an edit to its terrain and flags meshes for rebuilding, and what the server
does with all of it.

Evidence: the two original images in `binary/` (`Cube.exe`, SHA-256 84a7a132…30bf;
`Server.exe`, SHA-256 1ffbcbce…4fb1), both with image base 0x400000, disassembled afresh
for this contract; the Alpha decompile was used only to locate routines. Nothing here is
copied code. **All addresses are RVAs** (virtual address minus 0x400000) and name their
image: "Server RVA 0x…" or "Cube RVA 0x…". Inside the world simulation step the client
routine lies at the server RVA + 0xDA240 (spot-checked for the placement handler, the bomb
explosion and the crater record write); where only a server RVA is given, the client twin is
at that offset. Offsets into in-memory objects are given for cross-checking. Readings not
confirmed by a second path are marked *believed*; facts taken from companion specs without
re-reading them here are marked *(companion)*.

Related specs (pointers only): `alpha-net-packets.md` (world-update section numbering and
wire encoding, packet 6, packet 11), `alpha-server-simulation.md` (step order, bomb item,
zone loading and unloading), `alpha-world-items.md` (item part of the zone save),
`alpha-saves.md` (zone record), `alpha-worldgen-terrain.md` (block format, implicit blocks),
`alpha-worldgen-structures.md` (generation-time prefab placer), `alpha-rendering.md`
(meshing and light), `alpha-creatures-behaviour.md` (action timing, hit resolution, sound
ids), `alpha-creatures-models.md` (box sizes), `alpha-ui-screens.md` (key and mouse names).

---

## 0. Conventions

* **Block**: 4 bytes R, G, B, type byte. Type = low 5 bits; flag **0x20** "natural"; flag
  **0x40** "protected"; flag **0x80** "keep". "Air" below means (255, 255, 255, type byte 0);
  "sea water" (255, 255, 255, 0x82). Both programs hold these two blocks as initialised
  globals; Server.exe's implicit-read routine (Server RVA 0x005FD0) also returns the
  implicit solid (200, 200, 200, type byte 0x01).
* Creature positions are signed 64-bit fixed point in 1/65,536 block. Block coordinates are
  signed 32-bit. Fixed-to-block conversions in this contract use one of two rules, named
  where they matter: **trunc** (signed 64-bit division by 65,536, toward zero) or **floor**
  (for a negative value the routines subtract one after, or 65,536 before, the division, so
  they equal floor except at exact negative multiples; all playable coordinates are positive,
  where both rules coincide). Zone = block ÷ 256 (toward zero).
* "*r*" = one C `rand()` result; pitch formulas below are evaluated in single precision as
  (rand × 0.4) ÷ 32,767 + 0.5.
* Day = the world clock's day counter at world +0x800160.
* "The step" = the world simulation step (Server RVA 0x1322D0, Cube RVA 0x20C510).
  World byte +0xB4 = online flag; world +0xB8 = local player (none on a server).
  "Offline" = a client world with the online flag clear; "server" = online flag clear and no
  local player; "online client" = online flag set.
* **Event set**: the step's output object. Its lists used here: +0x08 sounds, +0x10
  particle bursts, **+0x18 block-edit records**, +0x38 per-zone placed-object lists. The
  world-update section numbers used by the companion network spec (block edits = section 1,
  particles = section 3, sounds = section 4, placed objects = section 8) are *(companion)*.
  On the client the frame's received lists sit in an event set embedded at game object
  +0x800630, so the block-edit list is +0x800648 and the placed-object list +0x800668.

---

## 1. Overview: what changes blocks in Alpha

Generation writes blocks but makes no edit records. At run time exactly **three** places in
the step append block-edit records:

| Producer | Who runs it | What it does | Record type byte |
|---|---|---|---|
| **Packet 6 kind 7**, place a voxel object (§6) | server and offline clients; skipped when the online flag is set | stamps one of 12 blueprint models into the world | 0x46 |
| **Bomb explosion** (§8) | the step's dead-creature pass; no online check on that path | removes natural blocks with d² < 64 | 0 (remove) |
| **Big-creature impact crater** (§9) | only when the online flag is clear | removes natural blocks around the impact point of an attack by a creature wider than 4 blocks | 0 (remove) |

Each producer writes the block into its own world at once **and** appends a 20-byte record
(§2) to event set +0x18. The append routine (Server RVA 0x128400) is called on event set
+0x18 from exactly these three sites in the step: Server RVA 0x137566 (placement),
0x134CEB (bomb), 0x139BF0 (crater). Its other callers are the generation-time prefab
placer (Server RVA 0x124540), which appends to a list passed as its ninth stack argument
only when that argument is non-null; all 37 calls of the placer in Server.exe pass null.

The client contains a complete "build mode" for kind 7 (§7) that no code path found
switches on.

---

## 2. The block-edit record (20 bytes)

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 12 | 3 × i32 | block x, y, z |
| 0x0c | 3 | 3 × u8 | colour R, G, B |
| 0x0f | 1 | u8 | the full **type byte** (type and flags). **0 = remove**; anything else = write this block |
| 0x10 | 4 | i32 | **day** on which the edit was made (world +0x800160). Negative = never expires on reload (no Alpha producer writes a negative day) |

Records written by Alpha:

| Producer | Colour | Type byte | Day |
|---|---|---|---|
| placement (§6) | the voxel's colour | **0x46** (type 6, protected) | today |
| bomb, crater (§8, §9) | (255, 255, 255) | **0** | today, also for z ≤ 0 (the appliers turn it into sea water, §4) |

The zone edit list holds the records with a 20-byte stride and the zone save stores them as
consecutive 20-byte records in list order (§4.2, §12). The wire encoding of the world-update
block-edit section is *(companion)*.

---

## 3. Life cycle of an edit

1. A producer (§6, §8, §9) changes the block in its own world and appends the record to
   event set +0x18.
2. **Server:** the world update carrying that list goes to the connected clients
   *(companion: to every connection, without a distance filter)*. **Offline client:** the
   step's own output is applied like received records (*believed*).
3. At the end of the same step (wrap-up, Server RVA 0x147111..0x1471DC) every record of
   event set +0x18 is appended, in order, to the edit list (zone +0x68, a vector of 20-byte
   records) of **the zone that contains the record's own (x, y)** (x ÷ 256, y ÷ 256 toward
   zero), if that zone is loaded. A record whose zone is not loaded is not stored.
4. The zone save writer (Server RVA 0x01FAA0) stores the whole edit list as a u32 count
   followed by the 20-byte records in list order. Its only caller (Server RVA 0x0D81E9..)
   saves a zone only when the world's save store is open (world +0xA4 non-zero) and either
   the zone byte +0x75 is set or the zone's edit list is non-empty. When that caller runs is
   *(companion: on unload)*.
5. When the zone is generated again, the saved list is filtered by day and re-applied
   (§4.2, §5); the kept records become the zone's new list.
6. **Server:** the packet-11 handling routine (Server RVA 0x0272D0) answers for a zone that
   is loaded by copying the zone's **whole** edit list (zone +0x68..+0x6C, in list order,
   at Server RVA 0x027A19) into the reply's block-edit list, together with the zone's item
   list (zone +0x30) and a further per-zone list of 0x188-byte entries (zone +0x0C). The
   section numbers of these parts are *(companion)*.
7. **Online client:** records received in the block-edit list are applied (§4.1); they are
   not added to the client's zone lists (*believed*).

---

## 4. Applying records

### 4.1 Live application on a client (frame update, Cube RVA 0x08F242..0x08F3D5)

Once per frame, if the frame's received block-edit list (game object +0x800648) is not
empty, under a lock (Cube RVA 0x19C480 taken, 0x201EA0 released), for each record in order:

* **Type byte ≠ 0: write** with the general block writer (Cube RVA 0x04E7A0; server twin
  Server RVA 0x01FF00):
  * nothing if the column (x, y) is not loaded;
  * nothing if the new type byte is 0 and z ≥ bottom + count (at or above the stored top);
  * at z ≤ 0 a block whose (type byte AND 0x5F) = 0 is replaced by sea water;
  * the block is stored at index z − bottom through the cell store (below), which grows the
    column upward or downward as needed;
  * if the write extended the column downward (z + 1 < old bottom), every block from z + 1
    to old bottom − 1 is filled with a type-6 block (type byte 0x06) whose colour comes from
    a position-dependent colour routine (Cube RVA 0x1F9620, Server RVA 0x12D030), the
    colour components truncated from float to byte.
* **Type byte = 0: remove** (Cube RVA 0x071920; server twin Server RVA 0x130470): only if
  0 ≤ x, y < 2²⁴, the zone of (x, y) is loaded, the column exists **and**
  bottom ≤ z < bottom + count (a stored block). The stored block becomes **sea water** if
  z ≤ 0, else **air**. Implicit blocks below the stored range cannot be removed this way.
* Both go through the cell store (Cube RVA 0x04E700, Server RVA 0x01FE60), which applies
  the **keep rule** to an index inside or above the stored range: if the stored block has
  flag 0x80, a new block whose type (low 5 bits) is 0 is ignored, and any other new block is
  written with 0x80 added. A write below the stored range extends the column and stores the
  block without this check.

While the records are applied, the minimum and maximum x and y over all records of the
list are tracked (starting from the first record). Then:

1. **Floating sprites** (Cube RVA 0x1A5CB0; server twin Server RVA 0x0D9160). The
   rectangle (min x, min y)..(max x, max y) is clamped at 0 and converted to zones. For
   every loaded zone in it, every entry of the zone's sprite list (zone +0x04) whose byte
   +0x38 has bit 1 (value 2) set and for which the block one block below the entry's
   position (position z minus 65,536 in fixed point, floor conversion, implicit read rules)
   has type 0 is **deleted**.
2. **Mesh flags.** The records are moved to a frame-local list; in the render part of the
   frame (Cube RVA 0x0956E0..0x095808), for every record the **3 × 3 chunks** (a chunk is
   32 × 32 columns, chunk index = column ÷ 32 toward zero) around the chunk holding (x, y)
   are flagged, if that chunk is present in the chunk cache: under the chunk's critical
   section, chunk byte +0x74 := 1 and chunk word +0x70 := a frame value. The flag is
   cleared when the chunk is rebuilt (Cube RVA 0x0A1394). How the rebuild re-meshes and
   re-lights the chunk is in `alpha-rendering.md` *(companion)*.

Records for columns that are not loaded change nothing.

### 4.2 When a zone is generated (Cube RVA 0x04CDB0, Server RVA 0x01EE20)

At the end of zone generation (call at Cube RVA 0x1EDE46, Server RVA 0x121C26), when the
zone's save was read successfully, the save is applied: first the item part
(`alpha-world-items.md`), then the edits, then the spawn-object words (§12). For the edits:
a u32 count, then the 20-byte records. Each saved record is **kept** if its day ≥ today − 3
or its day < 0. A kept record is written with the general block writer of §4.1 into this
zone (also for type byte 0, which then writes air inside or below the stored range — below
it, with the type-6 fill of §4.1 — sea water at z ≤ 0, and nothing at or above the stored
top) and appended to the zone's edit list. If the saved count was at least one, the
floating-sprite rule of §4.1 step 1 runs over this zone's sprite list. All of this happens
**before** the zone's light pass (Cube RVA 0x1EDE84 calling 0x19A0E0; Server RVA 0x121C64
calling 0x0D1A70), so a freshly generated zone is lit with its edits in place. Dropped
records are gone: the next save no longer contains them.

### 4.3 Inside the step

The producers write their own world directly with the server-side writers (Server RVA
0x01FF00 write, 0x130470 remove; the same rules) and the server additionally re-lights the
area (§6.5, §8, §9).

---

## 5. The day field and the 3-day expiry

* A record made on day *d* survives every reload of its zone while the reloading day is
  ≤ *d* + 3; on the first reload on day *d* + 4 or later it is dropped, and the generated
  terrain comes back.
* While the zone stays loaded nothing expires: the filter exists only in the zone-load path.
* Kept records keep their original day: re-saving does not renew them.
* How fast the day counter advances is *(companion: `alpha-server-simulation.md`)*.

**Timeline.** A bomb crater made on day 5. The zone is saved and unloaded. Reloaded on
day 8: 5 ≥ 8 − 3, the crater is re-applied. Saved and reloaded again on day 9: 5 < 6, the
crater's records are dropped, the generated ground is back and the zone's next save has no
edits from that crater.

---

## 6. Placing a voxel object (packet 6 kind 7)

### 6.1 The request

Packet 6 with kind byte (+0x128) **7**. The handler (Server RVA 0x1370B7..0x137787,
Cube RVA 0x2112F7..0x2119C7, in the per-creature part of the step) reads nothing from the
packet body besides the kind; all inputs come from the **sender's creature record** (record
= creature object + 0x10):

| Input | Record offset | Meaning |
|---|---|---|
| object number *n* | 0x17D, signed byte | blueprint number (§6.2) |
| rotation *q* | 0x17C, signed byte | quarter turns: angle = *q* · 90° |
| aim **a** | 0x150, 3 × f32, blocks | offset from the creature to the placement point |
| position **P** | 0x00, 3 × i64 fixed | the creature's position |

How the client gets these fields to the server (packet 0 before packet 6) is
*(companion)*. The handler does nothing when the online flag is set, and nothing if
blueprint *n* does not exist. It performs no checks of distance, rate, permission or
ownership. From before the voxel loop until after the object-list update (Server RVA
0x137211..0x137782) it holds the world lock (Server RVA 0x0D3E00 / 0x0D5C70).

### 6.2 Blueprints (world +0x800154)

Both programs fill, while creating the world object (Server RVA 0x0C8570, keys pushed at
Server RVA 0x0CCE79..0x0CD43B; Cube RVA 0x18EB00, keys at Cube RVA 0x193409..0x1939CB), a
map from object number to a voxel model read from `data1.db`:

| *n* | Model key |
|---|---|
| 0 | `framework-floor-wood.cub` |
| 1 | `framework-wall-window.cub` |
| 2 | `framework-wall-door.cub` |
| 3 | `framework-roof1.cub` |
| 4 | `framework-entrance.cub` |
| 5 | `framework-wall.cub` |
| 6 | `framework-roof2.cub` |
| 7 | `framework-roof3.cub` |
| 8 | `framework-wall-balcony.cub` |
| 9 | `framework-roof-wall2.cub` |
| 10 | `building-stone-well.cub` |
| 11 | `building-stone-stairs2.cub` |

A model has sizes sx, sy, sz (model +0x44, +0x48, +0x4C) and one RGB colour per voxel;
colour (0, 0, 0) = empty. The only readers of the map are the placement handler (lookup
Server RVA 0x0D1860, Cube RVA 0x198840 called from Cube RVA 0x2112F7..) and the client's
build-mode preview (Cube RVA 0x0B3359). Other `framework-*.cub` keys exist in the images
(`-arc`, `-base`, `-floor`, `-floor-stairs`, `-wall-indoor`, `-wall-lamp`, and in
`data1.db` `-roof-wall1`) but are not in the map. The sizes and voxel contents of the
twelve models were not re-measured in this pass (§15).

### 6.3 Where the voxels go

1. **Base block B**: each aim component is multiplied by 65,536 in single precision and
   truncated to a 64-bit integer; B = trunc((**P** + that + 32,768) ÷ 65,536) per axis: the
   block nearest the point **P** + **a**. B is used as is (no snapping to the ground).
2. **Transform.** A single-precision 4 × 4 matrix is built from the identity: first a
   rotation about the vertical axis by *q* · 90° (the angle is *q* · 90 times π/180 in single
   precision; its cosine *c* and sine *s* are computed in double precision and rounded to
   single), then a translation by (−sx/2, −sy/2, 0) applied in the rotated frame. For voxel
   (i, j, k) the point (i + 0.5, j + 0.5, k + 0.5) is transformed, so with
   *u* = i + 0.5 − sx/2 and *v* = j + 0.5 − sy/2 the result is x′ = *c*·*u* − *s*·*v*,
   y′ = *s*·*u* + *c*·*v*, z′ = k + 0.5 (counter-clockwise; the matrix evaluates
   c·(i+0.5) − s·(j+0.5) + (c·(−sx/2) − s·(−sy/2)) term by term). Each coordinate is
   multiplied by 65,536 in single precision, truncated, added to B · 65,536 and converted
   with trunc. For B ≥ 0 the result is:

   | *q* mod 4 | X | Y | Z |
   |---|---|---|---|
   | 0 | Bx + ⌊*u*⌋ | By + ⌊*v*⌋ | Bz + k |
   | 1 | Bx + ⌊−*v*⌋ | By + ⌊*u*⌋ | Bz + k |
   | 2 | Bx + ⌊−*u*⌋ | By + ⌊−*v*⌋ | Bz + k |
   | 3 | Bx + ⌊*v*⌋ | By + ⌊−*u*⌋ | Bz + k |

   When sx and sy are even, *u* and *v* are half-integers, the single-precision rounding of
   *c* and *s* moves x′ and y′ far less than half a block, and the table is exact for every
   *q* (−128..127). With an odd size a coordinate is an integer and the rounding can push
   it across the truncation (off by one, *as read*). The model is centred on B in x and y and
   stands on B in z (k = 0 at Bz).
3. **Order**: i ascending (outer, 0..sx−1), j ascending, k **ascending** (inner).
4. **Per column (i, j)**: zlo / zhi = lowest / highest k with a non-empty voxel (zlo = sz,
   zhi = 0 when the column is empty). For each k, with C = the voxel colour and E = the
   world block now at (X, Y, Z) read with the implicit rules (an unloaded column or a cell
   below the stored range reads as the implicit solid (200, 200, 200, 0x01); above the
   stored top, air for z > 0 and sea water for z ≤ 0; a stored block of type 0 without flag
   0x40 at z ≤ 0 reads as sea water):
   * C empty and zlo < k < zhi: write (0, 0, 0) with type byte **0x40** (protected air)
     through the general writer. **No record** is made.
   * C empty otherwise: nothing.
   * C non-empty and E has flag **0x40**: nothing, no record.
   * C non-empty otherwise: write (C, type byte **0x46**) with the general writer (§4.1,
     including its downward fill) and append the record (X, Y, Z, C, 0x46, today) to
     event set +0x18. The record is appended even when the writer did nothing because the
     column is not loaded.

   Every non-empty colour is treated alike: the handler compares voxels only with the
   empty colour.

### 6.4 The zone's placed-object list

After the voxels, if the zone containing (Bx, By) is loaded, a 16-byte entry
**(i32 *n*, i32 Bx, i32 By, i32 Bz)** is appended to that zone's object list (zone +0x3C)
and the zone is inserted into the step's "object list changed" set. In the wrap-up
(Server RVA 0x1470A0..0x1470E4) each such zone contributes one element to event set +0x38,
built by Server RVA 0x12E950: the zone key (zone +0x60, +0x64) and a copy of the zone's
**whole** object list. The wire encoding of that section is *(companion)*.

A client receiving such elements (Cube RVA 0x08F58D..0x08F6B9) looks up each zone; for a
loaded zone it empties the zone's object list (zone +0x3C), inserts the received entries and
clears the zone byte +0x74, which the item handler (Cube RVA 0x08F435..) also clears. The
zone loader reads no such list from the save and the save writer writes none, and the
rotation is not in the entry. No other reader of the list was identified (*believed*).

### 6.5 After placing

* **Server only** (world +0xB8 = none): the light pass (Server RVA 0x0D1A70, called at
  Server RVA 0x13769C) with a margin of 8 columns over x from trunc(Bx − sx/2) and
  y from trunc(By − sy/2) to the upper bounds trunc(Bx + **sy**/2) and trunc(By + **sz**/2)
  (the sizes are mixed up *as read*), all computed in single precision. The pass visits
  columns lo − 8 ≤ x < hi + 8 and likewise in y. An offline client skips this.
* Server and offline: the floating-sprite rule (§4.1 step 1) over x ∈ [Bx, Bx + sx],
  y ∈ [By, By + sy] (not centred, *as read*).
* No sound, particle or creature effects.

---

## 7. Build mode in the client (present but not switched on)

The client contains the user side of kind 7, gated by the game-object byte
**G+0x800704** ("build mode"). Key and button names below follow the key-state bytes as
named in `alpha-ui-screens.md` *(companion)*.

| Input | Effect in build mode | Where |
|---|---|---|
| key-state byte G+0x0F (R) held now and not held in the previous frame (previous state kept in the Cube .data byte at RVA 0x36B138) | queue a packet-6 request on the local player: a default-initialised request body (Cube RVA 0x059530) with kind 7 | Cube RVA 0x09797D..0x0979C7 |
| right mouse button event | local player record byte 0x17C (rotation) += 1 (signed byte, wraps) | Cube RVA 0x07CCFF..0x07CD0E |
| key-state bytes G+0x0B / +0x0C / +0x0D / +0x0E / +0x12 / +0x13 held | move the aim (record 0x150) by 0.005 blocks per ms of frame time along −y / +y / +x / −x / +z / −z, each direction turned by the camera frame before it is added | Cube RVA 0x09B18D..0x09B4C1 |
| (frame) | draw a preview of blueprint record 0x17D at the aim point with rotation record 0x17C | Cube RVA 0x0B3338.. |
| (frame) | the attack/ability input branch and the following slot input are skipped while build mode is on | Cube RVA 0x09797D, 0x09B5B1 |

The default-initialised request is not all zero: its constructor sets, among others, the
word +0x10 and the word +0x12A to 1 and the three dwords +0x118..+0x120 to −1. None of
these is read by the kind-7 handler.

G+0x800704 is written only at game-object creation, where it is cleared (Cube RVA
0x05A09B). The only client stores found at record offset 0x17D are member-wise record
copies (Cube RVA 0x04B23F, 0x06624E); a static search cannot exclude dword stores at the
same offset through unrelated pointers. *Believed* therefore: in vanilla Alpha no item, key
or menu enters build mode or sets the object number, so a server only receives kind 7 from
modified clients, and a client that only turned build mode on would place blueprint 0.

---

## 8. The bomb explosion

The explosion (Server RVA 0x134998..0x134ED0; Cube RVA 0x20EBD8..) runs in the step's
dead-creature pass for a creature object with: health (object +0x16C, record 0x15C) ≤ 0
(a NaN health does not qualify), object byte +0x60 (record 0x50) non-zero, object dword
+0x6C (record 0x5C) **equal to 0**, and type (object +0x64, record 0x54) **144**. The
meaning of record bytes 0x50 and 0x5C in this pass and the bomb item itself are
*(companion: `alpha-server-simulation.md`)*. With **P** = the bomb's position (fixed):

1. One **particle burst** at **P** appended to event set +0x10: colour components 1–3 =
   (1.0, 0.2, 0.5) in that order, the fourth colour component not written by this code (it
   copies whatever the temporary holds, *as read*), count 20, size 0.25, style 1, velocity
   (0, 0, 20), record field +0x40 left at its default 3.0.
2. For dx = −8 … 8 (outer), dy = −8 … 8, dz = −8 … 8 (inner): Q = **P** + (dx, dy, dz)
   blocks in fixed point; the cell is visited if the single-precision squared length of
   Q − **P** is **< 64**. E = the block at Q (floor conversion, implicit rules of §6.3). If E
   has flag **0x20** (Server RVA 0x1306C0):
   * a small burst at Q to event set +0x10: colour (R/255, G/255, B/255, 1.0) of E, count 3,
     size 0.5, style field 0, field +0x40 at 3.0, velocity (0, 0, 10);
   * the block is removed with the remover of §4.1 at (trunc Qx, trunc Qy, floor Qz) —
     stored range only, sea water at z ≤ 0, else air;
   * the record (floor Q, 255, 255, 255, type byte 0, today) goes to event set +0x18.
   Implicit blocks, air, sea water, placed objects (0x46) and protected air (0x40) lack
   flag 0x20 and are never removed by this rule; whether a given generated block carries
   0x20 is *(companion: `alpha-worldgen-terrain.md`, `alpha-worldgen-structures.md`)*.
3. If at least one natural block was found:
   * a sound at **P** appended to event set +0x08: id **1**, pitch (rand × 0.4) ÷ 32,767
     + 0.5, volume 1.0 (constructor default);
   * server only (world +0xB8 = none): the light pass with margin 8 over the rectangle
     trunc(Px − 8) .. trunc(Px + 8) by trunc(Py − 8) .. trunc(Py + 8) (columns
     lo − 8 ≤ x < hi + 8);
   * under the world lock (Server RVA 0x0D3E00 / 0x0D5C70): the floating-sprite rule over
     the same rectangle.
4. Then the pass continues with the damage to creatures around the bomb *(companion)*.

Up to 2,103 cells lie inside the sphere (§13.1).

There is no online check on this path. *Believed*: an online client whose creature map
contains a dead bomb meeting these conditions would remove the same blocks locally and add
its own records to its local output (§15).

---

## 9. Craters from big creatures' attacks

When a creature's action time (object +0x6C, record 0x5C) satisfies previous bound <
time ≤ current bound (the two bounds held by the step at Server RVA 0x1395B7; their meaning
*(companion: `alpha-creatures-behaviour.md`)*), the code at Server RVA 0x1395A9..0x139E44
computes an **impact point** and an **impact radius**, used for the hit and, for big
creatures, for a crater.

Let *w* = the first component of the creature's box vector (object +0x80, record 0x70),
*h* = its third component, *A* = the action id byte (object +0x68, record 0x58), and yaw =
the third float of the record field at 0x18 (record 0x20). The rotation routine multiplies
the yaw by π/180 (single-precision constant 0.01745329), so the yaw is in **degrees**.

**Radius *ρ* and forward reach *f*** (the action sets are disjoint):

| Action ids | *ρ* | *f* |
|---|---|---|
| 1, 2, 3, 4, 6, 7, 9, 13, 14, 18, 19 | 2*w* | *w* |
| 20, 21 | 3*w* | 1.5*w* |
| 68, 70 | 1.5*w* | 0 |
| 86, 93, 104 | 3*w* | 0 |
| 87, 88 | 8 | 0 |
| 30, 32 | 3 | 0 |
| 31, 33 | 5 | 0 |
| 74, 91 | 20 | 0 |
| 72, 76, 77, 78 | 1.8*w* | 0 |
| any other | 1.5*w* | 1.5*w* |

**Impact point C**: position + the vector (0, *f*, 0) turned by yaw about the vertical
(forward is +y at yaw 0), converted to fixed point by single-precision multiplication by
65,536 and truncation. Then: actions 30–33 use C = position + aim (record 0x150 converted
the same way); actions 69 and 75 raise C.z by (*h* − *w*)/2; actions **11, 54, 61** use
C = position and multiply *ρ* by 3.

**Crater** — only if the online flag is clear, *w* **> 4.0** blocks and *A* ∉ {87, 91, 74}:
*R* = min(trunc(*ρ*), 8). For dx, dy, dz = −*R* … *R* (dx outer, dz inner) with
squared length < *R*² (single precision): the per-block rule of §8 step 2 around C (natural
blocks only; the same small burst; removal at (trunc, trunc, trunc) of C + d; the record at
trunc(C + d) with today's day). If at least one natural block was found: a sound at the
creature's position, id **2**, pitch (rand × 0.4) ÷ 32,767 + 0.5, volume 1.0; server
only, the light pass with margin 8 over trunc(Cx − *ρ*) .. trunc(Cx + *ρ*) by
trunc(Cy − *ρ*) .. trunc(Cy + *ρ*); and the floating-sprite rule over the same rectangle
(no world-lock call around it here, *as read*).

Which creature types have a box width above 4 is in `alpha-creatures-models.md`
*(companion)*. Because of the online-flag check, online clients never dig craters
themselves; they receive the server's records.

**Example** (assumed box *w* = 5.6, *h* = 12.6). The creature's centre is at z = 71.3 on
flat natural ground whose top block is at z = 64; it swings with action 3: *ρ* = 11.2,
*f* = 5.6, so C is 5.6 blocks in front of it at z = 71.3, *R* = 8. Cell
z = trunc(71.3 + dz) reaches 64 only for dz = −7, where dx² + dy² < 15: the top ground
layer is removed in a disc of 45 cells under C. With action 54 (*ρ* = 4.5*w* = 25.2,
C at the centre) *R* is still 8.

---

## 10. The server's role

* **Authority.** All block-edit records of the step are made by the three producers above
  (§1); on a server they reach clients only through the world update. That clients send no
  block-edit records of their own is *(companion)*.
* **Storage.** The wrap-up appends the step's records to their zones' lists (§3 step 3); the
  zone save writer stores them (§3 step 4) and the loader re-applies them with the 3-day
  filter when the server generates the zone again (Server RVA 0x01EE20).
* **Requests.** Packet 11 for a loaded zone → its whole stored edit list in the reply's
  block-edit list, plus its item list and its 0x188-byte-entry list (§3 step 6). A zone the
  server has not loaded is not answered from storage. *Believed*: the client marks a zone as
  requested in zone byte +0x74 (set at Cube RVA 0x07051C) and the item and placed-object
  handlers clear it (§6.4); how often requests are sent is *(companion)*.

---

## 11. Ordering and conflicts

1. **Edits after generation.** Saved edits are applied after the zone generator and before
   the zone's light pass (§4.2); an online client applies the server's records whenever they
   arrive, on top of its own freshly generated zone. Edits win over generated blocks except
   under the keep rule.
2. **List order.** Within a zone, records apply in creation order; a later record for the
   same cell wins. The list is never compacted: placing twice in the same spot or exploding
   a bomb in an old crater appends more records.
3. **Keep flag.** A stored block with flag 0x80 ignores removal (§4.1).
4. **Protection.** Placement never overwrites a block with flag 0x40. Bombs and craters
   only remove blocks with flag 0x20; placed blocks (0x46) and protected air (0x40) lack it.
5. **Zone borders.** Each record is stored in the zone of its own (x, y), so an object or
   crater crossing a border is split between zones. The placed-object entry goes to the
   zone of the base block only.
6. **Unloaded zones.** Records for cells whose column is not loaded change nothing in the
   applier and, on the server, are not stored when their zone is not loaded (§3 step 3).
7. **Interior air of placements** (§6.3) is written only where the step runs (server,
   offline) and has no record, so online clients keep whatever terrain was inside a hollow
   object.
8. **Day expiry is per record**: a crater and a later object placed in it can expire on
   different days (*as read*).

---

## 12. The zone save, revisited

The zone save writer (Server RVA 0x01FAA0) and loader (Server RVA 0x01EE20, Cube RVA
0x04CDB0) agree on this order after the item part:

* **Block edits**: u32 count, then the zone's edit list, 20 bytes each (§2).
* **Spawn-object words**: u32 count, then two 32-bit words per entry of the zone's pointer
  list at zone +0x18, taken from offsets +0x38 and +0x3C of each pointed-to object. On load
  they are copied back only if the saved count equals the number of entries the generator
  just put in that list, in order. Their meaning (respawn countdown and day of death) is
  *(companion: `alpha-creatures-behaviour.md`, `alpha-server-simulation.md`)*.
* The zone's 16-byte placed-object list (§6.4) is **not** saved.
* The item part and the zone byte +0x75 that also forces a save are
  *(companion: `alpha-world-items.md`)*.

---

## 13. Worked examples

### 13.1 A bomb crater

Flat natural ground: every column has stored natural blocks (flag 0x20) up to the top block
at z = 64 and down past z = 56. A bomb sits at
**P** = (8,398,402.5, 8,397,472.5, 65.4) blocks; day 5.

* Cells inside the sphere: 2,103. Their block z = floor(65.4 + dz) = 65 + dz. Those with
  z ≤ 64 (dz ≤ −1) are ground: **955 records**, by layer: z 64: 193, 63: 185, 62: 177,
  61: 145, 60: 121, 59: 89, 58: 45.
* Loop order gives the first record at dx = −7, dy = −3, dz = −2:
  (8,398,395, 8,397,469, 63):

      3b 26 80 00  9d 22 80 00  3f 00 00 00  ff ff ff 00  05 00 00 00

  and the last at dx = 7, dy = 3, dz = −1: (8,398,409, 8,397,475, 64).
* The step's output: 955 records in event set +0x18, 1 + 955 bursts in +0x10, one sound in
  +0x08. A client applying the 955 removals deletes ground-resting sprites above the removed
  top layer in zone (32806, 32802) and flags the 3 × 3 chunks around each edited column
  (chunks x 262,448–262,451 × y 262,419–262,422 here).
* All 955 records are appended to zone (32806, 32802)'s list. Expiry as in §5.

### 13.2 Placing a 10 × 10 × 6 solid blueprint

Assumed model (not re-measured, §15): sx = sy = 10, sz = 6, every voxel non-empty, colour
(165, 165, 165) for k = 0 and (101, 40, 0) for k = 5. The sender's record: position
**P** = (8,398,402.3, 8,397,472.8, 65.35), aim **a** = (3.4, −0.2, −1.2), rotation 1;
open grassland (no 0x40 blocks); day 5.

1. B = trunc(**P** + **a** + 0.5) = (8,398,406, 8,397,473, 64).
2. *q* = 1: *u* = i − 4.5, *v* = j − 4.5, so X = Bx + ⌊4.5 − j⌋ = 8,398,410 − j,
   Y = By + ⌊i − 4.5⌋ = 8,397,468 + i, Z = 64 + k. The slab covers
   x 8,398,401…8,398,410, y 8,397,468…8,397,477, z 64…69.
3. Every column is full (zlo 0, zhi 5): **600 records**, i outer, j, k inner. First, voxel
   (0, 0, 0):

       4a 26 80 00  9c 22 80 00  40 00 00 00  a5 a5 a5 46  05 00 00 00

   Last, voxel (9, 9, 5): (8,398,401, 8,397,477, 69):

       41 26 80 00  a5 22 80 00  45 00 00 00  65 28 00 46  05 00 00 00

4. Zone of B: (32806, 32802). Its object list gains (n, 8,398,406, 8,397,473, 64) and the
   zone's whole list goes to event set +0x38 keyed (32806, 32802).
5. Server: light pass with margin 8 over x from 8,398,401 − 8 to below 8,398,411 + 8 and y
   from 8,397,468 − 8 to below 8,397,476 + 8 (the x end uses sy/2 = 5, the y end sz/2 = 3);
   sprite rule over zone (32806, 32802).
6. The 600 records are stored in that zone's edit list and expire as in §5. A later bomb
   next to the slab removes natural ground around it but not the slab (type byte 0x46 has
   no flag 0x20).

---

## 14. Companion pointers

Facts this contract relies on from other specs without re-reading them: world-update
section numbering and wire encoding, the bomb item and its dead-pass bookkeeping, the
day-counter rate, key/button names, creature box sizes, sound-id names, and the meaning of
the spawn-object words. They are listed in §15 where they matter for behaviour.

---

## 15. Unresolved

* **U-1** Blueprint model contents (sizes, voxel counts, colours, interior gaps) were not
  re-measured from `data1.db` in this pass; §6.2 gives only keys and §13.2 assumes a model.
  Whether every shipped blueprint has even sx and sy (which makes the §6.3 table exact) and
  which blueprints have interior empty voxels between zlo and zhi is therefore open.
* **U-2** Wire encoding and section numbers of the block-edit, particle, sound, item and
  placed-object parts of the world update and of the packet-11 reply, and whether the
  server sends the block-edit section to every connection without a distance filter.
* **U-3** Whether an online client also runs the bomb explosion for a received dead bomb
  (§8; no online check on that path), and what record byte 0x50 means in that pass.
* **U-4** Whether an online client keeps received edits in its zone lists or saves them;
  whether an offline client applies its own step output through the frame lists.
* **U-5** Which sound files ids 1 (bomb) and 2 (crater) select.
* **U-6** The fourth colour component of the bomb's main burst (not written by the
  explosion code).
* **U-7** Whether anything sets build mode (G+0x800704) or record byte 0x17D through a path
  not visible statically.
* **U-8** The meaning of chunk word +0x70 set with the rebuild flag, how the rebuild
  re-lights the chunk, and whether an edited zone's world-map tile is rebuilt.
* **U-9** The meaning of the two bounds of the hit-moment test at Server RVA 0x1395B7.
* **U-10** When the zone save writer's caller runs (unload rule), the meaning of zone byte
  +0x75, and whether saving a zone only for its edits changes its item list on reload.
* **U-11** Which other readers, if any, use the zone's placed-object list (zone +0x3C).

---

## Re-certification (2026-11-15)

Re-checked against fresh MSVC dumpbin disassembly of the original `Server.exe` and
`Cube.exe` (hashes above), constants read from the image bytes. The Infinite-Cubes
reconstruction was not consulted.

**Confirmed**: the three step producers and their append sites; the 20-byte record layout,
the remove encoding (type byte 0) and the day field; air, sea water and implicit-solid
values; the client applier (writer, remover, keep rule, z ≤ 0 sea-water rule, downward
type-6 fill); the floating-sprite rule (bit value 2, one block below, type 0, clamp at 0);
the 3 × 3 chunk flagging with 32-column chunks; the wrap-up storage by the record's own
zone; the zone save layout and the save condition; the loader's keep rule (day ≥ today − 3
or day < 0), its order (items, edits, spawn words, sprite rule, then the light pass); the
placement inputs and record offsets, base-block rounding, rotation direction and evaluation
precision, loop order, zlo/zhi rule, 0x40 protection, 0x46 records, the mixed-up
light-pass sizes, the uncentred sprite rectangle and the 16-byte object entry; the 12
blueprint keys and their numbering; the build-mode inputs and the 0.005 blocks/ms aim rate;
the bomb conditions, sphere test, burst parameters, sound id 1, pitch formula, light-pass
and sprite rectangles; the crater radius/reach table, impact-point rules, exclusions,
*R* = min(trunc ρ, 8), sound id 2; the sphere counts and both worked-example byte dumps.

**Corrected** (old → new):
1. All addresses were virtual addresses ("client 0x…", "server 0x…") → RVAs with the image
   named (VA − 0x400000).
2. Placement "fill-down of a gap below the column bottom … rock-coloured" → type-6 blocks
   coloured by a position-dependent colour routine; also stated that a removal record
   re-applied on load below the stored range writes air and this fill.
3. Writer "nothing if the block is air at or above the column top" → nothing if the new
   type byte is 0 and z ≥ stored top (any colour).
4. Keep rule "a new block of type 0" → a new block whose low five bits are 0; added that
   writes below the stored range bypass the keep check.
5. Kind-7 request "zeroed record" / "client sends item and target zeroed" → a
   default-initialised request body (some fields 1 or −1); the handler reads only the kind.
6. Bomb burst colour "(1.0, 0.2, 0.5)" → three components (1.0, 0.2, 0.5) with the fourth
   component left unwritten (*as read*); small-burst colour carries alpha 1.0.
7. Bomb/crater cell conversions "floor(Q)" / "trunc(C + d)" → stated per use: bomb removal
   trunc x, trunc y, floor z; bomb record floor; crater removal and record trunc; block test
   floor (identical for positive coordinates).
8. Light-pass rectangles "[lo, hi] with margin 8" → columns lo − 8 ≤ x < hi + 8.
9. "Nothing else reads the zone object list" and "no instruction writes field 32 on the
   client" → downgraded to *believed* (only record copies found; static search not
   exhaustive) (U-7, U-11).
10. Bomb condition "not a player" → record byte 0x50 non-zero, meaning *(companion)* (U-3).
11. "Structure blocks … cannot be removed by bombs" → stated only for the flags read here
    (0x46, 0x40); which generated blocks carry 0x20 is *(companion)*.
12. §12 "Saving a zone only because of its edits stores an empty item list" → not
    established by this pass (the writer stores the zone's current item list); moved to U-10.

Clarifications added without changing earlier statements: the placement record is appended
even when the target column is not loaded; the crater's sprite rule runs without the
world-lock call the bomb uses.

None of the corrections reverses a behaviour. Items 2, 3, 4 and 7 affect block results in
edge cases (columns extended downward, keep-flagged blocks, negative coordinates); item 6
affects a particle colour. No random-draw order changed: each producer draws one `rand()`
for its sound pitch after its block loop.

**Removed**: the section "Implementing it in AlphaVulkan"; the replacement advice in §6.4
and §7; the "Corrections to other specs" section (its native facts are in the body); the
game-run, capture and by-ear checks of the old open points; the measured model sizes and
voxel counts; the creature list with box widths; day-length and request-rate figures; the
200-block sound/particle distance and other network distribution details not re-verified;
the bomb recap from the simulation spec.

**Unresolved**: U-1 … U-11 above.
