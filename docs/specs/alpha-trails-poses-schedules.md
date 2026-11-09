# Alpha weapon trails, the remaining pose cells, town schedules and seats

Behavioural specification of four pieces of Cube World **Alpha** that other specs left
open:

1. the **weapon trails** — how the stored blade points of `alpha-creatures-models.md` §6.5
   are aged and drawn, including the arc of pose 69 (follow-up **F-5** there);
2. the **pose cells marked "n.t."** in Appendix C of `alpha-creatures-models.md`
   (remainder of follow-up **F-2**), with corrections to a few phase boundaries;
3. **SpawnLocation** — the time-of-day schedule of town residents
   (`alpha-creatures-behaviour.md` §13.5, §9.2 step 1; open point **A-2** there);
4. **RandomInteraction** — residents (and one group of zone creatures) walking to a stool,
   bench, beach towel or bed and using it (also **A-2**).

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/client` and
`…/server`), the per-instruction listings, and fresh disassembly of `Cube.exe` and
`Server.exe` where the C output was missing or lossy (the pose cases of client 0x4128f0 and
the frame routine client 0x4ac260 were read instruction by instruction; constants were
read from the executables). No code was copied. Routines are cited as "client 0x…" or
"server 0x…"; offsets are for cross-checking only. Readings marked *believed* rest on
partial evidence; *as read* marks behaviour that looks unintended but is what the code
does.

This document **adds to** and does not repeat:

* `alpha-creatures-models.md` — §6.5 (equipment matrices, the stored blade points), §7.7
  (channels, smoothing, per-frame order), §7.8 (phases W, A, R, E, STANCE, grip G),
  Appendix C (pose cases); §2 here fills its "n.t." cells;
* `alpha-rendering.md` — §1 (frame order and the world-pass blend state), §11.2 (creature
  drawing);
* `alpha-creatures-behaviour.md` — §3.3 (phase lengths P1, P2, P3, D), §9 (spawn objects,
  activation), §13 (behaviour layer); `alpha-materials-wildlife-routes.md` §4 (behaviour
  classes, "handled" results, WalkPath, RandomWalk);
* `alpha-path-search.md` — §1 (line of sight), §3 (search callers; the SpawnLocation and
  RandomInteraction rows are completed here), §4 (the search), §6 (the follower);
* `alpha-interaction.md` — §4 (static objects, kinds, cell index), §6.1 step 4 (seats and
  beds), §6.4 (occupant release);
* `alpha-physics-camera.md` §2 step 3 (intent ends sitting);
* `alpha-worldgen-structures.md` §5.5.4 step 16 (house seat and bed lists), §5.5.9 (town
  NPCs, the day schedule).

---

## 0. Conventions

* As in `alpha-creatures-models.md` §0 and `alpha-path-search.md` §0: positions are 3 × i64
  in 1/65536 block; "obj +x" is a creature-object offset (record offset + 0x10); "pose
  state +x" is an offset in the per-creature pose state at obj +0x1498.
* *t* = the copied action time (ms), P1, P2, P3 the phase lengths and D = P1 + P2 + P3 of
  the current action (`alpha-creatures-behaviour.md` §3.3, attack-speed factors included).
* **Time of day** τ = the world's integer at W +0x80015c, milliseconds, 0 … 86 399 999
  (`alpha-rendering.md` §1). "hh:mm" in examples is τ written as a clock.
* "Relax(*x* → 1, *r*, dt)" = start at 0 and repeat dt times *x* ← *x* + (1 − *x*)·*r*; the
  result is *w* = 1 − (1 − *r*)^dt (client 0x411d40, as `alpha-creatures-models.md` §7.7).
* Behaviour "handled" results as `alpha-materials-wildlife-routes.md` §0 and §4.2: a
  sequence stops at the first behaviour that returns handled.
* Device states are named by their Direct3D 9 meaning (the original calls the device's
  SetRenderState, SetTexture, SetFVF, SetTransform and DrawPrimitiveUP); AlphaVulkan maps
  them to pipeline state.

---

## 1. Weapon trails (resolves F-5 of `alpha-creatures-models.md`)

### 1.1 Storage

The pose state holds four **point histories** of 16 points each (3 × f32 per point,
point 0 is the newest):

| Pose state | Object | Hand | Point | Weapon-model point stored as point 0 |
|---|---|---|---|---|
| +0x56c | +0x1a04 | main | **tip** | (*sx*/2, *sy*/2, *sz*) |
| +0x62c | +0x1ac4 | main | **butt** | (*sx*/2, *sy*/2, 0) |
| +0x6ec | +0x1b84 | off | tip | (*sx*/2, *sy*/2, *sz*) |
| +0x7ac | +0x1c44 | off | butt | (*sx*/2, *sy*/2, 0) |

(*sx*, *sy*, *sz*) are the voxel sizes of the weapon model actually drawn in that hand.
Point 0 is written after each in-hand or sheathed draw of that weapon (models §6.5; client
0x421a9a, 0x421b18 for the main hand, 0x4223b4, 0x422432 for the off hand) by transforming
the model point with the weapon's full part matrix (the base translation of models §7.4
step 7 included, so x and y are relative to the camera, z is absolute). The z = 0 point
(models §6.5 "bottom") is the **butt** of the weapon (pommel end; the hand holds voxel
z = 4), the z = *sz* point ("top") is the **tip**.

**Frame of the history.** Right after the draws, every frame, the four point-0 entries are
shifted by (camera − displayed position) in x and y (z unchanged; client 0x4225fd…0x4226f1),
so the whole history is kept **relative to the creature's displayed position in x and y**
(not rotated with it) and absolute in z. When the trail is drawn the creature's current
(displayed position − camera) is added back (§1.3). Consequences: a running creature carries
its trail along horizontally (the trail does not stream out behind it), while a jumping or
falling one leaves the trail at the heights where it was made.

### 1.2 Ageing the history (client 0x4226f6…0x422fe5, once per call of the pose routine)

The pose routine (client 0x4128f0) receives the frame time dt as its fifth argument. The
main creature pass (client 0x4ba8fe) passes the frame's dt (G +0x8006e8); its other
callers pass 0, so they re-run the rules below with weight 0. One rule applies per call,
chosen by the pose id (models §7.2) in this order:

| # | Poses | Window (else falls to the next row) | Point 0 adjustments | Points 1 … 15 |
|---|---|---|---|---|
| 1 | **69** | P1 < *t* < P1 + P2 | all 16 points of both main-hand histories are **overwritten** by the arc of §1.2.1 | (overwritten) |
| 2 | 1, 2, 3, 8, 10, 12, 14, 28, 29, 30, 31, 32, 35, 48, 49 | P1 < *t* < D | none | **chain** (below) on all four histories, rate 0.05 |
| 3 | 15, 17, 19 | *t* > P1 (no upper limit) | none | off-hand histories: chain, rate 0.05; both main-hand histories: every point := main **butt** point 0 |
| 4 | 16, 18, 20, 50 | P1 < *t* < P1 + P2 + P3/2 | pose 50: main tip point 0 := base frame · (20, 0, −12), main butt point 0 := base frame · (8, 0, −12) (model units, base frame of models §7.4 step 7); other poses: main butt point 0 := midpoint of main butt and main tip | main-hand histories: chain, rate **0.075**; both off-hand histories: every point := off **butt** point 0 |
| 5 | anything else | — | none | **collapse**: in each history every point := its point 0 |

**Chain.** With *w* = Relax(→ 1, rate, dt) = 1 − (1 − rate)^dt, for *i* = 1 … 15 in
increasing order, in each history: point *i* ← point *i* + (point *i*−1 − point *i*)·*w*.
The order matters: point *i* uses the already-updated point *i*−1 (a lagging chain). The
four histories are processed in the order main tip, main butt, off tip, off butt.

"Collapse" makes the strip of §1.3 degenerate (zero area, invisible); this is how a trail
is **cleared**: at the end of the window, outside trail poses, and whenever the window has
not opened yet (W phase). There is no other reset; a history is not cleared when the
creature changes weapons or when it is not drawn (the pose routine then does not run for
it).

Pose 50 (actions 20, 21: the fist off-hand ability, a spinning strike, models Appendix C)
traces two fixed points 8 and 20 model units to the creature's right and 12 below the
pivot instead of a weapon (*inferred*: a foot or fist sweep). In rule 4 for the other
poses only the outer half of the weapon (midpoint to tip) leaves a trail.

#### 1.2.1 The arc of pose 69 (action 104, the shield charge release)

While P1 < *t* < P1 + P2 (for action 104: P1 = 0, P2 = 300 / (*S*·*a*)):

* *f* = 20·(*t* − P1)/(P1 + P2) blocks (constant 20.0 at client 0x6fcd74);
* M = T(displayed position − camera in x, y; displayed z + lean *E*.z − landing bob, the
  same as the base translation of models §7.4 step 7) · R*z*(displayed rotation z) — no
  model scale, so lengths are **blocks**;
* for *i* = 0 … 15: *a*ᵢ = *i*·π/16 − π/2 (radians, double precision then rounded),
  *u*ᵢ = (sin *a*ᵢ, cos *a*ᵢ, 0);
* main **tip** point *i* := M·(*f*·*u*ᵢ) + (camera − displayed position in x, y, 0);
* main **butt** point *i* := M·(0.5·*f*·*u*ᵢ) + the same shift.

So the 16 points run over **half a circle**, from the creature's left (−x) through its front
(+y) to 11.25° short of its right, at the height of its centre. The strip of §1.3 between
the two rings is a half-annulus whose outer radius grows linearly from 0 to 20·P2/(P1 + P2)
blocks (20 blocks for action 104) and whose inner radius is half of it — an expanding
frontal shock wave (*inferred*). After P1 + P2 rule 5 collapses it. (The weapon points of
point 0 are ignored during the arc.)

### 1.3 Drawing (client 0x4b9a10…0x4ba13e, in the frame routine)

**When.** Inside step 12 of the frame (`alpha-rendering.md` §1), after the terrain and the
other effect draws and **before the water** (client 0x4ba13e). The creature pass that ages
the histories (step 14, client 0x4ba5d0) comes later in the same frame, so the trail drawn
in a frame shows the history aged in the **previous** frame (*as read*; one frame of lag).

**Device state** (set once before the loop, client 0x4b8bb9…0x4b8c64 and 0x4b9980…0x4b99d3):
fixed-function pipeline (no vertex or pixel shader), **depth writes off** (render state 14
= 0), depth test as left by the frame (on, less-or-equal), **culling off** (state 22 = 1),
**lighting off** (state 137 = 0), **no texture** (stage 0 texture null), vertex format
position (3 × f32) + diffuse colour (one 32-bit ARGB word), stride 16 bytes; view =
the frame's view matrix, world = identity. Blending is not set here: the world pass state
(**alpha blending on, source alpha / inverse source alpha**, `alpha-rendering.md` §1 step
10) is still in force (*believed*: no state change found between step 10 and here). No fog
state is set (*believed* off for this pass). After the trails: depth test on, depth writes
on, culling state reset (client 0x4ba162…0x4ba199), then the water.

**Which creatures.** Every creature of the world's creature list for which all hold:

1. health (obj +0x16c) > 0;
2. it passes the frame's creature visibility test (client 0x47f760, *believed* the view
   test used for drawing creatures);
3. it is **not a ranged user** (client 0x444520, `alpha-creatures-behaviour.md` §13.2 step
   9: class 3, types 117, 86, 104, or a staff, wand, bracelet, bow, crossbow or boomerang
   equipped) — so mages and archers never show trails;
4. *t* ≤ D (the action has not run past its duration).

Then the **main-hand strip** is always drawn (it is invisible while collapsed), and the
**off-hand strip** only if the off-hand slot item's type (obj +0x990) is 3 (a weapon).

**Geometry.** One triangle strip per hand, 32 vertices, 30 triangles: for *i* = 0 … 15 the
pair (butt point *i*, tip point *i*), each plus (displayed position − camera) in x and y
(the inverse of the shift of §1.1). Positions are therefore camera-relative blocks, as all
world geometry of the frame.

**Colour.** Every vertex is **white** (r = g = b = 255) — see the note below. Alpha per
vertex pair, with *x* = *i*/15 and *k*ᵢ = (1 − (1 − 2*x*)²)² (= (4*x*(1 − *x*))², zero at
both ends, peak at the middle of the history):

| *i* | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| *k*ᵢ | 0 | 0.0619 | 0.2136 | 0.4096 | 0.6119 | 0.7901 | 0.9216 | 0.9911 | 0.9911 | 0.9216 | 0.7901 | 0.6119 | 0.4096 | 0.2136 | 0.0619 | 0 |
| butt vertex alpha byte = trunc(50·*k*ᵢ) | 0 | 3 | 10 | 20 | 30 | 39 | 46 | 49 | 49 | 46 | 39 | 30 | 20 | 10 | 3 | 0 |
| main tip alpha = trunc(200·*k*ᵢ) | 0 | 12 | 42 | 81 | 122 | 158 | 184 | 198 | 198 | 184 | 158 | 122 | 81 | 42 | 12 | 0 |
| off tip alpha = trunc(255·*k*ᵢ) | 0 | 15 | 54 | 104 | 156 | 201 | 235 | 252 | 252 | 235 | 201 | 156 | 104 | 54 | 15 | 0 |

The trail is thus invisible at the blade itself (point 0) and at its oldest end, strongest
a third to two thirds of the way back, and much more opaque along the tip edge than along
the butt edge (the off hand slightly more than the main hand). In the arc of pose 69 the
inner ring takes the butt alphas and the outer ring the main-tip alphas.

**The combo tint is computed and discarded** (*as read*). The routine first builds two
colours from *c* = min(0.1 × combo counter (record field 11), 1): butt colour = (1 − *c*)·(100,
255, 255) + *c*·(200, 100, 255), tip colour = (1 − *c*)·(255, 255, 255) + *c*·(255, 50, 100);
then it overwrites both with (255, 255, 255) before reading them. The visible trail is
white at every combo count. (Open point T-1.)

### 1.4 What a trail depends on

| Factor | Effect |
|---|---|
| Pose (action) | which rule of §1.2 and the time window |
| Weapon model | its length *sz* and the weapon scale *w* set the strip width (point positions); a missing off-hand weapon (type ≠ 3) hides the off-hand strip |
| Weapon class | ranged users (and every class-3 creature) get none (§1.3 item 3) |
| Material, spirits, rarity, shimmer | **none** (the shimmer and the spirit lights of `alpha-materials-wildlife-routes.md` §1 are separate) |
| Combo counter | none in effect (§1.3, discarded tint) |
| Frame time | the chain weight *w* is per frame, so the trail length in time is roughly constant: at 60 fps (dt 16) *w* = 0.560 (rate 0.05) or 0.713 (rate 0.075) |
| Attack speed | through P1, P2, P3 (window lengths) only |

### 1.5 Worked examples

* **Sword swing, action 1 (pose 16), *S*·*a* = 1.** P1 = 300, P2 = 200, P3 = 500, D = 1000
  (behaviour §3.3). Rule 4 runs for 300 < *t* < 300 + 200 + 250 = 750: the main butt
  point 0 is the blade midpoint, the chain rate 0.075 gives *w* = 1 − 0.925¹⁶ = 0.7127 at
  dt 16. A point that moved 1 block between frames is followed by point 1 at 0.71 of the
  way, point 2 at 0.71 of the remaining gap to point 1, and so on: the 16 points span
  roughly the last 10–15 frames of blade motion. At *t* = 750 rule 5 collapses the strip;
  between 750 and 1000 it is still drawn but has zero area.
* **Blade points.** A sword model 3 × 3 × 16 voxels with weapon scale 1: butt point =
  matrix · (1.5, 1.5, 0), tip point = matrix · (1.5, 1.5, 16); the grip translation of
  models §6.5 puts the hand at voxel z = 4, so the strip of rule 4 runs from 4 voxels past
  the hand (midpoint z = 8) to the tip.
* **Pose 69 arc, action 104, *S*·*a* = 1, *t* = 150.** *f* = 20·150/300 = 10. Point 8:
  *a* = 0, *u* = (0, 1, 0): outer (tip) point = 10 blocks in front of the creature, inner
  (butt) point 5 blocks in front, at its centre height; alphas 198 and 49 (/255). Point 0:
  *a* = −90°, *u* = (−1, 0, 0): 10 and 5 blocks to its left, alpha 0. At *t* = 300 the
  outer radius would reach 20 blocks; the arc collapses from then on.
* **Alpha check.** *i* = 4: *x* = 0.2667, 1 − 2*x* = 0.4667, *k* = (1 − 0.2178)² = 0.6119;
  bytes 30, 122, 156.

---

## 2. The remaining pose cells (F-2 of `alpha-creatures-models.md`)

Notation and phase names as Appendix C there. Each row gives the full value for a cell that
was "n.t."; ":=" and "+=" as there. Read from the case bodies (client addresses in
Appendix C) with the channel targets identified from the smoothing code (client
0x41ea05…0x41ef76) and the sitting block (client 0x413959…0x413a12).

### 2.1 Resolved cells

| Pose | Cell | Value |
|---|---|---|
| 1 | 500 ≤ *t* < D, *B*.x | *B*.x := −60·(1 − *g*), *g* = min(2·(*t* − 500)/500, 1): the body tilt eases back from −60 to 0 within 250 ms |
| 3 | W, Δ*R* = Δ*L* | (−5, 8, 0) |
| 3 | phase "A" | runs only for P1 ≤ *t* < P1 + P2/2 (not to P1 + P2), and there Δ*R* = Δ*L* := (−8, −4, 20) (not "as W"); the "R" values hold for P1 + P2/2 ≤ *t* < D |
| 4 | A..D, *q*₂ | *q*₂ = min(1.5·(*t* − P1/2)/(P1/2), 1); for every *t* ≥ P1 this is 1 (*as read*), so ρ*R* = (30, 0, 0) and Δ*R* = (−6, 0, 16) throughout; *r* = 0.005 (class 1 spec 1: 0.01) |
| 4 | Δ*R*.z factor | 16·*q*₂ |
| 7 | *U* | *t* < D/2: *U* := record field 25 (offset 0x144), *r* = 0.005. D/2 ≤ *t* < D: *k* = min(15·((*t* − D/2)/(D/2))³, 1), *U* := *k*·field 24 + (1 − *k*)·field 25, *r* = 0.03. *t* ≥ D: *U* := field 24 (offset 0x138) |
| 10 | A..D sine term | *L*.x (left-hand base) += sin(0.1·(*t* − 600)); *r* = 0.03 |
| 18 | A..D, ρ*L* | (0, 0, 40) if the off-hand subtype is 13 (shield), else (−40, 0, −30) |
| 18 | A..D, Δ*R* | (3, −1, 0) |
| 19 | otherwise, the *S* sine | *S*.z += 0.5·sin(0.005·*t*) |
| 21 | the two limits | the first branch holds for *t* < **2·D** (*as read*: the second branch, "*t* ≥ D" in Appendix C, is reached only if the action outlives twice its duration); the inner "second limit" is *t* < **D/2** |
| 23 | W time term | Δ*R*.x and Δ*L*.x += 0.5·sin(0.04·*t*); Δ*R*.z and Δ*L*.z += 0.5·cos(0.05·*t*) (the same wobble in poses 24, 25, 26 W, 27) |
| 30 | A, ρ*R* and Δ*R* blends | ρ*R* := *q*₂·(−110, 60, 0) + (1 − *q*₂)·(−90, 90, 48); Δ*R* := *q*₂·(−12, 0, −1) + (1 − *q*₂)·(−10, 8, 8); *E*.x := −0.8*q*₂ |
| 31 | A blends | ρ*R* := *q*₂·(0, −90, −130) + (1 − *q*₂)·(0, −90, 20); Δ*R* := *q*₂·(0, 2, 0) + (1 − *q*₂)·(−8, 6, 6); *E*.x := 0.8*q*₂; W also sets *E*.x := 0 |
| 32 | A blends | ρ*R* := *q*₂·(−90, 0, −45) + (1 − *q*₂)·(90, 0, 45); Δ*R* := *q*₂·(0, 9, 0) + (1 − *q*₂)·(2, 2, 14); *r* = 0.05 (W: *r* = 0.04) |
| 30–33 | phase of the "R.." row | the blend branch holds for P1 ≤ *t* < **D** (the test is P1 + P2 + P3); the "R.." values (Δ*R* := (2, 0, 0), ρ*R* := 0, *T*.z := −30, *r* = 0.03) apply only for *t* ≥ D (phase E) |
| 38 | W, *K*.x | *K* := (−3, −2, 0) |
| 38, 39 | phase split | "W" is *t* < P1 + **P2/2** and "after W" is P1 + P2/2 ≤ *t* < D; nothing is set for *t* ≥ D |
| 39 | after W, *Q* | *Q* := (−10, 0, 0) |
| 42 | after W, *Q* | *Q* := (−20, 0, 0) (not "as 39") |
| 43 | W, *Q* | *Q* := (−50, 0, 0) (not "as 39") |
| 44, 73 | thresholds | step 1: *t* < P1; step 2: *t* < P1 + P2/3; step 3: *t* < P1 + 2·P2/3; step 4: *t* < P1 + P2; step 5: *t* < D. Step 2 *Q* := (−30, 0, 0) |
| 45 | R, *K*.x | *K* := (0, −2, 0) |
| 54 | foot base *z* | both foot bases z += 3 (the constant 3.0 carried in a register from client 0x41391b, *as read*) |
| 56 | ρ*R* | (−10, 10, −90) |
| 63 | the sign/parity test | the case runs while *t* < P1 + P2 + **400**. With *k*ₙ = ⌊(*t* − P1 + 100)/*n*⌋: if *t* ≤ P1 or *k*₅₀₀ is odd, the "A" set (Δ*L* := (2, −2, 12), Δ*R* := (−2, −2, 12), *T*.x := 20, *B*.x := 10, default *r*); else if *k*₁₀₀₀ is odd, the left set (Δ*L* := (2, −2, 12), Δ*R* := (0, 0, −5), *T* := (−10, 0, 20), *B* := (−5, 0, 10)); else the right set (mirror); *r* = 0.03 for the two single-hand sets. So "W" and "A" of Appendix C are not phases but alternating 500 ms windows: right, both, left, both, right, … |
| 65 | the sine argument | *T*.z := 90·sin(0.03·*t*), while *t* < D (read from the record's action time); *r* = 0.04 |
| 66 | "off-hand model replaced" | the off hand is given the **main-hand weapon model** (a second boomerang is shown); in A the off hand keeps that copy and the main hand loses its model; R removes both |
| 67 | cos argument; replacement | *R*.z and *L*.z += 0.5·cos(0.5·*t*); if record field 23 > 0 the off hand is given the main-hand weapon model |
| 69 | *B*.z fraction | *B*.z := −360·*t*/(P1 + P2) |
| 71 | R.., Δ*R*.z | Δ*R* := (2, −1, 0) |

### 2.2 Corrections to the text of models §7.8 and Appendix C

* **STANCE exception.** The test that skips the arm pose of STANCE reads the creature
  **type** (record field 8, obj +0x64), not the action: types **87** (mole) and **96**
  (zombie) keep their arms down. (Client 0x4199a7, 0x41ba2f, 0x41d341, 0x41d5b1, 0x41ddeb,
  0x41e338.)
* The phase boundaries listed in §2.1 for poses 3, 30–33, 38, 39, 44/73, 63 replace those of
  Appendix C.

### 2.3 Cells still not transcribed

* Pose 21: besides the limits, the sub-branch for *t* < D/2 also lowers one previously
  selected component by 2 and sets two rotation components to −50 and 70 (*believed*
  ρ*L*.z and ρ*R*.z); the components were not identified.
* Pose 37: the left-arm base taken "from the unmodified arm" before G((2, 0, 0)).

---

## 3. SpawnLocation (server 0x428940, client 0x4dfad0)

### 3.1 Data

| Where | Content |
|---|---|
| spawn object +0x10a0 | the **schedule**: a list of 32-byte entries (point 3 × i64, time τ as i32, 4 unused bytes), written by the town builder (`alpha-worldgen-structures.md` §5.5.9 step 4); copied to the creature at activation (`alpha-creatures-behaviour.md` §9.2 step 7) |
| creature obj +0x148c / +0x1490 | the creature's copy of the schedule (begin, end) |
| obj +0x1488 | **current index** *c* (starts 0) |
| obj +0x1484 | **arrived index** *a* (starts 0) |
| obj +0x1c0 | **home** (record +0x1b0), the point RandomWalk leashes to |
| obj +0x1428, +0x1440, +0x1458 | path start, goal, arrival radius (`alpha-path-search.md` §0.1) |

The behaviour object itself (`cube::SpawnLocationBehavior`, constructor client 0x4dfab0)
has no fields; all state is in the creature, so it survives the cloning of §13.1 of the
behaviour spec.

### 3.2 Which entry is current

**Current entry** for τ = the **last entry in list order whose time ≤ τ**; if none, entry 0.
Ties (equal times) go to the later entry. Times are compared as signed integers; nothing
wraps, so an entry whose time is 86 400 000 or more (later than midnight) **never becomes
current** (*as read*). Every town schedule starts with (own position, 0), so from midnight
until the first timed entry the resident's place is home, and at midnight any later entry
gives way to entry 0 — that is the wrap.

### 3.3 One update (the behaviour's update; dt is not used)

1. **Empty schedule** → not handled.
2. Find the current index *n* (§3.2).
3. **New entry** (*n* ≠ *c*):
   1. Drop the path and search state (server 0x405330); *c* := *n*.
   2. Path start := own position + (0, 0, 0.5 − *h*/2) blocks (feet + 0.5; the z offset is
      converted to fixed point by truncation); goal *G* := entry point.
   3. **Find standable ground** by single **block** tests (not the creature's box): the
      block containing *G* (block type = low 5 bits, `alpha-path-search.md` §0):
      * **raise**: up to 11 tests; while the block at *G* is solid (neither empty nor
        water), *G*.z += 1 block. If it is still solid after 11 tests → **not handled**
        (nothing started);
      * **lower**: then up to 11 tests of the block **below** *G*: if it is solid, ground
        is found; else *G*.z −= 1 block. If no solid block is found in 11 tests → **not
        handled**.

      Only whole blocks are added or removed, so *G* keeps the fractional part of the
      entry point. Water counts as "not solid" in both tests: a point over water sinks to
      the sea or lake bed.
   4. Ground found: start the search (`alpha-path-search.md` §4.1) with goal *G* and
      arrival radius **r = 4**, run **one** round and rebuild the path once, intent := 0;
      **handled**.
4. **Same entry, already arrived** (*n* = *c* and *a* = *c*) → **not handled**.
5. **Same entry, on the way** (*a* ≠ *c*):
   1. The nearest creature with controller kind 0 (a player) is looked up (squared
      distance in blocks, starting from 40 000) and compared with 65 536; since the minimum
      can never exceed 40 000 the comparison never skips anything (*as read*: a dead
      "player nearby" gate; the walk proceeds whether or not a player is near).
   2. **Search rounds**, only if the node map is not empty: up to 10 rounds, each preceded
      by the stop tests of `alpha-path-search.md` §3 (path longer than 50 cells, or its last
      cell within r of the goal cell: squared cell distance < r² = 16).
   3. **Arrival test**: arrived if the path is **empty**, or if its last cell **is** the
      goal cell and the path has **at most 2** cells. Otherwise → **handled** (walking;
      the follower of `alpha-path-search.md` §6 moves the creature).
   4. **Arrived**: drop the path and search state, intent := 0, *a* := *c*, **home := the
      entry point** (the raw point, not *G*) → **handled** (this tick). From the next
      update step 4 returns not handled.

There is **no waiting and no facing** on arrival: SpawnLocation only stops reporting
handled, and the later behaviours of the sequence (RandomInteraction, then RandomWalk with
the new home) take over. While walking, the follower makes the creature look along the
path (`alpha-path-search.md` §6.1).

The path is usually emptied by the follower itself (it drops path and state when the feet
cell is within r = 4 cells of the goal, path-search §6.2), so "arrival" is in practice
"within 4 cells of the ground point". An **abandoned search** (open set ≥ 501 or node map
≥ 20 001, path-search §4.2) and anything else that empties the path also count as arrival
(*as read*); so does LookAtPlayer clearing the path (§3.6).

### 3.4 Return values

| Situation | Result |
|---|---|
| no schedule | not handled |
| new entry, ground found, search started | handled |
| new entry, no ground within 11 blocks | not handled (and next update arrives at once, §3.6) |
| walking | handled |
| the arrival tick | handled |
| arrived, entry unchanged | not handled |

### 3.5 The spawn object follows the schedule (client 0x60df20…0x60df93; behaviour §9.2 step 1)

Every world step, for each spawn object of each loaded zone, before its countdown is
decremented: for every schedule entry in list order, if τ ≥ the entry's time, the spawn
object's position := the entry point (raw, no ground fit). The result is the position of the
current entry of §3.2. Since activation creates the creature at the spawn position
(behaviour §9.2 step 7) and the activation radius is measured from it, a resident appears
**at the place its schedule says it should be now**, with *c* = *a* = 0; on its first update
the current index differs from 0 (unless it is night), a new search starts from where it
stands to the ground under the same point, and it "arrives" within an update or two. The
server's world step is *believed* to do the same (only the client routine was read).

### 3.6 Which town creatures run what

From the town builder (client 0x5af170; constructors: sequence 0x4dc2d0, Combat
0x42c550(20), LookAtPlayer 0x4c8130, WalkPath 0x58cfa0(radius), SpawnLocation 0x4dfab0,
RandomInteraction 0x4db100, RandomWalk 0x4dbf10; the point list of a WalkPath gets the
creature's own spawn position through 0x5bf4f0):

| Creature (structures §5.5.9) | Class byte +0x30 | Behaviour sequence |
|---|---|---|
| **Resident** (sub-0 houses) | 0x88 | Combat 20, LookAtPlayer, **SpawnLocation**, **RandomInteraction**, RandomWalk |
| **Shopkeeper** (subs 2–5) | 0x80–0x83 | Combat 20, LookAtPlayer, **WalkPath (r = 2, one point: its own spawn point)** |
| **Innkeeper** (sub 1) | 0x84 | as shopkeeper |
| Airship creature of kind-4 plots (dead code, structures §5.5.5) | 0x88 | LookAtPlayer only |
| **Guards** | — | **none**: the Alpha town builder creates no guard NPCs. (The dungeon "guard creature" of structures §1 is a vase, type 143, controller kind 6, without behaviour.) |

So shopkeepers and innkeepers stand at their counter point: with the WalkPath rules of
`alpha-materials-wildlife-routes.md` §4.6 and a one-point route of radius 2, each arrival
starts a 2.0–6.0 s wait and the next "route point" is the same counter point; if pushed or
lured more than 2 blocks away they walk back. They never look up a schedule or a seat.

Outside towns one more routine gives RandomInteraction to creatures: the zone generator's
creature routine client 0x5dc6e0 (called at 0x5eace7) places 1–3 creatures with controller
kind 1 and the mission flag 0x1000 on a ring of radius 3 blocks, each with the sequence
Combat 20, WalkPath (r = 2, own point), RandomInteraction, RandomWalk (the first may get
the boss flag 0x200). With outdoor stools, benches or towels within reach they sit down
between fights (*believed*: camp groups; the routine is not otherwise specified).

**Resident day, as read.** Combat has no target (residents are not hostile to players,
behaviour §13.2 step 7) and is not handled; LookAtPlayer is handled while a player is
within 8 blocks (unless the resident sits or lies, action 83/84), and then it **clears the
path**; SpawnLocation is handled while it walks to a new entry; once arrived,
RandomInteraction looks for a seat (§4) and otherwise RandomWalk ambles within 10 blocks of
home (now the entry point).

Consequences worth keeping for parity:

* A player who walks up to a resident on its way stops it (LookAtPlayer) and clears its
  path and search; when the player leaves, step 5 sees an empty path and declares
  **arrival** where the resident stands, setting home to the entry point. RandomWalk then
  walks it **straight** toward that point (no path search) whenever it is more than 10
  blocks away.
* A schedule point with no ground within 11 blocks below (or solid for 11 above) is
  "arrived at" on the next update without moving, with the same straight-line leash.
* Entries after midnight are skipped; the resident keeps the last entry before midnight
  until 00:00, then heads home.

### 3.7 Worked example

A resident's schedule (times as built by structures §5.5.9 step 4 with sample draws):

| Entry | Point | Time τ | Clock |
|---|---|---|---|
| 0 | home | 0 | 00:00 |
| 1 | plaza | 30 000 000 | 08:20 |
| 2 | market | 34 800 000 | 09:40 |
| 3 | shop door | 43 800 000 | 12:10 |
| 4 | plaza | 44 400 000 | 12:20 |
| 5 | inn door | 49 800 000 | 13:50 |
| 6 | shop door | 61 800 000 | 17:10 |
| 7 | shop door | 63 300 000 | 17:35 |
| 8 | plaza | 63 600 000 | 17:40 |
| 9 | home | 69 600 000 | 19:20 |
| 10 | home | 76 800 000 | 21:20 |

* τ = 13:00 (46 800 000): entries 0…4 have time ≤ τ, so the current entry is **4** (plaza).
  The spawn object stands at the plaza point; a resident activated now appears there.
* At 13:50 the current index becomes 5: path start = feet + 0.5; for the inn-door point
  (1203.5, 884.5, 40.25) the block (1203, 884, 40) is air, so no raise; the block below
  (z = 39) is the doorstep (solid), so no lowering: *G* = (1203.5, 884.5, 40.25),
  goal cell (1203, 884, 40), r = 4. One round now, up to 10 per update after that; the
  follower drops the path within 4 cells and the next update records arrival, home := the
  inn-door point.
* At 21:20 the index changes 9 → 10 although both points are "home": a new search from
  where the resident stands to the same place, and arrival at once if it is already within
  4 cells.
* A schedule whose last entries fall after midnight (e.g. a first time of 09:59 and long
  stays) simply never reaches them; at 00:00 the index changes from the last due entry to
  0 and the resident walks home.

---

## 4. RandomInteraction (server 0x41bc20, client 0x4db130)

### 4.1 Who has it

Town **residents** (class 0x88, §3.6) and the creatures of the zone routine client 0x5dc6e0
(§3.6). Shopkeepers, innkeepers, wildlife, wandering groups and pets do not.

### 4.2 State

| Where | Content |
|---|---|
| behaviour +0x04 | **timer** (ms; starts 0) |
| creature obj +0x1478, +0x147c, +0x1480 | **target**: the static's triple (zone x, zone y, index in the zone's static list; `alpha-interaction.md` §4); "none" = (−1, −1, 0) |
| creature action (obj +0x68) | 83 sitting, 84 lying (`alpha-interaction.md` §6.1 step 4) |

### 4.3 One update

1. timer := max(timer − dt, 0).
2. If timer = 0:
   * if the action is **83 or 84**: **stand up** — drop the path and search state, target
     := none, action := 0, intent := **(0, 0, 1)**, timer := **20 000**; **not handled**.
     (The non-zero intent also ends sitting in the physics step and frees the seat:
     `alpha-physics-camera.md` §2 step 3, `alpha-interaction.md` §6.4.)
   * otherwise action := 0 (any action, every update while the timer is 0, *as read*).
3. If the action is 83 or 84 → **handled** (sitting or lying out the timer).
4. **Keep the current target?** It is kept (step 6) when it can still be found and its
   occupant (static +0x40, u64) is 0; where it is looked for depends on where the creature
   stands (§4.4).
5. **Pick a new target**, only if the timer is 0 (§4.4). If a free seat is picked: timer
   := 20 000, target := it, start a path search (path start = feet + 0.5 as §3.3 step 3.2;
   goal = the seat point of §4.4; arrival radius **r = 2**; `alpha-path-search.md` §4.1),
   then continue with step 6. If nothing is kept or picked (no candidate, the drawn seat
   is taken, or the timer is not 0): drop the path and search state, target := none →
   **not handled**.
6. **Walk**: if the node map is not empty, up to 10 search rounds with the stop tests of
   §3.3 step 5.2 (here r² = 4).
7. **Use**: if the target static still resolves (through the world's region and zone
   tables) and the squared 3-D distance from the creature's **centre** to the static's
   position is **< 9** (3 blocks): queue an interaction record of kind **3** on the target
   in the creature's own queue (obj +0x130c; the same record the player's R key makes,
   `alpha-interaction.md` §5.5), drop the path and search state, target := none, intent :=
   0, velocity := 0, timer := 20 000.
8. **Handled** (steps 6–7, whether or not it was close enough to use the seat).

The owner's step then handles the queued kind-3 record (`alpha-interaction.md` §6.1): if the
seat is still free and the action is 0, 83 or 84, the creature is placed on it, turned, and
its action becomes **83** (stool, bench) or **84** (bed, beach towel).

### 4.4 Where seats come from

The creature's **block** is tested against the houses of its zone (client 0x4d4c20, server
0x4d4c20): the zone at (block x ÷ 256, block y ÷ 256); for each house of the zone's house
list (zone +0x88, `alpha-worldgen-structures.md` §5.5.4), each grid cell whose **role is 1**
(storey) covers the blocks O + (13*a* … 13*a* + 13, 13*b* … 13*b* + 13, 7*k* … 7*k* + 6)
(both ends included, *as read*), with O the house origin; the first house with a covering
cell is "the house the creature is in". Cells of role 2 (ground floor) do not count
(*as read*; open point R-1).

**In a house.** Keep: the target is in the house's **seats** list (+0x48) or **beds** list
(+0x54) (structures §5.5.4 step 16: seats = statics 16, 18; beds = 19) and is free. Pick:
by day — τ from 06:00 (21 600 000) to 22:59:59.999 (82 799 999) inclusive — the seats list,
otherwise (23:00 … 05:59) the beds list; one entry is drawn (one `rand()`, index = draw
mod count, unsigned); if it is free it becomes the target, its **raw position** is the
goal (no ground fit). If the list is empty or the drawn entry is taken: nothing this update
(the timer stays 0, so the draw is repeated on the next update).

**Not in a house.** The candidates are the statics of the 3 × 3 cells of 8 × 8 blocks
around the creature (cell = block ÷ 8; zone = cell ÷ 32; cells outside 0 … 2²¹ − 1 skipped;
the zone's cell index at zone +0xac, `alpha-interaction.md` §4.3), i.e. a 24 × 24-block
square that contains the creature with 8–16 blocks on each side. Keep: the target is among
them and free. Pick: every static of kind **16 (stool), 18 (bench) or 68 (beach towel)** in
those cells that is in **line of sight** from the creature's position to the static's
position (`alpha-path-search.md` §1, with its object flag set and the 200-block limit) is
appended to a candidate list in cell order (x outer, y inner); one draw picks index = draw
mod count; if that static is free it becomes the target. (Stone stools 15 and sandstone
stools 17 are never chosen; beds outside houses are not either.) The goal is the static's
position fitted to the ground by **block** tests with **no limit**: while the block at the
goal is solid, raise one block; then while the block below is not solid, lower one block.

Occupancy is only read here; it is set and cleared by the interaction step
(`alpha-interaction.md` §6.1 step 4, §6.4).

### 4.5 Timing of a visit

* **Search for a seat** only when the timer is 0. The timer starts at 0, so the first time
  RandomInteraction is reached (for a resident: after its first schedule arrival) it looks
  at once.
* **Walk** with r = 2 (path-search §6: the follower drops the path within 2 cells of the
  goal cell); the use test is 3 blocks from the creature's centre, which the arrival radius
  normally satisfies.
* **Sit or lie** for the rest of the 20 000 ms timer started at the use (step 7) — about
  **20 s**, minus the time until the owner's step handles the record.
* **Stand up** when the timer reaches 0 (step 2), then a **20 s cooldown** (timer 20 000)
  during which RandomInteraction returns not handled every update, so RandomWalk runs.
* Then the next pick (possibly the same seat).

While seated, LookAtPlayer does not turn the creature (it ignores actions 83 and 84),
Combat does not reset actions 83/84 (behaviour §13.2 step 1), and SpawnLocation returns not
handled until the next schedule entry. When the next entry comes due, SpawnLocation starts
walking (intent from the follower), which ends the sitting in the physics step and frees
the seat; the target triple stays set.

Consequences, *as read*:

* A kept target is walked to only while a search is running. If something else emptied the
  search state (LookAtPlayer when a player came near, or SpawnLocation's arrival), the
  target is still "kept" (step 4) but step 6 does nothing: the creature stands still,
  handled, until the seat is taken by someone else, or (outdoors) until it is no longer in
  the 3 × 3 cells. In a house, where the lists are searched regardless of distance, such a
  resident can stand until its next schedule entry. (Open point R-2.)
* Beach towels are "beds" for the use step: residents who pick one outdoors by day **lie
  down** on it (action 84).

### 4.6 Worked example

A resident (box height 1.8) has just arrived at a plaza point; it stands at (1000.5,
2000.5, 31.9) (feet on z = 31), not in a house cell. Its block (1000, 2000, 31) is in cell
(125, 250); the scan covers blocks x 992…1015, y 1992…2015. Two statics there are in
sight: a bench (kind 18) at (1006.0, 2003.5, 31.0) and a stool (kind 16) at (995.5, 1996.5,
31.0), both free. Cells are visited x-outer, so the stool's cell (124, 249) comes before the
bench's (125, 250): the list is [stool, bench]. Draw 7 → 7 mod 2 = 1 → the **bench**. Target
:= the bench; timer := 20 000. Goal: block (1006, 2003, 31) is air
(statics are not blocks), block below solid → goal stays (1006.0, 2003.5, 31.0). Path start
(1000.5, 2000.5, 31.5), r = 2. The follower walks it; at (1003.7, 2002.1, 31.9) the use
test gives 2.3² + 1.4² + 0.9² = 5.29 + 1.96 + 0.81 = 8.06 < 9: it queues kind 3, stops, timer
:= 20 000. The owner's step seats it on the bench (action 83, yaw (rotation + 2)·90). Twenty
seconds later: action 0, intent (0, 0, 1), the bench is freed, and for 20 s it ambles
(RandomWalk) before choosing again.

At 23:30 the same resident, back home on an upper storey, draws from its house's beds list
instead and lies down (action 84) on a free bed for 20 s at a time.

---

## 5. Additions to other specs (not edited here)

* `alpha-creatures-models.md`: **F-5** answered by §1; **F-2** remainder by §2 (two cells
  left, §2.3); §6.5 "bottom/top" are the weapon's butt and tip (§1.1); STANCE exception is by creature type
  (§2.2); several Appendix C phase boundaries corrected (§2.1).
* `alpha-creatures-behaviour.md` **A-2**: SpawnLocation (§3) and RandomInteraction (§4);
  §13.5 "when the time of day passes an entry" is made exact in §3.2; town sequences in
  §3.6.
* `alpha-path-search.md` §3: the SpawnLocation row is confirmed (start feet + 0.5, goal =
  ground point, r = 4, rounds as Companion; the ground test is by single blocks, 11 up then
  11 down); the RandomInteraction row: start feet + 0.5 (was "not traced"), goal = the
  static's position (outdoors fitted with unlimited block tests, in a house raw), r = 2,
  candidates by line of sight within the 3 × 3 cells (not "within 200 blocks": 200 is only
  the line-of-sight limit), up to 10 rounds per update.
* `alpha-interaction.md` §6.1 end note: the NPC use distance is from the creature's centre
  to the static's position; day/night lists are seats by day, beds by night (confirmed),
  but only while the creature stands in a storey cell of a house (§4.4).
* `alpha-materials-wildlife-routes.md` §4.3 table: SpawnLocation — handled while starting
  or walking and on the arrival tick; not handled with no schedule, after arrival, or when
  no ground is found. RandomInteraction — handled while seated, walking to a kept or new
  target, or using it; not handled when standing up, during the cooldown, or with no free
  candidate.

---

## 6. Notes for AlphaVulkan

* **Trails**: keep 4 × 16 points per creature in creature-relative x/y and absolute z; age
  them in the pose update with the five rules of §1.2 (frame-time weights); draw one
  32-vertex strip per hand in the transparent effects pass before water: depth test on,
  depth write off, no culling, unlit, untextured, straight alpha blending, white with the
  alpha table of §1.3. The one-frame lag and the discarded combo tint are parity details; a
  "fixed" option can tint by combo using §1.3's formulas.
* The pose-69 arc is world-scale (20 blocks); keep it, it is what Alpha shows.
* **Behaviours**: implement SpawnLocation and RandomInteraction with the exact "handled"
  results of §3.4 and §4.3; their side effects on the shared path state (and LookAtPlayer's
  clearing of it) produce the visible quirks of §3.6 and §4.5. Schedules, the seat kinds
  (16, 18, 68 outdoors; house lists) and the 06:00/23:00 switch are good data-table
  candidates for mods.
* Run both on the simulation owner (server, or host offline), like every behaviour
  (behaviour §13.1).

---

## 7. Open points

* **T-1** Trail colour. *Game check*: chain a combo of five or more sword hits (TEST
  character) and screenshot the trail against a dark background; as read it stays white
  (no cyan/purple or pink tint).
* **T-2** Blend and fog state of the trail pass (§1.3). *Game check*: swing in front of
  bright sky and dark rock; the trail should darken neither (straight alpha, no fog).
* **T-3** Point 0 when no weapon model is drawn in that hand (unarmed creatures in trail
  poses): not traced whether anything else writes it; as read it keeps being shifted by
  (camera − position) every frame. *Check*: log pose state +0x56c for an unarmed NPC using
  action 1 with the mod DLL.
* **T-4** The pose-69 arc size (20 blocks for action 104). *Game check*: let a shield NPC
  (or the TEST character with a shield and full mana) release action 104 and record the
  white half-ring.
* **P-1** Pose 21's unidentified components and pose 37's arm base (§2.3); and a recording
  of poses 3, 38, 39, 63 to confirm the corrected boundaries (*check*: log pose state +0x08
  and the channels, as models F-6).
* **S-1** The server's world step: confirm that spawn objects follow their schedules there
  too (server 0x5322d0, assembly only). *Check*: online, watch a resident at 08:20 move from
  its house to the plaza point while a second client observes.
* **S-2** Interrupted walks (§3.6): *game check* — walk next to a resident while it is on its
  way, then leave; as read it resumes in a straight line toward the entry point (no path
  around houses) once more than 10 blocks from it.
* **S-3** Entries after midnight (§3.2): *game check* — with a resident whose late entries
  fall after 24:00 (log its schedule with the mod DLL, creature +0x148c), confirm it stays
  at the last due point until 00:00.
* **R-1** House cells of role 2 (§4.4). *Game check*: at 23:30 enter a one-storey house with
  beds; as read, a resident standing on the ground floor uses the outdoor rule (stools,
  benches, towels nearby) and never lies in the bed.
* **R-2** Stale targets (§4.5): *game check* — let a resident start walking to a bench, step
  in front of it until it stops, then step away; as read it then stands still until the
  bench is taken or its next schedule entry.
* **R-3** The zone routine client 0x5dc6e0 (hostile ring creatures with RandomInteraction):
  identify the location kind it serves and whether its creatures sit on camp stools.
