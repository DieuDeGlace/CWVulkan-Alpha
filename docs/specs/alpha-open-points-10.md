Status: FINAL / STABLE / HELD

# Alpha open points, batch 10: dungeon styles 1 to 5

Clean-room behavioural specification. It covers the parts of the dungeon builder that depend on the
style value v (v = 1 to 5), plus the routines those parts call where a v-dependent result is decided.

Evidence: the original client image `Cube.exe` only (image base 0x400000). Every address below is an
RVA in `Cube.exe`. No recordings, captures or implementation material were used. Statements that
could not be confirmed in the image are listed under "Unresolved" and are not part of the contract.

Names used throughout:

* Xo, Yo: the dungeon origin passed to the builder (RVA 0x1CC510).
* The builder works on a cell grid of 22 × 22 × 22 cells (constructed at RVA 0x1CDFC9..0x1CDFD5);
  a cell is 10 blocks. Its centre cell column is (10, 10), so Xc = Xo + 100 and Yc = Yo + 100.
* base and ce: the base height and the anchor cell's third index chosen by the fit step
  (RVA 0x1CED52..0x1CED7A); "base + 10ce" is the anchor's absolute block height.
* c2: the colour record the builder passes to most box fills (stack slot written by the palette step,
  RVA 0x1CE5C4..0x1CE5D8, and overwritten per style below); c3: the second record set alongside it.
* g: the glow rate value passed with c2.
* "box": the box filler at RVA 0x1DF600. "carve": the carve routine at RVA 0x19ABA0.

---

## 1. The rough carve (carve routine, RVA 0x19ABA0)

Arguments: corner (X, Y, Z), a size (sx, sy, sz), a pass-through value, and a "rough" flag (last).

Order: cells are visited with the x index i outermost (0 .. sx − 1), then the y index j
(0 .. sy − 1), then the z index k innermost, running **downwards** from sz − 1 to 0. For each cell
(X′, Y′, Z′) = (X + i, Y + j, Z + k) the routine first writes the carved block, then, only when the
rough flag is set, tries the four faces below in this order. Each face is tried only when the cell
lies on that face of the box. A face writes its neighbour block when the noise value, as a float,
is strictly greater than 0.5.

| Order | Face condition | Neighbour written | Noise arguments (first, second) | RVA |
|---|---|---|---|---|
| 1 | i = 0 | (X′ − 1, Y′, Z′) | 0.1·Y′ + 13·X′, 0.2·Z′ | 0x19AC35 |
| 2 | j = 0 | (X′, Y′ − 1, Z′) | 0.1·X′ + 13·Y′, 0.2·Z′ | 0x19ACC0 |
| 3 | i = sx − 1 | (X′ + 1, Y′, Z′) | 0.1·Y′ + 13·X′, 0.2·Z′ | 0x19AD50 |
| 4 | j = sy − 1 | (X′, Y′ + 1, Z′) | 0.1·X′ + 13·Y′, 0.2·Z′ | 0x19ADE3 |

So on every face the coordinate running along the wall is scaled by 0.1 and the coordinate across
the wall is multiplied by the integer 13. The x faces and the y faces therefore use exchanged
coordinates.

Precision: the 0.1 term is the coordinate converted to double times the double 0.1 (RVA 0x31A048);
the 13 term is a 32-bit integer product converted to double; the two are added in double. The second
argument is the z coordinate converted to double times the double 0.2 (RVA 0x31DB58). The noise
routine is at RVA 0xC0EF0; its result is stored as a float and compared with the float 0.5
(RVA 0x345DA0).

Written block, for the carved cell and for every opened neighbour: three zero colour bytes and a
fourth byte 0xC0 (block writer RVA 0x4E7A0). The routine makes no random draws.

The builder sets the rough flag for v = 1, 2 and 4 only, on the per-cell carve of (10, 10, 10) at
the cell corner (RVA 0x1D0ADC..0x1D0B6A).

Why this matters for the stream: the torch wall test (§9) reads a block one step outside the cell
wall, which is exactly a block the rough carve can open. Its result decides whether the torch routine
(which makes draws) runs; on the +y side it also decides whether the furniture branch runs (§9.3).

---

## 2. The v = 4 pyramid stairs

These boxes are part of the per-column pass, after the 12 × 12 grass/terrain step (which is skipped
when the core top does not exceed the lowest column base sampled under the column; the stairs run
in either case, jump at RVA 0x1CFD81). H is the column's core top height (RVA 0x1CFCD3..0x1CFD05).
With (a, b) the column's cell index and (X, Y) = (Xo + 10a, Yo + 10b):

Each run has **11 boxes, t = 0 .. 10**: the counter starts at −1 and repeats while it is below 10,
the same form in all twelve loops (first one RVA 0x1D0018..0x1D0086).

| Where | Run 1 | Run 2 | Run 3 | RVA |
|---|---|---|---|---|
| a = 10, b ≤ 8 | box(X − 2, Y − 1 + t, H, (14, 1, t + 2)) | box(X − 2, Y − 1 + t, H, (2, 1, t + 4)) | box(X + 10, Y − 1 + t, H, (2, 1, t + 4)) | 0x1D0012 |
| a = 10, b ≥ 12 | as above with y = Y + 10 − t | same | same | 0x1D0183 |
| b = 10, a ≤ 8 | box(X − 1 + t, Y − 2, H, (1, 14, t + 2)) | box(X − 1 + t, Y − 2, H, (1, 2, t + 4)) | box(X − 1 + t, Y + 10, H, (1, 2, t + 4)) | 0x1D033E |
| b = 10, a ≥ 12 | as above with x = X + 10 − t | same | same | 0x1D04CC |

The row condition is: the column's index equals the centre index on one axis, and on the other axis
differs from the centre by more than 1; the sign picks the row. All boxes use colour c2, rate g and
flags 0.

Draws. The box filler makes 3 draws when it starts (for table slots 0, 8 and 16 of its 23-slot
table, RVA 0x1DF630..0x1DF653) and then one draw per block (RVA 0x1DF79D). Per stair column:
run 1 is 33 + 14·77 = 1,111; runs 2 and 3 are 33 + 2·99 = 231 each; total **1,573**.

Eligible columns: on a 22-wide grid, 9 + 10 = 19 per centre line, 38 in all. A column is processed
only when the terrain column lookup at its corner succeeds (skip at RVA 0x1CFB90).

---

## 3. The v = 4 and v = 5 central shaft

Common values:

* **Ze = base + 10ce + 8** for both styles (v = 4: RVA 0x1D3DE8..0x1D3DFB; v = 5:
  RVA 0x1D4317..0x1D432A; the code forms ce + 4(ce + 1) and doubles it).
* v = 4: **Zt = base + 10ce + 44** (RVA 0x1D3E01..0x1D3E10).
* v = 5: **Zt = base + 10ce + 38** (RVA 0x1D4330..0x1D433F), then the working height is lowered by
  12 right after the first box (RVA 0x1D43D7): **Zc = Zt − 12 = base + 10ce + 26**. Every later step
  of v = 5 uses Zc. v = 4 lowers nothing.

v = 5 steps, in order (all carves not rough):

| Step | Call | RVA |
|---|---|---|
| 1 | box(Xc − 2, Yc − 2, Zt, (14, 14, 14), c2, g, 0) | 0x1D43C3 |
| 2 | carve(Xc + 2, Yc − 32, Zc, (6, 74, 6)) | 0x1D4413 |
| 3 | carve(Xc − 32, Yc + 2, Zc, (74, 6, 6)) | 0x1D4460 |
| 4 | carve(Xc, Yc, Ze, (10, 10, Zc − Ze + 6)), i.e. 24 high | 0x1D44B3 |
| 5 | pillar box(Xc + 4, Yc + 4, Ze − 30, (2, 2, n)), n = Zc − Ze + 30 = 48 | 0x1D4536 |
| 6 | spiral, below | 0x1D4548..0x1D4812 |

v = 4 uses the same pillar and spiral form with Zt in place of Zc: central carve (Xc, Yc, Ze) of
height Zt − Ze = 36 (RVA 0x1D3F78), pillar at RVA 0x1D3FFB with n = Zt − Ze + 30 = 66.

Spiral, both styles: only when n > 1. For u = 1, 2, … while u < n (so u = 1 .. n − 1), one box at
height z = Ze − 30 + u (first z = Ze − 29, one step per block), chosen by u & 7 through an 8-entry
jump table (v = 4: RVA 0x1D5FF8, v = 5: RVA 0x1D6018). Both tables give identical corners and sizes;
colour c2, rate g, flags 0:

| u & 7 | Corner | Size |
|---|---|---|
| 0 | (Xc, Yc) | (5, 5, 1) |
| 1 | (Xc + 5, Yc) | (1, 5, 1) |
| 2 | (Xc + 6, Yc) | (5, 5, 1) |
| 3 | (Xc + 6, Yc + 5) | (5, 1, 1) |
| 4 | (Xc + 6, Yc + 6) | (5, 5, 1) |
| 5 | (Xc + 5, Yc + 6) | (1, 5, 1) |
| 6 | (Xc, Yc + 6) | (5, 5, 1) |
| 7 | (Xc, Yc + 5) | (5, 1, 1) |

Loop RVAs: v = 4 start 0x1D400D, index 0x1D4020, end 0x1D42C1; v = 5 start 0x1D4548, index 0x1D4560,
end 0x1D4801.

Draws (box filler rule of §2; carves make none):

* v = 5: step 1 = 3 + 14³ = 2,747; pillar 3 + 4·48 = 195; spiral 24 odd u at 8 and 23 even u at 28
  = 836. Pillar plus spiral 1,031.
* v = 4: pillar 3 + 4·66 = 267; spiral 33 odd u at 8 and 32 even u at 28 = 1,160. Pillar plus
  spiral 1,427.

---

## 4. The level of the ravine creature of type 59

In the zone generator, the type 59 creature's level comes from the routine at RVA 0x19A9B0, called
at RVA 0x1E68F9. Its two 64-bit arguments are the block corner of the column shifted to fixed point:
**(x << 16, y << 16)**, without the +0.5 block offset that the same code adds to the creature's own
x and y position (offset value from the double −32768 at RVA 0x302AD8). Then +0x34 = the result,
+0x28 = 1 and +0x2C = 59 (RVA 0x1E68FE..0x1E6908).

The neighbouring ravine case writes +0x34 = 1 and +0x28 = 6 as constants (RVA 0x1E66F4, 0x1E66FB);
its type is 132 unless a draw % 10 (RVA 0x1E670C) is 0, 1 or 3, which select other paths.

Direct callers of RVA 0x19A9B0 in the image: 0x1B1C74, 0x1E68F9, 0x1F392E, 0x1F429B, 0x20DC7F
(five). The level rule itself is in the companion spec open-points-5 §1 (pointer only).

---

## 5. Box colour and moss

### 5.1 Colour step of the box filler (RVA 0x1DF6E8..0x1DF8C9)

For each block, a table value t is taken from the box's 23-slot table. R = c.R + t, G = c.G + t and
B = c.B + t as integers, each converted to float. Only G takes moss: with the moss value m,
G becomes m·(120 − G) + G, evaluated in single precision with each subtraction, product and sum
rounded to float (120 at RVA 0x2FCE58). Each channel is then clamped below at 0 and above at 255
(RVA 0x2FCE64) and truncated to an integer. R and B take no moss.

### 5.2 Moss value (RVA 0x1EFDB0, arguments x, y, z)

1. Moisture mo: field +4 of the terrain column record for (x, y) when the lookup (RVA 0x347A0)
   succeeds, otherwise the value of RVA 0x1C4800 for (x, y).
2. First noise. Its first argument is y converted to float, times the float 0.2 (RVA 0x2FCDBC),
   rounded to float, plus the float 534 (RVA 0x31DF60), rounded to float, then widened to double.
   Its second argument is z converted to float, times the float 0.2, rounded, plus the float 13
   (RVA 0x31DF50), rounded, widened to double. The result is stored as a float, then multiplied by
   the float 0.1 (RVA 0x2FCB7C) in single precision: n1.
3. Second noise, in double: first argument x converted to double times 0.05 (RVA 0x31DB48) plus
   4343 (RVA 0x31DE50); second argument z converted to double times 0.1 (RVA 0x31A048) plus 84734
   (RVA 0x31DEF8). The result is stored as a float: n2.
4. q = ((n1 + n2) · 0.7) + 0.2, each operation in single precision (0.7 at RVA 0x2FD570, 0.2 at
   RVA 0x2FCDBC).
5. m = q · mo in single precision; if above 1.0 (RVA 0x345DC0) it is 1, if below 0 it is 0.

### 5.3 Palette per style (dispatch RVA 0x1CE674, table RVA 0x1D5FA4, index v − 1)

Before the dispatch every style draws the common palette (RVA 0x1CE520..0x1CE665), including one
extra draw for v = 3 only (RVA 0x1CE573..0x1CE585), with byte additions that wrap at 8 bits; g is
set to 0.005 (RVA 0x2FCD9C).

* v = 1, 2 (RVA 0x1CE67B): c2 and c3 re-rolled with six more draws; g = 0.0015 (RVA 0x31DCE4).
* v = 3: dispatch goes straight on; g stays 0.005.
* v = 4 (RVA 0x1CE756): c2 and c3 re-rolled with six more draws; g = 0.0025 (RVA 0x31DCE8).
* v = 5 (RVA 0x1CE806): fixed, no draws: c2 = (255, 210, 130), c3 = (200, 180, 80); g stays 0.005.

---

## 6. (Section withdrawn)

The former section about v = 5 terrain colours rested on recordings; see "Unresolved". One fact
was confirmed in the image: the range RVA 0x1E4E00..0x1E6464 contains 9 single-precision truncating
conversions and no double-precision or x87 truncation calls.

---

## 7. Style-dependent sites in the builder (RVA 0x1CC510)

Each row gives the v condition read in the image. Agreement with the wording of the companion
structures spec was not re-checked in this pass (see "Unresolved").

| Part | RVA | v-dependent content confirmed |
|---|---|---|
| Style setup | 0x1CC5DA..0x1CC933, table 0x1D5F90 (index v − 1) | v = 1, 2 share a path with one draw % 3 (RVA 0x1CC800) |
| Room selection | 0x1CCD08..0x1CCD23 | v = 4, 5 → RVA 0x1CDB6E; v = 2 → RVA 0x1CCD29; other v → RVA 0x1CD0A4 |
| Palette | 0x1CE4E1..0x1CE83E | §5.3 |
| Arch model | 0x1CE844..0x1CEA59 | slot 2545 (offset 0x27C4) for v = 4, 5; slot 2544 (offset 0x27C0) otherwise |
| Raised floors | 0x1CE887..0x1CEA05 | only v = 0, 1, 2, 3; v = 2 skips the draw % 6 (RVA 0x1CE906) |
| Fit | 0x1CEA5F..0x1CEE13 | base + 50 for v = 4, 5 (RVA 0x1CEDB0..0x1CEDC9); base − 100 when liquid was found |
| Last palette | 0x1CEE19..0x1CEEB5, table 0x1D5FB8 | one extra draw for v = 0 and 3 only |
| Roof | 0x1CF06A | a v = 2 branch |
| Pyramid core, stairs | 0x1CFBF8..0x1D0651 | v = 4 (§2) |
| Mound | 0x1D06F2 | v = 5 |
| Carve flag | 0x1D0ADC..0x1D0B6A | rough for v = 1, 2, 4 (§1) |
| Windows | 0x1D0B9A | v = 0, 1, 3 |
| Stairs pass | 0x1D1270 | every v = 0 .. 5 |
| Step 5 | 0x1D176F | for v = 4, a cell whose kind byte is 4 skips the rest of the cell pass (to RVA 0x1D3D45) |
| Floor | 0x1D17A4 | every v = 0 .. 5 |
| Carpet | 0x1D1856 | v = 0, 3 |
| Ceiling | 0x1D1C64..0x1D2328 | liana v = 2, 4; cobwebs v = 1, 2; constants 360, 0.04, 0.08, 20, 0.05, 10 (RVA 0x2FCE6C, 0x2FCDB0, 0x302AFC, 0x2FCD74, 0x2FCDB4, 0x345E74) |
| Torch | 0x1D2478, 0x1D27DD, 0x1D2B42, 0x1D2EB1 | v is passed to the torch routine RVA 0x1F8960 |
| Loot kinds | 0x1D30EA..0x1D31F8 | kinds 10, 13, 12, 35, 36, 37 tested |
| Loot sub-choice | table 0x1D5FD0 (d & 3) | k = 1 and k = 2 make one more draw % 3; k = 0 and 3 none |
| Chandelier | 0x1D3963 | v = 3 |
| Shaft | 0x1D3DA2, 0x1D42D7 | v = 4, v = 5 (§3) |
| Erosion | 0x1D4817..0x1D4AA3 | v = 1, 2; block kept when attribute bit 0x40 is set and the test at RVA 0x4BE30 (attribute bit 0x80 clear) passes |

Builder callers (RVA 0x1E84A2..0x1E84F7): when a location record's kind byte (+0x18) is 3, v is
that record's byte +0x19 and the last flag is 0; when the kind is 5, v = 3 and the last flag is 1.
Separately, the 8 bytes at zone +0x78 are copied from field +0x18 of a 16-byte location record
(RVA 0x1E6BA1..0x1E6BA7), so zone +0x79 holds a record's byte +0x19.

---

## 8. Room creature fields (no draws)

* **+0x54**: ring and centre creatures: the float angle θ widened to double, divided by π
  (RVA 0x2FCE08), times 180 (RVA 0x2FCE18), plus 90 (RVA 0x31DC78), narrowed to float
  (RVA 0x1D5884..0x1D58A9, 0x1D5CDF..0x1D5D04).
* **+0xF60**: ring and centre creatures: multiplied by 0.75 (RVA 0x2FCDC4; at RVA 0x1D59FB and
  0x1D5D6D); escorts: multiplied by 0.5 (RVA 0x345DA0; at RVA 0x1D5603).
* **+0x109C**: for the ring creatures, a 0x0C-byte object is created (RVA 0x1D590A), a 20.0 object
  (RVA 0x1D595A) and a 0x1C-byte object built with 2.0 (RVA 0x1D598B..0x1D59AD) are added to it, and
  it is stored at +0x109C (RVA 0x1D59E6). +0x109C is also written at RVA 0x1D3C65, 0x1D533A,
  0x1D55F5 and 0x1D5E59.
* **+0x48, +0x4C**: the leader receives the 64-bit result of RVA 0x1C0120 (RVA 0x1D53C8..0x1D53DF),
  called with the two elements at zone +0x60 and a value obtained from the list at zone +0x18
  (RVA 0x87F50); the escort's follow object copies it (RVA 0x1D555C..0x1D5565).

---

## 9. The torch wall test and the fixed-point block read

### 9.1 Positions

The four torch sides build their test position with z = base + 10c + 2 (c the cell's z index) on
every side (RVA 0x1D237C, 0x1D26E1, 0x1D2A46, 0x1D2DB1), as a block value shifted to fixed point
(RVA 0x12080). Along the wall the coordinate is cell corner + 3 + r, with r = draw & 3 drawn while
the position is built. Across the wall the coordinate is a double offset by half a block, shifted
to fixed point (RVA 0x59C00, factor 65536 at RVA 0x2FD588), then moved one block outward
(RVA 0x3ABC0 subtracts one block, RVA 0x3ABF0 adds one). With (X, Y, Z) the cell corner, the −x
side reads block (X − 1, Y + 3 + r, Z + 2) and the −y side reads (X + 3 + r, Y − 1, Z + 2), subject to
§9.2.

### 9.2 The read (RVA 0x2F860)

The read takes three 64-bit fixed-point coordinates. For each axis whose value is negative it first
subtracts 65536 (RVA 0x2F870..0x2F8C3), then divides by 65536 with signed 64-bit division that
truncates toward zero (RVA 0x28D9E0). This floors a fractional negative value correctly but gives
one block too low for a whole negative block. Hence, for the torch test: the wall block z is Z + 2
when Z + 2 ≥ 0 and Z + 1 when Z + 2 < 0, on all four sides. The x and y results follow the same rule;
they are unaffected only while those coordinates are non-negative.

In the builder the four torch tests are the only direct calls of this read (RVA 0x1D2459, 0x1D27BE,
0x1D2B23, 0x1D2E8E). Other builder reads use the integer-block read at RVA 0x2F7E0, which has no such
adjustment. The image has 61 direct calls of the fixed-point read in total.

### 9.3 Order on each side

Each side first draws d % 3 (−x: RVA 0x1D2328) and requires remainder 0 and a solid neighbour cell
(RVA 0x1EEA40). It then draws a parity. On odd parity it draws r, performs the wall test, and calls
the torch routine when the wall block passes. On −x (RVA 0x1D249C..0x1D24B2) both parity outcomes
continue to the following d & 7 draw. On +y (RVA 0x1D2DAB, 0x1D2ED5) the odd branch ends the side
and only the even branch reaches the d & 7 draw.

### 9.4 Furniture sizes (RVA 0x1F6E20)

The furniture routine writes the size triples (1.5, 1, 1) at RVA 0x1F6F22, (1, 1, 0.5) at
RVA 0x1F701D, (3, 3, 1) at RVA 0x1F7197 and (2, 1, 1.57) at RVA 0x1F734C. The loot pass reads the
stored scale components 0 and 1 through the record accessor (RVA 0x1D3213..0x1D325D) without a
rotation-dependent exchange at those reads.

---

## 10. Pointers to companion specs

These sections bear on companion specs (not opened in this pass): structures §1.10.2 items 1 and 2
(§1, §5), structures §1.10.4 step 3 (§2), structures §1.10.7 (§3), structures §1.10.6 step 7.7
(§9), structures §1.11 (draw totals of §2 and §3), open-points-5 §1.2 (§4).

---

## Unresolved

* Whether the corrections listed in the former §9 actually differ from the current companion spec
  text; the companion specs were not opened.
* The value returned by RVA 0x19A9B0 for the type 59 creature in any particular case (the former
  "3 inside the radius, 1 outside" example was recording-derived), and whether the location record
  is prepared before the call.
* Whether the record copied to zone +0x78 is the same record instance whose byte +0x19 becomes v,
  and therefore whether the furniture routine's style value equals v in fresh worlds.
* The furniture routine's per-style paths (t = 3, 4, 5 shelves and t = 5 table without loot; two
  d % 10 draws per shelf), the torch routine table at RVA 0x1F8AB8, and the item generators at
  RVA 0x1F7A60 and 0x1F6D50.
* "Done before the items" ordering of the +0xF60 scaling.
* The exact across-wall block of the +x and +y torch tests (their base coordinate was not
  re-derived; only the outward one-block step and the z rule were confirmed).
* The v = 5 terrain colours of the former §6: the fire-tint explanation, the B = 9 / 8 blocks and
  which surface mix produces them.
* Agreement of all §7 rows with the companion structures spec, including room parameters, room
  record layout, fixed reset, entrance, parapets, shell, foundation, mound, windows, boss, room
  spawns, view transforms (RVA 0x1FA3C0, 0x1FA440) and the solid test.
* Prefab slot 2545 block content.
* Whether the rules here account for all differences in a complete v = 1 to 5 build (no replay).

---

## Re-certification (2026-11-15)

Evidence: `Cube.exe` only, disassembled with dumpbin; constants read from the image's data
sections. All addresses converted from virtual addresses to RVAs (VA − 0x400000).

**Confirmed:** rough-carve face order, conditions, noise arguments with x/y exchange, precision,
threshold, written block, no draws; rough flag for v = 1, 2, 4; v = 4 stairs (11 boxes per run, all
twelve loops, positions and sizes, run after the core even when the grass step is skipped); box
filler draw rule (3 + one per block) and the per-column total 1,573; 38 eligible columns; v = 5
shaft heights Zt, Zc = Zt − 12 and Ze = base + 10ce + 8 for both styles; v = 5 step calls; pillar n
(48 / 66); spiral loop and table, identical for v = 4 and 5; spiral and pillar draw totals; type 59
level call, its block-corner arguments and the fields written; five callers of RVA 0x19A9B0; box
colour step and the full moss computation with all constants; v = 5 fixed palette; glow rate
constants; arch slots; fit + 50; last-palette extra draw; ceiling constants and style conditions;
loot kinds and sub-choice draws; erosion bit tests; +0x54, +0xF60, +0x109C, +0x48/+0x4C fields;
torch test z on all four sides; fixed-point read behaviour; four torch reads are the only such calls
in the builder; furniture size triples; 9 float truncations in RVA 0x1E4E00..0x1E6464.

**Corrected (old → new):**

1. Addresses: unlabelled virtual addresses → RVAs in `Cube.exe`.
2. Glow rate row: values only → assigned per style (0.005 default and v = 0, 3, 5; 0.0015 for v = 1,
   2; 0.0025 for v = 4).
3. Type 132: "the second ravine case" → the default of a draw % 10 choice in that case.
4. Leader +0x48/+0x4C: "(zone coordinates, list size)" → two zone +0x60 elements and a value from the
   zone +0x18 list (the latter's meaning not traced).
5. Fixed-point read: "x and y are unaffected, since world x and y are positive" → same rule on all
   axes, harmless only while non-negative.
6. "Town builder (22 calls), zone generator (4)" → 61 direct calls in the image (per-routine split
   not established).
7. Rough-carve loop: order of the z index added (descending).
8. zone + 0x79: "the same byte the builder gets as v; doubt closed" → field identity confirmed, record
   identity unresolved (a reversal of the former "closed" statement).

None of these changes alters a world-generation rule or random-draw order stated in the former text;
items 7 and 8 refine the description of order and provenance.

**Removed:** all recording references and recording-derived material: the recording list, the
"confirmed = measured" confidence labels and the confidence column, the `.avzd` column-order warning,
the measured hole tables and style-4 test vectors (§1), the measured shaft positions (§3), the 140 of
140 type 59 match and example location (§4), the zone moisture estimate, mismatch tables and moss test
vectors (§5), the v = 5 terrain colour analysis (§6), the style-2 worked example heights (§11.2), the
reconstructed stream tables of two style-2 cells (§11.3), comparisons against an earlier reading's
draw totals, statements about a port's symptoms and causes, the note on the decompiler and listing
holes, and verification instructions.

**Unresolved:** as listed in the section above.
