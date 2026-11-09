# Alpha hit item presentation vector

This contract covers the item-effect branch of original generic hit producer RVA `0x196D30`, specifically `0x19754D–0x1976D8`, its source-item selector `0x03DF60`, and emission helper `0x19A800`. The evidence root is `disassembled alpha cube world`. Supported `binary/Cube.exe` is 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. All addresses below are RVAs. Evidence is the original image, `decompiled/client/assembly.txt`, and the individually identified original functions cited below. No reconstruction, other contract, implementation or runtime artifact was read.

This is an emission contract. It does not reconstruct the complete generic hit admission, downstream particle simulation/rendering, or the gameplay meaning of every numeric record field. Decimal floating constants below denote the original binary32 literals unless binary64 is specified.

## Reached branch and ratio

Assuming the source-present ordinary hit path has reached `0x19754D`, it converts signed 32-bit source counter `+0x70` to binary32 and divides it by helper `0x044CB0`'s signed 32-bit result converted to binary32. Call this rounded quotient Q. This is a fresh ratio calculation at this site. It has no upper clamp. An ordinary Q strictly greater than binary32 `0.25` enters the item branch; equality and smaller values skip it. The branch does not add a positive-final-hit-amount or critical requirement. Evidence: `binary/Cube.exe`, `0x19754D–0x19758A`.

The denominator helper multiplies source binary32 `+0x17C` by a selected literal and truncates the binary32 product toward zero to signed 32-bit. Category byte 3 at `+0xAA8` selects 60 for subtype byte 6 at `+0xAA9`, 80 for subtype 8, 20 for subtypes 11, 12, 15, 16 and 17, and 50 otherwise. If category is not 3, low-byte bit `0x10` at `+0x7E` selects 20, otherwise 50. Thus this integer conversion happens before the ratio division. Zero denominators, nonfinite values and out-of-range conversions are outside the ordinary relation. Evidence: `0x044CB0–0x044D28`; `decompiled/client/functions/00444cb0.c`.

The same completed binary32 Q is passed to each selected-item emission call. It is not divided by the number of selected items or qualifying subrecords, and the zero low byte of generic argument 5 does not suppress this branch. That argument supplies the variant flag inside the helper. The branch's broader predecessor gates remain a dependency rather than being replaced with a claim that every hit reaches it.

## Input vector and position have separate origins

The vector comes directly from the generic hit routine's eighth stack argument, a pointer to three binary32 components. This branch does not compute a difference of source and target positions, use an actor velocity in place of that argument, or derive a horizontal direction from the effect position. The effect position separately comes from the target's main position beginning at `+0x10`, not the source position or the target's rendered position.

For the vector calculation, the caller squares each supplied component in binary32. It adds the Y-square to the X-square contribution, in the native Y-then-X accumulation order, then adds the Z-square. Each multiplication and addition rounds separately to binary32. Only a strictly positive resulting squared length takes normalization. That completed binary32 scalar is promoted to binary64 for the imported precise square root; the result is rounded to binary32 before binary32 1 is divided by it. The supplied X and Y components are multiplied by the common rounded reciprocal. Z participates in the length but its normalized value is not emitted.

Whether normalization occurred or not, the resulting X and Y are separately multiplied by binary32 4. The prepared vector's Z is then literal binary32 5. Thus its ordinary positive-length horizontal components use the full three-dimensional length, followed by scaling by four, while vertical output is replaced. When the computed squared length is zero, normalization and square root are skipped: the original X/Y are multiplied by four and Z is still five. In particular, an exactly zero supplied vector produces `(0,0,5)`. A squared length rounded to zero follows the same gate, even if an original component was nonzero. Evidence: `0x19758A–0x197624`.

The square-root call uses client thunk `0x28D9C8`, import-address slot `0x2FC460`, named import `MSVCR110.dll!_libm_sse2_sqrt_precise`. This identifies the original operation and conversion boundary; the runtime's complete numerical error/exception contract is not audited here.

The position passed to `0x19A800` is a pointer to target `+0x10`. For each emitted record, the helper copies its three eight-byte position components unchanged. These are the original signed 64-bit fixed position components, with 65,536 units per world unit; no float conversion, normalization or position offset is introduced by this helper. The prepared vector is passed separately and is copied anew for each qualifying subrecord. Evidence: `0x19763E–0x197670`, `0x19A84B–0x19A886`; position conversion context in original helper `0x02C4A0`, `decompiled/client/functions/0042c4a0.c`.

## Which items are visited, and in what order

The caller builds a temporary list of pointers into the source creature through `0x03DF60`, then invokes `0x19A800` once per pointer in that list's order. The selector uses the source's item beginning at `+0xAA8`, called first item here, and the item beginning at `+0x990`, called second item. These names identify offsets without assigning weapon-hand semantics.

The first item having category byte 3 and subtype 6 or 7 selects only that item, before consulting the second item's corresponding override. Otherwise, the second item having category 3 and subtype 6 or 7 selects only the second item. Either override finishes selection without applying the action table below.

Without an override, the unsigned source action byte at `+0x68` controls the list:

| Source action | Selected sequence |
|---|---|
| 2, 4, 7, 8, 10, 18, 39, 41 | Second item only, if its category is 3. |
| 40, 42 | First item only, if its category is 3. |
| 11, 12, 16, 17, 20, 54, 86, 91, 96 | Second item if category 3, then first item if category 3. Each admission is independent. |
| Every other action byte | First item if category 3; then second item only if its category is 3 and subtype is 12. |

The list therefore contains zero, one or two pointers. It does not deep-copy or merge item values, and matching contents at the two offsets do not cause deduplication. Selection occurs before the per-item calls. Evidence: original selector `0x03DF60–0x03E11D`, its byte/destination tables at `0x03E120/0x03E130`; `decompiled/client/functions/0043df60.c`; caller `0x197624–0x197681`.

## Per-item qualification and one-record emission

Inside `0x19A800`, item field `+0x114` is read as a signed 32-bit subrecord count. A non-positive count emits nothing. For a positive count, index zero is considered first and the item byte at `+0x17 + 8 times index` is read as unsigned. Values at least 128 qualify; values below 128 emit nothing. The count is reread for the signed loop comparison after each index increment, including skipped entries. There is no separate capacity validation of the item storage in this helper. No adjacent numeric subrecord-strength field is read to determine this branch's output.

Every qualifying byte produces one output record. The helper completes that record and its append before advancing the subrecord index. All qualifying records for one selected item are processed before the next selected item. Equal bytes and identical resulting records remain separate appends. There is no averaging across entries and no division by qualifying-entry count. The emitted integer field described below is independent of how many records are appended. Evidence: `0x19A829–0x19A845`, `0x19A94F–0x19A98B`; `decompiled/client/functions/0059a800.c`.

The output aggregate's sublist head/count are at `+0x10/+0x14`. Each successful emission tail-appends after the existing sequence and increments the 32-bit list count. It does not clear the aggregate, replace an earlier record or invoke a hit-application callback. The ordinary list-length guard rejects an already maximal count `0x03333332` through the native length-error path. Allocation and exception recovery are not closed by this contract.

The allocator reserves 80 bytes per node, including two 32-bit links. Copy helper `0x065E60` copies exactly 68 payload bytes, at offsets `+0x00` through `+0x43`; the remaining four bytes of the 72-byte payload extent have no established semantic copy. The following offsets are payload-relative, excluding list links:

| Payload offset | Width and produced contents |
|---|---|
| `+0x00`, `+0x08`, `+0x10` | Three signed 64-bit position components, copied from target main position. |
| `+0x18`, `+0x1C`, `+0x20` | Binary32 effect vector, with the flag variant applied as below. |
| `+0x24`, `+0x28`, `+0x2C`, `+0x30` | Four binary32 color components from `0x0C7250`. |
| `+0x34` | Binary32 literal `0.1`, encoding `0x3DCCCCCD`. |
| `+0x38` | Signed 32-bit value obtained by truncating the binary32 result of Q multiplied by 3 and then increased by 1, with the flag-dependent integer increment below. |
| `+0x3C` | 32-bit effect/material selector value 2. The qualifying input byte is not copied into this field. |
| `+0x40` | Binary32 literal 3, encoding `0x40400000`. |

Evidence: `0x19A84B–0x19A97C`; `decompiled/client/functions/004520d0.c`, `00465e60.c`, `0046d810.c`; native copy `0x065E60–0x065ECD`.

The producer closes these scalar values and their dependence on Q. It does not establish whether downstream code interprets `+0x34` as size, strength or another scale, or whether `+0x40` is a lifetime and in what units. Those time/strength consumer meanings remain explicit dependencies. Likewise the integer at `+0x38` is a deterministic produced field, not proof here of the number of subsequently simulated particles. It does not control how many list nodes this helper appends.

## Color-strength arithmetic and material-byte cases

Each emission supplies an all-ones binary32 base color to color helper `0x0C7250`, with the qualifying byte zero-extended to 32 bits. Its strength argument is Q multiplied by binary32 `0.5`, rounded before the call. Call that rounded strength P. For input bytes 128 through 131, the color helper multiplies P by `0.5` in binary32, then adds 1 to form a shared scalar S. Its fourth component is obtained by adding P to base fourth component 1, then multiplying that rounded sum by S. The resulting RGB is:

| Qualifying unsigned byte | RGB produced with S |
|---|---|
| 128 | S; S multiplied by `0.5`; S multiplied by `0.1`. |
| 129 | S multiplied by `0.3`; S; S multiplied by `0.5`. |
| 130 | S multiplied by `0.3`; S multiplied by `0.5`; S. |
| 131 | S multiplied by `0.8`; S multiplied by `0.8`; S. |
| 132 through 255 | `(0.5,0.5,0.5)`, with fourth component 1, through the default branch; P does not scale this result. |

All listed multiplies/additions and returned components are binary32. The two consecutive half-strength operations, and the addition before the fourth-component multiply, are the original conversion/arithmetic boundaries; they are not replaced here by a real-number simplification. The helper copies all four returned components into the payload. It applies no upper color clamp or color normalization, so values above 1 remain possible. The material-byte cases are numeric identities only; no unverified element name is assigned. Evidence: original `0x0C7250–0x0C75A9`, especially `0x0C7415–0x0C7551` and default `0x0C7552–0x0C75A9`; `decompiled/client/functions/004c7250.c`.

## Nonzero flag variant and fixed ordering

The helper reads only the low byte of its flag argument, forwarded from generic hit argument 5. Zero retains the prepared vector and the truncated integer at payload `+0x38`. Nonzero multiplies each of the three copied vector components by binary32 2, and adds integer 10 to the already truncated signed integer field. The integer addition happens after conversion; it is not added to the floating expression before truncation. The flag does not modify the position, color-strength argument, RGBA result, selector 2, scalar `0.1` or scalar 3.

For an exactly zero original vector and admitted Q, the ordinary flag-zero record therefore has vector `(0,0,5)`; the nonzero variant has `(0,0,10)`. Each qualifying subrecord starts by copying the caller's prepared vector again. Doubling affects only that temporary output record; it does not accumulate across subrecords or selected items. For ordinary in-range Q equal to 1, the integer field is 4 in the zero variant and 14 in the nonzero variant, regardless of how many qualifying bytes exist.

Within each emission, the position/vector copy and deterministic integer calculation precede the color-helper call. The color is completed before the flag variant, and the variant is complete before the payload is appended. The caller frees only its temporary selected-pointer list after all item calls; emitted records remain in the output aggregate. No random-number call occurs in this caller region, selector, emission helper or reached color helper. The source-counter ratio, subrecord byte, color strength, integer field and list-node count are deterministic inputs/results here. Random effects elsewhere in the generic hit routine are outside this assertion. Evidence: `0x197624–0x1976D8`, `0x19A84B–0x19A98B`; literal 2 at RVA `0x345E10` and literal 4 at `0x345E60`.

## Limits

The ordinary relations assume valid, unchanged source/target/item storage during evaluation, finite floating operands, in-range signed conversions and valid output containers. This audit does not close earlier hit acceptance, numeric counter provenance, source/target state production, concurrent count changes, malformed subrecord extents, allocation failure, integer overflow, nondefault floating controls, all square-root edge cases, or downstream consumption of the effect/material selector and remaining scalar fields. It establishes the argument-vector and target-position origins and complete produced payload values without reconstructing a broad hit resolver or particle engine.

Status: FINAL / STABLE / HELD after complete author readback. All prior held artifacts remain unopened and unchanged.
