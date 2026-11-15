Status: FINAL / STABLE / HELD

# Alpha open points, batch 7: dungeon corridors, link order and the entrance

Clean-room behavioural specification of native Alpha behaviour. It covers these points:

| Point | Related companion spec (pointer only) | Section here |
|---|---|---|
| Which room is A and which B; link order for v = 0, 1, 3 | `alpha-worldgen-structures.md` §1.4, §1.6 | §1 |
| The corridor routine (client RVA 0x1C52A0): overlap tests, m, clamp, end points, extra lines | structures §1.6 | §2 |
| The line routine (client RVA 0x1EF6D0) | structures §1.6 | §3 |
| The entrance room | structures §1.7 | §4 |
| Grid writes between the corridors and the fit | structures §1.7 to §1.9 | §5 |

**Summary.**

1. For v = 0, 1, 3 (flag = 0) each link's **first** room is the one nearer room 10 in the link
   list; every table starts from room 10 (§1.2).
2. On a z step of a line, the **stair flag always lands on the lower of the two cells**. Going
   down, the extra open cell is the one above the current cell; going up, the extra open cell is
   the one below it, and that extra cell is the flagged one (§3).
3. In a corridor along x, the third-line test compares the y coordinate m with **x** extents:
   one of its two bounds when B is lower, both when B is higher or equal (§2.3).
4. The entrance of v = 0, 1, 3 is always in room 10 (§4).

Evidence: the Alpha client image `Cube.exe` (image base 0x400000) per-instruction listing, and
the Alpha server image `Server.exe` (image base 0x400000). All addresses below are RVAs; unless a
server image is named, they are in `Cube.exe`. The server's corridor routine (server RVA 0x0F9010)
and line routine (server RVA 0x1234B0) match the client's instruction for instruction apart from
absolute addresses, and the link sites in the server builder (server RVA 0x100300; sites at server
RVA 0x100F64..0x101948) push their arguments the same way and with the same values as the client's.
Addresses are citations only.

Conventions: as in the structures spec §0 and §1. A cell is (a, b, c) and occupies 2 bytes: the
first byte is the kind, the second is the flag byte. "Kind 3" is open. The "stair flag" is the low
bit (value 0x01) of the flag byte, i.e. bit 0x100 of the 16-bit cell. All grid arithmetic is
32-bit signed integer. A "draw" is one call of the C runtime `rand` import (IAT slot RVA 0x2FC468);
"odd" means the signed remainder of the result by 2 is non-zero. The cell grid used here is
created at 0x1CDFD5 with size 22 × 22 × 22 and rotation 0, mirror 0 (constructor 0x1C38C0); its
rotation is first assigned at 0x1CE4F5, after the entrance write, so rot = mir = 0 for everything
in §1 to §4.

Room numbering: the room at link position (a, b, c), each 0..2, is room 9a + 3b + c. Its record is
28 bytes; byte 0 is the present flag and the following six 32-bit fields are p0..p5.

---

## 1. Which room is A and which is B

### 1.1 The call

The link loop (0x1CE206..0x1CE3ED) walks the link list from its first node to the end. A link node
holds two positions of three 32-bit integers: **first** at node + 0x08 and **second** at
node + 0x14. For each node:

1. If first's room is not present (test at 0x1CE246), or second's room is not present (test at
   0x1CE254), go to the next node: no call, no draws.
2. Box of each room from its record: P = (7a + 2 + p0, 7b + 2 + p1, 7c + 3 + p2),
   S = (p3 + 3, p4 + 3, p5 + 1).
3. Call the corridor routine with (P of first, S of first, P of second, S of second)
   (argument set-up and call 0x1CE3B0..0x1CE3DE).

So **A = the node's first room, B = the node's second room**.

### 1.2 Link tables for v = 0, 1, 3, flag = 0

The flag argument is tested at 0x1CCADF..0x1CCAE7; for flag = 0, v = 4 or 5 and v = 2 take their own
paths and every other v takes the path at 0x1CD0A4 (tests 0x1CCD08..0x1CCD23). That path draws once,
takes s = the signed remainder of the draw by 3, marks room 10 present (0x1CD0AE), and then builds the
links of the selected table.

These links are built with the small two-position constructor (0x1C3B70), which copies its first
argument to the link's first position and its second argument to the second position. Each site
fills two temporary positions, pushes the one at frame offset −0x34C and then the one at −0x340; the
last push is the first argument, so the temporary at **−0x340** is the link's first position. The
links are appended to the end of the list in site order.

| s | Sites | Links, in list order (first → second) | Rooms |
|---|---|---|---|
| 0 | 0x1CD174..0x1CD41B | (1,0,1)→(0,0,1), (1,0,1)→(2,0,1), (0,0,1)→(0,1,1), (0,1,1)→(1,1,1), (0,1,1)→(0,2,1), (1,1,1)→(1,2,1), (1,1,1)→(2,1,1), (2,1,1)→(2,2,1) | 10→1, 10→19, 1→4, 4→13, 4→7, 13→16, 13→22, 22→25 |
| 1 | 0x1CD8B1..0x1CDB58 | (1,0,1)→(1,1,1), (1,0,1)→(2,0,1), (1,1,1)→(0,1,1), (0,1,1)→(0,0,1), (0,1,1)→(0,2,1), (0,2,1)→(1,2,1), (1,2,1)→(2,2,1), (2,2,1)→(2,1,1) | 10→13, 10→19, 13→4, 4→1, 4→7, 7→16, 16→25, 25→22 |
| 2 | 0x1CD4E2..0x1CD7EA | (1,0,1)→(0,0,1), (1,0,1)→(2,0,1), (2,0,1)→(2,1,1), (0,0,1)→(0,1,1), (0,1,1)→(1,1,1), (0,1,1)→(0,2,1), (1,1,1)→(1,2,1), (1,1,1)→(2,1,1), (2,1,1)→(2,2,1) | 10→1, 10→19, 19→22, 1→4, 4→13, 4→7, 13→16, 13→22, 22→25 |

Every table starts from room 10, and each later link's first room is room 10 or the second room of
an earlier link. For s = 0 and s = 1 the links form a tree rooted at room 10; for s = 2 there are
nine links over nine rooms, and room 22 is the second room of two links (19→22 and 13→22).

The links of flag = 1, of v = 2 and of v = 4, 5 are written field by field into a temporary pair
(first position, then second) and appended in that order.

**Draw count.** The overlap tests of §2.1 are symmetric in A and B and the present test looks at both
rooms, so the number of corridor-stage draws does not depend on which room is A: 2 per link whose
rooms are both present and whose boxes overlap on x or y, 0 otherwise, in list order.

---

## 2. The corridor routine (0x1C52A0)

Arguments PA, SA, PB, SB. The routine reads only these grid coordinates and writes only through the
line routine (§3). Every line goes **from A towards B**: the line routine's first argument F is at A's
end and its second argument T at B's end.

### 2.1 Overlap tests and m

The x test comes first; the y test is reached only if the x test fails.

| Case | Test (both strict) | m, before the clamp | Clamp range |
|---|---|---|---|
| **Along y** (0x1C52C9..0x1C52D9) | PB.x < PA.x + SA.x **and** PA.x < PB.x + SB.x | SB.x/2 + PB.x | PA.x … PA.x + SA.x − 1 |
| **Along x** (0x1C54C9..0x1C54E5) | PB.y < PA.y + SA.y **and** PA.y < PB.y + SB.y | SB.y/2 + PB.y | PA.y … PA.y + SA.y − 1 |
| Neither | | no lines, **no draws** | |

The halving is signed division by 2 truncating towards zero. The clamp: if m is below the low bound,
m becomes the low bound; then if m ≥ PA + SA on that axis, m becomes PA + SA − 1. So m is taken from
the middle of **B** and kept inside **A**.

### 2.2 End points of the first line

z is always **PA.z at F** and **PB.z at T**.

| Case | Direction test | F | T |
|---|---|---|---|
| Along y, B lower | PB.y < PA.y | (m, PA.y, PA.z) | (m, PB.y + SB.y, PB.z) |
| Along y, B higher or equal | PB.y ≥ PA.y | (m, PA.y + SA.y − 1, PA.z) | (m, PB.y − 1, PB.z) |
| Along x, B lower | PB.x < PA.x | (PA.x, m, PA.z) | (PB.x + SB.x, m, PB.z) |
| Along x, B higher or equal | PB.x ≥ PA.x | (PA.x + SA.x − 1, m, PA.z) | (PB.x − 1, m, PB.z) |

F is the last cell inside A on the side facing B, and T is the first cell outside B on the side facing
A. In the "B lower" along-y case the first call (0x1C532A..0x1C5332) passes A's end as F; every other
call also passes A's end as F.

### 2.3 The two extra lines

Order, in every case once it is entered:

1. first line;
2. **draw**; if odd and the second-line test holds: second line;
3. **draw**; if odd and the third-line test holds: third line.

Both draws are always made. The tests are made after each draw and take no draws. The extra lines have
the same F and T as the first line except that the coordinate m is replaced by m − 1 for the second
line and by m + 1 for the third.

| Case | Second line (m − 1) needs | Third line (m + 1) needs | Third test |
|---|---|---|---|
| Along y, B lower | m > PA.x and m > PB.x | m < PA.x + SA.x − 1 and m < PB.x + SB.x − 1 | 0x1C53A2..0x1C53BC |
| Along y, B higher or equal | m > PA.x and m > PB.x | m < PA.x + SA.x − 1 and m < PB.x + SB.x − 1 | 0x1C5482..0x1C549C |
| Along x, B lower | m > PA.y and m > PB.y | m < PA.y + SA.y − 1 and **m < PB.x + SB.x − 1** | 0x1C55AB..0x1C55CD |
| Along x, B higher or equal | m > PA.y and m > PB.y | **m < PA.x + SA.x − 1 and m < PB.x + SB.x − 1** | 0x1C5682..0x1C569A |

All comparisons are strict and signed. The bold bounds are as the code has them: in the along-x case
m is a y coordinate, yet the third test compares it with x extents (one bound when B is lower, both
when B is higher or equal). The server routine does the same.

---

## 3. The line routine (0x1EF6D0)

Inputs F (first argument) and T (second argument). Integer arithmetic only.

1. d = T − F per axis. n = max(|dx|, |dy|, |dz|). If n = 0, **nothing is written**, not even F.
2. prev = F.z.
3. For t = 0, 1, …, n (both ends included):
   1. Position p = F + (d·t) / n per axis, where / is signed division truncating towards zero
      (0x1EF75A..0x1EF7AB). Because n is the largest component, z changes by at most 1 between
      consecutive steps, and at t = n the position is exactly T.
   2. The cell at p gets kind byte 3 (0x1EF85F), unconditionally, whatever kind it held; its flag
      byte is not touched.
   3. **If p.z < prev** (going down, test at 0x1EF865): the cell (p.x, p.y, p.z + 1) gets kind 3,
      then the cell p gets the stair flag (flag byte OR 0x01).
   4. **If p.z > prev** (going up, test at 0x1EF890): the cell (p.x, p.y, p.z − 1) gets kind 3 and
      then the stair flag.
   5. prev = p.z.

Details:

* The step test compares this step's integer z with the previous step's integer z; it does not use
  T.z, and x and y play no part. t = 0 never counts as a step because prev starts at F.z.
* In both directions the stair flag lands on the lower of the two cells and the upper one is open.
* The last step is not special.
* All writes go through the grid's view transform (rotation and mirror, both 0 here). A position
  outside the 22 × 22 × 22 grid resolves to a shared scratch cell, so the write is lost.
* The routine makes no draws.

---

## 4. The entrance

After the link loop, the first node's **first** position (a, b, c) gives the room, and the cell
(P.x, P.y − 1, P.z) of that room's box (§1.1) gets kind 4 (0x1CE3F3..0x1CE4DE). By the tables:

| Case | First position of the first link | Room |
|---|---|---|
| v = 0, 1, 3, flag = 0, any s | (1, 0, 1) | **10** |
| v = 2, flag = 0 | (1, 0, 2) | 11 |
| v = 4, 5, flag = 0 | (1, 1, 2) | 14 |
| flag = 1 | (1, 1, 1) | 13 |

Room 9, the position directly below room 10, is never marked present on the v = 0, 1, 3 path: no
instruction of that path writes its present flag.

---

## 5. Grid writes between the corridors and the fit

In 0x1CE4DE..0x1CEEB3 there are exactly two cell writes: the entrance kind 4 at 0x1CE4DE and the
raised-floor kind 2 at 0x1CE9CF. The other cell accesses in that range are reads: 0x1CE95C and
0x1CE97A (raised-floor scan, comparing with kind 4 and kind 0) and 0x1CEB29 (entrance search,
comparing with kind 4). No flag byte is written in that range.

---

## Re-certification (2026-11-14)

Re-checked against the `Cube.exe` and `Server.exe` instruction listings only.

**Confirmed:** link node layout (first at +0x08, second at +0x14); both present tests and the
skip-without-draw; box formulas P and S; argument order (PA, SA, PB, SB) with A = first; the
small constructor's argument-to-position mapping and the push order at every site; all three
v = 0, 1, 3 tables, link for link, with their site ranges, and the identical server tables; the s
dispatch by draw remainder 3 and room 10 marked present; both overlap tests, order, m, truncating
halving and the clamp; all four first-line end-point pairs and the F-at-A direction of every call;
the draw/test order of the extra lines and every second- and third-line bound, including the x
extents in the along-x third test; the line routine's n, n = 0 case, inclusive loop, truncating
division, kind write, both z-step rules and stair-flag placement, and absence of draws; the
entrance formula and the first-room table for all four cases; the two kind writes and three reads
in the post-corridor range; server corridor and line routines identical to the client's apart from
absolute addresses.

**Corrected:**
1. Addresses were absolute virtual addresses; now RVAs with the image named (e.g. 0x5c52a0 →
   client RVA 0x1C52A0; server 0x4f9010 → server RVA 0x0F9010).
2. Summary claimed the extra cell and the stair flag both belong to the lower cell on every z step;
   on a descent the extra open cell is the upper one and only the flag is on the lower (current)
   cell. §3 body was already right. No effect on generation; prose only.
3. "Each table is a tree" was wrong for s = 2 (nine links, room 22 reached twice); now stated per s.
4. Stair flag described as "bit 1 of the flag byte"; now the low bit (value 0x01).
5. "Including pit pillars" replaced by "unconditionally, whatever kind it held" (the companion
   meaning of pillars was not re-checked).
6. Third-test code ranges for the two along-y cases given per direction.
No correction reverses a rule or changes world generation or random-draw order.

**Removed:** statements about what earlier companion specs said and the cross-spec correction table
(not native behaviour; companions were not opened); the remark that the §1.5 fixed-room reset holds
for the boxes (depends on a companion); the worked example against a seed recording, its cell counts
and block heights, the entrance coordinate of that recording, the fit/g0 discussion and the "what
stayed open" list, all of which relied on owner capture data or addressed another implementation;
the note on which files were used for that check.

**Unresolved:**
* Initial value of the room present flags before the builder's table paths (only the absence of any
  write to room 9's flag on the v = 0, 1, 3 path was checked).
* Whether any v outside 0..5 reaches the v = 0, 1, 3 path in practice (the code sends every v other
  than 2, 4, 5 there).
* The parity of individual draws for any particular seed is not established here.
