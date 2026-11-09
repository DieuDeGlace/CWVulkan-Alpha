# Alpha location creatures: camps, rings and groups (0x5dc6e0)

Behavioural specification of the routine that populates a **location zone** with creatures
during zone generation in the Cube World **Alpha** client (`Cube.exe`): client **0x5dc6e0**,
called once per zone at **0x5eace7**, between the location candidate grid and the wildlife
pass. It is written so that a reimplementation reproduces the zone's spawn list, the static
objects it adds and the zone's `rand()` stream bit for bit.

Sources: the Alpha decompile (`disassembled alpha cube world/decompiled/client`), the
per-instruction listing of 0x5dc6e0 and an independent disassembly of the binary for the
parts the listing drops (the decompiler treats `operator delete` as not returning and loses
the jumps after it, e.g. 0x5ddd6f … 0x5ddd94), constants read from `Cube.exe`, and a
constant-by-constant comparison with the server copy (**server 0x5104e0**, called at
server 0x51eac7; every immediate value and call pattern matches). No code was copied.

This document replaces the outline of `alpha-spawn-objects.md` §6.3 (see §9 for the
corrections to it). Related: `alpha-spawn-objects.md` §3 (finishing pass), §4 (draw counts of
the appearance and equipment generators), §6.2 (candidate grid);
`alpha-worldgen-structures.md` §2.2–§2.3 (the camp routine 0x5ad320 and the settle test
0x5f4da0); `alpha-worldgen-locations.md` §1.2, §6.3 (location records and types);
`alpha-worldgen-regions.md` §7 (f(L, x, y), 0x5f8e10); `alpha-materials-wildlife-routes.md`
§3–§4 (wildlife pass, behaviour objects); `alpha-creatures-behaviour.md` §9.3, §13;
`alpha-creatures-models.md` Appendix B (type numbers, flags, model names).

---

## 0. Conventions

* Addresses are `Cube.exe` virtual addresses (base 0x400000) unless marked **server**.
* **draw** = one call of the C runtime `rand()` (import slot 0x6fc468), result 0 … 32767.
  "d mod n" is the plain remainder. Where the binary uses a signed remainder, a bit mask or an
  unsigned division, the result is the same for non-negative draws; the instruction is
  cited where it matters.
* **fx** = 64-bit fixed point, 65536 per block. A position is (x, y, z) in fx.
* **Lz** = the zone's location record (`alpha-worldgen-locations.md` §1.2): +0x00 centre x,
  +0x08 centre y (fx), +0x18 type, +0x20 serial, +0x24 level, +0x28 sub-level.
* "Spawn +0x…" are offsets in the 0x10f0-byte spawn object (`alpha-spawn-objects.md` §2).
  Every spawn here is made by `operator new` + the constructor 0x5adb20 (no draws), so every
  field not listed below keeps the constructor's value.
* Type numbers are creature types; names are the model names of
  `alpha-creatures-models.md` Appendix B (e.g. 11 "orc m", 46 "ogre", 19 "collie").
* *As read* marks behaviour that looks unintended but is what the code does; *believed*
  marks readings resting on partial evidence.

---

## 1. Call and inputs

### 1.1 The call (0x5eaa24 … 0x5eacec)

The zone generator 0x5e4850 runs the candidate grid (`alpha-spawn-objects.md` §6.2) and then
calls 0x5dc6e0 **only when Lz's type is not 0, 1, 5, 10 or 14** (the same test that guards
the grid, 0x5eaa2d … 0x5eaa53). For those five types neither the grid nor this routine runs
and nothing is drawn. Otherwise 0x5dc6e0 is called even when the grid produced no
candidate.

Arguments (`__thiscall`, the callee pops 16 bytes, `ret 0x10` at 0x5df5a1):

| Argument | Value at 0x5eace7 | Use |
|---|---|---|
| this (ECX) | the world object of the zone generator | passed on to column lookup 0x4347a0, the settle test 0x5f4da0 and the equipment generator 0x5c7730 |
| 1 | the zone | +0x0c static-object vector, +0x18 spawn list, +0x60/+0x64 zone coordinates (zx, zy) |
| 2 | Lz | type, serial, level, sub-level, centre |
| 3 | the candidate list | vector of 24-byte fx points (x, y, z), in grid order (`alpha-spawn-objects.md` §6.2) |
| 4 | the constant 5 | **unused** (never read) |

Location types that reach the routine in a freshly generated world:
2, 3, 4, 6, 7, 11, 12 and 15 (`alpha-worldgen-locations.md` §6.3). Types 9 and 13 are
handled by the code but are never created by the region preparation.

### 1.2 Early return

If Lz's type (+0x18) is 0, return at once (0x5dc725): no draws, nothing written. At the
only call site this cannot happen (type 0 is filtered by §1.1).

### 1.3 What the routine writes

* **Spawns** appended to the zone's spawn list (zone + 0x18, a vector of spawn pointers,
  append 0x66add0).
* **Static objects** (camp fire and camp props) appended to the zone's static-object vector
  (zone + 0x0c, 0x188-byte records), by a plain vector append written inline (not through
  0x5a0e00).
* Nothing else in the zone. In particular it adds **no** point to the zone's object list
  (`alpha-worldgen-caves.md` §3.1), so the wildlife pass's 20-block clearance around camps
  (`alpha-materials-wildlife-routes.md` §3.1 step 7) does **not** see these camps (*as
  read*).

---

## 2. Set-up: the creature set

### 2.1 The set list and the choice (no draw)

A small list of **set numbers** is built from Lz's type T:

* T = 4 (lake): the list is **[9]**.
* Any other T: the list starts **[0, 1, 2, 3, 4, 5, 1, 7, 8]** (note the second **1** in
  position 6, *as read*: set 6 is not in this position); then **6** is appended unless T is
  13 or 2; then **10** is appended if T is 3.

| Lz type | List | Size |
|---|---|---|
| 4 | 9 | 1 |
| 2, 13 | 0 1 2 3 4 5 1 7 8 | 9 |
| 3 | 0 1 2 3 4 5 1 7 8 6 10 | 11 |
| 6, 7, 9, 11, 12, 15 (any other) | 0 1 2 3 4 5 1 7 8 6 | 10 |

The set is **list[serial mod size]** with serial = Lz +0x20 as an unsigned 32-bit value
(unsigned division, 0x5dc90e). No draw.

### 2.2 What a set holds

Every set fills three things:

* **R**, the **ring list**: creature types for ring creatures (§4.3).
* **G**, the **group list**: an ordered list of groups; each group is a pair of type lists,
  **A** (leader types) and **B** (follower types) (§4.4).
* **camp flag**: whether ring sites also get a camp fire and props (§4.3.1).

Built in this exact order (each line is one draw where marked; "d mod n → …" lists the value
for remainder 0, 1, 2, …; "—" means nothing is appended for that remainder):

| Set | Camp | R | Group 1 (A; B) | Group 2 (A; B) | Group 3 (A; B) |
|---|---|---|---|---|---|
| 0 (the default case: set number 0 and any number above 10) | yes | 11, 12 | A: 46; B: 19, 33, 26 | A: 11, 12; B: 11, 12 | — |
| 1 | yes | 15, 16 | A: **draw 1**, d mod 4 → 17, 41, 97, — ; B: **draw 2**, d mod 4 → (40, 37, 38, 39), 59, 41, 96 | A: 15, 16; B: 15, 16 | — |
| 2 | yes | 2, 3 | A: **draw 1**, d mod 3 → 94, 79, 82; B: **draw 2**, d mod 4 → 30, 26, 19, 33 | A: 2, 3; B: 2, 3 | — |
| 3 | yes | 9, 10 | A: **draw 1**, d mod 2 → 47, 88; B: **draw 2**, d mod 3 → 87, 91, 33 | A: 9, 10; B: 9, 10 | — |
| 4 | yes | 7, 8 | A: 7, 8; B: **draw 1**, d mod 3 → 60, 53, 58 | A: 7, 8; B: 7, 8 | — |
| 5 | yes | 4, 5 | A: 88; B: 87, 55, 60 | A: 4, 5; B: 4, 5 | — |
| 6 | no | 70, 69 | A: 71; B: 71 | A: 70, 69; B: 70, 69 | — |
| 7 | no | 52 | A: 62; B: 62 | A: 60; B: 60 | A: **60, 52**; B: 52 |
| 8 | no | 102, 105, 104 | A: 104; B: 104 | A: 102; B: 102 | A: 105; B: 105 |
| 9 | no | 86, 106 | A: 86; B: 86 | A: 106; B: 106 | — |
| 10 | no | 35, 90, 91 | A: 35; B: 35 | A: 90; B: 90 | A: 91; B: 91 |

Details that matter for parity:

* Lists keep the order shown; the type chosen later is list[d mod size].
* In set 1, draw 1 with d mod 4 = 3 appends **nothing**, so group 1's A list is **empty**
  (§4.4 step 2). Set 2's second draw is a remainder by 4 into a six-way table (0x5df5e8);
  its entries 4 → 31 and 5 → 32 are unreachable.
* Set 7's group 3 inherits the 60 of group 2 because only B is cleared between groups 2 and
  3 (0x5dd3da … 0x5dd3e0), *as read*: A = [60, 52].
* Sets 1–3 draw twice (A first, then B), set 4 once, all others not at all.

Set names (from the model names): 0 orcs with an ogre and dogs; 1 undead with skeleton,
frightener or vampire leaders and slimes, bats or zombies; 2 humans with werewolf, jester or
minotaur leaders and pets; 3 dwarves with rockling or biter leaders; 4 lizardmen with
insects or parrots; 5 goblins with a biter and moles, crows, flies; 6 plant creatures and
onionlings; 7 insectoids with mosquitoes and flies; 8 beetles; 9 squid and crabs;
10 bunnies, squirrels and raccoons.

### 2.3 Set-up draws

| Set | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Draws | 0 | 2 | 2 | 2 | 1 | 0 | 0 | 0 | 0 | 0 | 0 |

These are the first draws of the routine (after the 39 of the grid).

---

## 3. The marked candidate (no draws)

1. Lz's zone: bx = Lz.x / 65536 and by = Lz.y / 65536 (64-bit division towards zero,
   0x68d9e0), then zx_L = bx / 256 and zy_L = by / 256 (32-bit, towards zero).
2. If (zx_L, zy_L) differs from the zone's (zone +0x60, zone +0x64), **no candidate is
   marked** (marked = −1).
3. Otherwise, for each candidate in list order: f = f(Lz, cand.x, cand.y) (0x5f8e10 with the
   candidate's own fx x and y, **including** the half-block offset of 32768 the grid added;
   the grid itself evaluated f at the un-offset point), v = f32(1 − f32(f)),
   w = v·v (float) if v > 0, else 0. Keep the index of the **first** candidate with the
   largest w, starting from a running maximum of 0.0 and replacing only on a **strictly**
   larger w. If every w is 0, nothing is marked.

So at most one candidate per location is marked: the one deepest inside the location, in the
zone that contains the location's centre.

---

## 4. Per candidate

For each candidate *i* in list order (the count is re-read each time; it does not change),
with C = (X, Y, Z) its fx position:

### 4.1 Neighbours (no draws)

Walk the whole candidate list in order (C itself included) and collect into a local list N
every candidate P with 25 < d² < 16384, where dx = (P.x − X), dy = (P.y − Y) are converted
from 64-bit integers to float (x87 load, rounded to float), multiplied by 2⁻¹⁶ in float, and
d² = f32(dy² + dx²) (each square in float). So N holds the other candidates 5 to 128 blocks
away horizontally (z is ignored); C itself never qualifies. Only the group branch uses N.

### 4.2 Branch choice

1. If *i* is the marked candidate: **ring branch** (§4.3), no branch draw.
2. Otherwise **draw a**: if a is odd, **nothing** happens for this candidate (1 draw).
3. Otherwise **draw b**: if b is odd, **ring branch**; if b is even, **group branch** (§4.4).
   (2 branch draws.)

In both branches, if the needed list is empty the code falls through: an empty R sends the
ring branch to the group branch, an empty G ends the candidate. No set has an empty R or G,
so both are dead in practice.

### 4.3 Ring branch

#### 4.3.1 Camp fire and props (only for sets 0–5, the camp flag)

The **centre** starts as C.

1. **Fire**: a static-object record (§4.5) of kind **65**, scale (2.4, 2.4, 0.5) (float bits
   0x4019999a, 0x4019999a, 0x3f000000). **Draw**: rotation index = d mod 4.
2. For a = 0, 1, 2 (outer) and b = 0, 1, 2 (inner): put the fire at
   (X + 65536·a, Y + 65536·b, Z) and run the settle test 0x5f4da0 with the floor check on
   (`alpha-worldgen-structures.md` §2.3; no draws; it writes the settled z back). The first
   position that passes is **appended** to zone + 0x0c, and the **centre becomes the
   fire's settled position** (x, y and settled z). If none of the nine passes, nothing is
   added and the centre stays C.
3. **Props**: for a ∈ {0, 7} (outer) and b ∈ {0, 7} (inner), a record at
   (centre.x + 65536·a − 229376, centre.y + 65536·b − 229376, centre.z) (229376 = 3.5 blocks,
   the double at 0x71dbc8 truncated, 0x68d946), so ±3.5 blocks around the **centre**:
   **draw**: k = d mod 4 selects

   | k | Kind | Model | Scale |
   |---|---|---|---|
   | 0 | 66 | `tent` | (4, 4, 3) |
   | 1 | 16 | `stool` | (1, 1, 0.5) |
   | 2 | 12 | `table` | (3, 3, 1) |
   | 3 | 69 | `wood-mat` | (2, 2, 0.1) |

   then **draw**: rotation index = d mod 4. Settle test (floor check on); if it passes, the
   record is appended to zone + 0x0c at its settled height, else dropped. The draws happen
   either way.

Always **9 draws** (1 + 4 × 2), whether or not the fire found a place.

Differences from the camp routine 0x5ad320 (`alpha-worldgen-structures.md` §2.2), which
builds the same fire and prop ring: here there is **no roughness test**, a failed fire does
**not** stop the props, and the props are placed around the **settled fire** (0x5ad320 uses
the original point).

Without the camp flag (sets 6–10) there is no fire, no props, no draws, and the centre is C.

#### 4.3.2 The ring

1. **Draw**: φ = f32(d · 2π / 32767) (double multiply by 6.283185307179586, double divide by
   32767.0, rounded to float).
2. **Draw**: k = d mod 3 + 1 ring creatures.
3. For c = 0 … k − 1, in order:
   1. θ = f32(2c · π / k + φ) (all double: (2c as double) × 3.141592653589793 ÷ (k as
      double) + φ, rounded to float).
   2. New spawn (0x5de9a4).
   3. Controller kind (+0x28) := 1; flags (+0x7a) |= **0x1000**.
   4. Yaw (+0x54) := f32(f32(θ · 180) / π + 90) (the product in float, then double divide by
      π and add 90.0, rounded to float).
   5. Position (+0x10 … +0x27): y := centre.y + trunc64(f32(f32(f32(sin θ) · 3) · 65536)),
      x := centre.x + trunc64(f32(f32(f32(cos θ) · 3) · 65536)), z := centre.z (no
      settling). sin and cos are the CRT's double routines (0x68d9c2, 0x68d7a4) on θ
      widened to double; the truncation is 0x68d946 (towards zero). So the ring has radius 3
      blocks around the centre.
   6. **Draw**: type (+0x2c) := R[d mod |R|].
   7. Level (+0x34) := Lz +0x24; power tier (+0x58, a byte) := the low byte of Lz +0x28.
   8. Behaviour (+0x109c): a sequence (0x4dc2d0) of **Combat** (0x42c550, radius 20),
      **WalkPath** (0x58cfa0, radius 2.0) whose route is one point, the spawn's own
      position, **RandomInteraction** (0x4db100) and **RandomWalk** (0x4dbf10), in that
      order.
   9. **Boss**: if *i* is the marked candidate and c = 0: flags |= **0x200** and
      +0x10e8 := 1. (Written between the WalkPath and RandomInteraction objects, before any
      generation.)
   10. **Appearance**: the generator 0x43f7c0 with the type at +0x2c, the appearance at
       +0x74 and no choice record (`alpha-spawn-objects.md` §4.1).
   11. **Equipment**: 0x5c7730 on the spawn in **mode 0** (`alpha-spawn-objects.md` §4.2;
       class +0x30 is 0, so the general branch applies to types without flags 0x08/0x10).
   12. Append to zone + 0x18.

Ring draws: 2 + Σ (1 + appearance + equipment) over the k creatures (§5).

### 4.4 Group branch

1. **Draw**: g = d mod |G| (unsigned); the group (A, B) = G[g].
2. If A is empty (only set 1, group 1, when the set-up's first draw gave 3): nothing more
   for this candidate (1 draw in this branch).
3. **Leader** (0x5dedc4):
   1. Controller kind := 1; flags |= 0x1000.
   2. **Draw**: yaw := float(d mod 360) (a whole number of degrees).
   3. Position := C (the candidate itself; no settling, no camp in this branch).
   4. **Draw**: type := A[d mod |A|].
   5. Level := Lz +0x24; tier := low byte of Lz +0x28.
   6. Behaviour: a sequence of **Combat** (radius 20), **WalkPath** (radius 2.0) and
      **RandomWalk**, in that order. The WalkPath route gets, first, the leader's own
      position; then, **only if N is not empty**, three times:
      * **draw**: P = N[d mod |N|] (unsigned; picks may repeat);
      * look up P's column (0x4347a0 with P.x / 65536 and P.y / 65536, towards zero); if it
        does not exist, add nothing;
      * otherwise z := the column's +0x14 (as a block height), and while the block at
        (P.x, P.y, z) (0x42f860, floor division) has a type other than 0 (air) or 2
        (water), z += 1 block; add the point (P.x, P.y, z·65536).
      The z found is the same rule the grid used for P (`alpha-spawn-objects.md` §6.2
      step 4), so the point equals P whenever nothing changed the blocks in between.
   7. No appearance or equipment generation here. Append to zone + 0x18.
4. **Followers** (0x5df1d4), only if B is not empty (always, in every set):
   1. **Draw**: n = d mod 3 + 1.
   2. For each of the n followers, in order: new spawn; position := the leader's position
      (+0x10 … +0x27 copied); controller kind := 1; flags |= 0x1000; **draw**: type :=
      B[d mod |B|]; level and tier as the leader; yaw stays 0. Behaviour: a sequence of
      **Combat** (radius 20), **Companion** (0x43ab80; its followed id, +0x08/+0x0c, is a
      copy of the leader's id +0x48/+0x4c **at this moment**, which is still 0, because ids
      are only given by the finishing pass — *as read*, see open question LC-2) and
      **RandomWalk**. No generation. Append to zone + 0x18.

Group draws: 1 + (0 if A is empty, else 2 + (3 if N is not empty) + 1 + n).

### 4.5 Static-object records written here

Both the fire and the props are built on the stack field by field (not by the constructor
0x58eaa0) with: +0x00 kind; +0x08, +0x10, +0x18 x, y, z (fx); +0x20 rotation index;
+0x24, +0x28, +0x2c scale; +0x30 (byte) 1; +0x34, +0x38 0; +0x40 (8 bytes) 0; +0x48 … +0x50
an empty list; +0x54 0; +0x58 (u16) 0; +0x5c, +0x60 0; +0x64 (u16) 0; +0x66 (byte) 0;
+0x68 (u16) 1; +0x6c … +0x16b zero; +0x16c, +0x170, +0x174 0; +0x178, +0x17c, +0x180 −1;
+0x184 0. Bytes +0x04 … +0x07 and +0x3c … +0x3f are not written. The append copies the
record into the vector.

### 4.6 Spawn fields written, by kind

| Field | Ring creature | Group leader | Follower |
|---|---|---|---|
| +0x10 … +0x27 position | centre + (trunc(3 cos θ·65536), trunc(3 sin θ·65536), 0) | C | leader's position (= C) |
| +0x28 controller | 1 | 1 | 1 |
| +0x2c type | R[d mod \|R\|] | A[d mod \|A\|] | B[d mod \|B\|] |
| +0x34 level | Lz +0x24 | Lz +0x24 | Lz +0x24 |
| +0x54 yaw | θ·180/π + 90 | d mod 360 | 0 (constructor) |
| +0x58 tier | Lz +0x28 (low byte) | same | same |
| +0x7a flags | 0x1000, + 0x200 for the boss, + the type's flags (generator) | 0x1000 | 0x1000 |
| +0x10e8 | 1 for the boss | 0 | 0 |
| +0x109c behaviour | Combat 20, WalkPath 2 (own point), RandomInteraction, RandomWalk | Combat 20, WalkPath 2 (own point + up to 3 neighbour points), RandomWalk | Combat 20, Companion (id 0), RandomWalk |
| appearance, equipment | generated inline **and** again in the finishing pass | finishing pass only | finishing pass only |
| +0x30 class, +0x48 id, schedule, name | constructor values (class and id are set later by the generators and the finishing pass) | same | same |

None of the spawns gets a schedule (+0x10a0 list), a special action, a name or an id here.

---

## 5. Draw counts

### 5.1 Inline generation of ring creatures

Ring creature cost = 1 (type) + appearance + equipment (mode 0, first run).

Appearance (0x43f7c0 without a choice record): **5** for every ring type (none of 2–5, 7–12,
15, 16, 35, 52, 69, 70, 86, 90, 91, 102, 104–106 has per-type draws;
`alpha-spawn-objects.md` §4.1).

Equipment (0x5c7730, mode 0; `alpha-spawn-objects.md` §4.2, `alpha-creatures-behaviour.md`
§9.3): 6 seed draws always; + 2 for the boss; + the weapon and armour part by type:

| Ring types | Flags | Weapon part | Armour part | Equipment (non-boss) | Creature total (non-boss) | Boss |
|---|---|---|---|---|---|---|
| 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 15, 16 | none | class + specialization + class draws: warrior 4 (two-hander or one-hander + shield) or 5 (two one-handers), ranger 3, mage 3, rogue 3 | 6 + one more per jewel draw with d mod 10 = 0 (0–3) | 15 … 20 | **21 … 26** | +2 |
| 52 | 0x20 | as above, 3 … 5 | none (flag 0x20) | 9 … 11 | **15 … 17** | +2 |
| 35, 69, 70, 86, 90, 91, 102, 104, 105, 106 | 0x08 or 0x10 | none | none | 6 | **12** | +2 (14) |

### 5.2 One candidate

| Case | Draws |
|---|---|
| not marked, a odd | **1** |
| not marked, a even, b odd (ring) | 2 + camp + ring |
| not marked, a even, b even (group) | 2 + group |
| marked (ring, first creature boss) | camp + ring |

with camp = 9 for sets 0–5, 0 for sets 6–10;
ring = 2 + Σ over k ∈ {1, 2, 3} creatures of §5.1 (the marked candidate's first creature
+2); group = 1 (A empty) or 1 + 2 + (3 if N ≠ ∅) + 1 + n with n ∈ {1, 2, 3}.

Examples of group costs: N non-empty, 3 followers: 1 + 2 + 3 + 1 + 3 = **10**; N empty,
1 follower: 1 + 2 + 0 + 1 + 1 = **5**.

### 5.3 The routine

Total = set-up (§2.3) + Σ over candidates (§5.2). With an empty candidate list only the
set-up is drawn (0, 1 or 2 draws).

All draws of the routine come from 21 call sites: set-up 0x5dc97d, 0x5dc9d6 (set 1),
0x5dcb9b, 0x5dcbeb (set 2), 0x5dcd80, 0x5dcdc4 (set 3), 0x5dcf51 (set 4); branch 0x5ddd4b,
0x5ddd95; fire rotation 0x5ddee4; props 0x5de3fd, 0x5de4cb; ring 0x5de907, 0x5de92d,
0x5deaa4; group 0x5ded65, leader 0x5dedf1, 0x5dee55, waypoints 0x5defe3, followers
0x5df192, 0x5df230 — plus whatever 0x43f7c0 (0x5deced) and 0x5c7730 (0x5decfe) draw. None
of the other callees (0x5f8e10, 0x5f4da0, 0x4347a0, 0x42f860, the behaviour constructors,
the list helpers) draws.

### 5.4 Later cost of these spawns in the finishing pass

Every spawn made here also goes through the finishing pass (`alpha-spawn-objects.md` §3) in
list order. Leaders and followers are generated there for the first time. Ring creatures
are generated **a second time** (`alpha-spawn-objects.md` §3.3): the second equipment run
re-clears weapon and armour types but not the item levels left by the first run, so a
non-boss ring creature that received armour in either run keeps those slots at the spawn
level and pays one re-roll draw per such slot in step 6 of the pass when its level is ≥ 3
(`alpha-spawn-objects.md` §3.2, §3.3; which slots the generator clears is stated
differently in `alpha-spawn-objects.md` §4.2 and `alpha-creatures-behaviour.md` §9.3 —
see open question LC-7).

---

## 6. Order of appends and of the stream

Within 0x5dc6e0, zone + 0x18 receives, candidate by candidate in list order:

* ring branch: ring creatures c = 0 … k − 1 (each after its own inline generation);
* group branch: the leader, then its followers in order.

Static objects go to zone + 0x0c in the order fire, then the props that passed, (0, 0),
(0, 7), (7, 0), (7, 7) (a = x offset outer, b = y offset inner).

Around the routine, in the zone generator:

1. statue (0 / 2 / 3 draws, `alpha-spawn-objects.md` §6.1);
2. candidate grid (39 draws, §6.2 there), for the types of §1.1;
3. **0x5dc6e0** (this document);
4. the type-9 guardian (0x5ead31 … 0x5eae65; Lz type 9 only, never in fresh worlds;
   no draws; appends one spawn after this routine's spawns);
5. the 64 × 64 table scan (no draws);
6. the **wildlife pass** (0x5eaf75 …), whose leaders, plants and companions are appended
   **after** every spawn of this routine, and whose first draw directly follows this
   routine's last draw (or the guardian, which draws nothing).

So the wildlife pass starts at stream position (grid end) + set-up + Σ candidates, and the
spawn indices it uses in the finishing pass (ids = zone base + index) are shifted by the
number of spawns made here.

---

## 7. Worked example (illustrative draw values)

The draw *values* below are invented to show the mechanics; the counts and formulas are the
specified ones.

Lz: type 2 (hill), serial 641, level 13, sub-level 1 (record 27 of the seed-123 home region,
`alpha-worldgen-locations.md` §12.1). List size 9; 641 mod 9 = 2 → **set 2** (humans).
Assume its centre lies in this zone and the grid produced three candidates C0, C1, C2, with
C1 the deepest (marked); all three are within 128 blocks of each other.

Set-up (2 draws): d = 12345 → 12345 mod 3 = 0 → A₁ = [94]; d = 20002 → mod 4 = 2 →
B₁ = [19]. So R = [2, 3]; G = [(A [94]; B [19]), (A [2, 3]; B [2, 3])]; camp flag on.

C0 (not marked): draw a = 7, odd → nothing. **1 draw.**

C1 (marked, ring branch, no branch draws):

* Fire rotation 1 draw; the fire settles at (a, b) = (0, 0) one block lower than C1 → the
  centre is C1 − (0, 0, 65536). Props: 8 draws; say k = 0 (tent) at (−3.5, −3.5), and three
  more; each settles or is dropped. **9 draws.**
* φ: d = 8192 → φ = 1.570844292640686. k: d = 4 → 4 mod 3 + 1 = **2** creatures.
* c = 0: θ = 1.570844292640686; yaw = 180.00274658203125; offsets x −9, y +196608 (fx).
  Type draw d = 101 → R[1] = 3. Boss: flags 0x1000 | 0x200, +0x10e8 = 1. Appearance 5.
  Equipment: class, specialization, warrior choice 2 + one weapon subtype (4), armour 6 with
  no jewel hit, boss 2, seeds 6 → 18. **1 + 5 + 18 = 24 draws.**
* c = 1: θ = f32(π + φ) = 4.712437152862549; yaw = 360.0027770996094; offsets x +9,
  y −196608. Type draw → R[0] = 2. Appearance 5; equipment as a ranger with one jewel hit:
  3 + 7 + 6 = 16. **1 + 5 + 16 = 22 draws.**
* C1 total: 9 + 2 + 24 + 22 = **57 draws**; spawns appended: [3 (boss), 2].

C2 (not marked): a = 10 (even), b = 6 (even) → group branch. g: d mod 2 = 0 → group 1.
Leader: yaw draw, type draw → 94 (werewolf), N = {C0, C1} → 3 waypoint draws (route: own
point + three of C0/C1). Followers: d = 5 → 5 mod 3 + 1 = 3, three type draws → 19, 19, 19
(collies). **2 + 1 + 2 + 3 + 1 + 3 = 12 draws**; spawns appended: [94, 19, 19, 19].

Routine total: 2 + 1 + 57 + 12 = **72 draws**; zone + 0x18 gains, in order, 3 (boss), 2,
94, 19, 19, 19; zone + 0x0c gains the fire and up to four props. Later, in the finishing
pass, the two ring creatures are generated a second time and the four group spawns for the
first time.

Ring geometry check (exact): with d = 8192 and k = 2 the two creatures stand at
(centre.x − 9, centre.y + 196608) and (centre.x + 9, centre.y − 196608) fx, facing
180.0027° and 360.0028°.

---

## 8. Test vectors: set choice in the seed-123 home region

From the records of `alpha-worldgen-locations.md` §12.1 (named world). Only zones whose
type passes §1.1 are listed; creatures get the record's level and sub-level.

| Record | Type | Serial | List size | serial mod size | Set | Level | Sub-level |
|---|---|---|---|---|---|---|---|
| 5 | 12 | 764 | 10 | 4 | 4 lizardmen | 26 | 2 |
| 6 | 4 | 901 | 1 | 0 | 9 squid, crabs | 87 | 3 |
| 11 | 11 | 1041 | 10 | 1 | 1 undead | 301 | 4 |
| 14 | 3 | 806 | 11 | 3 | 3 dwarves | 34 | 3 |
| 22 | 3 | 711 | 11 | 7 | 7 insectoids | 21 | 3 |
| 27 | 2 | 641 | 9 | 2 | 2 humans | 13 | 1 |
| 31 | 6 | 845 | 10 | 5 | 5 goblins | 45 | 4 |
| 32 | 12 | 945 | 10 | 5 | 5 goblins | 141 | 2 |
| 35 | 2 | 495 | 9 | 0 | 0 orcs | 7 | 0 |
| 37 | 2 | 670 | 9 | 4 | 4 lizardmen | 16 | 1 |
| 53 | 6 | 320 | 10 | 0 | 0 orcs | 2 | 0 |

Game check: dump zone + 0x18 right after 0x5dc6e0 returns (break at 0x5eacec) for a zone of
record 27 and compare types, positions, yaw, flags and +0x10e8 with §4.6; hook `rand` and
count the calls from the 21 sites of §5.3.

---

## 9. Corrections to `alpha-spawn-objects.md` §6.3 and §7

* The routine is reached only for Lz types other than 0, 1, 5, 10, 14 (not "even with an
  empty list" for every type); its own type-0 return is redundant.
* Set-up draws: sets 1, 2 and 3 draw twice, set 4 once, the rest never (§2.3).
* Per candidate there are **two branch draws** (none for the marked candidate), and an odd
  first draw skips the candidate.
* 0x5dedc4 is the **group leader** (yaw d mod 360, type from A, WalkPath through up to three
  neighbour candidates), not a "single creature"; 0x5df1d4 makes its **followers**
  (count d mod 3 + 1, type from B, Companion behaviour). Neither is generated inline.
* The camp (fire + props, 9 draws) belongs to the ring branch and only to sets 0–5; it
  never fails as a whole; the ring stands around the settled fire.
* In the table of §7 there, order 8 should read: 0x5de9a4 (ring creatures, generated
  inline), 0x5dedc4 (group leaders), 0x5df1d4 (followers).

---

## 10. Notes for AlphaVulkan

* Keep the set tables of §2.2 as data (one record per set: camp flag, R, groups with A and
  B, and the set-up draws as "choose one of" entries), so a mod can add sets or change
  types without touching the routine; keep the set-list rule of §2.1 including the
  duplicated 1, behind the vanilla-parity default.
* The routine needs three hooks to stay faithful: the settle test, the appearance generator
  and the equipment generator, all already required elsewhere; run the two generators inline
  for ring creatures exactly as §4.3.2 steps 10–11, and leave them to the finishing pass for
  leaders and followers.
* Model the Companion id copy as written (leader id 0 at the time); a fixed mode that links
  followers to the leader's final id can be offered later behind a flag, since it does not
  change the stream.
* The camp statics are ordinary static objects (kinds 65, 66, 16, 12, 69); the "sit on
  stools and mats" behaviour of RandomInteraction (`alpha-trails-poses-schedules.md` §3.6)
  will find them.

---

## 11. Open questions

* **LC-1** *Game check of the stream*: hook `rand` on the generating thread in a type-2 or
  type-6 location zone and log return addresses between 0x5eace7 and 0x5eacec; the sequence
  must match §5 (set-up sites, then per candidate 0x5ddd4b [0x5ddd95], camp, ring or group
  sites, with 0x43f7c0 and 0x5c7730 sites inside each ring creature).
* **LC-2** Followers' Companion id: copied from the leader's +0x48/+0x4c before the
  finishing pass gives ids, so it is 0 (*as read*). Whether a creature with id 0 can ever be
  found at run time (making followers trail something) or whether Companion is simply never
  handled (followers then only fight and wander) is not settled; check with a running
  location zone (do followers stay near their leader?).
* **LC-3** The duplicated set 1 in list position 6 (where 6 would be expected) and set 7's
  inherited A = [60, 52] look unintended but are what the binary does (client and server
  agree); confirm by observing an Lz with serial mod size = 6 (should show undead).
* **LC-4** Unwritten bytes of the static records (§4.5) and whether the inline field values
  equal those of the constructor 0x58eaa0 used by other passes; a static dump comparison
  should skip +0x04 … +0x07 and +0x3c … +0x3f.
* **LC-5** Types 9 and 13 never occur in fresh worlds; whether stored location data can
  produce them (and so the [0 … 8, 6] list for type 9 and the 9-entry list for type 13) is
  open (`alpha-worldgen-locations.md` open question 3).
* **LC-6** Server parity was checked by constants and call patterns (server 0x5104e0); a
  byte-level comparison of the branch structure would close it.
* **LC-7** Exact re-roll cost of a ring creature in the finishing pass depends on which
  equipment slots 0x5c7730 clears and re-levels (slot 8 is "cleared" in
  `alpha-creatures-behaviour.md` §9.3 but "never touched" in `alpha-spawn-objects.md` §3.3);
  settle from 0x5c7730 before counting finishing-pass draws of ring creatures. Draws inside
  0x5dc6e0 itself are not affected.
