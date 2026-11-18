# Alpha connected client spawn recovery

**FINAL / STABLE / HELD**

Source is the original x86 `Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses below are RVAs. Scope is the ordinary connected local creature's
position and terrain contact after a world change with no saved visit.

## Placement and simulation entry

Connected activation writes the terrain anchor as 8,396,928, 8,396,928,
and zero at world `+0x8000f0` (`0x1a55f8` through `0x1a560c`).
The absent-visit branch converts this vector to signed 64-bit fixed
coordinates at 65,536 units per block and stores it in the local creature
(`0x06b54d` through `0x06b590`). Its fixed X and Y are both 550,301,073,408
and its fixed Z is zero. This is the center of zone (32,800, 32,800) in
region (512, 512). The common placement tail mirrors the position to
creature `+0x1350` and zeroes the three velocity values at `+0x34`
through `+0x3c` (`0x06b618` through `0x06b686`).

On a matching pass the worker gives the terrain generator the world and
the selected zone's X and Y (`0x06acd1`). Its return path updates the
discovery map when the corresponding entry exists. The traced preparation
path ends its player placement at the zero-height anchor. The subsequent
terrain-dependent Z correction belongs to creature simulation.

The frame compares the requested and active seed and name at `0x08cea8`
through `0x08cec9`. Matching identities permit world simulation
(`0x08cf2e`, `0x08cf49`) even if the creature's terrain is still absent.
A frame can run several world steps. The separate location and map lookups
later in this frame path follow the simulation calls.

## Upward terrain correction

Before applying ordinary position increments, the simulation calls
`0x20ab30` at `0x21b47e` for the connected world's local creature
(world pointer `+0xb8`) when the actor byte at `+0x60` differs from 6.
The gate compares that single byte at `0x21b475`. Equality skips the call
and continues at `0x21b483` (`0x21b479`). The correction helper exits
early for negative health at creature `+0x16c`. It runs on eligible
world steps and can correct any starting height.

The terrain check spans the body box centered on the current position.
For each axis, multiply its binary32 extent (creature `+0x80`, `+0x84`,
or `+0x88`) by 0.5, then by 65,536 in binary32, and truncate toward zero
to get the fixed half extent. Convert each fixed corner to whole cells by
dividing toward zero by 65,536 and decrementing the quotient for a
strictly negative fixed input (`0x02f040`). Both endpoints are included.
The check visits X, then Y, then Z with Z innermost
(`0x20ab70` through `0x20aff3`).

The terrain check skips cells in an absent region or absent zone slot
(`0x20aeae`, `0x20aee4`). For present terrain it counts a cell as solid
if the low five bits of byte 3 give a kind other than 0 or 2. A sample
below the column's stored span uses the solid kind-1 fallback
(`0x20af69` through `0x20afbb`, initializer `0x2fb020`).

If the box overlaps solid terrain, take its fixed bottom Z, which is the
current center Z with the fixed half height subtracted. Convert that
bottom to a whole cell by the same signed rule as the corners. Convert
the whole cell back to fixed units and add the fixed half height and
72,089 fixed units. The latter is the truncation of the binary32 constant
with bits `0x478ccccd`. Store the result as center Z and zero the dword
at creature `+0x1180` (`0x20b934` through `0x20b9f7`). The terrain
correction preserves X and Y and writes once at most in that call. Further
eligible steps can lift the creature again if it still overlaps solids.

## Ordinary movement and support

Ordinary movement tests terrain for each proposed position increment
(`0x21b51b`, `0x21b670`, `0x21b677`). Its column reader
`0x02f7e0` supplies a solid kind-1 fallback for missing columns
(`0x02f7f7`, initializer `0x2f9750`). Proposals through those samples
therefore reach the ordinary solid-collision handling. The prior upward
correction requires an actual loaded zone sample.

The usual contact pass clears the old contact flags at creature `+0x5c`
(`0x21cceb`). In its dry support branch, it samples the horizontal footprint
0.2 blocks below the current bottom and sets bit 0 when a sample is solid
(`0x21d1df` through `0x21d2b6`). It uses the same column reader and
fixed-to-whole conversion for that support level.

## Evidence limits

Static tracing establishes the placement, readiness condition and incremental
terrain correction. The first recovered height and number of steps depend
on available terrain and creature state. Live timing, special teleports
and the separate object-collision response remain open.
