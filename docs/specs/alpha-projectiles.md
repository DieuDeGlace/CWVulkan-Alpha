Status: FINAL / STABLE / HELD

# Alpha projectiles: shoot record, creation, flight, hits, drawing

Behavioural specification of Cube World **Alpha** projectiles (arrows, bolts, boomerangs,
spells, thrown rocks and stationary bursts). Evidence: the Alpha client image `Cube.exe`
(SHA-256 `84a7a132…30bf`, 32-bit, image base 0x400000). **Every address in this document
is an RVA in that image.** Main routines: the world step RVA 0x20c510 (action switch at
RVA 0x211e1b with tables RVA 0x222c84/0x222ccc; projectile loop RVA 0x220a70–0x222b92),
the frame routine RVA 0x88ee0 (receive and send), and the renderer RVA 0xac260 (lights
RVA 0xaf380, models RVA 0xb8151). The Ghidra decompilations of these three routines timed
out; the facts below were read from disassembly of the image bytes. RVAs are citations only.
Intended to resolve open question F-3b of `alpha-creatures-behaviour.md`.

Related: `alpha-creatures-behaviour.md` (actions and phases, hit resolution, sounds,
abilities), `alpha-net-packets.md` (packet 9, world-update section 5),
`alpha-creatures-models.md` (model ids), `alpha-rendering.md` §10 (point lights).

---

## 0. Conventions

* Positions are i64 in **world units** (65 536 per block); vectors, speeds and sizes are f32
  in **blocks** (speeds in blocks per second). "→ wu" means: multiply the f32 by 65 536 in
  single precision and convert to i64 by truncation toward zero (each component separately).
  "In blocks" means: convert the i64 to f32 and multiply by 1/65 536.
* dt = the step's elapsed milliseconds (integer); τ = f32(dt) × 0.001 (single). The frame
  routine (RVA 0x8cef6) splits a frame of E ms into steps: when E > 20 it first runs
  ⌊(E − 21)/20⌋ + 1 steps of 20 ms, then one step with the remainder; so every step has
  1 ≤ dt ≤ 20 for positive E.
* *u* = one `rand()` draw ÷ 32767 (0..1); "draw" = one `rand()`. Where a draw is scaled,
  the image multiplies the draw (as f32) by the factor first and divides by 32767 after.
* **P1, P2, total** of an action are the phase routines RVA 0x3caa0, RVA 0x47310 and
  RVA 0x3d1a0 (described in `alpha-creatures-behaviour.md`). In the creation code *t* is the
  action time **before** this step's advance (the advance at RVA 0x218541 runs after the
  action switch). "Crosses *x*" means *t* ≤ *x* < *t* + dt.
* *S* = the attack-speed factor RVA 0x47700 of a creature.
* "obj +x" = creature object offset (the companion behaviour spec equates it with record
  offset + 0x10). Creature fields used here: obj +0x08 creature id (u64), +0x10 position,
  +0x60 controller kind (u8), +0x64 type, +0x68 action (u8), +0x6c action time, +0x70 combo
  counter, +0x74 combo timer, +0x7e appearance flags (u16), +0x80 collision box (3 × f32,
  full extents), +0xb0 a scalar used as boomerang draw scale, +0x124 mode flags, +0x128 dodge
  time, +0x134 field 18 (a millisecond value raised by projectile hits), +0x140/+0x141
  class/specialization, +0x144 **charge**, +0x160 aim vector, +0x16c health, +0x170 mana,
  +0x180 damage multiplier, +0x190 level, +0x1a8 tier byte, +0x1b0 field 39 (3 × i32),
  +0x990 off-hand item (type byte, subtype byte at +0x991), +0xaa8 main-hand item (type
  byte, subtype byte at +0xaa9), +0x11b4 a set of creature ids ("evaded by"), +0x11c0 a u64
  creature id excluded from hits, +0x13b4 **spent charge** (the charge taken when the
  action fired).
* The world object is game object G + 0x2e4. It holds the creature list at world +0x04,
  the projectile list at world +0x14 (= G + 0x2f8), an online flag (u8) at world +0xb4 and
  the local player at world +0xb8.
* **Owned** (used by the creators): online (world +0xb4 ≠ 0) a creature is owned only if
  it is the local player; offline, if it is the local player or its controller kind ≠ 0.

---

## 1. The shoot record (112 bytes, packet 9 / world-update section 5)

The record is both the network message and the live projectile: the world list stores
these records unchanged, and the step mutates them in place.

| Off | Type | Meaning | Default (constructor RVA 0x59660) |
|---|---|---|---|
| 0x00 | u64 | **shooter id** | −1 |
| 0x08 | 3 × i32 | copy of the shooter's obj +0x1b0 (field 39), written by the bow, salvo and spell creators only. Not read by the projectile code | −1, −1, 0 |
| 0x14 | 4 bytes | not written by the constructor or any creator | — |
| 0x18 | 3 × i64 | **position**, world units | not set |
| 0x30 | 3 × f32 | never written by a creator; not read by the projectile code | 0, 0, 0 |
| 0x3c | 3 × f32 | **velocity**, blocks/s | not set |
| 0x48 | f32 | **damage** (no creator produces a negative value; healing is applied as a negative amount, §6.3) | not set |
| 0x4c | f32 | **radius** *r*: half-size of the direct-hit cube, blocks; also scales the light colour and is passed to the view test | 0.5 |
| 0x50 | f32 | **draw scale**: the model is scaled by value/14; kind-1 trail size | 1.0 |
| 0x54 | f32 | **strength** *c*: knock-back strength, passed to the hit routine, kind-0 trail width | 1.0 |
| 0x58 | f32 | written at launch (§2.3, §2.4); not read by the projectile, hit or drawing code | 0 |
| 0x5c | u8 | **special flag**: the projectile does not hit directly but bursts (§6.4); bow and spell launches with the flag consume a proc buff | 0 |
| 0x5d | 3 bytes | padding | — |
| 0x60 | i32 | **kind**: 0 arrow/bolt, 1 spell, 2 boomerang, 3 stationary burst, 4 thrown rock | 0 |
| 0x64 | u8 | **element**: 0 none, 1 fire, 2 water (water heals non-hostile targets, §6.3) | 0 |
| 0x65 | 3 bytes | padding | — |
| 0x68 | i32 | **age**, ms | 0 |
| 0x6c | i32 | raises a hit target's field 18 to this value if larger; no Alpha creator sets it (always 0) | 0 |

Intended to answer the shoot part of open question P-9 of `alpha-net-packets.md`.

---

## 2. Who creates projectiles

Creation runs inside the world step's per-creature action switch (RVA 0x211e1b). Action →
handler, from the switch tables: 22, 23, 26 → RVA 0x21707a; 50, 55, 96 → RVA 0x217634;
37–46 and 108 → RVA 0x217a21; 89, 90 → RVA 0x2151f9; 28, 94, 95 → RVA 0x215c68; 24, 25,
27, 36, 59, 63, 64 → RVA 0x215bb3 (charge sound). Each handler acts only for an owned
creature (§0), except the channel sound of 89/90 (§2.6). Each new record is appended to the
world projectile list **and** to the frame's shoot output (frame outputs +0x20). In the
frame routine (RVA 0x9c1e2) every shoot-output record whose shooter id equals the local
player's id is copied to the outgoing list at G + 0x8006fc (packet 9 per
`alpha-net-packets.md` §4.2).

### 2.1 Charge (field obj +0x144)

World step RVA 0x21a74d–0x21a918, every step, after the action-time advance (so the phase
test below compares the **advanced** action time with P1):

* Action 36: charge += *S* × (f32(dt) × 0.0005) (no phase test).
* Actions 8, 24, 25, 27, 59, 63, 64 with advanced action time ≥ P1: *q* = *S* × (f32(dt) ×
  0.00075). Class 2 specialization 0: *q* := (1 + min(combo / *m*, 4)) × *q*, with *m* the
  integer returned by RVA 0x44cb0 and the division in single precision. Buff kind 2 active
  (RVA 0x43f10): *q* := (1 + 9 × *v*) × *q* with *v* = RVA 0x3ed60(102). charge += *q*.
  Then, if buff kind 10 is active: charge := mana.
* Always: if charge > mana, charge := mana.

So a creature with *S* = 1 and no modifiers gains 0.012 per 16-ms step once past P1 and
reaches 1.0 after 84 such steps (1344 ms), capped by mana.

**Charge sound** (RVA 0x215bb3), for owned creatures in actions 24, 25, 27, 36, 59, 63, 64
(not action 8), with no phase test: whenever ⌊(*t* + dt)/200⌋ ≠ ⌊*t*/200⌋, sound 55
(charge2.wav) at the creature, pitch 1 + 0.5 × charge (no draw).

### 2.2 Damage bases

* **W** (RVA 0x3dc60): *L* = level (obj +0x190), *g* = 3 × (1 − 1/((*L* − 1) × 0.05 + 1));
  base = damage multiplier (obj +0x180) × (2^(0.25 × 0) × 2^*g*) (powers evaluated in double,
  rounded to single). Then the weapon set of RVA 0x3df60 is formed:
  * if the main-hand item (obj +0xaa8) has type 3 and subtype 6 or 7, the set is that item
    alone; else if the off-hand item (obj +0x990) has type 3 and subtype 6 or 7, that item
    alone;
  * otherwise it depends on the action. For actions 22, 23, 26, 50, 55 and 108 (and most
    others): the main hand if its type is 3, then the off hand if its type is 3 and its
    subtype is 12. For action 96: the off hand if its type is 3, then the main hand if its
    type is 3. (Other actions use other selections; see the behaviour spec.)
  If the set is non-empty, W = base + Σ item damage (RVA 0xc7f60) over the set, in set
  order. If it is empty: W = base, + 2^*g* × 2^(0.25 × tier) × 2 if appearance flag 0x08
  is set, + 2^*g* × 2^(0.25 × tier) × 8 if flag 0x10 is set (tier = obj +0x1a8). Finally
  W × 50 during action 87, × 2 during 91, × 5 during 93, × 3 during 96.
* **M** (RVA 0x46e80): the same computation without the per-action factor.
* **a(id)** (RVA 0x3ec90): 0.5 for 5, 26; 0.1 for 21, 72; 0.4 for 30, 32; 0.6 for 31, 33;
  2 for 37, 43, 57, 58, 68; 0.25 for 55; 1 otherwise.
* **Spread** σ = (1.25 − 0.5·*u*) + 0 (one draw; 0.75..1.25).
* **cost(id)** (RVA 0x44ae0) is 0 for actions 22, 23, 26, 38, 50, 55, 96 and 108 (other
  spell actions were not evaluated here).
* Products are single precision in the order written.

### 2.3 Bow, crossbow, boomerang: actions 22, 23, 26 (RVA 0x21707a)

1. **When P1 is crossed**: if cost(action) > 0, mana −= cost, else mana −= charge; then
   mana := 0 if negative. Spent charge *s* (obj +0x13b4) := charge; charge := 0. (cost is
   0 for these actions, so mana −= charge.)
2. **Fire** when P1 is crossed; for action 26 with *s* > 0 also when P1 + 300 is crossed
   (a charged boomerang throws a second one). There is no aim test.
3. Record (constructor defaults otherwise): shooter id; field-39 copy; position = shooter
   position; velocity = **100** × unit(aim vector) (RVA 0x27870, normalisation via a double
   square root); age 0; radius 0.5; kind 0, or **2** for action 26.
   * Action 26: radius 2·*s* + 2, strength 0.2·*s*, draw scale = obj +0xb0. Others:
     strength *s*, scale 1.
   * +0x58 := cost if cost > 0 (and then strength := cost too), else the strength.
   * Special flag := 1 if *s* > 0 or the action is 23. If set and the shooter has an active
     buff of kind 10, that buff is cleared (re-added with duration 0, RVA 0x46af0) and a
     40-byte buff record (shooter id twice, then the buff) is appended to frame outputs
     +0x58 (the "event-40" record of `alpha-creatures-behaviour.md`).
   * Damage = σ × ((a × W) × (*s*² × K + B)) with K = 5 (1 for 26); B = 1 if the record's
     strength is 0, else 0.1; B = 0 for action 23. One draw (σ).
   * Appended to the world list, then to the shoot output.
4. **Sound** at the shooter (one more draw): 22/23 with the flag set → 22 (salvo2.wav),
   pitch 1 + 0.5*u*; 22/23 without → 15 (swing.wav), pitch 2 + 0.5*u*; 26 → 15, pitch
   computed in double as (draw × 0.51)/32767 + 0.7 (the single-precision 0.7), then
   rounded to single.

Draw order per shot: σ, then the pitch draw.

### 2.4 Retreat, salvo, shuriken: actions 50, 55, 96 (RVA 0x217634)

1. When P1 is crossed: *s* := charge; mana −= charge (no floor here); charge := 0; if a
   buff of kind 10 is active it is consumed as in §2.3 (regardless of any flag).
2. Let *N* = ⌊P2/4⌋ (toward zero). If P1 ≤ *t* < P1 + P2, ⌊(*t* + dt)/*N*⌋ ≠ ⌊*t*/*N*⌋ and
   |aim| > 0, one projectile is fired in that step (one per crossed multiple of *N*):
   1. **first** sound 22 (salvo2.wav) at the shooter, pitch 1 + 0.5*u* (one draw);
   2. then the record: kind 0, velocity **150** × unit(aim), position = shooter, radius
      0.5, strength 0.25·*s*, +0x58 = 0, special flag 1, scale 1, field-39 copy, damage =
      σ × ((a × W) × (*s*² × 5 + B)) with B = 1 (0.1 for action 55) (one draw);
   3. appended to the world list, then to the shoot output.

Draw order per shot: pitch draw, then σ.

### 2.5 Spells: actions 37–46 and 108 (RVA 0x217a21)

1. **Channel sound** while *t* ≤ P1 − 200: whenever ⌊(*t* + dt)/200⌋ ≠ ⌊*t*/200⌋, sound 36
   (channel2.wav) at the caster, pitch 1 + 0.25*u* (one draw).
2. **Fire time** F = P1. For 45 and 46: *n3* = ⌊P2/3⌋, *k* = ⌊(*t* + dt − P1)/*n3*⌋ (both
   toward zero), F = *n3*·*k* + P1, raised to P1 if below — a volley of up to three shots
   in P2.
3. When P1 is crossed and |aim| > 0: mana −= cost(action); mana := 0 if negative.
4. **Fire** when *t* < P1 + P2, F is crossed and |aim| > 0. With *c* = the charge **at that
   moment**:
   * velocity = unit(aim) × (**100** if the main-hand subtype byte obj +0xaa9 is 12, else
     **50**); age 0; radius 4·*c* + 0.5; strength *c*; scale *c* + 0.5; position = caster
     position, and for creature type 101 additionally + (box y → wu) in y and +
     (0.5 × box z → wu) in z; shooter id; field-39 copy.
   * Action 108 (thrown rock): kind **4**, velocity := 100 × unit(aim), damage =
     ((σ × (a × W)) × 1) × 5, scale 4, strength 1.
   * Otherwise kind **1**, damage = (σ × (a × M)) × 1; for 38–42 and 44 scale and strength
     stay as above; for 45 and 46 strength 0.25 and scale *c* + 1; for 37 and 43 strength 1
     and scale *c* + 1.5.
   * Element: shooter class 3 → 2 if specialization 1, else 1; other classes 0 (default).
   * Special flag := 1 for 37, 43, 45, 46, else 0 (also for 108).
5. Appended to the world list; then if *c* > 0: mana −= *c* (floor 0), charge := 0. If the
   flag is set and a buff of kind 9 is active, it is consumed (re-added with duration 0;
   buff record to outputs +0x58). Then appended to the shoot output.
6. Sound at the caster, volume field 0.9, no draw: element 2 → 41 (watersplash.wav), pitch
   1.25 for 41, 42, 44 else 1; otherwise 38 (fireball.wav), pitch 2 for 38, 39, 40 else 1.

Draw order: the channel-sound draw (in steps that play it), then σ when firing.

### 2.6 Stationary bursts (kind 3)

* **Actions 89, 90** (RVA 0x2151f9):
  * Channel sound for **every** creature in action 89/90 (before the ownership test, so
    on every machine): while *t* < P1, whenever ⌊(*t* + dt)/200⌋ ≠ ⌊*t*/200⌋, sound 36,
    pitch 1 + 0.25*u* (one draw).
  * Owned creature, when P1 is crossed: sound 38 at the caster, pitch 1 + 0.25*u* (one
    draw), and one record: kind 3, element 1 (89) or 2 (90), position = caster, velocity 0,
    damage = (a × M) × 2, radius 8, scale 1, strength 0, +0x58 0, flag 0, age 0. No
    field-39 copy (+0x08 keeps −1, −1, 0).
* **Healer strikes** (class 3 specialization 1), in the handler of actions 28, 94, 95
  (RVA 0x215c68), owned creature, when P1 is crossed:
  * at RVA 0x21608a one record: kind 3, element 2, damage 0.1 × M, radius 4, scale 1,
    strength 0, +0x58 0, flag 0, age 0, velocity 0, at the strike's reach point = attack
    origin (RVA 0x46bb0) + (reach × unit(aim) → wu), reach = the block ray cast
    RVA 0x1a35d0 from the origin along unit(aim) with limit 200. (The handler substitutes a
    reach of 10 for action 104, which the switch never routes to this handler: unreachable
    as read.)
  * at RVA 0x21681b, inside the handler's per-creature strike loop (starting RVA 0x216210),
    one more identical record at the attack origin for each creature that loop processes
    as struck.
  * Neither record has the field-39 copy.

---

## 3. Receiving shoots

The received-shoot list at G + 0x800650 (filled by the network receive path,
`alpha-net-packets.md`) is processed once per frame by the frame routine (RVA 0x90390):
every record whose shooter id is **not** the local player's id is appended unchanged to the
world projectile list (the local player's own shoots already exist locally). They are then
simulated exactly like local ones from their launch position; no latency compensation is
applied.

---

## 4. Per-step update (RVA 0x220a70–0x220d68), for each projectile *p* in list order

1. **Velocity**, by kind (*age* here is the value before this step's increment):
   * kind 1: unchanged (no gravity).
   * kind 2 (boomerang): look up the shooter; none → **remove**. Age > 1200 → **remove**
     (caught). 1100 < age ≤ 1200: *f* = f32(age − 1100) / 100, converted once to 16.16
     fixed point *F* = trunc(*f* × 65 536); *p*.pos := *p*.pos + ((shooter.pos − *p*.pos)
     scaled by *F* and again by *F*), each scaling being the 64-bit product divided by
     65 536 toward zero (RVA 0x2c900); velocity unchanged. 800 < age ≤ 1100: velocity :=
     10 × (shooter.pos − *p*.pos in blocks). Age ≤ 800: aim point A = shooter.pos + (aim
     vector → wu); target T = 5 × (A − *p*.pos in blocks); repeat dt times: velocity +=
     (T − velocity) × 0.05 (RVA 0x205950, once per millisecond).
   * every other kind (0, 3, 4): velocity.z −= (τ × 30) × 0.25 (gravity **7.5 blocks/s²**).
2. **Displacement** *d* = τ × velocity (component-wise, single).
3. age += dt.
4. **Timeout** (kind ≠ 2) when age > **5000**: kind 3 → remove. Others: if the shooter
   exists, *S*old := *S*; combo counter := 0; *S*new := *S*; action time := trunc((*S*old /
   *S*new) × f32(action time)) (ratio first, single precision); then remove (also when no
   shooter exists).
5. Otherwise run the **flight** of §5 (RVA 0x221465). Removed projectiles are erased after
   the whole list has been processed.

---

## 5. Flight, creature and block collision (RVA 0x221465–0x222b92)

Set up: the shooter is looked up by id (it may be absent); *n* = trunc(f32(|*d*| × 2) + 1)
sub-steps (|*d*| single precision); *ds* = *d* / f32(*n*); if *n* ≤ 0 the flight is skipped
and the projectile kept. **k** (knock-back) = velocity, normalised only if its length is
> 0, multiplied by 5·*c*; then k.z := 3·*c* (*c* = strength +0x54). So a still projectile
has **k** = (0, 0, 3·*c*).

For each sub-step *i* = 0..*n* − 1, the flags *destroy*, *landed* and *wall* start at 0
(all three are reset every sub-step):

### 5.1 Creatures

For every creature *C* in creature-list order with health > 0:

1. **Eligible**: if the projectile is kind 3 with element 2, or the shooter is absent, no
   further tests. Otherwise *C* ≠ shooter, RVA 0x1988a0(shooter, *C*) is true, and *C*'s id
   ≠ the shooter's obj +0x11c0 id. RVA 0x1988a0 ("can damage") is false for identical
   creatures; true if either controller kind is 6; true if exactly one of the two has
   controller kind 1, or both have kind 1 and the creature-type predicate RVA 0x44680
   differs between them; otherwise true only if either has mode flag 0x20 (obj +0x124).
2. **Overlap**, on each axis (all three "≥" tests first, then the "<" tests): *p*.pos +
   (*r* → wu) ≥ *C*.pos − (*b*/2 → wu), and *p*.pos − (*r* → wu) < *C*.pos + (*b*/2 → wu)
   (*b* = *C*'s full box extent on that axis, *r* = radius; the box is centred on the
   creature position on all three axes *as read*).
3. **Line of sight** RVA 0x19ee90 from *p*.pos to *C*'s position, arguments 1 and 200.
4. **Dodging** (*C* dodge time ≠ 0): if a shooter exists, *C*'s id is inserted into the
   shooter's +0x11b4 set; either way no hit; next creature.
5. **Hit gate**, by kind (age already advanced, so "changed this step" compares age − dt
   with age, integer division toward zero):
   * kinds 0, 1, 4 with flag 0: hit.
   * kind 3: hit only if ⌊age/1000⌋ changed this step (one tick per second).
   * kind 2: hit only if age > 300 and ⌊age/100⌋ changed this step.
   * kinds 0, 1, 4 **with** flag 1: no hit (they burst instead, §6.4).
   The gate is evaluated in every sub-step, so a burst or boomerang may hit the same
   creature once per sub-step of a qualifying step, *as read*.
6. **On a hit**: kinds other than 2 and 3 set *destroy* first; then §6.1–6.3; then kinds
   0, 1, 4 set *destroy* and stop scanning creatures. For a gate failure (step 5), kinds
   0, 1, 4 also set *destroy* and stop (a flagged projectile is spent on contact). Kinds
   2 and 3 continue with the next creature. Failing steps 2–3 simply moves on.

### 5.2 Movement and blocks (kinds other than 3)

Kind 3 skips this section and goes to the next sub-step (its position never changes; *n*
is computed as above from its gravity-accumulated velocity, so it is 1 only while
|*d*| < 0.5).

1. Kind 2: whenever ⌊age/200⌋ changed this step, sound 15 (swing.wav), pitch
   1 + 0.25*u* (one draw), at *p* (each sub-step, *as read*).
2. *p*.pos += *ds* → wu.
3. **Solid block** at *p*.pos (block lookup RVA 0x2f860 on world units; solid = the low
   five bits of the block's byte 3 are neither 0 nor 2, RVA 0x3b480): *p*.pos −= *ds* →
   wu; *wall* := 1. Kind 0 also emits a **particle burst** (record RVA 0x65ed0, frame
   outputs +0x10) at *p*.pos: count 4, colour = the block's bytes 0–2 / 255 with alpha 1,
   velocity (0, 0, 10), size 0.1.
4. Otherwise a **ray cast** RVA 0x1a35d0 from *p*.pos along unit(velocity) with limit
   *L* = |velocity| × τ: if *L* > the returned distance, *p*.pos −= *ds* → wu and
   *wall* := 1.
5. If *wall*, not *destroy*, and the shooter exists: the shooter's combo is reset with the
   time rescale of §4 step 4 (a miss into terrain ends the chain).
6. Kind 2: if *landed* and the shooter exists, the shooter's combo += 1 and its combo
   timer := 0. If *wall*: velocity := 0 (the return of §4 brings it back). Next sub-step.
7. Other kinds: neither *destroy* nor *wall* → next sub-step; otherwise **end of flight**
   (§6.4) and stop.

After the last sub-step the projectile is kept.

---

## 6. Hits

### 6.1 Mana, critical and ranger proc (in this order, on every hit of §5.1)

* **Mana on hit** (kinds 0 and 2, flag 0, shooter exists): *g* = (1 − (2 × draw)/32767) ×
  0.05 + 0.1, × 0.2 for kind 2, then × (f32(shooter's current action total) / 300), then ×
  2 if a second draw mod 8 = 0. **Both draws happen whenever these conditions hold**; only
  the mana addition is then limited to the owner: online, only if the shooter is the local
  player; offline, any shooter. mana := mana + *g*, set to 1 if above 1.
* **Critical**: if the shooter exists, one critical test RVA 0x444a0 of the shooter
  (draws as described in the behaviour spec); otherwise not critical. Damage *x* = +0x48,
  × 2 if critical.
* **Ranger proc**: if the shooter exists and is the local player, or (offline and the
  shooter's controller kind ≠ 0): one chance test RVA 0x441a0(shooter, 0.15) (one draw;
  the chance is scaled by the shooter's main-hand weapon as that routine defines). If it
  succeeds, flag 0 and the shooter is class 2 specialization 1: buff kind 10 for 30 000 ms
  (RVA 0x46af0; buff record to outputs +0x58) and sound 47 (upgrade2.wav) at the shooter.
  A kind-10 buff fills the charge (§2.1) and is consumed by the next flagged bow shot or
  the next salvo.
* Then *x* := 0 if negative.

### 6.2 Damage (element ≠ 2)

The hit routine RVA 0x196d30 (`alpha-creatures-behaviour.md`, hit resolution) is called
with, in order: target *C*; the shooter (or none); *x*; the critical result; the special
flag; the strength; *p*.pos; **k**; the frame outputs; a step-local list; 1 if kind = 1
else 0; 0; the projectile record; 1. Parameter meanings are defined by the behaviour spec.
A true result sets *landed*.

After either §6.2 or §6.3 (including a heal that was not applied): *C*'s field 18 :=
+0x6c if +0x6c is larger.

The projectile loop has no ownership gate: every client simulates every projectile and
calls the hit routine. How the resulting hit records are attributed and transmitted is
specified in `alpha-net-packets.md` §5.1 (not re-verified here).

### 6.3 Healing (element 2)

If the shooter exists, RVA 0x196ca0(shooter, *C*) ("hostile") is false and *C*'s
controller kind ≠ 6: a 72-byte hit record (constructor RVA 0x59a30: attacker id = the
record's shooter id, target id = *C*'s id, amount at +0x10 = −*x*, × 0.5 if the shooter's
controller kind is 1, critical flag at +0x14, position at +0x20 = **_C_'s position**) is
appended to the frame outputs; then, offline, or online when both the shooter and *C* are
the local player, it is applied at once (RVA 0x195a60). RVA 0x196ca0 is false for a
controller-5 shooter against a controller-0 target and whenever either controller kind is
6; otherwise it follows the controller-1, predicate and mode-flag rules of "can damage"
(§5.1), without the identity test. Hostile targets take nothing from water.

### 6.4 End of flight (RVA 0x222457)

* Kinds 0, 2, 4 with flag 0: no burst.
* Kind 1, or any flagged projectile:
  1. If it did not end on *destroy*, an impact sound at *p*.pos, pitch 1 + 0.4*u* (one
     draw): kind 1 → 42 (watersplash-hit.wav) for element 2, 39 (fire-hit.wav)
     otherwise; other kinds → 20 (blade1.wav).
  2. **Burst** over every creature in list order (the flag is saved first). If a shooter
     exists, the machine is offline, the shooter's controller kind is 0 and it is not the
     local player, the whole burst is skipped (tested at the first creature). For each
     creature *C*: if a shooter exists, *C* ≠ shooter, "can damage", not the +0x11c0 id
     and health > 0 are required (with no shooter none of these, not even health, is
     tested); overlap as §5.1 step 2 with a fixed half-size of **5 blocks** in place of *r*
     (a 10-block cube); line of sight as §5.1 step 3; a dodging *C* is skipped (and
     inserted into the shooter's set) only if a shooter exists — with no shooter it is hit.
     Per creature hit: critical test of the shooter if one exists; damage = +0x48, × 2 if
     critical (no clamp at 0); then, if the shooter exists and is the local player or
     (offline and its controller kind ≠ 0), a chance test RVA 0x441a0(shooter, 0.25) (one
     draw), and if it succeeds with saved flag 0, kind 1 and element 1: buff kind 9 (free
     big spell) for 30 000 ms, buff record, sound 47 at the shooter. Then the hit routine
     of §6.2 with the same argument list (saved flag in the flag position). A true result
     sets *landed*.
  *As read*, an unflagged spell therefore hits the creature it touched twice (direct hit,
  then burst).
* If *landed* (of this sub-step) and the shooter exists: shooter combo += 1, combo timer
  := 0.
* **Water spells** (kind 1, element 2) are not removed: they become a **kind-3 puddle** at
  the impact point, keeping their velocity: age := 0 if flagged else 3000 (so 5 s or 2 s of
  life), damage := damage × 0.05, radius := strength × 5 + 5, strength := 0, flag := 0.
  Every other projectile is removed.

(A "sound 19, arrow-destroy.wav, then remove" branch at RVA 0x2223ec can only be reached
for a non-boomerang from a path that requires kind 2: unreachable *as read*; arrows end
silently.)

---

## 7. Drawing (renderer RVA 0xac260)

* **Visibility** (model pass RVA 0xb8151): RVA 0x7f760 with *p*.pos, the radius and a view
  value — skipped when it fails. Kinds above 4 draw nothing.
* **Model** by kind: 0 → model **840** (`arrow`); 2 → the shooter's main-hand item model
  (RVA 0xec400) when the shooter exists and its main-hand item has type 3 and subtype 8,
  else 840; 4 → model **2101** (`iron-deposit`, used as a rock); 1 and 3 → no model.
* **Brightness**: for kinds other than 1, the world light value of *p*'s block
  (RVA 0x718b0) / 255 becomes the alpha term of the material (1, 1, 1, light).
* **Orientation**: kind 2: tilt min(0.5 × age, 90)° (RVA 0x244f0), then spin (−age) ×
  1.5° (RVA 0x243d0); kind 4: rotate 0.5 × age° about the cross product of velocity and
  (0, 0, 1); others with non-zero velocity: look along unit(velocity) with up (0, 1, 0)
  (RVA 0xc12f0). Scale = +0x50/14 on all axes; the model is centred (−½ of its size).
* **Trails** (ribbon routine RVA 0x71d50, internals not traced): kind 0 with strength ≠
  0: 20 segments, colours (0.9, 0.9, 1, 1), (0.75, 0.75, 1, 1), (0.5, 0.5, 1, 1), widths
  from 0.5 × strength and strength, along unit(velocity), keyed by age. Kind 1: 20
  segments, colours fire (1, 0, 0, 1), (1, 0.25, 0, 1), (1, 0.5, 0, 1) or, when the shooter
  exists with specialization 1 (any class), water (0, 0, 1, 1), (0, 0.25, 1, 1),
  (0.25, 0.5, 1, 1); size from 5 × scale.
* **Light** (RVA 0xaf380): each kind-1 projectile adds a point light at *p*.pos, colour
  (0.5, 0.5, 0.2) × radius, light radius **16**, into the frame light list of
  `alpha-rendering.md` §10 (which does not list this source yet).
* A third pass over the list (RVA 0xb8cd0) handles only kind-3 records; its content was not
  traced.

---

## 8. Test vectors (single precision, dt = 16)

τ = 0.016, gravity per step 0.120000005.

**Arrow**, action 22 uncharged, aim (1, 0, 0), launched at (0, 0, 0) wu, nothing in the way
(*n* = 4 sub-steps, each adds trunc(0.4 × 65536) = 26214 in x):

| Step | velocity.z | position (wu) | blocks |
|---|---|---|---|
| 1 | −0.12 | (104856, 0, −124) | (1.59998, 0, −0.00189) |
| 2 | −0.24 | (209712, 0, −372) | (3.19995, 0, −0.00568) |
| 3 | −0.36 | (314568, 0, −748) | (4.79993, 0, −0.01141) |

It times out after 313 steps (age 5008 > 5000) if it hits nothing.

**Spell** kind 1, velocity 50: *n* = 2, 52428 wu (0.79999 blocks) per step, no drop.

**Damage**: level 1 gives *g* = 0, so with damage multiplier 1 the base is 1; with a bow of
item damage 8 (value from the behaviour spec, not re-verified here) W = 9. Action 22
uncharged (cost 0, strength 0, B = 1): 9 × 1 × σ = **11.25** (draw 0) … **6.75** (draw
32767); critical × 2. Charged with *s* = 0.5: 9 × (5 × 0.25 + 0.1) × σ = 12.15 × σ,
flagged → a burst on impact, knock-back (2.5·ux, 2.5·uy, 1.5). Level-1 mage with M = 9,
action 38: 9 × σ, radius 0.5.

**Mana on hit**: for an action total of 550 ms, *g* ranges from 0.05 × 550/300 = 0.0917 to
0.15 × 550/300 = 0.275, doubled one time in eight.

---

## 9. Open points

* Ribbon routine RVA 0x71d50 parameters in detail, and the content of the kind-3 pass
  RVA 0xb8cd0.
* The meaning of the obj +0x11c0 id (perhaps the pet; `alpha-creatures-behaviour.md` places
  a pet id at obj +0x11c8). A reader of the +0x11b4 "evaded by" set exists in the world
  step at RVA 0x2185a1 (it runs when the creature's action is 0); its effect is not traced.
* Whether the double hit of unflagged spells (§6.4) and the per-sub-step repeat of
  boomerang/burst hits (§5.1 step 5), which the code shows, are observable in the running
  game remains unconfirmed.
* What starts actions 89 and 90; who could set shoot +0x6c in Alpha (no creator does).
* Which creatures the strike loop at RVA 0x216210 treats as struck (second healer-burst
  site); not re-traced.
* The names "pitch" and "volume" for sound-record fields +0x10 and +0x14 follow the
  companion specs (both default to 1.0, RVA 0x65fb0); the "event-40" and the hit-routine
  parameter names likewise.
* Whether shoot-record bytes +0x14..+0x17 are transmitted as-is.

---

## Re-certification (2026-11-14)

Re-checked against `Cube.exe` (SHA-256 confirmed `84a7a132…30bf`, image base 0x400000)
using MSVC dumpbin disassembly and direct reads of constants and jump tables from the image
bytes. The Infinite-Cubes reconstruction was not consulted.

**Confirmed**: shoot-record layout and constructor defaults; kinds, elements and the
gravity, timeout, boomerang phases and approach routine; sub-step count and displacement;
the hit gates; mana-on-hit formula; critical doubling and the negative clamp of direct hits;
ranger and fire procs (kinds, durations, sound 47); water heal halving and application
rule; water puddle conversion; impact sounds; unreachable sound-19 branch; charge rates,
combo and warfrenzy factors and caps; the a(id) table; W and M level formula and per-action
factors; spread; bow, salvo, spell and burst record contents, speeds, radii, scales,
strengths, damages and sounds; received-shoot filtering; packet-9 output filtering; all
drawing constants (models 840/2101, scale /14, tilt/spin, trail colours, light colour and
radius 16); test-vector arithmetic.

**Corrected** (old → new):
1. Addresses were virtual addresses labelled RVAs → true RVAs (VA − 0x400000) throughout.
2. "Creation code and loop are outside the Ghidra listing" → the decompilations of the
   routines timed out; disassembly was used.
3. Radius "also trail size" → radius scales the light and feeds the view test; trails use
   strength (kind 0) or scale (kind 1).
4. +0x14 "stack garbage on the wire" → not written; transmission moved to open points.
   +0x08 "element 2 is the spawn index" removed; kind-3 creators do not write +0x08.
5. Charge phase test "*t* ≥ P1 (pre-advance)" → the advanced action time ≥ P1.
6. Charge sound "while charging" → actions 24, 25, 27, 36, 59, 63, 64 only (not 8), owned,
   no phase test.
7. W "Σ weapons in slots 6 and 7" → the action-dependent weapon set of RVA 0x3df60.
8. Salvo: sound listed after the record → the pitch draw precedes the spread draw
   (**random-draw order**); mana is not floored; "four at most" → one per crossed multiple
   of ⌊P2/4⌋.
9. Actions 89/90 channel sound "as spells (*t* ≤ P1 − 200)" → *t* < P1, and it runs for
   every creature regardless of ownership (**random-draw order**).
10. Healer bursts "any melee-group action, reach 10 for 104" → handler of actions 28, 94,
    95; ray limit 200; the 104 branch is unreachable as read.
11. Timeout/terrain time rescale "⌊time·Sold/Snew⌋" → ratio computed first, then × time.
12. Still projectile "**k** = 0" → **k** = (0, 0, 3·*c*).
13. **Reversal**: "*landed* kept across sub-steps" → reset every sub-step.
14. **Reversal**: "the shooter must exist" for eligibility → with no shooter the tests are
    skipped and the creature is eligible.
15. Overlap "≤" on the upper side → strict "<"; half-sizes converted to wu separately.
16. Line of sight target "*C*" → *C*'s position.
17. Kind 3 "has *n* = 1" → *n* computed normally from its gravity-grown velocity.
18. Mana on hit "owned shooter" → both draws occur whenever kind/flag/shooter hold; only the
    addition is ownership-limited (**random-draw order**).
19. Ranger proc ownership → local player, or offline with controller ≠ 0; the chance draw
    precedes the flag/class tests (**random-draw order**).
20. Hit-routine arguments → an extra step-local list argument added; parameter names
    deferred to the behaviour spec.
21. Heal record position "*p*.pos" → the target's position.
22. Burst "5-block cube" → half-size 5 (10 blocks wide); health and dodge exemptions apply
    only with a shooter; no negative clamp; skip rule stated as controller kind 0;
    per-creature draw order critical → fire-proc chance (**random-draw order**).
23. Visibility test → takes a third (view) value; third pass → handles only kind 3.

**Removed**: the suggestion to check behaviour with a recording; the network attribution
claim of §6.2 (deferred to `alpha-net-packets.md`); the "to reconcile" note on the pet id
(now an open point); "harmless animal", "bracelet" and "record field 30" labels.

**Unresolved**: listed in §9.
