# Alpha open points, batch 5: map-cell level, the mana cube of the ruins, and the end of the zone generator

Clean-room behavioural specification. It closes these points:

| Point | Spec it belongs to | Section here |
|---|---|---|
| Routine 0x59a9b0: level of a creature placed at (x, y) | `alpha-worldgen-structures.md` §5.5.3, §6.2; `alpha-worldgen-caves.md` open question 4 | §1 |
| Which creatures use it, and who overwrites the result | structures §5.5.9, §5.5.10, §6.2; `alpha-spawn-objects.md` §3 | §2 |
| Routine 0x5f77b0: the ground item of the sphinx and the obelisk | structures §5.5.5 | §3 |
| Leftovers of the zone generator with an effect on a zone | overview §5, `alpha-spawn-objects.md` §3.1, trees open questions 1 and 4 | §4 |

It also corrects a few statements of earlier specs (§5), notably: kinds 17 to 20 lower their
origin until it stands on something; kind 18 has **four** models; item byte +0x138 **is** read;
and a pass after the finishing pass drops generated items to the ground.

Written from the Alpha client (`Cube.exe`) decompile and per-instruction listing. The server
(`Server.exe`) was checked where noted. Constants were read from the client's data section. No
code was copied; addresses are citations only. *Believed* marks a reading that rests on partial
evidence; *as read* marks behaviour that looks unintended but is what the code does.

Conventions: as in the structures spec §0. "fx" is 1/65536 block in an i64; a "draw" is one
`rand()` from the zone's stream; "spawn +x" is the 0x10f0-byte record of
`alpha-spawn-objects.md`; "item record" is the 0x118-byte record of
`alpha-items-generator.md` §0; "world item" is the 0x148-byte record of
`alpha-world-items.md` §1. L is a location record (`alpha-worldgen-locations.md` §1.2), Lz the
one of the zone's own cell, f the normalised distance of `alpha-worldgen-regions.md` §7.

---

## 1. Routine 0x59a9b0: level of a point (client 0x59a9b0 … 0x59aa53, server 0x4d2340)

**Inputs:** the world, and a position (x, y) as two i64 in fx. Only x and y; no z, no creature
type, no zone argument. **Output:** an i32 level. **Draws: none.** The routine and everything
it calls (the conversion 0x68d946, the division 0x68d9e0, the lookup 0x487da0, the distance
0x5f8e10, the noise 0x4c0ef0) make no `rand()` call and never re-seed. It writes nothing.

### 1.1 The rule

1. **Map cell.** cx = x ÷ 65536 ÷ 2048 and cy = y ÷ 2048 ÷ 65536, each ÷ truncating towards
   zero. (y goes through a double, multiplied by 1/2048 = 0.00048828125 and truncated to i64,
   then ÷ 65536; x uses integers only. The double is exact, so for world positions the two
   orders agree and a port can use floor(block ÷ 2048) for both.)
2. **Record.** The location lookup of regions spec §6.3: cx and cy must lie in 0 … 8191, and the
   region (cx >> 3, cy >> 3) must be prepared (the world's region table holds a pointer). If
   not: **no record**. Otherwise record 8·(cx mod 8) + (cy mod 8) of that region, at region +
   0x14018 + 0x68 · index.
3. **Result.** Level 1, unless all of these hold: the record exists; its type (+0x18) is not 0;
   f(record, x, y) ≤ 1.0 (constant 0x745dc0 = 1.0, compared in single precision: exactly 1.0
   passes). Then the result is the record's level (+0x24), unchanged and unclamped.

Points that follow from this:

* Only **the one record of the point's own cell** counts. A location whose rim reaches into the
  next cell gives nothing there.
* Any non-zero type qualifies, not only towns and ruins.
* f is taken at the exact position passed. All generator callers pass the whole-block corner
  (X·65536, Y·65536), **not** the creature's position with its +0.5 offsets (§2.1).
* f < 0 cannot occur. A radius under 0.001 gives f = 0, so such a record passes; generated
  records of non-zero type never have one. A NaN f would give level 1.
* The level-1 default is what the trap damage formula meets outside a location
  (`alpha-open-points-1.md` §1.5 step 2).

### 1.2 Callers (complete; no indirect reference found)

| Client site | Server site | Who | Position passed | Result goes to |
|---|---|---|---|---|
| 0x5b1c74 | 0x4e53e4 | town builder, kind 6 garden, the scarecrow (kind 140), structures §5.5.3 | (x, y) of the middle column, as fx | spawn +0x34 |
| 0x5f392e | 0x52718e | prefab placer, marker 11 (kinds 141 `aim`, 142 `dummy`) | block (X, Y) of the marker voxel, as fx | spawn +0x34 |
| 0x5f429b | 0x527afb | prefab placer, marker 15 (NPC) | block (X, Y) of the marker voxel, as fx | spawn +0x34, may be overwritten, §2.2 |
| 0x60dc7f | 0x533a3f | world step, trap damage | trap position | `alpha-open-points-1.md` §1.5 |

### 1.3 Worked examples

Common to all: a marker voxel lands on block (X, Y) = (8 392 900, 8 392 300). The inputs are
X·65536 and Y·65536. cx = 8 392 900 ÷ 2048 = 4098 (4098.1…); cy = 8 392 300 ÷ 2048 = 4097
(4097.8…). Region (512, 512), slot (2, 1), record index 8·2 + 1 = 17, at region + 0x14018 +
17·0x68 = region + 0x14700.

| Record 17 | f at the point | Result | Why |
|---|---|---|---|
| type 0 | not computed | **1** | no location |
| type 1 (town), level 1 | 0.30 | **1** | the record's level, which is 1 (§2.3) |
| type 1 (town), level 1 | 1.40 | **1** | outside the rim; same value |
| type 5 (ruin), level 12 | 0.81 | **12** | f ≤ 1 |
| type 5 (ruin), level 12 | 1.00 exactly | **12** | the test is ≤ |
| type 5 (ruin), level 12 | 1.02 | **1** | outside the wobbly rim, though in the same cell |
| region not prepared | not computed | **1** | no record |

(f itself needs the noise of regions spec §7; the values above are given for illustration.)

---

## 2. Which creatures, and what happens to the level

### 2.1 Prefab placer: exactly two markers

Marker 11 and marker 15 of structures §6.2 call the routine right after the creature gets its
position. There is no draw between, so the order of level and facing has no effect on the stream.

**As read: the placer has no per-voxel zone test.** The zone clip of §6.2 rejects a prefab only
when it lies wholly outside the zone. A marker voxel of a prefab that overlaps the zone border
still makes its creature, appended to **this** zone's list (zone + 0x18), and the level comes
from the cell of the marker's own block, which in a corner case is a neighbouring cell. An
engine should not filter markers by zone.

### 2.2 Overwrites inside the placer

For marker 15 the mode switch (jump table 0x5f4b20, modes 6 … 15) runs **after** the call. So
the routine's result survives only in some modes:

| Placer mode M | Who (byte +0x30) | Final +0x34 |
|---|---|---|
| 1 … 5 | house NPC (unchanged) | 0x59a9b0 |
| 6 | identifier (0x83) | 0x59a9b0 |
| 7 | tower guard (0x89) | **20** (0x5f42dd) |
| 9, 10, 11, 12 | class trainers (1, 2, 4, 3) | **20** (0x5f430e, 0x5f4425, 0x5f453f, 0x5f4641); their equipment takes the same 20 |
| 13, 14, 15 | smith, carpenter, weaver (0x85, 0x86, 0x87) | 0x59a9b0 |

Marker 11 has no switch: kinds 141 and 142 always keep the routine's result.

### 2.3 Town zones: the result is always 1

A town record keeps level 1 (`alpha-worldgen-locations.md` §4.1 step 6), and no later phase of
region preparation gives it another. So in a type-1 zone the routine returns 1 on both branches
(a marker whose block lies in a neighbouring cell excepted, §2.1). Towns therefore have level-1
scarecrows, dummies, crafters and marker NPCs, and level-20 guards and class trainers. The test
of f only matters in ruins (type 5), where Lz.level comes from the ranked assignment.

### 2.4 The town builder's other creatures do **not** use it

The town builder (0x5af170) has **one** call of the routine, the garden scarecrow. Every write
of spawn +0x34 in the builder was listed from the assembly:

| Site | Creature | +0x34 |
|---|---|---|
| 0x5b1c8a | garden scarecrow | 0x59a9b0 |
| 0x5ba4f2 | **animals on open plots** (§5.5.10) | lo + d % (hi − lo + 1), a draw; (lo, hi) from 0x444020. Not 0x59a9b0 |
| 0x5baa74 | kind-4 landing creature (dead code) | Lz.level |
| 0x5bcc77 … 0x5bd64a, 0x5be638, 0x5be9b4, 0x5bed70, 0x5bf0ae | inn and shop keepers, residents, ruin creatures (§5.5.9) | Lz.level |

(0x5ba9ce is the airship object of the dead kind-4 code, not a creature.)

So **an innkeeper or shopkeeper of §5.5.9 does not call the routine**: it takes Lz.level with no
f test. Only NPCs that come from marker 15 of a model do. The difference shows outside the rim
of a ruin: at f > 1 a §5.5.9 creature keeps Lz.level, a marker creature gets 1.

Prefabs reached from elsewhere in the zone generator (home building, dungeon arches, palm
stamps) run in mode 0, which reads no markers, so the routine is not reached from them.

### 2.5 After the placer: is the level changed later?

**Not by the finishing pass.** In 0x5ed2c6 … 0x5ed865 spawn +0x34 is read once (0x5ed38b, the
boss spirit's level) and never written. The equipment generator 0x5c7730 only reads it, as a
16-bit value, into item levels. The appearance generator 0x43f7c0 writes +0x34 only of the
appearance record, which is a different field (spawn + 0x74 + 0x34). The level re-roll of
`alpha-spawn-objects.md` §3.2 step 6 changes the levels of **items**, never the spawn's.

The level does steer the stream, though: the equipment step sets slots 7 and 6 to the spawn
level, and the re-roll draws once per slot only when that level is ≥ 3. A level of 1 or 2
therefore means fewer draws than a level of 20 (`alpha-spawn-objects.md` §4.3).

**Other writers after creation:**

* The zone pass 0x5d9460 writes +0x34 only on spawns it creates itself (0x5d97d8).
* Activation copies +0x34 to the creature. It replaces it with the map cell's value only for
  mission creatures (`alpha-creatures-behaviour.md` §9.2 step 6), which needs appearance flag
  0x1000. The placer never sets that flag (it sets +0x7a = 0x40 for modes 9 … 12), and the
  appearance generator writes no such constant (*believed*: checked by searching its listing
  for the immediate 0x1000).

---

## 3. Routine 0x5f77b0: the mana cube (client 0x5f77b0 … 0x5f7811, server 0x52b1c0)

### 3.1 What it makes

The item is a **mana cube**, type 25. It depends on no generator of `alpha-items-generator.md`.
Called with the world in ECX and three stack arguments (`RET 0xC`): the output record, then
twice Lz.level (read from Lz +0x24, 0x5bb31e). **Both level arguments and the world are
ignored.**

| Field | Offset | Value |
|---|---|---|
| type | +0x00 | **25** (0x19) |
| subtype | +0x01 | 0 |
| seed | +0x04 | **d % 200**, 0 … 199 |
| product | +0x08 | 0 |
| rarity | +0x0c | 0 |
| material | +0x0d | 0 |
| byte +0x0e | +0x0e | 0 |
| level | +0x10 | **1** (not the location's level) |
| spirits | +0x14 | 256 zero bytes |
| spirit count | +0x114 | 0 |

There is no colour field and no separate modifier field on an item; the seed is the only varying
value. Bytes +0x02, +0x03, +0x0f, +0x12, +0x13 are not written and are not copied, as for every
item (items spec §0). The name is "Mana Cube" (`alpha-item-names.md`, (25, 0)).

**What the seed means here.** For type 25, +0x04 is the **number of the cube**: the filter
0x43e550 (server 0x409660, identical) shows and offers the item to a player only when +0x04
equals v, with c the player's mana-cube counter (+0x1164), v = c for c ≤ 100, and v = 100 +
(c mod 100) for c > 100 (`alpha-world-items.md` §2 step 5, §3.3).

| d | +0x04 | Visible to counters c |
|---|---|---|
| 45 | 45 | 45 only |
| 12 345 | 145 | 145, 245, 345, … |
| 300 | 100 | 100, 200, 300, … |
| 32 767 | 167 | 167, 267, … |

So a player sees a given generated cube only at one counter value in a hundred at best, and a
cube numbered 0 … 99 only at that exact count. This is not a shared pick-up that everybody sees.

### 3.2 Who builds it, and how many exist

**Callers:** only the town builder, once for each of kinds 17 and 19: client 0x5bb32c and
0x5bb69c, server 0x4eea9c and 0x4eee0c. No other reference to the routine exists in the listing.

Kinds 17 and 19 are made only in **type-5 zones of variant 2** (structures §5.4 step 4; variant 2
means warmth > 0.8 and moisture < 0.2, locations spec §6.2), so a mana cube of this routine lies
only in those desert ruins. A zone holds at most two: one sphinx and one obelisk.

### 3.3 Where it lands

The steps at one plot, no draws except step 5:

1. Prefab origin (X0, Y0, Z0) := (cx − sx′/2, cy − sy′/2, hi), as in structures §5.5.5.
2. **Settle the origin** (0x5bb11b … 0x5bb15b, the second site 0x5bb487 …): while the block at
   (X0, Y0, Z0) is not solid, Z0 −= 1. Solid = type not 0 or 2 (0x43b480), implicit read rules
   of terrain §1.4, so water is not solid and the loop ends at the column's bottom at the latest.
3. The placer runs (model slot 2564 or 2565, type byte 6, mode 0, fill-down, rotation = face).
4. **Item height** (0x5bb1f0 … 0x5bb26d): at the column (X0, Y0) of this zone, z := its bottom +
   stored count. While the block at (X0, Y0, z) is not solid, z −= 1. Then z += 1. So z is one
   above the highest solid block of that column, sphinx voxels included.
5. Default world item (0x44a970), position, and then **the draw** (§3.1).
6. Position := (X0 + 0.5, Y0 + 0.5, z). X0, Y0 and z are converted to fx; the x and y offsets
   are +32768. (0x468fc0 subtracts the double × −65536, constant 0x702ae0, so it **adds** 0.5.)

The cube stands on the column (X0, Y0): the **lowest x, lowest y corner** of the footprint for
any rotation, not the middle of the statue. If that corner voxel of the model is empty, the
column may hold only the ground, and the cube is on the ground beside the statue.

**World item fields:**

| Field | Offset | Value |
|---|---|---|
| angle | +0x130 | **not written**: stale stack bytes (*as read*). The drawn pose ignores it |
| scale | +0x134 | 1/14, the default |
| origin tag | +0x138 | **0** (unlike other generated items) |
| landing timer, hide timer | +0x13c, +0x140 | 0, 0 |
| day | +0x144 | −1 (generated) |

AlphaVulkan should store angle 0. The stale value does reach saves and section 7, and 0x449ce0
compares it, so in the original a reload can find the list "different" and adopt the saved one.
That changes nothing visible.

No hop: *T* = 0. Drawn as `alpha-world-items.md` §2.1 type 25: hovering one model height over
the position, turning 20° per second (model slot 2566, `alpha-creatures-models.md`).

**The later item pass (§4.1) leaves it where it is:** the cell at z − 0.5 is the solid block found
in step 4.

### 3.4 Draws and order

**One draw**, in the routine, after the record's fields are cleared. The total at the plot is
that draw plus the face draw of structures §5.4, which came earlier. The zone clip of the placer
does not stop the item: a sphinx clipped away would still add the cube. *As read*, step 4 does
not test whether the column exists: a missing column would be a null read in the original, so an
origin outside the zone must not happen. Whether a large model can put the origin outside
depends on the model sizes (P5-3). There is no behaviour to match in that case; an engine should
guard it.

### 3.5 List, save, network

* **List:** appended to zone + 0x30 (0x44d590) in build order, before the vegetation items.
* **Regenerated** with the zone, like every generated item (`alpha-world-items.md` §4.3).
* **Saved:** an ordinary world item of 325 bytes in `zone<x>_<y>` (`alpha-saves.md` §5.2), with
  its +0x04, only when the zone's "items changed" flag (+0x75) is set. A freshly generated,
  untouched cube is not stored at all. Day −1 exempts it from the 3-day rule.
* **Sent:** as a section 7 record (`alpha-net-packets.md` §3.7): the whole item list goes out
  when the zone's list changes, or in answer to packet 11. There is no special packet.
* **Pick-up:** kind 5 of `alpha-world-items.md` §3.3 and `alpha-server-simulation.md` §8: the
  filter, then within 4 blocks. The item is removed from the zone for everybody, and a section 9
  gift gives the counter + 1. The zone is saved with the cube gone.

**Worked example.** A sphinx in zone with origin (5000, 7000, 35) before settling. Solid first at
z = 33, so Z0 = 33. After placement the column (5000, 7000) has bottom 20 and 20 stored blocks,
so z starts at 40; block 40 is above the column (not solid), block 39 is a sphinx voxel (solid);
z = 39 + 1 = 40. d = 12 345: seed 145. Position in fx: (5000·65536 + 32768, 7000·65536 + 32768,
40·65536) = (327 712 768, 458 784 768, 2 621 440). Scale 1/14, tag 0, day −1, level 1. A player
with 145 or 245 cubes sees it; a player with 3 cubes does not.

---

## 4. Other effects in the zone generator

Every "not specified", "left out", "not traced" and "follow-up" note in the worldgen specs,
`alpha-spawn-objects.md`, `alpha-location-creatures.md` and `alpha-materials-wildlife-routes.md`
was searched. So was every call made by 0x5e4850, the town builder, the dungeon builder, the
placer, 0x5dc6e0, the zone pass and the tree builder, against the addresses the specs name; the
draw sites of 0x5e4850 were matched against the specified ranges. Only the items below have an
effect on a zone or settle an open note. Every other outline row of overview §5 and caves §8 is
closed by a later spec.

### 4.1 **Not specified anywhere:** the item and census passes (0x5ed86d … 0x5edd56)

These run after the finishing pass and before zone + 0xa0 := spawn count. Overview §5 called them
"finalising object and creature lists", follow-up. **No draws**, but pass A moves items. Server:
the same code at 0x5216f5 and 0x5219ad.

**Pass A: settle items (0x5ed88c … 0x5eda48).** For each world item of zone + 0x30 in list order:

1. Add 1 to a count kept per key (type, subtype, material), bytes +0x00, +0x01, +0x0d.
2. If bit 0 of +0x138 is **set**, do nothing more. So structure and cave loot (tag 1) stays put.
3. Otherwise (tag 0 or 2): while the cell at (x, y, z − 0.5) of the item's position is not solid,
   z −= 1 block. Floor division per axis (0x42f860), this zone's columns only, solid =
   0x43b480. x and y do not change. A missing column reads solid, so nothing happens there.

The test uses z − 0.5, so an item resting on a block does not move, and fractional z is kept.
Water is not solid, so an item sinks through water to the bottom.

Example: an item at z = 30.0 with block 27 the highest solid one: cells 29 and 28 are not solid
(z becomes 28.0), the cell at 27.5 is solid, stop. Final z = 28.0.

**Pass B: abundant items (0x5eda5b … 0x5edb71).** Over the counts of pass A, ascending by
(type, subtype, material): if type is 1 or 11 and count ≥ 8, append a 0x140-byte marker record
to zone + 0x48 (0x5f4ba0), default-built by 0x5c39e0: +0x00 = **9**, the item bytes at +0x04 +
offset: type +0x04, subtype +0x05, material +0x11; position (+0x128) = (256zx + 128, 256zy + 128,
0) in fx.

**Pass C: abundant creatures (0x5edb83 … 0x5edd47).** For each spawn of zone + 0x18 in order: if
its active-window end (+0x44) is exactly 86 400 000, add 1 to a count per key (controller kind
+0x28, type +0x2c). The generator's constant write is the wildlife pass's (0x5eb845). Then
ascending by key: if the kind is 1, 5 or 6 and count ≥ 5, append a marker record: +0x00 = **10**,
creature type at +0x120, position as in pass B.

Both counters are local and thrown away. The list zone + 0x48 keeps dungeon markers first
(types 5 and 6, structures §1.10.6), then type 9, then type 10, each in key order.

**Reader of zone + 0x48: not found.** The listing shows only the appends, the zone constructor,
copy and destructor. Nothing was found that reads the records in a generated zone. It looks like
a census for a map or a hint feature that Alpha does not use. An engine should fill it if it
wants parity with a dump, and may leave it out otherwise: it changes neither blocks nor draws.

**Consequence for item heights:** vegetation and garden items (tag 2) and any tag-0 generated item
go through pass A, so `alpha-world-items.md` §4 positions are positions **before** this pass. For
an item on a column top nothing changes unless that top is not solid (water, for example).

### 4.2 Palm-leaf stamps (trees open question 1): closed

Structures §6.2 answers it. The stamp is the placer with T = 0x28, mode 0, fill-down off, no crop,
the zone given, no output list. Mode 0 reads no markers, so **no draws in the placer**, and every
non-empty voxel gets the default write: the model's colour with type byte 0x28 | 0x40 = **0x68**
(low five bits 8). The empty-voxel rule still writes air inside a column, and the skip rule keeps
existing structure blocks.

### 4.3 zone + 0x24 (`alpha-spawn-objects.md` SO-4, trees open question 4): closed

It is **not** a plant list. Inside the generator the only append found is in the town builder's
kind-4 landing code (0x5bab56): pointers to airship objects, each with its u64 id at offset 0. The
loop at 0x5ec5ee … 0x5ec64b gives each element with id 0 the id (zy·65536 + zx)·256 + index, no
draws. The kind-4 code is dead (structures §8 item 4), so in fresh worlds the list is empty and
the loop does nothing. The same code explains spawn +0x10e0, +0x10e4: the airship id of the rider,
computed from this list's size.

### 4.4 Dead code, no effect

* **Kind 4**, the landing place with airship and creature (structures §5.5.5): never built.
* **Type-9 guardian** (0x5ead31 … 0x5eae65, MW-6): type 9 never occurs in fresh worlds (LC-5), so
  it does not run.
* **E.kind 5** dungeon call (caves §6.1): region preparation never writes kind 5.

### 4.5 Closed notes with one line each

* Terrain open question 3, column +0x14: read by the open-plot and ruin creatures (structures
  §5.5.9, §5.5.10). Terrain spec line "town ground, not traced": type 11 is the road, read at
  structures §5.5.8.
* Dungeon, camps, boulders, town: specified to the draw in the structures spec; 0x5dc6e0 in
  `alpha-location-creatures.md`; the ground items and trees in the vegetation and trees specs. The
  older outlines that call them follow-ups are out of date.

---

## 5. Corrections to earlier specs

1. **Structures §5.5.5, kinds 17, 18, 19, 20.** The table gives position (…, hi). All four also
   lower Z0 while the block at the origin is not solid, like the well (0x5bb11b, 0x5bb487,
   0x5bb807, 0x5bb9f6). This changes where the model stands and the fill-down.
2. **Structures §5.5.5, kind 18.** The model is 2124 + (d & **3**) (0x5bb8ce), slots 2124 … 2127,
   `antique-building1` … `4`. Only kind 20 uses 2124 + (d & 1) (0x5bb6df). One draw each. The
   server does the same (0x4ef03e, 0x4eee4f).
3. **Structures §5.5.5, the ground item.** "One draw, d % 200" is the mana cube of §3; its position
   was not given.
4. **Structures §6.2.** Marker voxels are not tested against the zone (§2.1).
5. **`alpha-world-items.md` §1.** "+0x138 has no behaviour" is wrong: bit 0 is read at 0x5ed915
   (server 0x5216f5), and tag 1 exempts an item from the settling pass (§4.1). The table's "0
   dropped" should read "0 dropped or mana cube". The tag still has no effect in play.
6. **`alpha-spawn-objects.md` §3.1, §5, SO-4 and `alpha-worldgen-vegetation.md`.** See §4.3; and
   the "bookkeeping loops" after the finishing pass are §4.1.
7. **`alpha-extractor.md`.** A dump of world item z taken after 0x5e4850 shows the settled value.
   zone + 0x48 is the marker list of §4.1, with record types 5, 6, 9 and 10.

---

## 6. Open points

* **P5-1** Who reads zone + 0x48 in Alpha, if anyone (§4.1). None found by searching the listing
  for its offset and record stride; a read through an unusual path cannot be excluded.
* **P5-2** Which ruin models hold marker 15 or marker 11 voxels, and so how many creatures of
  type-5 zones get their level from §1. This is model data; count from the extracted `.cub` files
  (structures open question 6). The rule itself is settled.
* **P5-3** Whether the origin of kind 17 or 19 can lie outside the zone (§3.4). It depends on the
  sizes of models 2564 and 2565, which were not read.
* **P5-4** A check in a running game: a variant-2 ruin zone with a sphinx should list a type 25
  item in zone + 0x30, at the model's low corner, with +0x04 < 200 and level 1.

Nothing in this document needs a change to the draw counts of structures §7 beyond §5 item 2,
where kind 18 still costs one model draw.
