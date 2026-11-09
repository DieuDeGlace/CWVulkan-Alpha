# Alpha player control: input, movement, jumping, dodging and the step

Behavioural specification of how the Cube World **Alpha** client (`Cube.exe`, SHA-256
`84a7a132…30bf`) turns the local player's input into creature-record changes, and of the
parts of the shared creature simulation step that were traced. Written from the Alpha
decompile and assembly; the simulation step itself (client 0x60c510, server 0x5322d0) has
no decompiled form and was read as assembly in the ranges cited.

**Coverage.** Input → intent, jumping, swimming up, dodging, climbing input, target lock,
health regeneration, contact flags, block solidity and the "unstick" rule are specified
exactly. The velocity integrator (intent → velocity, gravity, air/water drag, the swept
collision of the box against blocks) and the camera are **not yet** specified; §8 lists
exactly where they are and what is known. An implementer can build the input side and the
network fields now and must treat §8 as the next specification step.

Related: `alpha-net-packets.md` §2.1 (field numbers used below), `alpha-creatures-models.md`
(§3 collision box, §7 animation state), `alpha-creatures-behaviour.md` (actions, combat),
`alpha-saves.md` §7 (`options.cfg`).

---

## 0. Conventions

* Addresses are `Cube.exe` (base 0x400000). "Field *n*" = bit *n* of the creature record
  (`alpha-net-packets.md` §2.1); record offsets are given with it. Object offsets ("obj
  +0x1194") are client-only state (object offset = record offset + 0x10).
* Time: the step is driven by an integer **dt in milliseconds**; dt_s = dt × 0.001
  (single precision). Velocities are blocks per second.
* "Relax *x* toward *t* at rate *r*" = repeat dt times *x* ← *x* + (*t* − *x*)·*r*
  (0x411d40 for scalars, 0x605950 for vectors).
* World axes: +z up. Positions are 64-bit integers in 1/65536 block.

---

## 1. Creature-record fields involved (adds to P-11)

| Field (record offset) | Name used here | Written by |
|---|---|---|
| 0 (0x000) | position: **centre of the collision box** | physics |
| 1 (0x018) | orientation (x, y, z degrees; z = yaw) | physics, riding, climbing |
| 2 (0x024) | velocity | physics, jump, dodge, hits |
| 3 (0x030) | **movement intent** (accumulated per frame, §3) | input (players), steering (NPCs) |
| 4 (0x03c) | *unknown* push vector; relaxed toward 0 by the step, zeroed by NPC steering | step |
| 5 (0x048) | **head pitch** (degrees): relaxed toward 45 while climbing, otherwise decays to 0 | step, input |
| 6 (0x04c) | **contact flags**: bit 0 on ground, bit 1 in water, bit 2 touching a wall | step (collision) |
| 9 (0x058) | action id | combat, input |
| 10 (0x05c) | action time (ms) | step |
| 14 (0x114) | **mode flags**: 0x01 climbing allowed, 0x04 aiming (cleared each frame), 0x10 gliding, 0x40 running, 0x200 lantern lit, 0x400 riding/sitting pose | input, step |
| 15 (0x118) | **dodge time** left (ms) | dodge |
| 16 (0x11c) | hit-stun counter (see behaviour spec) | hits |
| 27 (0x15c) | health | regeneration, hits |
| obj +0x1194 | **stamina** 0..1 (not on the network) | dodge (−0.25), regeneration (*follow-up*) |
| obj +0x11a0 | wall normal (3 × f32), ±1 along the creature's local x or y | collision |
| obj +0x11c0 | mount id (u64) | riding |

The client owns its player completely (`alpha-net-packets.md` §2.3); everything here runs
on the client and the result is sent with packet 0.

---

## 2. Frame order (client)

Per rendered frame with elapsed dt:

1. **Input** (0x4a6b50, §3–§6) for the local player.
2. **World step** (0x60c510) over all creatures, projectiles and items with the same dt:
   health regeneration (§7), per-creature physics and collision (§8), actions and combat
   (`alpha-creatures-behaviour.md`), animation state (`alpha-creatures-models.md` §7.3).
3. Network send of the changed fields (`alpha-net-transport.md`).
4. Rendering.

`options.cfg` has `minTimeStep 16`; whether this clamps dt from below, caps it, or splits
long frames into steps is open question C-1.

---

## 3. Movement intent (0x4a6b50)

Inputs: four direction keys, an analogue stick (two signed values *ax*, *ay* scaled by
1/32768), the camera yaw *ψ* (degrees, game object) and the full camera orientation.

**Acceleration magnitude** *A* = 80, or 8 while gliding (0x444650: mode 0x10, falling
(velocity z < 0), not on ground, field 16 < 1, not dodging).

Let **f** and **r** be the camera's forward and right directions in the horizontal plane
(from *ψ*: the camera basis rotated by *ψ* about z) and, for swimming, **F**, **R** the
camera's full 3-D forward and right axes.

Each frame, for every held key, in the order forward, back, left, right, then the stick,
exactly one of these rules applies, tested in this order:

1. **Climbing** (mode 0x01 and wall contact): forward → intent.z += 0.2·*A*; back →
   intent.z −= 0.2·*A*; left/right → intent −= / += 0.2·*A*·(wall normal rotated 90° about
   z); stick → the same with *ax*, and intent.z += *ay*·0.2·*A*/32768.
2. **Dodge** (dodge key held, stamina ≥ 0.25, and the game's input focus is free): only if
   (dodge time < 1 or action ≠ 0) and (on ground or in water): velocity := 40·d + (0, 0, 5)
   where d is the key's direction (±**f**, ±**r**, stick direction), dodge time := 600,
   stamina −= 0.25, action := 0. Otherwise nothing.
3. **Swimming** (in water, action ≠ 107): intent += *A*·(±**F** or ±**R**; stick:
   −*ax*·**R** + *ay*·**F** … in camera space).
4. **No control** while gliding without stamina: mode 0x10, not on ground, stamina ≤ 0
   → the key does nothing.
5. **Walking**: intent += *A*·(±**f**, ±**r**); the stick adds *A*·(−*ax*·**r** − *ay*·**f**)
   /32768 (sign conventions per open question C-2).

So one key gives |intent| = 80 per frame before the step consumes it; diagonal keys give
80√2. The step clears intent x and y after use (0x60d842, 0x60d89f; §8).

Also every frame: mode flag 0x04 is cleared; if the action is 52 it is set to 0; if
intent is non-zero, open inventory-type windows are closed.

---

## 4. Jumping and swimming up (0x4a6b50, end)

Two persistent values: *held* (ms the jump key has been held) and *lock* (ms, a short
window after take-off).

Each frame:

1. If the jump key is up: *held* := 0.
2. If it is down: *held* += dt. If *held* ≥ 1:
   * **take-off**: if *held* < 200, (on ground or dodging) and velocity.z ≤ 0:
     velocity.z := min(10, |velocity_xy| + 2); *lock* := 100.
   * **swim up**: if in water and velocity.z < 10: *lock* := 100; velocity.z += 0.001·dt.
3. **Boost**: if *lock* > 0 (or step 2's swim-up ran) and the key is down and
   velocity.z < 10: velocity.z += 0.08·dt; if that reaches 10 or more, velocity.z := 10.
4. *lock* := max(0, *lock* − dt).

Consequences: a tap gives a jump of initial speed |v_xy| + 2 (2 when standing); holding
the key adds up to 0.08 × 100 = 8 blocks/s more during the first 100 ms, capped at 10. In
water, holding jump rises at about 0.081·dt per frame up to 10 blocks/s.

Worked example: standing still, key held for 3 frames of 16 ms: frame 1: *held* = 16,
take-off v.z = 2, *lock* = 100, boost +1.28 → 3.28, *lock* 84; frame 2: +1.28 → 4.56
(gravity of the step not included, §8); frame 3: 5.84; … until 10 or *lock* = 0.

---

## 5. Target lock (0x4a6b50)

The game object keeps one locked target. Each frame it is cleared if the target's health
≤ 0 or its squared distance from the player exceeds 10,000 (100 blocks). Pressing the
target key cycles: among creatures that are hostile (controller kind 1, or mode flag
0x20) and alive, that project inside the screen (−1..1 in both axes after the camera and
projection matrices), and that pass a line-of-sight test (0x59ee90, range 200), the
nearest one farther than the current target becomes the target (wrapping to the nearest
overall). The target is used by combat aiming (`alpha-creatures-behaviour.md`).

---

## 6. Stamina and dodging

* A dodge costs 0.25 stamina and sets dodge time 600 ms; while it is > 0 the renderer
  rolls the body one full turn (`alpha-creatures-models.md` §7.4) and footsteps are
  silent. The step decrements the dodge time by dt (*inferred*; follow-up F-3).
* Stamina also gates gliding control (§3 rule 4).
* The renderer's walk swing uses 1.25× while running (mode 0x40 with intent and speed),
  which therefore exists as a mode; what sets mode 0x40 is follow-up F-4.

---

## 7. Health regeneration (0x60c59f)

For the local player only (the step's "own creature"), each step while health > 0:
health += maxHealth × dt × 0.001 × 0.05, then health := min(health, maxHealth). That is
5 % of maximum health per second. maxHealth is `alpha-creatures-behaviour.md` §4.1.

---

## 8. Physics step: what is known

The per-creature physics lives inside the world step, assembly only. Known pieces:

### 8.1 Collision box and blocks

* The creature's position is the **centre** of its collision box (appearance 0x08, size
  sx, sy, sz in blocks); the box spans position ± size/2 (0x60ab30).
* A block is **solid** when its type (low 5 bits) is neither 0 (air) nor 2 (water). Out of
  a column's stored range fixed default blocks are used: above the top, water if z ≤ 0 and
  air above (`alpha-worldgen-overview.md` §2, sea level 0); below the bottom and outside
  loaded zones a third default block (*inferred* solid; check its type byte at 0x76de37).
* **Unstick** (0x60ab30): if any solid block overlaps the box, the creature is lifted so
  that the box bottom rests on the top of the block layer it is in (z := ⌊bottom⌋ + 1 +
  sz/2 in fixed point, *as read*; confirm the +1), the landing bob is cleared, and the
  test repeats once. Then the
  same is done against static objects of the zone.

### 8.2 Ground and wall contact (0x61d4c7..0x61dbab)

* Contact bit 0 (on ground) is set when a solid block (0x43b480: type ≠ 0 and ≠ 2) is
  found in the cells just under the box bottom; at the same time the landing bob (obj
  +0x1180) is updated from the height difference (exact rule follow-up F-1).
* Wall contact (bit 2): probe cells just outside the box sides in the creature's yaw
  frame — at half width + 0.2 for players (controller 0), and at ⌊half width + 0.5⌋ with
  a 0.05 margin for NPCs (*as read*, constants 0.2, 0.5, 0.05); a solid probe sets bit 2 and the wall normal (obj
  +0x11a0) to (0, +1), (0, −1), (+1, 0) or (−1, 0) in the creature's frame.

### 8.3 Friction and pushes

* **On ground**, except during an attack wind-up (action 48; action 54 before the end of
  its second phase; actions 6, 7, 10, 17–20 before their first phase): velocity relaxes
  toward (0, 0, 0) at a per-step rate *R* (0x61d6fe). *R* comes from the surface or state
  (open question C-3).
* **Boat** (action 107 in water): velocity x and y relax toward 0 at 0.1·*R*, z at *R*.
* Field 4 relaxes toward 0 at *R* unless (not on ground, not in water, gliding).

### 8.4 Not yet specified (follow-up F-1)

* intent → velocity (the assumed form "velocity relaxes toward intent·*c*" is **not**
  verified), gravity, terminal speed, water buoyancy and drag, glide lift;
* the swept move of the box through blocks (axis order, step-up of 1 block, ledge
  behaviour);
* orientation (yaw) turning toward the movement or the camera.

They are inside 0x60c510 between 0x60d3b0 and 0x61e9e6; the decompiled NPC steering
(0x5a7eb0, `alpha-creatures-behaviour.md` §2.3) shows the same style (relax toward a
target velocity at a per-ms rate) and is the best guide to read them.

---

## 9. Camera (follow-up F-2)

Known: options `cameraSpeed` (default 50), `cameraSmoothness` (100) and `invertY` (0) are
parsed by 0x4ce6e0 and written by 0x4cef80 (`alpha-saves.md` §7). The camera yaw used by
§3 is a degree value in the game object (game object +0x1ac). Follow
distance, pitch limits, zoom, collision and smoothing are not yet traced; start from the
readers of the three option values.

---

## 10. Open questions and follow-ups

* **C-1** Meaning of `minTimeStep` for dt. Log dt per frame with a hook on 0x60c510.
* **C-2** Exact sign/axis convention of **f**, **r** and the stick. Walk in each direction
  and log intent.
* **C-3** The friction rate *R* (its sources at 0x60cf0a, 0x60f9b4, 0x619217, 0x61afe2,
  0x61b0e4, 0x61b1f1, 0x61b607, 0x61b73d, 0x61bf57, 0x61c0f3).
* **F-1** Velocity integrator, gravity and swept collision (§8.4).
* **F-2** Camera (§9).
* **F-3** Dodge-time and stamina regeneration in the step.
* **F-4** What sets mode 0x40 (running), 0x01 (climbing allowed), 0x10 (gliding: requires
  the glider in equipment slot 11 — the step clears 0x10 when slot 11 is not item type 23
  subtype 0, 0x61f008) and 0x200 (cleared when slot 10 is not a lantern, type 24).

Verification: with the user's mod DLLs, log the local player's record fields 2, 3, 6, 14,
15 and obj +0x1194 each frame while walking, jumping (tap and hold), dodging, swimming
and climbing; every rule above predicts those logs exactly.
