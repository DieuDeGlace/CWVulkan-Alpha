# Alpha created-candidate appearances: kinds 17 and 96

Status: FINAL / STABLE / HELD after complete author readback.

This independent original-source-only contract closes the ordinary null-optional-input paths for full 32-bit kinds 17 (`0x11`) and 96 (`0x60`) in client appearance helper RVA `0x03F7C0`. It covers fresh appearance initialization, every byte of the appearance extent through `+0xAB`, ordered writes, reached helper calls, imported random/seed boundaries, and the immediate action-92 caller's subsequent flag-word update. Numeric fields are described without assigning asset, body-part or gameplay names.

The supported image is `binary/Cube.exe`, image base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Evidence paths below are relative to `disassembled alpha cube world`. The original image's jump-table entries independently confirm kind 17 at RVA `0x040762` and kind 96 at RVA `0x0408A7`. Instruction bytes for the scoped appearance constructor, null-input prefix, both case bodies, reached triple-copy helper and caller fragment were compared with the original image. No earlier contract, reconstruction, replacement implementation or runtime observation is evidence for this document.

## Input and construction boundary

The reached appearance helper receives, in argument order, an address of a four-byte kind, an address of the appearance record, and a null optional-input pointer. The ordinary caller uses actor `+0x64` for the kind and actor `+0x78` for the appearance; those memory intervals are distinct. The helper's dispatch reads the complete 32-bit kind. Values whose low byte happens to be `0x11` or `0x60` but whose other bytes differ are not these two cases. The null-input path does not rewrite the kind before dispatch, and neither scoped case changes it afterward. Thus the resulting kind remains full 32-bit 17 or 96. Nonnull optional input follows a different prefix and is outside this closure. [Evidence: `decompiled/client/functions/0043f7c0.c`; native RVAs `0x03F7D0–0x03F962`; original table at image RVA `0x043CA0`.]

Fresh actor constructor `0x03B690` invokes body initializer `0x03C100` with actor `+0x10`; that initializer calls appearance constructor `0x028750` at body `+0x68`, which is actor `+0x78`. The appearance extent is `0xAC` bytes. The concrete fresh-state results below depend on that appearance constructor having completed and on no intervening appearance replacement. The subsequent generation caller writes the new actor's discriminator at actor `+0x60` and its kind at `+0x64`; those are outside the appearance extent. [Evidence: native RVAs `0x03B6CB–0x03B6D1`, `0x03C180–0x03C1B5`, `0x215938–0x2159B4`; original files `0043b690.c`, `0043c100.c`, `00428750.c`.]

The appearance constructor first stores 32-bit `0xFFFF0000` at `+0`, then byte `0xFF` at `+4`, then a zero 16-bit word at `+6`. It does not write byte `+5`. Next come the 32-bit values at `+8`, `+0x0C`, `+0x10`, `+0x24`, `+0x28`, `+0x2C`, `+0x30`, `+0x34`, `+0x38`, `+0x3C`, `+0x40`, `+0x44`, then `+0x48`, in that order. The eight 16-bit fields `+0x14` through `+0x22` are then set to `0xFFFF` in ascending offset order. Finally all successive 32-bit fields from `+0x4C` through `+0xA8` are written in ascending offset order. Their exact values appear in the construction column of the full-extent table below. There is no blanket clearing of the complete `0xAC` bytes, allocation, nested helper call, `rand` or `srand` call in `0x028750`. [Evidence: native RVAs `0x028750–0x0288A9`.]

## Shared prefix and random-call order

Before either kind-specific appearance write, the null-input prefix makes exactly five calls to the same imported `rand` entry. Calls one, two and three are reduced to temporary byte values; in the assembled temporary three-byte value the third result supplies the first byte, the second supplies the middle byte, and the first supplies the last byte. Calls four and five are reduced by signed remainder with divisor 100 and stored in two temporary 32-bit fields, in that order. The intervening remainder sign handling does not add random calls. Neither kind 17 nor kind 96 reads those generated temporary fields into the appearance. Their output values therefore do not depend on any of the five returned random values, although all five calls still occur.

Only after these five calls does the helper perform its common appearance writes: bits `0x3F75C290` at `+8`, the same bits at `+0x0C`, and bits `0x400A3D71` at `+0x10`, in that order. All three are stored as complete 32-bit words representing binary32 values. These writes precede the full-width kind dispatch, and both scoped cases retain them. No multiplication, normalization, trigonometric call or float-to-integer conversion is used to produce them. [Evidence: native RVAs `0x03F807–0x03F962`; original file `0043f7c0.c`.]

The original PE import table identifies `rand` from `MSVCR110.dll` at IAT RVA `0x2FC468` and `srand` from that same DLL at IAT RVA `0x2FC4B4`. This identifies the API calls, not a complete runtime random algorithm, seed lifetime, thread-state model or output distribution. Neither the shared prefix nor either scoped case calls `srand`. Runtime DLL internals are not expanded into an additional inferred draw algorithm. [Evidence: `binary/Cube.exe`, PE import descriptors and original call sites `0x03F80B`, `0x03F824`, `0x03F838`, `0x03F85C`, `0x03F869`.]

## Kind-specific changes and reached helpers

Kind 17 begins with a 32-bit store of `0x01490148` at appearance `+0x14`, then a 32-bit store of `0x014B014C` at `+0x1A`. In the original little-endian image these establish the word pairs `0x0148/0x0149` and `0x014C/0x014B`, respectively. A 16-bit store of `0x014A` at `+0x18` follows. Binary32 bits `0x3F4CCCCD` are then stored at `+0x24`, before the first triple-copy call.

Its four calls to helper `0x011E30` occur in the following destination order. The first transfers `(positive zero, -90, positive zero)` into `+0x4C/+0x50/+0x54`; the second transfers `(positive zero, positive zero, -2)` into `+0x64/+0x68/+0x6C`; the third transfers `(positive zero, 4, 7)` into `+0x70/+0x74/+0x78`; the fourth transfers `(5, 3, positive zero)` into `+0x7C/+0x80/+0x84`. The case then ORs mask `0x0428` into the 16-bit word at `+6`, and returns through the ordinary cookie check. On fresh state that word is `0x0428` at helper return. [Evidence: native RVAs `0x040762–0x04081D`.]

Kind 96 first stores 16-bit `0x00E3` at `+0x14`, then 16-bit `0xFFFF` at `+0x16`. It next stores 32-bit `0x00E200E4` at `+0x1A`, establishing words `0x00E4/0x00E2`, then stores 16-bit `0x00E5` at `+0x18`. The first triple-copy call transfers `(6, 3, -2)` into `+0x7C/+0x80/+0x84`. The shared tail reached by this case then transfers `(positive zero, -90, positive zero)` into `+0x4C/+0x50/+0x54`. Only after both calls does it OR mask `0x0028` into the 16-bit word at `+6`, and return through the ordinary cookie check. On fresh state that word is `0x0028`. This case does not write `+0x24`, `+0x64/+0x68/+0x6C`, or `+0x70/+0x74/+0x78`; their construction values survive. [Evidence: native RVAs `0x0408A7–0x04091E`.]

Every stated triple is first formed from literal 32-bit values in temporary storage. Helper `0x011E30` copies the first component as one 32-bit word, then the second, then the third. It has no nested calls, allocation, random/seed operation or arithmetic. The counts are therefore four such copy calls for kind 17 and two for kind 96. The numeric bits used here are positive zero `0x00000000`, -90 `0xC2B40000`, -2 `0xC0000000`, 3 `0x40400000`, 4 `0x40800000`, 5 `0x40A00000`, 6 `0x40C00000`, and 7 `0x40E00000`. Their copying does not introduce rounding or depend on an actor position, update amount, existing scalar magnitude or random result. [Evidence: native RVAs `0x011E30–0x011E49`; original file `00411e30.c`; the two case ranges cited above.]

The only additional reached routine in each case's ordinary return is cookie check `0x28D788`. On a matching cookie it compares the supplied value with the cookie global and returns without another call. That successful path adds no `rand` or `srand`. Its failure transfer at `0x28D792`, corrupted stack/storage, asynchronous interference and a changed cookie are outside the ordinary result contract. [Evidence: native RVAs `0x28D788–0x28D792`.]

## Complete fresh appearance extent at helper return

All offsets in this table are appearance-relative, beginning at actor `+0x78` for the scoped actor caller. A list of offsets in one row means that each listed offset has the stated width and value. All fields from `+8` onward described as binary32 are given by exact bits; approximate decimal spellings are unnecessary. The constructor's 16-bit index-like fields are kept as raw words without assigning signed asset semantics.

| Appearance offset(s) | Width | After appearance construction | Kind 17 helper return | Kind 96 helper return |
| --- | --- | --- | --- | --- |
| `+0`, `+1` | Byte each | `0x00` | `0x00` | `0x00` |
| `+2`, `+3`, `+4` | Byte each | `0xFF` | `0xFF` | `0xFF` |
| `+5` | Byte | Unspecified | Unchanged, unspecified | Unchanged, unspecified |
| `+6` | 16-bit word | `0x0000` | `0x0428` | `0x0028` |
| `+8`, `+0x0C` | Binary32 each | `0x3F800000` | `0x3F75C290` | `0x3F75C290` |
| `+0x10` | Binary32 | `0x3F800000` | `0x400A3D71` | `0x400A3D71` |
| `+0x14` | 16-bit word | `0xFFFF` | `0x0148` | `0x00E3` |
| `+0x16` | 16-bit word | `0xFFFF` | `0x0149` | `0xFFFF` |
| `+0x18` | 16-bit word | `0xFFFF` | `0x014A` | `0x00E5` |
| `+0x1A` | 16-bit word | `0xFFFF` | `0x014C` | `0x00E4` |
| `+0x1C` | 16-bit word | `0xFFFF` | `0x014B` | `0x00E2` |
| `+0x1E`, `+0x20`, `+0x22` | 16-bit word each | `0xFFFF` | `0xFFFF` | `0xFFFF` |
| `+0x24` | Binary32 | `0x3F8147AE` | `0x3F4CCCCD` | `0x3F8147AE` |
| `+0x28`, `+0x2C`, `+0x34`, `+0x40`, `+0x44` | Binary32 each | `0x3F800000` | `0x3F800000` | `0x3F800000` |
| `+0x30` | Binary32 | `0x3F7AE148` | `0x3F7AE148` | `0x3F7AE148` |
| `+0x38` | Binary32 | `0x3F733333` | `0x3F733333` | `0x3F733333` |
| `+0x3C` | Binary32 | `0x3F4CCCCD` | `0x3F4CCCCD` | `0x3F4CCCCD` |
| `+0x48`, `+0x4C` | 32-bit each | `0x00000000` | `0x00000000` | `0x00000000` |
| `+0x50` | Binary32 | `0x00000000` | `0xC2B40000` | `0xC2B40000` |
| `+0x54`, `+0x58`, `+0x5C`, `+0x60`, `+0x64`, `+0x68` | 32-bit each | `0x00000000` | `0x00000000` | `0x00000000` |
| `+0x6C` | Binary32 | `0xC0A00000` | `0xC0000000` | `0xC0A00000` |
| `+0x70` | 32-bit | `0x00000000` | `0x00000000` | `0x00000000` |
| `+0x74` | Binary32 | `0x3F000000` | `0x40800000` | `0x3F000000` |
| `+0x78` | Binary32 | `0x40A00000` | `0x40E00000` | `0x40A00000` |
| `+0x7C` | Binary32 | `0x40C00000` | `0x40A00000` | `0x40C00000` |
| `+0x80` | Binary32 | `0x00000000` | `0x40400000` | `0x40400000` |
| `+0x84` | Binary32 | `0x00000000` | `0x00000000` | `0xC0000000` |
| `+0x88` | Binary32 | `0x40400000` | `0x40400000` | `0x40400000` |
| `+0x8C` | Binary32 | `0x3F800000` | `0x3F800000` | `0x3F800000` |
| `+0x90` | Binary32 | `0xC1280000` | `0xC1280000` | `0xC1280000` |
| `+0x94` | 32-bit | `0x00000000` | `0x00000000` | `0x00000000` |
| `+0x98` | Binary32 | `0xC1000000` | `0xC1000000` | `0xC1000000` |
| `+0x9C` | Binary32 | `0x40000000` | `0x40000000` | `0x40000000` |
| `+0xA0`, `+0xA4`, `+0xA8` | 32-bit each | `0x00000000` | `0x00000000` | `0x00000000` |

The table covers all 172 bytes, including the explicit unknown byte. Byte `+5` has no verified default in these paths; zero is not supplied from an allocator assumption. No other byte within this extent is left unspecified by the inspected fresh constructor plus scoped case. Neither case assigns the temporary random byte values to bytes `+2/+3/+4`; those bytes retain `0xFF`. The absence of changes to a field means constructor inheritance, not another hidden generation write. [Evidence: native appearance constructor and complete case bodies cited above.]

## Immediate actor caller and call counts

In the action-92 creation fragment, a nonempty valid collection at source actor `+0x13F8` causes one direct imported `rand` call before the creation-alternative test. The result is reduced by unsigned remainder over the four-byte collection size, and the selected complete four-byte entry is read. Full 32-bit source member `+0x13F4` equal to 1 or 2 takes a copy alternative instead; every other full-width value reaches generation. On the generation alternative the selected four-byte kind is stored at the new actor's `+0x64`, then the helper is called with that kind address, the fresh appearance address and full-width null optional input. Empty candidates use kind 37 and therefore do not enter either scoped case. One candidate entry still spends the selection call. This document does not reconstruct the earlier action, map-scan or actor-creation admission. [Evidence: native RVAs `0x215938–0x2159BC`; original image/listing for client world routine `0x20C510`.]

For either selected kind 17 or 96 on that admitted generation path, the call sequence from candidate selection through appearance-helper return contains six calls to imported `rand`: one candidate-selection call, followed by the five prefix calls. The four or two nested triple-copy calls occur only afterward and add no random calls. There is no `srand` call in that segment. The standalone appearance-helper count is exactly five `rand` calls, with zero nested additional `rand` or `srand` in the audited case/copy/successful-cookie paths. The appearance constructor adds zero. These counts start at the stated boundaries; complete actor allocation, other actor-constructor subobjects, earlier world calls and later stat/behavior work are not silently included.

After the appearance helper returns, the actor caller reads the 16-bit word at new actor `+0x7E`, which is appearance `+6`, clears mask `0x0200`, ORs mask `0x0800`, and writes the resulting 16-bit word back. Thus the fresh kind-17 result changes from `0x0428` to `0x0C28`, and the fresh kind-96 result from `0x0028` to `0x0828`. All other bits are preserved; neither fresh case had mask `0x0200` set. This caller update occurs after all generation writes and before the following source-derived amount/position and behavior-object setup visible in the caller. It does not write byte `+5`. [Evidence: native RVAs `0x2159B4–0x215A46`.]

## Bounds

The full-extent values apply to fresh appearance state, valid disjoint kind/appearance storage, null optional input and an ordinary return. Applying a case to an arbitrary reused record gives only the common-prefix and case-specific writes, with the case mask ORed into the incoming word; it does not reset all other fields to the construction column. The full-width kind equality and byte-`+5` limit remain part of the contract.

Neither scoped case reads actor position, equipment, class, specialization, update duration, local/remote discriminator or a world pointer. Their output relation has no data-dependent branch after exact kind selection. The five prefix draws remain mandatory even though their values do not reach these appearances. This does not imply that the complete world action is independent of those surrounding inputs or random state.

Unclosed dependencies include nonnull optional input, other kinds, complete action-92 admission and configuration production, runtime DLL internal random state/algorithm, invalid-pointer or aliased storage, asynchronous mutation, cookie failure, whole actor-constructor and allocator exception behavior, later appearance mutations, rendering interpretation and a server counterpart. No semantic asset identity, additional random draw, default padding value or gameplay classification is inferred for those dependencies.
