# Alpha physics and camera: the creature integrator, collision and the follow camera

Behavioural specification of the movement physics of the shared creature step (client
0x60c510, body 0x6184c2..0x61f6da; server twin 0x5322d0) and of the client camera (game
object methods 0x47ea00, 0x47ef40 and the camera block of the game update 0x488ee0 at
0x4965a2..0x4966c9 and 0x49c2e6..0x49cf2a). Written from the Alpha assembly; nothing here
is copied code. This document resolves follow-ups F-1, F-2 and F-3 and open questions
C-1, C-2 and C-3 of `alpha-player-control.md`, and corrects five statements there (§8).

**Server twin.** The server's 0x5322d0 was compared instruction by instruction with the
client's 0x60c510 after addresses were normalised. It is identical apart from one extra
stack adjustment at the start, and every floating-point constant it loads is equal. All
of §1–§6 therefore applies to both programs.

Related: `alpha-player-control.md` (input, intent, jump, dodge), `alpha-creatures-models.md`
§3 (appearance record, collision box), `alpha-creatures-behaviour.md` (actions, P1/P2/P3
durations, skills), `alpha-worldgen-terrain.md` §1 (block reads, solidity).

---

## 0. Conventions

* As in `alpha-player-control.md` §0. "obj +x" is a creature-object offset (record offset
  + 0x10). The fields used most often are position obj +0x10 (3 × i64, 1/65536 block),
  orientation +0x28 (x pitch, y bank, z yaw; degrees), velocity **v** +0x34, intent +0x40,
  push vector **p** +0x4c (record field 4), contact flags +0x5c, controller kind +0x60,
  creature type +0x64, action +0x68, action time +0x6c, appearance +0x78 (flags +0x7e,
  collision box size **s** = (sx, sy, sz) at +0x80), mode flags +0x124, dodge time +0x128,
  hit-stun +0x12c, health +0x16c, stamina +0x1194.
* **dt** is the step length in integer milliseconds. dt_s = dt × 0.001, and dt_f = dt as a
  float.
* **Relax(x → t, r)** means: repeat dt times, x ← x + (t − x)·r. This holds for scalars
  (0x411d40) and for vectors (0x605950). The result is x ← t + (x − t)(1 − r)^dt, computed
  by iteration in single precision.
* **Blend(x → t, r)** (0x4ac150 + mix, used by the camera): w = 1 − (1 − r)^dt, computed by
  iterating in double precision; then x ← (1 − w)·x + w·t.
* **Fixed-point conversion.** Every float-to-position conversion is trunc(f × 65536), with
  truncation toward zero (x87 `_ftol2`). Every position-to-block conversion is
  floor(position / 65536), rounding toward −∞.
* **Box cells.** For a position P and size **s**, lo = floor(P − trunc(s/2 · 65536)) and
  hi = floor(P + trunc(s/2 · 65536)), per axis and inclusive. A box face lying exactly on
  a block boundary therefore includes the block beyond it on the **max** side and
  excludes it on the min side. A creature resting at bottom = k does not overlap block
  k − 1. A creature whose +x face is exactly at m does overlap block m.
* "Solid" means a block type (low 5 bits) other than 0 and 2 (0x43b480). "Water" means
  type 2 (0x434c10).
* Directions: world +z is up. ψ is the camera yaw and θ the camera pitch (degrees, §7).

---

## 1. Time step (resolves C-1)

1. **Frame limiter (main loop 0x4c8ae0).** Each loop iteration reads `timeGetTime`,
   pumps one window message, reads the keyboard and mouse, and runs the frame (0x4c85f0,
   which calls game update vtable +0x28 with dt = the ms elapsed since the previous call).
   If the iteration took less than **minTimeStep** ms (`options.cfg`, game object +0x19c,
   default 16), the loop then `Sleep`s for the rest. So minTimeStep is a **minimum frame
   time**: the frame rate is capped near 1000/16 = 62.5 per second. It does not clamp,
   pad or split the simulation dt.
2. **Slow motion.** If game flag +0x8006e5 is set, dt := dt / 4 (integer division,
   rounded toward zero). If that gives less than 1, dt := 1 and the game sleeps 5 ms.
   What sets the flag is open question Q-9.
3. **World-step splitting (0x48cef4).** If dt > 20, the world step 0x60c510 runs
   n = ⌊(dt − 21)/20⌋ + 1 times with 20 ms, and then once more with the remainder
   dt − 20n (1..20). Otherwise it runs once with dt. **No step is longer than 20 ms.**
4. Inside one creature step, the movement is divided again into equal substeps by distance
   (§4.1).

Test vectors: dt 16 → [16]; dt 20 → [20]; dt 21 → [20, 1]; dt 40 → [20, 20];
dt 45 → [20, 20, 5]; dt 100 → [20, 20, 20, 20, 20].

---
## 2. Creature step: order of operations (per creature, per step)

These run after health regeneration and the per-creature setup at the top of 0x60c510.
Only the parts that affect motion are given in full.

1. **Hit-stun.** stun −= dt. If stun > 0 afterwards: action := 0 and action time := 0.
2. **Action clock** (not for controller kind 6): unless the action time is negative while
   an action is set, action time += dt. Ground slam: for action 54, while
   P1 < time < P1 + X (X = 0x447310, the action's second-phase length), v.z := −60.
3. If an action is set (≠ 0): clear mode 0x10 (gliding). If |intent|² > 0: clear mode
   0x400. If the action is 83 or 84 and |intent|² > 0: action := 0.
4. **Acceleration.** a := intent × dt_s (intent is in blocks/s², §3.1).
   * If stun > 0: a := 0.
   * If mode 0x01 (climbing allowed) is clear, contact bit 0x10 (§5.1) is set and a.z > 0:
     a.z := 0. This stops a swimmer from accelerating up out of the water.
5. **Bank angle** (§3.4).
6. **Speed limit S** (§3.2), stamina (§3.6) and action motion (§3.3).
7. **Integrate.** v += a. Then, if v.x² + v.y² > S²: (v.x, v.y) := S × the unit vector of
   (v.x, v.y). Only the horizontal component is limited; v.z is not.
8. **Vertical forces** (§3.5): climbing, flyers, gliding or gravity, and the fall-start
   height.
9. **Glider control**, for the local player only (§3.7).
10. If stun > 0: clear modes 0x10 and 0x01, and set obj +0x144 := 0. If an action is set
    and action time < its duration: clear modes 0x10 and 0x01.
11. **Displacement.** d := (v + p) × dt_s.
12. **Creature separation** (§4.4) adds to d, except for controller kind 6 and for
    actions 83 and 84.
13. **Swept move and collision** (§4).
14. **Contact flags, water and ground** (§5), then friction (§6), wall probes (§5.4),
    water drag and swim pitch (§5.3), and the climb-over-ledge push (§3.8).
15. Then come the animation-side updates (`alpha-creatures-models.md` §7): dodge time
    −= dt (clamped at 0; for the local player, sound 26 plays when it crosses from above
    500 to 500 or less; resolves F-3), the landing-bob decay (§4.3), and the glider and
    lantern checks (`alpha-player-control.md` F-4).

---

## 3. Intent, speed and forces

### 3.1 Intent becomes velocity (resolves the integrator part of F-1)

Intent is an **acceleration**. The input adds A = 80 (8 while gliding) per held key per
frame (`alpha-player-control.md` §3), and the step adds intent × dt_s to v (step 7). The
hard horizontal limit S then caps the speed. Ground friction is applied *after* the move
(§6). So a player who holds a key on flat ground moves at exactly S during the move of
every frame, and the stored velocity between frames is S·(1 − R)^dt.

The step never clears intent. Who resets it between frames is open question Q-1.

### 3.2 Speed limit S (computed fresh each step, in this order)

| Step | Rule |
|---|---|
| base | S = 6 |
| fast types | S = 8 if the creature type is 101, 107, 108, 109, 114, 115, 116, 117, 118 or 119 (0x4445d0), or appearance flag 0x0200 is set |
| kind 5 | S = 12 for controller kind 5 |
| sailing | action 107 (boat) and in water (contact bit 1): S ×= k_sail with k_sail = 1 + 3·(1 − 1/(0.1·n₅ + 1)) for n₅ ≥ 1, otherwise 0 (n₅ = skill 5, obj +0x114c) |
| swimming | otherwise, in water: S ×= 0.5 + 0.5·(1 − 1/(0.1·n₄ + 1)) for n₄ ≥ 1, otherwise 0.5 (n₄ = skill 4, +0x1148) |
| riding | local player, action 106, mount valid and alive and of a mountable type (0x444760): S ×= 1 + (1 − 1/(0.1·n₁ + 1)) (n₁ = skill 1, +0x113c); if that factor is 0, action := 0. If the mount is invalid or not mountable: action := 0 |
| debug | local player and game flag +0x84 bit 0: S = 20 (Q-8) |
| glider | mode 0x10: g = 1.5 + 3·(1 − 1/(0.1·n₃ + 1)) for n₃ ≥ 1, else 0 (n₃ = skill 3, +0x1144). If g ≤ 0: clear 0x10. Else, if not on ground: S ×= g |
| sneak | action 79: S ×= 0.5 + 0.4·k with k = 1 − 1/(0.1·n₇ + 1) (skill 7) |
| sprint | mode 0x40, unless (climbing and on a wall) or (gliding and in the air): S ×= 2 (computed in double precision) |
| haste buff | buff kind 12 active: S ×= 1 + (1 − 1/(0.1·n₈ + 1)) (skill 8; for NPCs with 0, L/2 + tier + 1) |
| charges | action 48; or action 54 while time ≥ P1 + X; or action 50 or 96 while time < 500: S = 40 |
| lunges | §3.3 |
| kind 4 | controller kind 4: S = 30 |
| channelled | actions 57 and 74 between P1 and their end: S = 0.1 |
| slow / haste | record field 18 (obj +0x134) ≠ 0: S ×= 0.5; field 19 (+0x138) ≠ 0: S ×= 1.5 |
| aiming backwards | mode 0x04, not dodging, and not (action 50 or 96 with time ≤ 1000): if dot(aim, v)_xy < 0, with aim = record field 26 (obj +0x160), then S ×= 1 + 0.5·(â·v̂)_xy (both normalised in xy). Walking straight backwards halves S |

A dodge sets v to 40·d + (0, 0, 5) in the input. Step 7 caps the horizontal part to S
in the same frame, so a dodge moves at S, not 40, for its 600 ms (open question Q-3).

### 3.3 Action motion (overrides a, sets S)

**Melee lunge.** This covers action 48, and actions 1–7, 9–20, 58, 62, 65–67 while their
time is < 200. It runs only for the local player or when the game is not a network
client. The step looks for a hostile, alive creature within contact range: horizontal
distance² < (0.7·their sx + my sx + 1)², and vertical distance < their sz/2 + my sz + 1
(0x5a7b50). If one is found, or the action time is > 500: action 48 ends (action := 0,
or 5, 17 or 20 according to the main-hand weapon subtype; field 28 := 1), and action 47
becomes 54. If **no target is near**, then a += 2S × facing(ψc) with **no dt factor**
(facing in §3.9). The cap in step 7 then brings the horizontal speed to S in the facing
direction.

**Dashes** (a := … replaces a, and S is set):

| Action | Window | a | S |
|---|---|---|---|
| 68, 69, 93 | time ≤ P1 | unchanged | 0 |
| 68, 69, 93 | P1 < time < P1 + ¾·X | 20·facing | 50 |
| 76 | P1 < time < P1 + X | 20·facing | 60 |
| 77 | same | 20·Rz(ψc)(−1, 0, 0) (to the creature's left) | 60 |
| 78 | same | 20·Rz(ψc)(+1, 0, 0) | 60 |
| 71 | time < duration | unchanged | 0 |
| 72 (spin) | time < duration | unchanged; yaw ψc += 720·dt_s | 12 |

Rz(ψ)(x, y, 0) = (x cos ψ − y sin ψ, x sin ψ + y cos ψ, 0).

### 3.4 Bank angle (orientation y)

Let â and v̂ be the xy unit vectors of a and v. If |a_xy|² > 0.01 and |v_xy|² > 0.01:
β = asin(clamp(â.x·v̂.y − â.y·v̂.x, −1, 1)) × (180/π) × 0.5 degrees; otherwise β = 0. If
sx > 2: β ×= 2/sx. While gliding (0x444650): β ×= 40, clamped to ±90, and orientation.y
relaxes toward β at 0.001; otherwise at 0.005.

### 3.5 Vertical forces and gravity

Exactly one of these applies, tested in order:

1. **Climbing**: mode 0x01, not gliding, and contact bit 2 (wall). v relaxes toward 0 at
   **0.0025**, in all three axes. No gravity. If v.z > −10 afterwards, fall-start := z.
2. **Flyer**: appearance flag 0x0002 and stun ≤ 0, **or gliding** (0x444650: mode 0x10,
   v.z < 0, not on ground, stun < 1, dodge < 1). Then v.z −= 3·dt_s and fall-start := z.
3. **Normal**: if v.z ≥ 0, fall-start := z. Then v.z −= **30·dt_s**. Gravity is 30
   blocks/s², and there is **no terminal velocity** apart from water drag.

fall-start (obj +0x13bc, f32 blocks) records the height at which the creature last moved
up or was supported. Fall damage uses it (§4.2).

Climbing speed: intent.z = ±0.2·A = ±16 blocks/s² (`alpha-player-control.md` §3), against
the relaxation at 0.0025. At dt = 16 this gives a steady v.z of 0.256·0.96075/0.03925 ≈
6.27 blocks/s.

### 3.6 Stamina (obj +0x1194, 0..1)

* While climbing (mode 0x01) with |intent|² > 0 and c = 1 − 1/((n₂ + 1)·0.1 + 1) (n₂ =
  skill 2, +0x1140), when not gliding, on a wall and not on ground: stamina −=
  (1 − c)·0.0005·dt_f, clamped at 0. If stamina < 0.01: clear mode 0x01. At skill 0,
  c = 0.0909, so full stamina lasts about 2.2 s of climbing.
* Gliding in the air with mode 0x40: stamina −= 0.0002·dt_f. Gliding in the air without
  0x40: no change.
* Climbing on a wall in the air: no change.
* Otherwise: stamina += 0.0001·dt_f (empty to full in 10 s).
* Then stamina is clamped to [0, 1].

### 3.7 Glider (local player only, mode 0x10)

* In water: clear 0x10.
* On ground: if |bank angle| > 40, the glider **crashes**.
* Otherwise, if touching a wall with |v|² > 64 and dot(wall normal, v) < −4, it also
  **crashes**.
* A crash sets stun := 0x444ff0 (1500 ms for players; 3000 − 375·tier for NPCs), clears
  0x10 and plays sound 23.
* **Lift**: with mode 0x40 and v.z < 0: v.z += 30·dt_s. This cancels glide gravity, so the
  player hovers while the key is held and stamina lasts.
* **Wind**: v += W·dt_s, where W = 0x5a6270(position, v) is a slowly varying field. It is
  two sine-noise terms ×2 for x and y and a noise + 0.5 for z, sampled from the position
  at 1/65536 scale (formula: Q-7).

### 3.8 Climbing over a ledge

Local player only. The test uses the previous step's wall normal N, with mode 0x01 set,
not gliding, a wall last step, no wall now, |intent_xy|² > 0, and dot(facing(ψc), N) ≤ 0
(facing the wall). Then **v := −5·N**: a 5 block/s shove onto the ledge.

### 3.9 Axis and sign conventions (resolves C-2)

* Camera-relative input (0x4a6b50, ψ = camera yaw, game +0x1ac):
  forward **f** = (sin ψ, −cos ψ, 0), right **r** = (−cos ψ, −sin ψ, 0) = f × up.
  W adds +A·f, S adds −A·f, D adds +A·r, A adds −A·r. Mouse right turns right (§7.2).
  The world is right-handed with z up.
* Creature facing used by the step: **facing(ψc) = Rz(ψc)(0, 1, 0) = (−sin ψc, cos ψc, 0)**
  for creature yaw ψc (orientation z). Note facing(ψ) = −f(ψ). Whether the input or the
  renderer adds 180° to ψc is open question Q-2.

---

## 4. Swept box collision (0x61b3f5..0x61cca7)

### 4.1 Substeps and axis order

1. **Unstick first** (0x60ab30, `alpha-player-control.md` §8.1). This runs for the local
   player, or for every creature when the game is not a network client; never for
   controller kind 6.
2. n = trunc(|d| + 1) (the 3-D length of d in blocks), and d := d / n. So a substep
   moves less than 1 block.
3. For each of the n substeps, for axis k = **x, then y, then z**:
   1. pos[k] += trunc(d[k]·65536).
   2. Compute the box cells lo..hi (§0) at the new position. A cell **blocks** if it is
      solid. For a **swimmer** (appearance flag 0x0100), a cell also blocks if it is not
      water. Fish cannot leave the water.
   3. If no block blocked, test the zone's static objects and the nearby voxel objects
      (§4.5). If none of those blocked either, go on to the next axis.
   4. Otherwise apply the axis response (§4.2 for z, §4.3 for x and y).

There is no sweep inside a substep. A substep of up to 1 block can tunnel through a
corner only where the cell test at the end position misses it. Implement exactly this
discrete test; do not use a continuous sweep.

### 4.2 Vertical response (axis z, any direction)

In this order:

1. **Landing sound**: if a local player exists and v.z < −2: sound 32 at the position,
   with pitch 1 + 0.2·r and volume 0.5.
2. **Fall damage** (local player only), when not in water by the previous step's flags,
   v.z < −5, and h > 20, where h = trunc(fall-start·65536) − pos.z in fixed point:
   * damage = ((h/20) − 1 block) × maxHealth × 0.5, evaluated in fixed point and then
     converted to float. In blocks, that is damage = (h/20 − 1)·maxHealth/2.
   * health −= damage, clamped at 0; stun := max(stun, 0x444ff0()) (1500 ms); clear mode
     0x01; sound 23; and a damage record with that amount is sent.
   * Test vectors: falls of 20, 22, 30, 40 and 60 blocks deal 0, 5 %, 25 %, 50 % and
     100 % of maxHealth.
3. Undo the move (pos.z −= trunc(d.z·65536)). Set v.z := 0, p.z := 0 and
   fall-start := pos.z. A ceiling hit also zeroes v.z.

### 4.3 Horizontal response and step-up (resolves the ledge part of F-1)

If the cells blocked on x or y, the creature is not a swimmer, and no object blocked
earlier in this step (an object hit turns step-up off for the rest of the step):

1. **Step-up test.** Take the box at the moved position, raised by 1.01 blocks:
   lo = floor(P − s/2 + (0, 0, 1.01)), hi = floor(P + s/2 + (0, 0, 1.01)). If any cell
   in it is solid, or a voxel object blocks it, the step fails.
2. **Step succeeds.** new bottom := floor(old bottom) + 1.01, so
   pos.z := (floor(pos.z − sz/2) + 1.01 + sz/2) in fixed point. The landing bob
   (obj +0x1180) := bob + (new pos.z − old pos.z) in blocks. Any object base offset
   from §4.5 is added back. Velocity is **unchanged**.
3. **Step fails.** Undo the move on that axis only: pos[k] −= trunc(d[k]·65536).
   Velocity is **not** zeroed. The creature keeps pushing into the wall until friction
   removes the speed.

Step-up does not require being on the ground: an airborne creature that touches a ledge
whose top is less than about 1.01 blocks above its feet climbs onto it.

The **landing bob** decays later in the step (0x61f081): if bob ≥ 0 it relaxes toward 0
at 0.02; if bob < 0, at 0.01. The camera focus subtracts it (§7.3), so steps look smooth.

### 4.4 Creature separation (before the move)

This applies to every other creature that is alive. A player (controller kind 0) is only
pushed by kinds 1 and 6. Let the other creature's box be its size **s′**, enlarged by
(1, 1, 0) unless it is kind 6. If the boxes overlap on all three axes (|Δ| < s′/2 + s/2
per axis):

* u = the horizontal unit vector from the other creature to me (z := 0). If my
  stun > 0: u ×= 0.1.
* d += u × 5·dt_s × (s′x / sx).

### 4.5 Objects

* **Static zone objects** (0x61b72e) are tested in the 3 × 3 sectors around the creature
  (sector = block/8). Object kinds 6, 7 and 9 are skipped. Kinds 1, 2, 3, 5 and 8 count
  only when the object's "active" byte (+0x30) is set. Each object's box (size at +0x24,
  x and y swapped when the object's rotation is odd) is tested axis by axis like a block.
  A hit sets the "touching object" result (contact flag 0x40), turns step-up off, and,
  on z while falling, counts as **standing on an object** (on ground).
* **Voxel objects** within 64 blocks (model 0xa03 group) are tested voxel by voxel in
  their rotated frame. A hit on z records the object's base height offset (re-added after
  a step-up).

These are rarely met outside towns; the exact object tables are follow-up Q-6.

---

## 5. Contact flags, water and ground (0x61cca7..0x61e9e6)

### 5.1 Flags rebuilt after the move

The previous flags are kept (old ground bit 0, old water bit 1, old wall bit 2, and the
old wall normal N). Then flags := 0, and obj +0x145c += dt.

* **0x20 stuck**: stun ≤ 0, |v_xy|² > 16, |intent|² > 16, and the distance moved² <
  0.01 × |d_sub|².
* **0x40 object**: an object blocked during the move.
* **0x10 not fully submerged**: some cell of the box (lo..hi at the final position) is
  not water.
* **0x02 in water** (not for controller kind 6): some cell of the box is water. Let
  waterTop = max(lo.z, max over water cells of z + 1).

### 5.2 Buoyancy

If in water: r = (waterTop − pos.z) / (sz/2), the depth of the box centre below the
water top in half-heights. b = 0 if r ≤ 0; b = r + 1 if 0 < r < 1; b = 1 if r ≥ 1. For
action 107 (boat), b = r + 1 for every r > 0. Then **v.z += 30·dt_s·b**.

Deep water therefore cancels gravity exactly (b = 1). Near the surface the net lift is
30·r blocks/s², which floats the centre up to the water line. Test vector: r = 0.5 gives
a net upward acceleration of 30·1.5 − 30 = 15 blocks/s² (before drag).

### 5.3 Water drag and swim pitch (0x61e684)

* In water and not action 107: **v relaxes toward 0 at 0.005** (all three axes). Unless
  the creature is controller kind 6 or a quadruped (appearance 0x0001), orientation.x
  relaxes toward **−60** at 0.01 (body pitched forward).
* Otherwise: orientation.x relaxes toward 0 at 0.01.
* **Splash**: when water is newly entered (old water bit clear) with v.z < −3: a particle
  burst at the position (kind 15; colour (0.2, 0.7, 1, 1); 0.4; velocity (0, 0, 10)),
  and sound 31 with pitch 0.9 + r/32767 and volume 1.
* Creature type 101: orientation.x relaxes toward 60 during action 51 (time < 1000),
  otherwise toward 0, at 0.01.

### 5.4 Ground, step-down and wall probes

* **Ground** (skipped when in water): z_g = floor(pos.z − sz/2 − 0.2). If any (x, y) of
  the box columns lo..hi at z_g is solid: set bit 0.
* **Step-down snap**: requires the old ground bit, no object hit this step, |d_xy|² > 0,
  now neither ground nor water, and d.z ≤ 0. Probe z_s = floor(pos.z − sz/2 − 1.2). If
  any column is solid there: set bit 0, set the bottom to z_s + 1.01 (so
  pos.z := z_s + 1.01 + sz/2), and bob += Δz. Walking down a 1-block step stays on the
  ground and does not fall.
* Standing on an object (§4.5) also sets bit 0.
* **Wall probes** (bit 2 and normal N, obj +0x11a0). These use world axes, not the
  creature frame, which corrects `alpha-player-control.md` §8.2. The margin is
  m = 0.2 for players; for NPCs the probe sits at ⌊s/2 + 0.5⌋ + 0.5 + 0.05 from the
  centre. Tested in order, each over the box's z range and the box's span on the other
  axis, stopping at the first hit:

  | Probe column | Normal N |
  |---|---|
  | y = floor(pos.y − sy/2 − m) | (0, +1, 0) |
  | y = floor(pos.y + sy/2 + m) | (0, −1, 0) |
  | x = floor(pos.x − sx/2 − m) | (+1, 0, 0) |
  | x = floor(pos.x + sx/2 + m) | (−1, 0, 0) |

  If none of those hit, the four corner columns (x ± (sx/2 + m), y ± (sy/2 + m)) are
  tested next. A corner c = (cx, cy) ∈ {±1}² is tested only when
  dot(facing(ψc), 0.7·(cx, cy)) ≥ −0.1, that is, when the creature is not facing away
  from it. A hit there sets N = (−cx, −cy, 0). N points away from the wall.

---

## 6. Friction and push decay (resolves C-3)

**R = 0.005 per ms**, or **0.0025 per ms while |intent|² > 0** (0x61d19e). There is no
surface-dependent friction; all the sources listed in C-3 are uses of this one value.

In this order, after the ground and water tests:

1. **Boat** (action 107 in water): v.x and v.y relax toward 0 at 0.1·R; v.z relaxes at R.
2. **Ground**: if bit 0 is set, v relaxes toward (0, 0, 0) at R. This is skipped during
   action 48; during action 54 until P1 + X; and during actions 6, 7, 10, 17, 18, 19 and
   20 until P1 (attack wind-ups keep their momentum).
3. **Push p**: relaxes toward 0 at R if the creature is on the ground or in water, or is
   gliding, or obj +0x1460's list is non-empty. A thrown, airborne creature keeps its
   push while that list is empty (Q-5).
4. **Air**: there is no horizontal drag in the air.
5. **Water**: §5.3.

Test vectors (dt = 16): (1 − 0.005)^16 = 0.92293; (1 − 0.0025)^16 = 0.96074;
(1 − 0.0005)^16 = 0.99203.

* Walking (S = 6): the move uses 6 blocks/s, so 0.096 blocks per frame. The stored
  velocity after friction is 5.764.
* Releasing the key: frame 1 stores 6·0.92293 = 5.538 and moves 0.0886; frame 2 moves
  0.0818; and so on. The total coast is about 1.15 blocks.
* Free fall from rest (dt = 16, semi-implicit: v first, then the move): after k frames
  v.z = −0.48k and the drop = 0.00768·k(k + 1)/2. After 1 s (62.5 frames) that is
  v ≈ −30 and a drop of about 15.2 blocks.
* Standing jump, holding the key 100 ms (`alpha-player-control.md` §4): v.z peaks near
  10 − 30·0.1 ≈ 7 to 8 blocks/s, for an apex of roughly 1.8 blocks. Verify this with Q-4.

---

## 7. Camera (resolves F-2)

### 7.1 State (game object)

| Offset | Meaning | Initial |
|---|---|---|
| +0x18c | cameraSpeed (option, int) | 50 |
| +0x190 | cameraSmoothness (option, int) | 100 |
| +0x194 | invertY (option, int) | 0 |
| +0x19c | minTimeStep (option, int; §1) | 16 |
| +0x1a4, +0x1a8, +0x1ac | **current** pitch θ, roll, yaw ψ (degrees) | 120, 0, 0 |
| +0x1b0, +0x1b4, +0x1b8 | **target** pitch, roll, yaw | copied from current |
| +0x1bc, +0x1c0 | current and target follow **distance** (blocks) | 0, 0 (as read: Q-10) |
| +0x1c4, +0x1c8 | current and target map zoom | 1, 1 |
| +0x158 | **focus point** (3 × i64) | — |
| +0x140 | camera position (3 × i64) | — |
| +0x1cc | distance actually used last frame | — |
| +0x1e8 | shake amplitude | 0 |
| +0x1ec / +0x22c | camera-to-world / world-to-view matrices | — |

The options are copied from `options.cfg` (reader 0x4ce6e0, writer 0x4cef80) into the
global block 0x76b1d8. That block is fullscreen, resX, resY, antiAliasing,
renderDistance, soundVolume, musicVolume, cameraSpeed, cameraSmoothness, invertY,
language and minTimeStep, one i32 each in that order. The game object holds a copy at
+0x170, and 0x4c85f0 copies changes back. Roll is never changed by input and stays 0.

### 7.2 Mouse look (vtable +0x18, 0x47ea00)

Each frame the main loop passes the mouse to this method. In gameplay ("look mode":
vtable +4 = 0x47f1d0 returns false when no window, map or menu is open), the cursor is
hidden and re-centred each frame, and the method gets the DirectInput relative counts
(Δx, Δy). Then, with s = cameraSpeed:

* pitch target θ* += s·0.005·Δy (or −= when invertY ≠ 0), then clamped to [0, 180];
* yaw target ψ* −= s·0.005·Δx, with no wrap.

So the sensitivity is **0.005·cameraSpeed degrees per count**: 0.25° at the default 50.
Mouse down looks further down (θ grows), and mouse right turns right.

In cursor mode (a window is open), the same formulas use the cursor's movement since the
last frame, and they apply only while the left button is held and the cursor is not over
a widget (0x650ae0 = 0). When the map is open (game +0x8006e4), the drag moves the map
instead.

### 7.3 Smoothing, zoom and follow (0x4965a2 and 0x49c2e6, once per frame with the frame dt)

1. k = 0.005^(cameraSmoothness/100) (single-precision `pow`). The current angles blend
   toward the targets: Blend((θ, roll, ψ) → (θ*, roll*, ψ*), k), component by component,
   as plain lerps with no angle wrapping. smoothness 0 → k = 1 (instant); 50 →
   k = 0.070711; 100 → k = 0.005.
2. Distance: Blend(dist → dist*, 0.004). Map zoom: Blend(+0x1c4 → +0x1c8, 0.004).
3. **Zoom input** (vtable +0x24, 0x47ef40, WM_MOUSEWHEEL): each message gives
   n = sign(wheel delta) (±1). Then dist* −= 2n, clamped to [**0, 14**]. Wheel up zooms
   in. On the map, the map zoom is ×0.9 per step up and ÷0.9 per step down, clamped to
   [0.01, 10].
4. **Character-screen orbit.** While inventory-type screens are open (game +0x800874 or
   +0x800894 visible and +0x800898 not), each frame sets dist* := 10 and θ* := 100, and
   adds ψ* += 0.005·dt.
5. **Focus** (skipped while the map or the screens +0x800874, +0x800888, +0x800894 or
   +0x80088c are open):
   * If |focus − player position|² > 2500 (more than 50 blocks): focus := player position.
   * focus.x and focus.y Blend toward the player's x and y at **0.025**. The
     multiplication is in fixed point: focus += trunc(Δ·w).
   * focus.z Blend toward a target at **0.005**, where:
     * normally, target = pos.z − bob + sz/2 + **0.5** (half a block above the head);
     * riding or in a boat (action 106 or 107), target = pos.z − bob + sz/2 + **1.5**;
     * sitting or sleeping (action 83 or 84), target = pos.z − bob + sx/2;
     * while screens +0x800880 or +0x800890 are open, target = pos.z − bob.
6. **Direction.** M = Rz(ψ)·Ry(roll)·Rx(θ), applied to rows (0x424610, 0x4244f0,
   0x4243d0). The camera's z row is the **view direction**
   d = (sin ψ sin θ, −cos ψ sin θ, cos θ). θ = 90 looks horizontally along f (§3.9);
   θ = 120 looks 30° down; θ = 0 straight up; θ = 180 straight down.
7. **Desired position.** P = focus − dist·d. The camera sits behind the focus and looks
   along +d at it.
8. **Block collision.** For each of the 8 corners c of a cube of side 1 centred on P
   (offsets ±0.5 per axis): if the cell at P + c is solid, cast a ray from P + c along d,
   with a maximum length of dist, that **runs while the cells are solid** (0x5a35d0 with
   "continue while solid" = 1, objects included, at most 500 cells). It returns the
   length h travelled to free space. The candidate is dist − h. Keep the minimum over
   all 8 corners (starting from dist). Then **current dist := that minimum** (written
   into +0x1bc, so step 2 eases it back out at 0.004 on later frames), and P :=
   focus − dist·d. +0x1cc := dist.
9. **Fallback.** If the cell at P is still solid: cast from the player's position toward
   P (unit vector u), stopping at the first solid cell within 100 blocks. If a hit
   distance t ≥ 0 is found, P := player position + u·t.
10. **Matrices.** +0x1ec = T(P)·Rz(ψ)·Ry(roll)·Rx(θ); +0x22c = Rx(−θ)·Ry(−roll)·Rz(−ψ)·T(−P).
11. **Shake.** A third matrix adds offsets 0.5·a·cos(0.04·T) and 0.5·a·cos(0.03·T)
    (a = +0x1e8; T = the world clock in ms, world +0x8000bc). After that,
    Blend(a → 0, 0.005). What sets a is Q-11.

There is **no separate first-person mode**. With dist* = 0 the camera sits at the focus,
half a block above the head, and the same collision applies. Whether the player model is
hidden then is Q-10.

### 7.4 Projection (render, 0x4ae5b0 → 0x427910)

Perspective, built by hand: **vertical field of view 50°** (0.8726647 rad), or **30°**
(0.5235988 rad) while the local player has mode flag 0x400 (the switch is instant, with
no smoothing). aspect = −(resX/resY); the negative sign mirrors x for the left-handed
view space. near = **0.1**, far = **2000**. The matrix is m00 = (1/tan(fov/2))/aspect,
m11 = 1/tan(fov/2), m22 = far/(far − near), m23 = 1, m32 = −near·far/(far − near). With
the map open, the renderer uses pitch 90 for its sky term instead of θ.

### 7.5 Camera test vectors (dt = 16)

* Angle smoothing at k = 0.005: w = 1 − 0.995^16 = 0.07707. A 90° yaw flick closes to
  90·(0.92293)^n remaining after n frames: 50 % after 9 frames.
* At k = 0.070711 (smoothness 50): w = 0.6907.
* Focus xy: w = 1 − 0.975^16 = 0.3331. Focus z: w = 0.07707. Distance: w = 1 − 0.996^16
  = 0.06208.
* Mouse: Δx = +40 counts at speed 50 → ψ* −= 10°.
* With θ = 120, ψ = 0 and dist = 10: d = (0, −0.8660, −0.5), so P = focus + (0, 8.660, 5).

---

## 8. Corrections to `alpha-player-control.md`

1. §2/§10 C-1: minTimeStep is the frame limiter (§1 here). The world step runs in chunks
   of 20 ms or less.
2. §3 and §8.4: the step does not clear intent. The addresses 0x60d842 and 0x60d89f
   cited there belong to the projectile loop.
3. §7: health regeneration applies to the local player only while its action is **84**
   (resting; 0x60c595). At the same time the world clock advances 100 × dt instead of
   10 × dt.
4. §8.2: the wall probes are world-axis aligned (§5.4 here), with diagonal probes chosen
   by facing.
5. §8.3: R is 0.005 per ms, or 0.0025 per ms while intent is non-zero (§6 here).

---

## 9. Open questions (each with a check against the running game)

* **Q-1 Intent reset.** Nothing in the step clears intent, yet keys add to it every
  frame. Hook 0x60c510 entry and exit and log the local player's obj +0x40 over several
  frames while holding and then releasing W. Expect it reset once per frame somewhere in
  the input or network code; find the writer with a hardware write breakpoint.
* **Q-2 Creature yaw.** The step's facing(ψc) = (−sin ψc, cos ψc) is the opposite of the
  camera forward f(ψ). Log orientation.z and the camera yaw while walking forward. Check
  whether ψc = ψ + 180 or is derived from velocity, and which way an attack lunge moves.
* **Q-3 Dodge speed.** By the reading in §3.2, the 40 block/s dodge is capped to S.
  Record a dodge with the logger: horizontal |v| on the frame after the press, and the
  distance covered in 600 ms (expect 6 × 0.6 ≈ 3.6 blocks plus coasting).
* **Q-4 Jump apex.** Record pos.z for a tap jump and for a 100 ms hold on flat ground at a
  fixed 16 ms frame. Compare with the integration of `alpha-player-control.md` §4 plus
  §3.5 here.
* **Q-5 Push list obj +0x1460.** Identify the list (knock-back sources?) and check whether
  a knocked-back airborne creature keeps p. Hit a creature into the air and log p.
* **Q-6 Object collision tables.** Confirm the object kinds 1, 2, 3, 5, 6, 7, 8 and 9 and
  the voxel-object test (§4.5) by walking into a town door, a table and a fence while
  logging contact flag 0x40.
* **Q-7 Glider wind.** Log v while gliding with no input at a fixed position and time to
  sample W; compare with the reading of 0x5a6270.
* **Q-8 Game flag +0x84 bit 0** (S = 20). Find what sets it (a debug key?) by watching the
  flag while pressing the function keys.
* **Q-9 Game flag +0x8006e5** (¼ speed). Find the setter the same way.
* **Q-10 Initial distance and first person.** The constructor ends with dist = dist* = 0.
  Load a world, read game +0x1bc and +0x1c0, and note whether the player model is drawn
  at distance 0.
* **Q-11 Camera shake** (+0x1e8). Watch it during hits, explosions and landings to find
  the writers and their amplitudes.
* **Q-12 Mode flag 0x400.** It narrows the field of view to 30° and is cleared by any
  intent. Find the input that sets it (a zoom key? sitting?) by logging +0x124 while
  trying each binding.
