Status: FINAL / STABLE / HELD

# Alpha clouds: placement of the cloud models

This contract describes where the original Alpha client draws its cloud models. Clouds are not voxel-world content: on every renderer invocation the client places copies of a single voxel model on a grid around the camera, using generator noise and climate fields. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. Addresses below are RVAs. The cloud block lies inside renderer `0x0AC260`, at `0x0B1527–0x0B1A65`, with the cell loop closing at `0x0B1A65`.

---

## 0. Conventions

N is the generator's two-argument noise kernel, original routine `0x0C0EF0`, called with two binary64 arguments converted from signed 32-bit integers and returning a value that is rounded to binary32. The climate and height quantities are original client routines called on the client's world object (client `+0x2E4`) with two signed 32-bit block coordinates:

- moisture(x, y): routine `0x1C4800`;
- fire(x, y): routine `0x1EF040`;
- M(x, y): routine `0x1EFFA0`;
- H(x, y): routine `0x1C5E20`.

These names follow the project's regions, terrain and random contracts, which own the meaning and arithmetic of those routines; this contract identifies them only by RVA and call order. Positions are in blocks unless stated. A fixed-point coordinate is a signed 64-bit value at 65,536 units per block. Converting fixed to block divides by 65,536 with signed 64-bit division, truncating toward zero, and keeps the low 32 bits (helper `0x02F570`). Converting a block integer to fixed shifts it left by 16 bits (helper `0x012080`). Converting a binary32 value to fixed multiplies it by binary32 65,536, then truncates to signed 64-bit (helper `0x02C580`).

---

## 1. The model

Only one model is used, `cloud02.cub`, registry index 2568. The model registration code pushes index `0xA08` (2568) directly after the `cloud02.cub` name string at RVA `0x311F00` (`0x11BDC6–0x11BDE0`). At renderer entry the client reads that entry as the 32-bit pointer at byte offset `0x2820` (4 × 2568) of the model table at client `+0x300`. It reads the entry only when the table holds more than `0x2820` bytes; otherwise the pointer is null. The original string table has no `cloud01` name. Evidence: `0x0AC2B0–0x0AC30F`.

---

## 2. When clouds are drawn

Render-state setup for the cloud pass runs at `0x0B1476–0x0B1522`, before any of the tests below. It is not specified here. Then:

1. If client byte `+0x8006E6` is nonzero, the whole block is skipped. The renderer also tests this byte at entry and after the block.
2. Let P be the position of the creature whose pointer is at client `+0x8006D0`, read from its fixed position at `+0x10` and converted to blocks. If moisture(P.x, P.y) ≤ binary32 0.01, no clouds are drawn.
3. If fire(P.x, P.y) ≥ binary32 0.01, no clouds are drawn.

Evidence: `0x0B1527–0x0B15C6`; literal 0.01 at RVA `0x2FCDA0`.

---

## 3. The grid

C is the client's fixed-point camera/reference position at `+0x140`, converted to blocks. Then cx = C.x / 200 and cy = C.y / 200, using signed integer division truncating toward zero. The binary uses a multiply-high by `0x51EB851F` with a 6-bit arithmetic shift and a sign correction.

The outer loop runs i from −6 to 6 inclusive. The inner loop runs j from −6 to 6 inclusive. Cell (X, Y) = (cx + i, cy + j). For each cell, in this order:

1. **Coverage.** c = (N(X, Y) + 1) × 0.5, computed in binary32. m1 = moisture(X, Y): the moisture routine is called with the cell indices themselves as block coordinates, not with the cell's position. The cell is skipped unless c < 0.8 × m1, where 0.8 is the binary32 literal at RVA `0x2FCDC8`.
2. **Position.** px = 200 × X. py = 100 × (2 × Y + r), where r is the C-style remainder of X by 2, so r is −1, 0 or 1. Alternate columns are therefore offset by half a cell, and the offset is negative for negative odd X. Both values are made fixed-point by the block-to-fixed rule, and Z starts at zero.
3. **Local moisture.** The cell is skipped if moisture(px, py) < binary32 0.3. The literal is at RVA `0x2FCDC0`. The position components are converted back to blocks for this call.
4. **Height.** The client evaluates M(px, py) first, then H(px, py). zb = H + M × 100, in binary32. zb is stored into the Z component by the float-to-fixed rule. If the resulting fixed Z is negative, it is replaced by zero. A second binary32 term, 50 × N(17X, 19Y) + 170, is converted separately by the float-to-fixed rule and added to fixed Z. The literals 100, 50 and 170 are at RVAs `0x2FD0B4`, `0x2FCD7C` and `0x2FD598`.
5. **Scale.** s = 2 × N(15X, 23Y) + 7, in binary32, applied uniformly to all three axes. The literals are at RVAs `0x345E10` and `0x2FCE28`.
6. **Rotation.** The integer (X + Y) × 90 is converted to binary32 degrees. The original rotation helper multiplies it by binary32 0.01745329238474369 and uses precise binary64 cosine and sine rounded to binary32 (helper `0x024610`).
7. **World matrix.** The matrix starts from identity. Four operations are applied in this call order: translation, scale s, rotation about Z, then translation by (−0.5 × w, −0.5 × d, 0). The second translation reads w and d as the signed 32-bit model fields `+0x44` and `+0x48`, converted to binary32, with −0.5 at RVA `0x2FCE8C`. Each operation combines on the vertex side of the existing matrix, so a model vertex receives them in reverse call order: the half-size recentring first, then the rotation, then the uniform scale (rotation and uniform scale commute), then the first translation. That translation is the cell's fixed position plus the client's signed 64-bit render shifts at `+0x1D8` (X) and `+0x1E0` (Y), with Z unshifted, converted to binary32 by rounding each signed 64-bit component to binary32 and multiplying by 1/65536.
8. The matrix is submitted through `0x0482A0`, and the model is drawn through `0x0E6DF0`.

Placement reads no clock, and the block makes no random draw. Cloud positions therefore change only when the camera's cell indices change. The 13 × 13 window reaches from 1,200 to 1,300 blocks in each axis direction from the camera's cell.

Evidence: `0x0B15CC–0x0B1A65`; helpers `0x02F570`, `0x012080`, `0x02C580`, `0x024610`, `0x024730`, `0x024990`, `0x024A60`, `0x024810`, `0x024820`.

---

## 4. Index arithmetic example

For a camera at block coordinate 8,396,928 on both axes, cx = cy = 41,984, and the window spans cell indices 41,978 through 41,990 on each axis. The coverage test then samples moisture at block columns in that same index range on both axes. All cells of such a window therefore read climate from one small area near those coordinates, not from under the clouds.

---

## 5. Evidence limits

The coverage sampling at cell indices (step 1) is the original's observed behaviour. Whether it was intended is not established. Any numeric value of N, of the climate fields or of the model dimensions depends on the routines named in §0 and on owner content, and is not stated here.

---

## Re-certification (2026-11-14)

Re-checked against the original listing, original image bytes (MSVC dumpbin) and original function files. No reconstruction or other contract was read.

Confirmed: model index 2568 and its table offset; the `+0x8006E6` skip; both player-position climate gates, with their comparison directions and the 0.01 literal; camera-cell division; the 13 × 13 loop order; the coverage formula and its sampling at cell indices; the staggered position with the C remainder; the 0.3 local-moisture skip; the height composition (H + 100M, clamp at zero, separate 50N + 170 term); scale; rotation; the model-size recentring from `+0x44`/`+0x48`; the absence of time dependence.

Corrected (none reverses an earlier placement claim):
- Fixed-point conversion of the base height: old open question; new: confirmed truncation of the binary32 value times 65,536.
- Render origin: old "translate relative to the render origin +0x1D8..0x1E4"; new: the shifts at `+0x1D8`/`+0x1E0` are added to X/Y, with Z unshifted.
- Helper identification: climate and noise quantities are now cited by original RVA, and their meaning is left to the owning contracts.
- Gate placement: noted that cloud render-state setup precedes the gates.
- Removed: the cross-references to rendering-spec sections and draw order; the "never below z ≈ 120" and "5..9" ranges; the seed-123 noise values; the runtime-checking procedure. None of these can be confirmed from the cloud block alone.

Unresolved:
- Whether routines `0x1C4800`, `0x1EF040`, `0x1EFFA0` and `0x1C5E20` are the moisture, fire, mountain-mask and base-height quantities as defined by the regions and terrain contracts was not re-verified in this pass.
- The output range of N, and therefore the scale and height ranges.
- Whether model fields `+0x44`/`+0x48` are the voxel X/Y dimensions for `cloud02.cub`.
- The render-state values set at `0x0B1476–0x0B1522` and their restoration after the block.
