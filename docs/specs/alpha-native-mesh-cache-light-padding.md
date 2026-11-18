# Native mesh-cache lighting updates

**FINAL / STABLE / HELD**

## Supported image and cache slot

This ruling covers the original 32-bit Alpha `Cube.exe` with SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs relative to image base `0x00400000`.

The byte read at `0x09db17` belongs to a 32-by-32-cell mesh cache
slot. Client offset `+0x2e0` points to the slot array, whose
stride is `0x268`. Client offset `+0x2dc` is the cache side
length *N*, initialized to 40. For valid chunk indices *cx*, *cy*,
the slot index is `(cy mod N) × N + (cx mod N)`.
The published chunk indices are stored at slot `+0x18` and
`+0x1c`, the edit timestamp at `+0x70`, and the lighting request
byte at `+0x74` (`0x059f39`, `0x061eeb`–`0x061f54`,
`0x09d956`–`0x09d9d1`).

## Native actions that request an update

The client frame processes a list of changed whole-cell coordinates.
Two original local producers append to the world-step result's
terrain-change list at offset `+0x18`.

- The native Bomb destruction branch matches entity kind `0x90`
  (`0x20ebce`–`0x20ebdf`), registered under the name Bomb
  (`0x18fb9d`–`0x18fbb2`). Its fixed-radius terrain clear uses
  `0x071920`. Each qualifying deletion appends the corresponding
  change record at `0x20ef2b`.
- A separate variable-radius combat destruction branch also clears
  terrain at `0x213dd0` and appends at `0x213e30`. Both sites
  use the same list append at `0x086240`.

The client frame passes the same result object to repeated world steps
and the final step at `0x08cf2e` and `0x08cf49`. It additionally
applies its queued cell records at client `+0x800648`. A record
with zero flag byte at `+0x0f` takes the clear route at
`0x08f320`. A nonzero byte takes the four-byte cell-write route
at `0x08f34a`. It appends those queued records to the same changed
list at `0x08f42c`, then clears the client queue at `0x0905e9`.
The object passed at `0x08cf22` and `0x08cf40` begins at frame
offset `+0x224`. Its `+0x18` list is the frame's `+0x23c`
list iterated at `0x0956a7` and `0x0956bb`.

The marking phase takes one `timeGetTime` value at `0x09568b`.
For each record it divides whole X and Y by 32 toward zero, then
visits chunk offsets −1 to +1 along each axis, with Y inside X.
A candidate outside the accepted nonnegative chunk/world bounds
branches to frame cleanup at `0x09d322`. For an in-range
candidate, the cache lookup must find an exact match in both
published indices (`0x0956e0`–`0x0957bf`, `0x068840`).
Only a match enters the slot's critical section at `+0x250`,
writes literal 1 to `+0x74`, and stores that phase's timestamp
at `+0x70` (`0x0957c1`–`0x0957e4`).

Each matching record rewrites the same byte and timestamp. Repeated
records before the next build reads the byte coalesce into one
nonzero request for that slot. Later marking phases refresh the
timestamp. The published coordinates gate marking, so a change
near a chunk that is still awaiting admission can mark its present
neighbors and leave its own unpublished slot untouched.

## Admission, padding and clear

Slot construction initializes the published indices to −1 and
both the edit timestamp and request byte to zero
(`0x059751`–`0x059768`). Cache invalidation at `0x086ba0`
resets the indices to −1 and releases the mesh resources. It
preserves the request byte and edit timestamp. A dirty slot can
therefore carry its still-set byte through invalidation to its
next admitted build, including a different chunk mapped to that
slot.

The mesh worker treats a coordinate-matched slot with a clear byte
as reusable. A nonzero byte or a coordinate mismatch reaches the
build consideration path. Coordinate mismatch first invalidates the
slot. The worker permits at most one builder dispatch per scan and
checks area admission before dispatch
(`0x0693e9`–`0x069485`, `0x069531`–`0x069580`).
Admission at `0x06f490` requires the containing 256-by-256 area.
For each chunk axis whose cell origin modulo 256 is 0 or 224,
it also requires the area across that edge. At a two-axis corner
it requires the diagonal and both side areas. The neighbor area
lookup divides the adjusted chunk origin by 256 toward zero.
An admission miss leaves the byte eligible for the next scan.

The admitted builder at `0x09d910` computes the whole-cell chunk
origin as `(32 × cx, 32 × cy)`. Its padding decision at
`0x09db10`–`0x09db89` uses the byte as follows.

| Request byte and chunk origin | Work and commit rectangle |
| --- | --- |
| Nonzero byte at any origin | Four cells beyond each edge of the 32-by-32 rectangle |
| Zero byte, with either origin axis 0 or 224 modulo 256 | Sixteen cells beyond each edge |
| Zero byte at other origins | Use the stored values without calling lighting preparation |

The padded ranges run from origin minus pad, inclusive, through
origin plus 32 plus pad, exclusive. Their widths are 40 or 64
whole cells on each axis before missing or out-of-bounds columns
are skipped. The byte takes priority over the seam test.
Publishing the rebuilt slot writes both chunk indices, the new
vertex count, and zero to `+0x74` before leaving the slot
critical section (`0x0a0f29`–`0x0a0f36`,
`0x0a136c`–`0x0a1398`). The build reads the byte before
acquiring that slot lock. The clear is unconditional at publication.

The write at `0x07051c` in the stock world initialization/reset
routine targets the separate `+0x74` byte on world area objects.
Those pointers come from the region's area table at `+0x10018`.
The frame tests that area byte at `0x09d1b6` after an area lookup.
The mesh padding decision uses the slot array described above.

## Which columns receive the light writes

The helper at `0x19a0e0` takes a destination rectangle, a working
margin and an optional explicit area. It uses stored cell bytes
`+1` and `+2` as propagation work fields on the selected
columns throughout the destination plus working margin. It
copies the resulting byte `+2` to stored byte `+0` for tag
0 and 2 cells inside the destination only
(`0x19a135`–`0x19a192`, `0x19a695`, `0x19a72b`,
`0x19a798`–`0x19a7cd`). The cell accessor `0x19aaf0`
returns the stored element from the column's cell-array pointer
for these in-span writes. With no explicit area it looks up
resident columns. With an explicit area it uses that object's
columns and rejects queries outside its bounds (`0x0347a0`).

| Native preparation call | Destination and ownership |
| --- | --- |
| Mesh builder at `0x09db84` | Resident columns. The selected 40-by-40 or 64-by-64 rectangle is both the working and commit rectangle. The working margin parameter is zero. |
| New area generation at `0x1ede84` | The explicit, newly allocated 256-by-256 area's own column storage. Work and commit cover that area with zero working margin. The same area pointer becomes resident at `0x1ee05a`. |
| Bomb destruction at `0x20f071` | Resident columns. The destination bounds on each axis are the whole coordinates of the center minus eight and center plus eight, with the upper bound exclusive. The separate working margin is eight more cells. |
| Variable-radius destruction at `0x213fc0` | Resident columns. Uses the whole coordinates of center minus and plus the computed action radius, with the upper bound exclusive and the same separate eight-cell working margin. |

The destruction branches reach these preparation calls after a
qualifying deletion and when world pointer field `+0xb8` is zero
(`0x20ef82`–`0x20f071`, `0x213e81`–`0x213fc0`).
They construct each bound in fixed units, then divide by 65,536
toward zero. The variable-radius branch converts its floating
radius to fixed displacement by multiplication by 65,536 in
binary32 and truncation toward zero
(`0x0122e0`, `0x012220`, `0x02f570`). The eight-cell
working margin leaves byte `+0` outside the destination as it
was, while the stored work bytes there can change. The area
generator writes its final cell storage before table publication.

## Evidence limits

The matched executable establishes the named Bomb branch, the
separate variable-radius combat branch, and the client queue
consumer. The full ability-name mapping and every producer of
the client queue remain unresolved. The source shows the slot's
publication clear and the separate edit writes. Scheduling or
recovery for an edit interleaved between the builder's byte read
and publication has no runtime verification here.
