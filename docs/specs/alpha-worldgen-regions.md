Status: FINAL / STABLE / HELD

# Alpha world generation: region cells, climate fields and locations

Behavioural specification of the large-scale layer of the Cube World **Alpha** world: the
1024 × 1024 grid of **region cells** (one jittered control point per 16384-block region,
carrying moisture, warmth, elevation and a fire flag), the noise-displaced nearest-cell and
distance-weighted blends that turn those points into smooth climate fields, the "link
channels" between neighbouring cells, and the 8 × 8 grid of **location records** inside
each region.

Evidence: `Cube.exe` (SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
image base 0x400000). Every address below is an **RVA in Cube.exe** unless it is marked
as a `Server.exe` RVA (`Server.exe` SHA-256 `1ffbcbce…0d34fb1`, image base 0x400000).
The companion [random spec](alpha-worldgen-random.md) defines the noise N(x, y), the world
value A and the world offset integers; this document only names them.

---

## 0. Conventions

* f32(…) means rounding to IEEE single precision; "float" and "double" name the precision in
  which an operation is carried out. trunc is truncation toward zero.
* **Truncating division** tdiv(a, b) is integer division rounded toward zero. The game uses
  it (not floor division) for every coordinate-to-index conversion in this document; for the
  non-negative coordinates of the world the two agree, except where noted in §3.2.
* **Block coordinates** are integers; region cells exist for 0 ≤ x, y < 2²⁴ blocks.
* **Fixed point** ("fx") positions are signed 64-bit integers in 1/65536 block; block → fx
  is a 64-bit shift left by 16.
* **Region index** (rx, ry) = (tdiv(x, 16384), tdiv(y, 16384)), 0..1023.
* **Location slot** (lx, ly) = (tdiv(x, 2048), tdiv(y, 2048)), 0..8191; it lies in region
  (lx >> 3, ly >> 3) at slot (sx, sy) = (lx mod 8, ly mod 8).
* **N** is the two-argument noise routine at RVA 0xc0ef0 (N of the random spec). It takes
  two doubles; every caller in this document rounds its result to f32 before using it.
* **A** is the world integer at W+0x800188 (W = world object): at world initialisation
  (RVA 0x1a52e0) the seed is passed to `srand` and the first draw mod 100000 is stored there.
* **O1fc … O218** are the eight world integers at W+0x8001fc, +0x800200, …, +0x800218, each
  a draw mod 100000 made during the same initialisation.
* "Draw" means one call of the C runtime `rand`; "cell" means region cell; "location"
  means location record.

---

## 1. Grids in the world object

| Grid | Where | Size | Entry |
|---|---|---|---|
| Region objects | W+0xbc | 1024 × 1024 pointers, index rx·1024 + ry | region object (§6) |
| Region cells | W+0x4000bc | 1024 × 1024 pointers, index rx·1024 + ry | cell record (§2) |

A missing entry is null. World (re)initialisation (RVA 0x1a52e0) releases every existing
region and cell and sets both pointers to null.

---

## 2. Region cells

### 2.1 Record (0x1c bytes)

| Offset | Type | Meaning (name used in these specs) |
|---|---|---|
| +0x00 | int32 | point x, blocks |
| +0x04 | int32 | point y, blocks |
| +0x08 | byte | **fire flag** (1 = fire region) |
| +0x0c | float | **moisture** m, 0..1 |
| +0x10 | float | **warmth** w, 0..1 |
| +0x14 | int32 | cell id = 1024·ry + rx + A |
| +0x18 | int32 | **elevation** e: exactly −100 for a sea cell, positive for land |

Bytes +0x09..+0x0b are not written by the creator.

### 2.2 Creation (RVA 0x1d7a70; Server.exe counterpart RVA 0x10b870, same length)

The creator takes (rx, ry). Indices outside 0..1023 give no cell. An existing cell is
returned unchanged; otherwise a cell is built and stored, and it is not changed afterwards
(only world reinitialisation removes it). The only static caller is region preparation
(§6.1). Steps, with every draw in order:

1. `srand(rx + 0x108a + 1024·ry + 3·A)`; allocate the record; id = 1024·ry + rx + A;
   fire = 0.
2. **Home test.** The world holds a home point at W+0x8000f0 / W+0x8000f4 (two floats, both
   set to 8396928.0 at world initialisation). The cell is the **home cell** when
   rx = tdiv(tdiv(trunc(hx), 256), 64) and ry likewise from hy; for the fixed home point this
   is (512, 512).
3. Draw r1.
4. **Temperate branch** if r1 is even **or** the cell is the home cell:
   * draw: m = f32(f32(f32(draw · 0.4f) / 32767f) + 0.3f), all in float (0.3..0.7);
   * draw: w = f32(draw · 0.4 / 32767.0 + 0.4000000059604645), computed in double and then
     rounded (0.4..0.8);
   * home cell: the point is (trunc(hx), trunc(hy)) = (8396928, 8396928) and creation
     continues at step 7 (no point draws, no fire draw);
   * otherwise draw r2; if r2 mod 10 = 0 (**fire region**): fire = 1, then
     draw: w = f32(f32(f32(draw · 0.2f) / 32767f) + 0.8f) (0.8..1.0), then
     draw: m = f32(f32(draw · 0.5f) / 32767f) (0..0.5).
5. **Extreme branch** otherwise (r1 odd, not home):
   * draw r3; then draw: m = f32(f32(draw · 0.1f) / 32767f) if r3 is even, or that value
     plus 0.9f (float add) if r3 is odd;
   * draw r4; then draw: w formed the same way from r4.
   Extreme cells are therefore dry or wet, and cold or hot, at the ends of the range.
6. **Point** (not for the home cell): the first draw gives
   y = 16384·ry + (draw mod 15360) + 512, the second gives x = 16384·rx + 512 + (draw mod 15360)
   — the y draw comes **first**.
7. **Snap** each coordinate v: v ← 2048·tdiv(v, 2048) + 1024 (done as "add 2047 if negative,
   clear the low 11 bits, add 1024"). Every cell point is the centre of one of its region's
   8 × 8 location slots. (The home point snaps to (8397824, 8397824).)
8. **Elevation**, with Ad = A as a double:
   n1 = f32(N(rx·1.4 + Ad, (ry·1.4 + Ad) + 843.0)), f = f32(f32(f32(n1 + 1) · 100) − 70)
   (float); n2 = f32(N(rx·4.0 + Ad, (ry·4.0 + Ad) + 843.0)); e = trunc(f32(f + f32(n2 · 30)))
   (the products with 1.4, 4.0 and the additions inside N's arguments are double).
9. If e ≤ 0 (**sea**):
   * fire = 0;
   * home cell: draw, e = (draw mod 50) + 20, store the cell and stop (the home region is
     always land);
   * otherwise e ← max(e − 100, −100), which is always −100;
   * draw r5; if r5 is odd, m and w are re-drawn as in the temperate branch (m float,
     then w double); if r5 is even, m and w are re-drawn as the "odd" forms of step 5
     (0.9 + …, float), m first. Each re-draw consumes one draw.
10. Store the cell at W+0x4000bc[rx·1024 + ry].

The elevation noise depends only on (rx, ry) and A, not on the draws; it is evaluated after
all branch, climate and point draws of steps 3–6.

---

## 3. Displacement and the owning cell

### 3.1 Sample displacement D(x, y) (RVA 0x1eefa0)

For a block column (x, y):

* dx = f32(f32(f32(N(y · 0.0005, 3423.0)) · 3) · 256)
* dy = f32(f32(f32(N(x · 0.0005, 23421.0)) · 3) · 256)

(y · 0.0005 and x · 0.0005 are double; the x shift is driven by y and the y shift by x;
dx is computed first.) |dx|, |dy| ≤ 768·|N|.

### 3.2 Search window

The candidate cells for column (x, y) are all (i, j) with
tdiv(x − 16384, 16384) ≤ i ≤ tdiv(x + 16384, 16384), and likewise j from y. For
16384 ≤ x this is the three indices around the column's own region; because the division
truncates, columns with 1 ≤ x < 16384 get only the two indices 0 and 1 (x = 0 gets −1..1),
and the same holds for y. The window uses the **undisplaced** column; only distances use
the displaced point.

### 3.3 Integer squared distance d²

The displaced point is p = (trunc(f32((float)x + dx)), trunc(f32((float)y + dy))) — the sum
is made in float, so from 2²³ blocks upward it is rounded to whole blocks before truncation.
For a cell c: ex = f32(f32(int64(c.x << 16) − int64(p.x << 16) converted to float) · 2⁻¹⁶),
likewise ey; **d²(c) = trunc(f32(f32(ey²) + f32(ex²)))** (the y term is the first operand).

### 3.4 Owner (RVA 0x77e10; index form RVA 0x1cade0)

The owner of (x, y) is the candidate with the smallest d²; on a tie the first one found
wins, scanning i (x) in the outer loop and j (y) in the inner loop, both ascending.
Missing or out-of-range cells are skipped. RVA 0x1cade0 writes the owner's (i, j) to its
output pair, or (−1, −1) if there is none; RVA 0x77e10 returns the owner cell pointer
(null if none). RVA 0x77e10 obtains d² through the helper at RVA 0x1eeee0, which converts
the 64-bit differences with x87 integer loads; the value is the same single-precision
result as above, truncated to an integer.

---

## 4. The climate blends

All four routines take a block column (x, y), use the window and d² of §3, and run:

1. **Nearest pass.** If any candidate of the window is missing or out of range, return the
   **default** at once. Otherwise d0² = the smallest d².
2. **Blend pass.** For each candidate in the same order: t = f32((float)(d²(c) − d0²) ·
   5·10⁻⁷) (the integer difference converted to float, times the float constant at
   **RVA 0x31dcdc**, value 4.999999987e-07); if t > 1 then t = 1; weight wc = 1 − t (float).
   Accumulate Σw += wc and Σv += value(c)·wc in float, in scan order.
3. If Σw > 0 the result is f32(Σv / Σw), else the default.

A weight reaches 0 when d² − d0² ≥ 2·10⁶ blocks². Between two cell points a distance D
apart, d² − d0² ≈ 2·D·δ at a distance δ from their bisector, so a blend is mixed only
within about 10⁶ / D blocks of a (displacement-wobbled) cell boundary — about 61 blocks for
D = 16384.

| Field | RVA | value(c) | Default | Extra rule |
|---|---|---|---|---|
| **Warmth** | 0x1c4dd0 | c.w (+0x10) | 0.5 | — |
| **Moisture** | 0x1c4800 | c.m (+0x0c) | 0.5 | oasis rule below |
| **Sea share** | 0x1f0720 | 1 if c.e < 0, else 0 | 0 | pre-scan |
| **Fire share** | 0x1ef040 | 1 if the fire byte = 1, else 0 | 0 | pre-scan |

* **Pre-scan** (sea and fire shares): before the nearest pass, scan the window rows; a
  missing or out-of-range cell returns 0; if no scanned cell has the property, return 0
  without blending. When a cell with the property is found, the rest of that row is not
  examined; the following passes return 0 if any cell is missing, so the result is the same.
* **Oasis rule** (moisture only, after a blend with Σw > 0): with m the blend result, look up
  the location of slot (tdiv(x, 2048), tdiv(y, 2048)) (§6.3). If m < 0.2, the location
  exists and its type is 3: f = the normalised distance of §7 at (x << 16, y << 16),
  v = 1 − f, b = v·v if v > 0 else 0, m ← f32(f32(0.3·b) + m) (float), and m ← 1 if it
  exceeds 1. The lookup itself is made whenever the blend succeeds, before m is tested; it
  draws nothing.

---

## 5. Link channels

### 5.1 Segment distance (RVA 0x1ef5a0)

Inputs: points A, B, P (doubles). With v = B − A and q = |v|²: if q < 9.999999682655225e-21,
return |P − A|²; else t = ((P − A)·v)/q; if t ≤ 0 return |P − A|²; if t ≥ 1 return
|P − B|²; otherwise |P − (A + t·v)|². Computed in double (each squared length is formed as
y² + x²) and returned rounded to float.

### 5.2 Channel distance B(x, y) (RVA 0x1eea60)

1. (ci, cj) = owner index of (x, y) (RVA 0x1cade0). If it is out of range or that cell is
   missing, B = 1.
2. Distorted point P (double), using four noises evaluated in this order:
   n1 = f32(N(O204 + 0.01x, O208 + 0.01y)), n2 = f32(N(O1fc + 0.0005x, O200 + 0.0005y)),
   P.x = (0.1·n1 + n2)·500 + x;
   n3 = f32(N(O214 + 0.01x, O218 + 0.01y)), n4 = f32(N(O20c + 0.0005x, O210 + 0.0005y)),
   P.y = (0.1·n3 + n4)·500 + y.
   (The offsets and x, y are converted to double; n1..n4 are widened to double.)
3. The running minimum starts at −1 ("none"). For the neighbours (ci−1, cj), (ci+1, cj),
   (ci, cj−1), (ci, cj+1) in that order, if in range and present: s = segment distance
   (§5.1) with A = the owner's point, B = the neighbour's point, at P (points converted to
   double). s replaces the minimum when the minimum is negative or larger than s.
4. If the minimum is still negative, B = 1; else **B = f32(s_min · 0.0002)** (float constant
   at RVA 0x31dce0).

B is proportional to the squared distance (blocks²) from the straight segments joining the
owner's point to its four grid neighbours' points; B = 1 at about 70.7 blocks, and it is not
clamped above.

### 5.3 Grid coordinates G(x, y) (RVA 0x19fc90)

G is returned as two doubles: G.x = ((0.1·n1 + n2)·500)·2⁻¹⁴ + x·2⁻¹⁴ and
G.y = ((0.1·n3 + n4)·500)·2⁻¹⁴ + y·2⁻¹⁴, with the same four noises, arguments and order as
§5.2. Because 2⁻¹⁴ is a power of two this equals P · 2⁻¹⁴ (P of §5.2).

---

## 6. Regions and location records

### 6.1 Region preparation (RVA 0x1da280; Server.exe counterpart RVA 0x10e080)

The Server.exe routine has the same entry steps (cell creation, seeding, 0x15a28-byte
allocation) but a different length (9264 vs 9088 bytes) and is not compared further here.

A region (rx, ry) covers 64 × 64 zones (16384 × 16384 blocks). The zone generator
(RVA 0x1e4850) begins by preparing the regions (zrx + di, zry + dj) for di = −1..1 (outer),
dj = −1..1 (inner) around the zone's own region. Preparation of (rx, ry) does nothing if
the indices are out of range or the region already exists. Otherwise:

1. Create region cells (rx + i, ry + j) for i = −2..2 (outer), j = −2..2 (inner).
2. `srand(A + 1024·ry + rx)`.
3. Allocate the region object (0x15a28 bytes) and construct it (RVA 0x1c3ac0): byte +0x08 = 1,
   +0x0c = 1, +0x10 = +0x14 = 0; a 64 × 64 per-zone table of 16-byte entries at +0x18;
   4096 zone pointers at +0x10018 cleared; 64 location records at +0x14018.
4. Fill the content plan (§6.4).
5. Store the region at W+0xbc[rx·1024 + ry]. This is the last step of planning, so lookups
   of the region's own locations made during its preparation (for example by the moisture
   blend) find no region.

### 6.2 Location record (0x68 bytes, at region + 0x14018 + 0x68·(8·sx + sy))

| Offset | Type | Meaning |
|---|---|---|
| +0x00 | int64 | centre x, fx |
| +0x08 | int64 | centre y, fx |
| +0x10 | float | **radius** r, blocks |
| +0x14 | float | **ground height** h (from the height function at RVA 0x1c5e20 at the centre) |
| +0x18 | int32 | **type** (0 = none) |
| +0x1c, +0x20, +0x24, +0x28 | various | type-dependent (variants, ids, parameters; §6.4) |

Types assigned by region preparation and their special handling confirmed here:

| Type | Assigned by (§6.4) | Confirmed special handling |
|---|---|---|
| 1 | the slot holding the cell point, when W+0xa4 ≠ 0 | — |
| 2, 3, 4, 5, 6, 7 | type draw cases 0–5 | 3: moisture oasis rule (§4) |
| 10 | the type-10 pass | — |
| 11, 12 | type draw cases 6, 7 (radius 128) | undistorted distance (§7) |
| 14 | queue positions n ≡ 2 (mod 4) (radius 150) | undistorted distance (§7) |
| 15 | cases 2, 4, 5 when the region's own cell is sea | — |
| 13 | not assigned by region preparation | distortion scale 100 (§7) |

### 6.3 Location lookup (RVA 0x87da0)

For a slot (lx, ly) with 0 ≤ lx, ly < 8192: the region (tdiv(8·lx, 64), tdiv(8·ly, 64)) =
(lx >> 3, ly >> 3) is fetched through RVA 0x34a90; if it is absent the result is null,
otherwise the record at slot (lx mod 8, ly mod 8). A record exists whenever its region
does; type 0 means no location.

### 6.4 Content plan (outline)

Confirmed order of the planning part of RVA 0x1da280, after construction:

1. **Header.** +0x0c = level from RVA 0x1a4710: 1 for region (512, 512), otherwise
   2 + trunc(0.75 · d), where d is the distance in region units from (512, 512) (squares
   summed in float, square root in double, then float). If the level exceeds 4, draw:
   +0x10 = draw mod 5. Draw: +0x14 = draw mod 4. Draw: a running id counter starts at
   draw mod 10000. If the region's own cell has warmth > 0.81 (float constant at
   RVA 0x31dd18), draw: +0x14 = (draw mod 2) + 4. Then RVA 0x1ee4b0 is called (not
   examined).
2. **Slot pass**, sx = 0..7 (outer), sy = 0..7 (inner), record 8·sx + sy:
   * **Cell-point slot**: when W+0xa4 ≠ 0 and (sx, sy) is the slot that holds the own cell's
     point: type = 1, +0x1c = region +0x14, +0x20 = the cell id; draw: r = (draw mod 200) +
     512; the centre is the cell point in fx; on each side in the order low-x, low-y, high-x,
     high-y, if the disc of radius r + 256 crosses that slot edge, that coordinate is moved
     to edge ± (r + 256) (computed in float, scaled by 65536, truncated); h = height at the
     centre (block coordinates by truncating fx / 65536), negative h → 0. No draws for the
     position and no queue entry.
   * **Other slots**: if the owner (RVA 0x77e10) of the slot centre is the region's own
     cell: draw: r = (draw mod 256) + 512; with M = r + 256 (float), draw: the centre y is
     slot_y0 + M + f32(f32(draw · f32(2048 − 2M)) / 32767) (in fx, each part converted by
     truncation), then draw: the centre x the same way from slot_x0 — **y first**; h =
     height at the centre (no clamp yet); +0x24 = 1. If the owner of the new centre is also
     the own cell, draw: the slot is queued with key f32(√(Δx² + Δy²)) + f32(draw·4/32767)
     − 2, where (Δx, Δy) is the slot-index difference from the cell point's slot, via
     RVA 0x1f4d10. If the first owner test fails, the slot makes no draws.
3. **Queue pass.** The queue is reordered by RVA 0x1c2270. For n = 0..63, stopping when
   the queue is empty: odd n does nothing; even n removes the first entry and, for its
   record, sets a negative h to 0 and +0x24 from a function of n/64 (RVA 0x462f0, not
   examined). If n ≡ 2 (mod 4): type 14, r = 150, the centre snapped on each axis to
   256·tdiv(v, 256) + 128 blocks, plus per-zone table entries and further draws (not fully
   traced). If n ≡ 0 (mod 4): draw a type case = draw mod 8:
   * 0 → type 2;
   * 1 → type 3, draw: +0x1c = draw mod 3;
   * 2 → type 4, or 15 if the own cell is sea (e < 0);
   * 3 → type 5 with +0x1c from the own cell (no draw): 0 if w ≤ 0.8; 3 if w > 0.8 and
     m > 0.8; 2 if w > 0.8 and m < 0.2; otherwise 0. Then draw: r = (draw mod 256) + 256,
     and the centre is re-placed in the slot with two more draws (y first, margin r + 256,
     as in step 2);
   * 4 → type 6, or 15 if sea;
   * 5 → type 7, or 15 if sea;
   * 6 → type 11, r = 128, centre snapped to 256·tdiv(v, 256) + 128 blocks;
   * 7 → type 12, r = 128, centre snapped the same way.
   After any case: +0x28 is set from the +0x24 value (0 below 5; one draw mod 2 below 10;
   one draw mod 3 below 15; one draw mod 3, plus 1, for 15..18; one draw mod 4, plus 1, from
   19); +0x20 = the running counter; draw: counter += 1 + (draw mod 50).
4. **Type-10 pass.** While the queue is not empty, up to 5 times: draw: remove the entry at
   index draw mod (queue length); its record gets type 10; draw: +0x20 = (draw mod 10⁷) + 1;
   the per-zone table entry of the zone holding its centre gets byte +0x18 = 4 and
   +0x1c = that id.
5. Further passes fill the per-zone table (not specified here).

---

## 7. Normalised location distance f(L, X, Y) (RVA 0x1f8e10)

Inputs: a location L and a position (X, Y) in fx (callers pass x << 16, y << 16).

* If L.r < 0.001: f = 0.
* Types 11, 12, 14: ex = f32((float)(X − L.x) · 2⁻¹⁶), ey likewise;
  f = f32(f32(ey²) + f32(ex²)) / f32(r²), all float. (No distortion.)
* All other types (distorted boundary):
  1. Ty = trunc(f64(Y) · (−0.0025)), then Tx = trunc(f64(X) · (−0.0025)) (signed 64-bit,
     through the conversion helper at RVA 0x28d946, which truncates toward zero).
  2. a = f64(0x80AD580000 − Tx) · 2⁻¹⁶ and b = f64(0x1617D0000 − Ty) · 2⁻¹⁶, i.e.
     8432984 + 0.0025·x and 90493 + 0.0025·y (x, y in blocks).
  3. k = 100 for type 13, 256 for every other type in this branch.
     DX = trunc(f32(f32(f32(N(a, b)) · k) · 65536)) − L.x + X (64-bit),
     ex = f32((float)DX · 2⁻¹⁶).
  4. a′ = f64(0xD5F0000 − Tx) · 2⁻¹⁶ = 3423 + 0.0025·x, b′ = f64(0x700000 − Ty) · 2⁻¹⁶ =
     112 + 0.0025·y; DY = trunc(f32(f32(f32(N(a′, b′)) · k) · 65536)) − L.y + Y,
     ey = f32((float)DY · 2⁻¹⁶).
  5. f = f32(f32(ey²) + f32(ex²)) / f32(r²) (float).

f is 0 at the (shifted) centre and 1 on the wobbly rim. The moisture oasis rule (§4) uses
the falloff max(1 − f, 0)².

---

## 8. Open questions

1. **Content plan details** (RVA 0x1da280): the call to RVA 0x1ee4b0, the ordering rule of
   RVA 0x1c2270, the n/64 function giving +0x24, the type-14 branch's zone-table writes and
   draws, the later per-zone table passes, and the meaning of W+0xa4. The type-10 helper at
   RVA 0x1c3ef0 and RVA 0x1f4d10 were not re-examined.
2. The meaning of B (rivers, paths or valleys) and of location types 2, 5, 10, 14, 15.

See also: [alpha-worldgen-locations.md](alpha-worldgen-locations.md) and
[alpha-worldgen-caves.md](alpha-worldgen-caves.md) (pointers only).

---

## Re-certification (2026-11-14)

Re-checked against Cube.exe (and the Server.exe counterparts' entry steps) using the
original image bytes, the static disassembly and the decompiled listing. All addresses are
now RVAs (image base 0x400000 subtracted).

**Confirmed:** both world grids, offsets and index order; cell record layout; the cell
seed, id, home test, branch structure, all climate constants and their float/double
precision, the fire rule and its w-then-m order, the y-before-x point draws, the snap,
the elevation noise arguments and arithmetic, the sea re-draw rule and its parity, and
the home-land rule; the displacement constants and driving axes; d² and its operand order;
owner scan order and tie rule; blend constants (5·10⁻⁷ at RVA 0x31dcdc), clamp, defaults,
value fields, pre-scan behaviour and the oasis rule; segment distance; channel distance
noise order, neighbour order, initial "none", 0.0002 at RVA 0x31dce0; G; the region
preparation order (5 × 5 cells, seed, 0x15a28 object, constructor, final store); location
record address and lookup; the location distance branches, constants and the 100/256
scale; A = first draw mod 100000; home point 8396928.0.

**Corrected (old → new):**
1. All addresses were image VAs → now Cube.exe/Server.exe RVAs.
2. Sea elevation "−100..−1" → always exactly −100 (max(e − 100, −100) with e ≤ 0). Affects
   described world generation values.
3. Index and window divisions "floor_div" / "floor rounding" → truncating division; near
   the world's low edge the window is 2 indices (x = 1..16383) instead of 3, so blends there
   do not see index −1 (affects described blend results at the edge).
4. Snap "floor(x/2048)·2048" → truncating form (same for non-negative points).
5. Blend mixing width "roughly 100 blocks" → about 10⁶ / D blocks (≈61 at D = 16384).
6. Displacement bound "< 768" → ≤ 768·|N| (N's range not re-verified here).
7. Oasis rule: the location lookup happens before the m < 0.2 test (no draws; no effect on
   results).
8. Location ground-height clamp "where the slot's owner is land" → clamped for the cell-point
   record and for every queued record in the queue pass.
9. Location radius "256 + (draw & 255)" → 512 + (draw mod 256) in the slot pass (256 + … only
   for type 5); position draws are **y first, then x** (random-draw order detail).
10. Type 15 "instead of 4 or 6" → instead of 4, 6 or 7; and the sea test uses the region's
    own cell, not the slot's cell.
11. Type 11 "centre snapped to a block centre" → both 11 and 12 are snapped to the centre
    of a 256-block cell.
12. Type 14 "separate pass" → assigned within the queue pass at positions n ≡ 2 (mod 4);
    type 1 assignment now specified (cell-point slot); type 13 is not assigned here.
13. Former open question 2 (region storage time) resolved: stored at the end. Former open
    question 3 resolved: the 64-bit conversion helper truncates toward zero.

No reversal of an earlier native claim's direction other than items 2, 3 and 12.

**Removed:** the cell and blend test vectors and the displacement test value (not
reproducible from the evidence without executing the noise; verification material); §8
"Checking against the running game" (verification procedure); claims about the palette
document, alpha-rendering.md's open question, the terrain/vegetation/overview effects of
location types, the terrain's use of B and G, zone-column caching of blend values, and the
"several rules" use of the owner's elevation (cross-spec claims not re-established here);
the "answered in" note about the locations spec (reduced to a pointer).

**Unresolved:** listed in §8; additionally, the per-column storage of moisture, warmth and
fire share by the zone generator and the town-street use of G were not re-verified.
