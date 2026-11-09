# Alpha house furniture: room clearing and the support check

Corrections and clarifications to `alpha-worldgen-structures.md` §5.5.4 steps 14 and 15
(modular houses). Written from the client assembly of the town builder 0x5af170
(step 14: 0x5b7218..0x5b75c4; step 15: 0x5b75ca..0x5b9746) and its helpers. Addresses are
citations only.

## 1. The question

Step 14 turns the blocks of every storey cell from w = 0 (the floor level O.z + 7z) up to
w = 5 into protected air. Step 15 keeps a furniture static only if the block under it,
at O.z + 7z, is solid. Read literally, the floor block has just been cleared, so every bed,
table, stool, counter and wall piece would be thrown away. Alpha's houses do have
furniture, so that reading is wrong.

**Answer.** The step-14 range and the step-15 heights are correct as written. The missing
fact is that step 14 skips every block whose type byte has bit 0x40 set, and every block
the prefab placer writes has that bit set. The placer's default write uses type byte
T | 0x40 (§6.2), so house modules are written with 0x46. The floor and carpet slabs at
O.z + 7z are therefore left alone. The support predicate masks out the flag bits, so it
sees these blocks as type 6, which is solid. Only blocks that did not come from the placer
(terrain, trees, plain air, and so on) inside the room box become protected air.

## 2. Step 14, restated (0x5b7240..0x5b75c4)

For a, b, z (view order, as before), for a cell with role 1:

* For u = 0..13 (outer), v = 0..13, w = 5 down to 0 (inner), read the block at
  (O.x + 13a + u, O.y + 13b + v, O.z + 7z + w). The 14 × 14 box overlaps the neighbouring
  cell by one column on the +x and +y sides.
* **Flag test** (0x4348c0): this tests bit 0x40 of the type byte and nothing else.
  * Bit set: the block is left untouched. This covers every placer-written block: the
    module solids (T | 0x40, for example 0x46), the placer's "inside" air (0xC0) and the
    marker air (0x40).
  * Bit clear: the block is rewritten (block writer 0x44e7a0) with **its own colour** and
    type byte exactly **0x40**. That is type 0 (air) with the protect flag, so the block
    stops being solid. The colour bytes are kept but not shown.
* The points (spawn, entrance, counter) follow as in §5.5.4 step 14. They are unchanged.

In effect, step 14 strips anything natural that sticks into the room (ground, rock,
vegetation blocks, older non-flagged blocks) and protects the empty room volume. It never
removes the house's own floor, carpet, walls or furniture-bearing slabs.

## 3. The solid predicate used by the support check (0x43b480)

A block is **solid** when (type byte & 0x1F) is neither 0 nor 2. The high bits 0x20, 0x40
and 0x80 are ignored, so:

| Type byte | & 0x1F | Solid? | Where it comes from |
|---|---|---|---|
| 0x46 | 6 | yes | placer module block (floor, carpet, wall) |
| 0x40 | 0 | no | step-14 cleared block, placer marker air |
| 0xC0 | 0 | no | placer inside-air |
| 0x42 | 2 | no | (water with the flag) |
| 0x01, 0x06, … | ≠ 0, 2 | yes | unflagged terrain or structure blocks (none survive inside the box) |

So protected air does **not** count as solid. The furniture survives because the floor
block was never cleared, not because 0x40 counts as solid.

## 4. Step 15.2, restated (support check)

Furniture is built only for a role-1 cell whose cell above (z + 1) has stairs = 0. This is
checked at 0x5b780d..0x5b7835.

* **Floor level.** Zf = O.z + 7z + 1, read as ([0xa3a4] = 7z) + 1 + O.z, for example at
  0x5b7919 and 0x5b92be. Every static in this step (wall pieces from helper W, beds, the
  table, stools, counters, the "other" table) is created with **z = Zf · 65536 exactly**.
  Helper W's chest, stool, desktop and cupboard shifts move only x or y, never z.
* **Support check**, the same code after every static (for example 0x5b794f..0x5b79d0 and
  0x5b84f6..0x5b8579):
  1. Take the static's position (x, y, z) in fixed point. Replace z with
     z − trunc(0.5 · 65536) = Zf · 65536 − 32768 (0x42f5a0 with the double 0.5 at
     0x745de8).
  2. Convert all three to block coordinates with **floor division** by 65536 (0x42f860
     subtracts 0xFFFF from negative values before dividing). This reads the live world
     block. For z this gives **Zf − 1 = O.z + 7z**. For x and y it is the block containing
     the static's own x and y (for example +1.5 gives block +1, +4 gives block +4).
  3. If that block is solid by §3, insert the static with 0x5a0e00 into zone + 0x0c.
     Otherwise discard it (it is destroyed with 0x593e80). Draws have already been made
     either way.
* **The floor block.** Step 12 places the floor F (or FS, or a carpet first and then F)
  with Z0 = O.z + 7z + 1 − sz, so the slab's top voxel row is at block **O.z + 7z**. This
  is the block the check reads. It holds placer type 0x46 (carpet blocks also have the
  flag, so F skips them and the carpet stays on top). Step 14 leaves it alone, and it tests
  solid.
* **Height of the furniture.** The floor's top block occupies [O.z + 7z, O.z + 7z + 1), so
  its top surface is at height O.z + 7z + 1 = Zf. Every furniture static stands with its
  base **exactly on the floor's top surface**, with no offset. A bed (scale z = 1) fills
  height Zf .. Zf + 1. The lantern of step 15.1 is the only object in step 15 that is not
  at Zf (it is at O.z + 7z + 5.3).
* **Same rule for everything.** Wall pieces (all helper-W results: chest, bedtable,
  shelves, bench, stool, desktop, cupboard, vase), beds, tables, stools and counters all
  use the identical z and the identical check. 26 inserts in 0x5b75ca..0x5b9746 each sit
  behind the same 0x42f860 + 0x43b480 test.

### When a piece really is discarded

A static is dropped only where the floor slab has no voxel under its (x, y) block. This
applies to *believed* cases only:

* A storey with stairs = 1 uses the floor-stairs model FS. Its stairwell opening is not
  solid at O.z + 7z, so a bed or wall piece over the hole is dropped. Furniture is still
  attempted when the cell above has stairs = 0.
* A floor model narrower than the furniture spot. The floor modules are believed to be
  13 × 13 in plan (the "13 × 13 × 7 modules" of §5.5.4), placed at X0 = O.x + 13a + 1
  (7 − ⌊13/2⌋). They would cover u = 1..13, which contains every furniture x and y used
  (+1.5 … +12.5). This is *believed*; the model sizes were not read in this pass.
* The chest's half-block shift and helper W's stool, desktop and cupboard shifts stay
  within 1 block of the wall line, so they stay over the floor.

## 5. Worked example: one bedroom

Take O = (1000, 2000, 60) and the cell (a, b, z) = (1, 0, 1). Its role is 1, room = 1
(bedroom), and the cell (1, 0, 2) above has stairs = 0. Type 1 (wood, set 2287..2299). The
floor F is assumed 13 × 13 × 1 (*believed*; only sz affects the formula, and the top row
lands at the same z whatever sz is).

* **Floor placement (step 12).** Z0 = 60 + 7 + 1 − 1 = 67. The slab's top row is at
  **z = 67**, columns x 1014..1026, y 2001..2013, type byte 0x46.
* **Clearing (step 14).** The box is x 1013..1026, y 2000..2013, z 72 down to **67**
  (w = 5..0). At z = 67 the floor blocks have 0x40 set and stay as they are (type 0x46).
  Column x = 1013 is the neighbour cell's floor or wall, which is also placer-written and
  untouched. Any unflagged block in the box, such as terrain poking up at z = 70, becomes
  (its colour, 0x40). The walls placed at the cell centre (step 10) carry 0x46 and are
  untouched.
* **Floor level.** Zf = 60 + 7 + 1 = **68**. The check reads block z = ⌊68 − 0.5⌋ = **67**.
* **The four beds** (kind 19, scale (2, 3, 1), one rotation draw each, in this order):

| # | Static position (blocks) | Block checked | Its type byte | & 0x1F | Inserted? |
|---|---|---|---|---|---|
| 1 | (1017, 2004, 68) | (1017, 2004, 67) | 0x46 (floor) | 6 | **yes** |
| 2 | (1023, 2004, 68) | (1023, 2004, 67) | 0x46 | 6 | **yes** |
| 3 | (1017, 2010, 68) | (1017, 2010, 67) | 0x46 | 6 | **yes** |
| 4 | (1023, 2010, 68) | (1023, 2010, 67) | 0x46 | 6 | **yes** |

(x = O.x + 13a + 4 or + 10 = 1017 or 1023; y = O.y + 13b + 4 or + 10 = 2004 or 2010.) All
four beds rest on the floor's top surface at height 68 and are added to zone + 0x0c. Step 16
then appends each one to the house's beds list.

The wall pieces of the same cell (step 15.2.1) are checked the same way. For example, the
−y piece at (1017.5, 2001.5, 68) checks block (1017, 2001, 67), which is the floor's edge
row, so it is inserted.

**Contrast, the failure mode.** An implementation that writes placer blocks without the
0x40 flag (or whose step 14 clears regardless of the flag) erases z = 67 to 0x40. All four
checks then read type 0 and every bed is discarded. The same thing happens if the support
test treats a 0x40 block as solid "to compensate": the floor holes would stay visible. The
fix is the flag test of §2 together with the T | 0x40 placer write of §6.2.

## 6. Summary of edits to §5.5.4

* Step 14: "if its type byte lacks 0x40" is exact (0x4348c0 tests bit 0x40 only). Add that
  all placer output carries the bit, so the house's own floor, carpet and walls are never
  cleared. The range (u, v = 0..13; w = 5..0 from O.z + 7z) is correct.
* Step 15.2: "solid" means (type byte & 0x1F) ∉ {0, 2} (0x43b480). The checked block is
  (⌊x⌋, ⌊y⌋, O.z + 7z), the floor slab's top row. Statics sit at z = Zf = O.z + 7z + 1, on
  the floor's top surface. The rule applies to every piece in step 15.2, including the
  helper-W results.
