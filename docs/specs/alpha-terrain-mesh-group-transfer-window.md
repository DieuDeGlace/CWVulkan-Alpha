# Alpha terrain mesh group elevations and renderer transfer window

**Author status — FINAL / STABLE / HELD**

Alpha anchors each chunk's 250-elevation mesh bands at its lowest cell with an exposed face. In the renderer, the preliminary dirty-key count visits the entire physical cache. The subsequent transfer pass visits only the part of the current chunk window inside the world. A dirty physical slot omitted from that second pass can delay transfer in another slot with the same key.

The reference is the original 32-bit `Cube.exe`, 3,885,568 bytes, preferred image base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Addresses below are RVAs. Evidence comprises the original image, its client instruction listing, call records and scoped routine decompilations.

## How the builder obtains the band origin

The builder at `0x09D910` makes a bounds pass before emitting faces. The requested chunk determines a square of 32 by 32 terrain columns. The pass visits increasing X, then increasing Y within each X. It skips current columns outside the horizontal world range from 0 through 16,777,215, or without an available region/column. Neighbor samples can cross the chunk boundary. Only a qualifying cell in a current column can set the lower bound. See `0x09D96A–0x09D99E` and `0x09DB89–0x09DCA5`.

For each current column, the search starts one elevation below the smallest stored base among the column and its available four horizontal neighbors. It scans upward, stopping before the larger of one and the current column's base plus its stored record count. This starting elevation merely defines where to look. The lower band origin changes only for cells that pass the following face tests. See `0x09DC9A–0x09DE46`.

| Effective current material | When the current elevation qualifies |
| --- | --- |
| Empty, type 0 | Never. |
| Liquid, type 2 | At least one axial neighbor is empty. |
| Any other nonzero type | At least one axial neighbor is empty or liquid. |

The tests use the low five material bits and include implicit cells. A missing neighbor or an elevation below the stored base supplies ordinary solid. An elevation at or above the stored end supplies liquid at zero or below, and air above zero. A stored nominally empty cell at zero or below acts as liquid unless its `0x40` flag is set. On a nonempty current cell, the neighbor checks stop at the first qualifying face, in the order negative X, positive X, negative Y, positive Y, below and above. The qualifying elevation is the current cell's Z even when its only exposed face is the top. See `0x02F7E0` and `0x09DE50–0x09DFD5`.

The lower and upper accumulators both start at zero with a separate first-exposure flag. The first qualifying cell sets both to its signed elevation. Later qualifying cells can lower the minimum or raise the maximum. With no qualifying cell, the lower accumulator stays zero. A fully buried low column base therefore cannot pull the band origin down by itself. See `0x09DB89–0x09DBB7` and `0x09DFD5–0x09DFFB`.

The upper accumulator has one additional input. Before each current column's elevation loop, the builder also considers the base plus stored count of the positive-Y neighbor. A larger value raises the upper accumulator. The first qualifying cell still overwrites both accumulators, including any such upper value encountered up to then. Subsequent upper raises only affect the number of band positions available. They cannot lower the origin or create an emitted face. At `0x09DE24–0x09DE3A` the native path reads the positive-Y column after its lookup, including a branch that leaves a null pointer before the read. Successful behavior with that column absent is unproved.

For a completed scan the number of band positions is one plus the signed upper-minus-lower span divided by 250 with truncation toward zero. Emission assigns a cell to the band obtained from its elevation relative to that lower bound, divided the same way. For example, when the lowest exposed cell is at 17, elevations 17 through 266 belong to the first band and 267 starts the second. A top face at the upper edge of a band stays with its cell. The bound is recomputed on rebuild. Empty bands can occupy a position in the range, while only a band with a primary buffer supplies a pending mesh group. See `0x09E03B–0x09E084`, `0x09E380–0x09E3B3`, `0x0A1230–0x0A123B`, `0x0A12D8` and `0x0A134F–0x0A1366`.

## Which slots the two renderer passes visit

| | Preliminary dirty-key count | Pending group transfer |
| --- | --- | --- |
| Iteration domain | Starts at physical slot zero in the array at client offset `0x2E0`, advances by the slot size of `0x268` bytes, and inspects width squared slots for the ordinary positive width. | Visits chunk coordinates in the square whose origin is at client offsets `0x2AC` and `0x2B0`. Each axis runs from its origin inclusive to origin plus width exclusive. X is the outer loop. Y restarts at its origin for each X. |
| Boundary handling | Uses the width at `0x2DC`. Neither the window origin nor world coordinates enter this count. The ordinary width 40 visits all 1,600 physical slots. | Discards any coordinate below 0 or at least `0x80000` on either axis. For each surviving coordinate, physical row is Y modulo width, physical column is X modulo width, and the row stride is width slots. The window is clipped in place at a world edge. |
| Tests before the operation | Adds one to the count for the slot's 32-bit key at `0x70` only if the key and dirty byte at `0x74` are both nonzero. Stored chunk identity, active or pending group counts, camera visibility and world-region availability are untested. | Looks up the visited slot's key in the preliminary table. A missing key has count zero. A positive count defers all transfers for that key on this visit. Key zero cannot have accumulated a dirty count. When the key passes, the renderer locks the slot and checks whether its pending group count is positive. |

The width is initialized at `0x059F39`. The preliminary table is freshly constructed at `0x0AC443–0x0AC46E` and counted at `0x0AC475–0x0AC4D8`. The coordinate limits, clipped lookup and transfer tests are at `0x0AC4E8–0x0AC5B1`. The inner and outer loop increments are at `0x0AD442–0x0AD472`. The integer-key lookup at `0x068910` supplies a zero count for a newly encountered key.

For example, with width 40 and origin (−1, 10), the preliminary count still inspects 1,600 slots. Transfer visits X from 0 through 38 and Y from 10 through 49, giving 1,560 in-world coordinates. The 40 physical slots with column residue 39 receive no transfer visit. A nonzero dirty key found there still defers a visited slot with that key. At a fully in-range origin, the transfer visit reaches each physical slot once, in the order induced by the world coordinates.

## Transfer, draw eligibility and timing

For a visited slot with a clear key count and a nonempty pending list, the renderer releases the old active groups, assigns the pending groups to the active list, clears the pending list and transfers the positioned-decoration list. An empty pending list leaves the active groups in place. This operation does not require an existing active group, matching stored X/Y, a visible bounding box, a particular distance from the camera, or a clean dirty byte on a zero-key slot. The test uses the key counts already collected for this renderer visit. No count is consumed when another slot transfers. See `0x0AC58A–0x0AC652`.

Draw eligibility comes afterward. The renderer takes the slot lock, tests for a nonempty active list, checks the stored chunk coordinates against the visited coordinates, and tests the slot bounds against distance and view planes. These checks govern whether terrain is collected for drawing. Failing one bypasses that collection and continues to the separate decoration path. A slot can therefore accept pending groups while offscreen, and a deferred transfer can leave older active groups eligible to draw. See `0x0AC658–0x0AC752` and the bounds test at `0x07F3C0`.

In the normal frame, the client update containing the fog scan precedes the renderer containing these two passes. The frame checks for a graphics device and client. The foreground-input branch rejoins the same frame call when input is inactive. Inside the renderer, both passes precede the per-slot active/visibility tests. Preliminary key reads occur before the per-slot transfer lock. A worker publication fills the pending list under its own acquisition of that slot lock. Publication and renderer acceptance are separate events. A group first accepted by this draw cannot affect the fog scan that already ran in the same frame. The source specifies no fixed number of frames from publication to acceptance. See the client virtual entries at `0x2FFDF0–0x2FFDF4`, frame calls at `0x0C8633` and `0x0C86EC`, input rejoin at `0x0C90C4–0x0C90CB` and `0x0C9432`, and builder publication at `0x0A0F29–0x0A1398`.
