# Alpha block edits: how the world's blocks change at run time

Behavioural specification of every way Cube World **Alpha** changes the blocks of an
already generated world, and of how such a change is recorded, sent, stored, expired and
applied: the 20-byte block-edit record, the zone edit list and its save, the "place a voxel
object" request (packet 6 kind 7) with its blueprint table and the client's hidden build
mode, the bomb explosion, the craters that big creatures dig with their attacks, how a
client applies an edit to its terrain and refreshes meshes and light, and what the server
does with all of it.

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/client` and
`…/server`), the per-function assembly of the two simulation steps (whose C output is
missing) and fresh disassembly of the binaries in `binary/`. Nothing here is copied code.
Routines are cited by address in `Cube.exe` ("client 0x…") or `Server.exe` ("server 0x…").
Inside the simulation step, client address = server address + 0xDA240
(`alpha-server-simulation.md` §1, §3); only the server address is given there. Offsets into
in-memory objects are given only for cross-checking. Readings not confirmed by a second
path are marked *believed*.

**Status.** The record, the three run-time producers, the client and load-time appliers,
the expiry rule, the server's storage and request answers, the blueprint table and the
placement geometry are specified. Open points are listed in §15.

Related specs (cited, not repeated): `alpha-net-packets.md` §3.4–3.5 (world update,
section 1), `alpha-server-simulation.md` §3, §6.2, §8, §11 (step order, bomb, packet 6,
event set), `alpha-world-items.md` §3.5, §4.3 (day filter, zone save quirk),
`alpha-saves.md` §5.2 (zone record), `alpha-worldgen-terrain.md` §1.3–1.5 (blocks,
implicit blocks, block writer), `alpha-worldgen-structures.md` §6.2 (prefab placer, the
generation-time cousin of §6 here), `alpha-rendering.md` §9.2 (light flood),
`alpha-creatures-behaviour.md` §3.3, §5, §6, §9, §14 (action timing, hit resolution, sound
records and ids, spawn objects), `alpha-ui-screens.md` §11 (keys and mouse buttons).

---

## 0. Conventions

* **Block**: 4 bytes R, G, B, type byte (`alpha-worldgen-terrain.md` §1.3). Type = low 5
  bits; flag **0x20** "natural" (set on blocks coloured by the terrain colouring); flag
  **0x40** "protected / dry" (structure blocks and protected air); flag **0x80** "keep".
  "Air" below means the block (255, 255, 255, type byte 0); "sea water" (255, 255, 255,
  0x82).
* Positions of creatures are i64 in 1/65536 block ("fixed"). Block coordinates are i32.
  Converting fixed to block coordinates divides by 65,536 rounding **toward zero** (all
  playable coordinates are positive, so this equals floor). Zone = block ÷ 256.
* "trunc" = toward zero, "floor" = toward −∞; "*r*" = one C `rand()` / 32767.
* Day = the world clock's day counter (world +0x800160, packet 5, save key `time`).
* "The step" = the world simulation step (server 0x5322d0 = client 0x60c510).
  "Offline" = a client world without a server (online flag world +0xb4 clear and a local
  player); "server" = online flag clear and no local player (world +0xb8 = none); "online
  client" = online flag set (`alpha-server-simulation.md` §1).

---

## 1. Overview: what changes blocks in Alpha

Generation writes blocks but makes no edit records; it is re-run every time a zone is
generated. At run time exactly **three** things change blocks, all inside the step:

| Producer | Who runs it | What it does | Record kind |
|---|---|---|---|
| **Packet 6 kind 7**, place a voxel object (§6) | server and offline clients; skipped by online clients | stamps one of 12 blueprint models into the world | place (type byte 0x46) |
| **Bomb explosion** (§8) | the step's dead-creature pass, wherever it runs | removes natural blocks within 8 blocks | remove |
| **Big-creature impact crater** (§9) | server and offline clients (online check) | removes natural blocks around the impact point of an attack by a creature wider than 4 blocks | remove |

There is **no** player digging, no mining tool and no block-placing item in Alpha. The only
block-changing thing a player can normally do is use a **Bomb** (item type 1, subtype 7;
crafted, `alpha-crafting.md` §1, or bought, `alpha-talk-vendors.md`). The client contains a
complete "build mode" for kind 7 (§7), but nothing in the shipped client turns it on.

Every producer writes the block into its own world at once **and** appends a 20-byte
record (§2) to section 1 of the step's output set. A search of both binaries for writers to
section 1 (event set +0x18) finds only these three places (server 0x537566, 0x534ceb,
0x539bf0), plus the prefab placer's optional output list, which no caller uses.

---

## 2. The block-edit record (20 bytes)

Replaces the table of `alpha-net-packets.md` §3.5:

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 12 | 3 × i32 | block x, y, z |
| 0x0c | 3 | 3 × u8 | colour R, G, B |
| 0x0f | 1 | u8 | the full **type byte** (type and flags). **0 = remove**; anything else = write this block |
| 0x10 | 4 | i32 | **day** on which the edit was made (world day counter). Negative = never expires (no Alpha producer writes a negative day) |

Records written by Alpha:

| Producer | Colour | Type byte | Day |
|---|---|---|---|
| placement (§6) | the voxel's colour | **0x46** (type 6 "rock", protected) | today |
| bomb, crater (§8, §9) | (255, 255, 255) | **0** | today, also for z ≤ 0 (the receiver turns it into sea water, §4.1) |

The day field is the one `alpha-world-items.md` §3.5 filters on. The in-memory copy is
exactly these 20 bytes with no padding, so the wire, the zone list and the save are
byte-identical.

---

## 3. Life cycle of an edit

1. A producer (§6, §8, §9) changes the block in its own world and appends the record to
   output section 1.
2. **Server:** the main loop sends section 1 to **every** connection, unfiltered
   (`alpha-net-packets.md` §3.4). **Offline client:** the frame update applies its own
   output like received records (*believed*: the step's local output set is merged into the
   frame's event lists; the visible effect, §4.1, is the mesh refresh).
3. At the end of the same step (wrap-up, server 0x547111..0x5471dc) every record is
   appended to the edit list of **the zone that contains the record's own (x, y)**
   (zone +0x68, a vector of 20-byte records), if that zone is loaded. A record whose zone
   is not loaded is lost from storage (it was still sent).
4. When the zone is saved (`alpha-saves.md` §5.2: written when the zone's item flag is set
   **or its edit list is not empty**; server on unload, `alpha-server-simulation.md` §2.1)
   the whole list is stored, in list order.
5. When the zone is generated again, the saved list is filtered by day and re-applied
   (§4.2, §5); the kept records become the zone's new list.
6. **Server:** a packet-11 request for a loaded zone is answered with the zone's **whole**
   edit list in section 1 (server 0x4272d0; `alpha-net-packets.md` §4.4), in list order,
   together with its items (section 7) and static-entity states (section 6).
7. **Online client:** records received in section 1 are applied (§4.1) and then forgotten;
   they are not added to the client's zone lists (*believed*; whether the client writes its
   own `world_online_<seed>.db` is `alpha-saves.md` V-1).

---

## 4. Applying records

### 4.1 Live application on a client (frame update, client 0x48f242..0x48f3d0)

Once per frame, if the frame's section-1 list (game object +0x800648) is not empty, under
the world lock, for each record in order:

* **Type byte ≠ 0: write** with the general block writer (client 0x44e7a0,
  `alpha-worldgen-terrain.md` §1.5): nothing if the column is not loaded; nothing if the
  block is air at or above the column top; at z ≤ 0 a block with (type byte AND 0x5f) = 0
  becomes sea water; the column grows as needed, and a gap opened below the old bottom is
  filled with rock-coloured type-6 blocks.
* **Type byte = 0: remove** (client 0x471920): only if 0 ≤ x, y < 2²⁴, the column is loaded
  **and** bottom ≤ z < bottom + count (a stored block). The stored block becomes **sea
  water** if z ≤ 0, else **air**. Blocks below the column's stored range (implicit solid,
  terrain §1.4) cannot be removed this way.
* Both go through the cell store (client 0x44e700), which applies the **keep rule**: if the
  stored block has flag 0x80, a new block of type 0 is ignored, and a non-zero one is
  written with 0x80 added. So removing a "keep" block does nothing.

Then:

1. **Floating sprites.** For every loaded zone overlapping the rectangle of the edited x and
   y (minimum to maximum, clamped at 0; client 0x5a5cb0), every record of the zone's sprite
   list (zone +0x04, `alpha-worldgen-trees.md` §5) whose word +0x38 has bit 1 (value 2: the
   "rests on the ground" mark of trees and plants) and whose block one below its position
   now has type 0 is **deleted**.
2. **Mesh refresh.** The records are moved to a frame-local list; in the render part of the
   frame (client 0x4956e0..0x495808), for every record the **3 × 3 chunks** (a chunk is
   32 × 32 columns) around the chunk holding (x, y) are flagged for rebuilding (chunk
   +0x74 := 1, +0x70 := the time), if that chunk is currently in the chunk cache.
3. The mesher rebuilds a flagged chunk and re-runs the **light flood** over it with a
   4-block border (`alpha-rendering.md` §9.2). So light and occlusion follow the edit within
   a frame or two. Records outside any loaded column are dropped without trace (they come
   again when the zone is requested, §10).

### 4.2 When a zone is generated (client 0x44cdb0, server 0x41ee20)

At the end of zone generation (call at client 0x5ede46) the zone's save, if any, is read
(`alpha-saves.md` §5.2): items (`alpha-world-items.md` §3.5), then the edits. Each saved
edit is **kept** if its day ≥ today − 3 or its day < 0. A kept edit is written with the
general block writer (client 0x44e7a0; also for type byte 0, which then writes plain air in
range, sea water at z ≤ 0, and nothing above the column top) and appended to the zone's new
edit list. If the saved list had at least one entry, the floating-sprite rule of §4.1 step 1
runs over that zone. This happens **before** the zone's light flood (client 0x5ede84), so a
freshly generated zone is lit with its edits in place. Dropped edits are gone for good: the
next save no longer contains them.

### 4.3 Inside the step

The producers write their own world directly (server twins of the writers above: 0x41ff00
write, 0x530470 remove; the same rules) and the server additionally re-lights the area
(§6.5, §8, §9); see each producer.

---

## 5. The day field and the 3-day expiry

* An edit made on day *d* survives every reload of its zone while the reloading day is
  ≤ *d* + 3; on the first reload on day *d* + 4 or later it is dropped, and the generated
  terrain comes back.
* While the zone stays loaded nothing expires (there is no in-play cleanup).
* Kept edits keep their original day: re-saving does not renew them.
* The day advances with the clock (one day ≈ 2 h 24 min of real time at the normal rate,
  faster while someone rests, and +1 at once at an inn at night;
  `alpha-server-simulation.md` §3, §8 kind 2), so craters and placed objects heal within a
  few hours of real time once nobody is near them.

**Timeline.** A bomb crater made on day 5 at 10:00. The player leaves; the zone is saved
and unloaded within a second. Back on day 8: the save is loaded, 5 ≥ 8 − 3, the crater is
re-applied. Leaving again and returning on day 9: 5 < 6, the crater is dropped, the
original ground is back and the zone's save now has no edits.

---

## 6. Placing a voxel object (packet 6 kind 7)

### 6.1 The request

Packet 6 (`alpha-net-packets.md` §4.1, `alpha-server-simulation.md` §8) with kind byte
(+0x128) **7**. The item and target fields are not used (the client sends them zeroed,
*believed*). Everything else comes from the **sender's creature record**, which the client
sends in packet 0 just before packet 6 in the same pass (`alpha-net-transport.md` §3.1):

| Input | Record field | Meaning |
|---|---|---|
| object number *n* | field 32, byte 0x17d, **signed** | blueprint number (§6.2) |
| rotation *q* | field 31, byte 0x17c, signed | quarter turns: angle = *q* · 90° |
| aim **a** | field 26, 0x150, 3 × f32, blocks | offset from the creature to the placement point |
| position **P** | field 0, fixed | the creature's centre |

Handled in the per-creature part of the step (server 0x5370b7..0x537787, client
0x6112f7..0x6119c7). It does nothing on an online client (online flag set), and nothing if
blueprint *n* does not exist. The server performs no checks of distance, rate, permission
or ownership.

### 6.2 Blueprints (world +0x800154)

Both programs load, when a world object is created (server 0x4c8570, client 0x58eb00), a
map from object number to a voxel model read from `data1.db` (same `.cub` layout as every
model: three u32 sizes, then sx·sy·sz RGB voxels, index (k·sy + j)·sx + i; colour
(0, 0, 0) = empty):

| *n* | Model key | sx × sy × sz | Non-empty voxels |
|---|---|---|---|
| 0 | `framework-floor-wood.cub` | 10 × 10 × 6 | 600 |
| 1 | `framework-wall-window.cub` | 16 × 16 × 7 | 100 |
| 2 | `framework-wall-door.cub` | 18 × 18 × 7 | 99 |
| 3 | `framework-roof1.cub` | 16 × 16 × 8 | 520 |
| 4 | `framework-entrance.cub` | 10 × 10 × 5 | 464 |
| 5 | `framework-wall.cub` | 14 × 14 × 7 | 98 |
| 6 | `framework-roof2.cub` | 16 × 16 × 8 | 616 |
| 7 | `framework-roof3.cub` | 16 × 16 × 8 | 798 |
| 8 | `framework-wall-balcony.cub` | 20 × 20 × 7 | 152 |
| 9 | `framework-roof-wall2.cub` | 12 × 12 × 6 | 25 |
| 10 | `building-stone-well.cub` | 10 × 10 × 10 | 102 |
| 11 | `building-stone-stairs2.cub` | 10 × 10 × 10 | 550 |

(Sizes and counts measured from the extracted assets.) Other `framework-*.cub` files
(`framework-arc`, `-base`, `-floor`, `-floor-stairs`, `-roof-wall1`, `-wall-indoor`,
`-wall-lamp`) exist in `data1.db` but are not in the table. All twelve models have even
x and y sizes, which matters in §6.3. Object 0 is a solid slab: five layers of grey
(165, 165, 165) under one layer of wood brown (101, 40, 0).

### 6.3 Where the voxels go

1. **Base block B** = (P + trunc(**a** · 65536) + 32768 per axis) ÷ 65536, toward zero: the
   block nearest to the point **P** + **a** (each conversion of a float to fixed truncates).
   There is no snapping to the ground.
2. **Transform.** For voxel (i, j, k) let *u* = i + 0.5 − sx/2 and *v* = j + 0.5 − sy/2
   (single precision). The point (*u*, *v*, k + 0.5) is rotated about the vertical axis by
   *q* · 90° counter-clockwise (x′ = cos·*u* − sin·*v*, y′ = sin·*u* + cos·*v*; angle in
   single precision, cosine and sine in double), each coordinate is multiplied by 65,536
   and truncated, added to B · 65,536, and divided by 65,536 toward zero. The result is:

   | *q* mod 4 | X | Y | Z |
   |---|---|---|---|
   | 0 | Bx + ⌊*u*⌋ | By + ⌊*v*⌋ | Bz + k |
   | 1 | Bx + ⌊−*v*⌋ | By + ⌊*u*⌋ | Bz + k |
   | 2 | Bx + ⌊−*u*⌋ | By + ⌊−*v*⌋ | Bz + k |
   | 3 | Bx + ⌊*v*⌋ | By + ⌊−*u*⌋ | Bz + k |

   With even sizes *u* and *v* are half-integers and the table is exact for every *q*
   (−128..127). With an odd size a coordinate becomes an integer and the tiny
   cosine/sine error of large |*q*| can push it across the truncation (off by one, *as
   read*); none of the shipped blueprints is affected. The model is therefore centred on
   B in x and y and stands on B in z (k = 0 at Bz).
3. **Order**: i ascending (outer), j ascending, k **ascending** (inner).
4. **Per column (i, j)**: zlo / zhi = lowest / highest k with a non-empty voxel (zlo = sz,
   zhi = 0 when the column is empty). For each k, with C = the voxel colour and E = the
   world block now at (X, Y, Z) (implicit read rules, terrain §1.4):
   * C empty and zlo < k < zhi: write (0, 0, 0) with type byte **0x40** (protected air)
     into the world. **No record** is made.
   * C empty otherwise: nothing.
   * C non-empty and E has flag **0x40** (any type, including protected air): nothing
     (structures and earlier placements win). No record.
   * C non-empty otherwise: write (C, type byte **0x46**) with the general writer
     (including its fill-down of a gap below the column bottom) and append the record
     (X, Y, Z, C, 0x46, today) to section 1.

   Unlike the generator's prefab placer (`alpha-worldgen-structures.md` §6.2) there are no
   marker colours, no water colour, no fill-down from the model's bottom voxel and no zone
   clip.

### 6.4 The zone's object list and section 8

After the voxels, if the zone containing (Bx, By) is loaded, a 16-byte entry
**(i32 *n*, i32 Bx, i32 By, i32 Bz)** is appended to that zone's object list (zone +0x3c)
and the zone joins the step's "object list changed" set. The wrap-up sends each such zone's
**whole** list in section 8: key = i32 zx, i32 zy (from zone +0x60/+0x64), u32 count, then
the 16-byte entries (server 0x52e950). This settles the layout of section 8.

A client receiving section 8 (client 0x48f58d..0x48f6b9) replaces that zone's list with the
received entries and clears the zone's "dynamic data requested" byte (zone +0x74, also
cleared by section 7). Nothing else reads the list (the only users of the blueprint map are
the placement itself and the build-mode preview, §7), it is **not saved**, and the rotation
is not in it. *Believed*: bookkeeping for an unfinished building feature. An AlphaVulkan
server must still fill and send it the same way for wire compatibility.

### 6.5 After placing

* **Server only** (no local player): the light flood (server 0x4d1a70, the algorithm of
  `alpha-rendering.md` §9.2 with a margin of 8 columns) over the rectangle x from
  trunc(Bx − sx/2) to trunc(Bx + sy/2), y from trunc(By − sy/2) to trunc(By + sz/2) — the
  sizes are mixed up *as read* (sy for the x end, sz for the y end); the 8-column margin
  hides this for the shipped blueprints. Computed in single precision. An offline client
  skips this and relies on the mesh refresh of §4.1.
* Server and offline: the floating-sprite rule (§4.1 step 1) over the zones overlapping
  x ∈ [Bx, Bx + sx], y ∈ [By, By + sy] (not centred, *as read*; zone granularity hides it
  most of the time).
* Sounds, particles and creature checks: none.

---

## 7. Build mode in the client (present but unreachable)

The client contains the whole user side of kind 7, gated by a byte of the game object,
**G+0x800704** ("build mode"):

| Input (`alpha-ui-screens.md` §11) | Effect in build mode | Where |
|---|---|---|
| **R** pressed (edge: R held now, not held last frame, client 0x49b596) | queue a kind-7 request (zeroed record, kind 7) on the local player, which goes to the server online or to the local step offline | client 0x49797d..0x4979c7 |
| **right mouse button** pressed | rotation field 31 += 1 (quarter turn; signed byte, wraps) | client 0x47ccff..0x47cd0e |
| **W / S / A / D**, **Space** / **Left Ctrl** held | move the aim (field 26) by 0.005 blocks per ms (5 blocks/s) along −y / +y / +x / −x / +z / −z of the camera's yaw frame, instead of setting it from the crosshair | client 0x49b19a..0x49b4c1 |
| (frame) | draw a preview of blueprint field 32 at the placement point with rotation field 31 | client 0x4b3338.. |
| (frame) | the ability/attack input and the slot list are skipped while build mode is on | client 0x49797d, 0x49b5b1 |

Nothing ever sets G+0x800704: its only write is the clear at game-object creation (client
0x45a09b), and no instruction writes the creature's field 32 (object number) on the
client. *Believed* therefore:

* In vanilla Alpha the player **cannot build**: no item, key or menu gives field 32 or
  enters build mode. Field 32 stays 0 (it is not in the character save, `alpha-saves.md`
  §3.2), so a mod that only turns build mode on would always place blueprint 0
  (`framework-floor-wood`).
* A server will only ever see kind 7 from modified clients. AlphaVulkan may expose build
  mode (for example as a debug or mod feature) as long as it sends exactly this record.

---

## 8. The bomb explosion

Recap of the bomb itself: `alpha-server-simulation.md` §8 kind 1 (a creature of type 144,
controller 6, losing 0.025·dt health per step) and §6.2. The explosion (server
0x534998..0x534ed0) runs in the dead-creature pass for a creature that is not a player,
has health ≤ 0, type **144** and death timer (record 0x5c) **still 0** — its first dead
step. With **P** = the bomb's position (fixed):

1. One **particle burst** at **P** (section 3, `alpha-server-simulation.md` §11): colour
   (1.0, 0.2, 0.5) *as read*, count 20, size 0.25, style 1, velocity (0, 0, 20).
2. For dx = −8 … 8 (outer), dy = −8 … 8, dz = −8 … 8 (inner), with
   dx² + dy² + dz² **< 64**: let Q = **P** + (dx, dy, dz) blocks and E = the block at
   floor(Q). If E has flag **0x20** (natural terrain; server 0x5306c0):
   * a small burst at Q in E's colour (count 3, size 0.5, velocity (0, 0, 10)) to
     section 3;
   * the block is removed in the world (stored range only; sea water at z ≤ 0, else air);
   * the record (floor(Q), 255, 255, 255, type byte 0, today) goes to section 1.
3. If at least one block was removed:
   * a sound (section 4) at **P**: id **1**, pitch 0.5 + 0.4·*r*, volume 1 (*as read*; id 1
     is `blade1.wav` in `alpha-creatures-behaviour.md` §14);
   * server only: the light flood over [Px − 8, Px + 8] × [Py − 8, Py + 8] (block units,
     toward zero) with margin 8;
   * under the world lock: the floating-sprite rule over the zones overlapping
     [Px − 8, Px + 8] × [Py − 8, Py + 8].
4. Then the damage to creatures around the bomb (`alpha-server-simulation.md` §6.2).

What a bomb does **not** remove: anything without flag 0x20 — structure blocks (0x46), placed
objects (0x46), water, the implicit solid below a column's stored range, and protected air.
Up to 2,103 cells lie inside the sphere; on flat ground about half of them are below the
surface (worked example §13.1).

*Believed*: this pass also runs on an online client that receives a dead bomb in packet 0
(there is no online check before it); the client would then remove the same blocks locally
and duplicate the server's records. Harmless, but see §15.

---

## 9. Craters from big creatures' attacks

When a creature's action reaches its hit moment (`alpha-creatures-behaviour.md` §3.3,
wind-up P1; code server 0x5395a9..0x539e44), the step computes an **impact point** and an
**impact radius**, used for the hit itself and, for big creatures, for a crater.

Let *w* = the creature's collision-box width (appearance +0x08, record 0x70), *A* = its
action id (record 0x58), yaw = record field 1 third component **in degrees** (the rotation
routine converts degrees).

**Radius *ρ* and forward reach *f*** (first matching row):

| Action ids | *ρ* | *f* |
|---|---|---|
| 1, 2, 3, 4, 6, 7, 9, 13, 14, 18, 19 (weapon swings) | 2*w* | *w* |
| 20, 21 | 3*w* | 1.5*w* |
| 68, 70 | 1.5*w* | 0 |
| 86, 93, 104 | 3*w* | 0 |
| 87, 88 | 8 | 0 |
| 30, 32 | 3 | 0 |
| 31, 33 | 5 | 0 |
| 91, 74 | 20 | 0 |
| 72, 76, 77, 78 | 1.8*w* | 0 |
| any other | 1.5*w* | 1.5*w* |

**Impact point C**: position + the vector (0, *f*, 0) turned by yaw about the vertical
(forward is +y at yaw 0). Then: actions 30–33 use C = position + aim (field 26); actions 69
and 75 raise C.z by (box height − *w*)/2; actions **11, 54, 61** use C = position and
*ρ* × 3.

**Crater** — only if the world is **not online**, *w* **> 4.0** blocks, and *A* ∉ {87, 91,
74}: *R* = min(trunc(*ρ*), 8). For dx, dy, dz = −*R* … *R* (dx outer, dz inner) with
dx² + dy² + dz² < *R*²: exactly the per-block rule of §8 step 2 around C (natural blocks
only; burst, removal, record with today's day; position = trunc(C + d)). If anything was
removed: a sound at the creature, id **2**, pitch 0.5 + 0.4·*r* (*as read*); server only,
the light flood over [Cx − *ρ*, Cx + *ρ*] × [Cy − *ρ*, Cy + *ρ*] with margin 8; and the
floating-sprite rule over the same rectangle.

Which creatures qualify: those with box width > 4 in `alpha-creatures-models.md` — blue crab
(107, 4.8), trolls (108, 109, 5.6), golems (111–113, 5.6), yeti (114, 5.6), cyclops (115,
4.8), lich (117, 6.4) and similar large monsters. Players (box 1.2) never dig. Because of
the owner rule (`alpha-server-simulation.md` §1) craters are made by the server for NPCs and
by an offline client for its NPCs; online clients never make them and get them as section-1
records.

**Example.** A troll (*w* = 5.6, box height 12.6) at centre z = 71.3 (feet at 65) swings with
action 3: *ρ* = 11.2, *f* = 5.6, so C is 5.6 blocks in front of it at z = 71.3, *R* = 8.
Cell z = floor(71.3 + dz) reaches 64 only for dz = −7, where dx² + dy² < 15: the top
ground layer is removed in a disc of 45 cells (radius about 3.9) under C — a shallow
dent. A troll on a slope or a smash (action 54, *ρ* = 3·8.4, C at its
centre) digs deeper.

---

## 10. The server's role

* **Authority.** All block edits that reach online players are made by the server's step
  (§6, §8, §9). Clients never send block edits; they only send packet 6 (kind 1 bomb, kind
  7 placement) and packet 0 (the fields kind 7 reads).
* **Broadcast.** Section 1 goes to every connected client every tick it is non-empty, with
  no distance filter. A client that has not generated the zone drops the records (§4.1).
* **Storage.** The wrap-up appends the step's records to their zones' lists (§3 step 3);
  the zone is saved when unloaded (5 × 5-zone rule, `alpha-server-simulation.md` §2.1) and
  re-applied with the 3-day filter when the server generates it again (server 0x41ee20).
* **Requests.** Packet 11 for a loaded zone → its whole stored list in section 1 (plus
  sections 6 and 7). The client asks for the zones it wants once a second
  (`alpha-net-transport.md` §3.1); *believed*: it stops asking for a zone once that zone's
  item list (section 7) has arrived (zone +0x74 cleared), so a list is normally sent once
  per zone visit; repeats are harmless because re-applying the list gives the same blocks.
* **Not answered**: a zone the server has not loaded. Its edits exist only in the server's
  save until a player comes near enough for the server to load it (§2.1 of the simulation
  spec), so a client generating far ahead shows unedited terrain there until then.

---

## 11. Ordering and conflicts

1. **Edits after generation.** Saved edits are applied after the whole zone generator
   (terrain, structures, vegetation, items) and before the zone's light flood (§4.2); an
   online client applies the server's list whenever it arrives, on top of its own freshly
   generated zone. Edits always win over generated blocks, except under the keep rule.
2. **List order.** Within a zone, records apply in creation order; a later record for the
   same cell wins. The list is never compacted: placing twice in the same spot or exploding
   a bomb in an old crater just appends more records.
3. **Keep flag.** A stored block with flag 0x80 ignores removal (§4.1).
4. **Protection.** Placement never overwrites a block with flag 0x40 (structures, earlier
   placements, protected air). Bombs and craters only remove blocks with flag 0x20, so they
   cannot destroy structures, placed objects or anything a placement wrote.
5. **Zone borders.** Each record is stored in the zone of its own (x, y), so an object or
   crater crossing a border is split correctly between zones. The section-8 entry goes to
   the zone of the base block only.
6. **Unloaded zones.** Records for cells whose column is not loaded are dropped by the
   applier and, on the server, not stored (§3 step 3).
7. **Interior air of placements** (§6.3) is written only where the step runs (server,
   offline) and has no record, so online clients keep whatever terrain was inside a hollow
   object. Of the twelve blueprints only `building-stone-well` (10) has such gaps (79
   interior empty voxels: the shaft); the others have none (measured).
8. **Day expiry is per record**: a crater and a later object placed in it can expire on
   different days, leaving the object floating or the crater half filled (*as read*).

---

## 12. The zone save, revisited

`alpha-saves.md` §5.2 parts 3 and 4, with what is now known:

* **Part 3, block edits**: the zone's edit list (§3), 20 bytes each (§2).
* **Part 4, two words per "dynamic object"** (open question **V-3**): the objects are the
  zone's **spawn objects** (zone +0x18 list, `alpha-creatures-behaviour.md` §9, the creature
  list of the structures spec), and the words are spawn object **+0x38 = respawn countdown
  (ms)** and **+0x3c = day of death**. On load (client 0x44cdb0, server 0x41ee20) they are
  copied back only if the saved count equals the number of spawn objects the generator just
  made, in order. This is what keeps a killed spawn dead across an unload
  (`alpha-server-simulation.md` §6.2 step 8). It is unrelated to the 160-byte world dynamic
  objects of packet 3.
* The zone's 16-byte placed-object list (§6.4) is **not** saved.
* Saving a zone only because of its edits stores an empty item list, which on reload
  deletes that zone's generated items (`alpha-world-items.md` §4.3, open W-3).

---

## 13. Worked examples

### 13.1 A bomb crater

Flat natural ground: every column has stored natural blocks (flag 0x20) up to the top block
at z = 64 and down past z = 56. A bomb (box 0.8) sits on it at
**P** = (8,398,402.5, 8,397,472.5, 65.4) blocks; day 5.

* Cells inside the sphere: 2,103. Their block z = floor(65.4 + dz) = 65 + dz. Those with
  z ≤ 64 (dz ≤ −1) are ground: **955 records**, by layer: z 64: 193, 63: 185, 62: 177,
  61: 145, 60: 121, 59: 89, 58: 45.
* Loop order gives the first record at dx = −7, dy = −3, dz = −2:
  (8,398,395, 8,397,469, 63):

      3b 26 80 00  9d 22 80 00  3f 00 00 00  ff ff ff 00  05 00 00 00

  and the last at dx = 7, dy = 3, dz = −1: (8,398,409, 8,397,475, 64).
* The tick's world update: section 1 count 955 (19,100 bytes of records) to every client;
  section 3 with 1 + 955 bursts and section 4 with the explosion sound to clients within
  200 blocks; then the damage. Each client applies the 955 removals, deletes plants
  standing on the removed top layer in zone (32806, 32802), and flags the 3 × 3 chunks
  around each edited column (chunks x 262,448–262,451 × y 262,419–262,422 here).
* All 955 records are appended to zone (32806, 32802)'s list. Expiry as in §5.

### 13.2 Placing blueprint 0

The sender's record: position **P** = (8,398,402.3, 8,397,472.8, 65.35), aim
**a** = (3.4, −0.2, −1.2), field 31 = 1, field 32 = 0; open grassland (no 0x40 blocks);
day 5.

1. B = trunc(**P** + **a** + 0.5) = (8,398,406, 8,397,473, 64).
2. Model 10 × 10 × 6, *q* = 1: *u* = i − 4.5, *v* = j − 4.5, so
   X = Bx + ⌊4.5 − j⌋ = 8,398,410 − j, Y = By + ⌊i − 4.5⌋ = 8,397,468 + i, Z = 64 + k.
   The slab covers x 8,398,401…8,398,410, y 8,397,468…8,397,477, z 64…69.
3. Every column is full (zlo 0, zhi 5): **600 records**, i outer, j, k inner. First,
   voxel (0, 0, 0) grey:

       4a 26 80 00  9c 22 80 00  40 00 00 00  a5 a5 a5 46  05 00 00 00

   Last, voxel (9, 9, 5) wood: (8,398,401, 8,397,477, 69):

       41 26 80 00  a5 22 80 00  45 00 00 00  65 28 00 46  05 00 00 00

4. Zone of B: (32806, 32802). Its object list gains (0, 8,398,406, 8,397,473, 64); if the
   list was empty, section 8 carries

       26 80 00 00  22 80 00 00  01 00 00 00
       00 00 00 00  46 26 80 00  a1 22 80 00  40 00 00 00

5. Server: light flood over x from 8,398,401 − 8 to below 8,398,411 + 8 and y from
   8,397,468 − 8 to below 8,397,476 + 8 (the mixed sizes of §6.5: the x end uses sy/2 = 5,
   the y end sz/2 = 3); sprite cleanup in zone (32806, 32802).
6. The 600 records are stored in that zone's edit list and expire as in §5. A later bomb
   next to the slab removes the grass around it but not the slab (type byte 0x46 has no
   flag 0x20).

---

## 14. Implementing it in AlphaVulkan

* Keep one record type (§2) for the wire, the zone list and the save; never invent a
  different layout. Write today's day in every record you create; treat negative days as
  permanent when reading.
* Server: run the three producers exactly as specified (they are the only sources); store
  per zone at the end of the step; answer packet 11 with the whole list; send section 8 for
  kind 7 even though nothing uses it.
* Client: apply section 1 with the two writers of §4.1 (including the keep rule and the
  "stored range only" rule for removal), then the sprite cleanup, then flag 3 × 3 chunks and
  re-run the light flood with a 4-block border when re-meshing.
* Zone load: apply the save after generation and before the light flood, with the 3-day
  filter.
* A modern renderer may refresh meshes and light differently, but the stored blocks and
  light values must end up as Alpha's for the same records.
* Build mode (§7) can be offered as an opt-in feature; it must send exactly a zeroed kind-7
  packet 6 after a packet 0 carrying fields 26, 31 and 32.

---

## 15. Corrections to other specs

* `alpha-net-packets.md` §3.5: byte 0x0f is the whole type byte (type and flags; 0 =
  remove); bytes 0x10–0x13 are the **day** (i32), not padding. Section 8 (§3.4) is the
  zone's placed-object list: key i32 zx, i32 zy, then *n* × (i32 object number, 3 × i32
  base block) (§6.4).
* `alpha-net-packets.md` §3.5 "rebuilds the meshes of the bounding box": the client flags
  the 3 × 3 chunks around each record and runs the sprite cleanup over the zones of the
  bounding box (§4.1).
* `alpha-server-simulation.md` §6.2: a bomb removes only **natural** blocks (flag 0x20)
  within the sphere dx² + dy² + dz² < 64, not "every block within 8 blocks" (§8).
  §8 kind 7: the rotation (field 31) is also used; placement skips 0x40 blocks; the object
  list entry is (number, base block) (§6). §13 "dynamic objects … saved per zone": the
  saved words belong to the zone's spawn objects (§12).
* `alpha-saves.md` V-3: answered (§12).
* `alpha-creatures-behaviour.md` / `alpha-server-simulation.md`: the attack impact of
  creatures wider than 4 blocks digs craters (§9); not mentioned there so far.
* `alpha-net-packets.md` §2.1 field 1 note "yaw in radians": the impact code turns it with
  a degree-based rotation (§9) (*believed* degrees).

---

## 16. Open points

* **E-1** Confirm in a game run (offline, a copy of the save) that a bomb crater appears as
  specified (955 blocks in §13.1's setting) and comes back after 4 game days.
* **E-2** Whether an online client also runs the bomb explosion for a received dead bomb
  (§8); a capture with two clients shows whether section 1 records are duplicated locally.
* **E-3** Whether an online client keeps received edits in its zone lists or saves them
  (`alpha-saves.md` V-1).
* **E-4** The sound ids 1 (bomb) and 2 (crater) read from the records look like weapon
  sounds; check by ear in a recording, or the client's sound mapping for these records.
* **E-5** The particle colour (1.0, 0.2, 0.5) of the bomb burst (component order as read).
* **E-6** Whether anything sets build mode or field 32 through a path not visible statically
  (unresolved indirect calls); a memory watch on G+0x800704 during play settles it.
* **E-7** The meaning of the chunk timestamp +0x70 set with the rebuild flag (a delay before
  re-meshing is likely) and whether the world map tile of an edited zone is ever rebuilt.
* **E-8** The exact hit-moment condition for the crater code (action time against the two
  bounds at server 0x5395b7) is shared with the hit itself and should be checked against
  `alpha-creatures-behaviour.md` §3.3.
