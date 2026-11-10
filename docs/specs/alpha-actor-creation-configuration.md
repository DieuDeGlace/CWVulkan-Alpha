# Alpha actor creation configuration and action-92 selection

Status: FINAL / STABLE / HELD after complete author readback.

This independently authored contract concerns the original client's full 32-bit actor member `+0x13F4`, its collection of four-byte entries at `+0x13F8`, their identified construction/configuration/copy/reset boundaries, and their consumer in the world update's action-92 branch. It also closes one bounded generated-appearance case, kind 37 on a freshly constructed actor. Raw numeric classifications are retained without assigning gameplay or ownership names.

The supported original is `binary/Cube.exe`, image base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All evidence paths below are relative to `disassembled alpha cube world`. Evidence consists of that image, `decompiled/client/assembly.txt`, and the individually identified original function files cited below. No earlier contract, reconstruction, replacement code or runtime material was consulted for this scope.

## Storage, fresh values and preservation

Actor `+0x13F4` is a separate four-byte scalar, subsequently compared at full width. The collection header consists of three 32-bit pointers at `+0x13F8`, `+0x13FC` and `+0x1400`: begin, logical end and allocated end. Each logical entry occupies four bytes. Empty means begin equals logical end; the ordinary size query takes the 32-bit pointer difference and arithmetic-shifts it by two. Indexed access returns begin plus four times the supplied index, without a bounds check in that helper. These are valid-container relations, not promises for malformed pointers or inconsistent extents. [Evidence: native helpers `0x077240`, `0x087F50`, `0x068C60`; original files `00477240.c`, `00487f50.c`, `00468c60.c`.]

Fresh actor construction at `0x03B690` explicitly writes full 32-bit value 1 at `+0x13F4`, then zeroes each of the three collection pointers. It does not populate a default candidate entry. The adjacent collection at `+0x13E8` is independently initialized; it is used for configured action values and must not be substituted for this candidate collection. [Evidence: native RVAs `0x03B991–0x03B9CD`; `decompiled/client/functions/0043b690.c`.]

The inspected actor reset `0x046330`, including its body reset `0x0465C0` and path reset, does not write the scalar or candidate header. It therefore does not restore construction's 1/empty configuration. The body initializer `0x03C100`, assignment `0x04B040` and copy construction `0x066050` operate on the actor body beginning at actor `+0x10`; their inspected state extent ends before these configuration members. Those body operations do not copy a source actor's `+0x13F4` or candidate collection into another actor. Thus a fresh actor followed only by such a body replacement retains 1/empty, whereas an existing actor passing through the inspected reset/body replacement retains its existing configuration. This does not close every possible alias-based reconstruction or whole-object copy. [Evidence: native routines `0x046330–0x0465BE`, `0x0465C0–0x046797`, `0x03C100`, `0x04B040`, `0x066050`; original files `00446330.c`, `004465c0.c`, `0043c100.c`, `0044b040.c`, `00466050.c`.]

One constructor cleanup entry at `0x2E2B4E` addresses actor `+0x13F8` and reaches `0x1FB860`, which deallocates the begin pointer when nonnull. This establishes a scoped storage-release boundary, not an ordinary reset to a usable empty header. Complete actor destruction, all exception paths and every potential indirect writer are not reconstructed here.

## Descriptor configuration source

The descriptor object constructed by `0x1ADB20` has the corresponding scalar at `+0x10B8` and the corresponding candidate header at `+0x10BC/+0x10C0/+0x10C4`. Its fresh scalar is 0, and all three header pointers are zero. This descriptor default differs from the fresh actor default. Its separate action collection is at `+0x10AC`. Relevant raw inputs also include a full 32-bit value at `+0x28`, full 32-bit kind at `+0x2C`, bytes at `+0x30/+0x31`, and a 16-bit word at `+0x7A`. Fresh descriptor construction sets `+0x28` to 1, `+0x2C` to 4 and the two bytes at `+0x30/+0x31` to zero; the appearance initialization at `+0x74` establishes the overlapping word at `+0x7A` as zero. [Evidence: `decompiled/client/functions/005adb20.c`; native RVAs `0x1ADB20–0x1ADC9B`.]

Builder `0x1C7730` takes the descriptor and a second argument whose low byte controls the later setup phase. It does not clear either configuration collection on entry. The complete direct write relation for the scoped scalar and candidate collection is as follows; earlier rows can prevent later rows, and the two appending rows can both apply to one call.

| Reached builder condition | Scalar and candidate effect |
| --- | --- |
| Descriptor word `+0x7A` has mask `0x0040` | Immediate return; no scoped scalar or candidate write. |
| That mask is clear, but byte `+0x30` is any value `0x80..0x89` | A separate item/stat setup returns before the generic configuration phases; no scoped scalar or candidate write. |
| Generic phase, word `+0x7A` has neither bit of `0x0018`, full `+0x28` is not 6, and full kind `+0x2C` is `0x75` | Store full 32-bit 0 at `+0x10B8`; append full 32-bit `0x11`, then `0x60`, to the existing candidates. This occurs before the second-argument low-byte test. |
| Generic phase, second argument's low byte is zero, and word `+0x7A` has mask `0x0200` | After the common stat/setup phase and one action-selection draw described below, take a new imported random result, store its signed remainder by 3 at `+0x10B8`, then append `0x11`, followed by `0x60`. |
| Other reached generic cases | No further direct write to the scoped scalar or candidate collection. Existing values and entries survive. |

The earlier generic test for kind `0x76` bypasses the special-kind equipment branch, and values `+0x28 = 6` or word mask `0x0018` bypass that branch as well. They do not themselves suppress the later mask-`0x0200` phase when the second argument's low byte is zero. A nonzero second-argument low byte skips that later phase, including its scalar draw and candidate appends; it does not undo the earlier kind-`0x75` write. [Evidence: `decompiled/client/functions/005c7730.c`; native RVAs `0x1C7758–0x1C7882`, `0x1C7E1F–0x1C7E3B`, `0x1C8035–0x1C81A6`.]

Each append uses `0x26ADD0`, which places the complete four-byte input at the logical end and advances the end by four, growing storage when required. It preserves earlier entry order and does not deduplicate or replace equal values. Consequently a fresh descriptor meeting both appending conditions ends with candidates `[17, 96, 17, 96]`; one meeting only either condition ends with `[17, 96]`. Repeated eligible builder calls append further pairs to existing entries. Allocation failure, invalid extents and exceptional paths remain outside the ordinary append relation. [Evidence: native RVAs `0x1C7841–0x1C7872`, `0x1C816C–0x1C8199`, helper `0x26ADD0–0x26AE32`.]

The mask-`0x0200` phase first constructs a temporary four-byte action list in the exact order `[92, 93, 86, 89, 87]`. One imported random result selects an index by unsigned remainder over that list's size; the chosen action is appended to descriptor `+0x10AC`. Only then is the separate random result for the full-width scalar's signed remainder by 3 obtained. For ordinary nonnegative random results, the scalar is 0, 1 or 2. The candidate pair is appended after that scalar write. The five-action list is then released. Candidate values 17 and 96 are passed as kind values by the consumer below; they are not entries of this temporary five-action list. [Evidence: native RVAs `0x1C80DC–0x1C81A1`.]

The builder ends its ordinary generic route with six direct random calls, storing the full 32-bit results at descriptor `+0x7B4`, `+0x8CC`, `+0x69C`, `+0x354`, `+0x584`, then `+0x46C`, in that order. On the special byte-`0x88` route there is instead one direct draw for its item byte; the other special bytes `0x80..0x89` take no such draw, and the generic six-call tail is skipped. The `0x0040` early return precedes all these direct draws. Other generic equipment branches can draw before the mask-`0x0200` phase; their complete appearance/equipment draw counts are not closed here. [Evidence: native RVAs `0x1C81A6–0x1C8260`, full direct call sites of `0x1C7730`.]

Two bounded configurations demonstrate the input relations that supply action 92. These descriptions concern fresh descriptor state and the stated raw gates; they do not assert how often a world generator chooses those inputs.

| Fresh descriptor inputs | Result before descriptor-to-actor transfer |
| --- | --- |
| Kind `0x51`, byte `+0x30` outside `0x80..0x89`, word masks `0x0040`, `0x0018`, `0x0200` clear, `+0x28` not 6, second argument low byte zero | Action entries `[87, 92]`; scalar 0; candidates empty. The scoped builder path has six direct random calls, all in the common item-field tail. |
| Kind `0x75`, same gates | Action entry `[92]`; scalar 0; candidates `[17, 96]`. Again six direct random calls occur in the common tail. |
| Either of those configurations with word mask `0x0200` additionally set | Its existing action entries receive one selected member of `[92, 93, 86, 89, 87]`; scalar becomes the later random result's signed remainder by 3. Kind `0x51` has `[17, 96]`; kind `0x75` has `[17, 96, 17, 96]`. The two direct selection draws precede the six tail draws, for eight direct builder draws on these particular paths. |

For kind `0x51`, action 87 is appended by the earlier kind branch and action 92 by the low-byte-zero stat phase; a nonzero setup argument skips that second append. For kind `0x75`, action 92 and the first candidate pair are already supplied by the earlier branch, so a nonzero setup argument still preserves those additions. The full upstream choice of kind/classification, every earlier generator draw, and whether an actor later chooses a configured action remain separate dependencies. Native calls with a zero setup argument are visible at `0x1D79C9`, `0x1D99FD`, `0x1DECFE` and `0x1ED34F`; their complete generators are not given a universal configuration contract here. [Evidence: native RVAs `0x1C782B–0x1C7872`, `0x1C796D–0x1C79A9`, `0x1C7FB0–0x1C7FB7`; individually identified original routines `0x1D6040`, `0x1D9460`, `0x1DC6E0`, `0x1E4850`.]

## Refresh, transfer and lifetime boundaries

The original world activation path can call the same builder with second argument 1 before the actor is constructed. At the reached refresh boundary, the context pointer must be nonnull, its full 32-bit member `+0x34` must equal 5, and descriptor word `+0x7A` must have mask `0x1000`. When descriptor `+0x28` is not 6, context `+0x3C` is copied as a full 32-bit value to descriptor `+0x34`, and context byte `+0x40` to descriptor byte `+0x58`. Next, context full 32-bit `+0x30` is added to the low 32-bit half of descriptor identifier `+0x48`, with 32-bit wrap, and supplied to imported `srand`. Builder invocation follows that seed call. The identity and earlier production of this context's fields are not closed here; no timer, coordinate or faction label is inferred. [Evidence: native RVAs `0x20FCFE–0x20FD4A`.]

This refresh can append another pair and store scalar 0 for the eligible kind-`0x75` configuration, despite the nonzero setup argument. It skips the later random scalar assignment and mask-`0x0200` pair. Thus an existing descriptor's earlier configuration must not be recomputed from a fresh default at this boundary. No saved-seed restoration is established by this local sequence; broader world seeding remains outside this audit.

Later in activation, descriptor action collection `+0x10AC` is copied to actor `+0x13E8` first. The full descriptor scalar at `+0x10B8` is then copied to actor `+0x13F4`, followed by candidate collection assignment from descriptor `+0x10BC` to actor `+0x13F8`. These operations precede the subsequent inventory setup and amount query visible in that activation segment. The scalar write occurs before candidate assignment, with no transactional rollback demonstrated if later allocation fails. The same activation separately maps descriptor full kind `+0x2C` to actor `+0x64`, and bytes `+0x30/+0x31` to actor `+0x140/+0x141`. Descriptor `+0x28` supplies actor discriminator byte `+0x60`; no gameplay name follows from these raw identities. [Evidence: native RVAs `0x21002B–0x210043`, `0x210376–0x2103BF`.]

Collection assignment `0x27A7B0` preserves the full four-byte entry values and their sequence. Self-assignment returns unchanged. An empty source sets destination end to its begin while retaining destination allocation. A nonempty source is copied into existing storage when it fits, otherwise the destination allocation can be released and replaced through the allocation/copy helpers. The ordinary result is a value copy, not an alias to the descriptor's candidate allocation. Capacity and exception behavior are not conflated with logical candidate contents. [Evidence: original file `0067a7b0.c`; native RVAs `0x27A7B0–0x27A8B3`, imported `memmove`, reached helpers `0x25AE10`, `0x23A710`.]

Descriptor destruction `0x1ADCA0` releases a nonnull candidate begin pointer separately from its action collection. The scoped cleanup does not establish a reusable zeroed descriptor after destruction. This and the actor's independent value assignment delimit candidate-storage ownership; full descriptor persistence, reconstruction and unknown indirect copies remain open. [Evidence: native RVAs `0x1ADCC2–0x1ADCF3`; original file `005adca0.c`.]

## Action-92 consumer and source choice

The original action-byte dispatch at `0x211E04–0x211E1B` reads actor `+0x68`; the original image's two-stage tables map value 92 to `0x2157A8`. At that branch, the source actor **A** is admitted when it is W's designated pointer `+0xB8`, or when W's byte `+0xB4` is zero and A's discriminator byte `+0x60` is nonzero. No ownership names are assigned to these raw conditions.

Creation also requires the two threshold-query boundaries to be crossed: `0x03CAA0(A,-1)` is called, then the signed 32-bit `+0x6C` read must be no greater than that result. The helper is called a second time, then the signed comparison requires the newly read `+0x6C` plus the world update's 32-bit amount, with 32-bit wrap, to be strictly greater than the second result. Equality at the second boundary does not create. The two calls and two reads are retained separately. The threshold helper's full internal relation, earlier common-action admission and later general update behavior are not restated here. A subsequent map scan counts nonnull actors whose full 64-bit `+0x198` equals A's stored identifier `+0x08`; a count above two suppresses this creation. That scan completes before construction and the candidate draw. [Evidence: native RVAs `0x2157A8–0x215938`; original action tables at image RVAs `0x222C84`, `0x222CCC`.]

After new actor **N** is constructed, its discriminator byte `+0x60` becomes 3 when A's discriminator is zero, otherwise 1. The candidate collection is then tested. If nonempty, exactly one direct imported random call occurs; its result is reduced by unsigned remainder over the valid collection count, and the selected four-byte entry is read. If empty, the candidate value is the full 32-bit constant 37 and no selection draw occurs. There is no removal, rotation, reordering or write-back of the candidate list. This selection happens before the full-width scalar branch, so a nonempty list spends its selection draw even when the ensuing copy alternative ignores the selected value. [Evidence: native RVAs `0x215938–0x215998`.]

| Full 32-bit A `+0x13F4` | Reached kind/equipment/appearance relation |
| --- | --- |
| Exactly 1 | Copy A's full 32-bit kind at `+0x64`, its equipment starting at `+0x300`, then its appearance starting at `+0x78`, in that order. The selected candidate has no effect on those copies. |
| Exactly 2 | Look up the full 64-bit key at A `+0x11D0` in W's actor map through `0x02F000`. Use the mapped pointer when nonnull; otherwise fall back to A. Copy that chosen actor's kind, equipment and appearance in the same order. |
| Every other full-width value, including 0 | Store the selected full 32-bit candidate at N `+0x64`, then call `0x03F7C0` with N's kind address, N's appearance address and a full-width null third argument. This branch does not copy A's equipment. |

These are exact 32-bit equality partitions. For example, a scalar with low byte 1 and nonzero upper bytes takes generation, not the value-1 copy path. The value-2 lookup has no separate zero-key, self-key, role or source-health test in `0x02F000`; an absent entry and a present null pointer both cause the caller's fallback. A nonnull result is used even if it aliases A. Missing identifiers are not replaced by a second invented key. [Evidence: native RVAs `0x215998–0x215A2E`; helper `0x02F000–0x02F027`, original file `0042f000.c`.]

Equipment copy `0x04AF00` visits thirteen item records, ascending from source/destination `+0x300` at stride `0x118`. Its item helper `0x02C5E0` copies bytes `+0/+1`, 32-bit fields `+4/+8`, bytes `+0x0C/+0x0D/+0x0E`, the 16-bit field `+0x10`, all 32 eight-byte records at `+0x14..+0x113`, then the 32-bit field `+0x114`. It does not copy intervening padding bytes `+2/+3/+0x0F/+0x12/+0x13`. Appearance copy `0x059800` copies bytes `+0..+4`, the 16-bit word `+6`, the three 32-bit fields `+8/+0x0C/+0x10`, eight successive 16-bit fields `+0x14..+0x23`, then all successive 32-bit fields `+0x24..+0xAB`; byte `+5` is skipped. These inspected copy helpers perform no numeric conversion, random draw or seed call. The item payloads' broader semantics are outside this configuration contract. [Evidence: original files `0044af00.c`, `0042c5e0.c`, `00459800.c`; native routines of the same RVAs.]

After any alternative, the caller clears word mask `0x0200` and sets `0x0800` in N `+0x7E`, which is appearance-relative `+6`. Other bits survive. Therefore even the copied appearance's flag word is subsequently modified. These branches do not copy A's creation scalar or candidate collection into N: N retains its own fresh 1/empty configuration through the inspected creation/publication segment. That segment's other position, identifier, amount and behavior-object setup does not supply a new direct writer for these two members. Their later action admission is not inferred from the appearance or kind alone. [Evidence: native RVAs `0x215A2E–0x215BA6`.]

Consequently the admitted builder configuration with kind `0x51`, scalar 0 and empty candidates supplies action 92's fallback generation of kind 37. The kind-`0x75` scalar-0 configuration supplies generation from `[17,96]`. The random scalar configuration supplies generation on result 0, A-copy on result 1, and key-lookup/fallback-copy on result 2, with its candidate draw preceding all three alternatives. The fresh actor default supplies A-copy if action 92 is otherwise admitted. These are concrete producer-to-consumer relations; the full world-generation distribution, input/AI action choice and existence of every classification combination are not proved here.

## Bounded generated appearance: fresh kind 37

The kind-37 generation result can be closed without assuming an asset's meaning. Actor construction initializes the appearance at actor `+0x78` using `0x028750`. The generated branch then calls `0x03F7C0` with null optional input. Before kind dispatch, that helper always makes five direct imported random calls: the first three form temporary byte values, then the fourth and fifth form signed remainders by 100 in temporary 32-bit fields. The kind-37 path does not read those generated values into the destination. The kind remains full 32-bit 37. Thus these five draws still advance the shared imported stream even though their values do not affect this bounded appearance.

The original jump table maps kind 37 to `0x0419EC`. After the common three binary32 writes at appearance `+8/+0x0C/+0x10`, this case stores 16-bit `0x095F` at `+0x1C`, copies three positive-zero 32-bit components to `+0x64/+0x68/+0x6C`, stores binary32 1.5 at `+0x28`, copies binary32 `0.8` to each of `+8/+0x0C/+0x10`, and ORs word mask `0x0029` at `+6`. The two triple-copy calls are to `0x011E30`, which copies three 32-bit words in component order and has no random or seed call. There are no further direct random calls on this case's path. The subsequent action-92 mask update changes fresh word `0x0029` to `0x0829`. [Evidence: original files `0043f7c0.c`, `00428750.c`, `00411e30.c`; native RVAs `0x03F7C0–0x03F962`, `0x0419EC–0x041A53`; image jump table at RVA `0x043CA0`.]

The following table covers the complete appearance extent `+0..+0xAB` for this fresh case after the caller's flag update. Offsets are relative to N's appearance, not N itself. Hexadecimal binary32 bits are authoritative; no decimal approximation or semantic body-part label is required.

| Appearance offset(s) | Width | Result |
| --- | --- | --- |
| `+0`, `+1` | Byte each | `0x00` |
| `+2`, `+3`, `+4` | Byte each | `0xFF` |
| `+5` | Byte | Not initialized by the inspected constructor or this generation/caller path; no fixed value asserted. |
| `+6` | 16-bit word | `0x0829`; helper-return value before the caller mask update is `0x0029`. |
| `+8`, `+0x0C`, `+0x10` | Binary32 each | `0x3F4CCCCD` |
| `+0x14`, `+0x16`, `+0x18`, `+0x1A`, `+0x1E`, `+0x20`, `+0x22` | 16-bit each | `0xFFFF` |
| `+0x1C` | 16-bit | `0x095F` |
| `+0x24` | Binary32 | `0x3F8147AE` |
| `+0x28` | Binary32 | `0x3FC00000` |
| `+0x2C`, `+0x34`, `+0x40`, `+0x44` | Binary32 each | `0x3F800000` |
| `+0x30` | Binary32 | `0x3F7AE148` |
| `+0x38` | Binary32 | `0x3F733333` |
| `+0x3C` | Binary32 | `0x3F4CCCCD` |
| `+0x48`, `+0x4C`, `+0x50`, `+0x54`, `+0x58`, `+0x5C`, `+0x60`, `+0x64`, `+0x68`, `+0x6C`, `+0x70` | 32-bit each | `0x00000000` |
| `+0x74` | Binary32 | `0x3F000000` |
| `+0x78` | Binary32 | `0x40A00000` |
| `+0x7C` | Binary32 | `0x40C00000` |
| `+0x80`, `+0x84` | 32-bit each | `0x00000000` |
| `+0x88` | Binary32 | `0x40400000` |
| `+0x8C` | Binary32 | `0x3F800000` |
| `+0x90` | Binary32 | `0xC1280000` |
| `+0x94` | 32-bit | `0x00000000` |
| `+0x98` | Binary32 | `0xC1000000` |
| `+0x9C` | Binary32 | `0x40000000` |
| `+0xA0`, `+0xA4`, `+0xA8` | 32-bit each | `0x00000000` |

This table depends on fresh appearance initialization. Reusing an arbitrary appearance buffer gives only the explicitly described generation writes, not a complete reset to the table. The common helper initially writes bits `0x3F75C290`, `0x3F75C290`, `0x400A3D71` at `+8/+0x0C/+0x10`; kind 37 subsequently replaces them with the table's `0x3F4CCCCD`. Its inherited constructor value `0xC0A00000` at `+0x6C` is likewise replaced by zero. The caller does not inspect or repair byte `+5`. The full generated appearances for candidate kinds 17 and 96, whose original entries are `0x040762` and `0x0408A7`, remain outside this bounded closure.

## Random identity and limits

The supported image imports `rand` from `MSVCR110.dll` at IAT RVA `0x2FC468` and `srand` from the same DLL at IAT RVA `0x2FC4B4`. The builder, candidate selection and null-input appearance prefix use this same imported random-call identity. Import identity alone does not establish a global algorithm, thread state, uniformity, seed lifetime or equality of unrelated call streams. No direct seed call occurs inside `0x1C7730`, the scoped action-92 candidate/copy/generation segment, or the bounded kind-37 appearance path. The identified activation refresh seed call is a separate earlier boundary. [Evidence: original PE import descriptors in `binary/Cube.exe`; native call sites cited above.]

The appearance path's five-call count and the builder's stated six/eight-call counts are scoped direct counts, not a promise for the complete world update or every nested constructor/stat callback. On a nonempty candidate list the consumer adds its one selection call before choosing an alternative. Empty candidates add no such call. The builder can append duplicates and refresh existing descriptors; the consumer does not consume entries. Those facts preserve ordering and draw boundaries without assuming a replacement's data model.

Unclosed dependencies include complete descriptor generator admission/distribution and earlier draws, all indirect/alias-based configuration writers, general snapshot or persistence behavior outside the inspected body operations, full allocator/exception and destruction behavior, the action threshold helper's internal relation, source actor lifetime/concurrency, and appearances for kinds other than the bounded fresh kind 37. No server counterpart or gameplay classification is inferred. The stated original client relations remain bounded by those limits.
