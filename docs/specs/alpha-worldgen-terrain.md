Status: FINAL / STABLE / HELD

# Alpha world generation: terrain, blocks and colours

Behavioural specification of how Cube World **Alpha** turns the climate fields and
locations of the [regions spec](alpha-worldgen-regions.md) into the voxel columns of a
zone: the shaping fields (town-ness, flatness, roughness, mountain mask), the base height
function, the per-column surface pass, ravines and lake basins, the block format and every
block colour rule. Evidence: the client image `Cube.exe` (SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x400000),
mainly the zone generator at RVA 0x1e4850 (37430 bytes, examined in decompiled and
disassembled form) and the routines it calls. Every address below is an RVA in `Cube.exe`
unless `Server.exe` is named. The noise routine, the world offsets and the precision rules
are those of the [random spec](alpha-worldgen-random.md).

---

## 0. Conventions

* **N(a, b)** is the two-argument noise routine (RVA 0xc0ef0, random spec). Both arguments
  are doubles; every product coordinate·k and every sum with a constant or offset is formed
  in double, and the result is used as a float.
* **On** (for example O170) is the signed 32-bit world field at world + 0x800000 + n,
  converted to double before it is added. **A** is the 32-bit world field at
  world + 0x800188.
* All other arithmetic is **float** unless marked double. Integer → float conversions are
  exact for block coordinates below 2²⁴.
* (x, y) is a block column, z a block height. +Z is up. **Sea level is z = 0**: every
  non-solid cell at z ≤ 0 reads as water (§1.4).
* Helpers: **sat(t)** clamps to [0, 1]; **smooth(t)** = t·3·t − t·2·t·t (evaluated in that
  form); **unit(n)** = (n + 1)·0.5; **bump(f)** = (1 − f)² if 1 − f > 0, else 0.
* **f(L, x, y)** is the location distance (RVA 0x1f8e10, regions/locations specs) of
  location L evaluated at the 64-bit fixed-point point (x << 16, y << 16).
  **L(x, y)** is the location record of the column's 2048-block sub-cell (lookup RVA 0x87da0,
  x ÷ 2048 and y ÷ 2048 truncating, valid for 0 … 8191), possibly absent. "type" is its
  32-bit field at +0x18, "radius" its float at +0x10, "height" its float at +0x14, and its
  first 16 bytes are the 64-bit fixed-point centre (L.x, L.y).
* **Climate of a column**: moist(x, y) (RVA 0x1c4800), warm(x, y) (RVA 0x1c4dd0) and
  fire(x, y) (RVA 0x1ef040). Routines that need them read the column record (§1.2) when the
  column exists and otherwise call these routines; the stored values are the same routines'
  results converted to float.

---

## 1. Records

### 1.1 Zone (200 bytes, constructor RVA 0x2240f0)

A zone is 256 × 256 columns; zone (zx, zy) covers x in [256·zx, 256·zx + 256), y likewise,
with 0 ≤ zx, zy < 65536. Zones are found through their region (regions spec §1): the
region object holds 64 × 64 zone pointers at region + 0x10018 + 4·(64·(zx mod 64) +
(zy mod 64)) (lookup RVA 0x34d10; the region is the one with index (zx ÷ 64, zy ÷ 64)).

| Offset | Meaning |
|---|---|
| +0x18, +0x24 | two object containers filled by the post passes (roles: unresolved, see §8) |
| +0x60, +0x64 | zx, zy (32-bit) |
| +0x78 … +0x87 | 16 bytes copied from the region's per-zone table (§5.7) |
| +0xa8 | pointer to 65536 column records (32 bytes each) |

### 1.2 Column (32 bytes, at zone+0xa8 + 32·((x mod 256) + 256·(y mod 256)); lookup RVA 0x347a0, valid for 0 ≤ x, y < 2²⁴)

| Offset | Type | Meaning |
|---|---|---|
| +0x04 | float | moisture of this column |
| +0x08 | float | warmth |
| +0x0c | float | fire share |
| +0x10 | int32 | **bottom**: z of the first stored block |
| +0x14 | int32 | surface z − 8 (use not traced) |
| +0x18 | pointer | stored blocks, 4 bytes each, from z = bottom upwards |
| +0x1c | int32 | **count** of stored blocks |

### 1.3 Block (4 bytes)

Bytes 0, 1, 2 are R, G, B (0..255). Byte 3: bits 0–4 **type**, bit 5 (0x20) set on blocks
coloured by the surface-colour routine, bit 6 (0x40) "dry air" (see §1.4, §2.3), bit 7
(0x80) a "keep" marker: when a block is written at an index inside the existing storage
(or at or above its end) and the stored block there has bit 7 set, the write happens only
if the new block's type (bits 0–4) is non-zero, and bit 7 is then set on the result;
otherwise the four bytes are written plainly. Writes that extend the storage downwards are
always plain. Newly allocated storage is zero-filled. The water constant used by the
generator has byte 3 = 0x82, so stored water written from it carries the keep marker.

Block types used by the generator (names inferred from use):

| Type | Use |
|---|---|
| 0 | air |
| 1 | implicit solid (grey 200, 200, 200) |
| 2 | water |
| 3 | lake bed |
| 4 | grass / ordinary ground (the surface routine's default) |
| 6 | rock, cliff, sub-surface |
| 9 | sand (desert, beach) |
| 10 | snow |
| 12 | fire ground |

### 1.4 Implicit blocks (reading a block, RVA 0x2f7e0)

* No column (zone missing, or outside the world): (200, 200, 200), byte 3 = 0x01.
* z < bottom: (200, 200, 200), byte 3 = 0x01.
* z ≥ bottom + count: air (255, 255, 255, byte 3 = 0x00) if z > 0, **water**
  (255, 255, 255, byte 3 = 0x82) if z ≤ 0.
* Stored block whose type is 0, with z ≤ 0 and bit 0x40 clear: read as that water.
  Otherwise the stored block is returned.

### 1.5 Writing a block (RVA 0x4e7a0)

Writing block b at (x, y, z) into a zone: nothing happens if the column is missing, or if
b's byte 3 is entirely zero and z ≥ bottom + count. If z ≤ 0 and (byte 3 AND 0x5f) = 0, b
becomes water (255, 255, 255, 0x82). The block is then stored at index z − bottom
(RVA 0x4e700), the storage being grown at the end or at the front as needed (bottom
decreases by the front growth). If the write was below the old bottom, every z′ from z + 1
up to old bottom − 1 is then written with the bytes trunc(Rk(x, y, z′)) (§6.6) and type 6.

---

## 2. Water, sea level and the shoreline

### 2.1 Sea level

There is no stored sea: water is every non-solid cell at z ≤ 0 (§1.4). Where the terrain
surface lies below z = 0 the column is under the sea. Cells carved by ravines (§5.5) at
z < 1 are written as explicit water blocks (byte 3 = 0x02).

### 2.2 Lakes

Location type 4 lowers the ground into a basin (§4.3) and the zone pass fills the inner
part of the basin with water blocks from trunc(h_L − 25) downwards (§5.6).

### 2.3 Dry air

Ravine carving (§5.5) writes its air cells (always at z ≥ 1) with bit 0x40 when the carve
top lies below the column's surface S, that is when the cavity keeps a roof. By §1.4 the
bit only changes how a type-0 cell reads at z ≤ 0; for the cells written here it has no
effect on reading. Other uses of the bit are not traced.

---

## 3. Shaping fields

### 3.1 Town-ness q(x, y) (RVA 0x1989d0)

q = bump(f(L, x, y)) if L(x, y) exists and has type 1, else 0.

### 3.2 Flatness W(x, y) (RVA 0x1f9f70)

1. w = B(x, y) (RVA 0x1eea60, regions spec §5.2); (g.x, g.y) = G(x, y), a pair of doubles
   (RVA 0x19fc90, regions spec §5.3).
2. If q > 0: a = min(3q, 1), a′ = 1 − (1 − a²)²; for g.x and then g.y:
   c = f32(cos(g·360)·a′ + 1), with g·360 in double, the cosine from the CRT
   (`_libm_sse2_cos_precise`), the product with a′ and the + 1 in double; w ← min(w, c).
3. If q > 0.65: w ← f32(max((0.7 − q)/0.05000000074505806, 0))·w, the quotient in double.
4. If L(x, y) has type 2 or 4: w ← 2·bump(f(L, x, y)) + w.

### 3.3 Roughness R(x, y) (RVA 0x1f9340)

1. v = f32(|f32(N(O170 + 0.01x, O174 + 0.01y))·0.1 + N(O168 + 0.001x, O16c + 0.001y)|),
   the float sum's magnitude taken in double.
2. v ← v · ((N(0.001x, 0.001y) + 1)·0.1 + 0.8) (no offsets in this noise).
3. t = 1 − 0.75·B(x, y); if t > 0: v ← t²·0.05 + v.
4. If L(x, y) exists: types 1, 2, 4, 13: v ← bump(f) + v; types 6, 7: v ← 0.5·bump(f) + v.
5. v ← fire(x, y) + v. Result v.

### 3.4 Mountain mask M(x, y) (RVA 0x1effa0)

1. a = N(0.01x, 0.01y); a′ = 1 − a².
2. b = f32(N(O1dc + 0.01x + 843, O1e0 + 0.01y + 984))·0.1;
   c = N(0.0025x + O1dc, 0.0025y + O1e0).
3. M = 1 − f32(|b + c|)·((1 − a′²)·1.3 + 2).
4. M ← M · (0.4·N(0.005x + 94, 0.005y + 874) + 0.6).
5. If L(x, y) has type 6 or 7, with f = f(L, x, y):
   s = 0 if M ≥ 1; else 1 − M if M + 0.5 > 1; else 0.5.
   * f ≤ 0.36: k = min((1 − f/0.36)·1.5, 1); M ← (M + s)·(1 − k²) + k²·0.
   * 0.36 < f < 1: r = f32(√f) (double square root of f); g = (r − 0.6)/d, where d is the
     float with bit pattern 0x3ECCCCCC (0.39999997615814209, one unit below the float
     nearest 0.4); M ← (1 − g²)²·s + M.
6. If M < 0: M = 0 and go to step 8.
7. M ← smooth(min(2R(x, y), 1))·M; M ← smooth(min(W(x, y), 1))·M;
   M ← (1 − smooth(min(2q, 1)))³·M.
8. Location adjustments (if L(x, y) exists):
   * type 2: **return** smooth(M) + bump(f) (no further step);
   * type 10 with radius ρ_L > 0: (dX, dY) = L's centre minus (x << 16, y << 16), as 64-bit
     integers (RVA 0x1c3ef0); each is converted to float and multiplied by 2⁻¹⁶;
     d = ((dY·2⁻¹⁶)² + (dX·2⁻¹⁶)²)/(ρ_L·ρ_L); if d < 1: M ← (1 − (1 − d)²)·M;
   * types 4, 5: f ≤ 0.25 → M = 0; 0.25 < f < 1 → g = (f32(√f) − 0.5)·2,
     M ← (1 − (1 − g²)²)·M.
9. Return smooth(M).

M is 0 on plains and near 1 on mountain ridges; it lifts the surface (§5.4) and decides
rock cover.

---

## 4. Base height H(x, y) (RVA 0x1c5e20; `Server.exe` counterpart RVA 0xf9b70)

### 4.1 Large-scale terms

1. s1 = unit(N(O18c + 0.0001x, O190 + 0.0001y))², s2 = unit(N(O194 + 0.0001x, O198 + 0.0001y))²,
   s3 = unit(N(O19c + 0.001x, O1a0 + 0.001y))², s4 = unit(N(O1a4 + 0.001x, O1a8 + 0.001y))²,
   s5 = unit(N(O1ac + 0.002x, O1b0 + 0.002y))².
2. g = smooth(min(4R(x, y), 1))²; m3 = g·s3, m4 = g·s4, m5 = g·s5.

### 4.2 Elevation blend

1. Displacement: δx = f32(f32(N(0.0005y, 3423)·3)·256), δy = f32(f32(N(0.0005x, 23421)·3)·256)
   (note the x displacement samples y, and the reverse). Fixed-point sample point:
   Px = (x << 16) + trunc64(f32(δx·65536)), Py = (y << 16) + trunc64(f32(δy·65536)),
   64-bit, truncation toward zero; the sub-block fraction is kept.
2. Window: cells (i, j) of the world's 1024 × 1024 cell table (pointers at
   world + 0x4000bc + 4·(1024i + j); cells of 16384 blocks) with
   i from (x − 16384) ÷ 16384 to (x + 16384) ÷ 16384 and j from (y − 16384) ÷ 16384 to
   (y + 16384) ÷ 16384 (÷ truncating, both bounds inclusive; note the window uses x, y, not
   Px, Py), i in the outer loop. Indices outside 0 … 1023 and **missing cells are skipped**.
   For each cell c (32-bit c.x at +0, c.y at +4, elevation c.e at +0x18):
   ex = f32((c.x << 16) − Px)·2⁻¹⁶, ey likewise with c.y and Py (64-bit differences
   converted to float), d² = ey² + ex² (float, **not** truncated). d0² = the smallest
   (the first strictly smaller value wins).
3. If a cell was found, a second scan in the same order: t = min((d² − d0²)·κ, 1) with κ the
   float constant at RVA 0x31dcd8 (bit pattern 0x3356BF95, 5.0000000584304871e-08),
   wc = (1 − t)². Σw += wc; Σe += (float)c.e·wc; if c.e > 0: Σl += wc.
4. **E** = Σe/Σw (blended elevation), **Lnd** = Σl/Σw (land share). If no cell was found:
   E = 0, Lnd = 1.

### 4.3 Assembly

1. n6 = N(O1bc + 0.0002x, O1c0 + 0.0002y), n7 = N(O1b4 + 0.0002x, O1b8 + 0.0002y);
   H = ((n6 + 1)·100·s2 + (n7 + 1)·100·s1)·Lnd + E.
2. If q > 0.5: t = min((q − 0.5)·2, 1); m5 ← (1 − smooth(t))·m5.
3. w = W(x, y); c = min(max(w, 0.02)·2, 1); u = 1 − c; g′ = 1 − u⁴;
   A = (0.1g′ + 0.9)·m3, Bm = (0.1g′ + 0.9)·m4, C = (0.5g′ + 0.5)·m5.
4. If L(x, y) has type 1: k = 1 − 0.5·bump(f); A, Bm, C ← k·A, k·Bm, k·C.
5. H ← ((N(O1cc + 0.002x, O1d0 + 0.002y) + 1)·50·Bm + (N(O1c4 + 0.002x, O1c8 + 0.002y) + 1)·50·A)
   + (N(O1d4 + 0.01x, O1d8 + 0.01y) + 1)·20·C + H, summed in that order.
6. **Location rules**, if L(x, y) exists with type ≠ 0, in this order:
   * **Island.** Let O = the cell owning the location centre (RVA 0x77e10 at L.x ÷ 65536,
     L.y ÷ 65536, 64-bit division truncating). If O exists, O.e < 0 and type ≠ 11:
     ax = 0.0025x, ay = 0.0025y (double);
     DX = (x << 16) + trunc64(f32(f32(N(ax + 8432984, ay + 90493)·100)·65536)) − L.x;
     DY = trunc64(f32(f32(f32(N(ax + 3423, ay + 112)·100) + (float)y)·65536)) − L.y
     (y is added in float here, x in 64-bit integers above);
     ex = f32(DX)·2⁻¹⁶, ey = f32(DY)·2⁻¹⁶; ρ = min((ey² + ex²)/((ρ_L + 256)·(ρ_L + 256)), 1)
     with ρ_L the location radius; if 1 − ρ > 0: t = min((1 − ρ)·1.1, 1);
     H ← H − smooth(t)·(float)O.e. Such locations therefore raise land by up to −O.e blocks.
   * **Type 4 (lake basin)**, f = f(L, x, y), h_L = the location height: f ≤ 0.25 →
     u = (1 − 4f)², H = (1 − u)·(h_L − 25) + (h_L − 50)·u; 0.25 < f < 1 → s = f32(√f),
     v = (1 − ((s − 0.5)·2)²)², H ← ((h_L − 25) − H)·v + H.
   * **Types 6, 7 (valley)**: f ≤ 0.25 → u = (1 − 4f)², H = (1 − u)·(H + 10) + (H − 30)·u;
     0.25 < f < 1 → H ← v·10 + H (v as above); then in all cases H ← max(H, E).
   * **Type 13 (spire)**: f ≤ 0.01 (float 0.010000000707805157) → result H + 150;
     0.01 < f < 1 → w = (f32(√f) − 0.1)/0.9, u = 1 − w², result u⁴·150 + H; otherwise H.
7. Return H (or the type-13 result).

H is a float height in blocks; values near 0 are the coast.

---

## 5. Generating a zone (RVA 0x1e4850; `Server.exe` counterpart RVA 0x118630)

### 5.1 Order

For zone (zx, zy) with 0 ≤ zx, zy ≤ 65535:

1. Prepare regions (zx ÷ 64) + i, (zy ÷ 64) + j for i = −1..1 (outer), j = −1..1 (inner)
   (regions spec §6.1). If the zone is then already present in its region table, stop.
2. Create the 200-byte zone record; `srand(A + 65536·zy + zx)` (CRT `srand` of
   MSVCR110; every later draw in this specification is that CRT's `rand`); set +0x60/+0x64.
3. Compute (not used further in the parts specified here) the owning cell of the point
   (256zx + 128, 256zy + 128) (RVA 0x77e10) and trunc of both components of G there; note
   the zone's own location **Lz** = location of sub-cell (zx ÷ 8, zy ÷ 8), which is L(x, y)
   for every column of the zone. A zone-wide count of P > 0.25 columns, a grass count and a
   running minimum and maximum of zs (both starting at 0) are kept.
4. Climate pass (§5.2). 5. Base-height grid (§5.3). 6. Surface pass (§5.4–§5.6), which also
   makes the ravine spawn draws (§5.5). 7. Further passes (§5.7).

### 5.2 Climate pass

For x = 256zx … 256zx + 255 (outer), y = 256zy … 256zy + 255 (inner): column +0x04 =
moist(x, y), +0x08 = warm(x, y), +0x0c = fire(x, y), in that call order.

### 5.3 Base-height grid

A 257 × 257 float grid h = H(256zx + i, 256zy + j) for i, j = 0..256 inclusive, x (i) in
the outer loop (so the extra row and column come from the neighbouring zones' ground).

### 5.4 Surface pass (per column)

x in the outer loop, y inner, over the 256 × 256 columns. With h = h(x, y):

1. **Steep** = |h − h(x+1, y)| > 0.3 or |h − h(x, y+1)| > 0.3 (differences in float,
   magnitude through double; the second test only if the first fails).
2. P = M(x, y). If P > 0.25 the zone's P counter is incremented.
3. **Ridge pattern** T:
   v1 = 1 − |N(0.04x + 432, 0.04y + 432) + 0.05·N(0.08x + 432, 0.08y + 432)| (the noise
   results widened to double, sum and magnitude in double, then float),
   v2 = 1 − |0.05·N(0.08x + 4234, 0.08y + 234) + N(0.05x + 423432, 0.05y + 54352)| (likewise),
   T = (1 − v2³)² + ((1 − v1³)² + 0). The four noises are evaluated in the order written.
4. If Lz has type 6 or 13: k = min(2·bump(f(Lz, x, y)), 1), k′ = 1 − (1 − k²)²,
   m = sat(1.3·N(0.01x + 985, 0.01y + 98584) + 1), P′ = smooth(m)·k′·0.4 + P; else P′ = P.
5. G = P′·T; **cover** = smooth(sat(1.25·G)) (rock cover).
6. **Surface** S = f32(f32((N(0.01x + 34432, 0.01y + 8992) + 1.5)·60)·P) + 8G + h
   ((n + 1.5)·60 in double; note P, not P′; sums in the order written); zs = trunc(S).
7. **Colour**: c = surface colour(x, y, zs) with type t (§6.1), using the column's stored
   moisture and warmth. If cover > 0.5, t = 6.
   c′ = trunc(c·(1 − cover) + Rk(x, y, zs)·cover) per channel (c as byte values).
8. Column bottom = zs. If S < (float)minimum the minimum becomes zs; r8 = min(8P′, 1); if
   (float)maximum < S the maximum becomes zs.
9. **Blocks** (stored indices 0 and 1, so z = zs and zs + 1): block at zs: if steep,
   (c′, t); else trunc(Cl(x, y, zs)·(1 − r8) + (c′ + Rk(x, y, zs))·(0.5·r8)) per channel
   with type 6 (§6.7). Block at zs + 1: (c′, t). So the ground's top face is at z = zs + 2,
   and in flat ground the block under the top one is cliff-coloured.
10. If (t AND 0x1f) = 4, the zone's grass counter is incremented.
11. Column +0x14 = zs − 8.

### 5.5 Ravines (same column, after §5.4)

1. Lr = (N(0.01x + 435, 0.01y + 847) + 1)·20 + h. If S ≤ Lr, go to §5.6.
2. v = 1 − f32(|N(0.005x + O178, 0.005y + O17c) + N(O178 + 0.01x, O17c + 0.01y)|)·4; if
   v < 0, go to the next column (§5.6 is skipped too).
3. u = 1 − v²; w1 = 1 − u²; w2 = 1 − u⁴.
4. t = sat((10·N(0.03x + 7635, 0.03y + 123847) + ((S − Lr) − 20))/10); k = 1 − (1 − t²)²;
   a7 = (1 − t)⁷.
5. **top** = ((20P + 4)·w1 + Lr)·(1 − a7) + (S + 2)·a7;
   mfl = 6·T·P + Lr; **floor** = (1 − w2)·S + w2·mfl + 2.
6. If mfl < S − 2: for i = 0, 1 (z = zs + i): read the block (rules of §1.4) and write
   (b·k + Rk(x, y, trunc(i + S))·(1 − k)) per channel, truncated, keeping b's byte 3.
7. If top ≤ floor, go to the next column (§5.6 is skipped).
8. Carve: for z = trunc(floor) while (float)z < top: write water (0, 0, 0, byte 3 = 0x02)
   if z < 1, else air (0, 0, 0, byte 3 = 0x40 if top < S, else 0x00) (§1.5 rules apply; a
   carve below the bottom extends the column downwards).
9. Floor block at zf = trunc(floor − 1): c2 = surface colour(x, y, zf) with its type;
   type 6 if cover > 0.5 or k > 0.5; c3 = trunc(c2·(1 − k) + (Cl(x, y, zf) + Rk(x, y, zf))·(0.4k));
   c4 = trunc(c3·(1 − cover) + Rk(x, y, zf)·cover); write c4 with that type at zf.
10. **Ravine spawns**: if top − floor > 2 and (x + 3y) mod 7 = 0: draw r1; if r1 mod 30 ≠ 0
    go to §5.6; else draw r2 and select by r2 mod 4:
    * 0: if top ≤ S: draw r3; if r3 mod 4 = 0, a record is built (RVA 0x4a970), a rotation
      draw r4 gives r4·6.283185307179586/32767 (double, stored as float), and the record is
      submitted (RVA 0x4d590) (vegetation spec §4).
    * 1: if top ≤ S: a 0x10f0-byte object is constructed (RVA 0x1adb20); draw r3; its 32-bit
      field +0x2c is set from r3 mod 10: 0 → 131, 1 → 133, 3 → a further draw r4 with
      r4 mod 100: 0 → 138, 1–3 → 137, 4–8 → 136, otherwise 135; any other value → 132; then
      the object is submitted (RVA 0x26add0).
    * 2: if top ≤ S: the same object type with field +0x2c = 59, no draw at this level.
    * 3: (no top ≤ S test) an entry from RVA 0x1a6b60 at (x, y, trunc(floor + 2)).
    These draws consume the zone's random stream; draws inside the called routines are
    unresolved (§8). Then §5.6.

### 5.6 Lake fill (same column)

If Lz has type 4 and f(Lz, x, y) < 0.25: for z = trunc(h_Lz − 25) downwards while
z ≥ column bottom: if the block there reads (§1.4) with type 0 or 2, write water
(255, 255, 255, 0x82); otherwise write (trunc(Rk(x, y, z)), type 3).

### 5.7 After the column passes

The height grid is freed; the zone copies its 16-byte entry at region + 0x18 +
16·(64·(zx mod 64) + (zy mod 64)) into +0x78 … +0x87; further passes follow (not
specified here). After them, block light is recomputed over the zone's column area
(RVA 0x19a0e0 over [256zx, 256zx + 256) × [256zy, 256zy + 256)); further non-terrain steps
follow, and the zone pointer is stored into its region's zone table at the end.

---

## 6. Colours

All colour routines return three floats (R, G, B) on a 0..255 scale. "clamp" means: each
channel below 0 becomes 0, above 255 becomes 255. Bytes are trunc of the floats.

### 6.1 Surface colour (RVA 0x1c56e0), with moisture mo and warmth wm of the column

Start: type t = 4, colour C = grass(x, y) (§6.2), rk = sat((10R(x, y) − 0.3)·1.5).

1. **Hot rock.** If wm > 0.8: a = min((wm − 0.8)/0.1, 1); n = sat(N(0.03x, 0.03y));
   b = (1 − (1 − n²)²)·0.8·a; C ← C·(1 − b) + Rk(x, y, z)·b.
2. **Desert.** If mo < 0.2 and wm > 0.75: t = 9; d = sat((1 − mo/0.2)·(wm − 0.75)·4·10);
   if L(x, y) has type 4: g = sat(f(L, x, y) − 0.5), d ← (1 − (1 − g²)²)·d; d ← d·rk;
   if d > 1 then d = 1, else if d < 0.5 then t = 4; C ← C·(1 − d) + sand(x, y)·d.
3. **Beach.** g = sat(N(0.01x + 854, 0.01y + 985) + (float)(15 − z)/10 − 0.5); g′ = 1 − (1 − g²)²;
   if wm < 0.2: g′ ← g′·(wm/0.2). If g′ > 0: C ← C·(1 − g′) + beach(x, y)·g′, t = 9.
4. **Snow.** If wm < 0.3: k = min(1 − (wm − 0.29)/0.01, 1); t = 10;
   C ← C·(1 − k) + snow(x, y)·k.
5. **Fire.** F = fire(x, y); if F > 0: C ← C·(1 − F) + Rk(x, y, z)·F; if F > 0.5, t = 12.
6. Output bytes trunc(C), type byte t OR 0x20.

### 6.2 Grass (RVA 0x1ee540)

a = unit(N(0.04x, 0.04y)); p = 120·N(0.005x + 45645, 0.005y + 456456);
k = 80·N(0.02x + 89648, 0.02y + 1649) + 1; e = 30·N(0.005x + 342, 0.005y + 23423);
R = (k·a + 200(1 − a)) − max(p, 0); G = (120a + 230(1 − a)) + e; B = (k·a + k(1 − a)) + p;
clamp. (Sums in the order written; noises evaluated in the order a, p, k, e.)

### 6.3 Sand (RVA 0x1f9bc0)

a80 = 80·unit(N(O274 + 0.03x, O278 + 0.03y)); b = unit(N(O27c + 0.003x, O280 + 0.003y));
g = 40·N(0.01x + 493, 0.01y + 789) + 200;
R = ((1 − b)·255 + 255b) + a80; G = ((1 − b)·g + 150b) + a80; B = ((1 − b)·100 + 50b) + a80;
clamp.

### 6.4 Beach (RVA 0x1c4520)

a80 = 80·unit(N(O274 + 0.03x, O278 + 0.03y)); b = unit(N(O27c + 0.01x, O280 + 0.01y));
R = ((1 − b)·240 + 240b) + a80; G = ((1 − b)·240 + 180b) + a80; B = ((1 − b)·100 + 50b) + a80;
clamp. (Shares two noises with sand, at a finer second scale.)

### 6.5 Snow (RVA 0x1f9e50)

a = unit(N(0.04x, 0.04y)); R = 190a + 100(1 − a); G = 220a + 180(1 − a); B = 255a + 255(1 − a);
clamp.

### 6.6 Rock Rk(x, y, z) (RVA 0x1f9620)

1. Strata v = unit((N(0.1y + 98984, 0.4z) + N(0.1x, 0.4z))·0.5) (z·0.4 in double).
2. Base by warmth wm: default (120, 120, 130). If wm < 0.5: t = 2wm, base =
   (10(1 − t) + 160t, 20(1 − t) + 160t, 50(1 − t) + 170t). Else if wm > 0.75:
   t = 4(wm − 0.75); n = N(0.001·t + 6544, 0.001y + 123) (the first argument really uses t);
   base = (120(1 − t) + 200t, 120(1 − t) + (20n + 130)t, 130(1 − t) + 100t).
3. C = 160v + base per channel, and B += 10·(N(O22c + 0.005x, O230 + 0.005y) + 1); clamp.
4. **Fire tint** F = fire(x, y) > 0: s = min(0.1·N(0.05x, 0.05y) + N(0.02x, 0.02y), 1);
   u5 = (1 − |s|)⁵; R ← R(1 − F) + (200u5 + 10)F; G ← G(1 − F) + ((N(0.1x, 0.1y) + 1)·u5·50 + 10)F;
   B ← B(1 − F) + 10F. (Not clamped again.)

### 6.7 Cliff Cl(x, y, z) (RVA 0x1c7140)

1. v = unit((N(0.1y + 98984, 0.4z + 8437) + N(0.1x, 0.4z))·0.5).
2. R = 60v + (20·N(O24c + 0.01x, O250 + 0.01y) + 180) + 20·N(O25c + 0.01x, O260 + 0.01y);
   G = 60v + (20·N(O254 + 0.01x, O258 + 0.01y) + 100) + 20·N(O264 + 0.01x, O268 + 0.01y);
   B = (0·v + 60) + 20·N(O26c + 0.01x, O270 + 0.01y); clamp. (Noise evaluation order:
   O25c, O264, O26c, O24c, O254.)
3. If wm > 0.75: t = 4(wm − 0.75); C ← C(1 − t) + Rk(x, y, z)·t·0.9.
4. If wm < 0.2: s = 1 − 4wm; C ← C·(1 − s) + (Rk(x, y, z) + snow(x, y))·(0.5s).
5. If fire F > 0: C ← C(1 − F) + Rk(x, y, z)·F.

### 6.8 Water and implicit colours

Water blocks carry (255, 255, 255) (byte 3 = 0x82) or (0, 0, 0) (carved, byte 3 = 0x02);
implicit blocks below a column carry (200, 200, 200). How they are drawn is outside this
specification.

---

## 7. Open questions

1. **Locations** (regions spec open question 1) feed H, M, R, W, q and the colours; until
   they are specified only location-free areas can be reproduced exactly.
2. **Ravine spawns and later passes** draw from the zone's random stream; whether the
   routines called in §5.5 step 10 (RVA 0x4a970, 0x4d590, 0x1adb20, 0x26add0, 0x19a9b0,
   0x1a6b60) make further draws was not traced, so every later draw in the zone depends on
   completing that trace.
3. The use of the counters (columns with P > 0.25, grass columns), the min/max zs and
   column +0x14 was not traced.
4. The double-precision √ used in §3.4, §4.3 is the MSVCR110 import
   `_libm_sse2_sqrt_precise` (thunk RVA 0x28d9c8); it is assumed correctly rounded.
5. Within the terrain passes traced here no write lands on a stored block whose bit 7 is
   set; the effect of the keep marker on stored water during the later passes is not traced.
6. The roles of zone +0x18 and +0x24 and the meaning of the values stored in the spawned
   objects' +0x2c field are not traced here.
7. The `Server.exe` counterparts (RVA 0xf9b70, 0x118630) were identified by equal or similar body
   size and by the same world offsets and seed formula; a step-by-step equivalence was not re-checked.

See also: questions 1 and 4 in [alpha-worldgen-locations.md](alpha-worldgen-locations.md);
question 2 (ravine spawns and post-pass draws) in [alpha-worldgen-caves.md](alpha-worldgen-caves.md).

---

## Re-certification (2026-11-15)

Source-side readback against `Cube.exe` (SHA-256 above) decompiled and disassembled
evidence, `Server.exe` for the counterparts, and constants read from the image bytes.

**Confirmed** (unchanged in substance): zone and column layouts and lookups; block byte 3
bits; implicit block constants (grey 200 type 1, air 255 type 0, water 255 with 0x82);
block-write rules and the rock gap fill; town-ness, flatness, roughness and mountain-mask
formulas and constants; base-height terms, elevation blend, assembly and location rules;
zone order, seed formula, climate, grid and surface pass including the steep test, ridge
pattern, surface height, colour mix and two-block layout; ravine formulas, carve, floor
block and spawn gating (x + 3y mod 7, mod 30, mod 4, mod 10, mod 100); lake fill; all
colour routines (grass, sand, beach, snow, rock, cliff, surface colour) with their
constants and evaluation order, including the cliff green-channel grouping.

**Corrected** (12):
1. All addresses restated as RVAs of the named image (old: virtual addresses with base
   0x400000 presented without an image base).
2. Mountain mask step 5: the divisor is the float 0x3ECCCCCC, not the nearest float to 0.4
   (precision; affects world generation).
3. Mountain mask type 10: the offset helper is the plain difference location centre minus
   column position, not an untraced "segment" offset; radius named explicitly (old open
   question 4 resolved; affects world generation).
4. Elevation displacement and window stated explicitly from the evidence (x displacement
   samples y and vice versa; window centred on the undisplaced x, y) instead of deferring
   to another specification; the comparison with the climate blends was dropped.
5. §1.5: "air" means byte 3 entirely zero, not type bits zero.
6. §1.3/§7: water written from the 0x82 constant carries the keep marker; the old claim
   that keep blocks arise only from structures running first was wrong in its reasoning
   (reversal of old open question 6).
7. §2.3 dry air: the marker is set only on carved air at z ≥ 1 when the carve top is below
   the surface, so it does not change reading there; old text claimed it kept cells air at
   z ≤ 0 (reversal).
8. §5.4: minimum and maximum zs start at 0 and compare S against the stored value.
9. §5.5: the skip after "top ≤ floor" also skips the lake fill (made explicit); the spawn
   draw sequence per case and object field values stated (random-draw order made explicit,
   no change of order).
10. §5.7: light recomputation is not the last step; further steps follow and the zone is
    registered in its region table at the end.
11. Island rule: removed the unsupported bound "≤ 100" on the raise.
12. §5.1: the region preparation precedes the presence check, which then stops generation.

**Removed**: the whole "Checking against the running game" section (verification
procedures, DLL calls, dump comparisons, visual expectations); the grass and snow test
vectors; the interpretive "yellow-green with little blue" remark; block types 11 (town
ground) and 13 (glowing) and the renderer statements about how implicit, water and
below-column blocks are drawn (not established from the terrain evidence); the
"overview spec" list of post passes.

**Unresolved**: listed in §7 (location inputs; draws inside spawn callees; counter uses;
square-root rounding; keep-marker effects in later passes; zone containers and object field
meaning; full server equivalence).
