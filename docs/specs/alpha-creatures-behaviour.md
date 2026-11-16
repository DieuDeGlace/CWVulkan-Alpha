# Alpha creatures: actions, combat, stats, AI and progression

Status: FINAL / STABLE / HELD

Behavioural contract for the creature simulation of the Cube World **Alpha** client
image `Cube.exe` (the image in the evidence root; addresses below are virtual addresses
in that image, image base 0x400000). It covers action timing and chaining, the primary
and secondary attack choice, derived stats, item stat formulas, hit resolution, the
damage-application side effects that the AI depends on (threat, kills, missions), the
NPC equipment generator, the AI behaviour layer, the sound-id table and the player's
ability bar values. Facts not re-confirmed against the original image are collected in
§U and must not be treated as specified.

Related contracts: `alpha-net-packets.md` (record layouts on the wire),
`alpha-creatures-models.md` (types, action → pose), `alpha-player-control.md` (input,
movement step), `alpha-projectiles.md` (shots), `alpha-worldgen-overview.md` and
`alpha-worldgen-terrain.md` (where spawn objects are created).

---

## 0. Conventions

* **Draw**: one call of the C runtime pseudo-random generator, returning 0..32767.
  *r* = draw / 32767 in single precision. "Draw parity" is draw mod 2; "draw mod *n*" is
  the non-negative remainder. Every draw named in this document is consumed in the order
  given; a draw that is described as "consumed" is taken even when its value is unused.
* **Single precision**: every arithmetic step stated is performed in 32-bit floating
  point, left to right as written, unless stated otherwise. "2^*x*" is a double-precision
  power of 2 whose argument is the single-precision *x* widened, rounded back to single.
  "Truncate" converts toward zero.
* **Object offset** ("obj +*x*") is an offset inside the in-memory creature object;
  **record offset** = object offset − 0x10. Record fields are numbered as in
  `alpha-net-packets.md`. Positions are 3 × signed 64-bit in world units of 1/65536
  block; "blocks" means world units × 1/65536 (single precision).
* *S* = attack-speed factor (§4.3); *a* = attack-speed multiplier (record 0x16C, the
  second element of the stat-multiplier array). A scaled duration "*c*" means
  truncate(*c* / (*S* × *a*)), with *c*, *S*, *a* single precision; "fixed *c*" is the
  constant itself.
* **Main hand** = equipment slot 7 (item type byte obj +0xAA8, subtype obj +0xAA9);
  **off hand** = slot 6 (obj +0x990, +0x991). Weapon subtypes: 0 sword, 1 axe, 2 mace,
  3 dagger, 4 fist, 5 longsword, 6 bow, 7 crossbow, 8 boomerang, 10 staff, 11 wand,
  12 bracelet, 13 shield, 15 greatsword, 16 greataxe, 17 greatmace, 18 pitchfork.
* **Two-handed set W2** = main hand of item type 3 with subtype 15, 16, 17, 5, 10, 11,
  18, 8, 6 or 7 (routine 0x444230).

---

## 1. Creature fields used here

Record offsets unless marked "obj".

| Offset | Meaning |
|---|---|
| 0x050 (u8) | controller kind: 0 player; 1 hostile NPC; 3, 5 (pet), 6 (inert object such as a deposit) also occur. A newly initialised record holds 3 (0x43C100). |
| 0x054 (i32) | creature type |
| 0x058 (u8), 0x05C (i32) | action id, action time (ms) |
| 0x060 (i32) | combo counter |
| 0x06E (u16) | appearance flag word (tested bits: 0x02, 0x08, 0x10 natural weapon, 0x20, 0x40, 0x200 boss, 0x800, 0x1000, 0x2000, 0x1A00 mask) |
| 0x070 (f32) | own collision box x (used for ranges) ; 0x078 box z |
| 0x114 (u16) | mode flags (0x01 climbing, 0x04 facing/looking, 0x10 gliding, 0x20, 0x40 running, 0x80, 0x200 lantern, 0x400 sitting/aiming) |
| 0x118 (i32) | dodge time |
| 0x11C (i32) | hit-stun counter; −3000 in a new record |
| 0x120 (i32) | action-lock countdown (§3.4) |
| 0x124, 0x128 (i32) | slow and haste countdowns (§5 step 9); each −= dt per step, floored at 0 |
| 0x12C (f32) | a value that, when negative, rises toward 0 by dt/180 per step and is clamped at 0 |
| 0x130, 0x131 (u8) | class (1 warrior, 2 ranger, 3 mage, 4 rogue; 0x80–0x89 non-combat), specialization |
| 0x134 (f32) | charge |
| 0x150 (3 × f32) | look / aim vector in blocks |
| 0x15C (f32) | health (500 in a new record) |
| 0x160 (f32) | mana, 0..1 |
| 0x164 (f32) | block gauge |
| 0x168 (5 × f32) | stat multipliers: health, attack speed, damage, armour, resistance (100, 1, 1, 1, 1 in a new record) |
| 0x180, 0x184 (i32) | level (1 in a new record), experience |
| 0x188 (u64) | owner id of a pet |
| 0x198 (u8) | power tier *t* |
| 0x1A0, 0x1A4, 0x1A8 (i32) | home zone x, y and the index of the creature's spawn object in that zone |
| 0x1B0 (3 × i64) | home position |
| 0x2F0 | equipment, 13 items of 0x118 bytes (slot *k* at record 0x2F0 + 0x118·*k*) |
| 0x1128 + 4·*i* | skill level *i* |
| obj +0x1178 | buff list; entry = kind (u8), value (f32 at +4), duration ms (i32 at +8), source id (u64 at +0x10) |
| obj +0x1184 | hit flash |
| obj +0x1190 | stealth amount *s*, 0..1 |
| obj +0x1194 | stamina, 0..1 |
| obj +0x11C8 | the creature's pet id (u64) |
| obj +0x11D0 | id of the last creature exchanging a hit with this one |
| obj +0x139C | cooldown table: ability id → remaining ms |
| obj +0x13A4 | **threat map**: creature id → threat (f32); creatures that hurt this one |
| obj +0x13AC | **hit map**: creature id → value (f32); creatures this one has hit |
| obj +0x13B4 | action power (f32, §3.5); consumers not traced |
| obj +0x13B8 | flag that suppresses the combo reset of §3.4 (meaning not established) |
| obj +0x13C4 | AI time accumulator |
| obj +0x13E4 | behaviour object |
| obj +0x13E8 | special-action id list |
| obj +0x1460 | path point list (count at +0x1464) |

---

## 2. Spawn objects

### 2.1 Defaults (constructor 0x5ADB20)

A spawn object is 4336 bytes; the image names its class `cube::Spawn`. A new one holds:
activation radius +0x08 = 200.0; controller kind +0x28 = 1; creature type +0x2C = 4;
class and specialization +0x30, +0x31 = 0; level +0x34 = 1; +0x38 through +0x4C = 0
(respawn countdown, death day, time window, creature id); +0x50 = 0; yaw +0x54 = 0;
tier +0x58 = 0; +0x5C = 0; +0x60 = −1; +0x64 = −1; +0x68 = 0; +0x6C = −1; +0x70 = 0;
default appearance and equipment; stat multipliers +0xF58 = 100 and +0xF5C..+0xF68 =
1, 1, 1, 1.

### 2.2 Fields

| Off | Type | Meaning |
|---|---|---|
| +0x08 | f32 | activation radius, blocks |
| +0x10 | 3 × i64 | position |
| +0x28 | i32 | controller kind of the creature (6 for deposits) |
| +0x2C | i32 | creature type |
| +0x30, +0x31 | u8 | class, specialization |
| +0x34 | i32 | level |
| +0x38 | i32 | respawn countdown, ms |
| +0x3C | i32 | day number at death |
| +0x58 | u8 | power tier |
| +0x74 | | appearance; its flag word is at +0x7A |
| +0x120 + 0x118·*k* | item | equipment slot *k* |
| +0xF58..+0xF68 | 5 × f32 | stat multipliers |
| +0x10AC | list of i32 | special-action ids |
| +0x10B8 | i32 | a value written by the equipment generator (§9) |
| +0x10BC | list of i32 | a second action-id list (consumer not traced) |

### 2.3 Death of a spawned creature (0x595A60)

When a creature's health reaches 0 on the simulating side, the spawn object found from
the creature's home zone (record 0x1A0, 0x1A4, each 0..65535) and spawn index (record
0x1A8, if in range) gets respawn countdown := 1 200 000 ms and death day := the world's
current day number.

---

## 3. Actions

An action is (id, time). A creature's attack inputs and the AI start actions by setting
the id and time 0.

### 3.1 Primary attack (0x445020)

"Chain *x* → *y*": if the current action is *x* and its time < total duration (§3.3) +
200, the result is *y*; otherwise *x*. Rules in order; the first that applies wins.

1. **Magic user**: class 3, or type 117 or 86, or main hand of type 3 with subtype 10, 11
   or 12. Then: main subtype 11 → 38 (44 for specialization 1); main subtype 10 → 30 (32
   for specialization 1); otherwise specialization ≠ 1 → chain 40 → 39, specialization 1
   → chain 42 → 41.
2. Type 104, or main hand of type 3 with subtype 6 or 7, or off hand of type 3 with
   subtype 6 or 7 → 22.
3. Main subtype 3 (item type not tested) → chain 19 → 18.
4. Main subtype 4, or no main-hand item (type 0) → chain 6 → 7.
5. Main subtype 5 → chain 14 → 13.
6. Main hand type 3 with subtype 8 → 26.
7. Main hand in W2 → chain 57 → 67, then 67 → 58 (giving 57, 67, 58, 57 …).
8. Off hand type 3 with subtype 13 → chain 10 → 9.
9. Appearance flag 0x10 → 75.
10. Otherwise → chain 1 → 2.

### 3.2 Secondary attack (0x4452A0)

1. Main hand type 3 subtype 6 → 23.
2. Magic user (as §3.1 rule 1): main subtype 11 → 46 (45 for specialization 1); main
   subtype 10 → 31 (33 for specialization 1); otherwise 37 (43 for specialization 1).
3. Main hand type 3 subtype 5 → 5.
4. Main hand in W2 → 65 if the combo counter is odd, else 66.
5. Otherwise → 4 if the combo counter is odd, else 3.

### 3.3 Durations

Total duration = P1 + P2 + P3 of the current action (0x43D1A0 sums three inline copies
of the phase tables). P1 = wind-up (0x43CAA0), P2 = active part (0x447310), P3 = recovery
(0x444270). Each entry is scaled unless marked fixed. Ids not listed use the default row.

| Action ids | P1 | P2 | P3 |
|---|---|---|---|
| default | 400 | 300 | 500 |
| 0 | 0 | 0 | fixed 100 |
| 1, 9 | 300 | 200 | 500 |
| 2 | 100 | 200 | 500 |
| 3, 4 | 100 | 100 | 300 |
| 5 | 100 | 400 | 300 |
| 6 | 50 | 150 | 500 |
| 7 | 50 | 150 | 200 |
| 8, 28, 98 | 0 | 300 | 500 |
| 10 | 100 | fixed 200 | fixed 600 |
| 11, 60, 61, 104 | 0 | 300 | 100 |
| 12, 16 | 200 | 400 | 500 |
| 13 | 200 | 200 | 500 |
| 14 | 100 | 200 | 200 |
| 15 | 200 | 200 | 400 |
| 17 | 100 | 400 | 500 |
| 18 | 100 | 150 | 200 |
| 19 | 100 | 150 | 500 |
| 20, 21 | 100 | 400 | 500 |
| 22 | 400 | 50 | 100 |
| 23 | 50 | 50 | 10 |
| 24, 25, 27, 36, 59, 63, 64 | 50 | 300 | 500 |
| 26 | 300 | 1200 | 100 |
| 30, 32 | 800 | fixed 600 | 100 |
| 31, 33 | 1600 | fixed 1200 | 100 |
| 34 | 1600 | 1200 | 100 |
| 37, 43 | 1200, or 600 when the main-hand subtype byte is 12 | 100 | 100 |
| 38, 44 | 500 | 300 | 100 |
| 39–42 | 200 | 300 | 100 |
| 45, 46 | 1200 | 500 | 100 |
| 48 | fixed 100 | 300 | fixed 0 |
| 49 | 400 | fixed 0 | 500 |
| 50, 96 | 0 | fixed 500 | fixed 100 |
| 54 | fixed 400 | fixed 100 | fixed 400 |
| 55 | 0 | 500 | 500 |
| 57, 58 | 800 | 300 | 300 |
| 62 | 0 | 100 | 300 |
| 65, 66 | 300 | 400 | 200 |
| 67 | 200 | 200 | 100 |
| 68, 69 | 300 | fixed 1000 | 800 |
| 70, 74, 75 | 300 | 300 | 500 |
| 71 | fixed 200 | fixed 3000 | fixed 200 |
| 72 | fixed 200 | fixed 5000 | fixed 200 |
| 73 | 300 | fixed 600 | 500 |
| 76, 77, 78 | 300 | fixed 500 | 500 |
| 86 | 400 | fixed 5000 | 500 |
| 87 | 5000 | 300 | 500 |
| 89 | as 37 | 300 | 500 |
| 91 | 1000 | fixed 6000 | 500 |
| 93 | 800 | fixed 600 | 800 |
| 94 | 500 | 500 | 100 |
| 95 | 1000 | fixed 2000 | 100 |
| 101 | fixed 100 | 300 | 500 |
| 105 | fixed 5000 | 300 | 500 |

### 3.4 Chaining, combo reset and action lock

**Chaining (0x445410)**, used by the attack input and by the AI. It refuses (returns
"busy") when the dodge time ≠ 0 or the action is 48. Otherwise *D* := total duration;
if the combo counter ≠ 0 and the action is neither 30 nor 32, *D* −= P3 of the current
action. If action time ≥ *D*: action := primary attack (§3.1), time := 0. It then reports
success whether or not an action was started.

**Combo reset (world step 0x613448–0x6134EE)**: when obj +0x13B8 is clear, time + dt ≥
P1 + P2, time < total duration and the dodge time is 0: *S*old := *S*; combo := 0;
*S*new := *S*; time := truncate(*S*old / *S*new × time).

**Action lock (0x611D0D)**: record 0x120 −= dt each step; while it is still positive,
action time := total duration + 1; it is floored at 0.

**Hit-stun (0x6184C8)**: record 0x11C −= dt each step; while it is still positive,
action := 0 and time := 0.

### 3.5 Action start and time advance

In the world step, for every creature:

1. **Start costs (0x613510–0x61361B)**, when the action time is 0 and the action is
   neither 31 nor 33: obj +0x13C0 := 0, action power := 0; *k* := cost(action) (§14.4).
   * *k* > 0: action power := *k*; mana −= *k*.
   * *k* ≤ 0 and the action is 5, 17 or 20: action power := mana; mana := 0; in addition
     a guaranteed-critical buff interaction runs (§U).
   * *k* ≤ 0 and the action is 21, 54, 87 or 88: action power := 1.
   Then, if charge > 0: action power := charge, mana −= charge, obj +0x13B8 := 0, and for
   actions 5 and 11 action power := 0.5 × action power. Charge := 0; mana is floored at 0.
2. **Deferred cost of 31 and 33 (0x613754)**: while time ≤ P1, on the step where time +
   dt > P1: action power := 0.1 and mana −= cost(action), floored at 0.
3. **Cooldown recording (0x6184F2–0x618533)**, every creature except controller kind 6,
   after the hit-stun rule: if the action time is exactly 0, cooldown[action] :=
   length(action) (§14.5).
4. **Time advance (0x618541)**: time += dt.
5. **Smash descent (0x618544–0x618591)**: while the action is 54 with P1 < time < P1 +
   P2 and time < total, vertical velocity := −60.

---

## 4. Derived stats

*L* = level, *t* = tier, *g* = 3 × (1 − 1/((*L* − 1) × 0.05 + 1)), *P* = 2^*g*.
NPC base *B* = *P* × 2^(0.25 × *t*); player base (controller kind 0) = 2^1 × *P*.

### 4.1 Maximum health (0x444DB0)

base × health multiplier; then × 1.3 for class 1, × 1.1 for class 2, × 1.2 for class 4
(class 3 unchanged); then × 1.25 for class 1 specialization 1; then + item health
(§6.3) of off hand and main hand when their type is 3, chest (slot 2) of type 4, boots
(slot 3) of type 6, gloves (slot 4) of type 5, shoulders (slot 5) of type 7, in that
order.

Example: level-1 warrior player, multiplier 100: 2 × 1 × 100 × 1.3 = 260, plus item
health.

### 4.2 Damage, armour, resistance

* **Damage (0x43E190)**: *d* := *P* × damage multiplier (no tier term and no player
  factor). If the off hand is type 3: off-hand subtype 13 → *d* += 4 × damage(main-hand
  item) and nothing else is added; otherwise *d* += damage(off hand), then *d* +=
  damage(main hand) if the main hand is type 3. If the off hand is not type 3, *d* +=
  damage(main hand) when it is type 3. Result: damage multiplier × *d*.
* **Armour (0x43CFF0)**: base × armour multiplier; if appearance flag 0x20, + *B* (the
  NPC formula, unmultiplied, also for players); + item armour of chest, boots, gloves,
  shoulders (type-checked as in §4.1).
* **Resistance (0x4467A0)**: identical with the resistance multiplier and item
  resistance.

### 4.3 Attack-speed factor *S* (0x447700)

*S* := 1 for controller kind 0, else 0.75 + *t* × 0.0625. Then + 0.5 × min(combo /
comboMax, 1) for class 1 specialization 0; + min(combo / comboMax, 1) for class 3
specialization 1; + the speed sum (0x4478B0): 0.1 × (*P* / 8) plus item speed (§6.3) of
off hand and main hand (type 3), chest, boots, gloves, shoulders, amulet (slot 1, type
8) and rings (slots 8 and 9, type 9). Then, if the buff list holds any kind-12 entry:
*S* × ((1 − 1/(0.1 × *n* + 1)) + 1), *n* = skill 8 level, or for a non-player with skill
8 at 0: truncate(*L* / 2) + *t* + 1.

### 4.4 Combo maximum (0x444CB0)

comboMax = truncate(*a* × *m*): main hand type 3 with subtype 6 → 60; subtype 8 → 80;
subtype 11, 12, 15, 16, 17 → 20; any other main hand of type 3 → 50; main hand not type
3 → 20 with appearance flag 0x10, else 50.

### 4.5 Critical hit (0x4444A0) and weighted chance (0x4441A0)

* Critical: *p* := 1 if the buff list holds a kind-11 entry, else (0.1 × *P* / 8 + item
  crit sum over the same slots as §4.3, 0x43EA40) + 0.15 × *s*. One draw is always
  consumed; critical when *r* < *p* (so even with a kind-11 buff a draw of 32767 is not
  critical).
* Weighted chance with probability *p*: main hand type 3 subtype 0, 1, 2 or 12 → *p* ×
  0.5; subtype 8 or 10 → *p* × 0.3; one draw, true when *r* < *p*.

### 4.6 Experience and levels

* cost(*L*) = truncate((1 − 1/((*L* − 1) × 0.05 + 1)) × 1000 + 50) (0x444D60): cost(1) =
  50, cost(2) = 97, cost(10) = 360, cost(100) = 881.
* Level-up (0x447B00): only if *L* > 0; while experience ≥ cost(*L*): experience −=
  cost(*L*), *L* += 1, health := maximum health.
* Health is clamped to the current maximum every step (0x611A9C).

---

## 5. Hit resolution (0x596D30)

Inputs: target, attacker (may be absent), amount *x*, critical flag, a second flag *A*
(gating spirit effects, step 9), a stun chance *σ*, knock-back vector **k**, magic flag,
attacker action id α, "blockable" flag, and the output lists. ρ := attacker combo /
comboMax (unclamped); *c* := 1 − min(ρ, 1).

1. Stop if target health ≤ 0, or the world's online byte (+0xB4) is set and the target is
   the creature referenced by world +0xB8.
2. **No attacker**: one draw consumed; hit flash := 0.5; continue at step 12 with *x*.
3. Attacker's current action 91 or 74 and target contact bit 0x01 clear → stop. Attacker's
   current action 28 → a particle burst at the target whose size uses one draw.
4. **Immune**: target controller kind 1 with mode flag 0x80 or contact flag 0x20: one
   draw (sound 24, pitch 1 + 0.3*r*, volume 1); a record of kind 3 (amount *x*, knock-back
   **k**) is queued; offline it is applied at once (§7); stop.
5. **Block**: blockable, target blocking (§5.12) and block gauge > 0: *q* := *x* /
   damage(target); mana := mana + gauge, capped at 1; charge := charge + gauge, capped at
   the new mana; a kind-1 record (amount *x*, knock-back 0.5 **k**) is queued; gauge :=
   gauge − *q*, floored at −1 (values below −1 become −1); one draw (sound 24, pitch 1 +
   0.3*r*); offline the record is applied at once; stop.
6. **Absorb**: *σ*6 := sum of the target's kind-6 buff values. If *c* × *σ*6 > 0: a
   kind-5 record (amount *x*, knock-back 0.5 **k**) is queued; one draw (sound 91 at the
   target, pitch 1 + 0.3*r*, volume 1); offline applied at once; *x* := *x* − *c* × *σ*6;
   stop if *x* ≤ 0.
7. **Armour**: *x* := max(0, *x* − *c* × (magic ? resistance : armour)(target)).
8. **Spirit bursts**: if ρ > 0.25: direction = the horizontal components of **k** divided
   by the 3-D length of **k** (unchanged if that length is 0), times 4, with vertical 5;
   for each item in the attacker's action weapon list (§5.11), spirit bursts (§5.10).
9. **Spirit effects**: if *A* is set and ρ > 0 and the weapon list is non-empty: *w* :=
   0.1 / (list length). For each listed item and each attached spirit *i* (count at item
   +0x114, entries of 8 bytes from item +0x14, material byte at +3, level i32 at +4),
   with *e* := 1 − 1/((ℓ − 1) × 0.05 + 1) for spirit level ℓ and *u* := *e* × *w* × ρ:
   * material 0x80: *x* += *u*;
   * 0x81: *x* += 0.5 × *u*; heal *h* += 2 × *u*;
   * 0x82: *x* += *u* × 0.25 (computed as *e* × *w* × 0.25 × ρ); target slow countdown
     (record 0x124) := truncate(ρ × 500 + that countdown as float);
   * 0x83: same damage; attacker haste countdown (record 0x128) likewise.
   Both countdowns are then capped at 5000. If *h* > 0: a record with amount −*h*,
   attacker id in both id fields, goes to the hit output list and the attacker's health
   += *h*.
10. **Flash**: attacker's current action 28 → hit flash := 0.3 if it is below 0.3;
    otherwise, blockable → hit flash := 1; otherwise skip to step 12.
11. **Impact sound** (blockable only; position = target, volume 1):
    * Not magic and α ≠ 28: class *u* := subtype of the first weapon-list item (−1 if the
      list is empty); 17 if the attacker's current action is 68. One draw: pitch 0.9 +
      0.2*r*. Normal / critical sound: *u* 0, 1 → 1 / 2 with pitch + 0.1; 15, 16 → 1 /
      2; 3, 4 → 7 / 8; 5 → 3 / 4; 6, 7, 8 → 9 / 10; anything else → 5 / 6. Then attacker
      action 54 → 11, action 10 → 5.
    * Magic: attacker specialization 1 → sound 42 at pitch 1; otherwise sound 39 at
      pitch 1.5. No draw.
12. **Knock-back**: with **v** the target velocity, *m*² := max(|**v**|², |**k**|²); if
    |**v**|² < *m*², **v** := *m* × **v** / |**v**| (a zero **v** produces a non-number
    in the original).
13. *x* × each kind-1 buff value of the target, in list order.
14. **Bleed**: α = 17 → the target gets a kind-4 buff, value 0.1 × *x*, 3000 ms, source =
    attacker id; a pair (attacker id, target id) is appended to a further output list.
15. **Hurt voice**: critical and target controller kind ≠ 6 → one draw (pitch 1 +
    0.1*r*) is consumed, then by target type: 0 → 60; 1 → 61; 2, 43 → 62; 3, 45 → 63;
    4 → 64; 5 → 65; 7 → 66; 8 → 67; 9 → 68; 10 → 69; 11 → 70; 12 → 71; 13 → 74; 14 → 75;
    15 → 72; 16 → 73; 37–40 → 79; 46, 108, 109, 114, 115 → 77; 48, 51, 87 → 78; 96 → 80;
    119 → 76; any other type → no sound (the draw is still consumed).
16. **Stun**: attacker present, target hit-stun counter < −3000 and target controller kind
    ≠ 6: *p* := *σ*; := 1 if the attacker's action is 58, 12, 68, 93 or 69; × 0.15 if the
    target is a big type (§5.12); × 0.1 if the attacker is not a big type and its
    controller kind ≠ 0; := 1 if the attacker's action is 91 or 74; := 1 if the target's
    action is 84. *p* := *p* × 2^(10 × (*e*(*L*att) − *e*(*L*tgt))), capped at 1, with *e*
    the curve of step 9. *q* := 0.9 × *p*, or 0 if the target has any kind-1 buff. One
    draw; if *r* < *q*: the record's stun value := stun length (§5.12), knock-back z :=
    10, and sound 23 (pitch 1, volume 1) is added at the target.
17. **Damage record** (kind 0, §5.13) goes to the hit output list; offline it is applied
    at once.

### 5.10 Spirit bursts (0x59A800)

For each spirit of the item with material ≥ 0x80: a particle burst at the target with
the direction of step 8, count truncate(3ρ + 1), colour from the material at strength
0.5ρ. With *A* set the count is +10 and the direction is doubled. Visual only; no draws.

### 5.11 Action weapon list (0x43DF60)

Main hand type 3 with subtype 6 or 7 → [main]. Else off hand type 3 with subtype 6 or 7
→ [off]. Else by the attacker's current action: 2, 4, 7, 8, 10, 18, 39, 41 → [off] if
off type 3; 11, 12, 16, 17, 20, 54, 86, 91, 96 → [off if type 3, then main if type 3];
40, 42 → [main] if type 3; any other → [main if type 3, then off if it is type 3 with
subtype 12].

### 5.12 Helpers

* **Blocking (0x444590)**: action-lock countdown < 1 and the action is 71, 72, 8 or 98,
  or 59 or 63 for class 1 specialization 1.
* **Big types (0x4445D0)**: 101, 107, 108, 109, 114, 115, 116, 117, 118, 119.
* **Stun length (0x444FF0)**: 1500 for controller kind 0; otherwise 3000 −
  floor(375 × *t*) (computed as (*t* × 1500) shifted right by 2).

### 5.13 Damage record fields

Offsets in the 0x48-byte record: +0x00 attacker id (0 if none), +0x08 target id, +0x10
amount, +0x14 critical flag, +0x18 stun value, +0x1C 0, +0x20 target position, +0x38
knock-back, +0x44 flag *A*, +0x45 kind (0 damage, 1 block, 3 immune, 5 absorb), +0x46 1
when the attacker's current action is 28.

---

## 6. Item stat formulas

### 6.1 Item fields

+0x00 type (3 weapon, 4 chest, 5 gloves, 6 boots, 7 shoulders, 8 amulet, 9 ring, 1
consumable, 13 experience, 20 pet food), +0x01 subtype, +0x04 seed (u32), +0x0C rarity
*R*, +0x0D material, +0x10 level *L* (i16), +0x14 spirits (32 × 8 bytes), +0x114 spirit
count *n*.

### 6.2 Level factors

* *F*(*v*, *R*) = 2^(3 × (1 − 1/((*v* − 1) × 0.05 + 1))) × 2^(0.25 × *R*) (0x445F60).
* *G*(*v*, *R*) = *F*(*v*, *R*) / 2^3 (0x445FF0).
* *v* = *n* × 0.1 + *L* for health, damage, armour, resistance; *v* = *L* for speed and
  crit.
* *u*1 = ((seed × 8) mod 2^32) mod 21; *u*2 = seed mod 21 (unsigned).

### 6.3 Formulas

| Stat (routine) | Types | Result |
|---|---|---|
| Health (0x4C70B0) | 3–7 | ((*F* × 5) × *k*) × *m*; *k* = 1 for type 4 else 0.5; *m* = (1 − *u*1/20) + 1, + 1 for material 1, + 0.5 for 26, + 0.75 for 27 |
| Damage (0x4C7F60) | 3 | *F* × 2 for subtypes 3, 4, 13; × 4 for 5; × 8 for the other W2 subtypes (15–18, 10, 11, 8, 6, 7); × 4 for all others |
| Armour (0x4C6A90) | 4–7 | *F* × (*k* × *a*); *a* = 0.8 for material 18; 0.85 for 19, 26, 27; 0.75 for 23, 25; else 1 |
| Resistance (0x4C7AF0) | 4–7 | *F* × (*k* × *a*); *a* = 0.85 for material 1, 19; 0.75 for 26, 27; else 1 |
| Speed (0x4C7C00) | 3–9 | (*G* × *k*) × *q*; *k* = 0.2 for a weapon in the subtype set of W2 or type 4, else 0.1; *q* = *u*2/20, + 1 for material 12; results below 0.001 give 0 |
| Crit (0x4C6BA0) | 3–9 | (*G* × *k*) × *q*; *k* = 0.1 for the W2 subtypes or type 4, else 0.05; *q* = 1 − *u*2/20, + 1 for material 11; results below 0.001 give 0 |

### 6.4 Test vectors (single precision)

| Item | Result |
|---|---|
| *F*(5, 0) | 1.4142137 |
| sword level 5, rarity 0, no spirits: damage | 5.6568546 |
| same, rarity 2 | 8.0 |
| same, 3 spirits | 5.7791777 |
| greatsword / dagger level 5: damage | 11.313709 / 2.8284273 |
| iron chest level 5, seed 0 / 1 / 7: health | 21.213205 / 18.384777 / 16.263456 |
| iron chest / iron boots level 5: armour | 1.4142137 / 0.7071068 |
| sword level 5, seed 10: speed / crit | 0.008838835 / 0.0044194176 |
| sword level 5, seed 20: crit | 0 |

---

## 7. Applying a hit record (0x595A60)

Only the parts used by the AI and progression are specified here; the full receiving
behaviour belongs to `alpha-net-packets.md`.

* **Threat**: for an amount ≥ 0 with an existing attacker: both creatures' obj +0x11D0 are
  set to each other's id; the attacker's hit map entry for the target += 1; the target's
  threat map entry for the attacker += amount × *f* + 0.01, *f* = 1 when the attacker is a
  ranged user (§11.2 step 9), else 10 for attacker type 25, else 20 for an attacker whose
  main hand is type 3 satisfying the predicate 0x444820 with specialization 1, else 2.
  The threat is then announced (§11.4).
* **Heal**: a negative amount on the simulating side raises health (capped at maximum);
  offline, creatures hostile to the healer gain threat on the healer of −0.1 × amount
  (which map is updated and the membership test are inferred, §U-10), and the threat is
  announced.
* **Kind 1** adds 0.25 to the target's mana, capped at 1.
* **Death**: §2.3 and §8.

---

## 8. Kills and missions (0x595A60, 0x5A0BF0)

* **Kill experience** *e* := max(1, 20 × *e*(*L* + *t*)) of the victim; × 20 for a big
  type; × 10 with appearance flag 0x200; 0 with flag 0x800.
* **Mission progress** (offline only): the map cell is (home zone x ÷ 8, home zone y ÷ 8),
  truncating. If the cell's state byte +0x41 ≠ 2 and (appearance flag 0x2000, or mission
  kind +0x34 = 5 with the victim's obj +0x1D38 set): for the first threat-map entry with
  threat > 0 whose creature is a player: kinds 1–5 and 13 complete; kinds 7–12 add 1 to
  the count +0x44, set state 1, and complete when the count ≥ +0x48. Completion sets state
  2 and refreshes the cell's zones. Only one entry is processed.
* **Kill records**: the victim is removed from every creature's threat map. For each
  player present in a hostile-NPC victim's threat map, a 24-byte record (player id, victim
  id, victim type, truncate(*e*)) is queued and applied: if the player is the world's
  local player its experience += amount and the level-up loop runs; offline, the player's
  living pet gains the same amount and levels, and the player's slot-12 item records the
  pet's experience (+0x04) and level (+0x10).
* **Mission rewards**: on completion, each such player gets an item of type 13 with level
  truncate(50 × *e*(*L*m + *t*m) + 1), then one draw: even → item from 0x5F51E0, odd →
  from 0x5F8AD0, made for the player's class.

| Victim | Kill experience |
|---|---|
| level 1, tier 0 | 1 |
| level 5, tier 0 | 3 |
| level 10, tier 2 | 7 |
| level 20, tier 0 | 9 |
| level 5, flag 0x200 | 33 |

---

## 9. NPC equipment generator (0x5C7730)

Input: spawn object and a mode (activation passes 1, the zone generator 0). Slots are
spawn +0x120 + 0x118 × slot.

1. Appearance flag 0x40: return.
2. Class 0x88: chest := type 4 subtype 0, material 6, then subtype := 2 + (draw mod 5).
   Classes 0x80–0x87 and 0x89: chest type 4 subtype 0, material 6. Both set multipliers
   attack speed 0.3, damage 0.1, armour 10, resistance 10, and return.
3. Clear slots 1–7 (rings in slots 8 and 9 are not cleared).
4. Then the first match: flags 0x18 or type 118 → weapons cleared; controller kind 6 →
   nothing; type 117 → special 92, +0x10B8 := 0, second list 17, 96; types 108 and 114 →
   specials 91, 93 and main hand type 3 subtype 17 material 2; type 46 → that main hand
   only; type 115 → specials 86, 93, fists (type 3 subtype 4) in both hands, material 1;
   type 109 → specials 86, 93, 91, type 3 subtype 2 in both hands, material 7; type 81 →
   special 87, type 3 subtype 12 in both hands, material 11; type 82 → type 3 subtype 0
   in both hands, material 1; types 47, 111, 112, 113 → nothing; types 43 and 45 →
   special 95, specialization := 2, main hand type 3 subtype 10 + draw parity, material 2.
5. **Everything else**, draws in this order: class := 1 + (draw mod 4) (then forced to 4
   for type 97); specialization := draw parity; then:
   * class 1: draw mod 3 — 0: main subtype 15 + (draw mod 3); 1: main subtype (draw mod
     3), then off subtype (draw mod 3); 2: main subtype (draw mod 3), off shield (13).
     Weapons are type 3, material 1.
   * class 2: main subtype 6 + (draw mod 3), material 2.
   * class 3: draw mod 3 — 0 staff, 1 wand (material 2); 2 bracelets in both hands
     (material 11).
   * class 4: main subtype 3 + (draw mod 3), material 1; the off hand copies it unless it
     is 5.
   Unless appearance flag 0x20: armour material 25 for main subtypes 10–12, 26 for 6–8,
   27 for 3–5, else 1; chest (type 4) always; then shoulders (type 7, slot 5), gloves
   (5, slot 4), boots (6, slot 3), each on an odd draw; then amulet (8, slot 1), ring
   (9, slot 9), ring (9, slot 8), each when draw mod 10 = 0, each followed by one draw for
   its material 12 − (draw parity). Armour levels := spawn level.
6. Both weapon slots' levels := spawn level.
7. **Mode 0 only** — multipliers (health, attack speed, damage, armour, resistance; "·"
   unchanged) and specials:

| Types | Multipliers | Specials |
|---|---|---|
| 17, 94, 97 | 400, ·, 1.5, 3, 3 | 93 |
| 21 | 250, ·, 2, 3, 3 | 69 |
| 25 | 200, ·, 0.5, 3, 3 | |
| 46, 82 | 250, ·, ·, 3, 3 | 86 |
| 47, 88 | 250, × 0.5, 3, 3, 3 | |
| 81 | 250, ·, ·, 3, 3 | 92 |
| 85 | 1000, ·, 2, 3, 3 | |
| 86 | class 3, specialization 1 | |
| 107–109, 111–116, 118 | 2500, 0.5 (0.75 when 0x444820 holds), 6, 5, 5 | |
| 117 | 2500, ·, 5, 4, 5 | |
| 119 | 2500, 0.5, 6, 10, 10 | |

   Boss flag 0x200 (mode 0): health × 15, capped at 10 000; damage × 3; attack speed ×
   0.75; armour + 2; resistance + 2; one draw selects one of 92, 93, 86, 89, 87 (draw mod
   5, in that order) which is appended to the specials; then +0x10B8 := draw mod 3; then
   17 and 96 are appended to the second list (+0x10BC).
8. Finally six draws give the seeds of slots 6, 7, 5, 2, 4, 3, in that order.

---

## 10. Sound events

Positional sounds carry position (blocks), id, pitch and volume. The id → key table is
§13.

---

## 11. AI behaviour layer

### 11.1 Scheduling (world step 0x611AE1–0x611CFC)

* NPC behaviour runs only when the world's online byte +0xB4 is 0 (the offline client
  simulates its NPCs).
* For each non-player with health > 0: accumulator += dt; if the hit-stun counter > 0,
  mode flag 0x80 is cleared and the behaviour does not run (the accumulator keeps
  growing); otherwise when the accumulator ≥ 32 the behaviour update runs with the
  accumulated time and the accumulator := 0.
* Type 144 (any controller), offline: health −= 0.025 × dt, floored at 0.
* Offline, every creature: mode flag 0x80 cleared; if its path list is empty nothing more
  happens; if stunned nothing more; otherwise flag 0x04 cleared and the steering of §12
  runs; then if the path is now empty flag 0x80 is set; otherwise flag 0x04 is set, the
  look vector is aimed along the path, and flag 0x80 is set.
* **Sequence (0x4DC390)**: children are called in order; the first that reports handled
  ends the update; if none does, intent := 0.

### 11.2 Combat (0x42CB20)

State: special timer, radius *ρ*a (20 when built at activation), chase time, cooldown.

1. Clear mode flag 0x04. If the action is not 83, 82, 84 or 79 and its time > 10 000,
   action := 0 (time unchanged).
2. Stop if the dodge time ≠ 0, or the action is 80 with time < 3000.
3. **Potion**: health < 0.25 × maximum and time > total duration: the first inventory
   slot with a non-zero count holding type 1 subtype 1 is used (action 80, time 0, count
   −1, slot emptied at 0); then two draws: intent x := 4*r* − 2, then intent y := 4*r* −
   2, intent z := 0; stop.
4. **Target** := the threat-map entry with the greatest value ≥ 0 (later entries win
   ties; id 0 skipped). For controller kind 5 with an existing owner: stop if the owner's
   action is 106; otherwise the target is the creature with the greatest positive value
   in the owner's **hit map** that still exists.
5. No target, controller kind 3 and health < 0.5 × maximum: action 80, time 0, the slot
   at record 0x1D8 := item type 1 subtype 4 with the creature's level; intent := 0; stop.
6. Special timer −= dt, floored at 0. "Pacified" = any kind-8 buff.
7. **Acquire**, when there is no target, the creature is not a pet with an owner, and the
   cooldown < 1: among all creatures other than itself with health > 0 and controller ≠
   5, compute the 3-D squared distance in blocks *d*²; if the candidate's stealth *s* > 0,
   *d*² := *d*² / *f*², *f* = max(0.1, 1 − 1.5^(*L*cand − *L*self) × *s*). A candidate is
   taken when the creature is not pacified, *d*² < the best so far (initially *ρ*a²),
   it is hostile (§11.3) — or the creature is an ownerless pet and the candidate is in
   action 82 holding pet food (slot-12 item type 20) whose subtype equals this creature's
   type — it is in line of sight (0x59EE90, limit 200 blocks), and this creature is not a
   harmless animal (§11.3) unless the same pet-food condition holds. The chosen creature
   gets threat 1 and is announced (§11.4).
8. Running flag 0x40 := a target exists and (action is 0 or time > truncate(P3 / 2) + P1
   + P2). Cooldown −= dt, floored at 0. No target → chase time := 0; stop.
9. **Range *R***: 2 × own box + target box. Ranged user (0x444520: class 3, types 117, 86,
   104, or main hand type 3 subtype 6, 7, 8, 10, 11, 12) with line of sight → 50, unless
   the action is 87, 86 or 89 with time < P1. Types 47, 111, 112, 113 → 20. Action 95,
   28 or 108 → 50. Action 72 → own box + target box. If the special timer is 0 and the
   special list is non-empty, one draw picks a special (draw mod count); 93, 91, 68, 69 →
   *R* := 20; 86, 89 → 2 × own box + target box. No line of sight → *R* := 1.
10. **Chase**, when *d*² > *R*² (3-D) or there is no sight: chase time += dt. While chase
    time ≤ 20 000: action 95 or 28 → action 0; direction to the target with the vertical
    part zeroed unless contact bit 0x02 (water) is set; normalised; intent := 40 × it,
    or 80 × it when 4*R*² ≤ the squared length of the normalised direction. With sight
    and a clear route (0x609480) and contact flag 0x20 clear: the path is cleared and the
    update stops. Otherwise the target's threat × 0.9, and if no path is pending and the
    target's contact bits 0x07 are not all clear, a path search to the target's feet is
    started. After 20 000 ms: threat map cleared, path cleared, cooldown := 3000, chase
    time := 0.
11. **In range**: chase time := 0; path cleared; obj +0x13E0 := 0; intent := 0. Then:
    * **Taming**: not pacified, target in action 82 holding pet food for this type, this
      creature not an owned pet, *d*² ≤ *R*²: obj +0x11D0 := target id; once the current
      action has finished: action ≠ 110 resets the combo; combo < 5 → action 110, time 0;
      otherwise the creature becomes the target's pet (0x42C520), existing kind-7 buffs
      are re-issued with value and duration 0, and a kind-8 buff of 20 000 ms is applied.
      Stop.
    * Mode flag 0x04 set; look vector := target − self in blocks.
    * A special was drawn in step 9 → action := it, time 0, special timer := 20 000.
    * Type 116, after the current action: one draw picks 69, 74 or 73.
    * Type 118, after the current action: one draw picks from 7, 6, 20 and (95 when the
      16-bit appearance field obj +0x8C is 0x86A, else 86).
    * Types 47, 111, 112, 113, after the current action: one draw picks 108, 54 or 76;
      54 also faces the target and sets vertical velocity 15.
    * Type 119, after the current action: a pool from its 16-bit appearance fields obj
      +0x8C, +0x90, +0x94, +0x96: 69 if +0x8C ∈ {0x863–0x865} else 68; 70 if +0x96 ∈
      {0x867, 0x868}; 75 if +0x90 ∈ {0x85F, 0x860} else 74; 77 and 78 if +0x94 ∈ {0x85C,
      0x85D}; 76 if +0x94 ∈ {0x857–0x85B}; if the action is not 71/72 and +0x94 = 0x855,
      one draw, odd → 71; likewise +0x94 = 0x856 → 72; 73 unless the action is 73. Then
      one draw picks from the pool.
    * **Magic user** (0x444610: class 3, types 117, 86, or main type 3 subtype 10–12):
      if *d*² < 3600: mana ≤ 0.5 → chaining; otherwise one draw, even → chaining, odd →
      secondary starter (§14.3). Action 28 at *d*² > 25 aims the look vector at a point
      circling the target (radius 1.5, angle = time × 0.005).
    * **Type 101**: 64 ≤ *d*² < 3600, after the current action → action 37; *d*² < 64 →
      the hop rule below with vertical speed 30.
    * **Natural-weapon hop** (flags 0x02 and 0x10), *d*² < 64: face, flag 0x04; once
      time > 2000: one draw; odd and a height test passes → vertical velocity 5; else
      action 51, time 0, velocity := 10 × unit velocity.
    * **Melee** (type ≠ 104 and main subtype byte not 6, 7, 8): flag 0x10 → after the
      current action, 72 for type 25 if usable (§14.4), else 75. Flag 0x08 → chaining.
      Otherwise within *d*² < (3 × (own + target box))²:
      - Action not 59, 63, 8 and no dodge: one draw; mod 10 = 0, not a big type, own box
        < 2, action finished, contact bits 0x03 not clear → **dodge**: a second draw mod
        4 picks toward, away, left or right; velocity := 20 × that horizontal unit, + 5
        vertical; dodge time 600.
      - Else mana < 0.8 and action not 59, 63, 8 → chaining.
      - Else, no dodge and the action finished: main subtype 5 → 5. If the main hand is
        empty or the off hand holds an item: off subtype 13 → charge 8, then 104 once
        charge ≥ mana; off subtype 3 → 17; 4 → 20; no main hand → 54 if usable (vertical
        velocity + 20), else the secondary starter; any other off hand → 54 if usable,
        else charge 63, then once charge ≥ mana action 11 with time := P1(11). Main hand
        with an empty off hand → 54 if usable, else charge 59, then 61 (specialization 1)
        or 60 with time := P1(60).
    * **Ranged** (type 104 or main subtype 6, 7, 8), *d*² < 3600: the dodge rule above
      (skipped during 24, 25, 27); else mana < 0.8 and action not 24, 25, 27 → chaining;
      else after the action: crossbow 24 → 22, boomerang 27 → 26, bow or no weapon 25 →
      55, each release once charge ≥ mana.

### 11.3 Hostility and harmless animals

* **Harmless (0x444680)**: appearance flags & 0x1A00 = 0 and type ∈ {19, 20, 22, 23, 25,
  26, 27, 30–35, 53, 55–58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107, 145, 146, 147}.
* **Hostile(*a*, *b*) (0x596CA0)**: no if *a* is kind 5 and *b* kind 0, or either is kind
  6. If *a* is kind 1: yes if *b* is not 1, or if exactly one of them is harmless. The
  same with *a* and *b* exchanged. Otherwise yes only if either has mode flag 0x20.

### 11.4 Announce (0x5A0970)

A record (self id, candidate id, self's threat on the candidate) is queued. Every other
creature hostile to the candidate within 8 blocks (*d*² < 64) of self whose threat on the
candidate is 0 gets threat 0.5 and a further record.

### 11.5 LookAtPlayer (0x4C8140)

Clear flag 0x04. Nearest controller-0 creature with health ≥ 0 at 3-D *d*² < 64. Unless
none or the action is 83 or 84: set flag 0x04, look vector := player − self, intent := 0,
path cleared.

### 11.6 RandomWalk (0x4DBF20)

Clear running. Timer −= dt, floored at 0. At 0: if (dx² / 65536 + dy² / 65536) ≤
6 553 600 in world units (each quotient truncated; about 10 blocks from home): two draws,
intent := 10 × (2*r* − 1, 2*r*′ − 1, 0); otherwise intent := 10 × the horizontal unit
vector toward home. Then timer := (draw mod 5000) + 3000 and action := 0.

---

## 12. Steering (0x5A7EB0)

Intent is zeroed on entry. With **u** the unit direction toward the next path point:

* Early exit (no further change) for a creature in the flying/swimming state without
  mode flag 0x01.
* Path length counter ≥ 4 (meaning inferred): intent := 40 × **u**.
* Otherwise, each relaxation is applied dt times in succession, each time *v* := (target
  − *v*) × *k* + *v*:
  * not flying: if u_z ≥ 0 (and a further condition holds), vertical toward 4 × u_z with
    *k* = 0.05; horizontal toward 6 × **u** (12 with running) with *k* = 0.001, or in
    the alternative movement state toward 8 × **u** with *k* = 0.01;
  * flying with u_z ≥ 0: velocity := 8 × **u** directly; with u_z < 0 the horizontal
    relaxation above applies.
  Intent := 20 × **u**.
* The push vector (record 0x3C) := 0.

---

## 13. Sound ids (0x484350)

| Id | Key | Id | Key | Id | Key | Id | Key |
|---|---|---|---|---|---|---|---|
| 0 | hit.wav | 26 | roll.wav | 52 | spike-trap.wav | 78 | mole-groan.wav |
| 1 | blade1.wav | 27 | destroy2.wav | 53 | fire-trap.wav | 79 | slime-groan.wav |
| 2 | blade2.wav | 28 | cry.wav | 54 | lever.wav | 80 | zombie-groan.wav |
| 3 | long-blade1.wav | 29 | levelup2.wav | 55 | charge2.wav | 81 | Explosion.wav |
| 4 | long-blade2.wav | 30 | missioncomplete.wav | 56 | magic02.wav | 82 | punch2.wav |
| 5 | hit1.wav | 31 | water-splash01.wav | 57 | drop.wav | 83 | menu-open2.wav |
| 6 | hit2.wav | 32 | step2.wav | 58 | drop-coin.wav | 84 | menu-close2.wav |
| 7 | punch1.wav | 33 | step-water.wav | 59 | drop-item.wav | 85 | menu-select.wav |
| 8 | punch2.wav | 34 | step-water2.wav | 60 | male-groan.wav | 86 | menu-tab.wav |
| 9 | hit-arrow.wav | 35 | step-water3.wav | 61 | female-groan.wav | 87 | menu-grab-item.wav |
| 10 | hit-arrow-critical.wav | 36 | channel2.wav | 62 | male-groan.wav | 88 | menu-drop-item.wav |
| 11 | smash1.wav | 37 | channel-hit.wav | 63 | female-groan.wav | 89 | craft.wav |
| 12 | slam-ground.wav | 38 | fireball.wav | 64 | goblin-male-groan.wav | 90 | craft-proc.wav |
| 13 | smash-hit2.wav | 39 | fire-hit.wav | 65 | goblin-female-groan.wav | 91 | absorb.wav |
| 14 | smash-jump.wav | 40 | magic02.wav | 66 | lizard-male-groan.wav | 92 | manashield.wav |
| 15 | swing.wav | 41 | watersplash.wav | 67 | lizard-female-groan.wav | 93 | bulwark.wav |
| 16 | shield-swing.wav | 42 | watersplash-hit.wav | 68 | dwarf-male-groan.wav | 94 | bird1.wav |
| 17 | swing-slow.wav | 43 | lich-scream.wav | 69 | dwarf-female-groan.wav | 95 | bird2.wav |
| 18 | swing-slow2.wav | 44 | drink2.wav | 70 | orc-male-groan.wav | 96 | bird3.wav |
| 19 | arrow-destroy.wav | 45 | pickup.wav | 71 | orc-female-groan.wav | 97 | cricket1.wav |
| 20 | blade1.wav | 46 | disenchant2.wav | 72 | undead-male-groan.wav | 98 | cricket2.wav |
| 21 | punch2.wav | 47 | upgrade2.wav | 73 | undead-female-groan.wav | 99 | owl1.wav |
| 22 | salvo2.wav | 48 | swirl.wav | 74 | frogman-male-groan.wav | 100 | owl2.wav |
| 23 | sword-hit03.wav | 49 | human-voice01.wav | 75 | frogman-female-groan.wav | | |
| 24 | block.wav | 50 | human-voice02.wav | 76 | monster-groan.wav | | |
| 25 | shield-slam.wav | 51 | gate.wav | 77 | troll-groan.wav | | |

---

## 14. Ability values, usability, costs, cooldowns, buffs

### 14.1 Skill of an ability (0x43C9C0) and ability value (0x43ED60)

Skill 6: 21, 34, 48, 54, 88. Skill 7: 50, 79, 86, 103. Skill 8: 49, 96, 97, 99, 100,
101, 102. Others: none. *v*(id) = 1 for no skill; otherwise *n* := that skill's level;
for a non-player with *n* = 0, *n* := truncate(*L* / 2) + *t* + 1; *v* = 1 − 1/(0.1 × *n*
+ 1). The skill factor used for climbing and gliding is 1 − 1/((*n* + 1) × 0.1 + 1)
(0x43E660).

### 14.2 Free to act (0x43E350)

False for action 48. Otherwise true when charge ≤ 0 and (time ≥ total duration, or the
action is one of 0, 1, 2, 3, 4, 6, 7, 9, 10, 12, 13, 14, 15, 16, 18, 19, 22, 26, 28, 29,
31, 33, 34, 37, 45, 46, 57, 58, 65, 66, 67, 86, 98, 101).

### 14.3 Secondary-attack starter (0x4456A0)

*D* := total duration, minus P3 of the current action when the combo ≠ 0. *x* :=
secondary attack (§3.2). *R* := class 4, specialization 1 and *x* ∈ {5, 17, 20}. Refuse
when time < *D*, or the dodge time ≠ 0 and not *R*, or mana < cost(*x*). Otherwise action
:= *x*, time := 0; if *x* ∈ {37, 46, 95, 31, 33} and a kind-9 buff exists, time := P1(*x*);
unless *R*, dodge time := 0.

### 14.4 Usable (0x43E5A0) and cost (0x444AE0)

Usable when: hit-stun counter < 1 or id = 101; cost(id) ≤ mana; the cooldown table has no
entry for id or its entry is 0; and for id 28, mana > 0.

Cost: 3, 4 → 0.1; 31, 33, 37, 43, 45, 46, 95 → 0.3, or 0 with a kind-9 buff; 34 → (1 −
*v*(34) × 0.75) × 0.125; others 0.

### 14.5 Cooldown lengths (0x43E6A0)

Truncate of: 21 and 88: 8000 − 8000*v*; 48: 20000 − 12000*v*; 49, 50: 16000 − 10000*v*;
54, 96: 20000 − 14000*v*; 72: fixed 15000; 86: 60000 − 40000*v*; 97, 100, 101, 102: 60000
− 30000*v*; 99: 12000 − 10000*v*; 103: 20000 − 10000*v*; others 0. Example, *v* =
0.1666667: 54 → 17666, 102 → 55000, 21 → 6666, 50 → 14333; *v* = 0.0909091: 102 → 57272.

Countdown (0x612048–0x6120E0): every entry −= dt each step, floored at 0; entries are not
removed. A re-arm pass (0x611F9F) uses a predicate (0x687D10) that always answers false.

### 14.6 Buff abilities (0x595850)

Reading cooldown[id] creates a 0 entry if missing; if it is > 0 nothing happens.
Otherwise:

| Id | Kind | Value | Duration |
|---|---|---|---|
| 97 | 3 | 0 | truncate(*v* × 12000 + 8000) |
| 100 | 12 | *v* + 1 | 10000 |
| 101 | 1 | 0.75 − *v* × 0.3 | 10000; a positive hit-stun counter is set to 0 |
| 102 | 2 | 0 | 10000 |
| 103 | 6 | (*q* × 2) × (*v* + 1), *q* from 0x446E80 | 30000 |
| other | none | | |

The buff is added (0x446AF0: kinds 1, 2, 3, 6, 7, 9, 10, 11 replace an existing entry of
the same kind, others append) and a 40-byte record is appended: own id (+0x00, +0x08),
kind (+0x10, the three following bytes not cleared), value (+0x14), duration (+0x18), four
bytes not initialised (+0x1C), eight zero bytes (+0x20). Then cooldown[id] := length(id)
(also for ids without a buff).

---

## U. Unresolved

Items not re-confirmed against the image in this pass, or confirmed only in part.
Implementers must not rely on them as specified.

* **U-1 Ravine spawns** (deposits 131–138, bat 59; draw order). Not located in the
  readable output of the zone generator 0x5E4850; the inherited reading is unconfirmed.
* **U-2 Spawn activation** (0x60FB48–0x6103E3): countdown handling, time-of-day window,
  existing-creature test, radius test, mission-creature level/tier/seeding and the boss
  item re-roll, the copied fields and the behaviour built at activation (Combat radius
  20, LookAtPlayer, RandomWalk exclusions). Only the presence of the day-length constant
  86 400 000 was observed.
* **U-3 Stamina, mana and stealth regeneration** (0x619400–0x619DCC): the constants
  0.0005, 0.0002, 0.0001, 0.00025 and 0.00001 occur in the routine, but branch
  conditions, the lantern sum and the sneaking branch were not re-traced.
* **U-4 Item lists of 0x5C8390** (tiers, rarity draws, consumers) not re-traced.
* **U-5 Mission reward item route** (experience += 10 × item level via the client code
  near 0x48F8D5): the region is not disassembled in the evidence.
* **U-6 Ability bar** (slot list, press and release tables, beams, M1 rule, quick-item
  use, inside 0x488EE0): not re-traced.
* **U-7 Steering details**: the flying/alternative-state conditions, the "path length ≥
  4" interpretation and the routine's return value.
* **U-8 Behaviour return values** of Combat, LookAtPlayer and RandomWalk (which stop the
  sequence), and the stunned and steering-handled branches of §11.1 (0x611DF0,
  0x611DD2).
* **U-9** SpawnLocation (0x4DFAD0), Companion (0x43ACE0), RandomInteraction (0x4DB130),
  WalkPath (0x58D050).
* **U-10** Meanings: flag *A* of §5, stun chance *σ* sources, predicate 0x444820, stat
  0x446E80, obj +0x13B8, action power consumers, the second action list +0x10BC, the
  guaranteed-critical interaction at the start of actions 5, 17, 20 (0x443F10 with kind
  11), mode flag 0x80, contact bits, the level re-roll 0x4C7E80.
* **U-11** Player record defaults on load and on an Alpha server; NPC health
  regeneration absence; the player's 5 %/s regeneration (owned by
  `alpha-player-control.md`).
* **U-12** Default collision box values used in range examples.

---

## Re-certification (2026-11-15)

Evidence: `Cube.exe` in the evidence root (decompiled routines, disassembly and data
constants read directly from the image). Reconstruction leads from Infinite-Cubes/alpha
were consulted only for the hit-record field naming (critical byte and kind byte
positions); the positions stated here come from the image.

### Corrections

1. Critical with a kind-11 buff: was "always"; now one draw is consumed and compared
   with probability 1, so a draw of 32767 is not critical (0x4444A0). *Reverses an
   earlier claim; draw count clarified.*
2. Hit resolution early exit also when the world online byte is set and the target is
   the world's +0xB8 creature (0x596D30). New.
3. Attacker action 28 particle consumes one draw (0x596D30, near 0x596E8x). *Changes
   random-draw count/order.*
4. Block and absorb sound pitches and their draws stated (pitch 1 + 0.3*r*); gauge floor
   is −1 (0x596D30).
5. Hit flash: 1 only when blockable; action-28 case raises to 0.3; otherwise sounds are
   skipped (0x596D30).
6. Hurt voice: was "types 0–10, others follow-up"; now the full table, and the draw is
   consumed even for types without a voice (0x596D30, near 0x59829D). *Changes
   random-draw count.* Resolves the old voice open item.
7. New stun step with its own draw, formula and constants (0x596D30, 0x5983F8–0x598598;
   0x444FF0; 0x4445D0). *Adds a draw; changes random-draw order after the hurt voice.*
8. Hit record: was "critical flag at byte 0x44"; now critical at +0x14, flag *A* at +0x44,
   kind at +0x45, stun value at +0x18, +0x46 set for action 28. *Reverses an earlier
   claim.*
9. Bleed (action 17) also appends an (attacker, target) pair record; the buff carries the
   attacker id.
10. Burst direction normalised by the 3-D length of the knock-back; the action weapon list
    (0x43DF60) is now specified.
11. Slow/haste countdown update is truncate(float(field) + 500ρ), not field + ⌊500ρ⌋.
12. Armour flag 0x20 adds the tier-based NPC base, unmultiplied, also for players
    (0x43CFF0).
13. comboMax is truncated; staff and crossbow get 50 (0x444CB0).
14. Several primary-attack and P1 tests read the main-hand subtype byte without checking
    the item type (0x445020, 0x43CAA0).
15. Action lock (record 0x120): time := total + 1 (was "total re-applied"); floor 0.
16. Steering relaxations are dt successive per-millisecond steps; intent zeroed on entry
    (0x5A7EB0).
17. Action start costs: 31/33 deferred to the P1 crossing; 5/17/20 consume all mana;
    21/54/87/88 power 1; charge halved for 5 and 11 (0x613510–0x6137D4). New.
18. Action 54 vertical velocity −60 between P1 and P1 + P2 (0x618544). New.
19. Equipment generator does not clear ring slots 8 and 9 (0x5C7730).
20. Boss: was "specials 92, 93, 86, 89, 87 and more (one random, then 17 and 96)"; now one
    of the five chosen by draw mod 5, then the +0x10B8 draw, then 17 and 96 into the
    second list; type 117's 17 and 96 also go to the second list (0x5C8142–0x5C8199).
    *Reverses an earlier claim; random-draw use changed.*
21. Pets target from the owner's hit map (+0x13AC), not "the attacker of their owner";
    threat accumulation specified (0x42CB20, 0x595A60). *Reverses an earlier claim.*
22. AI 10 s reset changes the action id only.
23. Potion: time strictly greater than the duration; intent draws in x-then-y order.
24. Taming acquisition only for ownerless pets; target tie rule stated.
25. Chase: "face the target" was wrong — the path is cleared; path search requires target
    contact bits (0x42CB20).
26. Types 47, 111–113 pool: was 108, 54, 67; now 108, 54, 76. *Reverses an earlier
    claim.*
27. Melee mana abilities: smash 54 is preferred when usable; the 59 charge path applies to
    any main weapon with an empty off hand.
28. Type 118 condition, type 119 pool and draws, natural-weapon hop range and draws stated.
29. AI scheduling flag behaviour corrected; type-144 drain offline only (0x611B04–0x611CFC).
30. Kill records go to players present in the victim's threat map (no threat > 0 test);
    mission progress requires a player with threat > 0, processed once, offline only; pet
    experience recorded in slot 12 (0x595A60, 0x5A0BF0).
31. Buff record: +0x1C four bytes uninitialised, +0x20 eight zero bytes (0x595850).
32. *F*(5, 0) in single precision is 1.4142137 (was 1.4142135).
33. Spawn defaults (level 1, −1 fields) and new-record controller kind 3 stated.
34. Removed implementer advice, engine references, verification procedures and drafting
    history.
35. Moved to §U: ravine spawns, activation, regeneration branches, item lists, mission
    item route, ability bar, steering conditions, behaviour return values, other
    behaviours, server/save defaults.

### Confirmed

Phase tables P1/P2/P3 for every listed id; chaining; primary and secondary attack rules;
max health, damage, resistance, attack-speed and crit formulas; level cost and level-up;
hit resolution steps other than those corrected; item formulas and all test vectors
(recomputed); kill-experience table; equipment generator class/weapon/armour draw order
and multiplier table; harmless list; hostility; LookAtPlayer; RandomWalk; sound table
(ids 0–100); ability skill map, value, free-to-act, usable, costs, cooldown lengths and
examples, buff table; death countdown 1 200 000 ms.

### Unresolved

U-1 to U-12 above.
