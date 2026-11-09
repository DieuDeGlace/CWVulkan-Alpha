# Alpha creatures: spawning, AI, combat, stats and progression

Behavioural specification of what the Cube World **Alpha** client (`Cube.exe`, SHA-256
`84a7a132…30bf`) simulates for NPCs, monsters and the player's combat: spawn objects,
steering, actions and their timing, hit resolution, derived stats, regeneration,
experience and levels. Written from the Alpha decompile (creature methods 0x43a000..
0x448200, hit resolution 0x596d30, steering 0x5a7eb0, zone generator 0x5e4850) and
checked in the assembly where precision matters. The shared world step (client 0x60c510,
server 0x5322d0, assembly only) was read only where cited.

**Coverage.** Stats, levels, experience, action timing, attack chaining, the primary and
secondary attack choice, hit resolution (block, absorb, armour, knock-back, sounds), the
ravine spawns and the spawn-object defaults are specified. The second pass (§8–§14) adds
spawn activation and respawn, the NPC equipment generator, the item stat formulas, kill
experience and mission rewards, stamina/mana/stealth regeneration, item "spirit" effects
and life steal, the full sound-id table and the AI behaviour layer. Projectiles are in
`alpha-projectiles.md` (§8.3); the zone post-pass placement is still open (§15).

Related: `alpha-net-packets.md` (§2.1 fields, §5.1 hit record), `alpha-creatures-models.md`
(types, action → pose), `alpha-player-control.md` (input, step), `alpha-worldgen-overview.md`
§5 and `alpha-worldgen-terrain.md` §5.5 (where spawns are created).

---

## 0. Conventions

As in `alpha-player-control.md` §0. *S* = attack-speed factor (§4.3), *a* = the creature's
attack-speed multiplier (record field 30, element 1). "c / (S·a)" means
⌊*c* / (*S*·*a*)⌋ computed in single precision and truncated toward zero to milliseconds.
"draw" = one `rand()` (0..32767). *r* = draw/32767.

---

## 1. Record fields used here (adds to P-11)

| Field (offset) | Meaning |
|---|---|
| 7 (0x050) | controller kind: 0 player, 1 hostile NPC (*inferred*: target-lock and immunity tests), 5 *unknown* (ignores its mount for posing), 6 *unknown* (no hurt voice) |
| 9, 10 (0x058, 0x05c) | action id, action time (ms) |
| 11 (0x060) | **combo counter** (hits in the current chain; compared with the combo maximum, §4.4) |
| 15 (0x118) | dodge time |
| 16 (0x11c) | hit-stun counter (raised to hit field 0x18) |
| 21, 22 (0x130, 0x131) | class (1 warrior, 2 ranger, 3 mage, 4 rogue) and specialization |
| 17 (0x120) | a countdown in ms, decreased by dt each step; while > 0 at the end of the step the action's total duration is re-applied (0x611d0d) — *meaning unknown* |
| 18, 19 (0x124, 0x128) | countdowns in ms (−dt per step, floor 0, cap 5000): 18 is raised on the **target** by ice spirits, 19 on the **attacker** by wind spirits (§12.2) |
| 20 (0x12c) | f32; when negative it rises toward 0 by dt/180 per step |
| 23 (0x134) | **charge**: mana gathered by a held ability; spent with the action (§11.3) |
| 28 (0x160) | **mana**, 0..1 (*inferred* from: action costs are subtracted from it, abilities need it, it is refilled by the block rule of §5 step 5) |
| 29 (0x164) | block gauge (§5 step 5) |
| 27 (0x15c) | health |
| 30 (0x168, 5 × f32) | **stat multipliers**: health, attack speed, damage, armour, resistance |
| 33, 34 (0x180, 0x184) | level, experience |
| 37 (0x198) | **power tier** (u8) of NPCs: raises health, armour and speed (§8.4 for its value) |
| 39 element 2 (0x1a8) | index of the creature's spawn object in its zone's spawn list (read at death, §9.4) |
| 40 (0x1b0) | home position = the spawn object's position (§9.2) |
| 44 (0x2f0) | equipment (slots in `alpha-creatures-models.md` §6.4) |
| 46 (0x1128) | 11 skill levels |
| obj +0x1178 | **buff list**: entries (kind u8, value f32, time…); kinds seen: 1 damage multiplier, 4 damage over time, 6 absorbing shield, 9, 11 guaranteed critical, 12 attack speed |
| obj +0x1190 | stealth amount *s*, 0..1 (*inferred*: the renderer fades the creature by it, a hit removes it, critical chance grows with it, it shrinks the AI detection radius, §13.3); also scales mana regeneration (§11.2) |
| obj +0x1194 | stamina, 0..1 (§11.1) |
| obj +0x11c8 | the creature's pet id (u64) |
| obj +0x13a4 | **threat map**: creature id → threat (f32) |
| obj +0x13e4 | the AI behaviour object (§13) |
| obj +0x13e8 | the NPC's special-action list (ids), copied from the spawn object |

---

## 2. Spawning

### 2.1 Spawn objects (0x5adb20)

Creatures placed by world generation are not created directly: the zone generator creates
**spawn objects** (4336 bytes; the executable's class name is `cube::Spawn`) that the world
step turns into creatures near the player. A new spawn object holds: radius-like float
200 (+0x08), position (3 × i64 at +0x10), a value 1 at +0x28 and 4 at +0x2c (overwritten
by the generator: +0x2c is the **creature type**, +0x28 a count or level, *inferred*),
a creature template (default appearance 0x428750, default equipment), +0xf58 = 100,
four multipliers 1.0 (+0xf5c..+0xf68; the template's stat multipliers, *inferred*), and
cleared state. The fields that become the creature, and the activation and respawn rules,
are in §9.

### 2.2 Ravine spawns (zone generator, terrain spec §5.5 step 10)

When a ravine column passes its test (draw mod 30 = 0, then draw & 3 selects):

* **1 → mineral deposit** (if top ≤ surface): a spawn object at the column (x, y, floor),
  +0x28 := 6, +0x34 := 1, +0xf58 := 25, and the type from a new draw *d* mod 10:
  0 → 131 gold deposit; 1 → 133 silver deposit; 3 → by a further draw mod 100:
  0 → 138 diamond, 1–3 → 137 ruby, 4–8 → 136 sapphire, otherwise 135 emerald;
  anything else → 132 iron deposit. (Creature types 131–139 are deposits, see
  `alpha-creatures-models.md` Appendix B.)
* **2 → type 59 (bat)** spawn object (if top ≤ surface).

(Cases 0 and 3 are vegetation and an object entry; see the terrain spec.)

### 2.3 Other placement

The post passes of the zone generator create creatures and spawn objects for towns,
castles and dungeons (`alpha-worldgen-overview.md` §5: 0x5ad320, 0x58d860, 0x5af170,
0x66add0). Follow-up F-2.

---

## 3. Actions

An action is (id, time). The step advances the time by dt and, when time ≥ the action's
total duration, ends it (action := 0; *inferred* from the chaining rule below). What is
known of the time advance, P2 and combo resets is in §8.

### 3.1 Choosing the primary attack (0x445020)

Let **M** = main weapon (slot 7) subtype if its type is 3, **O** = off-hand (slot 6).
"Chain *x*→*y*" means: if the current action is *x* and its time < duration(*x*) + 200,
return *y*, otherwise *x*.

1. Magic users — class 3, or creature type 117 (lich) or 86 (squid), or M is staff (10),
   wand (11) or bracelet (12):
   * M = wand: 38, or 44 for specialization 1;
   * M = staff: 30, or 32 for specialization 1;
   * otherwise specialization ≠ 1: chain 40→39 (i.e. 40, then 39 if re-pressed in time);
     specialization 1: chain 42→41.
2. Type 104 (snout beetle), or M or O is a bow/crossbow (6, 7): 22.
3. M = dagger (3): chain 19→18.
4. M = fist (4) or no main weapon: chain 6→7.
5. M = longsword (5): chain 14→13.
6. M = boomerang (8) with main type 3: 26.
7. M two-handed or ranged (greatsword, greataxe, greatmace 15–17, longsword 5, staff,
   wand, pitchfork 18, boomerang 8, bow 6, crossbow 7 — 0x444230): chain 57→67 and
   67→58 (57, 67, 58, 57…).
8. O is a shield (13): chain 10→9.
9. Appearance flag 0x10 (natural weapon): 75.
10. Otherwise (one-handed sword, axe, mace…): chain 1→2.

### 3.2 Secondary attack (0x4452a0)

* M = bow: 23.
* Magic users (rule 1 above): M = wand → 46 (spec ≠ 1) / 45 (spec 1); M = staff → 31 /
  33 (spec 1); otherwise 37 (spec ≠ 1) / 43 (spec 1).
* M = longsword: 5.
* M two-handed or ranged: 65 if the combo counter is odd, else 66.
* Otherwise: 4 if the combo counter is odd, else 3.

### 3.3 Durations (0x43d1a0 = P1 + P2 + P3; P1 alone = 0x43caa0)

Each phase is c / (*S*·*a*) unless marked "fixed". **P1** is the wind-up: the hit happens
when the action time reaches P1 (*inferred* from the attack-window tests of the animation
and friction code). Unlisted ids use the default.

| Action ids | P1 | P2 | P3 |
|---|---|---|---|
| 0 | 0 | 0 | fixed 100 |
| 8, 28, 98 | 0 | 300 | 500 |
| 11, 60, 61, 104 | 0 | 300 | 100 |
| 50, 96 | 0 | fixed 500 | fixed 100 |
| 55 | 0 | 500 | 500 |
| 62 | 0 | 100 | 300 |
| default | 400 | 300 | 500 |
| 1, 9 | 300 | 200 | 500 |
| 2 | 100 | 200 | 500 |
| 3, 4 | 100 | 100 | 300 |
| 5 | 100 | 400 | 300 |
| 6 | 50 | 150 | 500 |
| 7 | 50 | 150 | 200 |
| 10 | 100 | fixed 200 | fixed 600 |
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
| 37, 43 | 1200 (600 if M is a bracelet) | 100 | 100 |
| 38, 44 | 500 | 300 | 100 |
| 39–42 | 200 | 300 | 100 |
| 45, 46 | 1200 | 500 | 100 |
| 48 | fixed 100 | 300 | 0 |
| 54 | fixed 400 | fixed 100 | fixed 400 |
| 57, 58 | 800 | 300 | 300 |
| 65, 66 | 300 | 400 | 200 |
| 67 | 200 | 200 | 100 |
| 68–70, 73–78 | 300 | 300 (68, 69: fixed 1000; 76–78: fixed 500; 73: fixed 600) | 500 (68, 69: 800) |
| 71, 72 | fixed 200 | fixed 3000 / fixed 5000 | fixed 200 |
| 87 | 5000 | 300 | 500 |
| 89 | as 37 | 300 | 500 |
| 91 | 1000 | fixed 6000 | 500 |
| 93 | 800 | fixed 600 | 800 |
| 94 | 500 | 500 | 100 |
| 95 | 1000 | fixed 2000 | 100 |
| 101 | fixed 100 | 300 | 500 |
| 105 | fixed 5000 | 300 | 500 |

So action 0 lasts 100 ms. Id 49: 400 | 0 | 500; id 86: 400 | fixed 5000 | 500. (Read the three switches
of 0x43d1a0 for any id not in this table; the table was transcribed from them.)

### 3.4 Chaining (0x445410)

Called when the attack input is pressed (player) or the AI attacks. Returns "busy" if
dodging or the action is 48. Otherwise let *D* = the current action's total duration; if
the combo counter ≠ 0 and the action is not 30 or 32, *D* −= P3 of the current action
(the recovery can be cancelled while a combo is running). If the action time ≥ *D*:
action := primary attack (§3.1), time := 0.

---

## 4. Derived stats

Let *L* = level, *g* = 3·(1 − 1/((*L* − 1)·0.05 + 1)) (single precision), *t* = power tier
(field 37), and *P* = 2^*g* (double `pow`, rounded to float). NPC base = *P*·2^(0.25·*t*);
player base (controller kind 0) = *P*·2 (the tier term is replaced by 2^1).

### 4.1 Maximum health (0x444db0)

maxHealth = base × health multiplier; then × 1.3 for class 1, × 1.1 for class 2, × 1.2
for class 4, and × 1.25 more for class 1 specialization 1; then + the health bonus of
each equipped item: weapons in slots 6 and 7, chest (slot 2), boots (3), gloves (4),
shoulders (5) (0x4c70b0, item formula §10.3).

Only an item of the expected type counts: type 3 in slots 6 and 7, type 4 in slot 2, 6 in
slot 3, 5 in slot 4, 7 in slot 5.

Example: level 1 player warrior with the default multiplier **100** (§8.4): *g* = 0, base
2, 2·100·1.3 = **260**, plus the item health of §10.2 (a level 1 iron sword in each hand
adds 5 to 7.5 each). (The earlier draft of this example used 50; that is not the value
any Alpha creature gets.)

### 4.2 Damage, armour, resistance

* **Damage** (0x43e190): *d* = *P* × damage multiplier, plus weapon damage: off-hand
  weapon (slot 6) if it is not a shield, plus main weapon (slot 7); if the off hand is a
  shield the main weapon counts ×4 *as read* (open question B-1). Result × damage
  multiplier (applied twice in total).
* **Armour** (0x43cff0): base × armour multiplier; + base again if appearance flag 0x20;
  + armour of chest, boots, gloves, shoulders.
* **Resistance** (0x4467a0): the same with the resistance multiplier (used against magic).

### 4.3 Attack-speed factor *S* (0x447700)

*S* = 1 for players, 0.75 + *t*/16 for NPCs; + 0.5·min(combo/comboMax, 1) for class 1
specialization 0; + min(combo/comboMax, 1) for class 3 specialization 1; + 0.1·*P*/8 +
the speed bonus of weapons, armour, amulet (slot 1) and rings (slots 8, 9) (0x4478b0);
then, if a buff of kind 12 is active, × (2 − 1/(0.1·*n* + 1)) with *n* = skill 8's level
(record 0x1148) or, for NPCs with it 0, *L*/2 + *t* + 1.

### 4.4 Combo maximum (0x444cb0)

comboMax = *a* × (60 for a bow, 80 for a boomerang, 20 for wand, bracelet, great weapons
or natural-weapon creatures, 50 otherwise).

### 4.5 Critical chance (0x4444a0) and others

* Critical: always if a buff of kind 11 is active; else probability
  0.1·*P*/8 + item crit bonuses (0x43ea40) + 0.15 × stealth (one draw, *r* < p).
* A chance test with weapon weighting (0x4441a0): p × 0.5 for sword, axe, mace,
  bracelet; × 0.3 for boomerang and staff (caller follow-up F-5).

### 4.6 Level and experience

* Experience needed for the next level (0x444d60):
  cost(*L*) = ⌊(1 − 1/((*L* − 1)·0.05 + 1))·1000 + 50⌋ (single precision, truncation).
  cost(1) = 50, cost(2) = 97, cost(10) = 360, cost(100) = 881.
* Level-up (0x447b00), while *L* > 0 and experience ≥ cost(*L*): experience −= cost(*L*),
  *L* += 1, health := maxHealth (§4.1).
* **Experience award**: two routes, both in §12.3. (a) A **kill record** (24 bytes) gives
  the killing player experience += its amount (client 0x5a0bf0). (b) A record holding a
  u64 player id and an **item of type 13** gives experience += 10 × the item's level
  (signed 16-bit at record byte 0x18 = item +0x10; client 0x48f8d5); this is the mission
  reward. The first draft called (b) "event 40 / section 12"; the record that carries an
  item is 8 + 280 = 288 bytes, which matches world-update section 9 (*inferred*, P-9).

### 4.7 Regeneration

Health: 5 % of maximum per second for the local player (`alpha-player-control.md` §7).
No other health regeneration was found in the step: NPCs do not regenerate (they heal
only by life steal, §12.2, and potions, §13.2). Stamina, mana and stealth: §11.

---

## 5. Hit resolution (0x596d30)

Inputs: target, attacker (may be none), damage *x*, critical flag, area flag, knock-back
vector **k**, magic flag, attacker action, "can be blocked" flag. Order:

1. Ignore if the target's health ≤ 0.
2. **No attacker**: one draw is consumed; hit flash := 0.5; continue at step 11 with *x*.
3. Attacker action 91 or 74 and target not on ground → ignore (ground attacks).
   Attacker action 28: a particle effect at the target (follow-up).
4. **Immune**: target controller kind 1 with mode flag 0x80 or contact flag 0x20: sound
   24 at the target with pitch 1 + 0.3*r*; a hit record of **kind 3** (attacker id,
   target id, amount *x*, knock-back **k**) is queued; stop.
5. **Block** (can be blocked, the target is blocking (0x444590), and field 29 > 0):
   *q* = *x* / damage(target) (§4.2); field 28 := min(1, field 28 + field 29);
   field 23 := min(field 28, field 23 + field 29); field 29 := max(−1, field 29 − *q*);
   a **kind-1** hit (amount *x*, knock-back **k**/2) is queued; sound 24; stop.
6. **Absorb**: *s* = sum of the target's kind-6 buff values; *c* = 1 −
   min(attacker combo/comboMax, 1); if *c*·*s* > 0: a **kind-5** hit (amount *x*,
   knock-back **k**/2) and sound 91; *x* −= *c*·*s*; stop if *x* ≤ 0.
7. **Armour**: *x* := max(0, *x* − *c*·(magic ? resistance : armour)(target)).
8. **Spirit effects** (corrected): if *ρ* = attacker combo/comboMax > 0.25, every
   **special spirit** on the attacker's weapons emits a particle burst at the target
   (0x59a800, §12.1) — nothing is pushed. If the area flag is set and *ρ* > 0, the
   spirits add damage, slow the target, haste the attacker and steal life (§12.2).
9. Hit flash := 1 (0.3 if the attacker's action is 28 and the flash is below 0.3).
10. **Impact sound** (when "can be blocked"): pitch 0.9 + 0.2*r*; sound by the attacker's
    weapon class (subtype of the first entry of the attacker's weapon list, 0x43df60;
    17 when the action is 68) and the critical flag (normal / critical): subtype 0–1 →
    1 / 2 with pitch + 0.1; 15–16 → 1 / 2; 3–4 → 7 / 8; 5 → 3 / 4; 6–8 → 9 / 10; other →
    5 / 6; then attacker action 54 → 11 and action 10 → 5 override. Magic attacks (magic flag): attacker specialization 1 →
    sound 42 at pitch 1; otherwise sound 39 at pitch 1.5 (volume 1). No impact sound if
    the attacker's action is 28.
11. **Knock-back**: the target's speed becomes max(|**v**|, |**k**|) keeping the
    direction of **v** (if **v** is zero the result is undefined in the original).
12. Damage multipliers: × each kind-1 buff value of the target.
13. Attacker action 17: the target gets a kind-4 buff (damage over time) of 0.1·*x* for
    3000 ms.
14. Critical hits on targets of controller kind ≠ 6 play the target's hurt voice: type
    0 → 60, 1 → 61, 2 and 43 → 62, 3 and 45 → 63, 4 → 64, 5 → 65, 7 → 66, 8 → 67, 9 → 68,
    10 → 69 (other types: follow-up), pitch 1 + 0.1*r*.
15. The final hit record (kind 0 = damage, with the critical flag at byte 0x44, amount
    *x*, knock-back **k**) is queued for the target's owner (`alpha-net-packets.md` §5.1).

The target's own client applies the record as `alpha-net-packets.md` §5.1 describes.

---

## 6. Sound events (world-update section 4, resolves part of P-12)

The 24-byte positional records are **sounds**: 3 × f32 position (blocks), i32 sound id,
f32 pitch, f32 volume. Ids seen: 24 block/deflect, 32–35 footsteps, 39 and 42 magic hits,
60–69 hurt voices, 91 shield absorb, 1–11 weapon impacts. The full id → `data2.db` key
table is §14.

---

## 7. AI steering (0x5a7eb0, verified part)

NPCs steer by setting their intent and relaxing their velocity, per step of dt ms:

* With a direction **u** (unit, horizontal, toward the next path point or target) and the
  path still long (≥ 4 cells, *inferred*): intent := 40·**u**.
* Otherwise, if the creature is on the ground path: velocity z relaxes toward 4·u_z at
  0.05/ms when u_z ≥ 0 (climbing up); velocity x, y relax toward 6·**u** (12·**u** when
  mode 0x40 running) at 0.001/ms; a flying/swimming variant relaxes toward 8·**u** at
  0.01/ms; if the creature can fly and u_z ≥ 0 its velocity is set to 8·**u** directly.
  Intent := 20·**u**.
* Field 4 := 0.

Target choice, aggro radius, chasing, fleeing and idle wandering are decided by the
behaviour layer before this routine (§13).

---


## 8. Action phases, end of an action, stat values

### 8.1 Phases

The three phases of §3.3 each have their own routine: **P1** 0x43caa0, **P2** 0x447310,
**P3** 0x444270 (checked against the fixed values: action 0 → P2 0, P3 100; action 10 →
P2 200, P3 600). Uses found:

* **P1** = wind-up; the hit lands when the time reaches it (§3.3).
* **P1 + P2** = end of the **active window**. When an action's time passes P1 + P2 (time
  + dt ≥ P1 + P2, time still < the total, no dodge running) and the creature's
  "hit landed in this action" flag (obj +0x13b8, *inferred*) is clear, the **combo counter
  is reset to 0** and the action time is rescaled by *S*_old / *S*_new (*S* of §4.3 before
  and after the reset) so the same fraction of the action has elapsed (0x613448–0x6134ee).
  So an attack that whiffs ends the chain.
* **P1 + P2 + P3/2**: an AI creature whose current action has passed this point may run
  again (§13.2, mode flag 0x40).
* The **total** P1 + P2 + P3: the chaining rule (§3.4) and the end of the action.

### 8.2 Time advance and costs

* Each step the action time += dt (0x618541).
* When an action starts (time = 0) its **mana cost** (0x444ae0, per action id) is taken
  from field 28; any charge (field 23) is also taken and field 23 := 0; mana is floored at
  0 (0x613538–0x61361b). The per-id cost table is not transcribed (open question C-3).
* The AI resets any action other than 79, 82, 83, 84 that has run longer than **10 000 ms**
  (§13.2 step 1).

### 8.3 Projectiles

Specified in **`alpha-projectiles.md`**: the 112-byte shoot record (packet 9 / world-update
section 5), creation by actions 22, 23, 26 (bow, crossbow, boomerang), 50, 55, 96 (four-shot
volleys), 37–46 and 108 (spells, thrown rock), 89, 90 and the class-3 specialization-1 melee
(stationary bursts); the charge fill rate; the per-step flight (gravity 7.5 blocks/s², 5 s
lifetime, boomerang return), sub-stepped creature and block collision, damage, healing,
bursts, mana-on-hit and procs; receiving remote shoots; drawing and lights; test vectors.

### 8.4 Stat multipliers (field 30) and power tier (field 37)

| Who | Field 30 (health, attack speed, damage, armour, resistance) | Field 37 | Where |
|---|---|---|---|
| Any new creature record | **100, 1, 1, 1, 1** | 0 | record initialiser 0x43c100 (which also sets health 500, level 1, field 16 = −3000) |
| Player loaded from `characters.db` | 100, 1, 1, 1, 1 — the save does not store them | 0 | the loader 0x44be40 first calls 0x43c100, then reads the saved fields (`alpha-saves.md` §3.2) |
| Player made on the creation screen | 100, 1, 1, 1, 1 (no write to field 30 was found) | 0 | *inferred* from the same initialiser |
| Player on an Alpha server | the server's record starts the same way; the client's first full update overwrites it anyway (`alpha-net-server.md` §3.1) | 0 | |
| NPC from a spawn object | the spawn object's +0xf58..+0xf68 (default 100, 1, 1, 1, 1; per-type values in §9.3) | spawn +0x58 (0 unless set; mission creatures get the map cell's tier, §9.2) | activation, §9.2 |

**Starting health on load**: the saved health (character record item 4) is restored as
saved, not raised to the maximum. Each step the step clamps health to the current maximum
(0x611a9c: if health > maxHealth then health := maxHealth), and the local player
regenerates 5 %/s. A new player on the Alpha server gets health = maximum
(`alpha-net-server.md` §3.1).

---

## 9. Spawn objects in the world step

### 9.1 Spawn object fields (as read by the step)

| Off | Type | Meaning |
|---|---|---|
| +0x08 | f32 | **activation radius**, blocks (default 200) |
| +0x10 | 3 × i64 | position (becomes position and home) |
| +0x28 | i32 | **controller kind** of the creature (default 1; 6 for deposits) |
| +0x2c | i32 | creature type |
| +0x30, +0x31 | u8, u8 | class, specialization |
| +0x34 | i32 | **level** |
| +0x38 | i32 | **respawn countdown**, ms |
| +0x3c | i32 | day number at death |
| +0x40, +0x44 | i32, i32 | active time-of-day window (start, end); equal = always |
| +0x48 | u64 | creature id |
| +0x50 | u8 | written into mode flag 0x08 |
| +0x54 | f32 | yaw (orientation element 2) |
| +0x58 | u8 | power tier |
| +0x5c, +0x60 | | copied to record 0x1c8 (field 42) and 0x1cc (field 41) |
| +0x74 | 172 bytes | appearance (its flags at +0x7a = appearance +0x06) |
| +0x120 | 13 × item | equipment |
| +0xf58..+0xf68 | 5 × f32 | stat multipliers (field 30) |
| +0xf6c | | inventory (bags of 284-byte slots: u32 count + item) |
| +0x109c | pointer | an optional behaviour object (cloned for the creature, §13.1) |
| +0x10a0 | list | time-of-day position schedule (SpawnLocation behaviour, §13.5) |
| +0x10ac | list | special-action ids (§13.2 step 9) |
| +0x10cc | string | name |

The first-draft guesses of §2.1 are corrected: +0x28 is the controller kind (the deposits'
6 is "inert object"), not a count; +0xf58 = 100 is the health multiplier.

### 9.2 Activation (client 0x60fb48–0x6103e3)

For every loaded zone, for every spawn object of the zone, each step:

1. The countdown +0x38 −= dt (0x60df9f). If the spawn object has a schedule its position
   follows the schedule (§13.5).
2. Skip if a time window is set (+0x40 ≠ +0x44) and the time of day is outside
   [start, end].
3. Skip if a creature with the spawn's id already exists.
4. If the countdown < 0 it is set to 0. Skip if the countdown ≠ 0 **and** +0x3c equals the
   current day number. So a killed spawn returns after **1 200 000 ms (20 min)** or at the
   next in-game day, whichever comes first.
5. Skip unless a creature of the step's player list (the local player offline) is within
   the activation radius: squared distance in blocks < radius² (200 → 40 000).
6. **Mission creatures**: if the location record of the spawn's map cell (zone ÷ 8, see
   `alpha-worldgen-locations.md` §1.2) has mission kind 5 at +0x34 and the spawn's
   appearance has flag 0x1000: (unless controller kind 6) level := cell +0x3c, tier :=
   cell +0x40; `srand(cell +0x30 + id low 32 bits)`; the equipment generator runs in
   activation mode (§9.3). Then, if the appearance has the boss flag 0x200: with spawn
   +0x10e8 set the appearance gets flag 0x2000; otherwise each of the nine equipment slots
   at +0x238..+0xaf8 has its level re-rolled by 0x4c7e80 with spread 0.05 (*inferred*: a
   random level between the inverse of curve(level) − 0.05 and the level). Every inventory
   item of types 3–9 gets level := cell +0x3c and rarity := cell +0x40 + 1, + 1 more when a
   draw mod 20 = 0, + 1 more when a draw mod 100 = 0, capped at 4.
7. Create the creature: id, position, home := spawn position, yaw, level, controller kind,
   type, class, specialization, tier, appearance, equipment, stat multipliers, mode flag
   0x08, name, schedule, specials, inventory as listed in §9.1; health := maxHealth (§4.1).
8. Build its behaviour (§13.1).

### 9.3 NPC equipment generator (0x5c7730)

Input: a spawn object and a mode flag (activation passes 1; the zone generator 0 —
*inferred*). Slot offsets are spawn +0x120 + 0x118·slot; item fields as in §10.1.

* Appearance flag 0x40: nothing.
* Class 0x88: chest (slot 2) type 4, subtype 2 + (draw mod 5), material 6; classes
  0x80–0x87 and 0x89: chest type 4 subtype 0, material 6. Both then set the multipliers
  attack speed 0.3, damage 0.1, armour 10, resistance 10 and stop (non-combat NPCs).
* Otherwise weapon slots 6, 7 and armour slots 1–5 and 8 are cleared. Appearance flags
  0x18, controller kind 6 and type 118: no weapons. Fixed loadouts: type 117 → specials
  92, 17, 96; types 108 and 114 → specials 91, 93, main hand type 3 subtype 17 material 2;
  type 46 → main hand subtype 17 material 2; type 109 → specials 86, 93, 91, main hand and off hand subtype 2,
  material 7; type 81 → special 87, main hand subtype 12 material 11 and off hand 12;
  type 82 → swords (subtype 0) in both hands, material 1; types 47, 111–113 → nothing;
  types 43 and 45 → special 95, specialization 2, main hand subtype 10 + draw parity
  (staff or wand), material 2; type 115 → specials 86, 93, fists (subtype 4) in both
  hands, material 1.
* **Everything else**: class := 1 + (draw mod 4) (type 97 always 4); specialization :=
  draw parity; then by class:

| Class | Draw mod 3 | Main hand (slot 7) | Off hand (slot 6) | Material |
|---|---|---|---|---|
| 1 warrior | 0 | greatsword/greataxe/greatmace (15 + draw mod 3) | — | 1 |
| | 1 | sword/axe/mace (draw mod 3) | another of sword/axe/mace (draw mod 3) | 1 |
| | 2 | sword/axe/mace (draw mod 3) | shield (13) | 1 |
| 2 ranger | — | bow, crossbow, boomerang (6 + draw mod 3) | — | 2 |
| 3 mage | 0 / 1 | staff (10) / wand (11) | — | 2 |
| | 2 | bracelet (12) | bracelet (12) | 11 |
| 4 rogue | — | dagger, fist, longsword (3 + draw mod 3) | same subtype unless longsword | 1 |

  Then, unless appearance flag 0x20, armour of material 25 for wand, staff, bracelet
  users, 26 for bow, crossbow, boomerang, 27 for dagger, fist, longsword, otherwise 1:
  chest (type 4, slot 2) always; shoulders (7, slot 5), gloves (5, slot 4), boots (6,
  slot 3) each on an odd draw; amulet (8, slot 1) and two rings (9, slots 9 and 8) each
  when a draw mod 10 = 0, with material 12 − (1 if the draw is odd). All levels := the
  spawn's level.
* Weapons get level := the spawn's level.
* **Mode 0 only** — per-type stat multipliers (health, attack speed, damage, armour,
  resistance; "·" = unchanged) and specials:

| Types | Multipliers | Specials |
|---|---|---|
| 17, 94, 97 | 400, ·, 1.5, 3, 3 | 93 |
| 21 | 250, ·, 2, 3, 3 | 69 |
| 25 | 200, ·, 0.5, 3, 3 | |
| 46, 82 | 250, ·, ·, 3, 3 | 86 |
| 47, 88 | 250, × 0.5, 3, 3, 3 | |
| 81 | 250, ·, ·, 3, 3 | 92 |
| 85 | 1000, ·, 2, 3, 3 | |
| 86 | (class 3, specialization 1) | |
| 107–109, 111–116, 118 | 2500, 0.5 (0.75 when 0x444820 holds), 6, 5, 5 | |
| 117 | 2500, ·, 5, 4, 5 | |
| 119 | 2500, 0.5, 6, 10, 10 | |

  Boss flag 0x200 (mode 0): health × 15 (capped at 10 000), damage × 3, attack speed ×
  0.75, armour and resistance + 2, specials 92, 93, 86, 89, 87 and more (one random, then
  17 and 96); +0x10b8 := draw mod 3.
* Finally the items in slots 6, 7, 5, 2, 4, 3 each get a seed (+0x04) from one draw, in
  that order.

Example: a level-5 class-1 NPC (draw mod 3 = 2, then sword) carries an iron sword of level
5 and a shield of level 5 and an iron chest of level 5 (§10.4 gives their stats).

### 9.4 Death of a spawned creature (0x595a60)

When an NPC's health reaches 0 its spawn object (found through the creature's home zone and
field 39 element 2) gets countdown := 1 200 000 and death day := today. Kill experience and
mission progress: §12.3.

---

## 10. Item stat formulas

### 10.1 Item fields read

| Off | Type | Meaning |
|---|---|---|
| +0x00 | u8 | type: 3 weapon, 4 chest, 5 gloves, 6 boots, 7 shoulders, 8 amulet, 9 ring (from the slot use, §9.3); 1 consumable (1/1 a health potion, *inferred*); 13 experience (§12.3); 20 pet food (§13.2) |
| +0x01 | u8 | subtype (weapon subtypes as §3.1) |
| +0x04 | u32 | **seed**: a per-item random number that varies the stats |
| +0x0c | u8 | **rarity** *R*, 0..4 |
| +0x0d | u8 | **material** (1, 2, 6, 7, 11, 12, 18, 19, 23, 25–27 seen; 1 iron and 2 wood *inferred*; 25–27 the armour sets of mage, ranger, rogue) |
| +0x10 | i16 | **level** *L* |
| +0x14 | 32 × 8 bytes | **spirits**: per entry x, y, z (3 × i8), material (u8), level (i32) |
| +0x114 | i32 | number of spirits *n* |

This resolves P-1 of `alpha-net-packets.md`: the sub-block list is the item's attached
spirit cubes (byte 3 = material, then the spirit's level).

### 10.2 Level factors

* *F*(*v*, *R*) = 2^(3·(1 − 1/((*v* − 1)·0.05 + 1))) · 2^(0.25·*R*) (0x445f60; the inner
  term in single precision, each power in double rounded to single, product in single).
* *G*(*v*, *R*) = *F*(*v*, *R*) / 8 (0x445ff0).
* *v* = *L* + 0.1·*n* for health, damage, armour, resistance; *v* = *L* for speed and crit.
* Seed terms: *u*₁ = ((seed × 8) mod 2³²) mod 21; *u*₂ = seed mod 21 (unsigned).
* "Two-handed set" W2 = weapon subtypes 5, 6, 7, 8, 10, 11, 15, 16, 17, 18 (as 0x444230).

### 10.3 Formulas

| Stat (routine) | Types | Result |
|---|---|---|
| Health (0x4c70b0) | 3–7 | 5 · *k* · *m* · *F*(*v*, *R*); *k* = 1 for type 4, else 0.5; *m* = 2 − *u*₁/20, + 1 for material 1, + 0.5 for 26, + 0.75 for 27 |
| Damage (0x4c7f60) | 3 | *F*(*v*, *R*) × weapon factor: dagger, fist (3, 4) and shield (13) → 2; bow, crossbow, boomerang, staff, wand, greatweapons, pitchfork (6, 7, 8, 10, 11, 15–18) → 8; everything else (sword, axe, mace 0–2, longsword 5, 9, 12, 14…) → 4 |
| Armour (0x4c6a90) | 4–7 | *k* · *a* · *F*(*v*, *R*); *k* as for health; *a* = 0.8 for material 18; 0.85 for 19, 26, 27; 0.75 for 23, 25; else 1 |
| Resistance (0x4c7af0) | 4–7 | *k* · *a* · *F*(*v*, *R*); *a* = 0.85 for material 1, 19; 0.75 for 26, 27; else 1 |
| Speed (0x4c7c00) | 3–9 | *G*(*L*, *R*) · *k* · *q*; *k* = 0.2 for W2 weapons and type 4, else 0.1; *q* = *u*₂/20, + 1 for material 12; a result < 0.001 gives 0 |
| Crit (0x4c6ba0) | 3–9 | *G*(*L*, *R*) · *k* · *q*; *k* = 0.1 for W2 weapons and type 4, else 0.05; *q* = 1 − *u*₂/20, + 1 for material 11; a result < 0.001 gives 0 |

The damage formula ignores material and seed. The longsword takes factor 4 from its own
case although it is in W2.

### 10.4 Test vectors (single precision)

| Item | Result |
|---|---|
| Iron sword (3/0), level 5, rarity 0, no spirits: damage | *F*(5, 0) = 2^0.5 = 1.4142135 → **5.6568546** |
| same, rarity 2 | **8.0** |
| same, rarity 0, 3 spirits (*v* = 5.3) | **5.7791777** |
| Greatsword level 5 / dagger level 5: damage | **11.313709** / **2.8284273** |
| Iron chest level 5, seed 0 (*u*₁ = 0): health | 5·1·3·1.4142135 = **21.213205** |
| same, seed 1 (*u*₁ = 8) / seed 7 (*u*₁ = 14) | **18.384777** / **16.263456** |
| Iron chest level 5: armour / iron boots level 5: armour | **1.4142137** / **0.7071068** |
| Sword level 5, seed 10: speed / crit | **0.008838835** / **0.0044194176** |
| Sword level 5, seed 20: crit | 0 (raw value 0, below 0.001) |
| Iron sword level 1, any seed: health | between **5.0** and **7.5** (*m* from 2 to 3, *k* 0.5) |

---

## 11. Stamina, mana, stealth (world step 0x619400–0x619dcc)

Per step of dt ms, for every simulated creature. *k*(*n*) = 1 − 1/((*n* + 1)·0.1 + 1) is the
skill factor (0x43e660) of skill level *n* (record 0x1128 + 4·index).

### 11.1 Stamina (obj +0x1194, 0..1)

* Climbing (mode 0x01) with a non-zero intent, *k*(skill 2) > 0, not gliding, touching a
  wall and not on the ground: stamina −= (1 − *k*(skill 2))·0.0005·dt; below 0.01 the
  climbing flag is cleared. With *k* ≤ 0 climbing is cleared at once.
* Gliding (0x10) in the air: stamina −= 0.0002·dt while running (0x40), unchanged
  otherwise. Gliding needs the glide factor of skill 3 > 0, else the flag is cleared.
* Climbing on a wall in the air (as above): no regeneration.
* Otherwise: stamina += **0.0001·dt** (empty to full in 10 s). Clamp to [0, 1].
* Dodging costs 0.25 (`alpha-player-control.md`).
* The movement factor is doubled while running unless climbing on a wall or gliding in the
  air; a buff of kind 12 multiplies it by 1 + (a value of action 100, 0x43ed60).

### 11.2 Mana (field 28)

* mana += **0.00025·dt·*s*** each step (*s* = stealth amount), capped at 1. Mana therefore
  regenerates only while *s* > 0 (sitting, sneaking); at *s* = 1 it fills in 4 s.
* Blocking adds field 29 to it (§5 step 5); actions spend it (§8.2).

### 11.3 Stealth amount *s* (obj +0x1190)

* While sitting (mode 0x400): *s* += dt·(0.0005·*x* + 0.00001), *x* = a value of action 99
  (0x43ed60), capped at 1.
* While sneaking (the branch before 0x619b47; its condition was not fully read): *s*
  changes by 0.0005·dt·(1 − (0.9 − 0.5·*y*)·*N* − …), *y* a value of action 79, *N* = Σ
  (1 − *d*²/100)² over creatures with a lit lantern (mode 0x200) within 10 blocks, capped
  at 1 — *partly read*, open question S-1.
* Otherwise *s* −= 0.0005·dt (gone in 2 s), floor 0. A buff of kind 3 sets *s* := 1.

### 11.4 Examples

* Stamina 0.5, standing for 2000 ms → 0.7.
* Climbing with skill 2 at level 0: *k* = 0.0909091; 1000 ms drains 0.4545.
* Sitting with *s* = 1 for 1000 ms: mana + 0.25.

---

## 12. Spirits, life steal, kill rewards

### 12.1 Spirit bursts (0x59a800)

For each weapon of the attacker (0x43df60) and each spirit whose material ≥ 0x80: a
particle burst at the target's position, moving along 4 × the horizontal hit direction
with z = 5 (both doubled with the area flag), count ⌊3·*ρ* + 1⌋ (+ 10 with the area flag),
colour from the material (0x4c7250) at strength 0.5·*ρ*. Visual only.

### 12.2 Spirit effects on the hit (0x596d30, area flag set and *ρ* > 0)

With *w* = 0.1 / (number of attacker weapons) and, per spirit, *c* = 1 − 1/((ℓ − 1)·0.05 + 1)
for the spirit's level ℓ, after armour (§5 step 7) the damage *x* grows by:

| Spirit material | Damage | Other |
|---|---|---|
| 0x80 (fire *inferred*) | + *c*·*w*·*ρ* | |
| 0x81 (unholy *inferred*) | + 0.5·*c*·*w*·*ρ* | **life steal** *h* += 2·*c*·*w*·*ρ* |
| 0x82 (ice *inferred*) | + 0.25·*c*·*w*·*ρ* | target field 18 += ⌊500·*ρ*⌋ ms |
| 0x83 (wind *inferred*) | + 0.25·*c*·*w*·*ρ* | attacker field 19 += ⌊500·*ρ*⌋ ms |

Fields 18 and 19 are capped at 5000. If *h* > 0 the attacker's health += *h* and a hit record
with amount −*h* on the attacker is queued (the heal number).

Example: one weapon with one unholy spirit of level 21 (*c* = 0.5), *ρ* = 1: damage + 0.025,
life steal 0.1.

### 12.3 Kills and missions (0x595a60, 0x5a0bf0)

When a hostile NPC (controller 1) is killed:

* **Kill experience** *e* = max(1, 20·*c*(*L* + *t*)), *c* the curve above, *L* the level, *t*
  the tier; × 20 for types 101, 107, 108, 109, 114, 115, 116, 117, 118, 119 (0x4445d0);
  × 10 with the boss flag 0x200; 0 with appearance flag 0x800. For every player in the
  victim's threat map with threat > 0 a **kill record** (u64 player id, u64 victim id,
  i32 victim type, i32 ⌊*e*⌋; 24 bytes) is queued; the client of that player adds ⌊*e*⌋ to
  its experience and runs the level-up loop (§4.6); offline, the player's pet gains it too.
* **Mission progress**: if the victim's map-cell location has a mission, the victim is
  flagged (appearance 0x2000, or mission kind 5 with the creature's +0x1d38 set) and the
  mission is not done (cell +0x41 ≠ 2): kinds 1–5 and 13 complete at once; kinds 7–12
  count kills (+0x44) and complete when the count reaches +0x48. On completion each
  participating player gets an **experience item** (type 13, level ⌊50·*c*(*L*m + *t*m) + 1⌋
  for the mission's level and tier → experience += 10 × that level) and one reward item
  from 0x5f51e0 or 0x5f8ad0 (even/odd draw), made for the player's class.
* No per-monster **loot drop** was found on the death path; 0x5c8390 is a zone-generation
  routine (§12.4).

| Victim | Kill experience |
|---|---|
| level 1, tier 0 | 1 |
| level 5, tier 0 | 3 (20·0.1666667 = 3.33) |
| level 10, tier 2 | 7 |
| level 20, tier 0 | 9 |
| level 5, boss flag 0x200 | 33 |

So a level-1 player (cost 50) needs 50 level-1 kills or 17 level-5 kills; completing a
level-5, tier-0 mission gives an item of level 9 → 90 experience.

### 12.4 Item lists of 0x5c8390

Called once from the zone generator (0x5bd02f). For tier *i* = 0..29 it draws an item level
between ⌊*L*(*i*/30)⌋ and ⌊*L*((*i* + 0.99999)/30)⌋, where *L*(*c*) = (1/(1 − *c*) − 1)·20 + 1
is the inverse of the curve (0x4462f0); at least 1; tier 0 always gives level 1. It then
builds candidate items of types 7, 4, 5, 6, 8, 9 (material 1) each with rarity (draw mod
100 = 0) + (draw mod 1000 = 0) + (draw mod 10000 = 0) after one discarded draw (at most 3;
the cap of 4 never bites) and a seed draw, and later picks one to three of them. Level
ranges: tier 1 → 1–2, 5 → 5, 10 → 11–12, 15 → 21–23, 20 → 41–47. What consumes the lists
(chests, shops) is open question L-1.

---

## 13. AI behaviour layer

### 13.1 Who runs it and how often

* The step runs NPC AI only when the world's byte +0xb4 is 0 (0x611af1): offline the
  **client** runs every NPC; when connected (*inferred*: +0xb4 is the online flag, set by
  0x58eb00, cleared by 0x554130 and 0x554340) the client runs no NPC AI and the **server**
  does (`alpha-net-server.md` §5). AlphaVulkan: the server, or the host in single player.
* Each step, for an NPC with health > 0: dt is added to an accumulator (obj +0x13c4); when
  the hit-stun counter (field 16) ≤ 0 and the accumulator ≥ **32 ms**, the behaviour's
  update runs with the accumulated time and the accumulator is reset. Then, if not
  stunned: mode flag 0x80 cleared, flag 0x04 cleared, the steering of §7 runs; if the
  creature has no path the flags 0x04 and 0x80 are set again (0x611b04–0x611cfc).
* Type 144 loses 0.025·dt health per step (a temporary creature).
* The behaviour built at activation (0x6101fc): the spawn's own behaviour if it has one;
  none for controller kind 6; otherwise a **sequence** of **Combat** (radius 20),
  **LookAtPlayer**, and — unless controller kind 6, class 0x80–0x87, or appearance flag
  0x40 at +0x7e — **RandomWalk**. The sequence calls them in order and stops at the first
  that reports "handled"; if none does, intent := 0 (0x4dc390). Which of them report
  "handled" was not settled (open question A-1).

### 13.2 Combat (0x42cb20)

State: special timer (starts at 20 000 ms), radius 20, chase time, cooldown.

1. Clear flag 0x04. Reset to action 0 any action other than 79, 82, 83, 84 older than
   10 000 ms.
2. Stop if dodging, or during the first 3000 ms of action 80.
3. **Potion**: health < 25 % of maximum and the current action finished: if the inventory
   holds an item of type 1, subtype 1, use one (action 80, time 0) and set a random intent
   (4·*r* − 2 on each horizontal axis); stop.
4. **Target** = the creature with the highest threat in the threat map. Pets (controller
   5) take the attacker of their owner with the highest threat; a pet does nothing while
   its owner's action is 106.
5. No target, controller kind 3 and health < 50 %: drink (action 80; the item slot of
   field 43 := type 1 subtype 4 at the creature's level), intent 0; stop.
6. Special timer −= dt (floor 0). "Pacified" = the creature has a buff of kind 8.
7. **Acquire** (no target, not a pet with an owner, cooldown ≤ 0, not pacified): among all
   creatures other than itself with health > 0 and controller ≠ 5, the nearest whose
   effective squared distance is below **20² = 400**, which is hostile (below) and in line
   of sight (0x59ee90, 200-block limit) — and only if this creature is not a **harmless
   animal** (0x444680: types 19, 20, 22, 23, 25–27, 30–35, 53, 55–58, 67, 68, 74, 87, 89,
   92, 93, 98, 100, 106, 107, 145–147, unless appearance flags 0x1a00), except for taming.
   Stealth: the candidate's squared distance is divided by *f*², *f* = max(0.1, 1 −
   1.5^(*L*candidate − *L*self)·*s*). The chosen creature gets threat 1 and is announced
   (0x5a0970). **Hostility** (0x596ca0): a pet against a player, or anything with
   controller 6 → no; controller 1 against a non-1 → yes; 1 against 1 → yes when exactly
   one of them is a harmless animal; otherwise yes only if either has mode flag 0x20.
8. Mode flag 0x40 (running) := a target exists and (no action, or the action time > P1 +
   P2 + P3/2). Cooldown −= dt. No target → chase time := 0; stop.
9. **Attack range** *R* = 2·(own box x) + (target box x); 50 for ranged users (0x444520:
   class 3, types 117, 86, 104, or a staff, wand, bracelet, bow, crossbow, boomerang)
   with line of sight (unless 86, 87 or 89 is still in its wind-up); 20 for types 47,
   111, 112, 113; 50 during actions 95, 28, 108; own box + target box during 72. If the
   special timer is 0 and the creature has specials, one is drawn now (93, 91, 68, 69 →
   *R* = 20; 86, 89 → 2·own + target box). No line of sight → *R* = 1.
10. **Chase** (distance > *R*, or no sight): chase time += dt. Up to **20 000 ms**: stop
    actions 95 and 28; direction to the target (the vertical part kept only in water);
    intent := **40** × unit direction (80 only if *R* ≤ 0.5); with sight and a clear route
    (0x609480) and no contact flag 0x20: face the target and stop; the target's threat ×
    0.9; start a path search to the target's feet if none is running. After 20 000 ms:
    **give up** — clear the threat map, cooldown := **3000 ms**, chase time := 0.
11. **In range**: chase time := 0, face the target, intent := 0, flag 0x04 set, look vector
    (field 26) := target − self in blocks. Then the first that applies:
    * **Taming**: the target is in action 82 holding item type 20 whose subtype is this
      creature's type: after the current action, action 110 up to five times, then the
      creature becomes the player's pet (0x42c520) and gets a buff of kind 8 for 20 000 ms.
    * A drawn special: action := it, time 0, special timer := 20 000.
    * Types 116, 118, 47/111/112/113, 119 and 101 choose from fixed action pools when the
      current action ends (116: 69, 74, 73; 118: 7, 6, 20, and 95 or 86; 47, 111–113: 108,
      54 or 67 — 54 also launches upward at 15; 119: by its armour models; 101: casts 37
      between 64 and 3600 squared blocks, closer it moves and jumps).
    * Magic users (0x444610): within 60 blocks (d² < 3600): primary attack (§3.4) when mana
      ≤ 0.5 or on an even draw, else the secondary attack (0x4456a0).
    * Natural-weapon creatures (appearance 0x02 and 0x10) hop toward the target every 2000
      ms with action 51; flag 0x10 alone → action 75 (type 25: 72 if it can); flag 0x08 →
      primary attack.
    * **Melee** (no bow, crossbow, boomerang; not type 104): within 3·(own + target box),
      one draw in 10 — if not a big type (0x4445d0), own box < 2, the action finished, on
      the ground or in water — **dodges**: velocity := 20 × one of {toward, away, left,
      right} + 5 up, dodge time 600 ms. Else mana < 0.8 → primary attack. Else, when the
      action finished, a mana ability: longsword → 5; shield → 8 (charge) then 104 once the
      charge ≥ mana; dagger off hand → 17; fist off hand → 20; no weapon → 54 (jump, + 20
      up) or the secondary attack; two one-handers → charge 63 then 11 (time := P1); a
      two-handed weapon → charge 59 then 61 (specialization 1) or 60.
    * **Ranged** (bow, crossbow, boomerang, or type 104) within 60 blocks: the same dodge
      draw; mana < 0.8 → primary attack; otherwise charge then release: crossbow 24 → 22,
      boomerang 27 → 26, bow 25 → 55.

Worked example: a level-5 warrior NPC with sword and shield, a player 15 blocks away in
sight: acquired (225 < 400); *R* = 2·0.96 + 0.96 = 2.88 (default boxes); chases with
intent 40 toward the player; when within 2.88 blocks it stops, faces, and with mana < 0.8
chains 10 → 9 (shield rule of §3.1); if the player escapes for 20 s it gives up and waits
3 s before acquiring again.

### 13.3 LookAtPlayer (0x4c8140)

Clear flag 0x04. The nearest **player** (controller 0, health ≥ 0) within **8 blocks**
(d² < 64): unless the action is 83 or 84, set flag 0x04, look vector := player − self,
intent := 0, clear the path.

### 13.4 RandomWalk (0x4dbf20)

Clear running. Timer −= dt; when it reaches 0: if within **10 blocks** of home
(horizontal d² ≤ 100) intent := 10·(2*r* − 1, 2*r*′ − 1, 0) (two draws); otherwise intent :=
10 × the unit horizontal vector toward home. Timer := 3000 + (draw mod 5000) ms; action := 0.

Example: draws 0 and 32767 → intent (−10, 10, 0), held for 3000–7999 ms.

### 13.5 Other behaviours

* **SpawnLocation** (0x4dfad0): a schedule of (position, time of day) entries; when the
  time of day passes an entry the creature paths to that position (searching up to 11
  blocks up and down for standable ground) — town NPC routines.
* **Companion** (0x43ace0), **RandomInteraction** (0x4db130), **WalkPath** (0x58d050):
  same interface; not specified (A-2).

### 13.6 Fleeing

There is no general flee: retreat happens only through the potion wobble (§13.2 step 3),
the dodge draw, the 20-second chase give-up and the RandomWalk leash home.

---

## 14. Sound ids (0x484350)

The play routine switches on the id (jump table 0x485e90) and asks the asset store for the
file name below; the name prefix is an empty string, so the `data2.db` key is exactly the
name.

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

Ids above 100 play nothing. The hurt voices of §5 step 14 (types 0–10 → 60–69) follow the
races (elves and humans share the plain male/female groans). Ids 70–80 are the natural
voices for orcs, undead, frogmen and monsters, but the mapping for types beyond 10 is
still F-8b.

---

## 15. Open questions

* **B-1** Shield ×4 in the creature damage formula (§4.2).
* **B-2** Hit record byte 0x44 = critical flag and kinds 0/1/3/5.
* **F-2** Creature placement of the zone post passes (towns, castles, dungeons).
* **F-3b** *Resolved* in `alpha-projectiles.md` (record, creation, flight, hits, drawing,
  test vectors). Remaining points are listed in its §9 (ribbon trail details, the pet-id
  offset, two *as read* double-hit behaviours to confirm in game, the starters of actions
  89/90). Correction: staff actions 30–33 do not create projectiles (they are melee; only
  the class-3 specialization-1 burst of `alpha-projectiles.md` §2.6 applies to them).
* **F-5** Callers of the weighted chance test 0x4441a0.
* **F-8b** Hurt voices for creature types beyond 10.
* **A-1** Return values of the Combat, LookAtPlayer and RandomWalk updates (which stop the
  sequence).
* **A-2** Companion, RandomInteraction and WalkPath; who sets a spawn's own behaviour
  (+0x109c).
* **C-3** The level re-roll 0x4c7e80 (the mana cost per action, 0x444ae0, is now in §16.4).
* **S-1** The sneaking branch of the stealth update (§11.3).
* **L-1** The consumer of the item lists of 0x5c8390; the reward item generators 0x5f51e0
  and 0x5f8ad0.
* **N-1** The online flag world +0xb4 (§13.1), and which world-update sections carry kill
  records (24 bytes; sections 10 and 11 likely) and item rewards (288 bytes; section 9
  likely).

Verification without running anything here: with the mod DLLs, log record fields 9–11, 16,
27–30, 33, 34, 37, obj +0x1190 and +0x1194, and the hit, kill and reward records of a fight;
the durations of §3.3 and the formulas of §4–§13 predict them. The item formulas can be
checked against the inventory tooltip (0x4a28c0 calls all six).

---

## 16. Player abilities: slots, use, cooldowns

How the local player's ability bar (`alpha-ui-screens.md` §5.5) is filled and what pressing
its controls does. Read from the per-frame GUI/game routine 0x488ee0 (slot list
0x48b8ab–0x48bd8a, use 0x49b58e–0x49c02a; that part of the routine has no C output and
several switch targets were not in the listing, so they were disassembled from the binary)
and from 0x43e5a0, 0x43e6a0, 0x43ed60, 0x43c9c0, 0x444ae0, 0x4456a0, 0x595850 and the world
step (0x611f41–0x6120e0, 0x6184c8–0x618541). Offsets are **object** offsets ("obj +x",
= record offset + 0x10) unless a record field is named. *P* is the player's creature
object (G+0x8006d0); *G* is the game object, whose controller flags are G+0x04 … (the
flags of `alpha-ui-screens.md` §11.1).

### 16.1 Values used

* **Ability ids** share the action-id numbering (§3, icon names in
  `alpha-ui-screens.md` §5.5). An ability id is stored into the action field (record field
  9) as a **byte**.
* **Skill of an ability** (0x43c9c0): skill 6 for 21, 34, 48, 54, 88; skill 7 for 50, 79,
  86, 103; skill 8 for 49, 96, 97, 99, 100, 101, 102; none (−1) for every other id. Skill
  *i*'s level is record 0x1128 + 4·*i* (field 46), so skills 6, 7, 8 are record 0x1140,
  0x1144, 0x1148.
* **Ability value** *v*(id) (0x43ed60, called with level −1): if the id has no skill,
  *v* = 1. Otherwise *n* = that skill's level; if *n* = 0 and the creature is not a player
  (controller kind, field 7, ≠ 0), *n* := ⌊level/2⌋ + power tier + 1 (integer division,
  level = field 33, tier = field 37). Then **v = 1 − 1/(0.1·n + 1)** in single precision.
  (This differs from the skill factor *k* of §11, which uses *n* + 1.) Values: *n* = 0 → 0,
  1 → 0.0909091, 2 → 0.1666667, 5 → 0.3333333, 10 → 0.5.
* **Aim vector** = record field 26 (0x150, 3 × f32, blocks). Each frame, before the ability
  code, 0x488ee0 sets it to (a world point − the player's position) converted to blocks
  (0x49b55e–0x49b589); the point is the one under the crosshair (*inferred*). With Left
  Shift held it is instead built from the camera axes (0x49b4d3–0x49b557), and a separate
  branch moves it with the movement keys (0x49b250–0x49b4c1, *meaning not traced*).
* **Cooldown table** obj +0x139c: a `std::map` from ability id (i32) to **remaining
  milliseconds** (i32). Client-only; it is not part of the creature record and is never
  sent.

### 16.2 The slot list (rebuilt every frame, 0x48b8ab–0x48bd8a)

The list (G+0x800814) is cleared and ids are appended in order, so its index is the slot.
*M* = subtype byte of equipment slot 7 (main hand, obj +0xaa9), *O* = subtype byte of slot
6 (off hand, obj +0x991). Only the subtype bytes are read (not the item type), except in
the two-handed test 0x444230, which also requires slot 7's type to be 3.

**Slots 0 and 1 (weapon):**

| Test, in this order | Slot 0 (M1) | Slot 1 (M2) |
|---|---|---|
| *M* = 10 (staff) | spec 1: 32; else 30 | spec 1: 33; else 31 |
| *M* = 12 (bracelet) | spec 1: 42; else 40 | spec 1: 43; else 37 |
| *M* = 11 (wand) | spec 1: 44; else 38 | spec 1: 45; else 46 |
| otherwise | the primary attack **now** (0x445020, §3.1) | first match below |

Slot 1 in the "otherwise" row, first match: *M* = 5 (longsword) → **5**; *M* = 3 (dagger)
→ **17** (ambush); *M* = 4 (fist) → **20** (kick); *M* = 6 or *O* = 6 (bow, tested in the
order *M*, *O*) → **25** (salvo); *M* = 7 or *O* = 7 (crossbow) → **24** (charge-shoot);
*M* = 8 (boomerang) → **27** (charge-boomerang); two-handed (0x444230: type 3 and subtype
15, 16, 17, 5, 10, 11, 18, 8, 6 or 7; only 15–18 can still reach this test) → **59**
(charge-slam); *O* = 13 (shield) → **8** (block); otherwise **63** (swirl).

The weapon rows do not look at the class: a warrior holding a staff gets the staff ids.
Slot 0's id is only what the bar shows; M1 itself always runs the chaining rule (§16.3).

**Slots 2–4 (class, obj +0x140; specialization obj +0x141):**

| Class | Slot 2 (key 1) | Slot 3 (key 2) | Slot 4 (key 3) |
|---|---|---|---|
| 1 warrior | 54 smash | 86 cyclone | spec 0: 102 warfrenzy; else 101 bulwark |
| 2 ranger | 21 ranger-kick | 50 retreat | spec 1: 100 swiftness; else 99 aim |
| 3 mage | spec 1: 34 healing-stream; else 88 fire-explosion | 103 mana-shield | 49 teleport |
| 4 rogue | 48 intercept | 79 sneak | spec 0: 97 camouflage; else 96 shuriken |
| other | — | — | — |

So the list has 5 entries for classes 1–4 (2 otherwise); **slot 5 (key 4) never has an
ability** in Alpha and its button shows `none`.

**Skill test (0x48bd3e–0x48bd8a).** For each index *i* from 2 to the list length − 1: if
the level of skill *i* + 4 (record 0x1140 + 4·(*i* − 2)) is **≤ 0**, the id at *i* becomes
0. Slot 2 ↔ skill 6, slot 3 ↔ skill 7, slot 4 ↔ skill 8 (slot 5 ↔ skill 9 would apply if
the list were longer). This matches the ability → skill map of §16.1.

### 16.3 Using the bar (0x49b58e–0x49c02a, once per frame after the slot list)

The whole block is skipped while the GUI holds the input (a local flag set by the window
code) or G+0x800704 ≠ 0. Then:

0. **Flag G+0x18** (a control not in `alpha-ui-screens.md` §11.1; its key was not
   traced): on the frame it goes down (previous state kept at 0x76b14e), if the action is
   0, or its time is past the action's total duration (0x43d1a0), action := **105**,
   time := 0.
1. **Slots 1 … length − 1** (M2, keys 1–3; control flag G+0x04 + *i*: +0x05 right mouse,
   +0x06 … +0x08 keys 1–3). For each slot with a non-zero id:
   * **Control held:**
     1. Mark "a slot was used this frame" (even if the steps below fail).
     2. Slot 1 (M2) only: the player must be **free to act** (0x43e350, below); slots
        2–4 skip this test.
     3. The ability must be **usable now** (0x43e5a0, §16.4).
     4. If equipment slot 12 holds an item of type 20 (*meaning unknown*), that item is
        put back into the inventory (obj +0x11dc) and the slot emptied.
     5. Act on the id (**press table** below).
   * **Control not held** and the current action equals the slot's id: **release table**
     below.
2. **Beams.** If M2 is up, slot 1's id is 28 and the action is 28 → action := 0. If M1 is
   up, slot 0's id is 29 and the action is 29 → action := 0. (No Alpha slot rule produces
   28 or 29, so this is unreachable *as read*.)
3. **M1.** If no slot was used this frame, the action is not 48 (intercept) and M1 is
   held: run the chaining rule (0x445410, §3.4), then the slot-12 item rule of step 1.4.
   Mana, cooldown and the free-to-act test are **not** checked for M1.
4. If a slot was used this frame: clear mode flag 0x01 (climbing allowed) in field 14.

**Free to act (0x43e350).** False if the action is 48. Otherwise true when the charge
(field 23) is ≤ 0 **and** either the action time ≥ its total duration, or the action is one
of 0, 1, 2, 3, 4, 6, 7, 9, 10, 12, 13, 14, 15, 16, 18, 19, 22, 26, 28, 29, 31, 33, 34, 37,
45, 46, 57, 58, 65, 66, 67, 86, 98, 101 (the ordinary attacks and casts, which a new
ability may cut short). False otherwise.

**Press table** (the control is held; "start *x*" = action := *x*, time := 0, dodge time
(field 15) := 0):

| Id(s) | What happens |
|---|---|
| 3, 4, 23, 31, 33, 37, 45, 46, 65, 66, 94, 95 | the **secondary-attack starter** 0x4456a0 (below); the id actually started is §3.2's, not the slot's |
| 5, 17, 20 | if mana > 0: start the id |
| 8, 24, 25, 27, 59, 63, 64 (hold-to-charge) | if the action is not already this id: start it (once; holding keeps it running) |
| 10 | if the action is not 10: start 10 |
| 11 | start 11 (every frame while held) |
| 28 | if mana > 0 and the action is not 28: action := 28, time := 0 (dodge time untouched) |
| 34 healing-stream | if the action time ≥ the current action's total duration: action := 34, time := 0, record field 36 (0x190, u64) := the player's own id (the stream's target, *inferred*) |
| 48 intercept | action := 48; velocity (field 2) := aim vector; obj +0x11d8 := ⌊\|aim\|/50·1000⌋ (travel time in ms, *inferred*; its reader was not found); if the velocity is non-zero: velocity := 50 × its unit vector, and if velocity.z > 5 then velocity.z := 5; time := 0, dodge time := 0 |
| 50 retreat | no action. Field 4 (push vector) := −aim with z := 0; if non-zero: field 4 := (15·*v* + 20) × its unit vector and field 4.z := 12·*v* + 5 (*v* = *v*(50)). Then cooldown[50] := length(50) (§16.5) |
| 54 smash | action := 54; velocity := 2 × aim; obj +0x11d8 := ⌊\|velocity\|/50·1000⌋; if \|velocity\|² > 2500: velocity := 50 × unit; velocity.z := 15; time := 0, dodge time := 0 |
| 96 shuriken | action := 96; velocity := −aim with z := 0; if non-zero: velocity := 30 × unit, velocity.z := 20; time := 0; dodge time := **600** |
| 97, 100, 101, 102, 103 | **buff** (0x595850, §16.6); 101 also plays sound 0x5d and 103 sound 0x5c at the player (volume 1, pitch 1, §14) |
| 99 aim | set mode flag 0x400 in field 14; cooldown[99] := length(99). (§11.3 reads ability 99's value while 0x400 is set; what clears the flag was not traced) |
| any other id (1, 2, 21, 43, 49, 79, 86, 88, …) | if mana ≥ cost(id) (§16.4): start the id |

**Release table** (control up, action = slot id):

| Slot id | Becomes |
|---|---|
| 8 block | action := 104, time := 0 |
| 24 charge-shoot | action := 23, time := 0 |
| 25 salvo | action := 55, time := 0 |
| 27 charge-boomerang | action := 26, time := 0 |
| 36 | action := 37, time := 0 |
| 35 | action := 0, time := 0 |
| 59 charge-slam | spec 1: action := 61, time := 0; else action := 60, time := P1(60) (its wind-up is skipped) |
| 63 swirl | action := 11, time := P1(11) (= 0) |
| 64 charge-mutilate | action := 16, time := P1(16) |
| 10 | only if the time is past action 10's total duration: action := 3, time := total duration of 3 |

(Ids 10, 11, 28, 35, 36, 64 are handled but never placed on the Alpha bar.)

**Holding and charging.** A hold-to-charge ability is started once and keeps running while
the control is held. If it is still running when the control is released, the release
action replaces it at once, so the hold time is whatever time the held action reached. If
the held action reaches its total duration (§3.3) first, it ends (action 0) and, the
control still being held, is started again from 0 the next frame. The charge gathered
during the hold is field 23, spent with the next action (§8.2); the rate at which the step
fills it was not traced (*inferred* from §8.2 and the free-to-act test).

**Secondary-attack starter (0x4456a0).** *D* := total duration of the current action; if
the combo counter (field 11) ≠ 0, *D* −= the current action's recovery (an inline copy of
P3, §8.1). *x* := the secondary attack (§3.2). Let *R* = (class 4, spec 1 and *x* is 5, 17
or 20). Refuse if time < *D*, or the dodge time ≠ 0 and not *R*, or mana < cost(*x*).
Otherwise action := *x*, time := 0; if *x* is 31, 33, 37, 46 or 95 and the buff list has a
kind-9 entry, time := P1(*x*) instead (the cast's wind-up is skipped); unless *R*, dodge
time := 0. (So a rogue assassin may use these three while dodging, and the dodge goes on.)

### 16.4 "Usable now" (0x43e5a0) and mana cost (0x444ae0)

An ability *id* is usable now when **all** hold:

1. the hit-stun counter (field 16) is **< 1**, or *id* = 101 (bulwark may be used while
   stunned);
2. **cost(id) ≤ mana** (field 28);
3. the cooldown table has **no entry** for *id*, or its entry is **exactly 0**;
4. for *id* = 28 only: mana > 0.

The bar greys an icon by the same test (`alpha-ui-screens.md` §5.5 rule 2).

**Mana cost** cost(*id*) (fraction of the full 0..1 mana bar):

| Id | Cost |
|---|---|
| 3, 4 | 0.1 |
| 31, 33, 37, 43, 45, 46, 95 | 0.3; **0** while a buff of kind 9 is active |
| 34 healing-stream | (1 − 0.75·*v*(34))·0.125 (skill 6: level 0 → 0.125, level 2 → 0.109375) |
| every other id | 0 |

This resolves the cost half of open question C-3. The cost is taken from field 28 by the
step when the action starts (§8.2), not by the bar code. Many abilities with cost 0 still
demand mana > 0 at the press (press table: 5, 17, 20, 28).

### 16.5 Cooldowns

**Length** length(*id*) (0x43e6a0, ms, truncated toward zero; *v* = *v*(*id*), §16.1):

| Id | Length | Skill |
|---|---|---|
| 21 ranger-kick | 8000 − 8000·*v* | 6 |
| 48 intercept | 20000 − 12000·*v* | 6 |
| 49 teleport | 16000 − 10000·*v* | 8 |
| 50 retreat | 16000 − 10000·*v* | 7 |
| 54 smash | 20000 − 14000·*v* | 6 |
| 72 | 15000 (fixed) | — |
| 86 cyclone | 60000 − 40000·*v* | 7 |
| 88 fire-explosion | 8000 − 8000·*v* | 6 |
| 96 shuriken | 20000 − 14000·*v* | 8 |
| 97 camouflage | 60000 − 30000·*v* | 8 |
| 99 aim | 12000 − 10000·*v* | 8 |
| 100 swiftness | 60000 − 30000·*v* | 8 |
| 101 bulwark | 60000 − 30000·*v* | 8 |
| 102 warfrenzy | 60000 − 30000·*v* | 8 |
| 103 mana-shield | 20000 − 10000·*v* | 7 |
| any other | 0 (no cooldown) | |

Each product and difference is single precision. Examples (skill level 2, *v* = 0.1666667):
smash 17666, warfrenzy 55000, ranger-kick 6666, retreat 14333.

**Recording.** cooldown[*id*] := length(*id*) is written in three places:

* **When an action starts** (world step 0x6184f2–0x618533, every creature except
  controller kind 6): if the action time is exactly 0 at that point of the step,
  cooldown[action] := length(action) (an entry of 0 for actions without a cooldown). This
  covers everything the press table *starts* (21, 48, 49, 54, 86, 88, 96, …). It runs after
  the hit-stun rule of the same step (counter −= dt; if still > 0 → action := 0,
  time := 0), so a stunned creature only records cooldown[0] = 0.
* **Directly by the press**: retreat (50) and aim (99), which start no action.
* **By the buff routine** 0x595850 for 97, 100, 101, 102, 103 (§16.6).

**Countdown** (world step 0x612048–0x6120e0, every creature, every step): each entry −= dt,
floored at 0. Entries are never removed, which is why "usable" tests for exactly 0. (A
second pass at the combo reset, 0x611f9f, would re-arm entries for which 0x687d10 returns
true; 0x687d10 always returns false, so it does nothing.)

The bar's cooldown overlay is remaining ÷ length(*id*) (`alpha-ui-screens.md` §5.5 rule 3).

### 16.6 Buff abilities and the "event 40" record (0x595850)

Called with (player, id, the frame's event output). If cooldown[*id*] > 0 nothing happens.
Otherwise, by *id* (*v* = *v*(*id*)):

| Id | Buff kind | Value (f32) | Duration (ms) | Extra |
|---|---|---|---|---|
| 97 camouflage | 3 | 0 | ⌊12000·*v* + 8000⌋ | (kind 3 sets the stealth amount to 1, §11.3) |
| 100 swiftness | 12 | 1 + *v* | 10000 | (attack-speed buff, §4.3) |
| 101 bulwark | 1 | 0.75 − 0.3·*v* | 10000 | if the hit-stun counter > 0 it is set to 0 |
| 102 warfrenzy | 2 | 0 | 10000 | |
| 103 mana-shield | 6 | 2·(1 + *v*)·*q*, *q* = 0x446e80 (a level- and damage-multiplier-based stat of the player, not further traced) | 30000 | absorbing shield, reduced by hits of kind 5 |
| 98, 99 | — | — | — | no buff and no record, only step 3 (the bar never calls it with these) |

1. The buff (kind, value, duration) is added to the player's own buff list (0x446af0: for
   kinds 1, 2, 3, 6, 7, 9, 10, 11 an existing entry of the same kind is overwritten, any
   other kind is appended).
2. A **40-byte record** is appended to the frame's event-40 output: u64 player id
   (+0x00), u64 player id again (+0x08, the target), u8 kind (+0x10), 3 padding bytes (not cleared), f32 value
   (+0x14), i32 duration (+0x18), 12 bytes 0 (+0x1c). When online this is sent as
   **packet 8** ("event 40", `alpha-net-packets.md` §4.2: the client sends every such
   record whose first 8 bytes are its own id) and the server relays it to the other
   clients — which answers the layout part of open question P-9 for this producer. (That
   the output list here is the one the packet-8 sender reads is *inferred* from the shared
   append routine 0x4460a0 and the send loop 0x49c03c–0x49c256.)
3. cooldown[*id*] := length(*id*).

### 16.7 Network effects

Nothing else is sent by the bar code itself. The changes it makes to the player's record —
action and time (fields 9, 10), velocity (2), push vector (4), mode flags (14), dodge time
(15), field 36 and the mana spent at the start (28, 23) — go out in the player's next
creature update (packet 0), since the client owns its player. Projectiles and hits that
the started actions produce later are packets 9 and 7. The cooldown table, the slot list
and the aim vector's source point are local.

### 16.8 Q (quick item)

Q is handled elsewhere in 0x488ee0 (0x4920ae–0x492145), not by the bar code above: on the
frame flag G+0x17 goes down (previous state at 0x76b100), the selected quick item is looked
up in the inventory (obj +0x11dc); if its level (item +0x14, i16) is above the player's
level (field 33) sound 0x32 is played at G+0x140 (the listener, *inferred*) and nothing else happens; otherwise
the item is used (0x4a2780, *not traced here*). No cooldown or mana test applies.

### 16.9 Worked examples

**Level-5 warrior, specialization 0, one-handed sword (*M* = 0), no shield; skill levels:
6 → 2, 7 → 0, 8 → 1.**

* Slots: 0 = primary attack now (§3.1 rule 10: 1, or 2 just after a 1); 1 = 63 swirl;
  2 = 54 smash; 3 = 0 (skill 7 is 0: cyclone emptied); 4 = 102 warfrenzy; 5 = none.
* **Key 1** (smash), held with the aim vector (6, 8, 0), mana 0, no stun, no cooldown entry:
  no free-to-act test for slot 2; usable (cost 0 ≤ 0, no entry). Action := 54, velocity :=
  (12, 16, 0), travel time ⌊20/50·1000⌋ = 400; |velocity|² = 400 ≤ 2500 so no clamp;
  velocity.z := 15 → (12, 16, 15); time 0, dodge 0. The same frame's step sees time 0 and
  writes cooldown[54] := 20000 − 14000·0.1666667 = **17666**. While the key stays held the
  next frames fail "usable" (entry > 0), so nothing more happens; the icon is grey and the
  overlay starts full and shrinks over 17.666 s.
* **Key 3** (warfrenzy, skill 8 level 1, *v* = 0.0909091): buff kind 2, value 0, 10000 ms on
  the player, a 40-byte record (packet 8 when online), cooldown[102] := ⌊60000 −
  30000·0.0909091⌋ = **57272**.
* **Key 2**: slot 3 is 0, nothing happens. **Key 4**: never anything.
* **M2** (swirl, 63): needs free-to-act (e.g. the current action is 1 or 0, charge ≤ 0);
  action := 63 while held; on release while still in 63 → action 11 with time := P1(11) = 0.

**Level-2 mage, specialization 1, wand (*M* = 11); skills 6 → 2, 7 → 1, 8 → 0.**
Slots: 0 = 44, 1 = 45, 2 = 34 healing-stream, 3 = 103 mana-shield, 4 = 0 (teleport emptied).
Key 1 with mana 0.2: cost(34) = 0.109375 ≤ 0.2 → usable; if the current action has run
its full duration, action := 34 aimed at the player itself (field 36 := own id); the step
then takes 0.109375 mana. M2 (45): the starter 0x4456a0 computes §3.2 for a wand, spec 1 →
45, needs mana ≥ 0.3 (0 with a kind-9 buff).

**Level-1 ranger, specialization 0, bow, skills all 0.** Slots 0 = 22 shoot (primary), 1 =
25 salvo, 2–4 emptied. M2 held → action 25; released before 25 ends → action 55.

### 16.10 Open points of this section

* The key behind controller flag G+0x18 (action 105) and the meaning of item type 20 in
  equipment slot 12.
* The charge fill (field 23) is now in `alpha-projectiles.md` §2.1. Still open: what ends
  mode flag 0x400 set by ability 99.
* The reader of obj +0x11d8 (dash travel time) and the stat 0x446e80 used by mana-shield.
* Quick-item use 0x4a2780.
