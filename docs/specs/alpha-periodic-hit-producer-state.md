# Alpha periodic hit producer state

This contract is limited to the original client's periodic call at RVA `0x218310–0x2183DF`, generic hit producer `0x196D30`, and resident-buff mutations reached through its permitted local callback `0x195A60`. The evidence root is `disassembled alpha cube world`. Supported image `binary/Cube.exe` is 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred base `0x00400000`. All addresses below are RVAs. Evidence consists of the original image, `decompiled/client/assembly.txt` and the individually identified original function files cited below. No reconstruction, other contract, implementation or runtime artifact was used.

The findings distinguish admitting a periodic call, reaching an ordinary producer calculation, emitting a record, and applying that record locally. None of those implies the others. The contract stops before unaudited fatal application effects and does not establish a server counterpart.

## Periodic admission and exact arguments

The enclosing resident list begins at creature `+0x1178`. A payload reached by the iterator has an unsigned type byte at `+0x00`, binary32 amount at `+0x04`, signed 32-bit remaining duration at `+0x08`, and an eight-byte source identifier at `+0x10`. The current duration is reduced by the update's integer duration before type-specific periodic handling. A new duration less than or equal to zero is placed in the update's deferred-removal collection and does not take the periodic path. Equality with zero therefore expires before this hit opportunity. Evidence: `binary/Cube.exe`, `0x2120E2–0x212163`.

For a surviving type-4 payload, the periodic route requires world byte `+0xB4` equal to zero. It compares the signed quotients by 400 of the new remaining duration and the reconstructed old duration, the latter obtained by adding the update duration back with 32-bit integer arithmetic. Division truncates toward zero. Different quotients admit one call; equal quotients admit none. The test does not emit once per crossed boundary. A duration that expires during this step emits none even if one or more 400-unit boundaries were crossed. The units of the supplied update duration, its wider admission and integer overflow states are not independently closed here. Evidence: `binary/Cube.exe`, `0x218310–0x218362`.

The payload's source identifier is looked up through world helper `0x02F000`. Failure produces a null source pointer and does not reject the periodic call. A lookup resolving to the target itself also has no separate identity rejection here. The routine passes the payload's binary32 amount directly, without multiplying it by update duration or the number of crossed boundaries. The caller does not test the producer's returned byte before advancing its iterator. Evidence: `0x218362–0x2183DF`; `decompiled/client/functions/0042f000.c`.

The following positions count 32-bit stack arguments from one, after the world object supplied to the native member call. Pointer widths here are 32 bits.

| Position | Periodic value and observed use |
|---|---|
| 1 | Current creature pointer: the target. |
| 2 | Resolved source pointer, possibly null. |
| 3 | Raw binary32 amount from current resident payload `+0x04`. |
| 4 | Full word zero; its critical byte is zero. |
| 5 | Full word zero; its low-byte auxiliary/item flag is zero. |
| 6 | Binary32 positive zero, the initial status-probability scalar. |
| 7 | Pointer to target `+0x1320`. No read of this argument is established in the inspected producer body. |
| 8 | Pointer to a newly formed binary32 vector `(0,0,0)`. |
| 9 | The update's shared output aggregate pointer. |
| 10 | A separate update-local pointer forwarded to local application; its wider object meaning is unresolved. |
| 11 | Full word 1; its low byte selects target helper `0x0467A0` in the ordinary subtraction path. |
| 12 | Full 32-bit discriminator zero. This is not inferred from just a low byte. |
| 13 | Full word zero. No semantic read of this argument is established in the inspected producer body. |
| 14 | Full word zero; its low byte disables the producer's optional shield interception and later ordinary impact-presentation block. |

Evidence: native call preparation `0x218371–0x2183DA`, interpreted against original callee argument reads in `0x196D30–0x1986D7`; original file `decompiled/client/functions/00596d30.c`. The single true flag at position 11 is material: this is not an all-zero call.

## Generic admission and the three direct writes to target +0x1184

The producer first rejects an ordinarily non-positive target health at binary32 `+0x16C`. It also rejects when world byte `+0xB4` is nonzero and the target pointer equals world pointer `+0xB8`. The periodic caller already requires that byte to be zero, so its call cannot trigger that second rejection under unchanged world state. Neither target `+0x1184` nor a positive requested amount is an initial admission test. Evidence: `binary/Cube.exe`, `0x196D73–0x196DB6`.

There are three direct stores to target `+0x1184` in the original producer. They store literal binary32 encodings, not an amount-weighted value or an update-duration reduction:

| Store RVA | Exact reached route | Stored value |
|---|---|---|
| `0x197E48` | Source pointer is null after initial producer admission. One random call occurs immediately before this store; its result is not used to compute the stored value. | `0x3F000000`, exactly `0.5`. This replaces the prior value even if it was greater. |
| `0x197ADF` | Source is nonnull, the ordinary late stage has been reached, and the source action byte at `+0x68` is 28. The old target binary32 value is strictly less than literal `0x3E99999A`. | `0x3E99999A`, the binary32 literal approximately `0.30000001192092896`. Equality and larger ordinary values produce no store. |
| `0x197AF3` | Source is nonnull, the same late stage has been reached, its action byte is not 28, and argument 14's low byte is nonzero. | `0x3F800000`, exactly 1, without comparing the old value. |

The action-28 comparison uses the old binary32 field directly against the encoded binary32 threshold. The 1 store is the alternative to the action-28 branch; a nonzero argument 14 does not make an action-28 source write 1. These are all direct references that store this target field inside `0x196D30`; this statement does not inventory writers in other routines or indirect callback effects. Evidence: `decompiled/client/assembly.txt` for function `00596d30`, native regions `0x197AC1–0x197B05`, `0x197E3D–0x197E5E`.

Consequently an admitted periodic call with a null source writes `0.5`. A surviving source-present periodic route can raise this field to the encoded `0.3` for action 28 despite argument 14 being zero. Other source actions do not directly write it for this periodic call. No store here tests the final emitted amount for positivity.

The two source-present stores occur after the target's type-6 absorption stage, the selected target-stat subtraction and the reached item/auxiliary stages, but before final resident type-1 amount multipliers and the final status-success test. A type-6 absorption early return therefore prevents either late store. A zero amount produced by the selected stat subtraction does not itself prevent them. The null-source store bypasses these source-present stages and proceeds toward the common primary-record stage. Evidence: `0x197235–0x197AC1`, `0x197AFD–0x197E5E`, `0x198023–0x19804D`.

## Source-present gates that still affect a periodic call

The source pointer's presence does not imply source-health, source/target-identity or relationship admission. There is no such initial test in the producer. The following original gates and calculations remain relevant with the periodic parameters.

Source actions 91 and 74 require low-byte bit `0x01` of target `+0x5C`. Its absence returns before the ordinary calculations or any `+0x1184` write. Source action 28 first produces a separate presentation effect with a random draw. This effect precedes the target-specific interception below and can occur even when that interception then returns.

A target with byte `+0x60` equal to 1 and either low-byte bit `0x80` at `+0x124` or bit `0x20` at `+0x5C` takes the early category-3 record route. It appends that record, makes the permitted local callback when world `+0xB4` is zero, and returns. The early presentation in this category has its own random draw. It does not reach a direct `+0x1184` store. The optional category-1 shield route additionally requires argument 14's low byte nonzero, so this separate route is disabled for the periodic call. Evidence: `0x196DBC–0x197235`.

Otherwise the producer sums binary32 amounts of every resident type-6 record at the target, in list order. It does not filter that sum by their remaining duration. It forms a source counter ratio from signed 32-bit source `+0x70` and the signed integer result of `0x044CB0`, converting each to binary32 before division. A first quotient greater than 1 is replaced by 1. At less than or equal to 1, including equality, the source counter and helper result are recomputed rather than reusing the first quotient. The binary32 difference of 1 and the selected ratio is multiplied by the completed type-6 sum.

Only a positive ordinary product takes the absorption route. It produces a category-5 record whose amount is the original requested amount, not the product or a per-record allocation of that product. The record append precedes the category-5 presentation's random draw, which precedes its permitted local callback. That callback completes before the producer subtracts the previously computed product from the original request. A non-positive remainder returns at this point. A positive remainder continues. With a non-positive product, the request continues unchanged and no category-5 record is produced. All sums, ratio arithmetic, subtraction and comparisons here use binary32 operands/results. Evidence: `0x197235–0x19750B`, especially callback `0x1974DD` and subsequent subtraction.

The continued route calls target helper `0x0467A0` because periodic argument 11 is true. Its floating return is stored to binary32. The producer multiplies that rounded return by the previously computed difference of 1 and the selected source ratio, subtracts the product from the running amount in binary32, and replaces an ordinary negative result with zero. The internal stat/item dependencies of `0x0467A0` are a remaining named dependency; they are not replaced here with a fixed constant. The alternative helper `0x03CFF0` is not selected by these periodic arguments. Evidence: `0x19750B–0x19754D`; `decompiled/client/functions/004467a0.c`.

A new source counter ratio is then calculated, without the preceding upper clamp. If it is greater than binary32 `0.25`, the producer can still gather source items through `0x03DF60` and invoke `0x19A800`. The periodic input vector has zero length, so this branch does not call the vector-normalization square root: its prepared item-effect vector has X/Y zero and Z 5. The zero argument-5 flag selects the ordinary variant of that helper; it does not suppress those calls or their qualifying item-effect records. Selection by `0x03DF60` remains a named dependency here. The inspected `0x19A800` body contains no random draw and writes effect output rather than the target's resident list. Evidence: `0x19754D–0x1976D8`; `decompiled/client/functions/0059a800.c`.

Argument 5's zero low byte does skip the later auxiliary amount/source-self stage beginning at `0x1976D8`. Argument 14's zero low byte skips the later ordinary impact-presentation stage. Neither skips the action-28 minimum store already described. The full zero discriminator in argument 12 also skips the general producer's discriminator-17 resident type-4 append and its associated buff event. Critical byte zero skips the critical presentation route. These zero values do not suppress the final type-1 multipliers or the status-probability calculation described next. Evidence: `0x1976D8–0x197B05`, `0x19804D–0x1981CB`, `0x1981CB–0x1983F8`.

At the common primary-record stage, every resident type-1 amount multiplies the running binary32 amount, sequentially in list order. No remaining-duration filter or single-match rule is applied. This can include nodes already marked for deferred removal by the enclosing scan. Null-source calls also reach these multipliers. Evidence: `0x198023–0x19804D`.

## Zero initial probability does not close the status route

For a nonnull source, the later status calculation is admitted when signed target timer `+0x12C` is strictly less than -3000 and target byte `+0x60` is not 6. Equality at -3000 does not enter. There is no positive-amount requirement for this stage. Its initial scalar is periodic argument 6, zero, but the original route can replace that scalar:

- Source actions 58, 12, 68, 93 and 69 set it to 1 before the kind-dependent multipliers.
- Target helper `0x0445D0` being true multiplies it by binary32 `0.15`. Source helper `0x0445D0` being false, with source byte `+0x60` nonzero, subsequently multiplies it by `0.1`.
- Source actions 91 or 74 then set it to 1 regardless of those earlier multipliers. Target action 84 then also sets it to 1.

Helper `0x0445D0` tests 32-bit kind `+0x64` against the set `{0x65,0x6B,0x6C,0x6D,0x72,0x73,0x74,0x75,0x76,0x77}`. Its target call precedes its source call. Evidence: `0x1983F8–0x1984A3`; `decompiled/client/functions/004445d0.c`.

The producer next calls numeric helper `0x03CA60` on source signed 32-bit `+0x190` converted to binary32, then on the target's corresponding converted value. For either input, that helper subtracts 1, multiplies by binary32 `0.05`, adds 1, divides binary32 1 by the result, and subtracts the quotient from 1; the return is stored as binary32. The producer subtracts target result from source result, multiplies by binary32 10, promotes that rounded exponent to binary64 and calls the original precise power with base 2. Its result is rounded to binary32 before multiplying the current probability scalar. An ordinary result above 1 is capped at 1; no lower clamp is added. The result is then multiplied by binary32 `0.9` (`0x3F666666`). Any resident type-1 node replaces this completed threshold with zero, again without checking its duration.

One random draw still occurs whenever this admitted stage is reached, even when the final threshold is zero. The nonnegative native integer result is converted to binary32 and divided by binary32 32,767. Success requires that sample strictly below the threshold; equality fails. Success chooses the record's first timer through target helper `0x044FF0`, changes the record vector's Z to 10, and appends a status presentation. Thus the originally zero vector and zero probability do not guarantee a zero output timer/vector for the periodic record. A source action 28 also marks the final record's separate byte at `+0x46`, independently of probability success. The full internals of `0x044FF0` and numerical behavior outside ordinary finite values remain dependencies. Evidence: `0x1984A3–0x198677`; `decompiled/client/functions/0043ca60.c`, `00596d30.c`.

## Local application and resident-list mutation before iterator return

Category-3, category-5 and final primary records are appended before their respective local callbacks. The producer calls `0x195A60` when world byte `+0xB4` is zero, as it is at the periodic caller. This callback resolves the record's target identifier through the same world's creature container and requires a nonnull ordinarily positive-health target. If world pointer `+0xB8` is null and target byte `+0x60` is zero, it instead appends the record to an output queue and returns without local category processing. These are pointer/byte gates; no broader standalone/client/server identity is inferred from them. Evidence: `0x196FEE–0x197014`, `0x1974BC–0x1974E2`, `0x198687–0x1986D5`; callback `0x195AAB–0x195B1D`.

The callback can also skip category processing when target word `+0x7E` has bit `0x2000`. For this bit-set target, it requires some positive binary32 entry in target container `+0x13A4` whose eight-byte key resolves in the world to a nonnull creature with byte `+0x60` equal to zero. It does not require that qualifying entry to be the actual hit source. Absence returns without reaching the resident mutation below. The callback's earlier nonresident accounting can precede this gate; it is not a transactional rollback of all callback effects. Evidence: `0x195CE7–0x195D70`.

For the periodic category-5 absorption record, local type-6 mutation additionally requires a strictly positive record amount and admission to the ordinary application path. With world `+0xB4` zero, its later local-mode gate passes. The callback walks all resident nodes, not just the first match and not just records attributed to this source. For every type byte 6, it subtracts the full category-5 record amount from that node's binary32 amount, stores the binary32 result, and tests that result. A non-positive ordinary result is replaced with binary32 positive zero, and the same node's signed 32-bit remaining duration is replaced with integer zero. A positive result leaves duration unchanged. Equality with zero therefore clears both fields.

This is the original requested amount from the category-5 record, applied separately to every matching node. It is not divided among nodes and is not replaced by the producer's effective type-6 product. A zero or negative category-5 record amount does not take this resident-decrement branch. No type-6 node is unlinked here, no resident count changes, and the node's type, source identifier and other payload bytes are not replaced.

After updating each matching node, the callback appends a 40-byte buff event to output sublist `+0x58`. Its leading two eight-byte identifiers come from the hit record's source and target identifiers. Its remaining 24 bytes copy the updated resident payload, including the resident's existing source identifier. Consequently the outer event source identifies the hit source while the embedded resident source remains the original stored value. One event is appended for each type-6 node, in traversal order, including a node whose amount and duration became zero. Evidence: native `0x195EED–0x195FC7`; append helper `0x0460A0`; `decompiled/client/functions/00595a60.c`, `004460a0.c`.

In the reached nonfatal category-3 and primary category-0 paths, the inspected callback performs no resident-list append, overwrite or erase. Category 3 exits category processing directly; category 0 has separate health/status behavior outside this resident-mutation inventory. The callback's separate category-4 helper is not selected by these periodic-produced categories, and the producer's own discriminator-17 append is disabled by the exact zero at argument position 12. Helper `0x1A0970` reached by nonnegative-record accounting operates on its accounting/relationship containers, not `+0x1178`, in the inspected body. This establishes the resident-list subset rather than claiming the callback has no other mutations. Evidence: `0x195D70–0x195F05`, `0x195FCC–0x196133`; `decompiled/client/functions/005a0970.c`, `0043cf20.c`.

After an ordinary callback return, the producer continues or returns as specified above; the periodic caller then increments its existing resident iterator and compares against the current list end. It does not restart the resident scan. The callback's zero-duration type-6 nodes remain linked at that point. An unvisited one can therefore be encountered later in the same scan, receive the normal duration reduction and enter deferred removal. One already visited is not revisited solely because this callback zeroed its duration. Actual erasure from the deferred collection happens only after the scan. These observations assume the nonfatal return leaves the target and iterator storage valid. Evidence: `0x2183DF–0x21843C`, deferred erasure beginning `0x218470`; `0x212143–0x212163`; `decompiled/client/functions/004860b0.c`.

## Explicit limits

The three `+0x1184` stores are closed as direct producer writes, with their exact literals, ordering and ordinary comparisons. Broader field semantics and writers elsewhere are not assigned. The periodic call is not reduced here to a direct health subtraction: its source lookup, early categories, type-6 callbacks, target-stat call, type-1 multipliers, possible item effects and status draw remain material.

This contract excludes the callback's fatal branch reached when its later health processing leaves the target non-positive. It does not establish death-related resident mutation, object/container lifetime, iterator validity, rewards, drops or world changes on that branch. It also leaves wider creature selection, source-state producers, output delivery, full stat/item-collection/status-timer helper internals, concurrent mutation, allocation failure, malformed lists, integer overflow, exceptional floating values and nondefault floating controls unresolved. No implementation or runtime findings were requested or used to fill those gaps.

Status: FINAL / STABLE / HELD after complete author readback. Prior held artifacts remain unopened and unchanged.
