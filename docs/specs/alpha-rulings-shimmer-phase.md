Status: FINAL / STABLE / HELD

# Alpha ruling: shimmer modulation phase (sine versus cosine)

Scope: the optional colour oscillation ("shimmer") at the end of the world pixel program
used for terrain, creatures, items and models. Conflict resolved: alpha-rendering.md §6.2
step 5 (and its re-certification note 30) say the colour is multiplied by 1 + 0.3·sin θ;
alpha-terrain-shader-color-numerics.md says the cosine output of the native sine/cosine
operation is used.

Image: client Cube.exe, SHA-256
84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF, 32-bit, image base
0x400000. RVAs below are image-relative; the section holding the shader blobs maps RVA to
file offset by subtracting 0x1000.

## Ruling

The modulation uses the **cosine**. With θ as below, the three colour channels of the
fog/sky-blended colour c are multiplied by m = 0.3·cos θ′ + 1, where θ′ is θ range-reduced
to [−π, π). Sine is not used anywhere in this program.

**alpha-terrain-shader-color-numerics.md wins.** alpha-rendering.md §6.2 step 5 is wrong in
the function: "c ← c · (1 + 0.3·sin θ)" must read cos θ. Its re-certification note 30 is
right about the range ([−π, π) from a fractional-part reduction) but wrong in "sin is
unaffected"; the function is cosine. Everything else in §6.2 step 5 (angle terms, amplitude
0.3, sign-only use of shininess) agrees with the bytecode.

Because θ′ is congruent to θ modulo 2π, cos θ′ = cos θ up to rounding; the difference from
the sin reading is a quarter-period phase shift, not a reduction artefact.

## Exact behaviour, in instruction order

Inputs: the second interpolated texture-coordinate register supplies X (clip x / clip w,
first component) and Y (clip y / clip w plus fog-gradient translation, second component);
the third supplies n.x, n.y, the first two components of the doubled, normalised
world/view face normal. c is the result of the final fog/sky interpolation.

| # | Blob offset | RVA | Operation |
|---|---|---|---|
| 1 | 0x394 | 0x2FEEE4 | copy X, Y to a temporary (xy mask) |
| 2 | 0x3A0 | 0x2FEEF0 | one two-component dot-and-add: (Y, X)·(10, 5) + n.y. The source swizzle is yx, so 10 multiplies Y and 5 multiplies X |
| 3 | 0x3B4 | 0x2FEF04 | add n.x. This sum is θ = 10·Y + 5·X + n.y + n.x |
| 4 | 0x3C4 | 0x2FEF14 | multiply-add: θ × 0x3E22F983 (≈ 0.159154937, 1/2π) + 0.5 |
| 5 | 0x3D8 | 0x2FEF28 | fractional part (frc), result in [0, 1) |
| 6 | 0x3E4 | 0x2FEF34 | multiply-add: × 0x40C90FDB (≈ 6.28318548) + 0xC0490FDB (≈ −3.14159274). This is θ′ |
| 7 | 0x3F8 | 0x2FEF48 | sincos into a temporary with write mask **.x only** |
| 8 | 0x404 | 0x2FEF54 | multiply-add: that .x × 0x3E99999A (≈ 0.300000012) + 0x3F800000 (1). This is m |
| 9 | 0x418 | 0x2FEF68 | multiply c.rgb by m (mask xyz) |
| 10 | 0x428 | 0x2FEF78 | cmp on −shininess (pixel c4.x): output RGB = c when −shininess ≥ 0, else m·c |
| 11 | 0x43C | 0x2FEF8C | output alpha = pixel c3.x |

Step 7 is the decisive token: 0x02000025 (sincos) with destination 0x80010001 (temporary
r1, write mask x) and source 0x80000000 (r0.x). For ps_3_0 sincos, the x component of the
destination receives the cosine and the y component the sine; only x is written, and step 8
reads x. [API meaning: Microsoft, sincos - ps.]

The literals are in def tokens at blob 0x148 (RVA 0x2FEC98: 10, 5, 0x3E22F983, 0.5) and
blob 0x160 (RVA 0x2FECB0: 0x40C90FDB, 0xC0490FDB, 0x3E99999A, 1).

* **Range reduction.** frc yields [0, 1), so θ′ = frac(θ/2π + 0.5)·2π − π lies in
  [−π, π) as computed with the stored constants: frc = 0 gives exactly 0xC0490FDB, slightly
  below real −π (by about 8.7e-8); the largest frc gives a value below 0x40490FDB. No other
  wrap, clamp or offset occurs.
* **Amplitude.** 0.3 (binary32 0x3E99999A); m ranges over about [0.7, 1.3]. No saturation
  is applied to m or to the product.
* **Channels.** Only R, G, B are modulated (masks xyz in steps 9 and 10). Alpha is never
  modulated; it is the uniform c3.x.
* **Selector.** Only the sign of shininess matters. Shininess ≤ 0 (including −0, since
  −(−0) = +0 passes the ≥ 0 test) selects the unmodulated colour; strictly positive selects
  m·c. Steps 1–9 always execute; the cmp is a select, not a branch.
* **Precision.** No partial-precision (_pp) modifier on any instruction or input
  declaration; documented full precision applies. The sincos result is the device's native
  approximation; its accuracy and exact rounding are not fixed by the bytecode.

## Evidence

* World pixel program at RVA 0x2FEB50 (file offset 0x2FDB50), 0x44C bytes through its end
  token (0x0000FFFF at blob 0x448), version token 0xFFFF0300 (ps_3_0). SHA-256 of those
  0x44C bytes: 773C20C6C523A4B59DD0D9CC5897C65516E9BC39EF6211E9AF3678C35AE39D14 (matches
  the identity recorded in alpha-terrain-shader-color-numerics.md). Embedded constant table
  names c4 "shininess", c3 "alpha".
* Disassembled with the system D3DCompiler_47 D3DDisassemble from the original image bytes,
  and cross-checked by walking the raw instruction tokens by hand (offsets above are token
  offsets from blob start).
* A token walk over every shader blob in the image (vertex programs at RVAs 0x2FD928,
  0x320558, 0x3212C0; pixel programs at 0x2FEB50, 0x2FEFA0, 0x2FF1D0, 0x2FF5C0, 0x2FF7C8,
  0x320E28, 0x321420, 0x321530, 0x321778, 0x321A98, 0x322270) finds exactly one sincos,
  at RVA 0x2FEF48. No other program contains a trigonometric oscillation.
* CPU side: the selector is uploaded by the setter at RVA 0x048FE0 (one-vector pixel
  constant upload through device method offset 0x1B4, SetPixelShaderConstantF), called with
  a computed float right after RVA 0x0C7BE0 at item sites such as 0x01FD8B, 0x0200E4,
  0x021DEF, 0x071BAD, 0x0765D5, and reset with 0.0 immediately after (e.g. 0x01FDA5,
  0x0200FE) and in the renderer at 0x0AD9A9 and 0x0BAE7E. These confirm the selector's
  role only; the predicate's value producer is not re-derived here.
* Infinite-Cubes/alpha was not consulted.

## Unresolved

* Static reading only; not verified at runtime or against a capture.
* Device-specific sincos approximation error, interpolation accuracy of X, Y, n, and the
  exact behaviour for NaN/infinite θ or a NaN shininess (cmp with NaN) are not established.
* The tiny excess of the stored 2π and −π constants over their real values makes θ′ = −π
  (by 8.7e-8) at frc = 0; whether a device's sincos treats that as in-domain is not
  documented beyond the [−π, π] input requirement. Its cosine is −1 to binary32 precision.
* What value the item path passes as shininess (output of RVA 0x0C7BE0) is outside this
  ruling; see alpha-rendering.md §11.2 and its note 20.
