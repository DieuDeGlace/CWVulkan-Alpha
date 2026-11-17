Status: FINAL / STABLE / HELD

# Alpha ruling: zone fields +0x04, +0x18, +0x24 and the ambience distance field +0x6c

Scope: two cross-spec conflicts recorded in `alpha-extractor.md` ("Re-certification
(2026-11-16)", cross-spec conflicts). The first is the object and value behind the "+0x6c"
distance gate of the ambient sounds (`alpha-audio.md` §3, AU-3). The second is which zone
containers hold plant sprites, static objects and Spawn objects (terrain, vegetation and trees
specs). Image: client `Cube.exe` (SHA-256
84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF), 32-bit, image base 0x400000.
Addresses below are virtual addresses in that image. All findings come from static reading of
the original image (disassembly, decompile and raw bytes). Infinite-Cubes/alpha was not used.

---

## Ruling 1: the ambience distance field is a render chunk's squared camera distance

### 1.1 Which object

The ambience loop of the render routine 0x4ac260 does not read `cube::Zone` at all. The slots
it visits (array pointer G+0x2e0, 0x268 bytes each, grid width G+0x2dc) are **`cube::Chunk`**
render records. The start-up routine 0x459c40 allocates width² of them (0x461eeb … 0x461f54),
and each one is built by constructor 0x4596e0, which stores the `cube::Chunk` vtable. A chunk
covers a **32 × 32-block column**, not a 256 × 256-block zone:

* the chunk builder 0x49d910 keeps a zone sprite record only when trunc(trunc(x · 0.03125) /
  65536) and the same for y equal the chunk's indices (0x4a0d05 … 0x4a0db4; 0.03125 at
  0x702a60). It stores those indices at chunk +0x18 and +0x1c (0x4a1380, 0x4a138a);
* the ambience grid range 0 ≤ x, y < 0x80000 is 2^24 blocks ÷ 32.

So both contracts are right about their own object. In `cube::Zone`, +0x6c is the end pointer
of the +0x68 edit vector: the save test 0x5a5050 compares +0x68 with +0x6c, and the serializer
0x44e340 takes (+0x6c − +0x68) / 20 as the edit count. That part of `alpha-extractor.md` wins
for the zone. `alpha-audio.md` is wrong to call the slot a "zone" and its value a "distance".

### 1.2 `cube::Chunk` fields used here

| Off | Meaning (evidence) |
|---|---|
| +0x08 / +0x0c | live geometry list / its count; a count of 0 means "not populated" (0x4ac665) |
| +0x10 / +0x14 | freshly built geometry list / its count (written by the builder, moved to +0x08 at publication) |
| +0x18, +0x1c | chunk indices x, y in 32-block units (0x4a1380, 0x4a138a); the constructor sets both to −1 |
| +0x20 … +0x4f | two i64 position triplets, the chunk bounds (0x4a115d … 0x4a11aa); the view test 0x47f3c0 takes +0x20 and +0x38 |
| +0x50 … +0x67 | i64 triplet: (first triplet + second triplet) × 0.5, scaled in fixed point with truncation toward zero (0x4a0f3c … 0x4a1047; 32768 at 0x702a88 used as a 1/65536-scale factor) — the bounds midpoint |
| +0x68 | f32: 1.5 × √(squared midpoint-to-bound offset, in blocks²) — a bounding radius (0x4a1136 … 0x4a1158; 1.5 at 0x6fcde0) |
| +0x6c | f32: **squared** camera distance in blocks² (§1.3); not set by the constructor |
| +0x70, +0x74 | a key and a "build in progress" byte; the builder clears +0x74 when it finishes (0x4a1394) |
| +0x240 | published record list, the one the render routine walks (list type of 0x688b20, as zone +0x04) |
| +0x248 | record list built by the chunk builder, waiting to be published |
| +0x250 | per-chunk lock |

### 1.3 What +0x6c holds (0x4ac6a8 … 0x4ac737)

For a visited slot that is populated, whose indices equal the current grid position (x, y) and
which passes the view test 0x47f3c0 (+0x20, +0x38, radius G+0x1d4 + 22.4 + 32), the routine:

1. forms d = chunk +0x50 − G+0x140 (the camera/listener position **L**) per axis, using 64-bit
   integer subtraction (0x42c7a0);
2. forms S = Σ over the three axes of (d_i · d_i, low 64 bits) / 65536, with 64-bit signed
   division truncating toward zero and the sum taken modulo 2^64 (0x43ac20). This is a fixed-point
   dot product of d with itself; **no square root is taken**;
3. converts S to binary32 through the x87 unit (exact 64-bit load, then a store to single
   precision that rounds to nearest), multiplies by 2^−16 (0x6fcd94 = 0x37800000) in binary32
   and stores the result at chunk +0x6c.

So +0x6c is |chunk midpoint − L|² in blocks². Before step 1, the chunk pointer is appended to a
local visible-chunk vector (0x4ac6de calls the vector append 0x66add0). That has no effect on
+0x6c.

The constructor 0x4596e0 never initialises +0x6c, and 0x4ac737 is the only writer found. The
chunk array lives as long as the game object and is reused for new chunk positions, and the
builder 0x49d910 does not write +0x6c. So a slot whose refresh is skipped keeps the value from
its last refresh, which may have been made for a different chunk position. Before its first
refresh the value is indeterminate.

### 1.4 The gate and the order of the walk

Per record of chunk +0x240, after the record's optional mode-bit-0 branch (0x4ac761 …
0x4acaab), which comes **before** the gate and is not governed by it, the routine compares
4096.0 (0x702b80 = 0x45800000) with +0x6c (0x4acab3 … 0x4acabf). The record goes on to the
time-of-day and kind tests of `alpha-audio.md` §3 only when **+0x6c < 4096.0**, strictly. An
unordered comparison (NaN) skips the record. Since +0x6c is a squared distance, the ambience
radius is **64 blocks** from the chunk's bounds midpoint to the camera, in three dimensions.
It is not 4096 blocks.

The +0x240 walk runs for every in-range slot once its lock is held, whether or not that slot
passed the populated, index and view tests (each failing test jumps to the walk at 0x4ac73c).
Slots outside 0 ≤ x, y < 0x80000 are skipped without lock or walk (0x4ac538 … 0x4ac55a). Slot
index = (y rem w) · w + (x rem w), with w = G+0x2dc and C remainders (0x4ac560 … 0x4ac580).
The loops run x outer from G+0x2ac and y inner from G+0x2b0, each over w positions.

Publication, as observed (0x4ac475 … 0x4ac652). Before the grid walk, the routine counts the
slots whose +0x70 is non-null and whose +0x74 is set, keyed by the +0x70 value. For each
visited slot whose key has count 0, it takes the slot lock. Then, if the built-geometry list
(+0x10) is non-empty, it releases the live geometry, moves +0x10 into +0x08 and replaces +0x240
with a copy of +0x248 (0x4570a0). The records heard are therefore those of the last published
build of the chunk.

### 1.5 Which contract wins

* **Wins:** `alpha-extractor.md` §B2.2 / correction 6 for `cube::Zone` +0x6c (end pointer of the
  edit vector; evidence 0x5a5050, 0x44e340, constructor 0x6240f0).
* **Wrong in `alpha-audio.md` §3 / AU-3 / change-log item 11:** "zone grid", "zone point
  +0x50", "zone coordinates (+0x18, +0x1c)" and "camera-to-zone distance in blocks". The correct
  reading: slots are `cube::Chunk` records (32 × 32-block columns), +0x50 is the chunk bounds
  midpoint, +0x18/+0x1c are chunk indices, and +0x6c is the squared camera distance in blocks²,
  so the gate +0x6c < 4096 is a 64-block radius. The rest of §3 stands: lock, stale value on
  skipped refresh, tick test, draws, kinds and constants. The records are copies, made by the
  chunk builder, of zone +0x04 sprite records that lie in the chunk (§2.2).
* **Behavioural consequence:** an engine that gates ambience at a 4096-block distance, or keys it
  per 256-block zone, plays birds, owls and crickets from far too many records. Native ambience
  comes only from records in chunks whose bounds midpoint lies within 64 blocks of the camera
  (by the stale-value rule, as of each chunk's last visible refresh).

---

## Ruling 2: zone containers +0x04, +0x0c, +0x18, +0x24

### 2.1 Container types (constructor 0x6240f0, destructor 0x624390, serializer 0x44e340)

| Off | Container and element | Appended by | Read / freed by |
|---|---|---|---|
| +0x04 | list (sentinel pointer, count +0x08); node 0x48 bytes, two links then a 0x40-byte sprite element (kind +0x00, i64 position +0x08/+0x10/+0x18, f32 +0x20, +0x24, +0x28, tint +0x2c … +0x34, mode +0x38) | 0x4862e0 (node insert 0x5c03f0, limit 0x38e38e2): generator plant pass 0x5ec0dd, 0x5ec2ec, 0x5ec586, 0x5ecb46; statics-to-sprites pass 0x5edf83 … 0x5ee048; tree builder through 0x5c03f0 (`alpha-worldgen-trees.md` §5) | chunk builder 0x49d910 (0x4a0ce4 … 0x4a0e9a), §2.2; destructor clears the list |
| +0x0c | vector of 0x188-byte static records, inline | 0x5a0e00 (generator 0x5e9e15, 0x5ea9b6, 0x5ebefb) | serializer (count = size / 0x188); `alpha-interaction.md` §4.1 |
| +0x18 | vector of `cube::Spawn` pointers (0x10f0-byte objects, constructor 0x5adb20) | vector append 0x66add0: generator 0x5eae65, 0x5eb4d7, 0x5eb6e7, 0x5eb85e, 0x5ebb6d; location builder 0x5af170 at 0x5bab4a | serializer writes each Spawn's +0x38 and +0x3c; destructor calls each element's virtual deleting destructor |
| +0x24 | vector of pointers to 0xa0-byte objects (constructor 0x466510: an ordered container at +0x78, +0x80 … +0x9c zeroed; the builder stores 90 · n as f32 at +0x50) | only 0x5af170 at 0x5bab59, directly after pushing a Spawn to +0x18 | destructor tears down each element's +0x78 container (0x67eb10) and frees it; **not** serialized; no reader identified |

At 0x5bab10 … 0x5bab40 the paired Spawn receives at +0x10e0 a 64-bit value built from two
32-bit inputs of the location builder and the element count of +0x24 **before** the new element
is pushed (0x487f50 gives the count, 0x5c0120 combines them as (b · 65536 + a) · 256 + count with
64-bit arithmetic). It also receives a pointer at +0x109c. The +0x24 element type and the role
of that value are not identified further.

### 2.2 Use of sprite records outside the zone

Under the world zone lock, the chunk builder 0x49d910 looks up the zone (0x434d10) and walks
zone +0x04 from head to tail. For each record in this chunk (§1.1 index test) it:

1. reads the block at (trunc(x / 65536), trunc(y / 65536), trunc(z / 65536)) through 0x42f7e0;
2. computes a value from the block's type bits (byte 3 & 0x1f): 255 for type 13; for type 0 or 2,
   byte 0 of the block raised to at least 5; for any other type, 0;
3. writes that value as f32 into the **zone's own** record at element +0x28 (0x4a0e49);
4. appends a copy of the element to the chunk's pending list, which becomes chunk +0x248
   (0x4a11c2) and later +0x240 (§1.4).

Order is kept (tail appends). The render routine 0x4ac260 uses only the chunk copies: the
mode-bit-0 branch, and the ambience of `alpha-audio.md` §3 as amended by Ruling 1.

### 2.3 Which contract wins

* **Plant sprites are at zone +0x04.** This agrees with `alpha-worldgen-vegetation.md` §1.3/§7
  (as re-certified 2026-11-14), `alpha-worldgen-trees.md` §5 and `alpha-extractor.md` §B2.2/§B2.6.
  Any statement that puts plant sprites at zone +0x24 is wrong. X-3 of `alpha-extractor.md` and
  item 4 of the trees spec's unresolved list are settled: the plant pass and the tree builder
  append to the same list, zone +0x04.
* **Static objects are at zone +0x0c**, not +0x18 (vegetation spec §1.3, extractor §B2.2 win).
* **Zone +0x18 is the Spawn-pointer vector** (extractor wins). The ravine-spawn "submit" of
  `alpha-worldgen-terrain.md` §5.5 step 10 cases 1 and 2 (0x66add0) appends the new 0x10f0-byte
  object to zone +0x18.
* **Zone +0x24** holds neither sprites nor statics. It is a vector of pointers to 0xa0-byte
  objects made only by the location builder 0x5af170, each paired with a Spawn. The terrain
  spec's "+0x18, +0x24: two object containers filled by the post passes" stands, with roles now
  as above. The location builder is called from the generator at 0x5e9672, so "post passes" is
  accurate for +0x24 only in that sense.

Behavioural consequences:

* **Draw / ambience:** none for correctly placed records. Sprites reach rendering and ambience
  only through zone +0x04 → chunk +0x248 → chunk +0x240. Nothing from +0x18 or +0x24 enters that
  path.
* **Lighting / record contents:** element +0x28 of every zone sprite record is overwritten by the
  chunk builder with the block-derived value of §2.2. Its value after meshing is therefore
  defined, and this applies to tree records as well. `alpha-worldgen-trees.md` §5 ("+0x20 …
  +0x2b copied but never set by the builder") still holds for the generator's output, but not
  after the chunk builder has run. How the renderer uses +0x28 is not established here.
* **Collision:** no reader of zone +0x04, +0x18 or +0x24 was found in collision code during
  this ruling, and no collision statement in the specs depends on these offsets. Nothing
  changes.
* **Persistence:** +0x18 (two words per Spawn) is saved by 0x44e340, and +0x24 is not.

---

## Evidence summary

Zone: constructor 0x6240f0, destructor 0x624390 (vtable 0x71e280 entry 0x624760), save test
0x5a5050, serializer 0x44e340, list append 0x4862e0, vector append 0x66add0, vector count
0x487f50. Generator 0x5e4850 append sites as listed. Location builder 0x5af170 (+0x24 element
0x5ba799 … 0x5ba7f1, pushes 0x5bab4a / 0x5bab59). Chunk: allocation 0x461eeb … 0x461f54,
constructor 0x4596e0, builder 0x49d910 (0x4a0ce4 … 0x4a1398). Render routine 0x4ac260 (0x4ac475
… 0x4ad472, gate 0x4acab3). Helpers 0x42c7a0 (i64 triplet difference), 0x43ac20 (fixed-point dot
product), 0x42c900 (fixed-point scale). Constants read from the image: 0x702b80 = 4096.0f,
0x6fcd94 = 2^−16, 0x702aa0 = 32.0f, 0x702a94 = 22.4f, 0x702a60 = 0.03125, 0x702a88 = 32768.0,
0x6fcde0 = 1.5f.

## Residual uncertainty

* Static reading only; nothing was verified at runtime.
* The meaning of chunk +0x70 (the publication key) and the exact publication rule are as
  observed. A few instructions between 0x4ac61b and 0x4ac631 are missing from the evidence
  listing. The key's producer was not traced.
* Which of the chunk's two bound triplets is the minimum was not checked. The midpoint does not
  depend on it.
* The view test 0x47f3c0 and the quantity at G+0x1d4 in its radius were not read.
* The +0x24 element type (0xa0 bytes), its +0x78 container, its readers, and the meaning of the
  paired Spawn's +0x10e0 / +0x109c values are unresolved. Readers were searched only through
  the zone lookups and the main client routines (0x488ee0, 0x60c510, 0x4ac260, 0x46a8a0). A
  reader elsewhere, or in `Server.exe`, would not have been found.
* The meaning and renderer use of the sprite element +0x28 value (§2.2), and of the
  mode-bit-0 branch, are not established.
* `Server.exe` was not examined for these fields.
