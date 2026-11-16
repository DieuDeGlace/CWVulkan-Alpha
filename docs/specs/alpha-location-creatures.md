Status: FINAL / STABLE / HELD

# Alpha location creatures: camps, rings and groups (Cube.exe RVA 0x1dc6e0)

Behavioural specification of the routine that populates a **location zone** with creatures
during zone generation in the Cube World **Alpha** client image `Cube.exe`: VA 0x5dc6e0
(RVA 0x1dc6e0), called once per zone from the zone generator at VA 0x5eace7 (RVA 0x1eace7),
after the location candidate grid and before the pass that starts at VA 0x5eaf75. It
describes the zone's spawn list, the static objects the routine adds and the order of its
`rand()` draws.

Evidence: the original `Cube.exe` and `Server.exe` image bytes (constants, jump tables,
import tables and the instruction bytes skipped by the instruction listing after calls to
`operator delete`, e.g. VA 0x5ddd6f … 0x5ddd94), the per-instruction listing of both images
and the decompile used as a reading aid. The server copy is `Server.exe` VA 0x5104e0, called
at server VA 0x51eac7; its ordered sequence of stack-stored immediate constants is identical
to the client's and it makes the same number (21) of calls through its C-runtime `rand`
import (§5.3).

Related companion specifications (pointers only): `alpha-spawn-objects.md` §3 (finishing
pass), §4 (appearance and equipment generators), §6.2 (candidate grid);
`alpha-worldgen-structures.md` §2.2–§2.3 (camp routine VA 0x5ad320 and settle test
VA 0x5f4da0); `alpha-worldgen-locations.md` §1.2, §6.3 (location records and types);
`alpha-worldgen-regions.md` §7 (location field f, VA 0x5f8e10);
`alpha-materials-wildlife-routes.md` §3–§4; `alpha-creatures-behaviour.md` §9.3, §13;
`alpha-creatures-models.md` Appendix B (creature type numbers).

---

## 0. Conventions

* Addresses are `Cube.exe` virtual addresses (image base 0x400000, so RVA = VA − 0x400000)
  unless marked **server** (`Server.exe`, image base 0x400000).
* **draw** = one call of the C runtime `rand()` through the `MSVCR110.dll` import slot at
  VA 0x6fc468 (client; server slot VA 0x5582f4); result 0 … 32767. "d mod n" is the plain
  remainder. Where the binary uses a signed remainder, a sign-corrected bit mask or an
  unsigned division, the result is the same for non-negative draws.
* **fx** = 64-bit signed fixed point, 65536 per block. A position is (x, y, z) in fx.
* **Lz** = the zone's location record: +0x00 centre x (fx, 64-bit), +0x08 centre y (fx,
  64-bit), +0x18 type (32-bit), +0x20 serial (32-bit, used unsigned), +0x24 a 32-bit value
  copied to spawn +0x34, +0x28 a value whose low byte is copied to spawn +0x58. The companion
  specs call the last two the location's level and sub-level.
* "Spawn +0x…" are offsets in the 0x10f0-byte spawn object (RTTI type `cube::Spawn`). Every
  spawn here is made by `operator new(0x10f0)` and the constructor VA 0x5adb20, which makes no
  draws; every field not listed below keeps the constructor's value. Relevant constructor
  values: +0x28 = 1, +0x2c = 4, +0x30 (16-bit) = 0, +0x34 = 1, +0x48 and +0x4c = 0,
  +0x54 = 0.0, +0x109c = 0, +0x10e8 (byte) = 0.
* Field names (controller kind +0x28, creature type +0x2c, class +0x30, level +0x34,
  id +0x48/+0x4c, yaw +0x54, tier +0x58, appearance +0x74, flags +0x7a, behaviour +0x109c)
  follow `alpha-spawn-objects.md`; the offsets and values written are confirmed here.
* Behaviour objects are named by their RTTI type names: `cube::SequentialBehavior`
  (constructor VA 0x4dc2d0, 0xc bytes), `cube::CombatBehavior` (VA 0x42c550, 0x14 bytes),
  `cube::WalkPathBehavior` (VA 0x58cfa0, 0x1c bytes), `cube::RandomInteractionBehavior`
  (VA 0x4db100, 8 bytes), `cube::RandomWalkBehavior` (VA 0x4dbf10, 8 bytes) and
  `cube::CompanionBehavior` (VA 0x43ab80, 0x10 bytes). A Sequential object holds an ordered
  list of child behaviours; children are appended in the order stated.
* *As read* marks behaviour that looks unintended but is what the code does.

---

## 1. Call and inputs

### 1.1 The call (VA 0x5eaa24 … 0x5eacec)

The zone generator VA 0x5e4850 reads Lz's type and skips both the candidate grid and this
routine **when the type is 0, 10, 14, 1 or 5** (tests at VA 0x5eaa2d … 0x5eaa53, which jump
to VA 0x5ead31). For those five types nothing of this document happens and nothing is drawn.
For every other type the grid runs and 0x5dc6e0 is then called unconditionally, also when
the grid produced no candidate.

Arguments (`__thiscall`, callee pops 16 bytes, `ret 0x10` at VA 0x5df5a1):

| Argument | Value at VA 0x5eace7 | Use |
|---|---|---|
| this (ECX) | the zone generator's own `this` | passed as ECX to the column lookup VA 0x4347a0, the block lookup VA 0x42f860, the settle test VA 0x5f4da0 and the equipment generator VA 0x5c7730 |
| 1 | the zone | +0x0c static-object vector, +0x18 spawn-pointer vector, +0x60/+0x64 zone coordinates (zx, zy); also passed as an argument to 0x4347a0, 0x42f860 and 0x5f4da0 |
| 2 | Lz | type, serial, +0x24, +0x28, centre |
| 3 | the candidate list | vector of 24-byte fx points (x, y, z), in grid order |
| 4 | the constant 5 | never read |

### 1.2 Early return

If Lz's type (+0x18) is 0, the routine returns at once (VA 0x5dc725): no draws, nothing
written. At the only call site type 0 is already filtered out (§1.1).

### 1.3 What the routine writes

* **Spawns** appended to the zone's spawn-pointer vector (zone + 0x18) with the vector
  append VA 0x66add0.
* **Static objects** (camp fire and camp props) appended to the zone's static-object vector
  (zone + 0x0c, 0x188-byte records) by an inline vector append (growth through VA 0x594b40).
* Nothing else in the zone is written.

---

## 2. Set-up: the creature set

### 2.1 The set list and the choice (no draw)

A list of **set numbers** is built from Lz's type T (VA 0x5dc79d … 0x5dc8f3):

* T = 4: the list is **[9]**.
* Any other T: the list starts **[0, 1, 2, 3, 4, 5, 1, 7, 8]** (position 6, counting from
  0, holds a second **1**, *as read*); then **6** is appended unless T is 13 or 2; then
  **10** is appended if T is 3.

| Lz type | List | Size |
|---|---|---|
| 4 | 9 | 1 |
| 2, 13 | 0 1 2 3 4 5 1 7 8 | 9 |
| 3 | 0 1 2 3 4 5 1 7 8 6 10 | 11 |
| any other type that reaches the routine | 0 1 2 3 4 5 1 7 8 6 | 10 |

The set is **list[serial mod size]**, with serial = Lz +0x20 divided unsigned (VA 0x5dc90e).
No draw.

### 2.2 What a set holds

The set number selects a case through the jump table at VA 0x5df5b0 (sets 1 … 10, in order:
VA 0x5dc92f, 0x5dcb51, 0x5dcd32, 0x5dcecb, 0x5dd058, 0x5dd1cd, 0x5dd303, 0x5dd43b, 0x5dd5b7,
0x5dd6b5); set 0 and any other number take the default case at VA 0x5dd831.

Every set fills three things:

* **R**, the **ring list**: creature types for ring creatures (§4.3).
* **G**, the **group list**: an ordered vector of 24-byte groups; each group is a pair of
  type vectors, **A** (leader types, at +0x00) and **B** (follower types, at +0x0c) (§4.4).
* **camp flag** (a byte): whether ring sites also get a camp fire and props (§4.3.1).

Built in this exact order. "draw 1 / draw 2" are draws in the order shown; "d mod n → …"
lists the type appended for remainder 0, 1, 2, …; "—" means nothing is appended:

| Set | Camp | R | Group 1 (A; B) | Group 2 (A; B) | Group 3 (A; B) |
|---|---|---|---|---|---|
| 0 (default case) | yes | 11, 12 | A: 46; B: 19, 33, 26 | A: 11, 12; B: 11, 12 | — |
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

Details:

* Lists keep the order shown; a type chosen later is list[d mod size].
* In set 1, draw 1 with d mod 4 = 3 appends **nothing**, so group 1's A list is **empty**
  (§4.4 step 2).
* Set 2's second draw is reduced to d mod 4 and dispatched through a six-entry jump table
  (VA 0x5df5e8); its entries 4 (type 31) and 5 (type 32) are unreachable.
* Set 1's second draw uses the four-entry jump table at VA 0x5df5d8.
* Between groups, the A and B vectors are emptied after each group is copied into G, except
  between groups 2 and 3 of set 7, where only B is emptied (VA 0x5dd3da … 0x5dd3e0), so set
  7's group 3 has A = [60, 52] (*as read*).
* Sets 1–3 draw twice (A first, then B), set 4 once, all others never.

### 2.3 Set-up draws

| Set | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Draws | 0 | 2 | 2 | 2 | 1 | 0 | 0 | 0 | 0 | 0 | 0 |

These are the first draws of the routine.

---

## 3. The marked candidate (no draws)

1. Lz's zone: bx = Lz.x / 65536 and by = Lz.y / 65536 (64-bit signed division toward zero,
   VA 0x68d9e0, low 32 bits kept), then zx_L = bx / 256 and zy_L = by / 256 (32-bit signed,
   toward zero) (VA 0x5dd9d7 … 0x5dda39).
2. If zx_L differs from zone +0x60 or zy_L differs from zone +0x64, **no candidate is
   marked** (marked = −1).
3. Otherwise, for each candidate in list order: f = the location field VA 0x5f8e10 called
   on Lz with the addresses of the candidate's own fx x and y. These coordinates include the
   +32768 (half-block) offset the grid added when it stored the candidate (the grid subtracts
   the double −32768.0 at VA 0x702ad8, truncated, VA 0x5eabf3 … 0x5eac24), whereas the grid
   itself evaluated the field at the un-offset block corner. Then v = f32(1.0 − f32(f)) (the
   1.0 is the float at VA 0x745dc0), w = f32(v·v) if v > 0, else 0. The routine keeps the
   index of the **first** candidate with the largest w, starting from a running maximum of
   0.0 and replacing only on a **strictly** larger w. If every w is 0, nothing is marked.

So at most one candidate per location is marked, and only in the zone whose coordinates
equal those of the location's centre.

---

## 4. Per candidate

For each candidate *i* in list order (the candidate count is recomputed from the vector at
each step, VA 0x5df485; this routine does not change it), with C = (X, Y, Z) its fx position:

### 4.1 Neighbours (no draws)

Walk the whole candidate list in order (C itself included) and collect into a local list N
(of 24-byte points) every candidate P with 25 < d² < 16384, where:

* dx = P.x − X and dy = P.y − Y as 64-bit integers, each loaded by the x87 unit and rounded
  to float, then multiplied in float by 2⁻¹⁶ (the float at VA 0x6fcd94);
* d² = f32(f32(dy·dy) + f32(dx·dx));
* the bounds are the floats 25.0 (VA 0x6fce3c) and 16384.0 (VA 0x71df74), both strict.

z is ignored. C itself (d² = 0) never qualifies. Only the group branch uses N.

### 4.2 Branch choice

1. If *i* is the marked candidate: **ring branch** (§4.3), no branch draw.
2. Otherwise **draw a** (VA 0x5ddd4b): if a is odd, N is released and **nothing else**
   happens for this candidate (1 draw).
3. Otherwise **draw b** (VA 0x5ddd95): if b is odd, **ring branch**; if b is even,
   **group branch** (§4.4). (2 branch draws.)

If the list a branch needs is empty, the code falls through: an empty R sends the ring
branch to the group branch, and an empty G ends the candidate. No set has an empty R or G,
so neither fall-through is reachable.

### 4.3 Ring branch

#### 4.3.1 Camp fire and props (only when the camp flag is set: sets 0–5)

The **centre** starts as C.

1. **Fire**: a static-object record (§4.5) of kind **65**, scale (2.4, 2.4, 0.5) (float bits
   0x4019999a, 0x4019999a, 0x3f000000). **Draw** (VA 0x5ddee4): rotation index (+0x20) =
   d mod 4.
2. For a = 0, 1, 2 (outer loop, x offset) and b = 0, 1, 2 (inner loop, y offset): set the
   fire's position to (X + 65536·a, Y + 65536·b, Z) (z reset to Z on every try) and run the
   settle test VA 0x5f4da0 on it with the zone and the floor-check argument 1. The settle
   test makes no draws and adjusts the record's z in place. The first position that passes
   is **appended** to zone + 0x0c, the **centre becomes the fire record's x, y and settled
   z**, and the search stops. If none of the nine passes, nothing is added and the centre
   stays C.
3. **Props**: K = 229376 (the double 229376.0 at VA 0x71dbc8, converted toward zero by
   VA 0x68d946), i.e. 3.5 blocks. For p ∈ {0, 7} (outer loop, x) and q ∈ {0, 7} (inner loop,
   y), a fresh record at (centre.x + 65536·p − K, centre.y + 65536·q − K, centre.z), i.e. at
   ±3.5 blocks around the centre:
   **draw** (VA 0x5de3fd): k = d mod 4 selects

   | k | Kind | Scale |
   |---|---|---|
   | 0 | 66 | (4, 4, 3) |
   | 1 | 16 | (1, 1, 0.5) |
   | 2 | 12 | (3, 3, 1) |
   | 3 | 69 | (2, 2, 0.1) (float bits of 0.1: 0x3dcccccd) |

   then **draw** (VA 0x5de4cb): rotation index = d mod 4. Then the settle test (zone, floor
   check 1); if it passes, the record is appended to zone + 0x0c at its settled height,
   else it is dropped. Both draws happen either way.

Always **9 draws** (1 + 4 × 2), whether or not the fire found a place.

Without the camp flag (sets 6–10) there is no fire, no props, no draws, and the centre is C.

#### 4.3.2 The ring

1. **Draw** (VA 0x5de907): φ = f32(d × 6.283185307179586 ÷ 32767.0), computed in double
   (constants: doubles at VA 0x702a78 and VA 0x702a80), rounded to float.
2. **Draw** (VA 0x5de92d): k = (d mod 3) + 1 ring creatures.
3. For c = 0 … k − 1, in order:
   1. θ = f32((2c as double) × π ÷ (k as double) + φ), all in double (π is the double
      3.141592653589793 at VA 0x6fce08), rounded to float.
   2. New spawn (constructor call at VA 0x5de9a4).
   3. +0x28 := 1; +0x7a |= **0x1000**.
   4. +0x54 := f32(f32(θ × 180.0) ÷ π + 90.0): the product in float (180.0 is the float at
      VA 0x6fce5c), then widened, divided by π and increased by the double 90.0
      (VA 0x71dc78) in double, rounded to float.
   5. Position (+0x10 … +0x27): y := centre.y + T(sin θ), x := centre.x + T(cos θ),
      z := centre.z (no settling), where T(s) = the 64-bit truncation toward zero
      (VA 0x68d946) of f32(f32(f32(s) × 3.0) × 65536.0) (floats at VA 0x6fcd70 and
      VA 0x6fce7c). sin and cos are the C-runtime double routines imported at VA 0x68d9c2
      (sin) and VA 0x68d7a4 (cos), applied to θ widened to double; sin is evaluated first.
      The ring therefore has radius 3 blocks around the centre.
   6. **Draw** (VA 0x5deaa4): +0x2c := R[d mod |R|] (unsigned).
   7. +0x34 := Lz +0x24; +0x58 (byte) := the low byte of Lz +0x28.
   8. +0x109c := a new Sequential behaviour whose children are, in order: **Combat**
      (constructed with the float 20.0), **WalkPath** (constructed with the float 2.0) whose
      route holds one point, the spawn's own position (+0x10 … +0x27), **RandomInteraction**
      and **RandomWalk**.
   9. **Boss**: if *i* is the marked candidate and c = 0: +0x7a |= **0x200** and +0x10e8
      (byte) := 1. This is written after the WalkPath child is added and before the
      RandomInteraction child, before either generator runs.
   10. **Appearance**: VA 0x43f7c0 called (VA 0x5deced) with the addresses of +0x2c and
       +0x74 and a null third argument.
   11. **Equipment**: VA 0x5c7730 called (VA 0x5decfe) as a method of the routine's `this`,
       with the spawn and the argument 0.
   12. Append to zone + 0x18.

Ring draws: 2 + Σ over the k creatures of (1 + appearance draws + equipment draws) (§5.1).

### 4.4 Group branch

1. **Draw** (VA 0x5ded65): g = d mod |G| (unsigned); the group (A, B) = G[g].
2. If A is empty (only set 1, group 1, when the set-up's first draw gave 3): N is released
   and nothing more happens for this candidate (1 draw in this branch).
3. **Leader** (constructor call at VA 0x5dedc4):
   1. +0x28 := 1; +0x7a |= 0x1000.
   2. **Draw** (VA 0x5dedf1): +0x54 := float(d mod 360) (a whole number).
   3. Position := C (the candidate itself; no settling and no camp in this branch).
   4. **Draw** (VA 0x5dee55): +0x2c := A[d mod |A|] (unsigned).
   5. +0x34 := Lz +0x24; +0x58 := low byte of Lz +0x28.
   6. +0x109c := a Sequential behaviour with children, in order: **Combat** (20.0),
      **WalkPath** (2.0) and **RandomWalk**. The WalkPath route first receives the leader's
      own position; then, **only if N is not empty**, three iterations, each:
      * **draw** (VA 0x5defe3): P = N[d mod |N|] (unsigned; picks may repeat);
      * look up P's column (VA 0x4347a0 with P.x / 65536 and P.y / 65536, 64-bit signed
        division toward zero, low 32 bits, and the zone); if there is none, add nothing
        (the iteration still counts);
      * otherwise start with z := the column's 32-bit field +0x14 (a block height) and,
        while the block at (P.x, P.y, z·65536) (VA 0x42f860) has a kind — the low five bits
        of its byte +3 — other than 0 or 2, increase z by one block; then add the point
        (P.x, P.y, z·65536).
      VA 0x42f860 converts each fx coordinate to a block index by subtracting one block when
      the coordinate is negative and then dividing by 65536 toward zero. For non-negative
      coordinates this column and block rule is the same one the grid used to give P its z,
      so the added point equals P when the blocks have not changed in between.
   7. No appearance or equipment generation. Append to zone + 0x18.
4. **Followers**, only if B is not empty (true for every group of every set):
   1. **Draw** (VA 0x5df192): n = (d mod 3) + 1.
   2. For each of the n followers, in order: new spawn (constructor call at VA 0x5df1d4);
      position := a copy of the leader's +0x10 … +0x27; +0x28 := 1; +0x7a |= 0x1000;
      **draw** (VA 0x5df230): +0x2c := B[d mod |B|] (unsigned); +0x34 and +0x58 as for the
      leader; +0x54 stays 0.0. +0x109c := a Sequential behaviour with children **Combat**
      (20.0), **Companion** and **RandomWalk**, in that order. The Companion object's
      +0x08/+0x0c receive a copy of the leader's +0x48/+0x4c **at this moment**, which still
      hold the constructor's 0 (*as read*; see U5). No generation. Append to zone + 0x18.

The leader is appended before its followers are made.

Group draws: 1 + (0 if A is empty, else 2 + (3 if N is not empty) + 1 + n).

### 4.5 Static-object records written here

The fire and each prop are assembled on the stack field by field (no constructor call)
with: +0x00 kind; +0x08, +0x10, +0x18 x, y, z (fx, 64-bit each); +0x20 rotation index;
+0x24, +0x28, +0x2c scale (floats); +0x30 (byte) 1; +0x34, +0x38 0; +0x40 (8 bytes) 0;
+0x48, +0x4c, +0x50 0 (an empty list); +0x54 0; +0x58 (16-bit) 0; +0x5c, +0x60 0;
+0x64 (16-bit) 0; +0x66 (byte) 0; +0x68 (16-bit) 1; +0x6c … +0x16b zero (`memset` of 0x100
bytes); +0x16c, +0x170, +0x174 0; +0x178, +0x17c, +0x180 −1; +0x184 0. Bytes +0x04 … +0x07
and +0x3c … +0x3f are not written. The append copies the record field by field into the
vector's new 0x188-byte slot (the list at +0x48 and the block at +0x58 through their copy
routines VA 0x4593e0 and VA 0x40ee70); it does not copy +0x04 … +0x07 or +0x3c … +0x3f
either.

### 4.6 Spawn fields written, by kind

| Field | Ring creature | Group leader | Follower |
|---|---|---|---|
| +0x10 … +0x27 position | centre + (T(cos θ), T(sin θ), 0) | C | copy of the leader's (= C) |
| +0x28 | 1 | 1 | 1 |
| +0x2c type | R[d mod \|R\|] | A[d mod \|A\|] | B[d mod \|B\|] |
| +0x34 | Lz +0x24 | Lz +0x24 | Lz +0x24 |
| +0x54 | θ·180/π + 90 | d mod 360 | 0.0 (constructor) |
| +0x58 | low byte of Lz +0x28 | same | same |
| +0x7a flags set here | 0x1000, plus 0x200 for the boss | 0x1000 | 0x1000 |
| +0x10e8 | 1 for the boss | 0 (constructor) | 0 (constructor) |
| +0x109c children | Combat 20, WalkPath 2 (own point), RandomInteraction, RandomWalk | Combat 20, WalkPath 2 (own point + up to 3 neighbour points), RandomWalk | Combat 20, Companion (copied id 0), RandomWalk |
| generators called here | appearance VA 0x43f7c0, equipment VA 0x5c7730 | none | none |

The routine writes no other spawn field directly; +0x30, +0x48/+0x4c and the remaining
fields keep the constructor values except for what the two generators write in ring
creatures.

---

## 5. Draw counts

### 5.1 Ring creatures

Ring creature cost = 1 (type) + the draws of the appearance generator VA 0x43f7c0 (called
without a choice record) + the draws of the equipment generator VA 0x5c7730 (argument 0) for
that spawn. Both generators are specified in `alpha-spawn-objects.md` §4 and
`alpha-creatures-behaviour.md` §9.3; their per-type counts are not re-established here (U1).

### 5.2 One candidate

| Case | Draws |
|---|---|
| not marked, a odd | **1** |
| not marked, a even, b odd (ring) | 2 + camp + ring |
| not marked, a even, b even (group) | 2 + group |
| marked (ring, first creature boss) | camp + ring |

with camp = 9 for sets 0–5 and 0 for sets 6–10; ring = 2 + Σ over the k ∈ {1, 2, 3}
creatures of §5.1; group = 1 (A empty) or 1 + 2 + (3 if N ≠ ∅) + 1 + n with n ∈ {1, 2, 3}.

Examples of group costs: N non-empty, 3 followers: 1 + 2 + 3 + 1 + 3 = **10**; N empty,
1 follower: 1 + 2 + 0 + 1 + 1 = **5**.

### 5.3 The routine

Total = set-up (§2.3) + Σ over candidates (§5.2). With an empty candidate list only the
set-up is drawn (0, 1 or 2 draws).

The routine's own draws come from exactly 21 call sites (a byte scan of VA 0x5dc6e0 …
0x5df5ad finds no others): set-up VA 0x5dc97d, 0x5dc9d6 (set 1), 0x5dcb9b, 0x5dcbeb
(set 2), 0x5dcd80, 0x5dcdc4 (set 3), 0x5dcf51 (set 4); branch 0x5ddd4b, 0x5ddd95; fire
rotation 0x5ddee4; props 0x5de3fd, 0x5de4cb; ring 0x5de907, 0x5de92d, 0x5deaa4; group
0x5ded65; leader 0x5dedf1, 0x5dee55; waypoints 0x5defe3; followers 0x5df192, 0x5df230. In
addition the appearance and equipment generators draw inside each ring creature (called
from VA 0x5deced and 0x5decfe). The other callees (the location field VA 0x5f8e10, the
settle test VA 0x5f4da0, the column lookup VA 0x4347a0, the block lookup VA 0x42f860, the
spawn and behaviour constructors and the container helpers) contain no `rand` call.

---

## 6. Order of appends and of the stream

Within 0x5dc6e0, zone + 0x18 receives, candidate by candidate in list order:

* ring branch: ring creatures c = 0 … k − 1, each after its own inline generation;
* group branch: the leader, then its followers in order.

Static objects go to zone + 0x0c in the order: fire, then the props that passed, in the
order (p, q) = (0, 0), (0, 7), (7, 0), (7, 7) (p = x offset, outer; q = y offset, inner).

Around the routine, in the zone generator VA 0x5e4850:

1. the candidate grid (VA 0x5eaa59 … 0x5eacd5), for the types of §1.1;
2. **0x5dc6e0** (this document), then release of the candidate list;
3. the type-9 guardian (VA 0x5ead31 … 0x5eae65): only when Lz's type is 9 **and** Lz's
   centre zone, computed as in §3 step 1, equals zone +0x60/+0x64 (VA 0x5ead41 … 0x5eadb7);
   no draws; appends one spawn (VA 0x5eae65) after this routine's spawns;
4. a 64 × 64 scan (VA 0x5eaea0 … 0x5eaf37) and a call to VA 0x5c23b0 (VA 0x5eaf6d), neither
   containing a `rand` call;
5. the pass beginning at VA 0x5eaf75, whose first draw (VA 0x5eaf9c) is therefore the next
   draw of the stream after this routine's last draw.

---

## 7. Unresolved

* **U1** Appearance and equipment draw counts per ring type (the earlier figures: 5
  appearance draws for every ring type; 6 seed draws plus 2 for a boss; weapon and armour
  parts by type flags 0x08/0x10/0x20, giving ring-creature totals of 21 … 26, 15 … 17 and
  12) depend on VA 0x43f7c0 and VA 0x5c7730 and the creature-type flag table; they were not
  re-established in this re-certification.
* **U2** The later handling of these spawns in the finishing pass (second generation of ring
  creatures, first generation of leaders and followers, re-roll costs of equipment slots,
  id assignment as zone base + list index) belongs to `alpha-spawn-objects.md` §3 and was
  not re-checked; the slot-clearing difference noted between `alpha-spawn-objects.md` §4.2
  and `alpha-creatures-behaviour.md` §9.3 remains open.
* **U3** Model names and labels: the creature model names for the type numbers (e.g. 11, 46,
  19), the descriptive set labels (orcs, undead, humans, dwarves, lizardmen, goblins, plant
  creatures, insectoids, beetles, squid and crabs, bunnies) and the static-object model
  names for kinds 65, 66, 16, 12 and 69 (fire, tent, stool, table, wood mat) come from
  companion tables and were not confirmed here.
* **U4** Which location types occur in a freshly generated world (earlier text: 2, 3, 4, 6,
  7, 11, 12, 15; types 9 and 13 never created) and whether stored data can produce types 9
  or 13 belong to `alpha-worldgen-locations.md` and were not re-checked.
* **U5** Runtime effect of the followers' Companion target id being 0 when copied: whether
  any creature can match it or whether followers only fight and wander.
* **U6** Whether the inline static-record values of §4.5 equal those produced by the
  static-object constructor VA 0x58eaa0 used elsewhere, and what the uncopied bytes
  +0x04 … +0x07 and +0x3c … +0x3f of the vector slot contain.
* **U7** Server parity was established from the identical ordered stack immediates and the
  21 `rand` import calls of server VA 0x5104e0; a full instruction-level comparison of the
  branch structure was not made.
* **U8** Differences from the camp routine VA 0x5ad320 (earlier text: that routine has a
  roughness test, stops the props when the fire fails and places props around the original
  point) were not re-checked.
* **U9** Zone-generator context: the draw count of the candidate grid (earlier text: 39),
  the statue step before it (0 / 2 / 3 draws), the identity of the pass at VA 0x5eaf75 as the
  wildlife pass, and the consequence that that pass's 20-block camp clearance does not see
  these camps (this routine adds nothing to the zone's object list) were not re-checked.
* **U10** The meanings of the float constructor arguments 20.0 (Combat) and 2.0 (WalkPath)
  as radii, and of the block kinds 0 and 2 as air and water, are companion interpretations.

---

## Re-certification (2026-11-15)

Evidence used: `Cube.exe` and `Server.exe` image bytes (section headers, import tables,
constants, jump tables, skipped instruction bytes), the per-instruction listings of client
VA 0x5dc6e0 … 0x5df5a9 and of the relevant part of VA 0x5e4850, the server listing of
VA 0x5104e0, and the decompile as a reading aid (callees VA 0x5adb20, 0x5f4da0, 0x42f860,
0x68d946 and the behaviour constructors). The Infinite-Cubes reconstruction was not
consulted.

**Confirmed:** call site, guard types and argument layout; unused fourth argument; type-0
return; set-list rule and unsigned selection; the jump-table mapping of sets 1–10 and the
default case; every type number, its A/B/R target and the camp flag of every set; the draw
order and reductions of the set-up (including the unreachable set-2 entries and set 7's
inherited A); zone match and marked-candidate rule with its constants; the grid's +32768
candidate offset; neighbour test constants and arithmetic; branch draws and fall-throughs;
fire and prop kinds, scales, offsets (229376), loop orders, centre update and 9 draws; ring
φ, k, θ, yaw and position arithmetic with all constants; boss flags and their write point;
behaviour children and their order (RTTI names); generator calls; leader and follower
fields, waypoint rule and draws; static-record field values; the 21 draw sites; server call
site, immediates and draw-site count; type-9 guardian and the next draw site.

**Corrected:**
1. Type-9 guardian condition: old "Lz type 9 only"; new: type 9 and Lz's centre zone equal to
   the zone's coordinates.
2. Block lookup VA 0x42f860: old "floor division"; new: negative coordinates are reduced by
   one block before division toward zero.
3. Waypoint point equals the grid's P: old unconditional; new: for non-negative coordinates.
4. Waypoint block test: old "type other than 0 (air) or 2 (water)"; new: low five bits of
   the block's byte +3 other than 0 or 2 (labels moved to U10).
5. Argument table: the zone is also an explicit argument of the column, block and settle
   lookups; ECX is described as the zone generator's own `this` (old "world object").
6. Static-record append: added that the copy also skips +0x04 … +0x07 and +0x3c … +0x3f.

None of the corrections changes world-generation results or the random-draw order stated
earlier; correction 1 narrows when the guardian spawn exists (no draws involved).

**Removed:** §7 worked example (invented draw values); §8 seed-123 set-choice table (record
data from a companion spec) and its runtime check procedure; §9 corrections addressed to
another specification (its native facts remain in §1–§6); §10 notes addressed to a
replacement; open questions LC-1 (stream-logging procedure) and the observation procedures
of LC-2, LC-3 and LC-6; the "replaces §6.3" note; set names and model names from the body
(now U3); the appearance/equipment count table and finishing-pass cost section (now U1, U2);
the camp-routine comparison (now U8); the statue and grid draw counts and wildlife-pass
consequences (now U9).

**Unresolved:** U1 … U10 above.
