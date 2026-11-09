# Alpha open points, batch 6: how a melee strike becomes damage, and the class of a spawn

Clean-room behavioural specification. It closes these points:

| Point | Spec it belongs to | Section here |
|---|---|---|
| Where the world step decides that a melee hit lands; reach and hit box | `alpha-creatures-behaviour.md` §3.3, §8.1 | §1 |
| The value x handed to the hit routine 0x596d30 for a melee hit | behaviour §4.2, §5 | §2 |
| Knock-back vector and stun (hit field 0x18) | behaviour §5, `alpha-net-packets.md` §5.1 | §3 |
| The area flag and the magic flag | behaviour §5 | §4 |
| Is the "damage stat" 0x43e190 what strikes use? Is W complete? | behaviour §4.2, `alpha-projectiles.md` §2.2 | §5 |
| Worked examples against the measured game | | §6 |
| Which spawn types have their class forced or remapped | `alpha-spawn-objects.md` §4.2, behaviour §9.3 | §7 |

The short answer to the measured gap: **strikes do not use the damage stat.** They use the weapon
damage W (0x43dc60). W gives a creature with no weapon and appearance flag 0x10 an extra 8·P, so
its strike base is 9·P, not P. The spread is one draw, 0.75 to 1.25. A critical doubles.

Written from the Alpha client (`Cube.exe`) per-instruction listing of the world step (the action
switch at 0x611e1b, table 0x622c84 with index 0x622ccc) and the decompile of the routines it
calls. Constants were read from the client's data section. Client and server step are the same
code (`alpha-server-simulation.md` §1); the server's melee block was located (server 0x53a321 and
following) but not compared again line by line. No code was copied; addresses are citations only.
*As read* marks behaviour that looks unintended but is what the code does; *believed* marks a
reading that rests on partial evidence.

Conventions: as in `alpha-creatures-behaviour.md` §0 and `alpha-projectiles.md` §0. *t* is the
action time before this step's advance, dt the step in ms, "draw" one `rand()`, *u* = draw/32767.
"obj +x" is the creature object. *b* = (bx, by, bz) is a creature's box (obj +0x80). *S* is the
attack-speed factor of behaviour §4.3. Products are single precision in the order written.

---

## 1. Where a melee hit lands

All of this is one block of the world step: the action switch's first case, 0x612357 to 0x614c32.
It sits outside the Ghidra function, in the same bodies as the projectile code. The hit call is at
0x61476b. There is no separate melee routine.

### 1.1 Which actions

The first case serves actions 1–7, 9–21, 30–33, 51, 54, 57, 58, 60, 61, 62, 65–70, 72, 74–78,
86, 87, 88, 91, 93, 104. These are the "melee" actions. 8 (charge) has its own case and makes no
hit. 28, 94, 95 are the beam of §1.7. 73 has its own case and was not traced.

### 1.2 Who runs it

| Machine | Creature | Runs the block? | What the result does |
|---|---|---|---|
| Offline client | local player | yes | applied at once (0x595a60) |
| Offline client | NPC (controller ≠ 0) | yes | applied at once |
| Offline client | a player that is not the local one | **no** (0x612357) | |
| Server (online flag clear, no local player) | NPC | yes | applied by the hit routine; a hit on a player is sent to that player (`alpha-server-simulation.md` §6.1) |
| Server | player | **no**; those hits arrive as packet 7 | |
| Online client | local player | yes | the record goes out as packet 7; the server applies it |
| Online client | any other creature | **yes**, for its local effects | never applied (0x595a60 runs only with the online flag clear); the sender passes only records whose attacker is the local player |

The last row is a correction, §8 item 8. The hit routine ignores an online client's own player as
a target (0x596d9f), so on an online client only the server's NPC and other players' records can
hurt the local player. Effects that the block does keep behind the owner test: mana on hit (§2.6).

### 1.3 The hit window

Let P1, P2 be the phases of behaviour §3.3 and ⌊ ⌋ truncate toward zero.

* **Normal actions.** A = P1 + ⌊P2/2⌋ − 100, raised to P1 if below it. B = P1 + ⌊P2/2⌋, lowered to
  P1 + P2 − 1 if it reaches P1 + P2.
* **68, 69:** A = P1 + ⌊P2/3⌋; B as above.
* **93:** A = P1 + ⌊P2/5⌋; B = P1 + P2.
* **Pulsing actions** 5, 11, 30, 31, 32, 33, 72, 86, 91. Pulse length T: 250 ms; 1000 ms for 91;
  ⌊P2/3⌋ for 5 and 11; ⌊P2/6⌋ for 30 and 32; ⌊P2/12⌋ for 31 and 33. While P1 ≤ *t* < P1 + P2:
  A = P1 + ⌊(*t* − P1)/T⌋·T and B = A + 100. Otherwise A = B = P1, which is an empty window. When
  P1 + ⌊(*t* + dt − P1)/T⌋·T differs from A and is below P1 + P2, a new pulse starts: both swing
  sets (§1.5) are emptied and the "new pulse" mark of §1.6 is set for this step.

The strike runs in **every step with A < *t* ≤ B**. So a normal swing tests for targets over a
100 ms span, about six steps, not once when the time reaches P1. This corrects behaviour §3.3.

### 1.4 Reach

From the attacker's own box: start with r = 1.5·bx, f = 1.5, then apply each row that matches.

| Actions | r | f |
|---|---|---|
| 1, 2, 3, 4, 6, 7, 9, 13, 14, 18, 19 | 2·bx | 1 |
| 20, 21 | × 2 | as it stands |
| 68, 70 | | 0 |
| 86, 93, 104 | × 2 | 0 |
| 87, 88 | 8 (absolute) | 0 |
| 30, 32 | 3 (absolute) | 0 |
| 31, 33 | 5 (absolute) | 0 |
| 72, 76, 77, 78 | × 1.2 | 0 |
| 74, 91 | 20 (absolute) | 0 |
| 11, 54, 61 | × 3, and the origin is the creature itself, see below | |

**Strike origin O**: the position plus (−sin yaw, cos yaw, 0) × (bx·f), with yaw = obj +0x30 in
degrees. Then:

* 30–33: O := position + aim vector (field 26).
* 69, 75: O.z += 0.5·(bz − bx) of the attacker.
* 11, 54, 61: O := position.

**Sight origin Q**: the attacker's position; for 30–33, O + (0, 0, r).

### 1.5 The hit test, for each creature C in list order

1. C exists, has health > 0, and is not in the attacker's **hit set** (obj +0x11ac). The attacker
   itself is not excluded here; the can-damage test of step 5 refuses it, so it can reach the
   healing rule.
2. **Horizontal**: (O − C.position), x and y only, squared length < (0.7·C.bx + r)². Compared in
   fixed point: the sum of squares ÷ 65536 against r² × 65536 truncated.
3. **Vertical**: |O.z − C.z| < 0.5·C.bz + attacker.bz + r.
4. **Sight**: 0x59ee90(Q, C, 1, 200), the test of `alpha-projectiles.md` §5.1 step 3.
5. **Can damage** 0x5988a0(attacker, C). If not, only the healing rule of §1.6 applies.
6. **Dodging**: if C's dodge time ≠ 0, C goes into the **evaded set** (obj +0x11b4) and is skipped.
7. **Hit**: §2, then C goes into the hit set, the "landed" byte obj +0x13b8 := 1, and the hit
   routine runs. The byte is set even if the hit is then blocked or the target is immune.

**One hit per target per swing or pulse.** A second target in reach is hit too: a swing has no
target limit, and each target has its own critical and spread draws.

**The sets** are emptied in every step with *t* ≤ P1, and also at each new pulse. Each step with
*t* ≤ P1, and each step with *t* ≥ B, every id in the evaded set that is not in the hit set gets a
kind-4 hit record (attacker, target, target position; the dodge notice 0x43cf20) and the evaded
set is emptied. obj +0x1314 := combo at the same *t* ≤ P1 moments (reader not traced).

### 1.6 After the hit routine returns

Its result is true for a damage hit, also one reduced to 0 and one fully absorbed; false for: dead
target, an online client's own player, a ground attack against a target in the air, immune,
blocked. It always returns a value; the Ghidra `void` is wrong (0x5986d5, 0x597014, 0x59722e).

On true, in order:

1. Action 30: chance test 0x4441a0(attacker, 0.25); on success and specialization 0, buff kind 9
   for 30 000 ms, event-40 record, sound 47 at the attacker.
2. **Combo**: if the hit set was empty before this target (the first target of this swing or
   pulse) and the attacker's own stun counter ≤ 0: combo += 1, combo timer := 0.
3. Area flag set and attacker class 4 specialization 0: stealth += s, capped at 1.
4. **Mana on hit**, §2.6.

For targets that cannot be damaged, in actions 32 and 33, only in a new-pulse step, only for an
attacker of specialization 1: a **heal** record with amount −0.1·M·σ on C (σ as §2.3, one draw,
then one critical draw), sound 42 at the attacker, applied at once offline or when attacker and
C are both the local player.

### 1.7 The beam (actions 28, 94, 95; 0x615c68–0x6169d7)

This is not melee, but it is the fourth hit site and uses the same hit routine (0x6166c8). Run
only by the owner (offline and controller ≠ 0, or the local player). 94 fires when P1 is crossed;
28 and 95 fire then and at each 100-ms boundary after P1 up to P1 + P2. Each firing walks one
block at a time from the attack origin 0x446bb0 along unit(aim), up to the distance the ray cast
0x5a35d0 gives (limit 200). At each block, a creature not in the hit set, not dodging, with
horizontal distance² < (0.5·C.bx + 1)² and |dz| < 0.5·C.bz + 1 is put in the hit set and gets:
one critical draw; x = 1.5·M·(1 + 0.05·*u*), × 2 if critical; then, if can-damage, the hit routine
with magic set, area flag set only for 95, area value 0.1 for 95 and 0 otherwise, knock-back zero,
action argument 0. Specialization-2 attackers then gain x as health, with a heal record.

The bursts of `alpha-projectiles.md` §2.6 "healer melee" belong to this block. See §8 item 6.

---

## 2. The damage x

### 2.1 The formula (0x614561–0x61469c)

**Weapon actions** (every melee action except 30, 31, 32, 33, 87, 88):

  x = a(id) × W × (5·s² + B) × σ, then × 2 if critical.

**Magic actions** 30, 31, 32, 33, 87, 88:

  x = a(id) × M × σ, then × 2 if critical. The power s has no effect on x.

There is no other factor. In particular:

* **No level difference** enters x. Levels matter only through W, M, armour and the stun chance.
* **No NPC-only or player-only factor.** The damage multiplier (field 30) is inside W once.
* **No swing, charge-time or combo factor.** The combo only weakens armour in the hit routine
  (behaviour §5 step 7: armour × (1 − min(combo/comboMax, 1))) and drives spirits.

### 2.2 The terms

**a(id)** (0x43ec90), complete: 0.5 for 5, 26; 0.1 for 21, 72; 0.4 for 30, 32; 0.6 for 31, 33;
2 for 37, 43, 57, 58, 68; 0.25 for 55; 1 for every other id, so 1 for 51, 75, 54, 1, 2, 9, 10, 13,
14, 18, 19, 67, 104, 11, 60, 61, 17, 20.

**B**: 0 for actions 60, 11, 62; 1 for all others.

**s**, the spent power (obj +0x13b4). In every melee step except actions 31 and 33:

1. At *t* = 0: s := 0, the "sure critical" byte obj +0x13c0 := 0. Then by mana cost (0x444ae0,
   behaviour §16.4; among melee actions only 3 and 4 have a cost here, 0.1): if cost > 0, s := cost,
   mana −= cost.
2. Otherwise at *t* = 0: 17, 20, 5: s := all mana, mana := 0; if a buff of kind 11 is active it is
   consumed (duration 0, event-40 record) and the sure-critical byte := 1. 21, 54, 87, 88: s := 1,
   no mana taken.
3. Any step: if the charge (field 23) > 0: s := charge, mana −= charge, the landed byte := 0, and for
   actions 5 and 11 s ×= 0.5. Then charge := 0 and mana is floored at 0.

Actions 31 and 33 run step 3 only, except that when *t* ≤ P1 < *t* + dt: s := 0.1 and mana −=
cost, floor 0.

So a plain primary attack has s = 0 and the bracket is 1. A held ability has s = what it spent:
a warrior's 5 at full mana and s = 1 has 0.5·W·(5·1 + 1)·σ = 3·W·σ.

**W** (0x43dc60) and **M** (0x446e80): §5.2.

**σ**, the spread: one draw, σ = (1.25 − 0.5·*u*) + 0, single precision, from 1.25 (draw 0) to 0.75
(draw 32767). In the magic branch σ is multiplied by (0 + 1), which changes nothing.

### 2.3 Critical and the order of the draws

The draws for one target C that passed §1.5, in order:

| # | Draw | When |
|---|---|---|
| 1 | critical, 0x4444a0 | unless the sure-critical byte is set, then none and the hit is critical |
| 2 | σ | always |
| 3 | absorb sound pitch | if the target has an absorbing buff that applies |
| 4 | impact sound pitch | can-be-blocked, not magic, action argument ≠ 28 |
| 5 | hurt voice pitch | critical and target controller ≠ 6, also for a type with no voice |
| 6 | stun | only if the gate of §3.2 opens |
| 7, 8 | mana on hit | only under §2.6 |

The block path instead uses one pitch draw and stops. The immune path likewise.

**Critical chance** (0x4444a0, 0x43ea40): certain with a buff of kind 11; otherwise *r* < p with
p = 0.1·P/8 + the crit of the items in slots 6, 7, 2, 3, 4, 5, 1, 8, 9 (each only if of the expected
type) + 0.15·stealth. Behaviour §4.5 stands. A level-29 creature with no items has p = 0.042.

**Factor**: exactly 2, before armour. Armour is not doubled, so a critical is a little more than
twice the plain value after armour.

### 2.4 Which weapons W counts (0x43df60)

W adds the item damage of a **list that depends on the action**:

1. Main hand (slot 7) is a bow or crossbow: the main hand only, whatever the action.
2. Off hand (slot 6) is a bow or crossbow: the off hand only.
3. Otherwise by the current action:

| Actions | List |
|---|---|
| 2, 4, 7, 8, 10, 18, 39, 41 | off hand, if it is type 3 |
| 11, 12, 16, 17, 20, 54, 86, 91, 96 | off hand, then main hand, each if type 3 |
| 40, 42 | main hand, if type 3 |
| all others | main hand if type 3; then the off hand if it is a bracelet (subtype 12) |

So in a chain of 1 then 2, the first blow uses the main-hand weapon and the second the off-hand
one. A shield in the off hand is a type-3 item with damage factor 2, and it is what action 10 uses.
A creature whose action selects an **empty** off hand has an empty list, and W is then the bare
base, or the natural-weapon value below (*as read*: a player with a single sword hits for far less
on the second blow of the chain).

### 2.5 Hit flags

One hit per target per swing or pulse (§1.5). A pulsing action hits a target once per pulse, so 11
hits up to 3 times, 5 up to 3, 30 and 32 up to 6, 31 and 33 up to 12, 72 and 86 once per 250 ms, 91
once per 1000 ms. Each pulse rolls its own critical and σ.

A whiff ends the chain as behaviour §8.1 says. The landed byte is cleared in steps with *t* ≤ P1,
and also when a charge is spent.

### 2.6 Mana on hit (0x614906–0x6149fb)

For the first target in a step only, if offline or the attacker is the local player, and the area
flag is clear, and the action is not 30 or 32: *g* = 0.1 + 0.05·(1 − 2·*u*); *g* ×= total/**500**,
total being the action's full duration in ms; one more draw, *g* ×= **1.5** if draw mod 8 = 0;
mana := min(1, mana + *g*). The projectile rule (`alpha-projectiles.md` §6.1, ÷ 300 and × 2) is a
separate piece of code with its own constants; both were read and both stand.

### 2.7 The action table

"Base" is W or M. "Weapons" uses §2.4: M = main hand (plus a bracelet off hand), O = off hand, B =
both. s: "—" means 0 unless a charge is spent. r and f are the values of §1.4, before any change of
origin. *k* is the knock-back of §3.1. "Stun" marks a v override to 1 in §3.2. Pulse is the T of §1.3.

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
| 11 | W | 1 | 0 | — | B | 4.5·bx | 0 | P2/3 | 10 |  |
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
| 54 | W | 1 | 1 | 1 | B | 4.5·bx | 0 |  | 5 |  |
| 57 | W | 2 | 1 | — | M | 1.5·bx | 1.5 |  | 2/S |  |
| 58 | W | 2 | 1 | — | M | 1.5·bx | 1.5 |  | 2/S | 1 |
| 60 | W | 1 | 0 | — | M | 1.5·bx | 1.5 |  | 6/S |  |
| 61 | W | 1 | 1 | — | M | 4.5·bx | 0 |  | 6/S |  |
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

### 3.1 The vector (0x614441–0x61452e)

1. **d** = C.position − attacker position in blocks, z := 0.
2. If |**d**|² > 0.01, normalise. So **d** stays (0, 0, 0) for a target exactly on top.
3. d.z := 0.25. Not rescaled afterwards.
4. **k** = **d** × √(attacker.bx / C.bx) × *k*(action). Multiplied in that order.

*k*(action) (0x444860), with *S* the attacker's factor at this moment:

| Actions | *k* |
|---|---|
| attacker controller kind 3 or 5, any action | 0 |
| 30, 31, 32, 33 | 0 |
| 1, 2, 9, 13, 14, 15, 60, 61 | 6/*S* |
| 3, 4, 6, 7, 18, 19, 62 | 3/*S* |
| 65, 66 | 10/*S* |
| 5 | 4 |
| 11, 87 | 10 |
| 17, 20 | 6 |
| 54 | 5 |
| 21, 88 | 30·*v*(21) + 5, *v* = the ability value of behaviour §16.1 |
| all others, so 51, 57, 58, 67, 75, 104 and so on | 2/*S* |

**What the target does with it.** In the hit routine (behaviour §5 step 11) the target's velocity
obj +0x34 is scaled up to length |**k**|. On application (0x595a60, hit amount ≥ 0): **k** ×= 0.1 if
the target is already stunned and this hit carries no stun; if the target's controller ≠ 6, and it
is owned here, and |obj +0x4c|² ≤ |**k**|²: obj +0x4c := **k**. A hit carrying stun also zeroes
obj +0x34.

A block halves the vector. A blocked hit gives **k**/2 (behaviour §5 step 5), as does a partly
absorbed one.

### 3.2 Stun, hit field 0x18 (0x5983f8–0x598658)

Field 0x18 is 0 unless all of this holds. Let *c*(L) = 1 − 1/((L − 1)·0.05 + 1).

1. **Gate**: there is an attacker; the target's stun counter obj +0x12c (field 16) < **−3000**; the
   target's controller ≠ 6. The gate means the target has been free of stun for at least 3 s. The
   counter starts at −3000 and loses dt every step with no floor at that point (0x6184ce).
2. v = s, the area value passed in.
3. Attacker action 58, 12, 68, 93, 69: v := 1.
4. Target is a big type (0x4445d0: 101, 107, 108, 109, 114, 115, 116, 117, 118, 119): v ×= 0.15.
5. Attacker is **not** a big type and its controller ≠ 0: v ×= 0.1.
6. Attacker action 91 or 74: v := 1. Target action 84: v := 1.
7. v := min(1, v · 2^(10·(*c*(attacker level) − *c*(target level)))).
8. Chance = 0.9·v; 0 if the target has any buff of kind 1.
9. One draw; if *r* < chance: field 0x18 := 1500 for a target of controller 0, else
   3000 − ⌊1500·tier/4⌋ (arithmetic shift). The record's knock-back z := 10 (the record only, not
   **k**). Sound 23 (sword-hit03.wav) at the target, pitch 1, volume 1.

So **with s = 0 there is no stun**, unless the action is in steps 3 or 6 or the target rests in 84.
Apart from that, actions 51 and 75 never stun, and the draw of step 9 is still taken if the gate
is open.

The target applies it: stun counter := max(counter, field 0x18); while the counter > 0 its action
is held at 0 each step (0x6184dd). A buff of kind 1 clears a positive counter (0x6183e7). Resting in
action 84 ends with counter := 3000 on any hit of amount > 0 (0x595a60).

### 3.3 The rest of the 72-byte hit record

This closes behaviour B-2 and part of `alpha-net-packets.md` P-10. As built after 0x597e5e:

| Off | Meaning |
|---|---|
| 0x10 | amount, after armour, spirits and kind-1 buffs |
| 0x14 | **critical** (u8) |
| 0x18 | stun, §3.2 |
| 0x1c | 0 here; raises field 17 (obj +0x130) when > 0 |
| 0x20 | 3 × i64: the **target's** position |
| 0x38 | knock-back |
| 0x44 | **area flag** (u8) |
| 0x45 | kind (0 here) |
| 0x46 | 1 if the attacker's action is 28 |

---

## 4. The area flag and the magic flag

### 4.1 Area flag

Melee: area flag = **s > 0, or action 11** (0x613625). It is evaluated each step from the current
s. So it is not a list of actions but a property of the blow: every costed, charged or mana-spending
blow has it. Set for: 3, 4 (0.1); 5, 17, 20 with mana > 0; 21, 54, 87, 88 (s = 1); 31, 33 after P1;
any action with charge spent; 11 always. Clear for plain primaries, so for 1, 2, 9, 10, 13, 14, 18,
19, 51, 57, 58, 67, 75.

It does **not** change the geometry. Despite the name it does this, and only this:

* The hit routine's spirit effects need it (behaviour §12.2); spirit bursts are larger with it
  (§12.1).
* It stops mana on hit (§2.6).
* Rogue specialization 0 gains stealth from it (§1.6).
* It is copied into hit byte 0x44. Reader not traced.

The area **value** passed beside it is s, and it feeds only the stun (§3.2).

Projectiles pass their special flag and strength instead (`alpha-projectiles.md` §6.2). The beam
sets it for 95 only (§1.7). A trap passes none.

Actual multi-target reach comes from §1.4: all melee hits every creature in range.

### 4.2 Magic flag

Melee: magic = action is 30, 31, 32 or 33 (0x6146f0). 87 and 88 use M for x but are **not** flagged
magic: armour, not resistance, applies, *as read*. The beam: always magic. Projectiles: kind 1.

Effect in the hit routine: resistance instead of armour (step 7), and the impact sound is 42 at
pitch 1 for an attacker of specialization 1, 39 at pitch 1.5 otherwise (step 10). No other effect.

Blocking is not prevented by it: melee and beam always pass can-be-blocked = 1.

---

## 5. The damage stat, W and M

### 5.1 0x43e190 is not used by strikes

It has exactly two readers: the hit routine's **block** step (behaviour §5 step 5, q = x /
damage(target)) and the client's handling of a received blocked hit (0x48ffa8, the "value derived
from the player" of `alpha-net-packets.md` §5.1). It answers "how much is this hit relative to what
the target itself could deal", which drains the block gauge. No melee, beam, projectile or bomb
path reads it.

**Correction to behaviour §4.2**: the first bullet is the *block reference value*, not the damage
of a strike. Using it as x gives 3.36 at level 29, the measured factor-of-nine shortfall. As a
reminder its own formula (P × m plus weapons, × m again) stays as written, including open question
B-1 (shield × 4).

### 5.2 W and M are the strike base

W (0x43dc60), complete:

1. *L* = level; *g* = 3·(1 − 1/((*L* − 1)·0.05 + 1)); *P* = 2^*g*.
2. W = *P* × 2^(0.25·0) × damage multiplier. The second power is **constant 1**: its exponent is
   0.25 times a stored zero (0x745d84). So the first term has **no tier** in it and players get no
   ×2, unlike §4's base.
3. Weapon list of §2.4 **non-empty**: W += the item damage of each entry (0x4c7f60).
4. List **empty**: if appearance flag 0x08: W += *P*·2^(0.25·tier)·2. If flag 0x10: W +=
   *P*·2^(0.25·tier)·8. Both can apply. These two terms do have the tier and are **not** scaled by
   the damage multiplier.
5. Action 91: × 2. Action 93: × 5. Action 96: × 3. Action 87: × 50.

M (0x446e80): steps 1–4 only.

The projectile formula's W in `alpha-projectiles.md` §2.2 is **complete in its terms** and matches
this, with one correction: "the weapons in slots 6 and 7" must read "the list of §2.4 here". For
projectile actions that means the bow or crossbow alone; for others the rule above. The test vector
there (level-1 bow, W = 9) stands. The damage multiplier scales only *P*, not the weapon or natural
terms.

Step 5's × 50 is never reached *as read*: action 87 takes the M branch of §2.1, and the shot callers
of W do not run during 87.

Who calls W: melee 0x614584; shots 0x617490, 0x617968, 0x617e37; bomb blast 0x60f3c1 (× 25,
critical on one draw in four, knock-back × 10, `alpha-pets-bombs.md` §4). 0x6164ef is the dead
branch of §8 item 7.

---

## 6. Worked examples

Values in single precision. Player armour, level 1: 2·1 = 2 (behaviour §4.2), **if** the player has
no appearance flag 0x20. The elf rows of the appearance table show no flags, and the measurement
agrees: with 3, the 35.1 hit would need σ = 1.259, above the maximum.

### 6.1 Type 54, level 29, action 51, on the level-1 warrior

*L* = 29: *g* = 1.75, *P* = 3.363586. Tier 0, no weapons, damage multiplier 1. Type 54 has flags
0x32 (0x02, 0x10, 0x20; `alpha-creatures-models.md` appendix B), so flag 0x10 applies:

* W = 3.363586 + 8·3.363586 = **30.272274**.
* a(51) = 1, s = 0, B = 1, so x = 30.272274·σ, from **22.70** to **37.84**.
* Armour: attacker combo 0, so the full 2. Result **20.70 to 35.84**, mean 28.27.
* Critical, p = 0.042: 2·30.272274·σ − 2, **43.41 to 73.68**.

| Measured | Implied σ | Implied draw *u* |
|---|---|---|
| 24.6 | 0.879 | 0.742 |
| 32.9 | 1.153 | 0.195 |
| 35.1 | 1.226 | 0.048 |
| 23.6 | 0.846 | 0.808 |
| 34.8 | 1.216 | 0.067 |
| 31.5 | 1.107 | 0.286 |
| 60.3 (critical) | 1.029 | 0.442 |

All lie inside the range. **60.3 is a critical** with an ordinary σ: it is above the non-critical
maximum 35.84 and inside the critical range. One critical in seven at p = 0.042 happens about a
quarter of the time, so it is not evidence against p.

Timing: *S* = 0.75 + 0.1·3.3636/8 = 0.79204. P1 = 505, P2 = 378, P3 = 631, total 1514. A = 505 +
189 − 100 = 594, B = 694. The strike tests at 594 < *t* ≤ 694. The hop every 2000 ms of behaviour
§13.2 sets the period, matching the measured ~2020 ms.

Knock-back: *k* = 2/0.79204 = 2.5251. With equal boxes, **k** = (2.525·ux, 2.525·uy, 0.631). Stun:
s = 0 and action 51 is not in §3.2 step 3, so 0. Reach: r = 1.5·bx, O 1.5·bx ahead of the creature.

### 6.2 Type 71, level 2, action 75

*g* = 3·(1 − 1/1.05) = 0.142857, *P* = 1.104089. Flags 0x30, so 0x10 applies. W = 9·1.104089 =
**9.936805**. x = 7.45 to 12.42. After armour 2: **5.45 to 10.42**, mean 7.94. Critical, p = 0.0138:
12.90 to 22.84.

Timing: *S* = 0.76380; P1 = 392, P2 = 392, P3 = 654, total 1438, matching the measured 1464 with the
AI's wait. A = 588 − 100 = 488, B = 588. O is raised by 0.5·(2.7 − 1.2) = 0.75 by §1.4.

Knock-back: *k* = 2.6184; with box 1.2 against the default 0.96, × 1.118 = 2.927: **k** = (2.927·ux,
2.927·uy, 0.732). Stun 0.

The measurement: seven bites × 7.94 = **55.6**, against the measured fall of 55. That agreement is
**probably a coincidence**, because offline the local player regains 5 % of maximum per second
(`alpha-player-control.md` §7): 13.25 health/s, or 119 over the 9 s. Bites alone (5.4 per second) lose
to that and the bar would stay full. A net fall of 55 needs about 174 gross, so most of the damage
came from the other creatures near by. Use a per-bite measurement to check this example.

### 6.3 A level-1 warrior's sword swing on a level-29 type 54 (health 336)

*P* = 1, level-1 sword: item damage 1·4 = 4. Action 1 uses the main hand: W = 1 + 4 = **5**. a = 1,
s = 0, B = 1. x = 5·σ = 3.75 to 6.25. Critical, p = 0.0125 plus item crit: 7.5 to 12.5.

Target armour: base 3.363586 + base again for flag 0x20 = **6.727172**.

| Attacker combo (max 50) | Armour taken | Plain hit | Critical |
|---|---|---|---|
| 0 | 6.727 | **0** | 0.77 to 5.77 |
| 25 | 3.364 | 0.39 to 2.89 | 4.14 to 9.14 |
| 50 | 0 | 3.75 to 6.25 | 7.5 to 12.5 |

So at combo 0 a plain swing does **nothing** and only the numbers, knock-back and sound show. A hit
of amount 0 still counts: the routine returns true, combo rises by one, the combo timer resets. So
the warrior's damage climbs as the chain runs. A plain hit can first do damage at combo 4
(armour × (1 − combo/50) < 6.25) and always does from combo 23 (< 3.75).

Action 2, the next in the chain, uses the off hand: with a second sword the same numbers; with a
shield in the off hand the chain is 10→9 instead; with an **empty** off hand W = 1, so x = 0.75 to
1.25 (§2.4).

Window: *S* = 1: P1 = 300, P2 = 200, A = max(300 + 100 − 100, 300) = 300, B = 400. Reach r = 2·bx,
f = 1. Knock-back *k* = 6, so **k** = (6·ρ·ux, 6·ρ·uy, 1.5·ρ) with ρ = √(warrior.bx / target.bx).
Stun: s = 0, none. Mana on hit (area flag clear, *S* = 1, total 1000): *g* = (0.05 to 0.15)·2 = 0.1 to
0.3, and one time in eight × 1.5, so 0.15 to 0.45.

---

## 7. The class of a spawn (0x5c7730)

### 7.1 The rule, complete

The general branch of the equipment generator writes the class exactly once, then at most once
more. These are **all** the writes to spawn +0x30 inside it (0x5c7a1c, 0x5c7a21, 0x5c7f36):

1. class := 1 + (draw & 3) (the same as draw mod 4; 0x5c7a1c).
2. **Type 97 only**: class := 4, immediately, before the class switch. So its weapon draws follow
   class 4.
3. **Type 86 only, mode 0 only**, near the end: class := 3, specialization := 1 (0x5c7f36). It comes
   after all weapon and armour draws, so those were made for the drawn class. A spawn of type 86 in
   mode 1 is not touched and keeps the drawn class.

**There is no other forced or remapped type.** Type 84 has no entry anywhere in the generator:
neither in the skip list, nor in the mode-0 multiplier switch, nor in a class test. The list of
types with their own treatment is:

| Type | Class effect |
|---|---|
| 97 | forced to 4 after its draw |
| 86 | forced to 3, specialization 1, mode 0 only, after the draws |
| 43, 45 | no class draw, class left as it was; specialization := 2 |
| 46, 47, 81, 82, 108, 109, 111, 112, 113, 114, 115, 117 | no class draw, class left as it was |
| 118, flag 0x18, controller 6 | no weapons, no class draw |
| flag 0x40 | generator returns at once |
| class bytes 0x80–0x89, set by creators before the call | generator stops after the chest |

Creators set the class themselves only as: towns 0x80–0x88 (0x5baa5b, 0x5bcc7a–0x5bd61e), prefab
marker 15 arguments 6, 13, 14, 15 → 0x83, 0x85, 0x86, 0x87 and 7 → 0x89 (0x5f42b9–0x5f42fc), and
arguments 9, 10, 11, 12 → classes 1, 2, 4, 3 for a type-2 level-20 creature with flag 0x40 (0x5f431c,
0x5f4433, 0x5f454d, 0x5f464f), which the generator leaves alone. None gives type 84. Later in the
zone generator the only writes to +0x30 are whole-record copies.

### 7.2 What explains the recording: the second run at activation

The generator runs a **second time** on the same spawn object, at activation, and overwrites class
and specialization **in place**. Behaviour §9.2 step 6: when a player comes in range, if the spawn's
map-cell record has mission kind 5 (`alpha-missions.md` §1.1) and the spawn has appearance flag
0x1000:

* unless controller 6: spawn level := mission level (cell +0x3c), tier := cell +0x40;
* `srand`(mission seed (cell +0x30) + low 32 bits of the spawn id), 0x60fd38;
* the generator in mode 1 (0x60fd4a), which redoes the whole class, specialization and gear part on
  that stream and skips the per-type multipliers and the boss part, then 6 item seeds.

A level of 171 points at this: it would be the mission level P_max. So spawn 8 is **believed** to be
a kind-5 mission creature and the class in the recording the mode-1 result, not the zone stream's.
Flag 0x1000 is set on location ring and centre creatures
(`alpha-location-creatures.md` §4.3).

This rests on the recording having been taken after activation, which this spec cannot see.

**Check.** The spawn id is (zy·65536 + zx)·256 + *i* = 0x80017F0108 for zone (32513, 32769), spawn 8,
so its low 32 bits are 0x017F0108 = 25 100 552. With the mission seed *m* of map cell (4064, 4096):
`srand`(*m* + 25 100 552), then the first `rand()` must satisfy draw & 3 = 3 (class 4). Second draw:
specialization = draw & 1 (odd or even). Third: rogue weapon, 3 + draw mod 3. If the cell's mission
kind is not 5 or the spawn lacks 0x1000, this explanation fails and the point is open again: the code
holds no other class write for it.

For a generator port: keep mode 0 at zone time exactly as it is, no type 84 rule. The extractor's
comparison should exclude, or re-derive through mode 1, spawns with flag 0x1000 in a kind-5 cell.
Level and tier of such spawns change at the same moment.

---

## 8. Corrections to earlier specs

1. **`alpha-creatures-behaviour.md` §4.2.** The "damage" is the block reference value, §5.1. Strikes
   use W and M. The heading should say so, and §5's x should cite §2 here.
2. **Behaviour §3.3, §8.1.** "The hit lands when the time reaches P1" is wrong: it lands in the
   window of §1.3, around P1 + P2/2.
3. **Behaviour §5 step 15.** The critical flag is byte **0x14**, not 0x44; 0x44 is the area flag.
   This closes B-2. Inputs of §5 also lack the area value (s). Hit-record table: §3.3 here, for
   `alpha-net-packets.md` §5.1 and its P-10.
4. **Behaviour §5.** The routine returns a value, §1.6.
5. **Behaviour §8.2.** Refine with §2.2: at action start only 3 and 4 have a cost among melee actions,
   and s takes mana or 1 for the listed actions.
6. **`alpha-projectiles.md` §2.6, "healer melee".** Those bursts (0x61608a, 0x61681b) are in the beam
   block: actions 28, 94, 95 only, not "any melee-group action". Melee healing is §1.6's rule.
7. **`alpha-projectiles.md` §2.6, reach "10 for action 104".** Action 104 dispatches to the general
   melee case (index table 0x622ccc), so the branches for it in the beam block (0x615c89, 0x615fe9,
   0x6162f6, 0x6164b0, 0x6165ee, 0x61666a) are never reached *as read*. No jump into that block from
   outside the table was found. 104 follows §2 and §1.4: a = 1, W·(5·s² + 1)·σ, r = 3·bx, f = 0.
8. **`alpha-server-simulation.md` §1, owner rule.** For melee it holds on offline clients and servers.
   An online client runs the melee case for every creature (0x612357) but applies nothing and sends
   only its own player's hits. Effects on the local copies of other creatures, such as hit flash and
   velocity, are visible only there. The beam and bomb blocks do use the strict rule.
9. **`alpha-projectiles.md` §2.2.** "The weapons in slots 6 and 7": see §5.2.
10. **`alpha-spawn-objects.md` §4.2 and behaviour §9.3.** Add type 86 to the rule (§7.1): the class
    is overwritten after the draws, mode 0 only. "Type 97 always 4" stands and is the only remap of
    the drawn value.

---

## 9. Open points

* **P6-1** Whether recorded spawn 8 is a kind-5 mission creature: needs the cell's mission record,
  §7.2. The code settles the rule; only the recording can confirm this instance.
* **P6-2** The 265 → 210 figure cannot be attributed from the code, §6.2.
* **P6-3** Reader of hit bytes 0x44 and 0x46, and of obj +0x1314.
* **P6-4** Action 73's case (0x616d35) was not traced; it is outside the melee case.
* **P6-5** What starts action 88, and the drain of action 28 (0x615d66 uses a stack value, *believed*
  dt, × 0.0002).
* **P6-6** 0x4453b0, called with each combo gain, walks the cooldown map and was read as having no
  effect; *believed*.
* **P6-7** The server's melee block was not re-compared here; the earlier instruction-level comparison
  of the two steps is relied on.
