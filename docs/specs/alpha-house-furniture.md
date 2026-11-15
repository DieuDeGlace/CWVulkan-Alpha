Status: FINAL / STABLE / HELD

# Alpha house furniture: room clearing and the support check

Native behaviour of the room-clearing step and the furniture support check of the Cube
World **Alpha** modular-house builder, the facts behind steps 14 and 15 of
`alpha-worldgen-structures.md` §5.5.4 (pointer only). Evidence: `Cube.exe` (SHA-256
`84a7a132…30bf`), the town builder 0x5af170 (step 12 floor placement around
0x5b69d8..0x5b6d09; step 14 at 0x5b7218..0x5b75c4; step 15 at 0x5b75ca..0x5b9746), the
prefab placer 0x5f0ce0, the wall-piece helper 0x5bf7c0 and the block helpers named below.
Addresses are citations. O is the house origin, (a, b, z) a cell of the house layout
(cell records read through 0x598930), and positions are 64-bit fixed point with 65536
units per block.

## 1. Summary

Step 14 turns the blocks of every role-1 storey cell, from w = 0 (the floor level
O.z + 7z) up to w = 5, into protected air, but only blocks whose type byte lacks bit
0x40. Step 15 keeps a furniture static only if the block at O.z + 7z under it is solid.
The floor survives step 14 because the placer writes the house modules with type byte
6 | 0x40 = 0x46, and the support predicate masks the flag bits away, so it sees type 6,
which is solid. Inside the room box, only blocks that the placer did not write (terrain,
trees, plain air, older unflagged blocks) become protected air.

## 2. The placer's writes for house modules (0x5f0ce0)

The house module placements inspected (the carpet at 0x5b6c11, the floor models at
0x5b6d09, and further module placements at 0x5b6ef2, 0x5b6fe5, 0x5b70d0, 0x5b71bd) call the placer
with the model, the target position, a rotation, type **6**, mode **1**, the zone, a
fill switch of **0**, a colour argument and a null record list. In that configuration:

* **Model voxels** are written with type byte T | 0x40 = **0x46** (0x5f49ef..0x5f4a79).
* **Existing flagged solids are kept.** Before writing a voxel the placer reads the world
  block there; if that block has bit 0x40 set and its type (byte & 0x1F) is neither 0 nor
  2, the voxel is skipped (0x5f1587..0x5f1699). A carpet placed before the floor model
  therefore stays where the two overlap.
* **Air writes** use type byte 0x40 (for example 0x5f1bff, 0x5f1de9, 0x5f248b, 0x5f2757,
  0x5f2bb5, 0x5f33ab, 0x5f48f6) or 0xC0 (0x5f157b); both carry bit 0x40.
* The placer has other configurations not used by these calls: a non-zero fill switch
  writes unflagged type-6 blocks downward beneath the model (0x5f106e..0x5f12ef), and
  mode 8 has a write with type byte 2 and no flag (0x5f49be).

## 3. Step 14 (0x5b7240..0x5b75c4)

For each cell (a, b, z) in layout order whose role byte is 1:

* For u = 0 … 13 (outer), v = 0 … 13, w = 5 down to 0 (inner), the block at
  (O.x + 13a + u, O.y + 13b + v, O.z + 7z + w) is read. The 14 × 14 box overlaps the
  neighbouring cells by one column on the +x and +y sides.
* **Flag test** (0x4348c0): bit 0x40 of the type byte, and nothing else.
  * Bit set: the block is left as it is. This covers every block the placer wrote in the
    configuration of §2 (0x46 solids and 0x40 / 0xC0 air).
  * Bit clear: the block is rewritten through the world block writer 0x44e7a0 with its
    own three colour bytes and type byte exactly **0x40** (built by 0x40ee40): type 0
    (air) with the flag, so it is no longer solid.
* The point records that follow in the same loop (0x5b742f..0x5b7565) are outside this
  document.

Step 14 thus removes anything unflagged that reaches into the room volume (ground,
rock, vegetation, older unflagged blocks) and marks that volume as protected air; it
does not remove the house's own placer-written floor, carpet or walls.

## 4. The solid predicate (0x43b480)

A block is **solid** when (type byte & 0x1F) is neither 0 nor 2. Bits 0x20, 0x40 and 0x80
are ignored:

| Type byte | & 0x1F | Solid? | Source in this context |
|---|---|---|---|
| 0x46 | 6 | yes | placer module voxel (floor, carpet, wall) |
| 0x40 | 0 | no | step-14 rewrite; placer air write |
| 0xC0 | 0 | no | placer air write |
| 0x42 | 2 | no | — |
| other values with & 0x1F ∉ {0, 2} | — | yes | unflagged blocks (rewritten by step 14 inside the box) |

Protected air is not solid. Furniture survives because the floor block is never
cleared, not because 0x40 counts as solid.

## 5. Step 15, the support check (0x5b75ca..0x5b9746)

Step 15 does nothing for a cell unless the house record's type (+0x18) is 1 (0x5b7630).
Furniture is then considered only for a role-1 cell whose cell (a, b, z + 1) has a stairs
byte (record +0x03) of 0 (0x5b780d..0x5b7835).

* **Floor level.** Zf = O.z + 7z + 1, formed as (the step's running 7z) + 1 + O.z, for
  example at 0x5b7919 and 0x5b92be. Every furniture static in this step (helper-W wall
  pieces, beds, tables, stools, counters) gets z = Zf · 65536. The wall-piece helper
  0x5bf7c0 changes only x or y of the position it is given, never z.
* **Support check**, after each static (for example 0x5b794f..0x5b79d0 and
  0x5b9309..0x5b9398):
  1. Take the static's position. Replace z by z − 32768 (0x42f5a0 with the double 0.5 at
     0x745de8), giving Zf · 65536 − 32768.
  2. Convert x, y and z to block coordinates with 0x42f860: a negative value first has
     65536 subtracted, then the value is divided by 65536 with truncation. This equals
     floor division except at exact negative multiples of 65536, where it gives one less.
     For z the result is **Zf − 1 = O.z + 7z**; for x and y it is the block containing the
     static's x and y (for example +1.5 gives +1, +4 gives +4).
  3. Read that world block. If it is solid by §4, the static is inserted with 0x5a0e00
     into the zone's static list (zone+0x0c, through a local set at 0x5b2dd0). Either
     way the temporary static is then destroyed with 0x593e80. Any draws for the static
     (such as a rotation draw) have already been made.
  The range contains 21 insertion sites, each preceded by the same 0x42f5a0, 0x42f860
  and 0x43b480 sequence.
* **The floor block.** Step 12 places the floor model F, or the stairs-floor model FS when
  the cell's stairs byte is non-zero (for F, optionally preceded by a carpet), at
  Z0 = O.z + 7z + 1 − sz, where sz is the model's z size (model +0x4c). The model's top
  voxel row is therefore at block **O.z + 7z**, the block the check reads. It has type
  byte 0x46; where a carpet was placed first, the carpet's flagged blocks are kept (§2).
  Step 14 leaves it in place and it tests solid.
* **Height.** The floor's top block occupies [O.z + 7z, O.z + 7z + 1), so its top surface is
  at Zf. Every furniture static stands with its base on that surface. A bed (scale
  vector z = 1) occupies Zf … Zf + 1.
* **Lantern.** Step 15 also adds a lantern (plant kind 13 to the zone's plant list,
  zone+0x04) for role-1 cells with a + b odd, before the stairs-above test; its z is
  O.z + 7z + 5.3 and it is not subject to the support check.

### When a piece is discarded

A static is dropped only where the floor model has no solid voxel in the block under its
(x, y). The helper-W shifts are bounded: the chest moves ±0.5 block, the stool by a random
0 … 1 block, the desktop ±0.2 and the cupboard ±0.3 block, all along x or y.

## 6. Worked example: one bedroom

O = (1000, 2000, 60) and cell (a, b, z) = (1, 0, 1), with role 1, and the cell (1, 0, 2)
above having stairs 0. The floor F is taken as 13 × 13 × 1; only sz enters the height
formula, and the top row lands at the same z for any sz. The room type is assumed to
be the bedroom of the structures spec.

* **Floor placement.** Z0 = 60 + 7 + 1 − 1 = 67. X0 = O.x + 7 + 13a − ⌊13/2⌋ = 1014 and
  Y0 = O.y + 7 + 13b − ⌊13/2⌋ = 2001, so the slab covers x 1014 … 1026, y 2001 … 2013 at
  z = 67, type byte 0x46.
* **Clearing.** The box is x 1013 … 1026, y 2000 … 2013, z 72 down to 67. At z = 67 the
  floor blocks are flagged and stay 0x46. Column x = 1013 belongs to the neighbouring
  cell's floor (O.x + 7 + 0 − 6 = 1001 … 1013) when that cell has one; being placer-written
  it is also kept. An unflagged block in the box, such as terrain at z = 70, becomes its
  own colour with type byte 0x40.
* **Floor level.** Zf = 60 + 7 + 1 = 68; the check reads z = ⌊68 − 0.5⌋ = 67.
* **The four beds** (kind 19, bed.cub, scale vector (2, 3, 1), one rotation draw each, in
  this order):

| # | Static position (blocks) | Block checked | Type byte | & 0x1F | Inserted? |
|---|---|---|---|---|---|
| 1 | (1017, 2004, 68) | (1017, 2004, 67) | 0x46 | 6 | yes |
| 2 | (1023, 2004, 68) | (1023, 2004, 67) | 0x46 | 6 | yes |
| 3 | (1017, 2010, 68) | (1017, 2010, 67) | 0x46 | 6 | yes |
| 4 | (1023, 2010, 68) | (1023, 2010, 67) | 0x46 | 6 | yes |

(x = O.x + 13a + 4 or + 10; y = O.y + 13b + 4 or + 10.) All four rest on the floor's top
surface at height 68 and are inserted into zone+0x0c.

A static at (1017.5, 2001.5, 68) would be checked against block (1017, 2001, 67), the
floor's edge row, and inserted.

If the block at z = 67 under a static is not solid (type & 0x1F of 0 or 2, including a
0x40 block), the check fails and the static is not inserted.

## 7. Relation to `alpha-worldgen-structures.md` §5.5.4

* Step 14: the test is bit 0x40 of the type byte only (0x4348c0). The house module
  placements write that bit on every block they write (§2), so the floor, carpet and
  walls are not cleared. The range u, v = 0 … 13, w = 5 … 0 from O.z + 7z holds.
* Step 15: "solid" means (type byte & 0x1F) ∉ {0, 2} (0x43b480). The checked block is
  the block containing the static's x and y at z = O.z + 7z, the floor's top row. Statics
  sit at z = Zf = O.z + 7z + 1. The rule applies to every inserted piece in step 15,
  including the helper-W results.

## 8. Open points

1. Which model voxels (colour codes) select the placer's 0xC0 air write and its other
   special paths.
2. The actual plan sizes of the floor models; the 13 × 13 assumption of §6 and the
   claim that the floor covers every furniture x and y (+1.5 … +12.5) were not read from
   the models.
3. Whether house module placements outside the inspected call sites (for example those
   in 0x5b5000..0x5b5f2d) use the same type, mode and fill arguments.
4. The room-type selection that leads to the bedroom layout in §6.

---

## Re-certification (2026-11-14)

Checked against `Cube.exe` (SHA-256 84a7a132…30bf): the town builder's steps 12, 14 and
15, the placer 0x5f0ce0, the wall-piece helper 0x5bf7c0, and the helpers 0x4348c0,
0x43b480, 0x40ee40, 0x44e7a0, 0x42f5a0, 0x42f860, 0x598930 and the model size getters;
furniture model names were resolved through table B and the registry 0x4ee230.

**Confirmed.** Step-14 loop ranges, order and box coordinates; the flag test is bit 0x40
only; the rewrite keeps the colour and writes type byte 0x40; the solid predicate; the
stairs-above condition; Zf = O.z + 7z + 1 for every furniture static; the 0.5-block
z offset and the support-block read; the floor height formula Z0 = O.z + 7z + 1 − sz;
house module placements write type 6 | 0x40 = 0x46; flagged solid blocks are not
overwritten by later placements; helper 0x5bf7c0 never changes z; the lantern height
5.3; bed kind, scale vector, offsets +4/+10 and per-bed rotation draw; the worked-example
arithmetic.

**Corrected.**
* Insertion sites: old "26 inserts"; new 21 insertion sites in 0x5b75ca..0x5b9746.
* Block conversion 0x42f860: old "subtracts 0xFFFF before dividing" (exact floor
  division); new: subtracts 65536 for negative values, which is floor division except at
  exact negative multiples of 65536.
* Placer flag claim: old "every block the prefab placer writes has that bit"; new: true
  for the house module configuration (type 6, mode 1, fill switch 0); other
  configurations write unflagged blocks (downward fill, mode 8). No world-generation
  result of the house steps changes.
* Discard wording: old "otherwise discard it (it is destroyed with 0x593e80)"; new: the
  temporary is destroyed in both cases, after insertion when the check passes.
* Added that the placer skips voxels where a flagged solid already exists (basis for the
  carpet statement), the helper-W shift magnitudes, the lantern's kind, list and
  condition, the insertion list's source (zone+0x0c via the local at 0x5b2dd0), and the
  house-type gate: step 15 runs only for house records of type 1.
* Second example range for the support check: old 0x5b84f6..0x5b8579; new citation
  0x5b9309..0x5b9398 (the bed sequence that was read).

**Removed.** The "contrast, the failure mode" paragraph describing what a wrong
implementation would do and how to fix it; the framing of §1 as a question about a
literal reading; the reference to a "§6.2" of another document (replaced by the placer
facts in §2); the "where it comes from" attribution of 0x42 to flagged water; the
"believed" stairwell-opening case (not checked).

**Unresolved.** Open points 1–4 of §8.
