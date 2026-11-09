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
(`alpha-creatures-behaviour.md` §13.2: 76, not 67) and the beam tick period
(`alpha-open-points-1.md` §3.1: 200 ms, not 100 ms).

Written from the Alpha client (`Cube.exe`) decompile and per-instruction listing, with the
server (`Server.exe`) listing checked where noted; constants were read from the client's data
section. No code was copied; addresses are citations only. *Believed* marks a reading that
rests on partial evidence; *as read* marks behaviour that looks unintended but is what the
code does.

Conventions are those of the cited specs: "obj +x" is a creature object offset (record offset
+ 0x10); positions are 3 × i64 in 1/65536 block; *t* is the action time (obj +0x6c) before this
step's advance; dt the step time; P1, P2, P3 the phase lengths and D = P1 + P2 + P3 the total
duration (`alpha-creatures-behaviour.md` §3.3; routines 0x43caa0, 0x447310, 0x43d1a0); a
"draw" is one C `rand()` call (0 … 32767); *u* = draw/32767. Integer divisions truncate toward
zero.

---

## 1. Creature action pools (Combat 0x42cb20, in-range part 0x42d5f0 … 0x42e25e)

### 1.1 Where the pools sit

Step 11 of `alpha-creatures-behaviour.md` §13.2 (the target is within *R*) tries, in this
order, and the first that applies ends the update:

1. taming;
2. a **drawn special** (step 9): action := it, time 0, special timer := 20 000 ms;
3. **type 116** (§1.3); 4. **type 118** (§1.4); 5. **types 47, 111, 112, 113** (§1.5);
6. **type 119** (§1.6);
7. **magic users** (0x444610: class 3, or type 117 or 86, or a main hand of type 3 subtype
   10, 11 or 12);
8. **type 101** (§1.7) — only reached when the dragon is not a magic user;
9. natural weapons, melee, ranged (§13.2 as written, with §1.8 here).

Rules common to the four pool branches (3–6):

* A new action is picked **only when the current one has finished**: D(current) < *t*
  (strict). Otherwise nothing changes (the creature stands, faces the target and waits).
* The candidate list is built in the order given below; then **one draw** *r* picks entry
  *r* mod *n* (uniform). Action := that id, time := 0.
* A pool creature never reaches rules 7–9: it never dodges, never drinks for mana, and never
  uses the weapon-based melee/ranged rules (the potion rule of step 3 still applies).
* At the end of every in-range update the **aim vector** (record field 26, obj +0x160) is set
  to (target position − own position) in blocks (0x42eaef … 0x42eb17). Anything a branch
  wrote to the aim before that is overwritten (§1.4, §1.7).

The type is the record's creature type (obj +0x64).

### 1.2 Attack range *R* of the pool creatures (step 9, default boxes of `alpha-creatures-models.md` App. B)

| Type | *R* | Example (target box 0.96) |
|---|---|---|
| 116 mammoth | 2·box.x + target box.x, box.x = 7 … 10 (draw) | 14.96 … 20.96 |
| 118 rune giant | 2·4 + target; **50** while its action is 95, 28 or 108 | 8.96 |
| 47, 111–113 | **20** | 20 |
| 119 saurian | 2·9 + target | 18.96 |
| 101 dragon | no box entry, so the default 0.96: 2·0.96 + target, unless it is a ranged user (bow, crossbow, boomerang; then 50 with sight) | 2.88 |

### 1.3 Mammoth (type 116; 0x42d5f8)

Pool: **69, 74, 73** (in that order). Example: draw 20 001 → 20 001 mod 3 = 0 → action 69.

### 1.4 Rune giant (type 118; 0x42d96d)

Pool: **7, 6, 20**, then **95** if the head model (appearance +0x14, obj +0x8c) is 2154
`rune-giant-head-laser`, otherwise **86**. The generator always gives type 118 that head
(`alpha-creatures-models.md` App. B row 118), so in practice the pool is 7, 6, 20, 95 and
86 never occurs (*as read*).

While the giant's action is **95** (whether just picked or still running), the branch also
computes the aim from the beam origin: (target position − the attack origin 0x446bb0, the
head point for appearance flag 0x04) in blocks, and that value — not target − own position —
is what the common exit stores in the aim. So the laser is aimed from the head at the
target's centre.

### 1.5 Rockling and golems (types 47, 111, 112, 113; 0x42e9f7)

Pool: **108, 54, 76** (client 0x42ea34, 0x42ea50, 0x42ea69; **correction**: the third id is
76, not 67).

If 54 is picked: velocity (obj +0x34) := **2 × (target position − own position)** in
blocks per second (a fixed-point multiply by 2.0, all three axes), then velocity.z := **15**.
No speed cap is applied here (*as read*; compare the player's action 54, capped at 50 in
`alpha-creatures-behaviour.md` §16.3).

### 1.6 Saurian (type 119; 0x42dae8 … 0x42dd5c)

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

With the generator's model draws (head mod 5, hand mod 3, body mod 9, tail mod 2 → only
spike1 of the spiked tails occurs): 3/5 of saurians carry 69 (horned), 2/3 carry 75 (claws),
1/2 carry 70, 5/9 carry 76, 2/9 carry 77 + 78, and 1/9 each can curl up (71 or 72, each
offered with chance ½ per pick and never twice in a row).

### 1.7 Dragon (type 101; 0x42dec0 … 0x42e104)

Let *d*² be the squared distance to the target in blocks (3-D).

* **64 ≤ *d*² < 3600** (8 to 60 blocks): when the current action has finished, action :=
  **37** (a spell; `alpha-projectiles.md`), time 0. The branch also writes an aim raised by
  0.4 × the target's box height, but the common exit overwrites it with plain
  target − self (*as read*: the raise has no effect).
* ***d*² ≥ 3600**: nothing.
* ***d*² < 64**: aim at the target, set mode flag 0x04; and if the current action's time
  is > **2000 ms**: one draw; if it is odd **and** the dragon is at most **8 blocks** above
  the target (own z − target z ≤ 524 288 in fixed point): velocity.z := **30** (a leap) and
  stop. Otherwise (even draw, or more than 8 blocks above): action := **51**, time 0,
  velocity := **10** × unit(target − own position) (all three axes; nothing if the offset is
  zero).

Because the dragon's default *R* is 2.88 blocks, a melee-armed dragon only ever runs the
close rule; a bow/crossbow/boomerang dragon (*R* = 50) casts 37 between 8 and 50 blocks; a
class-3 or staff/wand/bracelet dragon uses the magic rule instead (§1.1 order).

### 1.8 Natural-weapon creatures, refined (appearance flags 0x02 and 0x10; 0x42e109)

Same shape as the dragon's close rule, with smaller numbers: only when *d*² < 64; after the
current action has run > 2000 ms, an odd draw while at most **2 blocks** above the target
(131 072) gives velocity.z := **5** (a hop in place); otherwise action := **51** with
velocity := **10** × unit(target − own position). (Flag 0x10 alone: action 75, type 25: 72 if
usable, as `alpha-creatures-behaviour.md` §13.2.)

### 1.9 What the pool actions do

| Action | Pose (models §7.2) | P1 / P2 / P3 (behaviour §3.3) | Movement (physics §3.3) |
|---|---|---|---|
| 68, 69 | 38, 39 (head swing) | 300 / fixed 1000 / 800 | stand during P1; then 20 × facing, speed 50, until P1 + ¾·P2 |
| 70 | 40 (body spin) | 300 / 300 / 500 | normal |
| 71, 72 | 41 (curl into the shell; parts hidden) | fixed 200 / fixed 3000 (71), 5000 (72) / fixed 200 | 71: speed 0; 72: spins, yaw += 720°/s, speed 12 |
| 73 | 42 | 300 / fixed 600 / 500 | normal |
| 74 | 43 | 300 / 300 / 500 | speed 0.1 from P1 to the end |
| 75 | 44 (five-step claw combo) | 300 / 300 / 500 | normal |
| 76 | 45 (forward flip) | 300 / fixed 500 / 500 | 20 × facing, speed 60, P1 … P1 + P2 |
| 77, 78 | 46, 47 (side rolls) | 300 / fixed 500 / 500 | 20 × left / right, speed 60, P1 … P1 + P2 |
| 108 | 71 (lift and throw a rock) | default 400 / 300 / 500 | normal; throws the rock projectile |
| 54 | 1 | fixed 400 / 100 / 400 | the launch of §1.5 |
| 7, 6, 20 | 19, 20, 50 | §3.3 | normal |
| 95, 86 | 11, 60 | §3.3 | beam; cyclone |
| 37 | 23 | §3.3 | normal; casts |
| 51 | 70 (hop) | default | the launch of §1.7/§1.8 |

The movement column is already specified in `alpha-physics-camera.md` §3.2–§3.3 (client
0x61a266 … 0x61a74d); it is repeated only for reading convenience. Nothing else moves these
creatures during the actions.

### 1.10 Spiders

There is **no spider** in Alpha: the creature-type table (types 0–155,
`alpha-ui-screens.md`), the model table and the client's strings contain no spider. No
spider pool exists to specify.

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
   dragon 3 blocks above the target: draw 5 (odd) and 3 ≤ 8 → velocity.z := 30. Next time
   (after another 2000 ms) a draw of 6 (even) → action 51 and velocity 10 × unit(offset).

---

## 2. AU-4: multi-hit swing sounds and the step's hit flag

### 2.1 The multi-hit block (client 0x612cb5 … 0x613052; server twin in 0x5322d0)

Runs for each simulated creature whose action is **72, 86, 30, 31, 32, 33, 11, 5 or 91**.
Nothing happens unless **P1 ≤ *t* < P1 + P2**. Then the **hit interval** *I* is:

| Action | *I* | With the fixed P2 of §3.3 |
|---|---|---|
| 72, 86 | **250 ms** | P2 = 5000 → 20 intervals |
| 91 | **1000 ms** | P2 = 6000 → 6 intervals |
| 11, 5 | ⌊P2/3⌋ | P2 = 300/(*S*·*a*); 100 at speed 1 |
| 30, 32 | ⌊P2/6⌋ | 600 → 100 |
| 31, 33 | ⌊P2/12⌋ | 1200 → 100 |

(P2 is the phase routine's value, attack-speed factors included.) Write
*k*(*x*) = trunc((*x* − P1)/*I*). Interval *k* starts at P1 + *k*·*I*.

1. **New interval** (side effects, not sound): if *k*(*t* + dt) ≠ *k*(*t*) and
   P1 + *k*(*t* + dt)·*I* < P1 + P2: the combo counter (obj +0x70) is copied to obj +0x1314,
   and both per-action hit sets (obj +0x11ac and +0x11b4) are **cleared**, so every creature
   can be hit again in the new interval.
2. Action **5** stops here (no sound; it is the silent multi-hit).
3. **Sound** — play **15** (`swing.wav`, pitch 1 + 0.25*u*, volume 1, at the creature) when
   either
   * *t* − dt < P1 (the step right after the P1 crossing: the first interval), or
   * *k*(*t* + dt) ≠ *k*(*t*) (the step crosses an interval start, including the crossing of
     P1 + P2 itself when *I* divides P2).
4. Action **91** also plays **82** (`punch2.wav`, pitch 0.5 + 0.1*u*) at the same moments.

*As read*: the crossing-P1 row of `alpha-audio.md` §6.2 (15, or 48 for action 11) also fires
for 72, 86, 30–33 and 11, one step earlier, so the first interval of those actions gets two
sounds in consecutive steps. Action 91 has no crossing-P1 swing, so it gets one.

### 2.2 The hit flag (byte at frame −0x2b29, client 0x616033 / 0x616918)

It is the **"this sweep hit an enemy" flag** of the beam block (`alpha-open-points-1.md`
§3.1). It is cleared each time the block starts its march (0x616033) and set when the march
runs the hit routine on a creature hostile to the actor (0x616918, the same moment the
tick's hit flag of open-points-1 §3.1 step 6 is set; it is what raises the combo there).
Friendly creatures entering the cylinder do not set it.

* **Action 104** (shield charge release). Its beam-block entry (client 0x615c89) consumes the
  charge at time 0 (mana −= charge, floored at 0; obj +0x13b4 := charge; charge := 0), then
  runs the march **once**, in the step where *t* ≤ P1 < *t* + dt — P1 is 0 for 104, so the
  first step — with length 10 and the widening radius of open-points-1. In that same step the
  sound is **5** (`hit1.wav`) if the sweep hit an enemy, else **16** (`shield-swing.wav`),
  pitch 1 + 0.25*u* (0x616c35).
* **Actions 28 and 95** (beams). After each tick's march (§2.3), if trunc((*t* + dt)/200) ≠
  trunc(*t*/200) the channel sound plays: **37** (`channel-hit.wav`) if this tick hit an enemy,
  else **36** (`channel2.wav`), pitch 1 + 0.25*u* (0x616c80 … 0x616d25). This test is reached
  only in tick steps. For action 28 (D = 0) every tick passes it. For action 95 the ticks are
  offset by its wind-up D; when D is a multiple of 200 ms (speed 1: D = 1000) every tick
  passes it, otherwise the two 200-ms grids rarely meet in one step and the channel sound is
  mostly silent (*as read*).

### 2.3 Correction: the beam tick period is 200 ms

The tick test of `alpha-open-points-1.md` §3.1 divides by **200**, not 100 (client 0x615ef6 …
0x615f30: the reciprocal multiply 0x51eb851f with a shift of 6; server 0x53bcb6 … 0x53bce4,
the same). Read there: a tick happens when trunc((*t* + dt − D)/200) ≠ trunc((*t* − D)/200) and
*t* + dt > D; the first tick comes when *t* − D passes **200**; at speed 1 action 95 ticks at
*t* ≈ 1200, 1400, 1600 … ms (example of open-points-1 §3.3), so at most **five** bursts per
second while it hits nothing.

### 2.4 Worked examples (dt = 16 ms, speed 1)

1. **Staff action 30** (P1 800, P2 600, *I* = 100). Step *t* = 784 crosses P1: the crossing
   swing of §6.2. Multi-hit swings at the steps *t* = 800 (first interval), 896, 992, 1088,
   1184, 1296, 1392 (each crosses 900, 1000, 1100, 1200, 1300, 1400) — seven, the last at the
   end of P2. Hit sets are cleared at the first five boundaries (900 … 1300), not at 1400.
2. **Cyclone action 86** (P1 400, P2 5000, *I* = 250): swings at *t* = 400 and at each step
   crossing 650, 900, …, 5400 — 21 swings plus the crossing swing at *t* = 384.
3. **Action 91** (P1 1000, P2 6000, *I* = 1000): swing + punch at *t* = 1008 (the step with
   *t* − dt < 1000 ≤ *t*), then at the steps crossing 2000, 3000, …, 7000.
4. **Action 104**: a shield NPC releases with charge 0.6; step 1 (*t* = 0): mana −= 0.6,
   obj +0x13b4 := 0.6; the 10-block sweep catches the player → hit routine, flag set, sound 5.
   Against empty air → sound 16.
5. **Action 28**: ticks (and channel sounds) at the steps crossing 200, 400, …: 37 while
   the beam is on an enemy, 36 otherwise.

---

## 3. AP-6: the flame-cube callers of `alpha-particles.md` §8

All callers live in the creature/scene draw 0x4ac260. "Clock" is the *t* argument of the
flame routine; "wall clock" is `timeGetTime()` read once at the start of the draw
(0x4ac2aa).

### 3.1 The swirl variant (client 0x4bc760)

Same arguments as 0x4bbd80 plus a **twist** *w* before the count. For flame *i* the steps
of §8 hold with two changes:

* step 4, radius and angle: the horizontal radius is ***r*·f** (a cone that opens upward:
  0 at the base, *r* at the tip; the rise stays 2·*r*·f), and the angle gains a swirl term:
  *a* = 2π·(*i*/10 + (*t* mod 10 000)·0.0001) + **4π·*w*·f²**;
* nothing else differs (edge, colour, the f·40°/f·30°/f·10° turn and the 443-ms stagger are
  the same).

### 3.2 Staff charge, actions 30–33 (client 0x4b4129 … 0x4b454e)

Drawn while the action is 30, 31, 32 or 33 and *t* < D, and the point
position + *d* passes the view test 0x47f760 (radius 4). *d* is the vector at **obj
+0x138c**, taken as 1 block per unit (*believed* the creature's unit aim direction: the
world step turns the body toward it while aiming and the beam visual casts along it), so the
fire sits about one block in front of the caster.

Values (*p* = *t*/P1 and *q* = *t*/D):

| Quantity | *t* ≤ P1 | *t* > P1 |
|---|---|---|
| radius *r* | 2·*q* | **4·*q*** |
| size *z* | 0.4·*q* | 0.4·*q* |
| twist *w* | 0.1·*p* | **0.2·*p*** |
| count *N* | **43** | **86** |

Call 1 (all four actions, 0x4b4430): base = position + *d* − (0, 0, *r*/2); colours A → B =
red (1, 0, 0) → yellow (1, 1, 0), or for specialization 1 (water mage) (0, 0.1, 1) → (0.5,
0.7, 1); clock = wall clock.
Call 2 (actions **31 and 33** only, 0x4b4549): radius **2*r***, size *z*, twist ***w*/2**,
count **2*N*** (86 / 172), base = position + *d* − (0, 0, *r*), same colours and clock.

### 3.3 Fire-explosion charge, actions 87 and 88 (client 0x4b6e85 … 0x4b707f)

Drawn while the action is 87 or 88 and *t* < D. Let *p* = *t*/P1; the **charge level** *c* =
*p* while *p* ≤ 1, and *c* = max(20 − 19·*p*, 0) after P1 (it collapses to 0 within P1/19,
263 ms for action 87 at speed 1).

Four **rings** *j* = 0, 1, 2, 3, each one call of 0x4bbd80 (the plain routine):

| | Ring *j* |
|---|---|
| count | 20 + 13·*j* (20, 33, 46, 59: 158 flames) |
| radius | 1 + 2·*j*·*c* |
| size | 0.5·*c* |
| base | position − (0, 0, landing bob + 1 + 4·*c*) (landing bob obj +0x1180) |
| colours | red (1, 0, 0) → yellow (1, 1, 0) |
| clock | wall clock |

*As read*, the base sits 1 + 4*c* blocks **below** the creature's centre; the flames rise
2·radius above it, so the dome reaches 1 + 4*c* + … above ground only for the outer rings.

### 3.4 Damage-over-time buff, the "type byte 4" object (client 0x4b53f6 … 0x4b55ba)

The object is not a world item: it is an entry of the creature's **buff list** (obj +0x1178;
entry: kind u8 at +0, value f32 at +4, remaining time ms at +8, source id at +0x10). For each
entry:

* kind 1 or 2: the creature is tinted (1, 0.5, 0.5);
* kind **4** (damage over time): tinted (0.5, 1.5, 0.5), and **8** flames are drawn with the
  plain routine: base = drawn position (obj +0x1350) − (0, 0, landing bob); radius = the
  box width (box.x) of the **local player's** creature (G+0x8006d0, *as read*; it equals the
  creature's own width only for the player); size 0.1; colours green (0, 1, 0) → yellow-green
  (0.5, 1, 0); clock = 100 000 − remaining time, so the flames move with the buff's own
  timer.

### 3.5 Healing stream, action 34 (client 0x4b4566 … 0x4b4676), clarified

The flames are drawn on the **stream's target**, the creature whose id is record field 36
(obj +0x1a0), if it exists and its drawn position passes the view test: base = the target's
drawn position, radius = the target's box.x, size 0.1, 20 flames, colours (0, 0, 1) →
(0.25, 0.75, 1), clock = the caster's action time; only while the caster's *t* < D.

### 3.6 The caller table, corrected

| Caller | What | *N* | *r* | *z* | A → B | Clock |
|---|---|---|---|---|---|---|
| 0x4b325c | campfire (unchanged) | 20 | 0.8 | 0.2 | red → yellow | as §8 |
| 0x4b4676 | action 34, on the target (§3.5) | 20 | target box.x | 0.1 | (0, 0, 1) → (0.25, 0.75, 1) | action time |
| 0x4b55b5 | buff kind 4 (§3.4) | 8 | player box.x | 0.1 | (0, 1, 0) → (0.5, 1, 0) | 100 000 − remaining |
| 0x4b705c | actions 87/88, four rings (§3.3) | 20 + 13*j* | 1 + 2*jc* | 0.5*c* | red → yellow | wall clock |
| 0x4b4430 via 0x4bc760 | actions 30–33 (§3.2) | 43 / 86 | 2*q* / 4*q* | 0.4*q* | red → yellow, or water | wall clock |
| 0x4b4549 via 0x4bc760 | actions 31, 33, second cone | 86 / 172 | 2× | 0.4*q* | same | wall clock |

### 3.7 Worked examples

1. **Staff 30 at *t* = 1200** (speed 1: P1 800, D 1500): *q* = 0.8, *p* = 1.5 → *r* = 3.2,
   *z* = 0.32, *w* = 0.3, *N* = 86, base = position + *d* − (0, 0, 1.6). Flame 0 with wall
   clock 1000: f = 0.5, q = 0, c = cos 0 = 1 → edge 1.5·0.32 = 0.48; *a* = 2π·0.1 +
   4π·0.3·0.25 = 0.62832 + 0.94248 = 1.57080 (π/2) → offset (0, 1.6, 3.2) from the base.
2. **Staff 30 at *t* = 400**: *q* = 0.26667, *p* = 0.5 → *r* = 0.53333, *z* = 0.10667,
   *w* = 0.05, *N* = 43.
3. **Fire explosion 87 at *t* = 2500** (P1 5000): *c* = 0.5 → rings of 20, 33, 46, 59 flames
   at radius 1, 2, 3, 4, size 0.25, base 3 blocks (plus the landing bob) below the centre.
   At *t* = 5100: *c* = 20 − 19·1.02 = 0.62; at *t* = 5300: *c* = 0 (flames of size 0, not
   seen).

---

## 4. Pose 7 and record fields 24, 25 (resolves M-10; the pose-7 cell of trails-poses §2.1)

* **No action uses pose 7.** The action → pose map (client 0x4b4ef0, jump table 0x4bbbcc /
  0x4bbcdc) never stores 7 in the pose id (obj +0x14a0), and no other code writes 7 there
  (every constant stored into the pose id is one of 0–6, 8–11, 15–35, 37–63, 65–73; 7, 12–14,
  36 and 64 never appear, and 12–14, 36, 64 have no case anyway). The pose-7
  case of 0x4128f0 (0x417ab1) is **dead code**. AlphaVulkan may keep it for completeness;
  it is never selected.
* **Record field 24** (offset 0x138, obj +0x148) and **field 25** (offset 0x144, obj +0x154),
  3 × f32 each, are **only ever zeroed and copied**: zeroed by the record resets 0x4465c0 and
  0x447270 (all six floats), copied with the whole record (packet and save serialisation,
  record copies), and read only by the pose function (poses 7, 15, 16, 48, 49). No routine of
  either program computes a value for them (client: no writer in the world step, the AI, the
  frame/GUI routine or the hit routine; server: only the copy routines 0x407020, 0x411360,
  0x41df70, 0x422f90). So in practice ***U* := field 24 in poses 15, 16, 48, 49 sets *U* to
  0** (*believed*: a hand-edited save or a foreign server could carry non-zero values, which
  the client would then draw). AlphaVulkan: keep the fields in the record (wire and save
  compatibility), always send and store 0, and treat *U* as 0 in those poses.

---

## 5. Pose 21: the t < D/2 sub-branch (resolves the first half of P-1)

Read from client 0x418591 … 0x4187b2 (bow/crossbow in the off hand) and its mirror
0x418854 … 0x418a48 (in the main hand). Full value of the case for **t < 2·D** (the first
limit of trails-poses §2.1), off-hand variant; the mirror swaps L ↔ R and negates the
x components and the z angles as listed:

| Channel | All of *t* < 2·D | Extra while *t* < D/2 |
|---|---|---|
| Δ*L* (bow hand) | += (−7, 5, 4) for a bow (off-hand subtype 6), += (−3, 2, 4) otherwise | **Δ*L*.z −= 2** (net +2) |
| Δ*R* | += (−10, 3, 3) | := (−5, 1, 0) |
| *T* (torso) | *T*.z −= 70 | ***T*.z := −50** |
| ρ*L* | ρ*L*.z := 70 (ρ*L*.y := −90 for an off-hand crossbow, before) | ρ*L*.x := −5; **ρ*L*.z := 70** (rewritten, same value) |
| ρ*R* | ρ*R*.z := 70 | — |
| models | — | main-hand model removed |

Mirror (bow in the main hand): Δ*R* += (7, 5, 4) / (3, 2, 4), and Δ*R*.z −= 2 while *t* < D/2;
Δ*L* += (10, 3, 3), then := (5, 1, 0); *T*.z := 70, then **50**; ρ*R*.z := −70 (rewritten
−70), ρ*R*.x := −5; ρ*L*.z := −70; off-hand model removed.

So the component "lowered by 2" is the **z offset of the bow hand**, and the two rotation
values are the **torso yaw *T*.z (−50, mirror +50)** and the **bow hand's ρ.z (70, mirror
−70)** — the *believed* "ρ*L*.z and ρ*R*.z" of trails-poses §2.3 is corrected. Also new: the
bow hand's x offset −7 / −3 (mirror +7 / +3), which Appendix C omitted.

Example (off-hand bow, *t* = 100, D = 500): Δ*L* = default + (−7, 5, 2), Δ*R* = (−5, 1, 0),
*T*.z = −50, ρ*L* = (−5, ·, 70), ρ*R*.z = 70, main-hand model hidden; at *t* = 300 (≥ D/2):
Δ*L* = default + (−7, 5, 4), Δ*R* = default + (−10, 3, 3), *T*.z = default − 70, model shown.

---

## 6. M-11: obj +0x13b4 is the action's spent power

obj +0x13b4 (f32) is the **power the current action consumed** — `alpha-projectiles.md` §1
already calls it the spent charge. Its writers in the world step (client 0x613510 …
0x6137d4, 0x615cc5, 0x617146, 0x617687; reset by 0x43b690, 0x446330, 0x447110, 0x61a104,
0x61a155):

1. **Action start** (*t* = 0, any action except 31 and 33): := 0. If the action's mana cost
   (0x444ae0) is > 0: := cost, mana −= cost. Otherwise actions **17, 20, 5**: := all current
   mana, mana := 0 (and if a kind-11 buff is active it is cleared, an event record is
   emitted as in `alpha-creatures-behaviour.md` §16.6, and obj +0x13c0 := 1);
   actions **54, 21, 87, 88**: := 1.
2. **Any step** (except actions 31, 33) with charge (field 23, obj +0x144) > 0: := charge,
   mana −= charge, the hit-landed flag +0x13b8 := 0; for actions 11 and 5 the value is then
   halved; charge := 0; mana floored at 0.
3. Actions **31, 33**: := 0.1 when P1 is crossed (and the cost is taken then).
4. Action **104** at *t* = 0: := charge (§2.2).
5. Actions **22, 23, 26** when P1 is crossed: := charge (projectiles §2.3); actions 50, 55, 96
   likewise (projectiles §2.4).

Pose 66 (boomerang throw, action 26) takes its **second branch when the value is > 0**: the
throw was charged, and the off hand shows a copy of the boomerang (a second one is thrown at
P1 + 300, projectiles §2.3). *As read*, rule 2 moves any charge into +0x13b4 at the throw's
first step, so the charge field is usually 0 when P1 is crossed and rule 5 then writes 0
(open point OP3-1).

---

## 7. Corrections to earlier specs (summary)

* `alpha-creatures-behaviour.md` §13.2 step 11: golem/rockling pool is **108, 54, 76**;
  type 118's pool is 7, 6, 20 and 95 (86 only with a non-laser head, which the generator
  never makes); type 119 and 101 rules as §1.6, §1.7; natural weapons as §1.8.
* `alpha-open-points-1.md` §3.1 and §3.3: beam ticks every **200 ms** (§2.3).
* `alpha-audio.md` §6.2: interval lengths and flag meaning as §2.1–§2.2; the 104 sound is
  tied to its single sweep in the first step.
* `alpha-particles.md` §8: caller rows as §3.6 (0x4b55b5 is the kind-4 buff, not a world
  item; action 34's flames are on the target).
* `alpha-trails-poses-schedules.md` §2.1 row 7: the case is dead (§4); §2.3 pose 21 as §5.
* `alpha-creatures-models.md` M-10 resolved (§4), M-11 resolved (§6).

---

## 8. Remaining open points

* **OP3-1** Second boomerang: whether a charged throw really carries a non-zero spent power
  at P1 (rule 2 vs rule 5 of §6). *Game check*: offline, charge a boomerang fully and throw;
  count projectiles (one or two) and watch the off hand during the wind-up (a second
  boomerang drawn means +0x13b4 > 0 then).
* **OP3-2** obj +0x138c as the unit aim direction (§3.2). *Game check*: with the mod DLL, log
  it and record field 26 while a mage charges a staff; the flame centre should sit one block
  along it.
* **OP3-3** The double swing at the start of the multi-hit actions (§2.1). *Game check*: hook
  the sound record routine 0x4864c0 during a cyclone (86) and log (id, *t*).
* **OP3-4** The kind-4 flame radius taken from the local player's box (§3.4). *Game check*:
  poison a large monster (yeti) and compare the ring with the monster's width.
* **OP3-5** The 87/88 flame base below the centre (§3.3). *Game check*: record a djinn's (type
  81, special 87) charge from the side.
* **OP3-6** The mostly silent channel sound of action 95 at speeds ≠ 1 (§2.2). *Game check*:
  listen to a wizard's beam (types 43/45, special 95) at power tier 0 and 2.
* **OP3-7** The dragon's discarded aim raise (§1.7): visible only as the height of its
  spell's launch; record a bow-armed dragon casting at a target below it.
* **OP3-8** Fields 24 and 25 (§4): confirm they stay 0 on a live server. *Check*: log record
  bytes 0x138–0x14f of several creatures with the mod DLL.
* Pose 37's arm base (trails-poses §2.3, second half of P-1) remains open.
