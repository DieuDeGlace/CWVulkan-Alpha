# Alpha open points, batch 8: the draws of a dungeon cell

Clean-room behavioural specification. It closes these points:

| Point | Spec it belongs to | Section here |
|---|---|---|
| Every draw of an interior cell, in order | `alpha-worldgen-structures.md` §1.10.6 | §1 |
| The wall sides, and what differs on +y | structures §1.10.6 step 7.7 | §2 |
| Torch 0x5f8960 and wall furniture 0x5f6e20 | structures §1.10.2 items 4, 5 | §3 |
| Whether inserting a static or a sprite draws, refuses or merges | structures §1.10.6 | §4 |
| Whether the window step misses anything | structures §1.10.6 step 3 | §5 |
| Worked example: cells (2,4,9), (2,4,10), (2,4,11) | | §6 |
| The whole dungeon against the recording | | §7 |

**The short version.** There is exactly one missing rule, and it makes the game draw **fewer**
numbers, not more:

1. **On the +y side (s = 0) the torch and the furniture are alternatives.** If the side's torch
   draw is odd, the side ends after the torch step. There is no furniture draw, no position draw,
   no furniture and no guard. The sides −x, +x and −y do both, one after the other.
2. At (2,4,9) that removes **5 draws** the port made (furniture-or-guard, position, three inside
   the furniture). The second stool on +y does not exist in Alpha. The drift was −5, so the
   inference "Alpha made at least one more draw" does not hold; §6.3 shows why one injected draw
   looked as if it helped.
3. §1.10.6 step 7.1 says the floor box costs 303 draws. A 10 × 10 × 1 box costs **103**. This is an
   error in the prose only (the formula of §1.10.2 item 1 is right).

Nothing else differs. Inserting a static or a sprite draws nothing and cannot be refused. The
window step is complete as written.

With rule 1, a replay of the raw stream reproduces the whole dungeon from the recording: every
floor colour, every rock/glow bit of every floor, all 191 window tests, both guards, the boss type,
and all 49 room spawns in order and position (§7).

Written from the Alpha client (`Cube.exe`) per-instruction listing of 0x5ceeb3..0x5d5f3a and of
every routine it calls. The decompiler's output was not used. The +y branch was compared with
`Server.exe`, builder 0x500300: the same two jumps exist there (0x506c8c, 0x506cc5). No code was
copied; addresses are citations only.

The check in §6 and §7 used the block and spawn data of the recording
`references/alpha-recordings/seed-41-dungeon/`, which is a game output, and nothing under `src`.

Conventions: as in the structures spec §0 and §1, cells (a, b, c), X, Y, Z, v, k, g, lv, sl, flag.
"Draw" is one call of the C library `rand` (import slot 0x6fc468). r is a position draw d & 3.

---

## 1. Every draw of a cell, in order

The cell loop is a = 0..21 (outer), b = 0..21, c = 0..21, only k ∈ {3, 4}. No step depends on rot
or mir except through the logical view of §1.5. Steps are numbered as in §1.10.6.

| Step | When it runs | Draws |
|---|---|---|
| 1 Exit marker | k = 4 | 0 (a temporary static record is also built and destroyed, 0x5d0b7a; no effect) |
| 2 Carve | always | 0 |
| 3 Windows | v ∈ {0, 1, 3}, k ≠ 4, stair flag clear | **4**, always, one per side (§5) |
| 4 Stairs | stair flag set | 693 for each of the four tests that holds; 0 otherwise |
| 5 | v = 4 and k = 4 | next cell |
| 6 | kind(a, b, c − 1) = 3 | go to step 8 |
| 7.1 Floor | | **103** (3 + 100) |
| 7.2 Carpet | v ∈ {0, 3}, floor-patch flag | 0 |
| 7.3 | k = 4: go to step 9. Stair flag: go to step 8 | 0 |
| 7.4 Trap | flag = 0 | 1 (d % 40). If it hits: 2 more (rotation, field +0x34), then go to step 8. flag ≠ 0: no draw at all |
| 7.5 | n0 = number of statics | 0 |
| 7.6 Ceiling | solid(kind(a, b, c + 1)) | see below |
| 7.7 Walls | always here | see §2 |
| 7.8 Loot | each static since n0 | as §1.10.6 step 7.8 |
| 8 Arches | k ≠ 4, kind(a, b, c + 1) ≠ 3 | 0, in mode 0 (the replay of §7 confirms it: arches are in the recording, and the stream stays right) |
| 8 Chandelier | same, v = 3 only | 1 (d % 10); no other draw |
| 9 Boss | mark flag set | 1 (kind) + 4 (rarity) + item generator + 1 (n) |

**Ceiling, step 7.6.** Skipped entirely if the cell above is not solid: no draw.

| v | Draws when it runs |
|---|---|
| 0, 3, 5 | 1, value has no effect |
| 4 | 1; if d % 3 = 0, 4 more for the liana (angle, size, y, x) |
| 2 | as v = 4, then the four cobweb sides |
| 1 | 1 (no liana, whatever the value), then the four cobweb sides |

Each cobweb side (order −x, +x, +y, −y) is 1 draw; if d % 3 = 0 and the neighbour is solid, 3 more
(size, q, kind). The order of size, q and kind is the same on all four sides.

**Order inside the boss step.** Kind; rarity (4, always; the force argument only replaces the
result, 0x5f8574); item generator; byte flags and marker; n; items, no draws. §1.10.6 step 9 omits
one thing that does not draw: +0x109c gets a new 0x14-byte object built with the value 20.0
(0x5d3c30..0x5d3c65).

---

## 2. The wall sides

The order is always **−x, +x, −y, +y**. It does not depend on anything. Each of the first three
sides is:

1. side draw; stop unless d % 3 = 0 and the neighbour at level c is solid;
2. torch draw; if odd: r (the along-wall coordinate), then the wall-block test, then, if the block
   is solid, the torch of §3 with its own draws;
3. **in every case**: furniture draw; if d & 7 ≠ 0: r, then the furniture of §3, inserted; if
   d & 7 = 0: guard, then r, then the inventory.

The **+y side differs in step 3 only**: step 3 runs **only if the torch draw was even**.

If the torch draw is odd, then after r and the wall test the side ends, whether or not the torch
was placed. Read at 0x5d2dab (even goes to the furniture at 0x5d2eda), 0x5d2e9c (wall block not
solid leaves the side) and 0x5d2ed5 (after the torch, leaves the side). The other three sides join
their furniture step in both cases (0x5d24a4, 0x5d2809, 0x5d2b6e). *As read*: it looks like a
slip in the original, but the client and the server have it, and the recording needs it.

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

Order of r for the guard: the guard's position is built after the record, and r is its only draw
before the inventory. The inventory comes after the position on all four sides, so the order is
the same.

The furniture routine itself takes s only as a number. **s = 0 draws exactly what the other sides
draw.** The difference is entirely in the pass.

The s passed: −x 1, +x 3, −y 2, +y 0 (0x5d24c3, 0x5d2828, 0x5d2b8d, 0x5d2ef9).

---

## 3. Torch and furniture

Both confirm §1.10.2 items 4 and 5.

**Torch 0x5f8960.** The style passed is the builder's v (0x5d2478). Jump table at 0x5f8ab8:

| t | Draws | Result |
|---|---|---|
| 0 | 1 | even: kind 50, z raised 2 blocks. Odd: lamp |
| 1, 2 | 1 | d % 3: 0 → 50, 1 → 49, 2 → 52 with tint |
| 3 | 0 | 48 |
| 4 | 1, or 2 if odd | odd: 44 + (second d & 3). Even: lamp |
| 5 | 0 | lamp |

So for v = 0, T = 1.

**Furniture 0x5f6e20.** The style passed is the byte zone + 0x79, not the pass's v (§8).

| First draw d % 50 | Second | Third | F |
|---|---|---|---|
| 0 | none | none | 1: chest; ±0.5 block, no further draw |
| ≠ 0 | d % 3 = 0, shelf | kind offset d % 3 | 3 |
| ≠ 0 | d % 3 = 1, table | offset 1 to 2 blocks | 3 |
| ≠ 0 | d % 3 = 2, stool | offset 0 to 1 block | 3 |

No branch depends on s except the direction of the offset (jump tables 0x5f73c8, 0x5f73d8,
0x5f73e8: s = 0 y −, s = 1 x +, s = 2 y +, s = 3 x −, as §1.10.2 says).

---

## 4. Inserting a static or a sprite

**Nothing draws and nothing is refused.** These calls can reach `rand` by no path, including
through their grow and copy helpers; the only indirect calls under them are the memory and error
imports.

| Routine | What it does |
|---|---|
| 0x5a0e00 (statics, 0x188 bytes each) | Append to the vector. If the argument lies inside the vector it remembers the index first, grows by one if full, then copies. No test on kind or position. |
| 0x4862e0 (sprites) | Append a node to the list. The size check only guards against overflow of the count. |
| 0x66add0 (creatures), 0x44d590 (ground items), 0x5f4ba0 (markers) | The same: plain appends. |
| 0x58ea10, 0x593e80 (static copy, destroy), 0x58e9f0, 0x58eaa0, 0x5adb20 (constructors) | No draws. |

There is no overlap or collision test, no merge. Two statics can occupy the same place. The only
functions in the pass that draw are: 0x5df600, 0x5f6e20, 0x5f8960, 0x5c9f90, 0x5f8530, 0x5f7a60,
0x5f6d50, and 0x5f0ce0 in the modes where markers are read.

---

## 5. The window step

§1.10.6 step 3 is complete. Read at 0x5d0b9a..0x5d126a:

* Side order −y, +y, −x, +x, fixed.
* The draw comes **first** on every side, before any test, so it is always 4 draws.
* The tests are parity, then neighbour kind = 0, then the test block not solid. They take no draws.
* The stair flag and k = 4 are tested once, before the first side. Stair cells make no window draws.
* The table of positions and rotations in §1.10.6 is right in every entry.

The test block is read at the moment of the window. A block below a column's bottom counts as solid
under the implicit read rules; a check against the recording must treat it so.

---

## 6. Worked example: (2,4,9), (2,4,10), (2,4,11)

### 6.1 Recovering the stream

The recording gives the stream itself. A floor shows its three table values directly: R of the
block at (i, j) in {0, 1}² minus the plain floor R (102) is d % 20 of the first draw, at
{2, 3}² the second, and at i ∈ {4, 5}, j ∈ {4, 5} the third. Floor (2,4,9) gives 9, 2, 14. A search of
all generator states for those values and your window bits 0, 1, 1, 1, followed by a replay forward,
leaves **one** state (bit 31 of the state never shows in the output, so states are given as their
low 31 bits). It is the state you can use as a test vector:

| Point in the stream | State, low 31 bits |
|---|---|
| Before the windows of (2,2,9), the first interior cell | 0x4EA1A83E |
| Before the windows of (2,4,9) (draw 248 of the pass) | 0x014F52B6 |

Generator: state ← state · 214013 + 2531011 (32-bit), draw = (state >> 16) & 0x7FFF.

### 6.2 Cell (2,4,9)

Numbering the draws from the first window draw of this cell:

| Draw | Value | Meaning | Result |
|---|---|---|---|
| 0–3 | 1618, 29875, 32255, 16065 | windows | 0, 1, 1, 1 (your list) |
| 4–6 | 6329, 24462, 20614 | floor table, % 20 | 9, 2, 14 |
| 7–106 | | floor cells | rock, except (i, j) = (1, 1) glowing |
| 107 | 30422 | trap, % 40 = 22 | none |
| | | ceiling: (2,4,10) is open | no draw |
| 108 | 12696 | −x side, % 3 = 0, (1,4,9) solid | |
| 109 | 6162 | even | no torch |
| 110 | 18967 | & 7 = 7 | furniture |
| 111 | 18077 | r = 1 | |
| 112 | 4222 | % 50 = 22 | not a chest |
| 113 | 20207 | % 3 = 2 | stool, kind 16, s = 1 |
| 114 | 28688 | stool offset | |
| 115 | 9541 | +x side, % 3 = 1 | |
| 116 | 29197 | −y side, % 3 = 1 | |
| 117 | 25578 | +y side, % 3 = 0, (2,5,9) solid | |
| 118 | 28001 | odd | torch |
| 119 | 15557 | r = 1 | wall block (X + 4, Y + 10, Z + 2) solid |
| 120 | 12401 | t = 0, odd | lamp, kind 54 |
| | | **+y side ends here** | |

This cell takes **121** draws. There is no loot, since a stool has none. (2,4,10) and (2,4,11) are
open below and open or arched above, so each is 4 window draws and nothing else, since v = 0 has no
chandelier.

### 6.3 Which rule gives Alpha's windows

| Draw | Value | Odd | Alpha uses it as | Port used it as |
|---|---|---|---|---|
| 121 | 82 | 0 | (2,4,10) −y | +y furniture test, & 7 = 2 |
| 122 | 24243 | 1 | (2,4,10) **+y** | r = 3 |
| 123 | 11634 | 0 | (2,4,10) −x | % 50 = 34 |
| 124 | 18365 | 1 | (2,4,10) +x | % 3 = 2, stool, s = 0 |
| 125 | 22713 | 1 | (2,4,11) −y | stool offset |
| 126 | 27611 | 1 | (2,4,11) **+y** | (2,4,10) −y |
| 127 | 28448 | 0 | (2,4,11) −x | (2,4,10) +y |
| 128 | 17025 | 1 | (2,4,11) +x | (2,4,10) −x |
| 129 | 14144 | 0 | (3,2,9) −y | (2,4,10) +x |
| 130 | 14602 | 0 | | (2,4,11) −y |
| 131 | 19850 | 0 | | (2,4,11) +y |

Alpha's window draws: (2,4,10): 0, 1, 0, 1; (2,4,11): 1, 1, 0, 1.

* (2,4,10): −y is open and +x is open, so those two bits show nothing. +y odd gives the hole. −x even
  gives none. That is Alpha's list.
* (2,4,11): the same neighbours, the same result. +y hole, no −x hole.

The port's draw list is the same stream with the 5 draws 121..125 consumed. Replaying the old rule
from this state gives exactly your list: stool, lamp, stool, then 1, 0, 1, 0 and 0, 0, 1, 1.

**Why one injected draw looked right.** It moves (2,4,10) to draws 127..130, whose +y bit (draw
128) happens to be odd and whose −x bit (draw 129) happens to be even. That is chance, and the next
cell shows it: (2,4,11) would read draws 131..134, where −x (draw 133) is odd.

### 6.4 The boss and the guards

The port's early guards came from the drifted stream. With the rule, the first creature made is the
boss at (11,19,11), and only two guards follow it, both later in the cell order:

| Index in Alpha's list | Creature | Cell, side |
|---|---|---|
| 0 | boss, type 12 at (146, 226, 167) in zone coordinates | (11,19,11); the kind draw 29817 is odd, K1[1] = 12 |
| 1 | guard 143 at (227, 123, 167) | (19,9,11), −y |
| 2 | guard 143 at (241, 128, 167) | (20,9,11), +x |

The class (+0x30) is not written in this pass. It is written by the finishing pass (spawn-objects
§4.2), so the wrong class followed from the drift or from whatever runs later. See §8.

---

## 7. The whole dungeon against the recording

A small independent replay of the rules above, reading only the recording and this spec, was run
from the state in §6.1 through all 193 cells and then §1.10.9. The torch wall test was read from
the recorded blocks. The boss's item generator is not specified, so the replay resynchronised over
it: from the end of the rarity draws to the start of (11,19,12) there are **137 draws** (generator
plus the n draw).

| Check | Result |
|---|---|
| Cells visited | 193 |
| Floors | 111 |
| Floors with no carpet over them: all three table values | 103 of 103 |
| Rock or glow bit of every block of those floors, g = 0.005 | 10,300 blocks, **no mismatch** (53 glowing blocks over all 111 floors) |
| Window sides with a closed neighbour: draw, kind and test block against the real hole | 191 sides, 34 holes, **no mismatch** |
| Guards | 2, the two of §6.4 |
| Traps | 1, at (19,11,11) |
| Chests, generator loot | none occur here |
| Room spawns, §1.10.9: type and position, in order | **49 of 49** |
| With the old +y rule, from every candidate state | no state passes even the second clean floor after (2,4,9) |

Draw totals, usable as test vectors:

| Stretch | Draws | State after, low 31 bits |
|---|---|---|
| (2,2,9), (2,2,10), (2,2,11) | 112, 4, 4 | |
| (2,3,9), (2,3,10), (2,3,11) | 120, 4, 4 | |
| (2,4,9), (2,4,10), (2,4,11) | 121, 4, 4 | |
| Whole interior pass, including the 137 | 21,542 | 0x5F99E90C |
| Room spawns, §1.10.9 | 133 | 0x030D5893 |

The +y rule fires twice in this dungeon: at (2,4,9) and at (5,11,11), both with the wall solid.

---

## 8. What stayed open

* **+y, torch odd, wall not solid.** The code is unambiguous (the jump at 0x5d2e9c), but this
  recording never does it. It has one failed wall test, at (7,2,10) on −y, and that side correctly
  goes on to its furniture.
* **v ≠ 0.** Only v = 0 was replayed. Ceiling, cobweb, chandelier and the torch styles other than
  t = 0 are read from the instructions, not tested.
* **Chests and generator loot.** Neither occurs here, so step 7.8 for chests, and the table-loot
  branches 4 and 5, are read and not tested. The generators 0x5f7a60 and 0x5f6d50 stay outside this
  document (structures §8, question 2). The boss's one item plus the n draw is 137 draws here.
* **The boss's class.** It comes after this pass. The stream state in §7 is the place to start
  checking it, but that pass was not replayed.
* **The furniture style.** Furniture reads the byte zone + 0x79, and the builder's v comes from the
  location entry's byte +1 (0x5e84a9). The call with flag = 1 passes v = 3 directly (0x5e84e3), so
  there the byte may differ from v. This changes the kinds only, not the draws, and nothing in the
  recording tests it.

---

## 9. Corrections to earlier specs

| Spec | Statement | Correction |
|---|---|---|
| structures §1.10.6 step 7.7 | Each of the four sides does the torch and then the furniture or guard | True for −x, +x, −y. On +y, the furniture or guard runs only if the torch draw was even (§2). |
| structures §1.10.6 step 7.1 | Floor: 303 draws | **103** (3 + 10·10·1). |
| structures §1.11 | "per interior cell … floor (303)" | Same: 103. The 303 of the v = 2 roof, a 10 × 10 × 3 box, stands. |
| structures §1.10.6 step 8 | Arch rotation on +y is 1, *inferred* | Confirmed: −x 0, +x 2, −y 3, +y 1 (0x5d3732, 0x5d37e2, 0x5d388d, 0x5d3938). |
| structures §1.10.6 step 9 | | Add: +0x109c gets a new 0x14-byte object with the value 20.0. No draws. |
| structures §1.10.2 items 4 to 8 | | No change. Items 1, 4, 5, 6 and 7 are confirmed; the rarity is always 4 draws, and the inventory of §1.10.2 item 7 held for both guards. |
| structures §1.10.6 step 3 | | No change. |
| structures §1.10.9 | | No change: 49 of 49 spawns. |
