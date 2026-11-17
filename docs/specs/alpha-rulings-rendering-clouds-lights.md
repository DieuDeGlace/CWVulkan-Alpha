Status: FINAL / STABLE / HELD

# Alpha rulings: clouds, frame projection, stars, point lights and creature order

This document compares the re-certified `alpha-rendering.md` (its "Re-certification
(2026-11-16)" section) with three other held contracts:

* `alpha-worldgen-clouds.md`;
* `alpha-terrain-chunk-point-lights.md`;
* `alpha-trails-poses-schedules.md`, §1 only (the creature pass order).

It does not edit or replace any of them. Where two contracts disagree, or where one leaves
something out, the ruling below names the contract that wins and gives the behaviour the
evidence shows. Where they agree, the ruling says so briefly. Behaviour that neither contract
states is marked **New**.

## Image and evidence

* Client: `binary/Cube.exe`, 3,885,568 bytes, SHA-256
  `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (confirmed), image base
  0x00400000.
* Addresses are virtual addresses (VA) in that image, which is how `alpha-rendering.md` cites
  them. RVA = VA − 0x400000, which is how `alpha-worldgen-clouds.md` and
  `alpha-terrain-chunk-point-lights.md` cite them. For example, cloud RVA 0x0B1534 is VA
  0x4B1534, and moisture routine RVA 0x1C4800 is VA 0x5C4800.
* Evidence:
  * the original instruction listing `decompiled/client/assembly.txt`, for the frame routine
    0x4ac260, the noise 0x4c0ef0, the star generator in 0x459c40, the projection builder
    0x427910, the rotation helper 0x424610, the visibility helper 0x47f760 and the fog
    update in 0x488ee0;
  * individual original function files under `decompiled/client/functions/`;
  * the call table `decompiled/client/calls.csv`, used for reachability searches;
  * constants read from the image bytes.
* No reconstruction, implementation, capture or runtime observation was consulted.
* "Frame" means one call of 0x4ac260. G is the game object passed to it. "Base +n" is a
  frame local at stack offset n, with no arguments pushed.

---

## 1. Cloud model and shade colour

**Agreement.** Only `cloud02.cub` is used. It is model-table entry 2568 (0xA08), read
through byte offset 0x2820 of the table at G+0x300. The entry is read only when the table
spans more than 0x2820 bytes (0x4ac2b0..0x4ac2ee). Both contracts state this.

**Rendering wins on the shade (an addition, not a conflict).** When the entry is non-null,
the frame sets its shade bytes on every frame (0x4ac2f9..0x4ac30f):

* +0x5c = 0;
* +0x5d = 100;
* +0x5e = 200.

This gives the blue shade (0, 100, 200) of `alpha-rendering.md` §11.1. The clouds contract
does not mention the shade.

**Residual.** Whether the mesh is built before the first shade write depends on when the
model is first drawn. It is not re-traced here.

## 2. Cloud gate

**Agreement.** Both tests use the position of the local player, the creature at G+0x8006d0,
read from +0x10 and converted to blocks. They do not use the camera position.

* Clouds are drawn only if moisture (0x5c4800) is greater than binary32 0.01. A value of
  0.01 or less skips them (0x4b157a, JBE).
* Fire (0x5ef040) must be below 0.01. A value of 0.01 or more skips them (0x4b15c6). The
  literal is at VA 0x6fcda0.
* The test of byte G+0x8006e6 at 0x4b1527, which the clouds contract lists, is consistent
  with rendering correction 3: the byte is never set, so the test always passes.

**Residual.** None for the gate.

## 3. Grid, cell order and coverage

**Agreement.** Both contracts give the same grid:

* The camera cell is (bx ÷ 200, by ÷ 200), from G+0x140. Fixed-to-block conversion
  truncates; the division by 200 truncates toward zero (multiply-high by 0x51EB851F,
  0x4b161b..0x4b1653).
* The outer counter runs −6..+6 and gives X. The inner counter runs −6..+6 and gives Y
  (0x4b1a47..0x4b1a65).
* Coverage: c = (N(X, Y) + 1) × 0.5, computed in binary32. The cell is skipped unless
  c < 0.8 × moisture(X, Y), and the moisture routine receives the cell indices as block
  coordinates (0x4b167e..0x4b16d7).

The argument order was checked: the first noise argument and the first moisture argument
are X.

**Residual.** If c or the product is NaN, the comparison does not skip the cell. This is
noted only for exceptional states.

## 4. Lattice noise 0x4c0ef0 and its hash constant

**Rendering wins (the clouds contract leaves the noise to other contracts). Its formula is
right**, with three precisions:

1. **Hash.** For each lattice index n:
   * n is replaced by n XOR (n << 13);
   * H = n·(n²·60493 + 19990303) − 0x2DF722F3, in 32-bit wrap-around arithmetic.

   The image subtracts 0x2DF722F3 (0x4c0fe7 and the three repeats). This is the same as
   adding 0xD208DD0D, the constant `alpha-rendering.md` gives. The following mask with
   0x7FFFFFFF discards bit 31, so the image's result also equals that of the familiar
   0x5208DD0D variant. Each lattice value is 1 − (H & 0x7FFFFFFF) × binary64 2⁻³⁰ (VA
   0x702b20).
2. **Lattice points.** The indices are i + 57j for the four corners (i, j), (i + 1, j),
   (i, j + 1) and (i + 1, j + 1). Here i and j are the truncated arguments, and i + 1 and
   j + 1 are formed as truncate(double + 1.0). Blending is in x and then in y, with weights
   w = (1 − cos(frac·π))·0.5 (precise cosine).

   The π constant is the binary64 nearest 3.1415927 (VA 0x702b48, bytes
   97 d1 7e 5a fb 21 09 40), **not** the binary64 nearest π. This does not matter for
   clouds: all cloud arguments are integers, so frac = 0 and w = 0.
3. **Result.** The binary64 result is rounded to binary32 on return (0x4c10ed..0x4c10f3). For
   integer arguments the value is exactly the corner value h(i + 57j), rounded to binary32.
   It lies in (−1, 1].

**Residual.** None for integer arguments.

## 5. Position, local moisture and height

**Agreement on these points:**

* **Position.** P = (200·X, 100·(2Y + r), z), where r is the signed remainder of X by 2
  (0x4b16e9..0x4b1730). Odd columns are offset by half a cell, negatively when X is negative
  and odd.
* **Local moisture.** The cell is skipped when moisture(P.x, P.y) < 0.3 (0x4b1779..0x4b178f).
* **Height.**
  * The routine 0x5effa0 is called first, then 0x5c5e20.
  * zb = H + 100·M in binary32.
  * zb is converted to fixed by truncation and replaced by 0 if strictly negative (0x42c860
    with argument 0).
  * A separately converted 50·N(17X, 19Y) + 170 is then added (0x4b17b4..0x4b18ad).

**Rendering wins on one detail.** The two height routines receive **three** arguments
(P.x, P.y, 0): a constant 0 is pushed first, at 0x4b1795 and 0x4b17cf. The clouds contract
describes them as two-coordinate calls; read its M(x, y) and H(x, y) as M(x, y, 0) and
H(x, y, 0). The moisture and fire calls do take two arguments.

**Residual.** The meaning of the third argument, and the contents of 0x5c5e20 and 0x5effa0,
belong to the terrain contracts.

## 6. Scale, rotation and model matrix

**Agreement.** A model vertex is transformed in this order:

1. recentred by (−0.5·[+0x44], −0.5·[+0x48], 0). The getters are 0x424810 (+0x44, for x)
   and 0x424820 (+0x48, for y);
2. rotated about +Z by (X + Y)·90° (integer product converted to binary32, ×0.01745329,
   precise cos/sin rounded to binary32);
3. scaled uniformly by s = 2·N(15X, 23Y) + 7;
4. translated.

The helpers are applied in the reverse call order (0x4b18b2..0x4b1a13).

**Rendering is right on the direction.** The rotation helper 0x424610 replaces row 0 by
c·row0 + s·row1 and row 1 by c·row1 − s·row0. It acts on the vertex side, so a vertex maps
as (x, y) → (x·c − y·s, x·s + y·c). That is counter-clockwise from +X toward +Y, seen from
above.

**The clouds contract's wording wins on the translation.** The translation is the cell
position **plus** the render shifts G+0x1d8 (x) and G+0x1e0 (y), with z unshifted (vector
add 0x42c800). It is then converted to binary32 blocks. Where `alpha-rendering.md` §11.4
step 5 says "relative to the render origin", read it as this addition.

**Residual.** Whether +0x44/+0x48 are the voxel x/y sizes of `cloud02` is unchanged from
the clouds contract.

## 7. Cloud pass state and the lights clouds see

**Rendering wins (the clouds contract leaves render state unspecified).** Before the gate
(0x4b1476..0x4b1522) the frame sets:

* the cloud pixel shader (0x447d50);
* darkness = ((sky1 + sky2)·0.5, alpha S) (0x4b1433..0x4b1485);
* fogScale from 1.5·F (0x4b14a1..0x4b14bd);
* material (1, 1, 1, S);
* the light colours (0x448170).

This agrees with the cloud row of `alpha-rendering.md` §4.

**New: clouds have no point lights.**

1. At 0x4ada42..0x4adaff the frame zeroes a 16-entry frame light list: positions at base
   +0x3a14, colours at base +0x37d4. It uploads that all-zero list through 0x448f10.
2. The next point-light upload in the frame is the first terrain chunk's (0x4b1c0b), which
   comes after the cloud block.
3. The cloud draws therefore see ten lights of radius 0, which the vertex shader disables.
   So does the sky quad.

The frame list is filled at 0x4b12ea..0x4b1433 (§12 below), before the clouds, but it is
**not** uploaded until 0x4b21c2, after the terrain.

**Residual.** None within the frame. Virtual calls outside the direct call table were not
searched.

## 8. One projection for the frame (§8.5)

**Rendering is right**, with one precision.

* 0x427910 builds the frame projection at base +0x74c (0x4ae4b3..0x4ae5d3). Its values:
  * vertical field of view 50° (0x3F5F66F4), or 30° (0x3F060A92) while bit 0x400 of the
    local player's +0x124 is set;
  * aspect −(G+0x11c / G+0x120): the sign is flipped at 0x4ae5ab;
  * near 0.1 (0x3DCCCCCD) and far 2000 (0x44FA0000);
  * k = 1/tan(fov/2), with x-scale k/aspect and y-scale k;
  * z-scale far/(far − near), z-translation −near·far/(far − near), and w taken from view z.
* It is copied to G+0x800a90 (0x4aed96). The view G+0x26c is copied to base +0x79c
  (0x4aedab).
* **Every 0x4482a0 upload after 0x4ae5d3 passes it:**
  * the cloud upload (0x4b1a36);
  * terrain (0x4b1d40);
  * every objects-pass upload from 0x4b2bff to 0x4b8388;
  * water (0x4ba34a).

  Each passes base +0x74c as projection and base +0x79c as view.
* **Precision.** The only earlier upload, for the sky gradient quad (0x4adea6), uses the same
  stack slot while it still holds a separate fixed matrix built at 0x4adb04..0x4adb31:
  identity with elements [0] = [5] = 2, [10] = 1.0e-4 (0x38D1B7A0) and [11] ≈ −1.0e-5
  (0xB727C61A). It does not use the perspective. This is consistent with §8.1 ("matrices
  built for it"). Read §8.5's "every world draw" as every world draw after the sky quad.
* **New.** Right after the world-pass state is set, the frame rebuilds the six frustum planes
  that the visibility helper 0x47f760 reads (G+0x1000fa4 onward). It builds them from
  G+0x800a90 and the view (0x4aee14..0x4af33c).

**Residual.** None.

## 9. Stars: rand count and geometry

**Rendering is right.** Neither of the other contracts covers stars.

**Generation** (0x4636a8..0x463859): 3000 entries (0xBB8). Each makes exactly four CRT
`rand` calls, in this order:

1. θ = rand·2π/32767 in binary64, rounded to binary32.
2. φ = rand·(π/2)/32767, likewise.
3. size = rand·0.1/32767 + 0.01, all in binary32 (0x46375b..0x46376e).
4. rand % 10 == 0 doubles the size (0x463820..0x46384b).

The position is x = cos θ·r, y = sin θ·r, z = 100·sin φ, with r = 100·cos φ. The cosine
helper is 0x40e420 and the sine helper 0x424b50; both are precise and rounded to binary32.

**Drawing.**

* Stars are drawn while 0.5 > S (0x4ae641..0x4ae651), the camera is not underwater and the
  list is not empty.
* Each star is one triangle fan of four primitives with a 16-byte stride (0x4aea47..0x4aea5e).
* The centre is opaque white and the corners are 0x00FFFFFF. The first corner is repeated
  (0x4aea31..0x4aea3c).

**Residual.** The CRT random state at start-up (rendering open question 14).

## 10. Point-light sources

**The point-light contract wins on the catalogue; rendering §10.1 is incomplete.** Rendering
lists only sector static objects and ground items. The frame also collects four other source
groups, which the point-light contract describes and which were confirmed here:

* **Chunk-associated sources** (0x4ac73c..0x4acaa6):
  * the list is chunk +0x240, and source byte +0x40 bit 0 must be set;
  * visibility 0x47f760 with margin 16 and distance limit F + 22.4. The limit is base +0xc0,
    written at 0x4ac50c as G+0x1d4 + 22.4;
  * radius 15 + 0.5·cos(G+0x8003a0·0.01 + a per-source counter step);
  * kind 13 scales the colour by 1 − 0.5·S. The point-light contract's "local day-related
    factor" is S: base +0xc4 is written at 0x4ac44a by the S computation.
* **World list G+0x2f8**: kind +0x60 = 1, radius 16, colour (0.5, 0.5, 0.2) × source +0x4c
  (0x4af334..0x4af442).
* **Fading list G+0x800748** (0x4af486..0x4af583).
* **Creature branches** (0x4af5c5..0x4b06ef).

The point-light contract's details for these groups stand.

**New, for those four groups.**

* The world-list, fading and creature candidates are appended **without any visibility or
  distance test**. The frame's only calls of 0x47f760 during collection are at 0x4ac78a,
  0x4b094d, 0x4b0aa6 and 0x4b0d3e.
* The chunk-associated test runs before the frustum is rebuilt (§8), so it uses the planes
  left by the previous frame.

**Sector objects and items** (0x4b06f3..0x4b0e3c). Rendering is right on kinds, radii,
colours and margins, with these corrections and additions:

* **New: the sector scan is centred on the local player, not the camera.**
  * The scan reads the local player's position (G+0x8006d0 +0x10, 0x4b0708..0x4b0827) and
    takes d = F + 22.4.
  * It visits the cells whose indices run from trunc((p − d)·0.00390625) to
    trunc((p + d)·0.00390625) on each axis, computed in binary32 from the block coordinate.
    These are 256-block cells, looked up through 0x434d10 with x outer and y inner.
  * The camera is used only by the visibility test.
* **Static objects.**
  * Kind 50: margin 20. Kinds 51 and 65: margin 32.
  * All three use distance limit F + 22.4 and radius 20.
  * The position is object +0x08 plus (0, 0, the third float of +0x24).
  * The colour (1.2, 1.0, 0.8) or (1.2, 0.8, 0.4) is multiplied by (1 − S) through 0x4252f0.
* **Ground items** (sector list +0x30, item +0x140 = 0):
  * The colour is from 0x4c71c0, and the item is used only if its squared length is > 0.
  * **The light position is the item position plus 3 blocks in z** (0x43abf0 with
    argument 3, 0x4b0c90..0x4b0cab).
  * Visibility margin 16, and **distance limit 0.2·R = 128 blocks**. The limit is base +0xac,
    written at 0x4ad9f3 as G+0x1d0 × 0.2; it is not F + 22.4.
  * Radius 16. The colour is not scaled by S.

  Rendering §10.1's sentence that 0x47f760 "requires the source to be within F + 22.4
  blocks" is right for the static objects and chunk-associated sources only.

**Residual.**

* The full creature-branch catalogue and the fading-record lifetime stay as the point-light
  contract leaves them.
* The per-source radius counter of the chunk-associated sources was not traced.

## 11. Sorting and per-chunk selection

**The point-light contract wins (rendering is silent on the order; there is no conflict).**

* All candidates are sorted ascending by a squared-distance key from the camera G+0x140
  (0x4b0e84 through 0x4abae0). The sector branch computes the key at 0x4b09b9..0x4b09f5.
* Each chunk record is then cleared.
* Each light goes into every cache cell of the inclusive rectangle covered by its x/y
  position ± its radius, until that cell holds 16 (0x4b0e90..0x4b12a0).

Each chunk therefore keeps the 16 candidates **nearest the camera** whose rectangle covers
it. Rendering's "every chunk whose 32-block cell overlaps the light's position ± its radius,
up to 16 per chunk" is right but omits this order.

**Residual.** Equal-key order for large sorts, as stated in the point-light contract.

## 12. The frame-wide list

**Rendering is right; the point-light contract does not cover it.** After assignment
(0x4b12ea..0x4b1433):

* count = min(number of candidates, 16);
* the first count sorted candidates are written into the frame list at base +0x3a14 and
  +0x37d4;
* each record holds:
  * the position as fixed position + (G+0x1d8, G+0x1e0, 0), converted to binary32 blocks;
  * the radius from candidate +0x18 in lane 4;
  * the colour from candidate +0x1c.

These are the 16 lights nearest the camera in the whole frame, regardless of chunk. Only the
first 10 reach the shader (§13).

**Residual.** None.

## 13. Upload count, and which lights each draw sees (New)

**Agreement on the count.** 0x448f10 uploads 10 positions and 10 colours from the 16-entry
arrays. Both contracts state this.

**New: the point-light constants bound for each part of the frame.** These are the only
point-light uploads in the frame routine. No routine that the frame calls reaches 0x448f10,
except the map renderer 0x5fc1b0 (map path at 0x4ae004, and at 0x4babaf after the creatures).

| Draws | Lights bound | Evidence |
|---|---|---|
| Sky quad, cloud models | the all-zero list (no point lights) | 0x4adaff |
| Terrain, per chunk | that chunk's first 10 | 0x4b1c0b |
| Objects pass: statics, items, opaque creatures and the shader-drawn effects | the frame list's first 10 (the 10 nearest the camera) | 0x4b21c2 |
| Water, per drawn sub-mesh | that chunk's first 10 | 0x4ba327 |
| Deferred far statics (0x4ba4a4..0x4ba588) and deferred translucent creatures (0x4ba5d0..0x4ba9f6) | **no new upload**: the lights of the last water sub-mesh drawn, which are its chunk's lights; or, if no water sub-mesh was drawn in the frame, the frame list | — |

* The water loop uploads only for sub-meshes whose water index count (+0xc) is non-zero
  (0x4ba262..0x4ba327).
* The far-static routines 0x4bd160 and 0x4be760 and the creature routine 0x4128f0 do not
  reach 0x448f10.
* Translucent creatures and far statics are therefore lit by one arbitrary chunk's list,
  the one drawn last, not by the frame list. Rendering §10.2's "the frame list for
  creatures and models" is right only for draws made before the water pass.

**Residual.**

* The visiting order of the water loop (which chunk is last) is the order of the container
  at 0x4ba1ae..0x4ba42c. It was not characterised.
* Calls through virtual tables are not in the call table.

## 14. Collection timing in the frame (rendering §1.3 step 4)

**Rendering is imprecise.** Step 4 places "point lights are collected and assigned" before
the underwater test (step 5). The evidence shows the work split in two:

1. Chunk-associated sources are collected inside the early chunk loop
   (0x4ac4e0..0x4ad47e), before the frame constants.
2. All other groups, the sort, the per-chunk assignment and the frame list come **after**
   the sun sprite, the world-pass state and the frustum rebuild (0x4af334..0x4b1433). They
   run immediately before the cloud pass.

Because no point-light upload falls between the two, this changes no lighting result, apart
from the previous-frame planes noted in §10.

**Residual.** None.

## 15. Fog distance to chunk centres, 640-block radius, neighbour substitution

**Rendering is right.** The other contracts do not cover these.

* **Fog update** (0x489034..0x489316).
  * It starts from R², with R = G+0x1d0 = window size × 0.5 × 32 (0x45ab13..0x45ab36). The
    window size G+0x2dc is written only as 40, at 0x459f39, so R = 640.
  * Window coordinates must lie in [0, 0x80000) (0x489084..0x4890a6).
  * The ring slot is row (j mod 40) and column (i mod 40). It counts when it is empty
    (+0xc = 0) or holds other coordinates (0x4688a0).
  * The centre is (32i + 16, 32j + 16). The half-chunk comes from the binary64 −32768 at
    VA 0x702ad8, converted and subtracted at 0x489107..0x48915d.
  * The distance is horizontal: the camera's own z is used for the centre's z.
  * The target is √min − 22.4, smoothed with 1 − 0.999ⁿ.
* **Neighbour substitution defaults.** The initialisers hold these bytes:
  * 0x6fab00: (200, 200, 200), type 1;
  * 0x6faad0: three 0xFF bytes, type 0;
  * 0x6fab30: three 0xFF bytes, type 0x82.

  These match the solid, air and lit-water defaults of `alpha-rendering.md` §9.

**Residual.** None beyond rendering's own.

## 16. Creature draw order relative to water

**`alpha-rendering.md` wins over `alpha-trails-poses-schedules.md` §1.**

**Order.**

1. Every visible creature is visited in the objects pass (loop from 0x4b4e1b, visibility
   0x47f760 at 0x4b4e4e).
2. A creature whose +0x1190 ≤ 0 is drawn there by 0x4128f0 (0x4b5db2), before the trails
   and before water.
3. A creature whose +0x1190 > 0 is queued instead (0x4b5d53).
4. Queued creatures are drawn after the water pass and the deferred far statics
   (0x4ba5d0..0x4ba9f6). Each gets a depth-only call and then a colour call, and the frame
   re-tests +0x1190 > 0 at 0x4ba871 before drawing.

**Trails-spec statements that are wrong.**

* §1.2 says "the main creature pass (client 0x4ba8fe) passes the frame's dt; its other
  callers pass 0". **Two** calls in the frame pass dt (G+0x8006e8) as the fifth argument:
  * 0x4b5db2, the opaque body draw in the objects pass;
  * 0x4ba8fe, the depth-only draw of a translucent creature.

  The motion-ghost calls (0x4b5cf3) and the translucent colour call (0x4ba9a5) pass 0.
* §1.3 says "the creature pass that ages the histories (… 0x4ba5d0) comes later in the same
  frame, so the trail … shows the history aged in the previous frame". This holds only for
  **translucent** creatures. For an opaque creature the ageing call (0x4b5db2) runs before
  the trail draw (0x4b9a10..0x4ba13e), so its trail shows the history aged in the same
  frame. The step numbers that §1 cites ("step 12", "step 14") follow an older numbering of
  the rendering frame list. The current equivalents are §1.3 steps 15 and 18.

**New: draws that stay in the objects pass for a queued creature.** The queue branch jumps to
0x4b5db7, so the code after the body draw runs for queued creatures as well:

* up to four calls of 0x471b60, gated by the creature fields +0x1574, +0x1578, +0x155c and
  +0x157c. Each draws an item model with its own matrices, material, shimmer and colour
  (0x4b5db7..0x4b5ee0);
* the reset of `white` to 0 (0x4b5ef3).

The motion-ghost draws (0x4b5b62..0x4b5d30) likewise precede the queue test. For a
translucent creature, these item models and ghosts are drawn opaque in the objects pass,
with the frame lights. Only the body is deferred.

**Residual.**

* What the four 0x471b60 slots carry (held, carried or worn items) was not identified.
* The other content of trails-spec §1 (ageing rules, strip geometry and colours) was not
  re-audited.

---

## Summary of winners

| Topic | Winner | Note |
|---|---|---|
| Cloud model, gate, grid, coverage, position, local moisture | both agree | |
| Cloud shade (0, 100, 200) | rendering | the clouds contract omits it |
| Lattice noise and hash | rendering | 0xD208DD0D = −0x2DF722F3; π constant 3.1415927 |
| Height-routine arguments | rendering | third argument 0 |
| Rotation direction | rendering | counter-clockwise from above |
| Cloud translation sign | clouds contract | the render shifts are added |
| Cloud render state | rendering | **New:** zero point lights |
| Frame projection §8.5 | rendering | except the earlier sky-quad matrix |
| Stars | rendering | not in the other contracts |
| Point-light catalogue | point-light contract | rendering incomplete; **New:** player-centred sector scan, item z + 3, item limit 128 blocks, no tests for list/fading/creature groups |
| Sort and per-chunk selection | point-light contract | |
| Frame list of 16 | rendering | |
| Lights seen per draw | **New** | post-water draws use the last water chunk's list |
| Collection timing §1.3 step 4 | correction | split collection |
| Fog centres, 640, neighbour defaults | rendering | |
| Creature order and trail ageing | rendering | trails §1.2/§1.3 corrected |
