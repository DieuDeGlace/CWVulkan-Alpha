# Alpha combat terrain lighting

Status: FINAL / STABLE / HELD

This source-side contract covers synchronous terrain lighting after the fixed
Bomb and variable-radius creature combat branches. Evidence is static tracing of
the original x86 images. All routine and instruction addresses below are RVAs.

| Image | SHA-256 |
| --- | --- |
| `Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| `Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

## Fixed Bomb

The branch takes a nonnull creature entry with a nonzero byte at `+0x60`,
an ordered float at `+0x16c` at or below zero, a zero signed dword at
`+0x6c` and a dword of `0x90` at `+0x64`. Its center is the creature's
three signed 64-bit fixed coordinates at `+0x10`. This branch is independent
of the world byte at `+0xb4`. The entry tests are at client `0x20eb9b`
through `0x20ebdf` and dedicated `0x13495b` through `0x13499f`.

The integer offset range on each axis is −8 through +8, inclusive. Evaluation
is X outermost, then Y, then Z. Candidates retain the center's fixed-point
fraction as they add the offset in whole blocks. The squared offset distance
must be strictly below 64. Every accepted candidate passes the terrain
predicate and replacement described below. The scan starts at client
`0x20ec9a` and dedicated `0x134a5a`.

## Variable-radius creature combat

The creature's signed 32-bit action timer at `+0x6c`, as it stands at the
terrain branch, must be strictly above the selected lower bound and at or
below the selected upper bound. The timer test is at client `0x2137f7` and
dedicated `0x1395b7`. It compares the timer on that update. The bounds use the
current action byte at `+0x68`. Phase start comes from client `0x03caa0` and
dedicated `0x007db0`. Active duration comes from client `0x047310` and
dedicated `0x011d60`. The signed integer phase arithmetic below uses division
toward zero.

- The ordinary lower bound is the larger of phase start and phase start plus
  half the duration minus 100. The upper bound is phase start plus half the
  duration, limited to one less than phase start plus duration.
- For `0x44` and `0x45`, the lower bound is phase start plus one third of the
  duration. For `0x5d`, the lower bound is phase start plus one fifth of the
  duration and the upper bound is phase start plus the full duration.
- For `0x48`, `0x56`, `0x1e`, `0x1f`, `0x20`, `0x21`, `0x0b`,
  `0x05` and `0x5b`, the window is recalculated from the timer before its
  final comparison. If that earlier timer is below phase start or at least
  phase start plus duration, both bounds are phase start. Inside that phase,
  the lower bound is phase start plus the elapsed time rounded down to its
  interval boundary. The upper bound is 100 above that lower bound. The
  interval is 250 for `0x48` and `0x56`, 1000 for `0x5b`, one third of the
  duration for `0x0b` and `0x05`, one sixth for `0x1e` and `0x20`, and
  one twelfth for `0x1f` and `0x21`.

Let the size input be the first binary32 component at creature `+0x80`.
The destruction scan also requires an ordered size greater than 4, a zero
world byte at `+0xb4`, and an action other than `0x57`, `0x5b` or `0x4a`.
The client sets the world byte when starting a remote connection
(`0x06fca7`). The destruction checks start at client `0x213b12` and
dedicated `0x1398d2`.

The default radius and the default forward distance are each 1.5 times the
size. Apply the following action overrides in binary32.

| Action bytes | Radius | Forward distance |
| --- | --- | --- |
| `0x01`, `0x02`, `0x03`, `0x04`, `0x06`, `0x07`, `0x09`, `0x0d`, `0x0e`, `0x12`, `0x13` | Twice the size | The size |
| `0x14`, `0x15` | Twice the default radius | Default |
| `0x44`, `0x46` | Default | Zero |
| `0x56`, `0x5d`, `0x68` | Twice the default radius | Zero |
| `0x58` | 8 | Zero |
| `0x1e`, `0x20` | 3 | Zero |
| `0x1f`, `0x21` | 5 | Zero |
| `0x48`, `0x4c`, `0x4d`, `0x4e` | Default radius times the binary32 value 1.2 | Zero |
| `0x0b`, `0x36`, `0x3d` | Three times the default radius | Center reset to creature position |

The ordinary center is the creature's fixed position plus the forward
distance along its yaw-rotated positive Y axis. Yaw in degrees is the third
binary32 component starting at `+0x28`. The angle conversion multiplies by
the binary32 value with bits `0x3c8efa35`. The resulting angle is widened for
sine and cosine, and their results are rounded to binary32. The rotated
float offset is multiplied by 65,536 per component. Each product is truncated
toward zero to a signed 64-bit fixed offset and added to the position.
The source rotates only around Z. Actions `0x1e`, `0x1f`,
`0x20` and `0x21` also add the three binary32 values at `+0x160`, with
that vector converted independently the same way. For `0x45` and `0x4b`,
subtract the first extent component at `+0x80` from the third at `+0x88` in
binary32. Multiply the difference by 0.5 in binary32, then multiply that
result by 65,536 in binary32. Truncate the product toward zero to a signed
64-bit fixed value and subtract it from the center's fixed Z. The extent
arithmetic starts at client `0x213a93` and dedicated `0x139853`. The fixed
subtraction helpers are client `0x02ca00` and dedicated `0x002eb0`.
Actions `0x0b`, `0x36`
and `0x3d` finish with the creature position as center. These center and
radius choices are at client `0x21380e` through `0x213b12` and dedicated
`0x1395ce` through `0x1398d2`.

For the mutation scan, truncate the resulting float radius toward zero and
cap that integer at 8. The integer offsets run from its negative to its
positive value, inclusive, in the same X, Y, Z order as Bomb. The squared
distance must be strictly below the square of this capped integer radius.
The squared distance compares binary32 components derived from the fixed
difference between candidate and center. The float radius remains the input
for the horizontal light bounds.

## Terrain predicate and synchronous light call

Each candidate that passes the strict sphere test queries existing terrain
with the final lookup argument zero. Both branches require bit `0x20` in
byte 3 of the returned terrain record. The accepted candidate invokes the
existing-cell replacement. The scan then records its acceptance even though
the replacement has no success result. The replacement itself requires a
loaded column and Z within the stored span. It passes the ordinary empty
record for positive whole Z and the shallow-water record otherwise to the
column writer at client `0x04e700` and dedicated `0x01fe60`. The writer
tests the destination's byte 3 for protection bit `0x80`. With the bit
clear, it copies all four replacement bytes as supplied. With the bit set,
it declines a replacement whose byte 3 has zero in the low five bits.
For a nonzero kind it copies the record and preserves the protection bit.
Empty has kind 0 and shallow water has kind 2. A protected positive-Z cell
therefore declines the clear even after the scan accepts its candidate.
The predicate calls are client `0x20ed98`, `0x213c94` and dedicated
`0x134b58`, `0x139a54`. The replacement calls are client `0x20eecd`,
`0x213dd0` and dedicated `0x134c8d`, `0x139b90`.

The fixed point terrain query has its own negative-coordinate convention.
For each strictly negative coordinate it subtracts one block before
dividing toward zero by 65,536. Both replacements first divide every
coordinate by 65,536 toward zero. The Bomb then decrements the whole Z
when the original fixed Z is strictly negative. The terrain query helpers
are client `0x02f860` and dedicated `0x006050`.
The Bomb's Z conversion is client `0x02f100` and dedicated `0x005510`.

If the scan accepts a candidate, the branch makes one CRT random draw for
its destruction sound and queues that sound before checking the world
pointer at `+0xb8`. A zero pointer permits the immediate light call. A
nonzero pointer skips the call while the subsequent region update still
receives the same four base bounds. The client assigns its local creature
to this world field at `0x087e90` and `0x070408`. On the dedicated startup
path, the constructed world starts with a zero `+0xb4` byte and a null
`+0xb8` pointer (`0x149cbd`, `0x0c867c`, `0x0c8682`). The same world
receives the simulation tick at `0x14a0ae`. Dedicated therefore has the
matching synchronous light writes under the same acceptance and null-pointer
gates.

| Branch | Client light gate and call | Dedicated light gate and call |
| --- | --- | --- |
| Fixed Bomb | `0x20efe4`, `0x20f071` | `0x134da4`, `0x134e31` |
| Variable-radius combat | `0x213efb`, `0x213fc0` | `0x139cbb`, `0x139d80` |

## Exact horizontal bounds

Each base interval is built from the fixed center coordinate itself. For
Bomb, subtract 524,288 fixed units for the lower bound and add 524,288 for
the upper bound, on X and on Y. The variable branch multiplies its full
binary32 radius by binary32 65,536, truncates that product toward zero to
a signed 64-bit fixed distance and uses the distance on both horizontal
axes. In each branch, convert each fixed endpoint separately by signed
division by 65,536 toward zero, taking the low 32-bit whole result.
The conversion is client `0x02f570` and dedicated `0x005640`.
For example, a center at +0.5 blocks with radius 8 yields lower −7 and
upper 8 on that axis.

Both call sites pass these base bounds in lower X, lower Y, upper X, upper Y
order, then margin 8 and lookup argument zero. The immediate relighter at
client `0x19a0e0` and dedicated `0x0d1a70` uses half-open intervals on each
axis. It subtracts the margin from the lower bounds and adds it to the
exclusive upper bounds for sky initialization and sixteen propagation
rounds. Column lookups can cross loaded chunk boundaries and skip absent
or out-of-world columns. Each round traverses the stored vertical span of
the padded columns without a Z bound from the caller. The relighter updates
scratch light at bytes 1 and 2 for terrain kinds 0 and 2 during these
passes. Its final copy from byte 2 to the published byte 0 uses only the
unpadded half-open intervals and the same two terrain kinds. The final
copy begins at client `0x19a772` and dedicated `0x0d2102`.

## Evidence limits

The action selection, timing helper internals, other terrain light call
sites, multiplayer delivery and live native acceptance remain open here.
