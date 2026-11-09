# Alpha open points 11: exact float arithmetic of the colour blends

Clean-room finding from the Ghidra listing of the Alpha client. Addresses are citations only.
Bit values of constants were read from `binary/Cube.exe`. AlphaVulkan source was not read, so
nothing here says what our code does.

## 1. Result in short

* **Every blend is a plain 32-bit float reading of the terrain spec.** All colour routines use
  SSE scalar single instructions. Nothing keeps extended or double precision across steps.
* **The flips do not come from the form of `C·(1−t) + D·t`.** Its order and rounding points are
  what §6 of the terrain spec says.
* **The likely source is the factor, not the blend.** The flips fit the surface pass (terrain
  §5.4 steps 7 and 10), which re-blends whole-number bytes with rock using `cover` and `r8`.
  Those factors come from smooth() and the M chain, and the spec leaves their rounding order
  open. See §5 below.

## 2. Rules that hold for all routines below

Confidence: high (instruction level; every routine listed was scanned for x87 arithmetic).

* **Precision.** No x87 arithmetic in any of: 0x4c0ef0, 0x5c4520, 0x5c4800, 0x5c4dd0, 0x5c56e0,
  0x5c7140, 0x5e4850, 0x5ee540, 0x5eea60, 0x5ef040, 0x5ef5a0, 0x5effa0, 0x5c3ef0, 0x5cade0,
  0x5989d0, 0x5f0720, 0x5f8e10, 0x5f9340, 0x5f9620, 0x5f9bc0, 0x5f9e50, 0x5f9f70, 0x5cb600,
  0x5cbe00. Their only x87 use is FILD, FLD, FSTP, FLDZ and FLD1, to return a value in ST0 or
  to convert a 64-bit integer. A returned value is stored to a 32-bit float at the call site
  before use, so every value crossing a call is an f32.
* **Every operation rounds to f32.** Each MULSS, ADDSS, SUBSS and DIVSS is one rounding point.
  Brackets below show the exact order. Spilling to the stack changes nothing.
* **Noise** (0x4c0ef0) is all double and is rounded to f32 once, at the end.
* **Rounding state.** The executable has no LDMXCSR, so MXCSR stays at its default: round to
  nearest, no flush to zero. FPU control writes (0x5654f0, 0x5fc0d0, 0x6e0bd0) are outside
  these routines, and the CRT start-up `_controlfp_s` cannot matter with no x87 arithmetic.
* **Vector helpers.** There are none here. All scaling and adding is inlined per channel. No
  routine forms C + (D−C)·t. No fused multiply-add exists in this binary's code paths (the
  instructions are plain MULSS and ADDSS).
* **1 − t** is always computed first as its own rounded f32 and shared by the three channels.
  So a blend is (C·(1−t)) + (D·t) with four rounding points. Operand order inside one
  operation does not matter, since addition and multiplication commute.
* **Bytes.** CVTTSS2SI straight from the f32, low 8 bits stored. No clamp at that point and no
  double involved. Clamps exist only inside the grass, sand, beach, snow, rock and cliff bases.
* **Clamp.** Plain comparisons; below 0 becomes 0, above 255 becomes the exact constant 255.0
  (0x437f0000). No arithmetic.
* **Constants** are the f32 of the written decimal, with **one exception**: 0x71dd08, the
  divisor "0.4" of M step 5, is 0x3ecccccc = 0.3999999761581421 (1 − 0.6f folded by the
  compiler), not 0.4f (0x3ecccccd). The other 0.4 (0x6fd318) is the normal 0.4f. Noise factors
  (0.03, 0.01 and so on) are true doubles, not widened floats.

## 3. Helpers

| Helper | Exact order | Sites read |
|---|---|---|
| **smooth(x)** | **((3·x)·x) − (((2·x)·x)·x)**, inlined, never a call | 0x5e51bd, 0x5e5240, 0x5f036e, 0x5f03c4, 0x5f0422, 0x5f04db, 0x5f06e5 |
| ease(g) = 1 − (1 − g²)² | a = g·g; b = 1 − a; c = b·b; result 1 − c | 0x5c5829, 0x5c59f3, 0x5c5b6b, 0x5e511d |
| Cube in M | (u·u)·u first, and only then times M | 0x5f044e |
| Cube in T | (v·v)·v | 0x5e4f71, 0x5e5058 |
| Fifth power u⁵ | (((u·u)·u)·u)·u, repeated multiply, no pow | 0x5f9ad4 |
| Absolute value | through double and back; lossless | 0x5f9aa9 |

In smooth, doubling is exact, so ((2x·x)·x) equals 2·((x·x)·x). **The 3x term is not exact**:
(3·x)·x and 3·(x·x) differ in the last bit. Neither x·x·(3−2x) nor "x²·3 − x²·x·2" (the shader
wording in alpha-rendering.md) is the generator's order.

## 4. Routines, in exact order

unit(n) means (n + 1)·0.5.

### 4.1 Grass 0x5ee540 (0x5ee6b4..0x5ee71b)
a = unit(N). p = 120·N, e = 30·N, k = (80·N) + 1.
R = ((k·a) + (200·(1−a))) + min(−p, 0).
G = ((120·a) + (230·(1−a))) + e.
B = ((k·a) + (k·(1−a))) + p. Clamp. Confidence: high.

### 4.2 Sand 0x5f9bc0
a80 = unit·80. b = unit. g = (40·N) + 200.
R = (((1−b)·255) + (b·255)) + a80.
G = (((1−b)·g) + (b·150)) + a80.
B = (((1−b)·100) + (b·50)) + a80. Clamp. Confidence: high.

### 4.3 Beach 0x5c4520
Same a80, b. R = (((1−b)·240) + (b·240)) + a80. G = (((1−b)·240) + (b·180)) + a80.
B = (((1−b)·100) + (b·50)) + a80. Clamp. Confidence: high.

### 4.4 Snow 0x5f9e50
a = unit. R = (a·190) + ((1−a)·100). G = (a·220) + ((1−a)·180). B = (a·255) + ((1−a)·255).
Clamp. Confidence: high.

### 4.5 Rock 0x5f9620
1. v = (((a + b)·0.5) + 1)·0.5, a and b the two strata noises. wm is column +0x08 if the lookup
   succeeds, else 0x5c4dd0.
2. wm < 0.5: t = wm·2. R = ((1−t)·10) + (t·160); G = ((1−t)·20) + (t·160);
   B = ((1−t)·50) + (t·170).
3. wm > 0.75: t = (wm − 0.75)·4. Noise n has first argument (double)t·0.001 + 6544.
   R = ((1−t)·120) + (t·200); G = ((1−t)·120) + (((n·20) + 130)·t);
   B = ((1−t)·130) + (t·100).
4. **Sum.** V = v·160, one rounded value shared by all channels. R = (V + baseR) + (0·10);
   G = (V + baseG) + (0·10); B = (V + baseB) + ((n2 + 1)·10). The (0·10) term adds exact zero.
   Clamp.
5. **Fire tint**, if F > 0 (F is column +0x0c if found, else 0x5ef040). s = (0.1·N1) + N2, then
   min with 1. u = 1 − |s|. u5 as in §3. i = 1 − F.
   R = (R·i) + (((u5·200) + 10)·F).
   G = (G·i) + ((((((n3 + 1)·u5)·50) + 10))·F).
   B = (B·i) + (F·10). No clamp after this. n3 reuses the doubles 0.1x and 0.1y of step 1.

Confidence: high.

### 4.6 Surface colour 0x5c56e0
rk = ((R·10) − 0.3)·1.5, sat.
1. **Hot rock** (wm > 0.8): a = (wm − 0.8)/0.1, min 1. n = sat(N). b = (ease(n)·0.8)·a.
   C = (C·(1−b)) + (Rk·b).
2. **Desert** (mo < 0.2 and wm > 0.75): d = (((1 − (mo/0.2))·(wm − 0.75))·4)·10, sat.
   If L has type 4: g = sat(f − 0.5), d = ease(g)·d. Then d = d·rk. If d > 1 then d = 1, else if
   d < 0.5 then type 4. C = (C·(1−d)) + (sand·d).
3. **Beach**: g = sat((N + ((15 − z)/10)) − 0.5), (15 − z) integer to f32 before the division.
   g′ = ease(g). If wm < 0.2: g′ = g′·(wm/0.2). If g′ > 0: C = (C·(1−g′)) + (beach·g′).
4. **Snow** (wm < 0.3): k = 1 − ((wm − 0.29)/0.01), min 1. C = (C·(1−k)) + (snow·k).
5. **Fire** (F > 0): C = (C·(1−F)) + (Rk·F). Rk already holds the tint of §4.5 step 5.
6. Bytes as in §2, then type OR 0x20.

Confidence: high.

### 4.7 Cliff 0x5c7140 (returns floats, no truncation)
1. Base: R = ((60v + (20n1 + 180)) + 20n2); G = ((60v + (20n3 + 100)) + 20n4);
   B = ((0·v + 60) + 20n5). Clamp.
2. wm > 0.75: t = (wm − 0.75)·4. C = (C·(1−t)) + ((Rk·t)·0.9). Rock is multiplied by t first.
3. wm < 0.2: s = 1 − (wm·4). i = 1 − s (rounded, not 4wm). h = s·0.5.
   C = (C·i) + ((Rk + snow)·h).
4. F > 0: C = (C·(1−F)) + (Rk·F).

Confidence: high.

### 4.8 Surface pass 0x5e4850 (where bytes get re-blended)
* **T** (0x5e4f47..0x5e5086): v = f32(1 − |sum|), the sum and the 0.05 product in double; then
  w = 1 − ((v·v)·v), w·w. T = (1−v2³)² + ((1−v1³)² + 0).
* **Types 6, 13** (0x5e510d..0x5e51f3): k = bump·2, min 1; k′ = ease(k);
  m = sat((N·1.3) + 1); P′ = ((smooth(m)·k′)·0.4) + P.
* **cover**: G = P′·T. s = sat(G·1.25). cover = smooth(s) (0x5e5240..0x5e525d).
* **Step 7** (0x5e5357..0x5e54d8): c bytes to f32 (exact). i = 1 − cover.
  c′ = trunc((c·i) + (Rk·cover)) per channel.
* **r8** = P′·8, min 1.
* **Step 10** (0x5e55f7..0x5e57d6): i = 1 − r8; A = Cl·i; h = r8·0.5; D = c′ + Rk, c′ being the
  bytes as f32; result = trunc((D·h) + A).
* **Ravine bank** (0x5e5c3a, 0x5e5d36..): j = 1 − k stored once; trunc((b·k) + (Rk·j)).
* **Ravine floor** (0x5e6065..0x5e63d4): i = 1 − k; h = k·0.4;
  c3 = trunc((c2·i) + ((Cl + Rk)·h)); then c4 = trunc((c3·i2) + (Rk·cover)), with i2 the
  1 − cover saved at step 7.
* **Layered mesa** 0x5cb600 (0x5cbc38..0x5cbd6e): t = sat((integer as f32)·0.05);
  result = trunc((Cl·(1−t)) + (Rk·t)), type 5. **Rock mound** 0x5cbe00: trunc(Rk), type 6.

Confidence: high.

### 4.9 M chain 0x5effa0 (feeds cover and r8)
Same reading as terrain §3.4, with these points fixed:
* M = 1 − (|b + c|·(((1 − (a′·a′))·1.3) + 2)), |b + c| from an f32 sum.
* Step 4: M = M·((N·0.4) + 0.6).
* Step 5, f ≤ 0.36: ((M + s)·(1 − k²)) + (k²·0).
* Step 5, f < 1: g = (f32(√f) − 0.6)/0.39999998 (see §2); result (((1 − g²)²)·s) + M.
* Step 7: x = smooth(..)·M; x = smooth(..)·x; u = 1 − smooth(..); M = ((u·u)·u)·x.
* Types 4, 5: g = (f32(√f) − 0.5)·2; M = ease(g)·M. Last line smooth(M).

Confidence: high for what is listed. H (0x5c5e20) was not audited, since it reaches colour only
through trunc(S). R and W were read at spec level; only R's listing was read, not W's.

## 5. What causes the flips

**Explanation, confidence medium-high.** It fits every symptom, but it was not proved on a real
block, because that needs the factors of a failing column.

1. **Why whole numbers.** c in step 7 is already a byte, and rock is clamped to exactly 255 in
   hot places. In fire land with F = 1, both sides have B exactly 10. So (c·i) + (Rk·cover) is
   255 or 10 plus rounding noise, and trunc turns noise just under the whole number into 254 or
   9. Step 10 does the same with r8. With small cover or Rk close to c the same happens at any
   byte value, which may be the (243, ...) case (not proved).
2. **B = 9 needs this blend.** A brute-force run of the rock tint and the fire step, over F near
   1, rock B from 50 to 255 and surface B from 0 to 255, gave a minimum of exactly 10. Rock B
   cannot be below 50 before the tint. So 9 cannot come from §6 alone, in Alpha or in a faithful
   port. It comes from the byte blend after it.
3. **Why hot and desert.** That is where c and Rk are equal, through clamping or the fire tint.
4. **Size of the effect.** Synthetic run, s uniform on (0, 1), 4 million samples, as a share of
   equal-value blends with 0 < cover < 1:

| Test | Result |
|---|---|
| Alpha form lands below 255 / below 10 | 4.0% / 1.8%, never above |
| Flip if cover is x·x·(3−2x) | 2.3% at 255, 0.5% at 10 |
| Flip if cover is 3(x·x) − 2(x·x·x) | 1.5% at 255, 0.4% at 10 |
| Flip if cover comes from double arithmetic | 2.1% at 255, 0.4% at 10 |
| Flip if s is off by one unit in the last place | 4.3% at 255, 1.3% at 10 |
| Flip if the blend uses fused multiply-add | 1.4% to 4.0% at 255 |
| Blend written C + (D−C)·t, or done in double | always exactly 255; flips one way only |

   The wrong-factor cases flip about evenly in both directions, as observed. The last row shows
   a different blend form would flip one way only, so our blend form is probably right and a
   factor differs. An overall 0.5% of blocks is plausible since only blended blocks count, but
   real cover values were not measured.

**Suspects, in order. Check these against our code:**

1. smooth() not written as ((3·x)·x) − (((2·x)·x)·x). "3t² − 2t³" in the random spec §0 does not
   fix the order, and the rendering spec's wording gives a different one.
2. The M chain: M·u·u·u instead of ((u·u)·u)·M, or other regrouping from §4.9.
3. The divisor 0.39999998 (only near location types 6 and 7).
4. T in the wrong precision: the sums are double, the cube and square are f32.
5. Compiler contraction: fused multiply-add or fast-math on the engine's float code. x64 SSE
   with strict float settings matches Alpha.
6. A last-bit difference further up: R, W, q, or the climate values. The sum order of the
   climate blends (scan order, regions spec §4) matters too.

**Ruled out by the listing:** x87 extended precision, double kept across blends, shared vector
helpers, clamp order, pow, trunc from double, changed rounding mode, flush to zero. Differences
in CRT cos (`libm_sse2_cos_precise`, CALL 0x68d7a4) at the last double bit are negligible: by
estimate they change the f32 noise result about once per 10⁸ values or less.

**Tests that would settle it:**

* Among flipped blocks, each should have 0 < cover < 1 or 0 < r8 < 1, or come from a §6 blend of
  equal values such as fire over sand. A flip at cover exactly 0 or 1 with F = 0 would disprove
  this explanation.
* With the 32-bit DLL method of terrain §7, call 0x5effa0 at a failing column and compare the
  float bits of P, then T, then cover with ours. The first differing value is the bug.

**Not checked:** Server.exe's 0x518630 was only surveyed. It has the same instruction mix (SSE
arithmetic, x87 only for FILD, FLD and FSTP) but was not read line by line.
