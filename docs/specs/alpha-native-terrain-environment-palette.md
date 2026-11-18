# Native terrain environment palette

**FINAL / STABLE / HELD**

## Supported image and output

This contract covers the original 32-bit Alpha `Cube.exe` with SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs relative to image base `0x00400000`. The call at
`0x0ad7a1` enters `0x19d640` with the world, the unrebased camera
position as three signed 64-bit coordinates with 16 fraction bits, and
six four-float output buffers. Their order is first sky color, second
sky color, fog, front directional color, back directional color, ambient.
The rules below cover RGB and the explicit sky replacement's alpha.

The clock is signed 32-bit milliseconds at world offset `+0x80015c`.
Convert it to binary32, then calculate the absolute value of
`(2 × time) / 86,400,000 − 1` in binary32, in that order. Raise that
magnitude to the third power in binary64 and round to binary32. Subtract
that result from one in binary32, then raise the difference to the fourth
power in binary64 and round to binary32. Call the final result *F*.
This routine does no clock wrapping or clamping
(`0x19d65b`–`0x19d708`).

RGB constants and arithmetic are binary32 unless specified. A weighted
mix from *P* to *Q* with weight *w* means `(1 − w) × P + w × Q`.
A difference blend means `P + (Q − P) × w`. Each operation rounds
separately. Hexadecimal float words in the tables and rules are the
IEEE 754 bit patterns.

## Directional and ambient RGB

The directional colors use the following time knots. For a positive
region sample *D*, each knot is a weighted mix from the default to the
corresponding last-column color for that side, with weight *D*.
At *D* ≤ 0 the default knots apply directly.

| *F* | Default front | Front at *D* = 1 | Default back | Back at *D* = 1 |
| --- | --- | --- | --- | --- |
| 0 | (0.01, 0.02, 0.01) | (1, 0.8, 0.1) | (0, 0.02, 0.03) | (0.7, 0.3, 0.3) |
| 0.5 | (1, 0.9, 0.6) | (0.8, 0.8, 0) | (0.6, 0.5, 1) | (0.8, 0.4, 0.4) |
| 1 | (1.1, 1.1, 0.9) | (1, 0.8, 0.1) | (0.5, 0.6, 0.8) | (1, 0.5, 0.5) |

For *F* ≤ 0.5, mix the first and second knots by `2 × F`.
For *F* > 0.5, mix the second and third by `2 × (F − 0.5)`.
Neither weight is saturated. The positive-*D* knot conversion precedes
the time mix (`0x19e7b5`–`0x19ed19`).

Ambient is (0.6, 0.6, 0.4) when region sample *A* is at least 0.2.
Below 0.2, mix (0.3, 0.4, 0.6) toward (0.6, 0.6, 0.4) by
`A / 0.2`, with no lower clamp (`0x19dce1`–`0x19dd65`).
Ambient RGB depends only on *A*. Front and back RGB depend only on *F*
and *D*. Thus camera Z and the camera-cell type have no effect on these
three outputs.

## Sky and fog RGB

| *F* | Default first sky | Default second sky | Default fog |
| --- | --- | --- | --- |
| 0 | (0.05, 0, 0.1) | (0, 0, 0.1) | (0, 0, 0.1) |
| 0.4 | (1, 0.3, 0.3) | (0.5, 0, 0.5) | (0.5, 0.2, 0.6) |
| 0.6 | (1, 0.9, 0.5) | (0.2, 0.1, 0.8) | (1, 0.9, 0.8) |
| 0.75 | (0.1, 0.6, 1) | (0.01, 0.05, 1) | (0.01, 0.1, 1) |

Only the 0.75 knots receive the following three adjustments. They occur
in the listed order.

1. When *A* > 0.6 and *B* < 0.4, add
   `(1 − B / 0.4) × ((A − 0.6) / 0.4)` times the RGB deltas
   (`0x3f4a3d71`, `0x3f199999`, `0xbe4ccccc`) to the fog knot
   (`0x19d935`–`0x19d9d8`).
2. When *A* > 0.3 and *B* > 0.6, evaluate *C*. The weight is
   `(1 − C) × (((B − 0.6) × u) / 0.4)`, where *u* is the smaller of
   1 and `((A − 0.3) / 0.7) × 4`. Add that weight times the deltas
   (0.4, `0x3e999998`, 0) to the first sky knot and the deltas
   (−0.01, `0x3f0ccccd`, `0xbdccccd0`) to the second. Use a difference
   blend toward (0, 0.7, 0.5) with the same weight for fog
   (`0x19d9d8`–`0x19dbce`).
3. When *B* < 0.4, difference-blend the first sky knot toward
   (0.6, 0.8, 1) and the second toward (0.1, 0.4, 1) with weight
   `1 − B / 0.4` (`0x19dbd6`–`0x19dce1`).

When *D* > 0, apply a weighted mix with weight *D* from the resulting
knots to the targets below (`0x19dda2`–`0x19e23b`). The two sky knots
at zero are used directly.

| *F* | First sky target | Second sky target | Fog target |
| --- | --- | --- | --- |
| 0 | — | — | (0.3, 0.2, 0.1) |
| 0.4 | (0.8, 0.1, 0) | (0.2, 0.2, 0.2) | (0.3, 0.1, 0) |
| 0.6 | (0.8, 0.1, 0) | (0.2, 0.2, 0.2) | (0.3, 0, 0) |
| 0.75 | (1, 0, 0) | (0.4, 0.2, 0.2) | (0.3, 0, 0) |

For *F* ≤ 0.4, mix the first and second knots by `F / 0.4`.
For 0.4 < *F* ≤ 0.6, mix the second and third by `(F − 0.4) / 0.2`.
For 0.6 < *F* ≤ 0.75, mix the third and fourth by `(F − 0.6) / 0.15`.
For *F* > 0.75, use the fourth knot directly. These divisions use the
listed binary32 literals and the weights are unsaturated
(`0x19e23b`–`0x19e75f`).

The final camera-cell test replaces both skies when the low five bits
of the sampled flags byte equal 2. Their RGB becomes
`(0.1 × F, 0.15 × F, F)` and alpha becomes 1. The first two products
use binary64 literals and operands, then round to binary32. Fog keeps
its ordinary result (`0x19e764`–`0x19e7ae`).

## Horizontal region samples

The producer divides raw camera X and Y by 65,536 toward zero and uses
the low 32 bits of each quotient as signed whole coordinates *x* and *y*.
It calls *A* at `0x19d8f2`, *B* at `0x19d92d`, *C* at `0x19da51`
only under the second sky rule, and *D* at `0x19dd9d`.

| Sample | Routine | Quantity being averaged | Fallback |
| --- | --- | --- | --- |
| *A* | `0x1c4dd0` | Binary32 at region `+0x10` | 0.5 |
| *B* | `0x1c4800` | Binary32 at region `+0x0c`, followed by the correction below | 0.5 |
| *C* | `0x1f0720` | Weighted share with signed dword at region `+0x18` below zero | 0 |
| *D* | `0x1ef040` | Weighted share with byte at region `+0x08` equal to 1 | 0 |

All four use the 1,024 by 1,024 table of 32-bit region pointers at world
`+0x4000bc`, with index `1024i + j` for region X and Y respectively.
For each axis the inclusive range is the whole axis shifted by
−16,384 and +16,384 respectively, with each bound divided by 16,384
toward zero. Find these bounds from the original whole coordinates
before any distance jitter. Traverse increasing X with increasing Y
inside each X. An out-of-range index, missing region record, or absence
of a positive weight sum returns the sample's fallback. *C* and *D*
can return zero early when there are no respective matches.

For the distances, jitter X by the noise sample at `(y × 0.0005, 3423)`
and Y by the sample at `(x × 0.0005, 23421)`. The noise arguments
use binary64. Multiply each noise result by 3 and then 256 in binary32,
add the corresponding whole coordinate converted to binary32, and
truncate to signed 32-bit. Each region center is signed 32-bit X and Y
at `+0` and `+4`. Form each difference in signed 64-bit fixed units,
convert it to binary32 and multiply by `2⁻¹⁶`. Square each axis and add
in binary32, then truncate the result to signed 32-bit to obtain the
squared distance *d*. The first pass finds the smallest distance
*dₘᵢₙ*, retaining the first region on a tie. In the second pass each
weight is `1 − min(1, float32(d − dₘᵢₙ) × k)`. The difference is signed
integer and *k* is binary32 word `0x350637bd` at `0x31dcdc`. The
arithmetic passes in *A* are at `0x1c4f53`–`0x1c50b6` and
`0x1c50e0`–`0x1c5267`.

Accumulate weights and weighted values separately in binary32, then
divide the numerator by the sum of all weights. For *C* and *D* the
numerator sums only weights whose records satisfy the table predicate.
The numerator and division sites are `0x1f0bde` and `0x1f0c4c` for *C*,
`0x1ef4fe` and `0x1ef56c` for *D*. For *A* they are `0x1c5225` and
`0x1c5267`, and for *B* they are `0x1c4c68` and `0x1c4cbc`.

### Deterministic noise and the correction to *B*

The value noise at `0x0c0ef0` uses the four lattice corners at the
truncated binary64 axes and their immediate successors. At corner
coordinates *i*, *j*, the seed is `i + 57j`, XORed with itself shifted
left by 13. With *n* denoting that XOR result, the hash is
`n × (n² × 0xec4d + 0x131071f) + 0xd208dd0d` modulo 2³².
Mask the result to 31 bits and form the corner value by subtracting
the masked integer times 2⁻³⁰ from one. Each axis's blend weight is
`(1 − cos((coordinate − trunc(coordinate)) × 3.1415927)) / 2`.
The cosine multiplier is the exact binary64 word
`0x400921fb5a7ed197`. Mix the corners along X and then Y in binary64
and round the result to binary32. This function consumes no random draws.

After a successful weighted average for *B*, the native code looks up
a cell using *x* and *y* divided by 2,048 toward zero (`0x1c4ceb`).
Each index *hₓ*, *hᵧ* must be 0 through 8,191. `0x087da0` selects
the parent region at those indices divided by eight from the world's
first region table at `+0xbc`. In the parent, the cell is at
`+0x14018` plus `0x68 × (8 × (hₓ mod 8) + (hᵧ mod 8))`.
If *B* is below 0.2 and the cell exists with signed dword `+0x18`
equal to 3, add 0.3 times the square of the positive part of `1 − R`
and cap the result at 1. *R* is the shape ratio obtained at `0x1c4d50`.

For this kind of cell, radius is binary32 at `+0x10`. The ratio is
zero if radius is below 0.001. Otherwise take the original whole
query coordinates in signed fixed units, *X* and *Y*, and calculate
*U* and *V* by multiplying each by −0.0025 in binary64, then truncating
to signed 64-bit. The X and Y perturbations respectively use these
pairs of inputs to the same noise function, with each difference
formed as signed 64-bit then converted to binary64 and scaled by 2⁻¹⁶.

| Axis being perturbed | First noise input before scaling | Second noise input before scaling |
| --- | --- | --- |
| X | `0x00000080ad580000 − U` | `0x00000001617d0000 − V` |
| Y | `0x000000000d5f0000 − U` | `0x0000000000700000 − V` |

For each perturbation multiply the noise result by 256 then 65,536 in
binary32, and truncate toward zero to obtain the signed 64-bit fixed
displacement. The shape center is signed 64-bit fixed X and Y at
`+0` and `+8`. Subtract the center from the perturbed query in
fixed units, round each difference to binary32 and multiply by 2⁻¹⁶.
The ratio is the sum of their squares divided by the radius squared,
with binary32 operations (`0x1f8e10`, `0x1f9092`–`0x1f92b5`).

## Camera-cell sample and renderer inputs

The camera-cell lookup at `0x19d715` calls `0x02f860` with the default
area argument zero. For each negative raw axis, subtract 65,536 before
dividing by 65,536 toward zero. For example, an exact coordinate of
−1 becomes −2. Use each quotient's low 32 bits as a signed whole
coordinate. The area lookup accepts whole X and Y from 0 through
16,777,215 inclusive.

The world region table at `+0xbc` uses the same X-outer layout as the
color table. Index it by whole X and Y divided by 16,384. The area
indices are whole X and Y divided by 256. In a region the area pointer
table is at `+0x10018`, with index `64 × (areaX mod 64) + (areaY mod 64)`.
The address of a column is the pointer at area `+0xa8` plus
`0x20 × ((X mod 256) + 256 × (Y mod 256))`
(`0x0347a0`, `0x034a90`, `0x034d10`).

The column's signed base Z is at `+0x10`, its four-byte cell array
pointer at `+0x18`, and its count at `+0x1c`. Within the span,
the stored cell index is whole Z minus base Z and the flags byte is
at cell offset `+3`. The lookup at `0x02f7e0` selects the effective
flags as follows.

| Case | Effective flags |
| --- | --- |
| Missing column or Z below the base | `0x01` |
| Z at or above base plus count, with Z > 0 | `0x00` |
| Z at or above base plus count, with Z ≤ 0 | `0x82` |
| Inside the span, with low five stored bits zero, Z ≤ 0 and bit `0x40` clear | `0x82` |
| Other in-span cell | Stored flags byte |

The default cells' flags are initialized at `0x2f9720`, `0x2f9750`
and `0x2f9780`. The frame caller independently tests the same low
five bits for 2 at `0x0ad6e2`–`0x0ad750`. That test changes the RGB
for vertex constant `c32` from (1, 1, 1) to (0.4, 0.5, 1).

The primary terrain path supplies front, back and ambient to vertex
constants `c34`, `c35` and `c38` at `0x0b1a9c` through `0x048170`.
Local point-light positions and colors are uploaded separately in
`c0`–`c9` and `c10`–`c19` from the terrain call at `0x0b1c0b`.
The traced palette route reads the world clock, horizontal region
records and the camera cell. It has no model skin, creature or
local point-light input.

## Verification scope

The documented branches and constants were checked against the matched
original image. A scalar trace of the sky/fog and ambient arithmetic
agreed with these rules over 720 synthetic intermediate inputs covering
all listed time intervals, region-edit decisions and the camera-cell
override. The scope is static behavior for finite native values and
valid record layouts. Region creation and field semantics, pathological
integer overflow or nonfinite fields, other mesh and creature shaders,
and runtime visual acceptance remain unresolved here.
