# Alpha rock/cliff color precision contract

Status: FINAL / STABLE / HELD  
Scope: original client routine RVA `0x1F9620`, its direct noise-result boundary, and direct float-to-color-byte callers.  
Evidence date: 2026-11-07. This document contains original-image findings only.

## Supported image and evidence

The supported original image is `binary/Cube.exe`, SHA-256
`84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`.
Its PE machine is `0x014C`, optional-header magic `0x010B`, and preferred image base
`0x00400000`. All addresses below are RVAs, independent of a runtime relocation.
The principal body occupies `0x1F9620–0x1F9BB1`.

Source-relative citations in this document are relative to the original
`disassembled alpha cube world` directory, not to the replacement project.
`decompiled/client/assembly.txt` supplies the instruction listing. Every listed
instruction byte in the principal body and noise helper RVA `0x0C0EF0` was compared
with `binary/Cube.exe`: 450 instructions, zero mismatches. Constant widths and
bit patterns below were read directly from the PE section mapping. The individually
scoped decompiles `decompiled/client/functions/005f9620.c` and
`decompiled/client/functions/004c0ef0.c` were orientation aids; machine
instructions and bytes determine precision claims. No reconstruction source was
needed.

This is a bounded numeric contract. It does not establish visual acceptance,
the physical meaning or valid range of cached environmental fields, or the complete
noise algorithm.

## Inputs and arithmetic domains

The routine writes three consecutive IEEE binary32 components to the caller's
output buffer and returns that buffer address. The third component, called blue
here, is at output offset `+8`. The three signed 32-bit numeric arguments after
the buffer are denoted \(x,y,z\). The routine also receives a lookup argument
after those coordinates and an object pointer. These names are mathematical
labels for this contract.

Two binary32 environmental inputs affect the result:

- \(a\): a successful lookup's field at offset `+8`, or the return of original
  helper RVA `0x1C4DD0` narrowed into binary32.
- \(s\): a later successful lookup's field at offset `+12`, or the return of
  original helper RVA `0x1EF040` narrowed into binary32.

The cache lookup is original RVA `0x0347A0`. It is performed separately for
these inputs. An intervening lookup can also call RVA `0x1C4800` and discard
that helper's x87 return. This contract does not merge those lookups or specify
their ownership or caching policy.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F96F0–0x1F9738`, `0x1F99CB–0x1F99F4`.]

Coordinate arithmetic uses SSE scalar binary64 operations. Channel arithmetic,
environmental normalization, blending and comparisons use SSE scalar binary32
operations. Each arithmetic instruction produces its specified precision before
the next dependent operation; there is no fused multiply-add and no
extended-precision channel accumulator. Keeping an SSE result in a register
does not increase its precision. A separate memory spill is not required for a
binary32 rounding boundary.

For the formulas below, \(F(v)\) means the binary32 result of the indicated
single SSE operation in the ambient MXCSR environment; \(D(v)\) means the
corresponding binary64 result. Parentheses and every explicit \(F\) or \(D\)
describe numeric boundaries, not an implementation recipe. Signed integer
coordinates convert exactly to binary64 before their first multiply.
The ordinary finite, round-to-nearest interpretation applies only when the
ambient controls actually select that mode.

## Constants are determined by their loaded width

The following are the actual binary32 loads, not unsuffixed decimal
approximations. The integral and dyadic values in this table are exactly
representable.

| Data RVA | Binary32 bits | Value |
| --- | --- | --- |
| `0x345DA0` | `0x3F000000` | 0.5 |
| `0x345DC0` | `0x3F800000` | 1 |
| `0x2FCE58` | `0x42F00000` | 120 |
| `0x2FD594` | `0x43020000` | 130 |
| `0x2FCDC4` | `0x3F400000` | 0.75 |
| `0x345E60` | `0x40800000` | 4 |
| `0x2FD0B4` | `0x42C80000` | 100 |
| `0x2FCE60` | `0x43480000` | 200 |
| `0x2FCD74` | `0x41A00000` | 20 |
| `0x345E10` | `0x40000000` | 2 |
| `0x345E74` | `0x41200000` | 10 |
| `0x303EF0` | `0x43200000` | 160 |
| `0x2FCD7C` | `0x42480000` | 50 |
| `0x2FD598` | `0x432A0000` | 170 |
| `0x2FCE64` | `0x437F0000` | 255 |
| `0x345D84` | `0x00000000` | positive zero |
| `0x2FCB7C` | `0x3DCCCCCD` | 0.100000001490116119384765625 |

Coordinate constants are loaded as binary64. In particular, the 0.1 used in
coordinate preparation is a different representation from the binary32
0.1 in the last row above.

| Data RVA | Binary64 bits | Decimal label |
| --- | --- | --- |
| `0x31A048` | `0x3FB999999999999A` | nearest binary64 to 0.1 |
| `0x31DD70` | `0x3FD999999999999A` | nearest binary64 to 0.4 |
| `0x31DF18` | `0x40F82A8000000000` | 98984, exact |
| `0x302B40` | `0x3F50624DD2F1A9FC` | nearest binary64 to 0.001 |
| `0x31DE68` | `0x40B9900000000000` | 6544, exact |
| `0x31DDC0` | `0x405EC00000000000` | 123, exact |
| `0x345DB8` | `0x3F747AE147AE147B` | nearest binary64 to 0.005 |
| `0x31DB48` | `0x3FA999999999999A` | nearest binary64 to 0.05 |
| `0x31DB38` | `0x3F947AE147AE147B` | nearest binary64 to 0.02 |

The binary proves these load widths and bit patterns. It does not prove the
spelling or declared type of literals in the original source.
[Evidence: `binary/Cube.exe`, data RVAs in the tables; width-bearing instructions
in `decompiled/client/assembly.txt`, principal routine.]

## Noise results and x87 boundaries

Let \(N(u,v)\) denote the result of original helper RVA `0x0C0EF0` for two
binary64 arguments. This symbol does not grant a substitute noise function
equivalence.

That helper prepares its fractions and performs its interpolation with SSE
binary64 arithmetic, with an intervening imported mathematical operation.
After the final binary64 addition, it stores binary64 to memory, loads that
value into x87, stores it to binary32, and reloads that binary32 value for its
x87 return. Thus the returned significand is already binary32 before the
caller sees it. It is not an unconstrained 80-bit noise result.
[Evidence: `decompiled/client/assembly.txt`, RVA
`0x0C0EF0–0x0C10F9`; final boundary `0x0C10E4–0x0C10F3`.]

Every noise return used by the principal routine is again stored by an x87
binary32 store and then loaded for SSE use. For finite ordinary results the
second store cannot recover any discarded bits. The relevant store RVAs are
`0x1F96A7`, `0x1F96BE`, `0x1F9839`, `0x1F990C`,
`0x1F9A41`, `0x1F9A89` and `0x1F9B3D`.
The environmental fallback helpers likewise return their final ordinary
division through a binary32 memory value; their fallback constants are also
exact binary32 values.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1C5267–0x1C5289`, `0x1EF56C–0x1EF58E`;
caller stores `0x1F970C`, `0x1F99F1`.]

The principal routine and the audited noise helper contain no x87 arithmetic
that keeps a wider channel intermediate between multiplies and additions.
Their x87 load/store transport does not justify emulating the color expressions
with extended precision. The noise helper's binary64-to-binary32 x87 store
does, however, use the ambient x87 rounding control, whereas SSE arithmetic
uses MXCSR. The x87 precision-control setting is not an extra significand
width for the SSE instructions or an instruction to pre-round an x87 load.

The imported operation is reached through original thunk RVA `0x28D7A4`,
whose IAT slot at RVA `0x2FC444` imports `MSVCR110.dll` ordinal 13514.
The scoped decompile identifies this operation as the precise SSE2 cosine
helper. Its installed DLL implementation and any internal fallback path
were not audited here. The noise helper multiplies its cosine argument by
the binary64 bit pattern `0x400921FB5A7ED197` at data RVA `0x302B48`,
approximately 3.1415926999999999047, rather than mathematical pi.
Its binary64 interpolation constants include exact 1 and 0.5
(data RVAs `0x345DF8`, `0x345DE8`) and exact \(2^{-30}\)
(data RVA `0x302B20`, bits `0x3E10000000000000`).
[Evidence: `binary/Cube.exe`, import descriptor/IAT and cited data RVAs;
`decompiled/client/assembly.txt`, RVAs
`0x0C0F54–0x0C0FC1`, `0x28D7A4`.]

## Shared tone and blue base

The first noise sample is taken at
\(D(D(y\cdot 0.1)+98984),D(z\cdot 0.4)\).
The second is taken at
\(D(x\cdot 0.1),D(z\cdot 0.4)\).
The decimal labels 0.1 and 0.4 in these expressions mean the binary64 table
entries. The first noise result is saved as binary32 before the second call.

For those already-binary32 samples \(n_1,n_2\), the shared tone supplied to
the channel additions has the numeric value

\(T=F(F(F(F(F(n_1+n_2)\cdot 0.5)+1)\cdot 0.5)\cdot 160)\).

There is an individual binary32 addition, multiply, addition, multiply and
multiply, in that dependency order. The intermediate before multiplication
by 160 is saved as binary32 across the environmental lookups. Algebraically
collapsing the factors or evaluating their sum in binary64 does not preserve
these boundaries.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F9626–0x1F96EB`, `0x1F990F–0x1F9914`.]

For finite \(a\), the blue base is 130 when \(0.5\leq a\leq0.75\).
Below 0.5, the interpolation weight is \(t=F(a\cdot2)\).
The blue base is then
\(F(F(F(1-t)\cdot50)+F(t\cdot170))\).
Above 0.75, the weight is \(t=F(F(a-0.75)\cdot4)\), and the blue base is
\(F(F(F(1-t)\cdot130)+F(t\cdot100))\).
Each complement is rounded before its multiply; both products are rounded
before their addition. No clamp of \(a\) or \(t\) occurs here.
Equality at either boundary retains the middle base.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F973A–0x1F97E1`, `0x1F97EA–0x1F97F6`,
`0x1F9856–0x1F98A4`.]

The upper branch also samples noise for another component. Its first
coordinate depends on the normalized binary32 weight \(t\), widened to
binary64, rather than directly on \(x\). That call occurs before the
common blue-contribution sample described next. Its result does not enter
the blue base.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F97E7–0x1F98B3`.]

## Blue contribution, addition order and clamp

Let \(p_x,p_y\) be the signed 32-bit object fields at offsets
`0x80022C` and `0x800230`. The next noise sample uses
\(D(p_x+D(x\cdot0.005)),D(p_y+D(y\cdot0.005))\),
with 0.005 meaning the exact binary64 table entry. Multiplication rounds
before the field addition. Neither coordinate is prepared as binary32.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F98B3–0x1F990C`.]

For its already-binary32 noise result \(n_b\), the blue-only contribution is
\(U=F(F(n_b+1)\cdot10)\).
The unclamped blue value is

\(B_{raw}=F(F(T+B_{base})+U)\).

This is specifically the sum of the rounded shared-tone/base addition and
the separately rounded blue contribution. It is not a single wider sum,
and the blue contribution is not added to the base first.
The other two components also have explicit addition of the result of
binary32 positive-zero multiplied by binary32 10.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F991C–0x1F995F`.]

Each raw component is written as binary32 and clamped by comparisons:
a value strictly below zero becomes positive zero; a value strictly above
255 becomes exact binary32 255. Finite values at the bounds are retained.
For blue, this produces \(B_c\). The comparison/branch sequence does not
replace an unordered NaN with either endpoint; masked-exception execution
can therefore retain NaN. Negative zero is not strictly below zero.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F9963–0x1F99C4`, blue `0x1F99AE–0x1F99C4`.]

## Last blend is after the clamp

The last environmental input \(s\) is compared with positive zero. Only a
strictly positive, ordered value enters the blend; zero, negative values
and an unordered NaN take the return path. There is no upper clamp of \(s\)
in this routine.

For the blue component, a taken blend has exactly the following value:

\(B_{out}=F(F(B_c\cdot F(1-s))+F(s\cdot10))\).

The complement is a binary32 subtraction, each product is independently
binary32, and their final sum is binary32. The old-blue product is
computed and saved before another noise call; the final binary32
\(s\cdot10\) is added afterward. There is no second channel clamp.
When the blend is skipped, the output is \(B_c\).
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F99F9–0x1F9A05`, `0x1F9AC9–0x1F9AF0`,
`0x1F9B89–0x1F9BA3`.]

Three additional noise calls occur in the taken-blend path. The first two
use coordinate scales binary64 0.05 and 0.02, in that order. Their
already-binary32 results are combined by multiplying the first by
binary32 bits `0x3DCCCCCD`, then adding the second with a separate
binary32 instruction. The result is capped only when strictly above 1,
then its absolute value is formed by widening to binary64, clearing the
sign bit with the mask at data RVA `0x345F50`, and narrowing again.
The complement to 1 is raised to the fifth power by successive dependent
binary32 multiplies: square, then multiply by the original complement
three more times. The third noise uses the previously prepared binary64
\(x\cdot0.1,y\cdot0.1\). These values affect the other channel targets;
the blue target remains exact binary32 10. Thus changing those calls
does not remove their native evaluation order, even though their values
do not enter the final blue blend.
[Evidence: `decompiled/client/assembly.txt`, RVAs
`0x1F9A0B–0x1F9AB9`, `0x1F9AC2–0x1F9AF5`,
`0x1F9B23–0x1F9B89`;
`binary/Cube.exe`, data RVA `0x345F50`: two 64-bit lanes
`0x7FFFFFFFFFFFFFFF`.]

## Final integer and stored color bytes

The principal routine itself returns floats; it performs no final integer
conversion. Direct native storage callers read each binary32 component,
convert it with `CVTTSS2SI` to signed 32-bit integer, and store that
integer's low eight bits. This conversion truncates toward zero,
independently of the rounding direction selected for ordinary SSE
arithmetic. It does not add one half or select the nearest integer.

Consequently a finite in-range component such as 129.99998 produces byte
129, while exact 130 produces byte 130. A one-ulp difference straddling an
integer can affect the byte. For finite results whose truncated integer
fits signed 32 bits but lies outside 0–255, low-byte storage wraps modulo
256; it does not saturate. Such values are not ruled out merely by the
earlier clamp, because the subsequent blend has no final clamp and
accepts \(s>1\) if supplied. For NaN, infinity or signed-32-bit overflow,
masked invalid conversion yields the x86 indefinite integer
`0x80000000`, whose low byte is zero; an unmasked invalid exception
need not reach the store.

Direct conversion sites include RVAs `0x04E840`, `0x1CC480`,
`0x1E6A90`, `0x1E8260`, `0x1E8793`, `0x1E8B30`,
`0x1E9562` and `0x1E9AD0`. These facts concern the direct byte
paths. Other callers first combine or normalize the float colors, so
their eventual byte cannot be inferred by applying this direct
conversion prematurely.
[Evidence: `decompiled/client/assembly.txt`, cited conversion RVAs and
following low-byte stores; examples of retained float paths
`0x09E768–0x09E828`, `0x1C7518–0x1C7570`.]

## Held uncertainties and limits

The principal routine does not set or save MXCSR or the x87 control word.
The audited noise helper also does not establish them. This source-only
audit does not prove the controls active on every terrain-generation
thread, DLL implementation/version, exception-mask state, or denormal
handling in an actual native session. Round-to-nearest, ties-to-even and
matching SSE/x87 rounding modes must therefore be stated as environmental
assumptions, not accepted as verified native defaults.

Under ordinary finite inputs, the instruction widths, stored constants,
dependent operation order, explicit binary32 return boundary and truncating
byte conversion are established. Bit-identical behavior for every input
also requires the external mathematical helper and the ambient floating-point
environment to be established independently. NaN payload propagation and
signaling-exception details are not completed acceptance claims here.

Neither a decompiler's extended-float return annotation nor a high-level
algebraic identity supplies evidence for retaining extended intermediates.
No original runtime capture, replacement implementation, test, probe output
or inherited specification was consulted. This document remains held after
complete readback; further source work requires a separately assigned scope.
