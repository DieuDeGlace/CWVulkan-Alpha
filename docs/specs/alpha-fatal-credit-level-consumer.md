# Alpha fatal-credit and level consumer

**FINAL / STABLE / HELD** — Source-author recertification, 2026-11-21.

This contract follows one fatal-credit record from lookup to its last item write in the original Alpha client. It also covers each level increase made while consuming that record.

## Supported image and evidence

The reference is `binary/Cube.exe` under `disassembled alpha cube world`. The x86 image has preferred base 0x00400000, length 3,885,568 bytes and SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All routine and instruction addresses below are RVAs in that image. Member offsets are byte offsets from the object named.

On 2026-11-21 the source author verified the image identity and inspected the consumer at 0x1A0BF0–0x1A0D7A and level helper at 0x047B00–0x047C1F through their returns. Native operands establish receiver identity, field widths, comparisons, revised lookup results and floating-point precision. The original decompilations used for comparison are in `decompiled/client/functions/`. Source filenames and further native evidence appear with the relevant observations.

## Resolving the two creatures

The consumer receives the world and a pointer to a credit record. Call the first resolved creature A, the second B and the creature currently designated by the pointer at world+0xB8 C. The linked identifier described below is at creature+0x11C8. Its presence establishes a lookup relationship only. Ownership and party membership require further evidence.

| Credit record offset | Width | Use |
| --- | --- | --- |
| +0x00 | 8 bytes | World-map identifier for A, the credited creature |
| +0x08 | 8 bytes | World-map identifier for B, the subject of the fatal credit |
| +0x10 | Unread | No admission or award decision uses this member |
| +0x14 | 4 bytes | Raw amount to add to each admitted recipient |

The world map at +0x04 is searched first for A and then for B. Both searches happen even when the first misses. A missing key or a found node with a null creature pointer ends processing once the two results are available. No later writes take place. The search creates no creature. Identifiers with value zero take the ordinary search path. Equal identifiers and distinct identifiers resolving to the same creature are both admitted when the resulting pointers are nonnull.

Lookup orders the upper 32 bits as signed, breaking ties with the lower 32 bits as unsigned. A match identifies the complete eight-byte key. For a found node the creature pointer is four bytes at node+0x18. The world lookup wrapper at 0x02F000 returns zero both on a miss and on a found null pointer. Invalid record pointers, truncated storage and stale creature pointers are outside the evidence for ordinary operation.

Evidence includes consumer 0x1A0BF9–0x1A0C48, map search 0x077D90 and world lookup 0x02F000. The corresponding original files are `005a0bf0.c`, `00477d90.c` and `0042f000.c`.

## Cleanup when B is the designated creature

After A and B resolve, the consumer compares B's pointer with C. A match admits cleanup regardless of the mode byte at world+0xB4. The comparison uses pointer identity. The consumer does not need a separate identifier for C.

1. Search for C's full linked identifier, including zero. If that entry is missing or stores a null pointer, continue directly to step 4.
2. Resolve the linked identifier again through 0x02F000. Write the four-byte zero bit pattern to health at +0x16C of the creature returned by this second search. Its previous health is unrestricted.
3. Read the designated pointer from the world again and resolve its linked identifier through 0x02F000 a third time. Clear the map at +0x13A4 of this result. In a valid tree the clear releases the nodes, restores the three sentinel links to the sentinel itself and sets the four-byte size to zero.
4. Read the designated pointer again. Clear four bytes at its +0x11D0, then four at +0x11D4. The linked identifier at +0x11C8 is preserved.

The two repeated world lookups do not recheck the returned pointers for null. This sequence describes stable world and creature storage. It supplies no guarantee if a linked actor is removed between searches. The cleared map is on the creature returned by the third search. Its clear has no hit application or fatal notification. Aliases of A, B or C take these same steps. Both experience decisions follow the cleanup, so the health write can affect later admission.

Evidence includes 0x1A0C4E–0x1A0CC8, tree clear 0x27E480, recursive node release 0x02CA80 and the deallocation import thunk at 0x28D66E. Native traversal continues after individual deallocations. The related original decompilations are `0067e480.c` and `0042ca80.c`.

## Experience awards in execution order

**Designated award.** Read C from the world. When C is nonnull, A equals C and the byte at B+0x60 equals 1, add record+0x14 to C's four-byte experience at +0x194. Read the designated pointer again and call the level helper on it. This award has no health test or world-mode requirement. It admits amounts of zero and amounts with the sign bit set.

**Linked award.** After that decision and any level processing, read the byte at world+0xB4. Only zero admits a search for A's full linked identifier at +0x11C8. The resulting creature P must exist, have a nonnull pointer and compare strictly above zero on its binary32 health at +0x16C. Both signs of zero and negative health fail. When the floating-point comparison completes with an unordered result, it also fails. An unmasked floating-point exception may interrupt the comparison. This branch can run with A distinct from C or B+0x60 different from 1.

For an admitted P, reread the record amount, add it to P+0x194, then call the level helper with P as receiver. Each award uses the whole amount. Each addition retains the low 32 bits, including wrap. No division between recipients, saturation or sign check occurs. The branches are independent. If P is also C, the linked health check sees the effects of cleanup and C's first level-helper call. When both decisions admit, that creature receives both additions. At both calls the world is also passed as a four-byte argument, unused by the inspected level helper and removed on return.

After the linked helper returns, copy P's final four bytes of experience to A+0x1024, then copy the low two bytes of P's level at +0x190 to A+0x1030. The low word is copied even when the full level is outside a 16-bit range. Finally, copy the selected item fields from A+0x1020 to A+0x1D48 as specified below. These three writes and copies follow every admitted linked award, including one with no level increase. A missing P or a failed health comparison skips the linked addition, level helper and item updates.

Evidence includes designated addition and call at 0x1A0CE6 and 0x1A0CF2, linked health comparison at 0x1A0D2D, linked addition and call at 0x1A0D39 and 0x1A0D41, and final field and item transfers at 0x1A0D46–0x1A0D6D.

## Level threshold and repeated health updates

Read the entry level at creature+0x190 as signed 32-bit. A level of zero or below returns immediately. For a positive level, the helper calculates the threshold in the following order. Each conversion to binary32 and each scalar arithmetic step produces a binary32 result under the active SSE floating-point controls.

1. Convert the signed level to binary32 and subtract 1.
2. Multiply by the binary32 number with bits 0x3D4CCCCD, then add 1. That multiplier is the binary32 approximation to 0.05.
3. Divide 1 by the preceding result. Subtract the quotient from 1.
4. Multiply by 1000, then add 50. Convert the result to signed 32-bit by truncating toward zero.

| Binary32 operand | Original constant RVA | Stored bits |
| --- | --- | --- |
| 1 | 0x345DC0 | 0x3F800000 |
| Approximation to 0.05 | 0x2FCDB4 | 0x3D4CCCCD |
| 1000 | 0x2FD8F4 | 0x447A0000 |
| 50 | 0x2FCD7C | 0x42480000 |

Compare the four-byte experience as signed with that integer threshold. Equality admits an increment. For each admitted increment, recalculate the threshold from the current level, subtract it from experience and store the level plus one. Both integer operations retain the low 32 bits. Query maximum health through 0x044DB0 on this same creature, then store its return value to the binary32 health field at +0x16C. The query sees the debited experience and the increased level. The assignment replaces the old health with the queried value, with no additive adjustment or clamp. The native caller stores the scalar from the x87 return register.

Next compute a threshold from the creature's current level and repeat while signed experience reaches or exceeds it. Only entry has the positive-level gate. For N increments that finish normally, the helper calculates one entry threshold plus two thresholds per increment. It makes N maximum-health queries and N health assignments, all in this call. An experience total below the first threshold causes neither a query nor a write to health. The helper imposes no fixed increment count.

For example, under round-to-nearest binary32 arithmetic, level 1 requires 50 and level 2 requires 97. A creature entering at level 1 with 146 experience reaches level 2 with 96 left and gets one health assignment. Entering with 147 reaches level 3 with zero left and gets two. These are calculated examples from the original operations. The helper establishes no special floating-point control mode. Exceptional arithmetic, an unmasked exception or level wrap on a later iteration has no assigned termination or recovery guarantee here.

Evidence includes the original `00447b00.c`, native threshold admission at 0x047B03–0x047B64, the four original constant words, the debit at 0x047BA3, level write at 0x047BAE, health query and assignment at 0x047BB4 and 0x047BB9, and continuation through 0x047C1F.

## Item transfer and nested calls

The item helper at 0x02C5E0 transfers these fields in order. Offsets in this table are relative to each item's beginning.

| Offsets | Transfer |
| --- | --- |
| +0x00, +0x01 | One byte each |
| +0x04, +0x08 | Four bytes each |
| +0x0C through +0x0E | One byte each |
| +0x10 | Two bytes |
| +0x14 through +0x113 | 32 successive eight-byte portions, each transferred as four single bytes followed by one four-byte field |
| +0x114 through +0x117 | One four-byte field |

The destination's previous bytes at +0x02, +0x03, +0x0F, +0x12 and +0x13 survive the copy. Both the newly written experience at item+0x04 and the level word at item+0x10 reach the destination. The helper makes no nested call or allocation. It returns the destination pointer, which the consumer discards. Evidence is the native body at 0x02C5E0–0x02C674 and `0042c5e0.c`.

The consumer's direct calls cover lookup, map clearing, level processing and item transfer. The level helper calls only maximum health. Neither body directly draws randomness, seeds a generator, calls through a virtual method, emits a presentation event or dispatches a game callback. Maximum health calls the power-math import via 0x28D8D4 and conditionally asks 0x0C70B0 for equipped-item health contributions. That item helper calls numeric helper 0x045F60, which reaches the same power import. No game-random or seed call appears in those bodies. Import internals, allocator exceptions and the complete maximum-health and item-stat formulas lie outside this contract. Evidence includes the native call sites and `00444db0.c`, `004c70b0.c` and `00445f60.c`.

## Callers and evidence limits

The original hit applier at 0x195A60 appends a record on its admitted path, then passes the local credit record to the consumer at 0x1968E8. A second call site at 0x08FB25 in the client frame routine at 0x088EE0 processes the current record in a collection traversal before advancing to the next. Both call the same consumer. The hit site is visible in `00595a60.c` and in the binary at 0x19689C–0x1968E8. Frame traversal is visible at 0x08FB03–0x08FB5E. The original `decompiled/client/calls.csv` also identifies both callers.

The consumer itself neither deduplicates records, verifies B's fatal health nor retries a missing key. Repeated calls with eligible state can award again. The evidence here does not establish whether normal callers deliver duplicates, how a credit amount is chosen, which records arrive over the network, or how actor lifetime is protected against concurrent changes. Meanings for the unused +0x10 record member, world mode byte, designated pointer and linked actor fields beyond their observed use also remain open.

**Source-author readback, 2026-11-21.** The complete contract was checked against the identified original image, including both lookups, the separate cleanup lookups, both awards, signed threshold decisions, each health query, the item field mask and both caller sites. The numeric examples are derived from the checked constant words. This is static evidence for the named image. Runtime observations and behavior outside these routines remain separate.
