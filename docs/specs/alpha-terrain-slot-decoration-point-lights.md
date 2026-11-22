# Alpha point lights for terrain solids and immediate decorations

FINAL / STABLE / HELD. Original-client analysis, 2026-11-21.

## Image and scope

The owner's Alpha Cube.exe has SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All addresses are RVAs for its preferred base of 0x400000. Evidence includes
the embedded vertex shader and its constant table, original instruction bytes
for the render pass at 0x0AC260, and scoped original functions for candidate
collection, slot assignment and draw submission. Slot and decoration offsets
below refer to distinct objects. A decoration's payload starts at node+0x08.

## Which state reaches the draw

The solid terrain entries and immediate decorations of a sorted slot use
the same two point-light uploads. The uploads occur before the solid entries.
The transition from the last solid entry to the decoration list performs a
range test and starts an iterator. It makes no additional point-light write.
A later slot that passes the solid-list gate replaces both arrays.

| Vertex constants | Meaning | Data at the slot | GPU upload |
| --- | --- | --- | --- |
| c0 through c9 | pointLightPositions | +0x78, four binary32 values per entry | XYZ render-space position and W radius, 10 entries |
| c10 through c19 | pointLightColors | +0x178, three binary32 values per entry | RGB with W zero, 10 entries |

The shader initialization at 0x047E73 through 0x047EF4 supplies starting
registers 0 and 10. The embedded shader constant table at RVA 0x2FD928 names
both arrays and gives each a length of ten. The uploader at 0x048F10 first
submits ten position vectors. It expands all sixteen CPU color triples to
four-component vectors with a zero fourth component, then submits the first
ten. Both writes are vertex float constants. The CPU slot can hold sixteen
lights even though this shader receives at most ten.

RVA 0x0B1BB4 takes the slot's critical section at +0x250. An empty solid
list at +0x08 skips the slot, including its decoration list, with the previous
GPU point-light state still present. Otherwise it sets the initial material
color, submits the two slot arrays at 0x0B1C0B and sets pixel alpha to 1.
This happens even if every solid entry subsequently lacks an index buffer.
The per-solid submissions at 0x0B1C60 through 0x0B1DD8 change transforms
and geometry bindings. They leave the two light arrays in place.

The distance test for immediate decorations at 0x0B1DDE through 0x0B1E54
can skip the entire +0x240 list after the arrays have already been installed.
A rejected decoration, an entry queued for the deferred pass, or the separate
type-60 effect path likewise leaves c0 through c19 alone. An immediate model
passes through 0x0B2801 to the draw routine at 0x0BD160, using the model table
at client+0x800718. A nonnull model's final submission at 0x0BE690 through
0x0BE725 sets pixel alpha, the three vertex transforms and material color,
then binds and draws the mesh. It does not upload point lights. In this shader
the transforms occupy c27 through c29, c24 through c26 and c20 through c23,
respectively. Material color is c32. The alpha setter writes pixel c3.

An empty slot changes no light register. An eligible slot with zero assigned
lights uploads zero-filled vectors, replacing the preceding slot's values.
When the sorted loop ends, the call at 0x0B21C2 uploads the frame-level
arrays, including when the loop had no entries. This write precedes the
separate zone traversal. Deferred decorations have no entitlement to the
point-light state of their source slot from the earlier immediate pass.

## Source and assignment of the arrays

During the earlier slot traversal, the +0x240 decoration list supplies one
possible source of light candidates. Bit 0 in the byte at payload+0x38
qualifies a decoration for the visibility query at 0x0AC76B through
0x0AC791. Its center is the record's fixed XYZ at payload+0x08, +0x10 and
+0x18. The query receives radius 16 and range equal to client+0x1D4 plus
binary32 22.4. The separate slot-key test against 4096 follows this branch
and does not gate the light candidate.

For each passing flagged decoration, increase a per-slot phase accumulator
by 33. It starts at zero before that slot's first record. Use the unshifted
record position as the candidate center. Its radius is 15 plus half the
cosine of the phase. The cosine argument is the binary32 sum of the signed
client clock at +0x8003A0 multiplied by binary32 0.01 and the accumulator
converted to binary32. A binary64 cosine is narrowed before scaling.

The candidate's color comes from payload+0x2C, +0x30 and +0x34. Kind 13
multiplies all three by a time-of-day factor. From the signed day clock
at client+0x800440, form twice the clock divided by 86400000, less 1,
in binary32. Take its fourth power in binary64 and narrow. Subtract that
value from 1 in binary32, take the fifth power in binary64 and narrow.
The color multiplier is 1 minus half this last value in binary32.
The candidate record also carries a squared-distance key from its center
to client+0x140. Its fixed-point dot product divides each 64-bit component
product by 65536 before summing, then converts the sum to binary32 and
multiplies by 1/65536. The pass appends the completed 0x30-byte record at
0x0ACAA6. Frame clock shaping is at 0x0AC3E1 through 0x0AC44A.

Other scene sources also enter the frame candidate collection. At 0x0B0E84
the renderer sorts the combined collection by increasing candidate distance
key at +0x28. At 0x0B0E90 through 0x0B0F82 it clears every physical slot's
+0x238 count and all sixteen position and color entries. For each candidate
in sorted order, the addressed XY cells extend inclusively from the center
minus radius to the center plus radius on each axis. The binary32 radius
is converted to fixed units, the endpoints are divided by 65536 to whole
world units, then by 32 to cell indices, with signed truncation toward zero.

Within the active grid and legal cell coordinates 0 through 524287, the
client maps the cell to its physical slot using each axis modulo grid width,
with X varying fastest in storage. Each slot accepts its first sixteen
candidates covering it. It stores position plus client XY render offsets
(+0x1D8, +0x1E0) and zero Z offset, converted to binary32 world units.
The fourth value is the candidate radius. It copies the RGB triple and
increments the count. This assignment at 0x0B0FC0 through 0x0B12D9 can
give a decoration lights from other objects and cells. The slot's ten GPU
entries are the first ten from this ordering, padded by the earlier clear.

## Limits

The evidence establishes register writes, data layout, order and persistence
for ordinary finite inputs on this client. It does not enumerate every other
scene light producer or establish a stable order among equal candidate keys
for every input size. The renderer's later passes and individual pixels were
outside this examination. No runtime capture was used.
