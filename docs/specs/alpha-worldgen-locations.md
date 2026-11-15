Status: FINAL / STABLE / HELD

# Alpha world generation: region preparation and location records

Native behaviour of Cube World **Alpha** when a region object is filled the first time it is
needed: the region header, the 8 × 8 **location records** consulted by the terrain (towns,
lakes, valleys, dungeon entrances, giant trees and so on) and the 64 × 64 **per-zone table**.
Image: `Cube.exe` (SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
image base 0x400000). All addresses are RVAs in that image unless marked **Server.exe**
(SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, image base
0x400000). Region preparation is RVA 0x1da280 (0x2439 bytes up to and including its return);
the Server.exe counterpart is RVA 0x10e080, of identical length (see §13).

Companion specs (pointers only): [random](alpha-worldgen-random.md) (rand, seed A),
[regions](alpha-worldgen-regions.md) (cells, owner, climate blends, location lookup, f(L, X, Y)),
[terrain](alpha-worldgen-terrain.md) (height H).

---

## 0. Conventions

* **f32(…)**: the value rounded to single precision; arithmetic below is single precision unless
  stated otherwise.
* **fx**: 64-bit fixed point, 1/65536 block.
* **trunc64(v)**: conversion of a floating value to a signed 64-bit integer, rounding toward
  zero (CRT helper RVA 0x28d946).
* **blk(X)**: the fx value X divided by 65536 as a signed 64-bit division rounding toward zero
  (CRT helper RVA 0x28d9e0). Positions produced here are positive, so this equals
  floor(X / 65536).
* **draw**: one call of MSVCR110 `rand`. Draws are listed in the order they are made and only on
  branches that are taken. "draw & m" and "draw % n" are taken on the non-negative draw.
* (rx, ry): the region being prepared. **C**: its own region cell (rx, ry) (regions spec).
  Cell fields used here: byte +0x08 fire flag, float +0x0c moisture, float +0x10 warmth,
  int32 +0x14 cell id, int32 +0x18 elevation (negative = sea).
* **Slot (i, j)**: location sub-cell i = 0..7 along x and j = 0..7 along y; its origin is
  (ox, oy) = (16384·rx + 2048·i, 16384·ry + 2048·j) blocks; its record is record 8·i + j.
* **owner(x, y)**: the region cell returned by RVA 0x77e10 for block column (x, y). "Owned by C"
  compares cell identity.
* **H(x, y)**: the height routine RVA 0x1c5e20 called with third argument 0 (terrain spec).
* **Named world**: the 32-bit value at world + 0xa4 is non-zero. The world constructor
  (RVA 0x18eb00) initialises an empty text string at world + 0x94 whose length field is
  world + 0xa4 (capacity 15 at world + 0xa8), so the test means "the world name is not empty".

---

## 1. The region object (0x15a28 bytes; constructor RVA 0x1c3ac0)

| Offset | Size | Meaning | Value set by the constructor |
|---|---|---|---|
| +0x00 | 4 | type-information pointer | fixed table address |
| +0x04 | 4 | not written by the constructor | — |
| +0x08 | 1 | flag | 1 |
| +0x0c | int32 | **tier**: distance class from the home region (§3) | 1 |
| +0x10 | int32 | **h10**: 0..4 draw for far regions (§3; use not traced) | 0 |
| +0x14 | int32 | **style**: 0..3, or 4..5 in hot regions (§3) | 0 |
| +0x18 | 64 × 64 × 16 | **per-zone table** (§1.1) | see §1.1 |
| +0x10018 | 64 × 64 × 4 | zone pointers | all 0 |
| +0x14018 | 64 × 0x68 | **location records** (§1.2), record 8·i + j | see §1.2 |
| +0x15a18 | 1 | "stored mission data loaded" flag (§9) | 0 |
| +0x15a1c | 4 | pointer returned by RVA 0x6d4b0 (an auxiliary container) | — |
| +0x15a20 | 4 | — | 0 |

### 1.1 Per-zone table entry (16 bytes, at region + 0x18 + 16·(64·zxl + zyl))

zxl and zyl (0..63) are a zone's indices inside its region.

| Offset | Size | Meaning | Constructor value |
|---|---|---|---|
| +0 | byte | **kind**: written by preparation as 0 (none), 1 (town quarter), 3 (dungeon entrance) or 4 (type-10 site) | 0 |
| +1 | byte | **variant**: town quarter 1..4; dungeon style 0..5 | 0 |
| +2, +3 | bytes | not written by preparation | 0 |
| +4 | int32 | **id** (dungeon or type-10 site id) | 0 |
| +8 | int32 | **level** | 1 |
| +0xc | byte | **sub-level** (dungeons) | 0 |
| +0xd..+0xf | bytes | not written by the constructor or by preparation | — |

How the zone generator consumes these entries is outside this document (terrain spec).

### 1.2 Location record (0x68 bytes; constructor RVA 0x1c3bb0)

The constructor writes zero to +0x00..+0x3f, the 16-bit field +0x40, +0x44..+0x5b and the byte
+0x5c, and **1 to the level at +0x24**. Bytes +0x42..+0x43 and +0x5d..+0x67 are not written.

| Offset | Type | Meaning |
|---|---|---|
| +0x00 | int64 | centre x, fx |
| +0x08 | int64 | centre y, fx |
| +0x10 | float | radius r, blocks |
| +0x14 | float | ground height h |
| +0x18 | int32 | type (0 = none) |
| +0x1c | int32 | **variant** (type 1: region style; type 3: 0..2; type 5: climate class; type 14: dungeon variant) |
| +0x20 | int32 | **serial** (running number, §6) or **id** (type 1: the cell id; type 10: random id) |
| +0x24 | int32 | **level** |
| +0x28 | int32 | **sub-level** (types placed by the generic switch, §6.2) |
| +0x2c..+0x53 | — | mission area; during preparation written only from stored data (§9) |
| +0x54..+0x67 | — | monster area; during preparation written only from stored data (§9) |

---

## 2. Order of region preparation

### 2.1 Steps (RVA 0x1da280)

Arguments rx, ry. If either is negative or at least 1024, the routine returns null. If the
world's region pointer table (world + 0xbc, element 1024·rx + ry) already holds a region, that
region is returned unchanged. Otherwise:

1. For i = −2..2 (outer) and j = −2..2 (inner), RVA 0x1d7a70 is called for cell
   (rx + i, ry + j) (the cell-creation step of the regions spec).
2. `srand(A + 1024·ry + rx)`, A being the world seed value at world + 0x800188.
3. Allocate and construct the region object (§1).
4. Header and level range (§3).
5. Slot pass: the 64 slots, candidate list (§4).
6. Candidate sort (§5).
7. Queue pass: types for up to 32 ranked candidates (§6).
8. Type-10 pass: up to five sites from the remaining candidates (§7).
9. Record levels copied into the per-zone table (§8).
10. Discarded height samples (§8.1).
11. Town quarters (§8.2).
12. Stored mission and monster data (§9; no draws).
13. The region pointer is stored in the world table (instruction RVA 0x1dc628). Only release of
    temporary lists follows.

### 2.2 Visibility of the region during its own preparation

Because the region pointer is stored only at step 13, location lookups for columns of the region
being prepared find no region while it is prepared. The location lookup inside H (an inline
lookup in RVA 0x1c5e20 of the record of the column's own slot, region (x >> 14, y >> 14),
record 8·((x >> 11) & 7) + ((y >> 11) & 7)) therefore yields no location for such columns, so
every H evaluated at a column of this region during preparation is the location-free height.

The heights and owners used by steps 5–8 are sampled at columns inside the region. The
discarded samples of §8.1 include columns at x = 16384·(rx + 1) and y = 16384·(ry + 1), which
belong to neighbouring regions. Region cells for the 5 × 5 block around the region exist from
step 1.

---

## 3. Header and level range

1. **tier** (RVA 0x1a4710, arguments rx, ry): 1 if (rx, ry) = (512, 512). Otherwise
   dx = float(512 − rx), dy = float(512 − ry), s = f32(f32(dy²) + f32(dx²)),
   d = the double-precision square root of s rounded to single precision, and
   tier = 2 − (f32(d · −0.75) truncated toward zero) (constant −0.75 at RVA 0x31db84). Thus
   tier = 2 + trunc(0.75·d): 2 at d = 1, 3 at d = √2 and d = 2, 5 at d = 4.
2. **h10** = 0; if tier > 4: draw, h10 = draw % 5.
3. draw: **style** = draw & 3.
4. draw: **serial** = draw % 10000 (the running serial number of §6).
5. If C's warmth > 0.81 (single-precision constant at RVA 0x31dd18): draw: style = 4 + (draw & 1).
6. **Level range** of C (RVA 0x1ee4b0, called at RVA 0x1da44b with the minimum preset to 1).
   The routine sets (min, max) = (1, 10), then applies, in this order, each rule whose
   condition holds, a later rule replacing an earlier one: warmth < 0.2 → (10, 20);
   moisture < 0.2 and warmth > 0.8 → (15, 25); moisture > 0.8 and warmth > 0.8 → (10, 20);
   fire flag (byte +0x08) = 1 → (20, 30). 0.2 and 0.8 are the single-precision constants at
   RVA 0x2fcdbc and 0x2fcdc8; all comparisons are strict. Region preparation uses only the
   minimum (**minLevel**, §4.2); the maximum is not read by RVA 0x1da280.

Draws in the header: 2, plus 1 when tier > 4, plus 1 when warmth > 0.81.

---

## 4. Slot pass

Let (cxs, cys) = ((C.x >> 11) & 7, (C.y >> 11) & 7), the slot containing C's point (C.x and C.y
are the cell point's block coordinates; regions spec).

For i = 0..7 (outer) and j = 0..7 (inner), with R = record 8·i + j:

### 4.1 The cell slot of a named world: the town (type 1)

If the world is named and (i, j) = (cxs, cys):

1. R.type = 1; R.variant (+0x1c) = region style; R.id (+0x20) = C's cell id.
2. draw: R.r = float(draw % 200 + 512) (512..711).
3. R.centre = (C.x · 65536, C.y · 65536) as fx.
4. Four edge corrections, in this order, each a signed 64-bit fx comparison, with
   q = trunc64(f32(r·65536)):
   * if R.x − q − 2²⁴ < ox·65536: R.x = trunc64(f32(f32(f32(float(ox) + r) + 256)·65536));
   * if R.y − q − 2²⁴ < oy·65536: R.y likewise with oy;
   * if R.x + q + 2²⁴ > (ox + 2048)·65536: R.x = trunc64(f32(f32(f32(float(ox + 2048) − r) − 256)·65536));
   * if R.y + q + 2²⁴ > (oy + 2048)·65536: R.y likewise with oy + 2048.
5. R.h = H(blk(R.x), blk(R.y)); a negative h is replaced by 0.
6. Nothing else: no owner test, the town is **not** put on the candidate list, and it keeps
   level 1 and sub-level 0 from the constructor.

One draw. In an unnamed world the cell slot is handled like every other slot (§4.2).

### 4.2 Every other slot

1. If owner(ox + 1024, oy + 1024) is not C, the slot is skipped: no draws, and the record keeps
   every constructor value (centre (0, 0), level 1).
2. draw: R.r = float((draw & 255) + 512) (512..767).
3. **Placement** with radius r (reused by §6.2 case 3). With r256 = f32(r + 256) and
   span = f32(2048 − f32(r256 · 2)) (2..512):
   * draw: R.y = oy·65536 + trunc64(f32(r·65536)) + 2²⁴
     + trunc64(f32(f32(f32(float(draw)·span) / 32767)·65536));
   * draw: R.x = ox·65536 + trunc64(f32(r256·65536))
     + trunc64(f32(f32(f32(float(draw)·span) / 32767)·65536)).
   The **y draw comes first**. Both axes place the centre at origin + r + 256 + u·span with
   u = draw/32767, so the disc plus a 256-block margin lies inside the slot.
4. R.h = H(blk(R.x), blk(R.y)), not clamped here. **R.level (+0x24) = minLevel** (§3 step 6;
   store at RVA 0x1daa55).
5. If owner(blk(R.x), blk(R.y)) is not C, the slot ends here: the record keeps its centre,
   radius, h and minLevel, its type stays 0 and it is not a candidate.
6. **Candidate key**: dx = float(cxs − i), dy = float(cys − j),
   d = the double-precision square root of f32(f32(dy²) + f32(dx²)) rounded to single precision;
   draw: key = f32(f32(f32(f32(float(draw)·4) / 32767) + d) − 2). The entry (key, i, j)
   (12 bytes) is appended to the candidate list (RVA 0x1f4d10, an ordinary append).

Draws per slot: 0 (skipped), 3 (centre not owned) or 4 (candidate). The key is the slot's
distance, in slots, from the cell's slot plus a random value in 0..4, minus 2.

---

## 5. Candidate sort

The list is sorted by key, **ascending**, by RVA 0x1c2270 (the C++ library introsort: lists of
more than 32 entries are partitioned recursively; ranges of at most 32 entries are finished by an
insertion sort). Only the key is compared. The insertion sort moves an entry only
past strictly larger keys, so for a list of at most 32 entries equal keys keep slot order; for
longer lists see unresolved item 1.

The list holds 0..63 entries in a named world (the town slot is never a candidate) and 0..64 in
an unnamed world.

---

## 6. Queue pass: ranked assignment

A counter c runs from 0 to 63. At each value, if the list is empty the pass ends; otherwise odd c
does nothing, and even c removes the **first** entry (smallest key, remaining order kept) and
assigns it. At most 32 candidates are assigned, at ranks c = 0, 2, 4, …, 62.

For the removed entry (key, i, j), with R = record 8·i + j:

1. If R.h < 0: R.h = 0.
2. **Level**: t = f32(float(c) · 0.015625) (= c/64 exactly), passed to RVA 0x462f0, which
   returns f32(f32(f32(1 / f32(1 − t)) − 1) · 20) + 1 (single precision; 20 at RVA 0x2fcd74);
   the result is truncated toward zero (RVA 0x28d910) and **replaces R.level** (store at
   RVA 0x1dacb1). No minimum or maximum is applied. For c = 0, 2, 4, …, 62 the levels are
   1, 1, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 14, 16, 18, 21, 23, 26, 30, 34, 39, 45, 52, 61,
   72, 87, 109, 141, 194, 301, 621.
3. If (c / 2) is odd (c = 2, 6, 10, …): **dungeon entrance**, §6.1. Otherwise (c = 0, 4, 8, …):
   the **generic switch**, §6.2.

### 6.1 Type 14 (dungeon entrance; ranks c ≡ 2 mod 4)

1. R.type = 14; R.r = 150.
2. The centre is moved to the centre of its zone: for y and then x, the block coordinate
   V = blk(centre) becomes V′ = (V rounded toward zero to a multiple of 256) + 128 and the
   centre becomes V′·65536. h is **not** recomputed.
3. Zone (zxl, zyl): each index is trunc64(double(centre fx) · 1/256) divided by 65536 (signed,
   toward zero), taken modulo 64 (= (V′ >> 8) & 63 for these positive values). Table entry E of
   that zone: E.kind = 3; draw: E.variant = draw & 3.
4. With the zone centre (Xc, Yc) = ((64·rx + zxl)·256 + 128, (64·ry + zyl)·256 + 128) (equal to
   the moved centre): if the warmth blend (RVA 0x1c4dd0) at (Xc, Yc) exceeds 0.8, then
   E.variant = 4 if the moisture blend (RVA 0x1c4800) at (Xc, Yc) exceeds 0.8, otherwise 5.
   No draw.
5. draw: E.id = draw % 10000000 + 1.
6. E.level = R.level.
7. R.serial (+0x20) = serial; draw: serial ← serial + 1 + draw % 50.
8. R.variant (+0x1c) = E.variant.
9. **Sub-level rule** (shared with §6.2), here written to E.sub-level (+0xc): level < 5 → 0
   with no draw; level < 10 → draw & 1; level < 15 → draw % 3; level < 19 → draw % 3 + 1;
   otherwise (draw & 3) + 1.

R.sub-level (+0x28) is not written. Draws: 3, plus 1 if level ≥ 5.

### 6.2 Generic switch (ranks c ≡ 0 mod 4)

draw: s = draw & 7 (jump table at RVA 0x1dc6bc). "Sea" means C's elevation is negative.

| s | Type | Further work, in order |
|---|---|---|
| 0 | 2 | — |
| 1 | 3 | draw: R.variant = draw % 3 |
| 2 | 4, or **15** if sea | — |
| 3 | 5 | R.variant from C's climate (below); draw: R.r = float((draw & 255) + 256) (256..511); then the placement of §4.2 step 3 again in the record's own slot (y draw, then x draw) with the new r. h is **not** recomputed. |
| 4 | 6, or 15 if sea | — |
| 5 | 7, or 15 if sea | — |
| 6 | 11 | R.r = 128; centre moved to its zone centre as in §6.1 step 2 |
| 7 | 12 | R.r = 128; centre moved to its zone centre as in §6.1 step 2 |

Type-5 variant: if C.warmth > 0.8: 3 if C.moisture > 0.8, else 2 if C.moisture < 0.2, else 0;
if C.warmth ≤ 0.8: 0.

Then, for every s:

1. Sub-level rule of §6.1 step 9, written to **R.sub-level (+0x28)**.
2. R.serial (+0x20) = serial; draw: serial ← serial + 1 + draw % 50.

Draws: 1 (switch), plus 1 for s = 1 or 3 for s = 3, plus 1 if level ≥ 5, plus 1.

### 6.3 Location types made by region preparation

| Type | Made by |
|---|---|
| 1 | §4.1, the cell slot of a named world (at most one per region) |
| 2 | §6.2, s = 0 |
| 3 | §6.2, s = 1 |
| 4 / 15 | §6.2, s = 2 (15 when C is sea) |
| 5 | §6.2, s = 3 |
| 6 / 15, 7 / 15 | §6.2, s = 4 and s = 5 (15 when C is sea) |
| 10 | §7 |
| 11 | §6.2, s = 6 |
| 12 | §6.2, s = 7 |
| 13 | never written by RVA 0x1da280 |
| 14 | §6.1 (dungeon entrance; its zone gets kind 3) |

The terrain, vegetation and structure effects of each type are specified by the companion specs
and are not restated here.

---

## 7. Type-10 pass

After §6, the entries still on the list (the largest keys, because ranks were taken from the
front) supply up to five type-10 sites. With a counter n = 0, while the list is not empty:

1. draw: k = draw modulo the list length (unsigned remainder); entry k is removed (remaining
   order kept).
2. R.type = 10; draw: R.id (+0x20) = draw % 10000000 + 1.
3. The table entry of the zone of R's centre, ((blk(R.x) >> 8) & 63, (blk(R.y) >> 8) & 63),
   gets kind = 4 and id = R.id.
4. n ← n + 1; the pass ends when n = 5.

R keeps its radius (512..767), its unclamped h and **level = minLevel** from §4.2. A list of at
most 32 candidates is emptied by §6 and yields no type-10 site.

---

## 8. Levels in the per-zone table

For each record in index order 0..63 (RVA 0x1db5c8..0x1db96c, unrolled by eight), the table
entry of the zone containing its centre, ((blk(x) >> 8) & 63, (blk(y) >> 8) & 63), gets
**level = record level (+0x24)**. Each placed record's centre lies in its own slot and outside
zone (0, 0), so the only collisions are at local zone (0, 0): records never placed (centre (0, 0),
level 1) write level 1 there. This pass therefore carries into the table the levels of ranked
records (rank formula), of type-10 records, unused candidates and records whose centre was not
owned (minLevel), and of the town (1). Dungeon entries already received the same value in §6.1.

### 8.1 Discarded height samples (RVA 0x1db972)

For zone (a, b), a = 0..63 (outer) and b = 0..63 (inner), H is evaluated at
(16384·rx + 256a, 16384·ry + 256b), then (+256, +0), (+0, +256), (+256, +256), and each result is
discarded. Zones with (64·rx + a + 5·(64·ry + b)) mod 13 = 0 are appended (a, b) to a list that
is released at the end of the routine without being read. No call of `rand` is reachable from
H, the climate blends, the owner routine or f(L, X, Y) through direct calls (indirect calls on
these paths are CRT helpers only).

### 8.2 Town quarters (RVA 0x1dbbeb)

For every record of type 1 (at most one), in index order i = 0..7, j = 0..7:

1. For a = 0..7 (outer) and b = 0..7 (inner), zone (8i + a, 8j + b) of the town's slot has centre
   (X, Y) = (16384·rx + 2048·i + 256·a + 128, 16384·ry + 2048·j + 256·b + 128);
   f = f(L, X·65536, Y·65536) (RVA 0x1f8e10, regions spec); v = 1 − f; w = v² if v > 0, else 0.
   The entry (w, 8i + a, 8j + b) is appended.
2. The 64 entries are sorted by w, **descending** (RVA 0x1c24f0, the same library introsort with
   the comparison reversed; only w is compared).
3. The zones of the first four entries get kind = 1 and variant = 1, 2, 3, 4 in that order.

---

## 9. Stored mission and monster data

If the byte at world + 0xb4 is 0 (copied from the world constructor's second argument,
RVA 0x18eb00) and the world is named, then for each slot in record order (i outer, j inner) the
routine forms names from the text "mission" or "monster", the sub-cell coordinates
lx = 8·rx + i and ly = 8·ry + j and the separator "_", and opens the stored data of that name.
When the mission data is present it is read into the record's mission area (+0x2c) and
region + 0x15a18 is set to 1; when the monster data is present it is read into the monster area
(+0x54). No draws and no effect on placement, types or levels. The stored format is outside this
document.

---

## 10. Location lookup by column

The record for block column (x, y) is record 8·((x >> 11) & 7) + ((y >> 11) & 7) of region
(x >> 14, y >> 14), when that region is present (regions spec; the inline form in H is cited in
§2.2).

---

## 11. Type-10 sites in the mountain-cover factor (RVA 0x1c3ef0)

RVA 0x1c3ef0 subtracts two fx points (two 64-bit differences). Its only caller, RVA 0x1f0584
inside RVA 0x1effa0 (the column routine that looks up the column's location through RVA 0x87da0),
uses it for a type-10 record with r > 0: (dX, dY) = (L.x − x·65536, L.y − y·65536), the offset
to the location centre without noise. Each difference is converted to single precision with
round-to-nearest and multiplied by 2⁻¹⁶ (RVA 0x2fcd94), giving ex and ey;
d = f32(f32(ey²) + f32(ex²)) / f32(r²); if d < 1 the factor M held by that routine becomes
f32((1 − f32((1 − d)²)) · M). A type-10 site is thus a round hole of radius r in that factor.
The role of M in the terrain is given by the terrain spec.

---

## 12. Answer: climate minimum level and location records

The climate-based minimum **is** applied, once, during the slot pass. RVA 0x1ee4b0 (called at
RVA 0x1da44b, after the header draws and before any slot) derives minLevel from C's warmth,
moisture and fire flag: 1, 10, 15, 10 or 20 (§3 step 6). The slot pass stores minLevel at
RVA 0x1daa55 into +0x24 of every record of a non-town slot whose slot centre is owned by C,
right after its height and before the centre-owner test, so it reaches both records that are
queued as candidates and records whose centre is not owned (never queued). Skipped slots and the
town keep the constructor value 1. The queue pass (RVA 0x1dacb1) later **overwrites** +0x24 of
the up to 32 ranked records with the rank formula, with no minimum applied. Type-10 records,
unused candidates and non-queued records keep minLevel, and §8 copies every record's +0x24 into
its zone's table level, so minLevel reaches the zone table for those zones. The maximum of the
range is never read by region preparation. The regions spec's "+0x24 = 1" for the slot pass is
therefore exact only when minLevel = 1 (a climate where none of the four rules applies).
Server.exe behaves the same at these points (§13).

RVA 0x1ee4b0 is also called from RVA 0x1af170 (around RVA 0x1bcb5d); that use is outside this
document.

---

## 13. Server.exe counterpart

Server.exe RVA 0x10e080 has the same length (0x2439 bytes) and the same observed structure at the
points checked: level range RVA 0x122290 (same constants and rule order as RVA 0x1ee4b0), called
at RVA 0x10e24b with the minimum preset to 1; slot-pass level store at RVA 0x10e855; rank level
store at RVA 0x10eab1; world-name test at world + 0xa4 and stored-data flag at world + 0xb4;
region pointer store at RVA 0x110428. A complete instruction-by-instruction comparison was not
made (unresolved item 2).

---

## 14. Unresolved

1. **Ties in the two sorts.** Lists longer than 32 entries (candidate lists of 33..64 entries;
   the 64-entry town-quarter list) are partitioned by the library introsort, which does not keep
   the order of equal keys. The order of tied entries in that case is not specified here.
2. **Server.exe** RVA 0x10e080 was compared only at the points listed in §13.
3. **Type 13**: no write of type 13 occurs in RVA 0x1da280; whether another routine or stored
   data creates it was not examined.
4. Whether helper routines reached from H consult location records of slots other than the
   column's own (which would make neighbouring, already-prepared regions visible to the samples
   of §8.1 near the region's far edges) was not traced; those samples are discarded.
5. The meanings of h10 (+0x10), of the type-5 variant and of the mission and monster areas, and
   the consumption of the per-zone table by the zone generator, belong to other specs.

---

## Re-certification (2026-11-15)

Every remaining factual statement was re-read against the Cube.exe image bytes, the full
disassembly of RVA 0x1da280..0x1dc6b8, its helpers (0x1a4710, 0x1ee4b0, 0x1c3ac0, 0x1c3bb0,
0x1f4d10, 0x1c2270 with its insertion sort 0x1c0e10, 0x1c24f0 with 0x1c0f80, 0x462f0, 0x28d910,
0x28d946, 0x1c3ef0 and its caller), the jump table at 0x1dc6bc, the constants named above, the
MSVCR110 import table (`rand`, `srand`, `sqrt`), the world constructor fields, and the Server.exe
points of §13.

**Confirmed**: object and record sizes; constructor values (with the precisions noted below);
step order; seed formula; header draws and tier arithmetic; the 0.81 hot-region rule; the level
range rules and their order; the town slot (draw, centre, four edge corrections in order, height
clamp, no owner test, no level write); slot-centre and record-centre owner tests; placement
arithmetic with the y draw first; minLevel stored at +0x24 in the slot pass; candidate key;
ascending sort on key only; queue-pass rank stepping, height clamp, level formula and the 32
level values; type-14 work and draw order; the generic switch table and draw order; sub-level
thresholds; serial update; the type-10 pass; the table level copy; the discarded samples and
their mod-13 list; town quarters with descending sort; stored-data names and flags; region
pointer stored last; type-10 hole arithmetic.

**Corrected** (old → new):

1. Addresses were virtual addresses → now RVAs (image base 0x400000 subtracted); image hash
   given in full.
2. Per-zone entry bytes +0xd..+0xf "initially 0" → not written by the constructor.
3. Record "every field starts at 0 except level" → bytes +0x42..+0x43 and +0x5d..+0x67 are not
   written by the constructor.
4. Region +0x04 was not listed → listed as not written.
5. Candidate list "at most 63 entries" → 63 in a named world, 64 in an unnamed world (§5, §14).
6. "All positions sampled during preparation lie inside the region" → true for the slot,
   queue and climate samples; the discarded samples of §8.1 include columns of neighbouring
   regions (§2.2).
7. Sort ties: added that lists of at most 32 entries keep slot order for equal keys (insertion
   sort); only longer lists remain open.
8. Type-10 hole: added the r > 0 condition and named the caller RVA.
9. Header draw count "2 or 3 (+1 when tier > 4)" restated as 2 + (tier > 4) + (warmth > 0.81)
   (same count, clearer).
10. Out-of-range region coordinates return null (was not stated).

No correction changes world generation, placement or random-draw order. Nothing was reversed;
the minLevel store in the slot pass, already stated by the inherited text, is confirmed.

**Removed**: the test vectors (former §12, produced by a model, not native evidence); the
running-game checking procedure (former §13); advice that a replacement may skip the discarded
height samples; "in normal play the world is named" and the test-vector assumption; verification
suggestions in the open questions; the "probable role" and behaviour column of the type table
and the summary of how other generator parts read the records (behaviour pointers to other
specs, not re-checked here); answers to and corrections of the regions spec's former open
questions, which referred to an older revision of that spec; the roads claim about link
channels; the zone-kind-5 remark and the zone-entry copy offsets attributed to the zone
generator.

**Unresolved**: §14 items 1–5.

**Specific question**: answered in §12. A climate-based minimum level (1, 10, 15 or 20 from
RVA 0x1ee4b0) is stored into +0x24 at RVA 0x1daa55 during the slot pass for every non-town record
whose slot centre is owned by C, queued or not; the queue pass overwrites it for the up to 32
ranked records without any minimum. Records that keep it (type-10 sites, unused candidates,
records whose centre is not owned) carry it into the per-zone table levels through §8.

Reconstruction consulted: none.
