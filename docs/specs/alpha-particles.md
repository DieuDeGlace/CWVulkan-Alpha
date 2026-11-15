Status: FINAL / STABLE / HELD

# Alpha particles: the burst record, the particle list, its update and drawing, every emitter

Behavioural specification of the **particle system** of Cube World **Alpha**: the 72-byte
**particle burst** record that a simulation step writes to section 3 of its output set, how
the client turns a burst into particles, the client's single **particle list**, the
per-frame **update** (gravity, spin, optional block collision, lifetime, cap), how each
particle is **drawn** (one lit cube), and every place in either program that creates
particles. The procedural **flame cubes** of campfires and some actions, which look like
particles but are not, are specified in §8.

Written from the original Alpha images (`Cube.exe`, the client, and `Server.exe`, the
server; both 32-bit with image base 0x400000) and their instruction listings; float
constants were read from the image bytes. Every address is an **RVA** (image-relative
address) and names its image, e.g. "Cube.exe RVA 0x952f0". Record and object offsets are
byte offsets. Readings marked *believed* rest on partial evidence; *as read* marks
behaviour that looks unintended but is what the code does.

**Scope status.** The burst record, the burst-to-particle expansion, the particle record,
the list and its cap, the update, the drawing state and lighting factor, all nine
section-3 producers, the client-only particle producers and the flame-cube routine are
specified. Open points are in §11.

Companion specifications (pointers only; not restated here): `alpha-rendering.md` (frame
order, world shader, frame material, block light), `alpha-server-simulation.md` §11 (event
sets and section 3), `alpha-creatures-behaviour.md` (hit resolution, spirits),
`alpha-block-edits.md` (bomb explosion, craters), `alpha-pets-bombs.md`,
`alpha-projectiles.md`, `alpha-physics-camera.md` (splash), `alpha-interaction.md`
(static kind 65), `alpha-worldgen-trees.md` (records 60 and 61),
`alpha-materials-wildlife-routes.md` (material colours, timed lights),
`alpha-trails-poses-schedules.md` (weapon trails, which are not particles).

---

## 0. Conventions

* Positions are 3 × i64 in 1/65,536 block. A float vector converted to a position is
  multiplied by 65,536 (float constant at Cube.exe RVA 0x2fce7c) and truncated toward
  zero per axis (Cube.exe RVA 0x2c460 for a vector, 0x2c580 for one component).
* Two different "block containing a position" rules are used:
  * **Collision and world lookups** (Cube.exe RVA 0x2f860): per axis, a non-negative
    coordinate p gives trunc(p / 65,536); a negative one gives trunc(p / 65,536) − 1.
    This equals floor(p / 65,536) except that an exact negative multiple of 65,536 lands
    one block lower (*as read*).
  * **The particle light lookup** of §6.3 (Cube.exe RVA 0x2f570 per axis): plain
    trunc(p / 65,536), i.e. rounding toward zero also for negative coordinates (*as read*).
* *u* is one call of the C runtime `rand()` converted to float and divided by 32,767.0
  (Cube.exe RVA 0x2fd338), so 0 ≤ *u* ≤ 1. "±*a*" means (*u* − 0.5)·2*a*, uniform in
  [−*a*, +*a*]. All particle draws use the C runtime generator, not a world generator.
* dt is the integer first argument of the client frame-update routine (Cube.exe RVA
  0x88ee0), used as milliseconds; *s* = dt converted to float × 0.001 (Cube.exe RVA
  0x2fd870).
* G is the client game object, W = G + 0x2e4 the world. The world millisecond clock is
  W + 0x8000bc (= G + 0x8003a0). G + 0x8006e8 is *believed* the frame's elapsed
  milliseconds (it is added to that clock to ask whether a boundary falls inside this
  frame).
* "Style" is the integer selecting a particle's behaviour (§2.2). Rotation angles are in
  **degrees** (converted with the float 0.01745329, Cube.exe RVA 0x2fcda4).

---

## 1. The burst record (section 3, 72 bytes)

A **burst** asks every client that handles it to spawn *n* particles at one point.

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 24 | 3 × i64 | **position** P (1/65,536 block) |
| 0x18 | 12 | 3 × f32 | **base velocity** v₀ (blocks/s), shared by every particle |
| 0x24 | 16 | 4 × f32 | **colour** (r, g, b, a). Replaced at expansion for style 1 (§2.1 step 6) |
| 0x34 | 4 | f32 | **size**: edge length of each particle cube, in blocks |
| 0x38 | 4 | i32 | **count** *n* |
| 0x3c | 4 | i32 | **style** (§2.2) |
| 0x40 | 4 | f32 | **spread** σ: each particle's velocity gets ±σ per axis (blocks/s) |
| 0x44 | 4 | — | **padding**; no producer writes it and nothing reads it |

* The record constructor (Cube.exe RVA 0x65ed0; Server.exe RVA 0xc8510) sets only style 0
  and spread **3.0**. Producers §5.1–§5.7 use it; §5.8 and §5.9 write style and spread
  3.0 themselves.
* Copying a record into a list node (node factory Cube.exe RVA 0x520d0, copy routine
  RVA 0x65e60) transfers only the first 0x44 bytes, so a listed record's padding is never
  taken from its source.
* A producer that does not set the colour leaves stale stack bytes there; every such
  producer uses style 1, whose colour is replaced at expansion.
* The client's section-3 decoder (Cube.exe RVA 0x58860) reads an i32 record count, then
  for each record pre-sets style 0 and spread 3.0 in a local, copies 72 bytes over it from
  the input and appends it (node factory RVA 0x520d0). If fewer than 72 bytes remain, it
  moves the read cursor to the end, copies nothing and appends the local as it stands.

---

## 2. From burst to particles (Cube.exe RVA 0x952f0 … 0x9556e)

### 2.1 Expansion

Inside the frame update (Cube.exe RVA 0x88ee0), earlier in the routine than the particle
update of §4, the client walks a list of section-3 records (*believed* the records of
the step output it is handling; open point AP-1). For each record with *n* > 0, for
*k* = 1 … *n*, it builds one particle from a freshly constructed blank (Cube.exe RVA
0x65de0, §3.1) and appends it to the end of the particle list (Cube.exe RVA 0x869d0):

1. style := record style; age := 0.
2. position := P (all particles of a burst start at the same point).
3. velocity := v₀ + (±σ, ±σ, ±σ). The three draws are made in the order z, y, x.
4. rotation := (0, 0, 0).
5. spin := 100 × (*u* − 0.5) per axis, i.e. ±50 degrees per second; draws z, y, x.
6. colour := record colour; **if style = 1** the colour is then replaced by
   **(1, *u*, 0, 1)** (one more draw).
7. size := record size; the extra matrix is set to identity.
8. Everything else keeps the blank's values: collides 0, frozen 0, lifetime 4000 ms.

So each particle costs 6 draws, 7 for style 1, in the order velocity z, y, x, spin z, y,
x, then the style-1 colour draw. Because the expansion runs before the update in the same
frame, new particles are moved by that frame's dt before they are first drawn.

### 2.2 Styles

| Style | Gravity | Moves | Colour at spawn | Light factor (§6.3) | Collision exceptions | Used by |
|---|---|---|---|---|---|---|
| 0 | yes | yes | as given | **world-lit** | — | splash, block bursts, arrow hit, death shatter |
| 1 | yes | yes | **(1, *u*, 0, 1)** | full | — | bomb, fire explosion, staff release (non-water), beam end, action-28 hit |
| 2 | yes | yes | as given | full | — | spirit bursts, staff release (water) |
| 3 | — | **never** (no gravity, motion or spin; ages normally) | as given | full | — | no producer |
| 4 | **no** | yes | as given | world-lit | passes through types 7 and 8 | falling leaves |

Any other value behaves as style 2.

---

## 3. The particle record and the list

### 3.1 Particle record (184 bytes; blank made by Cube.exe RVA 0x65de0)

| Off | Type | Field | Blank value |
|---|---|---|---|
| +0x00 | i32 | style (§2.2) | 0 |
| +0x04 | i32 | **age** in ms; negative = not started yet (§4 step 1) | 0 |
| +0x08 | 3 × i64 | **position** (centre of the cube) | not set |
| +0x20 | 3 × i64 | second position, written only by the death shatter (a copy of its spawn position); not read by the update or the draw | not set |
| +0x38 | 3 × f32 | **rotation** (x, y, z) in degrees | 0 |
| +0x44 | 4 × f32 | **colour** (r, g, b, a) | not set |
| +0x54 | 3 × f32 | **velocity** (blocks/s) | 0 |
| +0x60 | 3 × f32 | **spin** (degrees/s per axis) | 0 |
| +0x6c | 4 × 4 f32 | **extra matrix** (§6.2) | identity |
| +0xac | f32 | **size** (cube edge, blocks) | 1.0 |
| +0xb0 | u8 | **collides** with blocks (§4 step 5) | 0 |
| +0xb1 | u8 | **frozen**: skip all motion | 0 (no non-zero writer found) |
| +0xb4 | i32 | **lifetime** in ms | **4000** |

A colour component may exceed 1 (spirit colours, §5.9); nothing clamps it before upload.

### 3.2 The list

There is **one** particle list in the client, a linked list at G + 0x800740 (count at
G + 0x800744). Every reference to it in the client is one of: its initialisation, the
burst expansion (§2), the update (§4), the draw (§6), the falling-leaf emitter (§7.1), the
death shatter (§7.2) and the dormant beam-spark emitter (§7.3). All producers append at
the end. The server has no particle list; it only writes burst records.

**Cap.** At the start of each update (§4), if the list holds more than **5000**
particles, nodes are removed from the end until 5000 remain (Cube.exe RVA 0x95f3c calling
RVA 0x870f0) — *as read*, the newest are dropped, not the oldest. Particles appended after
the update (by the frame routine's leaf and shatter emitters) can raise the count above
5000 until the next update.

---

## 4. The update (Cube.exe RVA 0x95f25 … 0x96329, once per frame)

After the cap, for every particle, in list order:

1. **Age**: age += dt. If the result is negative, or the frozen byte is set, or
   style = 3, skip to step 7.
2. **Gravity** (style ≠ 4): velocity.z −= (dt × 0.001) × 30 (30.0 at Cube.exe RVA
   0x2fcd78), before the move.
3. **Spin**: rotation += spin × *s*.
4. **Without collision** (collides = 0): position += velocity × *s* converted to fixed
   point as in §0, then **rotation += spin × *s* a second time** (*as read*: free-flying
   particles turn twice as fast as their spin). Go to step 7.
5. **With collision** (collides ≠ 0): d = velocity × *s*; *m* = min(trunc(|d|) + 1, 10)
   with |d| the Euclidean length; e = d × (1 / *m*) (the float reciprocal of *m* is
   formed first and multiplied in). Repeat *m* times; each time, for each axis *i* in the
   order x, y, z:
   * position[*i*] += e[*i*] converted to fixed point;
   * look up the block containing the position (first rule of §0). It **blocks** if its
     type (low 5 bits of the block's byte +3) is neither 0 (air) nor 2 (water),
     **except** that for style 4, types 7 and 8 do not block;
   * if it blocks: position[*i*] −= the same fixed-point step; velocity[*i*] := 0; and if
     *i* is z, the whole velocity := (0, 0, 0) and spin := (0, 0, 0).

   e is fixed for the frame, so after a hit the remaining sub-steps still try to move by
   e and are undone again by the same block. The collision path does not repeat the spin.
6. (No other motion.)
7. **Removal**: if age > lifetime the particle is erased.

Consequences:

* **There is no bounce, friction, drag or water effect.** Water is not solid. A colliding
  particle that meets the ground stops and stops turning, and stays (each frame gravity
  gives it a little downward speed, the downward step is undone and the velocity zeroed)
  until its lifetime ends. A wall stops only the component that hit it.
* **Burst particles never collide**: the expansion leaves the collision byte at 0.
* **Nothing fades or shrinks.** Size and colour are constant over the life; a particle
  vanishes in one frame when its age passes the lifetime.
* The integration is one explicit step per frame, so trajectories depend on the frame
  length.

---

## 5. Burst producers (section 3)

The simulation step writes bursts into section 3 of its output set (the list at output
set + 0x10) at nine places. Each exists in both images. Colours given for style-1 bursts
are what the record carries; clients display (1, *u*, 0) instead.

| # | Trigger | Cube.exe RVA / Server.exe RVA | Position | v₀ | Colour | Size | *n* | Style | σ |
|---|---|---|---|---|---|---|---|---|---|
| 5.1 | dead bomb (below) | 0x20ebeb / 0x1349ab | bomb | (0, 0, 20) | (1, 0.2, 0.5) + stale alpha | 0.25 | 20 | 1 | 3 |
| 5.2 | each qualifying block cell of a bomb or crater | 0x20edab, 0x213ca7 / 0x134b6b, 0x139a67 | centre + whole-block offset | (0, 0, 10) | block RGB/255, 1 | 0.5 | 3 | 0 | 3 |
| 5.3 | staff release, actions 30–33 | 0x212594 / 0x138354 | creature + offset at +0x160 | (0, 0, 2) | water: (0, 0.2, 1, 1); else not set | 0.25 | 20 | water 2, else 1 | **8** |
| 5.4 | fire explosion, actions 87/88 | 0x2128a1 / 0x138661 | creature | (0, 0, 2) | not set | 0.25 | **80** | 1 | **20** |
| 5.5 | splash | 0x21e73c / 0x1444fc | creature | (0, 0, 10) | (0.2, 0.7, 1, 1) | 0.4 | **15** | 0 | 3 |
| 5.6 | beam end, actions 28/95 | 0x2169fc / 0x13c7bc | beam end point | −(per-step vector) | (1, 0.2, 0.5) + stale alpha | 0.1 | 5 | 1 | 3 |
| 5.7 | arrow (projectile kind 0) in a solid block | 0x22211a / 0x147eda | projectile | (0, 0, 10) | block colour, 1 | 0.1 | 4 | 0 | 3 |
| 5.8 | hit by an attacker in action 28 | 0x196dd1 / within 0xcfd50 | target | (0, 0, 10) | (0.6, 0.6, 1, 1) | 0.1 + 0.1·*u* | 8 | 1 | 3 |
| 5.9 | special spirit on a combo hit | 0x19a800 / 0xd2190 | target | §5.9 | §5.9 | 0.1 | ⌊3ρ + 1⌋ (+10) | 2 | 3 |

The constructor (§1) is called at exactly the eight sites of §5.1–§5.7 in each image. In
the client, the section-3 append routine (Cube.exe RVA 0x86470) is called from the sites
of §5.1, §5.2 (two), §5.5, §5.6, §5.7, §5.8 and one site shared by §5.3 and §5.4
(Cube.exe RVA 0x2126dc); §5.9 appends through the node factory directly. The only other
users of the node factory are the decoder (§1), a range copy (Cube.exe RVA 0x549b0) and a
forwarder (Cube.exe RVA 0xdc8e0) that copies records whose position lies within a squared
distance of 40,000 (200 blocks) into another list; neither creates bursts. In the server
the corresponding append routine is Server.exe RVA 0x28540.

### 5.1 Bomb

Written for a creature whose health (float +0x16c) is ≤ 0, whose byte +0x60 is non-zero,
whose +0x6c is 0 and whose type (+0x64) is 144 (*believed* the bomb's first dead step;
`alpha-block-edits.md`). The colour (1, 0.2, 0.5) is built as a 3-vector and copied as 4
floats, so the alpha is stale stack (*as read*); both are irrelevant on screen because
style 1 is recoloured.

### 5.2 Block bursts

For each whole-block offset (dx, dy, dz) examined by the bomb or crater routine, the
candidate position is the explosion centre (for the bomb: the bomb's own position)
**plus** the offset converted to fixed point (offset × 65,536). If that cell passes the
routine's distance test and its block has bit 5 of byte +3 set, one burst is written **at
that candidate position** — the centre's sub-block fraction is kept and the cube cloud is
not snapped to the cell — coloured with the block's three colour bytes / 255 and alpha 1.

### 5.3 Staff release (actions 30–33)

While the action time *T* (+0x6c) is below the action's duration *D* (Cube.exe RVA
0x3caa0 with argument −1), sound 36 is requested whenever ⌊(*T* + dt)/200⌋ ≠ ⌊*T*/200⌋,
with pitch 1 + 0.25·*u* (one draw). On the step where *T* < *D* ≤ *T* + dt, the burst is
written at the creature's position (+0x10) plus the float vector at +0x160 converted to
fixed point (*believed* the aim offset), with spread **8.0**; byte +0x141 = 1 selects style
2 and colour (0, 0.2, 1, 1), otherwise style 1 with the colour left unset. Sound 41 (byte
+0x141 = 1) or 38 (otherwise) is requested with the sound record's default pitch and volume
fields (1.0, 1.0).

### 5.4 Fire explosion (actions 87, 88)

The same 200 ms sound 36 while charging. On the step where *T* − dt < *D* ≤ *T*: 80
particles at the creature with v₀ = (0, 0, 2) and spread **20**, plus sound 81 with
default pitch and volume. (The matching crater is in `alpha-block-edits.md`.)

### 5.5 Splash

Count 15, size 0.4, style 0, spread 3, at the creature's position. The trigger (a value
below a threshold at Cube.exe RVA 0x31e214, believed the water-entry speed test) is
specified in `alpha-physics-camera.md`. Being style 0, the splash is world-lit (§6.3).

### 5.6 Beam end (actions 28 and 95)

After the routine's ray loop (Cube.exe RVA 0x2160a0 … 0x2169d0), if the list examined at
Cube.exe RVA 0x2169d9 is empty (*believed*: no creature was hit) and the action is not
104, five fire cubes are written at the creature position plus the normalised aim vector
× the loop's final step count, with v₀ = −1 × the routine's per-step vector (its
magnitude is not established here).

### 5.7 Arrow into a block

When projectile kind 0 finds a solid block (the test of §4) at its position, the
position is corrected by the routine and the burst is written at the corrected
projectile position, coloured from the struck block (colour routine with scale 255),
alpha 1.

### 5.8 Action-28 hit

In hit resolution (Cube.exe RVA 0x196d30), after its entry guards (target health > 0
among them), when the attacker's action byte (+0x68) is 28: a burst at the target's
position (+0x10) with size 0.1 + 0.1·*u* (one draw, at record time), colour
(0.6, 0.6, 1, 1) — but style 1, so clients show fire colours (*as read*).

### 5.9 Spirit bursts

The routine (Cube.exe RVA 0x19a800; Server.exe RVA 0xd2190) takes a position, a velocity
vector, a ratio ρ, an area flag, an item and the output set. It is called once per entry
of a list its caller (Cube.exe RVA 0x197670) iterates (*believed* the equipped weapons),
with the velocity (4·hx, 4·hy, 5), where (hx, hy) is a two-component direction given to
the caller (*believed* the horizontal hit direction), normalised when its squared length
exceeds a threshold. For each of the item's
spirit entries with material *m* ≥ 0x80 it writes one burst:

* position as given; v₀ = the given velocity, doubled with the area flag;
* count = trunc(3ρ + 1), plus 10 with the area flag; size 0.1; style 2; spread 3;
* colour from the material colour routine (Cube.exe RVA 0xc7250) with base (1, 1, 1, 1)
  and strength *t* = 0.5·ρ. With f = 0.5·*t* + 1:

| Spirit | Colour (r, g, b, a) |
|---|---|
| 0x80 | (f, 0.5f, 0.1f, (1 + *t*)·f) |
| 0x81 | (0.3f, f, 0.5f, (1 + *t*)·f) |
| 0x82 | (0.3f, 0.5f, f, (1 + *t*)·f) |
| 0x83 | (0.8f, 0.8f, f, (1 + *t*)·f) |

Style 2 keeps the colour (components may exceed 1) and gets light factor 1 (§6.3).

---

## 6. Drawing (Cube.exe RVA 0xb7dbc … 0xb8138)

### 6.1 Render state

The draw is part of the client frame routine (Cube.exe RVA 0xac260); its place among the
other passes is as described in `alpha-rendering.md`. Before the list it sets render
state 0x16 (cull mode) to 1 (no culling) and after the list restores it to 2. Inside the
loop it uploads only the material colour (§6.3) and the world matrix (§6.2) and draws the
cube model; depth, blending and shaders are whatever the frame has bound at that point.

### 6.2 Geometry and culling

Each particle is the client's **unit cube model** (G + 0x800730: a 1 × 1 × 1 voxel
model whose single voxel is (255, 255, 255), created at start-up in Cube.exe RVA 0x59c40
at RVA 0x61f95, sized by RVA 0xe75c0 and finalised by RVA 0xe7870). Its world matrix is
built by successive local-space compositions, in this call order: translate(position
relative to the render origin), rotate rot.z about z, rotate rot.y about y, rotate rot.x
about x (Cube.exe RVA 0x241b0, degrees), scale(size), the extra matrix, and
translate(−0.5, −0.5, −0.5). Applied to a model vertex, the last-listed step acts first,
so the cube is centred on the particle position, *size* blocks on each edge. Burst and
leaf particles have an identity extra matrix; shatter particles carry their model's
orientation and scale there (§7.2).

**Culling per particle** (Cube.exe RVA 0x7f760, called with the position, the size and a
radius R): skipped unless its squared distance from the camera position (G + 0x140) is at
most R², and unless it lies inside all six view planes (stored from G + 0x1000fa8, 16
bytes each) with a margin of *size*. R is a frame-routine local set at Cube.exe RVA
0xb0721 to **0.2 × (the float at G + 0x1d4 + 22.4)** (the float G + 0x1d4 plus 22.4 is
formed at RVA 0xac50c).

### 6.3 Colour and light factor

The material colour uploaded for each particle (Cube.exe RVA 0x48280, a vertex shader
constant whose register comes from the renderer object) is the componentwise product

colour × frame material × (1, 1, 1, L),

with the frame material the frame routine's material vector (`alpha-rendering.md`) and

* **styles 0 and 4 (world-lit)**: L = max(V / 255, **0.2**), where V is the light value
  (Cube.exe RVA 0x718b0) of the block found with the second rule of §0: type 13 → 255;
  type 0 or 2 → the block's byte +0 raised to at least 5; any other type → 0;
* **all other styles**: L = 1, and after the product the **alpha is forced to 1**.

How the bound shader uses this colour is specified in `alpha-rendering.md`.

---

## 7. Client-only particle producers

### 7.1 Falling leaves (tree records of kind 60; Cube.exe RVA 0xb1eb4 … 0xb212f)

In the frame routine, for every zone whose position (zone + 0x50) lies horizontally (x, y
only) within **0.2 × (the float at G + 0x1d0) + 32** blocks of the camera (G + 0x140) —
the 0.2 × factor is formed at Cube.exe RVA 0xad9d1, the + 32 at RVA 0xb1dde — and for
every record of kind **60** in the zone's list, when ⌊(clock + G + 0x8006e8)/1000⌋ ≠
⌊clock/1000⌋ (clock = W + 0x8000bc) and then `rand()` mod 16 = 0, **one** particle:

* style **4**, collides = 1, lifetime **20,000 ms**, age 0;
* position = the record position (record + 0x8); rotation 0;
* velocity = (4·(*u* − 0.5), 4·(*u* − 0.5), −2) (two draws, y then x);
* spin ±5 °/s per axis (draws z, y, x; factor 10);
* colour = the record's three floats at +0x2c, alpha 1; size **0.5**; extra matrix
  identity.

Draw order per record and crossing: the mod-16 draw, then velocity y, x, then spin z, y,
x. The only leaf append in the frame routine is gated on kind 60, so records of kind 61
make no particles.

### 7.2 Death shatter (Cube.exe RVA 0xb3a31 … 0xb4124; model routine RVA 0x70d80)

When the frame routine meets a creature: if its health (+0x16c) is > 0, byte +0x1d10 is
cleared. Otherwise, if +0x1d10 is set and the creature passes its draw-visibility test
(Cube.exe RVA 0x7f760), it breaks the creature's models into voxel cubes by **ten** calls
to RVA 0x70d80 (at RVAs 0xb3b6f, 0xb3c50, 0xb3cd9, 0xb3d62, 0xb3deb, 0xb3e74, 0xb3efd,
0xb3f86, 0xb400f, 0xb4098); in every case the byte is then cleared, so a dead creature that
fails the visibility test loses the flag without shattering. What sets +0x1d10 is open
(AP-2).

Every call passes style **0** and a tint. Nine calls pass tint (1, 1, 1, 1); the second
call (skipped when bit 0x400 of the creature's word +0x7e is set) passes (byte +0x7a/255,
byte +0x7b/255, byte +0x7c/255, 1).

For a model with world transform M (render-origin relative), the routine visits voxels
with x outermost, then y, then z innermost. For every voxel that is **not empty** and has
**at least one empty face neighbour** (the model's own empty test; neighbours outside the
model use a default voxel), one draw `rand()` & 1 decides; when it is 0 one particle is
made:

* style := the caller's argument (0);
* age := −(`rand()` mod 1000): the cube **waits** up to 0.999 s before it starts moving,
  drawn where it is meanwhile;
* position := M applied to the voxel's integer corner (x, y, z) (with perspective
  divide; no +0.5, *as read*), converted to fixed point and taken back to world space;
  the same value is also stored at +0x20;
* velocity := 2 × (that position − M applied to the model's centre (dims / 2)), computed
  in fixed point (factor 131,072, then two 1/65,536 scalings), blocks/s;
* spin := (±50, ±50, ±50) °/s with draws x, y, z; rotation 0;
* colour := (voxel bytes / 255) × the tint per channel, alpha = tint alpha;
* extra matrix := M with its translation row zeroed; size 1.0;
* collides = 1, frozen 0, lifetime **5000 ms**.

Draw order per surface voxel: selection, then (if selected) age, spin x, y, z.

If the creature's type (+0x64) is **144**, the same pass then appends one timed light to
G + 0x800748: position the creature, colour (1.0, 0.8, 0.5), 16.0 in the field
`alpha-materials-wildlife-routes.md` calls the radius, lifetime 800 ms.

### 7.3 Beam sparks (dormant; Cube.exe RVA 0x71d50)

The segmented-beam drawer contains a spark emitter behind a byte argument. Per segment,
each frame: first one draw with `rand()` mod 8 = 0, then the test
⌊(clock + G + 0x8006e8 + phase)/200⌋ ≠ ⌊(clock + phase)/200⌋ (clock = G + 0x8003a0,
phase = segment index × 200 / the segment-count argument). If both hold: style 2,
position on the segment, velocity (5 − 10*u*, 5 − 10*u*, 5 + 5*u*) with draws z, y, x,
size (0.05 + 0.005*u*) × the width argument, colour the beam's, appended to the list. All
three callers (Cube.exe RVAs 0xb3009, 0xb68f0, 0xb6e7c) pass **0** for the enable byte, so
it never runs in the shipped client.

---

## 8. Flame cubes (procedural, not particles; Cube.exe RVA 0xbbd80)

Fire is not made of list particles. Some objects and actions draw, every frame, *N* unit
cubes whose positions, sizes and colours are pure functions of a millisecond value *t*;
nothing is stored and nothing collides. The routine takes a base position, colours A and
B, *t*, two further values passed through to the matrix upload, a radius *r*, a size *z*
and a count *N*. For flame *i* = 0 … *N* − 1:

1. f = ((*t* + 443·*i*) mod 2000) / 2000 — progress from base to tip, a 2 s cycle
   staggered by 443 ms per flame.
2. c = cos(13·*i* + 2π·(*t* mod 500)/500); q = 2(f − 0.5);
   **edge** = (0.5·c + 1) · *z* · (1 − q²).
3. **colour** = A·(1 − f) + B·f, uploaded as the material colour unchanged.
4. a = 2π·(*i*/10 + (*t* mod 10000)·0.0001); **position** = base +
   (*r*·cos a, *r*·sin a, 2·*r*·f).
5. The cube is turned by f·40°, f·30° and f·10° about three axes, scaled by the edge
   (when it differs from 1), centred, and drawn with the unit cube model.

Callers:

| Cube.exe RVA | What | *N* | *r* | *z* | A (base) | B (tip) |
|---|---|---|---|---|---|---|
| 0xb325c | static object of kind 65 (`alpha-interaction.md`), base = position + (0, 0, half of a scale value) | 20 | 0.8 | 0.2 | (1, 0, 0) | (1, 1, 0) |
| 0xb4676 | creature in action 34 while *T* < *D*, *t* = *T*; drawn for the creature found by the id at the actor's +0x1a0 (visibility-tested at that creature's +0x1350) | 20 | x of that creature's vector +0x80 | 0.1 | (0, 0, 1) | (0.25, 0.75, 1) |
| 0xb705c | creature path *believed* action 87/88 charging | 20 on the path seen | frame local | 1 + 2·(frame values) | (1, 0, 0) | (1, 1, 0) |
| 0xb55b5 | entry with first byte 4 in a list walked per drawn creature (entries 1 and 2 of that list set a tint instead); base = creature +0x1350 minus (0, 0, float +0x1180); *t* = 100,000 − entry +0x8 | 8 | x of the vector +0x80 of the creature referenced by G + 0x8006d0 (*as read*) | 0.1 | (0, 1, 0) | (0.5, 1, 0) |
| 0xb4430, 0xb4549 | a variant routine (Cube.exe RVA 0xbc760) for actions 30–33 while *T* < *D* (second call for 31 and 33 only); byte +0x141 = 1 selects (0, 0.1, 1) → (0.5, 0.7, 1), else (1, 0, 0) → (1, 1, 0) | open | open | open | | |

Worked value (kind 65, flame 0 at *t* = 1000): f = 0.5, q = 0, c = 1, edge = 1.5 × 0.2 =
0.3 blocks, colour (1, 0.5, 0), a = 2π·0.1 → offset (0.647, 0.470, 0.8).

Since nothing else references the particle list (§3.2), no other effect — torches,
fire-ground blocks, footsteps, landing, level-up, pickups, eating, drinking — creates
particles.

---

## 9. Server and client

* **The server** neither simulates nor draws particles. It writes the bursts of §5 for
  what it simulates into section 3; the distribution of section 3 to players is specified
  in `alpha-server-simulation.md` §11.
* **Each client** expands every burst it handles with its own `rand()` draws, so two
  players see different spreads, spins and fire colours for the same burst.
* **Purely local**: falling leaves (§7.1), death shatter (§7.2), flame cubes (§8); they
  depend on the local clock, local `rand()` and local game-object values.

---

## 10. Worked examples

1. **Bomb burst.** Record: v₀ = (0, 0, 20), size 0.25, *n* 20, style 1, spread 3. A
   particle with velocity draws z 0.9, y 0.2, x 0.5 gets velocity (0, −1.8, 22.4); its
   colour is (1, *u*, 0, 1). At dt 16 (*s* 0.016), v.z drops by 0.48 per frame; the apex
   is about 22.4/30 = 0.747 s and 22.4²/60 ≈ 8.36 blocks up; with no collision it falls
   through the ground and is removed on the first frame its age exceeds 4000 ms, roughly
   89.6 − 240 ≈ 150 blocks below P. Rotation advances by 2 × spin × *s* per frame.
2. **Spirit 0x80, ρ = 0.5, no area, hit direction (1, 0).** *n* = trunc(2.5) = 2;
   *t* = 0.25, f = 1.125: colour (1.125, 0.5625, 0.1125, 1.40625); v₀ = (4, 0, 5), each
   particle ±3; size 0.1; style 2, light factor 1.
3. **Leaf.** Record colour (0.8, 0.4, 0.1); velocity draws *u* = 0.75 (y), 0.25 (x) give
   velocity (−1, 1, −2). It passes through blocks of types 7 and 8, stops on the first
   other non-air, non-water block, and vanishes when its age exceeds 20,000 ms.
4. **Collision sub-steps.** A shatter cube with velocity (3, 0, −40) at dt 20: d = (0.06,
   0, −0.8), |d| ≈ 0.802 → *m* = 1, e = d. If moving z enters a solid block: undone,
   velocity := 0, spin := 0. Next frame v.z = −0.6, d.z = −0.012: undone again; the cube
   rests.
5. **Cap.** Two bombs each removing 1000 cells plus their own bursts expand to
   2 × 3020 = 6040 particles in one frame; the cap at the start of that frame's update
   removes the 1040 newest.

---

## 11. Open points

* **AP-1** Which step outputs the burst expansion walks (§2.1): the local step's set, the
  received world update, or both; and therefore whether an online client can expand a
  burst twice.
* **AP-2** The writer of the shatter byte +0x1d10 (§7.2).
* **AP-3** The meaning of the empty-list condition of the beam-end burst and the
  magnitude of its per-step vector (§5.6).
* **AP-4** The meaning of the floats G + 0x1d0 and G + 0x1d4 used for the cull radius
  (§6.2) and the leaf radius (§7.1) (*believed* view/fog distance settings), and of
  G + 0x8006e8 (§0).
* **AP-5** Shader state and constants not set by the particle draw (depth, blending,
  shader programs, point lights, flash constants) and how the bound shader uses the
  material colour (§6.1, §6.3).
* **AP-6** The caller of §8 at Cube.exe RVA 0xb705c (action gate, *r* and *z* sources)
  and the variant routine RVA 0xbc760 (count, radius, size, differences from RVA 0xbbd80).
* **AP-7** Whether bursts produced by an online client's own step are ever seen by other
  players.

## Re-certification (2026-11-15)

Checked against Cube.exe and Server.exe instruction listings and image bytes. All
addresses are now RVAs with the image named (old text used image-base-relative VAs; RVA =
VA − 0x400000 for both images).

**Confirmed.** The 72-byte record layout and constructor defaults (both images); decoder
pre-set of style 0 / spread 3.0; expansion order and draw order (velocity z, y, x; spin z,
y, x, ×100; style-1 colour (1, *u*, 0, 1)); blank particle values and the 184-byte layout;
the single list at G + 0x800740 and its 5000 cap removing from the end; the update steps
(age, skip conditions, gravity 30 before the move, double spin without collision, collision
sub-steps min(trunc(|d|) + 1, 10), axis order x, y, z, the solid test and the style-4
exceptions, zeroing on z hits, removal when age > lifetime); cull-state 1/2 around the
draw; the unit cube model; the matrix composition order; the 0.2 light-factor floor,
alpha forcing for non-world-lit styles and the light value rule; all producer fields of
§5.1, §5.2 (count, size, colour, v₀), §5.4–§5.9; spirit colour formulas and material
mapping; leaf emitter fields and gating; shatter fields, 50 % selection, age wait,
lifetime 5000, bomb flash light; spark emitter fields and the three disabled callers; the
flame-cube formulas and the kind-65 caller and worked value.

**Corrected** (old → new):
1. Cull radius R: (0.9·V)² with V the view/fog distance → R = 0.2 × (float G + 0x1d4 +
   22.4). **Reversal; affects visuals** (particles beyond R are not drawn).
2. Leaf zone radius: V + 32 with the same V as the cull → 0.2 × (float G + 0x1d0) + 32,
   horizontal distance. **Reversal; affects visuals.**
3. Staff release spread: 3 → **8.0** (both images); "only §5.4 changes the spread" → §5.3
   and §5.4 do. **Affects visuals.**
4. Block-burst position: the cell's integer corner → explosion centre + whole-block
   offset, keeping the centre's fraction. **Affects visuals.**
5. Light lookup cell: "the block containing the particle" → per-axis truncation toward
   zero; collision/world cell: floor → floor except exact negative multiples one block
   lower. Affects visuals only at negative coordinates.
6. Collision sub-step: e = d / *m* → d × (1 / *m*) (float reciprocal; numeric precision).
7. Shatter: style and tint *believed* → confirmed style 0 for all ten calls; nine white
   tints, one from creature bytes +0x7a…+0x7c; added that a dead flagged creature failing
   visibility loses the flag without shattering (**affects visuals**); added voxel visit
   order and per-voxel **random-draw order** (selection, age, spin x, y, z).
8. Beam sparks: "chance ⅛ per 200 ms boundary" → mod-8 draw every frame per segment
   *before* the boundary test (**random-draw order**); added velocity draw order z, y, x;
   caller identities removed.
9. Leaves: added the per-crossing **draw order** (mod 16, velocity y, x, spin z, y, x).
10. "Every producer uses the constructor" → §5.8 and §5.9 set style and spread inline.
11. Flame caller RVA 0xb55b5: "object with type byte 4, believed a world item" → an entry
    of a per-creature list; base, *t* and *r* sources stated. Caller RVA 0xb705c: "*r* as
    above" removed (*z* = 1 + 2·(frame values), sources open).
12. §5.6 beam: "moving back at 1 block/s" removed (step magnitude not established);
    position stated as creature + normalised aim × final step count.
13. §5.3 position: "creature + aim (field 26)" → creature + float vector at +0x160;
    release condition stated as *T* < *D* ≤ *T* + dt (§5.4 differs: *T* − dt < *D* ≤ *T*).
14. §2.1: "after the other event sections" → only "earlier than the update" (another
    section walk follows it).
15. §1: padding "whatever was on the producer's stack" → node copies transfer only the
    first 0x44 bytes; decoder behaviour on truncated input added.
16. §6.3: "shader constant c32" → a vertex shader constant whose register comes from the
    renderer object; light rule "stored value" → block byte +0.
17. §2.2: arrow hit added to style-0 users; style 3 noted as still ageing.

**Removed.** Advice to implementers (writing 0 into padding/unset colours, receiver rule,
"reproduces vanilla by not emitting", wire-compatibility requirement); every *Game check*
verification procedure; the "Corrections to other specs" section; the claim that the
server's 200-block filter reads the record position; the depth/blend/opacity claims and
the vertex-shader lighting description (night ambient example) of §6.1/§6.3; the
statement that a client never uploads its own bursts; splash night/noon example numbers;
the bomb "~1000 blocks" estimate; "negligible frame-rate dependence"; spirit names; the
spark emitter's subject identification; the per-effect explanations of what torches,
pickups etc. do instead.

**Unresolved.** AP-1 to AP-7 in §11.
