Status: FINAL / STABLE / HELD

# Alpha rulings: E.kind = 4 formation angle and appearance box constants

Two bounded rulings read from the original image. Image: `Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x400000.
All addresses are RVAs in that image. Constants were read from the image bytes (`.rdata`
raw offset = RVA − 0x2FC000 + 0x2FB000; immediates from the instruction bytes). No
reconstruction source was consulted. This document supplements, and does not edit,
`alpha-worldgen-caves.md` §8 item 5 and `alpha-spawn-objects.md` §4.1.

## 1. Angle of formation i in the E.kind = 4 site (RVA 0x1e9783 … 0x1e9bc3)

**Ruling: the angle of formation i (i = 0 … n − 1) is 2·i·π / n, not i·π / n.** The
statement "angle i·π / n" in `alpha-worldgen-caves.md` §8 item 5 is superseded by this
ruling.

Evidence and exact evaluation:

1. n = 6 + draw % 3 (draw at RVA 0x1e9783, signed remainder by 3) is kept as a loop count;
   n is also converted once to binary64 (RVA 0x1e97a6) and held for the whole loop. If
   n ≤ 0 the loop is skipped (RVA 0x1e979c; unreachable since n ≥ 6).
2. A 32-bit loop multiplier k starts at 0 (RVA 0x1e97aa). At the loop tail (RVA 0x1e9ba6 …
   0x1e9bc3) k is increased by **2** and the count decremented; the loop repeats while the
   count is non-zero. No other write to k occurs in the body. So formation i uses k = 2·i.
3. Angle (RVA 0x1e97c0 … 0x1e97d8): k converted to binary64; multiplied (binary64) by π,
   the binary64 constant at RVA 0x2fce08, bytes `18 2D 44 54 FB 21 09 40`, bit pattern
   0x400921FB54442D18 (= 3.141592653589793); then divided (binary64) by binary64 n; the
   quotient is rounded to binary32 (round-to-nearest under the default MXCSR). Order:
   (k · π) / n, multiplication first.
4. Sine first (RVA 0x1e97e0 … 0x1e97ec): the binary32 angle is widened to binary64 and
   passed to the runtime's precise SSE2 sine (thunk RVA 0x28d9c2, imported
   `_libm_sse2_sin_precise` from MSVCR110.dll, IAT slot VA 0x6fc45c); the binary64 result
   is rounded to binary32.
5. Cosine second (RVA 0x1e97f4 … 0x1e980a): the same binary32 angle, widened, is passed to
   the precise SSE2 cosine (thunk RVA 0x28d7a4, `_libm_sse2_cos_precise`, IAT slot VA
   0x6fc444); the result is rounded to binary32.
6. Radius (RVA 0x1e9814, 0x1e982c): each binary32 result is multiplied in binary32 by the
   binary32 constant at RVA 0x2fce3c, bytes `00 00 C8 41`, bit pattern 0x41C80000 (= 25.0),
   and converted to a 32-bit integer by truncation toward zero (RVA 0x1e9820, 0x1e983a).
7. Application (RVA 0x1e983e … 0x1e9862): the centre pair (two 32-bit integers, first then
   second component, copied from the frame each iteration) receives
   first += trunc(25 · cos(angle)) and second += trunc(25 · sin(angle)). The cosine term
   goes to the first component, the sine term to the second.
8. The two & 3 draws (RVA 0x1e9884, 0x1e989f) follow these computations; the trigonometry
   makes no draw.

Consequence: the n formations are spread over the full circle at equal steps of 2π / n.

## 2. Appearance box constants (RVA 0x3f93f … 0x3f954)

**Ruling: the box is written as binary32 0x3F75C290, 0x3F75C290, 0x400A3D71.**

| Write RVA | Instruction bytes | Appearance offset | Bit pattern | Exact value |
|---|---|---|---|---|
| 0x3f93f | `C7 46 08 90 C2 75 3F` | +0x08 | 0x3F75C290 | 0.96000003814697265625 |
| 0x3f946 | `C7 46 0C 90 C2 75 3F` | +0x0c | 0x3F75C290 | 0.96000003814697265625 |
| 0x3f94d | `C7 46 10 71 3D 0A 40` | +0x10 | 0x400A3D71 | 2.1600000858306884765625 |

Each is a 32-bit immediate store to the appearance record passed as the routine's second
argument, in the order +0x08, +0x0c, +0x10. The +0x10 value 0x400A3D71 is the
round-to-nearest binary32 of 2.16. The +0x08/+0x0c value 0x3F75C290 is **one unit in the
last place above** the round-to-nearest binary32 of 0.96 (which would be 0x3F75C28F,
0.959999978542327880859375); the image stores 0x3F75C290, and that pattern is normative.

Placement note: in the routine at RVA 0x3f7c0 these stores are not the first instructions.
They follow the no-record prelude (its 5 draws, RVA 0x3f80b … 0x3f877) or, with a choice
record, the record's selector dispatch (RVA 0x3f87c … 0x3f938, any selector > 7 jumping
straight to RVA 0x3f93f). Both paths converge at RVA 0x3f93f, so the box is written on
every call, before the per-type step (selector compare at RVA 0x3f956). Draw order stated
in `alpha-spawn-objects.md` §4.1 is unaffected: no draw occurs between the prelude and these
stores.

## Unresolved

* Units of the centre pair at the site (whole blocks versus another scale) were not
  re-derived here; they are as stated in `alpha-worldgen-caves.md`.
* The precise-sine and precise-cosine results depend on MSVCR110's implementation; their
  bit-exact outputs for each angle were not tabulated.
* The site's remaining body (shape writes, final static draw) was not re-examined.
