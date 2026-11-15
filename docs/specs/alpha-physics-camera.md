Status: FINAL / STABLE / HELD

# Alpha physics and camera: the creature integrator, collision and the follow camera

Behavioural specification of the movement physics performed by the shared creature step
and of the client follow camera. Written from the original Alpha images; it contains no
transcribed code.

**Images and addresses.** Every address is an RVA. Unless marked otherwise it is an RVA in
Cube.exe (SHA-256 84a7a132…30bf, image base 0x400000). Server.exe (SHA-256 1ffbcbce…4fb1,
image base 0x400000) is named explicitly where it is meant.

* Creature step: Cube.exe RVA 0x20c510; the movement body covered here is RVA
  0x2184c2..0x21f6da. Server twin: Server.exe RVA 0x1322d0.
* Camera: mouse-look method RVA 0x7ea00, wheel-zoom method RVA 0x7ef40, and the camera
  blocks of the game update (RVA 0x88ee0) at RVA 0x965a2..0x966c9 and 0x9c2e6..0x9cf2a.

**Server twin.** After normalising image-specific addresses (jump tables, import slots,
string and data addresses), the Server.exe routine at RVA 0x1322d0 matches the Cube.exe
routine at RVA 0x20c510 instruction for instruction. Every floating-point constant it loads
has the same bit pattern. The only differences are one extra stack adjustment near the
start, in the per-step setup before the creature loop, and the import slots, which resolve
to the same imports (rand, srand). So §1.2–§6 describe both programs.

Related specifications (pointers only): `alpha-player-control.md` (input, intent, jump,
dodge, unstick), `alpha-creatures-models.md` §3 (appearance record, collision box),
`alpha-creatures-behaviour.md` (actions, phase durations, skills), and
`alpha-worldgen-terrain.md` §1 (block reads, solidity).

---

## 0. Conventions

* "obj +x" is an offset in the creature object. The creature record starts at obj +0x10,
  so the mode-flag helper that edits record +0x114 is editing obj +0x124. The fields used
  are:

  | Field | Offset | Width |
  |---|---|---|
  | position **P** | +0x10 | 3 × i64, 1/65536 block |
  | orientation (x pitch, y bank, z yaw; degrees) | +0x28 | 3 × f32 |
  | velocity **v** | +0x34 | 3 × f32 |
  | intent | +0x40 | 3 × f32 |
  | push vector **p** | +0x4c | 3 × f32 |
  | contact flags | +0x5c | u32 |
  | controller kind | +0x60 | u8 |
  | creature type | +0x64 | i32 |
  | action | +0x68 | u8 |
  | action time | +0x6c | i32 ms |
  | appearance flags | +0x7e | u16 |
  | collision box size **s** = (sx, sy, sz) | +0x80 | 3 × f32 |
  | mode flags | +0x124 | u16 |
  | dodge time | +0x128 | i32 ms |
  | hit-stun | +0x12c | i32 ms |
  | health | +0x16c | f32 |
  | stamina | +0x1194 | f32 |

* **dt** is the step length in integer milliseconds. dt_f is dt converted to single
  precision, and dt_s = dt_f × 0.001, also in single precision.
* **Relax(x → t, r)** means: dt times in succession, x ← x + (t − x)·r, in single precision
  (scalar RVA 0x11d40, three-component vector RVA 0x205950). Mathematically this is close
  to x ← t + (x − t)(1 − r)^dt, but the native result is the iterated single-precision
  value.
* **Blend(x → t, r)** (camera; weight routine RVA 0xac150): the weight w starts at 0 and is
  moved toward 1 at rate r, dt times, in double precision. w is then rounded to single, and
  x ← (1 − w)·x + w·t in single precision.
* **Float to fixed point.** The float is multiplied by 65536 in single precision and then
  truncated toward zero (conversion helper RVA 0x28d946). A vector is converted component
  by component.
* **Fixed point to float.** The 64-bit value is converted to single precision first and
  then multiplied by 2^-16. Large coordinates therefore lose low-order bits.
* **Fixed point to block index** comes in two kinds:
  * The *box* conversion (RVAs 0x2f040 and 0x2f100) divides by 65536, truncating toward
    zero, and then subtracts 1 whenever the value is negative. This equals floor(P/65536),
    except at exact negative multiples of 65536, where it gives one less.
  * The *plain* conversion (RVA 0x2f570) divides by 65536 and truncates toward zero.

  Each use below names the kind it uses.
* **Box cells.** For position P and size s, each axis converts trunc((s/2)·65536) to fixed
  point, forms P − half and P + half, and applies the box conversion to each. The range
  lo..hi is inclusive. For non-negative coordinates this behaves as floor: a min face lying
  exactly on boundary k excludes block k − 1, and a max face lying exactly on m includes
  block m. At an exact negative multiple of 65536 both outcomes shift down by one block.
* "Solid" means that the low 5 bits of block byte +3 are neither 0 nor 2 (RVA 0x3b480).
  "Water" means block type 2 (type reader RVA 0x34c10).
* World +z is up. ψ is the camera yaw and θ the camera pitch (degrees, §7).
* P1 is the action's first-phase time (RVA 0x3caa0), X its second-phase length
  (RVA 0x47310), and "duration" its total time (RVA 0x3d1a0). Their tables are covered by
  `alpha-creatures-behaviour.md`.

---

## 1. Time step

1. **Main loop (RVA 0xc8ae0).** After calling timeBeginPeriod(1), each iteration reads
   timeGetTime and handles at most one window message. While the window has focus it then
   reads the DirectInput keyboard and mouse and passes the mouse to the game's vtable +0x18
   method. It then runs the frame (RVA 0xc85f0). The frame calls the game update (vtable
   +0x28) with dt equal to the milliseconds elapsed since the previous frame's timeGetTime,
   and then calls vtable +0x2c. If the iteration took less than **minTimeStep** ms (game
   object +0x19c), the loop Sleeps for the remainder. minTimeStep is therefore a minimum
   frame time. It does not clamp, pad or split the simulation dt. The built-in default is
   **9** ms, which caps the rate near 111 frames per second (§7.1). options.cfg may
   override it.
2. **Slow motion (RVA 0x8c104).** If game byte +0x8006e5 is non-zero, dt is divided by 4
   (signed, truncating toward zero). If the result is below 1, dt becomes 1 and the game
   Sleeps 5 ms. What sets the byte is unresolved.
3. **World-step splitting (RVA 0x8cef4).** If dt > 20, the creature step runs
   n = ⌊(dt − 21)/20⌋ + 1 times with 20 ms, and then once more with the remainder
   dt − 20n (1..20). Otherwise it runs once with dt. No step is longer than 20 ms. Example
   splits: 16 → [16]; 20 → [20]; 21 → [20, 1]; 40 → [20, 20]; 45 → [20, 20, 5];
   100 → [20, 20, 20, 20, 20].
4. Within one creature step, the movement is divided again into equal substeps by
   distance (§4.1).

---

## 2. Creature step: order of operations (per creature, per step)

Earlier in RVA 0x20c510 (§8) come health regeneration, the clocks and other per-creature
work. The movement body then runs in this order. Parts that do not affect motion are only
named.

1. **Hit-stun.** stun −= dt. If stun is then above 0, action := 0 and action time := 0.
2. **Action clock**, skipped for controller kind 6:
   * When the action time is negative while an action is set, the clock does not run and
     the action-0 bookkeeping is skipped. Otherwise action time += dt. When the time was
     exactly 0, the action is first recorded in the table at obj +0x139c.
   * **Ground slam.** While the action is 54 and P1 < time < duration and time < P1 + X,
     v.z := −60.
3. With action 0, a pending list at obj +0x11b4 is processed (not motion). Two meters are
   then updated: obj +0x170, and obj +0x174, which gains dt_f/2000 or loses dt_f/600 (or
   dt_f/1200 for a specific weapon). These are not motion.
4. If an action is set, mode 0x10 (gliding) is cleared. If |intent|² > 0, mode 0x400 is
   cleared. If the action is 83 or 84 and |intent|² > 0, action := 0.
5. Consumption actions 80 and 81 run their healing ticks (not motion).
6. **Acceleration.** a := intent × dt_s (intent is in blocks/s², §3.1).
   * If stun > 0, a := 0.
   * If mode 0x01 is clear, contact flag 0x10 (§5.1, kept from the previous step) is set
     and a.z > 0, then a.z := 0. A swimmer cannot accelerate upward out of the water this
     way.
7. **Bank angle** (§3.4).
8. **Speed limit S** (§3.2), stamina (§3.6) and action motion (§3.3), interleaved in the
   order of the §3.2 table.
9. **Integrate.** v += a. Then, if v.x² + v.y² > S², (v.x, v.y) := S × the unit vector of
   (v.x, v.y). Only the horizontal part is limited; v.z is not.
10. **Vertical forces** (§3.5), including the fall-start height.
11. **Glider control**, local player only (§3.7).
12. If stun > 0, modes 0x10 and 0x01 are cleared and obj +0x144 := 0. If an action is set
    and its time is below its duration, modes 0x10 and 0x01 are cleared.
13. **Displacement.** d := (v + p) × dt_s.
14. **Creature separation** (§4.4) adds to d. It is skipped for controller kind 6 and for
    actions 83 and 84.
15. **Substepped move and collision** (§4).
16. **Contact flags, water and ground** (§5.1, §5.2, §5.4), then friction and push decay
    (§6), wall probes (§5.4), water drag and swim pitch (§5.3), the climb-over-ledge push
    (§3.8), and the type-101 pitch (§5.3).
17. **Attachment** (RVA 0x21e9b5). If obj +0x11c0 names a creature that has health
    ≤ 0, the reference is cleared. If it names a living creature and this creature is not
    controller kind 5, the creature is attached to it:
    * P := its position + (0, 0, 1.5 − its bob);
    * v := 0 and intent := 0;
    * orientation := its orientation + (20, 0, 0);
    * the animation fields are copied from it (obj +0x1188 halved).

    Two further branches after the body (RVAs 0x21fb72 and 0x2210fa) also copy pose from
    another creature and zero v and intent.
18. Animation-side updates follow (`alpha-creatures-models.md` §7), in this order:
    1. The walk-cycle fields (obj +0x1188, +0x118c) advance.
    2. The glider and lantern equipment checks run (RVA 0x21f008): mode 0x10 is cleared
       unless equipment byte +0xf08 is 0x17 and +0xf09 is 0, and mode 0x200 is cleared
       unless byte +0xdf0 is 0x18.
    3. The landing bob decays (§4.3).
    4. Climbing-orientation relaxations run while climbing on a wall.
    5. Dodge timing: if a local player exists (game +0xb8 non-zero) and this creature's
       dodge time is above 500 and dodge − dt ≤ 500, sound 26 plays at this creature's
       position. Then obj +0x1318 := dodge time, dodge −= dt if it is non-zero, and the
       result is clamped at 0.

---

## 3. Intent, speed and forces

### 3.1 Intent becomes velocity

Intent is an acceleration in blocks/s². The input routine (RVA 0xa6b50) adds A = 80 per
held movement key, or 8 while the gliding test (RVA 0x44650) holds. The step adds
intent × dt_s to v, and S then caps the horizontal speed. Ground friction comes after the
move (§6). When intent is large enough to reach the cap each step, a player holding a key
on flat ground therefore moves at exactly S during the move, and the velocity stored after
friction is S·(1 − 0.0025)^dt.

Intent is left unchanged by the step except in the attachment branches (§2 item 17). What
resets it between frames is unresolved.

### 3.2 Speed limit S (computed fresh each step, in this order)

The skill factor k(n) below means 1 − 1/(0.1·n + 1).

| Step | Rule |
|---|---|
| base | S = 6 |
| fast types | S = 8 if the creature type is 101, 107, 108, 109, 114, 115, 116, 117, 118 or 119 (RVA 0x445d0), or appearance flag 0x0200 is set |
| kind 5 | S = 12 for controller kind 5 |
| sailing | action 107 and in water (flag 0x02): S ×= 1 + 3·k(n₅) for n₅ ≥ 1, else × 0 (n₅ = obj +0x114c; RVA 0x3e2c0) |
| swimming | otherwise, in water: S ×= 0.5 + 0.5·k(n₄) for n₄ ≥ 1, else × 0.5 (n₄ = obj +0x1148; RVA 0x476a0) |
| boat on land | local player, action 107, ground flag set and water flag clear: action := 0 |
| riding | local player and action 106. The mount is looked up by the id at obj +0x11c8 and must exist, have health ≥ 0 and be of a mountable type (RVA 0x44760). Then the factor f = 1 + k(n₁) for n₁ ≥ 1, else 0 (n₁ = obj +0x113c; RVA 0x46aa0). If f = 0, action := 0; otherwise S ×= f. If the mount is missing, dead or not mountable, action := 0. While still in this branch: if the local player's |intent|² > 0, obj +0x1198 −= 0.002·dt_s; if obj +0x1198 ≤ 0 it becomes 0 and action := 0; and if mode 0x01 is set, action := 0 |
| debug | local player and game byte +0x84 bit 0: S = 20 |
| glider | mode 0x10: g = 1.5 + 3·k(n₃) for n₃ ≥ 1, else 0 (n₃ = obj +0x1144; RVA 0x3f770). If g ≤ 0, mode 0x10 is cleared. Otherwise, if the ground flag is clear, S ×= g |
| sneak | action 79: S ×= 0.5 + 0.4·k(n), with n the action's skill (RVA 0x3ed60; action 79 maps to skill index 7, obj +0x1154). For a non-player with skill 0, n = obj +0x190 / 2 + tier (obj +0x1a8) + 1 |
| sprint | mode 0x40, unless (mode 0x01 and wall flag 0x04) or (mode 0x10 and ground flag clear): S ×= 2, computed in double precision and rounded to single |
| haste buff | query RVA 0x43f10 with argument 12 non-zero: S ×= 1 + k(n) with n from RVA 0x3ed60 for id 100 (skill index 8, same non-player fallback) |
| charges | S = 40 for action 48; for action 54 while time < P1 + X; and for action 50 or 96 while time < 500 |
| lunge | §3.3 (adds to a; S unchanged) |
| action 81 | while time ≤ the length given by RVA 0xc6b80 for obj +0x1e8: S = 0; after that, action := 83 |
| dodge cancel | dodge time ≠ 0, and not (action 50 or 96 with time ≤ duration). If bytes obj +0x140 = 4 and +0x141 = 1, action := 0 unless it is 5, 17 or 20. Otherwise action := 0 and obj +0x144 := 0 |
| dashes | §3.3 (sets S) |
| kind 4 | controller kind 4: S = 30 |
| channelled | actions 57 and 74 while P1 ≤ time < duration: S = 0.1 |
| slow / haste | obj +0x134 ≠ 0: S ×= 0.5; then obj +0x138 ≠ 0: S ×= 1.5 |
| aiming backwards | mode 0x04, dodge time = 0, and not (action 50 or 96 with time ≤ 1000). Let aim be obj +0x160 in xy. If dot(aim, v)_xy < 0 (v as before integration), S ×= 1 + 0.5·(â·v̂), both normalised in xy. Walking straight backward halves S |

The dodge impulse set by input is v := 40·f + (0, 0, 5), with f the camera-relative
forward of §3.9. Step 9 caps the horizontal part to S in the same step, so in this step a
dodge moves horizontally at S, not at 40.

### 3.3 Action motion

**Melee lunge (RVA 0x219e60).** This applies to action 48 at any time, and to actions
1–7, 9–20, 58, 62 and 65–67 while their time is below 200. It runs for the local player,
or for any creature when game byte +0xb4 is zero. The step looks for another creature that
is alive (health > 0), passes the hostility test RVA 0x1988a0, and is within contact range
on both axes:

* horizontal distance², in fixed point, below (0.7·their sx + my sx + 1)² (RVA 0x1a7b50);
* |Δz| < their sz/2 + my sz + 1.

If such a target is found, or the action time exceeds 500, then for action 48:

* obj +0x13b4 := 0, obj +0x170 := 1, action := 0 and time := 0;
* if main-hand byte +0xaa8 is 3, the action instead becomes 5 when subtype +0xaa9 is 5,
  17 when it is 3, and 20 otherwise.

An action-47 → 54 conversion sits in the same branch, but action 47 never enters this
block. If no target is found, a += Rz(ψc)(0, 2S, 0), with no dt factor (facing in §3.9).
This includes the step on which action 48 times out. The cap in step 9 then brings the
horizontal speed to S along the facing.

**Dashes and spins (RVA 0x21a266 onward).** In these rows "a := …" replaces a, and S is
set. Each row with a window also marks the action as moving.

| Action | Window | a | S |
|---|---|---|---|
| 68, 69, 93 | time ≤ P1 | unchanged | 0 |
| 68, 69, 93 | P1 < time < P1 + trunc(3X/4) | 20·facing | 50 |
| 77 | P1 < time < P1 + X | 20·Rz(ψc)(−1, 0, 0) | 60 |
| 78 | P1 < time < P1 + X | 20·Rz(ψc)(+1, 0, 0) | 60 |
| 76 | P1 < time < P1 + X | 20·facing | 60 |
| 71 | time < duration | unchanged | 0 |
| 72 (spin) | time < duration | unchanged; yaw ψc += 720·dt_s | 12 |

Rz(ψ)(x, y, 0) = (x cos ψ − y sin ψ, x sin ψ + y cos ψ, 0), with ψ converted to radians
in single precision and the sine and cosine evaluated in double precision.

### 3.4 Bank angle (orientation y)

Let â and v̂ be the xy unit vectors of a and v, with v taken before integration. If
|a_xy|² > 0.01 (compared in double precision) and |v_xy|² > 0.01, then
β = asin(clamp(â.x·v̂.y − â.y·v̂.x, −1, 1)) × (180/π) × 0.5 degrees. The asin and the
scaling are evaluated in double precision. Otherwise β = 0.

* If sx > 2, β ×= 2/sx.
* While gliding (RVA 0x44650), β ×= 40 and is clamped to ±90, and orientation.y relaxes
  toward β at 0.001. Otherwise it relaxes at 0.005.

### 3.5 Vertical forces and gravity

Exactly one of these applies, tested in order:

1. **Climbing**: mode 0x01, mode 0x10 clear, and wall flag 0x04 (previous step's flags).
   v relaxes toward (0, 0, 0) at **0.0025** on all three axes, with no gravity. If
   v.z > −10 afterwards, fall-start := z.
2. **Flyer or glider**: either appearance flag 0x0002 with stun ≤ 0, or the gliding test
   RVA 0x44650 (mode 0x10, v.z < 0, ground flag clear, stun ≤ 0 and dodge ≤ 0). Then
   v.z −= (dt_s·0.1)·30, which is 3·dt_s, and fall-start := z.
3. **Normal**: if v.z ≥ 0, fall-start := z. Then v.z −= **30·dt_s**. Gravity is 30
   blocks/s², and nothing in the air limits the fall speed. Only water drag does.

fall-start (obj +0x13bc, f32 blocks, converted from P.z as in §0) records the height at
which the creature last moved upward or was supported. Fall damage uses it (§4.2).

Climbing speed: the climb input adds intent.z = 0.2·A = 16 blocks/s² while W is held on a
wall. Against the relaxation at 0.0025, a fixed dt of 16 gives a steady stored v.z of
0.256·0.96075/0.03925 ≈ 6.27 blocks/s.

### 3.6 Stamina (obj +0x1194, 0..1)

* **Climbing drain (RVA 0x2194c1).** With mode 0x01 and |intent|² > 0, let
  c = 1 − 1/((n₂ + 1)·0.1 + 1) (n₂ = obj +0x1140; RVA 0x3e660). If c ≤ 0, mode 0x01 is
  cleared; this cannot happen for n₂ ≥ 0. Otherwise, when mode 0x10 is clear, the wall
  flag is set and the ground flag is clear: stamina −= (1 − c)·0.0005·dt_f, clamped at 0,
  and if stamina < 0.01 mode 0x01 is cleared. At skill 0, c = 0.0909, so full stamina
  lasts about 2.2 s of climbing.
* **Regeneration block (RVA 0x219cf9):**
  * gliding with the ground flag clear: with mode 0x40, stamina −= 0.0002·dt_f; without
    0x40, no change;
  * otherwise, climbing on a wall with the ground flag clear and mode 0x10 clear: no
    change;
  * otherwise: stamina += 0.0001·dt_f (empty to full in 10 s).
* Stamina is then clamped to [0, 1].

### 3.7 Glider control (local player only, mode 0x10; RVA 0x21ac47)

The flags and wall normal N (obj +0x11a0) used here are those left by the previous step.

* In water (flag 0x02): mode 0x10 is cleared and nothing else here applies.
* On the ground (flag 0x01): if |orientation.y| > 40, the glider crashes.
* In the air: if the wall flag is set, |v|² > 64 and dot(N, v) < −4, the glider crashes.
  Then:
  * **Lift**: with mode 0x40 and v.z < 0, v.z += 30·dt_s. This cancels glide gravity, so
    the player can hover while the key is held and stamina lasts.
  * **Wind**: v += W·dt_s (RVA 0x1a6270). W depends only on the horizontal position, not
    on velocity or time. Each component is one call of the two-input noise routine at
    RVA 0xc0ef0. The inputs are the position x and y in fixed point, each scaled in double
    precision, truncated, offset and multiplied by 2^-16:
    * W.x = 2 × noise at (0.05·x, 0.05·y);
    * W.y = 2 × noise at (3455 + 0.05·x, 8438 + 0.05·y);
    * W.z = noise at (4234 + 0.02·x, 5345 + 0.02·y) + 0.5.

    Here x and y are in blocks, and the first noise input is the x term. The offsets are
    0xd7f0000 and 0x20f60000 (for W.y) and 0x108a0000 and 0x14e10000 (for W.z) in fixed
    point. The formula of the noise routine is unresolved.
* A crash sets stun := the value of RVA 0x44ff0, clears mode 0x10 and plays sound 23. The
  value is 1500 ms for controller kind 0, and 3000 − trunc(1500·tier/4) = 3000 − 375·tier
  otherwise, with tier = obj +0x1a8.

### 3.8 Climbing over a ledge (RVA 0x21e8a2)

This applies to the local player only. Let N be the wall normal saved at the start of this
step's flag rebuild, which is the previous step's normal. The conditions are:

* dot(facing(ψc), N) ≤ 0;
* mode 0x01 set and mode 0x10 clear;
* the previous step's wall flag set and no wall flag now;
* |intent_xy|² > 0.

Then **v := −5·N**, a 5 block/s shove onto the ledge.

### 3.9 Axis and sign conventions

* **Camera-relative input** (RVA 0xa6b50, ψ = camera yaw at game +0x1ac): forward
  **f** = (sin ψ, −cos ψ, 0) and right **r** = (−cos ψ, −sin ψ, 0) = f × up. W adds +A·f,
  S adds −A·f, D adds +A·r, and A adds −A·r. Mouse movement to the right turns right
  (§7.2). The world is right-handed with z up.
* **Creature facing** used by the step: facing(ψc) = Rz(ψc)(0, 1, 0) = (−sin ψc, cos ψc, 0)
  for creature yaw ψc (orientation z). Note that facing(ψ) = −f(ψ).

---

## 4. Substepped box collision (RVA 0x21b3f5..0x21cca7)

### 4.1 Substeps and axis order

1. n = trunc(|d| + 1), with |d| the 3-D length of d in blocks. d is multiplied by the
   single-precision reciprocal 1/n, so each substep moves less than one block.
2. **Unstick** (RVA 0x20ab30, `alpha-player-control.md`) runs before the substeps. It runs
   for the local player, or for every creature when game byte +0xb4 is zero, and never for
   controller kind 6.
3. For each of the n substeps, and within it for axis k = **x, then y, then z**:
   1. P[k] += trunc(d[k]·65536).
   2. The box cells lo..hi (§0) are computed at the new position and scanned x-outer,
      y-middle, z-inner, stopping at the first cell that blocks. A cell **blocks** if it is
      solid. For a **swimmer** (appearance flag 0x0100), a cell also blocks if it is not
      water, so fish cannot leave the water.
   3. If a cell blocked:
      * on x or y, the axis response (§4.3) follows directly;
      * on z, the response (§4.2) also follows directly if the creature is already
        standing on a static object this step. Otherwise only the first static-object
        sector (sector x − 1, y − 1) is scanned before the response, so that an object
        under the creature can still count as support.
   4. If no cell blocked, the static zone objects of all nine sectors are tested, then the
      voxel objects (§4.5). An object hit leads to the response for this axis. Otherwise
      the next axis follows.

The test is discrete: only the end position of each axis move is examined. A substep of up
to one block can pass a corner that the end-position cell test misses.

### 4.2 Vertical response (axis z, either direction)

In this order:

1. **Landing sound**: if a local player exists (game +0xb8 ≠ 0) and v.z < −2, sound 32
   plays at the position, with pitch 1 + 0.2·rand()/32767 and volume 0.5. This applies to
   any creature.
2. **Fall damage**, local player only. It requires that the previous step's water flag is
   clear, that v.z < −5, and that h > 20 blocks. Here h = trunc(fall-start·65536) − P.z in
   fixed point, and the comparison is strict against trunc(20·65536).
   * The damage is computed with each stage converted to single precision and truncated
     back to fixed point: h/20, minus one block, × maxHealth, × 0.5. The result is
     converted to float. In blocks this is (h/20 − 1)·maxHealth/2.
   * health −= damage. stun := max(stun, value of RVA 0x44ff0), which is 1500 ms for a
     player. Mode 0x01 is cleared, sound 23 plays, health is clamped at 0, and a damage
     record with that amount is sent.
   * Falls of 20, 22, 30, 40 and 60 blocks deal 0 %, 5 %, 25 %, 50 % and 100 % of
     maxHealth.
3. The move is undone (P.z −= trunc(d.z·65536)). Then v.z := 0, p.z := 0 and
   fall-start := P.z. A ceiling hit also zeroes v.z.

### 4.3 Horizontal response and step-up

The response happens on a block on x or y when the creature is not a swimmer and no static
object has blocked earlier in this creature step. A static-object hit turns step-up off for
the rest of the step; a voxel-object hit does not.

1. **Step-up test.** The box at the moved position is raised by 1.01 blocks. lo and hi use
   the box conversion of P + trunc(1.01·65536) ∓ trunc((s/2)·65536), per axis. If any cell
   in this box is solid, or a voxel object within 64 blocks has a voxel there, the step
   fails. Static zone objects are not tested here.
2. **Step succeeds.**
   1. The voxel-object base offset recorded on this step's z hits (§4.5) is subtracted
      from P.z.
   2. The new P.z is (box-converted bottom block of P.z − sz/2)·65536
      + trunc((sz/2)·65536) + 66191. 66191 is 1.01·65536 computed in double precision and
      truncated toward zero.
   3. The landing bob (obj +0x1180) increases by the change in z, measured in blocks from
      the base-adjusted position.
   4. The base offset is added back.

   Velocity is **unchanged**.
3. **Step fails.** The move is undone on that axis only (P[k] −= trunc(d[k]·65536)).
   Velocity is **not** zeroed, so the creature keeps pressing into the wall until friction
   removes the speed.

Step-up does not require being on the ground. An airborne creature that touches a ledge
whose top is less than about 1.01 blocks above its feet climbs onto it.

The **landing bob** decays near the end of the step (RVA 0x21f081). It relaxes toward 0 at
0.02 when bob ≥ 0 and at 0.01 when bob < 0. The camera focus subtracts it (§7.3), which
smooths steps visually.

### 4.4 Creature separation (before the move; RVA 0x21aee0)

This considers every other creature that is alive (health > 0). If this creature is
controller kind 0, only others of kind 1 or 6 are considered. The other creature's box size
is s′, enlarged by (1, 1, 0) unless it is kind 6. The boxes overlap when, on every axis,
my max ≥ the other's min and my min < the other's max, compared in fixed point.

When they overlap:

* u is the horizontal vector from the other creature to me (z := 0). If |u|² is 0, the
  pair is skipped. u is normalised, and if my stun > 0, u ×= 0.1.
* d += u × (5·dt_s·other sx / my sx), where other sx is the other creature's *unenlarged*
  box size.

### 4.5 Objects

**Static zone objects** (RVA 0x21b72e) are tested in the 3 × 3 sectors around the creature.
The sector index is (plain-converted block x)/8 and likewise for y, truncating toward zero.

* An object is skipped when its kind (first dword) is 6, 7 or 9, or when it is in the
  creature's ignore set at obj +0x1468. Kinds 1, 2, 3, 5 and 8 count only while their byte
  +0x30 is non-zero.
* The box size is at +0x24, with x and y swapped when the rotation (+0x20) is odd. The
  position is the object's i64 position, which is the centre in x and y and the base in z.
* On the current axis the object is ignored unless the creature moves toward it:
  (P[k] − object P[k])·d[k] must be negative. A d[k] of zero never hits.
* Overlap is then tested on all three axes in fixed point: x and y as in §4.4, and z
  between the object's base and base + height.
* A hit sets the object-contact result (flag 0x40, §5.1) and turns step-up off for the
  rest of the step. A hit on z while v.z < 0 marks the creature as standing on an object
  (ground, §5.4).

**Voxel objects** are the entries of the list at game +0xc whose position (entry +0x80) is
within 64 blocks (|Δ|² ≤ 4096). Each is resolved through game +0x1c with identifier 0xa03.

* The box is tested voxel by voxel in the object's frame, rotated by minus its yaw (entry
  +0x98) and centred on half the model size.
* A voxel hit counts like a blocking cell, so step-up stays allowed.
* A hit on z marks ground support (§5.4) and records the base offset, which is the object
  z minus half the model height.

The meaning of the object kinds and of byte +0x30 is unresolved.

---

## 5. Contact flags, water and ground (RVA 0x21cca7..0x21e9e6)

### 5.1 Flags rebuilt after the move

The previous ground (0x01), water (0x02) and wall (0x04) flags and the previous wall
normal N are saved. Then flags := 0 and obj +0x145c += dt.

* **0x20 stuck**: stun ≤ 0, |v_xy|² > 16, |intent|² > 16, and the squared distance moved
  over the whole step is below 0.01 × |d_sub|², where d_sub is the per-substep d.
* **0x40 object**: a static object blocked during the move.
* **0x10 not fully submerged**: some cell of the box (lo..hi at the final position) is not
  water.
* **0x02 in water** (not for controller kind 6): some cell of the box is water. Let
  waterTop = max(lo.z, the largest z + 1 over the water cells).

### 5.2 Buoyancy

If in water, r = (waterTop − P.z) / (sz/2). The numerator is converted from fixed point as
in §0. r is the depth of the box centre below the water top, in half-heights.

* b = 0 if r ≤ 0; b = r + 1 if 0 < r < 1; b = 1 if r ≥ 1.
* For action 107, b = r + 1 for every r > 0.

Then **v.z += 30·dt_s·b**.

Deep water therefore cancels gravity exactly (b = 1). Near the surface the net lift is
30·r blocks/s², which floats the centre up toward the water line. At r = 0.5 the net
upward acceleration before drag is 30·1.5 − 30 = 15 blocks/s².

### 5.3 Water drag and swim pitch (RVA 0x21e684)

* **In water and not action 107:** v relaxes toward 0 at **0.005** on all three axes.
  Unless the creature is controller kind 6 or has appearance flag 0x0001, orientation.x
  relaxes toward **−60** at 0.01, pitching the body forward.
* **Splash:** when the previous step's water flag was clear and v.z < −3, a particle burst
  appears at the position: kind 15, colour (0.2, 0.7, 1, 1), field value 0.4, velocity
  (0, 0, 10). Sound 31 plays with pitch 0.9 + rand()/32767 and volume 1.
* **Otherwise** (not in water, or action 107): orientation.x relaxes toward 0 at 0.01.
* **Creature type 101:** orientation.x relaxes toward 60 during action 51 while its time
  is below 1000, and toward 0 otherwise, at 0.01. This happens after the ledge push.

### 5.4 Ground, step-down and wall probes

* **Ground** (skipped when in water). z_g is the box conversion of P.z − trunc((sz/2)·65536)
  − trunc(0.2·65536). If any (x, y) of the box columns lo..hi is solid at z_g, flag 0x01 is
  set. A ground cell of type 3 also marks the column, as water does. For controller kind 5
  that mark tops up obj +0x1198 of the creature named at obj +0x198 by 0.05·dt_s (while it
  is below 1), clamped to 1. This is not motion.
* **Step-down snap.** The conditions are:
  * the previous step's ground flag is set;
  * no static object blocked this step;
  * |d_sub,xy|² > 0;
  * neither the ground flag nor the water flag is set now;
  * d_sub.z ≤ 0.

  Then z_s is the box conversion of P.z − sz/2 − 1.2. If any column is solid at z_s, flag
  0x01 is set, P.z := z_s·65536 + trunc(1.01·65536) + trunc((sz/2)·65536), and the bob
  increases by the change in z. Walking down a one-block step therefore stays on the
  ground.
* Standing on a static object (§4.5) or a voxel-object z hit also sets flag 0x01.
* **Wall probes** (RVA 0x21d7ac). The probes write flag 0x04 and normal N (obj +0x11a0).
  N is first set to (0, 0, 0). The probes use world axes and the plain (truncating)
  block conversion.
  * The probe sits at s/2 + m from the centre on its axis, with m = 0.2 for controller
    kind 0. Other kinds use trunc(s/2 + 0.5) + 0.5 + 0.05.
  * **All four axis probes run**; none stops at a hit. Each scans the box's span on the
    other horizontal axis and the box's z range at one column:

    | Probe column | On a hit |
    |---|---|
    | y below the box (pos.y − probe offset) | flag 0x04, N.y := +1 |
    | y above the box (pos.y + probe offset) | flag 0x04, N.y := −1 |
    | x below the box (pos.x − probe offset) | flag 0x04, N.x := +1 |
    | x above the box (pos.x + probe offset) | flag 0x04, N.x := −1 |

    A later hit overwrites the same component. N can therefore be diagonal, such as
    (+1, −1, 0), and it is not normalised.
  * Only if none of the four hit are the corner columns tested. There are four, at
    (x + cx·offset, y + cy·offset) with c = (cx, cy) ∈ {±1}², taken in the order (−,−),
    (+,−), (−,+), (+,+). Each spans the box's z range.
    * A corner is tested only when dot(facing(ψc), 0.7·(cx, cy)) ≥ −0.1, that is, when the
      creature is not facing away from it.
    * The first corner hit sets flag 0x04 and N = (−cx, −cy, 0), and stops the corner
      tests.
  * N points away from the wall.

---

## 6. Friction and push decay

**R = 0.005 per ms, or 0.0025 per ms while |intent|² > 0** (selected at RVA 0x21d19e).
This routine has no surface-dependent friction.

In this order, after the ground tests:

1. **Boat** (action 107 in water): v.x and v.y relax toward 0 at 0.1·R, and v.z relaxes at
   R.
2. **Ground** (flag 0x01): v relaxes toward (0, 0, 0) at R. This is skipped during action
   48, during action 54 while time ≤ P1 + X, and during actions 6, 7, 10, 17, 18, 19 and 20
   while time < P1. Attack wind-ups keep their momentum.
3. **Push p** relaxes toward 0 at R if the ground or water flag is set, if mode 0x10 is
   set, or if the list at obj +0x1460 is non-empty. A thrown, airborne creature keeps its
   push while that list is empty.
4. **Air**: there is no horizontal drag in the air.
5. **Water drag**: §5.3, applied after the wall probes.

Worked values at a fixed dt of 16: (1 − 0.005)^16 = 0.92293; (1 − 0.0025)^16 = 0.96074;
(1 − 0.0005)^16 = 0.99203.

* **Walking (S = 6).** The move uses 6 blocks/s, which is 0.096 blocks per step. The
  velocity stored after friction is 5.764.
* **Releasing the key**, with intent zero on the release step. Step 1 moves 0.0922 and
  stores 5.764·0.92293 = 5.320. Step 2 moves 0.0851, and so on. The total coast is about
  1.20 blocks.
* **Free fall from rest** (v updated first, then the move). After k steps v.z = −0.48k and
  the drop is 0.00768·k(k + 1)/2. After 1 s (62.5 steps) that is v ≈ −30 and a drop of
  about 15.2 blocks.

---

## 7. Camera

### 7.1 State (game object)

| Offset | Meaning | Initial |
|---|---|---|
| +0x18c | cameraSpeed (option, i32) | 50 (built-in default) |
| +0x190 | cameraSmoothness (option, i32) | 80 (built-in default) |
| +0x194 | invertY (option, i32) | 0 |
| +0x19c | minTimeStep (option, i32; §1) | 9 (built-in default) |
| +0x1a4, +0x1a8, +0x1ac | **current** pitch θ, roll, yaw ψ (degrees) | 120, 0, 0 |
| +0x1b0, +0x1b4, +0x1b8 | **target** pitch, roll, yaw | copied from current |
| +0x1bc, +0x1c0 | current and target follow **distance** (blocks) | 0, 0 (first set to 16, then overwritten with 0 later in the constructor RVA 0x59c40) |
| +0x1c4, +0x1c8 | current and target map zoom | 1, 1 |
| +0x158 | **focus point** (3 × i64) | — |
| +0x140 | camera position (3 × i64) | — |
| +0x1cc | distance actually used last frame | — |
| +0x1e8 | shake amplitude | 0 |
| +0x1ec / +0x22c / +0x26c | camera-to-world / world-to-view / shake-view matrices | — |

**Option block.** The block at RVA 0x36b1d8 holds twelve i32 values in this order:
fullscreen, resolutionX, resolutionY, antiAliasing, renderDistance, soundVolume,
musicVolume, cameraSpeed, cameraSmoothness, invertY, language and minTimeStep.

* Its built-in defaults (static initialiser RVA 0xce660) are 0, 0, 0, 0, 50, 100, 100,
  50, 80, 0, 0 and 9.
* At start-up the resolution entries are set from the screen metrics. options.cfg is then
  read into the block (reader RVA 0xce6e0, keyed by name); the writer RVA 0xcef80 saves
  options.cfg.
* The game object holds a copy at +0x170..+0x19c, and the frame (RVA 0xc85f0) copies any
  change back into the block.

The mouse-look and zoom routines never change roll.

### 7.2 Mouse look (vtable +0x18, RVA 0x7ea00)

Look mode applies when the vtable +4 method (RVA 0x7f1d0) returns false, which happens when
no menu window, the map or another listed screen is open. In look mode the main loop
re-centres the cursor each frame while the window has focus and passes the DirectInput
relative counts (Δx, Δy). With s = cameraSpeed:

* the pitch target θ* += s·0.005·Δy (or −= when invertY ≠ 0), then is clamped to [0, 180];
* the yaw target ψ* −= s·0.005·Δx, with no wrap.

The sensitivity is therefore 0.005·cameraSpeed degrees per count, which is 0.25° at the
default 50. Moving the mouse down looks further down (θ grows), and moving it right turns
right.

**Cursor mode** (a window is open). The method gets the cursor position, and the same
formulas use the cursor's movement since the previous frame. They apply only while
mouse-button byte +0x4 is set and the cursor is not over a widget (RVA 0x250ae0 returns 0).
A separate drag path pans the view state of the screen at game +0x8008dc by 0.5 × the
cursor movement while byte +0xa is set and that screen is open. Two earlier branches
route the movement to the widgets of the screens at game +0x800888 and +0x80088c instead
and return.

### 7.3 Smoothing, zoom and follow (once per frame)

Items 1 and 2 run at RVA 0x965a2 and items 5–11 at RVA 0x9c2e6. They use the frame dt after
the slow-motion division (§1).

1. **Angle smoothing.** k = 0.005^(cameraSmoothness/100), evaluated by the double-precision
   library pow and rounded to single. The current angles blend toward the targets
   component by component, Blend((θ, roll, ψ) → (θ*, roll*, ψ*), k), as plain lerps with
   no angle wrapping. Example values of k:
   * smoothness 0: k = 1 (instant);
   * 50: k = 0.070711;
   * 80 (default): k = 0.014427;
   * 100: k = 0.005.
2. **Distance and map zoom.** Blend(dist → dist*, 0.004) and Blend(+0x1c4 → +0x1c8, 0.004).
3. **Wheel zoom** (vtable +0x24, RVA 0x7ef40). The window procedure (RVA 0xc83f0) passes
   n = the sign of the WM_MOUSEWHEEL delta (+1, −1, or 0).
   * Outside the map (game byte +0x8006e4 clear): dist* −= 2n, clamped to [**0, 14**], so
     wheel up zooms in.
   * On the map: the map zoom is ×0.9 when n < 0 (wheel down) and ÷0.9 otherwise (wheel
     up), clamped to [0.01, 10].
4. **Character-screen orbit** (game update RVAs 0x89d0c, 0x89d7d and 0x8a842). Three
   separate blocks each set dist* := 10 and θ* := 100 and add ψ* += 0.005·dt, using the dt
   received before slow motion:
   * when the screen at game +0x800874 or +0x800894 is visible;
   * when +0x800888 is visible;
   * when +0x80088c is visible.

   Each applies only while +0x800898 is not visible. When several qualifying screens are
   visible, the yaw increment is applied once per block.
5. **Focus.**
   1. If |focus − player position|² > 2500 (more than 50 blocks), focus := player position.
      This is tested first.
   2. If any of the screens +0x800874, +0x800888, +0x800894 or +0x80088c is visible, the
      focus is instead set from the point that RVA 0x87fe0 returns for the world object
      (game +0x2e4, offset +0x8000f0), converted to fixed point (RVA 0x9c6c0). If the
      column at that x, y is loaded, its z is set from the column record (the sum of RVAs
      0x348b0 and 0x34b70, in blocks). The focus is then raised by 50 blocks. The meaning
      of this point is unresolved.
   3. Otherwise:
      * w_xy = 1 − (1 − 0.025)^dt (Blend weight). focus.x and focus.y each move by
        (trunc(w_xy·65536) × (player − focus)) / 65536, in 64-bit integer arithmetic
        truncating toward zero (RVA 0x68ca0).
      * Unless the screen +0x800880 or +0x800890 is visible, the z target depends on the
        action:
        * sitting or sleeping (action 83 or 84): P.z − bob + sx/2;
        * riding or in a boat (action 106 or 107): P.z − bob + sz/2 + **1.5**;
        * otherwise: P.z − bob + sz/2 + **0.5**, half a block above the head.

        With +0x800880 or +0x800890 visible, the target is P.z − bob.
      * focus.z blends toward the target at **0.005** (RVA 0x573d0), with w_z the Blend
        weight. focus.z := trunc(float(focus.z)·(1 − w_z)) + trunc(float(target)·w_z),
        with each term formed from the single-precision conversion of the 64-bit value.
6. **Direction.** M is the identity rotated by yaw (RVA 0x24610), then roll (RVA 0x244f0),
   then pitch (RVA 0x243d0); each rotation acts on two rows. The third row of M is the view
   direction d = (sin ψ sin θ, −cos ψ sin θ, cos θ).
   * θ = 90 looks horizontally along f (§3.9);
   * θ = 120 looks 30° down;
   * θ = 0 looks straight up;
   * θ = 180 looks straight down.
7. **Desired position.** P = focus − dist·d. The camera sits behind the focus and looks
   along +d toward it.
8. **Block collision.**
   * For each of the 8 corners c of a unit cube centred on P (offsets ±0.5 per axis):
     if the cell at P + c is solid, a march (RVA 0x1a35d0) runs from P + c along d with a
     maximum length of dist. It continues while the cells are solid, stops at the first
     non-solid cell, gives up after about 500 cells, and includes the object test. It
     returns the length h travelled. The candidate is dist − h.
   * The minimum over all corners is kept, starting from dist. Then **current dist :=
     that minimum**. It is written into +0x1bc, so item 2 eases it back out at 0.004 on
     later frames.
   * Then P := focus − dist·d and +0x1cc := dist.
9. **Fallback.** If the cell at P is still solid and P differs from the player's position,
   u is the unit vector from the player's position toward P. The same march runs from the
   player's position along u, with a maximum of 100 and the same mode: it continues while
   cells are solid. If the returned length t ≥ 0, P := player position + u·t. With the
   player's own cell free, the march ends at once, subject to its object test (unresolved).
10. **Matrices.** +0x1ec is the identity translated by P (in float blocks), then rotated by
    Rz(ψ), Ry(roll) and Rx(θ). +0x22c is the identity rotated by Rx(−θ), Ry(−roll) and
    Rz(−ψ), then translated by −P.
11. **Shake.** The matrix +0x26c is the identity translated, in its own x and y axes, by
    0.5·a·cos(0.04·T) and 0.5·a·cos(0.03·T). Here a = +0x1e8 and T is the world clock in
    ms (world +0x8000bc, read through game +0x2e4). The view rotations and the camera
    translation follow. Afterwards the shake decays, Blend(a → 0, 0.005).

There is no separate first-person mode in these routines. With dist* = 0 the camera sits
at the focus, half a block above the head, and the same collision applies.

### 7.4 Projection (render routine RVA 0xac260; call at RVA 0xae5d3 to RVA 0x27910)

The perspective is built directly.

* The vertical field of view is **50°** (0.87266475 rad), or **30°** (0.5235988 rad) while
  the local player has mode flag 0x400. The switch is instant, with no smoothing.
* aspect = −(game +0x11c / game +0x120), both i32 converted to float. near = **0.1** and
  far = **2000**.
* The matrix has m00 = (1/tan(fov/2))/aspect, m11 = 1/tan(fov/2), m22 = far/(far − near),
  m23 = 1, and m32 = −near·far/(far − near). The tangent is evaluated in double precision.
* While the map is open (game byte +0x8006e4), the render routine passes 90 instead of θ to
  RVA 0x48090 on the object at game +0x800580 (call at RVA 0xada3d).

### 7.5 Camera worked values (fixed dt of 16)

* Angle smoothing at k = 0.005: w = 1 − 0.995^16 = 0.07707. A 90° yaw flick has
  90·(0.92293)^n remaining after n frames, which falls below half after 9 frames.
* At k = 0.070711 (smoothness 50), w = 0.6907. At k = 0.014427 (default 80), w = 0.2075.
* Focus xy: w = 1 − 0.975^16 = 0.3331. Focus z: w = 0.07707. Distance: w = 1 − 0.996^16
  = 0.06212.
* Mouse: Δx = +40 counts at speed 50 gives ψ* −= 10°.
* With θ = 120, ψ = 0 and dist = 10: d = (0, −0.8660, −0.5), so P = focus + (0, 8.660, 5).

---

## 8. Native facts relevant to `alpha-player-control.md`

1. minTimeStep is a minimum frame time in the main loop (§1). The world step runs in chunks
   of 20 ms or less.
2. The movement body clears intent only in the attachment branches (§2 item 17). The writes
   at RVAs 0x20d842 and 0x20d89f, earlier in the routine, zero two dwords at +0x40/+0x44 of
   a different, non-creature record handled in that loop.
3. **Health regeneration and clocks** (RVA 0x20c580..0x20c692). For each creature of
   controller kind 0 that has action 84 (resting) and health > 0:
   * if it is the local player, health += maxHealth·(dt·0.001·0.05), clamped to maxHealth;
   * the presence of any such creature makes the time-of-day counter (world +0x80015c)
     advance by 100 × dt instead of 10 × dt.

   The millisecond world clock (world +0x8000bc) always advances by dt.
4. The wall probes use world axes, run all four axis probes, and choose diagonal probes by
   facing (§5.4).
5. R is 0.005 per ms, or 0.0025 per ms while intent is non-zero (§6).

---

## 9. Unresolved

* **Intent reset.** The writer that resets the local player's intent between frames,
  outside this routine, is not identified.
* **Creature yaw.** facing(ψc) = (−sin ψc, cos ψc) is the opposite of the camera forward
  f(ψ). Whether input or rendering relates ψc to ψ by 180°, or derives ψc from velocity,
  is not established here.
* **Jump apex.** Jump-input magnitudes are outside this routine, so no apex figure is
  stated.
* **Push list.** The identity of the list at obj +0x1460 is unknown.
* **Object tables.** The meaning of the static-object kinds 1, 2, 3, 5, 6, 7, 8 and 9,
  their byte +0x30, the ignore set at obj +0x1468, and the voxel-object identifier 0xa03
  are unknown.
* **Wind noise.** The formula of the noise routine at RVA 0xc0ef0 is not established.
* **Setters.** What sets game byte +0x84 bit 0 (S = 20), game byte +0x8006e5 (quarter
  speed), mode flag 0x400 (30° field of view) and the shake amplitude +0x1e8 is unknown.
* **First person.** Whether the player model is hidden at distance 0 is not established.
* **Network flag.** Game byte +0xb4 gates the lunge and unstick for non-local creatures.
  Reading it as "network client" is an interpretation, not a confirmed fact.
* **Dodge-cancel bytes.** The meaning of bytes obj +0x140/+0x141 in the dodge-cancel
  exception, and of the query RVA 0x43f10 (argument 12 read as a buff), is unknown.
* **Focus override.** The meaning of the screen-override focus point (world +0x8000f0) and
  of the column terms used for its z is unknown.
* **Fallback march.** The exact result of the fallback march when the player's cell is free,
  given the march's object test, is not traced.
* **Projection inputs.** Whether game +0x11c/+0x120 are the window, viewport or option
  resolution is not established. The role of RVA 0x48090 (assumed sky or ambient) is not
  established.
* **Smaller items.** Not established: the item behind the action-81 length (RVA 0xc6b80
  on obj +0x1e8), the 0.4 splash field, and the type-3 ground mark.

---

## Re-certification (2026-11-15)

Re-checked against Cube.exe and Server.exe image bytes, the Ghidra listing and the
decompilation of the original images. The Infinite-Cubes reconstruction was not used.

**Confirmed (unchanged in substance).**

* Step order; hit-stun; action clock.
* Acceleration and the swimmer clamp.
* Bank angle; S base, fast types, kind 5, sail and swim factors, debug, glider, sneak,
  sprint (double precision), haste, kind 4, channelled 0.1, slow/haste, aiming-backward
  rule.
* Lunge contact ranges and the 2S facing push.
* Dash windows and speeds; spin 720°/s.
* Climbing relax 0.0025; flyer and glider gravity 3; gravity 30 with no terminal velocity;
  fall-start rules.
* Stamina rates and the 2.2 s figure.
* Glider crash conditions, crash stun (1500, or 3000 − 375·tier), lift 30; the ledge shove
  −5·N.
* The camera-relative f and r vectors and the facing vector.
* Substep count trunc(|d| + 1); x, y, z axis order; the swimmer cell rule.
* Landing sound; fall damage (h > 20, formula and percentages); vertical undo and zeroing.
* The step-up height 1.01 and its unchanged velocity; the step-fail undo without zeroing.
* Bob decay rates.
* Separation eligibility, enlargement and the 0.1 stun factor.
* Flag rebuild and flag meanings; waterTop; buoyancy b and the 15 blocks/s² example.
* Water drag 0.005, swim pitch −60, splash parameters, type-101 pitch.
* Ground probe 0.2 and step-down probe 1.2; R values; boat friction; the ground-friction
  exemptions; push decay.
* The free-fall and walking values.
* The 20 ms world-step split and its examples; slow motion ÷4 with the 5 ms sleep; the
  main-loop limiter.
* Camera field offsets and initial values (except the defaults below); the option block
  order.
* Mouse-look formulas, clamp and sign; Blend semantics; k values at 0, 50 and 100;
  distance and map-zoom blends at 0.004; wheel n = sign and the [0, 14] clamp.
* Focus 50-block snap, xy rate 0.025, z rate 0.005 and z targets.
* View direction, P = focus − dist·d, 8-corner collision and the write-back to +0x1bc.
* Matrices; shake terms and decay.
* Projection field of view (50° and 30°), near, far and matrix entries.
* The server twin: identical apart from one stack adjustment and import slots.
* Health regeneration being local-only during action 84.

**Corrected.** Old reading on the left, native behaviour on the right. "Reversal" marks a
reversed claim. "Feel" marks an effect on movement feel, and "camera" an effect on the
camera.

1. Addresses were virtual addresses → RVAs with the image named (format only).
2. Block index described as floor → truncation, minus one when negative; differs at exact
   negative multiples of 65536 (feel: collision at negative coordinates).
3. Wall-probe columns and object sectors described as floor → plain truncation toward zero
   (feel).
4. Wall probes "stop at the first hit" → all four axis probes run, later ones overwrite N
   components, and N can be diagonal (**reversal**; feel: climbing, ledge, glider crash).
5. Charge S = 40 for action 54 "while time ≥ P1 + X" → while time < P1 + X
   (**reversal**; feel).
6. Separation scale "s′x/sx" with enlarged s′ → the other creature's unenlarged sx (feel).
7. Separation overlap "|Δ| < s′/2 + s/2" → my max ≥ their min and my min < their max;
   a zero-length u skips the pair.
8. Substep "d := d/n" → d multiplied by the single-precision 1/n (precision).
9. Object tests lacked the move-toward-centre gate, the ignore set at obj +0x1468, the
   base-referenced z extent and truncating sectors → added (feel near objects).
10. "If no block blocked, test objects" → x/y blocks respond directly; z blocks scan one
    sector first unless already standing on an object; voxel hits act as cells
    (step-up allowed), set ground on z, and their base offset is removed before the
    step-up floor.
11. Ground slam → added the time < duration condition.
12. Dodge sound "for the local player" → any creature, when a local player exists; the
    end-of-step order corrected to equipment checks, bob decay, then dodge.
13. "The step never clears intent" → it zeroes intent and v in the attachment branches
    (**reversal**; feel).
14. Map zoom "×0.9 per step up, ÷0.9 per step down" → ÷0.9 on wheel up, ×0.9 on wheel down
    (**reversal**; camera).
15. Defaults cameraSmoothness 100 and minTimeStep 16 (62.5 fps) → 80 and 9 (about
    111 fps) (camera and timing feel).
16. k "single-precision pow" → double-precision pow, rounded (camera precision).
17. Focus xy "trunc(Δ·w)" → (trunc(w·65536)·Δ)/65536 in integers; focus z truncated per
    term (camera precision).
18. Focus "skipped while map or screens open" → the 50-block snap always runs first; the
    four screens substitute a world point raised 50 blocks; the map is not part of this
    test (camera).
19. Orbit only for +0x800874/+0x800894 → also +0x800888 and +0x80088c, as separate blocks,
    using the dt before slow motion (camera).
20. Fallback ray "stops at first solid cell" → the same continue-while-solid march as the
    corner rays; the cell limit is about 500 (**reversal**; camera).
21. Cursor-mode "drag moves the map when +0x8006e4" → a separate byte +0xa drag pans the
    screen at +0x8008dc at half rate (camera and UI).
22. Projection aspect from "resX/resY" → game +0x11c/+0x120; the "sky term" claim is
    softened to the call fact.
23. Wind "two sine-noise terms, depends on position and v" → one noise call per component,
    from horizontal position only, with stated scales and offsets (feel: gliding).
24. Regeneration / "world clock 100× dt" → the time-of-day counter +0x80015c, triggered by
    any resting kind-0 creature; the ms clock +0x8000bc always advances by dt.
25. Coast value "frame 1 stores 5.538, total 1.15" → 5.320 and about 1.20 blocks (feel
    reference).
26. Distance weight 0.06208 → 0.06212.
27. Missing native rules added: boat on land ends action 107; riding drain and climb
    dismount; mount health ≥ 0; action 81 S = 0, then action 83; the dodge-cancel rule;
    the sneak non-player skill fallback; the glider and lantern equipment checks; the
    type-3 mark (feel).
28. RVAs 0x20d842/0x20d89f "projectile loop" → an unidentified non-creature record.
29. "Cursor hidden" → only re-centring is confirmed.
30. Record field numbers and "quadruped" labels → raw offsets and flags.

**Removed.**

* The implementer instruction to implement the discrete test and avoid a continuous
  sweep.
* "Verify this with Q-4" and the unsupported jump-apex worked value.
* All test procedures in the former open-questions list (hooks, logging, recordings,
  breakpoints, key trials); the native questions are kept in §9.
* The former "resolves F-x / C-x" bookkeeping and the claims about what the companion
  document says.
* The open question on dodge speed, which the code settles (§3.2).

**Unresolved.** As listed in §9.
