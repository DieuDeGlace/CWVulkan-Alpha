Status: FINAL / STABLE / HELD

# Alpha open points, batch 5: map-cell level, the mana cube of the ruins, and the end of the zone generator

Clean-room behavioural specification. It covers these points:

| Point | Companion spec (pointer only) | Section here |
|---|---|---|
| Routine 0x19a9b0: level of a creature placed at (x, y) | `alpha-worldgen-structures.md` §5.5.3, §6.2; `alpha-worldgen-caves.md` open question 4 | §1 |
| Which creatures use it, and who overwrites the result | structures §5.5.9, §5.5.10, §6.2; `alpha-spawn-objects.md` §3 | §2 |
| Routine 0x1f77b0: the ground item of the sphinx and the obelisk | structures §5.5.5 | §3 |
| Late passes of the zone generator with an effect on a zone | overview §5, `alpha-spawn-objects.md` §3.1, trees open questions 1 and 4 | §4 |

It also records native facts that differ from statements in companion specs (§5): kinds 17 to 20
lower their origin until it stands on something; kind 18 has four models; world item byte +0x138
is read; a pass after the finishing pass drops untagged items to the ground.

Address convention: every address is an RVA. Both images have image base 0x400000. "Client" means
`Cube.exe`, "server" means `Server.exe`; an address without an image name is a client RVA.
Written from the original client per-instruction listing and image bytes; the server was checked
where noted. *As read* marks behaviour that looks unintended but is what the original does.

Conventions: as in the structures spec §0. "fx" is 1/65536 block in an i64; a "draw" is one call
of the C runtime `rand()` (client import slot 0x2fc468) on the zone's stream; "spawn +x" is the
0x10f0-byte record of `alpha-spawn-objects.md`; "item record" is the 0x118-byte record of
`alpha-items-generator.md` §0; "world item" is the 0x148-byte record of `alpha-world-items.md` §1.
L is a location record (`alpha-worldgen-locations.md` §1.2), Lz the one of the zone's own cell,
f the normalised distance of `alpha-worldgen-regions.md` §7.

---

## 1. Routine 0x19a9b0: level of a point (client 0x19a9b0 … 0x19aa53, server 0xd2340)

**Inputs:** the world (this pointer) and a position (x, y) as two i64 in fx on the stack (16
bytes, callee-cleaned). No z, no creature type, no zone argument. **Output:** a 32-bit level.
**Draws: none.** Neither the routine nor anything it reaches (the float-to-i64 conversion
0x28d946, the 64-bit signed division 0x28d9e0, the cell lookup 0x87da0 and its region lookup
0x34a90, the distance 0x1f8e10, the noise 0xc0ef0, whose only calls are to the runtime cosine)
calls `rand()` or re-seeds. It writes no memory outside its own stack frame.

### 1.1 The rule

1. **Map cell.** cx = trunc(trunc(x ÷ 65536) ÷ 2048) and cy = trunc(trunc(y ÷ 2048) ÷ 65536), each
   division truncating towards zero. x uses integer division only (the 2048 step works on the low
   32 bits of the first quotient). y is converted to a double, multiplied by the constant at
   0x31db20 (0.00048828125 = 1/2048), truncated to i64, then divided by 65536. The double step is
   exact, so both axes equal trunc(v ÷ 2^27). Because the divisions truncate, a coordinate in
   (−2^27, 0) fx gives cell 0, not −1.
2. **Record.** cx and cy must each lie in 0 … 8191 (0x2000 exclusive). The region (cx >> 3,
   cy >> 3) is looked up in the world's 1024 × 1024 table of region pointers at world + 0xbc
   (index 1024·rx + ry); a null pointer means no record. Otherwise the record is number
   8·(cx mod 8) + (cy mod 8) of that region, at region + 0x14018 + 0x68 · number.
3. **Result.** Level 1, unless all of these hold: a record exists; its dword +0x18 (type) is not 0;
   f(record, x, y) ≤ 1.0. f is rounded to single precision and compared with the single-precision
   constant at 0x345dc0 (1.0); exactly 1.0 passes, NaN fails. Then the result is the record's
   dword +0x24 (level), unchanged and unclamped.

Consequences:

* Only the one record of the point's own cell counts. A location whose rim reaches into the next
  cell gives nothing there.
* Any non-zero type qualifies, not only towns and ruins.
* f is taken at the exact position passed. All three generator callers pass a whole-block corner
  (X·65536, Y·65536), not the creature's position with its +0.5 offsets (§2.1).
* f is never negative: in every branch of 0x1f8e10 it is a sum of squares divided by the squared
  radius. A record radius (float +0x10) below 0.001 (constant 0x2fd870) gives f = 0, so such a
  record passes.
* The level-1 default is what the trap damage caller receives outside a location
  (`alpha-open-points-1.md` §1.5 step 2).

### 1.2 Callers

These are the only references to the routine in either listing.

| Client site | Server site | Who | Position passed | Result goes to |
|---|---|---|---|---|
| 0x1b1c74 | 0xe53e4 | town builder (client 0x1af170), kind 6 garden, the scarecrow (spawn type 140), structures §5.5.3 | whole-block (x, y) of the middle column, as fx | spawn +0x34 (0x1b1c8a) |
| 0x1f392e | 0x12718e | prefab placer (client 0x1f0ce0), marker 11 (spawn type 141 or 142) | whole-block (X, Y) of the marker voxel, as fx | spawn +0x34 |
| 0x1f429b | 0x127afb | prefab placer, marker 15 (NPC) | whole-block (X, Y) of the marker voxel, as fx | spawn +0x34, may be overwritten, §2.2 |
| 0x20dc7f | 0x133a3f | world step routine (client 0x20c510), trap damage | trap position | `alpha-open-points-1.md` §1.5 |

### 1.3 Worked examples

Common to all: a marker voxel lands on block (X, Y) = (8 392 900, 8 392 300). The inputs are
X·65536 and Y·65536. cx = 8 392 900 ÷ 2048 = 4098 (4098.1…); cy = 8 392 300 ÷ 2048 = 4097
(4097.8…). Region (512, 512), slot (2, 1), record number 8·2 + 1 = 17, at region + 0x14018 +
17·0x68 = region + 0x14700.

| Record 17 | f at the point | Result | Why |
|---|---|---|---|
| type 0 | not computed | **1** | no location |
| type 1 (town), level 1 | 0.30 | **1** | the record's level, which is 1 |
| type 1 (town), level 1 | 1.40 | **1** | outside the rim; same value |
| type 5 (ruin), level 12 | 0.81 | **12** | f ≤ 1 |
| type 5 (ruin), level 12 | 1.00 exactly | **12** | the test is ≤ |
| type 5 (ruin), level 12 | 1.02 | **1** | outside the rim, though in the same cell |
| region not prepared | not computed | **1** | no record |

(f itself needs the noise of regions spec §7; the values above are illustrative.)

---

## 2. Which creatures, and what happens to the level

### 2.1 Prefab placer: exactly two markers

The placer 0x1f0ce0 takes (model, origin, rotation, type byte, mode, zone, fill-down flag, an
object pointer, output list). Markers are processed only when the mode is 1 … 7 or 9 … 15 (test
at 0x1f169f … 0x1f16e6); modes 0 and 8 read no markers.

Marker 11 and marker 15 of structures §6.2 call the routine right after the creature gets its
position and facing. There is no draw between those writes and the call.

* Marker 11 makes spawn type 141 when the mode is 10 and 142 otherwise (0x1f3850 … 0x1f3889).
* Marker 15 sets spawn +0x28 = 3 and draws spawn +0x2c = d & 3 before the position is set
  (0x1f41b3 … 0x1f41cc); no draw follows before the call.

**Zone handling, as read.** The placer rejects a prefab only when its footprint lies wholly
outside the zone (0x1f0daf … 0x1f0e13). Per voxel it reads the target block through this zone's
columns only (0x347a0 with the zone argument). A missing column reads as the static solid block
(type 1, structure bit 0x40 clear), which does not suppress marker processing. So a marker voxel
of a prefab that overlaps the zone border still makes its creature, appended to this zone's list
(zone + 0x18), and the level comes from the cell of the marker's own block, which in a corner case
is a neighbouring cell. What does suppress a voxel (and its marker) is an existing target block
that has structure bit 0x40 set and is solid (0x1f1604 … 0x1f1699).

### 2.2 Overwrites inside the placer

For marker 15 the mode switch (jump table at 0x1f4b20, indexed by mode − 6, entries for modes
6 … 15) runs after the call. Modes outside 6 … 15 skip the switch. Mode 8 never reaches it
(no marker processing).

| Placer mode | Spawn byte +0x30 | Final +0x34 |
|---|---|---|
| 1 … 5 | unchanged (house NPC) | routine result |
| 6 | 0x83 (identifier) | routine result |
| 7 | 0x89 (tower guard) | **20** (0x1f42dd) |
| 9, 10, 11, 12 | 1, 2, 4, 3 (class trainers) | **20** (0x1f430e, 0x1f4425, 0x1f453f, 0x1f4641); each then copies the 16-bit value of +0x34 into its equipment item levels |
| 13, 14, 15 | 0x85, 0x86, 0x87 (smith, carpenter, weaver) | routine result |

Marker 11 has no switch: types 141 and 142 always keep the routine's result.

### 2.3 Town zones

If a town record's level is 1 (as `alpha-worldgen-locations.md` §4.1 step 6 states), the routine
returns 1 in a town cell on both branches, so scarecrows, marker-11 creatures, crafters and
marker-15 NPCs in towns are level 1, while guards and class trainers are level 20. Whether any
later preparation phase changes a town record's level was not re-established here.

### 2.4 The town builder's other creatures do not use it

The town builder 0x1af170 has exactly one call of the routine (the garden scarecrow). These are
all its writes of a dword at offset +0x34:

| Site | Object | +0x34 |
|---|---|---|
| 0x1b1c8a | garden scarecrow | routine result |
| 0x1ba4f2 | animals on open plots (§5.5.10) | lo + d % (hi − lo + 1), one draw; (lo, hi) from 0x44020 called with the spawn type |
| 0x1ba9ce | airship object of the kind-4 code (not a spawn) | the object's own +0x50 |
| 0x1baa74 | kind-4 rider spawn | Lz.level (Lz +0x24) |
| 0x1bcc77, 0x1bce3f, 0x1bd01b, 0x1bd1f7, 0x1bd3d3, 0x1bd64a, 0x1be638, 0x1be9b4, 0x1bed70, 0x1bf0ae | inn and shop keepers, residents, ruin creatures (§5.5.9) | Lz.level (Lz +0x24) |

So an innkeeper or shopkeeper of §5.5.9 takes Lz.level with no f test; only marker creatures of a
placed model get the routine's result. Outside the rim of a ruin (f > 1) a §5.5.9 creature keeps
Lz.level while a marker creature gets 1.

The eight palm-leaf stamps (§4.2) call the placer in mode 0 and so cannot reach the routine.

### 2.5 After the placer: is the level changed later?

**Not by the finishing pass.** In 0x1ed2c6 … 0x1ed865 spawn +0x34 is read once (0x1ed38b, 16-bit)
and never written. The equipment generator 0x1c7730 only reads +0x34, as a 16-bit value. The
appearance generator 0x3f7c0 writes offset +0x34 only of the appearance record it is given
(spawn + 0x74), which is a different field.

**Other writers after creation:**

* The zone pass 0x1d9460, called right after zone + 0xa0 is set (0x1edd5e … 0x1edd64), writes
  +0x34 only on spawns it allocates itself (0x1d97d8).
* None of the placer's writes to the appearance flag word (spawn +0x7a) contains bit 0x1000: modes
  9 … 12 store 0x40 there. None of the 29 writes of the flag word in the appearance generator
  contains bit 0x1000 either (ORed values 0x2, 0x10, 0x20, 0x28, 0x29, 0x2a, 0x2c, 0x30, 0x31,
  0x33, 0x131, 0x402, 0x420, 0x428, 0x429). The activation rule that replaces the level for
  creatures with that flag is in `alpha-creatures-behaviour.md` §9.2 step 6.

---

## 3. Routine 0x1f77b0: the mana cube (client 0x1f77b0 … 0x1f7811, server 0x12b1c0)

### 3.1 What it makes

The item is a mana cube, type 25. Called with the world as this pointer and three stack
arguments (12 bytes, callee-cleaned): the output item record, then Lz.level twice (read from
Lz +0x24 at 0x1bb31e). Only the output pointer is used; both level arguments and the world are
ignored. It returns the output pointer.

| Field | Offset | Value |
|---|---|---|
| type | +0x00 (byte) | **25** (0x19), written after the clear |
| subtype | +0x01 (byte) | 0 |
| seed | +0x04 (dword) | **d % 200**, 0 … 199 |
| dword +0x08 | +0x08 | 0 |
| bytes +0x0c, +0x0d (rarity, material) | +0x0c (one 16-bit write) | 0 |
| byte +0x0e | +0x0e | 0 |
| level | +0x10 (16-bit) | **1** (not the location's level) |
| spirit area | +0x14 | 256 zero bytes |
| spirit count | +0x114 (dword) | 0 |

The seed is the only varying value. Bytes +0x02, +0x03, +0x0f, +0x12, +0x13 are not written, and
the item-record copy 0x2c5e0 used to put it into the world item does not copy them either. The
name is "Mana Cube" (`alpha-item-names.md`, (25, 0)).

**What the seed means here.** For type 25, +0x04 is the number of the cube. The filter 0x3e550
(server 0x9660, identical) accepts any item whose type is not 25; for type 25 it accepts only
when +0x04 equals v, where c is the player's mana-cube counter (dword +0x1164), v = c for
c ≤ 100, and v = 100 + (c mod 100) for c > 100.

| d | +0x04 | Accepted for counters c |
|---|---|---|
| 45 | 45 | 45 only |
| 12 345 | 145 | 145, 245, 345, … |
| 300 | 100 | 100, 200, 300, … |
| 32 767 | 167 | 167, 267, … |

A cube numbered 0 … 99 is accepted only at that exact count; a cube numbered 100 … 199 at one
counter value in each further hundred.

### 3.2 Who builds it

**Callers:** only the town builder, once in the kind-17 branch (sphinx, model slot 2564) and once
in the kind-19 branch (obelisk, model slot 2565): client 0x1bb32c and 0x1bb69c, server 0xeea9c
and 0xeee0c. No other reference exists in either listing.

### 3.3 Where it lands

The steps at one plot (kind 17; kind 19 is the same with slot 2565, settle loop at 0x1bb487 and
item call at 0x1bb69c). No draws except step 5:

1. Prefab origin (X0, Y0, Z0) := (plot x − sx′/2, plot y − sy′/2, plot height), where sx′, sy′
   are the model's footprint sizes, swapped when the face value is odd; each halving truncates
   towards zero (structures §5.5.5).
2. **Settle the origin** (0x1bb11b … 0x1bb15b): while the block at (X0, Y0, Z0) is not solid,
   Z0 −= 1. Blocks are read through this zone's columns (0x2f7e0). Solid means the low five bits
   of the block's type byte (+3) are neither 0 nor 2 (0x3b480). Below a column's bottom, and for a
   missing column, the read gives a static block of type 1 (solid), so the loop ends at the
   column bottom at the latest; above the top it gives air (z > 0) or water (z ≤ 0), both not
   solid.
3. The placer runs: model slot 2564, origin as settled, rotation = face, type byte 6, mode 0,
   this zone, fill-down flag 1, no output list (0x1bb196 … 0x1bb1ca).
4. **Item height** (0x1bb1cf … 0x1bb26d): take this zone's column at (X0, Y0) (0x347a0) and set
   z := its bottom (+0x10) + its stored block count (+0x1c). While the block at (X0, Y0, z) is
   not solid, z −= 1. Then z += 1. So z is one above the highest solid block of that column,
   model voxels included.
5. Default world item (0x4a970), its position, and then **the draw** (§3.1). The item record is
   copied into the world item (0x2c5e0) and the world item appended to zone + 0x30 (0x4d590).
6. Position := (X0 + 0.5, Y0 + 0.5, z) in fx. X0 and Y0 are converted to fx and the helper 0x68fc0
   subtracts trunc(0.5 × −65536) (double constants 0x345de8 = 0.5 and 0x302ae0 = −65536), i.e.
   adds 32768. z is converted to fx without offset. The position is stored at world item +0x118.

The cube stands on the column (X0, Y0), which is the low-x, low-y corner of the placed footprint,
not the middle of the statue. If the model has no voxel in that column, the column holds only
the ground and the cube rests on the ground beside the statue.

*As read*, step 4 does not test whether the column exists: for an (X0, Y0) outside this zone the
column lookup returns null and the bottom read dereferences it.

**World item fields:**

| Field | Offset | Value |
|---|---|---|
| angle | +0x130 | **not written** by the default constructor or the builder: whatever the stack slot held (*as read*) |
| scale | +0x134 | 1/14 (0x3d924925), the default |
| origin tag | +0x138 | 0, the default |
| timers | +0x13c, +0x140 | 0, 0 |
| day | +0x144 | −1, the default |

The world-item list comparison 0x49ce0 compares, per element, the item record (via 0x2f4a0), the
position, angle +0x130, scale, tag and both timers (not the day), so the unwritten angle takes
part in that comparison.

The later item pass (§4.1) leaves the cube where it is: the cell at z − 0.5 is the solid block
found in step 4, unless a later step of the generator changed that column.

**Worked example.** A sphinx in a zone with origin (5000, 7000, 35) before settling; blocks 35 and
34 are not solid, 33 is, so Z0 = 33. After placement the column (5000, 7000) has bottom 20 and 20
stored blocks, so z starts at 40; block 40 is above the column (air), block 39 is a sphinx voxel
(solid); z = 39 + 1 = 40. d = 12 345 gives seed 145. Position in fx: (5000·65536 + 32768,
7000·65536 + 32768, 40·65536) = (327 712 768, 458 784 768, 2 621 440). Scale 1/14, tag 0,
day −1, level 1. The filter accepts it for a player with 145 or 245 cubes, not for one with 3.

### 3.4 Draws and order

The kind-17 and kind-19 branches contain no draw of their own between the kind test and the item
routine; the only draw is the one in the routine, after the record is cleared. The placer in
mode 0 makes no draw (all of its `rand()` sites lie in the marker code). The result of the placer
is not tested, so the cube is added even if the placer wrote nothing.

### 3.5 Lists

Appended to zone + 0x30 (0x4d590) in build order. Saving, regeneration, network sending and
pick-up follow the general world-item rules of `alpha-world-items.md`, `alpha-saves.md`,
`alpha-net-packets.md` and `alpha-server-simulation.md`; those were not re-established here.

---

## 4. Late passes of the zone generator (0x1e4850)

### 4.1 The item and census passes (0x1ed86d … 0x1edd56)

These run after the finishing pass and before zone + 0xa0 := spawn count of zone + 0x18
(0x1edd53 … 0x1edd5e), which is followed by the zone pass 0x1d9460. **No draws.** Pass A moves
items. Server counterparts: the tag test at server 0x1216f5 and the pass-C compare at server
0x1219ad.

**Pass A: settle items (0x1ed88c … 0x1eda48).** For each world item of zone + 0x30 in list order:

1. Add 1 to a count kept in an ordered map keyed by (type +0x00, subtype +0x01, material +0x0d).
2. If bit 0 of byte +0x138 is set (0x1ed915), do nothing more.
3. Otherwise, while the cell containing (x, y, z − 0.5) of the item's position is not solid, z −= 1
   block (65536 fx). x and y do not change; fractional z is kept. Cells are read with 0x2f860
   through this zone's columns and tested with 0x3b480. 0x2f860 converts each fx coordinate to a
   block number by truncating division by 65536, after first subtracting 65536 from negative
   values; this equals floor division except that an exact negative multiple of 65536 maps one
   block lower. A missing column reads solid, so nothing moves there. Water is not solid, so an
   item sinks through water to the bottom.

The test uses z − 0.5, so an item resting on a block does not move. Example: an item at z = 30.0
with block 27 the highest solid one: cells 29 and 28 are not solid (z becomes 28.0), the cell at
27.5 is solid, stop. Final z = 28.0.

**Pass B: abundant items (0x1eda5b … 0x1edb71).** Over the counts of pass A, in ascending key
order: if type is 1 or 11 and count ≥ 8, a 0x140-byte marker record is default-built (0x1c39e0),
given +0x00 = **9**, byte +0x04 = type, byte +0x05 = subtype, byte +0x11 = material, position
(+0x128, three i64) = (256·zx + 128, 256·zy + 128, 0) blocks in fx, and appended to zone + 0x48
(0x1f4ba0).

**Pass C: abundant creatures (0x1edb83 … 0x1edd47).** For each spawn of zone + 0x18 in order: if
its dword +0x44 is exactly 86 400 000 (0x5265c00), add 1 to a count keyed by (dword +0x28,
dword +0x2c). Inside the zone generator that constant is written to +0x44 only at 0x1eb845. Then,
in ascending key order: if the first key is 1, 5 or 6 and count ≥ 5, a marker record is
default-built with +0x00 = **10**, dword +0x120 = the second key (spawn type), position as in
pass B, and appended to zone + 0x48.

Both maps are local to the generator and discarded. Within zone + 0x48, the records of pass B
follow whatever earlier steps appended, and those of pass C follow pass B, each in key order.
No reader of zone + 0x48 was established (P5-1).

Because untagged items go through pass A, any position a companion spec gives for an untagged
generated item is its position before this pass. For an item on a column top nothing changes
unless that top is not solid.

### 4.2 Palm-leaf stamps (trees open question 1)

The tree builder 0x1df960 stamps palm leaves with eight placer calls (0x1e1a8e, 0x1e1b4b,
0x1e1c13, 0x1e1cd8, 0x1e1dcd, 0x1e1e84, 0x1e1f39, 0x1e1ffb), each with type byte 0x28, mode 0,
fill-down flag 0, a zone argument, a non-null object-pointer argument, no output list, and a
constant rotation 0, 1, 2, 3, 0, 1, 2, 3 in that order. Mode 0 reads no markers, so the placer makes no draws for them. Each non-empty voxel
that is written gets the model's colour with type byte 0x28 | 0x40 = **0x68** (low five bits 8)
(0x1f4a2d … 0x1f4a35), and an existing solid block with structure bit 0x40 is kept (§2.1).

### 4.3 zone + 0x24 (`alpha-spawn-objects.md` SO-4, trees open question 4)

It is not a plant list. Within the zone generator, the town builder and the placer, the only
append to it is in the town builder's kind-4 landing code (0x1bab56 … 0x1bab59): a pointer to an
airship object whose u64 id is at offset 0. Before that append, the rider spawn gets +0x10e0,
+0x10e4 = the u64 (zy·65536 + zx)·256 + n, n being the list's size before the append
(0x1c0120). The loop at 0x1ec5ee … 0x1ec64b gives each element whose id is 0 the id
(zy·65536 + zx)·256 + index, without draws.

### 4.4 Other branches

* **Kind 4**, the landing place with airship and rider (§4.3): its code is present (kind test at
  0x1ba636); structures §8 item 4 describes it as never built.
* The type-9 guardian (0x1ead31 … 0x1eae65) and the dungeon call for E.kind 5 (caves §6.1) are
  described by companion specs as unreachable in fresh worlds.

---

## 5. Native facts that differ from companion specs

1. **Kinds 17, 18, 19, 20** (structures §5.5.5). After the origin is computed, each lowers Z0
   while the block at the origin is not solid (settle loops starting 0x1bb11b, 0x1bb487, 0x1bb807,
   0x1bb9f6), which changes where the model stands and its fill-down.
2. **Kind 18.** The model is slot 2124 + (d & 3) (0x1bb8ce), slots 2124 … 2127. Kind 20 uses
   2124 + (d & 1) (0x1bb6df). One draw each. Server: 0xef03e (kind 18) and 0xeee4f (kind 20).
3. **The kind-17/19 ground item** is the mana cube of §3, with the position of §3.3.
4. **Marker voxels** are not tested against the zone (§2.1).
5. **World item +0x138.** Bit 0 is read at 0x1ed915 (server 0x1216f5) and exempts an item from
   the settling pass (§4.1). The mana cube has tag 0.
6. **zone + 0x24** is the airship list of §4.3, and the loops after the finishing pass are those
   of §4.1. zone + 0x48 holds the marker records of §4.1 (types 9 and 10) after any appended
   earlier.

Kind 18 still costs exactly one model draw, so the draw count of structures §7 changes only in
the model range, not in the number of draws.

---

## 6. Open points

* **P5-1** Who reads zone + 0x48 in Alpha, if anyone (§4.1). No reader was established.
* **P5-2** Which ruin models contain marker-15 or marker-11 voxels, and so how many creatures of
  type-5 zones get their level from §1. This is model data (structures open question 6); the rule
  itself is settled.
* **P5-3** Whether the origin of kind 17 or 19 can lie outside the zone (§3.3). It depends on the
  sizes of models 2564 and 2565, which were not read.

---

## Re-certification (2026-11-15)

Re-checked against the original `Cube.exe` and `Server.exe` instruction listings and image bytes
(constants, import table, jump table). No other project material was used.

**Confirmed:** inputs, callee-cleaned argument sizes and absence of draws of the level routine;
the cell rule, range 0 … 8191, region table, record base 0x14018 and stride 0x68, index
8·(cx mod 8) + (cy mod 8); the type and f tests, constants 1/2048 and 1.0, single-precision
compare, NaN to level 1, radius threshold 0.001, f ≥ 0; the four callers in both images; the
worked examples of §1.3; marker 11 and 15 call sites and the absence of a draw before the call;
the mode switch table, modes 6 … 15 and their byte +0x30 values, the level-20 writes, +0x7a = 0x40
for modes 9 … 12 and the 16-bit equipment level copy; the list of +0x34 writes in the town
builder and the draw formula of the open-plot animals; one read and no write of +0x34 in the
finishing pass; the zone pass write on its own spawns; the mana cube record fields, the draw
d % 200 and the ignored arguments; the filter rule in both images and its table; the two callers
in both images; settle loop, solid test, column-bottom static block, item-height rule, fx
conversion with +32768, default world-item fields, unwritten angle and the list comparison of
+0x130; the worked example of §3.3; passes A, B and C (keys, thresholds, record fields, offsets
+0x04, +0x05, +0x11, +0x120, +0x128, size 0x140, centre 256·z + 128, constant 86 400 000 and its
single write at 0x1eb845); the zone + 0x24 id loop and id formula; kind 18 and 20 model draws in
both images.

**Corrected (old → new):**

1. Addresses were virtual addresses → all are now RVAs (image base 0x400000 subtracted throughout,
   including data constants 0x31db20, 0x345dc0, 0x302ae0, 0x345de8).
2. Cell rounding: "a port can use floor(block ÷ 2048)" → truncation towards zero; coordinates in
   (−1 cell, 0) give cell 0.
3. Settling-pass cell conversion: "floor division per axis" → floor except that exact negative
   multiples of 65536 map one block lower.
4. Marker 11: "kinds 141 and 142" without rule → 141 when the placer mode is 10, 142 otherwise.
5. Placer zone handling: "no per-voxel zone test" → per-voxel reads are zone-restricted, a missing
   column does not suppress markers, but an existing solid structure block does suppress the voxel
   and its marker.
6. Appearance generator and flag 0x1000: "believed" → confirmed by enumerating all its flag-word
   writes.
7. Mana cube record: rarity and material are one 16-bit write and the level a 16-bit write
   (formerly listed as separate byte/dword fields without width).
8. Town-builder +0x34 table: implicit range "0x5bcc77 … 0x5bd64a" → every site listed;
   the kind-4 object at 0x1ba9ce takes its own +0x50.
9. zone + 0x24 append site: 0x5bab56 → the address pair 0x1bab56 … 0x1bab59 (address set-up and
   call); the rider id formula is now stated.
10. Mana cube origin tag "0 (unlike other generated items)" → "0, the default"; the comparison
    claim about other items was not checked and is dropped.

No correction reverses a previously stated behaviour. Items 2, 3 and 5 affect world generation
(cell choice for negative coordinates, settling of items at negative z, marker suppression by
existing structure blocks); none affects random-draw order or count.

**Removed:** the advice to a replacement on the angle field and the claim about its effect on
reloads; the advice on filtering markers, on guarding a missing column and on filling zone + 0x48
for dump parity; the statement about an extractor dump; the runtime check P5-4; the procedure of
counting markers from extracted files (P5-2 now states only the question); the description of the
search procedure in the former §4 introduction; statements that §4 closes outlines of other
specs; the "believed" search note of §2.5.

**Unresolved (not re-established, kept only as pointers or dropped):**

* That kinds 17 and 19 occur only in type-5 zones of variant 2 and at most once each per zone.
* That a town record's level stays 1 through region preparation (§2.3).
* That generated records of non-zero type never have a radius below 0.001.
* That the level steers the equipment re-roll draws (≥ 3 rule of `alpha-spawn-objects.md` §4.3).
* The activation rule for flag 0x1000 (pointer only).
* That other generated items carry a non-zero origin tag.
* That the drawn pose of a mana cube ignores +0x130, its hover height, turn rate and model slot
  2566, and whether the unwritten angle reaches saves and network records.
* Save size, save condition, regeneration, network section and pick-up rules of §3.5.
* That the cube is listed before the vegetation items.
* Model names of slots 2124 … 2127.
* The meaning of the placer's object-pointer argument (the former "no crop" for palm stamps) and
  the empty-voxel write rule.
* That home building and dungeon arches call the placer without marker processing.
* That kind 4, the type-9 guardian and the E.kind 5 dungeon call are unreachable in fresh worlds.
* That earlier entries of zone + 0x48 are dungeon markers of types 5 and 6.
* Any reader of zone + 0x48 (P5-1).
