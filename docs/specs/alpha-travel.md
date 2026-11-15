Status: FINAL / STABLE / HELD

# Alpha travel: gliding, boats, riding pets, climbing, swimming and falls

Behavioural specification of how the Cube World **Alpha** client moves the player by
hang-glider, boat and ridden pet, by climbing walls and by swimming, and how falls end.
Established from the original client image and its disassembly; nothing here is copied
code. All addresses are **RVAs in `Cube.exe`** (image base 0x400000) unless marked
`Server.exe` (image base 0x400000). Addresses are citations only.

Two routines carry most of the rules: the client **game update** (RVA 0x88ee0, once per
rendered frame) and the **world step** (RVA 0x20c510). Companion contracts are named as
pointers only: `alpha-physics-camera.md`, `alpha-player-control.md`,
`alpha-creatures-models.md`, `alpha-creatures-behaviour.md`, `alpha-interaction.md`,
`alpha-ui-screens.md`.

---

## 0. Conventions

* "obj +x" is an offset into the in-memory creature object. Positions are signed 64-bit
  fixed point, 1/65536 block per unit, x/y/z at obj +0x10 / +0x18 / +0x20. World +z is up.
* Velocity **v** is at obj +0x34 (v.z at +0x3c), the movement intent (an acceleration in
  blocks/s²) at obj +0x40, orientation (three floats, degrees) at obj +0x28: orientation.y
  is the bank, orientation.z the yaw. The collision box size is at obj +0x80.
* dt is the step's elapsed time in integer milliseconds; dt_f = dt as a float;
  dt_s = dt_f × 0.001 (single precision).
* **Relax**(x → target, rate) over dt: the step x := x + (target − x)·rate is applied once
  per millisecond, dt times, in single precision (RVA 0x11d40; the three-axis form is
  RVA 0x205950). Per frame this equals a factor (1 − rate)^dt on the distance to target.
* **Blend**(x → target, rate) over dt (RVA 0x57460): a weight w is built by starting at 0
  and applying w := w + (1 − w)·rate dt times in double precision, then rounding w to a
  float (RVA 0xac150); then x := (1 − w)·x + w·target per axis in single precision. Thus
  w ≈ 1 − (1 − rate)^dt.
* "Local player" is the step's own creature, the pointer at world +0xb8. Rules marked
  *local* run only for that creature.
* Other fields used: action byte obj +0x68 and its elapsed time obj +0x6c (ms); hit-stun
  obj +0x12c (ms); dodge time obj +0x128 (ms); health obj +0x16c; creature type obj +0x64;
  controller kind obj +0x60 (0 = player, 5 = pet).
* **Skill factor.** For a skill level *n*, v(*n*) = 1 − 1/(0.1·*n* + 1), computed in
  single precision. The travel skills (32-bit levels in consecutive slots) use it as
  follows:

  | Skill (level offset) | Factor routine | Factor | For *n* < 1 |
  |---|---|---|---|
  | pet riding (obj +0x113c) | RVA 0x46aa0 | 1 + v(*n*) | **0** |
  | climbing (obj +0x1140) | RVA 0x3e660 | k = 1 − 1/(0.1·(*n* + 1) + 1) | k(0) = 0.090909 |
  | hang-gliding (obj +0x1144) | RVA 0x3f770 | g = 1.5 + 3·v(*n*) | **0** |
  | swimming (obj +0x1148) | RVA 0x476a0 | 0.5 + 0.5·v(*n*) | 0.5 |
  | boat driving (obj +0x114c) | RVA 0x3e2c0 | 1 + 3·v(*n*) | **0** |

  Test values: v(1) = 0.090909, v(3) = 0.230769, v(5) = 0.333333, v(10) = 0.5.

---

## 1. Fields and flags involved

| Field | Meaning here |
|---|---|
| obj +0x68 | action: **106** riding, **107** sailing (boat) |
| obj +0x124 (16-bit) | mode flags: **0x01** climb, **0x04** aim, **0x10** glider open, **0x40** run (speed ×2; while gliding in the air: lift) |
| obj +0x5c | contact flags: 0x01 ground, 0x02 water, 0x04 wall |
| obj +0x198 | for a pet: its **owner's id** (u64) |
| obj +0x11c8 | the player's **pet id** (u64) |
| obj +0x11c0 | "mount id" (u64), read by the input routine and by the step; never set to a non-zero value by any direct write (§5.9) |
| obj +0x1198 | **riding energy**, 0..1; the constructor sets it to 1 (RVA 0x3b73b) |
| obj +0x1194 | stamina, 0..1 |
| obj +0x13bc | fall-start height (float, blocks) |
| obj +0x11a0 | wall normal N |
| obj +0xf08 / +0xf09 | Special equipment slot: item type / subtype (23/0 glider, 23/1 boat) |
| obj +0x1020 / +0x1021 | Pet equipment slot: item type / subtype (type 19 = pet item; subtype = creature type) |

The Special and Pet slots lie 0x118 bytes apart; that they are equipment slots 11 and 12
of an array with this stride is *believed* (the array base was not traced).

---

## 2. Frame order for travel

The game update (RVA 0x88ee0) does, in this order, for the local player:

1. **Glide steering or intent reset** (RVA 0x97723..0x9788d, §2.1). Always runs.
2. A gate (RVA 0x97952..0x9807f): in frames where the local player's hit-stun is > 0
   (RVA 0x98065), or where certain interface/session states hold, steps 3 and 4 are
   skipped entirely and the mode flags keep their previous values.
3. **Input** (call at RVA 0x982b9 to RVA 0xa6b50), only when none of the six game
   windows checked there is open. It adds the key accelerations to the intent, and
   handles jump and dodge (`alpha-player-control.md`).
4. **Mode flags** from held keys (RVA 0x982be..0x98404, §2.2).

The input in step 3 therefore sees the mode flags computed in the previous frame. The
world step(s) follow later in the frame.

The G key arrives as a window key message (dispatch in the handler at RVA 0x7e1b0, case
at RVA 0x7e83c); it is processed only when none of the same six game windows is open.
The R key (riding) is a polled key byte read inside the game update (§5.2).

### 2.1 Intent reset and glide steering

"Gliding in the physics sense" (RVA 0x44650): mode 0x10 set, v.z < 0, not on ground,
hit-stun < 1 and dodge time < 1.

* If the local player is **not** gliding in that sense: **intent := (0, 0, 0)**. The
  input then adds the keys, so the intent of a frame is exactly that frame's key
  accelerations.
* If gliding: let **h** = (v.x, v.y, 0). If |**h**|² > 0, with ĥ = **h**/|**h**|, the
  side vector **s** = (0, 0, 1) × ĥ = (−ĥ.y, ĥ.x, 0) and β = orientation.y:
  1. intent := Blend(intent → 10·ĥ, 0.001) over the frame's elapsed milliseconds (the
     value the update also passes to the input routine);
  2. intent := intent − ((**s**·β)·0.05).

  If |**h**|² = 0 the intent is left unchanged. The intent is **not** reset while gliding,
  so held keys accumulate in it (§3.3).

### 2.2 Mode flags (when step 4 runs)

Controller key bytes used: +0x12 (the jump key), +0x13 (the climb key), +0x14 (the walk
key); their physical key assignment is given in `alpha-player-control.md`.

1. 0x04 := 0.
2. **0x40** (RVA 0x982d0): if 0x10 is set and the player is not on ground:
   0x40 := (jump byte held and stamina > 0). Otherwise 0x40 := (walk byte **not** held).
3. If the walk byte is held (RVA 0x98318), and the action is none of 109, 83, 106, 107,
   105, 82, 34, 84, 80, 81, 79, 11, and either the action time is at least the action's
   total duration (RVA 0x3d1a0) + 200 ms or the action is one of 24, 25, 27, 8, 36, 64, 59,
   63, 28, 35: then 0x04 := 1 unless the action is 87, 88, 89 or 92, and in either case
   **0x40 := 0**. For action 0 the total duration is 100 ms, so after 300 ms of action
   time the walk byte also cancels the glider lift.
4. **0x01** (RVA 0x98404) := climb byte held.
5. Later in the same update, using an ability slot clears 0x01 again (RVA 0x9c02a;
   *believed*: the clearing condition is a flag set inside the ability-slot loop).
6. 0x10 is not touched here; only the G key and the automatic ends change it (§3).

---

## 3. Gliding

### 3.1 Opening and closing the glider (G key, RVA 0x7e83c)

1. **Special slot empty** (obj +0xf08 = 0): clear 0x10; if the action is 107, action := 0.
2. **Subtype 0** (glider): let *h* = trunc₆₄(fall-start × 65536) − pos.z, where the
   product is a float and trunc₆₄ truncates toward zero to a signed 64-bit integer; the
   subtraction is 64-bit. Then **0x10 := (*h* < 2·65536) and (0x10 was clear)**
   (RVA 0x2c860 is the comparison), and **action := 0**. This toggle can only **open** the
   glider while the fixed-point drop since the fall-start is below 2 blocks; it always
   closes an open glider.
3. **Subtype 1** (boat): §4.1.

No skill is checked at the key; only the Special slot's type byte (non-zero) and subtype
are tested. The G handler plays no sound.

Consequences (*derived*): the glider can be opened on the ground (the step then keeps the
flag: see §3.2) and the player glides as soon as he walks or jumps off an edge. Because
gliding sets the fall-start to the current height every step (§3.4), the glider can be
closed and reopened in flight; a free fall of 2 blocks or more without the glider cannot
be saved by opening it.

### 3.2 Automatic ends of mode 0x10 (world step)

| When | Where |
|---|---|
| action ≠ 0 | RVA 0x21888f |
| hang-gliding factor g ≤ 0 (skill < 1) | speed-limit stage, RVA 0x2194b5 |
| hit-stun > 0 (also clears 0x01 and zeroes the 32-bit field obj +0x144) | RVA 0x21ae3f |
| action ≠ 0 and its elapsed time below its total duration (RVA 0x3d1a0) (also clears 0x01) | RVA 0x21ae70 |
| *local:* in water | glider control, RVA 0x21ac67 |
| *local:* crash (§3.6) | glider control |
| Special slot not type 23 subtype 0 | RVA 0x21f008 |

Landing does **not** close the glider unless the landing crashes (§3.6). A new pet is not
spawned while 0x10 is set (RVA 0x20e700).

### 3.3 Control while gliding

* Input acceleration A = **8** per held key instead of 80 while gliding in the physics
  sense (RVA 0xa6b8b..0xa6bb2); otherwise (for example while the lift holds v.z ≥ 0) it is
  80.
* Walking-rule keys do nothing while 0x10 is set, the player is airborne and stamina ≤ 0
  (RVA 0xa7161).
* *Derived:* because the intent is not reset while gliding, a key held with unit
  horizontal direction **k** drives the intent toward 10·ĥ + 8·**k**/w (bank term aside).
  At dt = 16, w = 0.015880 and 8/w = 503.8 blocks/s². With no key the intent tends to
  10·ĥ: the glider accelerates along its flight direction up to the speed limit.
* **Banking** (RVA 0x219100..0x21920a): the bank target of `alpha-physics-camera.md` is
  multiplied by 40 and clamped to [−90, 90] while gliding in the physics sense, and
  orientation.y relaxes toward it at rate 0.001; otherwise orientation.y relaxes toward the
  unmultiplied target at rate 0.005. The side term of §2.1 then pushes the intent by
  −0.05·β along **s**.

### 3.4 Per step, in the step's order

For a creature with mode 0x10:

1. Action ≠ 0 clears 0x10 (RVA 0x21888f).
2. **Speed limit stage** (from RVA 0x219234): base S = 6 (8 for certain creature types or
   a flag, 12 for controller kind 5); × swimming factor if in water and not sailing;
   (riding, §5.3); then, if 0x10: g = 1.5 + 3·v(n_glide); if g ≤ 0 clear 0x10; else if not
   on ground S ×= g (RVA 0x219470). Later in the same stage (RVA 0x219d8e) the run factor
   S ×= 2 (double precision, rounded to float) applies when 0x40 is set, **except** when
   0x01 is set with wall contact, or 0x10 is set while not on ground.
3. **Stamina** (same stage, RVA 0x219cf9): if 0x10 is set and not on ground:
   stamina −= 0.0002·dt_f when 0x40 is set, otherwise unchanged (no regeneration). The
   result is clamped to [0, 1].
4. **Integrate and cap** (before RVA 0x21aafe): the intent is added to v and the
   horizontal speed is capped at S (|v_xy|² compared with S²).
5. **Vertical** (RVA 0x21ab9e): while gliding in the physics sense,
   v.z −= dt_s·0.1·30 (= **3·dt_s**) and fall-start := current z (fixed point × 1/65536,
   float); the same reduced gravity applies to a creature with bit 0x02 of obj +0x7e set
   and no hit-stun. Otherwise (and not under the climbing rule of §6.2): if v.z ≥ 0,
   fall-start := z; then v.z −= 30·dt_s.
6. **Glider control** (*local*, only while 0x10 is set, RVA 0x21ac47):
   * in water → clear 0x10; nothing else of this list.
   * on ground → crash if |orientation.y| > 40; nothing else of this list.
   * airborne with wall contact, |v|² > 64 and dot(N, v) < −4 → crash; the lift and wind
     below still apply in this step.
   * **lift** (RVA 0x21ad36): if 0x40 and v.z < 0: v.z += **30·dt_s**.
   * **wind** (RVA 0x21ad69): v += W(position)·dt_s (§3.5).

*Derived:* with the jump key held the creature hovers: the lift outweighs the glide
gravity while v.z < 0, and once v.z ≥ 0 the creature is no longer gliding in the physics
sense, so the full gravity 30·dt_s applies until v.z falls below 0 again. Full stamina
gives 1/0.0002 = **5000 ms** of lift; then 0x40 is cleared at the next flag update
(§2.2) and the walking keys stop working (§3.3) until the glider lands.

### 3.5 Wind field W (RVA 0x1a6270)

W depends on position only; the routine receives the velocity pointer but does not read
it. With the position (P_x, P_y) in fixed point, t() truncating a double toward zero to a
signed 64-bit integer, all products in double precision and N2 the value noise below:

* W.x = 2·N2(t(0.05·P_x)/65536, t(0.05·P_y)/65536)
* W.y = 2·N2((0x0D7F0000 − t(−0.05·P_x))/65536, (0x20F60000 − t(−0.05·P_y))/65536)
* W.z = N2((0x108A0000 − t(−0.02·P_x))/65536, (0x14E10000 − t(−0.02·P_y))/65536) + 0.5

The subtractions are 64-bit; each N2 result is a float, the ×2 and +0.5 are single
precision. In blocks this is approximately W.x = 2·N2(0.05x, 0.05y),
W.y = 2·N2(3455 + 0.05x, 8438 + 0.05y), W.z = N2(4234 + 0.02x, 5345 + 0.02y) + 0.5: the
horizontal wind lies in [−2, 2] blocks/s² on a 20-block lattice, the vertical wind in
[−0.5, 1.5] on a 50-block lattice.

**N2(x, y)** (RVA 0xc0ef0), double precision except the final result:

1. i = x truncated toward zero to a 32-bit integer, j likewise from y; fx = x − i,
   fy = y − j. The neighbour index is formed as the truncation of (double)i + 1.
2. Corner value for lattice point (a, b): n = a + 57·b (32-bit wrap); n := n XOR (n << 13);
   q = (n·(n·n·60493 + 19990303) + 1376312589) AND 0x7FFFFFFF (unsigned 32-bit wrap);
   r = 1 − q × 2⁻³⁰.
3. wx = (1 − cos(π·fx))·0.5 and wy = (1 − cos(π·fy))·0.5, where π is the
   double-precision constant 3.1415927 (a seven-digit decimal value, not the full-precision
   π and not a float-rounded one).
4. N2 = (r(i, j)·(1 − wx) + r(i+1, j)·wx)·(1 − wy) + (r(i, j+1)·(1 − wx) + r(i+1, j+1)·wx)·wy,
   rounded to a float.

Test vector: at a lattice point N2 = r(i, j); for (i, j) = (0, 0): n = 0,
q = 1376312589, r = −0.281791. *Derived:* since i truncates toward zero and the weights
are even in fx, N2 at a negative argument −a within (−1, 0) equals N2 at +a.

### 3.6 Crash

Crash (glider control, RVA 0x21acd5 wall / RVA 0x21adde ground): hit-stun := 1500 ms for
controller kind 0, otherwise 3000 − (1500·tier)/4 with tier the byte at obj +0x1a8
(RVA 0x44ff0); clear 0x10; play sound 23 at volume 1. No health is changed. The hit-stun
then clears 0x01 in the same step (RVA 0x21ae3f).

### 3.7 Worked examples (dt = 16 ms, gliding skill 1; *derived*)

g = 1.5 + 3·0.090909 = 1.772727, so S = 6·1.772727 = **10.636 blocks/s**.

1. **One step, no key.** v = (8, 0, −2), intent (10, 0, 0), bank 0, W = (0.4, −0.2, 0.6)
   assumed at this position. Integrate: v = (8.16, 0, −2); |v_xy| < S. Glide gravity:
   v.z = −2.048. No lift. Wind: + (0.0064, −0.0032, 0.0096) → v = (8.1664, −0.0032,
   −2.0384). Displacement v·0.016 = (0.130662, −0.0000512, −0.0326144). Stamina unchanged.
2. **Glide from a cliff, no wind, no keys**, from v = (S, 0, 0): v.z = −3t, drop 1.5·t²:
   after 2 s 6 blocks down and 21.3 forward. With W.z = 0.5 the sink is −2.5t.
3. **Hover:** v.z = −0.5, jump key held, stamina 1: gravity → −0.548, lift +0.48 →
   −0.068; stamina 1 − 0.0032 = 0.9968.
4. **Opening:** jump from flat ground and press G after the apex. With gravity 30 alone
   the drop since the apex is 15·t²: at t = 0.3 s 1.35 < 2 → opens; at t = 0.4 s 2.4 →
   refused.
5. **Crash:** landing with bank 45° → crash (stun 1500 ms). Flying into a wall at
   v = (−9, 0, −1) with N = (1, 0, 0): |v|² = 82 > 64, dot = −9 < −4 → crash.

---

## 4. Boats

### 4.1 Boarding and leaving (G key, RVA 0x7e8b9)

With a subtype-1 item in the Special slot:

* action ≠ 107 and in water (contact 0x02): action := 107, action time := 0.
* action ≠ 107 and not in water: nothing.
* action = 107: action := 0.

Other ends of action 107:

* *local:* action 107, on ground and not in water: action := 0 (RVA 0x2192e5).
* Special slot emptied while sailing, then G: action := 0 (§3.1 step 1).

No other end of action 107 was found in the step.

### 4.2 Movement while sailing (action 107)

* **Input:** the in-water three-axis rule is skipped for action 107 (RVA 0xa705f), so
  the boat uses the walking rule with A = 80.
* **Speed limit:** in water, S = 6 × (1 + 3·v(n_boat)) (0 for n_boat < 1), × 2 with 0x40.
  The swimming factor is not applied. Out of water neither factor applies (RVA 0x219280).
* **Buoyancy** (RVA 0x21d159): for action 107, b = r + 1 for every r > 0, with no upper
  limit; other creatures use b = r + 1 only for r < 1 and b = 1 otherwise. v.z += 30·dt_s·b.
* **Drag** (RVA 0x21d5d4): in water with action 107, v.x and v.y relax toward 0 at rate
  0.1·R and v.z at rate R, where R = 0.0025 when |intent|² > 0 and 0.005 otherwise
  (RVA 0x21d19e). The ordinary water drag and the swimming pitch rule (RVA 0x21e68e) are
  skipped for action 107.
* The walk-cycle advance at RVA 0x21ea3a is skipped for action 107.
* Name/life-bar anchors: for actions 106 and 107 the update adds 2.0 to the z of an anchor
  vector at RVA 0x9a14e (*believed* to be the name and life-bar anchor).

### 4.3 Worked examples (dt = 16; *derived*)

* Boat skill 1, walk key not held: factor 1.272727 → S = 6·1.272727·2 = **15.273
  blocks/s**; the frame's move is 0.24436 blocks. With a key held the horizontal relax
  rate 0.00025 gives 0.996008 per frame (stored 15.212).
* Released: rate 0.0005 → 0.992029 per frame; the boat coasts about
  15.212·0.016·0.992029/0.007971 ≈ **30.3 blocks**.
* Boat skill 0: S = 0, so the horizontal velocity is capped to 0 every step.
* Boat skill 10: factor 2.5 → S = 30 blocks/s.

---

## 5. Riding pets

### 5.1 The pet (world step, RVA 0x20e485..)

For each creature visited there whose Pet slot holds item type 19:

1. If its pet id (obj +0x11c8) is zero or names no creature, the id is zeroed and then
   taken from the first creature found of controller kind 5 whose owner (obj +0x198)
   equals this creature's id.
2. If the pet exists and either the level obj +0x1138 differs from its copy at obj
   +0x1d44, or the Pet-slot item differs from its copy at obj +0x1d48, the pet is reset and
   rebuilt (RVA 0x20e5bc): type := item subtype; pet obj +0x190 := the item's signed
   16-bit field at item +0x10; pet obj +0x194 := the item's 32-bit field at item +0x04;
   if the item's entry count (item +0x114) is below 16, pet obj +0x1168 receives one byte
   from each entry (the byte at item +0x17 + 8·index) followed by a zero byte;
   owner := this creature's id. The copies are then refreshed every step.
3. If no rebuild happened, the pet id is still zero and 0x10 is clear, a new pet is
   created: its id is min(0, smallest existing creature id) − 1 (64-bit), its owner is
   this creature, controller kind 5, type := item subtype and the same two item fields are
   copied (RVA 0x20e7e3..).

**Rideable types** (RVA 0x44760): 19, 20, 22, 23, 25–40, 63–67, 74, 75, 98–100, 102–105,
151.

### 5.2 Mounting (R key, game update RVA 0x96a59)

For the interaction target, when it is of controller kind 5, of a rideable type, its owner
(obj +0x198) is the player, the player's pet-riding skill level (obj +0x113c) is
non-zero and the player's action is not 106, the prompt "[R] Ride" is shown. When the R
key byte (controller +0x0f) is set and a global block flag is clear: action := 106,
action time := 0. No distance is tested here.

### 5.3 Per step while riding (*local*, action 106; speed-limit stage RVA 0x219306)

In this order:

1. Look up the player's pet (obj +0x11c8). If it exists, its health is not negative and
   its type is rideable: let f = 1 + v(n_ride). If f ≠ 0, S ×= f; if f = 0,
   action := 0. If the pet is missing, has negative health or is not rideable:
   action := 0.
2. **Riding energy:** if the local player's |intent|² > 0, energy −= 0.002·dt_s. If the
   energy is then ≤ 0: energy := 0 and action := 0.
3. If mode 0x01 is set: action := 0 (dismount with the climb key).

### 5.4 The pet under its rider (every creature, RVA 0x21f968 and RVA 0x220f1b)

For each creature of controller kind 5 whose owner exists and has action 106, at two
places in the step:

* offset **o** = (0, 0, 0.5·box_pet.z − 0.5·box_owner.z + 0.01);
* the pet's displayed position (obj +0x1350) := the owner's displayed position + **o**;
  the pet's position (obj +0x10) := the owner's position + **o**;
* copied from the owner: obj +0x1374, orientation (obj +0x28), obj +0x1180, +0x1188,
  +0x118c;
* the pet's velocity and intent := (0, 0, 0).

The life bar of a creature owned by the local player is not drawn while the local player
rides (RVA 0x99f14).

### 5.5 Riding energy refill (RVA 0x21d2dc..0x21d385)

When a creature is not in water, its ground probe in the cells under its box finds a block
of **type 3**, the creature is of controller kind 5 and its owner exists with riding
energy below 1: the owner's energy += 0.05·dt_s, capped at 1. Whether the owner is riding
is not tested.

The riding bar shows min(5, trunc(5·energy + 0.9999)) icons (single precision,
RVA 0x91817).

### 5.6 Seat offset (RVA 0x46950)

The seat offset by pet type has a z component only:

| Pet type | Seat z |
|---|---|
| 26, 30 | −0.9 |
| 33 | −0.2 |
| 74 | −0.3 |
| 75 | +0.1 |
| 151 | −0.35 |
| others | 0 |

### 5.7 Ends of riding

Action 106 ends when: the climb key flag 0x01 is set during a step (§5.3), the riding
energy runs out, the pet is lost, has negative health or is not rideable, the riding
factor is 0, G is pressed with a glider (§3.1), or another action replaces 106.

The total duration of actions 106 and 107 (RVA 0x3d1a0) is the sum of three separately
truncated terms 400/(a·s), 300/(a·s) and 500/(a·s), where s is obj +0x17c and a is the
creature's speed factor (RVA 0x47700): **1200 ms when a·s = 1**. While the action time is
below this, the step clears 0x10 and 0x01 (RVA 0x21ae70).

### 5.8 Worked examples (dt = 16; *derived*)

* Riding skill 1, walk key not held: S = 6·1.090909·2 = **13.09 blocks/s**; skill 5:
  S = 16. Skill 0: no prompt, and an existing ride ends at once.
* Energy: continuous movement drains 0.002 per second, so a full meter lasts **500 s**
  (100 s per icon); standing still costs nothing. Refill on type-3 ground at 0.05 per
  second: empty to full in 20 s.
* Illustrative boxes: a pet of box height 2.0 under a rider of box height 2.16 stands at
  rider z + (1.0 − 1.08 + 0.01) = rider z − 0.07.

### 5.9 The dormant "mount" path

The input routine (RVA 0xa6e4e) directs its keys at the creature named by obj +0x11c0
when that id is non-zero, and the step compares creature ids against it (for example
RVA 0x20f1b1). The only direct writes to obj +0x11c0/+0x11c4 in either image are zeroing
stores: `Cube.exe` RVA 0x3b7aa and RVA 0x46373, `Server.exe` RVA 0x651a and RVA 0x11113.

---

## 6. Climbing

### 6.1 Starting and stopping

* Mode 0x01 = climb key byte held, set when the flag update runs (§2.2). No item is
  required; the climbing skill only changes the stamina cost.
* The input uses the climbing rule only with 0x01 **and** wall contact (RVA 0xa6ec4).
* Cleared in the step by: stamina below 0.01 after the climbing drain (§6.3), hit-stun,
  fall damage (§8), an action within its total duration; and in the update by using an
  ability slot (§2.2). The next flag update sets it again if the key is still held.
* While riding, 0x01 dismounts (§5.3).

### 6.2 Movement

* Input (RVA 0xa6ec4): forward/back add ±0.2·A to intent.z (±16 blocks/s² with A = 80);
  the side keys accelerate along the wall using N (obj +0x11a0).
* Step (RVA 0x21ab04): with 0x01, 0x10 clear and wall contact, **v relaxes toward
  (0, 0, 0) at rate 0.0025** in all three axes and no gravity is applied; fall-start :=
  z when v.z > −10.
* The run ×2 is not applied with 0x01 and wall contact (§3.4 step 2), so a player's limit
  is the base 6 times any other factors.
* Climbing speed does not depend on the climbing skill.

### 6.3 Stamina (speed-limit stage, RVA 0x2194c1 and RVA 0x219cf9)

* If 0x01 is set and |intent|² > 0: k = 1 − 1/(0.1·(n_climb + 1) + 1). If k ≤ 0, clear
  0x01. Otherwise, if 0x10 is clear, the wall contact flag is set and the creature is not
  on ground: stamina −= (1 − k)·0.0005·dt_f; if negative, stamina := 0; then if
  stamina < 0.01, clear 0x01.
* Regeneration later in the same stage: stamina += 0.0001·dt_f, except (a) while 0x10 is
  set and not on ground (§3.4) and (b) while 0x01 is set, 0x10 clear, with wall contact
  and not on ground. The result is clamped to [0, 1].

*Derived:* hanging on the wall with no key held costs nothing and regenerates nothing.
Because the drain and the 0.01 test come before the climbing rule of the same step, a
player whose stamina is below 0.01 + one step's drain cannot hold the wall while a key is
held: 0x01 is cleared before the vertical stage every step, the player falls under full
gravity, and stamina settles at one step's regeneration. Releasing the movement keys
while still holding the climb key keeps 0x01 set and the player hangs.

### 6.4 Orientation (RVA 0x21f0fb)

While 0x01, wall contact and 0x10 clear:

* the head pitch (obj +0x58) relaxes toward **45** at rate 0.005;
* yaw: **u** = −N normalised, with u.y clamped to [−1, 1]; θ = acos(u.y) converted to
  degrees (single-precision acos, conversion in double with π = 3.14159265358979 and 180),
  negated when u.x > 0. Then orientation.z relaxes toward ψ + asin(clamp(sin θ·cos ψ −
  cos θ·sin ψ))·180/π (RVA 0x23f70; the shortest turn) at rate 0.005, ψ = current yaw.

Otherwise the head pitch is multiplied by exp(−0.01·dt_f) every step (RVA 0x21f262).

### 6.5 Worked examples (dt = 16, forward key held; *derived*)

* Per frame v.z ← (v.z + 0.256)·0.96075: v₁ = 0.246, steady **6.27 blocks/s**.
* Skill 0: k = 0.090909, drain 0.00045455 per ms → about **2.2 s** on a full bar
  (≈ 2.18 s to the 0.01 limit), climbing about **11.3 blocks** from rest. Skill 4:
  k = 0.333, 3.0 s; skill 9: k = 0.5, 4.0 s (about 22.6 blocks).
* Facing: N = (0, +1, 0): **u** = (0, −1, 0), θ = 180°. N = (−1, 0, 0): **u** = (1, 0, 0),
  θ = −90°.

---

## 7. Swimming

* In water (contact 0x02) and not sailing, the input uses the three-axis rule
  (RVA 0xa705f); S = 6 × (0.5 + 0.5·v(n_swim)) (0.5 for n_swim < 1), × 2 with 0x40.
* Water drag (RVA 0x21e68e): v relaxes toward 0 at rate 0.005 in all three axes;
  orientation.x relaxes toward −60 at rate 0.01, except for controller kind 6 or when
  bit 0x01 of obj +0x7e is set.
* Stamina regenerates normally in water; no swimming stamina cost exists in the stamina
  code.
* Examples (*derived*): n_swim = 0 → S = 6 (3 with the walk key); n_swim = 3 → 0.615385
  → S = 7.38; n_swim = 10 → S = 9. At dt = 16 the drag leaves S·0.92293 after a frame.

---

## 8. Falls (RVA 0x21c20c..)

On the landing check, when the creature is not in water (contact 0x02 clear), v.z < −5
and trunc₆₄(fall-start × 65536) − pos.z exceeds 20·65536, and the creature is the local
player: health is reduced (amount: §Unresolved), then clamped at 0; hit-stun := max(hit-
stun, 1500 for kind 0 / 3000 − 375·tier otherwise); 0x01 is cleared; sound 23 plays at
volume 1.

Interactions with travel (*derived*):

* Gliding sets the fall-start to the current height every step (§3.4), so a glided
  landing never exceeds the threshold; closing the glider in the air starts the count
  there.
* Climbing keeps the fall-start at the current height while v.z > −10.
* Landing in water never triggers the rule, including from a boat.
* A ridden pet is not the local player and never triggers it; the rider can.

---

## Re-certification (2026-11-15)

Evidence: `Cube.exe` and `Server.exe` image bytes (image base 0x400000 for both) and the
disassembly/decompile of each. Every statement above was read back against that
evidence; statements that could not be confirmed were moved to the list below.

### Confirmed

Skill factor routines and constants (1, 0.1, 3, 1.5, 0.5); rideable list; seat offsets;
gliding test; intent reset and glide steering (10·ĥ, 0.001, 0.05, side vector sign);
mode-flag rules for 0x01 and the base 0x40 rule; G-key glider toggle (2-block fixed-point
test, action := 0) and boat boarding/leaving; all automatic ends of 0x10; A = 8 / 80;
bank ×40 clamp ±90 and relax rates 0.001 / 0.005; glide gravity 3·dt_s, normal gravity
30·dt_s, lift 30·dt_s, fall-start rules; glider-control order, water exit, ground crash
|bank| > 40, wall crash |v|² > 64 and dot < −4; crash stun 1500 / 3000 − 375·tier and
sound 23 at volume 1; wind field constants, offsets and lattice; N2 hash, weights and
interpolation order; boat speed factor, buoyancy, drag rates and skipped water drag; the
beached end; riding factor, energy drain 0.002/s, refill 0.05/s on type 3, initial energy
1, bar formula; R-key conditions and prompt; pet-under-rider offset (½, ½, +0.01);
climbing input 0.2·A, climbing relax 0.0025, fall-start while v.z > −10, stamina drain
(1 − k)·0.0005 and the 0.01 limit, regeneration 0.0001; climbing facing rule; water drag
0.005 and swim pitch −60; fall threshold −5 / 20 blocks, local-only; the zeroing-only
writes of the mount id in both images. All worked-example arithmetic was recomputed.

### Corrected (old → new)

1. Addresses were VAs without an image → stated as `Cube.exe` / `Server.exe` RVAs.
2. **Frame order** "mode flags, then input" → **input, then mode flags**; the input uses the
   previous frame's flags. *Affects movement feel (one-frame flag latency).*
3. New: the input call and the flag update are skipped while the local player's hit-stun
   is > 0 (and in certain interface states); flags keep their values. *Affects movement
   feel.*
4. 0x40 rule lacked the walk-key/action clause → holding the walk key (with the action
   condition, met after 300 ms of action 0) sets 0x04 and clears 0x40 even while gliding
   in the air, cancelling the lift. *Affects movement feel.*
5. Skill offset formula "obj +0x113c + 4·skill index" contradicted its own table → removed;
   per-skill offsets kept.
6. Noise π "the float 3.1415927" → the double-precision decimal constant 3.1415927.
   *Precision.*
7. Blend/Relax precision made explicit (double-precision iterated weight; single-precision
   per-millisecond relax). *Precision.*
8. Pet under rider: "record and displayed position := owner's displayed position + o" →
   displayed from the owner's displayed position, position from the owner's position;
   copied fields listed by offset.
9. New pet id "(lowest existing id) − 1" → min(0, smallest existing id) − 1.
10. Pet rebuild "name from the item's name bytes" and "level" → neutral description of the
    copied item fields.
11. Riding/boat duration "1200 ms" → 400 + 300 + 500 divided by the speed product, each
    truncated; 1200 ms only at unit factors.
12. **Reversal:** exhausted climbing "slides down in short grabs" → with a movement key held
    the player cannot hold the wall and falls; with no key the player hangs. *Affects
    movement feel.*
13. Climbing stamina is clamped to 0 before the 0.01 test (added).
14. Wall crash: lift and wind still apply in that step; ground crash and water exit skip
    them (clarified).
15. Stun side effect "clears the push list" → zeroes the 32-bit field obj +0x144.
16. Fall stun "plus 1500 ms" → raised to at least that value.
17. Head-pitch decay "decays toward 0" → multiplied by exp(−0.01·dt_f) per step.
18. "Key presses that arrive as window messages (G, R …)" → only G is a window message; R
    is a polled key byte in the update.
19. "No item or skill check at the key" → no skill check; the Special slot type and subtype
    are tested.
20. Boat "orientation.x relaxes toward 0" → the −60 swim-pitch rule is skipped; a relax
    toward 0 is unconfirmed.
21. Non-boat buoyancy above r = 1 stated as b = 1 (added for contrast).

### Removed

Implementer advice (the dormant mount path "may be omitted or kept inactive"); the open
questions with logging/recording procedures (old §10); cross-specification resolution
notes (old §9 and the "what this document adds" list); network field numbers and the
net-replication paragraph; the tooltip "Ridable" claim; the sound file name; the
type-3 terrain gloss; pose, drawing, camera, walk-cycle and model-placement statements;
"the pet's AI does nothing while its owner rides"; combat-from-the-saddle reasoning; the
server-has-no-local-player claim; the fall-damage amount and its percentage examples and
the 30-block landing speed.

### Unresolved

* Physical keys behind controller bytes +0x12 (jump), +0x13 (climb), +0x14 (walk) and
  +0x0f (R) were not traced to the DirectInput mapping here.
* The interface/session states of the §2 gate other than hit-stun, and the meaning of the
  global flag tested before mounting.
* The ability-slot condition that clears 0x01 (RVA 0x9c02a) is *believed*, not traced.
* Whether the wall normal N points away from the wall (the facing rule assumes it).
* Contact bit 0x10 ("not fully submerged") and equipment-slot rules (for example that the
  Special slot accepts only type 23).
* Riding energy being saved with the character (reads/writes exist at RVA 0x4be40 /
  0x4d790, not examined).
* The fall-damage amount, the bank-target formula, the integration step and the world-step
  chunk length (described in `alpha-physics-camera.md`; not re-derived here).
* Absence of horizontal/vertical air drag while gliding; whether attacking can replace
  action 106 or 107 after their duration; any end of action 107 by equipping.
* Pose, seat height beyond the offset table, boat/glider drawing, camera focus heights and
  walk-cycle scaling for actions 106/107 (`alpha-creatures-models.md`,
  `alpha-physics-camera.md`).
* Whether the anchor raised by 2.0 at RVA 0x9a14e is the name and life-bar anchor.
* Indirect (block-copy or deserialisation) writes to obj +0x11c0.
* Whether world positions can be negative (relevant to the N2 mirror note).
* Behaviour of the same rules in `Server.exe` other than the mount-id zeroing.
* Sign behaviour of the glide bank feedback (whether a turn sustains itself).
* No random draw occurs in any rule above; the internals of sound playback were not
  examined.
