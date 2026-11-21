# Alpha client terrain-mesh window origin

**Author status — FINAL / STABLE / HELD**

## Source and result

The original Alpha client derives its terrain-mesh window from the adjusted camera position. The frame update publishes a pair of signed 32-bit chunk coordinates at game-controller offsets `0x2AC` (X) and `0x2B0` (Y). The mesh worker takes a snapshot of that pair independently on each pass. Controller construction sets the window width at `0x2DC` to 40 chunks.

The reference is the original 3,885,568-byte x86 `Cube.exe`, preferred image base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All addresses below are RVAs. The evidence is the client image, its disassembly, scoped routine decompilations, virtual table and call records.

## Which position moves the window

The camera eye is a controller-owned vector of three signed 64-bit fixed-point coordinates starting at `0x140`. X is at `0x140`, Y at `0x148`, and Z at `0x150`. One terrain column spans 65,536 fixed-point units. The controller also maintains a separate target vector at `0x158`. The local actor pointer at `0x8006D0` leads to the actor position at actor offset `0x10`. In the tracking path the target can be copied from the actor position or moved toward it. The frame then computes the eye relative to the target using a rotated direction and distance. Terrain checks can shorten the displacement. A further correction can assign a position relative to the actor if the proposed eye occupies blocking terrain.

The frame obtains references to components 1 and 0 of the eye before that last correction. They point into the controller's eye vector. The origin calculation dereferences them after the correction, so it sees the adjusted values. The same eye supplies the view-transform calculations in between. Only eye X and Y enter the origin conversion. A camera movement that crosses an origin threshold can therefore move the window while the actor stays in the same chunk.

Evidence includes target tracking at `0x09C367–0x09C3BF` and `0x09C44A–0x09C4EF`, use of the eye vector at `0x09C76F–0x09C802`, terrain checks and eye assignment at `0x09C95A–0x09CA68`, component selection at `0x09CA93–0x09CAA7`, the indexed component accessor at `0x0120B0`, and optional correction before view processing at `0x09CAE1–0x09CC09`. The blocking test called here examines the five-bit terrain type at `0x03B480`. The complete camera-control and collision model lies outside this contract.

## Conversion to the window origin

Apply the following conversions independently to the two horizontal components, preserving the precision changes and their order.

1. Convert the signed 64-bit eye coordinate to binary64, divide by 32 in binary64, then truncate toward zero to a signed 64-bit integer. The result is still in fixed-point units, now scaled to chunks.
2. Take the signed integer window width divided by two, truncated toward zero. Subtract that many whole fixed-point units of **one chunk each**, or 65,536 counts per chunk, from the result of step 1. With width 40 the subtraction is 1,310,720 counts.
3. Convert the resulting signed 64-bit value to binary32. Multiply in binary32 by exactly 1/65,536. Truncate the resulting binary32 value toward zero to obtain the signed 32-bit origin component. Store X first and Y second.

The native helper RVAs are `0x069060` and `0x28D946` for division and truncation, `0x03ABC0` for the fixed-point subtraction, `0x0120F0` with the binary32 constant at `0x2FCD94` for scaling, and `0x22F720` for the final conversion. The producer at `0x09CEEB–0x09CF76` publishes the signed result without clamping it to the world edges. The worker clips individual candidate chunk coordinates later at `0x069220–0x069242`. A second pair at controller offset `0x2B4` is written immediately afterward from the eye in terrain-column units, without the chunk division or half-width subtraction. The worker reads the first pair.

The following one-axis examples use width 40 and ordinary round-to-nearest floating-point conversion. Each input is a raw fixed-point camera coordinate. The other horizontal axis is independent.

| Raw camera coordinate | Terrain-column position | Published origin |
| ---: | --- | ---: |
| 0 | 0 | −20 |
| 1 | 1/65,536 | −20 |
| 32 | 1/2,048 | −19 |
| 65,536 | 1 | −19 |
| 41,877,504 | 639 | 0 |
| 41,943,040 | 640 | 0 |
| 44,040,192 | 672 | 1 |
| 2,097,151,999 | 32,000 minus 1/65,536 | 980 |
| 1,099,511,562,240 | 16,777,215 | 524,267 |

The inputs 1 and 32 expose the first truncation at the lower edge. For the input one fixed-point count below column 32,000, the conversion in step 3 rounds the intermediate fixed-point value to 64,225,280 before scaling. That is why the stored origin has already reached 980.

## Publication timing

The normal application loop calls the frame routine at `0x0C9432`. With its graphics device and controller available, that routine takes the millisecond clock difference and calls controller virtual slot `0x28` at `0x0C8633`. The initial clock sample produces a zero delta. In this image the slot resolves to `0x088EE0` through the controller table at `0x2FFDC8`. The render callback in the following slot runs later in the same frame routine at `0x0C86EC`.

On the traced update path, after adjusting the eye and processing the view transforms, the controller acquires its `0x8005D0` critical section at `0x09CEDE–0x09CEE5`. It publishes the origin at `0x09CF70–0x09CF76`, updates related state, and releases the lock at `0x09D2D7–0x09D2DE`. There is no test for a changed chunk, a minimum distance moved, or elapsed time at this publication site. An earlier return from the frame update can bypass it. For example, an out-of-range coordinate in one terrain-change traversal can branch to common cleanup at `0x095746–0x09576E`.

The mesh worker snapshots both origin components together under the same lock at `0x0690E5–0x069103`. A pass that took its snapshot before a frame's publication uses its earlier origin for that pass. A pass that acquires the lock after publication can use the new pair. The worker's sleep and build budget therefore impose no fixed presentation-frame latency on origin changes.

## Initialization and limits

The controller constructor writes 40 to the signed width field at `0x059F39`. It sizes the physical cache to width squared, using a `0x268`-byte slot stride, and stores the array pointer at `0x2E0` on the path at `0x061EEB–0x061F54`. Its constructor inputs for the eye are 10,000 columns on both horizontal axes and 60 on Z. The separate target vector starts at zero. The writes appear at `0x059CF9–0x059DD2`. The worker starts later in construction at `0x063512–0x06354D`.

A guaranteed origin pair before the first camera publication has not been established. The initial eye coordinates alone cannot establish what an already-started worker reads before that publication. The ordinary frame producer reads the width for its half-width subtraction. An in-game route changing or reallocating that width after construction has not been established. Alternate origin writers, if any, and interleavings before the first complete update remain open.
