# Alpha indexed static model 10: chest base and lid

FINAL / STABLE / HELD. Independent source-side record, 2026-11-21.

## Provenance and inputs

The original 32-bit Alpha Cube.exe has SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All routine addresses below are RVAs for the preferred image base 0x400000.
The original client instruction listing and scoped function exports were
checked against this executable. The owner's original data1.db has SHA-256
`0e482f6d628414db017d983f62843e5dbdc52c5b54fd478f6f0e06c786639b9d`.
Its hash was checked first. The two model rows named below were examined
using an immutable, read-only connection. Only geometric facts are recorded.

| Part | Indexed slot | Resource ID | Original model | Grid X, Y, Z |
| --- | ---: | ---: | --- | --- |
| Base | 10 | 2490 | chest-base02.cub | 23, 16, 10 |
| Lid | 11 | 2491 | chest-top02.cub | 23, 16, 8 |

The resource setup at 0x118811 and 0x11885B associates the two names with
these IDs. At 0x062D1C through 0x062D5E the client fills the slots in its
indexed pointer table at +0x800724. Original loader 0x0E7290 reads each
model's three dimensions in order; 0x0E7650 places them at model+0x44,
+0x48 and +0x4C. Both decoded header triples agree with their respective
record sizes. The geometry builder at 0x0E7870 visits zero-based voxel cells
and supplies integer face corners at the cell coordinate or its successor.
Vertex packing at 0x066650 preserves X, Y and Z order. The geometric input
to the draw is in model-corner coordinates, with no half-cell centering
already included. A grid dimension describes the span of the cell grid.

## Common placement

RVA 0x0BE760 reads the signed 32-bit kind at record+0x00. For kind 10 it
first constructs a local transform and submits the base at 0x0BEE42. It
then fetches slot 11 directly and returns if that pointer is null. With a
lid present, it starts again with an identity matrix and submits the lid
at 0x0C049A. The record still carries kind 10 throughout the second draw.
The base branch dereferences its slot before reaching the later null check.

Both parts use the following placement, in matrix composition order. Each
operation after the anchor acts in the local coordinate frame produced by
the preceding operations.

1. Translate to the object anchor. The record holds three signed 64-bit
   coordinates at +0x08, +0x10 and +0x18. Add client+0x1D8 to X and
   client+0x1E0 to Y using 64-bit integer addition. Use the record's Z
   directly. Convert each resulting signed integer to binary32 and multiply
   by 1/65536. These are the three render-space anchor coordinates.
2. Apply a uniform scale. Divide the binary32 width at record+0x24 by
   the current model's signed X dimension converted to binary32. The
   primary and secondary calls each calculate their own scale. For these
   resources, both use width divided by 23. The record's +0x28 and +0x2C
   do not supply independent Y and Z scale factors on this path.
3. Rotate about local positive Z. Multiply the signed 32-bit orientation
   at record+0x20 by 90 in integer arithmetic, convert to binary32 and
   multiply by binary32 0.01745329238474369. A positive quarter turn
   carries positive X toward positive Y. The routine uses the numeric
   orientation directly, without a four-way index lookup.
4. Translate by minus half the current model's X dimension, minus half
   its Y dimension, and zero in Z. Both models therefore receive
   (-11.5, -8, 0) in their scaled, oriented frame. This uses full grid
   dimensions and no Z half-height. The base transform is complete here.

The base placement occupies 0x0BE7B5 through 0x0BED3F. The independent
lid placement starts at 0x0BEE60 and reaches the end of horizontal
centering before 0x0BF8E0. The identity initializer is 0x023E70.

## Lid hinge

After horizontal centering, the lid appends three operations in this order:
translate by (0, 0, +9), rotate by the opening angle about local positive X,
then translate by (0, 0, -1) along the rotated axis. The +9 and -1 are
literal model units. Neither is taken from the base or lid height. The X
rotation leaves X fixed, turns positive Y toward positive Z and turns
positive Z toward negative Y. The object's quarter-turn orientation and
uniform scale carry this entire hinge into render space.

For example, consider an uncentered lid vertex (x, y, z). Before the common
scale, quarter turn and anchor, its coordinates are

- X equal to x minus 11.5.
- Y equal to -8 plus y times the cosine of the opening angle, minus
  (z minus 1) times the sine.
- Z equal to 9 plus y times the sine, plus (z minus 1) times the cosine.

The pivot line is therefore y = 0, z = 1 in the original lid grid. It lies
at y = -8, z = 9 in the horizontally centered object frame. With the lid
closed, its grid origin sits at (-11.5, -8, 8), and the base's grid
origin sits at (-11.5, -8, 0). As the lid opens, its far positive-Y edge
rises. Its origin moves to (-11.5, -8 + sin(angle), 9 - cos(angle)).
This example describes the geometry; the client composes binary32 matrices
in the order above. The hinge branch occupies 0x0BFC5C through 0x0BFF99.

## Closed, moving and open states

Record+0x30 supplies a state byte. Record+0x34 supplies signed 32-bit
elapsed time. Convert elapsed time to binary32 and multiply by binary32
0.0010000000474974513. With state zero, use this result capped above at
1. With any nonzero state, subtract it from 1 and clamp the result from
below to zero. Multiply the resulting fraction by 80 degrees, then by
binary32 0.01745329238474369 for the trigonometric routines.

| Elapsed time | State zero | State nonzero |
| ---: | --- | --- |
| 0 | Closed, 0 degrees | Open, 80 degrees |
| 500 | Opening, 40 degrees | Closing, 40 degrees |
| 1000 and above | Open, 80 degrees | Closed, 0 degrees |

The two branches have one-sided caps. A negative elapsed value can produce
an angle below zero in state zero or above 80 degrees in a nonzero state.
The base never reads these animation fields to place its mesh. Both yaw and
hinge sine and cosine take a binary32 radian value widened to binary64 and
narrow their results to binary32 for composition.

The visited static-record update at 0x20D3B0 through 0x20D3C6 adds the
update's time increment to the record's elapsed field. Kind-10 population
at 0x1D30EA through 0x1D3196 can set its state byte to 2, which the
renderer treats as nonzero. The state-change helper at 0x1A58E0 compares
the requested byte to the stored byte. On a change, it stores 1000 minus
the previous signed elapsed value, clamped from below to zero, then stores
the requested byte. An identical requested byte leaves those fields alone.
Reversing between zero and nonzero midway through a normal 1000-unit
transition in this way preserves the angle up to binary32 rounding.

## Limits

The contract gives the local placement and source dimensions for the two
identified original resources. The draw caller supplies the render matrices
and per-instance position, orientation and width. World generation, all
interaction or replication paths that write the state, material and fade
state, and the point at which a submitted mesh reaches the screen fall
outside this record. The source and model checks establish static behavior
for this image and content store. No runtime visual observation was used.
