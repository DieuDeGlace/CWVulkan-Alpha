# Alpha world generation: dungeons, camps, boulders, sites and towns

Status: FINAL / STABLE / HELD

Behavioural contract for the structure passes that the Cube World **Alpha** zone generator
(client RVA 0x1e4850, VA 0x5e4850) runs after its terrain passes and before the voxel-tree
grid. Covered: the dungeon builder (VA 0x5cc510), the camp attempts and the camp routine
(0x5ad320), boulder pass 3, the type-10 site, the town builder (0x5af170), the home building
and the prefab placer (0x5f0ce0). Supported image: `Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (image base 0x400000;
addresses below are VAs, RVA = VA − 0x400000). `Server.exe` (SHA-256 `1ffbcbce…4fb1`) holds
the same zone-generator range (§7).

Related contracts: [caves](alpha-worldgen-caves.md) (§6.1 and §8 lead into this document),
[random](alpha-worldgen-random.md) (the random stream, the noise N, f32 rules),
[terrain](alpha-worldgen-terrain.md) (columns, block format, read and write rules, colours),
[locations](alpha-worldgen-locations.md) (zone entry E, location records),
[vegetation](alpha-worldgen-vegetation.md) (static objects, model tables),
[trees](alpha-worldgen-trees.md) (voxel-tree builder), and the items contract for the item
generators named in §1.10.2.

Statements that could not be confirmed in the original image are tagged **[unconfirmed]**
in place and listed in **Unresolved** (§8).

---

## 0. Conventions

* As in the caves contract §0: f32 means rounding to single precision; trunc means
  truncation towards zero; x is the outer loop and y the inner loop unless stated; a
  **draw** is one value (0..32767) taken from the zone's random stream; column lookups are
  zone-local; block reads and writes follow the terrain contract.
* Draws are never negative, so "d & 1", "d % n" and similar are plain non-negative values.
  "**even(d)**" means the low bit of the draw is 0.
* Colours are byte triples (R, G, B). When a colour is built from three draws d1, d2, d3
  taken in that order, it is written with the draw that feeds each channel; for example
  (d3 % 50 + 20, d2 % 50 + 20, d1 % 50 + 20) means the **third** draw sets red.
* **Model slot k**: entry k of the model pointer list at world + 0x20. The list is filled at
  start-up by the model registry (0x4ee230), which loads each `.cub` key from `data1.db`
  into a fixed slot (slots up to 2568 are filled). Slot names below come from that registry.
* **fx**: a block coordinate times 65536, held as a 64-bit integer. A float coordinate f
  becomes trunc(f32(f · 65536)); a double d becomes trunc(d · 65536).
* **Creature record**: 0x10f0 bytes, initialised by 0x5adb20 (caves contract §7); fields are
  named by offset. **Static object**: 0x188-byte record (initialiser 0x58eaa0) in the vector
  at zone + 0x0c, normally inserted with 0x5a0e00 (vegetation contract §1.3).
  **Sprite**: plant-sprite record (vegetation contract §1.3; layout in trees contract §5)
  appended to zone + 0x04. **Ground item**: 0x148-byte record appended to zone + 0x30
  [unconfirmed: size and helper addresses not re-read].
* **Cell-position read** (0x42f860): converts an fx position to a block by subtracting 65536
  from a negative value and then truncating. This equals floor division for every
  non-negative coordinate, which covers all world positions.

---

## 1. The dungeon builder (0x5cc510)

### 1.1 Inputs

The zone generator (caves contract §6.1; call site 0x5e84a2..0x5e84f7) runs the builder with
the zone, an origin (x0, y0), a style **v** and a **flag**:

* E.kind = 3: (x0, y0) = (256zx + 32, 256zy + 32), v = the entry's variant byte (+0x19),
  flag = 0;
* E.kind = 5: v = 3, flag = 1.

The builder reads the zone level **lv** (zone + 0x80) and the sub-level **sl** (byte
zone + 0x84). It never re-seeds. The dungeon is a grid of 22 × 22 × 22 **cells**, each
10 × 10 × 10 blocks, spanning blocks x0 … x0 + 219 (zone columns 32 … 251) and likewise in
y. All block writes use the zone-local block writer.

### 1.2 Creature kinds (draws only for v = 1, 2)

Two kind tables are filled (case table at 0x5d5f90): a list **K1** of single kinds and a
list **K2** of groups, each group a pair (leaders, escorts) of kind lists:

| v | K1 | K2 | Draws |
|---|---|---|---|
| 0, 4, any other | 11, 12 | ({46}, {19}) | 0 |
| 1, 2 | 15, 16 | ({15, 16}, {96}), then ({k}, {}) | **1**: k = 94 if d % 3 = 1, 17 if 2, 97 if 0 |
| 3 | 2, 3 | ({2, 3}, {19}) | 0 |
| 5 | 78, 77 | ({17, 81}, {62, 30}) | 0 |

### 1.3 Room parameters (162 draws)

There are 27 **rooms**, one per position (a, b, c) of a 3 × 3 × 3 grid, index
r = 9a + 3b + c. Each has four flags **present**, **marked**, **quiet**, **fixed** (all
clear initially) and six integers p0 … p5. For a = 0..2 (outer), b = 0..2, c = 0..2
(inner), six draws are taken in this order and assigned:
p5 = d % 3; p4 = d % 3 − 1; p3 = d % 3 − 1; p2 = d % 3 − 1; p1 = d & 1; p0 = d & 1.

### 1.4 Room selection and links

A **link** joins two grid positions (a, b, c) → (a′, b′, c′). Exactly one of the following
tables applies; unmentioned flags stay clear. "Present iff even(d)" consumes one draw.

**v = 2, flag = 0** (3 draws). First a draw whose value is not used. Present: rooms 11, 16,
25, 22, 19, 7; room 4 iff even(d); rooms 0, 3; room 6 iff even(d); room 12 present and
marked. Links in order: (1,0,2)→(1,2,1), (1,2,1)→(0,2,1), (1,2,1)→(2,2,1),
(0,2,1)→(0,1,1), (2,2,1)→(2,1,1), (2,1,1)→(2,0,1), (2,0,1)→(0,0,0), (0,0,0)→(0,1,0),
(0,1,0)→(0,2,0), (0,1,0)→(1,1,0).

**v = 0, 1, 3, flag = 0** (4 draws). A draw s = d % 3; room 10 present; then:

* s = 1: room 19 iff even(d); rooms 13, 4; room 1 iff even(d); rooms 7, 16; room 25 present
  and marked; room 22 iff even(d). Links: (1,1,1)→(1,0,1), (2,0,1)→(1,0,1),
  (0,1,1)→(1,1,1), (0,0,1)→(0,1,1), (0,2,1)→(0,1,1), (1,2,1)→(0,2,1), (2,2,1)→(1,2,1),
  (2,1,1)→(2,2,1).
* s = 2: room 19 iff even(d); rooms 1, 4; room 7 iff even(d); rooms 13, 22; room 25 iff
  even(d); room 16 present and marked. Links: (0,0,1)→(1,0,1), (2,0,1)→(1,0,1),
  (2,1,1)→(2,0,1), (0,1,1)→(0,0,1), (1,1,1)→(0,1,1), (0,2,1)→(0,1,1), (1,2,1)→(1,1,1),
  (2,1,1)→(1,1,1), (2,2,1)→(2,1,1).
* s = 0: room 19 iff even(d); rooms 1, 4; room 7 iff even(d); room 13; room 16 iff even(d);
  room 22; room 25 present and marked. Links: (0,0,1)→(1,0,1), (2,0,1)→(1,0,1),
  (0,1,1)→(0,0,1), (1,1,1)→(0,1,1), (0,2,1)→(0,1,1), (1,2,1)→(1,1,1), (2,1,1)→(1,1,1),
  (2,2,1)→(2,1,1).

**v = 4, 5, flag = 0** (3 draws). First an unused draw. Room 14 present, quiet and fixed;
rooms 16, 25, 22, 19, 7 present; room 4 iff even(d); rooms 0, 3; room 6 iff even(d); room 12
present and marked. Links: (1,1,2)→(1,2,1), then the remaining nine links of the v = 2
table in the same order.

**flag = 1** (any v, no draws): rooms 13, 16, 4, 22, 1, 19 present. Links: (1,1,1)→(1,2,1),
(1,1,1)→(0,1,1), (1,1,1)→(2,1,1), (0,1,1)→(0,0,1), (2,1,1)→(2,0,1).

Room indices used: 0 (0,0,0), 1 (0,0,1), 3 (0,1,0), 4 (0,1,1), 6 (0,2,0), 7 (0,2,1),
10 (1,0,1), 11 (1,0,2), 12 (1,1,0), 13 (1,1,1), 14 (1,1,2), 16 (1,2,1), 19 (2,0,1),
22 (2,1,1), 25 (2,2,1).

### 1.5 Cell grid and room carving

The cell grid holds 22 × 22 × 22 two-byte cells, all zero at first, plus a rotation **rot**
(0..3) and a mirror flag **mir**, both 0 while rooms and corridors are carved. The first
byte is the cell **kind**: 0 solid/outside, 2 **pillar**, 3 **open**, 4 **entrance**. The
second byte holds flags: 1 **stair**, 2 **floor patch**, 4 **mark**.

**View transform** (0x5fa3c0, used for every cell access): a logical cell (a, b, c) is
stored at (A, B, c) with rot 0: (a, b); rot 1: (21 − b, a); rot 2: (21 − a, 21 − b);
rot 3: (b, 21 − a); then, if mir is set, B becomes 21 − B. Any access outside 0..21 uses a
scratch cell that reads kind 1 (it is reset on each such access) and writes to it are lost.
The inverse (0x5fa440) first undoes the mirror and then the rotation.

For a = 0..2 (outer), b = 0..2, c = 0..2, each **present** room:

1. If fixed: p0 = p1 = p2 = 0 and (p3, p4, p5) = (0, 0, 1).
2. Room box: corner P = (7a + 2 + p0, 7b + 2 + p1, 7c + 3 + p2), size
   S = (p3 + 3, p4 + 3, p5 + 1).
3. If the room is neither marked nor fixed: one draw; **pit** holds when d % 3 = 0.
   Otherwise pit is false and nothing is drawn.
4. Room carving (0x5ef8f0):
   1. every cell of the box becomes kind 3;
   2. a draw; if it is odd: w = d % Sx + 1 (draw), h = d % Sy + 1 (draw); starting from
      u = P.x, v = P.y: if Sx − w > 0, a draw adds d % (Sx − w + 1) to u; if Sy − h > 0, a
      draw adds d % (Sy − h + 1) to v; the cells (u … u + w − 1, v … v + h − 1, P.z) get the
      floor-patch flag;
   3. if pit and Sx > 2 and Sy > 2: w = d % (Sx − 2) + 1 (draw), h = d % (Sy − 2) + 1 (draw);
      starting from u = P.x + 1, v = P.y + 1: if Sx − w > 1, a draw adds d % (Sx − w − 1)
      to u; if Sy − h > 1, a draw adds d % (Sy − h − 1) to v; the cells
      (u … u + w − 1, v … v + h − 1, P.z … P.z + Sz − 1) become kind 2.
5. If marked: the cell (P.x + Sx/2, P.y + Sy/2, P.z + Sz/2) (integer halves) gets the mark
   flag; its kind is unchanged.

### 1.6 Corridors (0x5c52a0)

For each link in table order whose two rooms are both present, with A the first room and B
the second (boxes PA, SA and PB, SB from §1.5):

* If the x ranges overlap (PB.x < PA.x + SA.x and PA.x < PB.x + SB.x): m = PB.x + SB.x/2,
  clamped to [PA.x, PA.x + SA.x − 1]. A corridor line runs along y at x = m:
  * if PB.y < PA.y: from (m, PA.y, PA.z) to (m, PB.y + SB.y, PB.z);
  * otherwise: from (m, PA.y + SA.y − 1, PA.z) to (m, PB.y − 1, PB.z).

  Then a draw: if it is odd and PA.x < m and PB.x < m, a second line with the same ends at
  x = m − 1. Then a draw: if it is odd and m < PA.x + SA.x − 1 and m < PB.x + SB.x − 1, a
  third line at x = m + 1.
* Otherwise, if the y ranges overlap: the same with the roles of x and y exchanged (m from
  PB.y and SB.y; lines along x; the extra lines at y = m − 1 and m + 1; the same two draws).
* Otherwise nothing is carved and nothing is drawn.

Every line starts on A's side and ends on B's side. A link between present rooms therefore
costs 2 draws when the rooms overlap in x or y, and 0 otherwise.

**Line** (0x5ef6d0) from F to T: n = max(|Δx|, |Δy|, |Δz|). If n > 0, for t = 0 … n the cell
F + Δ·t/n (division truncating towards zero, per axis) becomes kind 3. Where the step's z
is higher than the previous step's z, the cell one level below the current step also
becomes kind 3 and receives the stair flag. Where the step's z is lower, the cell one level
above becomes kind 3 and the current cell receives the stair flag. In both cases the stair
flag ends up on the lower of the two levels.

### 1.7 Entrance, orientation and palette

1. After all corridors, the first link's first room (a, b, c) receives the **entrance**:
   with that room's parameters after the fixed reset, the cell
   (7a + 2 + p0, 7b + 1 + p1, 7c + 3 + p2), i.e. (P.x, P.y − 1, P.z), becomes kind 4
   whatever it held. That cell lies one step outside the room's low-y wall, on its floor
   layer, at its low-x corner. The write uses rot = mir = 0.
2. A draw sets rot = d & 3 (provisional); a draw sets mir = 1 when even(d).
3. **c1** = three draws d1, d2, d3 → (d3 % 50 + 20, d2 % 50 + 20, d1 % 50 + 20).
4. A draw gives e = d % 50; for v = 3 a further draw replaces it with d % 50 + 100.
5. **c2** = three draws, each channel d % 50 + 50 + e, third draw red; **c3** = three more
   draws by the same rule.
6. Defaults: glow rate **g** = 0.005 (f32 at 0x6fcd9c), a float triple (0.6, 0.5, 0.1) and
   a style flag 0. Then by v:
   * v = 1, 2: style flag 1, triple (0, 0.5, 0.1); c2 is redrawn from three draws as
     (d3 % 20 + 50, d2 % 50 + 50, d1 % 50 + 50); c3 likewise from three more; g = 0.0015;
   * v = 4: c2 = (d3 % 50 + 100, d2 % 50 + 80, d1 % 50 + 20) from three draws, c3 likewise;
     g = 0.0025;
   * v = 5: c2 = (255, 210, 130), c3 = (200, 180, 80), no draws.
7. **Arch model**: slot 2545 (`temple-arc.cub`) for v = 4 or 5, otherwise slot 2544
   (`castle-arc.cub`).

Draws in steps 2–6: 2 + 3 + 1 + [v = 3] + 6 + 6·[v ∈ {1, 2, 4}].

### 1.8 Raised floors (v ∈ {0, 1, 2, 3})

For each logical column (i, j), i = 0..21 (outer), j = 0..21: w = 1; unless v = 2, a draw,
and if d % 6 = 0 a further draw sets w = d % 3 + 2. Then scan c from 21 down to 0 for the
first cell whose kind is neither 0 nor 4; the cells c + 1 … min(c + w, 21) above it become
kind 2 without regard to their previous kind (an entrance cell in that range is
overwritten). A column without such a cell is left alone. Cells are accessed through the
view with the provisional rot and mir.

Draws: none for v = 2; otherwise 484 plus one per column whose first draw is a multiple
of 6.

### 1.9 Fitting the dungeon to the ground (no draws)

For k = 0..3: rot becomes (rot + 1) & 3. The entrance cell is located in logical
coordinates: the found cell starts as (0, 0, 0), then c runs from 21 down to 0 (outer),
a = 0..21, b = 0..21 (inner), and every kind-4 cell replaces the found one, so the winner is
the kind-4 cell with the lowest c, then the highest a, then the highest b. If there is no
kind-4 cell, (0, 0, 0) is used.

With (X, Y) = (x0 + 10a, y0 + 10b) for the found cell, the column (X + 5, Y + 5) is sampled.
If it is missing, **the builder ends** at once (whatever was already appended stays).
Otherwise g0 = that column's top, lowered while the block at g0 has type 0, plus 1. **wet**
holds when some block among (X + i, Y + j, g0 − 5 + k), i, j, k = 0..9, has type 2 or 3.
score = g0 + 100·wet − 10c. A rotation replaces the best when no best exists yet or its
score is strictly smaller; its wet flag and c are kept.

The final rot is the best rotation. **base** = the best score; base += 50 if v ∈ {4, 5};
base −= 100 if the winner was wet; if base + 10c < 0, base = −10c. Logical cell (a, b, c)
then occupies blocks (x0 + 10a … + 9, y0 + 10b … + 9, base + 10c … + 9). mir keeps its
value from §1.7 step 2.

Between grid creation and this step the only grid writes are the room carving, the mark,
the corridors, the entrance and the raised floors.

### 1.10 Building pass

Notation:

* Cell (a, b, c) is a logical cell read through the view with the final rot and mir;
  **kind(a, b, c)** is its kind, its flags as in §1.5; cells outside 0..21 read kind 1.
* X = x0 + 10a, Y = y0 + 10b, Z = base + 10c (the cell's lowest block corner).
* **closed(k)**: k ∈ {0, 1, 4}. **solid(k)**: k ∈ {0, 1, 2} (0x5eea40).
* **Block solid**: the block read with the implicit read rules has a type other than 0
  and 2 (0x43b480).
* g, c1 … c5 are the colours and rate of §1.7 and §1.10.1; K1, K2 from §1.2.

#### 1.10.1 Palette after the fit

Six draws give **c4** = (d3 % 180 + 50, d2 % 180 + 50, d1 % 180 + 50) from the first three
and **c5** by the same rule from the next three. For v ∈ {0, 3} one further draw follows
whose value is unused.

#### 1.10.2 Helpers (none re-seeds)

1. **Box filler** (0x5df600): box(x, y, z, size (sx, sy, sz), colour C, rate r, flags f).
   * A 23-entry table t is zero except t[0], t[8], t[16], which take d % 20 from three
     draws in that order.
   * Cells: i = 0..sx − 1 (outer), j = 0..sy − 1, k = sz − 1 down to 0 (inner); one draw per
     cell whether or not its column exists.
   * Draw count: **3 + sx·sy·sz** when all sizes are positive, otherwise **3**.
   * For cell (x + i, y + j, z + k): v = t[((7k + i)/2 + 7·((j + 7k)/2)) mod 23] (integer
     halves). The moss weight m = moss(x + i, y + j, z + k) where moss(X, Y, Z) is
     mo · (0.7 · (0.1 · N(0.2Y + 534, 0.2Z + 13) + N(0.05X + 4343, 0.1Z + 84734)) + 0.2)
     with each product and sum rounded to f32, mo the column's stored moisture (or the
     moisture blend when the column is missing), clamped to [0, 1].
   * Colour: red C.R + v, green m·(120 − (C.G + v)) + (C.G + v), blue C.B + v, computed in
     float, each clamped to [0, 255] and truncated.
   * Type byte: f | 0x46 (rock with the structure bit) when the cell's draw divided by
     32767 (float) is greater than r; otherwise f | 0x4d (glowing type 13 with the
     structure bit). The block is written with the block writer (missing columns ignore it).
2. **Carve** (0x59aba0): carve(x, y, z, size, rough). For i = 0..sx − 1 (outer),
   j = 0..sy − 1, k = sz − 1 down to 0: write colour (0, 0, 0) with type byte **0xC0** at
   (x + i, y + j, z + k). When rough, after each such write, for each box face the cell
   touches, in the order −x (i = 0), −y (j = 0), +x (i = sx − 1), +y (j = sy − 1), the block
   just outside that face is also written as (0, 0, 0, 0xC0) when a noise value exceeds 0.5.
   With (X′, Y′, Z′) the carved cell, the x faces use N(0.1Y′ + 13X′, 0.2Z′) and the y faces
   use N(0.1X′ + 13Y′, 0.2Z′). No draws.
3. **Plain fill** (0x5cb550): fill(x, y, z, size, C): every cell, in the box filler's order,
   becomes (C, 0x46). No draws.
4. **Wall furniture** (0x5f6e20): furniture(position P in fx, side s, style t) yields a
   static object:
   * a draw; if d % 50 = 0: kind **10** (`chest-base02`), scale (1.5, 1, 1), rotation
     (s + 2) & 3, and P shifted half a block (32768 fx): s = 0: y − ½, s = 1: x + ½,
     s = 2: y + ½, s = 3: x − ½. 1 draw;
   * otherwise a draw k = d % 3:
     * k = 0, shelf, a draw selects kind: t = 3 → 32 + d % 3 (`shelf1..3`); t = 4 →
       38 + d % 3 (`stone-shelf1..3`); t = 5 → 41 + d % 3 (`sandstone-shelf1..3`); any
       other t → 35 + d % 3 (`castle-shelf1..3`); scale (2, 1, 1.57); rotation s; no shift.
       3 draws;
     * k = 1, table: kind 13 (`stone-table`) if t = 4, 14 if t = 5, otherwise 12 (`table`);
       scale (3, 3, 1); rotation s; a draw gives the shift o = trunc(f32((d/32767 + 1) ·
       65536)) in fx (one to two blocks), applied in the same directions as the chest.
       3 draws;
     * k = 2, stool: kind 15 (`stone-stool`) if t = 4, 17 (`sandstone-stool`) if t = 5,
       otherwise 16 (`stool`); scale (1, 1, 0.5); rotation s; a draw gives the shift
       o = trunc(f32(d/32767 · 65536)) (zero to one block). 3 draws.
5. **Torch sprite** (0x5f8960): torch(style t, P, angle A): a sprite with scale fields
   (1, 1, 1), +0x38 = 2, +0x20 = 0.0625, +0x24 = A (degrees), position P, then by t:
   * t = 0: a draw; even → kind **50** (`castle-chain`) raised by 2 blocks; odd → lamp;
   * t = 1, 2: a draw k = d % 3: 0 → kind 50; 1 → 49 (`wall-skull`); 2 → 52 (`torch-green`)
     with +0x38 = 1 and tint (0, 0.5, 0.1);
   * t = 3: kind 48 (`crest1`), no draw;
   * t = 4: a draw; odd → a further draw selects kind 44 + (d & 3) (`inca-art1..4`);
     even → lamp;
   * t = 5: lamp, no draw;
   * **lamp**: kind 54 (`torch-yellow`), +0x38 = 1, tint (0.8, 0.7, 0.2).
6. **Rarity** (0x5f8530): rarity(r, force): q = d % (r + 1); then three draws add 1 each
   when d % 100 = 0, d % 1000 = 0, d % 10000 = 0 respectively; if force, q = r + 1; the
   result is min(q, 4). Always 4 draws.
7. **Spawner inventory** (0x5c9f90 with 0x5f8750): the creature's inventory (+0xf6c) is
   reset to **10** item records, each filled in order:
   * a draw; if odd: **coins**: a draw r; F = f32(2 ^ (f32(2r/32767 + 1) · f32(0.25·lv)))
     (power in double, result rounded to f32); a draw d; the item is category 12,
     subtype 0, byte +0x0d = 10, count +0x10 = d % (2·trunc(F) + 2). 3 draws;
   * if even: count +0x10 = lv; a draw; if d % 6 = 0: a draw k = d % 5: 0 → (category 1,
     subtype 1); 1 → coins as above with subtype 4 and +0x0d = 12 (2 more draws); 2 → coins
     with subtype 0 and +0x0d = 12 (2 more draws); 3 → (1, 7); 4 → (1, 5). If d % 6 ≠ 0:
     a draw; if odd: category 18, subtype 0, +0x04 = d % 3 from a further draw, +0x0d = 0;
     if even: the record stays empty.

   3 to 5 draws per record; 30 to 50 per inventory.
8. **Item generators** 0x5f7a60 and 0x5f6d50 produce random equipment and belong to the
   items contract; their draws are not counted here.

#### 1.10.3 Shell, walls and roofs (per column)

For each logical column (a, b), a = 0..21 (outer), b = 0..21:

1. **m** = the lowest column top (bottom + count) over (X + i, Y + j), i, j = 0..9.
2. found = false, ent = false. For c = 21 down to 0, skipping cells of kind 0, with
   k = kind(a, b, c):
   1. If not found: **Htop** = base + 10(c + 1), minus 10 when k = 4.
   2. If k = 4: ent = found = true; no box.
   3. Otherwise, if not found (the topmost cell) and base + 10c + 5 > m:
      * v = 2: box(X, Y, Z + 11, (10, 10, 3), c2, 0, 0);
      * v ∈ {0, 1, 3}: **parapets** on the sides −x, +x, −y, +y in that order, each only
        if closed(kind) holds for the neighbour on that side at level c **and** at c + 1.
        With Zr = Z + 10:
        * −x: box(X − 3, Y − 3, Zr, (2, 16, 3)); then for t = 0..7, q = Y − 2 + 2t: if the
          block (X − 3, q, Zr + 3) reads air or water, it is written (0, 0, 0) with type
          byte 0x40; then box(X − 3, q − 1, Zr + 3, (1, 1, 1));
        * +x: box(X + 11, Y − 3, Zr, (2, 16, 3)); for t, q = Y − 1 + 2t: the same test and
          air at (X + 12, q, Zr + 3); box(X + 12, q − 1, Zr + 3, (1, 1, 1));
        * −y: box(X − 3, Y − 3, Zr, (16, 2, 3)); for t, q = X − 2 + 2t: test and air at
          (q, Y − 3, Zr + 3); box(q − 1, Y − 3, Zr + 3, (1, 1, 1));
        * +y: box(X − 3, Y + 11, Zr, (16, 2, 3)); for t, q = X − 1 + 2t: test and air at
          (q, Y + 12, Zr + 3); box(q − 1, Y + 12, Zr + 3, (1, 1, 1)).

        All in colour c2, rate 0, flags 0; 131 draws per parapet;
      * additionally for v = 3, if closed(kind) holds for all four neighbours at level c:
        a **stepped roof**: box(X − 2, Y − 2, Z + 10, (14, 14, 4), c1, 0, 0), then for
        t = 0..6: box(X − 2 + t, Y − 2 + t, Z + 14 + 2t, (14 − 2t, 14 − 2t, 2), c1, 0, 0);
      * the centre column's ground height is then read without effect.
   4. Unless step 2 applied: the **shell** box(X − 2, Y − 2, Z − 2, (14, 14, 14), c2, g, 0),
      2,747 draws. Every non-zero, non-entrance cell gets one.
   5. found = true.
3. After the c loop, if found or v ∈ {4, 5}, and the column (X, Y) exists:
   **Bmin** = the lowest column bottom over (X − 2 … X + 11, Y − 2 … Y + 11), starting from
   the bottom of (X, Y) and ignoring missing columns. Then:
   * v ∈ {0, 1, 2, 3} (found only): **foundation** box(X − 2, Y − 2, Bmin,
     (14, 14, Htop − Bmin), c2, g, f) with f = 0x80 when ent, else 0;
   * v = 4: the pyramid step of §1.10.4 (every column, found or not);
   * v = 5: nothing here.

#### 1.10.4 The v = 4 pyramid

Let (ae, be, ce) be the entrance cell found in §1.9 (logical, final rot). For column (a, b):
m = max(|a − 10|, |b − 10|), e = 10 − m, minus a further 1 when a = b = 10;
**H** = base + 10·(ce − 5 + e), plus 2 when |a − 10| ≤ 1 and |b − 10| ≤ 1.

1. box(X − 2, Y − 2, Bmin, (14, 14, H − Bmin − 4), c2, g, 0).
2. If H − Bmin > 0:
   1. box(X − 1, Y − 1, H − 4, (12, 12, 3), c2, 0, 0); box(X, Y, H − 1, (10, 10, 3), c2, 0, 0).
   2. **Grass skin**: for i = −2..11 (outer), j = −2..11: z = H + 2, less 6 when i or j is
      −2 or 11, otherwise less 3 when i or j is −1 or 10. If N(0.02(X + i), 0.02(Y + j)) > 0
      (double arguments) and the block (X + i, Y + j, z) reads air or water, it becomes the
      terrain surface block for that position (terrain contract §6.1), whose two climate
      inputs come from the column blends 0x5c8350 and 0x5c8310. No draws.
3. **Stairs** on the centre lines, k = 0..9 each, all boxes in colour c2 at rate g:
   * a = 10, b < 9 (|b − 10| > 1, b < 10): box(X − 2, Y − 1 + k, H, (14, 1, k + 2)) for each
     k; then box(X − 2, Y − 1 + k, H, (2, 1, k + 4)); then box(X + 10, Y − 1 + k, H,
     (2, 1, k + 4));
   * a = 10, b > 11: the same three runs with y = Y + 10 − k;
   * b = 10, a < 9: box(X − 1 + k, Y − 2, H, (1, 14, k + 2)); then box(X − 1 + k, Y − 2, H,
     (1, 2, k + 4)); then box(X − 1 + k, Y + 10, H, (1, 2, k + 4));
   * b = 10, a > 11: the same with x = X + 10 − k.

#### 1.10.5 The v = 5 mound

After the column loop of §1.10.3, if v = 5: for x = x0 … x0 + 219 (outer),
y = y0 … y0 + 219, for each existing column with bottom **bot** and
d = max(|x − (x0 + 105)|, |y − (y0 + 105)|): box(x, y, bot, (1, 1, base + 10ce + 59 − d − bot),
c2, g, 0). This forms a square pyramid of slope 1; each column costs
3 + max(0, base + 10ce + 59 − d − bot) draws. The centre column's ground height is then
read without effect.

#### 1.10.6 Interior pass

For a = 0..21 (outer), b = 0..21, c = 0..21 (inner), only cells with k = kind(a, b, c) ∈
{3, 4}, in order:

1. **Exit marker**: if k = 4, a marker record of type 5 with position (X + 5, Y + 5, Z)
   (fx) is appended to the zone list at zone + 0x48. No draws.
2. **Carve** the cell: carve(X, Y, Z, (10, 10, 10), rough when v ∈ {1, 2, 4}).
3. **Windows** (v ∈ {0, 1, 3}, k ≠ 4, stair flag clear): sides −y, +y, −x, +x in that
   order. Per side: a draw; if it is odd **and** the neighbour on that side (level c) has
   kind 0 **and** the test block is not solid: carve(cx, cy, Z + 2, (2, 2, 4), not rough)
   and a static object of kind **4** (`castle-window`), scale (2, 0.2, 4), at the window
   position:

   | Side | Test block | Carve corner (cx, cy) | Window position | Rotation |
   |---|---|---|---|---|
   | −y | (X + 5, Y − 3, Z + 2) | (X + 4, Y − 2) | (X + 5, Y − 1, Z + 2) | 0 |
   | +y | (X + 5, Y + 13, Z + 2) | (X + 4, Y + 10) | (X + 5, Y + 11, Z + 2) | 2 |
   | −x | (X − 3, Y + 5, Z + 2) | (X − 2, Y + 4) | (X − 1, Y + 5, Z + 2) | 3 |
   | +x | (X + 13, Y + 5, Z + 2) | (X + 10, Y + 4) | (X + 11, Y + 5, Z + 2) | 1 |

   One draw per side. [unconfirmed: the −x and +x rows apart from the −x test block, the
   −x carve corner and the +x rotation.]
4. **Stairs** (any v, stair flag set): each of the four cases below that holds builds a
   staircase of eleven boxes t = 0..10 in colour c3 at rate g (the four tests are made one
   after another):
   * kind(a + 1, b, c) ≠ 3, kind(a − 1, b, c) = 3, kind(a + 1, b, c + 1) = 3:
     box(X − 1 + t, Y, Z − 1 + t, (11 − t, 10, 1));
   * kind(a − 1, b, c) ≠ 3, kind(a + 1, b, c) = 3, kind(a − 1, b, c + 1) = 3:
     box(X, Y, Z − 1 + t, (11 − t, 10, 1));
   * kind(a, b + 1, c) ≠ 3, kind(a, b − 1, c) = 3, kind(a, b + 1, c + 1) = 3:
     box(X, Y − 1 + t, Z − 1 + t, (10, 11 − t, 1));
   * kind(a, b − 1, c) ≠ 3, kind(a, b + 1, c) = 3, kind(a, b − 1, c + 1) = 3:
     box(X, Y, Z − 1 + t, (10, 11 − t, 1)).

   A staircase costs 33 + 660 = **693 draws**.
5. If v = 4 and k = 4: continue with the next cell.
6. If kind(a, b, c − 1) = 3: skip to step 8.
7. **Floor and furnishing**:
   1. Floor: box(X, Y, Z, (10, 10, 1), c3, g, 0), **103 draws**.
   2. Carpet (v ∈ {0, 3} and the floor-patch flag): x1 = X, x2 = X + 10, y1 = Y,
      y2 = Y + 10; if the cell at a − 1 lacks the patch flag, x1 += 2 and the −x border is
      due; likewise a + 1 → x2 −= 2 (+x border), b − 1 → y1 += 2 (−y border), b + 1 →
      y2 −= 2 (+y border). fill(x1, y1, Z, (x2 − x1, y2 − y1, 1), c4); then the due borders
      in the order −x, +x, −y, +y: fill(x1, y1, Z, (1, y2 − y1, 1), c5),
      fill(x2 − 1, y1, Z, (1, y2 − y1, 1), c5), fill(x1, y1, Z, (x2 − x1, 1, 1), c5),
      fill(x1, y2 − 1, Z, (x2 − x1, 1, 1), c5). No draws.
   3. If k = 4: skip to step 9. If the stair flag is set: skip to step 8.
   4. **Trap** (flag = 0 only): a draw; if d % 40 = 0: static kind **7** (`spike-trap`) at
      (X + 5, Y + 5, Z + 1), scale (10, 10, 10); a draw gives rotation d & 3; a draw gives
      the record's field +0x34 = d % 4000; inserted; skip to step 8.
   5. n0 = the current number of statics in the zone.
   6. **Ceiling decorations**, only if solid(kind(a, b, c + 1)):
      * a draw; if d % 3 = 0 and v ∈ {2, 4}: a **liana** sprite (kind 55, scale fields 1,
        +0x38 = 2): a draw sets +0x24 = f32(d/32767 · 360); a draw sets
        s = f32(f32(0.04d)/32767 + 0.08) into +0x20; z = f32((Z + 10) − 20s); a draw gives
        y = Y + 2 + d % 5; a draw gives x = X + 2 + d % 5; appended to zone + 0x04;
      * if v ∈ {1, 2}: **cobwebs** on the sides −x, +x, +y, −y in that order. Per side: a
        draw; if d % 3 = 0 and solid(kind of that neighbour at level c): a sprite with
        +0x24 = 0, 180, 270, 90 respectively; a draw sets s = f32(f32(0.05d)/32767 + 0.08)
        into +0x20; z = Z + 10 − 20s; a draw gives q = 2 + d % 5; position −x:
        (X + 10s, Y + q, z); +x: (X + 10 − 10s, Y + q, z); +y: (X + q, Y + 10 − 10s, z);
        −y: (X + q, Y + 10s, z); a draw selects kind 57 + (d & 1) (`cobwebs`, `cobwebs2`);
        appended to zone + 0x04.
   7. **Wall decorations**, sides −x, +x, −y, +y. Per side: a draw; if d % 3 = 0 and
      solid(kind of that neighbour at level c):
      1. a draw; if odd: **torch**: a draw r = d & 3 for the position along the wall
         (table below); if the block one step beyond the torch towards the wall is solid,
         torch(v, P, A) (item 5, with its own draws) is appended to zone + 0x04.
      2. a draw; if d & 7 ≠ 0: **wall furniture**: a fresh draw r = d & 3 for the position;
         furniture(P, s, t = v) inserted. If d & 7 = 0: a **guard** creature: +0x28 = 6,
         kind +0x2c = **143**, +0x34 = lv, +0xf58 = 10.0, +0x08 = 150.0, position from a
         fresh draw r = d & 3; inventory per item 7; appended to zone + 0x18.

      | Side | Torch P, angle A | Furniture P, side s | Guard position |
      |---|---|---|---|
      | −x | (X + 0.5, Y + 3 + r, Z + 2), 270 | (X + 0.5, Y + 3 + r, Z + 1), 1 | (X + 1, Y + 3 + r, Z + 1) |
      | +x | (X + 9.5, Y + 3 + r, Z + 2), 90 | (X + 9.5, Y + 3 + r, Z + 1), 3 | (X + 9, Y + 3 + r, Z + 1) |
      | −y | (X + 3 + r, Y + 0.5, Z + 2), 0 | (X + 3 + r, Y + 0.5, Z + 1), 2 | (X + 3 + r, Y + 1, Z + 1) |
      | +y | (X + 3 + r, Y + 9.5, Z + 2), 180 | (X + 3 + r, Y + 9.5, Z + 1), 0 | (X + 3 + r, Y + 9, Z + 1) |

      [unconfirmed: the exact one-block offset used for the torch wall test.]
   8. **Loot**, for every static added since n0, in vector order:
      * kind 10 (chest): a draw gives n = (d & 3) + 1; n items from the item generator
        0x5f7a60 for (lv, sl + 1) are added to the chest's contents (+0x48); the chest's
        byte +0x30 = 2;
      * kinds 12, 13, 35, 36, 37: for u = 0, 1, … while u < scale x, and inside it
        w = 0, 1, … while w < scale y: a draw; if d % 10 = 0: a ground item with
        +0x134 = 0.06 and +0x138 = 1. A draw: if d % 6 ≠ 0, a draw k = d & 3: 0 → (category
        11, subtype 26); 1 → (18, subtype = the style flag of §1.7), +0x04 = d % 3 (draw);
        2 → (11, 9), +0x0d = d % 3 + 25 (draw); 3 → (1, 7). If d % 6 = 0, a draw k = d % 6:
        0 → (1, 1); 1 → (1, 4); 2 → (1, 5); 3 → coins as in §1.10.2 item 7 (2 draws),
        +0x0d = 10; 4 → item generator 0x5f7a60 for (lv, sl); 5 → item generator 0x5f6d50
        for (lv, sl). Position = the static's position plus fx of
        (u − 0.5·sx + 0.5, w − 0.5·sy + 0.5, sz) (sx, sy, sz its scale); a draw gives
        rotation +0x130 = f32(f32(360d)/32767); appended to zone + 0x30.
8. **Arches and chandelier**, only if k ≠ 4 and kind(a, b, c + 1) ≠ 3:
   * if the block (X, Y, Z + 10) is solid: for each side −x, +x, −y, +y whose neighbour at
     level c is not open (kind ≠ 3), the arch model is placed with the prefab placer (§6.2)
     at (X, Y, Z), rotation 0, 2, 3, 1 respectively, type byte 0x46, mode 0, the zone, no
     fill-down, no crop, no output list. Mode 0 makes no draws;
   * v = 3: a draw; if d % 10 = 0: a **chandelier** sprite, kind 56, at
     (X + 5.5, Y + 5.5, Z + 10 − 2.7) (the z sum in double), +0x20 = 0.1, +0x24 = 0,
     +0x38 = 1, tint = the float triple of §1.7; appended to zone + 0x04.
9. **Boss** (mark flag set): a creature at (X + 4.5, Y + 4.5, Z + 1), +0x28 = 1,
   +0x7a |= 0x1000, +0x34 = max(lv, 1), byte +0x58 = sl; a draw selects kind
   K1[d mod |K1|]; one item from the item generator 0x5f7a60 for the creature level with
   rarity(sl, force) (4 draws, made first) is added to the inventory (+0xf6c);
   +0x7a |= 0x200; a marker record of type 6 (position, kind, creature index) is appended to
   zone + 0x48; a draw gives n = d & 3 items (category 1, subtype 1, count = level) added;
   +0x08 = 150.0, +0x10e8 = 1, and a helper object stored at +0x109c; appended to
   zone + 0x18.

#### 1.10.7 Central shafts (v = 4 and v = 5)

After the interior pass. Xc = x0 + 100, Yc = y0 + 100 (logical cell (10, 10)),
**Ze** = base + 10ce + 8.

* v = 4: Zt = base + 10ce + 44. box(Xc − 2, Yc − 2, Zt, (14, 14, 16), c2, g, 0);
  carve(Xc, Yc − 2, Zt, (10, 14, 10)); carve(Xc − 2, Yc, Zt, (14, 10, 10));
  carve(Xc, Yc, Ze, (10, 10, Zt − Ze)). Then **Zc** = Zt.
* v = 5: Zt = base + 10ce + 38. box(Xc − 2, Yc − 2, Zt, (14, 14, 14), c2, g, 0); then
  **Zc** = Zt − 12 = base + 10ce + 26; carve(Xc + 2, Yc − 32, Zc, (6, 74, 6));
  carve(Xc − 32, Yc + 2, Zc, (74, 6, 6)); carve(Xc, Yc, Ze, (10, 10, Zc − Ze + 6)).
* Both: a **pillar** box(Xc + 4, Yc + 4, Ze − 30, (2, 2, n), c2, g, 0) with
  n = Zc − Ze + 30 (66 for v = 4, 48 for v = 5); then, if n > 1, a **spiral**: for
  z = Ze − 29, Ze − 28, … while z − Ze + 30 < n, with u = z − Ze + 30, one box in colour c2
  at rate g chosen by u & 7:

  | u & 7 | Corner | Size |
  |---|---|---|
  | 0 | (Xc, Yc, z) | (5, 5, 1) |
  | 1 | (Xc + 5, Yc, z) | (1, 5, 1) |
  | 2 | (Xc + 6, Yc, z) | (5, 5, 1) |
  | 3 | (Xc + 6, Yc + 5, z) | (5, 1, 1) |
  | 4 | (Xc + 6, Yc + 6, z) | (5, 5, 1) |
  | 5 | (Xc + 5, Yc + 6, z) | (1, 5, 1) |
  | 6 | (Xc, Yc + 6, z) | (5, 5, 1) |
  | 7 | (Xc, Yc + 5, z) | (5, 1, 1) |

  Both styles use identical spiral tables.

#### 1.10.8 Erosion (v ∈ {1, 2}, no draws)

For grid columns a = 0..21 (outer), b = 0..21, then x = X … X + 9 (outer), y = Y … Y + 9:
e = f32(2·N(0.1x, 0.1y)) + f32(8·N(0.05x, 0.05y)) (float sum), and e = 0 when positive.
With top = bottom + count and h = trunc(f32(top + e)): for z = top down to h, a block whose
type byte has bit 0x40 set and bit 0x80 clear becomes the implicit-air block
(255, 255, 255, type 0).

#### 1.10.9 Room spawns

For rooms (a, b, c) of §1.3, a outer, c inner, each **present and not quiet**, with P and S
from §1.5. The corner cells Q0 = P, Q1 = (P.x + S.x − 1, P.y, P.z),
Q2 = (P.x + S.x − 1, P.y + S.y − 1, P.z), Q3 = (P.x, P.y + S.y − 1, P.z) are converted from
storage to logical coordinates with the inverse view. A creature "at Q" stands at
(x0 + 10Q.x + 5, y0 + 10Q.y + 5, base + 10Q.z + 1). Every creature has +0x34 = lv and AI
helper objects (no draws) and is appended to zone + 0x18. **Kind rule**: if flag ≠ 0:
+0x28 = 3 and a draw gives kind d & 1; otherwise +0x28 = 1, +0x7a |= 0x1000, a draw gives
the kind from the named list (unsigned modulus), and byte +0x58 = sl.

1. **Group** (S.x·S.y > 6 and K2 non-empty): a draw selects G = K2[d mod |K2|]; if G's
   leaders are empty, go to 2. A draw k = d & 3; the leader at Qk with the kind rule on
   G.leaders; a draw gives route = d % 3 (patrol waypoints over the corners, no draws);
   +0x08 = 150.0; a draw gives n = d & 1 items (category 1, subtype 1, level) added. Then,
   if flag = 0 and G.escorts is non-empty: a draw gives n = d % 3 + 1 escorts, each at the
   leader's position with +0x28 = 1, +0x7a |= 0x1000, a draw for kind
   G.escorts[d mod |G.escorts|], +0x58 = the leader's byte, +0xf60 multiplied by 0.5,
   +0x08 = 150.0 and the leader's route pair copied to +0x48/+0x4c.
2. **Rings** (K1 non-empty and the room not marked): a draw gives G = (d & 1) + 1; a draw
   gives k = d & 3; if S.x·S.y > 7, G += 1. For g = 0..G − 1: a draw gives m = (d & 1) + 2;
   corner Q = Q((k + g) & 3); for j = 0..m − 1: θ = f32(2jπ/m) (π the double
   3.141592653589793, division in double); a creature at
   (x0 + 10Q.x + 5 + f32(2 cos θ), y0 + 10Q.y + 5 + f32(2 sin θ), base + 10Q.z + 1)
   (cosine and sine in double of the float θ, rounded to f32; x and y float sums → fx),
   facing +0x54 = f32((θ/π)·180 + 90), +0x08 = 150.0, +0xf60 multiplied by 0.75, kind rule
   on K1, then a draw gives n = d & 1 items (1, 1) added.
3. **Centre** (K1 non-empty, room not marked, S.x > 2 and S.y > 2): a draw; if odd:
   Qm = the midpoint of Q0 and Q2 (component-wise integer halves); if kind(Qm) = 3: a draw
   gives m = (d & 1) + 2 and a ring of m creatures around Qm as in step 2.

### 1.11 Draw summary

Up to the end of §1.9: [v ∈ {1, 2}] + 162 + the link-table draws (3 for v = 2, 4 or 5;
4 for v ∈ {0, 1, 3}; 0 for flag = 1) + one pit draw per present room that is neither marked
nor fixed + the room carving draws (1, 3, 4 or 5 for the floor patch, plus 0 or 2–4 for a
pit) + 2 per overlapping link + 12 to 18 for §1.7 + §1.8.

Then in order: 6 or 7 (§1.10.1); per column of §1.10.3 the listed boxes (2,747 per shell,
3 + 196·h per foundation of height h, 131 per parapet, 303 for the v = 2 cap); §1.10.4 or
§1.10.5; per interior cell of §1.10.6 the window, stair (693), floor (103), trap, ceiling,
wall and loot draws; §1.10.7; §1.10.9. Terrain enters the counts through the box heights
that use Bmin or column bottoms, the window and torch block tests; the parapet air tests do
not change the count. The item generators add their own draws (items contract).

### 1.12 Worked examples

* View transform, rot 1, mir 1: logical (3, 5) → (21 − 5, 3) = (16, 3) → mirror → (16, 18).
  Inverse: (16, 18) → unmirror (16, 3) → (3, 21 − 16) = (3, 5).
* Room carving with S = (3, 3, 2), no pit: first draw even → 1 draw; odd with w = 3,
  h = 3 → 3 draws; odd with w = 1, h = 2 → 5 draws.
* Box filler: (14, 14, 14) → 2,747 draws; (10, 10, 3) → 303; (10, 10, 1) → 103;
  (14, 14, 0) or (14, 14, −5) → 3.
* With the stream 440, 19053, 23075, 13104, … (the first values after seeding with 123):
  box table t[0] = 0, t[8] = 13, t[16] = 15; wall furniture on side 1, style 0:
  440 % 50 = 40 (no chest), 19053 % 3 = 0 (shelf), 23075 % 3 = 2 → kind 37
  (`castle-shelf3`), 3 draws; spawner record with lv = 5: 440 even → count 5,
  19053 % 6 = 3, 23075 odd → category 18, 13104 % 3 = 0 → +0x04 = 0, 4 draws;
  rarity(2, false): 440 % 3 = 2, no bonus → 2.
* Coins with lv = 5 and r = 19053: exponent f32(38106/32767 + 1)·1.25 = 2.703673,
  F = 6.514583, modulus 14; with d = 23075 the count is 3.
* Table shift for d = 19053: trunc(f32(1.5814691 · 65536)) = 103643.
* Ring offsets (2 cos θ, 2 sin θ): m = 2: (2, 0), (−2, −1.7484555e−07); m = 3: (2, 0),
  (−1.0000001, 1.7320508), (−0.9999998, −1.7320509).
* Staircase 33 + 660 = 693 draws; parapet 3 + 96 + 32 = 131 draws.
* v = 4 heights with base = 100, ce = 5: column (10, 10) H = 192; (9, 10) H = 192;
  (8, 10) H = 180; (0, 0) H = 100.
* v = 5 mound with base = 100, ce = 5: the centre column reaches z = 209 (top block 208);
  a column 40 blocks away along x tops out at 168.
* v = 5 shaft: Zc − Ze = 18, central carve height 24, pillar height n = 48.

---

## 2. Camps (zone generator 0x5e8d86..0x5e8f8d, camp routine 0x5ad320)

### 2.1 The attempts

Only when (zx + zy) is odd. Up to 10 tries; per try:

1. A draw gives py = 256zy + 48 + d % 160; then a draw gives px = 256zx + 48 + d % 160
   (**y is drawn first**).
2. P = (px·65536 + 32768, py·65536 + 32768, 0) in fx.
3. If Lz has type 1 or 5: v = 1 − the location distance (regions contract §7, routine
   0x5f8e10) of Lz at P; if v > 0 and v² > 0, the try **fails** with no further draws.
4. Otherwise, if the column (px, py) exists, P.z = its top · 65536.
5. The camp routine (§2.2) runs. If it succeeds, an object-list node holding P
   (caves contract §3.1) is appended and the attempts **stop**; otherwise the next try
   follows.

Per try: 2 draws plus the camp routine's 0, 1 or 9.

### 2.2 The camp routine

Input: the zone and the fx position P. (bx, by) = (P.x / 65536, P.y / 65536) (64-bit,
towards zero).

1. **Roughness**: R = R(bx, by) (terrain contract §3.3). If 1 − 50R ≥ 0 (float comparison
   with 0), the routine fails with 0 draws. Camps therefore need R > 0.02.
2. **Campfire**: static kind **65** (`campfire.cub`), scale (2.4, 2.4, 0.5); a draw gives
   rotation d & 3.
3. For i = 0..2 (outer, x) and j = 0..2 (inner, y) the fire is tried at
   (P.x + 65536i, P.y + 65536j, P.z) with the settle test (§2.3, floor check on). The first
   position that passes is inserted at its settled height. If none passes, the routine
   fails (1 draw in total).
4. **Ring**: for i ∈ {0, 7} (outer) and j ∈ {0, 7} (inner), at
   (P.x + 65536i − 229376, P.y + 65536j − 229376, P.z), i.e. ±3.5 blocks around the
   original P: a draw k = d & 3 selects

   | k | Kind | Model | Scale |
   |---|---|---|---|
   | 0 | 66 | `tent.cub` | (4, 4, 3) |
   | 1 | 16 | `stool.cub` | (1, 1, 0.5) |
   | 2 | 12 | `table.cub` | (3, 3, 1) |
   | 3 | 69 | `wood-mat.cub` | (2, 2, 0.1) |

   then a draw gives rotation d & 3. If the settle test passes, the object is appended to
   the zone's static vector at the settled height; otherwise it is dropped. Both draws
   happen either way.
5. The routine succeeds.

Draws: 0, 1 or 9. Static fields set here: +0x00 kind, +0x08/+0x10/+0x18 position (fx),
+0x20 rotation index, +0x24/+0x28/+0x2c scale, byte +0x30 = 1; other fields keep their
defaults.

The object-list node of §2.1 is what later passes measure distances against (boulder
pass 3 and the tree grid, 40 blocks).

### 2.3 The settle test (0x5f4da0)

Input: a static record, the zone and a floor-check flag (set in every use here).

1. Footprint: (sx, sy) = scale x and y, swapped when the rotation index is odd;
   hx = trunc(f32(f32(0.5·sx)·65536)), hy likewise. Columns from (x − hx)/65536 to
   (x + hx)/65536 and likewise in y (64-bit, towards zero), inclusive.
2. **Drop**: up to 50 times, if every footprint cell at z/65536 reads air or water, z is
   lowered by 65536; it stops at the first height where some footprint cell is solid.
3. **Rise**: up to 50 times, while some footprint cell at z/65536 is solid (type neither
   0 nor 2), z is raised by 65536.
4. If z ≤ 0 (64-bit fx), the test fails.
5. Floor check: every footprint cell at (z − 65536)/65536 must be solid; otherwise it fails.
6. It passes when the cell holding the position (cell-position read, §0) is not water
   (type ≠ 2).

Reads in this test: a missing column or a height below the column bottom reads solid
(200, 200, 200) type 1; above the top it reads air when z > 0 and water (type 0x82) when
z ≤ 0; a stored type-0 cell at z ≤ 0 without bit 0x40 also reads as water. The settled z is
written back into the record. No draws.

### 2.4 Worked examples

* zx = 32800, zy = 32801, draws 1000 then 2000: py = 8397056 + 48 + 40 = 8397144;
  px = 8396800 + 48 + 80 = 8396928; P = (8396928.5, 8397144.5) blocks.
* Ring offsets: ±3.5 blocks (229376 / 65536).
* Footprint half-widths: tent (4, 4) → 131072; table (3, 3) → 98304; campfire →
  trunc(f32(1.2 · 65536)) = 78643.

---

## 3. Boulder pass 3 (0x5e8f8d..0x5e9649)

Only when E.kind ∉ {1, 3, 4}; otherwise no draws.

1. A draw gives n = d % 10. Climate bonus with the zone-centre climate values A (blend
   0x5c4dd0) and B (blend 0x5c4800): n += 10 when (A > 0.6 or A < 0.3) and (B > 0.7 or
   (B < 0.4 and (B > 0.2 or A < 0.8))), all strict. If n ≤ 0, the pass ends.
2. Per boulder (n of them):
   1. three draws give ax, ay, az = (d & 7) + 3 in that order (3..10 each);
   2. a draw gives x = 2(128zx + ax) + d % (4(64 − ax)); a draw gives
      y = 2(128zy + ay) + d % (4(64 − ay)); so x ∈ [256zx + 2ax, 256zx + 255 − 2ax];
   3. **home**: skipped when f32(f32(y − homeY)² + f32(x − homeX)²) < 400 (x and y
      converted to float, the float home coordinates subtracted);
   4. **objects**: for each object-list node (fx position): the 64-bit differences
      y·65536 − y_o and x·65536 − x_o are converted to float and scaled by 2⁻¹⁶; skipped
      when the sum of their squares is < 1600;
   5. **town**: skipped when q(x, y) > 0.25 (terrain contract §3.1);
   6. **ground**: G = one above the highest stored block of column (x, y) that is neither
      air nor water, scanning down from the top; G = the column bottom when there is none;
   7. skipped when the block at (x, y, G − 1) has type 11, 8 or 7 or has bit 0x40;
   8. **shape**: for X = x − ax … x + ax (outer), u = f32((X − x)/ax); for
      Y = y − ay … y + ay: v = f32((Y − y)/ay), s2 = f32(v² + u²),
      k = f32(0.8·N(0.05X, 0.05Y)) (double arguments); for Z = G + 2az down to G − 2az:
      w = f32(f32((Z − G)/az) + k); if f32(w² + s2) ≤ 1 and the block at (X, Y, Z) has bit
      0x40 clear, it is written with the rock colour Rk(X, Y, Z) and type byte **0x26**.

Draws: 1 + 5n; skipped boulders still use their five. The centre is on the ground surface
G, and bit 0x80 is not tested. A boulder whose centre column is town ground is skipped, but
its shape can still reach neighbouring town ground.

Worked examples: x range for zx = 32800, ax = 3: 8396806 + (0..243). n = 0 without bonus →
1 draw; with the bonus and n = 0 → 51 draws.

---

## 4. The type-10 site (0x5e967f..0x5e9e7d)

Runs after the town builder call (§5) and only when E.kind = 4 (a type-10 site, locations
contract §7).

1. (cx, cy) = (256zx + 128, 256zy + 128); T = top(cx, cy).
2. An object-list node is appended at (cx·65536, cy·65536, T·65536).
3. A draw gives n = d % 3 + 6 **pillars**.
4. For k = 0 … n − 1:
   1. a = f32(2kπ/n) (π the double 3.141592653589793, computed in double); s = f32(sin a),
      c = f32(cos a) (double-precision sine and cosine of the float a);
   2. centre (px, py) = (cx + trunc(f32(25c)), cy + trunc(f32(25s))); zp = top(px, py) + 4,
      or T + 4 when that column is missing;
   3. a draw gives a1 = (d & 3) + 3; a draw gives a2 = (d & 3) + 3;
   4. for X = px − 2a1 … px + 2a1 (outer), Y = py − 2a2 … py + 2a2:
      ny = f32(0.3·N(0.05Y, 0.05Y)) (both arguments 0.05Y, in double); for Z = zp + 20 down
      to zp − 20: w = f32((Z − zp)/10), v = f32(f32((Y − py)/a2) + ny),
      u = f32(f32(0.3·N(0.05X, 0.05Z)) + f32((X − px)/a1)); if f32(f32(u² + v²) + w²) ≤ 1
      and the block has bit 0x40 clear, it is written with Rk(X, Y, Z) and type **6**.
5. **Runestone**: static kind **45** (`runestone.cub`) at
   (cx·65536 + 229376, cy·65536 + 229376, T·65536). While the cell at its position reads
   air or water, z is lowered by 65536; then while it reads solid, z is raised by 65536
   (no limit). A draw gives rotation d & 3; scale (4, 4, 5); inserted.

Draws: 1 + 2n + 1 = 14, 16 or 18. The result is a ring of six to eight rough rock pillars at
radius 25 around the zone centre with a runestone near the middle.

Pillar offsets (trunc(25c), trunc(25s)): n = 6: (25, 0), (12, 21), (−12, 21), (−25, 0),
(−12, −21), (12, −21); n = 7: (25, 0), (15, 19), (−5, 24), (−22, 10), (−22, −10), (−5, −24),
(15, −19); n = 8: (25, 0), (17, 17), (0, 25), (−17, 17), (−25, 0), (−17, −17), (0, −25),
(17, −17).

---

## 5. The town builder (0x5af170)

### 5.1 Inputs

The zone generator runs the builder for **every** zone whose sub-cell location Lz has
type 1 (town) or 5 (ruins), after boulder pass 3 and before the type-10 site
(0x5e9655..0x5e967f). All 64 zones of such a sub-cell run it; zones far from the centre
usually end early (§5.4). Inputs:

* Lz.type (+0x18), Lz.variant (+0x1c: region style 0..5 for a town, climate class for
  ruins), Lz.level (+0x24), Lz centre (+0x00, +0x08);
* the quarter number Q = byte zone + 0x79 (1..4 in the four quarter zones of a town, 0
  elsewhere; locations contract §8.2);
* w(X, Y) (0x5fa4c0) = max(1 − f(Lz, X, Y), 0)² at an fx point (f as in the regions
  contract §7); this is q of terrain §3.1 at fx positions.

An animal kind list 34, 30, 19, 26, 33 is prepared first (no draws; §5.5.10).

### 5.2 Plot survey

g = 5 plots per side for type 1, 4 for type 5; plot size s = 256 / g (51 or 64). Plot
(i, j), i = 0..g − 1 outer, j inner, has origin (ox, oy) = (256zx + 256i/g, 256zy + 256j/g)
(integer division) and record index r = i + g·j. Records hold **lo**, **hi**, **inner**,
**kind**, **sub**, **face** and **wc**.

For each plot, for a = 0..s − 1 (outer), b = 0..s − 1, column (X, Y) = (ox + a, oy + b):

1. wv = w(X·65536, Y·65536); **busy** when any wv > 0.1 (strict);
2. **bad** when any column has bottom + count ≤ 0 or stores a block whose type (low five
   bits) is 2 or 3;
3. when 7 < a < s − 7, 7 < b < s − 7 and wv < 0.65 (strict): **road** when the column stores
   a block of type 11;
4. h = bottom + count − 1; lo and hi are the minimum and maximum of h over the plot (the
   first column sets both);
5. when 8 ≤ a < s − 8, 8 ≤ b < s − 8 and inner ≠ 0: inner = h (inner starts at −10000).

Then, still for that plot:

6. a draw gives face = d & 3;
7. wc = w at the plot centre ((ox + s/2)·65536, (oy + s/2)·65536);
8. kind: if not bad and busy: with road, kind = 7; without road, a draw, and kind = 2 when
   f32(wc + 0.25) > d/32767 (strict), else 0, after which kind = 0 if hi − lo > 16. In all
   other cases kind = 0.

The stream order is face(plot 0), [kind(plot 0)], face(plot 1), … in plot order. Draws:
g² plus one per plot that is busy, not bad and without road.

### 5.3 Facing (no draws)

For i = 0..g − 1 (outer), j = 0..g − 1: up to three times, the neighbour in direction
face & 3 is examined (1: (i − 1, j), 2: (i, j + 1), 3: (i + 1, j), 0: (i, j − 1)); if it lies
inside the grid and its kind is not 2, the face stays; otherwise face increases by 1.

### 5.4 Gardens and special plots

1. **Gardens** (type 1 only): for each record in index order with kind 0 and
   hi − lo < 16: a draw; if even(d) and wc > 0, kind = 6.
2. List **B** holds the indices of kind-2 plots in index order (street plots are not in
   it). If B is empty, the builder ends here with no further draws and nothing written.
   Otherwise B is sorted by wc ascending, ties keeping index order [unconfirmed: sort
   stability]. "Pop" takes the last entry (highest wc); "pick" makes a draw and removes
   entry d mod |B| (unsigned), keeping the order of the rest.
3. **Type 1**, by Q, in this order:
   * Q = 1 and |B| > 1 (count taken after the sort): pop → kind 9 (market);
   * Q = 3: four picks, each only while B is non-empty → kinds 10, 11, 12, 13;
   * Q = 2: three picks, each only while B is non-empty → kinds 14, 16, 15;
   * Q = 3 and |B| > 1: pop → kind 3 (tower);
   * Q = 0 or 3, and |B| > 1: pop → sub = 1 (inn);
   * Q = 1: up to four pops while B is non-empty → sub = 2, 3, 4, 5;
   * Q = 0: pick (no emptiness test) → kind 5 (well);
   * then six times: if |B| > 3, pick → kind 0.

   Q = 4 has no case of its own and runs only the six final picks.
4. **Type 5**:
   * if the location centre lies in this zone (each fx coordinate divided by 65536 and
     then by 256, towards zero, equals zx and zy) and the count after the sort is > 1:
     pop → kind 20;
   * twice: if |B| > 3, pick → kind 0;
   * Lz.variant = 0: twice: if |B| > 3, pick → kind 18, then a draw sets face = d & 3;
   * Lz.variant = 2: a draw; if odd and |B| > 3: pick → kind 17 and a draw sets
     face = d & 3. Then a further draw (made regardless); if odd and |B| > 3: pick →
     kind 19 and a draw sets face = d & 3.

Plot kinds:

| Kind | Made by | Building | Model |
|---|---|---|---|
| 0 | default | nothing | — |
| 2 | survey | modular house (§5.5.4) | module set |
| 3 | Q = 3 | tower | 2177 `frame-tower` |
| 5 | Q = 0 | well | 2178 `building-stone-well` |
| 6 | gardens | field with crops and fences | sprite kinds 21, 23, 24, 25, 26, 29, 30 |
| 7 | survey (road blocks) | nothing besides §5.5.10 | — |
| 9 | Q = 1 | market | static kinds 21..27 |
| 10..16 | Q = 3, 2 | class and craft houses | 2303 `building-warrior`, 2304 `-ranger`, 2305 `-rogue`, 2306 `-mage`, 2307 `-smithy`, 2309 `-weavingmill` (kind 15), 2308 `-carpentersshop` (kind 16) |
| 17 | ruins, variant 2 | sphinx | 2564 `sphinx01` |
| 18 | ruins, variant 0 | antique building | 2124 + (d & 3): `antique-building1..4` |
| 19 | ruins, variant 2 | obelisk | 2565 `obelisk` |
| 20 | ruins, centre zone | antique building | 2124 + (d & 1): `antique-building1..2` |

No write in the routine stores kind 4 or kind 8; the blocks for those kinds (§5.5.1 and the
landing place in §5.5.5) are never reached.

### 5.5 Building pass

Notation: plot (i, j) with origin (ox, oy), size s and record fields lo, hi, inner, kind,
sub, face, wc; **cx** = ox + s/2, **cy** = oy + s/2; top(x, y) = bottom + count. Statics are
inserted into zone + 0x0c; "settled" means the settle test of §2.3 with the floor check,
the object being inserted only when it passes. Sprites go to zone + 0x04, creatures to
zone + 0x18, ground items to zone + 0x30. The box filler is §1.10.2 item 1, the prefab
placer §6.2. "Lowest open z" of a column starts at its bottom and steps up while the block
there is solid.

**Plot loop.** i = 0..g − 1 (outer), j = 0..g − 1. Each plot runs the block of its kind; the
blocks follow each other in the order 8, 9, 6, 2, open-plot creatures (§5.5.10, every plot
whose kind is not 2), 3, 4, 10..16, 5, 17, 19, 20, 18. So kinds 9 and 6 build their block
and then run §5.5.10; kinds 3, 5 and 10..20 run §5.5.10 before their prefab; kinds 0 and 7
run only §5.5.10. All kinds are fixed before the loop starts. The plaza pass (§5.5.8) and
the NPC pass (§5.5.9) follow the loop. Houses are collected in the zone's house list
(zone + 0x88) in build order.

#### 5.5.1 Kind 8 (unreachable)

For completeness: T = top(cx, cy) + 1; for n = 4, 8, 12, 16, 20: box(cx − n/2, cy − n/2,
T − (n − 4)/4, (n, n, 1), (220, 220, 220), 0, 0), i.e. a five-step plinth from
(cx − 2, cy − 2, T, (4, 4, 1)) to (cx − 10, cy − 10, T − 4, (20, 20, 1)); then a static of
kind 0 (`goddess2`) at (cx, cy, T), scale (3, 3, 12), raised one block at a time while its
cell is solid, rotation d & 3 from one draw, inserted. 896 draws in all.

#### 5.5.2 Kind 9: market

1. If the column (cx, cy) exists, (cx, cy, top) is added to the **market list** used by
   §5.5.9. No draws.
2. Twenty slots in order. Per slot: a draw; if d % 5 = 0 the slot stays empty. Otherwise a
   draw q = d % 3 for the position, then the object (with its own draws) at z = hi,
   settled and inserted:

   | Slots | Object | Positions (x, y) | Rotation |
   |---|---|---|---|
   | 1, 2, 3 | stand | (cx − 7, oy + 6 + q), (cx, oy + 6 + q), (cx + 7, oy + 6 + q) | 2 |
   | 4, 5, 6 | stand | (cx − 7, oy + s − 6 − q), (cx, …), (cx + 7, …) | 0 |
   | 7, 8, 9 | stand | (ox + s − 6 − q, cy − 7), (…, cy), (…, cy + 7) | 3 |
   | 10, 11, 12 | stand | (ox + 6 + q, cy − 7), (…, cy), (…, cy + 7) | 1 |
   | 13, 14 | goods | (cx − 3, oy + 6 + q), (cx + 3, oy + 6 + q) | 2 |
   | 15, 16 | goods | (cx − 3, oy + s − 6 − q), (cx + 3, …) | 0 |
   | 17, 18 | goods | (ox + s − 6 − q, cy − 3), (…, cy + 3) | 3 |
   | 19, 20 | goods | (ox + 6 + q, cy − 3), (…, cy + 3) | 1 |

3. **Stand**: a draw selects kind 21 + d % 3 (`market-stand1..3`), scale (3.5, 2, 3).
4. **Goods**: a draw k = d & 3: 0 → barrel (kind 24), (2, 2, 2); 1 → crate (25), a draw
   gives f = f32(f32(f32(0.5d)/32767 + 1)·1.5), scale (f, f, f); 2 → open crate (26), a
   draw gives e = f32(f32(0.5d)/32767 + 1), scale (1.5e, 1.5e, 0.75e); 3 → sack (27),
   (1.5, 1.5, 1.4).

Per slot: 1 draw (empty), 3 (stand) or 3–4 (goods).

#### 5.5.3 Kind 6: garden

1. A draw gives o = d & 1 (stripe direction); a draw gives crop = d % 6, which fixes the
   sprite kind P, ground-item subtype I (0 = none), sprite flag F (+0x38) and size S (+0x20):

   | crop | P | I | F | S |
   |---|---|---|---|---|
   | 0 | 23 `pumpkin-leaves` | 16 | 0 | 0.1 |
   | 1 | 25 `sunflower` | 0 | 4 | 0.2 |
   | 2 | 26 `bean-tendril` | 0 | 0 | 0.2 |
   | 3 | per column: a draw; d % 10 = 0 → 21 `cornflower` (S 0.05), else 29 `wheat` (S 0.2) | 0 | 4 | by P |
   | 4 | 30 `corn` | 0 | 4 | 0.2 |
   | 5 | 24 `pineapple-leaves` | 17 | 0 | 0.15 |

   For crop 3 that draw is made at the start of **every** column of the plot (s² draws),
   before the column is examined.
2. For u = 0..s − 1 (outer, x = ox + u), w = 0..s − 1 (y = oy + w): if the column has
   stored blocks and its top stored block has type 4 (grass):
   1. The top block (zt = bottom + count − 1) gets type byte **5** (field) and colour
      0.7·Cl(x, y, zt) + (10, 10, 0)·sel per channel, with sel = (u >> 1) & 1 when o = 0
      and (w >> 1) & 1 when o = 1, Cl the cliff colour of terrain §6.7, f32 arithmetic
      (0.7 as f32), clamped to [0, 255] and truncated. Bit 0x80 of the old type byte is
      kept when set. No draws. [unconfirmed: exact order of the colour operations.]
   2. If u = s/2 and w = s/2: a creature with +0x28 = 6, kind **140**, +0x34 = the local
      level (0x59a9b0) at (x, y), position (x + 0.5, y + 0.5, top); a draw gives facing
      +0x54 = 90·(d & 3).
   3. Otherwise: a draw; if d % 10 = 0: a draw k = d & 3; if k ≠ 0 or I = 0: a sprite of
      kind P with +0x38 = F, +0x24 = 0 for P = 25, else f32(360d/32767) from a draw;
      position (x + 0.5, y + 0.5, top); +0x20 = S. If k = 0 and I ≠ 0: a ground item of
      category 11, subtype I, at (x + 0.5, y + 0.5, top), rotation +0x130 = f32(360d/32767)
      from a draw, +0x134 = 0.1, +0x138 = 2.
3. **Fences**, two runs. Run 1: a draw; if odd, for k = 0..s/2 − 1: a draw; if d % 10 ≠ 0
   and the column (ox + 2k + 1, oy) has stored blocks whose top type is not 0: a draw
   selects kind 52 + (d & 3) (`fence01..04`) at (ox + 2k + 1, oy + 0.5, top), rotation 0,
   scale (2, 1, 1.5), inserted without settling. Run 2 likewise over the columns
   (ox, oy + 2k + 1), at (ox, oy + 2k + 1.5, top), rotation 1.

Draws: 2 + [s² for crop 3] + per grass column 1 to 3 (the centre column 1) + per active
fence run 1 + (s/2)·(1 or 2) (inactive run: 1).

#### 5.5.4 Kind 2: modular houses

A house is a 3 × 3 × 4 grid of module cells; each cell becomes prefab placements (§6.2) of
13 × 13 × 7-block modules.

**Placement shorthand.** "Place M (r, T, mode, F)" runs the placer with model M, rotation r,
type byte T, that mode, the zone, fill-down F, crop (0, 0, 0, 0) unless stated and no output
list. "At the cell centre" means X0 = O.x + 7 + 13a − ⌊sx/2⌋, Y0 = O.y + 7 + 13b − ⌊sy/2⌋,
with sx, sy the unrotated sizes of the model being placed and (a, b) the cell's view
coordinates.

1. **Module set** (no draws), by Lz.type and Lz.variant:

   | Set | stairs S | base B | floor F | floor-stairs FS | wall W | window WW | door WD | indoor WI | balcony WB | lamp WL | roofs R1..R3 | arc A |
   |---|---|---|---|---|---|---|---|---|---|---|---|---|
   | type 1, variant 0 or > 5 `framework` | 2185 | 2189 | 2190 | 2191 | 2192 | 2193 | 2194 | 2195 | 2196 | 2197 | 2198..2200 | 2201 |
   | type 1, 1 `wood` | 2185 | 2287 | 2288 | 2289 | 2290 | 2291 | 2292 | 2293 | 2294 | 2295 | 2296..2298 | 2299 |
   | type 1, 2 `whitewood` | 2185 | 2215 | 2216 | 2217 | 2218 | 2219 | 2220 | 2221 | 2222 | 2223 | 2224..2226 | 2227 |
   | type 1, 3 `stone` | 2185 | 2202 | 2203 | 2204 | 2205 | 2206 | 2207 | 2208 | 2209 | 2210 | 2211..2213 | 2214 |
   | type 1, 4 `clay` | 2241 | 2228 | 2229 | 2230 | 2231 | 2232 | 2233 | 2234 | 2235 | 2236 | 2237..2239 | 2240 |
   | type 1, 5 `whiteclay` | 2242 | 2243 | 2244 | 2245 | 2246 | 2247 | 2248 | 2249 | 2250 | 2251 | 2252..2254 | 2255 |
   | type 5, variant 2 `desertruin` | 2276 | 2277 | 2278 | 2279 | 2280 | 2280 | 2281 | 2282 | 2281 | 2281 | 2283..2285 | 2286 |
   | type 5, variant 3 `jungleruin` | 2265 | 2266 | 2267 | 2268 | 2269 | 2269 | 2270 | 2271 | 2269 | 2269 | 2272..2274 | 2275 |
   | type 5, other `antiqueruin` | 2256 | 2257 | 2258 | 2259 | 2260 | 2260 | 2260 | 2260 | 2260 | 2260 | 2261..2263 | 2264 |

   Slot names: `<set>-base`, `-floor`, `-floor-stairs`, `-wall`, `-wall-window`,
   `-wall-door`, `-wall-indoor`, `-wall-balcony`, `-wall-lamp`, `-roof1..3`, `-arc`;
   2185 `entrance-stairs`, 2241 `clay-entrance-stairs`, 2242 `whiteclay-entrance-stairs`,
   2256/2265/2276 the ruin sets' `-entrance-stairs`.
2. **Grid** (0x5aeb40): 3 × 3 × 4 cells of 12 bytes: **role** (+0), **r** (+1),
   **entrance** (+2), **stairs** (+3), **height** (+4, 32-bit), **room** (+8). Grid fields:
   **face** (+0x04), **mirror** (+0x08), **origin O** (+0x0c, three 32-bit values), **sub**
   (+0x60, the plot's sub) and point lists **spawn** (+0x24), **entrance** (+0x30),
   **counter** (+0x3c), **seats** (+0x48), **beds** (+0x54). Everything starts at zero
   except the fields +0x18 and +0x1c, which start at −1. Roles: 0 none, 1 storey, 2 ground
   floor, 3 roof, 5 arc.
   Every cell access maps **view** coordinates (a, b, z) to **storage** (x, y, z) (0x5a5b40):
   face 0: (a, b); 1: (2 − b, a); 2: (2 − a, 2 − b); 3: (b, 2 − a); then, if mirror ≠ 0,
   y becomes 2 − y. Accesses outside 0..2 × 0..2 × 0..3 use a dummy cell re-zeroed on each
   such access. The layout (step 3) is written with face = mirror = 0, in storage
   coordinates; later steps loop over view coordinates.
3. **Layout**. Cells are listed as "(x, y, z) = role", with "room k" or "stairs" when set,
   in write order.
   * **sub ∈ {2, 3, 4, 5}** (the Q = 1 shops), no draws: (0,0,0)=2, (0,1,0)=2, (1,1,0)=2,
     (1,0,0)=2, (0,0,1)=1, (0,1,1)=1 room 3, (1,1,1)=1, (1,0,1)=1, (0,0,2)=3, (0,1,2)=3,
     (1,1,2)=3, (1,0,2)=3.
   * **sub = 1** (inn), 1 draw: (0,0,0)=2; (0,0,1)=1 room 2; (0,0,2)=1 stairs; (0,0,3)=3;
     (1,0,0)=2; (1,0,1)=1 room 2; (1,0,2)=1 room 1; (1,0,3)=3; (2,0,0)=2; (2,0,1)=1 room 2;
     (2,0,2)=1 room 1; (2,0,3)=3; (0,1,0)=2; (0,1,1)=1 room 2; (0,1,2)=3; a draw: if odd,
     (2,1,0)=2, (2,1,1)=1 room 2, (2,1,2)=3; then (1,1,0)=2; (1,1,1)=1 room 3; (1,1,2)=3;
     (1,2,0)=2; (1,2,1)=1; (1,2,2)=3.
   * **sub = 0** (ordinary house): a draw k = d % 3.
     * k = 0 (two towers), 4 draws in all: (0,0,0)=2; (0,0,1)=1; (0,0,2)=1 room 1 stairs;
       (0,0,3)=3; (2,0,0)=2; (2,0,1)=1; (2,0,2)=1 stairs; (2,0,3)=3; (0,1,0)=2; (0,1,1)=1;
       a draw: even → (0,1,2)=1, (0,1,3)=3; odd → (0,1,2)=3. A draw: odd → (2,1,0)=2,
       (2,1,1)=1, (2,1,2)=3. A draw: even → (1,0,0)=2, (1,0,1)=1, (1,0,2)=3; odd →
       (1,0,1)=5 (an arc between the towers).
     * k = 1 (L shape), 3 draws: (0,0,0)=2; (0,0,1)=1; (0,0,2)=3; (1,0,0)=2; (1,0,1)=1;
       (1,0,2)=3; (0,1,0)=2; (0,1,1)=1 room 1; (0,1,2)=3; (1,1,0)=2; (1,1,1)=1; a draw:
       odd → (1,1,2)=1 stairs, (1,1,3)=3; even → (1,1,2)=3. A draw: odd → (0,2,0)=2,
       (0,2,1)=1, (0,2,2)=3.
     * k = 2 (bar with wings), 4 draws: a draw: even → (0,0,0)=2; (0,0,1)=1; (0,0,2)=1
       stairs; (0,0,3)=3; (1,0,0)=2; (1,0,1)=1; (1,0,2)=1; (1,0,3)=3; (2,0,0)=2;
       (2,0,1)=1 room 1; (2,0,2)=1; (2,0,3)=3. Odd → (0,0,0)=2; (0,0,1)=1; (0,0,2)=3;
       (1,0,0)=2; (1,0,1)=1 room 1; (1,0,2)=3; (2,0,0)=2; (2,0,1)=1; (2,0,2)=3. Then a
       draw: odd → (0,1,0)=2, (0,1,1)=1, (0,1,2)=3. A draw: odd → (2,1,0)=2, (2,1,1)=1,
       (2,1,2)=3. Then (1,1,0)=2, (1,1,1)=1, (1,1,2)=3.

   Then face = the plot's face; a draw sets mirror = 1 when even(d), else 0.
4. **Storey heights** (no draws). For a = 0..2 (outer), b = 0..2: cell (a, b, 1) is
   **eligible** when its role is 1, its own room is not 3, and the cell (a, b, 2) has
   stairs = 0. Its height = the largest top of the existing columns
   (ox + 7 + 13a + u, oy + 7 + 13b + v), u, v = 0..12 (start 0). **M** = the largest height
   (start 0).
5. **Entrance** (0 or 1 draw): the eligible cells with height M, in (a, b) order, form a
   list; if it is non-empty, a draw selects entry d mod n (unsigned), whose cell gets
   entrance = 1.
6. **Rotations**: for a, b, z (z inner), every cell including empty ones: role 5 →
   r = face & 1 (no draw); otherwise a draw sets r = d & 3. 36 draws, one fewer per arc.
7. **Origin** (no draws) [unconfirmed]: h0 = 0, "first" set; for z = 3 down to 0 (outer),
   a, b: for a cell with entrance = 1, over the 15 × 15 columns
   (ox + 6 + 13a + u, oy + 6 + 13b + v), u outer: for each existing column, v = top − M; if
   first or v > h0, h0 = v and first is cleared. **O** = (ox + 7, oy + 7, M + h0 − 6). The
   grid is appended to the house list.
8. **Road under the house** (type 1, variants other than 4 and 5; no draws): rectangle
   x0 = O.x − 2, y0 = O.y − 2; x1 = max(O.x − 2, the largest O.x + 13a + 13 over cells
   (a, b, 0) with role ≠ 0) + 2; y1 likewise with b.
   *Test pass* (x outer, y inner): for each existing column, z = its top stored block,
   lowered while the block at z is not solid; if that block's type is not 11 or z < O.z,
   the step ends without writing.
   *Write pass* (same order and search): for a type-11 block with colour C: on the border
   (x ∈ {x0, x1} or y ∈ {y0, y1}) C = 0.7·(140 + 40n) in every channel with
   n = N(0.05x + 843, 0.05y + 984) (f32); bytes trunc(C) with type 11 are written at
   z + 1 … O.z + 5.
9. **House mode** Mh = sub + 1 (sub 1..5 → 2..6; otherwise 1). It is passed only to doors
   and entrance stairs (step 10), which carry the sign markers of §6.2.
10. **Bases, arcs and walls**. For a, b, z (z inner), by role:
    * 5: place A (cell r, T 6, mode 1, F 0) at the cell centre, Z0 = O.z + 7z;
    * 2: place B (cell r, T 6, mode 1, **F 1**) at the cell centre, Z0 = O.z + 7z;
    * 1: the sides **−y (rotation 0), +y (2), −x (1), +x (3)** in that order, with N the
      neighbour on that side (same z) and N′ the cell below N:
      * N role 0: if this cell has entrance = 1, or N′'s role is neither 0 nor 3: place WD
        (side rotation, T 6, mode **Mh**, F 0) at the cell centre, Z0 = O.z + 7z; then, if
        entrance = 1, place S (side rotation, T 6, mode Mh, **F 1**) at Z0 = O.z + 7z − 4,
        thirteen blocks out: −y: Y0 = O.y − 6 + 13b − ⌊sy/2⌋; +y: Y0 = O.y + 20 + 13b −
        ⌊sy/2⌋; −x: X0 = O.x − 6 + 13a − ⌊sx/2⌋; +x: X0 = O.x + 20 + 13a − ⌊sx/2⌋ (the
        other coordinate as at the cell centre). Otherwise (N′ empty or roof): z = 1: a
        draw, d % 6 = 0 → WL, else WW; z ≥ 2: a draw, d % 6 = 0 → WB, else WW; z ≤ 0: WW
        without a draw; placed with the side rotation, T 6, mode 1, F 0, at the cell
        centre, Z0 = O.z + 7z;
      * N role 1: nothing;
      * other roles: place W (side rotation, T 6, mode 1, F 0) at the cell centre.
    * roles 0 and 3: nothing.
11. **Roofs** [unconfirmed]. For a, b, z: role 3: crop (e0, e1, e2, e3) = 1 for each of the
    neighbours −x, −y, +x, +y (same z) whose role is not 0, else 0. Two halves, the first
    with the cell's r, the second with (r + 2) & 3. A half uses R1 unless its front
    neighbour (first half: r = 0 → −x, 1 → +y, 2 → +x, 3 → −y; second half the opposite
    side) has role 3, in which case it uses R3 when that neighbour's r & 1 equals r & 1 and
    R2 otherwise. Both: T **14**, mode 1, F 0, that crop, at the cell centre (with the
    half's model sizes), Z0 = O.z + 7z.
12. **Floors and carpets** [unconfirmed: slab z]. For a, b, z:
    * role 3 with role 1 below: place F (cell r, T 6, mode 1, F 0) at the cell centre,
      Z0 = O.z + 7z + 1 − sz (sz = model height);
    * role 1 with stairs = 1: place FS the same way;
    * role 1 with stairs = 0: for type 1, a draw; if even and the cell above has
      stairs = 0: a draw selects carpet slot 2186 + d % 3 (`carpet1..3`), then a draw gives
      its rotation d & 3; placed with T 6, mode 1, F 0 at the cell centre with
      Z0 = O.z + 7z + 1 − sz(carpet). Then, for both types, place F as above.
13. **Indoor walls** [unconfirmed: side order]. For a, b, z: role 1 and room 1: for the
    neighbours −y (0), −x (1), +y (2), +x (3) in that order whose role is 1: place WI (that
    rotation, T 6, mode 1, F 0) at the cell centre, Z0 = O.z + 7z.
14. **Room clearing and points** (no draws) [unconfirmed]. For a, b, z with role 1: for
    u = 0..13 (outer), v = 0..13, w = 5 down to 0: the block at
    (O.x + 13a + u, O.y + 13b + v, O.z + 7z + w) is rewritten with its own colour and type
    byte **0x40** unless it already has bit 0x40. Then, if the cell below has role 2, the
    point (O.x + 7 + 13a, O.y + 7 + 13b, O.z + 7z + 1) is added to the entrance list when
    entrance = 1, to the counter list when room = 3, and always to the spawn list.
15. **Lanterns and furniture** (type 1 only). For a, b, z:
    1. Role 1 and (a + b) odd [unconfirmed: parity]: a sprite of kind **13** (`lantern02`)
       at (O.x + 7 + 13a, O.y + 7 + 13b, O.z + 7z + 5.3), +0x20 = 0.075, +0x24 = 0,
       +0x38 = 1, tint (0.6, 0.5, 0.4). No draws.
    2. Role 1 and the cell above has stairs = 0: furniture with floor level
       Zf = O.z + 7z + 1. "Supported" means inserted only when the block at
       (x, y, Zf − 0.5) is solid [unconfirmed].
       1. **Wall pieces** via helper W (each supported), for each side whose neighbour is
          not a storey, in this order:
          * −x: a draw, d % 3 ≠ 0 → W(O.x + 13a + 1.5, O.y + 13b + 4.5, rotation 1); a
            draw, d % 3 ≠ 0 → W(O.x + 13a + 1.5, O.y + 13b + 9.5, 1);
          * +x: the same with x = O.x + 13a + 12.5 and rotation 3;
          * −y: W(O.x + 13a + 4.5, O.y + 13b + 1.5, rotation 2) without a gate draw;
          * +y: gated W(O.x + 13a + 4.5, O.y + 13b + 12.5, 0), then gated
            W(O.x + 13a + 9.5, O.y + 13b + 12.5, 0).
       2. **By room**:
          * 1, bedroom: four beds, kind **19**, scale (2, 3, 1), each with a rotation draw
            d & 3, at offsets (4, 4), (10, 4), (4, 10), (10, 10) from (O.x + 13a, O.y + 13b);
            each supported;
          * 2, dining room: table, kind **12**, (3, 3, 1), at the cell centre with a
            rotation draw; four stools, kind **16**, (1, 1, 0.5): (O.x + 13a + 4.5, centre
            y) rotation 1, (O.x + 13a + 9.5, centre y) 3, (centre x, O.y + 13b + 4.5) 2,
            (centre x, O.y + 13b + 9.5) 0; all supported; 1 draw;
          * 3, counter room: four counters, kind **31**, (4, 1, 1):
            (O.x + 13a + 3.5, centre y) rotation 3, (O.x + 13a + 10.5, centre y) 1,
            (centre x, O.y + 13b + 3.5) 0, (centre x, O.y + 13b + 10.5) 2; supported; no
            draws;
          * other: a draw; d % 5 = 0 → a table (kind 12, (3, 3, 1)) at the centre with a
            rotation draw, supported.

    **Helper W** (0x5bf7c0)(P, r, room) returns a static at P with rotation r:
    * a draw; d % 50 = 0 → kind **10** (`chest-base02`), scale (1.2, 0.8, 0.8), P shifted
      half a block into the room (r 0: y − ½; 1: x + ½; 2: y + ½; 3: x − ½). 1 draw. The
      chest is empty;
    * else, for room 1: a draw; odd → kind **20** (`bedtable`), (1, 1, 1); even → a draw
      selects kind 32 + d % 3 (`shelf1..3`), (2, 1, 1.57). 2 or 3 draws;
    * else a draw d % 6: 0 → a draw selects shelf 32 + d % 3, (2, 1, 1.57); 1 → kind 18
      (`bench`), (3, 1, 0.4); 2 → kind 16 (`stool`), (1, 1, 0.5), and a draw gives the
      shift o = trunc(f32(d/32767)·65536) (r 0: y − o; 1: x + o; 2: y + o; 3: x − o);
      3 → kind 30 (`desktop`), (3, 1, 1), shift 13107 fx with the same signs; 4 → kind 29
      (`cupboard`), (2.5, 1, 3), shift 19660 fx; 5 → a draw selects kind 56 + d % 9
      (`flower-vase1..9`), (1, 1, 1). 2 or 3 draws.
16. **Seats and beds** (no draws) [unconfirmed]: each static added since the house began
    with kind 16 or 18 is recorded (zx, zy, static index) in the seats list; kind 19 in the
    beds list.
17. **Outdoor goods** (type 1 only). For a, b with (a, b, 0) of role 2: the sides −x, +x,
    −y, +y in that order; for a side whose neighbour at z 0 has role 0, two slots, each: a
    draw; d % 6 = 0 → helper G at the slot, settled and inserted. Slots (z = O.z + 1)
    [unconfirmed: positions]: −x: (O.x + 13a − 1.5, O.y + 13b + 4), then
    (…, O.y + 13b + 10), rotation 3; +x: (O.x + 13a + 15.5, same y values), rotation 1;
    −y: (O.x + 13a + 4, O.y + 13b − 1.5), then (O.x + 13a + 10, …), rotation 0;
    +y: (same x values, O.y + 13b + 15.5), rotation 2.

    **Helper G** (0x5bf5b0)(P, r): a draw d % 7: 0 → kind 24 (`barrel`), (2, 2, 2); 1 →
    kind 25 (`crate`), a draw gives f as for market goods, (f, f, f); 2 → kind 26
    (`open-crate`), a draw gives e as for market goods, (1.5e, 1.5e, 0.75e); 3 → kind 27
    (`sack`), (1.5, 1.5, 1.4); 4 → kind 18 (`bench`), (3, 1, 0.4); 5 → kind 16 (`stool`),
    (1, 1, 0.5); 6 → kind 28 (`shelter`), (3, 3, 2.5). Rotation r. 1 or 2 draws.

**Draw order of one house**: layout (0, 1, 3 or 4) → mirror (1) → entrance (0 or 1) →
rotations (36 minus arcs) → step 10 (1 per open storey side at z ≥ 1 without a door, plus
the marker draws of every placed module in placement order) → roofs (markers) → floors
(type 1: 1 per stair-less storey, plus 2 for a carpet, plus markers) → indoor walls
(markers) → furniture (type 1) → outdoor goods (type 1).

#### 5.5.5 Special buildings (kinds 3, 5, 10..20)

One prefab each, type byte 6, fill-down on, no crop, no output list, the zone,
rotation = face:

| Kind | Model slot | Position (x, y, z) | Mode |
|---|---|---|---|
| 3 tower | 2177 `frame-tower` | (cx − 16, cy − 16, hi − 7) | 7 |
| 5 well | 2178 `building-stone-well` | (cx − sx/2, cy − sy/2, hi), z lowered while the block there is not solid [unconfirmed] | 8 |
| 10 warrior | 2303 | (cx − 16, cy − 16, Hm − 7) | 9 |
| 11 ranger | 2304 | as kind 10 | 10 |
| 12 rogue | 2305 | as kind 10 | 11 |
| 13 mage | 2306 | as kind 10 | 12 |
| 14 smithy | 2307 | as kind 10 | 13 |
| 15 weaving mill | 2309 | as kind 10 | 15 |
| 16 carpenter | 2308 | as kind 10 | 14 |
| 17 sphinx | 2564 | (cx − sx′/2, cy − sy′/2, hi), sx′, sy′ the model sizes swapped for odd face [unconfirmed] | 0 |
| 19 obelisk | 2565 | as kind 17 | 0 |
| 18 antique building | 2124 + (d & 3), one draw first | as kind 17 | 0 |
| 20 antique building | 2124 + (d & 1), one draw first | as kind 17 | 0 |

**Hm** = the largest top (start 0) of the existing columns (cx − 16 + u, cy − 16 + v),
u = 0..sx − 1 (outer), v = 0..sy − 1, with the unrotated model sizes [unconfirmed: loop
bounds]. Kinds 17 and 19 then add a ground item (0x5f77b0: category 25, seed d % 200 from
one draw, level 1) to zone + 0x30. Mode 0 interprets no markers and mode 8 only turns blue
voxels into water, so kinds 5 and 17..20 cost no draws beyond the model draw of 18/20 and
the item draw of 17/19; modes 7 and 9..15 make the marker draws of §6.2.

The kind-4 block (unreachable) would place slot 2183 (`landingplace`) at
(cx − 16, cy − 16, hi − 5) with mode 7 and fill-down, followed by airship-related objects;
it is not specified further.

#### 5.5.6 Rotation and mirroring

Buildings turn by quarter turns only, through the placer's rotation: the plot's face for
single prefabs and per-cell and per-side rotations for house modules. The house grid's face
and mirror act on every cell access, turning and mirroring the layout; the placer itself
never mirrors.

#### 5.5.7 Walls and doors

Town houses take their walls, doors and windows from the wall modules (§5.5.4 step 10) and
the module markers (§6.2 markers 1 and 2). The town builder builds no separate town wall or
gate.

#### 5.5.8 Plaza trees and benches

After the plot loop. For each plot in plot order whose final kind is 0 or 7: Fp = 0; then
four points, x offset p outer and y offset q inner, each from {0, h} with
h = (256 / g) / 2 (25 or 32) and k = s / 4 (12 or 16): X = ox + k + p, Y = oy + k + q. Per
point:

1. w = q(X, Y) at fx positions (§5.1); if w < 0.72 (double comparison) the point is skipped
   (no draws).
2. H = the lowest open z of (X, Y).
3. **Type 1 only**: if the block at (X, Y, H − 1) is not of type 11, the point is skipped.
   Otherwise the **plaza disc** [unconfirmed: second-pass order and surface-colour inputs]:
   * pass 1: x′ = X − 8 … X + 8 (outer), y′ = Y − 8 … Y + 8: with
     r² = (X + 0.5 − x′)² + (Y + 0.5 − y′)² (double), if r² ≤ 64, H = max(H, lowest open z
     of (x′, y′));
   * pass 2, same order: if r² ≤ 64: H′ = the lowest open z of (x′, y′); mo and wm = the
     moisture and warmth of (x′, y′); for z = H′ − 1 … H: if r² < 49, the block becomes the
     terrain surface block (terrain §6.1) evaluated at the centre column (X, Y, H) with
     these mo and wm, with its own type byte (no draw); otherwise a draw gives
     k = f32(f32(f32(0.1d)/32767) + 0.75) and the block becomes (trunc(St(x′, y′, z)·k),
     type 6). So the disc of radius 8 is filled up to H, with a paving ring outside
     radius 7 (one draw per ring block).
4. Fp = 1. **Tree**: m = the warmth of column (X, Y) (the warmth blend when the column is
   missing); tree kind 3 when m > 0.8, else 5. If H > 1: a draw d1 then a draw d2; the tree
   builder (trees contract §4) runs at (X, Y, H + 1) with size s = d2 % 6 + 7,
   t = (d1 & 7) + 13 and that kind, making its own draws.
5. **Type 1 only**: four benches; each: a draw; if d & 7 ≠ 0: a static of kind **18**
   (`bench`), scale (3, 0.4, 0.4), at z = H and (X + 0.5, Y − 7.5) rotation 0,
   (X + 0.5, Y + 9.5) rotation 2, (X − 7.5, Y + 0.5) rotation 3, (X + 9.5, Y + 0.5)
   rotation 1, in this order; settled and inserted.

After the four points, if Fp = 1, (ox + s/2, oy + s/2, lowest open z there) is added to the
**plaza list**.

**St(x, y, z)** (stone colour) [unconfirmed]: v = ((N(0.01y + 98984, 0.3z + 8437) +
N(0.01x, 0.3z))·0.5 + 1)·0.5 (f32); base = 200v + 50; each channel = base + 20·N(o + 0.005x,
o′ + 0.005y) with the world noise offsets of terrain §6.7 (one offset pair per channel);
clamped to [0, 255].

Draws per point: 0 when skipped; otherwise (type 1) one per ring block, then 2 plus the
tree's own when H > 1, then 4 bench draws.

#### 5.5.9 NPCs

After §5.5.8. From the house list two lists are made: **inns** (sub 1) and **shops**
(sub 2..5). Creatures are appended to zone + 0x18; integer points convert exactly to fx.
"AI objects" are helper objects stored at +0x109c, with the creature's own position as its
home point; they make no draws.

**Town (type 1).**

1. **Points of interest** (no draws) [unconfirmed]: the 8 × 8 location records of the
   region (zx >> 6, zy >> 6) whose type is neither 1 nor 10 (type 0 only when a lookup at
   the record's position finds an entry) are added with their positions to a POI list.
2. **Level band** [unconfirmed]: computed from the region record and passed to the vendor
   routines, which do not use it.
3. **Per house**, in list order; a house with an empty spawn list is skipped:
   * **sub 1 (inn)**: only if its counter list is non-empty: a draw selects the position
     counter[d mod n]; +0x28 = 3; a draw gives kind +0x2c = (d & 1) + 2; +0x34 = Lz.level;
     +0x30 = **0x84**; AI objects; appended. 2 draws.
   * **sub 2..5 (shops)**: the same two draws with +0x30 = **0x80, 0x81, 0x82, 0x83**
     respectively, then the vendor stock fills the inventory (+0xf6c): sub 2 → **V1**,
     sub 4 → **V2**, sub 3 and 5 → **V3**.
   * **sub 0**: a draw gives n = (d & 1) + 1 residents. Each:
     1. a draw selects the position spawn[d mod |spawn|]; +0x28 = 3, +0x30 = 0x88; a draw
        gives kind (d & 1) + 2; +0x34 = Lz.level;
     2. if the POI list is non-empty: a draw; if even, +0x60 = (x, y, 0) of POI entry
        c mod |POI| (c a running zone counter, starting at 0, incremented on use) and
        +0x5c = 4;
     3. a draw; d % 10 = 0 → +0x5c = 3;
     4. **day schedule** at +0x10a0: entries (fx point, time in ms); "+t" means the time of
        the previous entry plus t; "min" = × 60000:
        * (own position, 0); a draw gives T = (d % 180 + 420) min;
        * plaza list non-empty: a draw selects (plaza[d mod n], T); a draw adds
          (d % 60 + 60) min;
        * market list non-empty: a draw selects (market[d mod n], T); a draw adds
          (d % 180 + 60) min;
        * shops non-empty: a draw selects a shop; if its entrance list is non-empty: a draw
          selects the point, a draw adds (d % 20) min;
        * plaza: as above;
        * inns non-empty: a draw selects an inn; if its entrance list is non-empty: a draw
          selects the point, a draw adds (d % 180 + 60) min;
        * shops non-empty: shop, point, + (d % 30) min; then again shop, point,
          + (d % 30) min;
        * plaza: as above;
        * if the own house's entrance list is non-empty: one draw whose value is unused;
          then (own position, T) and a draw adds (d % 60 + 60) min;
        * finally (own position, T + 30 min), no draw;
     5. AI objects; appended.

**Vendor stock** (V1, V2, V3): 30 rounds k = 0..29. Each round starts with a level:
L0 = trunc(f(k/30)), L1 = trunc(f((k + 0.99999)/30)) with f(t) = (1/(1 − t) − 1)·20 + 1
(f32); a draw gives L = L0 + d % (L1 − L0 + 1), at least 1; round 0 uses L = 1. Items are
appended to +0xf6c; an item record has category (+0), subtype (+1), seed (+0x04), product
(+0x08), rarity (+0x0c), material (+0x0d), +0x0e and a 16-bit level (+0x10). "Rarity
draws" are three draws: r = 1 if d % 100 = 0; +1 if d % 1000 = 0; +1 if d % 10000 = 0;
r = min(r, 4).

* **V1** (general shop). Round 0 first adds: (11, 12) material 24 level 1; (24, 0)
  material 1 level 1; for i = 1..4: a draw, d % (10i) = 0 → (24, 0) rarity i, material 1,
  level 1; then (23, 0) rarity 0 material 2; (23, 1) material 2; (20, X) material 0
  level 1 with a draw selecting X from (34, 35, 19, 26, 30, 87)[d % 6]. Every round: a draw
  gives m = d % 3 recipes; each: a draw r = d & 1; draws d % 100 = 0 → +1, d % 1000 = 0 →
  +1, d % 10000 = 0 → +1; r = min(r, 4); a cleared record (level 1); a draw: even → an
  armour item, odd → a weapon item, both for (L, r) from the items contract (24 and 16
  draws) [unconfirmed: which parity is armour]; the result becomes the product with
  category 2; appended. Then a draw: odd → item (1, 7) level L.
* **V3** (weapon shop, identifier). After the level draw, eleven candidates of category 3
  with +0x0e = 1 and level L, each ending with its rarity draws and a seed draw (+0x04 = the
  draw value), in this order: subtype from a draw d % 3, then an unused draw (material 1);
  subtype from a draw d % 3 + 15, then an unused draw (material 1); subtypes 13, 3, 5, 4
  (material 1) and 6 (material 2), each after one unused draw; then **five unused draws**;
  subtypes 8, 10, 11 (material 2), each after one unused draw; subtype 12: a draw sets the
  material, **11 if odd, 12 if even**, then one unused draw. 63 draws in all. Then a draw
  gives p = d % 3 + 1 picks; each, while candidates remain, draws an index
  d mod (count) (unsigned), appends that candidate and removes it. Per round:
  1 + 63 + 1 + p draws.
* **V2** (armour shop). After the level draw, candidates of subtype 0, level L,
  +0x0e = 1, each with one unused draw, the three rarity draws and a seed draw (5 draws):
  material 1: categories 7, 4, 5, 6; material 25: 7, 7, 4, 5, 6; material 26: the same
  five; material 27: the same five; then category 8 and category 9, each preceded by a
  draw that sets the material, 11 if odd and 12 if even (6 draws each). 21 candidates,
  107 draws; then the picks as in V3. Per round: 1 + 107 + 1 + p draws.

**Ruins (type 5).**

1. **Kind lists** (no draws): a list K of kinds and a list GL of groups (a | b), by
   Lz.variant: 0: K = (41, 17, 96, 96), GL = ((41, 17 | 59, 37, 38, 40, 39)); 1: K = (9, 10),
   GL = ((9, 10 | 22, 55)); 2: K = (83, 84), GL = ((83, 84 | 53, 60), (81 | —));
   3: K = (76), GL = ((76 | 58, 60), (81 | —)) [unconfirmed: the second b entry 60];
   4: K = (15, 16), GL = ((15, 16 | 96), (X | —)) with X ∈ {94, 17, 97} chosen by the
   location's serial number (+0x20) % 3 [unconfirmed: mapping]; other variants:
   K = (11, 12), GL = ((46 | 19)).
2. **Per plot** (plot order):
   * kind 20: a creature at (cx, cy, z + 1), z = the first open z at or above column
     field +0x14 [unconfirmed]; a draw gives facing +0x54 = f32(f32(d)·360)/32767;
     +0x34 = Lz.level; +0x28 = 1; a draw selects kind K[d mod |K|]; +0x58 = the location's
     sub-level byte (+0x28); +0x7a |= 0x1000 | 0x200; +0x10e8 = 1; AI objects; appended.
     2 draws;
   * other kinds ≠ 2 with wc > 0.2: the four points of §5.5.10; each: a draw; if
     d % 5 = 0 and the column exists: a creature at the point (z as above); a draw for
     facing; +0x34 = Lz.level; +0x28 = 1; +0x7a |= 0x1000; a draw selects the kind from K;
     +0x58 as above; appended (no AI objects).
3. **Per house** with a non-empty spawn list; P = a copy of that list:
   * a draw gives m = d % 3 rings; each, while P is non-empty: a draw selects and removes
     centre P[d mod |P|]; a draw gives n = d % 3 + 2; member t = 0..n − 1:
     a = f32(2tπ/n) (double, then f32); position = centre (fx) plus
     (trunc(f32(2·f32(cos a))·65536), trunc(f32(2·f32(sin a))·65536), 65536)
     [unconfirmed]; facing +0x54 = f32((a/π)·180 + 90); +0x34 = Lz.level; +0x28 = 1;
     +0x7a |= 0x1000; a draw selects the kind from K; +0x58 as above; AI objects; a draw
     gives e = d & 1, and an item (category 1, subtype 1, level = +0x34) is added e times;
     appended;
   * if GL is non-empty: a draw gives m = d & 1 groups; each, while P is non-empty: a draw
     selects and removes a point; a draw selects a group; if its list a is non-empty: a
     creature at point + (0, 0, 1); a draw for facing; +0x34 = Lz.level; +0x28 = 1;
     +0x7a |= 0x1000; a draw selects the kind from a; +0x58; AI objects; if the house list
     is non-empty: a draw selects a house, and if its spawn list is non-empty a draw adds
     one of its points to the creature's route; a draw gives e = d & 1 items as above;
     appended. List b is not used here.

#### 5.5.10 Creatures on open plots

Run inside the plot loop for every plot whose kind is not 2 and whose wc > 0.2 (strict):
for p ∈ {0, s} (outer), q ∈ {0, s}: a draw; if d & 7 = 0: X = ox + s/4 + p/2,
Y = oy + s/4 + q/2 (integer divisions); if the column exists: z = the first open z at or
above column field +0x14 (surface − 8) [unconfirmed]; a creature at (X, Y, z + 1); a draw
gives facing +0x54 = f32(f32(d)·360)/32767; a draw selects kind +0x2c =
(34, 30, 19, 26, 33)[d mod 5]; a draw gives level +0x34 = lo + d % (hi − lo + 1) with
(lo, hi) = (3, 6) for kinds 19, 26, 30 and (2, 4) for kinds 33, 34; +0x28 = 5;
+0x58 = the location's sub-level byte; appended. Draws: 4 per plot plus 3 per creature.

### 5.6 Worked examples

* Plot origins, g = 5, zx = 32800: ox = 8396800 + {0, 51, 102, 153, 204}, size 51; column
  255 of the zone belongs to no plot. g = 4: + {0, 64, 128, 192}, size 64.
* A zone where no column has w > 0.1: type 5 → exactly 16 draws; type 1 → 25 plus one
  garden draw per plot with hi − lo < 16.
* Market in plot (2, 2) with zx = 32800, zy = 32801 (g = 5): ox = 8396902, oy = 8397158,
  cx = 8396927, cy = 8397183. Slot 1 with q = 1 at (8396920, 8397165); slot 7 with q = 1 at
  (8396946, 8397176); slot 19 with q = 0 at (8396908, 8397180).
* With the stream 440, 19053, 23075, 13104, 32363, 3265, 30749, 32678, 9760, 28064, 20429,
  750, 4480, … (seed 123):
  * market from slot 1: 440 % 5 = 0 → empty; slot 2: 19053 % 5 = 3 → placed,
    q = 23075 % 3 = 2, stand kind 21 + 13104 % 3 = 21; slot 3 starts with 32363;
  * ordinary house: k = 440 % 3 = 2; 19053 odd → the short bar; 23075 odd → wing at
    (0, 1); 13104 even → no wing at (2, 1); mirror 32363 odd → 0; 5 draws;
  * inn: 440 even → no (2, 1) column; mirror 19053 odd → 0; 2 draws;
  * rotations from 440: 0, 1, 3, 0, 3, …;
  * helper W, room 0: 440 % 50 = 40, 19053 % 6 = 3 → kind 30, 2 draws; room 1: 19053 odd →
    kind 20, 2 draws; from 23075 (room 0): 23075 % 50 = 25, 13104 % 6 = 0,
    32363 % 3 = 2 → kind 34, 3 draws;
  * helper G: 440 % 7 = 6 → kind 28; from 23075: 23075 % 7 = 3 → kind 27;
  * open-plot animal: 440 & 7 = 0 → creature; facing f32(19053·360)/32767 = 209.3289;
    kind index 23075 % 5 = 0 → 34; level 2 + 13104 % 3 = 2; next point 32363 & 7 = 3 →
    none;
  * resident schedule: first time draw 440 → T = (80 + 420)·60000 = 30,000,000 ms;
  * V1 round 0: level draw 440 (L = 1); gates 19053 % 10 = 3, 23075 % 20 = 15,
    13104 % 30 = 24, 32363 % 40 = 3 → no extra items; pet 3265 % 6 = 1 → 35;
    m = 30749 % 3 = 2; first recipe r = 32678 & 1 = 0, no bonus; 750 even → the even-branch
    generator begins with 4480.
* Cell centre in plot (2, 2) above: O.x = 8396909, O.y = 8397165; a module of 13 × 13 in
  cell (1, 1) has X0 = 8396923, Y0 = 8397179; its −y entrance stairs with sy = 9 have
  Y0 = 8397168.
* Crate scale with d = 16383: f = 1.8749886; with d = 440: f = 1.5100712.
* Vendor levels (L0, L1), k = 0..29: (1,1) (1,2) (2,3) (3,4) (4,4) (5,5) (6,7) (7,8) (8,9)
  (9,10) (11,12) (12,14) (14,16) (16,18) (18,20) (21,23) (23,27) (27,30) (31,35) (35,40)
  (41,47) (47,55) (56,66) (66,80) (81,100) (100,130) (131,180) (180,280) (281,580)
  (580,67108844).
* Ruin ring offsets (fx): n = 3: (131072, 0) facing 90; (−65536, 113511) 210;
  (−65535, −113511) 330. n = 2: (131072, 0) 90; (−131072, 0) 270. n = 4: (131072, 0),
  (0, 131072), (−131072, 0), (0, −131072), facings 90, 180, 270, 360.

---

## 6. Home building and prefab placer

### 6.1 Home building (0x5e8c2b..0x5e8d80)

Runs only when the dword at world + 0xa4 is 0 [unconfirmed: that this means an unnamed
world] and the zone holds the home point: zx = ⌊trunc(homeX)/256⌋, zy likewise, and the
column (trunc(homeX), trunc(homeY)) exists.

Model slot 2123 (`character-platform.cub`, always present) is placed at
(trunc(homeX) − sx/2, trunc(homeY) − sy/2, bottom + count of the home column) with sx, sy
its x and y sizes (integer halves), rotation 0, type byte 6, **mode 0**, the zone,
fill-down on, no crop and no output list. No draws.

### 6.2 The prefab placer (0x5f0ce0)

**Inputs**: model, origin (X0, Y0, Z0) in blocks, rotation r, type byte T, mode M, zone,
fill-down F, crop (e0, e1, e2, e3), output list. r is used modulo 4 (callers pass 0..3).
The model has sizes sx, sy, sz and one colour per voxel (index (k·sy + j)·sx + i); colour
(0, 0, 0) means **empty**, and voxels outside the model read empty.

**Zone clip**: with (wx, wy) = (sx, sy), swapped for odd r, the placer does nothing (no
writes, no draws) when X0 + wx ≤ 256zx, Y0 + wy ≤ 256zy, X0 ≥ 256zx + 256 or
Y0 ≥ 256zy + 256.

**Warmth**: wm = the warmth of column (X0, Y0), or the warmth blend there when that column
is missing; read once.

**Rotation**: model voxel (i, j, k) lands at Z = Z0 + k and

| r | X | Y |
|---|---|---|
| 0 | X0 + i | Y0 + j |
| 1 | X0 + j | Y0 + sx − 1 − i |
| 2 | X0 + sx − 1 − i | Y0 + sy − 1 − j |
| 3 | X0 + sy − 1 − j | Y0 + i |

There is no mirroring.

**Crop**: entries in world directions (e0 = −x, e1 = −y, e2 = +x, e3 = +y) remove whole
model columns: i runs over [i0, sx − i1), j over [j0, sy − j1), with (i0, i1, j0, j1) =
(e0, e2, e1, e3) for r = 0, (e3, e1, e0, e2) for r = 1, (e2, e0, e3, e1) for r = 2,
(e1, e3, e2, e0) for r = 3.

**Order**: i ascending (outer), j ascending, k from sz − 1 down to 0 (inner). Draws occur
only at marker voxels, in this order.

**Per column (i, j)**:

1. zlo = the lowest and zhi = the highest k with a non-empty voxel (sz and 0 for an empty
   column); "inside" means zlo < k < zhi.
2. **Fill-down** (F set): if voxel (i, j, 0) is non-empty and the target column exists: for
   z = Z0 − 1, Z0 − 2, … while z > the column bottom: stop at the first solid block (type
   not 0 or 2); otherwise write the colour of voxel (i, j, 0) with type byte **6**. These
   writes are not added to the output list.
3. For each k (from the top), with C the voxel colour and E the block now at the target:
   * C empty: if inside, write (255, 255, 255) with type byte **0xC0**; otherwise nothing;
   * C non-empty and E has bit 0x40 and is solid: skipped;
   * C non-empty and M ∉ {1..7, 9..15}: the default write;
   * otherwise the marker table below, matched by exact colour; an unmatched colour gets
     the default write.

**Default write**: in mode 8 a voxel of colour (0, 0, 255) writes (255, 255, 255) with type
byte **2** (water); otherwise C is written with type byte T | 0x40. Every placer block
write other than fill-down also adds (position, block) to the output list when one is
given; this does not affect draws.

**Blobs** (markers 1, 2, 10): a voxel of colour C is a blob **origin** only when none of
(i − 1, j, k), (i, j − 1, k), (i, j, k − 1) has colour C. From the origin the blob extends
along +x, +y and +z while the colour stays C, giving the model box [i, i′] × [j, j′] ×
[k, k′]. Both corners are rotated with the table above but without the origin, sorted per
axis, and the maxima get + 1. The static then has x = (min.x + max.x)·32768 + X0·65536, y
likewise, z = (min.z + Z0)·65536; rotation index 0 when the x extent exceeds the y extent,
else 1; scale (long side, short side, height); byte +0x30 = 1. Non-origin voxels make no
object. [unconfirmed: details of the corner rotation and run directions.]

**Marker table.** P = the target block (X, Y, Z) in fx; "+(a, b, c)" adds a, b, c blocks.
"Air" = write (0, 0, 0) with type byte **0x40**; "air if inside" = only when zlo < k < zhi;
"untouched" = nothing written.

| # | Colour | Effect | Draws |
|---|---|---|---|
| 1 | (255, 0, 0) | Door: at a blob origin, a static of kind **1** (`door`), or **2** (`big-door`) in mode 7, appended directly to the zone's static vector. Always air. | 0 |
| 2 | (0, 255, 0) | Window: at a blob origin, a static of kind **3** (`window`), inserted. Always air. | 0 |
| 3 | (0, 127, 127) | Wall plant: if wm ≤ 0.5, or the block now at the target is solid: untouched. Otherwise a draw; if d % 5 = 0, a draw selects sprite kind **41 + d % 3** (`ivy`, `wall-roses-red`, `wall-roses-white`) at P + (32964, 32964, 196) fx, +0x20 = 0.15, +0x24 = −90r, scale fields 1, +0x38 = 0, added to zone + 0x04. Then air. | 1 or 2 |
| 4 | (255, 127, 0) | Bed: static **19** (`bed`) at P + (0.5, 0.5, 0), rotation 0, scale (2, 3, 1); static **20** (`bedtable`) at P + (2.25, 0.5, 0), rotation 0, scale (1, 1, 1); both inserted. Air if inside. | 0 |
| 5 | (255, 127, 127) | Shelf: a draw selects static **32 + d % 3** (`shelf1..3`) at P + (0.5, 0.5, 0), rotation r, scale (2, 3, 1). Air if inside. | 1 |
| 6 | (127, 127, 0) | Table set: static **12** (`table`) at P + (0.5, 0.5, 0) with rotation d & 3 from a draw, scale (3, 3, 1); static **16** (`stool`) at x + 176947 fx (2.7 blocks), y + 0.5, with rotation d & 3 from a draw, scale (1, 1, 0.5); the same stool again at x − 111411 fx (−1.7 blocks) with that rotation; then one more draw whose value is unused. Air if inside. | 3 |
| 7 | (255, 255, 0) | Lantern: sprite **13** (`lantern02`) at P + (0.5, 0.5, 0), +0x20 = 0.1, +0x24 = 0, tint (0.6, 0.5, 0.4), +0x38 = 1. Air if inside. | 0 |
| 8 | (63, 0, 0) | Crest: sprite **48** (`crest1`) at P + (0.5, 0.5, 0.5), +0x20 = 0.065, +0x24 = −90r, scale fields 1, +0x38 = 0. Air if inside. | 0 |
| 9 | (0, 0, 255) | Sign by mode: 2 → **33** `inn-sign`, 3 → 34 `shop-sign`, 4 → 35 `weapon-shop-sign`, 5 → 36 `armor-shop-sign`, 6 → 37 `identifier-sign`, 13 → 38 `smithy-sign`, 14 → 39 `carpentersshop-sign`, 15 → 40 `weavingmill-sign`; other modes no sign. Sprite at P + (0.5, 0.5, −0.5), +0x20 = 0.1, +0x24 = (−1 − r)·90, scale fields 1, +0x38 = 0. Air if inside. | 0 |
| 10 | (255, 0, 255) | Flower box: if wm ≤ 0.5, untouched. Otherwise at a blob origin a draw selects static **47 + d % 3** (`flower-box01..03`) from the blob, inserted. Air if inside. | 0 or 1 |
| 11 | (0, 255, 255) | Mode 13: static **71** `furnace`, (3, 3, 6); mode 14: **75** `sawbench`, (1.5, 1.5, 1); mode 15: **73** `spinningwheel`, (1.5, 1.5, 1); each at P + (0.5, 0.5, 0) with rotation index −r stored as is; air if inside. Other modes: a creature, +0x28 = 6, kind **141** in mode 10 else **142**, at P + (0.5, 0.5, 0.5), facing +0x54 = (2 − r)·90, +0x34 = the local level at (X, Y), appended to zone + 0x18; always air. | 0 |
| 12 | (0, 0, 127) | Mode 13: static **77** `customization-bench`, (1.8, 1.8, 1.2), at P + (0.5, 0.5, 0), rotation −r; always air. Other modes: untouched. | 0 |
| 13 | (0, 255, 127) | Mode 13: static **72** `anvil`, (1, 1, 1); mode 14: **76** `workbench`, (1.8, 1.8, 1.2); mode 15: **74** `loom`, (1.5, 1.5, 3); at P + (0.5, 0.5, 0), rotation −r; air if inside. Other modes: untouched. | 0 |
| 14 | (0, 127, 0) | Vase: a draw selects static **56 + d % 9** (`flower-vase1..9`) at P + (0.5, 0.5, 0); a draw gives rotation d & 3; scale 0.5 on all axes below kind 60, else 1; inserted. Air if inside. | 2 |
| 15 | (127, 0, 127) | NPC: a creature with +0x28 = 3; a draw gives kind +0x2c = d & 3; facing +0x54 = (2 − r)·90; at P + (0.5, 0.5, 0.5); item slot +0x350 = (category 4, subtype 0) with material +0x35d = 6; +0x34 = the local level; then the mode table below; AI objects (no draws); appended to zone + 0x18. Air if inside. | 1 + mode table |

**NPC marker by mode.** Item slots inside the creature: +0x8c8 (category 3), +0x698 (7),
+0x350 (4), +0x468 (6), +0x580 (5); in each slot byte +0 = category, +1 = subtype,
+0x0c = modifier (d % 5), +0x0d = material, 16-bit +0x10 = level. Draws in order:

| M | Byte +0x30 | Fields and draws |
|---|---|---|
| 1..5 | unchanged | none |
| 6 | 0x83 | none |
| 7 | 0x89 | +0x34 = 20; a draw gives kind (d & 1) + 2 |
| 9 | 1 | +0x7a = 0x40, +0x34 = 20, kind 2; slot +0x8c8 = (3, subtype d % 3 + 15 from a draw), then one modifier draw each for slots +0x8c8, +0x698, +0x350, +0x468, +0x580; level 20; all materials 1. 6 draws |
| 10 | 2 | as mode 9 but subtype (d & 1) + 6 from a draw, slot +0x8c8 material 2, other materials 26. 6 draws |
| 11 | 4 | as mode 9 but slot +0x8c8 = (3, 5) without a subtype draw, material 1, other materials 27. 5 draws |
| 12 | 3 | as mode 9 but slot +0x8c8 = (3, 10) without a subtype draw, material 2, other materials 25. 5 draws |
| 13, 14, 15 | 0x85, 0x86, 0x87 | none |

**Modes used**: 0 (dungeon arches, sphinx, obelisk, antique buildings, home platform; no
markers), 1 (house modules), 2..6 (house mode Mh, only on the door module and entrance
stairs of the inn and the four Q = 1 shops), 7 (tower), 8 (well), 9..15 (class and craft
houses).

**Draw count of one placement** = the sum, over marker voxels in placement order, of the
"Draws" column (rows 3, 5, 6, 10, 14, 15). It depends only on the model's marker voxels, on
wm (rows 3 and 10), on the target block of each row-3 voxel and on structure blocks
already present (the skip rule).

Offsets of +0.5 are 32768 fx; P + (0.5, 0.5, −0.5) uses double halves × 65536; creature
offsets use the float triple (0.5, 0.5, 0.5) × 65536.

---

## 7. Draw order of the whole range

For one zone, after the ravine boulders, each line skipped when its condition fails:

| § | Pass | Condition | Draws |
|---|---|---|---|
| 6.1 | home building | world + 0xa4 = 0, zone holds the home point | 0 |
| 2 | camp attempts | (zx + zy) odd | 11 to 38 when a camp is built (each failed try 2 or 3, the successful one 11); 20 to 30 when none is |
| 3 | boulder pass 3 | E.kind ∉ {1, 3, 4} | 1 + 5n, n = 0..19 |
| 5 | town builder | Lz type 1 or 5 | g² + … (§5.2–§5.5, plus the marker draws of placed models) |
| 4 | type-10 site | E.kind = 4 | 14, 16 or 18 |
| — | voxel-tree grid | always | vegetation contract §5 |

The dungeon builder (§1) runs earlier, at caves contract §6.1. `Server.exe`'s zone
generator (0x518630) holds the same range from the home building to the type-10 site,
with the same draw sites and identical constants.

---

## 8. Unresolved

1. Item generators 0x5f7a60 and 0x5f6d50: their draws and argument lists belong to the
   items contract; this document does not count them.
2. Whether entries with E.kind = 5 (dungeon flag = 1) ever occur in fresh worlds, and the
   value range of the dungeon style v (taken from the entry byte).
3. Static kind 14 (the t = 5 table of §1.10.2 item 4) and kind 6 have no entry in the
   static kind → model map; the visible result of a kind-14 table is unknown.
4. Dungeon: the −x and +x window rows beyond the parts noted in §1.10.6; the exact
   one-block offset of the torch wall test; the patrol waypoint order for route values 1
   and 2; the midpoint rule of the centre ring; the defaults of the sprite and static
   initialisers; the sizes of the exit and boss marker records; which of the blends
   0x5c8350 and 0x5c8310 is moisture and which warmth in the pyramid grass skin.
5. Which of the blends 0x5c4dd0 and 0x5c4800 is warmth and which moisture (§3 climate
   bonus; A = 0x5c4dd0 and B = 0x5c4800 as coded, names not established).
6. Helper routines not re-read: location distance (0x5f8e10), roughness R (0x5f9340), q
   (0x5989d0), rock colour Rk (0x5f9620), noise N, local level (0x59a9b0), object-list node
   creation (0x58d860); the ground-item record size and helpers.
7. Boulder ground scan (§3 step 2.6) and the site's T = top(cx, cy) (§4 step 1) read the
   column without a missing-column check; behaviour for a missing column is undefined.
8. Blob extent details (§6.2): corner rotation and run directions.
9. Town items tagged [unconfirmed] in §5: the wc sort stability; house steps 7, 11, 12
   (slab z), 13 (side order), 14, 15 (lantern parity, support test), 16 and 17 (slot
   positions); well z lowering, Hm bounds and the kind 17/19/18/20 positions; the plaza
   disc second pass and St(); POI filter and level band; ruin variant 3 list b, the
   variant-4 serial mapping, ruin ring and z rules; the V1 recipe parity; the garden colour
   operation order; the kind-4 airship block.
10. Numeric test values that require evaluating the noise function (moss, boulder k,
    site pillar noise) are not included.
11. Server: only the zone-generator range was compared; the server's own camp routine,
    settle test and placer were not compared.
12. That world + 0xa4 = 0 means an unnamed world.

---

## Re-certification (2026-11-15)

Basis: the original `Cube.exe` (SHA-256 84a7a132…30bf) assembly listing, constants read
from its data sections, its symbol table and model-registry strings, and a comparison with
`Server.exe`. No reconstruction leads were consulted. The prior text was rewritten as
independent prose; history notes, verification procedures, implementer advice and the
former open-question and checking sections were removed. Unconfirmed statements are tagged
in place and listed in §8.

### Corrections

1. **Corridor line stair cells (§1.6)** — reverses an earlier claim; changes generation
   results and later draw counts. Old: when climbing, the cell above is opened and the
   current cell flagged; when descending, the cell below is opened and flagged. New: when
   climbing, the cell below is opened and flagged; when descending, the cell above is
   opened and the current cell flagged (flag always on the lower level). Evidence
   0x5ef862..0x5ef8b4.
2. **Corridor line direction (§1.6)** — changes generation results (carved cells), not
   draw counts. Old: for PB.y < PA.y the line ran from B's side (m, PB.y + SB.y) to
   (m, PA.y). New: every line runs from A's side to B's side, e.g. (m, PA.y, PA.z) →
   (m, PB.y + SB.y, PB.z), also for the extra lines and the exchanged-axis case. Evidence
   argument order at 0x5c532a, 0x5c5374, 0x5c5406, 0x5c56bd.
3. **v = 5 central shaft (§1.10.7)** — reverses earlier claims; changes geometry and draw
   counts. Old: corridor carves at Zt, central carve height Zt − Ze + 6 (36), pillar
   n = Zt − Ze + 30 (60). New: after the shell, Zc = Zt − 12; corridor carves at Zc, central
   carve height Zc − Ze + 6 (24), pillar n = Zc − Ze + 30 (48). Evidence 0x5d42d7..0x5d453b.
4. **Carve roughness noise (§1.10.2 item 2)** — changes generation results, no draws. Old:
   all four faces use N(0.1Y′ + 13X′, 0.2Z′) (inferred). New: the y faces use
   N(0.1X′ + 13Y′, 0.2Z′). Evidence 0x59acc0, 0x59ade3.
5. **Dungeon floor box (§1.10.6 step 7.1, §1.11)** — draw-count statement. Old: 303 draws.
   New: 103 (box 10 × 10 × 1). Evidence 0x5d17cb..0x5d1851.
6. **Spawner inventory range (§1.10.2 item 7)** — draw-count statement. Old: 2–5 per record,
   20–50 per inventory. New: 3–5 and 30–50. Evidence 0x5f8750.
7. **Omitted creature fields (§1.10.6 step 9, §1.10.9)** — changes written fields, no draws:
   ring creatures get a facing f32((θ/π)·180 + 90), +0x08 = 150.0 and +0xf60 × 0.75
   (0x5d5878..0x5d59ec); escorts get +0xf60 × 0.5, +0x08 = 150.0 and the leader's route
   pair (0x5d55f5..0x5d561a); the boss gets a helper object at +0x109c.
8. **Kind-8 plinth (§5.5.1, §5.6)** — unreachable code; no effect on generated worlds. Old:
   20 steps n = 4..23, corner −2 per step, 4,371 draws. New: 5 steps n = 4, 8, 12, 16, 20,
   centred, from (cx − 2, cy − 2, T, 4 × 4) to (cx − 10, cy − 10, T − 4, 20 × 20), 896 draws.
   Evidence 0x5b03f0..0x5b046f.
9. **Kind 18 model (§5.4 table, §5.5.5)** — changes generation results (model chosen), not
   draws. Old: kinds 18 and 20 use 2124 + (d & 1). New: kind 18 uses 2124 + (d & 3)
   (`antique-building1..4`); kind 20 keeps 2124 + (d & 1). Evidence 0x5bb8c8..0x5bb8da,
   0x5bb6d9..0x5bb6eb.
10. **Vendor material (§5.5.9, V3 subtype 12 and V2 categories 8, 9)** — reverses an earlier
    claim; changes generation results, not draw order. Old: material 12 if odd, 11 if even.
    New: 11 if odd, 12 if even. Evidence 0x5cabb8..0x5cabcc, 0x5c97e1..0x5c97f5,
    0x5c9904..0x5c9918.
11. **Ruin centre-zone test (§5.4)** — wording. Old: blk(L.x) >> 8. New: the fx coordinate
    divided by 65536 and then by 256, both towards zero (identical for non-negative
    values). Evidence 0x5aff4b..0x5aff94.
12. **Storey eligibility (§5.5.4 step 4)** — clarification: the room ≠ 3 test applies to
    cell (a, b, 1) itself, the stairs test to (a, b, 2). Evidence 0x5b38d0..0x5b392d.
13. **House grid initial state (§5.5.4 step 2)** — precision: fields +0x18 and +0x1c start
    at −1, and the out-of-range dummy cell is re-zeroed on each access. Evidence 0x5aeb40,
    0x598930.
14. **Floor-division wording (§0, §2.3, §4)** — precision: the cell-position read subtracts
    65536 from negative values and then truncates; equal to floor division for all world
    coordinates. Evidence 0x42f860.
15. **Settle-test read rules (§2.3)** — added precision: missing column or below bottom
    reads solid type 1; above the top reads air for z > 0 and water for z ≤ 0; stored
    type-0 cells at z ≤ 0 without bit 0x40 read as water. Evidence 0x5f4f03..0x5f4f4d and
    the block constants set up by 0x6fae30, 0x6fae60, 0x6fae90.
16. **Home model presence (§6.1)** — wording. Old: "none if the list is shorter". New: slot
    2123 is always present (the registry fills slots up to 2568); the caller reads the
    model sizes without a presence test. Evidence 0x5e8cb8..0x5e8d36.
17. **Camp ring records (§2.2)** — wording: the ring objects are appended directly to the
    static vector; only the campfire goes through the insert routine. No behavioural
    difference.
18. **Removed material**: the claim that lists longer than 32 entries would switch to an
    unstable sort (not re-read and irrelevant for at most 25 plots), the run-time checking
    section, open questions that were procedural advice, and the numeric test values that
    depend on evaluating the noise function (not confirmable from the image alone).

None of the corrections changes the order of random draws. Corrections 1 and 3 change the
number of draws made; 5 and 6 correct stated counts without changing behaviour; 1–4, 7, 9
and 10 change generation results (8 concerns unreachable code).

### Confirmed

* §1: kind tables and the v = 1/2 extra draw; the 162 room draws and their field order;
  every presence and link table with its draw count; room carving, pit, mark and both view
  transforms; corridor overlap tests and the two draws; entrance cell formula; rot/mir,
  c1–c5 channel order, e, glow rates 0.005/0.0015/0.0025 and triples; arch slots 2544/2545;
  raised floors; the fit and base adjustment; box filler table, 3 + volume count, moss and
  colour constants; furniture, torch, rarity and inventory branches; all parapets including
  +y; the v = 3 roof; shell, foundation, pyramid, mound, windows (partly), stairs (693),
  trap, lianas, cobwebs, wall decorations and guard fields, chest and table loot, arch
  rotations 0, 2, 3, 1, chandelier, boss; the v = 4 shaft and Ze = base + 10ce + 8;
  identical spiral tables for v = 4 and 5; erosion; room spawn draw order; the arithmetic
  examples of §1.12.
* §2–§4: camp y-before-x draws, 0/1/9 camp draws, fire search, ring offsets 229376, ring
  kinds and scales; settle test; boulder pass (climate constants 0.6, 0.3, 0.7, 0.4, 0.2,
  0.8; 400; 1600; 0.25; 0.05; 0.8; type 0x26; 1 + 5n); type-10 site (π, radius 25, 0.05,
  0.3, 10, type 6, runestone 229376 and scale (4, 4, 5), 1 + 2n + 1); worked examples.
* §5: plot sizes and survey constants (0.1, 0.65, 0.25, 32767), per-plot draw order,
  facing, gardens, list B and every Q and ruin-variant gate; the absence of kind 4/8
  writes; plot-loop block order; market; garden crops, creature 140 and fences; all house
  layouts and their draws, mirror, entrance, rotations, road paving, wall window/lamp/
  balcony draws, carpets, lantern z + 5.3, wall-piece gates, helpers W and G; model slots
  and class-house cases and modes; the 0x5f77b0 item; plaza threshold 0.72, radii 64/49,
  ring colour factor, tree kind and d1/d2 order, benches; inns, shops and resident
  schedules including the unused draw; vendor level curve and table; V1, V2 (107) and V3
  (63) draw sequences; ruin kind lists for variants 0, 1, 2 and default; open-plot animals;
  the arithmetic examples of §5.6.
* §6: home-building conditions and arguments; placer argument order, clip, warmth,
  rotation and crop tables, loop order, fill-down, air, skip and default rules; every
  marker row's colour, kinds, scales, offsets and draws; the NPC mode table; blob result.
* §7: pass order; the server's zone-generator range matches instruction for instruction
  with identical constants.

### Unresolved

As listed in §8, items 1–12.
