# Alpha terrain-slot bounds and indexed static-model admission

Source-side contract, 2026-11-21. **FINAL / STABLE / HELD** after complete author readback.

The supported original is the PE32 client `binary/Cube.exe`, preferred base `0x00400000`,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All addresses are RVAs. Unless specified otherwise, positions are signed 64-bit values with
65,536 raw units per voxel. Dimensions and rendering distances are in voxel units.

## Terrain-slot publication

The builder `0x09D910` receives logical horizontal slot coordinates. The worker at `0x0690A0`
selects these from the current window, admitting coordinates from zero through `0x7FFFF` on
each axis. For this domain, multiply each coordinate by 32 to obtain its first voxel column.
The builder selects a physical slot from client `+0x2E0`. For each axis it takes the logical
coordinate modulo the side length at `+0x2DC`, using X as the faster array index. Slots have a
`0x268`-byte stride. Reusing a physical slot does not change the logical coordinates from which
the bounds are produced. Original exports `004690a0.c`, `0049d910.c`, especially `0x09D971–0x09D9D0`.

The two published corners are three signed 64-bit components each. The table gives their values
in voxel units before multiplication by 65,536. The two height accumulators are described below.

| Axis | Lower corner | Upper corner |
| --- | --- | --- |
| X | `+0x20`, first X column | `+0x38`, first X column plus 33 |
| Y | `+0x28`, first Y column | `+0x40`, first Y column plus 33 |
| Z | `+0x30`, lower height accumulator | `+0x48`, upper height accumulator plus one |

The horizontal scan visits 32 columns on each axis, ending at first plus 31 inclusive. The
published upper X and Y both use the separate literal 33. For example, logical X slot 2 starts
at 64, scans through 95, publishes upper X 97 and obtains reference X 80.5. This horizontal
extent is independent of which columns contribute faces. Original image `0x09DB89–0x09DBDC`,
`0x0A08F0–0x0A09C0` and `0x0A115D–0x0A11AC`.

### Vertical extent

The preliminary pass visits X in increasing order, with increasing Y inside each X. Both height
accumulators start at zero, with a separate flag marking the first accepted height. On the
ordinary populated-column path, the column has a signed base at `+0x10` and a count at `+0x1C`.
The candidate scan starts one below the minimum of the current base and the available four
horizontal neighbor bases. Its exclusive upper limit is the greater of one and the current
column's base plus count. Original image `0x09DB89–0x09DE46`.

Before the per-height scan, the builder also raises the upper accumulator to the positive-Y
neighbor's base plus count when that signed sum exceeds its current value. The actual data
dependency is on the positive-Y neighbor. This update does not establish a lower bound or clear
the first-height flag. The following statements describe the path with the required neighbor
record available. The traced instructions do not establish a safe substitute when this final
neighbor lookup fails. Original image `0x09DDCA–0x09DE46`.

At a visited Z the builder examines the effective material in the low five bits of the voxel's
fourth byte. Zero does not contribute a height. A nonzero voxel qualifies if any of its six face
neighbors is zero, or if its own material differs from 2 and a face neighbor is 2. The tests visit
negative X, positive X, negative Y, positive Y, negative Z and positive Z, stopping at the first
qualifying face. The first qualifying Z sets **both** accumulators, overwriting an earlier
neighbor-top contribution if one exists. Later qualifying heights lower the minimum or raise
the maximum as appropriate. Later positive-Y column-top contributions can also raise the maximum.
The final vertical extent therefore depends on the scan order, effective voxel materials and
neighbor column records. Original image `0x09DE50–0x09E00F`.

The current-column lookup in this pass has explicit material substitutions. A position below
the stored base uses fallback material 1. A stored material zero at Z at or below zero becomes
material 2 when bit `0x40` is clear. Beyond the stored column the fallback is material 2 at Z
at or below zero and material zero above that. The current-column lookup's missing-record
branch also selects the material-1 fallback at `0x09DEBA`. These observations concern the
inner sample operation. The surrounding XY loop skips unavailable current columns earlier.
Original image `0x09DC80–0x09DC9D`, `0x09DE50–0x09DEC5`, and the fallback initializers
`0x2F9B10`, `0x2F9B40` and `0x2F9B70`. The full neighbor sampling and mesh emission contracts
are outside this record.

### Reference and consumers

The builder converts the six integer limits to 16.16 fixed positions. Under the slot critical
section at `+0x250`, it adds the corresponding raw lower and upper components. It scales each
sum by the fixed value 32,768. The helper at `0x02C900` multiplies and then divides by 65,536
using signed 64-bit arithmetic. The result at `+0x50`, `+0x58`, `+0x60` is the midpoint,
including the half-voxel contribution of an odd sum. The horizontal midpoint is always the first
column plus 16.5. The vertical midpoint includes the upper endpoint's plus one. Original image
`0x0A0F30–0x0A104D`, constant `0x302A88`, and original export `0042c900.c`.

The builder also takes the 3D difference between the midpoint and the upper corner, converts each
component to binary32 voxel units, takes its Euclidean length, and multiplies by 1.5 for slot
`+0x68`. It stores the two bounding corners and publishes the logical X/Y tags at `+0x18/+0x1C`,
clears `+0x74` and leaves the slot critical section. Original image `0x0A104A–0x0A11AC` and
`0x0A1373–0x0A1398`.

In scene routine `0x0AC260`, one slot-admission block requires nonzero slot `+0x0C`, a match
for both logical tags, and the box test at `0x07F3C0`. That test uses horizontal midpoint
distance and the box corners against six planes. For plane evaluation only, the client adds
its horizontal translations at `+0x1D8` and `+0x1E0` to X and Y. Z remains in its original
vertical coordinate. For each coefficient, the test uses the upper bound if the coefficient is
positive and the lower otherwise, rejecting on a strictly negative plane result for ordinary
finite inputs. After slot admission the client uses the **3D** fixed-point difference from
slot `+0x50` to the reference at client `+0x140`, storing the converted squared distance at
slot `+0x6C`. These consumers explain why the stored midpoint and its Z matter separately from
the 2D box-distance gate. Original image `0x0AC665–0x0AC737` and original exports `0047f3c0.c`,
`0042c7a0.c`, `0043ac20.c`.

## Indexed static scene records

In the traced scene pass, a nonzero client byte `+0x8006E6` bypasses the zone iteration. The
ordinary path visits the scene's inclusive zone window, skips missing zones and walks the
zone collection at `+0x0C` in its iteration order after the preceding collection at `+0x30`.
Each indexed record carries a signed 32-bit model index at `+0x00`, a fixed XYZ position at
`+0x08/+0x10/+0x18`, and three binary32 dimensions at `+0x24/+0x28/+0x2C`. A negative index
or an index greater than or equal to the current pointer count is skipped. The count is the
byte span from client `+0x800724` to its end pointer at `+0x800728` divided by four. On the
draw path the model pointer is fetched from that table at four times the index. An in-range
index alone establishes no guarantee about a missing model resource. Original image
`0x0B21C7–0x0B225F`, `0x0B2C95–0x0B2D17`, `0x0B3261–0x0B32D4`, `0x0BE784–0x0BE79D`
and original exports `00487f50.c`, `004be760.c`.

### Raised center and eligibility

The range supplied to the indexed-model gate comes from the binary32 scene distance formed at
`0x0AC4E0–0x0AC515` by adding the binary32 constant 22.4 to client `+0x1D4`. The static far
distance is that result multiplied by binary32 0.3. Its immediate-pass boundary is the rounded
far distance multiplied by binary32 0.9, with separate roundings in this order. Original image
`0x0B06F3–0x0B076C` and constants `0x302A94`, `0x2FCDC0` and `0x2FD31C`.

The client takes the largest dimension, checking X, then Y, then Z with strict greater-than
updates, and adds exactly 1.0 to obtain a binary32 sphere radius. Separately it takes the
**Z dimension** for the center displacement, halves it in double precision, multiplies by
65,536 and converts to a signed fixed-point integer. It adds this displacement to the record's
Z position, leaving X and Y at the record position. Neither rotation nor model table mesh
extrema enter these two calculations. Original image `0x0B2D17–0x0B2DF7`, constants `0x345DC0`,
`0x345DE8` and `0x2FD588`, and helpers `0x028970`, `0x059C00` and `0x02C800`.

The helper `0x07F760` first compares the squared 3D distance from client `+0x140` to the raised
center against the squared static far distance. The distance uses fixed-point component
squares, each rescaled by division by 65,536 before summation. The far threshold squares the
binary32 far distance and multiplies by binary32 65,536 before integer conversion. Equality
passes. The sphere radius does not enlarge this radial threshold. The helper then evaluates
six four-coefficient planes beginning at client `+0x1000FA4`, in stored order. X and Y receive
the same horizontal translations as the terrain box, while Z uses the raised absolute vertical
value. Each plane result adds the radius after its offset. A strictly negative result rejects,
and zero passes. Original image `0x07F760–0x07F930` and `0x0B2D72–0x0B2DF7`, and original
exports `0043ac20.c` and `0047f760.c`.

### Choosing and consuming the draw pass

After admission the client computes a **second** squared distance, this time from the original
record position to client `+0x140`. It subtracts in fixed point, converts each difference to
binary32 voxel units, squares the components and adds X squared to Y squared, then Z squared.
The raised center is used only for the earlier gate. A finite squared distance less than or
equal to the squared immediate boundary calls the static renderer `0x0BE760` during traversal.
A strictly greater value appends a 16-byte entry to the shared deferred collection. For an
indexed static entry the first pointer is zero, the second points to the original record, and
the float at `+0x08` is the unraised squared distance. The append does not draw the model.
Original image `0x0B300E–0x0B3097` and `0x0B3117–0x0B3169`, and original exports `0042c4a0.c`,
`00424860.c`, `004abc00.c` and `004c1190.c`.

Type-specific presentation calls can flank this choice. Index 6 with a nonzero byte `+0x30`
enters a separate effect path after admission and rejoins before the split. Index `0x41` has
an additional immediate-path effect check after the static renderer returns. These calls do
not replace the indexed static draw decision. Original image `0x0B2DFD–0x0B300E` and
`0x0B3169–0x0B3261`. Individual effects, transforms and materials are outside this scope.

After zone traversal, the client sorts the shared deferred range in descending order of the
binary32 key at entry `+0x08`. Other scene entries can participate in the same sort. Much
later in the same routine it consumes that range from first to last. A nonnull first pointer
dispatches to `0x0BD160`. A nonnull second pointer dispatches to `0x0BE760` with the saved
distance and static thresholds. The tracer therefore establishes immediate encounter order
and farthest-first deferred order for distinct finite keys, including the exact immediate
tie at the split. It establishes no stable tie order in the shared sort. Original image
`0x0B32ED–0x0B3335` and `0x0BA467–0x0BA588`. Original sort exports `004abac0.c`, `004aa490.c`,
`004aaeb0.c` and `004a9470.c` identify the key and ordering.

## Evidence limits

The named original function exports and `decompiled/client/assembly.txt` were checked against
the supported image. Across the 21 reviewed builder, renderer and helper routines, all 19,690
exported instruction rows that were compared matched the image. The scene export `004ac260.c` timed out,
so scene branches and argument order come from the instruction listing and image. This contract
covers ordinary finite coordinates, valid populated inputs and completed native paths. It does
not establish NaN or overflow behavior as gameplay policy, resource lifetime under concurrent
mutation, complete zone streaming, every scene pass or native runtime/visual acceptance.
