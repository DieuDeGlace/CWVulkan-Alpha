# Alpha open points, batch 2: path-search statics, object collision, the compare panel

Behavioural specification resolving three open points left by earlier specs:

1. **PS-2** of `alpha-path-search.md` (§4.3, §4.4): how the search classifies the static
   objects around a node ("on static" and the four side-blocking flags);
2. **Q-6** of `alpha-physics-camera.md` (§4.5): the object tables of the physics step and
   the "voxel objects within 64 blocks" test;
3. **I-3** of `alpha-ui-screens.md` (§8.1.7, §8.1.12): when the `equippedpreview`
   comparison panel is visible.

Written from the Alpha decompile and the per-instruction listings
(`disassembled alpha cube world/decompiled/…`) and constants read from the two executables.
No code was copied; addresses are citations only ("server 0x…", "client 0x…"). *Believed*
marks partial evidence; *as read* marks behaviour that looks unintended but is what the code
does. This document adds to the specs named above and does not repeat them.

Conventions are those of `alpha-path-search.md` §0 and `alpha-physics-camera.md` §0:
positions are 3 × i64 in 1/65536 block; a creature's box **s** = (w, d, h) (obj +0x80) is
centred on its position; a static's record (`alpha-interaction.md` §4.1) has kind +0x00,
position +0x08/+0x10/+0x18 (x, y at the footprint centre, z at the base), rotation index
+0x20, scale +0x24/+0x28/+0x2c and state byte +0x30.

**Static box** (used by every test here): half-widths hx = sx/2, hy = sy/2 with sx, sy the
scale x and y **swapped when the rotation index is odd**; height H = scale z. The box is
x ∈ [X − hx, X + hx], y ∈ [Y − hy, Y + hy], z ∈ [Z, Z + H]. Every float-to-fixed conversion
truncates (·65536, toward zero).

---

## 1. PS-2: statics in the path search (server 0x4dde90 at 0x4de6xx…0x4df77b; client 0x5aaab0)

### 1.1 Where it runs

In one search round (`alpha-path-search.md` §4.2 step 5), after the cell flags of §4.3 have
all been computed and before the neighbours are tried. The node box N is the creature's
**real box** (w, d, h), centred on (x + 0.5, y + 0.5, z + h/2) for node cell (x, y, z)
*believed* (the 0.01 raise of §4.3 is used for the cell range only). Write N's faces
nx₋ = x + 0.5 − w/2, nx₊ = x + 0.5 + w/2, likewise ny₋, ny₊, and nz₋ = z (the feet),
nz₊ = z + h.

The statics looked at are those listed in the zone **cell index** (8 × 8-block cells,
`alpha-interaction.md` §4.3) for every index cell covering the grown cell range of §4.3:
index-x from floor((x₀ − 1)/8) to floor((x₁ + 1)/8), index-y likewise. For the default
box that is one cell, or two or four near an index-cell border — **not** a 3 × 3 block.
*As read*: a static listed in an index cell outside that range is not seen, even if its
box reaches the node (only a problem for statics wider than the gap to their cell edge).

Every listed static S is processed as follows, in list order. The four side flags and
"on static" start false for the node and are only ever set, never cleared.

### 1.2 Step 1 — ignored statics

If S is in the creature's **ignored-statics set** (+0x1468, the statics touching the start
or goal box, `alpha-path-search.md` §4.1 step 3; looked up by record address): skip S
entirely. **Correction**: `alpha-path-search.md` §4.1 says "the search itself does not
consult the set"; it does — ignored statics neither support nor block in the search (and
they are ignored by the physics too, §2.2).

### 1.3 Step 2 — "on static" (any kind, any state)

Set **on static** when S overlaps N **lowered by one block**:

| Axis | Test |
|---|---|
| x | X − hx ≤ nx₊ **and** nx₋ < X + hx |
| y | Y − hy ≤ ny₊ **and** ny₋ < Y + hy |
| z | Z ≤ nz₊ − 1 **and** nz₋ − 1 < Z + H |

There is **no** kind or state test here: doors (open or closed), traps, levers and the
model-less kinds all count. **Correction** to `alpha-path-search.md` §4.3 ("a non-passable
static"). In words: the creature would stand on S's top (feet within one block above the
top, not reaching it) or is partly inside S.

### 1.4 Step 3 — the side-blocking gate

S can block sideways only when **at least one** of these holds (otherwise go to the next
static):

* its height H ≥ **3.0** blocks (server constant 0x558718, client 0x6fcd70);
* the node's **ceiling** flag (§4.3: solid cell over the body);
* a "**ceiling over −x / +x / −y**" flag: a solid cell at height z₁ + 1 in the column face
  x₀ − 1 (resp. x₁ + 1, y₀ − 1) of §4.3 (these three flags are computed in the cell pass but
  not listed in §4.3's table);
* the fourth flag of that family, which *as read* tests the face y₁ + 1 at the **floor**
  height z₀ − 1, so it is set whenever the **ledge +y** flag is (`alpha-path-search.md`
  §4.3 "ledge +y" note). So any ground cell beside the floor on the +y side opens the gate.

### 1.5 Step 4 — kinds that never block sideways

If S's kind is **1 (door), 2 (big door), 6 (unseen trap), 7 (spike trap) or 8 (stomp
trap)**: next static. The state byte is not read: a closed door never blocks the search
(the follower opens it, `alpha-path-search.md` §6.5). Kinds 3 (window), 5 (gate) and 9
(lever) are **not** exempt here, unlike in the physics (§2.2).

### 1.6 Step 5 — the creature's target static

If S's index triple (zone x, zone y, index; carried in the cell-index node) equals the
creature's **target triple** obj +0x1478/+0x147c/+0x1480 in all three words: next static.
That field is the RandomInteraction seat target (`alpha-trails-poses-schedules.md` §4.2;
"none" = (−1, −1, 0), which matches no static). So a resident walking to a stool, bench
or bed is never side-blocked by it. This answers the "+0x1478 *unknown*" row of
`alpha-path-search.md` §0.1.

### 1.7 Step 6 — the four side tests

For each direction, S blocks it when it overlaps N **shifted one block** that way, with
the z range **not** shifted:

| Flag | x test | y test |
|---|---|---|
| **static −x** | X − hx ≤ nx₊ − 1 and nx₋ − 1 < X + hx | Y − hy ≤ ny₊ and ny₋ < Y + hy |
| **static +x** | X − hx ≤ nx₊ + 1 and nx₋ + 1 < X + hx | (same as −x) |
| **static −y** | X − hx ≤ nx₊ and nx₋ < X + hx | Y − hy ≤ ny₊ − 1 and ny₋ − 1 < Y + hy |
| **static +y** | (same as −y) | Y − hy ≤ ny₊ + 1 and ny₋ + 1 < Y + hy |

z test for all four: **Z ≤ nz₊ and nz₋ < Z + H** (the static's base at or below the head
top, its top above the feet).

The four flags then gate moves 1–4 exactly as `alpha-path-search.md` §4.4's table says
("no static −x", …). Diagonals are affected only through their two straight moves.

### 1.8 What this means

* **Low furniture never blocks the search on open ground**: tables (scale z 1), stools
  (0.5), beds (1), fences (1.5, `alpha-worldgen-structures.md` §5.5.3), barrels and crates
  shorter than 3 are walked through by the planner; they become "on static" support when
  the creature stands over them. Only statics ≥ 3 tall (market stands 3.5 × 2 × 3, tall
  shelves, the 3-tall statics of the generators) block sideways — or any static when the
  node's head or a head-height side cell is closed, which stops the planner from routing
  "under" a low ceiling onto a static it could not step onto.
* The physics does block low furniture (§2), so a planned path through a fence makes the
  creature push into it until the follower's stuck handling (`alpha-path-search.md` §6)
  re-plans (*believed* to be the observed "NPC stuck on fence" behaviour).
* There is no fall or step limit tied to statics; climbing onto a static uses the normal
  climb move (10), which needs a solid-**cell** wall, so the search never climbs onto a
  table: it only treats it as floor once a node is over it.

### 1.9 Worked example

Default box (0.96, 0.96, 2.16), flat solid ground at z = 4, node (10, 20, 5): N spans
x [10.02, 10.98], y [20.02, 20.98], z [5, 7.16]; cell range x₀ = x₁ = 10, y₀ = y₁ = 20,
z₀ = 5, z₁ = 7; no ceiling flags.

**Fence** (kind 52) at (12, 20.5, 5), rotation 0, scale (2, 1, 1.5): box x [11, 13],
y [20, 21], z [5, 6.5].

* On static: x needs 11 ≤ 10.98 — no.
* Gate: H = 1.5 < 3, no ceiling flags → skipped. **static +x stays false**; move 2 to
  (11, 20, 5) is allowed (cost 10).
* At node (11, 20, 5) (N x [11.02, 11.98]): lowered z [4, 6.16] against [5, 6.5] →
  **on static** true. The planner walks straight over the fence.
* Variant: put a solid block at (10, 20, 8) (head height over the node) → ceiling flag →
  gate open; kind 52 is not exempt; +x shifted box x [11.02, 11.98] overlaps [11, 13], y and
  z overlap → **static +x** true, move 2 refused.

**Table** (kind 12) at (10.5, 22.5, 5), scale (3, 3, 1): box x [9, 12], y [21, 24], z [5, 6].

* From (10, 20, 5): +y shifted y [21.02, 21.98] overlaps, but H = 1 → not blocked.
* Node (10, 21, 5): lowered z [4, 6.16] vs [5, 6] → on static. Node (10, 21, 6) (feet on
  the table top): lowered [5, 7.16] → on static. Node (10, 21, 7): lowered [6, 8.16];
  6 < 6 fails → not on static (so the drop move applies).

**Market stand** (kind 21) at (10.5, 23, 5), scale (3.5, 2, 3): box x [8.75, 12.25],
y [22, 24], z [5, 8]. From (10, 21, 5): H = 3 ≥ 3 → gate open; +y shifted y [22.02, 22.98]
overlaps → **static +y**, move 4 refused. From (10, 20, 5) the +y test fails (22 ≤ 21.98 is
false), as it should: the stand is two cells away.

If the creature is a resident whose target is that stand's triple, step 5 skips it and the
move is allowed.

---

## 2. Q-6: object collision in the physics step (client 0x60c510, server twin 0x5322d0)

Both tests run inside the per-axis move of `alpha-physics-camera.md` §4.1 step 3, after
the cell test of that axis found no solid cell. Fixed order: statics first (§2.2), then the
voxel objects (§2.4) only if no static was hit.

Frame values used (client listing): pos = creature position after moving on the current
axis k (obj +0x10); d[k] = this substep's move on axis k (blocks); s = obj +0x80.

### 2.1 Which statics (client 0x61b72e…0x61b828)

The statics listed in the 3 × 3 index cells around the creature's **centre**: index cell
(floor(bx/8) + i, floor(by/8) + j), i, j ∈ {−1, 0, 1}, bx, by = the centre's block
coordinates (zone grid 0x42f640, as `alpha-interaction.md` §4.3).

### 2.2 Static kind table (client 0x61b80d…0x61b888)

| Kind | Physics | Search side-block (§1.5) | Search "on static" (§1.3) |
|---|---|---|---|
| 6 unseen trap, 7 spike trap, 9 lever | **never** collide | 6, 7 never; **9 can** | yes |
| 1 door, 2 big door, 3 window, 5 gate, 8 stomp trap | collide only while the **state byte +0x30 ≠ 0** (closed / raised) | 1, 2, 8 never; **3, 5 can** (state not read) | yes |
| every other kind (0, 4, 10–77) | always collide | can (gate §1.4) | yes |

Then, before any box test:

1. **Ignored statics**: if the static is in the creature's +0x1468 set (§1.2): skip it.
   A creature that started a path search therefore also walks through the statics that
   touched its start and goal boxes (e.g. the seat a resident is heading for), until the
   search state is cleared.
2. **Approach test**: let q = trunc(float(pos[k] − S.pos[k]) · d[k]) (fixed-point
   difference times the move, client 0x468cf0, compared with 0 by 0x6091d0). If **q ≥ 0**
   skip the static: a static only stops motion **toward** its reference point on that axis
   (centre on x and y, base on z). So a creature already overlapping a static can always
   move out of it, and on z a creature whose centre is above a static's base is stopped
   only while moving down (landing), one below it only while moving up (head bump).

### 2.3 Static box test (client 0x61b88e…0x61bbb8)

Overlap of the creature box at the moved position with the static box, all three axes:

| Axis | Test |
|---|---|
| x | X − hx ≤ pos.x + w/2 and pos.x − w/2 < X + hx |
| y | Y − hy ≤ pos.y + d/2 and pos.y − d/2 < Y + hy |
| z | Z ≤ pos.z + h/2 and pos.z − h/2 < Z + H |

On a hit (client 0x61bc61): if k is z and **v.z < 0**, "standing on an object" := true;
"touched an object" := true (contact flag 0x40 later); step-up is turned off for the rest
of the step; the axis response of `alpha-physics-camera.md` §4.2/§4.3 follows (so on x/y
the move is undone without a step-up: low furniture must be jumped onto). These confirm
§4.5's first bullet; the new facts are §2.2 items 1–2 and the exact table.

### 2.4 The voxel objects are airships (client 0x61bcb0…0x61c13e)

* **List**: the world object (G + 0x2e4) keeps at +0x0c (G + 0x2f0) a **map from a 64-bit
  id to an airship object**; the airship draw (client 0x4b35dc…) walks the same map. The
  model is registry slot **0xa03 = 2563 = `airship`** (`alpha-creatures-models.md`
  Appendix A, row 2560). The airship object (0xa0 bytes, constructor client 0x466510) has
  its position at +0x80 (3 × i64, the model centre), its yaw in degrees at +0x98, and at
  +0x78 a set of the ids of creatures touching it (§2.5).
* **What creates them**: the only constructor call is the kind-4 "landing place" plot code
  (client 0x5ba7b6, `alpha-worldgen-structures.md` §5.5.5), which is dead in the shipped
  generator (no plot gets kind 4). Nothing else was found that creates one *believed*. So
  **in the shipped Alpha the map is empty and this collision never fires**; placed statics,
  item stands and world items are **not** part of it. An implementation may leave it out
  or keep it as a hook for airships.
* **Test** for each airship A with |pos − A.pos|² ≤ **4096** (64 blocks, centre to centre,
  constant 0x702b80) and the airship model present (size m = (mx, my, mz) voxels, model
  fields +0x44, +0x48, +0x4c):
  1. Build the transform model ← world: p ↦ Rz(−A.yaw)·(p − A.pos) + (mx/2, my/2, mz/2)
     (client 0x424a60, 0x424610, 0x424990; order *believed*), in blocks, one voxel per
     block.
  2. Transform the creature box's two corners pos − s/2 and pos + s/2; lo = floor of the
     first, hi = the rounding of the second by 0x412280 (*believed* floor as well); for
     every voxel (i, j, k) with lo ≤ (i, j, k) ≤ hi per axis (as computed, without
     re-sorting the corners after the rotation — *as read*, at a yaw near 90°/180° the range
     can be empty) look the voxel up (0x4a2870); a non-empty voxel is a **hit**.
  3. On a hit: the axis counts as **blocked like a solid cell** (not as an object: step-up
     stays allowed, contact 0x40 is not set); if k is z, "on a voxel object" := true and the
     **base height** b := A.pos.z − mz/2 (fixed point, the model's bottom); the creature's
     id (obj +0x08) is inserted into A's set (+0x78).
* **Step-up** (client 0x61c6e7…0x61cc23): the step-up box (raised 1.01,
  `alpha-physics-camera.md` §4.3 step 1) is also tested against the airships the same way;
  a hit fails the step (and also registers the creature in A's set). On success the new
  height is computed **relative to b**: pos.z −= b, then the 1.01 step of §4.3 step 2, then
  pos.z += b — so steps snap to the airship's voxel grid instead of the world grid.
  *As read*, b is reset to 0 at the start of every substep (0x61b4c0) and is only set by a
  z-axis hit, which comes after the x and y axes; step-up happens on x and y only, so the
  re-added b is **always 0** in practice.
* **Riding** (client 0x61f6e7…): a creature whose record field +0x10e0 (obj +0x1d30, u64)
  holds an airship id is carried by that airship each step (only the dead kind-4 code sets
  it). Not specified further.

### 2.5 Worked example

A player, s = (0.96, 0.96, 2.16), centre (100.5, 50.5, 21.08) (feet 20), moves +x by 0.2
toward a **closed door** (kind 1, state 1) at (101.5, 50.5, 20), scale (0.5, 1, 3),
rotation 0: box x [101.25, 101.75], y [50, 51], z [20, 23].

* Cells clear. Kind 1 with state 1 → counts. Not in +0x1468. Approach: pos.x after the move
  100.7, (100.7 − 101.5) · 0.2 < 0 → test it.
* x: 101.25 ≤ 100.7 + 0.48 = 101.18? No → no hit. Next substeps bring the face to 101.25,
  then the hit: move undone, flag 0x40, no step-up.
* Door open (state 0): skipped, the player walks through. A **lever** (kind 9) or spike trap
  is always passed through.
* A **table** (3 × 3 × 1) in the same place: hit on x, no step-up even though it is 1 block
  high — the player has to jump. Falling onto it: on z, centre above the base and v.z < 0 →
  hit → "standing on an object" → on ground.
* Spawned inside a **fence** and walking out (+x away from the fence's centre at x = 100):
  (pos.x − 100) · d > 0 → skipped, the player leaves freely; walking back in is blocked.
* No airship exists, so §2.4 adds nothing.

---

## 3. I-3: the comparison panel `equippedpreview` (client 0x48ef0b…0x48f17e)

**Answer**: code does toggle it. Each frame, after the tooltip item is chosen
(`alpha-ui-screens.md` §8.1.7 "Which item is shown") and placed:

1. The PreviewWidget (G+0x8008e8) stores the shown item at +0x160 and clears its compare
   list (+0x174).
2. The **compare list** is built from the local player's equipped items (creature
   G+0x8006d0), in the order of §8.1.7, adding an equipped item when its **type byte equals
   the shown item's type byte** and it is not excluded:
   * weapons (+0xaa8, +0x990) and rings (+0xcd8, +0xbc0) are tested as **pairs**: if the
     shown item **is** either slot of a pair, **neither** slot of that pair is listed;
   * every other slot (pet +0x1020, then +0x878, +0x530, +0x760, +0x648, +0x418 in the
     §8.1.7 order) is skipped only when it is the shown item itself.
   Empty slots have type 0 and never match a real item.
3. `equippedpreview` (G+0x8008ec) is made **visible exactly when the compare list is not
   empty and the `preview` node is visible**, hidden otherwise (0x48f14d…0x48f17e).

So the inventory (and every other tooltip source, including the world item under the
target) shows the comparison whenever the player has something of the same type equipped
and is not hovering that very item. Examples: hovering a bag sword with a sword equipped →
panel with the equipped sword; hovering the equipped sword → no panel (its pair partner is
excluded too, so a second equipped sword is not compared); hovering a ring in the bag with
two rings equipped → both rings listed; hovering a potion → nothing equipped matches, no
panel.

**What is drawn**: the panel node is the 250 × 300 `equippedpreview` box (`gui.plx`
template; black, alpha and rim as the template says) at the place given in §8.1.7; the
PreviewWidget draw (client 0x4d6af0…0x4d6bd5) then writes "Currently equipped" and one
text block per listed item (scale 1, wrap 230, name and class lines on; no model, stars or
price for them) at the offsets of §8.1.7, measured from the **tooltip's** top-left, 130 px
apart. With the window source the box (mouse + (−235, 20)) sits behind that text. *As
read*, with the world source the box is placed **above** the tooltip (W − 320, H − 575)
while the text is still drawn to the tooltip's **left** (from X − 240), so text and box do
not line up.

---

## 4. Corrections to earlier specs (summary)

| Spec | Statement | Correct reading |
|---|---|---|
| `alpha-path-search.md` §0.1 | +0x1478 *unknown* | the RandomInteraction target triple; that static never side-blocks (§1.6) |
| `alpha-path-search.md` §4.1 step 3 | the search does not consult +0x1468 | ignored statics are skipped by the search (§1.2) and the physics (§2.2) |
| `alpha-path-search.md` §4.3 | on static = a non-passable static under the lowered box | any static, any kind and state (§1.3) |
| `alpha-path-search.md` §4.4 Statics bullet | partly decoded | §1.4–§1.7 |
| `alpha-physics-camera.md` §4.5 | voxel objects within 64 blocks | airships only; none exist in the shipped Alpha (§2.4); object base offset always 0 *as read* |
| `alpha-ui-screens.md` §8.1.12 I-3 | no code toggles `equippedpreview` | visible iff compare list non-empty and tooltip visible (§3) |

---

## 5. Remaining open points

* **OP2-1** Who, if anyone, inserts into the airship map G+0x2f0 (only the dead kind-4
  constructor was found). *Game check*: with the TEST character in a town, at sea and in a
  dungeon, read the map's size at G+0x2f0 from memory (mod DLL); expect 0 everywhere.
* **OP2-2** The voxel-lookup routine 0x4a2870 (bounds, which voxels count as solid) and the
  exact transform order of §2.4 step 1. Only matters if airships are added; no game check
  possible without one.
* **OP2-3** Low statics do not block the planner (§1.8). *Game check*: offline, stand
  behind a garden fence from a hostile and log its path cells (obj +0x1460) — expect cells
  through the fence and the creature pushing into it; repeat behind a market stand —
  expect a path around it.
* **OP2-4** The index-cell coverage of §1.1 (statics listed outside the node's index cells
  are unseen). *Game check*: same log with a market stand whose centre lies across an
  8-block cell border from the chaser.
* **OP2-5** The approach test of §2.2 item 2. *Game check*: with the logger, walk the TEST
  character into a closed door, then (teleport) start overlapped with a fence and walk
  out; log contact flag 0x40 — expect 0x40 only when moving toward the object.
* **OP2-6** The world-source misalignment of §3. *Game check*: capture the screen while
  aiming at a dropped sword with a sword equipped; compare box and text positions.
