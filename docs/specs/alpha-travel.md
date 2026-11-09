# Alpha travel: gliding, boats, riding pets, climbing, swimming and falls

Behavioural specification of how the Cube World **Alpha** client (`Cube.exe`) moves the
player by hang-glider, boat, ridden pet, by climbing walls and by swimming, and of fall
damage. Written from the Alpha decompile and from the assembly of the routines Ghidra could
not decompile (the game update 0x488ee0 and the world step 0x60c510). Nothing here is
copied code; addresses are citations only.

**What this document adds.** Most of the physics already lives in
`alpha-physics-camera.md` (the integrator, speed limit, vertical forces, glider control,
stamina, buoyancy, friction, fall damage). This document does **not** repeat those rules;
it cites them by section and adds what was missing:

* how gliding, sailing, riding and climbing **start and end** (keys, items, skills,
  automatic ends);
* the **per-frame glide steering** and the **reset of the movement intent** (resolves
  `alpha-physics-camera.md` Q-1);
* the **wind field** used while gliding (resolves Q-7 there);
* how a ridden pet is **attached** under its rider, the **riding energy** meter, and the
  seat offsets;
* what sets mode flags 0x01, 0x10 and 0x40 (resolves `alpha-player-control.md` F-4);
* worked examples for every mode.

Related: `alpha-player-control.md` (input → intent, jump, dodge), `alpha-physics-camera.md`
(step order, §3–§7), `alpha-creatures-models.md` §6.4–§6.5 (glider and boat drawing),
§7.3 (walk cycle), §7.10 (displayed position), pose table rows 58/59,
`alpha-creatures-behaviour.md` §3 (actions), §11 (stamina), §13.2 (pets),
`alpha-skills.md` §2.1, §5.4 (skill texts), `alpha-interaction.md` §5.2 (Ride),
`alpha-ui-screens.md` §5.3 (`ridingbar`, `staminabar`), §11 (keys).

---

## 0. Conventions

* As in `alpha-physics-camera.md` §0: object offsets "obj +x" (record offset + 0x10),
  dt in integer milliseconds, dt_s = dt × 0.001, dt_f = dt as a float, **Relax** and
  **Blend** as defined there. World +z is up; positions are 1/65536 block.
* "Local player" is the step's own creature (world +0xb8); rules marked *local* run only
  for it. On the server there is no local player, so those rules never run there.
* **Skill factor.** For a skill level *n* (obj +0x113c + 4·skill index):
  v(*n*) = 1 − 1/(0.1·*n* + 1). The travel skills use it as follows (0x446aa0, 0x43f770,
  0x43e2c0, 0x4476a0, 0x43e660):

  | Skill (index, obj offset) | Factor | For *n* < 1 |
  |---|---|---|
  | pet riding (1, +0x113c) | 1 + v(*n*) | **0** (riding impossible) |
  | climbing (2, +0x1140) | k = 1 − 1/(0.1·(*n* + 1) + 1) | k(0) = 0.0909 |
  | hang-gliding (3, +0x1144) | g = 1.5 + 3·v(*n*) | **0** (gliding impossible) |
  | swimming (4, +0x1148) | 0.5 + 0.5·v(*n*) | 0.5 |
  | boat driving (5, +0x114c) | 1 + 3·v(*n*) | **0** (the boat cannot move) |

  Test values: v(1) = 0.090909, v(3) = 0.230769, v(5) = 0.333333, v(10) = 0.5.
  The skill-screen texts of `alpha-skills.md` §5.4 are these same factors.

---

## 1. Fields and flags involved

| Field | Meaning here |
|---|---|
| record field 9, obj +0x68 | action: **106** riding, **107** sailing (boat) |
| record field 14, obj +0x124 | mode flags: **0x01** climb (Ctrl held), **0x10** glider open, **0x40** "run" (normal speed ×2; while gliding: lift) |
| record field 6, obj +0x5c | contact: 0x01 ground, 0x02 water, 0x04 wall, 0x10 not fully submerged |
| record field 35, obj +0x198 | for a pet (controller kind 5): its **owner's id** (u64) |
| obj +0x11c8 | the player's **pet id** (u64), the creature spawned from the Pet slot |
| obj +0x11c0 | "mount id" (u64): read by the input and the walk cycle, **never set in Alpha** (§4.9) |
| obj +0x1198 | **riding energy** 0..1 (initial 1, 0x43b73b; saved with the character) |
| obj +0x1194 | stamina 0..1 |
| obj +0x13bc | fall-start height (f32 blocks) |
| obj +0x11a0 | wall normal N (points away from the wall) |
| obj +0xf08 / +0xf09 | Special slot (equipment slot 11) item type / subtype: 23/0 glider, 23/1 boat |
| obj +0x1020 / +0x1021 | Pet slot (equipment slot 12) item type / subtype: 19 = pet item, subtype = creature type |

The client owns the local player; actions and mode flags reach other players through
fields 9 and 14, and the pet's owner through field 35, so every client can reproduce the
glider, the boat and the riding pose (`alpha-net-packets.md` §2.1; field 35 was listed
there as an unknown id — it is the pet's owner).

---

## 2. Frame order for travel

Per rendered frame (game update 0x488ee0), in this order (*believed*: straight-line order
of the routine):

1. **Glide steering or intent reset** (0x497723..0x49788d, local player, §2.1).
2. **Mode flags** from held keys (0x4982be..0x498404, §2.2).
3. **Input** 0x4a6b50 — only when no game window is open — adds the key accelerations to
   the intent (`alpha-player-control.md` §3), jump, dodge.
4. World step(s) 0x60c510 in chunks of at most 20 ms (`alpha-physics-camera.md` §1, §2).

Key presses that arrive as window messages (G, R …) are handled between frames
(0x47e1b0, §3.1, §4.1, §5.2).

### 2.1 Intent reset and glide steering (resolves physics Q-1)

* If the local player is **not gliding** in the physics sense (0x444650: mode 0x10,
  v.z < 0, not on ground, hit-stun < 1, dodge time < 1): **intent := (0, 0, 0)**. The keys
  then add to it in step 3, so the intent of a frame is exactly that frame's key
  accelerations.
* If gliding: let **h** = (v.x, v.y, 0). If |**h**|² > 0, with ĥ = **h**/|**h**|, the side
  vector **s** = up × ĥ = (−ĥ.y, ĥ.x, 0) (to the left of the flight) and β = orientation.y
  (bank, degrees):
  1. intent ← Blend(intent → 10·ĥ, 0.001) over the frame's dt, i.e. with
     w = 1 − 0.999^dt: intent := (1 − w)·intent + w·10·ĥ;
  2. intent := intent − 0.05·β·**s**.

  If |**h**|² = 0 the intent is left unchanged. The intent is **not** reset while gliding,
  so held keys accumulate in it (§3.3).

### 2.2 Mode flags set every frame (resolves player-control F-4)

* 0x04 := 0 (then possibly set again by the aiming rule, not a travel matter).
* **0x40** (0x4982d0): if mode 0x10 is set and the player is not on ground: 0x40 := (Space
  held and stamina > 0). Otherwise 0x40 := (Left Shift **not** held). So on foot, in water,
  on a boat or on a mount the player normally has 0x40 ("run", speed ×2) and Left Shift
  walks; in the air with the glider, Space is the lift key.
* **0x01** (0x498404) := Left Ctrl held (controller flag +0x13). Using an ability slot in
  the same frame clears it again (0x49c02a).
* 0x10 is not touched here; only the G key and the automatic ends change it (§3).

---

## 3. Gliding

### 3.1 Opening and closing the glider (G key, 0x47e83c)

Processed only when no game window (inventory, crafting, map-type screens) is open.

1. **Special slot empty** (obj +0xf08 = 0): clear 0x10; if the action is 107, action := 0.
2. **Glider** (Special slot subtype 0; the slot only accepts item type 23): let
   *h* = trunc(fall-start × 65536) − pos.z, the height fallen since the creature last moved
   up or was supported (fixed point). Then
   **0x10 := (*h* < 2 blocks) and (0x10 was clear)** — a toggle that can only **open** the
   glider while the player has fallen less than 2 blocks; it always closes it. In both
   cases **action := 0** (this also ends riding or an attack).
3. **Boat** (subtype 1): §4.1.

There is no item or skill check at the key. The glider can be opened on the ground: the
flag then stays set (the glider is drawn, `alpha-creatures-models.md` §6.5) and the
player glides as soon as he walks or jumps off an edge. Since gliding moves the fall-start
with the creature (§3.4), the glider can be closed and reopened in flight; a free fall of
more than 2 blocks without the glider cannot be saved by opening it.

### 3.2 Automatic ends of mode 0x10

| When | Where |
|---|---|
| any action ≠ 0 is set at the start of a step (attacks, riding, boat, sitting…) | step order 3 (`alpha-physics-camera.md` §2) |
| hang-gliding skill < 1 (g = 0) | speed limit (0x6194b5) |
| hit-stun > 0, or an action running before its total duration | step order 10 |
| *local:* in water | glider control (0x61ac67) |
| *local:* crash (§3.6) | glider control |
| Special slot no longer item 23 subtype 0 | end of step (0x61f008) |

Landing normally does **not** close the glider. A pet cannot be spawned from the Pet slot
while 0x10 is set (0x60e700).

### 3.3 Control while gliding

* Input acceleration A = **8** per held key instead of 80, but only while gliding in the
  physics sense (falling); while the lift holds v.z at or above 0 it is 80
  (`alpha-player-control.md` §3).
* With stamina ≤ 0 in the air the movement keys do nothing (player-control §3 rule 4).
* Because the intent is not reset (§2.1), a key held for many frames builds up: with key
  direction **k** (unit, camera-relative horizontal) the intent tends to
  10·ĥ + 8·**k**/w. At dt = 16, w = 0.015880, so 8/w = 503.8 blocks/s²: the velocity
  turns to the key direction within a few frames and is then held at the speed limit.
* With no key, the intent tends to 10·ĥ, so the glider accelerates forward along its
  flight direction by 10 blocks/s² up to the speed limit.
* **Banking.** The bank target β* of `alpha-physics-camera.md` §3.4 is multiplied by 40
  (clamped to ±90) while gliding and orientation.y relaxes toward it at 0.001 per ms. The
  side term of §2.1 then pushes the intent by −0.05·β along **s**. A sideways acceleration
  to the left gives a negative bank, and a negative bank pushes further left, so a turn
  keeps itself going until the player steers out of it (*believed* from the signs; Q-T1).

### 3.4 Per step (world step), in the step's order

For a creature with mode 0x10 (references to `alpha-physics-camera.md`):

1. Step order 3: an action ≠ 0 clears 0x10.
2. **Speed limit** (§3.2 there): base S (6 for players); swimming factor if in water;
   then **glider**: g = 1.5 + 3·v(n₃) (0 for n₃ < 1 → clear 0x10); if not on ground,
   S ×= g. The run ×2 is **not** applied while gliding in the air.
3. **Integrate**: v += intent·dt_s; cap the horizontal speed at S.
4. **Vertical** (§3.5 there): while gliding (falling), v.z −= **3·dt_s** (one tenth of
   gravity) and fall-start := z every step. There is no vertical drag; only the wind and
   the lift slow the sink.
5. **Glider control** (*local*, §3.7 there), only while 0x10 is set:
   * in water → clear 0x10, stop;
   * on ground → crash if |orientation.y| > 40, stop;
   * touching a wall with |v|² > 64 and dot(N, v) < −4 → crash;
   * **lift**: if 0x40 (Space, stamina > 0) and v.z < 0: v.z += **30·dt_s**;
   * **wind**: v += W(position)·dt_s (§3.5).
6. Displacement, collision, friction as usual; in the air there is no horizontal drag.
7. **Stamina** (§3.6 there): while 0x10 is set and not on ground, stamina −= 0.0002·dt_f
   if 0x40 (lift held), otherwise unchanged; it does not regenerate in the air with the
   glider open.

So with Space held the creature hovers: gravity (3·dt_s) and lift (30·dt_s) push v.z up to
about 0, after which normal gravity and lift alternate around 0. Full stamina gives
1/0.0002 = **5000 ms** of hovering; then 0x40 is cleared (§2.2) and the keys stop working
(§3.3) until the glider lands.

### 3.5 Wind field W (0x5a6270; resolves physics Q-7)

W depends on position only (not on time and not on velocity). With the position (P_x,
P_y) in fixed point (1/65536 block) and N2 the 2-D value noise below:

* W.x = 2·N2(t(0.05·P_x)/65536, t(0.05·P_y)/65536)
* W.y = 2·N2((0x0D7F0000 − t(−0.05·P_x))/65536, (0x20F60000 − t(−0.05·P_y))/65536)
* W.z = N2((0x108A0000 − t(−0.02·P_x))/65536, (0x14E10000 − t(−0.02·P_y))/65536) + 0.5

where t() truncates a double to a 64-bit integer toward zero and the subtractions are
64-bit. In blocks that is approximately W.x = 2·N2(0.05x, 0.05y),
W.y = 2·N2(3455 + 0.05x, 8438 + 0.05y), W.z = N2(4234 + 0.02x, 5345 + 0.02y) + 0.5. So the
horizontal wind is in [−2, 2] blocks/s² with a 20-block lattice, and the vertical wind is
in [−0.5, 1.5] with a 50-block lattice: on average a slight updraft.

**N2(x, y)** (0x4c0ef0), all in double precision except where noted:

1. i = (int)x, j = (int)y (truncation toward zero); fx = x − i, fy = y − j.
2. Corner value for lattice point (a, b): n = a + 57·b (32-bit wrap); n := n XOR (n << 13);
   q = (n·(n·n·60493 + 19990303) + 1376312589) AND 0x7FFFFFFF (all unsigned 32-bit wrap);
   r = 1 − q / 1073741824.
3. wx = (1 − cos(π·fx))/2, wy = (1 − cos(π·fy))/2 (π as the float 3.1415927).
4. N2 = (r(i, j)·(1 − wx) + r(i+1, j)·wx)·(1 − wy) + (r(i, j+1)·(1 − wx) + r(i+1, j+1)·wx)·wy,
   returned as a float.

Test vector: at a lattice point (fx = fy = 0) N2 = r(i, j). For (i, j) = (0, 0): n = 0,
q = 1376312589, r = 1 − 1.281791 = −0.281791.

### 3.6 Crash

Crash (glider control, 0x61acd5 / 0x61adde): hit-stun := 1500 ms (players; NPCs
3000 − 375·tier, 0x444ff0), clear 0x10, and play sound 23 (`sword-hit03.wav`) at volume 1.
The stun then also clears 0x01 and the push list (step order 10). There is no damage.

### 3.7 Pose, drawing, camera and sound

* Drawing: the glider model (`glider`, key 2547) above the body while 0x10 is set, kept
  level (`alpha-creatures-models.md` §6.5).
* Pose: arms raised and legs not swinging while 0x10; body height channel *h* := box.z/2
  while gliding (models §7.4 step 2, §7.7). Bank is orientation.y, drawn as body roll.
* Camera: no glider-specific rule; the normal follow camera (`alpha-physics-camera.md`
  §7). No opening or closing sound was found; only the crash sound.

### 3.8 Worked examples (dt = 16 ms, skill n₃ = 1)

g = 1.5 + 3·0.090909 = 1.772727, so S = 6·1.772727 = **10.636 blocks/s**.

1. **One step, no key.** State v = (8, 0, −2), intent (10, 0, 0) after §2.1, bank 0,
   W = (0.4, −0.2, 0.6) at this position. a = 0.16·(1, 0, 0) → v = (8.16, 0, −2);
   |v_xy| < S. Glide gravity: v.z = −2 − 0.048 = −2.048. No lift. Wind: + (0.0064,
   −0.0032, 0.0096) → v = (8.1664, −0.0032, −2.0384). Move d = v·0.016 =
   (0.130662, −0.0000512, −0.0326144). Stamina unchanged.
2. **Glide from a cliff, no wind, no keys**, starting at v = (S, 0, 0): v.z = −3t, drop
   1.5·t² blocks: after 2 s 6 blocks down and 21.3 blocks forward; after 4 s 24 down,
   42.5 forward. With the average updraft W.z = 0.5 the sink is −2.5t (drop 1.25·t²).
3. **Hover**: v.z = −0.5, Space held, stamina 1: gravity → −0.548, lift +0.48 → −0.068;
   stamina 1 − 0.0032 = 0.9968.
4. **Opening**: jump from flat ground and press G after the apex. Under normal gravity
   the fall since the apex is 15·t² blocks: at t = 0.3 s that is 1.35 < 2 → opens; at
   t = 0.4 s, 2.4 → refused.
5. **Crash**: landing with bank 45° → crash (stun 1500 ms). Flying into a wall at
   v = (−9, 0, −1) with N = (1, 0, 0): |v|² = 82 > 64, dot = −9 < −4 → crash.

---

## 4. Boats

### 4.1 Boarding and leaving (G key, 0x47e8b9)

With the boat (item 23 subtype 1) in the Special slot:

* action ≠ 107 and the player **in water** (contact 0x02): action := 107, time := 0.
* action ≠ 107 and not in water: nothing.
* action = 107: action := 0.

Other ends of action 107:

* *local:* in action 107, **on ground and not in water** (beached): action := 0
  (0x6192e5).
* Special slot emptied while sailing, then G: action := 0 (§3.1 step 1); equipping a
  Special item sets action := 0 (`alpha-ui-screens.md` §8, equip table).
* Any other action replacing it (attack, ability: the action clock of 107 runs, and after
  its default total duration of 1200 ms the attack input may replace it, *believed* from
  `alpha-creatures-behaviour.md` §3.4).

No duration-based end of 107 was found in the step; it persists until one of the above.

### 4.2 Movement while sailing (in water, action 107)

* **Input**: the swimming rule of player-control §3 (3-D camera axes) is skipped for
  action 107, so the boat uses the **walking** rule: horizontal camera-relative keys,
  A = 80. Jump: the "swim up" rule still applies in water (*as read*).
* **Speed limit**: S = 6 × (1 + 3·v(n₅)) (0 for n₅ < 1), × 2 with 0x40 (Shift not held).
  The swimming factor is not applied. Out of water (airborne off a wave) the normal limit
  applies.
* **Buoyancy** (`alpha-physics-camera.md` §5.2): b = r + 1 for every r > 0 (no cap at 1).
* **Drag**: the water drag (relax at 0.005) is **skipped**; instead v.x, v.y relax toward
  0 at 0.1·R and v.z at R (R = 0.0025 per ms with a key held, else 0.005; §6 there).
* Swim pitch: orientation.x relaxes toward 0 (not −60).
* No walk-cycle phase (models §7.3); pose 59 (`alpha-creatures-models.md` pose table): the
  body sits at *E*.z + 1.25, the boat model (`boat2`, key 2548) is drawn under the body
  (−9·box.z/box.x, §6.5 there).
* Camera focus height +1.5 instead of +0.5 (physics §7.3 step 5). Name and life-bar
  anchors are raised by 2 blocks for actions 106 and 107 (0x49a14e, *believed*).

### 4.3 Worked examples (dt = 16)

* n₅ = 1, Shift not held: k = 1.272727 → S = 6·1.272727·2 = **15.273 blocks/s**. Holding
  W: the move is 0.24436 blocks per frame; afterwards friction 0.1·0.0025 = 0.00025 per ms
  gives 0.996008 per frame (stored 15.212).
* Release: 0.1·0.005 = 0.0005 per ms → 0.992029 per frame. The boat coasts about
  15.212·0.016·0.992029/0.007971 ≈ **30.3 blocks**.
* n₅ = 0: S = 0, so the horizontal velocity is clamped to 0 every step: the boat floats
  but cannot be driven.
* n₅ = 10: k = 2.5 → S = 30 blocks/s.

---

## 5. Riding pets

### 5.1 The pet

* The Pet slot (equipment slot 12, obj +0x1020) holds a pet item (type 19) whose subtype
  is the creature type. Each step, for every player with a pet item (0x60e485):
  1. If the player's pet id (obj +0x11c8) is unset or names no creature, it is taken from
     any creature of controller kind 5 whose owner (obj +0x198) is the player.
  2. If the pet exists and the pet item changed (level obj +0x1138 against the copy at
     +0x1d44, or the item against the copy at +0x1d48), the pet is rebuilt from the item:
     type := subtype, level, name (obj +0x1168, from the item's name bytes), owner := the
     player.
  3. If there is no pet and the player is **not gliding**, a new pet creature is created
     with id = (lowest existing id) − 1 and owner := the player (0x60e7e3).
* **Rideable** types (0x444760): 19, 20, 22, 23, 25–40, 63–67, 74, 75, 98–100, 102–105,
  151 (the same list labels a pet item "Ridable" in the tooltip).

### 5.2 Mounting (R key, 0x496a59)

As `alpha-interaction.md` §5.2, with one correction: the target creature is of controller
kind 5, of a rideable type, its owner (obj +0x198) is the player, the player's
**pet-riding skill** (obj +0x113c) is non-zero (the interaction spec calls it "a riding
item"), and the player's action is not 106. Prompt "[R] Ride". Pressing R: action := 106,
time := 0. There is no distance rule beyond the interaction target choice.

### 5.3 Per step while riding (rider = local player, action 106)

In the speed-limit stage (0x619306), in this order:

1. Look up the player's pet (obj +0x11c8). If it exists, its health is not negative and
   its type is rideable: S ×= 1 + v(n₁). If that factor is 0 (n₁ < 1): action := 0. If
   the pet is missing, dead (health < 0) or not rideable: action := 0.
2. **Riding energy**: if the player's intent is non-zero, energy −= 0.002·dt_s. If the
   energy is then ≤ 0: energy := 0 and action := 0.
3. If mode 0x01 (Ctrl held): action := 0 (**dismount** with Ctrl).

Otherwise the rider is an ordinary walking creature: walking input (A = 80), the run ×2,
gravity, friction and collision with the **rider's own** box. The rider's box and
position do the moving; the pet follows (§5.4).

### 5.4 The pet under its rider (every client; 0x61f968 and 0x620f1b)

For each creature of controller kind 5 whose owner (obj +0x198) exists and has action
106, after its own physics in the step:

* its record and displayed position := the owner's displayed position +
  (0, 0, ½·sz_pet − ½·sz_owner + 0.01) — the two boxes stand on the same floor, the pet
  0.01 higher;
* orientation and displayed body rotation are copied from the owner, the landing bob, walk
  amplitude and phase are copied, velocity and intent are zeroed
  (`alpha-creatures-models.md` §7.10 "second rule"; the condition is this one).

The pet's AI does nothing while its owner rides (`alpha-creatures-behaviour.md` §13.2).
The ridden pet has no life bar over it (0x499f14).

### 5.5 Riding energy refill

When a pet's ground probe (physics §5.4, not in water) finds a block of **type 3** in the
cells under it (0x61d2dc; type 3 is the wet lake-bed / terrace ground of
`alpha-worldgen-terrain.md` §5.6 and `alpha-worldgen-caves.md`, which also grows reeds),
its owner's riding energy += 0.05·dt_s, capped at 1 (0x61d36c). This happens whether or
not the owner is riding.

The `ridingbar` shows *k* = min(5, trunc(5·energy + 0.9999)) icons
(`alpha-ui-screens.md` §5.3).

### 5.6 Pose, seat and camera

* Rider pose 58 (`alpha-creatures-models.md` pose table): the body is raised by the pet's
  box.z·0.75 plus the seat offset of the pet type (0x446950, z only):

  | Pet type | Seat z |
  |---|---|
  | 26, 30 | −0.9 |
  | 33 | −0.2 |
  | 74 | −0.3 |
  | 75 | +0.1 |
  | 151 | −0.35 |
  | others | 0 |

  Types 22 and 23 lower it a further 0.75 (pose 58 row). Legs 20/20, lean 10, hands on
  the reins.
* Walk cycle of the rider: actions 106/107 scale the walk swing by 0.1 (models §7.4).
* Camera focus +1.5 (physics §7.3).

### 5.7 Ends of riding and combat

Riding (action 106) ends when: Ctrl is pressed (§5.3), the riding energy runs out, the
pet is lost or dies, the riding skill is 0, G is pressed with a glider (§3.1), or any
other action replaces 106. There is no combat from the saddle: the attack input and
abilities start their action when the current action is "finished"
(`alpha-creatures-behaviour.md` §3.4, §16), which for 106 is after its default total
duration of 1200 ms (*believed*: 106 has no own entry in the duration table). The attack
then replaces 106 and the pet is released where it stands.

### 5.8 Worked examples (dt = 16)

* n₁ = 1, Shift not held: S = 6·1.090909·2 = **13.09 blocks/s**; n₁ = 5: S = 16;
  n₁ = 0: cannot mount (no prompt) and an existing ride ends at once.
* Energy: moving continuously drains 0.002 per second, so a full meter lasts **500 s** of
  movement (100 s per icon); standing still costs nothing. A pet on type-3 ground refills
  0.05 per second: empty to full in 20 s.
* Pet type 98 (box z 2.0, say) under a human (box z 2.16): pet z = rider z +
  (1.0 − 1.08 + 0.01) = rider z − 0.07.

### 5.9 The dormant "mount" path

The input routine (0x4a6e4e) sends the keys to the creature named by obj +0x11c0 instead
of the player when it is non-zero, and the walk cycle (models §7.3 "Riding") places a
rider on top of that creature. No Alpha code sets obj +0x11c0 (only the constructors zero
it: client 0x43b7aa, 0x446373; server 0x40651a, 0x411113), so in Alpha these rules never
run. An implementation may omit them or keep them inactive.

---

## 6. Climbing

### 6.1 Starting and stopping

* Mode 0x01 = **Left Ctrl held**, set every frame (§2.2). There is no item requirement;
  the climbing skill only changes the stamina cost.
* The input uses the climbing rule only with 0x01 **and** wall contact (contact 0x04 from
  the previous step).
* Cleared in the step by: stamina < 0.01 while climbing with a key held (§6.3), hit-stun,
  fall damage, an action running before its total duration (step order 10), and in the
  same frame by using an ability slot. The next frame sets it again if Ctrl is still
  held.
* While riding, Ctrl dismounts (§5.3). While swimming at the surface, 0x01 lifts the rule
  that stops upward acceleration out of the water (physics §2 step 4), so Ctrl lets the
  player climb out onto a bank.

### 6.2 Movement

* Input (player-control §3 rule 1): forward/back → intent.z ± 0.2·A = ±16 blocks/s²;
  left/right → ±16 along the wall (the wall normal turned 90° about z).
* Step (physics §3.5 rule 1): with 0x01, not gliding and touching a wall: **v relaxes
  toward 0 at 0.0025 per ms** in all three axes, no gravity; fall-start := z while
  v.z > −10. The horizontal speed limit is 6 (no run ×2 on a wall).
* At the top: the ledge push v := −5·N (physics §3.8) and the step-up (§4.3 there).
* Climbing speed does **not** depend on the climbing skill.

### 6.3 Stamina (physics §3.6)

While 0x01, a key held, not gliding, on a wall and not on ground: stamina −=
(1 − k)·0.0005·dt_f with k = 1 − 1/(0.1·(n₂ + 1) + 1); if stamina < 0.01: clear 0x01.
Hanging on the wall without a key costs nothing and does not regenerate. When the flag is
cleared the creature falls (normal gravity) that step and stamina regenerates
(+0.0001·dt_f); holding Ctrl re-grabs next frame, so an exhausted climber slides down in
short grabs (*believed*; Q-T3).

### 6.4 Orientation and pose (0x61f0fb)

While 0x01, wall contact and not gliding:

* head pitch (record field 5) relaxes toward **45** at 0.005 per ms;
* yaw: let **u** = −N normalised (into the wall), clamp u.y to [−1, 1];
  θ = acos(u.y)·180/π, negated when u.x > 0. Then orientation.z relaxes toward
  ψ + asin(sin(θ − ψ)) (the shortest turn) at 0.005 per ms, ψ = current yaw. The creature
  ends up facing the wall: facing(θ) = (−sin θ, cos θ) = **u**.

Otherwise head pitch decays toward 0. Pose: climbing arm/leg channels of models §7.4 step
10; walk-cycle phase from the 3-D speed (models §7.3).

### 6.5 Worked examples (dt = 16, holding W)

* Per frame v.z ← (v.z + 0.256)·0.96075: v₁ = 0.246, steady **6.27 blocks/s**.
* Skill 0: k = 0.090909, drain 0.00045455 per ms → a full bar lasts **2.2 s**, which
  climbs about **11.3 blocks** from rest (Σ v_k·0.016 over 137.5 frames). Skill 4: k =
  0.333, 3.0 s; skill 9: k = 0.5, 4.0 s (about 22.6 blocks).
* Facing: N = (0, +1, 0) (wall on the −y side): **u** = (0, −1, 0), θ = 180°;
  facing(180) = (0, −1, 0). N = (−1, 0, 0): **u** = (1, 0, 0), θ = −90°.

---

## 7. Swimming

All in `alpha-physics-camera.md`; summarised for completeness:

* In water (contact 0x02) and not sailing: input uses the 3-D camera axes (player-control
  §3 rule 3); S = 6 × (0.5 + 0.5·v(n₄)) (0.5 for n₄ < 1), × 2 with 0x40.
* Buoyancy §5.2, water drag 0.005 per ms and body pitch −60 (§5.3), no upward key
  acceleration at the surface unless Ctrl (§2 step 4), jump to swim up
  (player-control §4).
* Stamina regenerates normally in water; no swimming stamina cost was found.
* Examples: n₄ = 0 → S = 6 (3 with Shift); n₄ = 3 → 0.615385 → S = 7.38; n₄ = 10 → S = 9.
  The move of each frame uses S; the stored velocity after drag is S·0.92293 at dt = 16.

---

## 8. Fall damage

`alpha-physics-camera.md` §4.2, *local player only*: on a vertical collision with
v.z < −5, not in water by the previous step's flags, and fall height h > 20 blocks
(h = fall-start − z): damage = (h/20 − 1)·maxHealth/2, plus 1500 ms stun, 0x01 cleared,
sound 23. Interactions with travel:

* The glider moves the fall-start with the creature every step (§3.4 step 4), so a glided
  landing never hurts, however high the start. Closing the glider in the air starts the
  fall count there.
* Climbing keeps fall-start at the current height while v.z > −10.
* Landing in water never hurts; neither does a boat.
* A ridden pet takes no fall damage (the rule is for the local player); the rider does.
* Examples: 20 blocks → 0; 30 → 25 % of maxHealth; 40 → 50 %; 60 → 100 % (death). A
  30-block free fall lands at v.z ≈ −42 blocks/s.

---

## 9. Resolutions and corrections to other specifications

1. `alpha-physics-camera.md` **Q-1**: the intent is reset to 0 once per frame by the game
   update before the input, except while gliding (§2.1).
2. `alpha-physics-camera.md` **Q-7**: the wind field is §3.5; it does not vary with time.
3. `alpha-player-control.md` **F-4**: 0x40 (§2.2), 0x01 (§2.2, §6.1), 0x10 (§3.1).
4. `alpha-interaction.md` §5.2: obj +0x113c is the pet-riding **skill**, not an item.
5. `alpha-creatures-models.md` §7.10: the "second rule" is the pet-under-rider rule of
   §5.4; §7.3 "Riding" is the dormant mount path of §5.9.
6. `alpha-net-packets.md` field 35 (record 0x188) is a pet's owner id.
7. `alpha-physics-camera.md` §3.2 "riding": the factor uses the player's pet (obj
   +0x11c8); "health > 0" there is "health not negative".

---

## 10. Open questions (with checks)

* **Q-T1 Glide turning.** The bank feedback of §3.3 may make turns self-sustaining. Glide
  with one sideways tap and log orientation.y and v for 5 s.
* **Q-T2 Glider opening height.** Confirm the 2-block rule: jump from a high place and
  press G after falling 1, then 3 blocks; log obj +0x13bc and pos.z.
* **Q-T3 Exhausted climbing.** Log stamina, mode 0x01 and v.z while holding Ctrl and W
  with empty stamina, to confirm the slide-and-grab behaviour.
* **Q-T4 Riding energy.** Confirm the drain 0.002/s and the type-3 refill by riding for a
  minute and standing the pet on a lake shore; log obj +0x1198.
* **Q-T5 Ending 106/107 by attacking.** Press M1 at 0.5 s and 2 s after mounting or
  boarding; log the action.
* **Q-T6 Shift.** Confirm that Left Shift (controller +0x14) slows the player to half
  speed on foot, on a boat and on a mount (mode 0x40 off).
* **Q-T7 Noise sign handling.** N2 truncates toward zero, so negative arguments
  interpolate across a mirrored cell; check W at a position with negative coordinates if
  the world ever produces one.
