# Alpha terrain fog and active mesh groups

**Author status — FINAL / STABLE / HELD**

The primary terrain fog scan treats a chunk as populated when its cache slot has **at least one active mesh group and its stored X and Y both match the chunk being scanned**. The word it tests is a 32-bit list length. It can exceed one. The word can stay zero after a successful build, or stay positive after a successful empty rebuild.

The reference is the original x86 `Cube.exe`, preferred image base `0x00400000`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All addresses below are RVAs. Evidence comprises the original client image, instruction listing, scoped routine decompilations and direct call records.

## The fog read

The update at `0x088EE0` scans the square starting at the two chunk coordinates at client offsets `0x2AC` and `0x2B0`, using width `0x2DC`. It skips coordinates outside the world. For the remaining coordinates, the column is X modulo width and the row is Y modulo width. Rows are contiguous in the slot array at client offset `0x2E0`, and each slot occupies `0x268` bytes.

At `0x0890CF` the scan reads slot offset `0x0C`. Zero takes the path that considers this chunk's distance when limiting fog. A nonzero count tests the stored coordinate pair at offsets `0x18` and `0x1C`. A mismatch also takes that distance path. Only a nonzero count with both coordinates equal skips it. There is no region-table lookup, dirty-bit test, triangle count or vertex-budget read in this decision. See `0x089034–0x089107`, the pair comparison at `0x0688A0`, and the distance minimum at `0x08923F–0x089268`.

## From constructed mesh to active mesh

| Slot field | Meaning on the traced paths |
| --- | --- |
| `0x08`, `0x0C` | Active group list and its 32-bit length. The renderer uses this list and the fog scan reads its length. |
| `0x10`, `0x14` | Newly built group list awaiting renderer acceptance, and its 32-bit length. A successful worker publication replaces this list. |
| `0x18`, `0x1C` | Chunk X and Y assigned to this slot. The worker writes them when publishing, even if no group qualifies. |
| `0x74`, `0x23C` | Dirty byte and the worker's primary vertex-record total, respectively. The builder finishes by clearing dirty and writing the total. |

Slot construction initializes both group counts to zero and both coordinates to minus one. The client constructs an array of these slots at `0x061F2C–0x061F54`. See `0x0596E0`, particularly `0x05971E–0x05976C`.

The builder at `0x09D910` gathers primary vertices into vertical groups covering 250 elevations each. Ordinary and liquid faces append records to the same primary vertex stream and use separate index streams. A group qualifies for publication when its primary vertex-buffer pointer is non-null. A qualifying group adds one pending entry regardless of which index stream has triangles or how many triangles it has. The builder replaces the previous pending list, increments the pending count once per qualifying group, writes the vertex total and coordinates, and clears dirty. The active group list is untouched by this publication. See `0x09EB34–0x09EBD7`, `0x0A0AB0–0x0A0C8D`, `0x0A0F29–0x0A0F36` and `0x0A11C7–0x0A1398`. Positioned decorations are transferred on separate lists at `0x248` and `0x240`. Their presence adds no active terrain group.

The terrain renderer performs the transfer. It first counts dirty slots by their nonzero key at `0x70`. If the current slot's key has a nonzero count in that table, the renderer skips transfer on that visit. Otherwise it enters the slot critical section at `0x250` and tests the pending length. **Only a nonempty pending list replaces the active groups.** On that path the renderer releases the old active group objects, assigns the pending group pointers to the active list, empties the pending list, and sets the pending count to zero. An empty pending list branches past the entire replacement. Bounds and draw checks occur afterward. See `0x0AC475–0x0AC4DA`, `0x0AC58A–0x0AC6A6`, list assignment at `0x0AB940` and list erasure at `0x077BF0`.

## Consequences for the scanned chunk

| Event | Result for fog population |
| --- | --- |
| First successful nonempty build | The worker can have written matching coordinates, a positive vertex total and a positive pending length while the active length is still zero. The chunk first qualifies once the renderer has transferred those groups and the fog scan observes the resulting count. |
| First successful empty build | The assigned coordinates can match, but both lists stay empty. The slot contributes a candidate distance to fog. |
| Empty rebuild of an already active chunk | With the same coordinates and no intervening reset, the pending length and vertex total become zero. The renderer skips replacement and the old active list survives. Fog still treats the chunk as populated. A later successful nonempty rebuild can replace those groups. |
| Water-only mesh | Exposed liquid faces produce primary records and can supply a primary buffer even with zero ordinary triangles. After transfer, one such group suffices for fog. A body of liquid producing no exposed faces follows the empty-mesh case. |
| Dirty or waiting for transfer | Marking a matching slot dirty does not itself clear its active length or change its coordinates. A previously active slot can remain populated while waiting. See the dirty write at `0x0957B5–0x0957E4`. |
| Region departs | The ordinary region-removal routine clears a world region-table entry, with optional persistence and object destruction. It does not directly write this mesh slot. The worker tests region availability before a new build. On a matching dirty slot a failed readiness check leaves the active groups intact. Departure alone therefore has no immediate effect on the fog predicate proven here. The separate reset paths below can change it. See `0x06AE23`, `0x1A4890`, region lookup at `0x034D10` and build admission at `0x069553–0x069580` and `0x06F490`. |
| Cache position reused for different coordinates | The old coordinates already fail the fog comparison for the new chunk. When the worker reaches a mismatched candidate it resets the slot before checking build admission. Reset invalidates both coordinates, releases active and pending groups and zeroes both counts and the vertex total. A successful rebuild assigns the new coordinates. The new chunk still needs a nonempty renderer transfer to qualify. See `0x06942A–0x06945B`, `0x069531–0x069580` and `0x086BA0–0x086CEB`. |
| Worker resets a slot for another reason | A candidate reached after the accumulated work exceeds the pass allowance is reset. A change in the worker's tracked world identity also takes a reset path across the cache. Both use the same clearing routine. See `0x069108–0x0691DB` and `0x069410–0x06942A`. |

## Graphics allocation and timing limits

On the normal path, empty primary data skips vertex-buffer creation for that group. Nonempty data causes the builder to request a GPU buffer, then immediately read the returned pointer and call its lock method. It does not branch on the allocation result before using the pointer. The two nonempty index streams have the same pattern. The later publication filter does skip a null primary pointer, but that filter is reached only if construction gets that far. A failed allocation with a null result can interrupt the builder before publication. The original code does not establish a graceful transition to “empty”, a retry policy, or the continued state of the application after such a failure. See `0x0A0AB0–0x0A0B4E`, `0x0A0B51–0x0A0C8D` and `0x0A1230–0x0A123B`.

The evidence fixes the field and the operations that affect it. It supplies no fixed delay from region departure to reset, nor from a completed build to a fog scan that observes renderer acceptance. Those intervals depend on the worker and renderer visits.
