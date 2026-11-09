# Alpha generic-hit presentation payloads and draw ordering

Status: FINAL / STABLE / HELD. This is independently worded, bounded source-side prose. It establishes original producer values and ordering, with the missing volume/pitch consumer interpretation stated explicitly. It does not certify replacement behavior or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred base `0x00400000`. Instruction and routine addresses below are RVAs. The scope is generic hit `0x196D30`'s presentation entries adjoining its early category-3, category-1 and category-5 records, plus its final status-success presentation. Numeric presentation kinds are retained without unproved sound names. No other contract, reconstruction, replacement, tests, captures or implementation history supplies facts here. No server counterpart is claimed.

## Payload and position boundary

The presentation append helper `0x0864C0` receives the event-output object at explicit generic-hit argument 9 plus **eight bytes**. This is a different list from the 72-byte hit records appended at that output object's base. Its copy helper `0x18D860` copies six consecutive four-byte fields into the new native node, establishing a 24-byte payload with no omitted payload field in this producer boundary.

| Presentation-relative offset | Width | Established value source |
| --- | --- | --- |
| `+0x00`, `+0x04`, `+0x08` | Three binary32 components | Converted target position. |
| `+0x0C` | 32-bit numeric kind | 24, 91 or 23 as specified below. No signed numerical ordering is used here. |
| `+0x10` | Binary32 scalar | One draw-dependent value for the three early branches; exact 1 for status success. |
| `+0x14` | Binary32 scalar | Exact 1 for every entry audited here. |

All these entries take position from the **target creature**, beginning at target `+0x10`, with three consecutive signed 64-bit components at target `+0x10`, `+0x18` and `+0x20`. Each component is converted to binary32 and then multiplied by exact binary32 `1/65536`, bit pattern `0x37800000`. The native conversion first stores the converted integer as binary32, before that multiplication. Position is neither the incoming hit vector nor source position, and no vertical offset is added by these blocks.

Categories 3 and 1 use native conversion helper `0x02C4A0`; category 5 and status success perform the same signed-integer conversion and scale directly in the caller. Category 3 reads position before its hit record is constructed; categories 1 and 5 read it after their hit record has been appended but before its conditional application. Status success reads it before the main hit record is appended.

**Volume/pitch limit:** the producer and semantic copy identify both scalar offsets and their exact values. This bounded audit does not close the playback consumer that assigns volume and pitch roles to those offsets. It therefore does not claim that `+0x10` is pitch or that `+0x14` is volume, or the reverse. The familiar draw-dependent range is insufficient evidence for that naming. No additional presentation argument is supplied or inferred.

Sources: `decompiled/client/functions/004864c0.c`, `0058d860.c`, `0042c4a0.c`; original routines `0x0864C0`, `0x18D860`, `0x02C4A0–0x02C4FD`; `binary/Cube.exe`, presentation receivers `0x196F22`, `0x1971DD`, `0x197495`, `0x1985B9`; inline position conversions `0x1973FF–0x19746F`, `0x198570–0x19860F`. Scale constant is at image RVA `0x2FCD94`.

## Early branches: payload values and one direct draw each

Each of the three early presentation blocks calls the original random function once, converts the signed 32-bit return to binary32, multiplies by native binary32 0.3, divides that binary32 product by exact binary32 32767, and then adds exact binary32 1. The result is stored in presentation `+0x10`. The order is multiplication, division, addition, with native binary32 operations; dividing the random return first is not the observed operation order. Native 0.3 has bits `0x3E99999A`. The other scalar at `+0x14` is exact binary32 1, bits `0x3F800000`.

| Adjoining hit-record category | Presentation kind at `+0x0C` | Direct random call | Presentation append |
| --- | --- | --- | --- |
| 3 | Decimal 24, bits `0x00000018` | `0x196EF9` | `0x196F41` |
| 1 | Decimal 24, bits `0x00000018` | `0x1971C0` | `0x197204` |
| 5 | Decimal 91, bits `0x0000005B` | `0x197478` | `0x1974BC` |

The position conversion precedes that branch's random call. The caller fills the numeric kind and both scalars before appending the complete presentation payload. No second direct random call occurs in those specified presentation blocks. Conversion helper `0x02C4A0` only reads/converts/stores position and introduces no random draw.

These are branch-local direct-call counts, not complete generic-hit draw counts. Earlier source-action presentation and later application behavior are separate dependencies. There is no native caller-side world-mode or distinguished-source test guarding the presentation append once its particular early branch has been admitted. The later application-mode gate does not suppress the preceding presentation draw or append.

Sources: `binary/Cube.exe`, category-3 block `0x196EDE–0x196F41`, category-1 block `0x1971A5–0x197204`, category-5 block `0x1973FF–0x1974BC`; `decompiled/client/functions/00596d30.c`, `0042c4a0.c`. Native binary32 constants are at image RVAs `0x2FCDC0` for 0.3, `0x2FD338` for 32767 and `0x345DC0` for 1. The original corresponding instruction ranges are in `decompiled/client/assembly.txt`.

## Order relative to hit append and application

For **category 3**, the target-position conversion, its random call, and presentation append all occur **before** hit-record construction. The hit record is then constructed and appended through `0x086290` at `0x196FEE`. World mode byte `+0xB4` is checked afterward; mode zero permits the application call to `0x195A60` at `0x19700F`. Nonzero mode skips application. The generic-hit low-byte result is false after this branch, including when both lists already received entries.

For **category 1**, target-resource updates and hit-record construction have already occurred when the hit append completes at `0x1971A0`. Only then does this block convert target position, draw once, and append presentation kind 24 at `0x197204`. The later world-mode check permits application at `0x197229` only for mode zero. The generic-hit low-byte result is false after this branch. Thus the category-1 presentation is between hit append and conditional application, unlike the category-3 presentation.

For **category 5**, the hit record is appended at `0x1973FA`, before the target-position conversion and random draw. Presentation kind 91 is appended at `0x1974BC`. The later mode test permits application at `0x1974DD` only for mode zero. The producer then calculates its remaining amount and decides whether to continue toward a main record or finish. The category-5 presentation is already appended before that remaining-amount decision, including the case where no main hit record follows.

These statements assume normal successful allocation/append. They establish caller order rather than the application routine's own further effects, queuing or random draws. The complete entry/caller admission predicates are not reconstructed in this small presentation contract.

Sources: original `binary/Cube.exe`, category-3 presentation/construction/append/application `0x196EDE–0x197016`; category-1 hit append through application/return `0x197196–0x197230`; category-5 hit append, presentation, application and remaining amount `0x1973FA–0x1974FB`; matching portions of `decompiled/client/functions/00596d30.c` and original assembly.

## Final status-success presentation

The main record's eligible status region consumes a probability draw at `0x198545`, converts it to binary32, divides by exact binary32 32767, and succeeds only when its already computed threshold is strictly greater than that fraction. Equality and unordered comparison do not succeed. The threshold construction and full status admission remain dependencies rather than a new probability formula in this contract.

After success, target helper `0x044FF0` supplies the hit record's duration, and the producer replaces its hit-vector Z component with exact binary32 10. It also builds a presentation payload whose position is the converted target position described above, kind is decimal **23** (`0x17`), and **both scalar fields are exact binary32 1**. This successful presentation block has no additional random call: the probability draw precedes it, and the payload's two scalars do not consume a draw. Nothing in this block tests whether the duration helper's returned integer is positive before constructing the presentation entry.

The producer appends this presentation to the output object's list at `+0x08` using its expanded tail-insertion operations, with copy/allocation call `0x18D860` at `0x198627` and tail linkage completed by `0x198656`. The main hit record is appended later at `0x198687`, after the main record's final source-action byte has been settled. Only after that hit append does mode zero permit application through `0x195A60` at `0x1986D0`. The status-success presentation therefore precedes main hit append and application, and its emission itself has no world-mode gate in this block.

The exact volume/pitch mapping remains the same missing consumer dependency stated above. Sound-file identity, attenuation, listener-relative transforms, playback rejection/mixing, external audio state, application-internal draws, complete whole-routine draw counts and server production are outside this bounded result.

Sources: original `binary/Cube.exe`, status draw/comparison `0x198545–0x198563`, duration/vector and payload construction `0x198569–0x198623`, presentation copy/append `0x198627–0x198656`, main final byte and hit append `0x198658–0x1986B2`, conditional application `0x1986B4–0x1986D0`; `decompiled/client/functions/00596d30.c`, `0058d860.c`, and matching original assembly. Constructor/copy names, pseudocode and native instruction text are not reproduced here.
