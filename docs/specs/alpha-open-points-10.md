# Alpha open points, batch 10: dungeon styles 1 to 5

Clean-room behavioural specification. It audits everything in the dungeon builder 0x5cc510 that
depends on the variant v, for v = 1 to 5.

Written from the per-instruction listing of `Cube.exe`: the whole builder (the decompiler timed out
on it, so its output does not exist), and the routines it calls that depend on v or that the
symptoms point to. Where the listing has a hole (0x5e6464..0x5e699f), the bytes of the binary were
disassembled directly. Addresses are citations only. No code was copied.

The checks used game outputs only: the recordings
`references/alpha-recordings/seed-41-dungeon-style1` to `-style5` and `seed-41-dungeon`. Nothing
under `AlphaVulkan/src` was read.

Confidence labels: **certain** = read instruction by instruction; **confirmed** = certain and also
measured against a recording; **likely** = the best explanation, not proved; **to check** = needs
your build.

---

## The short version

| # | Symptom | Cause | Wrong in | Confidence |
|---|---|---|---|---|
| 1 | v = 1, 2, 4 drift from early on | The rough carve's noise on the y faces has x and y exchanged. Torch wall tests on the −y and +y sides then read the wrong block and the draw count changes | structures §1.10.2 item 2 | **confirmed** |
| 2 | v = 4: nearly all rock colours wrong, types 6/13 swap | The pyramid stairs have **11** boxes per run, not 10: 233 more draws per stair column, 8,854 before the interior pass | structures §1.10.4 step 3 | **certain** |
| 3 | v = 5: type 0/6 swaps, spawn group differs | The two long corridors, the central shaft, the pillar and the spiral all use a height 12 lower than the box. 220 fewer draws before the room spawns | structures §1.10.7 | **confirmed** |
| 4 | Type 59 level | The level of the point: 3 inside the location's radius, 1 outside. Rule of open-points-5 §1 at the block corner | open-points-5 §1.2 omits this caller | **confirmed**, 140 of 140 |
| 5 | v = 3: G one unit off, both signs | The box filler and moss are right as specified. The first noise's arguments must be rounded to float. Leaving them unrounded gives this symptom | wording of structures §1.10.2 item 1 | formula **confirmed**; port's cause **likely** |
| 6 | v = 5: B one unit off | Not dungeon blocks. Terrain rock in a fire zone, where a mix of two equal values truncates low in single precision | not a dungeon matter | **likely** |

Everything else that depends on v was checked and stands (§7). Rules 1 to 3 are all in the building
pass; none of the setup, room, link, corridor, entrance, orientation, raised floor or fit steps is
wrong for any v.

The set v = 1, 2, 4 is the set with `rough` carves, so your pointer was right for v = 1 and 2. v = 4
has rule 1 as well, but rule 2 hits it first and much harder.

**A warning for anyone reading the recordings:** columns in `.avzd` run **x fastest**, so the column
index is 256·y + x. Reading it as 256·x + y hides rule 1: both noise forms then look like chance.

---

## 1. The rough carve (corrects structures §1.10.2 item 2)

The face order, the condition "the cell touches that face", the threshold and the written block
stand. The noise of the y faces does not. With (X′, Y′, Z′) the carved cell:

| Order | Face | Neighbour written | Hole if strictly greater than 0.5 | Read at |
|---|---|---|---|---|
| 1 | i = 0 | (X′ − 1, Y′, Z′) | N(0.1·Y′ + 13·X′, 0.2·Z′) | 0x59ac35 |
| 2 | j = 0 | (X′, Y′ − 1, Z′) | **N(0.1·X′ + 13·Y′, 0.2·Z′)** | 0x59acc0 |
| 3 | i = sx − 1 | (X′ + 1, Y′, Z′) | N(0.1·Y′ + 13·X′, 0.2·Z′) | 0x59ad50 |
| 4 | j = sy − 1 | (X′, Y′ + 1, Z′) | **N(0.1·X′ + 13·Y′, 0.2·Z′)** | 0x59ade3 |

So the coordinate along the wall gets 0.1 and the coordinate across the wall gets the integer 13.
The argument is built in double: (double)coordinate · 0.1, plus (double)(32-bit integer coordinate ·
13). The second argument is (double)Z′ · 0.2 (constant 0x71db58). The result is compared as a float
(`N > 0.5`, 0x745da0). The written block is (0, 0, 0, 0xC0). Faces are tried in that order after
each carved cell, inside the same k loop. No draws.

**Why this moves the stream.** The torch wall test of §1.10.6 step 7.7 reads the block one beyond the
torch: for −y that is (X + 3 + r, Y − 1, Z + 2), exactly a block the rough carve can open. A wrong
hole flips the test. On −x, +x and −y a flip changes the T draws of the torch. On +y it changes much
more, since the furniture only runs on an even torch draw and the torch's own draws depend on the
test. Every later window, trap, guard and spawn then reads the wrong numbers, and the zone's trees
after it. That fits "first spawns agree in type, positions differ": the types come from short lists,
and the guards are placed by draws.

The same holes also show in the block data of v = 1, 2, 4.

**Measured.** Wall blocks one step outside a carved cell whose neighbour cell is closed, counted by
prediction and by whether the block is a recorded hole:

| Style | Faces | Formula | Predicted hole, is a hole | Predicted hole, no hole |
|---|---|---|---|---|
| 1 | y | this document | 1,540 | 51 |
| 1 | y | structures §1.10.2 | 407 | 2,055 |
| 2 | y | this document | 2,324 | 140 |
| 2 | y | structures §1.10.2 | 279 | 1,173 |
| 4 | y | this document | 2,321 | 253 |
| 4 | y | structures §1.10.2 | 319 | 1,376 |
| 4 | x | §1 (unchanged) | 2,128 | 164 |

Predicted-hole hit rates are 90 to 97% for the new form and 17 to 19%, the base rate, for the old
one. The misses were not traced; a corridor passing behind the wall is the likely cause.

Test vectors from style 4: carved block (8356810, 8360756, 64), wall block (8356810, 8360755, 64) is
a recorded hole. N(0.1X + 13Y, 0.2Z) = 0.5329713821411133, so it opens. The old form gives
−0.3842719793319702, so it would not. Likewise (8356811, 8360756, 64): 0.5129570364952087 against
0.4192010164260864.

---

## 2. The v = 4 stairs (corrects structures §1.10.4 step 3)

Each of the three runs has **11 boxes, t = 0..10**, not 10. The counter starts at −1 and runs while
below 10 (0x5d0018..0x5d0086; the same form at all twelve loops).

| Where | Run 1 | Run 2 | Run 3 | Code |
|---|---|---|---|---|
| a = 10, b < 10, \|b − 10\| > 1 | box(X − 2, Y − 1 + t, H, (14, 1, t + 2)) | box(X − 2, Y − 1 + t, H, (2, 1, t + 4)) | box(X + 10, Y − 1 + t, H, (2, 1, t + 4)) | 0x5d0012 |
| a = 10, b > 11 | y = Y + 10 − t, otherwise the same | the same | the same | 0x5d0183 |
| b = 10, a < 10, \|a − 10\| > 1 | box(X − 1 + t, Y − 2, H, (1, 14, t + 2)) | box(X − 1 + t, Y − 2, H, (1, 2, t + 4)) | box(X − 1 + t, Y + 10, H, (1, 2, t + 4)) | 0x5d033e |
| b = 10, a > 11 | x = X + 10 − t, otherwise the same | the same | the same | 0x5d04cc |

All colour c2, rate g, flags 0. They run after step 2 and also when H − Bmin ≤ 0.

Draws per stair column: run 1 is 33 + 14·77 = 1,111; runs 2 and 3 are 33 + 2·99 = 231 each; total
**1,573**. The old reading gave 1,340. There are 38 stair columns (19 on each centre line), so the
stream is **8,854** draws longer by the end of the column loop.

The extra box is hard to see in the blocks: its row is covered by the next column's boxes and by
the core. Only its draws and its table values matter, so a cell-by-cell comparison will not show this
one directly. Only the colours do. **Certain** from the loop bounds; not replayed.

---

## 3. The v = 5 shaft (corrects structures §1.10.7)

The box stands at Zt. After it the code lowers the working height by 12 (0x5d43d7), and everything
after that uses the lowered value. With **Zt = base + 10ce + 38** and **Zc = Zt − 12 = base + 10ce +
26**, and **Ze = base + 10ce + 8** (§11.2; not the value in §1.10.7):

| Step | Call | Code |
|---|---|---|
| 1 | box(Xc − 2, Yc − 2, Zt, (14, 14, 14), c2, g, 0) | 0x5d43c3 |
| 2 | carve(Xc + 2, Yc − 32, **Zc**, (6, 74, 6), not rough) | 0x5d4413 |
| 3 | carve(Xc − 32, Yc + 2, **Zc**, (74, 6, 6), not rough) | 0x5d4460 |
| 4 | carve(Xc, Yc, Ze, (10, 10, **Zc − Ze + 6**), not rough) | 0x5d44b3 |
| 5 | pillar box(Xc + 4, Yc + 4, Ze − 30, (2, 2, n), c2, g, 0), **n = Zc − Ze + 30** | 0x5d4536 |
| 6 | spiral for u = 1 … n − 1 at z = Ze − 30 + u | 0x5d4548 |

So step 4 is always 24 high and n is always 48. The spiral's jump table
0x5d6018 was read entry by entry: **the same corners and sizes as v = 4**, so the *inferred* marks on
the v = 5 corridor offsets and spiral can go. v = 4 does not lower anything: its Zt = base + 10ce +
44, n = 66, and the rest of §1.10.7 for v = 4 stands except Ze.

Draws: pillar 3 + 4·48 = 195. Spiral: 24 odd steps at 8 and 23 even steps at 28 = 836. Total **1,031**,
against 1,251 under the old reading, so **220 fewer** before the room spawns. Steps 1 to 3 draw
only in step 1 (2,747).

**Measured**, seed 41 zone (32708, 32684), base + 10ce = 206: the corridor at block column (136, 105)
is open at z = 232..237, which is Zc..Zc + 5, not 244..249. The pillar at (136, 136) stops at z = 231
(Zc − 1; it starts at Ze − 30 = 184), and the shaft is open up to z = 237. Your 4,090 and 3,671 swapped blocks fit two corridor
systems of about 5,100 blocks each, one in the old place and one in the right place, less what the
centre shares.

---

## 4. The level of type 59 (corrects open-points-5 §1.2)

Type 59 here is the third ravine case of caves §7, and caves §7 is right: level = routine 0x59a9b0.
The call is 0x5e68f9, in a stretch the listing lost. Its arguments are the block corner **(x << 16,
y << 16)**, not the spawn's position with its +0.5. Then +0x34 = result, +0x28 = 1, +0x2c = 59
(0x5e68fe..0x5e6908).

Add this row to the caller table of open-points-5 §1.2, which lists four callers and misses it:

| Client site | Who | Position passed | Result goes to |
|---|---|---|---|
| 0x5e68f9 | ravine creature, type 59 (caves §7) | block corner of the column | spawn +0x34 |

The rule of open-points-5 §1.1 needs no change. Type 132 is the second ravine case with +0x34 = 1
fixed (0x5e66f4) and +0x28 = 6, which is why it stays 1. The flags 0x32 come from the finishing pass.

**Measured:** all 140 type 59 spawns of the six recordings match. In style 3 the record is the
location at (8363136, 8377216), type 14, r = 150, level 3. Six spawns inside are level 3, at distances
44 to 148; the rest are level 1, the nearest of them at 187.6. Styles 1 and 5 have spawns inside, but
their record's level is 1. Styles 2 and 4 have none inside (nearest 331 and 369), which is why even
the level 6 dungeon shows only level 1.

If yours gives 1 in that case, the likely cause is that the port writes 1 for this creature, or
that it asks before the region's record is filled. The record must be prepared by then.

---

## 5. Colours inside the dungeon: the formula stands (confirms structures §1.10.2 items 1)

**The palette needs no change.** c1 to c5 for every v were read draw by draw at 0x5ce520..0x5cee9b
and match §1.7 and §1.10.1, including the 8-bit wrap of the byte sums and the channel order. So does
the colour step of the box filler (0x5df6e8..0x5df771): integers C + t, converted to float, then
G = m·(120 − G) + G in single precision, each operation rounded to float, then clamp, then truncate.
R and B take no moss.

**Measured**, style 3, zone (32668, 32723), palette c2 = (200, 180, 212):

* Blocks with q ≤ 0, so m = 0: all 11,258 give G = 180 + t.
* For each of 14,386 columns, the blocks of the column were turned into an interval of the moisture
  mo. **No column was inconsistent.**
* The intervals are so narrow that they fix **mo = 0.97370 ± 0.00001 for the whole zone**. Style 5
  gives about 0.033.
* Predicting G from these rules with that mo: 2 mismatches in 46,312 sampled blocks.

So moisture and the formula are right when computed this way, and any error in a port is in the
arithmetic of q. Trying what a port might do differently, on the same 46,312 blocks:

| Variant | Mismatches | G too high / too low |
|---|---|---|
| As below | 2 | 0 / 2 |
| First noise's arguments not rounded to float | 2,772 (6.0%) | 1,421 / 1,351 |
| Second noise's arguments rounded to float | 6,684 (14.4%) | 3,467 / 3,217 |
| No intermediate rounding anywhere | 2,772 | 1,421 / 1,351 |

Both signs in equal numbers is your symptom. Over the zone's 370,000 c2 blocks the second row comes
to about 22,000, the same order as your 15,487. **Likely: the port does not round N's arguments in
the first noise.** The error scales with mo·|120 − g|, which explains why it stayed hidden: the
recorded v = 0 dungeon has c2 = (123, 124, 115), so g is close to 120 (mo about 0.60), and v = 5 has
mo about 0.033. The test vectors of §1.12 land on values where the rounding changes nothing.

The exact moss, to replace the shorthand of §1.10.2 item 1:

```
a = f32( f32( (float)Y · 0.2f ) + 534f )
b = f32( f32( (float)Z · 0.2f ) + 13f )
n1 = N( (double)a, (double)b )                 float result
n1 = f32( n1 · 0.1f )
n2 = N( (double)X · 0.05 + 4343.0, (double)Z · 0.1 + 84734.0 )    double arguments
q = f32( f32( f32( n1 + n2 ) · 0.7f ) + 0.2f )
m = f32( q · mo ), then 0 if negative, 1 if above 1
```

(Constants 0x6fcdbc, 0x71df60, 0x71df50, 0x71db48, 0x71de50, 0x71a048, 0x71def8, 0x6fcb7c, 0x6fd570.)

Test vectors that do separate the rounding (mo = 0.9737, computed with the rules above; the formula
produced 2 mismatches in 46,312 blocks):

| (X, Y, Z) | a | b | n1 before ·0.1 | n1 if not rounded | q |
|---|---|---|---|---|---|
| (8363141, 8377207, 163) | 1675975.375 | 45.60000228881836 | −0.01016006525605917 | −0.004184170626103878 | 0.4355536699295044 (m = 0.4240986108779907) |
| (8363173, 8377123, 170) | 1675958.625 | 47 | 0.38975927233695984 | 0.3397551476955414 | −0.05454765260219574 (m = 0) |

**Type 13** blocks go through the same colour step, so the same fix covers them.

---

## 6. The v = 5 colours: terrain, not dungeon

(58, 19, 9) is not a dungeon colour: c2 is (255, 210, 130) and c3 is (200, 180, 80). In the recording
those blocks have type 6 or 0x2c, no structure bit, and lie at the rim at z = 172 or so, in rock with G
from 10 to 27 and B of 10. That looks like the fire tint of terrain §6.6: B ← B(1 − F) + 10F gives
B = 10 at F = 1. The fire share of this zone was not measured.

Rock B cannot go below 10 by that formula. Yet the zone holds a few blocks with B = 9 and even 8, about
1 to 2% of those in this colour range, so they do not come from Rk but from one of the surface pass's
mixes (terrain §5.4, §5.5): trunc(c·(1 − k) + Rk·k) with c and Rk both 10. In single precision this
sum can land just under 10 and truncate to 9. A second such mix gives 8. The surface pass truncates
with float conversions in the range 0x5e4e00..0x5e6464 (9 sites, no double ones).

**Likely:** the port does these mixes in double, or rounds differently. Each product and each sum must
be rounded to float, then truncated. No dungeon change. Which mix was not traced.

---

## 7. Audit list: what depends on v and stands

Each row was read in the listing; "as spec" means the current text plus batches 7 and 8.

| Part | Addresses | v-dependent content | Result |
|---|---|---|---|
| Style setup | 0x5cc5da..0x5cc933, table 0x5d5f90 | K1, K2; the one draw of v = 1, 2 | as spec, §1.2 and §1.10 |
| Room parameters | 0x5cca08..0x5ccab8 | none | as spec |
| Selection, v = 2 | 0x5ccd29..0x5cd09a | room flags, 10 links, written in order | as spec |
| Selection, v = 4, 5 | 0x5cdb6e..0x5cdfc1 | room 14 present, quiet and fixed; 10 links | as spec |
| Selection, v = 1, 3 | 0x5cd0a4..0x5cdb58 | as batch 7 §1.2 | as spec |
| Room record | stack 0xffffcccc | four flag bytes, then six ints | as spec |
| Fixed reset | 0x5ce02a..0x5ce08c | (0,0,0), (0,0,1) | as spec |
| Rooms, mark, corridors | 0x5ce000..0x5ce3ed | none | as spec |
| Entrance | 0x5ce3f3..0x5ce4de | room by first link | as batch 7 §4 |
| Palette | 0x5ce4e1..0x5ce83e, table 0x5d5fa4 | v = 1, 2 and 4 re-roll; v = 5 fixed | as spec |
| Glow rate g | 0x6fcd9c, 0x71dce4, 0x71dce8 | 0.005, 0.0015, 0.0025 | as spec |
| Arch model | 0x5ce844..0x5cea59 | slot 2545 for v = 4, 5, else 2544 | as spec |
| Raised floors | 0x5ce887..0x5cea05 | no draws for v = 2; none at all for v = 4, 5 | as spec |
| Fit | 0x5cea5f..0x5cee13 | + 50 for v = 4, 5 | as spec |
| Last palette | 0x5cee19..0x5ceeb3, table 0x5d5fb8 | one extra draw for v = 0, 3 **only** | as spec |
| Roof, parapets, stepped roof | 0x5cf06a..0x5cfae2 | v = 2, v = 0, 1, 3, v = 3 | as spec; the +y parapet is no longer *inferred* (0x5cf6a4..0x5cf7fa) |
| Shell, foundation | 0x5cfae2, 0x5d0656 | none | as spec |
| Pyramid core, grass | 0x5cfc2a..0x5cffaf | v = 4 | as spec |
| Mound | 0x5d0705..0x5d08bf | v = 5 | as spec |
| Carve flag | 0x5d0adc..0x5d0b6a | rough for v = 1, 2, 4 | as spec |
| Windows | 0x5d0b9a..0x5d126a | v = 0, 1, 3 | as spec |
| Stairs | 0x5d1270..0x5d1763 | any v | as spec |
| Step 5, floor, carpet | 0x5d176f..0x5d1ada | v = 4 skip; carpet v = 0, 3 | as spec |
| Trap | 0x5d1af3..0x5d1c15 | flag only | as spec |
| Ceiling | 0x5d1c64..0x5d2328 | liana v = 2, 4; cobwebs v = 1, 2 | as spec and batch 8 §1; constants 0.04, 0.08, 20, 0.05, 10, 360 checked |
| Torch | 0x5f8960, table 0x5f8ab8 | t = 1, 2, 3, 4, 5 | as spec |
| Furniture | 0x5f6e20 | t = 3, 4, 5 | as spec |
| Loot kinds | 0x5d30ea..0x5d31f8 | kinds 10, 13, 12, 35, 36, 37 | as spec. t = 3, 4, 5 shelves and the t = 5 table have no loot |
| Arches, chandelier | 0x5d3611..0x5d3ab8 | v = 3 chandelier | as spec; +y rotation 1 is confirmed by batch 8 |
| Boss | 0x5d3ac3..0x5d3d40 | K1 | as spec |
| v = 4 shaft | 0x5d3db1..0x5d4aa9 | | as spec |
| Erosion | 0x5d4817..0x5d4aa3 | v = 1, 2 | as spec; the second bit test is "bit 0x80 clear" (0x44be30) |
| Room spawns | 0x5d4aa9..0x5d5ef2 | K1, K2 only | as spec, with §8 below |
| View transforms | 0x5fa3c0, 0x5fa440 | rot, mir | as spec |
| Solid | 0x5eea40 | | k ∈ {0, 1, 2}, as spec |

`zone + 0x79` is written at 0x5e6ba7 before the builder runs, and it is the same byte the builder
gets as v for E.kind = 3, so furniture's t = v in fresh worlds. The doubt in batch 8 §8 is closed for
that call.

---

## 8. Small additions to §1.10.9 (no draws)

The spec is silent on these; they do not change the stream.

* **+0x54:** ring and centre creatures: f32(θ / π · 180 + 90), in double, with θ the float angle
  (0x5d5884..0x5d58a9, 0x5d5cdf..0x5d5d04).
* **+0xf60:** ring and centre creatures: multiplied by 0.75 (0x6fcdc4); escorts: multiplied by 0.5
  (0x745da0). Done before the items.
* **+0x109c:** every room creature gets a 0x0c-byte behaviour object holding the 20.0 object, and the
  patrol objects (0x5d4e73.., 0x5d5937..).
* **+0x48, +0x4c:** the leader gets the value of 0x5c0120(zone coordinates, list size), used by the
  escort's follow object (0x5d53c8, 0x5d555c).

---

## 9. Corrections to earlier specs

| Spec | Statement | Correction |
|---|---|---|
| structures §1.10.2 item 2 | "the other three faces use the same constants and are taken to use the same formula, *inferred*" | Faces 2 and 4 exchange x and y (§1) |
| structures §1.10.2 item 1 | moss shorthand | Use the full form of §5; the shorthand hides which steps are float |
| structures §1.10.4 step 3 | k = 0..9, runs of 10 boxes | t = 0..10, runs of 11 boxes (§2) |
| structures §1.10.7 | v = 5 corridors and shaft at Zt, n = Zt − Ze + 30, positions *inferred* | Use Zc = Zt − 12 (§3); positions read, not inferred |
| structures §1.10.3 | +y parapet *inferred* | Read; the text stands |
| structures §1.11 | | Add: 1,573 per v = 4 stair column; pillar and spiral 1,427 for v = 4, 1,031 for v = 5 |
| structures §1.10.7 | Ze = base + 10(ce + 1) | Ze = base + 10ce + 8, both v (§11.2) |
| structures §1.10.6 step 7.7 | Torch wall test at the torch's height | Below z = 0 it reads one block lower (§11.1) |
| open-points-5 §1.2 | Four callers of 0x59a9b0 | Five, add 0x5e68f9 (§4) |
| open-points-8 §8 | Furniture style doubt | Closed for flag = 0 (§7) |

---

## 10. What stayed open

* **No whole replay of v = 1 to 5 was run.** Rules 1 and 3 are confirmed in the blocks, and rule 2
  from the code. Whether they account for **every** drift needs your build: after them, v = 1, 2, 4
  and 5 should match as v = 3 does. If one does not, the first differing spawn or floor table is the
  place to start.
* **v = 1, 2 rule 1 alone** is expected to fix them, because nothing else specific to them was found.
* **The +y torch case with the wall not solid** (batch 8 §8) will now actually occur, since v = 1, 2,
  4 open wall blocks. It is read and was never exercised before.
* **Item generators** 0x5f7a60 and 0x5f6d50 stay outside this batch. The chests and bosses of v = 1 to
  5 reach them with other levels, which v = 0 did not exercise.
* **Prefab 2545** (`temple-arc`) goes through the same mode 0 placement as slot 2544. Its blocks were
  not compared.
* **The cause in §5 and §6** is inferred from your symptom and a simulation; the port was not read.

---

## 11. Follow-up: walking the floors of style 2, and the spiral

### 11.1 The torch wall test below z = 0 (replaces the "+y reads Z + 1" finding)

**All four sides pass Z + 2 to the same read**, in every style. There is no +y special case. The
position is built with z = base + 2 + 10c on each side (0x5d237c, 0x5d26ed, 0x5d2a46, 0x5d2db1), and
the z argument of 0x42f860 is that element. **Certain.**

The difference is inside the read. 0x42f860 takes fx coordinates and does this to each axis
(0x42f870..0x42f8c3), then divides by 65536, truncating towards zero:

```
if value < 0: value -= 65536
block = value / 65536   (C division)
```

That is correct for a fraction but one too low for a **whole negative block**. So:

| fx z | Block read |
|---|---|
| (Z + 2) << 16, Z + 2 ≥ 0 | Z + 2 |
| (Z + 2) << 16, Z + 2 < 0 | **Z + 1** |

Exact rule for all four sides: wall block z = Z + 2 if Z + 2 ≥ 0, otherwise Z + 1. x and y are
unaffected, since world x and y are positive. Styles 1 and 3 never showed this because they stand
above z = 0; style 2's zone (32643, 32804) has base −150. Your "+y" cases and the two cells of (c) are
the same rule. If your port reads Z + 1 on +y only, or always, change it to this.

In this builder the four torch tests are the only calls of 0x42f860. Reads through 0x42f7e0 with
integer blocks (window test, arch test, fit, parapets) are not affected. The town builder (22 calls),
the zone generator (4) and others also call 0x42f860, so any of them passing a whole negative fx
coordinate has the same behaviour. A port should share one routine with this fix-up.

### 11.2 Ze and the spiral (corrects §3, structures §1.10.7)

The spiral has no index offset. The base height is the error:

**Ze = base + 10ce + 8, for v = 4 and v = 5** (0x5d3df0..0x5d3df8, 0x5d431f..0x5d4327: the code forms
ce + 4(ce + 1), then doubles it). The value base + 10(ce + 1) in §1.10.7 and in my §3 was wrong, and
§3 is corrected above. With the old value u comes out 2 too small, which is your (u + 2) & 7.

The exact form for both v, with **Zs** = Zt for v = 4 and Zc for v = 5:

```
n = Zs - Ze + 30                         v = 4: 66, v = 5: 48
box(Xc + 4, Yc + 4, Ze - 30, (2, 2, n), c2, g, 0)
if n > 1:
    for u = 1, 2, ... while u < n:       u = 1 .. n - 1
        z = Ze - 30 + u                  first z = Ze - 29, one step per block
        switch (u & 7): one box at height z, table of §1.10.7, c2, g, 0
```

Code: pillar 0x5d3ffb and 0x5d4536; start 0x5d400d and 0x5d4548; index 0x5d4020 and 0x5d4560; end
0x5d42c1 and 0x5d4801. The central carve (Xc, Yc, Ze) is also taller: 36 high for v = 4, 24 for v = 5.

Style 5, base + 10ce = 206: Ze = **214**, not 216. n = **48**, not 46. The pillar runs z = 184..231, the
spiral z = 185..231 with u = z − 184, and u runs to 47. That gives (Xc, Yc) at z = 192, 200, 208, 216,
224 and (Xc, Yc + 5) at 191, 199, … 231, the positions you saw. Boxes at 185 and 186 exist, which are
your missing lower levels: u = 1 is (Xc + 5, Yc), size (1, 5, 1), and u = 2 is (Xc + 6, Yc), size
(5, 5, 1).

Your pillar looked right because its three table values are unchanged, but the two missing layers
are 8 missing draws, and they offset all the spiral. In the recorded pillar colours, start 184 leaves
4 non-zero blocks in slots the table makes zero (spiral boxes overlap the pillar), and start 186
leaves 12.

Draws for pillar plus spiral: v = 4: 267 + 1,160 = 1,427. v = 5: 195 + 836 = 1,031. **Certain** from the
code; not replayed through to the room spawns.

### 11.3 The two style 2 stretches: nothing is wrong in furniture or loot

Furniture 0x5f6e20 was read on every path. No scale or loot rule depends on rotation: the loot loop
reads scale x and y from the record, unswapped (0x5d3218, 0x5d325a), and the record keeps the scale
the routine wrote. Shelf (2, 1, 1.57), stool (1, 1, 0.5), table (3, 3, 1) and chest (1.5, 1, 1) are as
in §1.10.2 for all t. A shelf makes exactly 2 d % 10 draws. In loot, k = 1 and k = 2 make one more
draw and k = 0 and 3 none (table 0x5d5fd0). §1.10.6 step 7.8 stands.

Both stretches are the rule of §11.1. I recovered the stream from the recording alone: searching all
states for each floor's three table values, then requiring its glow blocks and the next floor's
values, then replaying the cell. One state fits each.

**Cell (9,3,9)**, Z = −60. State before the floor box, low 31 bits, 0x160f81d2. Floor glow at (7,8)
agrees. 13 draws:

| Draw | Value | Meaning |
|---|---|---|
| 1 | 9336 | trap, % 40 = 16 |
| 2 | 6827 | −x, % 3 = 2 |
| 3 | 32367 | +x, % 3 = 0, neighbour open |
| 4 | 19383 | −y, % 3 = 0, wall |
| 5 | 22297 | torch, **odd** (your note said even) |
| 6 | 1145 | r = 1 |
| | | wall block (X + 4, Y − 1, **Z + 1**) is solid; Z + 2 is a hole |
| 7 | 21045 | torch kind, d % 3 = 0 |
| 8 | 23065 | & 7 = 1, furniture |
| 9 | 22291 | r = 3 |
| 10, 11 | 10856, 30992 | % 50 = 6; % 3 = 2, stool, s = 2 |
| 12 | 31955 | stool offset |
| 13 | 1350 | +y, % 3 = 0, neighbour open |

Reading Z + 2 here fails the test, loses draw 7, and gives your 12.

**Cell (10,18,7)**, Z = −80. State 0x12ae173b. Floor glow at (4,6) agrees. It is **not a shelf**, and it
has **a ceiling**: cell c = 8 is closed. Exactly 21 draws, all accounted for:

| Draw | Value | Meaning |
|---|---|---|
| 1 | 27089 | trap, % 40 = 9 |
| 2 | 11226 | ceiling, % 3 = 0 |
| 3–6 | 6594, 30069, 28107, 12140 | liana: angle, size, y, x |
| 7–10 | 17355, 12120, 5476, 16657 | cobweb sides, none makes a web |
| 11, 12 | 23753, 17620 | −x, +x, neighbours open |
| 13 | 21708 | −y, % 3 = 0, wall |
| 14, 15 | 8077, 5761 | torch odd, r = 1 |
| | | wall block (X + 4, Y − 1, Z + 1) is a hole; Z + 2 is solid: **no torch** |
| 16, 17 | 5742, 26878 | & 7 = 6, furniture; r = 2 |
| 18–20 | 14393, 31733, 18923 | % 50 = 43; % 3 = 2, stool, s = 2; offset |
| 21 | 19514 | +y, % 3 = 2 |

Reading Z + 2 here passes the test, adds one torch draw, and shifts everything after it. The shelf and
its loot in your replay most likely came from that shifted stream. **Confirmed** by exact replay of
these two cells. The other floors of style 2 were not replayed by me.
