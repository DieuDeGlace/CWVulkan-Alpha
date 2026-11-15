Status: FINAL / STABLE / HELD

# Alpha world generation: vegetation, plants and ground items

Behavioural contract for the living things the Cube World **Alpha** zone generator places
on its terrain: **plant sprites** (small voxel models chosen from a fixed kind table),
**ground items** (category-11 item records lying on grass and snow), **street lights** in
towns, and **voxel trees** written into the terrain. Evidence: `Cube.exe` (SHA-256
`84a7a132…30bf`), chiefly the zone generator 0x5e4850 and the start-up routine 0x459c40
with the model registry 0x4ee230. The [terrain spec](alpha-worldgen-terrain.md) and the
[random spec](alpha-worldgen-random.md) are pointers for the column layout, the climate
quantities and the random generator.

The plant-sprite and ground-item passes (§3, §4) and the tree placement (§5) are
specified to the draw; the tree builder itself (0x5df960) is outside this document.

---

## 0. Conventions

As in the random spec §0. "Draw" is one `rand` call; draws happen in the order listed and
only when their branch is reached. Rotations are in degrees. Positions are 64-bit fixed
point with 65536 units per block. "Type" of a block is its type byte masked with 0x1F; a
block is **solid** when its type is neither 0 nor 2 (0x43b480). x and y are world block
coordinates of the column; zx, zy are the zone indices, so the zone covers
x = 256zx … 256zx + 255 and y = 256zy … 256zy + 255.

Climate inputs at a column (x, y), read from the column record when the column exists
and computed otherwise:

* **W(x, y)**, called warmth in the terrain spec: 0x5c8350 (column +0x08, else 0x5c4dd0);
* **M(x, y)**, called moisture: 0x5c8310 (column +0x04, else 0x5c4800);
* **Q(x, y)**, the town measure: 0x5989d0.

---

## 1. Model tables (start-up 0x459c40; registry 0x4ee230)

The registry gives every `.cub` model a numeric identifier. Two kind tables in the game
object map generator kinds to those identifiers: table A at G+0x800718 (filled at
0x462563..0x462bf4) and table B at G+0x800724 (filled at 0x462c2f..0x463491).

### 1.1 Plant sprite kinds (table A)

| Kind | Model | Kind | Model |
|---:|---|---:|---|
| 0 | flowers2.cub | 27 | desert-flower01.cub |
| 1 | flowers.cub | 28 | desert-flower02.cub |
| 2 | grass.cub | 29 | wheat.cub |
| 3 | grass2.cub | 30 | corn.cub |
| 4 | grass3.cub | 31 | water-lily01.cub |
| 5 | underwater-plant.cub | 32 | water-lily02.cub |
| 6 | coral.cub | 33 | inn-sign.cub |
| 7 | alga.cub | 34 | shop-sign.cub |
| 8 | lava-flower.cub | 35 | weapon-shop-sign.cub |
| 9 | lava-grass.cub | 36 | armor-shop-sign.cub |
| 10 | thorn-plant.cub | 37 | identifier-sign.cub |
| 11 | echinacea2.cub | 38 | smithy-sign.cub |
| 12 | leaf.cub | 39 | carpentersshop-sign.cub |
| 13 | lantern02.cub | 40 | weavingmill-sign.cub |
| 14 | torch.cub | 41 | ivy.cub |
| 15 | stone.cub | 42 | wall-roses-red.cub |
| 16 | stone2.cub | 43 | wall-roses-white.cub |
| 17 | christmas-tree.cub | 44–47 | inca-art1.cub … inca-art4.cub |
| 19 | tendril.cub | 48 | crest1.cub |
| 20 | tulips-colorful.cub | 49 | wall-skull.cub |
| 21 | cornflower.cub | 50 | castle-chain.cub |
| 22 | reed.cub | 51–54 | torch-read.cub, torch-green.cub, torch-blue.cub, torch-yellow.cub |
| 23 | pumpkin-leaves.cub | 55 | liana.cub |
| 24 | pineapple-leaves.cub | 56 | chandelier.cub |
| 25 | sunflower.cub | 57, 58 | cobwebs.cub, cobwebs2.cub |
| 26 | bean-tendril.cub | | |

Table A has no entry for kind 18.

### 1.2 Static object kinds (table B, entries used here)

Kind 50 = street-light02.cub, kind 51 = street-light01.cub, kind 65 = campfire.cub. Table B
holds 75 entries in all; the remaining ones are outside this document (see also
alpha-rendering.md §10.1 for the light kinds).

### 1.3 Records

* **Plant sprite** (initialised by 0x58e9f0, appended to the list at zone+0x04 by
  0x4862e0): kind at +0x00 (32-bit); position at +0x08, +0x10, +0x18 (x, y, z, 64-bit
  fixed point); scale at +0x20 (float); rotation at +0x24 (float, degrees); a scale vector
  at +0x2c, +0x30, +0x34 (floats, initialised to (1, 1, 1) and not changed by this pass);
  a flags word at +0x38, initialised to **2**. Every plant of §3 is placed at
  (x + 0.5, y + 0.5) in the horizontal plane, the centre of the column, at height zt
  (§2) unless stated otherwise.
* **Static object** (initialised by 0x58eaa0; inserted into zone+0x0c by 0x5a0e00; the
  temporary is then destroyed by 0x593e80): kind at +0x00, position at +0x08 (three
  64-bit fixed-point values), rotation index (0..3) at +0x20, scale vector at +0x24
  (three floats).
* **Ground item** (a 0x148-byte record initialised by 0x44a970 and appended to the vector
  at zone+0x30 by 0x44d590): category byte +0x00, subtype byte +0x01, count (16-bit)
  +0x10, position +0x118 (three 64-bit fixed-point values), rotation +0x130 (float), scale
  +0x134 (float, initialised to 0x3d924925 ≈ 0.0714286), flag byte +0x138.

---

## 2. The pass and its order (0x5ebc12..0x5ed2c1)

The zone generator walks every column of the zone with x in the outer loop and y in the
inner loop (loop control at 0x5ebc12..0x5ec5e8). For each column it visits the stored
blocks from the bottom, index i = 0 … count − 1 (the column's bottom z is at
column +0x10, its count at +0x1c):

* If i < count − 1 and block i + 1 is solid, nothing happens for this i.
* Otherwise **here** = type of block i, **above** = type of block i + 1 (0, air, when i is
  the last stored block), and **zt** = bottom + i + 1, the z of the cell above block i.
* If above = 0: here = 11 selects §3.1, here = 3 selects §3.2, here = 2 selects §3.3; any
  other here continues to §3.4.
* If above ≠ 0 (that is, above is water, type 2): only §3.4 applies.

Every exposed top surface in the column is therefore considered, from bottom to top.

---

## 3. Plant sprites

### 3.1 Street lights (here = 11, above = 0)

If Q(x, y) > 0.75 and (90y + x) mod 470 = 0: draw; continue only if draw & 15 = 0 and the
seven world cells (x, y, zt) … (x, y, zt + 6) are all non-solid. Then build a static
object: if W(x, y) > 0.8, kind **51** with scale vector (1, 1, 8), otherwise kind **50**
with scale vector (2, 2, 8); position exactly (x, y, zt) (the block corner, no half-block
offset); draw: rotation index = draw & 3. Insert it into zone+0x0c.

### 3.2 Reeds (here = 3, above = 0)

If W(x, y) > 0.2 and |N(f32(0.05x + 9843), f32(0.05y + 8437))| > 0.5, with the noise
arguments formed in float (x converted to float, multiplied by the float 0.05, the float
offset added) and then widened: draw; continue only if draw & 7 = 0. Plant kind **22**
(reed), scale 0.09 (bits 0x3db851ec), flags word set to exactly **4**; draw: rotation =
f32(draw) · 360 / 32767 in float.

### 3.3 Water lilies (here = 2, above = 0)

If W(x, y) > 0.2, zt > 0 and |N(f32(0.05x + 24234), f32(0.05y + 53565))| > 0.7 (float
arguments as in §3.2): draw; continue only if draw mod 10 = 0. Plant at height
zt − 0.1 (fixed point zt · 65536 − 6553, 0x42f5a0 with the double 0.1), scale 0.09, flags
word left at 2; draw: rotation = draw · 360 / 32767 (float); draw: kind = **31 + (draw & 1)**.

### 3.4 Ground cover (here ∈ {4, 9, 10, 12}, above = 0 or water)

Gate: |N(f32(0.05x + 9843), f32(0.05y + 8437))| > 0.6 (float arguments); then draw;
continue only if draw & 7 = 0. Then by zt:

* **Underwater** (zt ≤ −5): plant at zt, scale 0.1; draw: rotation = (draw & 3) · 90.
  nz = N(0.01x + 9843, 0.01y + 8437) with double arguments. If nz > 0: draw: kind =
  **5 + (draw & 1)**; kind 5 (underwater-plant) gets scale 0.075 and flags |= 4; kind 6
  (coral) keeps scale 0.1 and flags 2. If nz ≤ 0: kind **7** (alga), scale 0.1,
  flags |= 4. Finally draw: scale ← (f32(draw) / 32767 + 1) · scale in float. Nothing
  further happens for this i.
* **Shallow** (−5 < zt < 1): nothing is placed and nothing further happens for this i.
* For zt ≥ 1, by here:
  * **here = 12**: plant, scale 0.075; draw: rotation = (draw & 3) · 90. If
    N(0.01x + 9843, 0.01y + 8437) > 0 (double arguments): kind **9** (lava-grass),
    flags |= 4. Otherwise kind **10** (thorn-plant) and draw: scale =
    f32(draw · 0.02 / 32767 + 0.03) evaluated in double. The plant is appended; nothing
    further happens for this i.
  * **here = 10**: no plant; go to the ground-item step (§4).
  * **here ∈ {4, 9}**, by W = W(x, y) and M = M(x, y):
    * **W > 0.75, M ≤ 0.25**: draw; if draw mod 100 = 0: plant, scale 0.075; draw:
      rotation = (draw & 3) · 90; draw: kind = **27** if (draw & 1) ≠ 0, else **28**; the
      plant is appended for either here. Then the ground-item step.
    * **W > 0.75, M > 0.25**: plant, scale 0.075; draw: rotation = draw · 360 / 32767
      (float). nA = N(0.01x + 9843, 0.01y + 8437) (double arguments). If nA > 0: kind
      **4** if N(f32(0.01x + 34234), f32(0.01y + 234234)) > 0 (float arguments), else
      **3**. If nA ≤ 0: kind **12** if N(0.01x + 34234, 0.01y + 234234) ≤ 0.5 (double
      arguments), else **11**. Kind 3 gets flags |= 4 (kind 4 does not). Kind 11: draw:
      scale = f32(draw) · 0.05 / 32767 + 0.05 and flags |= 4. Kind 12: draw: scale =
      f32(draw) · 0.02 / 32767 + 0.1 and flags |= 4 (float). The plant is appended if
      here = 4 or the kind is 2 or 3. Then the ground-item step.
    * **W ≤ 0.75**: plant, scale 0.075; draw: rotation = (draw & 3) · 90. With
      nA = N(0.01x + 9843, 0.01y + 8437) (double), nF = the float-argument noise
      N(f32(0.01x + 34234), f32(0.01y + 234234)) and nD = the double-argument noise
      N(0.01x + 34234, 0.01y + 234234):
      * M > 0.5: if nA > 0, kind **12** if nF > 0, else **4**; if nA ≤ 0, kind **1** if
        nD ≤ 0, else **0**.
      * M ≤ 0.5: if nA > 0, kind **2** if nF > 0, else **3**; if nA ≤ 0, kind **1** if
        nD ≤ 0, else **0**.
      * Kinds 2, 3, 4 and 12 get flags |= 4; kind 12 additionally draws: scale =
        f32(draw) · 0.02 / 32767 + 0.1 (float). The plant is appended if here = 4 or the
        kind is 2, 3 or 4. Then the ground-item step.

The float-argument and double-argument forms of the 34234 / 234234 noise are distinct
evaluations; at large x and y their values differ.

---

## 4. Ground items (after §3.4, for here = 4 or 10, zt ≥ 1)

Only columns that passed the §3.4 gate (noise test and draw & 7 = 0) with zt ≥ 1 reach
this step. For here = 4 it follows the plant decision whether or not a plant was placed;
for here = 9 the step does nothing.

* **here = 4**: draw; continue only if draw mod 150 = 0. Item record at
  (x + 0.5, y + 0.5, zt), category **11**; draw: rotation = (f32(draw) / 32767) · 360
  (float); draw: if draw & 1 = 0, subtype **15** (scale left at its initial value);
  otherwise subtype **22** with scale 0.1. Count 1, flag byte 2; appended to zone+0x30.
* **here = 10**: draw; continue only if draw mod 300 = 0. Item record at
  (x + 0.5, y + 0.5, zt), category 11; draw: rotation as above; one further draw whose
  value is not used: subtype **24** with scale 0.1 in every case. Count 1, flag byte 2;
  appended to zone+0x30.

The meaning of the subtypes belongs to the item tables (see also alpha-rendering.md
§10.1 for a category-11 item that glows).

---

## 5. Voxel trees (grid pass, 0x5e9e7d..0x5ea7e1)

Trees are not models: the builder 0x5df960 (20099 bytes, outside this document) writes
trunk and crown voxels into the zone. The placement runs on a 14 × 14 grid of 18-block
cells, earlier in the generator than the plant pass. For i = 0 … 13 (outer, x) and
j = 0 … 13 (inner, y):

1. gx = 256zx + 18i and gy = 256zy + 8 + 18j. The **sample point** is (gx + 8, gy) =
   (256zx + 18i + 8, 256zy + 18j + 8).
2. **Oasis bonus** b: if the zone's location record has type 3 (location +0x18), let f be
   the value of 0x5f8e10 for the sample point (passed in fixed point); b = (1 − f)² when
   1 − f > 0, else 0. Without a type-3 location, b = 0.
3. mo = M(sample point).
4. Draw: crown size s = trunc(f32(draw mod 5) + (2·mo + 6) + 4b), float arithmetic in that
   grouping.
5. Draw: height t = trunc((f32(draw) · 8 / 32767 + (6b + 8)) · (0.5·mo + 1)), float.
6. Position x = gx + s = 256zx + 18i + s and y = gy − 8 + s = 256zy + 18j + s. Skip the cell
   if (f32(x) − hx)² + (f32(y) − hy)² < 400 in float, where (hx, hy) is the world's home
   position (W+0x8000f0, W+0x8000f4; 8396928 after initialisation, random spec §2.2).
7. With h = s / 2 truncated toward zero: x ← min(x, 256zx + 256 − h) and
   y ← min(y, 256zy + 256 − h).
8. Skip the cell if any entry of a list local to the zone generator (created at 0x5e6bb4
   and filled by earlier passes) lies within 40 blocks: for each entry the fixed-point
   differences in y and x are converted to float, multiplied by 1/65536 in float, squared
   and summed; a sum below 1600 skips.
9. **Tree kind** k: n = N(O294 + 0.001x, O298 + 0.001y) in double (O294, O298 from random
   spec §2.2). Draw: k = 5 if (draw & 1) ≠ 0, else 0. If n > 0.3: draw; if draw mod 10 ≠ 0,
   k = 1. If n ≤ 0.3: draw; if draw mod 10 = 0, k = 1. Then, in both cases, draw; if
   draw mod 10 = 0, k = 2.
10. wm = W(x, y). If wm > 0.8 and mo > 0.7: draw: k = 5 if (draw & 1) ≠ 0, else 4; draw: if
    draw & 3 = 0, k = 3.
11. With n2 = N(0.001x + 8473, 0.001y + 9438) (double):
    * wm < 0.3: k becomes 1, except when wm > 0.2, n2 > 0.6 and a draw gives
      draw mod 5 ≠ 0, in which case k becomes 2. (n2 and the draw are evaluated only when
      wm > 0.2, and the draw only when n2 > 0.6.)
    * 0.3 ≤ wm < 0.7: if n2 > 0.8: draw; if draw mod 5 ≠ 0, k = 2.
    * wm ≥ 0.7: no change.
    Then, if k = 1: when f32(s) > 0.5·f32(t), s ← trunc(0.5·f32(t)); then s ← max(s, 1).
    If k = 2: draw; t ← t + draw mod (t / 2), with t / 2 truncated.
12. Keep the crown inside the zone: if x + s > 256zx + 256, x ← 256zx + 256 − s; if
    y + s > 256zy + 256, y ← 256zy + 256 − s.
13. **Density** (0x5a5bc0): p0 = sat(1.2 · N(O1f4 + 0.008x, O1f8 + 0.008y)) (double
    arguments, float result); m′ = M(x, y) at the current position;
    p = min(0.2·m′·m′ + 0.02 + p0, 1), float; then p ← p + b. Draw; skip the cell if
    f32(draw) / 32767 > p.
14. **Ground.** Skip if the column (x, y) does not exist. Starting at the column's highest
    stored block z = bottom + count − 1, move down while the world block at (x, y, z) is
    air or water (type 0 or 2). Skip if the resulting z < 0. If W(x, y) > 0.8 and
    M(x, y) < 0.2, k ← 3. The tree is built only if the block at z has type 4 or 10, or
    type 9 with z + 1 ≤ 2. Then call the builder with (x, y, z + 1, size, t, k, zone), where
    size is s, or 1 when s < 1 (the substitute is a local holding 1, written at 0x5e4cd0).

Every draw of this pass precedes every draw of the plant pass (§3) in the zone's
sequence; other passes between 0x5ea7e7 and 0x5ebc12 may draw in between.

---

## 6. Other placements in the same zone

* **Location type 12**: when the zone containing the location's centre is this zone, one
  draw whose value is not used, then the builder 0x5df960 with size 80, height 80 and kind
  6 at (256zx + 128, 256zy + 128, bottom + count) of that column, i.e. one cell above its
  highest stored block (call at 0x5e7231). This happens earlier in the generator than
  the tree grid.
* Ravine spawns: see the terrain spec §5.5 (not re-checked here).
* Reeds (§3.2) appear on every type-3 surface with air above.

---

## 7. Observable state

* After a zone is generated its plant sprites are in the list at zone+0x04, its static
  objects (including street lights) in zone+0x0c and its ground items in the vector at
  zone+0x30.
* Lilies require zt > 0, so they lie on water blocks at z ≥ 0.

---

## 8. Open questions

1. Meaning of the plant flags values 2 (default), 4 (reeds) and 6 (default with bit 4
   added), and how the renderer uses the scale and scale-vector fields.
2. Tree builder 0x5df960: its own draws, per-kind shapes and voxel colours (outside this
   document; see alpha-worldgen-trees.md).
3. Whether the ground-item subtypes 15, 22 and 24 are herbs, mushrooms or similar.
4. Model scale convention (.cub voxel size × scale), shared with the clouds spec.
5. Contents of the zone-generator object list consulted in §5 step 8: which earlier passes
   append to it and which positions they store.
6. The value of 0x5f8e10 (the normalised distance used for the oasis bonus) is defined
   in the regions spec and was not re-checked here.

---

## Re-certification (2026-11-14)

Checked against `Cube.exe` (SHA-256 84a7a132…30bf): start-up tables, registry names,
record initialisers, and the zone generator's tree pass, plant pass and giant-tree site,
with all constants read from the image.

**Confirmed.** Every table-A kind and model name (kind 18 absent); table-B kinds 50, 51,
65; the column walk order and the solid-above skip; street-light conditions, kinds,
scale vectors and draws; reed, lily and gate noise offsets, multipliers, thresholds and
float argument formation; underwater, fire-ground, hot-dry and hot-moist decisions,
scales and draw order; ground-item probabilities, subtypes, count and flag; the tree
grid, s and t formulas, position, home check, clamps, 40-block object check, tree-kind
draws, size and height rules, crown clamp, the density routine's noise and constants,
the ground search and ground types; the giant tree's size, height and kind.

**Corrected.**
* Temperate ground cover (affects world generation): old "M > 0.5, nA > 0 → 2 if nF ≤ 0
  else 3; M ≤ 0.5, nA > 0 → 12 if nF > 0 else 4"; new "M > 0.5 → 12 if nF > 0 else 4;
  M ≤ 0.5 → 2 if nF > 0 else 3". This reverses the earlier claim on both branches.
* Plant position (affects world generation): old "the plant stands at the block corner
  (x, y)"; new: plants and ground items are at (x + 0.5, y + 0.5); street lights are at
  the corner (x, y).
* Lily height: old "zt adjusted by 0x42f5a0" (open question 2); new: zt − 0.1.
* Plant flags: old "flag 4" set as a bit; new: the word starts at 2, reeds set it to
  exactly 4, other flagged kinds OR in 4 (giving 6).
* Density (affects world generation): old "0.2·mo²" with the sample-point moisture; new:
  moisture re-read at the tree's current position.
* Step 14 (affects world generation): added the k = 3 override (W > 0.8, M < 0.2) after
  the ground search, the column-exists requirement and the search start (highest stored
  block, not "top − 1"); builder argument order (x, y, z + 1, size, t, k, zone) and the
  size substitute 1.
* Step 11 wording: for wm < 0.3 the kind always becomes 1 or 2 (the old "otherwise k = 1"
  was ambiguous).
* Step 4 grouping stated: f32(draw mod 5) + (2·mo + 6) + 4b.
* Plant list offset: old zone+0x24; new zone+0x04 (items at zone+0x30, statics at
  zone+0x0c).
* Underwater kind 6 keeps flags 2; hot-moist kind 4 has no flag added (made explicit).
* Snow items: the subtype draw is a real draw whose value is ignored (made explicit).
* Address range of the pass extended to 0x5ed2c1 (the land and item branches lie past
  0x5ec5e8); table address ranges stated.

**Removed.** The in-line "Correction" history paragraph and its transcribed instruction
text in §5 step 6 (the resulting fact is kept); the "checking against the running game"
advice (§7, reduced to observable state); the unverified claim that the object list is
filled by the house builders 0x5cbe00/0x5cb600 and the see-also note attributing them
to rock formations; the "quarter turns" interpretation of the rotation index; the
"raised lakes, never the sea" interpretation; the cross-spec precedence note naming
alpha-open-points-12.md.

**Unresolved.** Open questions 1–6 of §8; the meaning of Q, W and M beyond their
routines; the contents of table B beyond kinds 50, 51, 65.
