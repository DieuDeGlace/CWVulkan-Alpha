# Alpha fatal-credit and level consumer

Status: FINAL / STABLE / HELD. This contract covers original client fatal-credit consumer RVA 0x1A0BF0 and level helper RVA 0x047B00. It establishes their lookup admission, ordered mutations, experience and level arithmetic, health-query boundaries and linked-actor item copies. It does not specify the complete upstream fatal resolver or a replacement implementation.

## Image and evidence

The evidence root is identified relatively as `disassembled alpha cube world`; every evidence path below is relative to that root. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base 0x00400000. All addresses in this document are RVAs within that image.

The complete native bodies at 0x1A0BF0–0x1A0D78 and 0x047B00–0x047C1F were inspected alongside `decompiled/client/functions/005a0bf0.c` and `00447b00.c`. Native operand widths and receiver arguments take precedence over incomplete high-level argument recovery. Additional individually identified original helpers are cited below. No reconstruction, earlier project contract, runtime capture or implementation material was used.

## Input identity and unconditional lookup order

The consumer receives a world and a pointer to a credit record. Within this document, A means the creature selected by the first record identifier, B means the creature selected by its second identifier, and C means the current designated creature pointer at world+0xB8. These labels distinguish the actual lookup and pointer tests; they do not impose an owner, party or pet interpretation on the linked fields.

| Record offset | Width read | Consumer meaning |
| --- | --- | --- |
| +0x00 | 8-byte complete identifier | Selects A, the credited creature |
| +0x08 | 8-byte complete identifier | Selects B, the creature described by the fatal credit |
| +0x10 | No read by this consumer | No consumer admission rule is derived from this member |
| +0x14 | 4-byte raw integer amount | Added to admitted experience accumulators |

The first lookup uses the world creature map at +4 with the key at record+0. The second uses that same map with the key at record+8. Both lookups are performed before the consumer tests whether either returned creature pointer is null. An absent key and a present key storing a null creature pointer each prevent all of this consumer's later mutations. The second lookup is not skipped merely because the first key was absent.

The keys are complete eight-byte identifiers. There is no preliminary zero-ID test, sign test, equality rejection or source/target pointer-distinctness test. Identifier zero is an ordinary lookup key here. Equal record IDs can therefore resolve successfully and proceed, and distinct IDs storing the same nonnull pointer are not rejected as aliases. No creature is created to satisfy a missing key.

The original map lookup helper at 0x077D90 compares the upper 32-bit part as signed and the lower 32-bit part as unsigned when upper parts tie. A successful key comparison still requires the whole identifier to match. All pointer tests in this consumer are four-byte tests appropriate to the supported original image. Invalid record pointers, undersized records, corrupt map storage and stale creature pointers have no established safe result.

Evidence: native 0x1A0BF9–0x1A0C48; `decompiled/client/functions/00477d90.c`. The world lookup wrapper at 0x02F000 has the same absent-or-stored-null result convention, confirmed in `decompiled/client/functions/0042f000.c` and its native body.

## Designated fatal-creature cleanup precedes experience

After both A and B have resolved, the first effect depends on pointer equality between B and C. It does not compare B's record identifier with a separate designated identifier, and it has no world+0xB4 mode gate.

When B is C, the consumer looks up the complete eight-byte key stored at C+0x11C8 in the world creature map. There is no explicit all-zero-key exclusion. If the key resolves to a nonnull creature L, it looks that key up again through 0x02F000 and writes four zero bytes to L+0x16C, the binary32 health field. It then obtains the key from the current designated pointer again, performs another 0x02F000 lookup and clears that resolved creature's relationship map at +0x13A4 through helper 0x27E480. Thus the successful path makes one initial map search and two additional world lookups, with the health-zero write between the latter two. It does not retain just one resolved pointer for both operations.

The relationship-map clear releases the old nodes, resets its sentinel links and sets its four-byte size to zero. It clears this one map; it is not a general fatal callback or an invocation of hit application. Its native node-release traversal and deallocation calls were inspected. The surrounding creature lifetime and allocator-failure behavior are not a safe-reentry guarantee.

Whether that linked key is absent, stores null or resolves successfully, B-equals-C cleanup next writes zero separately to the four-byte fields C+0x11D0 and C+0x11D4. The eight-byte key at C+0x11C8 is not cleared by these writes. If the linked lookup fails, the health/map operations are skipped but these two designated-creature writes still occur.

All of this precedes either experience-award branch. There is no positive-health admission for L before the zero write, and no distinctness test excluding L equal to A, B or C. Alias effects follow this order. This contract does not infer the broader meaning of +0x11C8, +0x11D0 or +0x11D4 from their proximity.

Evidence: native 0x1A0C4E–0x1A0CC8; `decompiled/client/functions/0067e480.c` and `0042ca80.c`, confirmed by native map-clear and node-release bodies at 0x27E480 and 0x02CA80.

## Direct and linked experience admission

The designated-creature award is considered after cleanup. It requires a nonnull current C, pointer equality A equals C, and B's one-byte member +0x60 equal to 1. It does not require world byte +0xB4 to be zero. It does not test A's or B's health, check whether the record amount is positive, or compare record IDs for inequality.

When admitted, four bytes from record+0x14 are added to the four-byte accumulator C+0x194. The native addition is a 32-bit operation: it retains the low 32 bits, including wrap, with no saturation, wider carry accumulation or overflow rejection. Negative bit patterns are not rejected before addition. The level helper is called immediately afterward with that designated creature as its receiver. A world pointer is also passed as a four-byte stack argument at these call sites; the inspected level-helper body does not read it and removes those argument bytes on return.

The linked award is a later, independent branch. It requires world byte +0xB4 exactly zero. It searches the world creature map with the complete identifier at A+0x11C8, including a possible zero identifier. The key must resolve to a nonnull creature P whose binary32 health at +0x16C compares strictly greater than zero. Zero, negative ordinary values and an unordered comparison do not pass this native test. There is no B+0x60-equals-1 requirement in this linked branch and no A-equals-C requirement.

For an admitted P, the same four-byte record amount is added to P+0x194 with the same 32-bit wrap behavior, then the level helper runs on P. The amount is read again for this later addition; the consumer does not use a single precomputed split or divide it between recipients. There is no exclusion for P equal to A or C. Consequently the two awards are not mutually exclusive and admitted aliases can receive two additions in this ordered consumer execution. The positive-health gate for the linked branch observes state after any earlier cleanup and designated award/level processing.

After P's level helper returns, the consumer copies P's final four-byte experience accumulator to A+0x1024, copies the low two bytes of P's four-byte level at +0x190 to A+0x1030, and then copies the source item beginning at A+0x1020 to the destination item beginning at A+0x1D48. The level copy is a raw low-word copy, not a saturating level conversion. The experience copy reflects any threshold debits already performed by the level helper.

The linked branch performs these item copies even if the added amount did not cause a level increment. A failure of linked lookup or its health test skips its addition, helper call and all these item-copy effects. Missing linked actors are not substituted with the designated creature.

Evidence: native 0x1A0CCB–0x1A0D6D. The designated award's addition/helper sites are 0x1A0CE6/0x1A0CF2; the linked award's sites are 0x1A0D39/0x1A0D41; final experience, level-word and item-copy sites are 0x1A0D46–0x1A0D6D.

## Threshold calculation and exact admission boundary

The level helper reads the four-byte level at creature+0x190 as signed 32-bit. Its entry condition requires that level to be strictly positive. A zero or negative entry level returns without threshold arithmetic, experience debit, level change or maximum-health query, regardless of experience.

For a positive entry level, the threshold calculation first converts that signed level to binary32. It subtracts binary32 1, multiplies by the binary32 constant with bits 0x3D4CCCCD, then adds binary32 1. The reciprocal step divides binary32 1 by that result. The helper subtracts this reciprocal from binary32 1, multiplies by binary32 1000, and finally adds binary32 50. Each of those scalar operations has a binary32 result before the next operation. The 0x3D4CCCCD constant is the stored approximation to decimal 0.05, not an exact decimal rational.

The final binary32 quantity is converted to signed 32-bit with truncation toward zero. The experience accumulator is compared as signed 32-bit against that integer threshold. Equality is sufficient to enter a level increment. Level 1, for example, produces an exact threshold of 50 under ordinary finite arithmetic. Replacing this calculation with an algebraically simplified expression, wider intermediates, rounding to nearest integer, or a different constant representation is not established by this evidence.

| Original constant RVA | Stored bits | Numeric value or role |
| --- | --- | --- |
| 0x345DC0 | 0x3F800000 | Binary32 1 |
| 0x2FCDB4 | 0x3D4CCCCD | Binary32 approximation to 0.05 |
| 0x2FD8F4 | 0x447A0000 | Binary32 1000 |
| 0x2FCD7C | 0x42480000 | Binary32 50 |

The ordinary numeric contract assumes finite representable conversions and the applicable native floating-point controls. Integer-to-binary32 conversion and scalar arithmetic use the ambient SSE controls; the final truncating conversion is not changed into a rounding-mode-dependent nearest-integer conversion. This helper does not save, establish or restore a special floating-point control mode. Invalid floating-to-integer conversion, unmasked exceptions, corrupted numeric state and overflow-driven later levels need separate treatment; no safe termination result is assigned to them here.

Evidence: native 0x047B03–0x047B64 and original constant bytes in `binary/Cube.exe`; `decompiled/client/functions/00447b00.c`. The native instructions confirm signed integer comparisons, separate scalar operations and the truncating conversion.

## Repeated increments and maximum-health query order

An admitted increment recomputes the threshold from the current level before deducting it. The debit is a four-byte subtraction from experience. The helper then increments the four-byte level by 1 and stores it before querying maximum health through 0x044DB0. The returned scalar is stored directly to the creature's binary32 health at +0x16C. There is no read of the old health for an additive heal, no lower/upper clamp around this assignment, and no extra maximum-health query at this level-helper boundary.

After that health assignment, the helper recomputes the threshold using the now-current level and tests experience with the same signed greater-than-or-equal admission. Another admitted increment repeats the debit, increment, maximum-health query and health assignment. There is no one-level-per-record limit, fixed loop cap or wait for a later frame. The strictly-positive-level entry check is not repeated between increments.

For an ordinary completion with N increments, the helper makes exactly N direct calls to maximum-health routine 0x044DB0 and performs N health assignments. Its threshold arithmetic is evaluated once for entry admission and twice per increment: once for that increment's debit and once for the subsequent continuation test. With no admitted increment, no maximum-health query or health write occurs. Each maximum-health query observes the level already incremented for that iteration and experience already debited for it.

Experience subtraction and level increment remain native 32-bit operations, retaining their low 32 bits. Entry-level positivity does not prove positivity after a level wrap, and the continuation test does not reintroduce that protection. No total behavior or termination guarantee is stated for a record/actor state that drives the loop through integer wrap or exceptional threshold arithmetic. Ordinary repeated levels consume the thresholds for successive levels; the routine does not compare experience against one cumulative threshold computed only at entry.

Evidence: native 0x047B70–0x047C1F. The maximum-health call is 0x047BB4; the health store follows at 0x047BB9 before the continuation calculation. The complete maximum-health value formula is outside this bounded level-consumer contract.

## Item-copy extent and nested-effect boundaries

The final item-copy helper 0x02C5E0 is a direct field copy. It copies the two leading bytes separately, the four-byte fields at source-relative +4 and +8, bytes +0x0C through +0x0E, and the two-byte field at +0x10. It then copies the 0x100-byte span +0x14 through +0x113 as thirty-two eight-byte portions, each using four byte copies followed by one four-byte copy, and copies the final four-byte member at +0x114. The addressed extent ends at +0x117. Bytes +2, +3, +0x0F, +0x12 and +0x13 are not assigned by this helper; it is not an unconditional raw copy of every byte in the extent.

In this consumer its source is A+0x1020 and destination A+0x1D48. Therefore the newly stamped experience and level-word members are propagated along with the other selected source fields. The copy helper has no nested call, allocation, random draw, event append or callback. Its return pointer is unused at this site.

The two scoped consumer/helper bodies contain no direct random or seed call, presentation emission, virtual call or game callback. Their complete direct-call inventory comprises world/map lookups, map clearing, level processing, maximum-health querying and the stated item copy. The map-clear dependency uses node traversal and native deallocation. The inspected maximum-health body at 0x044DB0 calls the original power-math import through thunk 0x28D8D4 and conditionally queries equipped-item health contributions through 0x0C70B0. That item helper calls numeric helper 0x045F60, which also reaches the same power thunk. No game random or seed call is present in these inspected native helper bodies. This does not specify imported math/deallocation internals or certify behavior under malformed containers or replacement import bindings.

Evidence: `decompiled/client/functions/0042c5e0.c`, confirmed by native 0x02C5E0–0x02C674; `00444db0.c`, `004c70b0.c`, `00445f60.c`, `0067e480.c`, `0042ca80.c`; corresponding original native call sites. Complete maximum-health and item-stat arithmetic, imported-runtime internals and allocator exceptional paths remain outside this contract.

## Caller and lifetime limits

Original hit applier 0x195A60 appends a fatal-credit record and invokes this consumer at 0x1968E8 on its admitted route. The client frame also invokes this same consumer while traversing its incoming credit records at 0x08FB25. These are distinct callers; the consumer's internal rules above do not depend on which of those two call sites supplied the record. The complete upstream credit eligibility, amount calculation, queue transmission, frame admission and duplicate-delivery prevention are not reconstructed here.

Within the consumer there is no record identifier used for deduplication, no fatal-health recheck on B and no automatic retry for absent creature IDs. Repeated admitted calls can award repeatedly. This contract does not infer that duplicate records ordinarily occur, that either caller permits arbitrary records, or that creature lookup results remain safe under external mutation outside the original ownership/locking conditions. Those caller and lifetime conditions are unresolved dependencies.

The record's unused +0x10 member, opaque storage beyond the read fields, linked-actor lifecycle, semantic names for mode byte +0xB4 and designated pointer +0xB8, exceptional integer/floating states, partial records and concurrency/reentrancy require independent evidence before broader claims. All earlier held artifacts remain outside this audit.
