Status: FINAL / STABLE / HELD

# Alpha open points, batch 4: the Combat update in exact order, charge release, return values, unstick, flyers

Clean-room behavioural specification of native Alpha behaviour. It refines the Combat
behaviour described in `alpha-creatures-behaviour.md` §13.2 and `alpha-open-points-3.md` §1,
and covers these points:

| Point | Companion spec (pointer only) | Section here |
|---|---|---|
| Step 1: the 10 000 ms reset | `alpha-creatures-behaviour.md` §13.2, §8.2 | §1 |
| Step 9: when a special is drawn; order of the *R* rules | §13.2 | §2 |
| Step 9: the wind-up exception for 86, 87, 89 | §13.2 | §3 |
| Step 10: stopping 95 and 28; the 80 intent; what else chase touches | §13.2 | §4 |
| Step 11: natural weapons, type 25, flag 0x08, hop timing | §13.2; `alpha-open-points-3.md` §1.8 | §5 |
| Step 11: melee and ranged test order, every draw | §13.2 | §6 |
| Step 11: release of charged abilities for NPCs | §13.2; `alpha-projectiles.md` §2.1 | §7 |
| **A-1** what each update returns | `alpha-creatures-behaviour.md` §15; `alpha-materials-wildlife-routes.md` §4.3 | §8 |
| Unstick; static objects | `alpha-physics-camera.md` §4.1; `alpha-player-control.md` §8.1 | §9 |
| Flyers: lift, take-off, landing, height | `alpha-physics-camera.md` §3.5; `alpha-creatures-models.md` §3.3 | §10 |

Evidence: the Alpha client image `Cube.exe` (image base 0x400000) and server image `Server.exe`
(image base 0x400000), read through their per-instruction listings and decompiles, with
constants read directly from the client image bytes. Every address below is an **RVA** and
names its image; "client" means `Cube.exe`, "server" means `Server.exe`. *Believed* marks a
reading that rests on partial evidence; *as read* marks behaviour that looks unintended but is
what the original does.

Conventions. "obj +x" is an offset from the creature object passed to the Combat routine; the
position block (3 × signed 64-bit, 1/65536 block) begins at obj +0x10. *t* is the action time
(obj +0x6c, 32-bit signed, ms); the action id is the byte at obj +0x68. dt is the update's
elapsed time. D is the total duration of the **current** action as returned by client RVA
0x3d1a0; P1 is the first-phase length of an action as returned by client RVA 0x3caa0 (current
action when no id is given). Both divide their speed-dependent terms by the creature's speed
product (*S*·*a*, companion behaviour spec §3.3). *d*² is the squared 3-D distance from the
creature to its target in blocks. A "draw" is one call of the C runtime random routine
(0 … 32767). "own" and "target" alone mean the x extent of the respective collision box
(obj +0x80, float). "Handled" is the update's boolean return (§8). "Common exit" means: the
aim vector at obj +0x160 … +0x168 := target − own position in blocks (computed on entry to the
in-range branch), then return handled. The behaviour state passed with the update holds the
special timer at +0x04, the chase time at +0x0c and the acquire cooldown at +0x10.

---

## 1. Step 1: the 10 000 ms reset (client RVA 0x2cb84 … 0x2cba8)

The exact condition: the action is **not** 79, 82, 83 or 84, and *t* **>** 10 000 (signed,
strict). Then the action byte := 0.

* The action time is **not** touched; it keeps counting.
* Action 0 passes the test too; writing 0 over 0 changes nothing.
* There is **no** test of D: only the raw time counts. Any action other than the four listed,
  including 80 and the charge actions, is cleared once older than 10 s, finished or not.
* Order: Combat first clears flag 0x04 of the mode word (obj +0x124), then applies this reset,
  then performs the dodge exit of step 2 (obj +0x128 ≠ 0).

---

## 2. Step 9: specials and the order of *R* (client RVA 0x2d3f1 … 0x2d4f0)

### 2.1 The draw (client RVA 0x2d494)

A special is drawn **on every update** that reaches step 9 (a target is held) while the special
timer is exactly 0 and the creature's special list (obj +0x13e8 … +0x13ec, begin and end of an
array of 32-bit ids) is not empty.

* **One draw** *r*; the id is entry *r* mod *n* (unsigned, *n* = number of entries). With one
  entry the draw still happens.
* The id lives only for the current update. **It is not stored**: an update that does not end
  in range discards it, and the next update draws again.
* The draw **does not touch the timer**. The timer is only written where the special is used.
* The draw happens whether or not the target is in sight (the no-sight rule of §2.2 then wins).
* **Use**: in step 11, after the taming test and after flag 0x04 is
  set, if the draw gave an id ≠ 0: timer := 20 000, action byte := low byte of the id,
  time := 0, common exit. There is **no** "current action finished" test: a special interrupts
  whatever runs. If the taming branch takes the update, the drawn id is discarded and the timer
  stays 0.

The timer is decremented by dt and floored at 0 on every Combat update that gets past the
earlier exits of §8.1 (dodge, action 80, potion, pet owner, controller 3), before target
acquisition, target or not. At 0 the creature rolls once
per update until a roll lands while the target is within the *R* computed for that roll.

### 2.2 The order of *R*

*R* starts at 2·own + target (constant 2.0 at client RVA 0x345e10). The rules run in this
order, each overwriting the last:

| # | Condition | *R* := |
|---|---|---|
| 1 | ranged user (client RVA 0x44520) **and** line of sight, unless the wind-up exception of §3 holds | 50 |
| 2 | creature type 47, 111, 112 or 113 | 20 |
| 3 | current action 95, 28 or 108 | 50 |
| 4 | current action 72 | own + target |
| 5 | an id was drawn this update (§2.1) and it is 93, 91, 68 or 69 | 20 |
| 5 | … it is 86 or 89 | 2·own + target |
| 5 | … any other id | unchanged |
| 6 | no line of sight | 1 |

Constants: 50 at client RVA 0x2fcd7c, 20 at 0x2fcd74, 1.0 at 0x345dc0. Line of sight is the
test at client RVA 0x19ee90 with range 200 blocks. Then "out of range" = *R*² < *d*², or no
sight. Rule 5 acts on the *R* of this update only.

The ranged-user test (client RVA 0x44520) is true for: class byte (obj +0x140) 3; type 117,
86 or 104; or main-hand item type (obj +0xaa8) 3 with main-hand subtype (obj +0xaa9) 6, 7, 8,
10, 11 or 12.

*As read*, for a ranged user whose list holds 86 or 89: while the timer is 0, every update
that rolls one of those ids has *R* = 2·own + target instead of 50. An archer at 30 blocks then
runs the chase branch on those updates and the in-range branch on the others, until a roll is
used.

### 2.3 Worked example

Bow user (main-hand type 3, subtype 6), boxes 0.96, specials [86, 93], timer 0, target 30
blocks away in sight.

| Update | Draw | Id | *R* | Result |
|---|---|---|---|---|
| 1 | 7 | 7 mod 2 = 1 → 93 | 20 | 30 > 20: chase; timer stays 0 |
| 2 | 8 | 0 → 86 | 2.88 | chase; timer stays 0 |
| 3, target now 15 blocks | 9 | 1 → 93 | 20 | in range: action 93, time 0, timer 20 000 |

---

## 3. Step 9: the wind-up exception (client RVA 0x2d3f1 … 0x2d416)

Only inside rule 1 of §2.2. For a ranged user with sight:

* if the current action is 86, 87 or 89 **and** *t* < P1 of the current action (signed,
  strict): rule 1 is **skipped**, so *R* stays 2·own + target for this update (later rules
  still apply);
* otherwise *R* := 50.

It does nothing else. The action is not stopped or changed. If the target is beyond the small
*R*, the creature takes the chase branch while its action keeps running (chase stops only 95
and 28, §4), so it moves during the wind-up. From *t* = P1 on, *R* is 50 again. P1 values
(client RVA 0x3caa0): 86 → 400 ÷ (*S*·*a*) (constant at client RVA 0x2fd8ec); 87 → 5000 ÷
(*S*·*a*) (constant at 0x2fd900); 89 → the same rule as 37. A creature that is not a ranged
user never applies rule 1, so the exception has no effect for it.

---

## 4. Step 10: chase (client RVA 0x2eb61 … 0x2eba5 give-up; 0x2ebaa onward the rest)

In order, on an out-of-range update:

1. Chase time += dt.
2. If chase time ≥ **20 001**: the give-up described below.
3. **Stop**: if the action is 95 or 28, action := **0**. Time is **not** touched. No other
   action is stopped: a running 86, a charge or a wind-up all continue.
4. Direction := target − own position in blocks. Unless contact bit 0x02 (obj +0x5c) is set,
   its z := 0 **before** normalising, so the vector is horizontal.
5. **Intent** (obj +0x40 … +0x48). Let *u* be that vector normalised (zero if the offset is
   zero). If |*u*|² < 4·*R*²: intent := **40**·*u*; otherwise **80**·*u* (constants 4 at client
   RVA 0x345e60, 40 at 0x2fce40, 80 at 0x2fce50). Because *u* is a unit vector, the test is
   1 < 4·*R*², that is *R* > 0.5 → 40 and ***R* ≤ 0.5 → 80**. *As read*, the test compares the
   unit vector, not the distance. No-sight *R* = 1 always gives 40.
6. With sight, a clear route (client RVA 0x209480 with radius *R*) and contact bit 0x20 clear:
   **clear the path** (client RVA 0x2ef10) and return handled. Nothing is faced here: no aim is
   written and flag 0x04 stays clear.
7. Otherwise the target's entry in the threat map × 0.9 (float at client RVA 0x2fd31c, once per
   update); then the path search start and rounds of `alpha-path-search.md`; handled.

**Give-up** (chase time > 20 000): clear the threat map (obj +0x13a4, count at +0x13a8), clear
the path, acquire cooldown := 3000, chase time := 0, **not handled**. Action, intent and flags
are left as they are.

Mode flag 0x40 (running) is written in step 8 (client RVA 0x2d237 … 0x2d281), before any of
this; chase neither sets nor clears it, so a stopped 95 only changes the flag on the next
update. Flag 0x04 stays clear through the whole of chase (cleared at entry, §1).

---

## 5. Step 11: natural weapons and the flag 0x08 branch

On entry to the in-range branch the chase time := 0, the path is cleared, obj +0x13e0 := 0
(client RVA 0x2d59f), intent := 0; after the taming test flag 0x04 is set and the aim offset is
computed. The in-range order after the special use and the type-specific branches (types 116,
118, 47/111/112/113, 119) is: the routine at client RVA 0x44610 (the magic-user test of the
companion behaviour spec); type 101; then the tests below, on the appearance flags (obj +0x7e,
16-bit).

### 5.1 Flags 0x02 and 0x10 together (hop; client RVA 0x2e109 … 0x2e25e)

1. Clear flag 0x04.
2. If *d*² ≥ 64 (constant at client RVA 0x2fd330): common exit (no draw, flag stays clear).
3. Aim at the target, then set flag 0x04.
4. If *t* ≤ 2000 (the current action's raw time; strictly more is required): common exit, no
   draw.
5. **One draw**. If it is **odd and** own z − target z ≤ 131 072 (2 blocks; signed 64-bit
   compare; constant 131 072.0 at client RVA 0x2fd33c): velocity z (obj +0x3c) := **5**,
   common exit. Action and time do not change.
6. Otherwise (even, or more than 2 blocks above): action := **51**, time := 0, velocity
   (obj +0x34 … +0x3c) := target − own position in blocks; then if its length² > 0,
   velocity := **10** × its unit vector (constant at client RVA 0x345e74). Common exit.

It **does not require the old action to be finished**: only *t* > 2000. Since step 5 leaves *t*
alone, every later update draws again until step 6 fires; while the creature is no more than 2
blocks above the target, each such update fires step 6 with the even-draw chance of one half. So the rhythm is one action 51
about every 2000 ms, preceded by zero or a few velocity z := 5 pushes. This branch always takes
the update; a creature with both flags never reaches melee, ranged or action 75.

### 5.2 The weapon gate (client RVA 0x2e25e)

The tests of §5.3, §5.4 and §6.1 run only if the type is not 104 and the main-hand subtype
(obj +0xaa9) is not 6, 7 or 8. Otherwise the update goes to the ranged rule (§6.2). The subtype
test does not look at the item type.

### 5.3 Flag 0x10 (without 0x02; client RVA 0x2e2a2 … 0x2e2c3)

No distance test and no draw. If D < *t* (strict): if the type is 25 **and** ability 72 is
usable now (client RVA 0x3e5a0), action := 72; otherwise action := 75. Time := 0 either way.
If the action has not finished: nothing changes. Common exit.

Usable (client RVA 0x3e5a0) means: obj +0x12c < 1 (waived for id 101); the ability cost
(client RVA 0x44ae0) ≤ mana (obj +0x170); and the creature's cooldown map (obj +0x139c) has no
entry for the id or that entry's remaining time is 0; for id 28 mana must also be > 0. The cost
of 72 is 0. The cooldown table (client RVA 0x3e6a0) gives **15 000 ms** for id 72.

### 5.4 Flag 0x08 (without 0x10)

No distance test, no draw, no mana test, no dodge. The chain starter (client RVA 0x45410) runs
directly. Its own gate: nothing if the dodge time (obj +0x128) ≠ 0 or the action is 48; else
the threshold is D, less the current action's final-phase allowance when the combo counter
(obj +0x70) ≠ 0 and the action is not 30 or 32; if *t* ≥ threshold (not strict), action := the
primary attack (client RVA 0x45020), time := 0. Common exit.

---

## 6. Step 11: melee and ranged in exact order

The dodge's "finished" test accepts D ≤ *t*; every other "finished" test in this section is
D < *t*.

### 6.1 Melee (no flag 0x10 or 0x08; weapon gate passed)

Let L = **3**·(own + target) (constant at client RVA 0x2fcd70).

1. If *d*² ≥ L²: go to step 5. **No draw.**
2. If the action is 59, 63 or 8, or the dodge time ≠ 0: go to step 5. No draw. (The dodge
   time is always 0 here; step 2 of Combat already left.)
3. **One draw**. If draw mod 10 ≠ 0: go to step 5. The draw stays consumed.
4. All of these must hold, else go to step 5 (no second draw): not a big type (client RVA
   0x445d0: 101, 107, 108, 109, 114, 115, 116, 117, 118, 119); own < 2 (strict); D ≤ *t*;
   contact & 3 ≠ 0. Then let *e* = horizontal part of (target − own position):
   * if |*e*|² = 0: common exit, **no dodge and no attack this update**;
   * otherwise a **second draw**; draw mod 4 picks the velocity (all three components
     overwritten), with *u* = unit(*e*):

     | mod 4 | Velocity (x, y) |
     |---|---|
     | 0 | 20·(*u*x, *u*y) (toward) |
     | 1 | −20·(*u*x, *u*y) (away) |
     | 2 | 20·(*u*y, −*u*x) (perpendicular) |
     | 3 | −20·(*u*y, −*u*x) (the other perpendicular) |

     then velocity z := **5** (constant at client RVA 0x2fce24; ±20 at 0x2fcd74 and 0x2fce9c),
     dodge time := **600**. The action does not change. Common exit.
5. If mana < 0.8 (double constant at client RVA 0x2fd320) and the action is not 59, 63 or 8:
   chain starter (§5.4). Common exit.
6. If the dodge time is 0 and D < *t* (strict): the choice of §7.1. Otherwise nothing. Common
   exit.

So a failed dodge condition continues to the attack on the same update. Draws per update: 0
outside L or while in 8, 59 or 63; 1 normally; 2 only on a real dodge.

### 6.2 Ranged (type 104, or main-hand subtype 6, 7, 8; client RVA 0x2e72f …)

1. If *d*² ≥ 3600 (constant at client RVA 0x2fd334): common exit. **No draw, no attack, no
   dodge.** The creature stands, flag 0x04 set, aiming. The 60-block limit precedes the dodge.
2. If the action is 24, 25 or 27, or the dodge time ≠ 0: go to step 5. No draw.
3. **One draw**. If draw mod 10 ≠ 0: go to step 5.
4. As §6.1 step 4, the same dodge. There is no L: the dodge can fire at any distance under 60.
5. If mana < 0.8 and the action is not 24, 25 or 27: chain starter. Common exit.
6. If the dodge time is 0 and D < *t* (strict): the charge of §7.1. Otherwise nothing. Common
   exit.

Being in range requires sight and *d* ≤ *R*, and *R* ≤ 50 unless 2·own + target exceeds 50, so
step 1 is not met in practice (*as read*, a dead guard).

### 6.3 Worked examples

Warrior, boxes 0.96, main and off hand present with off-hand subtype 13 (shield), on the
ground, action finished, mana 0.9, target 5 blocks east: L = 5.76, 25 < 33.18.

* Draw 23 450: mod 10 = 0. Not big, own < 2, D ≤ *t*, grounded. Second draw 3: *u* = (1, 0) →
  velocity (0, 20, 5), dodge 600. No attack.
* Draw 23 451: mod 10 = 1. Mana ≥ 0.8, D < *t* → §7.1 row 2: action 8, time 0.
* Same with target 7 blocks away: 49 ≥ 33.18, no draw at all; straight to the mana test.

---

## 7. Step 11: the mana abilities and the release (melee client RVA 0x2e58f …; ranged 0x2e908 … 0x2e9f0)

### 7.1 The choice

Reached only with D < *t* (§6.1 step 6, §6.2 step 6). *M* = main-hand item type (obj +0xaa8),
*Ms* its subtype (+0xaa9), *O* the off-hand item type (+0x990), *Os* its subtype (+0x991);
"charge" is obj +0x144, mana obj +0x170. "Charge *c* → *r*" means:

* if the action is already *c*: if mana ≤ charge, action := *r*, time := 0; otherwise nothing
  (the charge goes on, time **not** reset);
* otherwise: time := 0, action := *c*.

Melee, first match:

| # | Condition | Result |
|---|---|---|
| 1 | *Ms* = 5 | action 5, time 0 |
| 2 | *M* empty or *O* present, and *Os* = 13 | charge 8 → 104 |
| 3 | same, *Os* = 3 | action 17, time 0 |
| 4 | same, *Os* = 4 | action 20, time 0 |
| 5 | same, *M* empty | if 54 is usable: **jump**; otherwise the secondary attack (client RVA 0x456a0) |
| 6 | same (*M* and *O* present) | action 63 with mana ≤ charge → release: action 11, time := P1(11). Otherwise: if 54 usable: **jump**; else keep or start 63 (time := 0 on starting) |
| 7 | *M* present, no *O* | action 59 with mana ≤ charge → release: if obj +0x141 = 1, action 61, time 0; else action 60, time := P1(60). Otherwise: if 54 usable: **jump**; else keep or start 59 (time := 0 on starting) |

**Jump**: velocity z += **20**, action := 54, time := 0. *As read*, rows 6 and 7 test 54
**before** charging, so these creatures charge only while 54 is unusable. P1 of 8, 11, 60 and
61 is 0 (client RVA 0x3caa0), so "time := P1" sets 0.

Ranged, first match: *M* present and *Ms* = 7: charge 24 → **22**; *M* present and *Ms* = 8:
charge 27 → 26; *M* present and *Ms* = 6, or type 104: charge 25 → 55; otherwise nothing.
(With *M* empty only type 104 charges.) The *Ms* = 7 charge releases into 22 (*as read*).

### 7.2 When the charge rises (client RVA 0x21a78c … 0x21a918, inside the creature step at 0x20c510)

The creature step applies one rule to every creature: with the action in the set 8, 24, 25, 27,
59, 63, 64 and *t* ≥ P1, charge += *S*·dt·0.00075 (constant at client RVA 0x31e1e0), multiplied
by further class and buff factors (companion `alpha-projectiles.md` §2.1). Then, on every step
and for every action, charge := min(charge, mana). There is no separate NPC rule. The charge
rises per step; the release is decided per Combat update.

Because of the cap, "mana ≤ charge" means **the charge has reached the whole of mana**. The
release also needs D < *t*, since the choice is not reached before that; whichever comes second
decides. Chase does not stop a charge (§4). Within Combat a charge action is otherwise replaced
only by writes of the action byte: the release, the 10 000 ms reset (§1), a special (§2.1), and
the other branches that set an action (taming, the potion and low-health branches, the
type-specific branches).

### 7.3 What is taken (client RVA 0x21351a … 0x21362d)

At the step in which the time of an action is exactly 0 (actions 31 and 33 take another path,
client RVA 0x2134fa):

1. Byte obj +0x13c0 := 0; spent power (obj +0x13b4) := 0.
2. If cost(id) > 0 (client RVA 0x44ae0): spent := cost; mana −= cost.
3. If charge > 0: spent := charge; mana −= charge; byte obj +0x13b8 := 0; if the action is 5 or
   11: spent × 0.5.
4. Charge := 0. If mana < 0: mana := 0.

Releases 104, 22, 26, 55, 60, 61 and 11 all cost 0 (client RVA 0x44ae0 can return non-zero
only for 3, 4, 31, 33, 34, 37, 43, 45, 46 and 95), so the price is the charge, which equals all the
mana: **mana goes to 0**. Actions 5, 17, 20 and 54 cost 0 as well, so an NPC on rows 1, 3 or 4
holding mana ≥ 0.8 spends nothing and keeps repeating them (*as read*).

### 7.4 Mana already 0

* Not in a charge action, mana < 0.8 (including 0): the chain starter takes the update. No
  charge starts, so NPC charges start only with mana ≥ 0.8.
* Already in a charge action when mana is 0 (the charge is capped to 0 with it): the action is
  not cancelled; at the first update with D < *t*, mana ≤ charge holds (0 ≤ 0) and the release
  fires with charge 0, so spent power 0.

### 7.5 Worked example (*S* = 1, *a* = 1, no class or buff factor, dt = 16 per step, a Combat update every 32 ms)

Main and off hand present, off-hand subtype 13, mana 1.0. Action 8 starts at time 0: cost 0,
charge 0. P1(8) = 0, so the charge rises at once by 0.012 a step. D(8) = 300 + 500 = 800
(client RVA 0x3d1a0).

| *t* | Charge | Update result |
|---|---|---|
| 800 | 0.600 | D < *t* false: nothing |
| 832 | 0.624 | D < *t*; mana 1 > charge: keep 8, time not reset |
| 1344 | capped 1.000 (after 84 steps) | mana ≤ charge: action 104, time 0 |
| next step | | spent 1.0, mana 0, charge 0 |

Afterwards mana < 0.8: chain starter.

---

## 8. A-1: what each update returns

In the client Combat routine (RVA 0x2cb20) the return byte is set to 0 at exactly two places
(RVA 0x2d2a7 and 0x2eb95); every other path to the exit passes the single "handled" store at
RVA 0x2eebe. The server Combat routine (server RVA 0x2f40) has the same two zero stores (server
RVA 0x36c7 and 0x4fb5).

### 8.1 Combat

| Exit | Returns |
|---|---|
| Dodging (step 2) | handled |
| First 3000 ms of action 80 | handled |
| Potion drunk | handled |
| **Pet whose owner is in action 106** | **not handled** (client RVA 0x2cd70; before the timers are decremented) |
| Controller 3, no target held, health < 0.5 × maximum | handled |
| **No target after acquisition** (step 8) | **not handled** (chase time := 0 first) |
| Chase, clear route | handled |
| Chase, walking or searching | handled |
| **Give-up** | **not handled** |
| Any in-range exit, including taming | handled |

### 8.2 LookAtPlayer and RandomWalk

| Behaviour | Handled | Not handled |
|---|---|---|
| LookAtPlayer (client RVA 0xc8140; returns at 0xc83ae, 0xc83c3) | a living player (controller 0, health ≥ 0) with squared distance < 64, and the action is not 83 or 84 | no such player, or action 83 or 84; flag 0x04 is cleared in both cases |
| RandomWalk (client RVA 0xdbf20; single return at 0xdc1d3) | **always**, whether or not the timer ran out | never |

When handled, LookAtPlayer writes the aim at the nearest such player, sets flag 0x04, zeroes
intent and clears the path. RandomWalk clears the running flag 0x40 on every run, decrements its
timer by dt (floored at 0), and only at 0 writes intent (wander: x and y each 10 × a uniform
value in [−1, 1], z 0, while within 10 blocks horizontally of the home point at obj +0x1c0;
otherwise 10 × the horizontal unit vector toward home), then timer := 3000 + (draw mod 5000) and action := 0.

---

## 9. Unstick (client RVA 0x20ab30, server RVA 0x1308b0; called once per creature step at client RVA 0x21b47e, server RVA 0x14123e)

### 9.1 When

Guard (client RVA 0x21b456 … 0x21b47e): world byte +0xb4 is 0, or the creature is the world's
local creature (world +0xb8); and the controller byte (obj +0x60) is not 6. It runs after the
displacement and the substep count are computed and after the copy of the starting position,
before the substeps, exactly **once per step**. Inside: nothing at all if health
(obj +0x16c) < 0.

### 9.2 Blocks

**Test box**: the full collision box (centre ± half of obj +0x80 … +0x88, in 1/65536 units),
converted to inclusive cell ranges lo..hi. Columns in order x outer, y inner; for each column,
every z from lo.z to hi.z.

A cell counts when its zone slot and sector are loaded (otherwise the cell is skipped, not
solid) and the type (low 5 bits of its fourth byte) is neither 0 nor 2. Cells outside the
column's stored range read default blocks:

* below the stored range (or a missing column): type **1** (solid), colour 200, 200, 200
  (static initialiser client RVA 0x2fb020);
* above the stored range: type 0 (air) when z > 0 (initialiser client RVA 0x2faff0), type 2
  with bit 0x80 when z ≤ 0 (initialiser client RVA 0x2fb050);
* a stored cell of type 0 at z < 1 without bit 0x40 reads as the type-2 default.

After scanning the z values of a column, if any counting cell was found in it, **lift**, using
only pos z:

* H = truncation of (size z ÷ 2 × 65536); B = pos z − H (fixed-point bottom);
* b = B ÷ 65536 truncated toward zero, minus 1 when B is negative (so an exact negative
  multiple ends one lower, *as read*);
* pos z := b·65536 + H + **72 089** (truncation of the float 72 089.6 at client RVA 0x31e20c,
  that is 1.1 × 65536).

So **new bottom = ⌊bottom⌋ + 72 089/65536 ≈ ⌊bottom⌋ + 1.09999**. The landing bob
(obj +0x1180) := 0. Velocity and fall start are not touched. The rise is between about 0.1 and
1.1 blocks, by the fraction of the old bottom.

**Only one lift per step.** The routine has a retry loop, but its counter ends it after the
first lift: no re-test. A deeply buried creature rises at most 1.1 blocks a step, again on every
step. There is **no** sideways escape, and nothing tests whether the lift lands in free space.

The trigger is **any** counting cell in the box, including at head height: a creature whose head
overlaps a ceiling goes up, not down, and climbs through the ceiling step by step until the box
is clear.

### 9.3 Airships, not statics

The second half of the routine is **not** about static objects. It walks the airship map at
world +0x0c. The routine calls only the cell-range conversion, the column-cell lookup and
runtime arithmetic helpers; nothing in it reaches a zone's static-object list. So **no unstick
exists for statics of any kind**: statics, doors, items and furniture never move a creature
through this routine.

The airship rule, after the block part and independent of the lift counter: for each airship
within 64 blocks (squared distance ≤ 4096, constant at client RVA 0x302b80) with its model
present, transform the creature centre into model space and take the box lo..hi around it,
*unrotated*. If any voxel in it is non-empty (any of its three bytes non-zero), try four exits
in the order +x, −x, +y, −y. An exit is free if the same box shifted one voxel that way has no
non-empty voxel (for +x: x from lo.x + 1 to hi.x + 1). The candidate centre is
lo.x + 1.05 + sx/2 (+x), hi.x − 0.05 − sx/2 (−x), lo.y + 1.05 + sy/2 (+y), hi.y − 0.05 − sy/2
(−y) (constants 1.05 at client RVA 0x302a50, 0.05 at 0x2fcdb4). Among free exits the smallest
squared distance from the centre wins, the first on a tie; if none is free nothing moves. The
offset is rotated back by the airship's yaw and added to pos; z does not change. Only one voxel
step is ever tried.

### 9.4 Worked example

A creature with box (0.96, 0.96, 2.16) and centre z = 37.0 over a solid cell at z = 36, air
above: bottom 35.92, top 38.08.

| Step | Bottom before | Counting cells in box | New bottom | Centre |
|---|---|---|---|---|
| 1 | 35.92 | 35, 36 | 35 + 1.09999 = 36.09999 | ≈ 37.18 |
| 2 | 36.09999 | 36 | 37.09999 | ≈ 38.18 |
| 3 | 37.09999 | none | | |

The top is then about 39.26. If a solid cell lies at z ≤ 39 above it, the box still overlaps and
the creature keeps rising about 1 block a step until clear, coming to rest on top of that
obstruction. Otherwise it is left about 0.1 block above the ground cell and falls the remainder
under its own gravity (§10).

---

## 10. Flyers

### 10.1 What the flag does

Appearance flag 0x0002 (obj +0x7e) is tested in the client at these places, from a listing of
every direct reference to obj +0x7e (register-mask tests resolved):

1. Combat's hop branch (§5.1), together with 0x10 (client RVA 0x2e10d).
2. Vertical forces in the creature step (client RVA 0x21ab88): if the flag is set and
   obj +0x12c ≤ 0, velocity z −= **3**·dt (0.1 at client RVA 0x2fcb7c × 30 at 0x2fcd78, against
   30·dt for ordinary gravity) and fall start (obj +0x13bc) := current z. That is all.
3. A walk-cycle gate in the creature step (client RVA 0x21e9fc), *believed* animation only.
4. Two reads in the routine at client RVA 0x128f0 (RVA 0x12ef2 and 0x13428), outside the creature
   step and the behaviours, *believed* pose code.

The server step (server RVA 0x1322d0) tests the flag at server RVA 0x140948 and 0x1447bc. The
path search (client RVA 0x209480), RandomWalk and LookAtPlayer have no flyer case, and the
creature step has no other test of 0x0002.

### 10.2 The answers

* **There is no lift and no flying height.** No code holds a flyer in the air, seeks a height or
  hovers. A flyer is a creature with one tenth of gravity. Without air drag (companion
  `alpha-physics-camera.md` §6) an airborne flyer always sinks and accelerates: from 20 blocks up
  it lands after √(2·20/3) ≈ 3.65 s at about 11 blocks/s.
* **There is no take-off rule and no landing rule.** The flyer gravity term has no contact test:
  it runs on the ground too, where the ordinary collision response holds the creature.
* **RandomWalk has no z part**, in either branch: wander intent z is 0 and the toward-home
  intent is horizontal.
* **Chase has no z part** unless contact bit 0x02 is set (§4 step 4). In-range intent is 0 and
  LookAtPlayer intent is 0.
* **Upward intent is cleared** in the creature step (client RVA 0x218fa0): when mode bit 0x01
  (obj +0x124) is clear and contact bit 0x10 (obj +0x5c) is set, a positive intent z := 0. So
  flyers cannot climb by intent in that state.

What can raise a flyer, none of it specific to flyers except the first:

| Source | Effect |
|---|---|
| Hop (§5.1) | velocity z := 5 |
| Action 51 launch (§5.1) | velocity 10 × unit(target − own), z included, so upward only if the target is higher |
| Melee or ranged dodge (§6) | velocity z := 5; reachable by a flyer only without flag 0x10 (and, for melee, without 0x08) |
| Path follower (`alpha-path-search.md`) | as for walkers |
| Unstick (§9) | as for all creatures |

The hop is what looks like flying in Alpha: at 3 blocks/s² a hop of 5 rises 25/6 ≈ 4.2 blocks
over 1.67 s and lasts 3.33 s on open ground, against 0.33 s and 0.42 blocks at ordinary gravity.

### 10.3 Consequence for a flyer near the ground

A flyer whose box overlaps the ground is lifted by unstick to about 0.1 block above it (§9.4),
then sinks that remaining distance in √(2·0.1/3) ≈ 0.26 s. Nothing raises it again except the
sources above; it wanders on the ground with z intent 0. A flyer standing on a raised
obstruction stays there until it walks off the edge, then sinks at one tenth gravity to the
ground and stays there.

---

## 11. Relation to companion specs

For the points listed in the table at the top, the readings of this document apply. Topics
affected: the 10 000 ms reset (§1); the special draw and the order of *R* (§2); the chase stop,
intent and the absence of facing (§4); the hop timing (§5.1); the melee and ranged draw order
and the dodge without an L limit for ranged users (§6); the 54-before-charge order (§7.1); the
Combat return values including the pet row (§8.1); the unstick lift constant 1.1, single lift
per step, type-1 default below the column, and the absence of any static-object unstick (§9);
flag 0x0002 lowering gravity (§10.1).

---

## 12. Remaining open points

* **OP4-1** obj +0x13e0 is cleared on entry to the in-range branch (client RVA 0x2d59f) and by
  other routines; its reader was not traced. No behaviour in this document depends on it as far
  as read.
* **OP4-2** Flag tests made through another pointer to the appearance flags (rather than
  obj +0x7e directly) could not be enumerated by search. *Believed* none in the simulation.
* **OP4-5** Whether any creature has *R* ≤ 0.5 (§4 step 5) was not surveyed; it matters only for
  the 80 intent.
* See also the unresolved list of the re-certification below.

---

## Re-certification (2026-11-15)

Scope: every factual statement of the inherited text was checked against the client and server
listings, decompiles and image bytes. The Infinite-Cubes reconstruction was not consulted.
Addresses were restated as RVAs (image base 0x400000 for both images).

**Confirmed** (against original evidence): the 10 000 ms reset condition and ordering; the
special draw (timer 0, non-empty list, unsigned modulo, not stored, timer 20 000 on use, no
finished test, taming discards it); the order of *R* and all its constants (2, 20, 50, 1); the
wind-up exception and P1 values for 86, 87, 89; the chase order, 20 001 threshold, stop of 95
and 28 with time kept, horizontal direction unless contact 0x02, the 4·*R*² intent test with
40/80, the clear-route return, threat × 0.9, the give-up writes and its not-handled return; the
hop branch (64, 2000, odd draw, 131 072, velocity 5, action 51, 10 × unit); the weapon gate; the
flag 0x10 branch with 72 and 75; the chain starter gate; the melee and ranged order with L = 3,
mod 10, big-type list, own < 2, D ≤ *t*, contact & 3, the mod-4 dodge table, z 5 and 600; the
60-block ranged guard; the melee choice rows 1–7 and jump +20; the ranged charges 24 → 22,
27 → 26; the charge set, P1 test, 0.00075 and the every-step cap; the mana-taking order and the
half for 5 and 11; costs of the releases and of 5, 17, 20, 54; D(8) = 800 at unit speed; the two
not-handled stores (client and server) and every other return; LookAtPlayer and RandomWalk
returns; the unstick guard, call site, box scan order, the type-1 default below the column, the
lift arithmetic with 72 089, the single lift, the bob reset, the airship exits and constants;
flyer gravity 3·dt and fall-start write; the upward-intent clearing; the hop and fall arithmetic.

**Corrected** (old → new):

1. Ranged choice: "otherwise, if *Ms* = 6 or the type is 104: charge 25 → 55" → *Ms* = 6 counts
   only with a main-hand item present; with *M* empty only type 104 charges. (Gameplay.)
2. "Nothing else ends a charge except stun, the 10 000 ms reset or a special" → other Combat
   writes of the action byte (taming, potion, low-health and type-specific branches) also
   replace it; the stun clause was not re-verified and is unresolved. (Partial reversal;
   gameplay.)
3. Flag 0x0002 "only three simulation places … plus the poses" → the places are listed
   explicitly, including two reads at client RVA 0x12ef2 and 0x13428 (*believed* pose code); the
   walk-cycle gate is marked *believed* animation only.
4. Default blocks: added that above the stored range z ≤ 0 reads the type-2 default and z > 0
   reads air, and that the "water" reading of a stored air cell at z < 1 is the type-2 default
   (its identity as water is a companion-spec matter).
5. Flyer dodge row: "if that creature has no flag 0x10" → without 0x10, and for melee also
   without 0x08.
6. Chain starter: added that actions 30 and 32 get no final-phase allowance.
7. Usability of an ability: added the waiver of the stun test for id 101 and the extra mana > 0
   requirement for id 28.
8. §6.2 "*R* is 50 at most" → *R* ≤ 50 unless 2·own + target exceeds 50.
9. Hop order: the aim is written before flag 0x04 is set (previously listed the other way).
10. §7.5 table: the second row now uses an update time (832) consistent with 32 ms updates; the
    example states its no-factor assumption.
11. D is now defined as the value of client RVA 0x3d1a0 and P1 as that of 0x3caa0; the inherited
    equation D = P1 + P2 + P3 over four named routines was not re-established (that routine
    computes its phases itself) and is not used here.

**Removed**: all implementer advice ("an implementation must not add …"); all game-check
procedures (recordings, mod DLLs, teleports, logging) from OP4-2, OP4-3 and OP4-4; the
right/left labels of the dodge table (handedness not established); the assertions about what
companion specs say and which of their sentences are wrong (§11 reduced to pointers); the
spawn-source offset of the special list; the crow-specific narrative depending on unverified
spawn and world-generation facts.

**Unresolved**:

* Whether the 10 000 ms reset exists only in Combat: another comparison of an action time with
  10 000 exists at client RVA 0x9c2d9 (routine RVA 0x88ee0), not examined.
* When and for which creatures the 15 000 ms cooldown of action 72 is recorded in the cooldown
  map (the table value is confirmed; the recording path was not found), and hence the "at most
  once every 15 s" rhythm.
* That stun clears the charge to 0 (companion physics spec step 10).
* Mana sources for NPCs (stealth, blocking).
* The default-sequence consequences of the return values: that the sequence zeroes intent only
  when nothing handled the update, that RandomWalk's timer does not run while Combat handles,
  and therefore that a chase intent persists after a give-up until RandomWalk's timer (3000 …
  7999 ms) expires.
* That the airship map is empty in the shipped Alpha.
* The meaning of world byte +0xb4 (offline flag) and the rate of steps per second (so the
  "≈ 50 blocks per second" unstick climb rate).
* The identity of the flag set 0x32 with crows, of type 25 with turtles, and the crow and bat
  spawn heights and the world-generation order (client RVA 0x1e9e7d before 0x1eaf75) used in the
  inherited §10.3.
* That contact bit 0x10 is set for anything not fully under water, that no air drag applies,
  and the ground probe and friction behaviour after landing (companion physics spec).
* That obj +0x12c is the stun timer and obj +0x141 the specialization byte (names only; the
  tested values are confirmed).
* OP4-1, OP4-2 and OP4-5 above.
