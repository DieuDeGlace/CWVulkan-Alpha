# Alpha open points, batch 4: the Combat update in exact order, charge release, return values, unstick, flyers

Clean-room behavioural specification. It refines `alpha-creatures-behaviour.md` §13.2 (Combat
0x42cb20) and `alpha-open-points-3.md` §1, and closes these points:

| Point | Spec it belongs to | Section here |
|---|---|---|
| Step 1: the 10 000 ms reset | `alpha-creatures-behaviour.md` §13.2 step 1, §8.2 | §1 |
| Step 9: when a special is drawn; order of the *R* rules | §13.2 step 9 | §2 |
| Step 9: the wind-up exception for 86, 87, 89 | §13.2 step 9 | §3 |
| Step 10: stopping 95 and 28; the 80 intent; what else chase touches | §13.2 step 10 | §4 |
| Step 11: natural weapons, type 25, flag 0x08, hop timing | §13.2 step 11; `alpha-open-points-3.md` §1.8 | §5 |
| Step 11: melee and ranged test order, every draw | §13.2 step 11 | §6 |
| Step 11: release of charged abilities for NPCs | §13.2 step 11; `alpha-projectiles.md` §2.1 | §7 |
| **A-1** what each update returns | `alpha-creatures-behaviour.md` §15; `alpha-materials-wildlife-routes.md` §4.3 | §8 |
| Unstick; static objects | `alpha-physics-camera.md` §4.1; `alpha-player-control.md` §8.1 | §9 |
| Flyers: lift, take-off, landing, height | `alpha-physics-camera.md` §3.5; `alpha-creatures-models.md` §3.3 | §10 |

It also corrects a few statements of earlier specs (§11), notably: unstick lifts **once** per
step by **1.1** blocks and does **not** look at static objects; the chase does not "face the
target"; two-handed NPCs try action 54 before they charge.

Written from the Alpha client (`Cube.exe`) decompile and per-instruction listing, with the
server (`Server.exe`) listing checked where noted (Combat server 0x402f40, unstick server
0x5308b0, step server 0x5322d0); constants were read from the client's data section. No code
was copied; addresses are citations only. *Believed* marks a reading that rests on partial
evidence; *as read* marks behaviour that looks unintended but is what the code does.

Conventions are those of the cited specs: "obj +x" is a creature object offset (record offset
+ 0x10); positions are 3 × i64 in 1/65536 block; *t* is the action time (obj +0x6c); dt the
update's accumulated time; P1, P2, P3 the phase lengths and D = P1 + P2 + P3 the total duration
of the **current** action (routines 0x43caa0, 0x447310, 0x444270, 0x43d1a0); *d*² the squared
3-D distance to the target in blocks; a "draw" is one C `rand()` call (0 … 32767). "own" and
"target" alone mean the x component of the collision box (obj +0x80). "Handled" is the
update's return value (§8). "Common exit" means: aim vector (field 26, obj +0x160) := target −
own position in blocks, then handled (`alpha-open-points-3.md` §1.1).

---

## 1. Step 1: the 10 000 ms reset (0x42cb84 … 0x42cbaa)

The exact condition: the action is **not** 79, 82, 83 or 84, and *t* **>** 10 000 (strict).
Then the action byte := 0.

* The action time is **not** touched; it keeps counting.
* Action 0 is not special: it passes the test too, and writing 0 over 0 changes nothing.
* There is **no** test of D: only the raw time counts. Anything older than 10 s is cleared
  whether or not it has finished, including action 80 and the charge actions.
* It runs first of all (after clearing flag 0x04), before the dodge exit of step 2. It exists
  only in Combat, so only creatures with a Combat behaviour get it, and only on an update.

---

## 2. Step 9: specials and the order of *R* (rule tests 0x42d3f4 … 0x42d464, draw 0x42d494)

### 2.1 The draw

A special is drawn **on every update** that reaches step 9 (a target exists) while the special
timer is exactly 0 and the creature's special list (obj +0x13e8 … +0x13ec, 32-bit ids, from
spawn +0x10ac) is not empty.

* **One draw** *r*; the id is entry *r* mod *n* (unsigned). With one entry the draw still
  happens.
* The result lives in a local of this update only. **It is not stored** anywhere: an update
  that does not end in range throws it away, and the next update draws again.
* The draw **does not touch the timer**. The timer is only written where the special is used.
* The draw happens even with no line of sight (the rule for *R* = 1 then wins).
* **Use**: in step 11, after the taming test, if the draw gave an id ≠ 0: timer := 20 000,
  action := the id, time := 0, common exit. There is **no** "current action finished" test: a
  special interrupts whatever runs. If the taming branch takes the update, the draw is lost and
  the timer stays 0.

So the timer counts down (step 6, on every Combat update that gets that far, target or not),
then at 0 the creature rolls once per update until a roll lands while it is in range for that
roll.

### 2.2 The order of *R*

*R* starts at 2·own + target. The rules run in this order, each overwriting the last:

| # | Condition | *R* := |
|---|---|---|
| 1 | ranged user (0x444520) **and** line of sight, unless the wind-up exception of §3 holds | 50 |
| 2 | type 47, 111, 112 or 113 | 20 |
| 3 | current action 95, 28 or 108 | 50 |
| 4 | current action 72 | own + target |
| 5 | a special was drawn this update (§2.1): 93, 91, 68, 69 | 20 |
| 5 | … 86, 89 | 2·own + target |
| 5 | … any other id | unchanged |
| 6 | no line of sight | 1 |

Then "out of range" = *R*² < *d*², or no sight. Rule 5 acts on the *R* of this update only.

*As read*, for a ranged creature whose list holds 86 or 89: while the timer is 0, every update
that rolls one of those ids has *R* = 2·own + target instead of 50. An archer at 30 blocks
then runs the chase branch on those updates and the in-range branch on the others. It stops
when a roll fires.

### 2.3 Worked example

Bow user, boxes 0.96, specials [86, 93], timer 0, target 30 blocks away in sight.

| Update | Draw | Id | *R* | Result |
|---|---|---|---|---|
| 1 | 7 | 7 mod 2 = 1 → 93 | 20 | 30 > 20: chase; timer stays 0 |
| 2 | 8 | 0 → 86 | 2.88 | chase; timer stays 0 |
| 3, target now 15 blocks | 9 | 1 → 93 | 20 | in range: action 93, time 0, timer 20 000 |

---

## 3. Step 9: the wind-up exception (0x42d3f4 …)

Only inside rule 1 of §2.2. For a ranged user with sight:

* if the current action is 86, 87 or 89 **and** *t* < P1 of that action (strict): rule 1 is
  **skipped**, so *R* stays 2·own + target for this update (later rules still apply);
* otherwise *R* := 50.

It does nothing else. The action is not stopped or changed. If the target is beyond the small
*R*, the creature takes the chase branch while its action keeps running (chase only stops 95
and 28, §4), so it walks during the wind-up. From *t* = P1 on, *R* is 50 again. Examples of
P1: 400 for 86, 5000 for 87, as 37 for 89 (each ÷ *S*·*a* unless fixed,
`alpha-creatures-behaviour.md` §3.3). A creature that is not a ranged user never uses rule 1,
so the exception means nothing for it.

---

## 4. Step 10: chase (give-up up to 0x42eba5; the rest from 0x42ebaa)

In order, on an out-of-range update:

1. Chase time += dt.
2. If chase time ≥ **20 001**: the give-up of step 5 below.
3. **Stop**: if the action is 95 or 28, action := **0**. Time is **not** touched. No other
   action is stopped: a running 86, a charge, a wind-up all continue.
4. Direction := target − own position in blocks. Unless the creature is in water (contact
   0x02), z := 0 **before** normalising, so the unit vector is horizontal.
5. **Intent**. Let *u* be that unit vector (zero if the offset is zero). If |*u*|² < 4·*R*²:
   intent := **40**·*u*; otherwise **80**·*u*. Because *u* is already normalised, the test is
   1 < 4·*R*², that is *R* > 0.5 → 40, and ***R* ≤ 0.5 → 80**. So the earlier text is right, and
   *as read* the test compares a unit vector, not the distance. No-sight *R* = 1 gives 40; with
   default boxes 80 never occurs.
6. With sight, a clear route (0x609480 with radius *R*) and no contact flag 0x20: **clear the
   path** (0x42ef10) and return handled. Nothing is faced here: no aim is written and flag
   0x04 stays clear.
7. Otherwise the target's threat × 0.9 (float 0.9, once per update); the path search start
   and rounds of `alpha-path-search.md`; handled.

**Give-up** (chase time > 20 000): clear the threat map, clear the path, cooldown := 3000,
chase time := 0, **not handled**. Action, intent and flags are left as they are.

Mode flag 0x40 (running) is written in step 8, before any of this; chase neither sets nor
clears it, and a stopped 95 only changes the flag on the next update. Flag 0x04 stays clear
through the whole of chase (cleared in step 1).

---

## 5. Step 11: natural weapons and the flag 0x08 branch

The in-range order after the pools is: magic users; type 101; then the tests below, on the
appearance flags (obj +0x7e). The chase time, intent and flag 0x04 are already set (step 11).

### 5.1 Flags 0x02 and 0x10 together (hop; 0x42e109 … 0x42e25e)

1. Clear flag 0x04.
2. If *d*² ≥ 64: common exit (no draw, flag stays clear).
3. Set flag 0x04. Aim at the target.
4. If *t* ≤ 2000 (the current action's raw time; strict > is needed): common exit, no draw.
5. **One draw**. If it is **odd and** own z − target z ≤ 131 072 (2 blocks, signed 64-bit
   compare): velocity.z := **5**, common exit. Action and time do not change.
6. Otherwise (even, or more than 2 blocks above): action := **51**, time := 0, velocity :=
   target − own position in blocks, then if its length² > 0: velocity := **10** × unit.

It **does not need the old action finished**: only *t* > 2000. Since step 5 leaves *t* alone,
every later update draws again until step 6 fires, usually within two updates. So the rhythm
is: one action 51 about every 2000 ms, with zero or a few extra velocity.z := 5 pushes before it.
This branch always takes the update; a creature with these flags never reaches melee, ranged
or action 75 (crows, flag set 0x32, are in this group).

### 5.2 The weapon gate

The next tests run only if the type is not 104 and the main-hand subtype (obj +0xaa9) is not
6, 7 or 8. Otherwise the update goes to the ranged rule (§6.2). The subtype test does not look
at the item type.

### 5.3 Flag 0x10 (without 0x02; 0x42e2a2 … 0x42e2bf)

No distance test and no draw at all. If D < *t* (strict): if the type is 25 **and** ability 72
is usable now (`alpha-creatures-behaviour.md` §16.4: stun < 1, cost 0 ≤ mana, cooldown[72]
missing or exactly 0): action := 72. Otherwise action := 75. Time := 0 either way. If the
action has not finished: nothing changes. Common exit.

Cooldown[72] is 15 000 ms, recorded by the step when 72 starts, for NPCs too, so a turtle
shells at most once every 15 s and uses 75 in between.

### 5.4 Flag 0x08 (without 0x10)

No distance test, no draw, no mana test, no dodge. The chain starter 0x445410 runs directly.
Its own gate: nothing if dodging or in action 48; else if *t* ≥ D (not strict), less the
recovery allowance when the combo counter ≠ 0 (`alpha-creatures-behaviour.md` §3.4), the action
:= the primary attack (§3.1), time 0. Common exit.

---

## 6. Step 11: melee and ranged in exact order

The dodge's two tests of "finished" and of ground are as below, and differ from the D < *t* of
everything else: the dodge accepts D ≤ *t*.

### 6.1 Melee (no flag 0x10 or 0x08; weapon gate passed)

Let L = **3**·(own + target).

1. If *d*² ≥ L²: go to step 5. **No draw is consumed.**
2. If the action is 59, 63 or 8: go to step 5. No draw. (The dodge-time half of this test is
   always clear here; step 2 of Combat already left.)
3. **One draw**. If draw mod 10 ≠ 0: go to step 5. The draw stays consumed.
4. All of these must hold, else go to step 5 (no second draw): not a big type (0x4445d0: 101,
   107, 108, 109, 114, 115, 116, 117, 118, 119); own < 2 (strict); D ≤ *t*; contact & 3 ≠ 0
   (ground or water). Then let *e* = horizontal (target − own position):
   * if |*e*|² = 0: common exit, **no dodge and no attack this update**;
   * otherwise a **second draw**, draw mod 4 picks velocity (all three axes overwritten), with
     *u* = unit(*e*):

     | mod 4 | Velocity (x, y) | Meaning |
     |---|---|---|
     | 0 | 20·(*u*x, *u*y) | toward |
     | 1 | −20·(*u*x, *u*y) | away |
     | 2 | 20·(*u*y, −*u*x) | to the right |
     | 3 | −20·(*u*y, −*u*x) | to the left |

     then velocity.z := **5**, dodge time := **600**. The action does not change. Common exit.
5. If mana < 0.8 and the action is not 59, 63 or 8: chain starter 0x445410 (§5.4). Common exit.
6. If D < *t* (strict): the mana ability of §7.1. Otherwise nothing. Common exit.

So a failed dodge condition **does** continue to the attack on the same update. Draws per update:
0 outside L or while charging; 1 normally; 2 only on a real dodge.

### 6.2 Ranged (type 104, or main-hand subtype 6, 7, 8; 0x42e742 …)

1. If *d*² ≥ 3600: common exit. **No draw, no attack, no dodge.** The creature stands still,
   flag 0x04 set, aiming. The 60-block limit is therefore **before** the dodge.
2. If the action is 24, 25 or 27: go to step 5. No draw.
3. **One draw**. If draw mod 10 ≠ 0: go to step 5.
4. As §6.1 step 4, same dodge. There is no L: the dodge can fire at any distance under 60.
5. If mana < 0.8 and the action is not 24, 25 or 27: chain starter. Common exit.
6. If D < *t* (strict): the charge of §7.1. Otherwise nothing. Common exit.

*R* for these creatures is 50 at most, so with sight *d* ≤ 50 here and step 1 is **never met in
practice** (*as read*, a dead guard).

### 6.3 Worked examples

Warrior, boxes 0.96, sword and shield, on the ground, action 1 finished, mana 0.9, target 5
blocks east: L = 5.76, 25 < 33.18.

* Draw 23 450: mod 10 = 0. Not big, own < 2, D ≤ *t*, grounded. Second draw 3: *u* = (1, 0) →
  velocity (0, 20, 5), dodge 600. No attack.
* Draw 23 451: mod 10 = 1. Mana ≥ 0.8, D < *t* → §7.1: shield → action 8, time 0.
* Same with target 7 blocks away: 49 ≥ 33.18, no draw at all; goes straight to the mana test.

---

## 7. Step 11: the mana abilities and the release (melee from 0x42e58f; ranged 0x42e908 … 0x42e9f0)

### 7.1 The choice

Reached only with D < *t* (§6.1 step 6, §6.2 step 6). *M* = main-hand item type (obj +0xaa8),
*Ms* its subtype (+0xaa9), *O* the off-hand item type (+0x990), *Os* its subtype (+0x991).
"Charge *c* → *r*" means:

* if the action is already *c*: if mana ≤ charge, action := *r*, time := 0; otherwise nothing
  (the charge goes on, time **not** reset);
* otherwise: time := 0, action := *c*.

Melee, first match:

| # | Condition | Result |
|---|---|---|
| 1 | *Ms* = 5 (longsword) | action 5, time 0 |
| 2 | *M* empty or *O* present, and *Os* = 13 (shield) | charge 8 → 104 |
| 3 | same, *Os* = 3 (dagger) | action 17, time 0 |
| 4 | same, *Os* = 4 (fist) | action 20, time 0 |
| 5 | same, *M* empty | if 54 is usable: **jump**; otherwise the secondary attack 0x4456a0 |
| 6 | same (*M* and *O* present) | action 63 with mana ≤ charge → release: action 11, time := P1(11). Otherwise: if 54 usable: **jump**; else keep or start 63 (time := 0 on starting) |
| 7 | *M* present, no *O* (two-handed) | action 59 with mana ≤ charge → release: specialization 1: action 61, time 0; else action 60, time := P1(60). Otherwise: if 54 usable: **jump**; else keep or start 59 |

**Jump**: velocity.z += **20**, action := 54, time := 0. *As read*, rows 6 and 7 test 54
**before** charging, so these creatures only charge while 54 is on cooldown. P1 of 11, 60 and
61 is 0 (`alpha-creatures-behaviour.md` §3.3), so "time := P1" is 0 today.

Ranged, first match: *M* present and *Ms* = 7: charge 24 → **22**; *M* present and *Ms* = 8:
charge 27 → 26; otherwise, if *Ms* = 6 or the type is 104: charge 25 → 55; otherwise nothing.
The crossbow really releases into 22, unlike the player's 24 → 23 (*as read*).

### 7.2 When the charge rises

NPCs use the same world-step rule as players (`alpha-projectiles.md` §2.1; 0x61a74d … 0x61a918):
with the action in the set 8, 24, 25, 27, 59, 63, 64 and *t* ≥ P1, charge += *S*·dt·0.00075,
with the listed factors; charge := min(charge, mana) every step. There is no separate NPC
rule. The charge rises by step, the release is decided by update (32 ms or more).

Because of the cap, "mana ≤ charge" means **the charge has reached the whole of mana**. The
release also needs D < *t*: the choice is not reached before that. Whichever comes second
decides. Nothing else ends a charge except stun (charge := 0, `alpha-physics-camera.md` §2 step
10), step 1 of Combat at *t* > 10 000, or a special.

### 7.3 What is taken

At the step in which the time of an action is exactly 0 (0x61351a … 0x61361b; actions 31 and 33
take another path, not relevant here):

1. Spent power (obj +0x13b4) := 0.
2. If cost(id) > 0 (§16.4): spent := cost; mana −= cost.
3. If charge > 0: spent := charge; mana −= charge; if the action is 5 or 11: spent × 0.5.
4. Charge := 0. If mana < 0: mana := 0.

Releases 104, 22, 26, 55, 60, 61, 11 all cost 0, so the price is the charge, which equals all
the mana: **mana goes to 0**. Actions 5, 17, 20 and 54 cost 0 as well, so a longsword or dagger
NPC holding mana ≥ 0.8 spends nothing and keeps repeating them (*as read*).

### 7.4 Mana already 0

* Not in a charge action, mana < 0.8 (including 0): the chain starter takes the update. No
  charge starts. NPC charges therefore start only with mana ≥ 0.8.
* Already in a charge action when mana drops to 0 (the charge is capped to 0 with it): the
  action is not cancelled; at the first update with D < *t*, mana ≤ charge is true (0 ≤ 0) and
  the release fires with charge 0, so spent power 0.
* Mana only rises by stealth (§11.2) or blocking (§5 step 5). A shield NPC gains through
  blocking; others stay on the primary attack after one release unless stealth applies.

### 7.5 Worked example (*S* = 1, *a* = 1, dt = 16, updates every 32 ms)

Sword and shield, mana 1.0. Action 8 starts at time 0: cost 0, charge 0. P1(8) = 0, so the
charge rises at once by 0.012 a step. D(8) = 800.

| *t* | Charge | Update result |
|---|---|---|
| 800 | 0.600 | D < *t* false: nothing |
| 816 | 0.612 | D < *t*; mana 1 > charge: keep 8, time not reset |
| 1344 | capped 1.000 (84 steps) | mana ≤ charge: action 104, time 0 |
| next step | | spent 1.0, mana 0, charge 0 |

Afterwards mana < 0.8: chain starter.

---

## 8. A-1: what each update returns

Read from the client listing: the Combat exit sets AL to 0 at exactly two places (0x42d2a7 and
0x42eb95), and every other path passes the single `handled` assignment at 0x42eebe. The server
routine has the same two (0x4036c7, 0x404fb5).

### 8.1 Combat

| Exit | Returns |
|---|---|
| Dodging (step 2) | handled |
| First 3000 ms of action 80 | handled |
| Potion drunk | handled |
| **Pet whose owner is in action 106** | **not handled** (0x42cd70; before step 6, so no timers run) |
| Controller 3, no target, health < 50 % | handled |
| **No target after acquire** (step 8) | **not handled** |
| Chase, clear route | handled |
| Chase, walking or searching | handled |
| **Give-up** | **not handled** |
| Any in-range exit, including taming | handled |

### 8.2 LookAtPlayer and RandomWalk (client 0x4c83ae, 0x4c83c3, 0x4dc1d3)

| Behaviour | Handled | Not handled |
|---|---|---|
| LookAtPlayer | a player within 8 blocks and the action is not 83 or 84 | no such player, or action 83 or 84 (flag 0x04 is cleared in both cases) |
| RandomWalk | **always**, whether or not the timer ran out | never |

This confirms `alpha-materials-wildlife-routes.md` §4.3 for the client and adds the pet row.

### 8.3 Consequences

* In the default sequence, RandomWalk runs on the update of a give-up, and on any no-target
  update where no player is near; LookAtPlayer on those updates runs first.
* The sequence zeroes intent only if **nobody** handled it, so only for sequences without
  RandomWalk. With RandomWalk the sequence never does. RandomWalk writes intent only when its
  timer reaches 0, and its timer does not run while Combat is handling. So *as read*, on a
  give-up the chase intent stays in force until the RandomWalk timer expires (under 8 s, or at
  once if it was already 0), unless LookAtPlayer zeroes it.
* RandomWalk clears running on every run; after a give-up with a player near (LookAtPlayer
  handled) the running flag from step 8 stays set until Combat next rewrites it.

---

## 9. Unstick (client 0x60ab30, server 0x5308b0; called once per creature step at 0x61b47e)

### 9.1 When

Guard: offline, or the creature is the local player; and not controller kind 6
(`alpha-physics-camera.md` §4.1 is right). It runs after the displacement *d* and the substep
count are computed and after the "old position" copy for the stuck test, before the substeps,
exactly **once per step**. Inside: nothing at all if health < 0.

### 9.2 Blocks

**Test box**: the full collision box, cells lo..hi of `alpha-physics-camera.md` §0 (inclusive).
Columns in order x outer, y inner; for each column, all of z from lo.z to hi.z.

A cell counts when: the zone slot and sector are loaded (otherwise the cell is simply skipped,
not solid), and its type (low 5 bits) is neither 0 nor 2. Above and below the column's stored
range, the default blocks apply. **The third default, used below the column's range, is type
1 (solid), grey 200, 200, 200** (static initialiser 0x6fb020; this settles the *inferred* of
`alpha-player-control.md` §8.1). A stored air cell at z < 1 without bit 0x40 reads as water.

After scanning the z's of a column, if any solid was found in it, **lift**, using only pos.z:

* H = trunc(sz/2 · 65536); B = pos.z − H (fixed-point bottom);
* b = B ÷ 65536 toward zero, minus 1 if B is negative (so an exact negative multiple rounds one
  lower, *as read*);
* pos.z := b·65536 + H + **72 089** (trunc of the constant 72 089.6 = 1.1·65536).

So **new bottom = ⌊bottom⌋ + 1.09999**. The landing bob (obj +0x1180) := 0. Velocity and
fall-start are not touched. The rise is between 0.1 and 1.1 blocks, by the fraction of the old
bottom.

**Only one lift per step.** The code has a retry loop, but its counter ends it after the first
lift: no re-test. A creature buried deep rises at most 1.1 blocks a step, but keeps doing so on
every step, at about 50 blocks per second. There is **no** escape sideways, and nothing tests
whether the lift lands in free space.

The trigger is **any** solid cell in the box, including at head height: a creature whose head
overlaps a ceiling goes up, not down, and climbs through the ceiling, step by step, until the
box is clear.

### 9.3 Airships, not statics

The second half of the routine is **not** about static objects. It walks the airship map (world
+0x0c, `alpha-open-points-2.md` §2.4), which is empty in the shipped Alpha. The routine calls
nothing that reaches the zone's static list. So:

* **No unstick exists for statics of any kind.** The sentence "the same is done against static
  objects of the zone" in `alpha-player-control.md` §8.1 is wrong. Statics, doors, items and
  furniture never move a creature. A creature overlapping a static gets out through the
  approach test of `alpha-open-points-2.md` §2.2 (motion away from the static's reference
  point is never blocked).

For an airship hook, the rule, after the block part and with no lift counter involved: for each
airship within 64 blocks with its model present, transform the centre into model space (as
§2.4 there) and take the box lo..hi around it, *unrotated*. If any voxel in it is non-empty, try
four exits in the order +x, −x, +y, −y. An exit is free if the same box shifted one voxel that
way, over the slab tested, has no voxel (for +x: x from lo.x + 1 to hi.x + 1). The candidate
centre is lo.x + 1.05 + sx/2 (+x), hi.x − 0.05 − sx/2 (−x), lo.y + 1.05 + sy/2 (+y), hi.y − 0.05 −
sy/2 (−y). Among free exits take the smallest squared distance from the centre, first wins on a
tie; if none is free nothing moves. The offset is rotated back by the yaw and added to pos; z
never changes. Only one voxel step is ever tried.

### 9.4 Worked example

Crow: no box entry, so box (0.96, 0.96, 2.16). Wildlife spawn at air z = 37 over grass at 36:
pos.z = 37.0 (`alpha-materials-wildlife-routes.md` §3.1 step 11). Bottom 35.92, top 38.08.

| Step | Bottom before | Solid in box | New bottom | Centre |
|---|---|---|---|---|
| 1 | 35.92 | 35, 36 | 35 + 1.09999 = 36.09999 | 37.17999 |
| 2 | 36.09999 | 36 | 37.09999 | 38.17999 |
| 3 | 37.09999 | none | | |

The top is then 39.26. If a crown has a solid cell at z ≤ 39, the crow is still overlapping and
keeps rising through the crown at 1 block a step, and comes to rest **on top** of it. With the
crown at z ≥ 40 it stays below and sinks the remaining 0.1 block to the grass (§10).

---

## 10. Flyers

### 10.1 What the flag does

Appearance flag 0x0002 is read in **only three** simulation places (every reference to obj
+0x7e in the client was listed; server twins checked at 0x540948, 0x5447bc):

1. Combat's hop branch (§5.1), together with 0x10.
2. Vertical forces (0x61ab88, `alpha-physics-camera.md` §3.5): if flag and stun ≤ 0, v.z −=
   **3**·dt_s (a tenth of normal gravity, 30 × 0.1) and fall-start := z. That is all.
3. The walk cycle (0x61e9fc, `alpha-creatures-models.md` §7.3), which is animation only.

Plus the poses. Path search, the follower, RandomWalk, LookAtPlayer, the speed limit, friction
and collision have no flyer case.

### 10.2 The answers

* **There is no lift and no flying height.** No code holds a flyer in the air, seeks a height or
  hovers. A flyer is a creature with one tenth of gravity. No drag acts in the air
  (`alpha-physics-camera.md` §6), so an airborne flyer always sinks and accelerates: from 20
  blocks up it lands after √(2·20/3) ≈ 3.65 s at about 11 blocks/s.
* **There is no take-off rule and no landing rule.** Landing is the ordinary z response
  (§4.2 there) and the ground bit. On the ground the flyer rule keeps running, the collision
  undoes it, and friction is the ground friction. The creature walks.
* **RandomWalk has no z part**, in either branch: wandering intent z is 0 and the toward-home
  intent is forced horizontal.
* **Chase has no z part** unless in water (§4 step 4). In-range and LookAtPlayer intent is 0.
* **Upward intent is discarded anyway**: physics step 4 (0x618fa0) clears a.z > 0 when mode 0x01
  is clear and contact 0x10 is set from the last step, which is true of anything not fully
  under water. So flyers cannot climb by intent.
* **Blocked overhead**: nothing special. A head bump is the z response: v.z := 0, and it sinks
  again.

What can raise a flyer, none of it specific to flyers except the first:

| Source | Effect |
|---|---|
| Hop (§5.1) | v.z := 5 |
| Action 51 launch (§5.1) | velocity 10 × unit(target − own), z included, so only up if the target is higher |
| Melee or ranged dodge (§6) | v.z = 5, if that creature has no flag 0x10 |
| Path follower Lift and partial cases (`alpha-path-search.md` §6.4 step 15) | same as for walkers |
| Unstick (§9) | same as for all |

The hop is what looks like flying in Alpha: at 3 blocks/s² a hop of 5 rises 25/6 ≈ 4.2 blocks
over 1.67 s and takes 3.33 s in all on open ground, against 0.33 s and 0.42 blocks for a walker.

### 10.3 Why a crow under a tree crown ends up on the ground

1. **It starts on the ground.** The wildlife pass, and the other spawn rules with a ground
   point, give z = the first air or water cell scanning up from column +0x14 (surface − 8,
   `alpha-worldgen-terrain.md`). The voxel-tree grid (0x5e9e7d) runs before the wildlife pass
   (0x5eaf75), so tree blocks are already there, but under a crown the scan stops at the ground
   air gap, far below the crown. It does not spawn up in the tree.
2. **Unstick lifts it just clear of the grass** (§9.4), at bottom 37.1.
3. **It sinks the last 0.1 block** in about 0.26 s, and is grounded by the 0.2 probe at once
   (§5.4 there).
4. **Nothing takes it up again.** It wanders on the ground by RandomWalk, with z intent 0.

A crow does end up in a tree in only two ways: a spawn column through a trunk, where the scan
runs up the solid wood and any solid crown above it, or a box tall enough to overlap the crown
(§9.4). Either way it stands on the crown; when it walks off the edge it sinks to the ground at
1/10 gravity and stays there. An implementation must not add a hover, a height or a take-off.

Bats (ravine spawn at the floor, `alpha-creatures-behaviour.md` §2.2) follow the same rules.

---

## 11. Corrections to earlier specs (summary)

* `alpha-creatures-behaviour.md` §13.2 step 1, §8.2: as §1 (raw *t* > 10 000; byte only; time
  kept).
* §13.2 step 9: order and draw as §2.1 and §2.2; "drawn now" means rolled afresh each update.
* §13.2 step 10: "face the target and stop" is **clear the path and return**, nothing faced; the
  stop of 95 and 28 sets action 0, time kept (§4).
* §13.2 step 11: taming exits before flag 0x04 and the look vector are set (the
  flag is set after the taming test). "Melee … Else mana < 0.8" as §6.1. Two-handed and
  dual-wield rows try 54 first (§7.1). "One draw in 10" comes before the box and ground tests,
  and is skipped outside L and in charge actions (§6.1).
* §13.2 step 11, Ranged: the dodge has no 3·box limit (§6.2).
* `alpha-open-points-3.md` §1.8: hop timing as §5.1; the wording "after the current action
  has run > 2000 ms" is right and there is no finished test.
* `alpha-materials-wildlife-routes.md` §4.3, Combat row: add the pet row of §8.1.
* `alpha-player-control.md` §8.1: lift constant is **1.1**, not 1 (the *confirm the +1* is
  closed); **no repeat**; the third default block is type 1, solid; the static-object
  sentence is wrong (§9.3).
* `alpha-physics-camera.md` §4.1 step 1: the cross-reference stands, read with §9 here.
* `alpha-creatures-models.md` §3.3, 0x0002: add that it also lowers gravity (§10.1).
* `alpha-creatures-behaviour.md` §15: **A-1** closed (§8).

---

## 12. Remaining open points

* **OP4-1** obj +0x13e0 is cleared on entry to the in-range branch (0x42d59f) and by a few other
  routines; its reader was not traced. No behaviour here depends on it as far as read.
* **OP4-2** Flag tests made through an appearance pointer (+0x06 rather than obj +0x7e) could not
  be enumerated by search. The ones seen are in the pose and render code. *Believed* none in the
  simulation. *Game check*: from a recording, log z of a crow placed high with a mod DLL; expect
  v.z falling by 3 per second, down to the ground, no recovery.
* **OP4-3** The crow's rest on a crown (§10.3) is deduced, not seen. *Game check*: find a crow on
  a trunk column in a recording, or teleport one above a crown; expect it to stay until it walks
  off.
* **OP4-4** The stale chase intent after a give-up (§8.3). *Game check*: outrun a monster for 20 s
  and log intent; expect it to hold for up to 8 s, then change to a 10-length wander.
* **OP4-5** The minimum real collision box: whether any creature has *R* ≤ 0.5 (§4 step 5) was
  not surveyed. With App. B's listed boxes it was not seen; it matters only for the 80 intent.
