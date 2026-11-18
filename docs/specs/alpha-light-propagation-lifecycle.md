# Alpha block-light propagation lifecycle

**FINAL / STABLE / HELD**

This contract describes terrain light stored in the original Alpha client's
columns. It covers the rectangular operation, initial zone lighting, the
admission and lighting steps of the terrain mesh worker, and two world-update
paths that relight after local terrain destruction.

## Evidence and supported image

The reference is `binary/Cube.exe` in the `disassembled alpha cube world`
evidence tree. All routine and data addresses are RVAs in this file.

| Identity property | Value |
| --- | --- |
| File length | 3,885,568 bytes |
| SHA-256 | `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF` |
| PE machine | `0x014C`, 32-bit x86 |
| Preferred image base | `0x00400000` |
| PE timestamp field | `0x51EA955E` |
| Image size | `0x003BC000` |
| Entry-point RVA | `0x0028E1E0` |

The behavior below comes from the matched PE. The listing at
`decompiled/client/assembly.txt` served as an index, with instruction bytes
checked against the image. Source-side instruction tracing matched the seed,
all sixteen transfers and the commit against independently calculated results
in 31 synthetic invocations. The attenuation calculation was also checked for
all 256 possible input maxima. The traces covered a narrow corridor,
implicit water, missing and admitted neighbors, a zone restriction, and reuse
of a working-margin value in a later call.

## Stored state and initial allocation

A terrain cell is four bytes. The low five bits of byte `+3` are its material
tag and the top three bits are flags. For stored tags 0 and 2, the three
preceding bytes have the following roles during lighting.

| Offset | Role |
| --- | --- |
| `+0` | Committed terrain light |
| `+1` | Next-round light |
| `+2` | Current-round light |

For other stored tags, the rectangle operation leaves all four bytes intact.
Ordinary terrain meshing reads the first three bytes of an ordinary solid as
color (`0x09E426`–`0x09E460`).

Each 32-byte column record has a signed base height at `+0x10`, a cell pointer at
`+0x18` and a signed count at `+0x1C`. Each field is 32 bits in this image. A
zone has signed coordinates at `+0x60` and `+0x64` and a column-array pointer at
`+0xA8`. Its array holds 256 by 256 columns, with X varying fastest.

Zone construction at `0x2240F0` initializes 65,536 column records through
`0x050BF0`. Base, pointer and count begin at zero. At `0x050C50`, a request
matching the current count returns immediately. A changed request of zero or
less discards the allocation and clears pointer and count. A changed positive
request allocates and zeros four bytes per cell before copying retained records.
Shrinking copies the first records that fit to their original indices.
Growing shifts each old record upward by the supplied index displacement. Both
copy paths retain all four bytes. On a changed count, the base decreases by
the supplied displacement. The ordinary cell insertion paths at `0x04E719`
and `0x04E74F` supply the downward displacement for a write below the base and
zero for extension above the upper end, respectively.

## Rectangular operation and reset

At `0x19A0E0` the inputs are a world, signed horizontal whole-cell bounds,
a horizontal margin and an optional explicit zone. The low bounds are
inclusive and the high bounds exclusive. The destination is the requested
rectangle. The working rectangle extends it on all four sides by the supplied
margin. The operation visits the existing full vertical span of each resolved
column. Missing columns are skipped. It creates neither columns nor cells.

The seed sweep starts at `0x19A130`. It advances X on the outside and Y on the
inside, both in ascending order. It visits each column from its top stored
cell down. Each column initially has open sky. Raw stored tags 0 and 2 receive
255 in both working bytes while the sky is open and zero once closed. The
first other tag closes the sky for every lower cell. Tag 13 closes it too.
There is no separate downward attenuation for tag 2. The test masks out the
three flag bits. Byte `+0` keeps its prior value throughout the seed sweep.

Every invocation reseeds the eligible cells in its working rectangle. At the
end, only the destination receives the new committed byte. The values left
behind are as follows.

| Stored cell | Bytes on return |
| --- | --- |
| Tag 0 or 2 inside the destination | The round-sixteen result fills `+0`, `+1` and `+2`. Flags and tag are preserved. |
| Tag 0 or 2 in the working margin alone | The round-sixteen result remains in `+1` and `+2`. Prior `+0` and `+3` are preserved. |
| Cell outside the working rectangle, or any other raw tag | All four bytes are preserved. |

## Propagation rounds, order and material gates

There are sixteen rounds. Each round calculates throughout the working area
before transferring any result to the current-round byte. Both sweeps visit
X first as the outer loop, Y inside X, and the stored cells from lowest to
highest within each column. Only stored tags 0 and 2 with current byte `+2`
below 255 are calculated. The 255 seeds keep their full working value.

The six reads are minus X, plus X, minus Y, plus Y, minus Z and plus Z. The
largest neighbor brightness is used and the remaining reads are skipped as
soon as the maximum reaches 255. Each resolved tag 13 contributes 255. A
resolved tag 0 or 2 contributes its unsigned byte `+2` with a minimum of 5.
Other resolved tags contribute zero. This reads only the six face neighbors.
A tie has the same numerical effect as a single sample at that value.

The new `+1` is 85 percent of the maximum, rounded down. A maximum of 255 gives
216, a maximum of 5 gives 4, and a maximum of zero gives zero. The target's
prior value participates only in the 255 eligibility check. The minimum of 5
belongs to the neighbor read. After the whole calculation sweep, every
eligible stored tag 0 or 2 copies `+1` into `+2`. After round sixteen, a sweep
of the destination in ascending X, then Y, then stored height copies `+2`
into `+0`. The round count is fixed in the image.

Relevant ranges are `0x19A20A`–`0x19A698` for calculation,
`0x19A6FF`–`0x19A757` for transfer, `0x19A765`–`0x19A76C` for the round bound,
and `0x19A772`–`0x19A7EC` for commit. The separate query at `0x0718B0` uses the
same material gates and minimum of 5. Its caller supplies the byte offset.
The rectangle operation's neighbor arithmetic specifically reads `+2`.

## Neighbor resolution and boundaries

Column lookup at `0x0347A0` accepts horizontal coordinates from zero through
`0x00FFFFFF` on both axes. Without an explicit zone, it uses the resident
region and zone lookups at `0x034A90` and `0x034D10`. Either lookup can
fail. With an explicit zone, the lookup only addresses columns within that
zone's own 256-by-256 extent. A point across that extent fails even if its
zone is in the world table. This applies both when visiting a target column
and when resolving its neighbors.

| Sample position or stored tag | Record used for the read |
| --- | --- |
| Column lookup fails, or height is below a resolved column's base | Opaque tag 1. Value bytes are 200, and the brightness gate returns zero. |
| At or above base plus count, at global height above zero | Implicit air, tag 0 with all three value bytes 255. |
| At or above base plus count, at global height zero or lower | Implicit water, tag 2 with all three value bytes 255 and flag `0x80`. |
| Stored tag 0 at global height zero or lower with flag `0x40` clear | The implicit-water record. |
| Any other stored cell in the span | The stored record itself. |

The sample routine is at `0x02F730`. The below-base check comes before the
above-span test. A stored tag 2 uses its own value even with flag `0x80` set.
A stored tag 0 with `0x40` set also uses its own value at or below zero. The
seed, transfer and commit visit raw stored cells, so the implicit-water read
creates no record and does not replace the target's tag. The original defaults
are initialized at `0x2FAAD0`, `0x2FAB00` and `0x2FAB30`, with data at
`0x36B7D0` (air), `0x36B7D4` (opaque), and `0x36B7CC` (water), respectively.

Face-neighbor reads can extend past the working rectangle. A resolved stored
sample there uses its existing `+2` under the same material and height rules.
That byte may be from an earlier working-margin pass while `+0` still holds
another result. World lookup allows these reads across adjacent admitted
zones. A missing zone contributes the opaque fallback.

## Generation, admission and later rebuilds

The generation entry at `0x1E4850` returns an already resident zone from
`0x1E4952`–`0x1E4959` without regenerating or relighting it on that path. For a
new zone, the call at `0x1EDE84` covers all 256 by 256 columns with margin zero
and the new zone supplied explicitly. It seeds, propagates and commits in that
zone's own storage. The same zone is installed in the resident table later at
`0x1EE05A`. The traced publication path has no second lighting call.

The mesh builder at `0x09D910` takes a 32-by-32 chunk. The byte at `+0x74`
belongs to its mesh-cache slot. At `0x09DB10`–`0x09DB89` the native choices are
as follows.

| Cache state | Lighting request |
| --- | --- |
| `+0x74` is nonzero at the check | Extend the chunk by four cells on every side, including at a zone edge. Commit the resulting 40-by-40 rectangle. |
| `+0x74` is zero and either origin axis has within-zone offset 0 or 224 | Extend by sixteen cells on every side. Commit the resulting 64-by-64 rectangle. |
| `+0x74` is zero and the chunk is strictly interior | Skip this lighting call. |

Both requests at `0x09DB84` use a separate margin of zero and world lookup.
The extended area is the destination, so eligible cells there receive all
three new light bytes in resident columns. The builder takes the world
critical section at `+0x8000D8` through `0x19C480` before this step and releases
it through `0x201EA0` at `0x0A0F24`. The rectangle routine and its lookup
helpers take no lock themselves.

The observed worker at `0x0690A0` tests admission through `0x06F490` before
dispatch. It needs the containing zone and, for an edge, the zone across the
touched side. A corner checks the diagonal first, then the X-side and Y-side
zones. These are resident-presence checks. The denial branch at
`0x069569`–`0x069580` bypasses the build and its publication.

The mesh cache initializes the pending byte and timestamp to zero and the
published chunk coordinates to minus one (`0x059751`–`0x059768`). One frame
change-marking path around `0x09568B` checks the 3-by-3 chunk neighborhood of
each changed cell. After a published-coordinate match, it sets `+0x74` to
one and writes the frame's sampled timestamp at `+0x70` under the slot lock
(`0x0957DC`–`0x0957E4`). Multiple marks before a build coalesce into the
same bit. The invalidation at `0x086BA0` can discard the published coordinates
and resources while preserving the pending byte and timestamp. The builder
publishes its new coordinates and clears `+0x74` at `0x0A1394` under the slot
lock at `+0x250`. Its earlier padding check occurs before taking that slot lock.

There are also two confirmed immediate resident-column calls in the world
update. The Bomb path handles entity kind `0x90` (`0x20EBD8`, name at
`0x18FB9D`–`0x18FBB2`). Its terrain deletion appends a changed cell at
`0x20EF2B`. A variable-radius combat terrain-deletion path appends at
`0x213E30`. After a qualifying deletion, the respective immediate calls
at `0x20F071` and `0x213FC0` require the pointer at world offset `+0xB8` to
be null. Both use world lookup and a separate working margin of eight.
For the Bomb, the destination bounds on each axis are centered on its fixed
position at a distance of eight cells to either side. The combat call uses
the computed action radius in place of that distance. It converts the
radius by binary32 multiplication by 65,536 and signed integer truncation
before adding or subtracting it in fixed units. Both calls convert each
resulting fixed-point bound to a whole cell by signed division by 65,536
toward zero. The resulting high end is exclusive. Conversion sites include
`0x0122E0`, `0x012220` and `0x02F570`.

## Explicit limits

The confirmed initialization rules cover cell allocation and the start of a
rectangle invocation. The initial committed value of a generated or loaded
cell depends on earlier writers. This work leaves the semantic identity and
ordinary occurrence of material 13 unresolved.

The terrain mesh path establishes a later opportunity to relight across an
admitted zone boundary. Exact scheduling, the outcomes of overlapping edits
and mesh publication, and the lock coverage of other native callers remain
open. The complete set of events that set the pending byte, terrain-changing
combat abilities, and save or network invalidation routes remain open as well.
The evidence covers the identified client image and controlled instruction
traces. In-game timing, visual acceptance and other Alpha builds remain
unverified.
