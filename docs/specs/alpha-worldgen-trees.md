Status: FINAL / STABLE / HELD

# Alpha world generation: voxel trees

Behavioural specification of the Cube World **Alpha** voxel-tree builder: the routine that
writes a tree's trunk, roots, branches and leaf crowns as blocks into a zone. All addresses are
RVAs (image base 0x400000 in both images). Client image `Cube.exe` (SHA-256 `84a7a132…30bf`):
builder RVA 0x1df960 (20099 bytes), leaf-blob routine RVA 0x1d7f60, ball writer RVA 0x19cbf0,
disc writer RVA 0x19c890, moss share RVA 0x1efdb0. Every formula was read from the machine code
and the image's constant bytes. Companion contracts used as pointers only: the
[random spec](alpha-worldgen-random.md) (draws, the noise N, precision words), the
[terrain spec](alpha-worldgen-terrain.md) (block format §1.3, reads §1.4, writes §1.5, colours
§6), the regions spec (warmth and moisture blends) and the
[vegetation spec](alpha-worldgen-vegetation.md) §5–§6 (where trees are placed).

**Server.** `Server.exe` (SHA-256 `1ffbcbce…4fb1`) contains the same five routines: builder
RVA 0x113760, leaf blob RVA 0x10bd60, ball RVA 0x0d4820, disc RVA 0x0d44c0, moss RVA 0x123b90.
Each has the same length and the same instruction sequence as its client counterpart once
absolute addresses are ignored; every constant they read has identical bytes in both images, the
import slots they call hold the same runtime imports (`rand` and the standard library's
length-error routine), and the sine and cosine calls go to the server's thunks for the same
runtime sine and cosine. The
server's calls to the builder are at RVA 0x11e58e and 0x11b011 (zone generator, RVA 0x118630) and
RVA 0x0ef970 (town builder, RVA 0x0e28e0).

Scope: kinds 0, 1, 2, 4, 5 and 6 are specified to the voxel and to the draw. Kind 3 (palm) is
complete except for the palm-leaf models, which are stamped by the structure routine
RVA 0x1f0ce0 (unresolved item 1).

---

## 0. Conventions

* As in the random spec §0 and terrain spec §0. **f32(…)** rounds to single precision;
  "[float]" after a formula means every operation in it rounds to single, "[double]" means
  double; where they mix, the f32 marks say where rounding happens. trunc truncates towards
  zero. "a div b" is integer division truncating towards zero; "a mod b" the matching remainder.
  All 64-bit float-to-integer conversions (helper RVA 0x28d946) truncate towards zero, and all
  64-bit divisions (helper RVA 0x28d9e0) are signed and truncate towards zero.
* **D** is one draw of the runtime's `rand()` (imported from the Visual C++ 2012 runtime), taken
  at the point where it is written. Every draw listed is made exactly once when its step is
  reached, in the order written. The builder makes no `srand` call; it continues the zone's
  stream (random spec §2.3).
* Arguments: **x, y** block column of the tree, **z** the first cell above the ground,
  **s** size, **t** height, **kind** (0–6), **Z** the zone being generated, with zone indices
  (zx, zy) at zone + 0x60 and zone + 0x64. **X0 = x + 0.5**, **Y0 = y + 0.5**: the builder holds
  them as 64-bit fixed point x·65536 + 32768 (it subtracts the double −32768 stored at
  RVA 0x302ad8, converted to an integer) and converts them to doubles exactly. Where this
  document writes "x" for a value the builder derives from that fixed point by dividing by
  65536, the quotient is truncated: it equals x for x ≥ 0 and x + 1 for x < 0 (likewise y).
* **w** = warmth and **mo** = moisture of column (x, y): the float at column + 0x08 / + 0x04
  when the column is in Z, otherwise the blends RVA 0x1c4dd0 / RVA 0x1c4800 (regions spec). The
  builder reads w once at the start and mo only where stated.
* **π** in angle formulas is the double 3.141592653589793 (RVA 0x2fce08), **2π** the double
  6.283185307179586 (RVA 0x302a78); **32767** in angle formulas is the double at RVA 0x302a80,
  elsewhere the float at RVA 0x2fd338. sin and cos are the runtime's precise double-precision
  sine and cosine, applied to the float angle widened to double; the results are rounded to
  float. In every direction vector below, cos goes with x and sin with y.
* Colours are three floats on a 0..255 scale; **bytes(c)** truncates each channel to an integer
  and keeps the low byte (no clamping unless "clamp" is written; clamp = each channel limited
  to [0, 255], the helper RVA 0x1c4770).
* The letter k is a cell height (z index) inside loops; the tree kind is always written "kind".
* Names of the tree kinds are inferences from the shapes (§7).

---

## 1. Callers and arguments

### 1.1 The builder

The builder is a member of the world object (the world in ECX) taking seven 32-bit stack
arguments, in this order: **x, y, z, s, t, kind, Z** (Z a zone pointer). It returns nothing; its
effects are block writes into Z (§2), records appended to Z (§5) and draws.

### 1.2 Call sites (client)

| Caller | Call RVA | Arguments |
|---|---|---|
| zone generator RVA 0x1e4850, tree grid | 0x1ea7ae | (x, y, ground + 1, s, or 1 when s < 1, t, kind, Z) |
| zone generator, location type 12 | 0x1e7231 | (256zx + 128, 256zy + 128, column bottom + count, 80, 80, 6, Z) |
| town builder RVA 0x1af170 | 0x1bc200 | (x, y, h + 1, D mod 6 + 7, (D′ and 7) + 13, kind, Z) |

* **Tree grid** (vegetation spec §5): the grid walks 14 × 14 positions 18 blocks apart. The
  column is first x = 256zx + 18i + s, y = 256zy + 18j + s (RVA 0x1ea030..0x1ea075), with s the
  grid's crown size at that point; the grid then limits x to at most 256zx + 256 − (s div 2) and
  y likewise (RVA 0x1ea0bc..0x1ea0f4), and later, if x + s > 256zx + 256, sets
  x = 256zx + 256 − s, and the same for y with zy (RVA 0x1ea57a..0x1ea5d3). s and t passed are
  the grid's crown size and height; s is replaced by 1 when s < 1. After the ground search, if warmth(x, y) > 0.8 **and**
  moisture(x, y) < 0.2, the kind becomes **3** (RVA 0x1ea6c3..0x1ea75e; both read from the
  column, or the blends). The call is then made only when the ground block's type (byte 3 and
  0x1f) is 4 or 10, or is 9 with the first cell above the ground at height ≤ 2.
* **Giant tree**: only for a location of type 12 lying in this zone. z is the bottom + count of
  the column at the zone centre (the first cell above the column's stored blocks), taken after
  one discarded draw.
* **Town builder**: two draws are made, first D′ (height = (D′ and 7) + 13), then D
  (size = D mod 6 + 7). The kind is 3 if warmth(x, y) > 0.8, else 5 (the warmth read through
  RVA 0x1c8350: column + 0x08, or the blend RVA 0x1c4dd0). The call happens only when the town
  builder's value h is greater than 1 (h, x and y belong to the structures follow-up).
* Argument order at the grid call (RVA 0x1ea78c..0x1ea7ae): x is pushed last, so it is the
  first argument.

---

## 2. Blocks written and write rules

### 2.1 Block types

| Byte 3 | Type | Written by |
|---|---|---|
| 0x27 | **7 wood**, flag 0x20 | trunks (all kinds but 6), roots, base flare, branches |
| 0x07 | **7 wood**, no flag | the trunk of kind 6 only |
| 0x28 | **8 leaves**, flag 0x20 | leaf blobs, vines, conifer discs, conifer snow caps, conifer top |

Alpha's leaf type is 8; the snow caps of conifers (§4.10.4) are also type 8. Palm leaves come
from models (§4.8.3); their block type is unresolved item 1.

### 2.2 Reading before writing

Except where §4.10.4 step 5 says otherwise, every voxel step of the builder and its helpers
first **reads** the target cell, restricted to zone Z, in the manner of terrain spec §1.4: a
cell whose column is outside Z, or which lies below the column's bottom, reads as the implicit
grey block (200, 200, 200, byte 3 = 0x01); a cell at or above the column's top reads as the
implicit air block (255, 255, 255, 0x00) when its height is > 0, otherwise as the implicit water
block (255, 255, 255, 0x82); a stored block of type 0 at height ≤ 0 without bit 0x40 also reads
as that water block. If the block read has bit 0x40 ("dry air", carved overhang space,
terrain §2.3) the cell is **skipped**; otherwise it is written.

The builder and the leaf blob use their own copy of the three implicit blocks
(RVA 0x36ddf4 / 0x36ddf8 / 0x36ddfc), the ball and disc writers another (RVA 0x36b7cc / 0x36b7d0
/ 0x36b7d4) and the fixed-point cell read used by the kind-5 branches (RVA 0x02f860, reading
through RVA 0x02f7e0) a third (RVA 0x36afbc / 0x36afc0 / 0x36afc4). Each triple is filled by
start-up initialisers (RVA 0x2fae30..0x2fae90, 0x2faad0..0x2fab30, 0x2f9720..0x2f9780) with the
bytes given above, so none carries bit 0x40. Only stored blocks can therefore carry 0x40: trees
overwrite air, water, terrain, other trees' blocks and their own earlier blocks alike, and never
fill cells that the surface pass marked dry.

### 2.3 Writing

Writes go through the generator's block write (RVA 0x04e7a0, terrain spec §1.5) with zone Z:

* A cell whose column is **outside Z is not written** (the column lookup RVA 0x0347a0 fails);
  the read before it still happened and any draw tied to it is still made. Trees are therefore
  clipped to their own zone; the grid, the kind-3 lean, the kind-5 clumps and the round-crown
  branch blobs are moved or limited inward.
* The write routine substitutes its water block only when the written byte 3 has none of the
  bits 0x5f set and the height is ≤ 0; tree bytes (0x27, 0x07, 0x28) never qualify. A write at
  a height below the column's bottom is followed by writes to every cell between it and the old
  bottom, with type-6 blocks (byte 3 = 0x06) whose colour comes from RVA 0x1f9620, as
  terrain §1.5 describes (roots of the giant tree can do this); the keep marker (bit 7) behaves
  as terrain §1.3 says.
* Later writes replace earlier ones. The order of the steps in §4 is therefore part of the
  result (for example the crown replaces the top of the trunk).

---

## 3. Shared helpers

### 3.1 Moss share m(i, j, k) (RVA 0x1efdb0)

Arguments: cell (i, j, k) and the zone.

* n1 = N(f32(f32(0.2·j) + 534), f32(f32(0.2·k) + 13)) (the arguments computed in float, then
  widened; the result rounded to float);
* n2 = N(0.05·i + 4343, 0.1·k + 84734) (arguments in double; result rounded to float);
* f = f32(f32(f32(n1·0.1) + n2)·0.7) + 0.2 [float];
* m = f32(f·mo(i, j)) limited to [0, 1], where mo(i, j) is the moisture of column (i, j)
  (column + 0x04 in the zone, else the blend RVA 0x1c4800).

N is the noise routine RVA 0x0c0ef0 (random spec). The ball and disc writers blend a colour c
towards **(50, 120, 60)** with it: each channel c ← f32(f32((T − c)·m) + c) with T = 50, 120, 60
[float], c taken as a float from its byte, then bytes.

### 3.2 Ball (RVA 0x19cbf0)

Ball(cx, cy, cz, r, block, Z, centred, moss): integer centre, float radius r, a 4-byte block
(colour bytes and byte 3) given by reference.

* Centre C = (cx, cy, cz) as doubles, plus 0.5 on each axis when **centred** (every tree use is
  centred).
* Cells i from trunc(f32(f32(cx) − r) − 1) while f32(i) ≤ f32(f32(cx) + r) + 1 (float compare);
  j and k likewise with cy and cz. Loop order: i outer, then j, then k, all ascending.
* A cell is inside when ((j − Cy)² + (i − Cx)²) + (k − Cz)² ≤ f32(r·r) (sums in double, in
  that order).
* Inside and not dry air (§2.2): the colour is the block's, or with **moss** the moss blend of
  it (§3.1) at (i, j, k). Written with the block's byte 3.
* As a consequence, a centred ball of radius 2 covers 32 cells and an uncentred one 33.

### 3.3 Disc (RVA 0x19c890)

Disc(cx, cy, z, r, block, Z, centred, moss): the same in the single layer z (z is never offset):
inside when (j − Cy)² + (i − Cx)² ≤ f32(r·r); loops i then j ascending; centre + 0.5 on x and y
when centred. Every tree use is centred and passes moss = false.

### 3.4 Leaf blob (RVA 0x1d7f60)

Blob(x, y, z, rH, rV, Ca, Cb, kind, Z): integer centre, float radii, two float colours (passed
by value), a kind.

1. Centre (cx, cy, cz) = (x + 0.5, y + 0.5, z + 0.5) [double].
2. **D1, D2** (two draws), used as noise offsets.
3. Columns: i from trunc((cx − rH) − 1) while i ≤ (rH + cx) + 1, and inside that j from
   trunc((cy − rH) − 1) while j ≤ (rH + cy) + 1 [double, rH widened].
4. Per column: **off** = f32(0.5·N(0.04i + D1, 0.04j + D2) + f32(0.3·N(0.1i + D1, 0.1j + D2)))
   (noise arguments double; the 0.5 product and the sum in double, the 0.3 product in float);
   H = ((j − cy)/rH)² + ((i − cx)/rH)² [double]. The noise is evaluated for every column.
5. Cells k from trunc((rV + cz) + 1) **downwards** while k ≥ (cz − rV) − 1:
   v = ((k − cz)/rV)·1.7999999523162842 + off [double] (the constant is float 1.8 widened);
   inside when v² + H ≤ 1.
6. Inside and not dry air: τ = f32((v + 1)·0.5) limited to [0, 1];
   colour = clamp(f32(f32(1 − τ)·Ca) + f32(Cb·τ)), bytes; write with byte 0x28.
7. **Vines (kind 4 only)**, right after that write: D; if D mod 20 = 0: D: n = D mod 10;
   D: if (D and 3) = 0, n ← 2n. Then for the n cells below (k − 1, k − 2, …, k − n) in the same
   column: if not dry air, write leaves with τ′ = f32((((k′ − cz)/rV)·1.8 + off + 1)·0.5)
   [double inside, as in 5–6] limited to [0, 1], colour as in 6. Vines ignore the blob shape
   and the ground; a dry-air vine cell is skipped and the next one below is still tried.

The shape is an ellipsoid of horizontal radius rH and vertical half-height rV/1.8, shifted up
or down per column by the noise, coloured Ca at its bottom and Cb at its top.

---

## 4. The builder, step by step (RVA 0x1df960)

### 4.1 Start

rA = rB = f32(f32(s)·0.25) (trunk radius at the foot and at the top). w is read; bark bases
gB = f32(f32(w·50) + 120), bB = f32(f32(w·100) + 50), with w as read (not yet limited).

### 4.2 The four initial draws: bark colours

**D1, D2, D3, D4**:
**A** = (240, f32(D1 mod 80 + gB), f32(D2 mod 50 + bB)) (bark at the trunk's foot),
**B** = (240, f32(D3 mod 80 + gB), f32(D4 mod 50 + bB)) (bark at the trunk's top).

### 4.3 Per-kind adjustments (no draws)

| Kind | Change |
|---|---|
| 5 | t ← trunc(f32(t·0.7)); s ← min(trunc(f32(s·0.2)), 3); if f32(8s) > f32(t) then t ← 8s. The new s and t are used from here on; rA, rB keep the values of §4.1. |
| 1 | rA ← f32(rA·0.5); t ← 2t; rB ← 0.1 (raised to 0.3 below) |
| 6 | rB ← f32(rA·0.5) |
| 3 | rA = rB = 2; A = (240, 180, 120); B = (220, 100, 50) |
| 2 | rA = rB = 1.5; A = B = (255, 255, 255) |
| 0, 4 | none |

Then, for every kind except 2 and 3: rA ← max(rA, 0.3), rB ← max(rB, 0.3).

### 4.4 Leaf palette (draws D5 …)

v = f32(f32(w − 0.5)·10) with the unlimited w; flag **F** = 0; then w is limited to [0, 1]
(all later uses of w are the limited value).

**D5**: the tree gets **green** leaves if f32(f32(D5)/32767) < v, or kind = 4, or kind = 3.
Otherwise mo is read and:

| Case | Draws | L1 (lower leaf colour) | d | L2 (upper leaf colour) | F |
|---|---|---|---|---|---|
| green | D6–D9 | clamp(D6 mod 20, D7 mod 155 + 50, D8 mod 100) | D9 mod 100 + 50 | clamp(L1 + (d, d, 0)) | 0 |
| mo > 0.5, "autumn" | D6–D9 | (D6 mod 55 + 200, D7 mod 100, D8 mod 50) | D9 mod 100 + 50 | clamp(L1 + (d, d, 0)) | 1 |
| mo ≤ 0.5, D6 even, "blossom" | D6–D9 | clamp(D7 mod 55 + 200, 0, D8 mod 200) | D9 mod 100 + 100 | clamp(L1 + (d, d, d)) | 1 |
| mo ≤ 0.5, D6 odd | D6–D10 | clamp(D7 mod 20, D8 mod 100 + 120, D9 mod 100) | D10 mod 150 + 100 | clamp(L1 + (d, d, 0)) | 0 |

("D6 even" means D6 and 1 = 0; every L1 component is already inside [0, 255].) **Kind 2** then
overrides: L1 = (150, 255, 0), L2 = (244, 255, 0), F = 0 (its palette draws were still made).
F enables the kind-60 records (§5).

### 4.5 Cold tint (w < 0.2)

c = min(1 − f32(f32(w − 0.1)/0.1), 1), q = 1 − c [float], S = the snow colour at (x, y) (routine
RVA 0x1f9e50, terrain §6.5; it is evaluated twice, once for each leaf colour):
L1 ← f32(c·S) + f32(q·L1), L2 ← f32(c·S) + f32(q·L2),
A ← (f32(c·120) + f32(q·A.r), f32(c·120) + f32(q·A.g), f32(c·80) + f32(q·A.b)) [float,
per channel]. B is unchanged.

### 4.6 Lean (kind 3 only)

D: jx = f32(f32(0.5 − f32(f32(D)/32767))·f32(t)); D: jy likewise. Then jx ← 0 if
f32(f32(x) + jx) − f32(s) < 256zx or f32(s) + f32(f32(x) + jx) ≥ 256zx + 256 (float compares);
jy likewise with y and zy. Both draws come before both tests. Other kinds: jx = jy = 0.

### 4.7 Trunk

Columns i from trunc(f32(f32(x) − rA) − 1) while f32(i) ≤ f32(f32(x) + rA) + 1 (float), and
inside that j likewise with y; cells k from **z + t down to z − 2**. For each cell:

1. u = f32(f32(k − z + 2)/f32(f32(t) + 2)) (0 at k = z − 2, 1 at the top), u2 = f32(u·u);
   radius r = f32(f32((1 − u2)·rA) + f32(u2·rB)) [float].
2. Target column: (i, j); for kind 3 (trunc(f32(f32(jx·u2) + i)), trunc(f32(f32(jy·u2) + j)))
   (the trunk leans quadratically towards (x + jx, y + jy)).
3. Shape [double, r widened], always computed from the unshifted (i, j):
   * r > 0.8 and kind 6: ex = f32(N(0.025j, 0.025k)·f32(5/r)) + (i − X0)/r,
     ey = f32(N(0.025i, 0.025k + 8473)·f32(5/r)) + (j − Y0)/r (noise products in float,
     f32(5/r) a float division);
   * r > 0.8, other kinds: ex = (i − X0)/r, ey = (j − Y0)/r;
   * r ≤ 0.8: ex = (i − x)/r, ey = (j − y)/r, with x and y the fixed-point centres divided by
     65536 (§0), so thin trunks are centred on the column and one column wide.
   Inside when ey² + ex² ≤ 1. The loop bounds use rA only.
4. Inside and the target cell not dry air: colour c = f32(A·f32(1 − u)) + f32(B·u)
   [per channel]. **Kind 2**: D; if D mod 8 = 0, c = (50, 50, 50) (one draw per such cell,
   including cells outside Z). Moss blend (§3.1) at the target cell, bytes, write at the target
   cell with byte 0x07 for kind 6, 0x27 otherwise.

### 4.8 Base: roots, flare or palm leaves

Kind 6 takes §4.8.1, kind 3 §4.8.3, kind 1 none, every other kind §4.8.2.

#### 4.8.1 Roots (kind 6)

D: nR = (D and 3) + 5. D: φ = f32(D·2π/32767) [double]. g = f32(rA·0.75).
For root ρ = 0 … nR − 1:

* a = f32(ρ·π·2/nR + φ) [double, in that order]; sa = f32(sin a), ca = f32(cos a).
* P = (X0 + f32(f32(ca·rA)·0.5), Y0 + f32(f32(sa·rA)·0.5), z) [double].
* Steps dX = f32(f32(ca·g)·0.1), dY = f32(f32(sa·g)·0.1), sink limit e = f32(g·0.05).
* For q = 0, 7, 14, …, 140 (21 steps): Ball(trunc PX, trunc PY, trunc PZ, radius
  10 − q div 20, bytes(A) with 0x27, Z, centred, moss); PX += dX; PY += dY; D;
  PZ −= f32(f32(D·e)/32767).

The roots run outwards in nR evenly spaced directions, shrinking from radius 10 to 3 and
sinking a random amount per step.

#### 4.8.2 Base flare (kinds 0, 2, 4, 5)

o = −1. For zc = z, then z − 1: r = f32(f32((4 − z + zc)·rA)/5) (0.8·rA, then 0.6·rA);
o ← f32(o + r); four balls of radius r, bytes(A) with 0x27, centred, moss, at
(trunc(f32(x) − o), y, zc), (trunc(f32(x + 1) + o), y, zc), (x, trunc(f32(y) − o), zc),
(x, trunc(f32(y + 1) + o), zc), in that order. No draws.

#### 4.8.3 Palm leaves (kind 3)

Crown column Xc = (x·65536 + 32768 + trunc(f32(jx·65536))) div 65536, Yc likewise with y and jy
(64-bit integers). The world's model table (a vector of 4-byte pointers starting at world
+ 0x20, ending at world + 0x24) holds palm-leaf.cub at index 2300 and palm-leaf-diagonal.cub at
index 2301 (their name strings are referenced at RVA 0x115aaf / 0x115af9); a model's x and y
sizes are its + 0x44 and + 0x48 fields (sx, sy). A model "exists" when the table is longer than
its index and the entry is not null.

* If model 2300 exists: four stamps of it, for rotations 0, 1, 2, 3 at
  (Xc − sx div 2, Yc + 1), (Xc + 1, Yc − sx div 2), (Xc − sx div 2, Yc − sy), (Xc − sy, Yc − sx div 2).
* Then, if model 2301 exists (y size dy): four stamps at rotations 0–3 at
  (Xc + 1, Yc + 1), (Xc + 1, Yc − dy), (Xc − dy, Yc − dy), (Xc − dy, Yc + 1).
* Each stamp first makes one draw D and uses height zp = z + t − 5 + D mod 3. The stamp is the
  structure routine RVA 0x1f0ce0 called with (model, reference to the position (X, Y, zp),
  rotation, 0x28, 0, Z, 0, reference to four zero integers, 0); its behaviour belongs to the
  structures follow-up (unresolved item 1).

### 4.9 Marker draw (all kinds)

D; if D mod 10 = 0 and w > 0.2: append a **kind 61** record (§5) at fixed point
(x·65536 + 32768, y·65536 + 32768, (z + t)·65536), with t after §4.3.

### 4.10 Crowns

Kind 3 has no further step; the other kinds continue as follows.

#### 4.10.1 Round crown (kinds 0, 2, 4)

1. D: nb = D mod 5 + 4. D: φ = f32(D·2π/32767). D: sf = f32(f32(D·0.8)/32767) + 0.6 [float].
   D: rC = f32((f32(f32(D·1.25)/32767) + 2)·rA) (the + 2 and the product in double).
2. **Main blob**: Blob(x, y, z + t, rC, f32(rC·sf), L1, L2, kind, Z), x and y as in §0.
3. g = f32(rA·0.75). For each branch b = 0 … nb − 1:
   1. D: h = f32(f32(f32(D·0.5)/32767) + 0.5); zb = f32(f32(h·t) + z).
   2. a = f32(b·π·2/nb + φ); sa, ca. P = (X0 + f32(f32(ca·rA)·0.5), Y0 + f32(f32(sa·rA)·0.5), zb)
      [double]. Branch colour cb = f32(A·f32(1 − h)) + f32(B·h) (not clamped).
   3. Six steps: if kind is 0 (not 2 or 4) and rA > 1.5, Ball(trunc PX, trunc PY, trunc PZ, 2,
      bytes(cb) with 0x27, centred, moss); then PX += f32(f32(ca·g)·0.5),
      PY += f32(f32(sa·g)·0.5); D; PZ += f32(f32(D·f32(g·0.5))/32767).
   4. D: rr = f32((f32(f32(D·1.25)/32767) + 2)·rA).
   5. Keep the blob in the zone [double compares]: if (PX − rr) − 1 < 256zx, PX =
      f32(f32(256zx + rr) + 1); the same for PY with zy; then if (rr + PX) + 1 ≥ 256zx + 256,
      PX = f32(f32(256zx + 256 − rr) − 2); the same for PY.
   6. If b > 4: PZ += f32(f32(sf·rr)·0.5).
   7. e = f32(f32(sf·rr)·0.25); Blob(trunc PX, trunc PY, trunc(e + PZ), rr, f32(sf·rr), L1, L2,
      kind, Z).
   8. If F: append a **kind 60** record at (trunc(PX·65536), trunc(PY·65536),
      trunc((PZ + e)·65536)) (64-bit truncations of doubles) with colour L1/255.

#### 4.10.2 Layered crown (kind 5; s and t as changed in §4.3)

1. n_L = trunc(f32(f32(t div 4 + 6) + f32(s·0.5))) layers.
2. D, D (both discarded). D: φ0 = f32((D and 7)·π/4) (π/4 = 0.7853981633974483, double,
   RVA 0x31dd80).
3. For layer i = 0 … n_L − 1 (with acc = i·t; acc grows by t for skipped layers too):
   1. If i is odd: D; if D mod 3 ≠ 0 the layer is skipped.
   2. hf = f32(f32(i·0.6)/f32(n_L − 1)) + 0.6 [float]; zl = f32(f32(t·hf) + z).
   3. a = f32(i·π·0.25 + φ0) [double]; sa, ca. Q = (X0 + f32(ca·rA), Y0 + f32(sa·rA), zl).
   4. Branch colour col = clamp(f32(A·f32(1 − hf)) + f32(B·hf)).
   5. D: n = t div 2 + D mod (t div 2) + (acc div 4) div n_L.
   6. fr = f32(f32(f32(s·8)·f32(1 − hf)) + s); rx = f32(fr·ca), ry = f32(fr·sa).
   7. For j = 0 … 2n:
      * e = 1 − (1 − j/(2n))² [float].
      * Fixed-point point PZ = trunc((f32(j·0.5) + zl)·65536), PY = trunc((f32(e·ry) + Q.y)·65536),
        PX = trunc((f32(e·rx) + Q.x)·65536) (doubles truncated to 64-bit).
      * The cell containing (PX, PY, PZ) is read, then, if it is not dry air, the cell containing
        (PX, PY, PZ + 65536). These reads go through the fixed-point read RVA 0x02f860, which
        divides each coordinate by 65536 after subtracting 65536 from negative coordinates.
      * If both cells are not dry air: with (bx, by) = (PX div 65536, PY div 65536), write
        col with 0x27 at (bx, by, (PZ + 65536) div 65536), then at (bx, by, PZ div 65536)
        (for non-negative coordinates these are the two cells read, the upper first). Then: if
        j = 2n, leaf clump; else if j mod 4 = 0: D, and if D mod 3 = 0, leaf clump. (If either
        cell is dry air, nothing of this bullet happens, draws included.)
      * **Leaf clump**: D: rH = f32(f32(f32(f32(D·f32(e·0.75))/32767) + 0.25)·f32(n·0.5)) + 2.
        D: rV = f32(f32(f32(D·0.5)/32767) + 0.5)·rH. Centre CY = PY +
        trunc(f32(f32(f32(rH·sa)·0.25)·65536)), CX = PX + trunc(f32(f32(f32(rH·ca)·0.25)·65536)),
        RH = trunc(f32(rH·65536)) (64-bit). Zone clamps, in this order (64-bit signed compares):
        if CX − RH − 65536 < 256zx·65536, CX = trunc(f32(f32(f32(256zx + rH) + 1)·65536));
        the same for CY with zy; if CX + RH + 65536 ≥ (256zx + 256)·65536,
        CX = trunc(f32(f32(f32(256zx + 256 − rH) − 2)·65536)); the same for CY.
        Blob(CX div 65536, CY div 65536, (PZ + 65536) div 65536, rH, rV, L1, L2, 5, Z). If F: a
        kind 60 record at (CX, CY, PZ + 65536) with colour L1/255.

Each kept layer is one wooden branch, two voxels thick, running from the trunk outwards and
upwards along a quarter-ellipse; successive layers turn by 45° and rise from 0.6t to 1.2t.

#### 4.10.3 Giant crown (kind 6)

1. D: nB = D mod 5 + 8. D: φ = f32(D·2π/32767). D, D (discarded). g = f32(rA·0.75).
2. For branch b = 0 … nB − 1:
   1. D: h = f32(f32(f32(D·0.5)/32767) + 0.5); zb = f32(f32(t·h) + z).
   2. a = f32(b·π·2/nB + φ); sa, ca; a2 = f32(a + π/2) (π/2 = 1.5707963267948966, double,
      RVA 0x2fcdf0); s2 = f32(sin a2), c2 = f32(cos a2).
   3. P = (X0 + f32(f32(ca·rA)·0.5), Y0 + f32(f32(sa·rA)·0.5), zb); cb = f32(A·f32(1 − h)) +
      f32(B·h) (not clamped).
   4. For q = 0, 5, 10, …, 200 with step number n = q div 5 (41 steps):
      * ρ = f32(6.5 − f32(q/40)); Ball(trunc PX, trunc PY, trunc PZ, ρ, bytes(cb) with 0x27,
        centred, moss).
      * If n > 10, five rounds, each of two halves:
        first half: D; if D mod 120 < n − 10: D: v1 = (D and 3) + 4; D: h1 = (D and 3) + 4;
        D: cz = trunc((D mod M1) + (PZ − 3)); D: cy = trunc(((D mod 20) + PY − 10) + f32(s2·4));
        D: cx = trunc(((D mod 20) + PX − 10) + f32(c2·4)); Blob(cx, cy, cz, h1, v1, L1, L2, 6, Z),
        with M1 = 2 − trunc(f32(ρ·−2)) [sums in double, left to right];
        second half: the same with M2 = (1 − trunc(f32(ρ·−2)))·2 and the s2, c2 terms
        subtracted instead of added.
      * If n > 10 and F: a kind 60 record at (trunc(PX·65536), trunc(PY·65536), trunc(PZ·65536))
        with colour L1/255.
      * PX += f32(f32(ca·g)·0.075); PY += f32(f32(sa·g)·0.075); PZ += f32(f32(f32(g·0.1)·n)/40).

The branches start half a trunk radius out, thin from 6.5 to 1.5, curve upwards more and more,
and from step number 11 on throw leaf clumps (radius 4–7) to both sides with rising probability.

#### 4.10.4 Conifer crown (kind 1; t doubled in §4.3; L1, L2 and F unused)

1. D: φ = f32(D·2π/32767).
2. D: p = D mod 100; D: q = D mod 50 + 100; D: r = D mod 100; **C1** = (r, q, p).
   D, D, D likewise: **C2** = (r′, q′, p′).
3. R = f32(s), droop = f32(f32(−s)·0.3), segs = t div 3.
4. For spoke n = 0 … 29:
   1. φ ← f32(φ + 0.9424778335276408) (0.3π, RVA 0x31dd88, added in double).
   2. v = f32(f32(n·0.7)/30) + 0.3; cr = f32(f32(cos φ)·R), sr = f32(f32(sin φ)·R);
      fV = f32(f32(1 − v)·2); hz = f32(f32(v·t) + z).
   3. End point E = (x + f32(fV·cr), y + f32(fV·sr), hz + f32(fV·droop)) [double sums].
   4. For m = segs down to 1: f = f32(f32(m)/f32(segs)), gm = f32(1 − f);
      P = (f·E.x + gm·x, f·E.y + gm·y, f·E.z + gm·hz) [double].
      * If w < 0.3: Disc(trunc PX, trunc PY, trunc(PZ + 1), 2, bytes(snow(x, y)) with 0x28,
        centred).
      * Disc(trunc PX, trunc PY, trunc PZ, 2, bytes(f32(C1·gm) + f32(C2·f)) with 0x28, centred).
   5. Write bytes(C1) with 0x28 at (x, y, z + t + 1), then at (x, y, z + t). These two writes
      are made directly, **without** reading the cells first: they replace dry-air cells too.

Each of the thirty spokes is a line of segs = t div 3 radius-2 leaf discs running from the
trunk at height v·t (rising with n) out to a drooping tip; low spokes are long and high spokes
short, which makes a cone. The two top cells are rewritten once per spoke.

---

## 5. Records added to the zone

The builder appends records to the doubly linked list at **zone + 0x04** (element count at
zone + 0x08) through RVA 0x1c03f0; each new element goes to the end of the list. A list node is
0x48 bytes: two links followed by a 0x40-byte element. Only element offsets + 0x00..+ 0x03 and
+ 0x08..+ 0x3b are copied from the builder's record:

| Offset | Content in tree records |
|---|---|
| + 0x00 | kind: **60** (0x3c) or **61** (0x3d) |
| + 0x04 … + 0x07 | not copied (alignment gap) |
| + 0x08, + 0x10, + 0x18 | x, y, z as 64-bit fixed point (1/65536 block) |
| + 0x20 … + 0x2b | copied but never set by the builder (whatever its stack held) |
| + 0x2c, + 0x30, + 0x34 | three floats: L1/255 per channel (kind 60), or 1, 1, 1 (kind 61) |
| + 0x38 | integer 2 |
| + 0x3c … + 0x3f | not copied (alignment gap) |

The builder makes no other kind of record. The client's per-frame world routine RVA 0x0ac260
tests the record kind of list elements: kind 60 at RVA 0x0b1eb4, kind 61 at RVA 0x0acae6 and
RVA 0x0acf5d, the two kind-61 branches being selected by whether the value at offset + 0x800440
of the object that routine works on lies strictly between 21600000 and 75600000 (06:00 and 21:00 if it is the clock in
milliseconds). What these branches produce is
unresolved item 3; it affects rendering only.

---

## 6. Draw order summary

| Step | Draws |
|---|---|
| §4.2 bark | 4 |
| §4.4 palette | 1 + 4 (green, autumn, blossom) or 1 + 5 (odd-D6 green) |
| §4.6 lean (kind 3) | 2 |
| §4.7 trunk (kind 2) | 1 per trunk cell that passes the shape and dry-air tests |
| §4.8.1 roots (kind 6) | 2 + 21 per root |
| §4.8.3 palm (kind 3) | 1 per stamp made (up to 8), plus whatever the stamp routine draws (unresolved item 1) |
| §4.9 marker | 1 |
| §4.10.1 round | 4; main blob 2; per branch 1 + 6 + 1 + blob 2 |
| §4.10.2 layered | 3; per layer: 1 if odd, then 1 if built; per branch cell: see §4.10.2 step 7; per clump 2 + blob 2 |
| §4.10.3 giant | 4; per branch 1; per step with n > 10: 10 tests, 5 + blob 2 per success |
| §4.10.4 conifer | 7 |
| every blob of kind 4 | per written cell: 1, and 2 more when D mod 20 = 0 |

Blobs of kinds 5 and 6 (the layered and giant clumps) never hang vines.

---

## 7. What the kinds look like (inferred)

| Kind | Placed by | Shape |
|---|---|---|
| 0 | grid | round broadleaf: trunk radius s/4, height t, root flare, big leaf blob and 4–8 short branches (wooden only when rA > 1.5) with leaf blobs |
| 1 | grid | conifer: trunk 2t tall, radius s/8 (at least 0.3) at the foot tapering to 0.3, one column wide wherever its radius is ≤ 0.8; cone of drooping disc layers in dark green (C1, C2), snow caps where w < 0.3 |
| 2 | grid | birch: white trunk radius 1.5 with grey spots, yellow-green leaves, leaf blobs without branch wood |
| 3 | grid (hot and dry), towns (hot) | palm: radius-2 trunk leaning up to t/2, sandy bark (240, 180, 120) → (220, 100, 50), palm-leaf models |
| 4 | grid | jungle tree: like 0 but always green, no branch wood, vines hanging from the leaf blobs |
| 5 | grid, towns | layered tree: trunk 0.7t tall, layers of upward-curving branches with leaf clumps |
| 6 | location type 12 | giant tree: trunk radius 20 tapering to 10, noise-distorted, 5–8 sinking roots, 8–12 rising branches with many leaf clumps |

Leaf colour families (§4.4): warm climates green; cool and moist autumn red-orange-yellow; cool
and dry either pink-to-white blossom or green. Moss (§3.1) greens the bark in moist places.

---

## 8. Unresolved

1. **Palm-leaf models (kind 3).** The stamp routine RVA 0x1f0ce0 (15724 bytes, 33 `rand` call
   sites) writes the palm-leaf models. Which of its draws run for these calls (structure-kind
   argument 0), the block type of the stamped voxels (whether the 0x28 argument makes them
   type 8, or the model supplies it), whether it reads cells first, and their colours belong to
   the structures follow-up.
2. **Negative coordinates.** Where the builder divides fixed-point values by 65536 (§0, §4.7,
   §4.8.3, §4.10.1 step 2, §4.10.2 step 7) it truncates towards zero, while the kind-5 cell read
   rounds negative coordinates differently. Whether world block coordinates handed to the
   builder can be negative was not established here.
3. **Meaning of records 60 and 61.** Their handling in RVA 0x0ac260 was located but not read:
   what the kind-60 branch and the two time-selected kind-61 branches emit, whether either kind
   has an entry in the plant-sprite table, and which random state any particle draws there use.
4. **Zone lists.** Tree records go to the list at zone + 0x04. Whether this is the list that the
   vegetation spec's plant pass appends to, and the relation to the list at zone + 0x24 named
   there, was not checked here.
5. **Town calls.** The value h that gates the town builder's call (§1.2) and its x, y come from
   RVA 0x1af170 (structures follow-up).
6. **Noise range.** The statement that the blob's per-column shift (§3.4) is at most 0.8 in
   magnitude depends on the range of N (random spec), which was not re-derived here.

---

## Re-certification (2026-11-15)

Source-side re-check of every statement against the original `Cube.exe` and `Server.exe` machine
code and constant bytes (MSVC dumpbin on the image headers, the per-image assembly listings).
The Infinite-Cubes reconstruction was not consulted.

**Confirmed.** Builder argument order and calling form; X0/Y0 fixed point and the −32768
constant; the warmth/moisture sources; π, 2π, 32767, π/2, π/4, 0.3π and 1.8 constants; bark
colours and their four draws; every per-kind adjustment; the leaf palette table, its draws and
the kind-2 override; cold tint; kind-3 lean and its zone tests; trunk loop bounds, order, radius,
shape tests (including the kind-6 noise wobble), birch spots, moss and the 0x07/0x27 bytes;
roots; base flare; palm stamp positions, rotations, heights and model indices 2300/2301; the
marker draw; round, layered, giant and conifer crowns step by step, including all random-draw
orders; cos on x and sin on y everywhere; zone clamps on + 0x60 / + 0x64; moss share, ball, disc
and blob arithmetic, loop orders and colour blends; the vines; record kinds, positions, colours
and the trailing 2; the call sites, the giant-tree arguments and its discarded draw, the town
draws and kind choice, the grid's kind-3 override and ground-type gate; the absence of `srand`;
the 33 `rand` call sites and 15724-byte length of the stamp routine; the server routines'
instruction-level and constant identity.

**Corrected.**
1. Addresses: absolute virtual addresses replaced by RVAs with the image named (no behavioural
   change).
2. §2.2 / §4.10.4: old "every voxel step first reads the target cell"; new: the two conifer top
   writes are made without a read and overwrite dry-air cells. **Affects world generation**
   (block writes; no draw-order change).
3. §4.10.4 summary: old "thirty radius-2 leaf discs per spoke line"; new: thirty spokes of
   t div 3 discs each, top cells rewritten once per spoke.
4. §5: old "0x3c-byte records, + 0x20…+ 0x2b not written"; new: 0x40-byte elements in 0x48-byte
   nodes, + 0x04…+ 0x07 and + 0x3c…+ 0x3f not copied, + 0x20…+ 0x2b copied but unset.
5. §5: old "kind 61 emits something only between 06:00 and 21:00"; new: kind 61 is tested in two
   branches selected by that time window; effects moved to unresolved. The claims "no model in
   the plant-sprite table" and "render thread's random state" moved to unresolved.
6. §1.2: old "x = 256zx + 18i + s, y = 256zy + 18j + s" as the passed values; new: those are the
   values before the grid's two edge limits, which are now stated. Type-9 gate restated as
   "first cell above ground at height ≤ 2" (same condition).
7. §3.3: the disc also takes a moss flag (trees pass false).
8. §4.8.3: the stamp's eighth argument is a reference to four zero integers, not four integers.
9. §0 / §4.7 / §4.10.1: centres derived from the fixed point by truncating division equal x only
   for x ≥ 0 (x + 1 for x < 0). §4.10.2: the kind-5 reads use a different conversion for
   negative coordinates (writes use truncating division).
10. §2.3: the below-bottom gap fill is stated natively (type byte 6, colour from RVA 0x1f9620).
11. §7 kind 1: old "one-column trunk (base radius s/8 down to 0.3)"; new: one column wide only
    where the radius is ≤ 0.8. Kind 0 branch wood only when rA > 1.5; climate notes on grid
    placements other than kind 3 removed (belong to the vegetation spec).
12. Old open question 2 resolved: the 64-bit conversion helper truncates towards zero. Old open
    question 6 resolved: all three implicit-block triples are initialised without bit 0x40 (the
    builder and blob use a third copy not previously named).
13. §4.7: "k = 6" in the shape rule meant kind 6; kind is now always written "kind".

No reversal of a previous claim about random-draw order was needed; no correction changes the
draw order.

**Removed.** Implementer advice ("an implementation may store 0", the leaf-sway hint, the note
that the terrain table should gain rows); the history of a correction to the vegetation spec;
"for this reason" motive wording; the worked example and the moss, blob and full-tree test
vectors (derived from the formulas, not native observations, and not re-derived here); the
section on checking against the running game and every verification procedure in the open
questions; the claim that the plant pass continues the stream after the grid (vegetation spec
matter, not checked here).

**Unresolved.** See §8: palm stamp behaviour; negative coordinates; record 60/61 effects; zone
list relation; town gating values; noise range bound.
