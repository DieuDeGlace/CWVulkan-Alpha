# Alpha generic-hit presentation payloads and draw ordering

Status: FINAL / STABLE / HELD. Independently worded, bounded source-side prose, re-certified 2026-11-22 against the original client image. It establishes original producer values and ordering; the missing volume/pitch consumer interpretation and other open points are listed under "Unresolved". It does not certify any replacement behavior or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred base `0x00400000`. Instruction and routine addresses below are RVAs. The scope is the generic hit routine at `0x196D30`: the presentation entries adjoining its early category-3, category-1 and category-5 hit records, plus its final status-success presentation. Numeric presentation kinds are retained without unproved sound names. Facts here come only from the original image and its disassembly; no other contract, reconstruction or implementation material supplies them. No server counterpart is claimed.

## Payload and position boundary

The generic hit routine takes fourteen stack arguments and is called with a receiver object. Its ninth stack argument is an event-output object. Hit records (72 bytes each) go to a list at that object's base; presentation entries go to a separate list at that object's `+0x08`. The presentation append helper `0x0864C0` operates on that `+0x08` list. It calls node helper `0x18D860`, which, when a node is obtained, copies six consecutive four-byte fields into it, so the payload is 24 bytes. The presentation list's size counter sits at output object `+0x0C` and is capped at 0x7FFFFFE entries before an error path; the hit-record list's counter sits at `+0x04`.

| Presentation-relative offset | Width | Established value source |
| --- | --- | --- |
| `+0x00`, `+0x04`, `+0x08` | Three binary32 components | Converted target position. |
| `+0x0C` | 32-bit numeric kind | 24, 91 or 23 as specified below. No signed numerical ordering is used here. |
| `+0x10` | Binary32 scalar | One draw-dependent value for the three early branches; exact 1 for status success. |
| `+0x14` | Binary32 scalar | Exact 1 for every entry audited here. |

All these entries take position from the target creature (the routine's first stack argument), using three consecutive signed 64-bit components at target `+0x10`, `+0x18` and `+0x20`. Each component is loaded as a 64-bit integer by the x87 unit, stored as binary32, and that binary32 value is then multiplied (SSE, binary32) by exact binary32 `1/65536`, bit pattern `0x37800000`. Position is neither the incoming hit vector nor the source position, and no vertical offset is added by these blocks.

Categories 3 and 1 use conversion helper `0x02C4A0`; category 5 and status success perform the same conversion and scale directly in the caller. Category 3 reads position before its hit record is built; categories 1 and 5 read it after their hit record has been appended but before its conditional application. Status success reads it before the main hit record is appended.

Sources: `binary/Cube.exe`; routines `0x0864C0–0x086502`, `0x18D860–0x18D8A5`, `0x02C4A0–0x02C4FF`; output-object argument load at `0x196D79`; presentation-list receivers formed at `0x196F22`, `0x1971DD`, `0x197495`, `0x1985B9`; inline position conversions `0x1973FF–0x19746F` and `0x198570–0x19860F`. The scale constant is at image RVA `0x2FCD94`. Matching ranges in `decompiled/client/assembly.txt`.

## Early branches: payload values and one direct draw each

Each of the three early presentation blocks calls the C runtime `rand` import (through the import slot at RVA `0x2FC468`) once, converts the signed 32-bit return to binary32, multiplies by native binary32 0.3, divides that product by exact binary32 32767, and then adds exact binary32 1. The result is stored in presentation `+0x10`. The order is multiplication, then division, then addition, each a binary32 operation; dividing the random return first is not the observed order. Native 0.3 has bits `0x3E99999A`. The other scalar at `+0x14` is exact binary32 1, bits `0x3F800000`, stored before the random call.

| Adjoining hit-record category (record byte `+0x45`) | Presentation kind at `+0x0C` | Direct random call | Presentation append call |
| --- | --- | --- | --- |
| 3 | Decimal 24, bits `0x00000018` | `0x196EF9` | `0x196F41` |
| 1 | Decimal 24, bits `0x00000018` | `0x1971C0` | `0x197204` |
| 5 | Decimal 91, bits `0x0000005B` | `0x197478` | `0x1974BC` |

In each block the position conversion precedes the random call. The kind and both scalars are filled before the complete payload is appended. No second direct random call occurs in these presentation blocks, and the conversion helper `0x02C4A0` introduces no draw.

These are branch-local direct-call counts, not complete generic-hit draw counts. Earlier source-action presentation and later application behavior are separate dependencies. Once a particular early branch has been entered, no receiver-mode or distinguished-source test guards its presentation append. The later application-mode gate does not suppress the preceding draw or append.

Sources: `binary/Cube.exe`, category-3 block `0x196EDE–0x196F41`, category-1 block `0x1971A5–0x197204`, category-5 block `0x1973FF–0x1974BC`. Binary32 constants are at image RVAs `0x2FCDC0` (0.3), `0x2FD338` (32767) and `0x345DC0` (1). Matching ranges in `decompiled/client/assembly.txt`.

## Order relative to hit append and application

The mode tested below is the byte at receiver object `+0xB4`; zero permits the application routine `0x195A60`, nonzero skips it.

For **category 3**, the position conversion, its random call and the presentation append all occur **before** the hit record is built. The record is then built and appended through `0x086290` at `0x196FEE`. The mode byte is checked afterward; mode zero permits application at `0x19700F`. The routine's low-byte result is false after this branch, including when both lists already received entries.

For **category 1**, the target-field updates and hit-record construction precede the hit append call at `0x1971A0`. Only after that call does this block convert position, draw once and append presentation kind 24 at `0x197204`. The later mode check permits application at `0x197229` only for mode zero. The low-byte result is false after this branch. The category-1 presentation therefore lies between hit append and conditional application, unlike category 3.

For **category 5**, the hit record is appended at `0x1973FA`, before the position conversion and random draw. Presentation kind 91 is appended at `0x1974BC`. The later mode test permits application at `0x1974DD` only for mode zero. The routine then subtracts an already computed amount from its third stack argument; when the difference is greater than zero it continues toward a main record, otherwise it finishes with a true low-byte result. The category-5 presentation is already appended before that decision, including the case where no main hit record follows.

These statements assume normal successful node allocation and append. They establish caller order only, not the application routine's own effects, queuing or draws. The full admission predicates of each branch are not reconstructed here.

Sources: `binary/Cube.exe`, category-3 presentation, construction, append and application `0x196EDE–0x197016`; category-1 hit append through application and return `0x197196–0x197230`; category-5 hit append, presentation, application and remaining-amount decision `0x1973FA–0x1974FB`; matching ranges in `decompiled/client/assembly.txt`.

## Final status-success presentation

The main record's eligible status region consumes a probability draw at `0x198545`, converts it to binary32, divides by exact binary32 32767, and succeeds only when the already computed binary32 threshold is strictly greater than that fraction. Equality and unordered comparison do not succeed. The threshold construction (including a preceding pass that can force it to zero) and full status admission remain dependencies, not a new probability formula in this contract.

After success, target routine `0x044FF0` returns an integer that is stored at main hit record `+0x18`, and the record's hit-vector Z component (`+0x40`) is replaced with exact binary32 10 (bits `0x41200000`). The routine also builds a presentation payload whose position is the converted target position described above, whose kind is decimal **23** (`0x17`), and whose **two scalar fields are both exact binary32 1**. This block makes no further random call: the probability draw precedes it and the scalars consume no draw. Nothing in the block tests whether the returned integer is positive before building the presentation entry.

The routine appends this presentation to the output object's `+0x08` list with its own inline tail insertion: node call `0x18D860` at `0x198627`, size-counter update at `+0x0C`, and linkage completed by `0x198656`. The main hit record's byte at `+0x46` is then settled (set to 1 when the routine's second stack argument has byte `+0x68` equal to 28, otherwise left as computed). The main hit record is appended next, through node call `0x051EE0` at `0x198687`, with linkage completed by `0x1986B2`. Only after that does mode zero permit application through `0x195A60` at `0x1986D0`. The status-success presentation therefore precedes the main hit append and application, and its emission has no mode gate in this block. The routine returns a true low-byte result after this region whether or not the status draw succeeded.

Sources: `binary/Cube.exe`, status draw and comparison `0x198545–0x198563`, returned-integer, vector and payload construction `0x198569–0x198623`, presentation node and append `0x198627–0x198656`, record byte and main hit append `0x198658–0x1986B2`, conditional application and result `0x1986B4–0x1986D7`; matching ranges in `decompiled/client/assembly.txt`.

## Unresolved

- Volume/pitch roles: the producer fixes both scalar offsets and their values, but this audit does not close the playback consumer that assigns volume and pitch roles. It does not claim `+0x10` is pitch and `+0x14` volume, or the reverse.
- The semantic role of the integer stored at main hit record `+0x18` by `0x044FF0` (earlier text called it a duration) is not established by this audit; only its source and placement are.
- Rounding: the x87 integer-to-binary32 store and the SSE integer conversion are assumed to run under default round-to-nearest control states; the control-word state at these sites is not established here.
- Branch admission predicates, threshold construction, the application routine's internal effects and draws, complete whole-routine draw counts, sound identity, attenuation, listener transforms, playback mixing and any server production are outside this bounded result.
- No lock, thread hand-off or cross-thread ordering claim is made; none is established for these blocks.

## Re-certification (2026-11-22)

A fresh source-only author re-checked this document against the original client image (hash, size, base, every cited address, offset, width, constant and import) and its disassembly. All payload offsets, kinds, constants, random-call sites, append sites and orderings were confirmed. Corrected or clarified: the main hit append at `0x198687` is a node call to `0x051EE0` with linkage completed by `0x1986B2`, rather than the append itself; the category-1 address `0x1971A0` is the hit append call, not its completion; the source-action byte is now described by its offset and condition; the helper ranges now end at their final instruction bytes. The "duration" naming of the `+0x18` integer and the rounding-mode assumption were moved to Unresolved. The volume/pitch consumer mapping, admission predicates, application internals and any lock or thread behavior remain open.
