# Alpha noise cosine boundary

Status: **FINAL / STABLE / HELD**. Source-author freeze: 2026-11-07.

This contract identifies the mathematical import used by the original Alpha client's two-dimensional noise routine and bounds the inspected numeric behavior to finite binary64 inputs whose magnitude is at most 3.1415927. It distinguishes directly inspected binary facts, source-derived arithmetic examples, and unresolved accuracy or environment questions. It does not certify an interchangeable cosine implementation or native runtime acceptance.

## Evidence boundary and image identity

The source-author policy was applied in an isolated source context. This assignment inspected original binary evidence only: `binary/Cube.exe` and, after its original location was authorized, `game-files/msvcr110.dll`. Paths in this document are relative to the `disassembled alpha cube world` evidence root. PE import/export tables, native instructions, constant bytes and file-version metadata supplied the evidence. No reconstruction, replacement engine, SDK, extractor, tests, captures, probes, logs or implementation history were consulted for this assignment. The DLL and game were not executed; numeric examples below came from static arithmetic analysis.

| Property | Client image | Bundled runtime |
| --- | --- | --- |
| Evidence path | `binary/Cube.exe` | `game-files/msvcr110.dll` |
| File length | 3,885,568 bytes | 875,472 bytes |
| SHA-256 | `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF` | `B30160E759115E24425B9BCDF606EF6EBCE4657487525EDE7F1AC40B90FF7E49` |
| PE machine | `0x014C`, x86 | `0x014C`, x86 |
| Preferred image base | `0x00400000` | `0x10000000` |
| PE timestamp field | `0x51EA955E` | `0x5098858E` |
| Image size | `0x003BC000` | `0x000D6000` |
| Version metadata | Not used to identify this image | Product version `11.00.51106.1`; file version `11.00.51106.1 built by: Q11REL` |

Each RVA below is explicitly associated with the client or the DLL. Absolute load addresses are not required by the contract.

## Import identity and caller boundary

The client thunk at RVA `0x28D7A4` transfers through the IAT entry at RVA `0x2FC444`. Direct parsing of the original PE structures identifies that entry as **`MSVCR110.dll!_libm_sse2_cos_precise`**.

The supporting import descriptor is at client RVA `0x360814`. Its original thunk array begins at `0x360BF8`, and its IAT begins at `0x2FC380`. The relevant entry is index 49. Its original lookup value is `0x003634CA`; the ordinal-import flag is clear. The import-by-name record there contains hint 877 followed by the exported name above. Hint 877 is not an imported ordinal. The bundled DLL independently exports that name at DLL RVA `0x36F80`, with export ordinal 878.

The native noise routine begins at client RVA `0x0C0EF0`. Its calls at client RVAs `0x0C0F80` and `0x0C0FBC` put one binary64 argument in the low half of XMM0 and receive one binary64 result there. The imported entry implements this scalar boundary even though its internal arithmetic also uses packed SSE2 instructions. The upper half of the input register does not supply a second independent cosine argument.

For each valid coordinate-to-signed-integer conversion in the noise routine, the angle is obtained from the signed fractional remainder after truncation toward zero, multiplied in binary64 by the constant at client RVA `0x302B48`. That constant has bits `0x400921FB5A7ED197`, or approximately **3.1415926999999999**. Its bit pattern differs from nearest-binary64 mathematical π, `0x400921FB54442D18`.

The fractional remainder has magnitude less than one when that integer conversion is valid. The closed magnitude bound in this document conservatively contains the resulting cosine inputs. This does not establish valid noise behavior for coordinates whose integer conversion overflows, or for non-finite coordinates. After either import call, the client forms a blending weight using one minus the returned value, followed by multiplication by one half; those are distinct binary64 operations. The entire noise routine's output contract is outside this document.

## Floating-point environment

At DLL RVA `0x36F80`, the entry checks two portions of floating-point control state. Its SSE2 calculation is selected when:

- MXCSR masked with `0x7F80` equals `0x1F80`.
- The x87 control word masked with `0x007F` equals `0x007F`.

The MXCSR condition requires nearest rounding and all six SSE exception masks set. It does not inspect existing exception-status flags, denormals-are-zero, or flush-to-zero. The x87 condition checks its low control bits, including the exception masks; it does not test x87 precision or rounding fields. Consequently, changing only those x87 precision or rounding fields while retaining the accepted low bits does not change the selected SSE2 arithmetic. The fast calculation reads the x87 control word but performs no x87 floating-point calculation.

The numeric discussion and arithmetic anchors below assume nearest, ties-to-even binary64 arithmetic, accepted x87 low control bits, and denormals-are-zero and flush-to-zero both disabled. The ordinary control values MXCSR `0x1F80` and x87 control word `0x037F` satisfy these assumptions. This assignment did not establish the control state at every native call during a running game.

No FMA instruction occurs in the inspected entry. Multiplication, addition and subtraction have separate rounding points. The scalar SSE2 instructions round to binary64 precision; packed instructions perform corresponding binary64 operations in two internal lanes. A compiler's contraction of these arithmetic steps, an extended-precision evaluation, or another library's cosine is not established to produce the same bits.

## Input partitions and reduction constants

Within the requested finite domain, the inspected SSE2 behavior has two magnitude classes.

For magnitudes below **2^-252**, including either signed zero and all binary64 subnormals, the value returned under the stated environment is exactly positive one, bits `0x3FF0000000000000`. The native tiny-input arithmetic subtracts the input magnitude from one; nearest rounding makes its result one throughout this class. This does not specify the resulting exception-status flags.

At magnitude **2^-252** and above, every input in the requested domain uses the table-assisted calculation. Its partition label is the nearest integer, ties to even, of the separately binary64-rounded product of the input and the stored scale near 32/π. The label ranges from -32 through +32. Thus there are 65 possible reduction labels in the closed domain, rather than one polynomial covering the entire interval.

Partition boundaries must be interpreted after the rounded multiplication. They are not defined by comparing the input directly with exact mathematical half-multiples of π/32. For example, the positive input bits `0x3FA921FB54442D18` have label zero; the immediately greater binary64 input has label one. Both are close to π/64.

The following are exact native binary64 encodings. Decimal values are explanatory approximations.

| Mathematical role | DLL constant RVA | Binary64 bits | Approximate value |
| --- | --- | --- | --- |
| Scale near 32/π | `0x37980` | `0x40245F306DC9C883` | 10.185916357881302 |
| Integral-rounding offset | `0x37988` | `0x4338000000000000` | 6,755,399,441,055,744 |
| Leading reduction step near π/32 | `0x379A0` | `0x3FB921FB54400000` | 0.098174770420882851 |
| Secondary reduction step | `0x37990` | `0x3D90B4611A600000` | 3.7981878164399787e-12 |
| Further reduction correction | `0x379A8` | `0x3B63198A2E037073` | 1.2639164054974691e-22 |
| One | `0x379B0` | `0x3FF0000000000000` | 1 |

The secondary step is present in both internal packed lanes. The integral-rounding offset is 3 times 2^51. Adding and subtracting it under the accepted nearest-rounding mode obtains the integral reduction value in the small range used here. The leading, secondary and further step values divide argument reduction across several separately rounded quantities rather than using a single subtraction by a rounded π/32.

The reduced displacement is small: its magnitude is below 0.0491 for this domain. The DLL combines it with phase-dependent data from the 64-position table at DLL RVA `0x37140`. The phase selected for cosine is sixteen positions beyond the reduction label, cyclically modulo 64. The data express the quarter-cycle shift needed to obtain cosine from angle-addition values. They contain a main sine component with a small correction, and a cosine component divided between a coarse dyadic value and a residual. This mathematical interpretation is supported by the phase shift, the arithmetic and the inspected table values; it is not a claim that regenerating the table with another trigonometric library reproduces its stored bits.

## Local correction and rounding limits

The small-displacement correction uses the following signed inverse-factorial coefficients. Their exact encodings, rather than decimal re-parsing or an unspecified polynomial library, identify the inspected constants.

| Correction family and degree | DLL RVA | Binary64 bits |
| --- | --- | --- |
| Sine, degree 3: -1/6 | `0x37940` | `0xBFC5555555555555` |
| Sine, degree 5: 1/120 | `0x37950` | `0x3F81111111111111` |
| Sine, degree 7: -1/5040 | `0x37960` | `0xBF2A01A01A01A01A` |
| Sine, degree 9: 1/362880 | `0x37970` | `0x3EC71DE3A556C734` |
| Cosine, degree 2: -1/2 | `0x37948` | `0xBFE0000000000000` |
| Cosine, degree 4: 1/24 | `0x37958` | `0x3FA5555555555555` |
| Cosine, degree 6: -1/720 | `0x37968` | `0xBF56C16C16C16C17` |
| Cosine, degree 8: 1/40320 | `0x37978` | `0x3EFA01A01A01A01A` |

These describe degree-nine sine and degree-eight cosine correction families around the selected phase. The native calculation additionally retains reduction and summation corrections. Treating it as an exact real polynomial followed by one final rounding does not capture those intermediate rounding effects. This bounded contract records the constants, partitions, scalar precision and representative results; it does not provide the entire table or a complete bit-exact evaluation contract for every rounded combination.

## Source-derived numeric anchors

The following values were evaluated statically from the DLL's extracted constants and inspected binary64 arithmetic. The DLL was not loaded or called. Each hexadecimal column denotes the full binary64 encoding, not bytes in file order. Both signs of each listed nonzero input produced the same listed result in this analysis.

| Input | Input bits | Result bits | Approximate result |
| --- | --- | --- | --- |
| Either signed zero | `0x0000000000000000` or `0x8000000000000000` | `0x3FF0000000000000` | 1 |
| 2^-252 | `0x3030000000000000` | `0x3FF0000000000000` | 1 |
| 2^-26 | `0x3E50000000000000` | `0x3FEFFFFFFFFFFFFF` | 0.99999999999999989 |
| 0.5 | `0x3FE0000000000000` | `0x3FEC1528065B7D50` | 0.87758256189037276 |
| 1 | `0x3FF0000000000000` | `0x3FE14A280FB5068C` | 0.54030230586813977 |
| Nearest-binary64 π/2 | `0x3FF921FB54442D18` | `0x3C91A62633145C07` | 6.123233995736766e-17 |
| One quarter of the client's angle constant | `0x3FE921FB5A7ED197` | `0x3FE6A09E6217A6A9` | 0.70710677298230451 |
| One half of the client's angle constant | `0x3FF921FB5A7ED197` | `0xBE58EA91FAE59D9C` | -2.3205103333142417e-08 |
| The client's angle constant | `0x400921FB5A7ED197` | `0xBFEFFFFFFFFFFFF6` | -0.99999999999999889 |

In particular, a fractional coordinate of one half supplies the client's half-constant argument, not nearest-binary64 π/2. Its small negative cosine follows from that input difference. Substituting mathematical π at the caller boundary changes the native numeric behavior even if the cosine operation itself is otherwise highly accurate.

## x87 fallback and unresolved coverage

A failed control-state check at DLL RVA `0x36F80` reaches a fallback at DLL RVA `0x3711F`. That fallback passes the input to DLL RVA `0x32775`, uses the returned x87 value, stores it to binary64, then returns it through XMM0. The inspected finite-number path includes an x87 `FCOS` instruction at DLL RVA `0x32793`. It can load the control word `0x027F` from DLL RVA `0x60344`; restoration and exception handling involve the helpers around DLL RVAs `0x42581` and `0x4258E`. Restoration occurs before the exported wrapper's final binary64 store, so the original x87 rounding mode can matter at that store.

This establishes that the export is not unconditionally the same SSE2 calculation under every control state. The fallback's complete exception behavior, handler interactions, processor-dependent `FCOS` accuracy and bitwise results are not closed here. The full effect of enabling denormals-are-zero or flush-to-zero, although those bits are ignored by the entry's gate, is also unresolved; the numeric anchors assume they are disabled.

No proof of correctly rounded results, a uniform maximum ULP error, or bitwise equivalence to a platform's standard cosine was established over all finite binary64 inputs in the bounded interval. The complete table and the entire ordered rounding contract remain additional work for a bit-exact replacement. Inputs outside the stated magnitude bound, infinities and NaNs are excluded. Original-game runtime verification and end-to-end noise parity remain separate open gates.

This artifact is held after complete final readback. Earlier held contracts were not edited.
