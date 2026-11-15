Status: FINAL / STABLE / HELD

# Alpha player control: input, movement, jumping, dodging and the step

Behavioural description of how the Cube World **Alpha** client (`Cube.exe`, SHA-256
`84a7a132…30bf`) turns the local player's input into creature-record changes, and of the
parts of the shared world step that were traced. Source: the Alpha client disassembly and
image bytes. The world step (client 0x60c510; server counterpart 0x5322d0, not re-read here)
was read as assembly in the ranges cited.

**Coverage.** Input to intent, jumping, swimming up, dodging, climbing input, target lock,
health regeneration, contact flags, block solidity and the unstick rule are described below.
The velocity integrator (intent to velocity, gravity, air and water drag, the swept box
collision) and the camera are not described; §8.4 and §9 state what is known about them.

Related contracts (pointers only): `alpha-net-packets.md` §2.1 (field numbers),
`alpha-creatures-models.md` (§3 collision box, §7 animation state),
`alpha-creatures-behaviour.md` (actions, combat), `alpha-saves.md` §7 (`options.cfg`).

---

## 0. Conventions

* Addresses are `Cube.exe` virtual addresses (image base 0x400000). "Record offset" is the
  offset in the creature record; the client's creature object holds the record at object
  offset +0x10, so object offset = record offset + 0x10. Offsets marked "obj" are
  client-only state outside the record.
* The world step receives an integer elapsed time **dt in milliseconds** as its first
  argument. Velocities are in blocks per second.
* "Relax *x* toward *t* at rate *r*" means: repeat dt times, *x* becomes
  *x* + (*t* − *x*)·*r*, each step rounded to f32. The scalar helper is 0x411d40, the
  three-component helper 0x605950; both do nothing when the count is not positive.
* World axes: +z is up. Positions are signed 64-bit integers in units of 1/65536 block.
* Block-index conversion used by the routines below: the fixed-point coordinate is divided
  by 65536 with truncation toward zero, and 1 is subtracted when the coordinate is
  negative (so an exact negative multiple of 65536 maps one block lower than a floor would).

---

## 1. Creature-record fields involved

| Record offset (obj offset) | Meaning used here | Written by |
|---|---|---|
| 0x000 (0x010) | position, three signed 64-bit fixed-point values: the **centre** of the collision box | physics, unstick |
| 0x018 (0x028) | orientation, three f32 degrees; the third is yaw | physics, riding, climbing |
| 0x024 (0x034) | velocity, three f32 | physics, jump, dodge, hits |
| 0x030 (0x040) | **movement intent**, three f32, accumulated by input (§3) | input |
| 0x03c (0x04c) | a further three-f32 vector relaxed toward zero by the step (§8.3); meaning unknown | step |
| 0x048 (0x058) | **head pitch**, f32 degrees (§8.5) | step |
| 0x04c (0x05c) | **contact flags**: bit 0 on ground, bit 1 in water, bit 2 touching a wall | step |
| 0x050 (0x060) | kind byte: 0 for the player kind, 1 for the kind the target lock treats as hostile | — |
| 0x058 (0x068) | action id (byte) | combat, input |
| 0x05c (0x06c) | action time (ms) | step |
| 0x070..0x078 (0x080..0x088) | collision box size, three f32 in blocks | — |
| 0x114 (0x124) | **mode flags** (16 bits): 0x01 climbing allowed, 0x04 cleared by input every frame, 0x10 gliding, 0x20 also counts as hostile for the target lock, 0x200 cleared by the step when no lantern is equipped | input, step |
| 0x118 (0x128) | **dodge time** left (ms, signed 32-bit) | dodge |
| 0x11c (0x12c) | hit-stun counter (signed 32-bit) | hits |
| 0x15c (0x16c) | health, f32 | regeneration, hits |
| obj +0x1180 | landing bob value, f32 | step, unstick |
| obj +0x1194 | **stamina**, f32 (not in the record) | dodge (−0.25) |
| obj +0x11a0 | wall normal, three f32 (§8.2) | step |
| obj +0x11c0 | mount id, 64-bit; zero when not mounted | riding |

The client owns its player (`alpha-net-packets.md` §2.3); everything here runs on the
client.

---

## 2. Frame structure (client)

The frame routine 0x488ee0 contains both the world-step calls and the input call.

* **Step subdivision** (0x48cef4..0x48cf49). With elapsed time *E* ms: if *E* ≤ 20 the
  step 0x60c510 runs once with dt = *E*. If *E* > 20 it first runs *n* times with dt = 20,
  where *n* = ⌊(*E* − 21) / 20⌋ + 1, and then once with the remainder *E* − 20·*n*, which
  lies in 1..20. So no single step receives more than 20 ms.
* Earlier in the same routine (0x48c104..0x48c11b), when the game-object byte +0x8006e5 is
  non-zero, the elapsed value is divided by 4 with truncation toward zero; the meaning of
  that byte was not established.
* **Input call** (0x4982b9). The input routine 0x4a6b50 is called only if six interface
  objects (pointers at game object +0x800874, +0x800880, +0x800890, +0x800888, +0x800894 and
  +0x80088c; each queried
  through 0x47fa10) all report false. Its argument is read from the stack local that held
  the elapsed time at 0x48ce3e; whether that local was rewritten in between was not
  established.

The execution order of step and input within one frame, and the relation of the option
`minTimeStep` to the elapsed time, were not established (§10).

---

## 3. Movement intent (0x4a6b50)

The routine returns immediately, doing nothing, when the 32-bit value at offset +0xcc of
the object referenced by game object +0x800710 is non-zero.

**Controlled creature.** The routine works on the local player's creature. If that
creature's mount id (obj +0x11c0) is non-zero, the movement rules below act on the creature
with that id instead (the dodge-availability test and the gliding test of the acceleration
still read the player's own creature). Behaviour when the id is not found is unspecified.

**Acceleration magnitude** *A* = 80; it is 8 when the player's creature passes the gliding
test 0x444650: mode 0x10 set, velocity z strictly negative, not on ground, hit-stun < 1 and
dodge time < 1.

**Directions.** The camera yaw ψ is the f32 degree value at game object +0x1ac, converted
with the f32 constant 0.0174532924 and evaluated with the CRT double cosine and sine, each
rounded to f32. From the identity basis rotated by ψ about z the routine forms two
horizontal unit vectors, written here **r** = (cos ψ, sin ψ, 0) and **f** = (sin ψ, −cos ψ, 0).
For swimming it uses two rows of the game object's camera matrix: **R** = the three f32 at
game object +0x1ec and **F** = the three f32 at +0x20c.

**Inputs.** Four key-state bytes in the game object, read in this order: +0x0b (named
"forward" here), +0x0c ("back"), +0x0e ("left"), +0x0d ("right"); then an analogue pair, two
signed 32-bit values *ax* (game object +0x124) and *ay* (+0x128), processed only when
*ax*² + *ay*² is non-zero. The physical keys bound to these bytes were not traced; the
names follow the geometry below. Let *sx* = *ax*/32768 and *sy* = *ay*/32768 (f32 multiply
by 2⁻¹⁵).

**Dodge availability** (evaluated once per frame): true only when a virtual query on the
game object (vtable slot 1) returns false, the dodge-key byte (game object +0x0a) is
non-zero and the player's stamina is at least 0.25.

For each held key, then the analogue pair, exactly one rule applies, tested in this order
(the creature is the controlled creature):

1. **Climbing** (mode 0x01 and contact bit 2). With *c* = 0.2·*A* and wall normal
   *n* = (*n*x, *n*y, *n*z), let **w** = (*n*y, −*n*x, 0). Forward: intent z += *c*. Back:
   intent z −= *c*. Left: intent −= *c*·**w**. Right: intent += *c*·**w**. Analogue: intent
   += *c*·(−*sx*)·**w**, then intent z += *ay*·*c*·2⁻¹⁵.
2. **Dodge** (dodge available). Applied only if (dodge time < 1 or action ≠ 0) and (contact
   bit 0 or bit 1). Velocity := 40·**d** + (0, 0, 5) with **d** = **f** (forward), −**f**
   (back), −**r** (left), **r** (right), or the unnormalised (−*sx*)·**r** + *sy*·**f**
   (analogue); dodge time := 600; the controlled creature's stamina −= 0.25; action := 0.
   Otherwise the input does nothing.
3. **Swimming** (contact bit 1, and for the keys also action ≠ 107). Intent += *A*·**F**
   (forward), −*A*·**F** (back), −*A*·**R** (left), *A*·**R** (right). Analogue: intent +=
   *A*·((−*sx*)·**R** + *sy*·**F**). The analogue swimming rule has no action-107 exclusion.
4. **No control**: mode 0x10 set, not on ground and stamina ≤ 0 on the controlled creature;
   the input does nothing.
5. **Walking**: intent += *A*·**f** (forward), −*A*·**f** (back), −*A*·**r** (left),
   *A*·**r** (right). Analogue: intent += *A*·((−*sx*)·**r** + *sy*·**f**).

So one walking key adds a horizontal vector of length 80 per frame and two perpendicular keys
add 80√2. Where the step consumes or clears intent was not located (§10).

Also each frame, before the key rules, mode flag 0x04 of the player's own creature is
cleared. After the target lock (§5): if the controlled creature's action is 52 it becomes 0;
and if the player's own intent has a non-zero squared length, a state entry of seven
interface objects (game object +0x8008bc, +0x8008c0, +0x8008c4, +0x8008dc, +0x8008f8,
+0x800900, +0x800910) is set to 0 and the 16-bit value at game object +0x8008f1 is cleared
(interpreted as closing open panels; not confirmed).

---

## 4. Jumping and swimming up (0x4a6b50, end)

Two persistent signed 32-bit globals: *held* (0x76b164, ms the jump key has been held) and
*lock* (0x76b168, ms). The jump key is game-object byte +0x12. The creature is the
controlled creature; dt is the routine's argument, converted to f32 where multiplied.

Each frame:

1. Key up: *held* := 0.
2. Key down: *held* += dt. If *held* ≥ 1:
   * **Take-off**: if *held* < 200, (contact bit 0 set or dodge time ≠ 0) and velocity z ≤ 0:
     velocity z := the smaller of 10 and (√(vy² + vx²) + 2), where the sum of squares is f32,
     the square root is the CRT double routine rounded to f32, and 2 is added in f32;
     *lock* := 100.
   * **Swim up**: then, if contact bit 1 is set and velocity z < 10: *lock* := 100 and
     velocity z += dt·0.001.
3. **Boost**: if the swim-up branch ran, or *lock* > 0: when the key is down and velocity z
   < 10, velocity z += dt·0.08, and if the result is 10 or more it becomes exactly 10.
4. *lock* := max(0, *lock* − dt).

Consequences (input only, before any step effects): a tap from standing gives velocity z = 2;
from horizontal speed *s* it gives min(10, *s* + 2). The boost adds 0.08 per ms of every frame
that starts with *lock* > 0, including the take-off frame, so its total depends on frame
lengths: with 16 ms frames, seven boosts of 1.28 occur (lock 100, 84, 68, 52, 36, 20, 4), and
from standing the value reaches the cap of 10 on the seventh frame. While swimming, holding
the key adds 0.081·dt per frame up to 10.

---

## 5. Target lock (0x4a6b50)

The game object holds one locked target (pointer at game object +0x8006d8).

**Clearing.** Each frame the target is cleared unless its health is > 0 and its squared
distance from the player is ≤ 10000; the distance is the 64-bit position difference with
each component converted to f32 and multiplied by 2⁻¹⁶, squared and summed in f32.

**Selection.** Selection runs on the frame the target-key byte (game object +0x16) becomes
non-zero after being zero on the previous frame (the previous state is kept in global
0x76b160). A creature is a candidate when its kind byte is 1 or its mode flag 0x20 is set,
its health is > 0, it lies in front of the camera and on screen, and it passes a
line-of-sight test:

* Projection: the creature position plus a horizontal fixed-point offset from the game
  object (+0x1d8 and +0x1e0 as 64-bit x and y, z offset 0), scaled by 2⁻¹⁶, is transformed by
  the 4×4 matrix at game object +0x26c with perspective division; the third component must
  be > 0. The result is transformed by the matrix at game object +0x800a90 with perspective
  division; both screen coordinates must lie in [−1, 1].
* Distance: squared distance from the player, computed in fixed point (each component
  product divided by 65536 with truncation toward zero, summed as a 64-bit integer), then
  converted to f32 and multiplied by 2⁻¹⁶. A candidate must be strictly below 10000.
* Line of sight: 0x59ee90 from the creature position toward the position held at game
  object +0x140, with maximum range 200 blocks.

Two passes over the creature list produce: (a) the nearest candidate overall, and (b) the
nearest candidate whose distance is at least the current target's distance (0 when there is
no target) and which is not the current target. The new target is (b) if it exists;
otherwise (a), except that when (a) is the current target the lock is cleared.

---

## 6. Stamina and dodging

* A dodge costs 0.25 stamina and sets dodge time to 600 ms (§3 rule 2). Stamina also gates
  gliding control (§3 rule 4) and dodge availability (≥ 0.25).
* Where the dodge time is decremented and how stamina regenerates were not traced (§10).

---

## 7. Health regeneration (step, 0x60c580..0x60c64c)

At the start of the step the client walks its creature list. For each creature whose kind
byte is 0, whose action is 84 and whose health is > 0:

* if it is the client's own creature (world +0xb8), health := health + maxHealth ×
  ((dt × 0.001) × 0.05), with dt converted to f32 and every product rounded to f32
  (maxHealth from 0x444db0); then, if health exceeds maxHealth, health := maxHealth;
* in every such case a step-local flag is set.

So regeneration happens only while the own creature is in action 84, at 5 % of maximum
health per second. The flag selects the time-of-day advance at world +0x80015c: dt × 100
when set, dt × 10 otherwise (0x60c672..0x60c68c).

---

## 8. Physics step: what is known

### 8.1 Collision box, blocks and unstick (0x60ab30)

* The position is the **centre** of the collision box. For each axis the half size is the
  box size × 0.5 × 65536, rounded toward zero to a 64-bit integer; the box spans position ±
  half size.
* A block is **solid** when the low 5 bits of its fourth byte (the type) are neither 0 (air)
  nor 2 (water) (0x43b480).
* The unstick routine does nothing when the creature's health is negative. Otherwise it
  scans every cell from the block containing the box's low corner to the block containing
  its high corner (x outermost, z innermost). Cell lookup:
  * a cell whose zone or region entry is absent is skipped (not solid);
  * a cell outside the column table, or below the column's stored base, uses default block
    0x76de34 (bytes 200, 200, 200 with type 1: solid);
  * a cell above the column's stored range uses default block 0x76de2c (type 2 with flag
    0x80: water) when z ≤ 0, otherwise 0x76de30 (type 0: air);
  * a stored block of type 0 at z ≤ 0 without flag 0x40 is replaced by the water default.
* If any scanned cell is solid, the creature is lifted once: with *h* = the z half size, the
  bottom block index *b* is computed from position z − *h* by the block-index conversion of
  §0, and position z := *b*·65536 + *h* + 72089, where 72089 is the f32 72089.6 (1.1 × 65536)
  rounded toward zero. The landing bob is set to 0. The block test is not repeated after a
  lift; the routine then continues with the zone's static objects.

### 8.2 Ground and wall contact (0x61d3e6..0x61e86b)

* **Ground**: contact bit 0 is set when a solid block (0x43b480) is found among probe cells
  under the box bottom (the probe uses the constant 1.2), or when either of two step-local
  flags is set; at the same time the landing bob is updated using the constant 1.01. The
  exact probe region and bob rule are open (§10).
* **Wall**: the wall normal is reset, then probes are made just outside the box sides in the
  creature's yaw frame. For creatures of kind 0 the probe offset uses half size and the
  constant 0.2; for other kinds it uses the integer part of (half size + 0.5) and the
  constants 0.5 and 0.05. Four axis probes are always all tested, in this order; a solid
  probe sets contact bit 2 and writes one component of the normal: second component +1,
  second component −1, first component +1, first component −1. A later hit therefore
  overwrites an earlier one in the same component, and hits on both axes leave both
  components non-zero. Only if none of the four axis probes hit, diagonal probes (using the
  constant 0.7) are tested in the order (+1, +1), (−1, +1), (+1, −1), (−1, −1); a hit sets
  bit 2 and writes both components. These normals are not normalised. Whether a diagonal
  hit ends the diagonal series, and the exact probe geometry, are open (§10).

### 8.3 Friction and pushes (0x61d5d4..0x61d7a9)

Let *R* be the step-local rate used here (its derivation is open, §10).

* **Boat**: action 107 with contact bit 1: velocity x and y relax toward 0 at 0.1·*R*, and a
  further component (read through a step-local pointer, interpreted as velocity z) at *R*.
* **Ground**: with contact bit 0 set, velocity relaxes toward (0, 0, 0) at *R*, except: never
  in action 48; in action 54 only once the action time exceeds the sum of two phase
  durations (0x43caa0 and 0x447310); in actions 6, 7, 10, 17, 18, 19 and 20 only once the
  action time is at least the first phase duration (0x43caa0).
* **Record vector 0x03c** relaxes toward (0, 0, 0) at *R*, except when the creature is on
  neither ground nor water, mode 0x10 is clear, and the query 0x477230 on obj +0x1460
  returns true.

### 8.4 Not described (open)

Intent to velocity, gravity, terminal speed, buoyancy and water drag, glide lift, the swept
move of the box through blocks (axis order, step-up, ledges) and yaw turning lie inside
0x60c510 and were not traced.

### 8.5 Equipment and climbing details (0x61f008..0x61f133)

* Mode 0x10 is cleared unless the item at obj +0xf08 (record +0xef8) has type byte 23 and
  subtype byte 0 (interpreted as the glider slot).
* Mode 0x200 is cleared unless the item at obj +0xdf0 (record +0xde0) has type byte 24
  (interpreted as the lantern slot; the two items are 0x118 bytes apart).
* While mode 0x01 is set, contact bit 2 is set and mode 0x10 is clear, head pitch relaxes
  toward 45 at rate 0.005.

---

## 9. Camera

Known: `options.cfg` keys `cameraSpeed`, `cameraSmoothness`, `invertY` and `minTimeStep`
are written by 0x4cef80; reading is in 0x4ce6e0 (defaults: see `alpha-saves.md` §7). The
camera yaw used by §3 is the f32 degree value at game object +0x1ac. Follow distance, pitch
limits, zoom, collision and smoothing were not traced.

---

## 10. Open questions

* **C-1** Execution order of input and step within a frame; relation of `minTimeStep` to the
  elapsed time; meaning of the divide-by-4 byte (§2).
* **C-2** Physical key and stick bindings of the four key bytes and of *ax*, *ay*.
* **C-3** Derivation of the friction rate *R* (§8.3).
* **F-1** Velocity integrator, gravity and swept collision (§8.4); where intent is consumed
  or cleared.
* **F-2** Camera (§9).
* **F-3** Dodge-time decrement and stamina regeneration.
* **F-4** What sets mode 0x01 and 0x10; meaning of mode 0x04 and 0x200 beyond the clears
  described.
* **F-5** Exact ground and wall probe geometry and landing-bob rule (§8.2).

---

## Re-certification (2026-11-14)

Checked against `Cube.exe` (SHA-256 84a7a132…30bf) disassembly and image constants. The
Infinite-Cubes reconstruction was not consulted.

**Confirmed:** relax helper semantics (0x411d40, 0x605950); record offsets of position,
velocity, intent, contact, action, action time, mode, dodge time, hit-stun, health, stamina,
wall normal, mount id; gliding test 0x444650; *A* = 80 or 8; climbing, dodge, swimming,
no-control and walking rule order and key order; dodge constants 40, 5, 600, 0.25; jump
constants 200, 100, 10, 2, 0.001, 0.08 and the exact-10 cap; target-lock clearing at 10000;
block solidity rule; above-range default (water at z ≤ 0, else air); action-52 reset;
glider (type 23, subtype 0) and lantern (type 24) clears; head pitch toward 45 while
climbing; boat rates 0.1·*R* and *R*; ground-friction exception actions; camera yaw at
+0x1ac; option writer 0x4cef80.

**Corrected (old → new):**
1. Health regeneration "every step while health > 0" → only while the own creature is in
   action 84 (and kind 0); also documents the time-of-day side effect. **Reversal.**
2. Unstick "z := ⌊bottom⌋ + 1 + sz/2, test repeats once" → lift of 1.1 block
   (72089/65536), applied once, no repeat; skipped for negative health; absent zones are
   not solid. **Reversal** of the repeat and the +1.
3. "Step clears intent x, y at 0x60d842/0x60d89f" → those writes act on a different object
   kind; intent consumption is open. **Reversal.**
4. Analogue walking "A·(−ax·r − ay·f)" → A·(−sx·r + sy·f) with f and r the forward/right key
   directions. **Sign reversal** of the f term.
5. Third default block "inferred solid" → confirmed solid, type 1, colour 200, 200, 200;
   air at z ≤ 0 without flag 0x40 becomes water (added).
6. "Outside loaded zones a default block is used" → absent zones are skipped.
7. Field-4 relaxation "unless (airborne, gliding)" → skipped only when airborne, not gliding
   and the 0x477230 query is true. **Reversal** of the gliding condition.
8. Wall normals: "one of four values" → four axis probes, all tested, each writing one
   component (later hits overwrite), plus diagonal fallback probes writing both components.
9. Frame order "input then step"; C-1 → step subdivided into ≤ 20 ms steps (confirmed);
   order unconfirmed.
10. Target lock "nearest farther than the current target" → at least as far, excluding the
    current target, candidates below squared distance 10000, rising-edge key, lock cleared
    when the wrap-around result is the current target.
11. Jump "dodging" → dodge time non-zero; boost "up to 8 blocks/s" → frame-dependent total
    (8.96 at 16 ms frames).
12. Input acts on the mounted creature when mounted; analogue input processed only when
    non-zero; analogue swimming lacks the action-107 exclusion; early exit and the
    interface gating of the input call added.
13. Climbing analogue rule stated exactly; "input focus is free" → a virtual query on the
    game object (meaning unknown).
14. Collision box "appearance 0x08" → record offsets 0x070..0x078.

**Removed:** implementer advice (coverage sentence on what can be built now, reading
guidance in §8.4 and §9), the logging procedures attached to C-1 and C-2, the verification
paragraph using mod DLLs, the unverified friction-source address list, and the renderer
claims of the old §6 (body roll, silent footsteps, 1.25× swing, running mode 0x40) and the
riding-pose meaning of mode 0x400 (moved to unresolved).

**Unresolved:**
* Input/step order within a frame; `minTimeStep` semantics; the divide-by-4 condition;
  whether the input routine receives the same elapsed time as the step loop.
* Physical bindings of key bytes +0x0b, +0x0c, +0x0d, +0x0e and of the analogue values.
* Where intent is consumed; the velocity integrator, gravity and swept collision.
* Friction rate *R* and its sources.
* Dodge-time decrement and stamina regeneration.
* Exact ground and wall probe geometry; landing-bob rule.
* Meaning of mode 0x04, 0x40, 0x200, 0x400 beyond the clears described; what sets 0x01
  and 0x10.
* Head pitch outside the climbing condition.
* Renderer effects of dodging and running (old §6 claims), not re-checked.
* Behaviour when a mount id is not found; meaning of the virtual dodge-gating query and of
  the interface objects cleared on movement.
* Server step 0x5322d0 not re-read.
