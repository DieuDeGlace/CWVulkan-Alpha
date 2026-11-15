Status: FINAL / STABLE / HELD

# Alpha open points, batch 2: path-search statics, object collision, the compare panel

Behavioural specification of three points left open by companion specs:

1. **PS-2** of `alpha-path-search.md` (§4.3, §4.4): how the path search classifies the
   static objects around a node ("on static" and the four side-blocking flags);
2. **Q-6** of `alpha-physics-camera.md` (§4.5): the object tests of the physics step and the
   "voxel objects within 64 blocks" test;
3. **I-3** of `alpha-ui-screens.md` (§8.1.7, §8.1.12): when the `equippedpreview` comparison
   panel is visible.

Written from the original Alpha images and their per-instruction listings. Addresses are
RVAs (image base 0x400000 for both images) and name the image: "server RVA 0x…" is
`Server.exe`, "client RVA 0x…" is `Cube.exe`. *Believed* marks partial evidence; *as read*
marks behaviour that looks unintended but is what the original does. Companion spec names
are pointers only; this document does not restate them.

Conventions: positions are 3 × signed 64-bit in 1/65536 block. A creature's box
**s** = (w, d, h) is the three floats at creature +0x80/+0x84/+0x88 and is centred on its
position (creature +0x10). A static's record has kind at +0x00 (32-bit), position at
+0x08/+0x10/+0x18 (x and y at the footprint centre, z at the base), rotation index at +0x20,
scale floats at +0x24/+0x28/+0x2c and a state byte at +0x30.

**Static box** (used by every test here): half-widths hx = sx/2 and hy = sy/2, where sx, sy
are the scale x and y, **exchanged when the rotation index is odd**; height H = scale z. The
box is x ∈ [X − hx, X + hx], y ∈ [Y − hy, Y + hy], z ∈ [Z, Z + H]. Every float-to-fixed
conversion multiplies by 65536 and truncates toward zero.

**Static kind names** (the static-kind name table built in client RVA 0x18eb00): 0 Statue,
1 Door, 2 BigDoor, 3 Window, 4 CastleWindow, 5 Gate, 6 FireTrap, 7 SpikeTrap, 8 StompTrap,
9 Lever, 10 Chest, 12–14 Table, 15–17 Stool, 18 Bench, 20 BedTable, 21–23 MarketStand1–3,
52–55 Fence1–4; named kinds run up to 77 (CustomizationBench).

---

## 1. PS-2: statics in the path search (server RVA 0x0dde90; client twin RVA 0x1aaab0)

The statics pass of one search round runs from server RVA 0x0de968 to 0x0df77b; the client
twin has the same structure, constants and gate (client 3.0 constant at RVA 0x2fcd70).

### 1.1 Node box, cell range and which statics are looked at

The pass runs after the cell flags of the node have been computed (`alpha-path-search.md`
§4.3) and before neighbour moves are tried. For node cell (x, y, z) the node box N is the
creature's **real box** (w, d, h) centred on (x + 0.5, y + 0.5, z + h/2) (offset built at
server RVA 0x0ddfd7–0x0de063 from the constants 32768 and h · 0.5 · 65536). Its faces:
nx₋ = x + 0.5 − w/2, nx₊ = x + 0.5 + w/2, likewise ny₋, ny₊; nz₋ = z (the feet),
nz₊ = z + h.

The cell range is the block cell of N's lower corner raised by 0.01 in z (x₀, y₀, z₀) and the
block cell of its upper corner (x₁, y₁, z₁). A block cell is the fixed-point value divided by
65536 rounding toward zero, minus one when the value is negative (server RVA 0x005450). The
cell pass examines x₀ − 1 … x₁ + 1, y₀ − 1 … y₁ + 1,
z₀ − 1 … z₁ + 1.

The statics looked at are those listed in the zone **index cells** (8 × 8 blocks) covering
index-x from (x₀ − 1) div 8 to (x₁ + 1) div 8 and index-y likewise, where div is integer
division **rounding toward zero**; an index cell with a negative coordinate or a coordinate
≥ 0x200000 is skipped. Index cells are walked x outer, y inner. For a 0.96-wide box this is one
index cell per axis, or two near an index-cell border, not a 3 × 3 block. *As read*: a static
listed only in an index cell outside that range is not seen, even if its box reaches the node.

Every listed static S is processed as follows, in list order. "On static" and the four side
flags start false for each node and are only ever set, never cleared.

### 1.2 Step 1: ignored statics

If S's record address is in the creature's **ignored-statics set** at creature +0x1468
(server RVA 0x0dea3f; how the set is filled: `alpha-path-search.md` §4.1), S is skipped
entirely: it neither supports nor blocks in the search. The physics also skips it (§2.2).

### 1.3 Step 2: "on static" (any kind, any state)

Set **on static** when S overlaps N **lowered by one block**:

| Axis | Test |
|---|---|
| x | X − hx ≤ nx₊ **and** nx₋ < X + hx |
| y | Y − hy ≤ ny₊ **and** ny₋ < Y + hy |
| z | Z ≤ nz₊ − 1 **and** nz₋ − 1 < Z + H |

There is **no** kind or state test here: doors (open or closed), traps and levers all count.
In words: the creature's feet are within one block above S's top without reaching it, or the
creature is partly inside S.

### 1.4 Step 3: the side-blocking gate (server RVA 0x0ded91)

S can block sideways only when **at least one** of these holds; otherwise the next static is
taken:

* H ≥ **3.0** blocks (server constant at RVA 0x158718, client at RVA 0x2fcd70);
* a solid cell at height z₁ + 1 with x₀ − 1 < x < x₁ + 1 and y₀ − 1 < y < y₁ + 1 (the
  ceiling over the body);
* a solid cell at height z₁ + 1 in the face column x = x₀ − 1 (with y₀ − 1 < y < y₁ + 1);
* the same in the face column x = x₁ + 1;
* the same in the face row y = y₀ − 1 (with x₀ − 1 < x < x₁ + 1);
* *as read*, a solid cell in the face row y = y₁ + 1 (with x₀ − 1 < x < x₁ + 1) at the
  **floor** height z₀ − 1, not the head height. This is the same cell test as the cell pass's
  +y floor-side flag, which serves as support for the +y move.

"Solid" is the cell classification of the cell pass (`alpha-path-search.md` §4.3).

**Consequence**: because of the last item, the gate is open whenever the floor-level cell on
the node's +y side is solid, which is the normal case on ordinary ground. The gate stays
closed only for a static lower than 3 blocks when that +y floor cell is not solid and none of
the head-height cells listed above is solid.

### 1.5 Step 4: kinds that never block sideways

If S's kind is **1 (Door), 2 (BigDoor), 6 (FireTrap), 7 (SpikeTrap) or 8 (StompTrap)**, the
next static is taken. The state byte is not read, so a closed door never blocks the search.
Kinds 3 (Window), 5 (Gate) and 9 (Lever) are **not** exempt here, unlike in the physics
(§2.2).

### 1.6 Step 5: the creature's target static

If the three 32-bit words at +0x0c/+0x10/+0x14 of S's index-cell list node all equal the
three words at creature +0x1478/+0x147c/+0x1480, the next static is taken. If any word
differs, step 6 runs. What these words mean is unresolved here (§5).

### 1.7 Step 6: the four side tests

For each direction, S blocks that direction when it overlaps N **shifted one block** that
way. The z range is **not** shifted:

| Flag | x test | y test |
|---|---|---|
| **static −x** | X − hx ≤ nx₊ − 1 and nx₋ − 1 < X + hx | Y − hy ≤ ny₊ and ny₋ < Y + hy |
| **static +x** | X − hx ≤ nx₊ + 1 and nx₋ + 1 < X + hx | (same as −x) |
| **static −y** | X − hx ≤ nx₊ and nx₋ < X + hx | Y − hy ≤ ny₊ − 1 and ny₋ − 1 < Y + hy |
| **static +y** | (same as −y) | Y − hy ≤ ny₊ + 1 and ny₋ + 1 < Y + hy |

The z test for all four is **Z ≤ nz₊ and nz₋ < Z + H**: the static's base is at or below the
head top, and its top is above the feet. The four tests run in the order −x, +x, −y, +y. A set
flag forbids the straight move toward x − 1, x + 1, y − 1 or y + 1 respectively (server: the
straight-move checks following the statics pass of the same routine).

### 1.8 What this means

* On ordinary ground the gate of §1.4 is open, so every non-exempt static whose shifted
  overlap holds blocks the corresponding straight move, whatever its height: tables, stools,
  benches, beds, fences, barrels and crates included.
* A static lower than 3 blocks is passed through by the planner only where the gate is
  closed, that is where the +y floor-level cell is not solid and no listed head-height cell is
  solid. Statics of height 3 or more always pass the gate.
* Wherever a node lies over or partly inside any static, including an exempt kind, the static
  counts as "on static" support (§1.3).

### 1.9 Worked example

Default box (0.96, 0.96, 2.16), flat solid ground at z = 4, node (10, 20, 5): N spans
x [10.02, 10.98], y [20.02, 20.98], z [5, 7.16]; cell range x₀ = x₁ = 10, y₀ = y₁ = 20,
z₀ = 5, z₁ = 7. The cell (10, 21, 4) is ground, so the last gate item holds.

**Fence** (kind 52) at (12, 20.5, 5), rotation 0, scale (2, 1, 1.5): box x [11, 13],
y [20, 21], z [5, 6.5].

* On static: the x test needs 11 ≤ 10.98, which fails.
* Gate: H = 1.5 < 3, but the +y floor cell (10, 21, 4) is solid, so the gate is open.
  Kind 52 is not exempt. The +x shifted box x [11.02, 11.98] overlaps [11, 13]; y and z
  overlap. **static +x** is true and the move to (11, 20, 5) is refused.
* Variant: if (10, 21, 4) is not solid (and no head-height cell is solid), the gate is closed,
  **static +x** stays false and the move is allowed. At node (11, 20, 5) (N x
  [11.02, 11.98]) the lowered z range [4, 6.16] overlaps [5, 6.5], so **on static** is true.

**Table** (kind 12) at (10.5, 22.5, 5), scale (3, 3, 1): box x [9, 12], y [21, 24], z [5, 6].

* From (10, 20, 5): the gate is open (ground cell), +y shifted y [21.02, 21.98] overlaps
  [21, 24], x overlaps, z: 5 ≤ 7.16 and 5 < 6, so **static +y** is true and the move to
  (10, 21, 5) is refused.
* On static: node (10, 21, 5), lowered z [4, 6.16] against [5, 6], gives on static. Node
  (10, 21, 6) (feet on the table top), lowered [5, 7.16], gives on static. Node
  (10, 21, 7), lowered [6, 8.16]: 6 < 6 fails, so it is not on static.

**Market stand** (kind 21) at (10.5, 23, 5), scale (3.5, 2, 3): box x [8.75, 12.25],
y [22, 24], z [5, 8]. From (10, 21, 5): H = 3 ≥ 3 opens the gate; +y shifted y
[22.02, 22.98] overlaps, so **static +y** is set. From (10, 20, 5) the +y test fails
(22 ≤ 21.98 is false): the stand is two cells away.

If S's list-node triple equals the creature's +0x1478 triple, step 5 skips S and S does not
block.

---

## 2. Q-6: object collision in the physics step (client RVA 0x20c510; server twin RVA 0x1322d0)

The details below were read from the client. The server twin performs the same
ignored-statics lookup (server RVA 0x1415f9); its other details were not re-read (§5).

Frame values: pos = the creature position (creature +0x10) after this substep's move on the
current axis k; d[k] = this substep's move on axis k, in blocks; s = creature +0x80.

**When the object tests run** (client RVA 0x21b6cd–0x21b728): on the x and y axes, only
when the cell test of that axis found no solid cell. On the z axis, the statics (§2.1–§2.3)
also run after a cell hit, unless "standing on an object" is already set during this step.
That flag is cleared once per step, before the substeps (client RVA 0x21b444). The airship
test (§2.4) runs only when no cell was hit on that axis and no static was hit.

### 2.1 Which statics (client RVA 0x21b72e–0x21b7b6)

These are the statics listed in the 3 × 3 index cells around the creature's position:
index cell (cx + i, cy + j) with i, j ∈ {−1, 0, 1}, i outer and j inner. Here cx is the
creature's x block coordinate (the fixed-point x divided by 65536, rounding toward zero)
divided by 8, again rounding toward zero; cy is formed the same way from y. Index-cell lists
come from the zone grid lookup at client RVA 0x02f640.

### 2.2 Static kind table and pre-tests (client RVA 0x21b80d–0x21b928)

Per static, in this order:

1. Kinds **6, 7, 9** are skipped (never collide).
2. If the static is in the creature's ignored-statics set (+0x1468), it is skipped.
3. Kinds **1, 2, 3, 5, 8** are skipped while the **state byte +0x30 is 0**; they collide
   while it is nonzero.
4. The approach test: q = trunc(float(pos[k] − S.pos[k]) · d[k]), where the difference is
   in fixed point and the product is truncated toward zero. If **q ≥ 0** the static is
   skipped. A static therefore only stops motion **toward** its reference point on that axis
   (centre on x and y, base on z), and a product of magnitude below one fixed-point unit also
   skips. A creature overlapping a static can always move away from its reference point.
   On z, a creature whose centre is above a static's base is stopped only while moving down,
   and one whose centre is below it only while moving up.

| Kind | Physics | Search side-block (§1.5) | Search "on static" (§1.3) |
|---|---|---|---|
| 6, 7, 9 | never collide | 6, 7 never; **9 can** | yes |
| 1, 2, 3, 5, 8 | collide only while state +0x30 ≠ 0 | 1, 2, 8 never; **3, 5 can** (state not read) | yes |
| every other kind | always collide | can (gate §1.4) | yes |

### 2.3 Static box test (client RVA 0x21b92e–0x21bbb8)

This tests the overlap of the creature box at the moved position with the static box, on all
three axes, in the order x, y, z:

| Axis | Test |
|---|---|
| x | X − hx ≤ pos.x + w/2 and pos.x − w/2 < X + hx |
| y | Y − hy ≤ pos.y + d/2 and pos.y − d/2 < Y + hy |
| z | Z ≤ pos.z + h/2 and pos.z − h/2 < Z + H |

The first static that passes ends the static scan. On that hit (client RVA 0x21bc61):

* if k is z and the creature's z velocity (z component of the vector at creature +0x34) is
  below 0, "standing on an object" is set;
* "touched an object" is set (it later sets contact flag 0x40 at creature +0x5c, client RVA
  0x21cdba);
* step-up is disabled for the rest of the step;
* the axis response of `alpha-physics-camera.md` §4.2 follows. On x and y the move is
  therefore not followed by a step-up.

### 2.4 Airships (client RVA 0x21bcb0–0x21c13e)

* **List**: the world object, G + 0x2e4 (the object the physics routine runs on), holds at
  its +0x0c (G + 0x2f0) an ordered map whose values point to airship objects. The model is
  registry slot **0xa03** (client RVA 0x21bd5a). An airship object is 0xa0 bytes
  (constructor client RVA 0x066510). It holds a set at +0x78 (§2.4 step 3), its position at
  +0x80 (3 × i64) and its yaw in degrees at +0x98.
* **Creation**: the only call of that constructor is in the region generator routine
  (client RVA 0x1af170) at client RVA 0x1ba7b6, after a 0xa0-byte allocation at 0x1ba799.
  Whether that branch is reachable in the shipped generator, and whether anything inserts
  into the map, is unresolved (§5).
* **Test** for each airship A whose object pointer is not null, with
  |pos − A.pos|² ≤ **4096** (distance in blocks, centre to centre; constant 4096.0 at client
  RVA 0x302b80; a larger value skips A), and with the 0xa03 model present (sizes mx, my, mz
  at model +0x44, +0x48, +0x4c):
  1. Model-space centre: c = Rz(−yaw) · (pos − A.pos) + (mx/2, my/2, mz/2), in blocks
     (fixed point × 1/65536). The rotation is about z by A's yaw converted with
     0.0174533 rad/degree, in the standard counter-clockwise sense. The order is confirmed:
     translate, then rotate, then offset by half the model size (client RVA
     0x21bd77–0x21be33).
  2. Voxel range: lo = c − s/2 and hi = c + s/2, taken along the model axes (the box is not
     rotated). Each component is converted by multiplying by 65536 and truncating, then
     divided by 65536 rounding toward zero, with one subtracted when the fixed-point value is
     negative. lo ≤ hi always holds. The voxels (i, j, k) with lo ≤ (i, j, k) ≤ hi are visited
     with i outer, j middle, k inner.
  3. Voxel lookup (client RVA 0x0a2870): indices outside 0 ≤ i < mx, 0 ≤ j < my,
     0 ≤ k < mz give a shared default voxel. Otherwise the voxel is the 3 colour bytes at
     model data (+0x30) + ((my · k + j) · mx + i) · 3. A voxel whose 3 bytes are not all zero
     is a **hit**.
  4. On every hit the scan continues: the axis counts as **blocked like a solid cell**. This
     is not an object hit, so step-up stays allowed and contact 0x40 is not set. If k is z,
     "on a voxel object" is set. On **any** axis the base height b is set to
     A.pos.z − trunc(mz · 0.5 · 65536), the model's bottom in fixed point (client RVA
     0x21c04f–0x21c092). The creature's id (creature +0x08) is inserted into A's set
     (+0x78).
* **Base reset**: b is reset to 0 at the start of every substep (client RVA 0x21b4c0).
* **Step-up** (client RVA 0x21c6e7–0x21cc23): the step-up box of `alpha-physics-camera.md`
  §4.3 is also tested against the airships the same way. A hit marks the step as blocked and
  registers the creature in A's set. On success the height is computed **relative to b**:
  b is subtracted from pos.z (client RVA 0x21cb18), the step computation of that section runs
  (it uses the constant 1.01 at client RVA 0x31e1f8), and b is added back (client RVA
  0x21cc17). Because b is set by an airship hit on any axis, b is nonzero during a step-up
  that follows an airship voxel hit on the same axis in the same substep.
* **Riding** (client RVA 0x21f6e7): a creature whose u64 at creature +0x1d30 is nonzero is
  carried by the airship with that id. The physics routine itself also writes that field
  (client RVA 0x210167). Riding is not specified further here.

### 2.5 Worked example

A player, s = (0.96, 0.96, 2.16), centre (100.5, 50.5, 21.08) (feet 20), moves +x by 0.2
toward a **closed door** (kind 1, state 1) at (101.5, 50.5, 20), scale (0.5, 1, 3),
rotation 0: box x [101.25, 101.75], y [50, 51], z [20, 23].

* The cells are clear. Kind 1 with state 1 is tested and the door is not in +0x1468. The
  approach test uses pos.x after the move, 100.7: (100.7 − 101.5) · 0.2 < 0, so the door is
  tested.
* x: 101.25 ≤ 100.7 + 0.48 = 101.18 fails, so there is no hit. Later substeps bring the face
  to 101.25; then the door is hit. The axis response follows, contact 0x40 is set and there
  is no step-up.
* With the door open (state 0) it is skipped and the player walks through. A **Lever**
  (kind 9) or SpikeTrap is always passed through.
* A **Table** (3 × 3 × 1) in the same place is hit on x, with no step-up even though it is
  1 block high. Falling onto it: on z the centre is above the base and v.z < 0, so the table
  is hit and "standing on an object" is set.
* Starting inside a **fence** whose centre is at x = 100 and walking +x:
  (pos.x − 100) · d > 0, so the fence is skipped and the player leaves freely. Walking back
  toward its centre is blocked.
* With an empty airship map, §2.4 contributes nothing.

---

## 3. I-3: the comparison panel `equippedpreview` (client RVA 0x08ef0b–0x08f17e, in 0x088ee0)

The GUI nodes are looked up by name at client RVA 0x05cb3b (`preview`, stored at G+0x8008e4)
and 0x05cb76 (`equippedpreview`, stored at G+0x8008ec). The PreviewWidget built over the
`preview` node is stored at G+0x8008e8 (client RVA 0x061a42).

Each frame, after the tooltip item is chosen and placed (`alpha-ui-screens.md` §8.1.7):

1. The PreviewWidget stores the shown item pointer at +0x160 and clears its compare list
   (+0x174).
2. If there is a shown item, the **compare list** is built from the local player's creature
   (G+0x8006d0). An equipped item is appended when its **first byte (item type) equals the
   shown item's** and it is not excluded. The slots are tested in this order: +0xaa8, +0x990,
   +0x1020, +0x878, +0x530, +0x760, +0x648, +0x418, +0xcd8, +0xbc0.
   * +0xaa8/+0x990 and +0xcd8/+0xbc0 are tested as **pairs**: if the shown item **is**
     either slot of a pair, **neither** slot of that pair is appended.
   * Every other slot is skipped only when it is the shown item itself.
3. `equippedpreview` is made **visible exactly when the compare list is not empty and the
   `preview` node is visible**, and hidden otherwise (client RVA 0x08f14d–0x08f17e).

So any tooltip source shows the comparison whenever an equipped item has the same type
byte as the shown item and the shown item is not that item or its pair partner. Examples:
hovering a bag sword with a sword equipped shows the panel with the equipped sword. Hovering
the equipped sword shows no panel, and its pair partner is excluded too. Hovering a bag ring
with two rings equipped lists both.

**Placement** (client RVA 0x08ed2f–0x08eeb0): with the window source, `equippedpreview` is
placed at (−235, 20) added to a position from the GUI root (G+0x800710) *believed* to be the
cursor. With the world source, `preview` is placed at (W − 320, H − 320) and
`equippedpreview` at (W − 320, H − 575), where W and H are the GUI root's width and height.

**Text** (inside client RVA 0x0d50a0; "Currently equipped" referenced at 0x0d68ca; item loop
0x0d6b3f–0x0d6bd5): the PreviewWidget draws "Currently equipped". It then draws one item text
block per listed item, in list order, at an x 240 px left of its reference x. Each call passes
scale 1.0, wrap width 230 and two flags set to 1, plus the widget byte +0x164. The vertical
position starts 10 px below the reference y; each block is drawn 30 px below the running
position, and the running position advances 130 px per item. *As read*, with the world
source the box sits 255 px higher than the tooltip node, while the text is still drawn
relative to the tooltip, so text and box do not line up.

---

## 4. Where this document supersedes companion specs

| Companion | Section | Native behaviour per this document |
|---|---|---|
| `alpha-path-search.md` | §0.1 (+0x1478) | three words compared with the static's index-list node; a match skips side-blocking (§1.6) |
| `alpha-path-search.md` | §4.1 | the search skips statics in the +0x1468 set (§1.2); so does the physics (§2.2) |
| `alpha-path-search.md` | §4.3 | "on static" counts any static, any kind and state (§1.3) |
| `alpha-path-search.md` | §4.4 Statics | gate, exemptions, target skip and side tests (§1.4–§1.7) |
| `alpha-physics-camera.md` | §4.5 | the voxel objects are the airship map; model-space test as in §2.4 |
| `alpha-ui-screens.md` | §8.1.12 I-3 | `equippedpreview` visible iff compare list non-empty and `preview` visible (§3) |

---

## 5. Unresolved

* **OP2-1** Reachability of the sole airship constructor call (client RVA 0x1ba7b6 in the
  region generator 0x1af170) in the shipped generator, and whether anything inserts into the
  map at G+0x2f0. Whether the map is always empty in the shipped Alpha is therefore not
  established.
* **OP2-2** Whether the airship drawing code walks the same map (an earlier reading cited
  client RVA 0x0b35dc; not re-checked).
* **OP2-3** Who sets the riding id at creature +0x1d30 besides the physics write at client
  RVA 0x210167, and the riding behaviour itself.
* **OP2-4** What the three words at creature +0x1478/+0x147c/+0x1480 and at +0x0c..+0x14 of
  the index-list node denote, and the value they hold when no target is set.
* **OP2-5** The runtime value of the shared default voxel returned for out-of-range lookups.
  It lies in the zero-initialised part of the client data section, so it is *believed* to be
  all zero (no hit); writers to it were not ruled out.
* **OP2-6** The `equippedpreview` box size and appearance (from the `gui.plx` template), the
  meaning of the two flags and the +0x164 byte passed to the item text call, and whether
  model, stars or price are drawn for listed items.
* **OP2-7** The exact position source (G+0x800710) of the window-source placement, believed
  to be the cursor.
* **OP2-8** The server physics twin (server RVA 0x1322d0) beyond its ignored-statics lookup.
* **OP2-9** The full cell solidity classification used by the gate cells, which belongs to
  `alpha-path-search.md` §4.3.

---

## Re-certification (2026-11-14)

All facts were re-read against `Cube.exe` and `Server.exe` (both with image base 0x400000)
and their listings. Addresses are now given as RVAs with the image named.

**Confirmed**: static record layout and the parity swap; truncating float-to-fixed;
ignored-set lookup by record address in the search and the physics; the "on static" lowered
test with no kind or state check; the 3.0 gate constant (server RVA 0x158718, client RVA
0x2fcd70); the search exemption kinds 1, 2, 6, 7, 8 without a state read; the three-word
target compare; the four shifted side tests and their z test; the physics kind classes
(6/7/9 never; 1/2/3/5/8 by state) and the order kind, ignored set, state, approach, box; the
approach test formula and q ≥ 0 skip; the static box table; the hit effects (standing flag
on z with negative z velocity, touched flag leading to 0x40, step-up disabled); the 4096
squared distance (client RVA 0x302b80); model slot 0xa03; airship object 0xa0 bytes with
+0x78/+0x80/+0x98; voxel hits block like cells without touching contact or step-up; creature
id inserted into +0x78; b reset per substep and b-relative step-up; compare list rules and
visibility condition; nodes `preview`/`equippedpreview`; the window offset (−235, 20); the
world positions (W − 320, H − 320)/(W − 320, H − 575); text x − 240, wrap 230, 130 px
spacing.

**Corrected** (old → new):

1. Statics pass location "server 0x4de6xx" → server RVA 0x0de968–0x0df77b; all addresses →
   RVAs.
2. Node box centre (x + 0.5, y + 0.5, z + h/2) "believed" → confirmed.
3. Index-cell division "floor" → integer division rounding toward zero, with negative or
   ≥ 0x200000 index cells skipped. The physics block coordinate is likewise truncated.
4. **Reversal**: "low furniture never blocks the search on open ground" → the gate's
   +y floor-level item opens the gate on ordinary ground, so low non-exempt statics do block
   sideways there. The §1.8 conclusions and the fence and table worked examples were
   rewritten: both now block. The physics-versus-planner "stuck" inference was removed.
5. Kind 6 name "unseen trap" → FireTrap (static-kind name table).
6. "Object tests run after the cell test found no solid cell" → on z, statics also run after
   a cell hit unless "standing on an object" is already set this step (that flag is cleared
   once per step, not per substep). Airships are skipped after a cell hit.
7. Airship voxel range: "transform both box corners; the range can be empty near
   90°/180°; hi rounded by 0x412280" → the centre is transformed and ± s/2 is applied along
   the model axes, so lo ≤ hi always. Both corners are converted the same way (truncate to
   fixed point, divide toward zero, minus one if negative); 0x412280 is a vector add.
8. Transform order "believed" → confirmed: translate, rotate by −yaw, add half the model
   size.
9. Base height b "set only on a z-axis hit, so always 0 in step-up" → set on a voxel hit on
   any axis, so it can be nonzero in a step-up.
10. Voxel solidity "non-empty" → a voxel whose 3 colour bytes are not all zero, with the
    bounds stated.
11. Riding field "record +0x10e0, set only by the dead kind-4 code" → creature +0x1d30; the
    physics also writes it; the setter question moved to unresolved.
12. "In the shipped Alpha the airship map is empty" → unresolved (OP2-1).
13. PreviewWidget draw "client 0x4d6af0 routine" → a range inside client RVA 0x0d50a0; the
    text y layout is now given (start +10, +30 per block, +130 per item).
14. Compare-list slot order stated explicitly (weapon pair first, ring pair last).

**Removed**: implementer advice ("An implementation may leave it out or keep it as a
hook"); the game-check and logging procedures of the former open-points list; the claim
that a planned fence path matches an observed "NPC stuck on fence" behaviour; the climb-move
remark (companion content not re-checked); the "diagonals only through straight moves"
remark; specific furniture dimensions taken from companion specs; statements quoting the
wording of companion specs.

**Unresolved**: OP2-1 to OP2-9 in §5. No random draws occur in any behaviour specified
here.
