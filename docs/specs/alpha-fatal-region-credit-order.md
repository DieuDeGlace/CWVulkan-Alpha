# Alpha fatal regional progress and recipient credit order

**FINAL / STABLE / HELD**

Status: FINAL / STABLE / HELD, re-certified 2026-11-21 (see the final section). This contract covers the regional-progress and later recipient traversal within original client hit applier RVA 0x195A60, together with the fatal-path steps between fatal entry and regional progress that affect its inputs, and the final per-target marker writes after the traversal. It closes the spatial admission, regional selector/default behavior, event extent, completion-tail ordering, prepared credit amount, completion-reward amount and generator selection, and the distinction between contact/threat cleanup and membership-tested credit. It begins within an admitted fatal path; it does not reconstruct all earlier hit protection or damage resolution.

## Image and evidence

The evidence root is identified relatively as `disassembled alpha cube world`. Evidence paths below are relative to that root. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base 0x00400000. All addresses are RVAs in that image.

The original body and native instructions of 0x195A60 were inspected with `decompiled/client/functions/00595a60.c` and `decompiled/client/assembly.txt`. Constants, import slots and the selector tables were read as bytes from the original image, independently of the high-level switch rendering. Where the instruction listing omits bytes (after the temporary-list free calls at 0x196AF7 and 0x196B06), those bytes were decoded from the image. Individually identified original helpers are cited below. No reconstruction, earlier contract, implementation or runtime material was used.

W denotes the world object on which 0x195A60 is invoked, O the output aggregate passed as its second argument, and the incoming hit record its first argument. I is the complete eight-byte identifier at incoming hit record+8. T denotes the resolved fatal target: the stored creature pointer of the entry found by searching W's creature map at W+4 with I (0x195AAB–0x195AE7; a missing entry or null pointer ends the routine). L is the complete identifier stored at T+0x11C8. I is taken from the incoming record, not substituted with T's own identifier member. Later, K denotes a world-map entry's stored identifier and R its stored creature pointer. Because I is the key that resolved T, the world entry whose K equals I ordinarily stores T. The numeric labels avoid assuming owner, party, pet or quest meanings not proved here.

## Entry and preceding effects

At 0x196139–0x196143 the fatal path is entered when T's binary32 health at +0x16C compares ordered and less than or equal to zero; this includes negative zero and negative infinity. A positive or unordered (NaN) health leaves the routine without any of the work below. On entry, at 0x196150–0x19618C, it writes zero to T's health, appends a presentation output to O+0x08 (built from T's position at +0x10, with a four-byte kind 0x1B and two binary32 fields 1.0), and writes zero to T+0x6C. This follows earlier hit admission and health processing. This document does not make these later regional predicates sufficient for admission to the whole hit applier.

Next, at 0x196193–0x19621E, the cell for T's raw coordinates +0x1B0/+0x1B4 is obtained through 0x034D10. For a nonnull cell, the routine scans that cell's vector of 0x188-byte descriptors at cell+0x0C/+0x10 in ascending order and stops at the first descriptor whose four-byte field +0x00 equals 5, whose four-byte field +0x178 equals T's four-byte +0x1B8, and whose byte +0x30 is nonzero. For that first match only, it calls helper 0x1A58E0 (descriptor, O, 0) and appends to O+0x30 a record carrying T's two raw coordinates, the descriptor's index and the descriptor. That descriptor work precedes loot routine 0x19B180 and is distinct from the later completion-tail removal specified below. The helper's mutation policy is outside this contract.

After loot, at 0x196233–0x196277, the routine traverses W's creature map in forward order and, for each nonnull stored creature whose byte +0x60 is zero, computes the 64-bit position difference between that creature's +0x10 position and T's +0x10 position into a temporary through 0x02C7A0. That helper only writes the temporary; the result is not used afterward and the traversal has no persistent effect established here.

Before regional progress, the fatal path tests whether either 32-bit half of L is nonzero. Only for a nonzero complete L does it search W's creature map and, on a present nonnull result, search again and write zero to the resolved linked creature's health at +0x16C. It does not require positive linked health or a pointer distinct from T. This nonzero gate belongs to that linked-health operation. It is not reused by the later contact and relationship cleanup, which still compare against L when L is zero.

Evidence: native 0x196139–0x19638D, particularly the descriptor scan at 0x1961A8–0x1961D8, the linked-ID test and repeated lookup at 0x196279–0x1962DC; `decompiled/client/functions/0042c7a0.c`. Loot, descriptor-helper internals and earlier protection/category branches are not expanded here.

## Prepared credit amount

The fatal path next prepares a binary32 credit amount A at 0x1962E6–0x19638D. All arithmetic below is binary32 with each operation rounded; the helper result returns through the x87 stack as an exactly representable binary32 value and is stored back unchanged.

1. S is the 32-bit wrapping sum of T's byte +0x1A8 (zero-extended) and T's four-byte +0x190, converted from signed 32-bit integer to binary32.
2. Helper 0x03CA60 maps a value x to 1 − 1 / ((x − 1) × 0.05 + 1), where 1 is binary32 0x3F800000 (image 0x745DC0) and 0.05 is binary32 0x3D4CCCCD (image 0x6FCDB4), evaluated in that order: subtract, multiply, add, divide, subtract.
3. A is that result multiplied by 20 (0x41A00000, image 0x6FCD74). If 1 compares ordered and greater than A, A becomes exactly 1; otherwise, including an unordered A, A is unchanged.
4. If T's four-byte +0x64 equals any of 0x65, 0x6B, 0x6C, 0x6D, 0x72, 0x73, 0x74, 0x75, 0x76 or 0x77 (helper 0x0445D0), A is multiplied by 20 again.
5. T's two-byte +0x7E is read once, zero-extended. Bit 0x0200 multiplies A by 10 (0x41200000, image 0x745E74). Bit 0x0800, tested afterwards, replaces A with positive zero.

These flags do not themselves skip the regional branch or the later credit-record membership test. The later record conversion is specified below.

## Spatial lookup and coordinate conversion

At 0x196393–0x1963FA the local completion byte and a 40-byte temporary snapshot are zeroed. Regional lookup is then attempted only when W's byte +0xB4 equals zero; a nonzero byte goes directly to the recipient traversal. It reads T's signed four-byte coordinates at +0x1B0 and +0x1B4, divides each by 8 with truncation toward zero, and supplies those two integer quotients to helper 0x087DA0, first coordinate first. This is not floor division for negative inputs.

Helper 0x087DA0 admits each signed argument from 0 through 8191 inclusive. A negative argument or one equal to or above 8192 is rejected with null. For admitted arguments, it obtains the underlying world block using each argument divided by 8. That block lookup at 0x034A90 admits block coordinates from 0 through 1023 inclusive and reads the four-byte block pointer from W+0xBC in first-coordinate-major order with 1024 second-coordinate entries per row. A null block returns null from the regional helper.

For a nonnull block, the regional helper returns an interior 0x68-byte record. Its address is the block's +0x14018 base plus 0x68 times the index formed by eight times the first argument's remainder modulo 8 plus the second argument's remainder modulo 8. It neither allocates a record nor checks the record's selector or state before returning it. Call this record M.

For ordinary signed target coordinates, -1 through -7 truncate to regional argument zero, while -8 produces -1 and is rejected. Coordinate 65535 produces 8191 and can pass; 65536 produces 8192 and is rejected. Admission of the quotient still requires a nonnull underlying block. These examples do not imply that every admitted quotient has an allocated world block.

Evidence: native 0x19639A–0x196433 and the complete helper at 0x087DA0–0x087E24; `decompiled/client/functions/00487da0.c`, `00434a90.c`. The cell lookup helper 0x034D10 is a different boundary, admitting each original cell coordinate from 0 through 65535, selecting the block by coordinate divided by 64 and returning the four-byte cell pointer at block+0x10018 indexed by 64 times the first coordinate's remainder modulo 64 plus the second's; it is used for descriptor lookup above and tail removal below.

## Regional state, flag and contributor admission

After nonnull M is obtained, M's one-byte state at +0x41 must differ from 2. An already-2 state skips the regional mutation, event append and completion-tail work. Other byte values are not normalized before this test.

Next, either bit 0x2000 in T's two-byte +0x7E field must be set, or both of these conditions must hold: M's four-byte selector at +0x34 equals 5, and T's byte +0x1D38 is nonzero. Consequently, without bit 0x2000 only selector 5 can reach the selector switch. Bit 0x2000 bypasses these latter two predicates only. It does not bypass the nonzero world-mode rejection, null spatial record, already-2 state, contributor search or selector switch. Earlier uses of that target flag in the hit resolver are not reconstructed by this regional rule.

The routine then traverses T's relationship map at +0x13A4 in its existing forward tree order. A contributor entry qualifies only when its binary32 value at node+0x18 compares strictly greater than zero, its complete eight-byte key at node+0x10 resolves in W's creature map, the stored creature pointer is nonnull, and that creature's byte +0x60 equals zero. Zero and negative values do not qualify; an unordered floating comparison does not qualify. The world lookup is repeated through helper 0x02F000 before the creature-byte test.

No contributor health, distance, designated-pointer equality or key-nonzero test is present here. A key zero is eligible if it resolves to a suitable creature. The first entry satisfying these predicates reaches the selector switch, which rereads M+0x34. There is no count increment for every contributor: a supported selection performs its regional work once and leaves this search. An unsupported selection also leaves this search, without trying a later qualifying contributor. An empty map or no qualifying entry skips the regional work.

Evidence: native 0x196439–0x1964FA; original map lookup helpers `decompiled/client/functions/00477d90.c` and `0042f000.c`; the zero comparand at image 0x745D84 is binary32 0x00000000. Full map initialization, relationship production and contributor-object lifetime remain outside this boundary.

## Selector result, count wrap and default behavior

| M+0x34 selector | Established result after contributor admission |
| --- | --- |
| 1, 2, 3, 4, 5 or 13 | Set the local completion byte to 1 and M state +0x41 to 2; leave count +0x44 unchanged; append one regional event |
| 7, 8, 9, 10, 11 or 12 | Increment four-byte count +0x44; write state byte +0x41 to 1; compare the stored new count with four-byte goal +0x48 as signed 32-bit; a new count at least the goal sets the local completion byte to 1 and state to 2; append one regional event in either comparison outcome |
| 6, zero, negative values or any other four-byte selector | No regional count/state change, no regional event append, no completion-tail removal from this selection; continue into the later recipient traversal |

The switch subtracts 1 and treats results above 12 as unsigned out of range, so zero and negative selectors take the default. The count increment retains the low 32 bits without saturation. It is stored before state 1 is written and before the signed goal comparison. Equality completes. There is no requirement that the old count or goal be positive. For example, incrementing raw count 0x7FFFFFFF stores 0x80000000; against an ordinary positive goal, that signed negative result does not complete. The state-1 write occurs even when the immediately following completion condition changes it to 2.

The local completion byte starts at zero for this fatal invocation. It is not initialized from M's previous state. Thus skipping an already-complete M does not request completion rewards or tail removal anew. For a supported counting selection, the event is appended even when the state byte stays at its previous value 1; it carries the updated count. Unsupported selections take the default exit before event construction. They do not publish an unchanged 56-byte snapshot.

The supported regional state/count writes precede snapshot construction and list allocation. No rollback of those writes is established for append failure.

Evidence: native 0x196393, 0x1964E6–0x19657E, with the 13-entry byte selector table at 0x196C84 (bytes 0,0,0,0,0,2,1,1,1,1,1,1,0) and three destination addresses stored at 0x196C78 (0x196510, 0x196501, 0x196685). Selector 6 maps to 0x196685, which bypasses the append site. Count/state store order is visible at 0x196501–0x196517.

## The 56-byte regional event

After supported state/count processing, a 40-byte selected-field snapshot of M beginning at +0x2C is copied to the temporary. A separate 56-byte event object is initialized, receives the two current coordinate quotients (reread from T), and receives that 40-byte snapshot at event+0x10. The event is appended to O's list at +0x60 through 0x086420. It is not locally applied through the fatal-credit consumer.

| Event-relative offset | Width assigned | Value established at this append |
| --- | --- | --- |
| +0x00, +0x04 | Two signed four-byte integers | T+0x1B0 and T+0x1B4 divided by 8, toward zero |
| +0x08, +0x0C | Two four-byte fields | Zero |
| +0x10 through +0x23 | Five four-byte fields | M+0x2C through M+0x3F, in order; includes selector M+0x34 |
| +0x24 | One byte | M+0x40 |
| +0x25 | One byte | Updated M state +0x41 |
| +0x26, +0x27 | No assignment or semantic read established here | Opaque padding; no zero-value guarantee |
| +0x28 | Four bytes | Updated M count +0x44 |
| +0x2C | Four bytes | M goal +0x48 |
| +0x30, +0x34 | Two four-byte fields | M+0x4C and M+0x50 |

The 0x38-byte payload extent is distinct from its list node's links. The node allocation is 0x40 bytes, with payload at +8. The copy into the node assigns the fields listed above and skips the two padding bytes. It is not a raw copy of all 56 bytes, and this audit does not assign wire behavior or a meaning to that padding.

The list has a four-byte count and native limit 0x03FFFFFE. On the successful append route the payload/node is constructed, the limit is checked, the count is increased, and the tail links are joined. Allocation and length-error paths do not establish a completed event append. The event append is completed before the local completion byte is tested for tail removal.

Evidence: native 0x19651B–0x19657E; `decompiled/client/functions/00468620.c`, `00465c70.c`, `00486420.c`, `00451d40.c`, `0046d750.c`, `00465c00.c`, confirmed by their original bodies. Those two copy helpers preserve the selected 40-byte and 56-byte field extents rather than defining padding values.

## Completion-tail removal and object lifetime

Only a local completion byte equal to 1 enters this work. Under unchanged coordinates and ordinary nonoverflowing state, it visits an 8-by-8 cell area. The first starting coordinate is obtained by dividing signed T+0x1B0 by 8 toward zero, then multiplying that integer quotient by 8. The second starting coordinate is obtained in the same order from T+0x1B4. The first coordinate is the outer ascending traversal; the second is the inner ascending traversal. These are cell coordinates passed to 0x034D10, not the regional quotient arguments passed to 0x087DA0.

Each nonnull cell supplies a four-byte pointer vector at cell+0x18/+0x1C and a four-byte tail boundary at cell+0xA0. The ordinary valid tail is the ascending index range from that boundary through the element before the current vector end; the boundary is compared with the element count as signed 32-bit. For each nonnull stored object pointer the routine invokes its first virtual method with argument 1. Null pointers skip that invocation. It reloads the vector beginning/end after each callback before testing the next index.

After the callbacks for that cell (or immediately, when the boundary is not below the count), it reads cell+0xA0 again and resizes the pointer vector to that count through helper 0x00D9D0. That helper compares the requested count with the current count as unsigned. A smaller count retains the prefix and changes the logical end; an equal count changes nothing; a larger count, including a negative boundary read as unsigned, takes an expansion path that appends null pointers. The resize helper does not itself invoke the removed objects' virtual methods. Those invocations already occurred, before truncation. The callbacks' precise object type, deletion policy and transitive side effects remain unresolved; argument 1 alone is not a complete lifetime contract.

The inner starting coordinate is recomputed from T when a new outer row starts. Inner and outer upper limits are also recomputed from T during traversal. Therefore exactly 64 visits is conditional on unchanged target coordinates and ordinary bounds. The native code does not preserve a separate immutable coordinate snapshot around the virtual calls. Likewise it does not clamp a corrupt tail boundary to the vector range: invalid negative, oversized or callback-modified boundaries do not have a safe removal-only guarantee.

The regional event contains its earlier snapshot before these callbacks run. Recipient iteration starts afterward. A callback that destroys T, changes cell storage or mutates the world map can invalidate assumptions used later; this audit does not establish safe continuation for such cases.

Evidence: native 0x19657E–0x196685; `decompiled/client/functions/00434d10.c`, `0040d9d0.c`. The virtual invocation is at 0x19660B; the subsequent resize is at 0x19662A. The helper's shrink and expansion boundaries were checked in the original image.

## Per-recipient cleanup uses identities without membership admission

At 0x196685 the routine starts a fresh traversal of W's creature map in forward tree order. Each entry supplies its key K and stored pointer R. Null stored pointers are skipped. R does not need positive health or a particular +0x60 byte for contact/threat cleanup, and there is no mode-byte gate at this later traversal. The successor entry is computed from the live tree only after the current entry's cleanup, credit and reward work (0x196B14–0x196B59), and the end sentinel is reread for each comparison.

First, R's complete contact identifier at +0x11D0/+0x11D4 is compared with I, then with L as needed. A match to either writes zero separately to those two four-byte fields. These are raw integer comparisons of both identifier halves, not floating comparisons despite ambiguous decompiler types.

Next, the routine scans R's relationship map at +0x13A4 and gathers iterators to entries whose complete keys equal I or L. Their binary32 relationship values are not tested. It finishes this scan before erasing the selected entries through 0x03EDE0, in collected traversal order. This deferred erasure removes the matching entries themselves rather than replacing their values with zero. The temporary iterator list stores four-byte iterators and has a native element limit of 0x15555554; all of its nodes and its head are released after this recipient's credit/reward work.

Zero and alias cases remain identity operations. L equal to zero still matches a zero contact or zero relationship key, although the earlier linked-health lookup was skipped for that L. I equal to L causes one matching map entry to be collected once, not erased twice. There is no exclusion for R equal to T or the linked creature, nor any deduplication of distinct world entries storing the same R pointer. An alias entry can therefore cause the same creature object to be visited again under a different K.

Evidence: native 0x196685–0x196824 and 0x196AD8–0x196B5F; `decompiled/client/functions/00583cb0.c` and `0043ede0.c`. The map's native key ordering compares upper 32-bit words as signed and lower words as unsigned when upper words tie. Structural corruption, mutation of gathered iterators by external callbacks and allocator failure remain outside the successful traversal contract.

## Membership-tested credit uses the world entry key

Only after R's contact cleanup and collected relationship erasures does the routine consider ordinary fatal credit for this entry. T's byte +0x60 must equal 1 and R's byte +0x60 must equal zero. It then searches the current T+0x13A4 map using K, the world-map entry key. It does not substitute the identifier stored at R+8, use the source identifier from the hit, or require a reciprocal surviving entry in R's map. Because the two byte tests conflict when R is T, the entry storing T never receives credit or the completion reward.

Presence is the test. The value of T's matching relationship entry is not read for this credit decision: zero, negative and unordered values do not fail a presence-only lookup. This differs from the strictly-positive contributor value needed for regional progress. There is no health, distance, owner, designated-pointer or K-nonzero gate here.

An admitted credit record is appended to O+0x48 with this 24-byte payload:

| Payload offset | Width | Value |
| --- | --- | --- |
| +0x00 | 8 bytes | K from the world-map entry |
| +0x08 | 8 bytes | Incoming record identifier I |
| +0x10 | 4 bytes | T's four-byte member +0x64 |
| +0x14 | 4-byte signed integer conversion | Prepared binary32 amount A, truncated toward zero; NaN or a value outside the signed 32-bit range yields 0x80000000 |

The append retains this record order and uses the native four-byte list count with limit 0x07FFFFFE. It is followed immediately by synchronous local consumption through 0x1A0BF0 (invoked on W with the 24-byte record) at 0x1968E8, before this recipient's completion-reward branch and before visiting the next world entry. This audit identifies the call boundary without restating the callee's complete level/linked-actor policy.

The membership test occurs after cleanup, against the map as it then exists. It is not a snapshot of contributors taken before all recipients. When the entry storing T precedes a later K in key order, T's own cleanup has already erased T's relationship entries keyed I or L, so a later K equal to L fails the membership test. Earlier local consumption can also mutate linked actors and their relationship state. The native order must not be replaced by an unordered batch of precomputed credit decisions. Equal identifiers, K zero and pointer aliases receive the rules above; they are not filtered by a separate anti-self rule at this producer site.

Evidence: native 0x196824–0x1968ED. Key address formation uses the world entry at +0x10; record construction and truncation are at 0x196862–0x19689D; the append and immediate consumer call are at 0x1968A0–0x1968E8. Consumer behavior and transport are explicit dependencies outside this section.

## Completion rewards

For a T-byte-1/R-byte-0 entry, the local regional-completion byte is tested after the optional membership-tested credit. A missing K in T's relationship map skips ordinary credit but still reaches this completion test. Thus the later completion-reward route is not itself admitted by that membership test. An entry failing either byte test skips both credit and reward.

On the completed route, in this order:

1. A reward amount is computed from the temporary M snapshot: the 32-bit wrapping sum of byte M+0x40 (zero-extended) and four-byte M+0x3C, converted to binary32, mapped through helper 0x03CA60 as above, multiplied by 50 (0x42480000, image 0x6FCD7C), plus 1, then truncated toward zero to a signed 32-bit integer (NaN or out of range yields 0x80000000). Only its low 16 bits are stored.
2. A temporary item is built: type byte +0x00 = 13 (0x0D); byte +0x01, four-byte +0x04 and +0x08, two-byte +0x0C, byte +0x0E, 0x100 bytes from +0x14 and four-byte +0x114 are zero; two-byte +0x10 is the reward amount. It is appended to O+0x40 as a record of K followed by that item (native list limit 0x00DD67C7).
3. If R equals W's designated pointer at +0xB8, the item is applied locally through 0x06EBE0 on R+0x11DC with argument -1.
4. One direct draw from the C runtime `rand` import (slot at image 0x6FC468, call at 0x196A1E) is taken per such recipient. An even result selects generator 0x1F51E0; an odd result selects 0x1F8AD0. The selected generator is invoked on W with a temporary output, four-byte M+0x3C, byte M+0x40 (zero-extended) and R's byte +0x140, and its result is assigned over the temporary item.
5. The generated item is appended to O+0x40 with K as in step 2, and the designated-pointer application of step 3 is repeated for it.

These are per-entry ordering boundaries before the temporary cleanup list is freed and the next world entry is visited. The generators' internals, their own random-draw counts, the item copy extent, the local application and transport are not specified here. No direct random draw occurs in the prepared-amount, regional contributor/selector/event, tail-removal, relationship cleanup or credit sites themselves. Earlier loot, the generators and the virtual tail callbacks can consume randomness and invoke unaudited nested work.

Evidence: native 0x1968ED–0x196AD8; `decompiled/client/functions/0043ca60.c`; constants and the import name read from the image.

## Final per-target marker writes

After the recipient traversal ends (0x196B5F–0x196C49), T's raw coordinates +0x1B0/+0x1B4 must each lie from 0 through 65535 as signed values. The block at coordinate divided by 64 is read from W+0xBC (block coordinates 0 through 1023) and must be nonnull; the cell pointer at block+0x10018 indexed as for 0x034D10 must be nonnull. T's four-byte +0x1B8, as a signed index, must be nonnegative and below the current element count of that cell's pointer vector at +0x18/+0x1C. When all hold, the stored object at that index receives four-byte +0x38 = 0x00124F80 (1,200,000), and then four-byte +0x3C = W's four-byte +0x800160. Any failed test skips both writes. The vector count used is the one after any completion-tail resize, so an index removed by that resize is skipped. No null test of the stored object pointer is made. The meaning of these fields is not assigned here.

## Remaining limits

The byte at T+0x1D38 has a verified producer: creature constructor 0x03B690 initializes it to zero at 0x03BB66. In world descriptor activation, a newly constructed creature receives a raw byte copied from the current descriptor's +0x10E8 at 0x210179–0x210185. The copy does not normalize it to zero/one. Original native 0x20FFB4–0x2100FA identifies that destination as the constructed creature and the source as its current descriptor. This closes that producer identity, not every origin or mutation of descriptor+0x10E8, indirect copies of creature state, or the byte's full lifetime.

This contract leaves earlier hit admission, all protection predicates, the descriptor helper internals, loot, complete world publication/receive paths, the credit consumer, virtual tail-object lifetime, generator/local reward application and malformed-state behavior unresolved. It does not label the numeric regional selectors, mode bytes, the +0x64 value set or the marker fields with unsupported gameplay meanings. All prior held artifacts remain unopened and unchanged.

## Re-certification (2026-11-21)

Every claim was re-checked against the original image and its native listing for 0x195A60 (0x195A60–0x196C6F) and the cited helpers. Image hash confirmed. No reconstruction source was consulted. Corrections and filled omissions:

1. Status marker moved directly under the title as **FINAL / STABLE / HELD**; scope sentence widened to the sections added below.
2. Fatal entry corrected: the path is entered for ordered health less than or equal to zero, including negative infinity, not only "finite" health; NaN or positive health exits (0x196139–0x196143).
3. Identity of T made explicit: T is the creature resolved by searching W+4 with I, so the world entry keyed I ordinarily stores T (0x195AAB–0x195AE7). The earlier wording omitted this.
4. Omission filled: descriptor match predicate (+0x00 = 5, +0x178 = T+0x1B8, byte +0x30 nonzero), first-match-only behavior, raw-coordinate cell lookup and the O+0x30 record contents (0x196193–0x19621E).
5. Omission filled: a world-map traversal between loot and the linked-ID test, which only computes a discarded position difference and has no persistent effect (0x196233–0x196277; 0x02C7A0).
6. Omission filled: the complete prepared credit amount (sum, 0x03CA60 mapping, ×20, clamp to at least 1, the +0x64 value set from 0x0445D0 with a further ×20, then ×10 and zero flags), with constants read from image 0x745DC0, 0x6FCDB4, 0x6FCD74, 0x745E74 (0x1962E6–0x19638D). The previous text called this an unresolved dependency.
7. Credit conversion completed: NaN or out-of-range amounts yield 0x80000000 (truncating conversion at 0x196894).
8. Consequence stated: without +0x7E bit 0x2000 only selector 5 can reach the switch (0x196443–0x19645C); the switch rereads M+0x34 and uses an unsigned range test after subtracting 1 (0x1964E6–0x1964ED). Selector and destination table bytes are now quoted.
9. Resize behavior refined: the tail loop compares the boundary signed, the resize helper compares unsigned; an equal count is a no-op, a larger or negative boundary appends null pointers (0x1965ED–0x19662A; 0x00D9D0).
10. Traversal successor order stated: the next world entry is computed from the live tree after the current entry's work, with the end sentinel reread (0x196B14–0x196B59). Listing gap bytes 0x196AFC–0x196B13 decoded from the image: all temporary list nodes and then its head are freed, and the world-map pointer is reloaded.
11. Alias consequence sharpened: the T entry can never itself receive credit or reward (conflicting +0x60 tests), and when it precedes K = L its cleanup removes T's L-keyed relationship before that membership test (0x196824–0x19685C).
12. Omission filled: completion-reward amount formula (×50 + 1, low 16 bits), the type-13 item fields, O+0x40 limit 0x00DD67C7, one `rand` draw per qualifying recipient with even/odd selecting 0x1F51E0/0x1F8AD0, generator arguments and the second append/application order (0x1968ED–0x196AD8; import slot 0x6FC468 named `rand`).
13. Omission filled: the final per-target marker writes after the traversal, including their coordinate, block, cell and index admission and their dependence on the post-resize vector count (0x196B5F–0x196C49).
14. Cell lookup 0x034D10 boundary completed (block by coordinate/64, cell index 64×remainder+remainder at block+0x10018), and its earlier use for the descriptor scan noted.

Remaining uncertainties:

- The integer-to-binary32 conversions and all binary32 arithmetic assume the default round-to-nearest SSE control state; no control-word change was established within this routine.
- Generator internals, their random-draw counts and the item copy extent through 0x051CC0 are unresolved.
- Credit consumer 0x1A0BF0, designated application 0x06EBE0, descriptor helper 0x1A58E0, loot 0x19B180 and the virtual tail callbacks may mutate W, T or the maps; safe continuation after such mutation is not established.
- The meaning of selectors, the +0x64 value set, the marker value 1,200,000 and W+0x800160 is not assigned.
- Behavior after allocation or length-error exits (0x196C6A) is outside the contract.

No cross-spec conflicts: this contract cites no other held specification, and no other held specification in `docs/specs` references it.
