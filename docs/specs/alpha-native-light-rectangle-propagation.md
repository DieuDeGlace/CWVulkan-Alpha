# Native light rectangle propagation

**FINAL / STABLE / HELD**

## Supported image and inputs

This contract covers the original 32-bit Alpha Cube.exe with SHA-256
84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf.
Addresses are RVAs relative to image base 0x00400000. The entry
at 0x19a0e0 takes a world, a half-open destination rectangle in
signed whole-cell X and Y, an integer working margin, and an optional
explicit area pointer. The signed coordinate arguments are the lower
X, lower Y, upper X and upper Y in that order, followed by the margin
and area. For a margin of *p*, subtract *p* from each lower bound and
add *p* to each upper bound to get the working rectangle
(0x19a0e6–0x19a125). Each visit covers the stored vertical span of
the selected column.

All column lookups first require X and Y in 0 through 16,777,215.
With a null explicit area pointer, the lookup at 0x0347a0 finds
the resident area at whole X and Y divided by 256. The area lookup
at 0x034d10 uses a pointer table at region +0x10018 with index
64 × (areaX mod 64) + (areaY mod 64). The lookup at 0x034a90
selects the region from the world pointer table at +0xbc with index
1,024 × regionX + regionY, where region indices are area indices
divided by 64.

With an explicit area pointer, the bounds come from that area's two
signed indices at +0x60 and +0x64, multiplied by 256. Only the
256-by-256 square beginning there can return a column. Queries
crossing that square return a missing column, including when the
adjacent area is resident (0x034806–0x034867).

The column address is the pointer at area +0xa8 plus
0x20 × ((X mod 256) + 256 × (Y mod 256)). The column stores a
signed base Z at +0x10, a pointer to four-byte cells at +0x18,
and the stored cell count at +0x1c. For an in-span Z the cell index
is Z minus base Z. The propagator writes at the actual cell-array
address (0x19aaf0, 0x19ab8d–0x19ab94). The raw tag is the low five
bits of cell byte +3. Raw tags 0 and 2 are eligible to receive
propagation writes.

## Seed pass

Visit existing columns in the working rectangle with Y increasing
inside increasing X. Visit each column's stored cells from the highest
index to the lowest. Each column starts open. The first stored cell
whose raw tag is outside 0 and 2 closes that column for the rest of
the scan. Tag 13 closes the column as well.

Set bytes +1 and +2 of each eligible stored cell to 255 while its
column is open, or zero once closed. The +0 value is preserved
within the seed's two-byte write at 0x19a189, and +3 stays as stored.
Seed selection uses the raw tag. A raw tag-zero cell at or below
Z zero can be seeded to zero under an obstruction even if a later
neighbor query sees it as implicit water (0x19a135–0x19a1bc).

## Effective neighbor reads

Each round queries whole-cell neighbors through the same world or
explicit area access. It visits −X, +X, −Y, +Y, −Z and +Z in that
order. It starts the maximum at zero and stops querying once it finds
a contribution of 255. Axis steps are one whole cell. Each call
resolves the neighbor independently of the working-rectangle bounds
(0x19a268, 0x19a30f, 0x19a3bb, 0x19a467, 0x19a515, 0x19a5c5).

Apply the effective cell rules in the following order. The four-byte
presets are listed in memory order.

| Neighbor query | Effective cell |
| --- | --- |
| Missing column, or Z below base Z | (200, 200, 200, 0x01) |
| Existing column, Z at or above base plus count, Z positive | (255, 255, 255, 0x00) |
| Existing column, Z at or above base plus count, Z zero or negative | (255, 255, 255, 0x82) |
| In span, raw tag zero, Z zero or negative and flag 0x40 clear | (255, 255, 255, 0x82) |
| Other in-span query | Stored four-byte cell |

The preset initializers are at 0x2faad0, 0x2fab00 and 0x2fab30.
The accessor at 0x02f730 supplies the in-span read. A present column
with count zero still uses the below-base test before the implicit
air or water case (0x19a26d–0x19a2c2).

| Effective tag | Contribution |
| --- | --- |
| 13 | 255 |
| 0 or 2 | Greater of 5 and unsigned effective byte +2 |
| Other | 0 |

Implicit water contributes 255. A stored tag-2 cell supplies its
own byte +2, including when its flags byte is 0x82. A stored
tag-zero cell with 0x40 set also supplies its own byte at or below
Z zero. The substitutions apply to reads and keep the stored
record intact. If a valid neighbor lies outside the working rectangle,
its existing byte +2 can influence the result. The propagator leaves
that neighbor's stored bytes untouched
(0x19a2c2–0x19a303, 0x19a369–0x19a67b).

## Sixteen rounds and destination commit

Repeat the following two passes exactly sixteen times. Each pass
visits working columns with Y inside X and their stored cells from
the lowest index to the highest.

1. For each raw tag 0 or 2 with byte +2 below 255, find the
   maximum contribution *M* from the six neighbor queries.
   Store the integer quotient of 85 times *M* divided by 100
   in byte +1. The current cell's previous light is excluded
   from the maximum. Byte +2 equal to 255 skips this update and
   retains the seed in byte +1 (0x19a20a–0x19a698).
2. After the entire first pass, copy byte +1 to byte +2 for
   every eligible stored cell in the working rectangle. The
   next round reads those copied values (0x19a6ff–0x19a757).

The arithmetic is integral and nonnegative. A maximum of zero
writes zero, a maximum of 5 writes 4, and a maximum of 255 writes
216. The counter at 0x19a765–0x19a76c always reaches sixteen.
In a synthetic one-cell-wide corridor with opaque boundaries and
one 255 seed, the final light by distance zero through sixteen was
255, 216, 183, 155, 131, 111, 94, 79, 67, 56, 47, 39, 33, 28, 23,
19, and 16. Farther connected cells in that corridor ended at 4
from the neighbor floor.

After round sixteen, scan the destination rectangle alone. For every
stored raw tag 0 or 2 copy byte +2 to byte +0. This step leaves
bytes +1, +2, and +3 as they are (0x19a772–0x19a7ec).

| Stored cell location and raw tag | State after return |
| --- | --- |
| Destination, tag 0 or 2 | Bytes +0, +1, and +2 hold the final light. Byte +3 is preserved. |
| Working margin outside destination, tag 0 or 2 | Bytes +1 and +2 hold the final light. Bytes +0 and +3 are preserved. |
| Outside work, or any other raw tag | All four stored bytes are preserved. |

The helper uses the column's own bytes for both work fields. They
survive the call, including in the margin, and a later invocation
can read the retained +2 value there. With an empty destination
and a nonempty working rectangle, work fields can still be updated
and the destination pass writes no cells.

## Reference observations and limits

An instruction trace matched every stored byte after seeding, after
each round, and after the final commit in 31 synthetic invocations.
The integer quotient matched for all 256 possible neighbor maxima.
At a resident area edge, an exterior eligible cell with byte +2
equal to 200 yielded 170 in a target surrounded elsewhere by opaque
neighbors. The exterior bytes were preserved. The same target yielded
4 when the exterior cell had byte +0 equal to 255 and byte +2 equal
to zero. Within an explicit-area call, a neighbor in the adjacent
resident area is missing and yielded zero in that otherwise opaque
fixture.

These results cover valid column layouts with nonnegative margins
and representable coordinate arithmetic. The synthetic traces used
fixed column contents outside the propagator's own writes. Broader
call scheduling, concurrent world changes and native runtime visual
acceptance remain outside this contract.
