Status: FINAL / STABLE / HELD

# Alpha open points, batch 8: the draws of a dungeon cell

Clean-room behavioural specification of native Cube World Alpha behaviour. It covers these points:

| Point | Companion pointer | Section here |
|---|---|---|
| Every draw of an interior cell, in order | `alpha-worldgen-structures.md` §1.10.6 | §1 |
| The wall sides, and what differs on +y | structures §1.10.6 step 7.7 | §2 |
| Torch routine and wall-furniture routine | structures §1.10.2 items 4, 5 | §3 |
| Whether inserting a static or a sprite draws, refuses or merges | structures §1.10.6 | §4 |
| The window step | structures §1.10.6 step 3 | §5 |

Companion names are pointers only; their text was not re-read for this document.

**Summary.**

1. **On the +y side the torch and the furniture are alternatives.** If the +y side's torch draw is
   odd, the side ends after the torch step: no furniture draw, no position draw, no furniture and no
   guard. The sides −x, +x and −y do both, one after the other (§2).
2. A 10 × 10 × 1 floor box costs **103** draws (3 table draws + 100 block draws); 303 is the cost of a 10 × 10 × 3 box (§1).
3. Inserting a static, sprite, creature, ground item or marker draws nothing and cannot be refused (§4).

Evidence: the Alpha client image `Cube.exe` (image base 0x400000) and server image `Server.exe`
(image base 0x400000). All addresses below are RVAs in the named image. The client dungeon-cell
builder starts at Cube.exe RVA 0x1CC510; the cell pass described here is the part of it at
RVA 0x1CEEB3..0x1D5F3A together with the routines it calls. The +y branch was compared with
Server.exe, whose builder starts at RVA 0x100300: the same two exits exist there (RVA 0x106C8C,
RVA 0x106CC5). Addresses are citations only.

Conventions. Symbols as in the structures spec §0 and §1: cell (a, b, c); X, Y, Z the block base of
the cell (cell index × 10 plus the zone offset); v the builder's style argument; k the kind byte of
the cell; flag the builder's last argument; lv the level. A cell is **solid** when its kind byte is 0,
1 or 2 (Cube.exe RVA 0x1EEA40); a cell read outside the grid yields kind 1, i.e. solid
(RVA 0x1C46F0). The cell's byte +1 holds bit 0 the stair flag, bit 1 the floor-patch flag and bit 2
the mark flag. "Draw" is one call of the C library `rand` through the import slot at Cube.exe
RVA 0x2FC468 (MSVCR110 `rand`). r is a position draw taken as d & 3.

---

## 1. Every draw of a cell, in order

The pass walks a (outer), b (middle), c (inner), each from 0 to the grid's extent on that axis in
the logical view (extents swap with odd rotation, RVA 0x1F9E10 and 0x1F9E30). Cells of kind other
than 3 or 4 are skipped with no draw. Cell and neighbour reads go through the logical-view accessor
(RVA 0x1C46F0). Steps are numbered as in the structures spec §1.10.6.

| Step | When it runs | Draws |
|---|---|---|
| 1 Exit marker | k = 4 | 0. A kind-5 marker is added to the zone's marker list; after the carve a temporary static record is built and destroyed (RVA 0x1D0B7A), with no effect |
| 2 Carve | v in 0..5 | 0 |
| 3 Windows | v ∈ {0, 1, 3}, k ≠ 4, stair flag clear | **4**, always, one per side (§5) |
| 4 Stairs | v in 0..5, stair flag set | 693 for each of the four neighbour tests that holds; 0 otherwise |
| 5 | v = 4 and k = 4 | next cell |
| 6 | kind(a, b, c − 1) = 3 | go to step 8 |
| 7.1 Floor | v in 0..5 | **103** (3 + 100) |
| 7.2 Carpet | v ∈ {0, 3}, floor-patch flag set | 0 |
| 7.3 | k = 4: go to step 9. Stair flag set: go to step 8 | 0 |
| 7.4 Trap | flag = 0 | 1 (d % 40). If it is 0: 2 more (rotation d & 3, then d % 4000 stored at record +0x34), a kind-7 static is inserted, then go to step 8. flag ≠ 0: no draw at all |
| 7.5 | n0 = number of statics in the zone | 0 |
| 7.6 Ceiling | solid(kind(a, b, c + 1)) | see below |
| 7.7 Walls | always when step 7.6 was reached | see §2 |
| 7.8 Loot | each static added since n0 | as structures §1.10.6 step 7.8 |
| 8 Arches | k ≠ 4, kind(a, b, c + 1) ≠ 3, an arch template is present, and the world block (X, Y, Z + 10) is solid (RVA 0x1D369D) | per side whose neighbour kind ≠ 3, one call of the template-stamping routine; its draw count is unresolved (§6) |
| 8 Chandelier | k ≠ 4, kind(a, b, c + 1) ≠ 3, v = 3 (runs even when the arch conditions fail) | 1 (d % 10); if 0 a kind-56 sprite is added; no other draw |
| 9 Boss | mark flag set | 1 (kind) + 4 (rarity) + item generator + 1 (n) |

**Stairs, step 4.** Each of the four tests compares only neighbour kinds (no draws). When one
holds, eleven boxes are filled with widths 11 down to 1, each 10 long and 1 high; a box of volume V
costs 3 + V draws, so the eleven cost 33 + 660 = 693 (RVA 0x1D1270..0x1D1763).

**Box fill (RVA 0x1DF600).** A box draws a 23-entry table in which only entries 0, 8 and 16 draw
(d % 20 each), then one draw per block. Hence 3 + width × length × height.

**Ceiling, step 7.6.** Skipped entirely if the cell above is not solid: no draw. Otherwise:

| v | Draws when it runs |
|---|---|
| 0, 3, 5 | 1 (d % 3), value has no effect |
| 4 | 1; if d % 3 = 0, 4 more for the liana (angle, size, then y offset d % 5, then x offset d % 5), a kind-55 sprite |
| 2 | as v = 4, then the four cobweb sides |
| 1 | 1 (no liana, whatever the value), then the four cobweb sides |

Each cobweb side (order −x, +x, +y, −y) is 1 draw (d % 3); if it is 0 and the neighbour at level c
is solid, 3 more in the order size, along-wall position d % 5, kind 57 + (d & 1). The order is the
same on all four sides.

**Order inside the boss step (RVA 0x1D3AC3..0x1D3D40).** Kind (draw modulo the length of the boss
kind list); rarity routine (RVA 0x1F8530: always 4 draws; its force argument, set here, only
replaces the result afterwards, RVA 0x1F8574, capped at 4); item generator (RVA 0x1F7A60);
byte flags; +0x109C receives a new 0x14-byte object built with the value 20.0 (RVA
0x1D3C30..0x1D3C65), no draw; a kind-6 marker; n = d & 3; n further items appended with no draws.
The boss record gets +0x28 = 1 and +0x08 = 150.0; its byte +0x30 is not written in this pass.

---

## 2. The wall sides

The order is always **−x, +x, −y, +y** (neighbours a − 1, a + 1, b − 1, b + 1). Each of the first
three sides is:

1. side draw; stop unless d % 3 = 0 and the neighbour at level c is solid;
2. torch draw; if odd: r (the along-wall coordinate, offset 3 + r), then the wall-block test, then,
   if the block is solid, the torch routine of §3 with its own draws, and the torch sprite is added;
3. **in every case**: furniture draw; if d & 7 ≠ 0: r, then the furniture routine of §3, whose
   static is inserted; if d & 7 = 0: a guard record is built, then r, then the inventory routine
   (RVA 0x1C9F90), then the guard is appended.

The **+y side differs in step 3 only**: step 3 runs **only if the torch draw was even**.

If the +y torch draw is odd, then after r and the wall test the side ends, whether or not the torch
was placed. Cube.exe: even branches to the furniture at RVA 0x1D2DAB → RVA 0x1D2EDA; a non-solid wall
block leaves the side at RVA 0x1D2E9C; after the torch the side is left at RVA 0x1D2ED5. The other
three sides join their furniture step in both cases (RVA 0x1D24A4, 0x1D2809, 0x1D2B6E). Server.exe
has the same two exits (RVA 0x106C8C, 0x106CC5). *As read*: this looks like an oversight in the
original, but both images have it.

Draw counts of one side, with T the draws of the torch routine and F of the furniture routine:

| Case | −x, +x, −y | +y |
|---|---|---|
| Side test fails | 1 | 1 |
| Torch even, furniture | 1 + 1 + 1 + 1 + F | the same |
| Torch even, guard | 1 + 1 + 1 + 1 + inventory | the same |
| Torch odd, wall solid, furniture | 1 + 1 + 1 + T + 1 + 1 + F | **1 + 1 + 1 + T** |
| Torch odd, wall solid, guard | 1 + 1 + 1 + T + 1 + 1 + inventory | **1 + 1 + 1 + T** |
| Torch odd, wall not solid, furniture | 1 + 1 + 1 + 1 + 1 + F | **1 + 1 + 1** |
| Torch odd, wall not solid, guard | 1 + 1 + 1 + 1 + 1 + inventory | **1 + 1 + 1** |

For the guard, r is its only draw before the inventory, on all four sides. The guard record gets
+0x28 = 6, type +0x2C = 143, +0x34 = lv, +0xF58 = 10.0 and +0x08 = 150.0.

The furniture routine takes s only as a number; s = 0 draws exactly what the other values draw.
The difference is entirely in the side sequence.

The s passed: −x 1, +x 3, −y 2, +y 0 (RVA 0x1D24C3, 0x1D2828, 0x1D2B8D, 0x1D2EF9). The torch
rotation passed is −x 270, +x 90, −y 0, +y 180 degrees.

---

## 3. Torch and furniture

**Torch (RVA 0x1F8960).** The style passed is the builder's v (RVA 0x1D2478). Jump table at
RVA 0x1F8AB8 on style t:

| t | Draws | Result kind |
|---|---|---|
| 0 | 1 | even: 50, raised 2 blocks. Odd: lamp (54, tinted) |
| 1, 2 | 1 | d % 3: 0 → 50, 1 → 49, 2 → 52 with tint |
| 3 | 0 | 48 |
| 4 | 1, or 2 if odd | odd: 44 + (second d & 3). Even: lamp (54) |
| 5 | 0 | lamp (54) |

So for v = 0, T = 1.

**Furniture (RVA 0x1F6E20).** The style passed is the zone's byte +0x79 (read at RVA 0x1D24B8 and
the matching places on the other sides), not the builder's v.

| First draw d % 50 | Second | Third | F |
|---|---|---|---|
| 0 | none | none | 1: chest, kind 10, rotation (s + 2) & 3, offset ½ block |
| ≠ 0 | d % 3 = 0, shelf | kind variant d % 3 (base 32, 38, 41 for style 3, 4, 5, otherwise 35) | 3 |
| ≠ 0 | d % 3 = 1, table (kind 12; 13 for style 4, 14 for style 5) | offset 1 to 2 blocks | 3 |
| ≠ 0 | d % 3 = 2, stool (kind 16; 15 for style 4, 17 for style 5) | offset 0 to 1 block | 3 |

No branch depends on s except the direction of the offset (jump tables RVA 0x1F73C8 for the chest,
0x1F73D8 for the stool, 0x1F73E8 for the table): s = 0 y −, s = 1 x +, s = 2 y +, s = 3 x −. The
shelf has no offset.

---

## 4. Inserting a static or a sprite

**Nothing draws and nothing is refused.** No call path from these routines reaches the `rand`
import.

| Routine | What it does |
|---|---|
| RVA 0x1A0E00 (statics, 0x188 bytes each) | Append to the vector. If the argument lies inside the vector it remembers the index first, grows by one if full, then copies. No test on kind or position. |
| RVA 0x0862E0 (sprites) | Append a node to the list. The size check only guards against overflow of the count. |
| RVA 0x26ADD0 (creatures), 0x04D590 (ground items), 0x1F4BA0 (markers) | Plain appends. |
| RVA 0x18EA10, 0x193E80 (static copy, destroy), 0x18E9F0, 0x18EAA0, 0x1ADB20 (constructors) | No draws. |

There is no overlap or collision test and no merge. Two statics can occupy the same place.

Besides the builder's own draws, the routines called in the pass that contain draws are:
RVA 0x1DF600 (box fill), 0x1F6E20 (furniture), 0x1F8960 (torch), 0x1C9F90 (inventory),
0x1F8530 (rarity), 0x1F7A60 (item generator), 0x1F6D50, and 0x1F0CE0 (template stamping, whose
draws depend on the template it is given).

---

## 5. The window step

Read at RVA 0x1D0B9A..0x1D126A:

* Side order −y, +y, −x, +x, fixed.
* The draw comes **first** on every side, before any test, so it is always 4 draws.
* The tests are: draw odd, then neighbour kind = 0, then the test block not solid. They take no
  draws. A window static (kind 4) and a carve follow when all hold.
* v, k = 4 and the stair flag are tested once, before the first side. Stair cells make no window
  draws.
* The test block is read from the world at the moment of the window.

---

## 6. What stayed open

* **Arch draws.** Each arch side calls the template-stamping routine (RVA 0x1F0CE0) with rotation
  −x 0, +x 2, −y 3, +y 1 (pushed at RVA 0x1D3737, 0x1D37E2, 0x1D388D, 0x1D3938) and type argument 70.
  That routine contains many draw sites, gated by the template's contents and its arguments.
  Whether the arch template reaches any of them is not established here.
* **+y, torch odd, wall not solid.** The code is unambiguous (RVA 0x1D2E9C), but no observation of
  this case was examined.
* **v ≠ 0.** Ceiling, cobweb, chandelier and the torch styles other than t = 0 are read from the
  instructions only.
* **Chests and generator loot.** Step 7.8 and the generators RVA 0x1F7A60 and 0x1F6D50 stay outside
  this document (structures §8, question 2).
* **The furniture style.** Furniture reads the zone's byte +0x79. The builder's v comes from a
  location entry: an entry of type 3 passes its next byte as v with flag 0 (RVA 0x1E84A9); an entry
  of type 5 passes v = 3 with flag 1 (RVA 0x1E84E3). Whether the zone byte equals v in either case
  was not traced. It changes the kinds only, not the draws.
* **Grid extents.** The extents of the cell loop are read from the grid object; their values were
  not re-derived here.
* **Wall and window block tests.** The block-read rules (for example below a column's bottom) of
  the test routines were not traced.
* **The generator inside `rand`.** It lives in MSVCR110, outside the examined images.

---

## 7. Native facts relevant to companion specs

| Companion pointer | Native fact |
|---|---|
| structures §1.10.6 step 7.7 | The torch-then-furniture-or-guard sequence holds for −x, +x and −y. On +y, the furniture or guard runs only if the torch draw was even (§2). |
| structures §1.10.6 step 7.1 and §1.11 | The floor box is 10 × 10 × 1: **103** draws. A 10 × 10 × 3 box costs 303. |
| structures §1.10.6 step 8 | Arch rotation per side: −x 0, +x 2, −y 3, +y 1. |
| structures §1.10.6 step 9 | +0x109C receives a new 0x14-byte object with the value 20.0. No draws. |
| structures §1.10.2 items 4, 5 | Torch and furniture as in §3. The rarity routine is always 4 draws. |

---

## Re-certification (2026-11-14)

Re-checked against Cube.exe and Server.exe image bytes (MSVC dumpbin disassembly and raw reads of
jump tables and constants). All addresses converted from virtual addresses to RVAs (image base
0x400000 in both images).

**Confirmed.** The `rand` import slot; cell-kind gating (3, 4 only); exit marker draws nothing;
window step (side order −y, +y, −x, +x, draw first, test order, gating once before the first
side); stairs 693 per holding test (eleven boxes of widths 11..1); floor 103 and box formula
3 + volume; carpet no draws; trap 1 draw, then rotation and d % 4000 at +0x34, flag ≠ 0 no draw;
ceiling table by v and liana order; cobweb side order −x, +x, +y, −y and the size, position, kind
order; wall side order −x, +x, −y, +y; the +y alternative rule and all its branch addresses in
both images; the s values; the side draw-count table; torch jump table and results; furniture
draw counts, kinds and offset directions; rarity always 4 draws with the force argument applied
after; boss order and the 0x14-byte object holding 20.0; insert routines reach no draw; chandelier
1 draw only for v = 3; arch rotations −x 0, +x 2, −y 3, +y 1; builder location-entry arguments.

**Corrected.**
1. Addresses restated as RVAs (old: virtual addresses such as 0x5d2dab; new: RVA 0x1D2DAB, etc.).
2. Arch rotation for −x: old citation 0x5d3732 points at another argument; the rotation is pushed
   at RVA 0x1D3737.
3. Arch conditions: old listed only k ≠ 4 and kind above ≠ 3; new adds a template present, the
   world block (X, Y, Z + 10) solid, and a per-side neighbour kind ≠ 3 test (no draws involved).
4. Arch draw count: old "0, in mode 0"; new: unresolved (see §6). This is a downgrade of a claim
   that affects random-draw order in world generation if arches draw.
5. Draw-site list: old "0x5f0ce0 in the modes where markers are read"; new: the template-stamping
   routine's draws depend on the template it is given.
6. Cell loop bounds: old fixed 0..21; new: the grid's logical extents, value unresolved.
7. Furniture kinds: old named the kinds only loosely; new lists kinds per style and the chest
   rotation and offset.
8. Evidence range: old "0x5ceeb3..0x5d5f3a" presented as the whole listing; new names the builder
   start (RVA 0x1CC510) and the examined slice.
No reversal of a native-behaviour rule; item 4 withdraws a confirmation. None of the corrections
changes the confirmed draw order of the rules in §1, §2, §3 and §5.

**Removed.** The worked example and recovered generator states of cells (2,4,9)..(2,4,11), the
whole-dungeon comparison and its draw-total tables (they relied on a recording, not on image
evidence); every statement about a port or replacement, its drift and injected draws; replay and
checking advice; the claim that the window position and rotation table of the companion spec is
right in every entry (companion not opened); recording-based rows of the old corrections table;
the claim that no step depends on rotation or mirror except through the logical view (not
exhaustively traced; restated as the accessor fact); the generator formula of `rand` (outside the
examined images).

**Unresolved.** Listed in §6: arch draws; the +y torch-odd, wall-not-solid case (read, never
observed); v ≠ 0 paths (read only); chests and generator loot; the furniture style byte against v;
grid extents; block-read rules of the wall and window tests; the `rand` generator itself.
