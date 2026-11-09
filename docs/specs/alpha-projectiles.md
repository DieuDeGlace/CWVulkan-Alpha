# Alpha projectiles: shoot record, creation, flight, hits, drawing

Behavioural specification of Cube World **Alpha** projectiles (arrows, bolts, boomerangs,
spells, thrown rocks and stationary bursts). Written from the Alpha client (`Cube.exe`,
SHA-256 `84a7a132…30bf`): the world step 0x60c510 (its creation code lies in the action
switch at 0x611e1b, the projectile loop at 0x620a70–0x622b92; both are outside the Ghidra
listing and were disassembled from the binary), the frame routine 0x488ee0 (send/receive)
and the renderer 0x4ac260 (lights 0x4af380, models 0x4b8151). RVAs are citations only.
This resolves open question F-3b of `alpha-creatures-behaviour.md`.

Related: `alpha-creatures-behaviour.md` (§3 actions and phases, §5 hit resolution, §14
sounds, §16 abilities), `alpha-net-packets.md` (packet 9, world-update section 5),
`alpha-creatures-models.md` (model ids), `alpha-rendering.md` §10 (point lights).

---

## 0. Conventions

* Positions are i64 in **world units** (65 536 per block); vectors, speeds and sizes are f32
  in **blocks** (speeds in blocks per second). "→ wu" means: multiply the f32 by 65 536 in
  single precision and convert to i64 by truncation toward zero.
* dt = the step's elapsed milliseconds (integer); τ = f32(dt) × 0.001 (seconds, single).
* *u* = one `rand()` draw ÷ 32767 (0..1). "draw" = one `rand()`.
* **P1, P2, total** of an action are the phase routines of `alpha-creatures-behaviour.md`
  §3.3/§8.1. *t* is the action time **before** this step's advance (the advance at
  0x618541 runs after the creation code). "Crosses *x*" means *t* ≤ *x* < *t* + dt.
* "obj +x" = creature object offset (= record offset + 0x10), as in the behaviour spec.
  Creature fields used: obj +0x10 position, +0x64 type, +0x6c action time, +0x68 action
  (u8), +0x70 combo counter (field 11), +0x74 combo timer, +0x80 collision box (3 × f32,
  appearance +0x08), +0x128 dodge time (field 15), +0x134 field 18 (slow countdown),
  +0x140/+0x141 class/specialization, +0x144 **charge** (field 23), +0x160 aim vector
  (field 26), +0x16c health, +0x170 mana (field 28), +0x1b0 field 39, +0xaa9 main-hand
  subtype, +0x11b4 a set of creature ids ("evaded by", below), +0x11c0 a u64 creature id
  excluded from hits (*inferred*: the creature's pet; §1 of the behaviour spec places the
  pet id at +0x11c8 — to reconcile), +0x13b4 **spent charge** (the charge taken when the
  action fired).
* The world object holds the projectile list at world +0x14 (= game object G+0x2f8).

---

## 1. The shoot record (112 bytes, packet 9 / world-update section 5)

The record is both the network message and the live projectile: the world list stores
these records unchanged, and the step mutates them in place.

| Off | Type | Meaning | Default (constructor 0x459660) |
|---|---|---|---|
| 0x00 | u64 | **shooter id** | −1 |
| 0x08 | 3 × i32 | copy of the shooter's record field 39 (record 0x1a0; element 2 is the spawn index). Not read by the projectile code | −1, −1, 0 |
| 0x14 | 4 bytes | not written (stack garbage on the wire) | — |
| 0x18 | 3 × i64 | **position**, world units | not set |
| 0x30 | 3 × f32 | never written by a creator; never read | 0, 0, 0 |
| 0x3c | 3 × f32 | **velocity**, blocks/s | not set |
| 0x48 | f32 | **damage** (negative never; healing is applied as a negative hit, §6.3) | not set |
| 0x4c | f32 | **radius** *r*: half-size of the hit cube, blocks; also light and trail size | 0.5 |
| 0x50 | f32 | **draw scale**: the model is scaled by value/14 | 1.0 |
| 0x54 | f32 | **strength** *c*: knock-back strength and the hit routine's "area value" | 1.0 |
| 0x58 | f32 | mana cost or charge spent at launch (written, never read by the client) | 0 |
| 0x5c | u8 | **special flag**: the projectile does not hit directly but bursts (§6.4); it also consumes a proc buff at launch | 0 |
| 0x5d | 3 bytes | padding | — |
| 0x60 | i32 | **kind**: 0 arrow/bolt, 1 spell, 2 boomerang, 3 stationary burst, 4 thrown rock | 0 |
| 0x64 | u8 | **element**: 0 none, 1 fire, 2 water (water heals friends, §6.3) | 0 |
| 0x65 | 3 bytes | padding | — |
| 0x68 | i32 | **age**, ms | 0 |
| 0x6c | i32 | slow (ms) given to a hit target's field 18 if larger; no Alpha creator sets it (always 0) | 0 |

This answers the shoot part of open question P-9 of `alpha-net-packets.md`.

---

## 2. Who creates projectiles

Creation runs inside the world step's per-creature action switch (0x611e1b, table
0x622c84/0x622ccc). It runs for a creature only if the machine owns it: **online**, only for
the local player (world +0xb8); **offline**, for the local player and every creature whose
controller kind ≠ 0. Each new record is appended to the world projectile list **and** to
the frame's shoot output; the packet-9 sender (0x49c1e2) sends every output record whose
shooter id is the local player (`alpha-net-packets.md` §4.2).

### 2.1 Charge (field 23) — resolves part of §16.10 of the behaviour spec

World step 0x61a74d–0x61a918, every step, after the action switch:

* Action 36: charge += *S*·dt·0.0005 (no phase test).
* Actions 8, 24, 25, 27, 59, 63, 64 with *t* ≥ P1: *q* = *S*·dt·0.00075 (*S* = attack-speed
  factor 0x447700). Class 2 specialization 0: *q* ×= 1 + min(combo/comboMax, 4). Buff kind 2
  active (warfrenzy): *q* ×= 1 + 9·*v*(102). charge += *q*. Buff kind 10 active:
  charge := mana.
* Always: charge := min(charge, mana).

So a ranger (*S* = 1) gains 0.012 per 16-ms step and reaches 1.0 after 84 steps (1344 ms),
capped by mana.
While charging, every 200-ms boundary of the action time plays sound 55 (charge2.wav) at
the creature, pitch 1 + 0.5·charge (0x615bb3; ids as `alpha-creatures-behaviour.md` §14).

### 2.2 Damage bases

* **W** (0x43dc60, "weapon damage"): *L* = level, *g* = 3·(1 − 1/((*L* − 1)·0.05 + 1));
  W = 2^*g* × damage multiplier (record field 30 element 2) + Σ item damage (0x4c7f60,
  `alpha-creatures-behaviour.md` §10.3) of the weapons in slots 6 and 7. With no weapon:
  + 2^*g*·2^(0.25·tier)·2 if appearance flag 0x08, + 2^*g*·2^(0.25·tier)·8 if flag 0x10.
  Then × 50 during action 87, × 2 during 91, × 5 during 93, × 3 during 96.
* **M** (0x446e80, "magic damage"): the same without the per-action factor.
* **a(id)** (0x43ec90): 0.5 for 5, 26; 0.1 for 21, 72; 0.4 for 30, 32; 0.6 for 31, 33;
  2 for 37, 43, 57, 58, 68; 0.25 for 55; 1 otherwise.
* **Spread** σ = (1.25 − 0.5·*u*) + 0 (one draw; 0.75..1.25).
* Products are single precision in the order written.

### 2.3 Bow, crossbow, boomerang: actions 22, 23, 26 (0x61707a)

1. **When P1 is crossed**: mana −= cost(action) if cost > 0, else mana −= charge; floor 0.
   Spent charge *s* (obj +0x13b4) := charge; charge := 0.
2. **Fire** when P1 is crossed, and for action 26 with *s* > 0 also when P1 + 300 is crossed
   (a charged boomerang throws a second one).
3. Record: shooter id; field-39 copy; position = shooter position; velocity = **100** ×
   unit(aim vector); age 0; kind 0, or **2** for action 26.
   * Action 26: radius 2·*s* + 2, strength 0.2·*s*, draw scale = the shooter's appearance
     scalar at appearance +0x38 (record 0xa0). Others: radius 0.5, strength *s*, scale 1.
   * +0x58 := cost if cost > 0 (strength := cost too), else the strength.
   * Special flag := 1 if *s* > 0 or action is 23. If set and the shooter has a buff of kind
     10, that buff is cleared (re-added with duration 0, 0x446af0) and an "event 40" record
     is emitted (`alpha-creatures-behaviour.md` §16.6).
   * Damage = a·W·(K·*s*² + B)·σ with K = 5 (1 for 26), B = 1 if *s* = 0 else 0.1, and B = 0
     for action 23.
4. **Sound** at the shooter: 22/23 with the flag set → 22 (salvo2.wav), pitch 1 + 0.5*u*;
   22/23 without → 15 (swing.wav), pitch 2 + 0.5*u*; 26 → 15, pitch 0.7 + 0.51*u* (double).

### 2.4 Retreat, salvo, shuriken: actions 50, 55, 96 (0x617634)

1. When P1 is crossed: *s* := charge; mana −= charge; charge := 0; a kind-10 buff is
   consumed as in §2.3.
2. Let *N* = ⌊P2/4⌋. While P1 ≤ *t* < P1 + P2, each step in which ⌊(*t* + dt)/*N*⌋ ≠ ⌊*t*/*N*⌋
   and the aim vector is non-zero fires **one** projectile (four per action at most):
   kind 0, velocity **150** × unit(aim), position = shooter, radius 0.5, strength 0.25·*s*,
   +0x58 = 0, special flag 1, scale 1, damage = a·W·(5·*s*² + B)·σ with B = 1 (0.1 for 55).
3. Sound 22 (salvo2.wav), pitch 1 + 0.5*u*, at the shooter, per projectile.

### 2.5 Spells: actions 37–46 and 108 (0x617a21)

1. **Channel sound** while *t* ≤ P1 − 200: each 200-ms boundary of *t* plays 36
   (channel2.wav), pitch 1 + 0.25*u*.
2. **Fire time** F = P1. For 45 and 46: *k* = ⌊(*t* + dt − P1)/⌊P2/3⌋⌋ (toward zero),
   F = P1 + *k*·⌊P2/3⌋, but not below P1 — a volley of up to three shots in P2.
3. When P1 is crossed and the aim is non-zero: mana −= cost(action), floor 0.
4. **Fire** when *t* < P1 + P2 and F is crossed and the aim is non-zero. With *c* = the
   charge (field 23) **at that moment**:
   * velocity = unit(aim) × (**100** if the main hand is a bracelet (12), else **50**);
     position = shooter (type 101: + its box height·0.5 in z and + its box y in y);
     radius 4·*c* + 0.5; strength *c*; scale *c* + 0.5; age 0.
   * Action 108 (thrown rock): kind **4**, velocity 100 × unit(aim), damage =
     a·W·σ·1·5, scale 4, strength 1.
   * Otherwise kind **1**: 38–42 and 44: damage = a·M·σ; 45, 46: same, strength 0.25,
     scale *c* + 1; 37, 43: same, strength 1, scale *c* + 1.5.
   * Element: shooter class 3 → 2 if specialization 1 else 1; others 0.
   * Special flag := 1 for 37, 43, 45, 46, else 0.
5. Appended to the world list; then if *c* > 0: mana −= *c* (floor 0), charge := 0. If the
   flag is set and a kind-9 buff is active, it is consumed (event-40 record). Then appended
   to the output.
6. Sound at the shooter, volume 0.9: element 2 → 41 (watersplash.wav), pitch 1.25 for 41,
   42, 44 else 1; otherwise 38 (fireball.wav), pitch 2 for 38, 39, 40 else 1.

### 2.6 Stationary bursts (kind 3)

* **Actions 89, 90** (0x6151f9): channel sound 36 as §2.5.1; when P1 is crossed sound 38,
  pitch 1 + 0.25*u*, and one record: kind 3, element 1 (89) or 2 (90), position = caster,
  velocity 0, damage = a·M·2, radius 8, scale 1, strength 0, flag 0.
* **Healer melee** (class 3 specialization 1, any melee-group action; 0x61608a, 0x61681b):
  when P1 is crossed, kind 3, element 2, damage 0.1·M, radius 4, scale 1, strength 0,
  velocity 0, at the attack's reach point (the attack origin 0x446bb0 + reach·unit(aim);
  reach = the block ray cast 0x5a35d0, or 10 for action 104); and one more at the attack
  origin for each creature the swing hits.

---

## 3. Receiving shoots

World-update section 5 records (relayed packet 9) are queued at G+0x800650. Once per frame
0x488ee0 (0x490390) appends every record whose shooter id is **not** the local player to
the world projectile list as received (the local player's own shoots already exist
locally). They are then simulated exactly like local ones from their launch position; no
latency compensation is applied.

---

## 4. Per-step update (0x620a70–0x620d68), for each projectile *p* in list order

1. **Velocity**, by kind:
   * kind 1: unchanged (no gravity).
   * kind 2 (boomerang): find the shooter; none → **remove**. Age > 1200 → **remove**
     (caught). 1100 < age ≤ 1200: *p*.pos += (shooter.pos − *p*.pos)·*f*·*f* with *f* =
     (age − 1100)/100 (fixed-point, 0x42c900). 800 < age ≤ 1100: velocity := 10 ×
     (shooter.pos − *p*.pos) in blocks. Age ≤ 800: aim point A = shooter.pos + aim vector →
     wu; target T = 5 × (A − *p*.pos) in blocks; repeat dt times: velocity += (T −
     velocity)·0.05 (0x605950, per millisecond).
   * kinds 0, 3, 4: velocity.z −= τ·30·0.25 (gravity **7.5 blocks/s²**).
2. **Displacement** *d* = τ·velocity (component-wise, single).
3. age += dt.
4. **Timeout** (kind ≠ 2) when age > **5000**: kind 3 → remove. Others: if the shooter
   exists, its combo counter := 0 and its action time := ⌊time·*S*old/*S*new⌋ (*S* before
   and after the reset, as in behaviour §8.1); remove.
5. Otherwise run the **flight** of §5. Removed projectiles are erased after the whole list
   has been processed.

---

## 5. Flight, creature and block collision (0x621465–0x622b92)

Set up: *n* = trunc(f32(2·|*d*|) + 1) sub-steps; *ds* = *d*/*n*; **k** (knock-back) =
unit(velocity) × 5·*c* with k.z := 3·*c* (*c* = strength +0x54; **k** = 0 for a still
projectile). Flags: *destroy* := 0, *wall* := 0, *landed* := 0 (all per sub-step, *landed*
kept across).

For each sub-step *i* = 0..*n* − 1:

### 5.1 Creatures

For every creature *C* with health > 0, in creature-list order:

1. **Eligible**: unless kind 3 with element 2, the shooter must exist, *C* ≠ shooter,
   0x5988a0(shooter, *C*) ("can damage": controller 6 on either side → yes; a controller-1
   creature against a non-1 → yes, 1 against 1 only when exactly one is a harmless animal;
   otherwise only if either has mode flag 0x20), and *C* is not the shooter's +0x11c0 id.
2. **Overlap**: on each axis, *p*.pos + *r* ≥ *C*.pos − *b*/2 and *p*.pos − *r* ≤ *C*.pos +
   *b*/2 (*b* = *C*'s collision box, *r* = radius, in wu; the box is centred on the creature
   position on all three axes *as read*).
3. **Line of sight** 0x59ee90(*p*.pos, *C*, 1, 200).
4. **Dodging** (*C* dodge time ≠ 0): *C*'s id is added to the shooter's +0x11b4 set; no
   hit; next creature.
5. **Hit gate**, by kind:
   * kind 0, 1, 4 with flag 0: hit.
   * kind 3: hit only in a step in which ⌊age/1000⌋ changed (one tick per second).
   * kind 2: hit only if age > 300 and ⌊age/100⌋ changed this step.
   * kinds 0, 1, 4 **with** flag 1: no hit (they burst instead, §6.4).
   (The gate is evaluated in every sub-step, so a burst or boomerang may hit the same
   creature once per sub-step of a qualifying step, *as read*.)
6. **On a hit** (§6.1–6.3); then kinds 0, 1, 4 set *destroy* and stop scanning creatures.
   For a non-hit, kinds 0, 1, 4 also set *destroy* and stop (a flagged projectile is spent
   on contact). Kinds 2, 3 continue with the next creature.

### 5.2 Movement and blocks (kinds other than 3)

1. Kind 2: whenever ⌊age/200⌋ changed this step, sound 15 (swing.wav), pitch 1 + 0.25*u*,
   at *p* (each sub-step, *as read*).
2. *p*.pos += *ds* → wu.
3. **Solid block** at *p*.pos (0x42f860; solid = block type not 0 or 2, 0x43b480): *p*.pos
   −= *ds* → wu; *wall* := 1. Kind 0 also emits a **particle burst** at *p*.pos: 4
   particles, colour = the block's RGB/255 with alpha 1, velocity (0, 0, 10), size 0.1
   (record 0x465ed0; output list +0x10).
4. Otherwise a **ray cast** 0x5a35d0 from *p*.pos along unit(velocity), limit *L* =
   |velocity|·τ: if it stops short of *L*, *p*.pos −= *ds* and *wall* := 1.
5. If *wall*, not *destroy*, kind ≠ 3 and the shooter exists: the shooter's combo is reset
   with the time rescale of §4 step 4 (a miss into terrain ends the chain).
6. Kind 2: if *landed*, the shooter's combo += 1 and its combo timer := 0. If *wall*:
   velocity := 0 (the return of §4 brings it back). Next sub-step.
7. Other kinds: neither *destroy* nor *wall* → next sub-step; otherwise **end of flight**
   (§6.4) and stop.

Kind 3 skips 5.2 entirely (and has *n* = 1).

---

## 6. Hits

### 6.1 Mana and procs

* **Mana on hit** (kinds 0 and 2, flag 0, shooter owned here — the local player, or any
  creature offline): *g* = 0.1 + 0.05·(1 − 2*u*), × 0.2 for kind 2, × (the shooter's current
  action total)/300, × 2 when draw mod 8 = 0; mana := min(1, mana + *g*).
* **Critical**: one critical draw of the shooter (0x4444a0, behaviour §4.5); damage *x* =
  +0x48, × 2 if critical; no shooter → not critical. *x* < 0 → 0.
* **Ranger proc** (owned shooter, chance test 0x4441a0(shooter, 0.15), flag 0, shooter
  class 2 specialization 1): buff kind 10 for 30 000 ms (event-40 record) and sound 47
  (upgrade2.wav) at the shooter. A kind-10 buff fills the charge instantly (§2.1) and is
  consumed by the next charged shot.

### 6.2 Damage (element ≠ 2)

The hit routine 0x596d30 (`alpha-creatures-behaviour.md` §5) is called with: target *C*,
attacker = shooter (or none), damage *x*, critical, **area flag** = the special flag,
**area value** = strength, hit position = *p*.pos, knock-back **k**, the frame outputs,
**magic** = (kind = 1), 0, the projectile record, **can be blocked** = 1. A true result
sets *landed*. Then *C*.field 18 := max(*C*.field 18, +0x6c).

Who computes it: every client simulates every projectile and runs the hit routine, but the
hit record carries the shooter as attacker and only the shooter's own client sends records
whose first 8 bytes are its id (packet 7). So, as with melee, **the shooter's client
decides the hit**; the target's owner applies it (`alpha-net-packets.md` §5.1).

### 6.3 Healing (element 2)

If the shooter exists, is **not** hostile to *C* (0x596ca0) and *C*'s controller ≠ 6: a
72-byte hit record (attacker = shooter, target = *C*, position = *p*.pos, amount = −*x*,
halved for a controller-1 shooter, critical flag) is queued; offline, or when both shooter
and *C* are the local player, it is applied at once (0x595a60). Hostile targets take
nothing from water.

### 6.4 End of flight (0x622457)

* Kinds 0, 2, 4 with flag 0: no burst.
* Kind 1, or any flagged projectile: if it ended on a block (not *destroy*), an impact
  sound at *p*.pos, pitch 1 + 0.4*u*: kind 1 → 42 (watersplash-hit.wav) for water, 39
  (fire-hit.wav) otherwise; other kinds → 20 (blade1.wav). Then a **burst**: every
  creature passing §5.1 steps 1, 3, 4 (with the shooter tests only if a shooter exists)
  and overlapping a **5-block** cube around *p*.pos is hit as §6.2 (critical drawn per
  creature; damage +0x48). The fire proc applies once per creature hit: owned shooter,
  chance test 0x4441a0(shooter, 0.25), flag 0, kind 1, element 1 → buff kind 9 (free big
  spell) for 30 000 ms, event-40 record, sound 47. Offline, a player shooter that is not
  the local player skips the burst. *As read*, an unflagged spell therefore hits the
  creature it touched twice (direct hit, then burst).
* If *landed* and kind ≠ 3: shooter combo += 1, combo timer := 0.
* **Water spells** (kind 1, element 2) are not removed: they become a **kind-3 puddle** at
  the impact point: age := 0 if flagged else 3000 (so 5 s or 2 s of life), damage ×= 0.05,
  radius := 5·strength + 5, strength := 0, flag := 0. Every other projectile is removed.

(A "sound 19, arrow-destroy.wav" branch at 0x6223ec is unreachable *as read*: arrows end
silently.)

---

## 7. Drawing

* **Visibility**: 0x47f760(*p*.pos, radius) — skipped when out of view.
* **Model** by kind: 0 → model **840** (`arrow`); 2 → the shooter's main-hand item model
  (0x4ec400) when the shooter exists and slot 7 is type 3 subtype 8 (boomerang), else 840;
  4 → model **2101** (`iron-deposit`, used as a rock); 1 and 3 → no model.
* **Brightness**: kinds other than 1 take the world light at *p*'s block (0x4718b0)/255 as
  the material alpha term (material (1, 1, 1, light)).
* **Orientation**: kind 2: tilt min(0.5·age, 90)° then spin −1.5·age° (0x4244f0, 0x4243d0);
  kind 4: rotate 0.5·age° about velocity × (0, 0, 1); others with non-zero velocity: look
  along unit(velocity) with up (0, 1, 0) (0x4c12f0). Scale = +0x50/14 on all axes; the
  model is centred (−½ size).
* **Trails** (ribbon routine 0x471d50, internals not traced): kind 0 with strength ≠ 0:
  20 segments, colours (0.9, 0.9, 1, 1), (0.75, 0.75, 1, 1), (0.5, 0.5, 1, 1), widths from
  0.5·strength and strength, along unit(velocity), keyed by age. Kind 1: 20 segments,
  colours fire (1, 0, 0, 1), (1, 0.25, 0, 1), (1, 0.5, 0, 1) or, for a specialization-1
  shooter, water (0, 0, 1, 1), (0, 0.25, 1, 1), (0.25, 0.5, 1, 1); size from 5·scale.
* **Light** (0x4af380): each kind-1 projectile adds a point light at *p*.pos, colour
  (0.5, 0.5, 0.2) × radius, light radius **16**, into the frame light list of
  `alpha-rendering.md` §10 (which does not list this source yet).
* A third pass over the list (0x4b8cd0) was not traced.

---

## 8. Test vectors (single precision, dt = 16)

τ = 0.016, gravity per step 0.120000005.

**Arrow**, action 22 uncharged, aim (1, 0, 0), launched at (0, 0, 0) wu, nothing in the way
(*n* = 4 sub-steps, each adds trunc(0.4·65536) = 26214 in x):

| Step | velocity.z | position (wu) | blocks |
|---|---|---|---|
| 1 | −0.12 | (104856, 0, −124) | (1.59998, 0, −0.00189) |
| 2 | −0.24 | (209712, 0, −372) | (3.19995, 0, −0.00568) |
| 3 | −0.36 | (314568, 0, −748) | (4.79993, 0, −0.01141) |

It times out after 313 steps (age 5008 > 5000) if it hits nothing.

**Spell** kind 1, velocity 50: *n* = 2, 52428 wu (0.79999 blocks) per step, no drop.

**Damage**: level-1 ranger, level-1 rarity-0 bow (item damage 8): W = 1 + 8 = 9. Action 22
uncharged: 9·1·σ = **11.25** (draw 0) … **6.75** (draw 32767); critical × 2. Charged with
*s* = 0.5: 9·(5·0.25 + 0.1)·σ = 12.15·σ, flagged → a 5-block burst on impact, knock-back
(2.5·ux, 2.5·uy, 1.5). Level-1 mage with a level-1 wand (M = 9), action 38: 9·σ, radius 0.5.

**Mana on hit**: action 22 at *S* = 1 has total 550 ms: *g* from 0.05·550/300 = 0.0917 to
0.15·550/300 = 0.275, doubled one time in eight.

---

## 9. Open points

* Ribbon routine 0x471d50 parameters in detail, and the third projectile pass 0x4b8cd0.
* The +0x11c0 id (pet?) vs §1's pet id at +0x11c8; the +0x11b4 "evaded by" set's reader.
* Whether the double hit of unflagged spells (§6.4) and the per-sub-step repeat of
  boomerang/burst hits (§5.1 step 5) happen in the running game — check with a recording.
* What starts actions 89 and 90, and who sets shoot +0x6c (slow) in Alpha (no creator does).
