# Alpha client terrain-mesh dispatch

**Author status — FINAL / STABLE / HELD**

## Evidence and scope

This contract covers the background worker that selects a 32-by-32 terrain chunk for mesh construction, its resident mesh accounting, and the builder's decision to refresh lighting. The reference is the original 32-bit Alpha client `Cube.exe`, 3,885,568 bytes, preferred image base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Addresses below are RVAs. The evidence consists of the client image, its original-client disassembly, individual decompilations, call index, strings and symbol records. Coordinates refer to horizontal axes throughout. A region in this document covers 256 by 256 terrain columns, or eight by eight mesh chunks.

## Worker lifetime and window

Client initialization sets the run byte at client offset `0x800584`, starts a thread for this worker, stores its handle at `0x800588`, and requests thread priority −1. The worker tests the run byte on entry. After every completed pass it requests a five-millisecond sleep and checks the byte again. Each pass can invoke the mesh builder at most once. Elapsed work, lock contention and system scheduling determine the actual interval. A rendered frame can therefore contain zero, one or multiple worker dispatches. A separate maintenance call near the sleep has its own 60,000-millisecond test. The mesh dispatch precedes that test. Evidence includes initialization at `0x063512–0x06356D`, thread creation at `0x050EE2`, callback entry at `0x06DB70`, worker at `0x0690A0`, and loop tail at `0x0694B9–0x06950F`.

At the start of a pass, the worker takes the client lock at `0x8005D0`, snapshots the two signed 32-bit window-origin coordinates at `0x2AC`, and checks two pairs of client state values. The 32-bit values at `0x800A50` and `0x800448` must agree. The strings at `0x800A54` and `0x378` must have equal lengths and equal contents. The lock is released after the comparison. If either pair differs, the worker takes the separate cache lock at `0x800600`, resets every physical slot in ascending slot index, releases the lock, and skips candidate selection for this pass. The setting of either state pair by a particular gameplay event is outside this contract. Evidence is at `0x0690E5–0x0691E1`.

For a matching state, the candidate window is a square of side length given by the signed integer at client offset `0x2DC`. Initialization writes 40 at `0x059F39`. With that width, an interior window has 1,600 candidates and its priority center is the origin plus 20 on each axis. Enumeration increases X in the outer loop and Y in the inner loop, starting at the origin and excluding origin plus width. Candidates must satisfy 0 through 524,287 inclusive on each axis. Clipping discards candidates without moving the priority center. Each retained candidate addresses a physical cache slot using Y modulo width as the row and X modulo width as the column. The slot array begins at the pointer at `0x2E0` and the native stride is `0x268` bytes.

The priority key is the signed 32-bit squared Euclidean distance in chunk coordinates from origin plus the integer half-width on both axes. The worker sorts by increasing key before processing any candidate. For a whole list of at most 32 candidates, the short sort preserves enumeration order at equal distance. The large-list partition can move equal-distance entries and uses no X, Y or slot-address comparison to break ties. Enumeration order alone cannot specify a default-window tie. Exact large-window equal-distance permutations remain open. Evidence is at `0x0691E6–0x069370` and in the ordering routines at `0x055D80`, `0x054D60` and `0x0563D0`.

## Accounting and dispatch order

The worker snapshots the signed render-distance option at client offset `0x180` under the settings lock. It converts that integer to binary64, multiplies first by the binary64 constant 0.01 and then by 25,165,824, and truncates to signed 64 bits. At render distance 100 the resulting allowance is 25,165,824 units. The running total starts at zero each pass. The units are counts of eight-byte mesh vertex records. For each published buffer group the builder adds the number of these records in its primary vertex stream. The two triangle counts stored with that group do not enter this sum. Evidence includes the render-distance option read/write mapping at `0x0CEA24–0x0CEAA5`, `0x0CF087–0x0CF090` and `0x06F390`, budget computation at `0x069375–0x0693DD` with constant RVA `0x302A48`, and builder accounting at `0x0A1230–0x0A137A`.

The following decisions are made in sorted order while holding the cache lock at `0x800600`. The worker holds it across the builder call.

1. Before examining this candidate's stored coordinates, compare the accumulated total with the allowance. If the total already exceeds the allowance, reset this slot and advance. The total stays at its previous value. Equality admits the current candidate.
2. Compare the requested coordinate pair with the slot's pair at offsets `0x18` and `0x1C`. A matching slot with a zero dirty byte at `0x74` needs no build. A mismatch resets the slot before any further checks. A matching dirty slot keeps its current mesh while awaiting a replacement.
3. While the pass still has its one build opportunity, test source-region residency for a mismatched or dirty candidate. A failed test leaves the opportunity available for a later candidate. The first passing test consumes it before the call to the mesh builder at `0x09D910`. Remaining candidates still undergo budget eviction, coordinate mismatch resets and accounting.
4. For each candidate admitted in step 1, add the slot's current signed 32-bit count at `0x23C` to the signed 64-bit total after any reset or build. A chunk can push the total over the allowance and remain resident. Eviction begins with the following candidate. A matching dirty slot whose source is unavailable can still contribute its old count. A reset slot contributes zero until rebuilt.

The selection and accounting sites are `0x0693E2–0x069487` and `0x069531–0x069585`. The direct builder call is at `0x069580`. Slot reset at `0x086BA0` takes the per-slot lock at `0x250`, changes both stored coordinates to −1, clears the slot's mesh collections, zeros the counts at `0x238` and `0x23C`, and releases the lock. This reset routine leaves the dirty byte untouched. On publication, the builder records the new count, X and Y, and clears the dirty byte before releasing the slot lock at `0x0A137A–0x0A1398`.

## Source-region residency

The worker tests the region containing the requested chunk through the terrain lookup at `0x034D10`. A non-null region pointer is required. For each axis, a chunk with local index zero in its eight-chunk region also needs the region toward decreasing coordinates. Local index seven needs the region toward increasing coordinates. Indices one through six add no requirement on that axis.

| Chunk position | Required lookup order, stopping at the first missing region |
| --- | --- |
| Interior on both axes | Containing region |
| Edge on one axis | Containing region, then the neighbor across that edge |
| Corner | Containing region, diagonal neighbor, X-only neighbor, Y-only neighbor |

The helper computes a neighbor's region index from its offset chunk coordinate, scaled by 32, with signed division by 256 truncated toward zero. At the lower world edge the outward sample at column −32 therefore maps back to region index zero. An outward sample at the upper edge maps to 65,536 and the region lookup rejects it. Passes at world edges must preserve this asymmetry. This gate checks source-region pointers. It does not depend on neighboring mesh slots having completed a build. Evidence is at `0x06F490–0x06F611` and `0x034D10`.

## Dirty notification and relighting

One client update path samples the millisecond clock once for a batch of horizontal terrain positions. For each position it divides each coordinate by 32 with truncation toward zero and visits a three-by-three block of mesh chunks around the result. The X offsets run from −1 to +1 outside the corresponding Y loop. For an in-range candidate whose slot coordinates match, it takes the slot lock, sets the dirty byte to one and writes the batch timestamp at `0x70`. A coordinate-range failure branches out of this traversal to a common cleanup path. The worker's build decision tests the dirty byte directly. The timestamp supplies no waiting interval in that decision. Evidence is at `0x09568B–0x095847`, `0x09D322` and pair comparison `0x068840`. The full set of event producers feeding this batch remains open.

The builder tests the slot's dirty byte and its position within the 256-column region before traversing the chunk's mesh columns. A dirty slot always requests a rectangular lighting refresh with four columns of padding on every side. With a clear dirty byte, an edge or corner chunk requests sixteen columns of padding on every side. A clear interior chunk skips this call. The terrain-column rectangle starts at chunk origin minus padding and excludes chunk origin plus 32 plus padding on each axis. Thus the dirty and clean-boundary requests cover 40 by 40 and 64 by 64 columns respectively. Both additional arguments to the region refresh are zero at this site. The decision uses the dirty byte as it exists when the builder runs, including after a coordinate mismatch reset. Evidence is at `0x09DB0B–0x09DB89`, with the refresh at `0x19A0E0`.

This contract establishes the worker-to-builder schedule and the builder's conditional relight request. Other callers of the lighting routine, exact presentation timing, complete equal-key permutations, asynchronous source-region arrival times, and the entire set of dirty-state transitions remain outside the verified scope.
