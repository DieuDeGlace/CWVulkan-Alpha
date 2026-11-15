Status: FINAL / STABLE / HELD

# Alpha world generation: rock formations, roads, terraces and the zone post passes

Behavioural specification of what the Cube World **Alpha** zone generator (Cube.exe RVA
0x1e4850) does between the surface pass and the voxel-tree grid (code RVA range
0x1e6b58..0x1e8c2a, plus the ravine spawns of the surface pass at RVA 0x1e6434..0x1e699e), and
of the two formation builders at RVA 0x1cbe00 and RVA 0x1cb600. Image: `Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x400000.
All addresses in this file are RVAs in that image.

**There are no caves in this range.** The builder at RVA 0x1cbe00 **adds** a noise-shaped
mound of rock on top of the ground; it does not carve. The only carving the zone generator
does is the ravine of the surface pass (terrain spec §5.5); the dungeon builder is called from
here (§6.1) but is specified elsewhere. The file keeps its planned name so links in other
specs stay valid.

Companion specs (pointers only): [random](alpha-worldgen-random.md),
[regions](alpha-worldgen-regions.md), [terrain](alpha-worldgen-terrain.md),
[locations](alpha-worldgen-locations.md), [trees](alpha-worldgen-trees.md).

---

## 0. Conventions

* Float (32-bit) unless marked double. Noise **N(a, b)** is the two-argument noise routine at
  RVA 0xc0ef0 (random spec); its arguments are doubles formed as coordinate·k + offset.
  "trunc" rounds toward zero. Column loops run x in the outer loop and y in the inner loop
  unless stated.
* **draw** = one call of the C runtime `rand` (import slot RVA 0x2fc468) from the zone's
  stream. The zone generator seeds that stream once with (world + 0x800188) + 65536·zy + zx
  (random/terrain specs); nothing in this file re-seeds it.
* The zone being generated is (zx, zy) (zone + 0x60, zone + 0x64); its x range is
  [256·zx, 256·zx + 256), likewise y.
  **Lz** = the location record (0x68 bytes, region + 0x14018) of the zone's sub-cell
  (zx >> 3, zy >> 3) within its region; its type is the 32-bit value at Lz + 0x18 and its
  centre is the pair of 64-bit fixed-point values at Lz + 0x00 and Lz + 0x08 (locations spec).
  **E** = the zone's 16-byte entry of the region's zone table (region + 0x18 + 16·index,
  index = 64·(zx & 63) + (zy & 63)); E.kind is its byte +0, E.variant its byte +1. E is
  copied to the zone at +0x78..+0x87.
* **top(x, y)** = column bottom (column + 0x10) + stored count (column + 0x1c), i.e. one above
  the highest stored block.
* **read(x, y, z)** (the block read used throughout): a missing column, or z below the column
  bottom, reads solid grey (200, 200, 200, byte 3 = 1); z above the stored range reads air
  (255, 255, 255, 0) when z > 0 and water (255, 255, 255, 0x82) when z ≤ 0; a stored block
  whose type (byte 3 AND 0x1f) is 0, with z ≤ 0 and bit 0x40 clear, also reads as that water.
  These three default blocks are set by static initialisers at RVA 0x2fae60 (grey),
  0x2fae30 (air) and 0x2fae90 (water).
* **write(x, y, z, b)** = the block writer at RVA 0x4e7a0. It does nothing when the column is
  missing, or when byte 3 of b is 0 and z ≥ top. When z ≤ 0 and (byte 3 of b) AND 0x5f is 0 it
  stores water (255, 255, 255, 0x82) instead (initialiser RVA 0x2f99c0). Writing below the
  column bottom also fills the cells between with rock-coloured type-6 blocks.
* **type** of a block = byte 3 AND 0x1f. Bits 0x40 and 0x80 of byte 3 are tested separately.
* **Zone-local.** Every column lookup in this range (RVA 0x347a0) passes the zone being
  generated; that lookup returns "missing" for any column outside the zone, so nothing here
  reads (beyond the grey default) or writes a neighbouring zone. Shapes extending past the
  zone edge are cut off there.
* **Colours**: Rk(x, y, z) = the rock colour routine (RVA 0x1f9620, three floats), Cl(x, y, z)
  = the cliff colour routine (RVA 0x1c7140), the surface block = the routine at RVA 0x1c56e0
  (terrain spec). A block built from float channels takes trunc of each channel and stores
  its low 8 bits.
* **home** = (8396928.0, 8396928.0), the floats at world + 0x8000f0 / + 0x8000f4 (set at RVA
  0x1a55f8). "Within d of home" compares the float (x − homeX)² + (y − homeY)² with d²;
  "within" means strictly less.
* **Object list** = the list created in §3.1. Its distance tests convert the 64-bit
  fixed-point difference to float, multiply by 1/65536 and compare the squared distance
  (strictly less) with d².

---

## 1. The rock mound builder (RVA 0x1cbe00)

Inputs, in order: centre x, centre y, base height z, half-sizes rx, ry, height rz (integers),
the zone.

**Shape.** For a column (X, Y) the mound top is
zt(X, Y) = trunc(f32(f32(N(0.01X + 4394, 0.01Y + 8974) · 20) + (float)(z + rz))).
A cell (X, Y, Z) is **inside** when v² + u² ≤ 1 (the sum is formed in that order), with
u = f32(N(0.05Y, 0.05Z) · f32(10 / rx)) + f32((X − x) / rx) and
v = f32(N(0.05X + 4374, 0.05Z + 9898) · f32(10 / ry)) + f32((Y − y) / ry)
(divisions in float). The cross-section is an ellipse of half-axes about rx, ry whose outline
wobbles by up to about ±10 blocks and changes with height; the body runs from the column
bottom up to the noisy top zt.

**Phase 1, the guard.** For X = x − 2rx … x + 2rx (outer) and Y = y − 2ry … y + 2ry (inner),
for each column that exists, for Z = zt(X, Y) down to the column bottom inclusive: if the
cell is inside, read it; if its type is not 2 and bit 0x80 of byte 3 is set, **the whole
routine returns** and nothing is written. Cells reading as the default water (type 2, byte
0x82) therefore never stop a mound.

**Phase 2, the fill.** Same loops. For each existing column with bottom b0, for
Z = zt(X, Y) down to b0 inclusive: read the cell; continue only if its type is 0, 2, 4, 9 or 5
and bit 0x40 is clear; then, if the cell is inside:
* when Z = zt(X, Y): write the surface block of (X, Y, Z) built from the column's two stored
  climate values (column + 0x04 passed first, column + 0x08 second);
* otherwise write (Rk(X, Y, Z), type 6).

Cells of types 0, 2, 4, 9 and 5 (5 being the mesa type of §2) become rock; every other type,
and everything below the column bottom, is left alone. A column whose top cell is outside the
ellipse gets no surface-block cap; its highest inside cell is plain rock.

No draws.

---

## 2. The layered mesa builder (RVA 0x1cb600)

Same inputs and loops as §1, with these differences:

* top: zt(X, Y) = trunc(f32(f32(N(0.01X + 4394, 0.01Y + 8974) · (float)rz) + (float)(rz + z)))
  (the noise is scaled by rz instead of 20);
* guard (phase 1) inside test: u = f32(N(0.05Y, 0.05Z) · 0.4) + f32((X − x) / rx),
  v = f32(N(0.05X + 4374, 0.05Z + 9898) · 0.4) + f32((Y − y) / ry);
* the guard has **no type-2 exception**: any inside cell with bit 0x80 set stops the mesa,
  including cells that read as the default water (byte 3 = 0x82), so sea or lake water inside
  the guard shape prevents the mesa;
* fill (phase 2) inside test: u = f32(N(0.05Y, 0.02Z) · 0.5) + f32((X − x) / rx),
  v = f32(N(0.05X + 4374, 0.02Z + 9898) · 0.5) + f32((Y − y) / ry) (vertical noise scale 0.02,
  fixed wobble factor 0.5);
* fill colour below the top: s = (float)(Z − 8 − b0) · 0.05 (the float 0.05000000074505806),
  clamped to [0, 1]; the rock colour Rk(X, Y, Z) is fetched first and scaled by s, then the
  cliff colour Cl(X, Y, Z) is scaled by (1 − s); each channel is Cl·(1 − s) + Rk·s; type **5**.
  The lowest 8 blocks above the column bottom are pure cliff colour and the colour reaches
  pure rock 20 blocks higher;
* the top cell gets the surface block exactly as in §1.

No draws.

---

## 3. Order of the post passes (RVA 0x1e6b58..0x1e8c2a)

After the surface pass, in this order:

| § | Code (RVA) | Condition | Draws |
|---|---|---|---|
| 3.1 | 0x1e6b58..0x1e6c76 | always | 0 |
| 3.2 | 0x1e6c77..0x1e6f6b | Lz type 6 | 9 to 36 |
| 3.3 | 0x1e6f72..0x1e703f | Lz type 13 or 4 with its centre in this zone | 0 |
| 4.1 | 0x1e7040..0x1e7143 | Lz type 11 with its centre in this zone | 1 |
| 4.2 | 0x1e7154..0x1e7278 | otherwise, Lz type 12 with its centre in this zone | 1 + tree builder |
| 4.3 | 0x1e727a..0x1e7319, loop body 0x1e7350..0x1e7687 | otherwise, E.kind ∉ {1, 3, 4} | 1 + 5 or 6 per formation |
| 5.1 | 0x1e7321..0x1e7e19 (body from 0x1e7696) | always (roads) | 0 |
| 5.2 | 0x1e7e2d..0x1e8490 | always (terraces) | 1 per terrace floor written |
| 6.1 | 0x1e8496..0x1e84fb | E.kind = 3 or 5 (dungeon) | dungeon builder |
| 6.2 | 0x1e84fc..0x1e888d | always (terrace boulders) | 3 per seed |
| 6.3 | 0x1e888e..0x1e8c2a | always (ravine boulders) | 3 per seed |

"Centre in this zone": the location centre's fixed-point x is divided by 65536 and then by
256, both divisions truncating toward zero (likewise y); the pair must equal (zx, zy). World
coordinates are non-negative, so this equals floor division.

### 3.1 Setup (RVA 0x1e6b58)

A temporary buffer of the surface pass is freed; E is copied into the zone (+0x78..+0x87); an
empty **object list** is created (RVA 0x39600). The list is filled only later in this routine
(§8 items 2 and 5), so every object-list test in §3 to §6 finds it empty and never rejects.
The zone-centre climate is read from the stored column (256zx + 128, 256zy + 128):
**mc** = its value at column + 0x04, **wc** = its value at column + 0x08 (computed by RVA
0x1c4800 and 0x1c4dd0 respectively if the column were missing).

### 3.2 Type 6: the 3 × 3 mound grid

For a = 0, 256, 512 (outer) and b = 0, 256, 512 (inner):

1. draw (RVA 0x1e6ca0); if (draw & 3) = 0, next candidate.
2. (X, Y) = (256zx + a/3 + 42, 256zy + b/3 + 42) (integer division; offsets 42, 127, 212).
3. f = the location-shape value of Lz at the fixed-point point (X·65536, Y·65536) (RVA
   0x1f8e10, regions spec); v = 1 − f; if v ≤ 0 or v² < 0.5, next candidate.
4. Object-list test, 80 blocks (squared distance < 6400 would skip; the list is empty here).
5. draw: rx = draw % 10 + 20; draw: ry = draw % 10 + 20; draw: rz = (draw & 15) + 20
   (RVA 0x1e6e67, 0x1e6e7e, 0x1e6e95).
6. If within 60 blocks of home (squared distance < 3600), next candidate.
7. Rock mound (§1) at (X, Y, top(X, Y)) with (rx, ry, rz).

Per candidate 1 draw, or 4 when it passes steps 1–3. Lz is shared by the 8 × 8 zones of its
2048-block sub-cell, so each of those zones runs this grid; the shape test confines the
mounds to where the location's shape value is low.

### 3.3 Types 13 and 4

When Lz is of type 13 or 4 and its centre lies in this zone, the routine searches down from
top(centre) for the first cell whose type is neither 0 nor 2. The result is not used; nothing
is written and nothing is drawn.

---

## 4. The exclusive chain: great mound, giant tree or rock pass

At most one of §4.1, §4.2, §4.3 runs. A type-11 or type-12 location whose centre is not in
this zone falls through to §4.3.

### 4.1 Type 11: the great mound

If Lz type = 11 and its centre is in this zone: draw (RVA 0x1e70ec); rock mound (§1) at the
zone centre (256zx + 128, 256zy + 128, top there) with rx = ry = 100 and
rz = draw % 100 + 100. Its ±200-block box is cut at the zone edges (zone-local writes), so
the rock covers at most 256 × 256 columns. Then the chain ends.

### 4.2 Type 12: the giant tree

Otherwise, if Lz type = 12 and its centre is in this zone: one draw (RVA 0x1e71f6) whose value
is unused; voxel tree builder (RVA 0x1df960, trees spec) called with (zone-centre x,
zone-centre y, top(centre), 80, 80, 6, zone); then a downward search from top(centre) for the
first cell that is neither type 0 nor type 2, whose result is unused. The chain ends.

### 4.3 The rock pass

Otherwise, if E.kind is not 4, 1 or 3:

1. draw (RVA 0x1e72a8): n = draw & 3.
2. **Climate bonus**: n ← n + 2 if (wc > 0.6 or wc < 0.3) and (mc > 0.7 or (mc < 0.4 and
   (mc > 0.2 or wc < 0.8))). Float constants 0.6000000238418579, 0.30000001192092896,
   0.699999988079071, 0.4000000059604645, 0.20000000298023224, 0.800000011920929; all
   comparisons strict. So 0..5 formations.
3. For each of the n formations:
   1. draw: rx = draw % 40 + 10; draw: ry = draw % 40 + 10; draw: rz = draw % 25 + 10
      (RVA 0x1e7356, 0x1e7369, 0x1e737c).
   2. draw: x = 2·(128zx + rx) + draw % (4·(64 − rx)); draw: y = 2·(128zy + ry) +
      draw % (4·(64 − ry)) (RVA 0x1e73a6, 0x1e73c9). So x lies in
      [256zx + 2rx, 256zx + 255 − 2rx] and the ±2rx box is inside the zone.
   3. Skip if within 60 blocks of home.
   4. Object-list test, 80 blocks (never rejects here).
   5. Skip if q(x, y) > 0.6, where q is the town-weight routine at RVA 0x1989d0 (terrain spec).
   6. Skip unless 1 − 50·R(x, y) < 0, R being the routine at RVA 0x1f9340 (terrain spec),
      i.e. R > 0.02.
   7. m = the routine at RVA 0x1effa0 at (x, y) (terrain spec's mountain mask); zt = top(x, y).
   8. draw (RVA 0x1e75f6): if (draw & 1) ≠ 0 and m < 0.25: **mesa** (§2) at (x, y, zt) with
      (rx, ry, rz / 2) (integer division); otherwise **mound** (§1) at (x, y, zt) with
      (rx, ry, rz).

   Per formation 5 draws, plus 1 when it passes steps 3–6.

---

## 5. Roads and terraces

### 5.1 Roads (RVA 0x1e7321..0x1e7e19)

For every column of the zone (x outer, y inner):

1. s = 1 − 50·W(x, y), W being the routine at RVA 0x1f9f70 (terrain spec); q = q(x, y). If
   s < 0 (W > 0.02), next column.
2. h = H(x, y) (RVA 0x1c5e20, terrain spec), replaced by 0 if negative. z1 = trunc(h + 1).
3. e = 1 − (1 − s)³; zc = trunc(f32(f32((N(0.02x + 55432, 0.02y + 974) + 1) · 4)
   + f32(f32(e · 5) + h))) (the clearance height; the e and h terms are summed first).
4. If read(x, y, z1) has type 0 or 2, or bit 0x40 set, next column.
5. **Road colour**: g = N(0.05x + 843, 0.05y + 984); C = 140 + 40g on all three channels.
   Cobbles: with a = 234·(x / 2) and b = 234·(y / 2) (integer division toward zero), if
   N(a + 4834, b + 49434) > 0.5 then C ← C + 20·N(a, b + 49) (both noises at integer
   arguments). If the region's **style** (the 32-bit value at region + 0x14) is 1, 4 or 5,
   C = Cl(x, y, z1) instead. Final colour = Cl(x, y, z1)·(1 − q) + C·q per channel, each
   channel clamped to [0, 255]. Write (colour, type **11**) at z1 − 1.
6. If q > 0.92 (float 0.9200000166893005): zc = top(x, y). For z = z1 … zc − 1: if read(x, y, z)
   has bit 0x40 clear, write air (255, 255, 255, 0) there (which the writer turns into water
   at z ≤ 0, §0).

Outside towns a road is a type-11 strip at the base height H with a few blocks of headroom
cleared above it; where q > 0.92 everything from the road level up to the column top is
cleared.

### 5.2 Terraces and pools (RVA 0x1e7e2d..0x1e8490)

An empty list L1 is created first. Then for every column (x outer, y inner):

1. s = 1 − 50·R(x, y); if s < 0 (R > 0.02), next column. If q(x, y) > 0.95 (float
   0.949999988079071), next column.
2. h = H(x, y), 0 if negative. zb = 5·(trunc(h) / 5) (integer division), the terrace level.
3. t = f32(h − zb) / 5. If t < 0.5: a = 2t. Otherwise a = 1 − 4(t − 0.5), and if that is
   negative: a = (a + 1)² − 1. (a ∈ (−1, 1].)
4. zw = zb when a ≥ 0, else zw = trunc(f32(zb − 5a)) (a little above zb).
   zlow = trunc(f32(f32(zb − 5a) + 2)).
5. **Pool**: for z = zlow … zb inclusive: if read has bit 0x40 clear, write
   (0, 0, trunc(255·(1 − a) + 0·a), type 2). The range is non-empty exactly when
   zlow ≤ zb, which holds for a > 0.2 (up to float rounding at the boundary): for
   0.2 < a < 0.4 the pool is the single cell zb; for larger a it is deeper. For a ≤ 0.2 (all
   negative a included) there is no pool. In t this is roughly 0.1 < t < 0.7.
6. **Floor**: if read(x, y, zw) is not of type 2 and has bit 0x40 clear: write
   (Rk(x, y, zw), type **3**); then draw (RVA 0x1e82b4): if draw % 200 = 0, append the
   **boulder seed** (x, y, zw) to L1 (list node routine RVA 0x1a6b60). Whenever the pool
   wrote zb, the floor cell reads as type 2 and is skipped **without a draw**.
7. **Clearing**: e = 1 − (1 − s)³; zc = trunc(f32(f32((N(0.02x + 55432, 0.02y + 974) + 1) · 2)
   + f32(f32(e · 5) + h))); for z = zw + 1 … zc − 1: if read has bit 0x40 clear, write air.

On smooth ground the effect is a staircase of 5-block terraces whose floors are type-3 rock-
coloured blocks, with shallow pools on part of each step.

---

## 6. Dungeons and boulders

### 6.1 Dungeon builder calls (RVA 0x1e84c3, 0x1e84f7)

* If E.kind = 3: the dungeon builder (RVA 0x1cc510) with (zone, region, 256zx + 32,
  256zy + 32, E.variant, 0).
* Then, if E.kind = 5: the same builder with (zone, region, 256zx + 32, 256zy + 32, 3, 1).

The builder (body about 39 KB, 39444 bytes) reads the 32-bit value at zone + 0x80 and the
byte at zone + 0x84 (E bytes +8 and +12), selects a style from its fifth argument (1 and 2
share one style that makes one extra draw, % 3; 3, 5 and all other values each have their own
style), then fills a 3 × 3 × 3 grid of rooms with six draws per room, and continues with many
further draws whose number depends on the layout and the terrain met. The builder body itself
contains no seeding call. Its full draw count is not specified here (follow-up
"alpha-worldgen-dungeons"); until it is, the later draws of a zone with E.kind = 3 (boulders,
camps, trees, plants) cannot be placed in the stream.

### 6.2 Terrace boulders (list L1)

For every seed (bx, by, bz) of L1, in the order appended (column order of §5.2):

1. draw: ax = draw % 3 + 2; draw: ay = draw % 3 + 2; draw: az = draw % 3 + 2
   (RVA 0x1e852d, 0x1e8544, 0x1e855b).
2. For X = bx − ax … bx + ax (outer), Y = by − ay … by + ay (inner):
   n = f32(N(0.05X, 0.05Y) · 0.8); ux = f32((X − bx) / ax), uy = f32((Y − by) / ay),
   s2 = f32(uy² + ux²); for Z = bz + 2az down to bz − 2az:
   w = f32(f32((Z − bz) / az) + n); if f32(w² + s2) ≤ 1: if read(X, Y, Z) has bit 0x80 clear
   and bit 0x40 clear, write (Rk(X, Y, Z), type 6).

Cells reading as the default water (byte 3 = 0x82) have bit 0x80 set and are not replaced.

### 6.3 Ravine boulders (list L2)

For every seed of L2 (§7, case 3), in the order appended: draw: ax = (draw & 3) + 4; draw:
ay = (draw & 3) + 4; draw: az = draw % 6 + 4 (RVA 0x1e88bf, 0x1e88da, 0x1e88f5); the same
shape as §6.2; write only where bit 0x80 is clear (bit 0x40 is not tested), with the Rk colour
and type byte **0x26** (type 6 with bit 0x20 set).

---

## 7. Ravine spawns (surface pass, RVA 0x1e640d..0x1e699e)

Reached in the surface pass right after a ravine column's floor block has been written
(which happens only when top > floor), with S, top and floor the ravine quantities of the
terrain spec §5.5. Condition before any draw: top − floor > 2 (float, strict) and
(x + 3y) mod 7 = 0. Then:

1. draw (RVA 0x1e6434); if draw % 30 ≠ 0, stop.
2. draw (RVA 0x1e6449): case = draw & 3. Let zf = trunc(floor) and the spawn position be the
   64-bit fixed-point triple (x·65536 + 32768, y·65536 + 32768, zf·65536).
   * **0, item.** If top > S, stop. draw (RVA 0x1e6480); if (draw & 3) ≠ 0, stop. Build an
     item record (constructor RVA 0x4a970) with byte +0x00 = 11 (category), byte +0x01 = 19
     (subtype; rendering spec §10.1), byte +0x0d = 9, 16-bit +0x10 = 1, the position at
     +0x118/+0x120/+0x128, the float 0.1 at +0x134, and draw (RVA 0x1e6598): the float at
     +0x130 = f32(draw · 6.283185307179586 / 32767) computed in double; append it to the
     zone's item list (zone + 0x30, RVA 0x4d590).
   * **1, creature.** If top > S, stop. Create a creature record (0x10f0 bytes, constructor
     RVA 0x1adb20); position at +0x10, +0x18, +0x20; +0x34 = 1, +0x28 = 6, the float +0xf58 =
     25.0; then draw (RVA 0x1e670c): k = draw % 10: k = 0 → kind 131, k = 1 → 133, k = 3 →
     draw (RVA 0x1e672c): j = draw % 100: j = 0 → 138, 1..3 → 137, 4..8 → 136, ≥ 9 → 135; any
     other k → 132. The kind is stored at +0x2c. Append to the zone list at zone + 0x18 (RVA
     0x26add0).
   * **2, creature.** If top > S, stop. Creature record with the position as above; +0x34 =
     the value of the routine at RVA 0x19a9b0 applied to the whole-block fixed-point x·65536
     and y·65536 (without the half-block offset; no draws); +0x28 = 1; kind +0x2c = **59**;
     append to zone + 0x18.
   * **3, boulder seed.** Append (x, y, trunc(floor + 2)) to list L2 (§6.3). No condition on S.

Draws per eligible column: 1; or 2 (cases 2 and 3, or cases 0 and 1 with top > S); case 0
otherwise 3, or 4 when the item is made; case 1 otherwise 3, or 4 when k = 3. None of the
record constructors draw.

---

## 8. From RVA 0x1e8c2b to the tree grid (outline)

These passes follow §6.3 and precede the voxel-tree grid (RVA 0x1e9e7d), so their draws come
before the tree and plant draws. Conditions and the zone generator's own draws are given; the
called builders' internal draw counts are not specified here.

1. **Home building** (RVA 0x1e8c2b..0x1e8d85): only if world + 0xa4 is 0, zx and zy equal
   trunc(homeX) and trunc(homeY) each divided by 256 (truncating), and the column
   (trunc(homeX), trunc(homeY)) exists. The model is the pointer at byte offset 0x212c (entry
   2123) of the world's model list (world + 0x20 .. world + 0x24) when the list is longer than
   0x212c bytes, otherwise null; the placement routine (RVA 0x1f0ce0) is then called with the
   position (trunc(homeX) − model width / 2, trunc(homeY) − model depth / 2, top of that
   column), the halves taken from the model's 32-bit values at +0x44 and +0x48 (integer
   division), plus (0, 6, 0, zone, 1) and a zeroed four-value block. The routine at RVA
   0x1f0ce0 contains draws.
2. **Camp attempts** (RVA 0x1e8d86..0x1e8f8c): only if (zx + zy) is odd. Up to 10 tries;
   each try: draw (RVA 0x1e8ddb): py = 256zy + 48 + draw % 160; draw (RVA 0x1e8e16):
   px = 256zx + 48 + draw % 160 (position px + 0.5, py + 0.5 in fixed point). If Lz has type
   1 or 5 and v = 1 − f > 0 at that point (f as in §3.2 step 3), the try fails. Otherwise the
   position's z becomes top(px, py) if that column exists (0 otherwise) and the camp routine
   (RVA 0x1ad320) is called. That routine returns without drawing when 1 − 50·R ≥ 0 at the
   point; otherwise it prepares a static of kind 65, draws (value & 3 kept in that static's
   record) and continues with further draws. When it reports success, a record is created
   (RVA 0x18d860) and appended to the object list of §3.1, and the tries stop.
3. **Boulder pass 3** (RVA 0x1e8f8d..0x1e9654): only if E.kind ∉ {4, 1, 3}. draw (RVA
   0x1e8fb5): n = draw % 10, plus 10 under the climate bonus of §4.3 step 2. Each boulder:
   5 draws (RVA 0x1e9056, 0x1e9070, 0x1e9087, 0x1e90b5, 0x1e90d3): ax = (draw & 7) + 3,
   ay = (draw & 7) + 3, az = (draw & 7) + 3, x = 2·(128zx + ax) + draw % (4·(64 − ax)),
   y = 2·(128zy + ay) + draw % (4·(64 − ay)); no further draws. Skipped within 20 blocks of
   home (squared distance < 400), within 40 blocks of an object-list entry (< 1600), or where
   q > 0.25. Otherwise the ground level g is one above the highest stored block of the column
   whose type is neither 0 nor 2; if the cell at g − 1 is of type 11, 8 or 7, or has bit 0x40
   set, nothing is written; else the §6.2 shape (noise N(0.05X, 0.05Y) · 0.8) centred on
   (x, y, g) with (ax, ay, az) is written as (Rk, type byte 0x26) wherever the cell has bit
   0x40 clear (bit 0x80 not tested).
4. **Town builder** (RVA 0x1e9655): if Lz has type 1 or 5, the routine at RVA 0x1af170 (body
   about 62 KB) with (zone, Lz). It draws.
5. **E.kind = 4 site** (RVA 0x1e967f..0x1e9e7c): a record (RVA 0x18d860) at the zone-centre
   column top is appended to the object list first; then draw (RVA 0x1e9783): n = 6 +
   draw % 3 formations; formation i (i = 0 … n − 1) is offset from the zone centre by 25
   blocks along the sine and cosine of the angle i·π / n, makes two draws (RVA 0x1e9884,
   0x1e989f, each masked with & 3) and writes a noise-shaped body of Rk-coloured blocks into
   the terrain; then a static is placed with a further draw (RVA 0x1e9dd5). Details:
   follow-up.
6. Voxel-tree grid (RVA 0x1e9e7d; vegetation spec §5).

---

## 9. Open questions

1. **Dungeon builder** (RVA 0x1cc510): full draw count and blocks; whether any routine it
   calls re-seeds the stream. Blocks every later draw in E.kind = 3 zones.
2. **Camp routine** (RVA 0x1ad320), **town builder** (RVA 0x1af170), **home placement**
   (RVA 0x1f0ce0) and the E.kind = 4 site (§8 item 5): draw counts, shapes and placements.
3. **Kind 5.** The zone generator tests E.kind = 5 (§6.1); no writer of that value has been
   identified in this re-certification.
4. **Mound guard.** Whether an inside cell with bit 0x80 set and a type other than 2 can occur
   in a freshly generated zone when a §1 mound is built.
5. **Home model list.** When the model list has 2123 or fewer entries the model pointer is
   null and the placement still proceeds; the resulting behaviour is not established.
6. **Field meanings.** Creature +0x28 (6 or 1), +0x34 (1 or the RVA 0x19a9b0 value),
   +0xf58 = 25.0; item byte +0x0d = 9; the meaning of world + 0xa4.
7. **Companion labels.** The names of the helper quantities (W, R, H, q, the mountain mask,
   the location-shape value, the surface block, mc/wc as moisture/warmth) and of block types
   0, 4, 7, 8, 9 are those of the companion specs and were not re-derived here.
8. **Pool colour.** Pool water stores (0, 0, trunc(255·(1 − a))); whether any consumer reads
   water colour is outside this file.

---

## Re-certification (2026-11-15)

Checked against Cube.exe (SHA-256 above) using the analysis listing of the original image,
the decompile of the original image, a direct disassembly of the ravine case bodies (RVA
0x1e646b..0x1e699e, absent from the analysis listing) and constants read from the image bytes.

**Confirmed:** both formation builders (loop bounds, noise offsets and scales 0.01/4394/8974,
0.05/4374/9898, 0.02, wobble 10/r, 0.4, 0.5, threshold 1.0, guard rules, accepted fill types,
surface cap, rock type 6, mesa colour ramp 8 and 0.05, type 5); default block values and
writer substitution; home value 8396928; post-pass order and conditions; type-6 grid (3 × 3,
step 256, offsets 42/127/212, draw order and masks, 0.5 shape threshold, 60/80-block tests);
type 13/4 dead search; great mound (100, 100, 100 + draw % 100); giant tree call
(80, 80, 6) and its unused draw; rock pass (mask 3, climate bonus constants and structure,
% 40/% 25 sizes, position formula, q 0.6, R 0.02, mask 0.25, parity draw, rz / 2); roads
(all constants, association of the clearance sum, cobble lattice 234/4834/49434/49, style
values 1/4/5, clamp, type 11 at z1 − 1, q 0.92 clearing); terraces (zb, t, a, zw, zlow,
pool colour, type 3, % 200 seed, clearance ·2); dungeon call arguments; L1/L2 boulders;
ravine spawn conditions, cases, fields, creature kinds 131..138 and 59, draw counts; §8
conditions and draw sites.

**Corrected (old → new):**
1. All addresses were image virtual addresses → now RVAs (image base 0x400000 subtracted).
2. Post-pass range end 0x5e8b7f / §8 start → RVA 0x1e8c2a / 0x1e8c2b (home building begins
   there); table code ranges tightened to instruction boundaries.
3. "Centre in this zone" and the home-zone test: floor division → two truncating divisions
   (equal for the non-negative world coordinates).
4. Mesa colour: "cliff part computed first" → rock colour fetched first (result unchanged).
5. Terrace pool: "empty unless a ≥ 0.4", pools for "t between 0.2 and 0.5" → non-empty
   whenever a > 0.2 (single cell for 0.2 < a < 0.4), roughly 0.1 < t < 0.7. **Affects world
   generation and random-draw order**: every column with a pool skips its floor write and
   its % 200 draw, so more columns skip the draw than the old wording implied.
6. Dungeon builder: "about 16 KB" → about 39 KB (39444 bytes); style taken from the "fourth"
   → fifth argument (zone counted first).
7. Ravine case 2: +0x34 "computed from the position" → from the whole-block fixed-point x and
   y without the half-block offset.
8. E.kind = 4 site (formerly "type-10 site, static objects"): it also appends an object-list
   entry first and writes rock-coloured terrain blocks for 6–8 formations. **Affects world
   generation** (blocks, and the object list seen by the tree pass).
9. Camp routine: "at least one draw when the roughness test passes" → returns without a draw
   when 1 − 50·R ≥ 0; otherwise draws, starting with a value & 3 kept in the kind-65 static.
10. Home building: "if that list is long enough" → placement is not skipped when the list is
    short (null model); moved to open questions.
11. Boulder pass 3: added the ground search, the 11/8/7 and bit-0x40 ground test, the
    bit-0x40-only write test and the noise arguments (outline previously).

No reversal of a draw order or of a pass order was found.

**Removed:** the provenance claim of the introduction; the section of corrections to other
specs (document history that cannot be checked from the original evidence; its native facts
remain in §3–§7); the test-vector section (derived values for implementer checking); the
section on checking against the running game (verification procedure); interpretive
cross-references (street lights, reeds, roads as link channels, "valley centre", "locations
visible now", "height grid", "glowing collectable", "named world"); the unconfirmed claim that
the mound guard never triggers in fresh zones; the claim that no region preparation path
writes kind 5; the open-question research instruction.

**Unresolved:** see §9 items 1–8.
