Status: FINAL / STABLE / HELD

# Alpha open points, batch 3: creature action pools, multi-hit sounds, flame cubes, pose leftovers

Clean-room behavioural specification. It closes these open points of earlier specs:

| Point | Spec it belongs to | Section here |
|---|---|---|
| Creature action pools (types 116, 118, 47/111–113, 119, 101; natural weapons) | `alpha-creatures-behaviour.md` §13.2 step 11 | §1 |
| **AU-4** multi-hit swing interval; the step's hit flag (frame −0x2b29) | `alpha-audio.md` §6.2, §8 | §2 |
| **AP-6** the *believed* flame-cube callers | `alpha-particles.md` §8, AP-6 | §3 |
| Pose 7's *U* channel; record fields 24 and 25 (**M-10**) | `alpha-trails-poses-schedules.md` §2.1; `alpha-creatures-models.md` M-10 | §4 |
| Pose 21's unidentified components (**P-1**, first half) | `alpha-trails-poses-schedules.md` §2.3 | §5 |
| **M-11** obj +0x13b4 (pose 66's second-boomerang branch) | `alpha-creatures-models.md` App. C, M-11 | §6 |

It also corrects a few statements of earlier specs (§7), notably the golem pool
(`alpha-creatures-behaviour.md` §13.2: 76, not 67), the beam tick period
(`alpha-open-points-1.md` §3.1: 200 ms, not 100 ms) and the reachability of the action-104
beam branch (§2.2).

Written from the Alpha client (`Cube.exe`) per-instruction listing and image bytes, with the
server (`Server.exe`) checked where noted; constants were read from the images' data
sections. No code was copied. **All addresses are RVAs** (both images have image base
0x400000); "client" means `Cube.exe`, "server" means `Server.exe`. *Believed* marks a reading
that rests on partial evidence; *as read* marks behaviour that looks unintended but is what
the code does.

Conventions are those of the cited specs: "obj +x" is a creature object offset (the record
sits at obj +0x10, so obj +x = record offset x − 0x10); positions are 3 × i64 in 1/65536
block; *t* is the action time (obj +0x6c) before this step's advance; the action id is the
byte obj +0x68; dt the step time; P1, P2, P3 the phase lengths (client RVAs 0x3caa0, 0x47310,
0x44270) and D = P1 + P2 + P3 the total duration (client RVA 0x3d1a0); a "draw" is one C
`rand()` call (0 … 32767); *u* = draw/32767. Integer divisions truncate toward zero.
"Scaled" phase values are divided by the creature's speed factors.

---

## 1. Creature action pools (combat update, client RVA 0x2cb20; in-range part from RVA 0x2d584)

### 1.1 Where the pools sit

Step 11 of `alpha-creatures-behaviour.md` §13.2 (the target is within *R*; the range test is
at RVA 0x2d57e) tries, in this order, and the first that applies ends the update:

1. taming;
2. a **drawn special** (step 9): action := it, time 0, special timer := 20 000 ms
   (RVA 0x2d869);
3. **type 116** (§1.3); 4. **type 118** (§1.4); 5. **types 47, 111, 112, 113** (§1.5);
6. **type 119** (§1.6);
7. **magic users** (test RVA 0x44610: class 3, or type 117 or 86, or a main hand of item type
   3 with subtype 10, 11 or 12);
8. **type 101** (§1.7) — only reached when the dragon is not a magic user;
9. natural weapons, melee, ranged (§13.2 as written, with §1.8 here).

Before rule 2, every in-range update after the taming test sets **mode flag 0x04** in the
word at obj +0x124 (RVA 0x2d804).

Rules common to the four pool branches (3–6):

* A new action is picked **only when the current one has finished**: D(current) < *t*
  (strict). Otherwise nothing changes.
* The candidate list is built in the order given below; then **one draw** *r* picks entry
  *r* mod *n* (uniform). Action := that id, time := 0.
* All pool branches leave through the common exit, so a pool creature never reaches rules 7–9.
* The common exit (RVA 0x2eaef … 0x2eb17) stores into the **aim vector** (record field 26,
  obj +0x160) a locally held vector, normally (target position − own position) in blocks
  (computed at RVA 0x2d739 … 0x2d826). A branch that writes obj +0x160 directly is therefore
  overwritten (§1.7, §1.8); the action-95 branch instead replaces the locally held vector, so
  its beam aim is what gets stored (§1.4).

The type is the record's creature type (obj +0x64).

### 1.2 Attack range *R* (step 9; RVA 0x2d3b0 … 0x2d52d)

*R* is computed in this order, each later rule overriding the earlier:

1. base: 2 · own box.x + target box.x;
2. a ranged user (test RVA 0x44520: the magic-user criteria, or type 104, or a main hand of
   item type 3 with subtype 6, 7, 8, 10 or 11) or magic user that sees the target: **50**,
   except that the base stays while the action is 87, 86 or 89 and *t* < P1;
3. types 47, 111, 112, 113: **20**;
4. any creature whose current action is 95, 28 or 108: **50**;
5. action 72: own box.x + target box.x;
6. adjustments for the drawn special;
7. no sight of the target (visibility test RVA 0x19ee90 with range 200, called at
   RVA 0x2d3ce): ***R* := 1.0**, overriding all of the above.

Default boxes (`alpha-creatures-models.md` App. B), examples with target box.x 0.96 and sight:

| Type | *R* | Example |
|---|---|---|
| 116 mammoth | box.x = 7 + 3*u* (one draw, 7 … 10 continuous) | 14.96 … 20.96 |
| 118 rune giant | box.x = 4 | 8.96 |
| 47, 111–113 | 20 | 20 |
| 119 saurian | box.x = 9 | 18.96 |
| 101 dragon | the generator writes no box, so the default 0.96 stays; 50 if a ranged user | 2.88 |

### 1.3 Mammoth (type 116; RVA 0x2d87f, pool RVA 0x2d8d4 … 0x2d912)

Pool: **69, 74, 73** (in that order). Example: draw 20 001 → 20 001 mod 3 = 0 → action 69.

### 1.4 Rune giant (type 118; pool RVA 0x2d9aa … 0x2da18)

Pool: **7, 6, 20**, then **95** if the head model (appearance +0x14, obj +0x8c) is 2154
(`rune-giant-head-laser`), otherwise **86**. The generator always gives type 118 that head
(RVA 0x405a8), so in practice the pool is 7, 6, 20, 95 and 86 never occurs (*as read*).

While the giant's action is **95** (whether just picked or still running), the branch
(RVA 0x2da78 … 0x2dab5) replaces the locally held aim with (target position − an origin point)
in blocks. The origin point comes from RVA 0x46bb0, which, when appearance flag 0x04
(obj +0x7e) is set, builds it from the box (obj +0x84, +0x88) and the orientation (*believed*
the head). The common exit stores this vector, so the beam is aimed from that point at the
target's position.

### 1.5 Rockling and golems (types 47, 111, 112, 113; RVA 0x2e9f7)

Pool: **108, 54, 76** (ids stored at RVA 0x2ea34, 0x2ea50, 0x2ea69; **correction**: the third
id is 76, not 67).

If 54 is picked: velocity (obj +0x34) := **2 × (target position − own position)** in
blocks per second (a fixed-point multiply by 2.0 on all three axes, then conversion to
blocks), then velocity.z := **15**. No speed cap is applied here (*as read*; compare the
player's action 54, capped at 50 in `alpha-creatures-behaviour.md` §16.3).

### 1.6 Saurian (type 119; RVA 0x2dade … 0x2dd67)

The list is built from the saurian's **part models** (appearance +0x14 head = obj +0x8c,
+0x18 hand = obj +0x90, +0x1c body = obj +0x94, +0x1e tail = obj +0x96; model numbers are
slots of `alpha-creatures-models.md` App. A), in this order:

| # | Condition | Adds |
|---|---|---|
| 1 | head is 2147, 2148 or 2149 (`monster-head-reptile-horn1…3`) | **69**; otherwise **68** |
| 2 | tail is 2151 or 2152 (`monster-tail-reptile-spike1/2`) | **70** |
| 3 | hand is 2143 or 2144 (`monster-foot-reptile-claw1/2`) | **75**; otherwise **74** |
| 4 | body is 2140 or 2141 (`monster-body-reptile-spiked1/2`) | **77** and **78** |
| 5 | body is 2135 … 2139 (`monster-body-reptile-crest1…5`) | **76** |
| 6 | current action is neither 71 nor 72, body is 2133 (`…-shell`): **one draw**; if odd | **71** |
| 7 | current action is neither 71 nor 72, body is 2134 (`…-shell-spiked`): **one draw**; if odd | **72** |
| 8 | current action is not 73 | **73** |

Then the pick draw. The draws of rows 6 and 7 happen only when their conditions hold, and
before the pick draw. The list is never empty (row 1 always adds).

The generator (RVA 0x43ad9 … 0x43b27) draws, in this order: head = 2145 + draw mod 5, body =
2133 + draw mod 9, hand = 2142 + draw mod 3 (written to both appearance +0x18 and +0x1a), tail
= 2150 + (draw parity), so of the spiked tails only spike1 (2151) occurs. Hence 3/5 of
saurians carry 69 (horned), 2/3 carry 75 (claws), 1/2 carry 70, 5/9 carry 76, 2/9 carry
77 + 78, and 1/9 each can curl up (71 or 72, each offered with chance ½ per pick and never
twice in a row).

### 1.7 Dragon (type 101; RVA 0x2dec0 … 0x2e104)

Let *d*² be the squared distance to the target in blocks (3-D).

* **64 ≤ *d*² < 3600** (8 to 60 blocks): when the current action has finished (D < *t*),
  action := **37** (a spell; `alpha-projectiles.md`), time 0. The branch also writes into
  obj +0x160 an aim raised by 0.4 × the target's obj +0x88 (box height), but the common exit
  overwrites it with the plain target − self vector (*as read*: the raise has no effect).
* ***d*² ≥ 3600**: nothing.
* ***d*² < 64**: the aim (obj +0x160) is written (RVA 0x2defc; overwritten at the exit), mode
  flag 0x04 is set again (RVA 0x2df01, redundant with §1.1); and if the current action's time
  is > **2000 ms**: one draw; if it is odd **and** the dragon is at most **8 blocks** above
  the target (own z − target z ≤ 524 288 in fixed point, signed 64-bit compare): velocity.z
  := **30** (a leap, RVA 0x2df51) and stop, with action and time unchanged. Otherwise (even
  draw, or more than 8 blocks above): action := **51**, time 0, velocity := the offset
  (target − own position) in blocks; then, only if its squared length is > 0, it is
  normalised and multiplied by **10** (all three axes). A zero offset therefore leaves a zero
  velocity.

Because the dragon's default *R* is 2.88 blocks, a melee-armed dragon only ever runs the
close rule; a ranged-user dragon (main-hand subtype 6, 7 or 8; *R* = 50) casts 37 between 8
and 50 blocks; a magic-user dragon uses the magic rule instead (§1.1 order).

### 1.8 Natural-weapon creatures, refined (appearance flags 0x02 and 0x10, obj +0x7e; RVA 0x2e109)

Both flags must be set. The branch first **clears** mode flag 0x04 (RVA 0x2e12a); it acts
only when *d*² < 64, then writes the aim (overwritten at the exit) and sets flag 0x04 again
(RVA 0x2e15a). After the current action has run > 2000 ms, an odd draw while at most
**2 blocks** above the target (131 072) gives velocity.z := **5** (a hop in place);
otherwise action := **51**, time 0, with velocity := offset, normalised and × **10** when
non-zero, as in §1.7.

When the two flags are not both set, the path at RVA 0x2e25e … 0x2e2ca applies: if the type
is not 104, the main-hand subtype (obj +0xaa9) is not 6, 7 or 8, and appearance flag 0x10 is
set, then once D < *t* the action becomes **72** if the type is 25 and the usability test
RVA 0x3e5a0 accepts 72, otherwise **75** (time 0), as `alpha-creatures-behaviour.md` §13.2.

### 1.9 What the pool actions do

Poses from the action → pose map (client RVA 0xb4ef0, inside the draw routine RVA 0xac260;
index action − 1, byte map RVA 0xbbcdc, jump table RVA 0xbbbcc). Phases from the phase
routines. Pose names are the labels of `alpha-creatures-models.md` §7.2.

| Action | Pose | P1 / P2 / P3 | Movement (client RVA 0x21a266 … 0x21a74d) |
|---|---|---|---|
| 68, 69 | 38, 39 | 300 scaled / fixed 1000 / 800 scaled | speed 0 while *t* ≤ P1; then 20 × facing (0, 1, 0), speed 50, while P1 < *t* < P1 + trunc(3·P2/4) |
| 70 | 40 | 300 / 300 / 500 scaled | normal |
| 71 | 41 | fixed 200 / fixed 3000 / fixed 200 | speed 0 while *t* < D |
| 72 | 41 | fixed 200 / fixed 5000 / fixed 200 | yaw += 720 × step time, speed 12, while *t* < D |
| 73 | 42 | 300 scaled / fixed 600 / 500 scaled | normal |
| 74 | 43 | 300 / 300 / 500 scaled | speed 0.1 while P1 ≤ *t* < D |
| 75 | 44 | 300 / 300 / 500 scaled | normal |
| 76 | 45 | 300 scaled / fixed 500 / 500 scaled | 20 × facing, speed 60, while P1 < *t* < P1 + P2 |
| 77, 78 | 46, 47 | 300 scaled / fixed 500 / 500 scaled | 20 × (−1, 0, 0) for 77, (+1, 0, 0) for 78, speed 60, while P1 < *t* < P1 + P2 |
| 108 | 71 | 400 / 300 / 500 scaled | normal |
| 54 | 1 | fixed 400 / fixed 100 / fixed 400 | the launch of §1.5 |
| 7, 6, 20 | 19, 20, 50 | behaviour §3.3 | normal |
| 95 | **26** | behaviour §3.3 | beam |
| 86 | 60 | behaviour §3.3 | cyclone |
| 37 | 23 | behaviour §3.3 | normal; casts |
| 51 | 70 | 400 / 300 / 500 scaled | the launch of §1.7/§1.8 |

The movement column matches `alpha-physics-camera.md` §3.2–§3.3 and is repeated only for
reading convenience.

### 1.10 Spiders

There is **no spider** in Alpha: neither the client's string table nor its image bytes
contain "spider", and the creature generator's type switch (types 0–155) has no spider type.
No spider pool exists to specify.

### 1.11 Worked examples

1. **Saurian.** Head 2148 (horn2), hand 2143 (claw1), body 2137 (crest3), tail 2150 (normal),
   current action 73 just finished (D < *t*). List: 69 (row 1), 75 (row 3), 76 (row 5); row 8
   adds nothing (current action 73) → [69, 75, 76]. Draw 1234 → 1234 mod 3 = 1 → **75**.
2. **Saurian, shell body.** Head 2145, hand 2142, body 2133, tail 2151, current action 0
   (finished). Rows: 68, 70, 74, then row 6 draws 7 (odd) → 71, then 73 → [68, 70, 74, 71, 73];
   the pick draw 9 → 9 mod 5 = 4 → **73**.
3. **Golem.** Target 6 blocks east and 1 block lower, pick draw 4 → 4 mod 3 = 1 → **54**:
   velocity := (12, 0, −2) then z := 15 → (12, 0, 15).
4. **Dragon, close.** Target 5 blocks away (*d*² = 25 < 64), current action 0 with time 2100,
   dragon 3 blocks above the target: draw 5 (odd) and 3 ≤ 8 → velocity.z := 30; action and
   time are unchanged. At the next in-range update the time is still > 2000, so another draw
   is made at once; a draw of 6 (even) → action 51, time 0, velocity 10 × unit(offset).

---

## 2. AU-4: multi-hit swing sounds and the step's hit flag

### 2.0 World-step dispatch

The client world step (RVA 0x20c510) dispatches on the action at RVA 0x211e04 … 0x211e1b:
a byte map at RVA 0x222ccc (index action − 1, actions 1 … 110) selects a handler from the
table at RVA 0x222c84. Relevant handlers: the **default handler** (RVA 0x212357) serves,
among others, actions 5, 11, 17, 20, 21, 30–33, 54, 72, 86, 87, 88, 91 and **104**; RVA
0x21707a serves 22, 23, 26; the **beam block** (RVA 0x215c68) serves only **28, 94, 95**;
RVA 0x217634 serves 50, 55, 96. The server has the same structure (dispatch server RVA
0x137bc4 … 0x137bdb, map 0x148a8c, table 0x148a44), with 104 also on the default handler.

### 2.1 The multi-hit block (client RVA 0x212cb5 … 0x213052; server twin at RVA 0x138a75 inside server RVA 0x1322d0)

Runs in the default handler for each simulated creature whose action is **72, 86, 30, 31,
32, 33, 11, 5 or 91**. Nothing happens unless **P1 ≤ *t* < P1 + P2**. Then the **hit
interval** *I* is:

| Action | *I* | With the fixed P2 |
|---|---|---|
| 72, 86 | **250 ms** | P2 = 5000 → 20 intervals |
| 91 | **1000 ms** | P2 = 6000 → 6 intervals |
| 11 | ⌊P2/3⌋ | P2 = 300 scaled; 100 at speed 1 |
| 5 | ⌊P2/3⌋ | P2 = **400** scaled; 133 at speed 1 |
| 30, 32 | ⌊P2/6⌋ | fixed 600 → 100 |
| 31, 33 | ⌊P2/12⌋ | fixed 1200 → 100 |

(P2 is the phase routine's value, speed factors included.) Write
*k*(*x*) = trunc((*x* − P1)/*I*). Interval *k* starts at P1 + *k*·*I*.

1. **New interval** (side effects, not sound): if *k*(*t* + dt) ≠ *k*(*t*) and
   P1 + *k*(*t* + dt)·*I* < P1 + P2: the combo counter (obj +0x70) is copied to obj +0x1314,
   and both per-action hit sets (obj +0x11ac and +0x11b4) are **cleared**, so every creature
   can be hit again in the new interval.
2. Action **5** stops here (no sound; it is the silent multi-hit).
3. **Sound** — play **15** (`swing.wav`, pitch 1 + 0.25*u*, volume 1, at the creature's
   position obj +0x10) when either
   * *t* − dt < P1 (the first step with *t* ≥ P1: the first interval), or
   * *k*(*t* + dt) ≠ *k*(*t*) (the step crosses an interval start, including the crossing of
     P1 + P2 itself when *I* divides P2).
4. Action **91** also plays **82** (`punch2.wav`, pitch 0.5 + 0.1*u*) at the same moments.

*As read*: the default handler's crossing-P1 swing (`alpha-audio.md` §6.2; client RVA
0x2126f3 … 0x212712) fires in the step with *t* ≤ P1 < *t* + dt, for every action there except
91. The first multi-hit swing fires in the step with *t* − dt < P1 ≤ *t*. The two fall in the
**same step** when P1 equals a step's *t*, and in consecutive steps otherwise; so the first
interval of 72, 86, 30–33 and 11 gets two swing sounds. Action 11 has P1 = 0, so both come in
the *t* = 0 step. Action 91 gets one.

### 2.2 The hit flag (byte at frame −0x2b29; client RVA 0x216033 / 0x216918)

In the beam block it is the **"this sweep hit an enemy" flag** (`alpha-open-points-1.md`
§3.1). It is cleared each time the block starts its march (RVA 0x216033) and set at RVA
0x216918, reached only after the target checks of RVA 0x216256 … 0x216595 (self, dead
targets, targets already in the hit set, targets with obj +0x128 ≠ 0, and the distance tests
are skipped; the remaining test is *believed* to be hostility to the actor). After the march,
a set flag increments the combo counter obj +0x70 (RVA 0x2169b1 … 0x2169ca). The same frame
byte is also used by the melee section of the world step (RVA 0x213b59 … 0x214bf1).

* **Action 104 is not run by the beam block.** The block contains a branch that tests for
  104 (RVA 0x215c89: at time 0, mana −= charge floored at 0, obj +0x13b4 := charge, charge
  := 0; a single march of length 10; sound 5 `hit1.wav` or 16 `shield-swing.wav` at RVA
  0x216bc7 … 0x216c6a). Because the dispatch (§2.0) sends 104 to the default handler in both
  images and nothing else jumps into the beam block, this branch is **never executed**.
* **Actions 28 and 95** (beams). After each march (§2.3), if trunc((*t* + dt)/200) ≠
  trunc(*t*/200) the channel sound plays: **37** (`channel-hit.wav`) if this march hit an
  enemy, else **36** (`channel2.wav`), pitch 1 + 0.25*u* (RVA 0x216c80 … 0x216d25). The test
  runs in every step that marches: the tick steps and the P1-crossing step. Its grid is
  absolute, while the ticks are offset by P1. For action 28 (P1 = 0) every tick passes it. For
  action 95, P1 is 1000 scaled; when P1 is a multiple of 200 ms (speed 1) every tick passes
  it, otherwise the two 200-ms grids rarely meet in one step and the channel sound is mostly
  silent (*as read*).

### 2.3 Correction: the beam tick period is 200 ms, offset by P1

The tick test of `alpha-open-points-1.md` §3.1 divides by **200**, not 100 (client RVA
0x215ef6 … 0x215f30: the reciprocal multiply 0x51eb851f with a shift of 6; server RVA
0x13bcb6 … 0x13bce4, the same). The offset is **P1** (not D). The beam block exits unless
*t* + dt ≥ P1 and *t* < P1 + P2. Within that window:

* in the P1-crossing step (*t* ≤ P1 < *t* + dt) the march runs without the tick test
  (RVA 0x215d40 … 0x215d56);
* otherwise a tick (march) happens when trunc((*t* + dt − P1)/200) ≠ trunc((*t* − P1)/200)
  and *t* + dt > P1 (RVA 0x215ed1 … 0x215f56).

At speed 1 action 95 (P1 = 1000) marches in the crossing step and then at *t* ≈ 1200, 1400,
1600 … ms, so at most five ticks per second of action time.

### 2.4 Worked examples (dt = 16 ms, speed 1)

1. **Staff action 30** (P1 800, P2 600, *I* = 100). The step *t* = 800 (800 ≤ 800 < 816)
   carries both the crossing swing of `alpha-audio.md` §6.2 and the first multi-hit swing.
   Further multi-hit swings at the steps *t* = 896, 992, 1088, 1184, 1296, 1392 (each crosses
   900, 1000, 1100, 1200, 1300, 1400): seven multi-hit swings in all, the last at the end of
   P2. Hit sets are cleared at the first five boundaries (900 … 1300), not at 1400.
2. **Cyclone action 86** (P1 400, P2 5000, *I* = 250): the step *t* = 400 carries the crossing
   swing and the first multi-hit swing; then a swing at each step crossing 650, 900, …, 5400:
   21 multi-hit swings plus the crossing swing.
3. **Action 91** (P1 1000, P2 6000, *I* = 1000): swing + punch at *t* = 1008 (the step with
   *t* − dt < 1000 ≤ *t*), then at the steps crossing 2000, 3000, …, 7000.

---

## 3. AP-6: the flame-cube callers of `alpha-particles.md` §8

There are exactly six calls into the two flame routines, all in the creature/scene draw
(client RVA 0xac260). "Clock" is the *t* argument of the flame routine; "wall clock" is
`timeGetTime()` read once at the start of the draw (RVA 0xac2aa).

### 3.1 The swirl variant (client RVA 0xbc760)

Same arguments as the plain routine (RVA 0xbbd80) plus a **twist** *w* before the count. For
flame *i* the steps of §8 hold with two changes:

* step 4, radius and angle: the horizontal radius is ***r*·f** (a cone that opens upward:
  0 at the base, *r* at the tip; the rise stays 2·*r*·f), and the angle gains a swirl term:
  *a* = 2π·(*i*/10 + (*t* mod 10 000)·0.0001) + **4π·*w*·f²**;
* nothing else differs (edge, colour, the f·40°/f·30°/f·10° turn and the 443-ms per-flame
  stagger are the same). Colour runs from A at f = 0 to B at f = 1.

### 3.2 Staff charge, actions 30–33 (client RVA 0xb4129 … 0xb454e)

Drawn while the action is 30, 31, 32 or 33 and *t* < D, and the point position + *d* passes
the view test RVA 0x7f760 (radius 4). *d* is the vector at **obj +0x138c**, scaled by 65 536,
i.e. 1 block per unit (*believed* the creature's unit aim direction), so the fire sits about
one block in front of the caster.

Values (*p* = *t*/P1 and *q* = *t*/D):

| Quantity | *t* ≤ P1 | *t* > P1 |
|---|---|---|
| radius *r* | 2·*q* | **4·*q*** |
| size *z* | 0.4·*q* | 0.4·*q* |
| twist *w* | 0.1·*p* | **0.2·*p*** |
| count *N* | **43** | **86** |

Call 1 (all four actions, RVA 0xb4430): base = position + *d* − (0, 0, *r*/2); colours A → B =
red (1, 0, 0) → yellow (1, 1, 0), or, when the specialization byte obj +0x141 is 1
(*believed*: water mage), (0, 0.1, 1) → (0.5, 0.7, 1); clock = wall clock.
Call 2 (actions **31 and 33** only, RVA 0xb4549): radius **2*r***, size *z*, twist ***w*/2**,
count **2*N*** (86 / 172), base = position + *d* − (0, 0, *r*), same colours and clock.

### 3.3 Fire-explosion charge, actions 87 and 88 (client RVA 0xb6e85 … 0xb707f)

Drawn while the action is 87 or 88 and *t* < D. Let *p* = *t*/P1; the **charge level** *c* =
*p* while *p* ≤ 1, and *c* = max(20 − 19·*p*, 0) after P1 (it collapses to 0 within P1/19,
263 ms for action 87, whose P1 is 5000 at speed 1).

Four **rings** *j* = 0, 1, 2, 3, each one call of the plain routine RVA 0xbbd80:

| | Ring *j* |
|---|---|
| count | 20 + 13·*j* (20, 33, 46, 59: 158 flames) |
| radius | 1 + 2·*j*·*c* |
| size | 0.5·*c* |
| base | position (obj +0x10) − (0, 0, obj +0x1180 + 1 + 4·*c*) (obj +0x1180: the landing bob of the companion specs) |
| colours | red (1, 0, 0) → yellow (1, 1, 0) |
| clock | wall clock |

*As read*, the base sits 1 + 4*c* blocks (plus the landing bob) **below** the creature's
centre; the flames rise 2·radius above it, so ring *j* tops out 1 + 4*c*·(*j* − 1) blocks
above the centre (minus the landing bob): ring 0 at 1 − 4*c*, ring 1 at 1, and only rings 2
and 3 higher.

### 3.4 Damage-over-time buff, the "type byte 4" object (client RVA 0xb53f6 … 0xb55ba)

The object is not a world item: it is an entry of the creature's **buff list** (obj +0x1178).
This block reads the entry's kind byte (+0) and its remaining time (+8). For each entry:

* kind 1 or 2: the creature is tinted (1, 0.5, 0.5);
* kind **4** (damage over time): tinted (0.5, 1.5, 0.5), and **8** flames are drawn with the
  plain routine: base = obj +0x1350 (the drawn position) − (0, 0, obj +0x1180); radius =
  component 0 of the box (obj +0x80) of the **local player's** creature (pointer at
  G+0x8006d0, *as read*; it equals the creature's own width only for the player); size 0.1;
  colours green (0, 1, 0) → yellow-green (0.5, 1, 0); clock = 100 000 − remaining time, so the
  flames move with the buff's own timer.

### 3.5 Healing stream, action 34 (client RVA 0xb4566 … 0xb4676), clarified

The flames are drawn on the **stream's target**, the creature whose id is obj +0x1a0 (record
field 36), when that id is non-zero, the caster's action is 34 with *t* < D, the creature
exists, and its drawn position passes the view test with a radius equal to component 2 of the
target's box. Base = the target's drawn position (obj +0x1350), radius = component 0 of the
target's box (obj +0x80), size 0.1, 20 flames, colours (0, 0, 1) → (0.25, 0.75, 1), clock =
the caster's action time (obj +0x6c).

### 3.6 The caller table, corrected (client RVAs)

| Caller | What | *N* | *r* | *z* | A → B | Clock |
|---|---|---|---|---|---|---|
| 0xb325c | campfire | 20 | 0.8 | 0.2 | red → yellow | wall clock |
| 0xb4676 | action 34, on the target (§3.5) | 20 | target box.x | 0.1 | (0, 0, 1) → (0.25, 0.75, 1) | action time |
| 0xb55b5 | buff kind 4 (§3.4) | 8 | player box.x | 0.1 | (0, 1, 0) → (0.5, 1, 0) | 100 000 − remaining |
| 0xb705c | actions 87/88, four rings (§3.3) | 20 + 13*j* | 1 + 2*jc* | 0.5*c* | red → yellow | wall clock |
| 0xb4430 via 0xbc760 | actions 30–33 (§3.2) | 43 / 86 | 2*q* / 4*q* | 0.4*q* | red → yellow, or water | wall clock |
| 0xb4549 via 0xbc760 | actions 31, 33, second cone | 86 / 172 | 2× | 0.4*q* | same | wall clock |

### 3.7 Worked examples

1. **Staff 30 at *t* = 1200** (speed 1: P1 800, P2 600, P3 100, D 1500): *q* = 0.8, *p* = 1.5
   → *r* = 3.2, *z* = 0.32, *w* = 0.3, *N* = 86, base = position + *d* − (0, 0, 1.6). Flame 0
   with wall clock 1000: f = 0.5, q = 0, c = cos 0 = 1 → edge 1.5·0.32 = 0.48; *a* = 2π·0.1 +
   4π·0.3·0.25 = 0.62832 + 0.94248 = 1.57080 (π/2) → offset (0, 1.6, 3.2) from the base.
2. **Staff 30 at *t* = 400**: *q* = 0.26667, *p* = 0.5 → *r* = 0.53333, *z* = 0.10667,
   *w* = 0.05, *N* = 43.
3. **Fire explosion 87 at *t* = 2500** (P1 5000): *c* = 0.5 → rings of 20, 33, 46, 59 flames
   at radius 1, 2, 3, 4, size 0.25, base 3 blocks (plus the landing bob) below the centre.
   At *t* = 5100: *c* = 20 − 19·1.02 = 0.62; at *t* = 5300: *c* = 0 (flames of size 0, not
   seen).

---

## 4. Pose 7 and record fields 24, 25 (resolves M-10; the pose-7 cell of trails-poses §2.1)

* **No action uses pose 7.** The action → pose map (client RVA 0xb4ef0; byte map RVA 0xbbcdc,
  jump table RVA 0xbbbcc) never stores 7 in the pose id (obj +0x14a0), and no other code in
  the client image writes 7 there: every immediate stored into that field is one of 0–6,
  8–11, 15–35, 37–63, 65–73, and no register store to it exists. In the pose function
  (RVA 0x128f0; switch at RVA 0x13af6, table RVA 0x23d4c, poses 1 … 73), 12–14, 36 and 64 go
  to the default case. The pose-7 case (RVA 0x17ab1) is therefore **dead code**; it is never
  selected.
* **Record field 24** (record offset 0x138, obj +0x148) and **field 25** (record offset
  0x144, obj +0x154), 3 × f32 each, are **only zeroed and copied**. Client: zeroed by the
  record constructor RVA 0x3c100 and the record resets RVA 0x465c0 and 0x47270 (all six
  floats), copied with the whole record (packet and save serialisation, record copies), and
  read only by the pose function: pose 7 reads both fields, poses 15, 16, 48 and 49 read field
  24. Server: zeroed by RVA 0x7020 and 0x11360, copied by RVA 0x1df70 and 0x22f90; its world
  step has no store to these offsets. No direct store that computes a value for them was found
  in either program's world step, AI, hit routine or draw. So in practice ***U* := field 24 in
  poses 15, 16, 48, 49 sets *U* to 0** (*believed*: a hand-edited save or a foreign server
  could carry non-zero values, which the client would then draw). The fields remain part of
  the record layout for wire and save compatibility.

---

## 5. Pose 21: the t < D/2 sub-branch (resolves the first half of P-1)

Read from client RVA 0x18591 … 0x187b2 and its mirror RVA 0x18854 … 0x18a48. The main-hand
subtype is obj +0xaa9 (item type obj +0xaa8); the off-hand subtype is obj +0x991 (item type
obj +0x990). The **mirror** runs when the main-hand subtype is 6 (bow) or 7 (crossbow);
otherwise the **off-hand variant** runs. Full value of the case for **t < 2·D** (the first
limit of trails-poses §2.1), off-hand variant; the mirror swaps L ↔ R and changes the signs
as listed below:

| Channel | All of *t* < 2·D | Extra while *t* < D/2 |
|---|---|---|
| Δ*L* (bow hand) | += (−7, 5, 4) for a bow (off-hand subtype 6), += (−3, 2, 4) otherwise | **Δ*L*.z −= 2** (net +2) |
| Δ*R* | += (−10, 3, 3) | := (−5, 1, 0) |
| *T* (torso) | *T*.z −= 70 | ***T*.z := −50** |
| ρ*L* | ρ*L*.z := 70 (ρ*L*.y := −90 for an off-hand crossbow, set before the *t* test) | ρ*L*.x := −5; **ρ*L*.z := 70** (rewritten, same value) |
| ρ*R* | ρ*R*.z := 70 | — |
| models | — | main-hand model slot emptied |

Mirror (bow or crossbow in the main hand; the −90 y rotation goes to ρ*R*.y for a main-hand
crossbow): Δ*R* += (7, 5, 4) for a main-hand bow, (3, 2, 4) otherwise, and Δ*R*.z −= 2 while
*t* < D/2; Δ*L* += (10, 3, 3), then := (5, 1, 0); *T*.z := 70 (an assignment, not an
increment), then **50**; ρ*R*.z := −70 (rewritten −70), ρ*R*.x := −5; ρ*L*.z := −70; off-hand
model slot emptied.

When *t* ≥ D/2 (and also in the *t* ≥ 2·D branch), the off-hand variant fills the main-hand
model slot with model number 840 (0x348, through the model lookup RVA 0x120c0) if the off-hand
item has item type 3 (RVA 0x18829 … 0x1884a); the mirror fills the off-hand slot the same way
if the main-hand item has item type 3 (RVA 0x18aa0 … 0x18abb).

So the component "lowered by 2" is the **z offset of the bow hand**, and the two rotation
values are the **torso yaw *T*.z (−50, mirror +50)** and the **bow hand's ρ.z (70, mirror
−70)**. This corrects the *believed* "ρ*L*.z and ρ*R*.z" of trails-poses §2.3. Also new: the
bow hand's x offset −7 / −3 (mirror +7 / +3), which Appendix C omitted.

Example (off-hand bow, *t* = 100, D = 500): Δ*L* = default + (−7, 5, 2), Δ*R* = (−5, 1, 0),
*T*.z = −50, ρ*L* = (−5, ·, 70), ρ*R*.z = 70, main-hand model slot empty. At *t* = 300
(≥ D/2): Δ*L* = default + (−7, 5, 4), Δ*R* = default + (−10, 3, 3), *T*.z = default − 70, and
the main-hand slot holds model 840 when the off-hand item has item type 3.

---

## 6. M-11: obj +0x13b4 is the action's spent power

obj +0x13b4 (f32) is the **power the current action consumed**; `alpha-projectiles.md` §1
already calls it the spent charge. Its live writers in the client world step are RVA
0x213510 … 0x2137d4 (default handler), 0x217146 (handler for 22, 23, 26) and 0x217687
(handler for 50, 55, 96). It is reset by RVA 0x3b690, 0x46330 and 0x47110, and inside the
world step at RVA 0x21a104 (action 48) and 0x21a155 (action 47 → 54). Below, "charge" is
record field 23 (obj +0x144).

1. **Action start, default handler** (*t* = 0, any action of that handler except 31 and 33):
   obj +0x13b4 := 0 and obj +0x13c0 := 0. If the action's mana cost (RVA 0x44ae0) is > 0:
   := cost, mana −= cost. Otherwise, for actions **17, 20, 5**: := all current mana, mana := 0;
   if a kind-11 buff entry is present, it is processed (RVA 0x46af0 / 0x460a0, as
   `alpha-creatures-behaviour.md` §16.6) and obj +0x13c0 := 1. For actions **54, 21, 87, 88**:
   := 1.
2. **Every default-handler step** (including 31 and 33): if the charge is > 0: := charge,
   mana −= charge, the hit-landed flag obj +0x13b8 := 0; for actions 11 and 5 the value is
   then halved. In every such step, charge := 0 and mana is floored at 0. Rule 2 does not run
   for actions outside the default handler.
3. Actions **31, 33**: := 0.1 in the step with *t* ≤ P1 < *t* + dt (the cost is taken then);
   a positive charge in that step then overwrites the 0.1 by rule 2.
4. Actions **22, 23, 26**, in the step with *t* ≤ P1 < *t* + dt: mana −= cost if the cost is
   > 0, otherwise mana −= charge; mana floored at 0; := charge; charge := 0
   (`alpha-projectiles.md` §2.3).
5. Actions **50, 55, 96**, at their P1 crossing: := charge, mana −= charge, with no floor in
   that code (`alpha-projectiles.md` §2.4).

The action-104 write of the beam block (§2.2) is never executed.

Pose 66 (boomerang throw, action 26; pose-switch case at RVA 0x17f9e) takes its **second
branch when obj +0x13b4 > 0** (test at RVA 0x17fa2). Action 26 is not on the default handler,
so neither rule 1 nor rule 2 touches it. A charged throw keeps its charge in field 23 until
the P1 crossing, where rule 4 stores it in obj +0x13b4. From the P1 crossing on, a charged
throw therefore has obj +0x13b4 = charge > 0 and takes the second branch. Before the
crossing, obj +0x13b4 still holds whatever an earlier action left there.

---

## 7. Corrections to earlier specs (summary)

* `alpha-creatures-behaviour.md` §13.2 step 11: golem/rockling pool is **108, 54, 76**;
  type 118's pool is 7, 6, 20 and 95 (86 only with a non-laser head, which the generator
  never makes); type 119 and 101 rules as §1.6, §1.7; natural weapons as §1.8; range order and
  the no-sight value 1.0 as §1.2.
* `alpha-open-points-1.md` §3.1 and §3.3: beam ticks every **200 ms**, offset by **P1**, with
  an extra march in the P1-crossing step (§2.3).
* `alpha-audio.md` §6.2: interval lengths and flag meaning as §2.1–§2.2; the crossing swing
  and the first multi-hit swing share a step when P1 is a step time; the action-104 beam
  branch and its sounds 5/16 are unreachable.
* `alpha-particles.md` §8: caller rows as §3.6 (RVA 0xb55b5 is the kind-4 buff, not a world
  item; action 34's flames are on the target).
* `alpha-trails-poses-schedules.md` §2.1 row 7: the case is dead (§4); §2.3 pose 21 as §5.
* `alpha-creatures-models.md` M-10 resolved (§4), M-11 resolved (§6); §7.2 pose of action 95
  is **26**.

---

## 8. Remaining open points

* **OP3-1** (narrowed) The value of obj +0x13b4 during a boomerang throw before its P1
  crossing (left over from an earlier action), and the off-hand copy and second throw of
  pose 66's second branch, were not re-established.
* **OP3-2** obj +0x138c as the unit aim direction (§3.2) remains *believed*.
* **OP3-3** What action 104 does in the default handler, now that its beam-block branch is
  known to be unreachable (§2.2).
* **OP3-4** The kind-4 flame radius taken from the local player's box (§3.4) is *as read*;
  whether it is intended is not knowable from the code.
* **OP3-5** Whether action 28's time runs past P1 + P2, and so how many ticks a held beam
  gets; and the effect on the tick count of the action-time rescale after a march with an
  empty hit set (RVA 0x2169d9 … 0x216b1e).
* **OP3-6** Pose 37's arm base (trails-poses §2.3, second half of P-1) remains open.
* Further items are listed under Unresolved in the re-certification below.

---

## Re-certification (2026-11-15)

Checked against the client and server per-instruction listings and the `Cube.exe` /
`Server.exe` image bytes (jump tables, byte maps and float constants read from the images).
All addresses were converted from virtual addresses to RVAs (image base 0x400000).

**Confirmed** (unchanged in substance): §1.1 order of the in-range rules, the strict
D < *t* pick condition, one pick draw with *r* mod *n*, the type field; §1.2 default box 0.96,
mammoth 7 + 3*u*, rune giant 4, saurian 9, golem range 20; §1.3 pool 69, 74, 73; §1.4 pool,
head 2154 always generated; §1.5 pool 108, 54, 76 and the 54 launch (× 2, z := 15, no cap);
§1.6 all eight rows, the model numbers and conditional draws, the generator fractions; §1.7
thresholds 64 / 3600, action 37, the discarded 0.4 raise, the 2000 ms test, the 524 288 height
limit, vz 30, action 51, × 10; §1.8 flags 0x02 + 0x10, 131 072, vz 5; §1.9 phases, poses
other than 95, the movement column; §1.10 no spider; §1.11 examples 1–3. §2.1 action list,
window, intervals 250 / 1000 / P2/3 / P2/6 / P2/12, new-interval side effects, silent action
5, sound 15 and 82 with their pitches; the sound files of ids 5, 15, 16, 36, 37, 82 (sound
loader, client RVA 0x84350); §2.2 the frame flag's clear and set points and the channel sound
36/37; §2.3 the divisor 200 in both images; §2.4 example counts. §3 entirely (callers,
constants 0.0001 / 4π / 443 ms / 40° / 30° / 10°, all table values, all three worked
examples). §4 pose-id value set (by a byte scan of the whole client image), dead pose-7 case,
readers and resets of fields 24 / 25. §5 every offset and rotation of both variants
(constants 7, 5, 3, 2, 4, 10, 70, −5, −50, 1 read from the image). §6 rule 1, rule 3, the
resets, pose 66's > 0 test.

**Corrected** (old → new):
1. All addresses: virtual addresses → RVAs with the image named.
2. §1 citations: in-range part start 0x42d5f0 → RVA 0x2d584; mammoth branch 0x42d5f8 → RVA
   0x2d87f; saurian range → RVA 0x2dade … 0x2dd67; 0x4b4ef0 is a location inside the draw,
   not a routine; jump tables listed in the order byte map, then dword table.
3. §1.1 "aim overwritten (§1.4, §1.7)" → the 95 aim survives (it replaces the held vector);
   only direct writes (§1.7, §1.8) are overwritten.
4. §1.1 / §1.7 mode flag 0x04: set on every in-range update before the drawn special, not
   only by the dragon. §1.8 now also clears and re-sets it.
5. §1.2: added the full order of the range rules, including 50 for actions 95 / 28 / 108 for
   any creature and **R := 1.0 without sight** (gameplay).
6. §1.4 aim origin: "head point … target's centre" → an origin point built from box and
   orientation (*believed* the head), aimed at the target's position.
7. §1.7 / §1.8 zero offset: "nothing" → velocity becomes zero (gameplay).
8. §1.8: the flag-0x10 path also requires type ≠ 104, a main-hand subtype other than 6/7/8,
   and D < *t*.
9. §1.6 generator draw order: head, hand, body, tail → head, body, hand, tail
   (**random-draw order**); the hand model is also written to +0x1a.
10. §1.9 pose of action 95: 11 → **26** (visuals; 11 is action 28's pose).
11. §1.11 example 4: "after another 2000 ms" → the leap leaves the time unchanged, so the
    next in-range update draws again at once (**gameplay and random-draw frequency**).
12. §2.1 action 5's P2: 300 → **400** scaled, interval 133 ms at speed 1 (gameplay and audio).
13. §2.1 crossing-swing timing: "one step earlier" → crossing swing at *t* ≤ P1 < *t* + dt,
    in the same step as the first multi-hit swing when P1 is a step time (audio timing); §2.4
    examples 1 and 2: crossing swing at 800 / 400, not 784 / 384.
14. **Reversal** §2.2 / §6 / §7: the action-104 beam-block branch (charge consumption,
    single sweep, sounds 5 / 16) → **unreachable** in both images, because the dispatch sends
    104 to the default handler (gameplay and audio).
15. §2.2 / §2.3 tick offset: D → **P1** (95: P1 = 1000 at speed 1; 28: P1 = 0); "reached only
    in tick steps" → the march and the channel test also run in the P1-crossing step
    (gameplay and audio timing).
16. §6 rule 1: added obj +0x13c0 := 0; the kind-11 buff is "processed" rather than "cleared".
17. §6 rule 2: "except 31, 33, only when charge > 0" → every default-handler step including
    31 / 33; charge := 0 and the mana floor every step; not for other handlers.
18. §6 rule 5 split: 22 / 23 / 26 take the cost or the charge from mana with a floor; 50 / 55 /
    96 take the charge without a floor.
19. **Reversal** §6 pose 66: "the charge field is usually 0 at P1, so rule 5 writes 0" → action
    26 never runs rule 2, so a charged throw stores its charge at P1 and shows the second
    branch from then on (visuals).
20. §6 writer 0x615cc5 → the load of an unreachable store; removed from the live writers.
21. §3.3 "dome reaches 1 + 4*c* + … above ground" → ring *j* tops out 1 + 4*c*(*j* − 1) above
    the centre.
22. §3.4 / §3.5: radius is component 0 of the obj +0x80 box; the healing view test uses box
    component 2; campfire clock stated as the wall clock.
23. §4: the server routines 0x407020 / 0x411360 zero the fields (they are not copies); added
    the client constructor RVA 0x3c100 and the field-by-pose readers.
24. §5: added the variant selection by main-hand subtype, the item-type fields and the
    model-840 fill; "model shown" → slot holds model 840 when the bow-hand item has item
    type 3.

**Removed**: advice to the replacement in §4 ("may keep it for completeness"; "keep the
fields …, always send and store 0, treat *U* as 0"), reworded as native facts; all *Game
check* / *Check* procedures of §8 (logging, recording, listening, mod-DLL hooks); §2.4
examples 4 (dead action-104 path) and 5 (unconfirmed action-28 tick timing); the original
§6 rule 4 (dead action-104 write); the unverified sentence "nothing else moves these creatures
during the actions"; the §1.1 claims "never dodges, never drinks for mana, the potion rule
still applies"; pose descriptions in §1.9 (now pointed to the companion's labels); the buff
entry's value and source-id fields, which this block does not read; "while it hits nothing"
in §2.3.

**Unresolved**:
* What action 104 does in the default handler (OP3-3).
* Action 28's time beyond P1 + P2 and the empty-hit-set time rescale (OP3-5); the 100-ms hit
  window opened at each multi-hit interval start (frame locals −0x2b3c / −0x2b50, flag
  −0x2c70 read at RVA 0x214a26 for actions 32 and 33) is a lead only.
* The hostility reading of the beam hit test (§2.2), and the widening sweep radius of
  `alpha-open-points-1.md`, not re-checked here.
* Pose 66: the stale pre-P1 obj +0x13b4 value, the off-hand copy and the second throw at
  P1 + 300 (OP3-1).
* Meanings (the offsets themselves are confirmed): obj +0x138c as aim direction; obj +0x1180
  landing bob; obj +0x1350 drawn position; obj +0x141 = 1 as water mage; the campfire identity
  of RVA 0xb325c; buff entry +8 as remaining time.
* Weapon subtype names (6 / 7 / 8 bow / crossbow / boomerang; 10 / 11 / 12 staff / wand /
  bracelet); which of 77 / 78 is left or right (only −x / +x is known); the yaw unit of
  action 72; the rock projectile of action 108; the identity of model 840.
* The crossing-swing sound id for action 11 (48 in `alpha-audio.md`), not re-checked.
* Taming details; whether the idle action-0 time keeps advancing in the world step.
* Absence of computed writers to fields 24 / 25 was established for direct stores only;
  block copies and indirect stores were not exhaustively traced.
* The "Rules common to the four pool branches" behaviour of the potion rule (step 3 of
  `alpha-creatures-behaviour.md` §13.2) was not re-checked.
