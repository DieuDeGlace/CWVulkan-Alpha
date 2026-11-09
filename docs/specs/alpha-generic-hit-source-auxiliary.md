# Alpha generic-hit source/self auxiliary production

Status: FINAL / STABLE / HELD. This independently worded source-side contract establishes the bounded auxiliary calculation, its native item/stat helpers, record production and direct source mutation. It does not certify a replacement implementation, complete generic-hit behavior or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. Addresses below are RVAs unless identified as creature, item or record offsets. The principal evidence is generic hit `0x196D30`, especially `0x1976D8–0x197A79`, item selector `0x03DF60`, numeric transform `0x03CA60`, and denominator helper `0x044CB0`. No other contract, reconstruction, replacement, SDK, tests, probes, captures, logs or implementation history supplies facts. No server counterpart is claimed.

## Admission and cached source ratio

Explicit arguments are numbered excluding the native world object. Target is argument 1, source is argument 2, and the auxiliary admission uses only the low byte of explicit argument 5, read at native frame `+0x18`. A nonzero upper byte with a zero low byte does not admit this phase. The incoming critical byte is a different argument and is not this admission byte.

The generic entry first rejects ordered nonpositive target binary32 health at creature `+0x16C`. With nonzero world mode byte `+0xB4`, it also rejects a target pointer equal to world's four-byte distinguished-creature pointer `+0xB8`. A nonnull source action byte `+0x68` equal to decimal 91 or 74 requires target byte `+0x5C` bit `0x01`. A null source takes a different continuation and does not enter this auxiliary phase.

Earlier terminal alternatives must also have been bypassed. One requires target control byte 1 and either target byte `+0x124` bit `0x80` or target byte `+0x5C` bit `0x20`. Another requires nonzero argument 14's low byte, target binary32 `+0x174` strictly greater than zero, and predicate `0x044590`: target signed 32-bit `+0x130` at most zero, with target action 71, 72, 8 or 98, or action 59 or 63 when both target class byte `+0x140` and specialization byte `+0x141` equal 1. Those alternatives return before this calculation. Earlier positive resident-type-6 processing can also finish the call when its subtraction leaves an ordered nonpositive remainder. The complete earlier absorption/stat calculation and application side effects are not reconstructed here.

After those continuations, `0x19754D–0x19757E` freshly calculates the ratio used by this phase. It converts source signed 32-bit `+0x70` to binary32, obtains a signed 32-bit denominator from `0x044CB0`, converts that denominator to binary32, divides the first converted value by the second, and stores the binary32 ratio. This phase does not cap that ratio at 1 or require either integer operand individually to be positive. It requires the resulting ratio to be strictly greater than zero. Zero, negative and unordered results fail the phase's admission.

The ratio is cached before an optional preceding item-operation region. A ratio strictly above exact binary32 0.25 can cause that separate region to select items and call `0x19A800` for each selected item before returning here. This auxiliary phase then selects its own fresh item-pointer list, while reusing the cached ratio rather than recalculating source `+0x70` or the denominator. The predecessor's full effects and random draws are explicit dependencies; this document does not reconstruct `0x19A800`.

Within the auxiliary phase itself, nonzero argument-5 low byte and strictly positive cached ratio permit item selection. A returned item count of zero skips the entire scan, its final counter caps, and self-record production. A nonzero count permits scanning and final caps, even if no selected item has contributing records. Only a strictly positive final auxiliary accumulation admits the self record and health addition. Zero, negative and unordered accumulations produce neither of those two operations.

Sources: `decompiled/client/functions/00596d30.c`, `00444590.c`; original `binary/Cube.exe`, entry/source gates `0x196D73–0x196DD1`, terminal target condition `0x196EC1–0x196EDE`, predicate admission `0x19701B–0x197043`, earlier remainder exit `0x1974E2–0x1974FB`, cached ratio `0x19754D–0x19757E`, optional predecessor `0x197584–0x1976D5`, immediate admission/selection/count `0x1976D8–0x19770C`, final positive test `0x19798D–0x197990`. Matching instructions are in `decompiled/client/assembly.txt`.

## Denominator helper and precision

Helper `0x044CB0` reads source binary32 `+0x17C`, multiplies it once by an exact binary32 coefficient, and converts that binary32 product to a signed 32-bit integer by truncation toward zero for finite, representable results. Its coefficient rules are:

| Source item/type condition | Coefficient |
| --- | --- |
| Source byte `+0xAA8` is 3 and subtype byte `+0xAA9` is 6 | 60 |
| Same type byte is 3 and subtype is 8 | 80 |
| Same type byte is 3 and subtype is 11, 12, 15, 16 or 17 | 20 |
| Same type byte is 3 and every other subtype | 50 |
| Source byte `+0xAA8` is not 3, and source byte `+0x7E` bit `0x10` is set | 20 |
| Source byte `+0xAA8` is not 3 and that bit is clear | 50 |

The source-flag alternative is considered only when byte `+0xAA8` is not 3. The helper does not inspect the item at source `+0x990`, the source action, source health, or an item record value for this denominator. In particular, subtype 7 uses coefficient 50 even though it has special priority in the separate item selector below.

There is no explicit zero-denominator guard or ratio clamp in the observed caller. Exceptional conversion, divide-by-zero, non-finite values and the machine's floating-point exception/control state are not given a replacement policy here. The confirmed arithmetic retains the intermediate binary32 product, signed-integer truncation, conversion of that integer back to binary32, and binary32 division. Replacing those stages with one higher-precision ratio is not the established relation.

Sources: `decompiled/client/functions/00444cb0.c`; original helper `0x044CB0–0x044D28`, subtype table at image RVA `0x044D2C`, caller `0x197553–0x19757E`. Coefficients are at image RVAs `0x2FCE48` (60), `0x2FCE50` (80), `0x2FCD74` (20) and `0x2FCD7C` (50); exact 0.25 is at `0x345D98`.

## Which resident source items are selected

Selector `0x03DF60` returns a native list of addresses of resident item objects, not copies of item contents. The only eligible object bases are source `+0xAA8` and source `+0x990`. At each base, its first byte is the type and its second byte the subtype. These numeric offsets and bytes are evidence identifiers; this contract does not assign unproved hand, weapon or equipment-slot names.

Priority first favors source `+0xAA8` when its type is 3 and subtype is 6 or 7. In that case the result contains exactly that one item, regardless of source action or the other item. Only if that priority does not apply is source `+0x990` considered for the same type-3/subtype-6-or-7 priority; when it qualifies, the result contains exactly that one item.

Without either priority, the source's current one-byte action at `+0x68` selects the following policy. Action values are decimal.

| Source action set | Eligible addresses, in returned order |
| --- | --- |
| 2, 4, 7, 8, 10, 18, 39, 41 | Only source `+0x990`, and only if its type byte is 3. |
| 40, 42 | Only source `+0xAA8`, and only if its type byte is 3. |
| 11, 12, 16, 17, 20, 54, 86, 91, 96 | Source `+0x990` if type 3, followed by source `+0xAA8` if type 3. Either or both may be absent. |
| Every other action byte | Source `+0xAA8` if type 3, followed by source `+0x990` only if its type is 3 and subtype is 12. |

Each listed qualification is independent within its applicable row. The returned count is a four-byte native list count and can be 0, 1 or 2 under these verified rules. The selector does not reject an otherwise selected item because its record count is zero, because it has no tag `0x81`, or because its numeric record values are nonpositive. It does not consume or overwrite either resident item.

The generic-hit caller scans items in exactly this returned order. The fresh selector invocation occurs after the optional predecessor mentioned above; it reads the action/type/subtype bytes at that invocation. The cached ratio remains the one computed before that predecessor.

Sources: `decompiled/client/functions/0043df60.c`, `004860b0.c`, `00583cb0.c`; original selector priority `0x03DFA9–0x03E045`, action dispatch/selection `0x03E047–0x03E0D6`, returned list/count `0x03E0D6–0x03E10A`; original action jump destinations at image RVA `0x03E120` and byte table at `0x03E130`. Native append `0x0860B0` increases the list count and appends each copied four-byte item address through `0x183CB0`.

## Record scan, transform and accumulation

For nonzero selected-item count, the caller starts the auxiliary accumulation at binary32 positive zero. It obtains a shared per-item scale by dividing native binary32 0.1, bits `0x3DCCCCCD`, by the selected-item count converted to binary32. The native count conversion passes through binary64 with unsigned-count compensation; counts 1 and 2 are represented exactly. The resulting scale is native binary32 0.1 for one selected item and native binary32 0.05, bits `0x3D4CCCCD`, for two. The divisor is the number of **selected items**, not the number of contributing records. An empty selected item still reduces the other selected item's scale.

Each selected item's signed 32-bit field `+0x114` supplies its record count. Counts at most zero skip that item's record walk. Otherwise records are visited in ascending index order: the first tag is the one-byte field at item `+0x17`, and its associated signed 32-bit value is at item `+0x18`; successive tags and values each advance by eight bytes. The count is reread for the continuation comparison. No smaller count limit, item-record validation, sorting or per-record positivity filter occurs in this observed walk. Memory-validity guarantees for malformed counts are outside the contract.

Only tags `0x80`, `0x81`, `0x82` and `0x83` call numeric helper `0x03CA60`. Other tag bytes are ignored here. For each recognized tag the associated signed integer is converted to binary32. The helper subtracts exact 1, multiplies that difference by native binary32 0.05, adds exact 1 to form a denominator, divides exact 1 by that denominator, and subtracts that reciprocal from exact 1. Every stage uses binary32 arithmetic. Its result is stored as binary32 before the native floating-point return and stored as binary32 again by the caller, so extended precision is not retained for the subsequent products. The helper contains no clamp, positive-value test, item lookup or random draw.

The recognized tags have these effects. “Transform result,” “item scale” and “cached ratio” below refer to the values just established; each multiplication and addition is a separate native binary32 operation.

| Tag | Working hit amount | Auxiliary amount and other mutations |
| --- | --- | --- |
| `0x80` | Transform result is multiplied by item scale, then by cached ratio; that result is added to the current working hit amount. | Does not add to the auxiliary accumulation and does not perform either counter update below. |
| `0x81` | Transform result is multiplied by item scale, then by cached ratio. One half of that intermediate is added to the working hit amount. | Twice that same intermediate is added to the auxiliary accumulation. The half and double are separate binary32 products; the working-amount addition precedes the auxiliary addition. |
| `0x82` | Item scale is first multiplied by exact 0.25; transform result is multiplied by that scaled quantity, then by cached ratio, then added to the working hit amount. | Does not add to the auxiliary accumulation. After the working-amount update, cached ratio is multiplied by exact 500, then added to target signed 32-bit `+0x134` converted to binary32; the result is truncated toward zero back into that signed field for finite representable values. |
| `0x83` | The same quantity and operation order as `0x82` are added to the working hit amount. | Does not add to the auxiliary accumulation. The corresponding cached-ratio-times-500 update is applied to source signed 32-bit `+0x138`, with the same conversion/addition/truncation stages. |

Only tag `0x81` can therefore change the auxiliary accumulation in this phase. Its transformed value is not independently required to be positive, so contributions may reduce or cancel earlier contributions. There is no per-item auxiliary record: one accumulation spans all visited records of all selected items, and only its final ordered-positive result is tested. Its calculation does not multiply by the incoming hit amount, incoming critical byte or hit vector. Earlier control flow can still determine whether this phase is reached.

Source/target counter values are not reused to refresh the cached ratio during this walk. Repeated tags `0x82` or `0x83` do read the most recently stored corresponding counter for their next conversion/addition. Their per-record counter increase is independent of the item scale and transformed record value. The working amount and auxiliary accumulation have no final clamp in this phase.

Sources: `decompiled/client/functions/00596d30.c`, `0043ca60.c`; original count/scale `0x1976FB–0x197748`, item/record walk `0x19774E–0x197954`, tag `0x81` arithmetic `0x19778C–0x1977E7`, tag `0x80` `0x1977F2–0x197828`, tag `0x82` `0x197833–0x19789D`, tag `0x83` `0x1978A5–0x197913`; four-tag jump table at image RVA `0x1986FC` confirms tag-to-operation membership. Numeric helper `0x03CA60–0x03CA94`; native 0.05 at `0x2FCDB4`, 0.1 at `0x2FCB7C`, 0.5 at `0x345DA0`, 2 at `0x345E10`, 0.25 at `0x345D98`, 500 at `0x2FCE70`, and count compensation doubles at `0x345F30/0x345F38`.

## Final counter caps, emitted record and health-write order

After a nonempty selected list has been scanned, the producer caps target signed 32-bit `+0x134` above at 5000, then source signed 32-bit `+0x138` above at 5000. Each cap uses signed greater-than; equality and negative values are retained. There is no lower cap here. The caps are deferred until all selected records have been processed and execute even when no recognized tag changed a counter or the final auxiliary accumulation is nonpositive. A zero selected-item count bypasses these caps.

Only after both caps does the producer require auxiliary accumulation strictly greater than positive zero. On success it constructs one 72-byte record with the following semantic values:

| Record-relative field | Width | Produced value |
| --- | --- | --- |
| `+0x00` | 64-bit identifier, copied as two words | Source creature `+0x08/+0x0C`. |
| `+0x08` | 64-bit identifier, copied as two words | The same source identifier again; no target identifier is used in this self record. |
| `+0x10` | Binary32 | Auxiliary accumulation with its sign bit toggled by native mask `0x80000000`. For a finite positive accumulation this is its negative amount. |
| `+0x14` | One byte | Zero critical byte, regardless of the incoming critical argument. |
| `+0x18`, `+0x1C` | Two 32-bit status fields | Zero. Their nonzero consumer interpretation is outside scope. |
| `+0x20–+0x37` | Three eight-byte position components | Raw 24-byte source position from source `+0x10` through `+0x27`. This producer does not convert it or use target position. |
| `+0x38`, `+0x3C`, `+0x40` | Three binary32 vector components | Positive zero. |
| `+0x44`, `+0x45`, `+0x46` | Three one-byte fields | Zero, including category byte `+0x45`. |
| `+0x15–+0x17`, `+0x47` | Padding | No initialized semantic value established; the semantic copier omits these bytes. |

Constructor `0x059A30` supplies the zero status/critical/position/vector/final-byte fields. The producer overwrites the position and separately fills identifiers and amount; the constructor itself does not initialize identifiers or amount. The sign operation is a bit toggle, not an absolute-value or saturating conversion.

The producer appends this record to the native list at the base of explicit output argument 9. Allocation/copy `0x051EE0` builds an 80-byte native node containing two four-byte links and the 72-byte semantic payload; list capacity is checked, the count is increased and tail linkage completed. Only **after** that append does the producer read source binary32 health `+0x16C`, add the positive auxiliary accumulation once, and store the binary32 result directly to that same source creature.

There is no world-mode or distinguished-source-pointer test in this auxiliary append/health block, no source-health positivity admission, no health-limit helper or upper health cap, and no call to separate application routine `0x195A60` for this record. It uses the original source pointer directly rather than resolving the emitted identifier again. The generic entry gates stated above still apply. Allocation/capacity failure is not a successful emission; the described health addition follows normal completed append. The earlier counter mutations/caps already precede this append attempt.

Source health addition and self-record append both occur before the later main target-record construction/application region. Appending this record does not itself specify what remote consumers do with its negative amount. The full later consumer, final generic-hit result and unrelated target/source effects remain outside this scoped calculation.

Sources: `decompiled/client/functions/00596d30.c`, `00459a30.c`, `004599a0.c`, `00451ee0.c`, `0046d810.c`; original counter caps and positivity `0x19795D–0x197990`, constructor/sign/position/identifiers `0x197996–0x197A21`, node copy/capacity/count/linkage `0x197A2E–0x197A5D`, direct source health arithmetic `0x197A5F–0x197A71`. The sign mask is at image RVA `0x345F70`; semantic constructor/copy are `0x059A30/0x0599A0` and allocation helper is `0x06D810`.

## Draw-order and remaining limits

There is no random-function call in auxiliary region `0x1976D8–0x197A79`, item selector `0x03DF60`, denominator helper `0x044CB0`, or numeric helper `0x03CA60`. Selection, the ratio/count gates, per-record arithmetic, caps, record filling and direct health addition are not chance-tested in this phase. The source/self production does not add a critical draw or presentation-pitch draw.

This is a scoped statement about the original routines and operations audited here. Earlier generic-hit presentation/application and the optional preceding `0x19A800` calls can run before it; later generic-hit phases and record consumers can run afterward. Their complete draw counts and side effects are not inferred. Allocator internals, corrupted item/list data, concurrent item/source mutation, non-finite or exceptional floating-point machine states, input meanings for unnamed item/action fields, the stat origin of source `+0x17C`, network serialization and server parity remain explicit dependencies. No fallback formula, extra clamp or inferred ownership gate is supplied for those missing cases.
