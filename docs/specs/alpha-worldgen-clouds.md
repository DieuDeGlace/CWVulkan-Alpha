# Alpha clouds: placement of the cloud models

Behavioural specification of where Cube World **Alpha** draws its clouds. Clouds are not
part of the voxel world: every frame the renderer places copies of one voxel model on a
grid around the camera, using the generator's noise and climate fields. This completes
alpha-rendering.md §11.4 and its open question 9. Written from the frame routine 0x4ac260
(cloud block 0x4b1489..0x4b1a65) of `Cube.exe` (SHA-256 `84a7a132…30bf`).

---

## 0. Conventions

As in the [random spec](alpha-worldgen-random.md) §0. N is the noise kernel; moisture(x, y),
fire(x, y), M(x, y) and H(x, y) are the moisture blend, fire share, mountain mask and base
height of the [regions](alpha-worldgen-regions.md) and [terrain](alpha-worldgen-terrain.md)
specs, evaluated with no zone (they read stored column values where a zone exists, which
gives the same numbers). Positions are blocks unless stated.

---

## 1. The model

One model, **cloud02.cub**, registry index 2568 (the model table entry at G+0x300 + 4·2568,
read at the start of 0x4ac260). No cloud01 model exists in the Alpha executable's model
table (the string is absent); alpha-rendering.md §11.4's mention of cloud01 should be read
as cloud02 only.

---

## 2. When clouds are drawn

Each frame, after the sky and sun (alpha-rendering.md §1.3 step 11), unless G+0x8006e6 is
set (the same flag that suppresses the sky). With **P** = the local player's position
(entity at G+0x8006d0, position at +0x10, converted to blocks by truncating division by
65536):

* if moisture(P.x, P.y) ≤ 0.01, no clouds;
* if fire(P.x, P.y) ≥ 0.01, no clouds.

---

## 3. The grid

Let **C** = the camera position (G+0x140) in blocks (truncating division by 65536), and
cx = trunc(C.x / 200), cy = trunc(C.y / 200) (integer division truncating towards zero,
done with a multiply-high by 0x51EB851F and a shift by 6).

For i = −6..6 (outer) and j = −6..6 (inner), cell (X, Y) = (cx + i, cy + j):

1. **Coverage test.** c = (N(X, Y) + 1)·0.5 (noise at the integer cell indices).
   m1 = moisture(X, Y) — the blend sampled at block column (X, Y), i.e. at the cell
   **index**, not at the cell's position (as the code does; see open question 1).
   Skip the cell unless c < 0.8·m1.
2. **Position.** px = 200·X, py = 200·Y + 100·(X mod 2) (C remainder), so alternate columns
   of cells are offset by half a cell.
3. **Local moisture test.** m2 = moisture(px, py); skip the cell if m2 < 0.3.
4. **Height.** zb = H(px, py) + 100·M(px, py) (float); if zb < 0, zb = 0. Then
   z = zb + (50·N(17X, 19Y) + 170). (The base height is set through the fixed-point
   position — float × 65536 truncated to an integer, then compared with 0 — and the second
   term is added as a separate fixed-point vector.)
5. **Scale.** s = 2·N(15X, 23Y) + 7 (uniform in x, y, z), so 5..9.
6. **Rotation.** (X + Y)·90 degrees about +Z (an integer converted to float; only the value
   modulo 360 matters).
7. **Matrix**, applied to model vertices in this order: translate by
   (−0.5·sizeX, −0.5·sizeY, 0) where sizeX, sizeY are the model's voxel dimensions (model
   +0x44 and +0x48), rotate by step 6, scale by s, translate to (px, py, z) relative to the
   render origin (G+0x1d8..0x1e4).
8. Draw the model with the cloud shader settings of alpha-rendering.md §4 and §6.3.

Nothing in the placement depends on time: Alpha's clouds do not drift. They move only
when the camera crosses a 200-block cell boundary (cells appear and disappear at the edge
of the 13 × 13 window, 1200–1300 blocks away, inside the fog).

---

## 4. Worked example (S = 123)

N(0, 0) = −0.28179, so a cell with indices (0, 0) would have c = 0.35910; it is drawn only
if 0.8·m1 > 0.35910, i.e. m1 > 0.44888. Its scale would be 2·N(0, 0) + 7 = 6.43642 and its
height offset 50·N(0, 0) + 170 = 155.910 above max(H + 100M, 0). Near the home point
(camera at 8396928), cx = cy = 41984 and the window spans cell indices 41978..41990;
m1 is then the moisture blend at block columns (41978..41990, 41978..41990), which lies in
region cell (2, 2) — far from the player — so the coverage threshold is the same for every
cell of the window (open question 1).

---

## 5. Checking against the running game

* Hook the draw call of cloud02 (0x4e6df0 with the cloud model) and log the world matrix
  set just before (0x4482a0); compare translations, rotation and scale with §3 for the
  current camera position.
* Visual check: clouds sit on a staggered 200-block lattice, rotated in quarter turns,
  higher over mountains (by 100·M), never below z ≈ 120, and are absent in dry (moisture
  ≤ 0.01) or fire regions.

---

## 6. Open questions

1. **Coverage sampled at the cell index.** The first moisture lookup uses (X, Y) as block
   coordinates. This reads the climate near the world origin rather than under the cloud;
   it is very likely a slip in the original, but a 1:1 port must keep it. A capture of
   which cells are drawn would confirm it.
2. The exact fixed-point conversion of the float base height (truncation of z·65536) is
   assumed; it only affects sub-block precision.
3. Whether the model dimensions read at +0x44/+0x48 are voxel counts in x and y (as in the
   placement guide's volume layout) was not re-checked for cloud02.cub.
