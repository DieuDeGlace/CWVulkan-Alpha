# Alpha particles: the burst record, the particle list, its update and drawing, every emitter

Behavioural specification of the **particle system** of Cube World **Alpha**: the 72-byte
**particle burst** record that a simulation step writes to section 3 of its output set, how
the client turns a burst into particles, the client's single **particle list**, the
per-frame **update** (gravity, spin, optional block collision, lifetime, cap), how each
particle is **drawn** (one lit cube), and every place in either program that creates
particles. The procedural **flame cubes** of campfires and some actions, which look like
particles but are not, are specified in §8 because they are what a player calls "fire".

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/client` and
`…/server`), the per-instruction listings, and fresh reads of float constants from the
`Cube.exe` image (a small section-table reader; the listing shows only their addresses).
No code was copied. Routines are cited as "client 0x…" or "server 0x…"; record and object
offsets are given for cross-checking only. Readings marked *believed* rest on partial
evidence; *as read* marks behaviour that looks unintended but is what the code does.

**Status.** The burst record (all fields), the burst-to-particle expansion, the particle
record, the list and its cap, the update, the drawing state and lighting, all nine
section-3 producers, the three client-only particle producers and the flame-cube routine
are specified. Not traced: what raises the creature "shatter" flag (§6.2), the exact
trigger of the beam-end burst (§5.6) and two of the flame-cube callers (§8.3). Open points
are in §12.

This document **adds to** and does not repeat:

* `alpha-rendering.md` — the frame (§1.3; the particle draw is the "then particle effects"
  of step 12), the world vertex shader (§5), the frame material (§4), the block light
  value rule of §11.2 step 1, the cube model conventions of §11.1;
* `alpha-server-simulation.md` §11 — the event set and section 3 (the record layout there is
  completed by §1 here), the 200-block distance filter;
* `alpha-creatures-behaviour.md` §5 (hit resolution, step 3 "particle effect at the
  target"), §12.1 (spirit bursts), §14 (sound ids);
* `alpha-block-edits.md` §8 (bomb explosion) and §9 (craters) — the per-block burst rule;
* `alpha-pets-bombs.md` §3.3–§4.1, `alpha-projectiles.md` §5.2, `alpha-physics-camera.md`
  §5.3 (splash), `alpha-interaction.md` (static kind 65, campfire), `alpha-worldgen-trees.md`
  (records 60 and 61), `alpha-materials-wildlife-routes.md` §1 (material colours, the
  timed-light list G+0x800748).

Weapon trails are not particles and are specified in `alpha-trails-poses-schedules.md`.

---

## 0. Conventions

* Positions are 3 × i64 in 1/65536 block (as in `alpha-physics-camera.md` §0); a float
  vector converted to a position is multiplied by 65,536 and truncated toward zero
  (client 0x42c460, 0x42c580). The block containing a position is floor(position / 65536)
  per axis (client 0x42f860).
* *u* is one draw of the C runtime `rand()` divided by 32,767.0 (constant 0x6fd338), so
  0 ≤ *u* ≤ 1. "±*a*" below means (*u* − 0.5)·2*a*, i.e. uniform in [−*a*, +*a*].
  All particle draws are made on the render/update thread with the C runtime generator,
  not with any world generator (`alpha-worldgen-random.md`).
* dt is the frame's elapsed time in whole milliseconds as handed to the frame update
  (client 0x488ee0, its local at +0x38); *s* = dt × 0.001 (constant 0x6fd870) seconds.
* G is the game object, W = G + 0x2e4 the world (as in `alpha-rendering.md`). The world
  millisecond clock is W+0x8000bc (= G+0x8003a0, `alpha-world-items.md`). G+0x8006e8 is
  *believed* the frame's elapsed milliseconds (it is added to that clock to ask "does a
  boundary fall inside this frame").
* "Style" is the integer that selects a particle's behaviour (§2.2). Angles of the particle
  rotation are in **degrees**.

---

## 1. The burst record (section 3, 72 bytes)

A **burst** asks every client that receives it to spawn *n* particles at one point. The
record is the same in client and server memory and on the wire (`alpha-net-packets.md`
§3.4 section 3; `alpha-server-simulation.md` §11):

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 24 | 3 × i64 | **position** P (1/65536 block). Because it comes first, the server's 200-block filter of section 3 reads it as a position, correctly. |
| 0x18 | 12 | 3 × f32 | **base velocity** v₀ (blocks/s), shared by every particle |
| 0x24 | 16 | 4 × f32 | **colour** (r, g, b, a). Ignored for style 1 (§2.1 step 6) |
| 0x34 | 4 | f32 | **size**: edge length of each particle cube, in blocks |
| 0x38 | 4 | i32 | **count** *n* |
| 0x3c | 4 | i32 | **style** (§2.2) |
| 0x40 | 4 | f32 | **spread** σ: each particle's velocity gets ±σ per axis (blocks/s) |
| 0x44 | 4 | — | **padding**. No producer writes it and the client never reads it; it exists because the record holds 64-bit integers and is 8-aligned. Its bytes are whatever was on the producer's stack. |

Defaults set by the record constructor (client 0x465ed0, server 0x4c8510): style 0,
spread **3.0**. The section-3 decoder (client 0x458860) also pre-sets style 0 and spread
3.0 before copying the received 72 bytes over them, so the defaults never survive a
received record. Every producer in §5 uses the constructor; only one (§5.4) changes the
spread. A producer that does not set the colour leaves stale bytes there; this matters
only for styles other than 1 (none of the producers that skip the colour use them).

An implementation should write 0 into the padding and into any colour it does not set.
A receiver must ignore the padding.

---

## 2. From burst to particles (client 0x4952f0 … 0x49556e)

### 2.1 Expansion

In the frame update (client 0x488ee0), after the other event sections and **before** the
particle update of §4, the client walks the section-3 records of the step output it is
handling (*believed*: both the set produced by its own simulation step and, online, the
world update received from the server, which the decoder 0x458860 appends to the same kind
of list). For each record, for *k* = 1 … *n*, it builds one particle (§3) from a freshly
constructed blank (client 0x465de0) and appends it to the list (§3.2):

1. style := record style; age := 0.
2. position := P (all particles of a burst start at the same point; no position jitter).
3. velocity := v₀ + (±σ, ±σ, ±σ). The three draws are made in the order z, y, x.
4. rotation := (0, 0, 0).
5. spin := (±50, ±50, ±50) degrees per second (draws in the order z, y, x; constant
   100 × (*u* − 0.5)).
6. colour := record colour; **if style = 1** the colour is replaced by **(1, *u*, 0, 1)**
   (one more draw): a random red-to-yellow fire colour.
7. size := record size.
8. Everything else keeps the blank's values: no collision, not frozen, lifetime 4000 ms,
   extra matrix = identity (§3.1).

So a burst costs 6·*n* draws, 7·*n* for style 1. Because the expansion runs before the
update in the same frame, new particles already move by one frame's dt before they are
first drawn.

### 2.2 Styles

| Style | Gravity | Moves | Colour at spawn | Lighting (§6.3) | Collision exceptions | Used by |
|---|---|---|---|---|---|---|
| 0 | yes | yes | as given | **world-lit**: scaled by sunlight and the block light where it is | — | splash, block bursts, death shatter |
| 1 | yes | yes | **(1, *u*, 0, 1)** | full strength | — | bomb, fire explosion, staff release (fire), beam end, action-28 hit |
| 2 | yes | yes | as given | full strength | — | spirit bursts, staff release (water) |
| 3 | — | **never** (no gravity, no motion, no spin) | as given | full strength | — | no producer |
| 4 | **no** | yes | as given | world-lit | passes through wood (7) and leaves (8) | falling leaves |

Any other value behaves as 2 (moves with gravity, full-strength lighting).

---

## 3. The particle record and the list

### 3.1 Particle record (184 bytes; blank made by client 0x465de0)

| Off | Type | Field | Blank value |
|---|---|---|---|
| +0x00 | i32 | style (§2.2) | 0 |
| +0x04 | i32 | **age** in ms; negative = not started yet (§4 step 1) | 0 |
| +0x08 | 3 × i64 | **position** (centre of the cube) | not set |
| +0x20 | 3 × i64 | second position, written only by the death shatter (its spawn point); never read | not set |
| +0x38 | 3 × f32 | **rotation** (x, y, z) in degrees | 0 |
| +0x44 | 4 × f32 | **colour** (r, g, b, a) | not set |
| +0x54 | 3 × f32 | **velocity** (blocks/s) | 0 |
| +0x60 | 3 × f32 | **spin** (degrees/s per axis) | 0 |
| +0x6c | 4 × 4 f32 | **extra matrix**, applied after rotation and scale (§6.2) | identity |
| +0xac | f32 | **size** (cube edge, blocks) | 1.0 |
| +0xb0 | u8 | **collides** with blocks (§4 step 5) | 0 |
| +0xb1 | u8 | **frozen**: skip all motion | 0 (no writer found) |
| +0xb4 | i32 | **lifetime** in ms | **4000** |

A float colour component may exceed 1 (spirit colours, §5.9); nothing clamps it before the
shader.

### 3.2 The list

There is **one** particle list for the whole client, a linked list at G+0x800740 (count at
G+0x800744), filled by the burst expansion (§2), the falling-leaf emitter (§7.1) and the
death shatter (§7.2), and appended at the end. It is not part of the world, not saved and
not sent anywhere. The server has no particle list at all; it only writes burst records.

**Cap.** At the start of each update (§4), if the list holds more than **5000** particles
the newest are removed from the end until 5000 remain (client 0x495f3c, 0x4870f0) — *as
read*, the newest are dropped, not the oldest. Particles appended later in the same frame
by the drawing pass (leaves, shatter) can push the count above 5000 until the next update.

---

## 4. The update (client 0x495f25 … 0x496329, once per frame)

After the cap, for every particle, in list order:

1. **Age**: age += dt. If age is now negative, or the frozen byte is set, or style = 3,
   skip to step 7.
2. **Gravity** (style ≠ 4): velocity.z −= 30 × *s* (30 blocks/s², constants 0x6fd870 and
   0x6fcd78 = 30.0). This happens before the move (semi-implicit).
3. **Spin**: rotation += spin × *s*.
4. **Without collision** (collides = 0): position += velocity × *s* (converted to fixed
   point as in §0), then **rotation += spin × *s* a second time** (*as read*: free-flying
   particles turn twice as fast as their spin says). Go to step 7.
5. **With collision** (collides = 1): d = velocity × *s*; *m* = min(trunc(|d|) + 1, 10)
   sub-steps; e = d / *m*. For each sub-step, for each axis *i* in the order x, y, z:
   * position[*i*] += e[*i*];
   * look up the block containing the position. It **blocks** if its type is not 0 (air)
     and not 2 (water) — the usual "solid" test — **except** that for style 4 types 7
     (wood) and 8 (leaves) do not block;
   * if it blocks: position[*i*] −= e[*i*]; velocity[*i*] := 0; and if *i* is z: the whole
     velocity := (0, 0, 0) and spin := (0, 0, 0).

   The sub-step increment e is fixed for the frame, so after a hit the remaining sub-steps
   keep trying to move by e (and are stopped again by the same block).
6. (The collision path does not repeat the spin.)
7. **Removal**: if age > lifetime the particle is erased.

Consequences:

* **There is no bounce, no friction, no drag and no water effect.** Water is not solid:
  particles fall through it at full speed. A colliding particle that hits the ground stops
  dead and stops turning, and stays on the ground (each frame gravity gives it a little
  downward speed, the first sub-step is undone, the speed is zeroed) until its lifetime
  ends. A wall stops only the horizontal component that hit it.
* **Burst particles never collide**: the expansion leaves the collision byte at 0. They
  fly through terrain, walls and the floor.
* **Nothing fades or shrinks.** Size and colour are constant over the life; a particle
  vanishes in one frame when its age passes the lifetime.
* The update is frame-rate dependent only through the per-frame Euler step; with the
  usual 10–20 ms frames the difference is negligible.

---

## 5. Burst producers (section 3)

The simulation step writes bursts into section 3 of its output set (list +0x10) at nine
places. Each exists in both programs (the client copy runs for what the client simulates
itself, `alpha-server-simulation.md` §1; the server copy for NPCs and the world). All use
spread 3 unless stated. Colours given for style-1 bursts are what the record carries;
clients display (1, *u*, 0) instead.

| # | Trigger | Client / server | Position | v₀ | Colour | Size | *n* | Style |
|---|---|---|---|---|---|---|---|---|
| 5.1 | bomb explodes | 0x60ebeb / 0x5349ab | bomb | (0, 0, 20) | (1, 0.2, 0.5) + stale alpha | 0.25 | 20 | 1 |
| 5.2 | each natural block removed by a bomb or a crater | 0x60edab, 0x613ca7 / 0x534b6b, 0x539a67 | the block's cell corner (trunc(C + d)) | (0, 0, 10) | block RGB/255, 1 | 0.5 | 3 | 0 |
| 5.3 | staff release, actions 30–33 | 0x612594 / 0x538354 | creature + aim (field 26) | (0, 0, 2) | water: (0, 0.2, 1, 1); fire: not set | 0.25 | 20 | water 2, else 1 |
| 5.4 | fire explosion, actions 87/88 | 0x6128a1 / 0x538661 | creature | (0, 0, 2) | not set | 0.25 | **80** | 1, spread **20** |
| 5.5 | water entered fast (splash) | 0x61e73c / 0x5444fc | creature | (0, 0, 10) | (0.2, 0.7, 1, 1) | 0.4 | **15** | 0 |
| 5.6 | beam end, actions 28/95 (*believed*: beam hit no creature) | 0x6169fc / 0x53c7bc | beam end point | −(beam step vector) | (1, 0.2, 0.5) + stale | 0.1 | 5 | 1 |
| 5.7 | arrow (projectile kind 0) hits a solid block | 0x62211a / 0x547eda | projectile | (0, 0, 10) | block RGB/255, 1 | 0.1 | 4 | 0 |
| 5.8 | hit by an attacker in action 28 | 0x596dd1 / in 0x4cfd50 | target | (0, 0, 10) | (0.6, 0.6, 1, 1) | 0.1 + 0.1·*u* | 8 | 1 |
| 5.9 | special spirit on a combo hit | 0x59a800 / 0x4d2190 | target | §5.9 | §5.9 | 0.1 | ⌊3ρ + 1⌋ (+10) | 2 |

A search of both binaries for appends to section 3 finds exactly these (client callers of
0x486470 and 0x4520d0; server callers of 0x428540 and the spirit routine).

### 5.1 Bomb (`alpha-block-edits.md` §8 step 1)

On the bomb's first dead step. The record colour (1, 0.2, 0.5) is built as a 3-vector and
copied as 4 floats, so its alpha is stale stack (*as read*); both are irrelevant on screen
because style 1 is recoloured. Seen: 20 fire-coloured quarter-block cubes thrown up at
20 ± 3 blocks/s.

### 5.2 Block bursts (`alpha-block-edits.md` §8 step 2, §9)

One record per removed natural block, at the removed cell's position (the integer cell
corner, so the cubes appear half a block off the cell centre, *as read*), in the block's
own colour. A bomb on flat ground removing ~1000 blocks therefore writes ~1000 records and
~3000 particles (the cap of §3.2 applies on the client).

### 5.3 Staff release (actions 30–33)

While a staff action runs, sound 36 (`channel2.wav`, pitch 1 + 0.25·*u*) plays each time
the action clock crosses a 200 ms boundary. On the step whose action time passes the
action's duration (client 0x43caa0 with −1) the burst is written at creature + aim, plus
sound 41 (`watersplash.wav`) for a water mage (specialization byte +0x141 = 1) or 38
(`fireball.wav`) otherwise, pitch and volume 1.

### 5.4 Fire explosion (actions 87, 88)

The same 200 ms sound 36 while charging; on the step where the time crosses the duration
(time − dt < duration ≤ time): 80 particles at the creature with v₀ = (0, 0, 2) and spread
**20** — a sphere-ish spray of fire cubes reaching ±20 blocks/s — and sound 81, pitch 1.
(The matching crater radius 8 is in `alpha-block-edits.md` §9.)

### 5.5 Splash (`alpha-physics-camera.md` §5.3)

The physics spec's "kind 15; 0.4" reads as count **15**, size **0.4**, style 0, spread 3.
Being style 0, the splash is world-lit (§6.3): bright at noon, dark at night.

### 5.6 Beam end (actions 28 and 95)

Actions 28 and 95 step a ray one block at a time from the attacker along its aim (client
0x6160a0 … 0x6169d0) and hit creatures on the way. After the ray, if the list examined at
0x6169d9 is empty (*believed*: no creature was hit) and the action is not 104, five small
fire cubes are written at the ray's end point moving back toward the attacker at 1 block/s
(v₀ = −1 × the per-step vector) ± 3.

### 5.7 Arrow into a block (`alpha-projectiles.md` §5.2 step 3)

As stated there; style 0, spread 3. The colour is the struck block's colour.

### 5.8 Action-28 hit (`alpha-creatures-behaviour.md` §5 step 3)

The "particle effect at the target": when hit resolution runs for an attacker whose action
is 28, before any other step, a burst at the target's position: 8 particles, size
0.1 + 0.1·*u* (one draw, at record time), v₀ = (0, 0, 10), colour (0.6, 0.6, 1) — but
style 1, so the clients show fire colours (*as read*: the pale blue is never seen).

### 5.9 Spirit bursts (`alpha-creatures-behaviour.md` §12.1)

Record fields as built by client 0x59a800 / server 0x4d2190, per weapon and per spirit of
material *m* ≥ 0x80, with ρ = combo/comboMax:

* position: the target; v₀ = (4·hx, 4·hy, 5), doubled with the area flag (h = horizontal
  hit direction, as given to the routine);
* count ⌊3ρ + 1⌋, plus 10 with the area flag; size 0.1; style 2; spread 3;
* colour from the material colour routine (client 0x4c7250) with base (1, 1, 1, 1) and
  strength *t* = 0.5·ρ. With f = 0.5·*t* + 1:

| Spirit | Colour (r, g, b, a) |
|---|---|
| 0x80 fire | (f, 0.5f, 0.1f, (1 + *t*)·f) |
| 0x81 unholy | (0.3f, f, 0.5f, (1 + *t*)·f) |
| 0x82 ice | (0.3f, 0.5f, f, (1 + *t*)·f) |
| 0x83 wind | (0.8f, 0.8f, f, (1 + *t*)·f) |

Style 2 keeps the colour (components may exceed 1) and is drawn at full lighting strength
whatever the alpha.

---

## 6. Drawing (client 0x4b7dbc … 0x4b8138)

### 6.1 Where in the frame

Inside step 12 of `alpha-rendering.md` §1.3, after the terrain chunk loop and the other
geometry that precedes it in that range, before water. The draw sets **culling off**
(render state 0x16 = 1) for the whole list and restores clockwise culling (2) after it.
Depth test stays on (less-or-equal) and the world pass's alpha blending
(source-alpha / inverse-source-alpha) stays enabled, but the output alpha is the vertex
alpha, which is 1 for models (`alpha-rendering.md` §5 step 9, §11.1), so **particles are
opaque**. The shader is whatever world shader is bound at that point (*believed* the world
pixel shader); particles are ordinary depth-tested, depth-writing cubes.

### 6.2 Geometry

Each particle is the client's **unit cube model** (G+0x800730: a 1 × 1 × 1 voxel model whose
one voxel is white (255, 255, 255), built at start-up by client 0x459c40 → 0x4e75c0,
0x4e7870), drawn with the world matrix

translate(position − render origin) · Rz(rot.z) · Ry(rot.y) · Rx(rot.x) · scale(size) ·
extra matrix · translate(−0.5, −0.5, −0.5)

(rotations about the axes in that order, each angle in degrees; client 0x4241b0). So the
cube is centred on the particle position, *size* blocks on each edge, tumbling by its
rotation. No billboards, quads or textures are used. Burst and leaf particles have an
identity extra matrix; shatter particles carry their model's rotation and scale there
(§7.2).

**Culling per particle** (client 0x47f760): skipped unless its squared distance from the
camera is within (0.9·V)², V being the frame's view distance (frame local at 0x4b0749,
*believed* the fog distance F of `alpha-rendering.md` §7.1), and it is inside the six view
planes with a margin of *size*.

### 6.3 Colour and lighting

The material colour (shader constant c32, client 0x448280) of each particle is the
componentwise product

colour × frame material × (1, 1, 1, L),

with the frame material (1, 1, 1, S) or the underwater (0.4, 0.5, 1, S)
(`alpha-rendering.md` §4) and

* **styles 0 and 4 (world-lit)**: L = max(light value of the block containing the particle
  / 255, **0.2**), the light value rule of `alpha-rendering.md` §11.2 step 1 (type 13 →
  255; air or water → max(stored value, 5); any other block → 0);
* **all other styles**: L = 1, and after the product the **alpha is forced to 1**.

The world vertex shader then lights the white cube: its sun/sky term is multiplied by the
material alpha, so world-lit particles darken at night and in caves like terrain, and
fire/spirit particles (styles 1, 2) get the full ambient + sun/back term at any hour — not
truly emissive: at midnight a style-1 cube shows about the ambient (0.6, 0.6, 0.4) times
its colour on every face. Point lights add as for any model (*believed*: the draw uploads
no lights of its own, so it uses the ten most recently uploaded). The colour's own alpha
never makes a particle transparent.

---

## 7. Client-only particle producers

### 7.1 Falling leaves (tree records of kind 60; client 0x4b1eb4 … 0x4b212f)

In the frame routine, for every zone whose centre lies horizontally within V + 32 blocks of
the camera (V as in §6.2), for every record of kind **60** in the zone's list
(`alpha-worldgen-trees.md` §5), when the world clock crosses a whole second during this
frame (⌊(clock + G+0x8006e8)/1000⌋ ≠ ⌊clock/1000⌋) and `rand()` mod 16 = 0, **one** particle:

* style **4** (no gravity, world-lit, passes wood and leaves), collides = 1,
  lifetime **20,000 ms**, age 0;
* position = the record position; rotation 0;
* velocity = (4·(*u* − 0.5), 4·(*u* − 0.5), −2): a constant drift of up to 2 blocks/s
  sideways and a steady fall of 2 blocks/s (two draws, y then x);
* spin ±5 °/s per axis (draws z, y, x; spread constant 10);
* colour = the record's three floats (L1/255 per channel), alpha 1; size **0.5**.

On average one leaf per record every 16 seconds. It falls through its own crown, lands on
the first other solid block, lies there motionless and vanishes 20 s after it was made.
Records of kind **61** make **no** particles (no list append exists near 0x4acae6).

### 7.2 Death shatter (client 0x4b3a31 … 0x4b4124, 0x470d80)

When the frame routine meets a creature with health ≤ 0 whose byte +0x1d10 is set, and the
creature passes the visibility test, it breaks each of the creature's drawn models (body,
head, hands, feet, equipment — ten calls to 0x470d80) into voxel cubes, then clears the
byte. A dead creature whose byte is clear is not drawn at all, and the byte is also cleared
for living creatures (*believed*: something raises it on the death transition; no direct
writer was found, open point AP-2).

For a model with its world transform M (the same matrix the model would be drawn with,
render-origin relative), for every voxel that is **not empty** and has **at least one empty
face neighbour** (a surface voxel; the empty test is the model's own, so key colours count
as empty for key-flag-clear models), with probability ½ (`rand()` & 1 = 0):

* style: the caller's argument (*believed* 0 for every call — world-lit);
* age := −(`rand()` mod 1000): the cube **waits** up to 0.999 s before it starts moving.
  While waiting it is drawn where it is, so the body appears to crumble over a second;
* position := M applied to the voxel's integer corner (x, y, z) (no +0.5, *as read*);
* velocity := 2 × (that position − M applied to the model's centre (dims/2)), in
  blocks/s: cubes fly outward from the model centre at twice their distance from it;
* spin ±50 °/s per axis (draws x, y, z); rotation 0;
* colour := voxel RGB/255 × the caller's tint (r, g, b), alpha = tint alpha
  (*believed*: white for most parts, the hair colour for hair-coloured parts);
* extra matrix := M with its translation removed (so each cube has the model's scale and
  orientation), size 1.0;
* collides = 1, lifetime **5000 ms**.

With gravity 30 and collision, the pieces scatter, drop and come to rest on the ground,
then all vanish 5 s after their start. If the creature is a **bomb** (type 144) the same
pass adds the explosion flash light to G+0x800748: position the bomb, colour
(1.0, 0.8, 0.5), radius 16, lifetime 800 ms.

### 7.3 Beam sparks (dormant; client 0x471d50)

The segmented-beam drawer (static fire traps and two creature beams) contains a spark
emitter: per segment, chance ⅛ per 200 ms boundary, style 2, velocity (5 − 10*u*,
5 − 10*u*, 5 + 5*u*), size (0.05 + 0.005*u*)·width, colour the beam's. All three callers
pass **0** for its enable flag, so it never runs in the shipped client. Listed for mod
authors; an implementation reproduces vanilla by not emitting.

---

## 8. Flame cubes (procedural, not particles; client 0x4bbd80, 0x4bc760)

Fire is not made of list particles. Campfires and some actions draw, every frame, a set of
*N* unit cubes whose positions, sizes and colours are pure functions of a millisecond clock
*t*. Nothing is stored and nothing collides. For flame *i* = 0 … *N* − 1 (client
0x4bbd80, arguments: base position, colours A and B, *t*, radius *r*, size *z*, count *N*):

1. f = ((*t* + 443·*i*) mod 2000) / 2000 — the flame's progress from base to tip, a 2 s
   cycle staggered by 443 ms per flame.
2. c = cos(13·*i* + 2π·(*t* mod 500)/500); q = 2f − 1;
   **edge** = (0.5·c + 1) · *z* · (1 − q²): zero at base and tip, largest halfway, pulsing
   at 2 Hz.
3. **colour** = A·(1 − f) + B·f (material colour, uploaded as given — no light factor, no
   alpha forcing).
4. angle a = 2π·(*i*/10 + (*t* mod 10000)·0.0001); **position** = base +
   (*r*·cos a, *r*·sin a, 2·*r*·f): flames sit on a circle of radius *r* turning once per
   10 s and rise to 2*r* above the base.
5. The cube is turned by f·40°, f·30° and f·10° about three axes, scaled by the edge,
   centred, and drawn with the unit cube model.

Callers:

| Caller | What | *N* | *r* | *z* | A (base) | B (tip) |
|---|---|---|---|---|---|---|
| 0x4b325c | campfire, static kind 65 (`alpha-interaction.md`), base = position + (0, 0, scale.z/2) | 20 | 0.8 | 0.2 | (1, 0, 0) red | (1, 1, 0) yellow |
| 0x4b4676 | creature in action 34 (healing stream) while time < duration, base = drawn position, *t* = action time | 20 | box width | 0.1 | (0, 0, 1) | (0.25, 0.75, 1) |
| 0x4b705c | creature in action 87/88 (fire explosion) while charging | 20 | *believed* as above | | (1, 0, 0) | (1, 1, 0) |
| 0x4b55b5 | an unidentified object with type byte 4 (*believed* a world item) | 8 | | 0.1 | (0, 1, 0) | (0.5, 1, 0) |
| 0x4b4430, 0x4b4549 via 0x4bc760 | staff charge, actions 30–33 (second call 31/33 only); water mage (0, 0.1, 1)→(0.5, 0.7, 1), else red→yellow | *believed* | | | | |

Worked value (campfire, flame 0 at *t* = 1000): f = 0.5, q = 0, c = cos(0 + 2π·0) = 1, edge
= 1.5 × 0.2 = 0.3 blocks, colour (1, 0.5, 0), a = 2π·0.1 → offset (0.647, 0.470, 0.8).

There are **no** particles for torches (kind 51, light only), fire-ground blocks (type 12),
footsteps, landing, level-up, item pickups, eating or drinking (those "effects" of
`alpha-item-names.md` are sound records, client 0x4864c0), mana, or the critical-hit
flash (a timed light only, `alpha-materials-wildlife-routes.md` §1.5).

---

## 9. Server and client

* **The server** never simulates or draws particles. It writes the bursts of §5 for what
  it simulates (NPCs, bombs, NPC projectiles, NPC hits and craters) into section 3 and
  sends each record to every player within 200 blocks of its position.
* **Each client** expands every burst it handles into its own particles with its own
  random draws, so two players see different spreads, spins and fire colours for the same
  burst. Bursts produced by a client's own step (its own player's actions, everything when
  offline) are expanded locally and are **not** uploaded — there is no particle packet
  from client to server.
* **Purely local**: falling leaves (§7.1), death shatter (§7.2), flame cubes (§8). They
  depend on the local clock, local `rand()` and the local view distance.

An implementation that keeps wire compatibility must send and accept the 72-byte record
exactly as in §1 (padding included) and must not expect particle state from peers.

---

## 10. Worked examples

1. **Bomb burst seen by a client.** Record: P = bomb, v₀ = (0, 0, 20), size 0.25, *n* 20,
   style 1, spread 3. One particle with draws z 0.9, y 0.2, x 0.5 gets velocity
   (0, −1.8, 22.4); spin draws give e.g. (−10, 30, 5) °/s; colour (1, *u*, 0). At 60 fps
   (dt 16 ms, *s* 0.016): v.z drops by 0.48 per frame; apex after 22.4/30 = 0.747 s,
   22.4²/60 = 8.36 blocks up; no collision, so it falls through the ground and is removed
   on the first frame its age exceeds 4000 ms, about 89.6 − 240 = 150 blocks below P.
   Rotation advances by 2 × spin × *s* per frame (free flight).
2. **Fire spirit, combo ρ = 0.5, no area, hit direction (1, 0).** *n* = ⌊2.5⌋ = 2;
   *t* = 0.25, f = 1.125: colour (1.125, 0.5625, 0.1125, 1.40625); v₀ = (4, 0, 5), each
   particle ± 3; size 0.1; style 2, so drawn at full lighting with the colour as given.
3. **Splash at 22:00 in the open.** Style 0: S at 22:00 is small (x = 0.833, S ≈ 0.04);
   L = 1 in open air (light 255) → material alpha 0.04: the 15 blue cubes are almost
   black apart from point lights. At noon S = 1 and they show (0.2, 0.7, 1) × the sun term.
4. **Leaf from an autumn tree.** Record colour (0.8, 0.4, 0.1); a draw sequence giving
   *u* = 0.75 (y), 0.25 (x): velocity (−1, 1, −2). It passes through the crown (type 8),
   meets grass 14 blocks below after 7 s, sits there until age 20,000 ms, then vanishes.
5. **Collision sub-steps.** A shatter cube with velocity (3, 0, −40) at dt 20: d = (0.06,
   0, −0.8), |d| = 0.802 → *m* = 1, e = d. Moving x is free; moving z enters grass → undo,
   velocity := 0, spin := 0. Next frame v.z = −0.6, d.z = −0.012: undone again. The cube
   rests.
6. **Cap.** A bomb removing 1000 blocks plus its own burst: 3020 particles; two such bombs
   in one frame: 6040 → after the update's cap the 1040 newest are gone.

---

## 11. Corrections to other specs

* `alpha-server-simulation.md` §11 / **SS-7**: the record is now fully decoded (§1): 0x34
  is the size, 0x3c the style, 0x40 the velocity spread (3.0 default), 0x44 padding.
* `alpha-physics-camera.md` §5.3: the splash's "kind 15" is the **count** 15; "0.4" is the
  size; the style is 0.
* `alpha-pets-bombs.md` §3.3: the (1.0, 0.8, 0.5) radius-16, 800 ms light is added when the
  dead bomb **shatters** (§7.2), once, not on every frame the bomb is drawn — so a bomb
  does not glow while its fuse burns; the light is the explosion flash. Its bomb burst
  colour (1, 0.2, 0.5) (`alpha-block-edits.md` **E-5**) is never shown: style 1 recolours.
* `alpha-creatures-behaviour.md` §5 step 3: the action-28 effect is §5.8 here (a burst,
  not a follow-up of hit resolution).
* `alpha-worldgen-trees.md` open question 3: kind 60 = falling leaves (§7.1); kind 61
  makes no particles.
* `alpha-item-names.md` (eating, 0x618b1e … 0x618b90): the spawned "effects" are sound
  records, not particles.

---

## 12. Open points

* **AP-1** Which step outputs the burst consumer walks (§2.1): the local step's set, the
  received world update, or both; and whether an online client therefore expands some
  bursts twice (once from its own step, once from the server). *Game check*: online, hit
  an NPC with a special-spirit weapon at a high combo and count the cubes per hit (2 vs 4
  at ρ = 0.5) with a mod-DLL log of the list count G+0x800744 before/after.
* **AP-2** The writer of the shatter byte +0x1d10 (§7.2). *Game check*: offline, kill a
  goblin and watch it crumble; breakpoint on writes to creature +0x1d10.
* **AP-3** The trigger of the beam-end burst (§5.6). *Game check*: a monster using action
  28 or 95 (beam) that misses versus hits; look for 5 fire sparks at the beam's end.
* **AP-4** The draw distance V and the frame-time source G+0x8006e8 (§0, §6.2, §7.1).
  *Game check*: log both in a frame; count leaves near an autumn tree over 160 s (≈ 10 per
  record expected).
* **AP-5** Shader constants not set by the particle draw (white/hit flash, point lights):
  whether a creature's hit flash or the last chunk's lights leak onto particles. *Game
  check*: screenshot fire particles next to a torch at night and next to a flashing
  creature.
* **AP-6** The callers of §8 marked *believed* (0x4b55b5, 0x4b705c, 0x4bc760).
  *Game check*: record a fire-explosion charge, a staff charge and a dropped item, and
  compare flame counts and colours.
* **AP-7** The style passed by each shatter call and the per-part tint (§7.2). *Game
  check*: kill a creature at midnight; world-lit (style 0) shatter cubes should be dark.
* **AP-8** Whether the bursts of an online client's own player (staff release, splash,
  arrow) are ever seen by other players (no upload exists; the server would have to make
  them itself). *Game check*: two clients, one jumps into water near the other.
