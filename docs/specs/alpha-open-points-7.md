# Alpha open points, batch 7: dungeon corridors, link order and the entrance

Clean-room behavioural specification. It closes these points:

| Point | Spec it belongs to | Section here |
|---|---|---|
| Which room is A and which B; link order for v = 0, 1, 3 | `alpha-worldgen-structures.md` §1.4, §1.6 | §1 |
| The corridor routine 0x5c52a0: overlap tests, m, clamp, end points, extra lines | structures §1.6 | §2 |
| The line routine 0x5ef6d0 | structures §1.6 | §3 |
| The entrance room | structures §1.7 | §4 |
| Steps between the corridors and the fit | structures §1.7 to §1.9 | §5 |
| Worked example against the seed 41 recording | | §6 |

**The short version.** Three statements of the earlier spec are wrong, and together they explain
every difference seen:

1. For v = 0, 1, 3 the link table of §1.4 is printed **backwards**: each link's two rooms are
   exchanged. The corridor routine's A is the link's true first room, so §1.6 is right as worded,
   but its input table was not.
2. The line routine puts its extra cell on the **other side** from what §1.6 says. On a z step the
   extra cell and the stair flag always belong to the **lower** of the two cells.
3. The third line of a corridor along x tests **x** bounds against the y coordinate m.

Because of point 1, the entrance of v = 0, 1, 3 is **always in room 10**, not room 1. For the
recorded dungeon it is cell (9, 1, 10), not (2, 1, 9). With the rules below, the recording is
reproduced exactly, cell for cell (§6.5).

Written from the Alpha client (`Cube.exe`) per-instruction listing. The decompiler's output was
not relied on. The server's routines were compared instruction by instruction with the client's:
corridors 0x4f9010 against 0x5c52a0 and line 0x5234b0 against 0x5ef6d0 are identical apart from
addresses, and the link constructor calls in builder 0x500300 push their arguments the same way.
No code was copied; addresses are citations only.

The check in §6 used the block data of the recording
`references/alpha-recordings/seed-41-dungeon/zone_32795_32796.avzd`, which is a game output, and
nothing under `src`.

Conventions: as in the structures spec §0 and §1. A cell is (a, b, c); "kind 3" is open; "stair
flag" is bit 1 of the cell's flag byte (bit 0x100 of the 16-bit cell). All grid arithmetic is
32-bit signed integer. rot = mir = 0 during this stage.

---

## 1. Which room is A and which is B

### 1.1 The call

The link loop 0x5ce206..0x5ce3ed walks the link list from its first node. A link node holds two
positions: **first** at node + 8 and **second** at node + 0x14. For each node:

1. If first's room is not present, or second's room is not present, go to the next node: no call,
   no draws (0x5ce246, 0x5ce254).
2. Box of each room from its record as in §1.5: P = (7a + 2 + p0, 7b + 2 + p1, 7c + 3 + p2),
   S = (p3 + 3, p4 + 3, p5 + 1). The fixed-room reset of §1.5 is written into the record, so it
   holds here too.
3. Call 0x5c52a0(P of first, S of first, P of second, S of second) (0x5ce3b0..0x5ce3de).

So **A = node first, B = node second**. That part of §1.6 is right.

### 1.2 The table of §1.4 is reversed for v = 0, 1, 3

The links of v = 0, 1, 3 are built with the small constructor 0x5c3b70. Each site fills two
temporaries, then pushes the one at 0xfffffcb4 and after it the one at 0xfffffcc0. The last push is
the first argument, and the constructor copies its first argument to the node's first position.
So the temporary at **0xfffffcc0** is the link's first position. §1.4 read them the other way
round.

The links of flag = 1, v = 2 and v = 4, 5 are written directly into the pair in memory, in order.
§1.4 is right for those three tables.

Corrected tables, extracted mechanically from every site. These replace the link lists of §1.4 for
v = 0, 1, 3, flag = 0. The room selection and draws of §1.4 do not change.

| s (d % 3) | Code | Links, in order (first → second) |
|---|---|---|
| 0 | 0x5cd174..0x5cd41b | (1,0,1)→(0,0,1), (1,0,1)→(2,0,1), (0,0,1)→(0,1,1), (0,1,1)→(1,1,1), (0,1,1)→(0,2,1), (1,1,1)→(1,2,1), (1,1,1)→(2,1,1), (2,1,1)→(2,2,1) |
| 1 | 0x5cd8b1..0x5cdb58 | (1,0,1)→(1,1,1), (1,0,1)→(2,0,1), (1,1,1)→(0,1,1), (0,1,1)→(0,0,1), (0,1,1)→(0,2,1), (0,2,1)→(1,2,1), (1,2,1)→(2,2,1), (2,2,1)→(2,1,1) |
| 2 | 0x5cd4e2..0x5cd7ea | (1,0,1)→(0,0,1), (1,0,1)→(2,0,1), (2,0,1)→(2,1,1), (0,0,1)→(0,1,1), (0,1,1)→(1,1,1), (0,1,1)→(0,2,1), (1,1,1)→(1,2,1), (1,1,1)→(2,1,1), (2,1,1)→(2,2,1) |

Read this way each table is a tree that grows outwards from room 10, the room §1.4 always makes
present, and that is the room each table starts from.

**Why the stream stayed right.** The overlap tests are symmetric in A and B, and the present test
looks at both rooms, so exchanging them leaves the number of draws the same (2 per link that
overlaps, 0 otherwise). The list order is also unchanged. Only the shapes change.

---

## 2. The corridor routine 0x5c52a0

Arguments PA, SA, PB, SB; the routine reads grid coordinates only and writes through the line
routine. Every line goes **from A towards B** (F at A's end, T at B's end). Where §1.6 says
otherwise, this section wins.

### 2.1 Overlap tests and m

The x test comes first, and the y test is reached only if the x test fails.

| Case | Test (both strict) | m, before the clamp | Clamp range |
|---|---|---|---|
| **Along y** (0x5c52c9..0x5c52d9) | PB.x < PA.x + SA.x **and** PA.x < PB.x + SB.x | SB.x/2 + PB.x | PA.x … PA.x + SA.x − 1 |
| **Along x** (0x5c54c9..0x5c54e5) | PB.y < PA.y + SA.y **and** PA.y < PB.y + SB.y | SB.y/2 + PB.y | PA.y … PA.y + SA.y − 1 |
| Neither | | no lines, **no draws** | |

The clamp is: if m is below the low bound, m = low bound; then if m ≥ PA + SA on that axis,
m = PA + SA − 1. The halving truncates; sizes are positive, so that is a plain integer half.
m takes the middle of **B** and is kept inside **A**.

### 2.2 End points of the first line

z is always **PA.z at F** and **PB.z at T**. Neither end adds anything to z.

| Case | Direction test | F | T |
|---|---|---|---|
| Along y, B lower | PB.y < PA.y | (m, PA.y, PA.z) | (m, PB.y + SB.y, PB.z) |
| Along y, B higher or equal | PB.y ≥ PA.y | (m, PA.y + SA.y − 1, PA.z) | (m, PB.y − 1, PB.z) |
| Along x, B lower | PB.x < PA.x | (PA.x, m, PA.z) | (PB.x + SB.x, m, PB.z) |
| Along x, B higher or equal | PB.x ≥ PA.x | (PA.x + SA.x − 1, m, PA.z) | (PB.x − 1, m, PB.z) |

So F is the last cell **inside** A on the side facing B, and T is the first cell **outside** B on
the side facing A. This is one rule in both directions.

**Correction to §1.6:** for "B lower" §1.6 gave the line from B's end to A's end. The code starts
at A (0x5c532a..0x5c5332 pushes so that F is A's end). The end points were right; the direction
was not, and with sloping lines the direction changes which cells are visited.

### 2.3 The two extra lines

Order, in every case:

1. first line;
2. **draw**; if odd and the second-line test holds: second line;
3. **draw**; if odd and the third-line test holds: third line.

Both draws are always made once the case is entered. The tests are made after each draw and take
no draws. The extra lines have the same F and T as the first line, except that the coordinate m is
replaced: m − 1 for the second line and m + 1 for the third.

| Case | Second line (m − 1) needs | Third line (m + 1) needs | Code of third test |
|---|---|---|---|
| Along y, either direction | m > PA.x and m > PB.x | m < PA.x + SA.x − 1 and m < PB.x + SB.x − 1 | 0x5c53a2, 0x5c5482 |
| Along x, B lower | m > PA.y and m > PB.y | m < PA.y + SA.y − 1 and **m < PB.x + SB.x − 1** | 0x5c55ab..0x5c55cd |
| Along x, B higher or equal | m > PA.y and m > PB.y | **m < PA.x + SA.x − 1 and m < PB.x + SB.x − 1** | 0x5c5682..0x5c569a |

The bold bounds are not slips of this document. In the along-x case m is a **y** coordinate, yet
the code compares it with x extents: one of the two bounds when B is lower and both when B is
higher. *As read*: it looks unintended, but it is what the code does, the server does the same,
and the recording needs it (§6.3).

---

## 3. The line routine 0x5ef6d0

Inputs F, T. Integer arithmetic only: no floats, no 0.5, no rounding step.

1. d = T − F per axis. n = max(|dx|, |dy|, |dz|). If n = 0: **nothing is written**, not even F.
2. prev = F.z.
3. For t = 0, 1, …, n (both ends included):
   1. Position p = F + (d·t) / n per axis, where / is C integer division, truncating towards zero
      (0x5ef75a..0x5ef7ab). Since n is the largest step, z changes by at most 1 per step, and at
      t = n the position is exactly T.
   2. The cell at p: kind byte = 3 (0x5ef85f). Flags are left alone. Any old kind is replaced,
      including pit pillars.
   3. **If p.z < prev** (going down, 0x5ef865): the cell (p.x, p.y, p.z + 1) gets kind 3, then the
      cell **p** gets the stair flag.
   4. **If p.z > prev** (going up, 0x5ef890): the cell (p.x, p.y, p.z − 1) gets kind 3 **and** the
      stair flag.
   5. prev = p.z.

Details:

* "Climbs" compares this step's integer z with the previous step's integer z. It does not compare
  with T.z, and x and y play no part. t = 0 can never count as a step because prev starts at F.z.
* **In both directions, the stair flag lands on the lower of the two cells and the upper one is
  open.** The two cases are the same rule seen from either end. §1.6 had the two sides the wrong
  way round: it opened the cell above on a climb and the cell below on a descent.
* The last step is not special. If z changes there, the extra cell is written like any other.
* The routine writes through the view of §1.5. Out-of-range cells go to the scratch cell and are
  lost.
* It makes no draws.

This fits the staircase test of §1.10.6 step 4, which sits in the flagged cell and looks for an
open cell at level c + 1: the stair cell has to be the lower one.

---

## 4. The entrance (correction to §1.7 step 1)

§1.7's formula is right: the first node's first position gives the room, and the cell
(P.x, P.y − 1, P.z) becomes kind 4 (0x5ce3f3..0x5ce4de). But with §1.2 above, that room is:

| Case | First position | Room |
|---|---|---|
| v = 0, 1, 3, flag = 0, any s | (1, 0, 1) | **10** |
| v = 2, flag = 0 | (1, 0, 2) | 11 |
| v = 4, 5, flag = 0 | (1, 1, 2) | 14 |
| flag = 1 | (1, 1, 1) | 13 |

In the recorded dungeon room 10 has P = (9, 2, 10), so the entrance is **(9, 1, 10)**. The recording
settles this (§6.4). The statement of §1.8 that the room directly below the entrance room is never
present still holds: room 9 is never present.

**A warning about the fit.** The fit of §1.9 is scored from the entrance cell, so a port that used
(2, 1, 9) measured the wrong columns. It reached rot 0 and base 56 by chance: g0 = 146 at block
column (57, 47), and 146 − 90 = 56. The true column is (127, 47), which must give g0 = 156 so that
156 − 100 = 56. See §7 for what is left to confirm.

---

## 5. Between the corridors and the fit

Nothing is missing. Every grid access in 0x5ce4de..0x5ceeb3 was listed. There are exactly two
kind writes: the entrance at 0x5ce4de, and the raised floors at 0x5ce9cf (kind 2). The rest are
reads: 0x5ce95c and 0x5ce97a in the raised-floor scan, 0x5ceb29 in the entrance search. §1.7 to
§1.9 describe all of them. No flag is written in that range.

The differences in cell kinds came only from §1.2, §2.3 and §3 of this document, as §6 shows.

---

## 6. Worked example: seed 41, zone (32795, 32796), v = 0

The rooms present are 1, 4, 10, 13, 16, 22, with 19, 7 and 25 absent. Room 25 absent rules out s = 0
and s = 1, so **s = 2**. Going down the table of §1.2, these links have both rooms present:

| Link | Rooms | Case |
|---|---|---|
| (1,0,1)→(0,0,1) | 10 → 1 | along x, B lower |
| (0,0,1)→(0,1,1) | 1 → 4 | along y, B higher |
| (0,1,1)→(1,1,1) | 4 → 13 | along x, B higher |
| (1,1,1)→(1,2,1) | 13 → 16 | along y, B higher |
| (1,1,1)→(2,1,1) | 13 → 22 | along x, B higher |

The four links with room 19, 7 or 25 are skipped. That leaves 10 draws, as the stream requires.

### 6.1 Room 1 → room 4 (the link you asked for as "room 4 to room 1")

PA = (2,2,9), SA = (3,3,3); PB = (3,10,11), SB = (2,3,1).

x test: 3 < 5 and 2 < 5, so along y. m = 2/2 + 3 = 4, inside 2..4, stays 4. PB.y = 10 ≥ 2, so
F = (4, 4, 9), T = (4, 9, 11). d = (0, 5, 2), n = 5.

| t | Position | 2t/5 | Step | Writes |
|---|---|---|---|---|
| 0 | (4,4,9) | 0 | | open |
| 1 | (4,5,9) | 0 | | open |
| 2 | (4,6,9) | 0 | | open |
| 3 | (4,7,10) | 1 | up | open; (4,7,9) open + stair |
| 4 | (4,8,10) | 1 | | open |
| 5 | (4,9,11) | 2 | up | open; (4,9,10) open + stair |

Draw 1 is odd and 4 > 2, 4 > 3, so the second line at a = 3 repeats the same table. Third line:
4 < 2 + 3 − 1 = 4 fails, so nothing, whatever draw 2 was.

Result at a = 3, 4: layer 9 at b = 5, 6, 7; layer 10 at b = 7, 8, 9; layer 11 at b = 9. That is the
recorded set, including the cells "one further" at each end, which are the extra cells of the two
z steps. (b = 4 at layer 9 is inside room 1.)

### 6.2 Room 13 → room 22

PA = (10,9,10), SA = (2,4,1); PB = (17,9,11), SB = (4,3,3).

x test: 17 < 12 fails. y test: 9 < 13 and 9 < 12, so along x. m = 3/2 + 9 = 10, inside 9..12.
PB.x = 17 ≥ 10, so F = (11, 10, 10), T = (16, 10, 11). d = (5, 0, 1), n = 5.

| t | Position | t/5 | Step | Writes |
|---|---|---|---|---|
| 0..4 | (11..15, 10, 10) | 0 | | open |
| 5 | (16,10,11) | 1 | up | open; (16,10,10) open + stair |

Draw 1 is odd and 10 > 9, 10 > 9, so the second line runs at b = 9. Draw 2 is odd and 10 < 10 + 2 − 1
= 11, 10 < 17 + 4 − 1 = 20, so the third line runs at b = 11.

Result: layer 10 a = 12..16 at b = 9, 10, 11, with stair cells at a = 16; a = 16 also open at
layer 11. Together with room 13 (a = 10, 11, b = 9..12) that gives the recorded a = 10..16 at b = 9
and the a = 10..16 part of b = 10, 11.

### 6.3 Room 4 → room 13 (the a = 9 cells)

PA = (3,10,11), SA = (2,3,1); PB = (10,9,10), SB = (2,4,1).

x test: 10 < 5 fails. y test passes, along x. m = 4/2 + 9 = 11, inside 10..12. PB.x = 10 ≥ 3, so
F = (4, 11, 11), T = (9, 11, 10). d = (5, 0, −1), n = 5. For t = 0..4, −t/5 truncates to 0, so
(4..8, 11, 11) stay on layer 11. At t = 5, (9, 11, 10) is a step down: (9, 11, 11) open,
(9, 11, 10) stair.

Second line: 11 > 10 and 11 > 9, so it runs at b = 10. Third line: 11 < 3 + 2 − 1 = 4 **fails**, so
b = 12 is never cut. With a true y test (11 < 12 twice) the stray at b = 12 would appear whenever
draw 2 is odd, which is what your exchanged run showed. Alpha has no such cell.

Result: a = 9 at layer 10 is open at b = 10, 11 only, the recorded column, and truncation towards
zero is why the corridor stays on layer 11 until the very end. Rounding down instead would drop to
layer 10 at once.

### 6.4 The remaining two links and the entrance

* **Room 10 → room 1.** PA = (9,2,10), SA = (3,4,2); PB = (2,2,9), SB = (3,3,3). Along x. m = 3/2 + 2
  = 3. PB.x < PA.x: F = (9, 3, 10), T = (5, 3, 9), n = 4. Layer 10 a = 8, 7, 6, then t = 4 at
  (5, 3, 9) steps down: (5, 3, 10) open, (5, 3, 9) stair. Second line at b = 2 (3 > 2 twice). Third
  line at b = 4 (3 < 5, 3 < 4).
* **Room 13 → room 16.** PA = (10,9,10), SA = (2,4,1); PB = (10,17,10), SB = (2,4,3). Along y.
  m = 2/2 + 10 = 11. F = (11, 12, 10), T = (11, 16, 10), n = 4, flat. Second line at a = 10. Third:
  11 < 11 fails.
* **Entrance** (9, 1, 10). The recording shows an open cell there with a floor, and below and
  around it the foundation of §1.10.3 with flag 0x80: blocks from z = 144 up to z = 155, covering
  a = 8..10 and b = 0..2 in the pattern of box (X − 2, Y − 2, …, 14 × 14). Its top matches
  Htop = base + 10(c + 1) − 10 = 156. The cell (2, 1, 9) has no block carrying that flag.

### 6.5 The whole dungeon against the recording

The carve leaves bit 0x80 set in the type byte of every block of a carved cell, so the recording
gives the set of carved cells directly: all cells with at least 900 flagged blocks. Base 56 was used.

* Simulated by the rules above: 193 cells (125 room cells, 67 corridor cells, the entrance).
* Recorded: 194 cells.
* Simulated but not recorded: **none**.
* Recorded but not simulated: **(9, 1, 9) only**, which holds no air: it is the foundation under
  the entrance (§6.4), not a carved cell.

The stair flag checks too. Exactly 12 cells of the set hold 450 air blocks, the staircase of
§1.10.6 step 4, and they are exactly the cells the rule of §3 flags: (3,7,9), (4,7,9), (3,9,10),
(4,9,10), (5,2,9), (5,3,9), (5,4,9), (9,10,10), (9,11,10), (16,9,10), (16,10,10), (16,11,10).

**No listed cell is missed.** Your list treats stair cells as open, as this document does.

---

## 7. What stayed open

* **g0 at (127, 47).** The dungeon overwrites this column, so the recording cannot show the ground
  height there. The surrounding ground is consistent: the top block rises from z = 153 at y = 37
  to z = 154 at y = 39. The other three candidate columns of the fit stand higher: logical (1, 12)
  has its top block at z = 165 and (12, 20) at z = 167, while (20, 9) is covered by a tall column
  reaching z = 221. So rot 0 should win, and base 56 follows if g0 = 156. This needs one run of
  your generator with the entrance moved. If that run does not give rot 0 and base 56, something
  else is wrong in the fit.
* **Draw values where the test fails.** The second draw of links 1 → 4, 4 → 13 and 13 → 16 leaves
  no mark in the blocks, because its test fails. The evidence that draw 2 of link 4 → 13 is odd is
  your exchanged run only. If it were even, the recording would not separate the bound of §2.3
  from a true y bound for this link, though the code and the server are unambiguous.
* The bold bounds of §2.3 are otherwise untested by this recording: only the "B higher" along-x
  case failed a test here.

---

## 8. Corrections to earlier specs

| Spec | Statement | Correction |
|---|---|---|
| structures §1.4, v = 0, 1, 3 | Link lists | Every link's two rooms are exchanged. Use the table of §1.2 here. Room selection, draws, and the other tables stand. |
| structures §1.6 | "B lower" line runs from B's end to A's end | It runs from A's end to B's end, in both directions (§2.2). |
| structures §1.6 | Third line of the along-x case is "the same with x and y exchanged" | Its tests use x extents (§2.3). |
| structures §1.6 | On a climb the cell above opens and the current cell is flagged; on a descent the cell below | Reversed. Going down: the cell above opens and the current cell is flagged. Going up: the cell below opens and is flagged (§3). |
| structures §1.7 step 1, §1.9 | Reading of "first link's first room" with the old table | For v = 0, 1, 3 it is always room 10. The formula stands; the seed 41 cell is (9, 1, 10) (§4). |
