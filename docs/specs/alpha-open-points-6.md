Status: FINAL / STABLE / HELD

# Alpha open points, batch 6: how a melee strike becomes damage, and the class of a spawn

Clean-room behavioural specification. It covers these points:

| Point | Companion spec (pointer only) | Section here |
|---|---|---|
| Where the world step decides that a melee hit lands; reach and hit box | `alpha-creatures-behaviour.md` §3.3, §8.1 | §1 |
| The value x handed to the hit routine for a melee hit | behaviour §4.2, §5 | §2 |
| Knock-back vector and stun (hit-record field 0x18) | behaviour §5, `alpha-net-packets.md` §5.1 | §3 |
| The area flag and the magic flag | behaviour §5 | §4 |
| Whether the "damage stat" is what strikes use; the weapon damage W | behaviour §4.2, `alpha-projectiles.md` §2.2 | §5 |
| Which spawn types have their class forced or remapped | `alpha-spawn-objects.md` §4.2, behaviour §9.3 | §6 |

Summary: strikes do not use the damage stat (Cube.exe RVA 0x3e190). Weapon strikes use the weapon
damage W (RVA 0x3dc60), magic strikes the magic base M (RVA 0x46e80). The spread is one draw,
1.25 down to 0.75. A critical doubles x.

Evidence: the Alpha client image `Cube.exe` (image base 0x400000). **Every address in this document
is an RVA in `Cube.exe`** unless it is explicitly marked as a data-section RVA of the same image. The
world-step action switch sits at RVA 0x211e1b (jump table RVA 0x222c84, byte index table RVA
0x222ccc, indexed by action − 1, actions above 110 go to the default case). The world step's body
could not be decompiled; it was read instruction by instruction. The server image `Server.exe` was not
compared in this pass (§9). Addresses are citations only.

*As read* marks behaviour that looks unintended but is what the code does; *believed* marks a
reading that rests on partial evidence.

Conventions: *t* is the creature's action time (obj +0x6c) before this step's advance, dt the step
in ms, "draw" one C-runtime `rand()` result (0..32767). "obj +x" is a creature object field. The
action id is obj +0x68 (byte); the controller kind obj +0x60; the type obj +0x64; the level obj
+0x190; the tier byte obj +0x1a8; the class obj +0x140; the specialization obj +0x141; health
obj +0x16c; mana obj +0x170; the charge obj +0x144; the combo counter obj +0x70; the combo timer
obj +0x74; the damage multiplier obj +0x180; the appearance flags word obj +0x7e. *b* = (bx, by, bz)
is a creature's box (obj +0x80, three floats). *S* is the attack-speed factor returned by RVA 0x47700.
P1 and P2 are the first two action phases (RVA 0x3caa0 and RVA 0x47310); the action's total duration
is RVA 0x3d1a0. Arithmetic is single precision in exactly the order written; where a draw is scaled,
the order is given explicitly.

---

## 1. Where a melee hit lands

All of this is one block of the world step, the switch's first case, RVA 0x212357 up to RVA 0x214c32.
The hit call is at RVA 0x21476b. There is no separate melee routine.

### 1.1 Which actions

The first case serves actions 1–7, 9–21, 30–33, 51, 54, 57, 58, 60, 61, 62, 65–70, 72, 74–78,
86, 87, 88, 91, 93, 104 (read from the index table). These are the "melee" actions. Action 8 has its
own case (RVA 0x2121f4). Actions 28, 94, 95 go to the beam case of §1.7. Action 73 has its own case
(RVA 0x216d35), not traced.

### 1.2 Who runs it

The block is entered (RVA 0x212357) unless all of these hold: the world's online flag (world +0xb4)
is clear, the creature's controller is 0, and the creature is not the local player (world +0xb8).

| Machine | Creature | Runs the block? | What the result does |
|---|---|---|---|
| Offline client | local player | yes | the hit routine applies it at once (RVA 0x195a60) |
| Offline client | creature with controller ≠ 0 | yes | applied at once |
| Offline client | controller-0 creature that is not the local player | **no** | |
| Online client | any creature | **yes** | the hit routine never applies (it calls RVA 0x195a60 only with the online flag clear) |

The hit routine refuses as a target an online client's own local player (RVA 0x196d9f). Owner tests
inside the block guard mana on hit (§2.6) and the application of the heal record (§1.6).

Additional native detail at block entry (RVA 0x212378): an attacker of class 4 and specialization 1
in action 5, 17 or 20 with *t* = 0 sets its own obj +0x128 to 600. (obj +0x128 is the field the hit
test reads as "dodging", §1.5 step 6.)

### 1.3 The hit window

Let ⌊ ⌋ truncate toward zero (integer division).

* **Normal actions.** A = P1 + ⌊P2/2⌋ − 100, raised to P1 if below it. B = P1 + ⌊P2/2⌋, lowered to
  P1 + P2 − 1 if it is ≥ P1 + P2.
* **68, 69:** A := P1 + ⌊P2/3⌋; B as above.
* **93:** A := P1 + ⌊P2/5⌋; B := P1 + P2.
* **Pulsing actions** 5, 11, 30, 31, 32, 33, 72, 86, 91 (RVA 0x212cb2–0x213052). Pulse length T:
  250 ms; 1000 ms for 91; ⌊P2/3⌋ for 5 and 11; ⌊P2/6⌋ for 30 and 32; ⌊P2/12⌋ for 31 and 33.
  While P1 ≤ *t* < P1 + P2: A = P1 + ⌊(*t* − P1)/T⌋·T and B = A + 100; then, if
  P1 + ⌊(*t* + dt − P1)/T⌋·T differs from A and is below P1 + P2, a **new pulse** starts: obj +0x1314
  := combo, both swing sets (§1.5) are emptied, and the step's pulse mark is set. The pulse mark is
  also set when *t* + dt > B. Outside P1 ≤ *t* < P1 + P2: A = B = P1, an empty window.

The strike runs in **every step with A < *t* ≤ B**, so a normal swing tests for targets over a
100 ms span of action time, not once.

### 1.4 Reach (RVA 0x213838–0x2139c5)

From the attacker's own box: start with r = 1.5·bx, f = 1.5, then apply each row that matches, in
this order.

| Actions | r | f |
|---|---|---|
| 1, 2, 3, 4, 6, 7, 9, 13, 14, 18, 19 | 2·bx | 1 |
| 20, 21 | × 2 | unchanged |
| 68, 70 | unchanged | 0 |
| 86, 93, 104 | × 2 | 0 |
| 87, 88 | 8 (absolute) | 0 |
| 30, 32 | 3 (absolute) | 0 |
| 31, 33 | 5 (absolute) | 0 |
| 74, 91 | 20 (absolute) | 0 |
| 72, 76, 77, 78 | × 1.2 | 0 |
| 11, 54, 61 | × 3 (applied after the origin step below) | origin replaced, see below |

**Strike origin O** (fixed-point world position): the attacker's position plus the offset
(−sin yaw, cos yaw, 0) × (bx·f), with yaw = obj +0x30 in degrees (converted by 0.01745329).
Then:

* 30–33: O := position + the aim vector obj +0x160.
* 69, 75: O.z += 0.5·(bz − bx) of the attacker.
* 11, 54, 61: O := position, and r ×= 3.

**Sight origin Q**: the attacker's position; for 30–33, O + (0, 0, r).

### 1.5 The hit test, for each creature C in world-list order (RVA 0x214178–0x214bb7)

1. C exists, has health > 0, and is not in the attacker's **hit set** (obj +0x11ac). The attacker
   itself is not excluded here; it is left to the can-damage test of step 5 and, when that refuses,
   to the heal rule of §1.6.
2. **Horizontal**: with the x and y components of (O − C.position) in fixed point, each component is
   squared and divided by 65536 (64-bit, truncating) and the two results summed; the test passes when
   that sum is below ((0.7·C.bx + r)² × 65536) computed in single precision and converted to a 64-bit
   integer by truncation.
3. **Vertical**: |O.z − C.z| in blocks < 0.5·C.bz + attacker.bz + r.
4. **Sight**: RVA 0x19ee90(Q, C.position, 1, 200).
5. **Can damage**: RVA 0x1988a0(attacker, C). If false, only the heal rule of §1.6 applies.
6. **Dodging**: if C's obj +0x128 ≠ 0, C goes into the attacker's **evaded set** (obj +0x11b4) and
   is skipped.
7. **Hit**: the critical draw and x of §2, then C goes into the hit set, the "landed" byte obj +0x13b8
   := 1, and the hit routine runs. The landed byte is set even if the hit is then refused, blocked or
   the target is immune.

**One hit per target per swing or pulse.** A swing has no target limit; each target gets its own
critical and spread draws.

**The sets.** In every step with *t* ≤ P1 (RVA 0x213058–0x2131f9): obj +0x1314 := combo; every id in
the evaded set that is not in the hit set is looked up and, if it exists, gets the dodge notice
(RVA 0x3cf20) and a kind-4 hit record (attacker id, target id, amount 0, critical 0, target
position); then both sets are emptied and the landed byte := 0. In every step with *t* ≥ B
(RVA 0x2132cc–0x213442) the same evaded-set processing runs and only the evaded set is emptied. New
pulses also empty both sets (§1.3).

### 1.6 After the hit routine returns

The hit routine (RVA 0x196d30) returns a value. False (RVA 0x197014, RVA 0x19722e): target dead, an
online client's own player as target, attacker action 91 or 74 against a target whose obj +0x5c bit 0
is clear, immune target, blocked hit. True (RVA 0x1986d5): every other completed hit, including one
reduced to 0 and one fully absorbed.

On true, in order:

1. Action 30: chance test RVA 0x441a0 with 0.25 (one draw; the chance is first halved when the
   main-hand item is type 3 of subtype 0, 1, 2 or 12, and multiplied by 0.3 for subtype 8 or 10). On
   success and specialization 0: a buff of kind 9 for 30 000 ms, a buff-change record, sound 47 at
   the attacker.
2. **Combo**: if the hit set was empty before this target and the attacker's stun counter obj +0x12c
   ≤ 0: combo += 1, RVA 0x453b0 is called, combo timer := 0.
3. Area flag set and attacker class 4, specialization 0: obj +0x1190 += s, capped at 1.
4. **Mana on hit**, §2.6.

**Heal rule** (RVA 0x214a11–0x214bac), run after every target that passed steps 1–4 of §1.5 when the
action is 32 or 33 and the step's pulse mark (§1.3) is set: if can-damage(attacker, C) is false and
the attacker has specialization 1, a heal record is built on C: amount = (σ × M) × (0 + 1) × (−0.1),
σ from one draw as in §2.1; then one critical draw whose result is stored in the record's critical
byte (the amount is not doubled). Sound 42 at the attacker, pitch 1. The record is applied at once
(RVA 0x195a60) when the online flag is clear, or when attacker and C are both the local player.

### 1.7 The beam (actions 28, 94, 95; RVA 0x215c68–0x2169d7)

Not melee, but it uses the same hit routine (RVA 0x2166c8). Run only when the online flag is clear and
the controller ≠ 0, or by the local player.

* **Mana.** Action 28 drains mana each step (below). For 94 and 95, in the step with
  *t* ≤ P1 < *t* + dt, mana −= RVA 0x44ae0(action) (0.3 for 95 unless a kind-9 buff is active, 0 for
  94) and an active kind-9 buff is consumed. If mana is then below 0, it is set to 0 and the beam
  does not fire in that step.
* **Firing.** 94 fires in the step where P1 is crossed (*t* ≤ P1 < *t* + dt). 28 and 95 fire then,
  and in every step with *t* < P1 + P2, *t* + dt > P1 and ⌊(*t* + dt − P1)/100⌋ ≠ ⌊(*t* − P1)/100⌋.
* **Each firing:** the hit set and evaded set are emptied. Direction = the aim vector obj +0x160,
  normalised if its squared length > 0. Origin = RVA 0x46bb0. Length = the ray cast RVA 0x1a35d0
  (limit 200). The walk tests at origin + k·direction for k = 0, 1, 2, … while k < length.
* **At each point**, for each creature C in list order: C exists, is not the attacker, its id is not
  the one stored at attacker obj +0x11c0, health > 0, not in the hit set, obj +0x128 = 0, horizontal
  test as §1.5 step 2 with threshold (0.5·C.bx + 1)², and |dz| in blocks < 0.5·C.bz + 1. Then C goes
  into the hit set; one critical draw; one spread draw; x = (M × 1.5) × (1 + (draw × 0.05)/32767),
  × 2 if critical. If can-damage is true, the hit routine runs with magic flag set, area flag clear,
  area value 0.1 for 95 and 0 otherwise, knock-back (0, 0, 0), action argument 0, can-be-blocked 1.
  When the attacker's action is 28 the hit routine itself takes one pitch draw first.
* On a true result and attacker specialization 2: a heal record of amount −x on the attacker; if the
  online flag is clear, the attacker's health rises by x, capped at its maximum (RVA 0x44db0).
* If the hit routine was called for any creature in the firing: combo += 1, RVA 0x453b0, combo timer
  := 0.
* Class-3 specialization-1 attackers emit bursts at the P1 crossing (RVA 0x21608a) and, in that
  step, after each hit-routine call (RVA 0x21681b); their content is not described here.

Action 28 additionally drains mana each step by 0.0002 times a step-local value (RVA 0x215d66, §9).

---

## 2. The damage x

### 2.1 The formula (RVA 0x214561–0x21469c)

With *d* the spread draw, σ = 1.25 − (*d* × 0.5)/32767, from 1.25 (draw 0) to 0.75 (draw 32767).

**Weapon actions** (every melee action except 30, 31, 32, 33, 87, 88):
x = (σ + 0) × ((a(id) × W) × ((s × s) × 5 + B)), then × 2 if critical.

**Magic actions** 30, 31, 32, 33, 87, 88:
x = (σ × (a(id) × M)) × (0 + 1), then × 2 if critical. The power s does not enter x.

There is no other factor in x: no level-difference term, no NPC/player factor beyond what W or M
contain, no swing, charge-time or combo factor. The combo enters later, inside the hit routine, as the
factor (1 − min(combo/comboMax, 1)) on armour or resistance and on absorption, and in the spirit
effects (combo ratio > 0.25).

### 2.2 The terms

**a(id)** (RVA 0x3ec90), complete: 0.5 for 5, 26; 0.1 for 21, 72; 0.4 for 30, 32; 0.6 for 31, 33;
2 for 37, 43, 57, 58, 68; 0.25 for 55; 1 for every other id.

**B**: 0 for actions 60, 11, 62; 1 for all others.

**s**, the spent power (obj +0x13b4). In every melee step except actions 31 and 33:

1. At *t* = 0: s := 0 and the "sure critical" byte obj +0x13c0 := 0. Then cost = RVA 0x44ae0(action);
   among melee-case actions only 3 and 4 have a cost here (0.1). If cost > 0: s := cost, mana −= cost.
2. Otherwise at *t* = 0: for 17, 20, 5: s := mana, mana := 0, and if a buff of kind 11 is active it is
   consumed (duration 0, buff-change record) and the sure-critical byte := 1. For 54, 21, 87, 88:
   s := 1, no mana taken.
3. Every step: if the charge > 0: s := charge, mana −= charge, the landed byte := 0, and for actions 5
   and 11 s ×= 0.5. Then charge := 0, and mana is floored at 0.

Actions 31 and 33 do not reset s at *t* = 0. In the step with *t* ≤ P1 < *t* + dt: s := 0.1 and mana
−= RVA 0x44ae0(action), which is 0.3, or 0 while a buff of kind 9 is active; mana floored at 0. Step 3
(charge) runs every step. Action 31, in strike steps (A < *t* ≤ B, which implies *t* ≥ P1), also
consumes an active buff of kind 9 (buff-change record) before the hit test.

So a plain primary attack has s = 0 and the bracket is 1. Action 5 with s = 1 has
0.5·W·(5·1 + 1)·σ = 3·W·σ.

**W** and **M**: §5.2.

### 2.3 Critical and the order of the draws

The draws for one target C that passed §1.5 steps 1–6, in order:

| # | Draw | When |
|---|---|---|
| 1 | critical, RVA 0x444a0 | unless the sure-critical byte is set (then no draw and the hit is critical) |
| 2 | σ | always |
| 3 | absorb sound pitch | if the target's kind-6 buffs absorb an amount > 0 |
| 4 | impact sound pitch | can-be-blocked, not magic, action argument ≠ 28 |
| 5 | hurt voice pitch | critical and target controller ≠ 6, also for a type with no voice |
| 6 | stun | only if the gate of §3.2 opens |
| 7 | action-30 chance test | action 30 and the hit routine returned true (§1.6) |
| 8, 9 | mana on hit | only under §2.6 |

Before row 3 the hit routine has, in order, its immune test and its block test; each of those paths
takes one pitch draw and ends the routine. Draws, if any, inside the spirit effects (RVA 0x19a800)
were not traced (§9).

**Critical chance** (RVA 0x444a0): exactly one draw is always taken; the hit is critical when
draw/32767 < p. With an active buff of kind 11, p = 1 (so the hit is critical unless the draw is 32767,
*as read*). Otherwise p = RVA 0x3ea40 + 0.15 × obj +0x1190, where RVA 0x3ea40 = (P × 2^0 / 2^3) × 0.1
plus the crit value (RVA 0xc6ba0) of each of these item slots, in this order, each only if it holds
the expected item type: slot 6 (obj +0x990, type 3), slot 7 (+0xaa8, type 3), slot 2 (+0x530, type 4),
slot 3 (+0x648, type 6), slot 4 (+0x760, type 5), slot 5 (+0x878, type 7), slot 1 (+0x418, type 8),
slot 8 (+0xbc0, type 9), slot 9 (+0xcd8, type 9). P is the level power of §5.2. A level-29 creature
with no such items has p ≈ 0.042.

**Factor**: exactly 2, applied to x before the hit routine, so before armour.

### 2.4 Which weapons W counts (RVA 0x3df60)

W adds the item damage of a list that depends on the action. Slot 7 (obj +0xaa8) is the main hand,
slot 6 (obj +0x990) the off hand; the type byte is at +0 of the slot, the subtype at +1.

1. Main hand is type 3 with subtype 6 or 7: the main hand only, whatever the action.
2. Else off hand is type 3 with subtype 6 or 7: the off hand only.
3. Otherwise by the current action:

| Actions | List |
|---|---|
| 2, 4, 7, 8, 10, 18, 39, 41 | off hand, if it is type 3 |
| 11, 12, 16, 17, 20, 54, 86, 91, 96 | off hand, then main hand, each if type 3 |
| 40, 42 | main hand, if type 3 |
| all others | main hand if type 3; then the off hand if it is type 3 with subtype 12 |

An action whose list selects an empty or non-type-3 slot has an empty list, and W then takes the
natural-weapon terms of §5.2 instead of item damage.

### 2.5 Hit frequency

One hit per target per swing or per pulse (§1.5). Each pulse rolls its own critical and σ.

The landed byte is cleared in steps with *t* ≤ P1 and when a charge is spent. If the landed byte is
still 0 in the step where *t* + dt ≥ P1 + P2, *t* < the total duration, and the attacker's obj +0x128
is 0, the combo is reset to 0 and the action time is rescaled by the ratio of the old to the new
attack-speed factor (RVA 0x213448–0x2134ee).

### 2.6 Mana on hit (RVA 0x214906–0x2149fb)

Runs after a true hit only when: the online flag is clear or the attacker is the local player; no
earlier target in this step had a true hit; the area flag is clear; the action is not 30 or 32. Then
*g* = (1 − (*d* × 2)/32767) × 0.05 + 0.1 with one draw *d*; *g* := (total ÷ 500) × *g*, total being
the action's total duration (RVA 0x3d1a0); one more draw, and *g* ×= 1.5 if that draw mod 8 = 0;
mana := min(1, mana + *g*).

### 2.7 The action table

"Base" is W or M. "Weapons" uses §2.4: M = main hand (plus a subtype-12 off hand), O = off hand,
B = both. s: "—" means 0 unless a charge is spent. r and f are those of §1.4; "self" means the origin
is the attacker's own position. *k* is the knock-back of §3.1. "Stun" marks a v override to 1 in
§3.2. Pulse is the T of §1.3.

| Id | Base | a | B | s | Weapons | r | f | Pulse | *k* | Stun |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | W | 1 | 1 | — | M | 2·bx | 1 |  | 6/S |  |
| 2 | W | 1 | 1 | — | O | 2·bx | 1 |  | 6/S |  |
| 3 | W | 1 | 1 | 0.1 | M | 2·bx | 1 |  | 3/S |  |
| 4 | W | 1 | 1 | 0.1 | O | 2·bx | 1 |  | 3/S |  |
| 5 | W | 0.5 | 1 | all mana | M | 1.5·bx | 1.5 | P2/3 | 4 |  |
| 6 | W | 1 | 1 | — | M | 2·bx | 1 |  | 3/S |  |
| 7 | W | 1 | 1 | — | O | 2·bx | 1 |  | 3/S |  |
| 9 | W | 1 | 1 | — | M | 2·bx | 1 |  | 6/S |  |
| 10 | W | 1 | 1 | — | O | 1.5·bx | 1.5 |  | 2/S |  |
| 11 | W | 1 | 0 | — | B | 4.5·bx | self | P2/3 | 10 |  |
| 12 | W | 1 | 1 | — | B | 1.5·bx | 1.5 |  | 2/S | 1 |
| 13 | W | 1 | 1 | — | M | 2·bx | 1 |  | 6/S |  |
| 14 | W | 1 | 1 | — | M | 2·bx | 1 |  | 6/S |  |
| 15 | W | 1 | 1 | — | M | 1.5·bx | 1.5 |  | 6/S |  |
| 16 | W | 1 | 1 | — | B | 1.5·bx | 1.5 |  | 2/S |  |
| 17 | W | 1 | 1 | all mana | B | 1.5·bx | 1.5 |  | 6 |  |
| 18 | W | 1 | 1 | — | O | 2·bx | 1 |  | 3/S |  |
| 19 | W | 1 | 1 | — | M | 2·bx | 1 |  | 3/S |  |
| 20 | W | 1 | 1 | all mana | B | 3·bx | 1.5 |  | 6 |  |
| 21 | W | 0.1 | 1 | 1 | M | 3·bx | 1.5 |  | 30·v + 5 |  |
| 30 | M | 0.4 | — | — | M | 3 | 0 | P2/6 | 0 |  |
| 31 | M | 0.6 | — | 0.1 at P1 | M | 5 | 0 | P2/12 | 0 |  |
| 32 | M | 0.4 | — | — | M | 3 | 0 | P2/6 | 0 |  |
| 33 | M | 0.6 | — | 0.1 at P1 | M | 5 | 0 | P2/12 | 0 |  |
| 51 | W | 1 | 1 | — | M | 1.5·bx | 1.5 |  | 2/S |  |
| 54 | W | 1 | 1 | 1 | B | 4.5·bx | self |  | 5 |  |
| 57 | W | 2 | 1 | — | M | 1.5·bx | 1.5 |  | 2/S |  |
| 58 | W | 2 | 1 | — | M | 1.5·bx | 1.5 |  | 2/S | 1 |
| 60 | W | 1 | 0 | — | M | 1.5·bx | 1.5 |  | 6/S |  |
| 61 | W | 1 | 1 | — | M | 4.5·bx | self |  | 6/S |  |
| 62 | W | 1 | 0 | — | M | 1.5·bx | 1.5 |  | 3/S |  |
| 65 | W | 1 | 1 | — | M | 1.5·bx | 1.5 |  | 10/S |  |
| 66 | W | 1 | 1 | — | M | 1.5·bx | 1.5 |  | 10/S |  |
| 67 | W | 1 | 1 | — | M | 1.5·bx | 1.5 |  | 2/S |  |
| 68 | W | 2 | 1 | — | M | 1.5·bx | 0 |  | 2/S | 1 |
| 69 | W | 1 | 1 | — | M | 1.5·bx | 1.5 |  | 2/S | 1 |
| 70 | W | 1 | 1 | — | M | 1.5·bx | 0 |  | 2/S |  |
| 72 | W | 0.1 | 1 | — | M | 1.8·bx | 0 | 250 | 2/S |  |
| 74 | W | 1 | 1 | — | M | 20 | 0 |  | 2/S | 1 |
| 75 | W | 1 | 1 | — | M | 1.5·bx | 1.5 |  | 2/S |  |
| 76 | W | 1 | 1 | — | M | 1.8·bx | 0 |  | 2/S |  |
| 77 | W | 1 | 1 | — | M | 1.8·bx | 0 |  | 2/S |  |
| 78 | W | 1 | 1 | — | M | 1.8·bx | 0 |  | 2/S |  |
| 86 | W | 1 | 1 | — | B | 3·bx | 0 | 250 | 2/S |  |
| 87 | M | 1 | — | 1 | M | 8 | 0 |  | 10 |  |
| 88 | M | 1 | — | 1 | M | 8 | 0 |  | 30·v + 5 |  |
| 91 | W × 2 | 1 | 1 | — | B | 20 | 0 | 1000 | 2/S | 1 |
| 93 | W × 5 | 1 | 1 | — | M | 3·bx | 0 |  | 2/S | 1 |
| 104 | W | 1 | 1 | — | M | 3·bx | 0 |  | 2/S |  |

Actions 57, 58, 68 double the base through a(id). 91 and 93 get W × 2 and W × 5 inside W.

---

## 3. Knock-back and stun

### 3.1 The vector (RVA 0x214441–0x21452e)

1. **d** = C.position − attacker position, converted to blocks, then d.z := 0.
2. If |**d**|² > 0.01, normalise. So **d** stays (0, 0, 0) for a target exactly on top.
3. d.z := 0.25. Not rescaled afterwards.
4. **k** = (**d** × √(attacker.bx / C.bx)) × *k*(action), multiplied in that order.

*k*(action) (RVA 0x44860), with *S* taken from the attacker at this moment:

| Actions | *k* |
|---|---|
| attacker controller 3 or 5, any action | 0 |
| 30, 31, 32, 33 | 0 |
| 1, 2, 9, 13, 14, 15, 60, 61 | 6/*S* |
| 3, 4, 6, 7, 18, 19, 62 | 3/*S* |
| 65, 66 | 10/*S* |
| 5 | 4 |
| 11, 87 | 10 |
| 17, 20 | 6 |
| 54 | 5 |
| 21, 88 | 30·*v* + 5, *v* = RVA 0x3ed60(21) of the attacker |
| all others | 2/*S* |

**In the hit routine** (RVA 0x197e5e–0x197fa8): the code compares the target's velocity obj +0x34
against max(|velocity|², |**k**|²) in a way that never selects the rescale branch, so the velocity is
left unchanged *as read*.

**On application** (RVA 0x195a60, record kind ≠ 3 and amount ≥ 0): the record's vector is × 0.1 if
the target's stun counter > 0 and the record's stun field < 1; then, if the target's controller ≠ 6,
and (the online flag is clear and the target's controller ≠ 0, or the target is the local player),
and |obj +0x4c|² ≤ |vector|²: obj +0x4c := vector. A record with stun > 0 also zeroes obj +0x34.

A blocked hit's record carries **k** × 0.5. An absorbing buff emits its own kind-5 record carrying
**k** × 0.5; if damage remains after absorption, the ordinary record that follows carries the full
**k**.

### 3.2 Stun, hit-record field 0x18 (RVA 0x1983f8–0x198658)

Field 0x18 is 0 unless all of this holds. Let *c*(L) = 1 − 1/((L − 1) × 0.05 + 1) (RVA 0x3ca60),
L the level obj +0x190.

1. **Gate**: there is an attacker; the target's stun counter obj +0x12c < −3000; the target's
   controller ≠ 6.
2. v = s, the area value passed in.
3. Attacker action 58, 12, 68, 93 or 69: v := 1.
4. Target is a big type (RVA 0x445d0: type 101, 107, 108, 109, 114, 115, 116, 117, 118, 119):
   v ×= 0.15.
5. Attacker is not a big type and its controller ≠ 0: v ×= 0.1.
6. Attacker action 91 or 74: v := 1. Target action 84: v := 1.
7. v := min(1, 2^((*c*(attacker level) − *c*(target level)) × 10) × v).
8. Chance = v × 0.9; 0 if the target has any buff of kind 1.
9. One draw; if draw/32767 < chance: field 0x18 := RVA 0x44ff0(target), which is 1500 for a target of
   controller 0, else 3000 − ((tier × 1500) shifted right by 2). The record's knock-back z := 10 (the
   record only, not **k**). Sound 23 at the target, pitch 1, volume 1.

So with v = 0 there is no stun. v is non-zero only if s > 0 (a costed, mana-spending, charged or
fixed-power blow, §2.2) or an override of step 3 or 6 applies. The draw of step 9 is taken whenever
the gate is open.

The step function applies the counter: the target's stun counter := max(counter, field 0x18) on
application (RVA 0x195a60, record kinds other than 1, 3 and 4); each step the counter loses dt with no floor at
that point (RVA 0x2184ce); while it is > 0 the action and action time are held at 0 (RVA 0x2184dd).
A buff of kind 1 sets a positive counter to 0 (RVA 0x2183e7). An application of amount > 0 to a target
in action 84 sets its action to 0 and its counter to 3000 (RVA 0x195a60).

### 3.3 The rest of the 72-byte hit record

As built from RVA 0x197e5e onward:

| Off | Meaning |
|---|---|
| 0x00 | attacker id (8 bytes; 0 with no attacker) |
| 0x08 | target id (8 bytes) |
| 0x10 | amount, after armour or resistance, absorption, spirits and kind-1 buffs |
| 0x14 | **critical** (u8) |
| 0x18 | stun, §3.2 |
| 0x1c | 0 here; on application raises obj +0x130 to it when > 0 |
| 0x20 | 3 × i64: the **target's** position |
| 0x38 | knock-back (3 floats) |
| 0x44 | **area flag** (u8) |
| 0x45 | kind (0 here; 1 block, 3 immune, 4 dodge, 5 absorb in the other paths) |
| 0x46 | 1 if the attacker's action is 28 |

---

## 4. The area flag and the magic flag

### 4.1 Area flag

Melee: area flag = **s > 0, or action 11** (RVA 0x213625), evaluated each step from the current s.
So it is a property of the blow: set for 3, 4 (0.1); 5, 17, 20 with mana > 0; 21, 54, 87, 88 (s = 1);
31, 33 from the P1 step on; any action whose charge was spent; 11 always. Clear for plain primaries
without a spent charge.

It does not change the geometry. Its effects:

* The hit routine's spirit effects receive it (RVA 0x196d30 passes it to RVA 0x19a800).
* It stops mana on hit (§2.6).
* Class-4 specialization-0 attackers gain obj +0x1190 from it (§1.6).
* It is copied into hit byte 0x44. Reader not traced.

The area value passed beside it is s, and inside the hit routine it feeds only the stun (§3.2).

The beam passes area flag clear and area value 0.1 for 95 (§1.7). Multi-target reach comes from
§1.4: melee hits every creature in range.

### 4.2 Magic flag

Melee: magic = action 30, 31, 32 or 33 (RVA 0x2146f0). 87 and 88 use M for x but are not flagged
magic, *as read*. The beam always passes magic set.

Effect in the hit routine: the reduction uses RVA 0x467a0 instead of RVA 0x3cff0, and the impact
sound is 42 at pitch 1 for an attacker of specialization 1, 39 at pitch 1.5 otherwise, with no draw.
No other use of the flag in the hit routine.

Melee and beam always pass can-be-blocked = 1.

---

## 5. The damage stat, W and M

### 5.1 RVA 0x3e190 is not used by strikes

It has exactly two direct call sites: the hit routine's block step (RVA 0x19704b, q = x / its
value for the target, which drains the block gauge obj +0x174) and one in the routine at RVA 0x88ee0
(RVA 0x8ffa8; *believed* to be the client's handling of a received blocked hit). No melee, beam,
projectile or bomb strike path calls it. Its own formula is not re-certified here.

### 5.2 W and M are the strike base

W (RVA 0x3dc60), complete:

1. *L* = level; *g* = 3 × (1 − 1/((*L* − 1) × 0.05 + 1)); *P* = 2^*g*.
2. W = damage multiplier × (2^(0.25 × 0) × *P*). The exponent's second factor is a stored 0.0
   (data-section RVA 0x345d84), so the term is 1 and the first term has no tier in it.
3. Weapon list of §2.4 non-empty: W += the item damage of each entry (RVA 0xc7f60), in list order.
4. List empty: if appearance flag 0x08: W += *P* × 2^(0.25 × tier) × 2. If flag 0x10: W +=
   *P* × 2^(0.25 × tier) × 8. Both can apply. These terms carry the tier and are not scaled by the
   damage multiplier.
5. Action 87: × 50. Action 91: × 2. Action 93: × 5. Action 96: × 3.

M (RVA 0x46e80): steps 1–4 only, identically.

From the melee block, step 5's × 50 is never reached: action 87 takes the M branch of §2.1.

Callers of W: melee RVA 0x214584; RVA 0x217490, RVA 0x217968, RVA 0x217e37 (shots); bomb blast
RVA 0x20f3c1 (W × 25, critical when one draw mod 4 = 0, knock-back vector × 10); RVA 0x2164ef, the
action-104 branch of the beam block, which is unreachable *as read* (§7 item 7).

---

## 6. The class of a spawn (RVA 0x1c7730)

### 6.1 The rule

Fields of the spawn record: type +0x2c (int), controller +0x28, class +0x30 (byte), specialization
+0x31, level +0x34, flags word +0x7a. The generator takes a mode argument (0 at zone time, 1 at
activation, §6.2).

1. Flags word bit 0x40 set: the generator returns at once.
2. Class byte 0x80–0x87 or 0x89: one fixed item is written and the generator returns; class 0x88 does
   the same but first takes one draw (draw mod 5 + 2 for that item's +1 byte).
3. Flags bits 0x18, controller 6, or type 118: no weapons, no class draw.
4. Types 46, 47, 81, 82, 108, 109, 111, 112, 113, 114, 115, 117: no class draw, class left as it
   was (fixed equipment, or none for 47, 111, 112, 113).
5. Types 43, 45: no class draw; specialization := 2; one draw for the weapon subtype.
6. All other types: class := 1 + (draw & 3) (RVA 0x1c7a1c). **Type 97**: class := 4 immediately
   (RVA 0x1c7a21), before the class switch, so its following draws follow class 4. Each class then
   draws specialization = draw & 1 and its weapon draws; then, unless flags bit 0x20 is set, the
   armour draws.
7. **Type 86, mode 0 only**, after all weapon and armour draws: class := 3, specialization := 1
   (RVA 0x1c7f36, one 16-bit write). A type-86 spawn in mode 1 keeps the drawn class.
8. Both modes end with six item-seed draws; mode 1 skips the per-type multiplier switch and the
   flag-0x200 part.

These are all the writes to spawn +0x30 in the generator. **No other type is forced or remapped**:
type 84 appears nowhere in it.

Creators that set the class before the call: the routine at RVA 0x1af170 (*believed* the town
creator) writes 0x88, 0x84, 0x80, 0x82, 0x81, 0x83, 0x88 (RVA 0x1baa5b, 0x1bcc7a, 0x1bce42,
0x1bd01e, 0x1bd1fa, 0x1bd3d6, 0x1bd61e). The prefab creator at RVA 0x1f0ce0 switches on an argument
(jump table RVA 0x1f4b20, argument − 6): 6 → 0x83, 13 → 0x85, 14 → 0x86, 15 → 0x87, 7 → 0x89 (with
level 20 and type 2 + (draw & 1)); 9, 10, 11, 12 → classes 1, 2, 4, 3 for a type-2 level-20 spawn
whose flags word is set to 0x40 (RVA 0x1f431c, 0x1f4433, 0x1f454d, 0x1f464f), which the generator
then leaves alone.

### 6.2 The second run at activation (RVA 0x20fd06–0x20fd4a)

The generator also runs on an existing spawn at activation, overwriting class, specialization and gear
in place. When a creature in the scanned list is within the spawn's radius (spawn +0x08, compared
squared), and the spawn's map-cell record has kind 5 (cell +0x34) and the spawn's flags word has
0x1000:

* unless controller 6: spawn level := cell +0x3c, spawn +0x58 := cell byte +0x40;
* `srand`(cell +0x30 + spawn +0x48) (RVA 0x20fd38);
* the generator in mode 1 (RVA 0x20fd4a).

---

## 7. Notes for companion specs (pointers only)

1. `alpha-creatures-behaviour.md` §4.2: strikes use W and M (§5.2); RVA 0x3e190 is the block
   reference value (§5.1). §5's x is §2 here.
2. Behaviour §3.3, §8.1: the hit test runs in the window of §1.3, not once at P1.
3. Behaviour §5: the critical flag is hit byte 0x14; 0x44 is the area flag; the routine also takes
   the area value s. Hit-record layout: §3.3 here (also for `alpha-net-packets.md` §5.1).
4. Behaviour §5: the routine returns a value, §1.6.
5. Behaviour §8.2: §2.2 here.
6. `alpha-projectiles.md` §2.6 "healer melee": the class-3 specialization-1 bursts at RVA 0x21608a
   and 0x21681b are in the beam block (actions 28, 94, 95). The melee heal is §1.6's rule.
7. `alpha-projectiles.md` §2.6, action 104: 104 dispatches to the melee case (index table), so the
   beam block's branches for 104 (RVA 0x215c89, 0x215fe9, 0x2162f6, 0x2164b0, 0x2165ee, 0x21666a) are
   unreachable *as read*. In melee, 104 has a = 1, B = 1, r = 3·bx, f = 0.
8. `alpha-server-simulation.md` §1: on an online client the melee case runs for every creature
   (RVA 0x212357) but the hit routine applies nothing; the beam uses the strict owner rule (§1.7).
9. `alpha-projectiles.md` §2.2: W's item terms are the list of §2.4.
10. `alpha-spawn-objects.md` §4.2 and behaviour §9.3: §6.1 (type 97, type 86 mode 0).

---

## 8. Open points

* **P6-3** Readers of hit bytes 0x44 and 0x46, and of obj +0x1314.
* **P6-4** Action 73's case (RVA 0x216d35) and action 8's case (RVA 0x2121f4) were not traced; the
  action-8 case is not present in the instruction listing used.
* **P6-5** What starts action 88; the step-local factor of action 28's mana drain (RVA 0x215d66,
  × 0.0002), *believed* dt.
* **P6-6** RVA 0x453b0, called with each combo gain, was not re-read here.
* **P6-7** The server image was not compared; see §9.

---

## 9. Unresolved (moved here in re-certification)

* Whether `Server.exe` runs the identical melee block, and the server-side handling of hits on
  players (formerly stated as packet 7 and as "sent to that player").
* The packet number and send filter for an online client's own hit records.
* That action 8 makes no hit (its case code is not in the listing).
* The naming of obj +0x128 as "dodge time" and of obj +0x1190 as stealth (offsets confirmed, meaning
  taken from companions).
* The meaning of the ids at attacker obj +0x11c0 excluded by the beam.
* Draws inside the spirit effects (RVA 0x19a800), and therefore whether rows 3–9 of §2.3 can be
  preceded by further draws when the attacker's combo ratio exceeds 0.25.
* The item-damage function RVA 0xc7f60 (e.g. a shield's damage factor) and the crit-value function
  RVA 0xc6ba0.
* The formula of RVA 0x3e190, and whether RVA 0x8ffa8 is the handling of a received blocked hit.
* Whether the shot callers of W can run during action 87 (only the melee path was checked).
* Whether projectiles, traps or other hit sites pass area flags or magic flags (only melee, beam and
  bomb were read); callers of the hit routine also exist at RVA 0x20dd18, 0x2183da, 0x221e8c, 0x222ab8.
* Whether the list scanned at activation (§6.2) is the player list, and whether spawn +0x48 is the
  low 32 bits of the spawn id and spawn +0x58 its tier.
* Whether any later zone-generator code writes spawn +0x30 other than by whole-record copies.
* That the stun counter starts at −3000.
* Which spawns carry flag 0x1000 (a companion pointer, not re-read).

---

## Re-certification (2026-11-15)

Checked against `Cube.exe` (SHA-256 84a7a132…6dc30bf): instruction listing of the world step's action
cases and the hit, application, weapon-list, W, M, critical, knock-back, stun, cost, big-type and
spawn-generator routines, plus data-section constants read from the image bytes. All addresses were
converted from virtual addresses to RVAs (image base 0x400000).

**Confirmed**: the action dispatch and the melee action list; the owner gate; hit window formulas and
pulse lengths; reach table and origins; hit-test steps and the hit/evaded sets; return values of the
hit routine; post-hit order (action-30 chance, combo, stealth, mana); a(id), B, s rules, W, M, the
weapon list; critical base, slot order and factor 2; the draw order crit then σ; knock-back vector and
*k* table; stun gate, steps, duration, sound 23; hit-record layout; area and magic flag rules; the two
call sites of RVA 0x3e190; W callers; generator class writes, type 97, type 86 mode 0, no type-84 rule;
prefab argument mapping; activation regeneration.

**Corrected** (old → new):

1. Horizontal test: "sum of squares ÷ 65536" → each squared component is divided by 65536 separately,
   then summed; threshold is (0.7·C.bx + r)² × 65536, truncated.
2. σ, mana-on-hit *g*, heal and beam formulas: operation order now stated exactly (draw × constant,
   then ÷ 32767). Affects numeric precision, not draw order.
3. Critical with a kind-11 buff: "certain, no draw implied" → one draw is always taken; certain unless
   the draw is 32767. **Affects random-draw order** (the draw is consumed even when the buff makes
   the critical near-certain).
4. Draw table: added the action-30 chance draw (RVA 0x441a0) after the hit routine and before the
   mana draws; added the beam's extra pitch draw for an attacker in action 28. **Affects random-draw
   order.**
5. The action-30 chance: added the main-hand subtype scaling (× 0.5 or × 0.3).
6. Mana on hit: "first target in a step" → first target in the step with a true hit result.
7. obj +0x1314 := combo also at each new pulse, not only at *t* ≤ P1.
8. Pulse mark: also set when *t* + dt > B; the heal rule depends on it (was "new-pulse step only").
9. Velocity in the hit routine: "scaled up to |k|" → the rescale branch is never taken (**reversal**).
10. Absorption: "a partly absorbed hit gives k/2" → the separate absorb record carries k/2; the
    following damage record carries the full k.
11. Stun: "51 and 75 never stun" → they can stun when s > 0 (spent charge) (**reversal**).
12. Beam: added that each firing empties both sets, excludes the attacker and the id at obj +0x11c0,
    skips (does not record) dodging targets, gains combo, and heals specialization-2 attackers only
    offline.
13. Reach row 11/54/61: the × 3 is applied together with the origin change; table "f = 0" → "self".
14. Town creators: "0x80–0x88" → only 0x80, 0x81, 0x82, 0x83, 0x84, 0x88.
15. Generator class bytes 0x80–0x89: class 0x88 takes one draw before returning; the stop path writes
    a fixed item (was "stops after the chest"). **Affects world-generation random-draw order.**
16. Generator field naming: flags tested in the generator are the spawn's +0x7a word (not the creature
    appearance word).
17. Actions 31/33: s is not reset at *t* = 0; the P1 cost is 0.3 (0 with an active kind-9 buff);
    action 31 consumes an active kind-9 buff in its strike steps.
18. Activation: the tier target is spawn +0x58, the seed addend spawn +0x48, the trigger the spawn
    radius at +0x08 (identity of these fields moved to unresolved).
19. Beam: added the 94/95 mana cost at P1 and that a negative mana after drain or cost suppresses the
    step's firing; the combo gain and bursts follow hit-routine calls, not can-damage tests.
20. Stun application: applies for record kinds other than 1, 3 and 4 (kind 5 included).

**Removed**: the "measured gap" framing; §6 worked examples (recording comparisons, implied draws,
timing and health measurements, test vectors); the "What explains the recording" section (spawn 8,
level 171, the seed check, extractor and port advice); "3.36 at level 29, the measured shortfall";
the projectile test vector; the sound file name; "about six steps"; statements about companion-spec
contents beyond pointers; the claims about projectiles, traps and the projectile mana rule not re-read
here; open points P6-1 and P6-2 (recording-dependent).

**Unresolved**: listed in §9.
