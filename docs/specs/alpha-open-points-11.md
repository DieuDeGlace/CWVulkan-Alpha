Status: FINAL / STABLE / HELD

# Alpha open points 11: exact float arithmetic of the colour blends

Derived from the disassembly of the Alpha client `Cube.exe` (SHA-256 84a7a132…30bf); constant
bit patterns were read from the image. Addresses are virtual addresses (base 0x400000) and
serve as citations. The terrain contract (`terrain` spec, pointer only) defines the
quantities named here; this document fixes their arithmetic order and precision.

## 1. Summary

* The colour routines below do their colour arithmetic with SSE scalar single-precision
  instructions. Double precision is used to form noise arguments, inside the noise routine,
  and in the explicitly stated places (absolute values, the T sums, square roots).
* A colour blend has the form (C·(1−t)) + (D·t) with 1 − t rounded once and shared by the
  three channels; the exact order of every listed routine is given in §4.

## 2. Rules that hold for all routines below

* **Precision.** The routines 0x4c0ef0, 0x5c4520, 0x5c4800, 0x5c4dd0, 0x5c56e0, 0x5c7140,
  0x5e4850, 0x5ee540, 0x5eea60, 0x5ef040, 0x5ef5a0, 0x5effa0, 0x5c3ef0, 0x5cade0, 0x5989d0,
  0x5f0720, 0x5f8e10, 0x5f9340, 0x5f9620, 0x5f9bc0, 0x5f9e50, 0x5f9f70, 0x5cb600 and
  0x5cbe00 contain no x87 arithmetic. Their only x87 instructions are FILD, FLD, FSTP, FLDZ
  and FLD1, used to return a value in ST0 or to load a 64-bit integer. At the call sites
  read, a returned value is stored as a 32-bit float before use.
* **Rounding points.** Each MULSS, ADDSS, SUBSS and DIVSS rounds to f32; the brackets in §4
  give the exact order. Intermediate spills are 32-bit stores and do not change values.
* **Noise** 0x4c0ef0 computes in double precision (its x87 use is FLD and FSTP only) and
  its result reaches the callers as an f32. It calls the CRT double cosine at 0x68d7a4.
* **Rounding state.** No LDMXCSR occurs in the client disassembly, so the SSE rounding mode
  is the default (round to nearest, no flush to zero). FLDCW occurs only in 0x5654f0,
  0x5fc0d0, 0x6e0bd0 and 0x6e0ec0, none of which is a routine listed here.
* **No fused or vector forms.** No FMA instruction occurs in the client disassembly. All
  scaling and adding is inlined per channel; no listed routine forms C + (D−C)·t.
* **1 − t** is computed first as its own rounded f32 and shared by the three channels, so a
  blend has four rounding points: two products, then their sum. Operand order inside a
  single operation does not matter (IEEE addition and multiplication commute).
* **Bytes.** Colour bytes are produced by CVTTSS2SI (truncation toward zero) directly from
  the f32 value, keeping the low 8 bits. There is no clamp at that point. Clamps exist only
  inside the grass, sand, beach, snow, rock and cliff base routines.
* **Clamp.** Plain comparisons: a value below 0 is replaced by 0, a value above 255.0 by
  the exact constant 255.0 (0x437f0000).
* **Constants** are the f32 of the written decimal, with **one exception**: the divisor at
  0x71dd08, used by M step 5 (0x5f02ee), is 0x3ecccccc = 0.3999999761581421 (numerically
  equal to 1 − 0.6f), not 0.4f (0x3ecccccd). The constant at 0x6fd318 is the normal 0.4f.
  Noise argument factors (0.1, 0.05, 0.005, 0.001 and so on) are doubles.

## 3. Helpers

| Helper | Exact order | Sites read |
|---|---|---|
| **smooth(x)** | **((3·x)·x) − (((2·x)·x)·x)**, inlined | 0x5e51bd, 0x5e5240, 0x5f036e, 0x5f03c4, 0x5f0422, 0x5f04db, 0x5f06e5 |
| ease(g) = 1 − (1 − g²)² | a = g·g; b = 1 − a; c = b·b; result 1 − c | 0x5c5829, 0x5c59f3, 0x5c5b6b, 0x5e511d |
| Cube in M | (u·u)·u first, then times M | 0x5f044e |
| Cube in T | (v·v)·v | 0x5e4f71, 0x5e5058 |
| Fifth power u⁵ | (((u·u)·u)·u)·u, repeated multiply | 0x5f9ad4 |
| Absolute value | widened to double, sign cleared, rounded back; lossless | 0x5f9aa9 |

In smooth, doubling is exact, so ((2x·x)·x) equals 2·((x·x)·x), but (3·x)·x and 3·(x·x)
can differ in the last bit. The forms x·x·(3−2x) and 3·(x·x) − 2·((x·x)·x) are not the
native order.

## 4. Routines, in exact order

unit(n) means (n + 1)·0.5. N, N1, N2, … are separate noise results.

### 4.1 Grass 0x5ee540 (0x5ee6b4..0x5ee71b)
a = unit(N1). p = 120·N2, e = 30·N4, k = (80·N3) + 1.
R = ((k·a) + (200·(1−a))) + min(−p, 0).
G = ((120·a) + (230·(1−a))) + e.
B = ((k·a) + (k·(1−a))) + p. Clamp.

### 4.2 Sand 0x5f9bc0
a80 = unit(N1)·80. b = unit(N2). g = (40·N3) + 200.
R = (((1−b)·255) + (b·255)) + a80.
G = (((1−b)·g) + (b·150)) + a80.
B = (((1−b)·100) + (b·50)) + a80. Clamp.

### 4.3 Beach 0x5c4520
a80 = unit(N1)·80, b = unit(N2). R = (((1−b)·240) + (b·240)) + a80.
G = (((1−b)·240) + (b·180)) + a80. B = (((1−b)·100) + (b·50)) + a80. Clamp.

### 4.4 Snow 0x5f9e50
a = unit(N). R = (a·190) + ((1−a)·100). G = (a·220) + ((1−a)·180). B = (a·255) + ((1−a)·255).
Clamp.

### 4.5 Rock 0x5f9620
1. v = (((a + b)·0.5) + 1)·0.5, a and b the two strata noises. wm is column +0x08 if the
   column lookup succeeds, else the result of 0x5c4dd0.
2. Base: (120, 120, 130) by default. If wm < 0.5: t = wm·2; R = ((1−t)·10) + (t·160);
   G = ((1−t)·20) + (t·160); B = ((1−t)·50) + (t·170).
3. Else if wm > 0.75: t = (wm − 0.75)·4. Noise n has first argument (double)t·0.001 + 6544
   and second argument (an integer coordinate)·0.001 + 123, both double.
   R = ((1−t)·120) + (t·200); G = ((1−t)·120) + (((n·20) + 130)·t);
   B = ((1−t)·130) + (t·100).
4. **Sum.** V = v·160, one rounded value shared by all channels. R = (V + baseR) + (0·10);
   G = (V + baseG) + (0·10); B = (V + baseB) + ((n2 + 1)·10). The (0·10) term adds an exact
   zero. Clamp.
5. **Fire tint**, if F > 0 (F is column +0x0c if found, else the result of 0x5ef040).
   s = (0.1·N1) + N2, then min with 1. u = 1 − |s|. u5 as in §3. i = 1 − F.
   R = (R·i) + (((u5·200) + 10)·F).
   G = (G·i) + (((((n3 + 1)·u5)·50) + 10)·F).
   B = (B·i) + (F·10). No clamp after this. n3 reuses the double noise arguments formed
   with the factor 0.1 in step 1.

### 4.6 Surface colour 0x5c56e0
The starting colour C is grass (§4.1) and the starting type is 4. rk = sat(((Q·10) − 0.3)·1.5),
with Q the result of 0x5f9340. Rk below is the rock colour of §4.5 (0x5f9620, called again
for each use).
1. **Hot rock** (wm > 0.8): a = (wm − 0.8)/0.1, then min with 1. n = sat(N).
   b = (ease(n)·0.8)·a. C = (C·(1−b)) + (Rk·b).
2. **Desert** (mo < 0.2 and wm > 0.75): type := 9. d = (((1 − (mo/0.2))·(wm − 0.75))·4)·10,
   sat. If the location lookup 0x487da0 returns an entry of type 4: g = sat(f − 0.5), with f
   from 0x5f8e10, and d = ease(g)·d. Then d = d·rk. If d > 1 then d = 1, else if d < 0.5 then
   type := 4. C = (C·(1−d)) + (sand·d).
3. **Beach**: g = sat((N + ((15 − z)/10)) − 0.5), (15 − z) converted from integer to f32
   before the division. g′ = ease(g). If wm < 0.2: g′ = g′·(wm/0.2). If g′ > 0:
   C = (C·(1−g′)) + (beach·g′) and type := 9.
4. **Snow** (wm < 0.3): type := 10. k = 1 − ((wm − 0.29)/0.01), then min with 1.
   C = (C·(1−k)) + (snow·k).
5. **Fire** (F > 0, F as in §4.5): C = (C·(1−F)) + (Rk·F); if F > 0.5, type := 12.
6. Bytes as in §2, then the type byte is ORed with 0x20.

### 4.7 Cliff 0x5c7140 (returns floats, no truncation)
1. Base: R = ((60v + (20n1 + 180)) + 20n2); G = ((60v + (20n3 + 100)) + 20n4);
   B = ((0·v + 60) + 20n5). Clamp.
2. wm > 0.75: t = (wm − 0.75)·4. C = (C·(1−t)) + ((Rk·t)·0.9). Rock is multiplied by t
   first.
3. wm < 0.2: s = 1 − (wm·4). i = 1 − s (rounded; not 4·wm). h = s·0.5.
   C = (C·i) + ((Rk + snow)·h).
4. F > 0: C = (C·(1−F)) + (Rk·F).

### 4.8 Surface pass 0x5e4850 (where bytes are re-blended)
* **T** (0x5e4f47..0x5e5086): each v = f32(1 − |sum|), where the sum (a noise value plus
  0.05 times another, both widened) and the subtraction from 1 are in double; then
  w = 1 − ((v·v)·v) and w·w in f32. T = (1−v2³)² + ((1−v1³)² + 0).
* **Types 6, 13** (0x5e508e..0x5e51f3): x = 1 − f (f from 0x5f8e10); bump = x·x if x > 0,
  else 0. k = bump·2, then min with 1; k′ = ease(k); m = sat((N·1.3) + 1);
  P′ = ((smooth(m)·k′)·0.4) + P. For other types P′ = P.
* **cover**: G = P′·T. s = sat(G·1.25). cover = smooth(s) (0x5e5210..0x5e525d).
* **Step 7** (0x5e5357..0x5e54ce): the colour bytes c are converted to f32 (exact).
  i = 1 − cover. c′ = trunc((Rk·cover) + (c·i)) per channel.
* **r8** = P′·8, then min with 1.
* **Step 10** (0x5e55f2..0x5e57cc): i = 1 − r8; A = Cl·i (Cl the cliff colour of §4.7);
  h = r8·0.5; D = c′ + Rk, c′ being the step-7 bytes as f32; result = trunc((D·h) + A).

### 4.9 M chain 0x5effa0 (feeds cover and r8)
Read as in terrain §3.4, with these points fixed:
* M = 1 − (|b + c|·(((1 − (a′·a′))·1.3) + 2)), with |b + c| taken from an f32 sum, widened
  to double for the absolute value and rounded back.
* Step 4: M = M·((N·0.4) + 0.6).
* Step 5, f ≤ 0.36: ((M + s)·(1 − k²)) + (k²·0).
* Step 5, f < 1: g = (f32(√f) − 0.6)/0.39999998 (see §2; the square root is a CRT double
  routine, 0x68d9c8, rounded to f32); result (((1 − g²)²)·s) + M.

## 5. Consequence of truncation after a blend

Because bytes are truncated from an f32 sum, a blend of two equal whole-number inputs with
0 < t < 1 (for example 255 against 255 after clamping) can produce a value one below that
whole number when the rounded sum (c·i) + (D·t) falls just below it. This is a property of
the native arithmetic of §4.8 steps 7 and 10.

## Re-certification (2026-11-14)

Checked against `Cube.exe` (SHA-256 84a7a132…30bf) disassembly and image constants; Server.exe
(SHA-256 1ffbcbce…4fb1) for the one survey item. The Infinite-Cubes reconstruction was not
consulted.

**Confirmed:** absence of x87 arithmetic in all 24 listed routines (instruction scan);
FILD/FLD/FSTP/FLDZ/FLD1 only; no LDMXCSR and no FMA in the client disassembly; 0x71dd08 =
0x3ecccccc and its use at 0x5f02ee; 0x6fd318 = 0.4f; clamp constant 0x437f0000; CVTTSS2SI
byte truncation; CRT cosine call 0x68d7a4; smooth order at all seven sites; ease, cube,
fifth power and absolute-value sites; grass, sand, beach, snow, rock, surface colour and
cliff formulas and orders; T, types 6/13, cover, step 7, r8 and step 10 of the surface pass;
M chain first line, step 4 and step 5.

**Corrected (old → new):**
1. FPU control writes listed as 0x5654f0, 0x5fc0d0, 0x6e0bd0 → 0x6e0ec0 also contains FLDCW.
2. "Every blend is a plain 32-bit float reading; nothing keeps double precision" → double
   precision is used for noise arguments, absolute values, the T sums and square roots; the
   listed routines 0x5ef5a0 and 0x5f9f70 also contain double subtraction, division and
   comparison (role not re-checked).
3. Rock: added the default base (120, 120, 130) for 0.5 ≤ wm ≤ 0.75 and the second noise
   argument of step 3.
4. Surface colour: added the starting colour (grass), starting type 4 and the type
   assignments 9, 10 and 12; rk's input named as the result of 0x5f9340 (previously "R").
5. Types 6/13: "k = bump·2" → bump defined as (1 − f)² for f < 1, else 0.
6. Grass: noise roles stated per noise (p from the second, k from the third, e from the
   fourth).
7. "0x71dd08 is 1 − 0.6f folded by the compiler" → stated as a numeric equality only.

**Removed:** the statement that the replacement source was not read; the whole diagnosis of
the replacement's colour "flips" (the explanation, its confidence, the synthetic simulation
table about alternative forms, the suspect list addressed to the implementation, the
compiler-contraction and strict-float advice, and the proposed tests using a 32-bit DLL
method); the claim about another document's shader wording; the "not audited / read at spec
level" notes about H, R and W, replaced by the unresolved list.

**Unresolved:**
* Ravine bank (0x5e5c3a, 0x5e5d36..), ravine floor (0x5e6065..0x5e63d4), layered mesa
  0x5cb600 (0x5cbc38..0x5cbd6e) and rock mound 0x5cbe00 orders from the earlier text were
  not re-verified and are withdrawn from this contract.
* M chain step 7 (only the cube at 0x5f044e was confirmed), the types 4/5 rule and the final
  smooth(M) were not re-verified and are withdrawn.
* Role of the double-precision subtract, divide and compare in 0x5ef5a0 and 0x5f9f70.
* The exact arithmetic of H (0x5c5e20), R and W.
* Whether the noise routine's final rounding to f32 happens inside the routine or only at
  the call site's 32-bit store.
* Server.exe 0x518630 was only surveyed: its x87 use is FILD, FLD and FSTP only, but it also
  contains CVTTSD2SI and double arithmetic; it was not read line by line.
* The numeric range of F (needed for any bound on fire-tinted channels) was not established.
