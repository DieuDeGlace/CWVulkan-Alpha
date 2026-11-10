# Alpha fatal regional progress and recipient credit order

Status: FINAL / STABLE / HELD. This contract covers the regional-progress and later recipient traversal within original client hit applier RVA 0x195A60. It closes the spatial admission, regional selector/default behavior, event extent, completion-tail ordering and distinction between contact/threat cleanup and membership-tested credit. It begins within an admitted fatal path; it does not reconstruct all earlier hit protection or damage resolution.

## Image and evidence

The evidence root is identified relatively as `disassembled alpha cube world`. Evidence paths below are relative to that root. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base 0x00400000. All addresses are RVAs in that image.

The original body and native instructions of 0x195A60 were inspected with `decompiled/client/functions/00595a60.c` and `decompiled/client/assembly.txt`. The selector tables in the original image were inspected as bytes, independently of the high-level switch rendering. Individually identified original helpers are cited below. No reconstruction, earlier contract, implementation or runtime material was used.

T denotes the resolved fatal target, W the world, and O the output aggregate. I is the complete identifier at incoming hit record+8. L is the complete identifier stored at T+0x11C8. I is taken from the incoming record, not substituted with T's own identifier member. Later, K denotes a world-map entry's stored identifier and R its stored creature pointer. The numeric labels avoid assuming owner, party, pet or quest meanings not proved here.

## Entry and preceding effects

At 0x196139–0x196193 the ordinary finite-health fatal path writes zero to T's binary32 health at +0x16C, appends its presentation output, and writes zero to T+0x6C. This follows earlier hit admission and health processing. An unordered health comparison does not establish entry to this fatal path. This document does not make these later regional predicates sufficient for admission to the whole hit applier.

Earlier fatal work also visits a matching 0x188-byte descriptor in a spatial object's +0x0C/+0x10 vector, calls helper 0x1A58E0 and publishes a separate output at O+0x30 before loot routine 0x19B180. That descriptor work is distinct from the later completion-tail removal specified below. Its complete mutation policy is outside this contract.

Before regional progress, the fatal path tests whether either 32-bit half of L is nonzero. Only for a nonzero complete L does it search W's creature map and, on a present nonnull result, search again and write zero to the resolved linked creature's health at +0x16C. It does not require positive linked health or a pointer distinct from T. This nonzero gate belongs to that linked-health operation. It is not reused by the later contact and relationship cleanup, which still compare against L when L is zero.

The fatal path next prepares a binary32 credit amount. Its modifiers are separate from regional admission: target word +0x7E bit 0x0200 scales the prepared amount by binary32 10, and bit 0x0800 subsequently replaces it with zero. These flags do not themselves skip the regional branch or the later credit-record membership test. The complete earlier numeric amount calculation remains a dependency; the later record conversion is specified below.

Evidence: native 0x196139–0x19638D, particularly the linked-ID test and repeated lookup at 0x196279–0x1962DC and amount flag operations at 0x19635E–0x196385. Loot, descriptor-helper internals and earlier protection/category branches are not expanded here.

## Spatial lookup and coordinate conversion

Regional lookup is attempted only when W's byte +0xB4 equals zero. It reads T's signed four-byte coordinates at +0x1B0 and +0x1B4, divides each by 8 with truncation toward zero, and supplies those two integer quotients to helper 0x087DA0, first coordinate first. This is not floor division for negative inputs.

Helper 0x087DA0 admits each signed argument from 0 through 8191 inclusive. A negative argument or one equal to 8192 is rejected with null. For admitted arguments, it obtains the underlying world block using each argument divided by 8. That block lookup at 0x034A90 admits block coordinates from 0 through 1023 inclusive and reads the four-byte block pointer from W+0xBC in first-coordinate-major order with 1024 second-coordinate entries per row. A null block returns null from the regional helper.

For a nonnull block, the regional helper returns an interior 0x68-byte record. Its address is the block's +0x14018 base plus 0x68 times the index formed by eight times the first argument's remainder modulo 8 plus the second argument's remainder modulo 8. It neither allocates a record nor checks the record's selector or state before returning it. Call this record M.

For ordinary signed target coordinates, -1 through -7 truncate to regional argument zero, while -8 produces -1 and is rejected. Coordinate 65535 produces 8191 and can pass; 65536 produces 8192 and is rejected. Admission of the quotient still requires a nonnull underlying block. These examples do not imply that every admitted quotient has an allocated world block.

Evidence: native 0x19640A–0x196433 and the complete helper at 0x087DA0–0x087E24; `decompiled/client/functions/00487da0.c`, `00434a90.c`. The later cell lookup helper 0x034D10 is a different boundary, admitting each original cell coordinate from 0 through 65535 and returning a stored cell pointer; it is used for tail removal below.

## Regional state, flag and contributor admission

After nonnull M is obtained, M's one-byte state at +0x41 must differ from 2. An already-2 state skips the regional mutation, event append and completion-tail work. Other byte values are not normalized before this test.

Next, either bit 0x2000 in T's two-byte +0x7E field must be set, or both of these conditions must hold: M's four-byte selector at +0x34 equals 5, and T's byte +0x1D38 is nonzero. Bit 0x2000 bypasses these latter two predicates only. It does not bypass the nonzero world-mode rejection, null spatial record, already-2 state, contributor search or selector switch. Earlier uses of that target flag in the hit resolver are not reconstructed by this regional rule.

The routine then traverses T's relationship map at +0x13A4 in its existing forward tree order. A contributor entry qualifies only when its binary32 value at node+0x18 compares strictly greater than zero, its complete eight-byte key at node+0x10 resolves in W's creature map, the stored creature pointer is nonnull, and that creature's byte +0x60 equals zero. Zero and negative values do not qualify; an unordered floating comparison does not qualify. The world lookup is repeated through helper 0x02F000 before the creature-byte test.

No contributor health, distance, designated-pointer equality or key-nonzero test is present here. A key zero is eligible if it resolves to a suitable creature. The first entry satisfying these predicates reaches the selector switch. There is no count increment for every contributor: a supported selection performs its regional work once and leaves this search. An unsupported selection also leaves this search, without trying a later qualifying contributor.

Evidence: native 0x196439–0x1964FA; original map lookup helpers `decompiled/client/functions/00477d90.c` and `0042f000.c`. Full map initialization, relationship production and contributor-object lifetime remain outside this boundary.

## Selector result, count wrap and default behavior

| M+0x34 selector | Established result after contributor admission |
| --- | --- |
| 1, 2, 3, 4, 5 or 13 | Set the local completion byte to 1 and M state +0x41 to 2; leave count +0x44 unchanged; append one regional event |
| 7, 8, 9, 10, 11 or 12 | Increment four-byte count +0x44; write state byte +0x41 to 1; compare the stored new count with four-byte goal +0x48 as signed 32-bit; a new count at least the goal sets the local completion byte to 1 and state to 2; append one regional event in either comparison outcome |
| 6, zero, negative values or any other four-byte selector | No regional count/state change, no regional event append, no completion-tail removal from this selection; continue into the later recipient traversal |

The count increment retains the low 32 bits without saturation. It is stored before state 1 is written and before the signed goal comparison. Equality completes. There is no requirement that the old count or goal be positive. For example, incrementing raw count 0x7FFFFFFF stores 0x80000000; against an ordinary positive goal, that signed negative result does not complete. The state-1 write occurs even when the immediately following completion condition changes it to 2.

The local completion byte starts at zero for this fatal invocation. It is not initialized from M's previous state. Thus skipping an already-complete M does not request completion rewards or tail removal anew. For a supported counting selection, the event is appended even when the state byte stays at its previous value 1; it carries the updated count. Unsupported selections take the default exit before event construction. They do not publish an unchanged 56-byte snapshot.

The supported regional state/count writes precede snapshot construction and list allocation. No rollback of those writes is established for append failure.

Evidence: native 0x196393, 0x1964E6–0x19657E, with the byte selector table at 0x196C84 and three destination addresses stored at 0x196C78. The original table maps selector 6 to 0x196685, which bypasses the append site. Count/state store order is visible at 0x196501–0x196517.

## The 56-byte regional event

After supported state/count processing, a 40-byte selected-field snapshot of M beginning at +0x2C is copied to a temporary. A separate 56-byte event object is initialized, receives the two current coordinate quotients, and receives that 40-byte snapshot at event+0x10. The event is appended to O's list at +0x60 through 0x086420. It is not locally applied through the fatal-credit consumer.

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

Each nonnull cell supplies a four-byte pointer vector at cell+0x18/+0x1C and a four-byte tail boundary at cell+0xA0. The ordinary valid tail is the ascending index range from that boundary through the element before the current vector end. For each nonnull stored object pointer the routine invokes its first virtual method with argument 1. Null pointers skip that invocation. It reloads the vector beginning/end after each callback before testing the next index.

After the callbacks for that cell, it reads cell+0xA0 again and resizes the pointer vector to that count through helper 0x00D9D0. Ordinary valid shrinkage retains the prefix and changes the logical end; this resize helper does not itself invoke the removed objects' virtual methods. Those invocations already occurred, before truncation. The callbacks' precise object type, deletion policy and transitive side effects remain unresolved; argument 1 alone is not a complete lifetime contract.

The inner starting coordinate is recomputed from T when a new outer row starts. Inner and outer upper limits are also recomputed from T during traversal. Therefore exactly 64 visits is conditional on unchanged target coordinates and ordinary bounds. The native code does not preserve a separate immutable coordinate snapshot around the virtual calls. Likewise it does not clamp a corrupt tail boundary to the vector range: invalid negative, oversized or callback-modified boundaries do not have a safe removal-only guarantee. The resize helper also has a separate expansion path outside an ordinary valid tail shrink.

The regional event contains its earlier snapshot before these callbacks run. Recipient iteration starts afterward. A callback that destroys T, changes cell storage or mutates the world map can invalidate assumptions used later; this audit does not establish safe continuation for such cases.

Evidence: native 0x19657E–0x196685; `decompiled/client/functions/00434d10.c`, `0040d9d0.c`. The virtual invocation is at 0x19660B; the subsequent resize is at 0x19662A. The helper's shrink and expansion boundaries were checked in the original image.

## Per-recipient cleanup uses identities without membership admission

At 0x196685 the routine starts a fresh traversal of W's creature map in forward tree order. Each entry supplies its key K and stored pointer R. Null stored pointers are skipped. R does not need positive health or a particular +0x60 byte for contact/threat cleanup, and there is no mode-byte gate at this later traversal.

First, R's complete contact identifier at +0x11D0/+0x11D4 is compared with I, then with L as needed. A match to either writes zero separately to those two four-byte fields. These are raw integer comparisons of both identifier halves, not floating comparisons despite ambiguous decompiler types.

Next, the routine scans R's relationship map at +0x13A4 and gathers iterators to entries whose complete keys equal I or L. Their binary32 relationship values are not tested. It finishes this scan before erasing the selected entries through 0x03EDE0, in collected traversal order. This deferred erasure removes the matching entries themselves rather than replacing their values with zero. The temporary iterator list stores four-byte iterators and has a native element limit of 0x15555554; its storage is released after this recipient's credit/reward work.

Zero and alias cases remain identity operations. L equal to zero still matches a zero contact or zero relationship key, although the earlier linked-health lookup was skipped for that L. I equal to L causes one matching map entry to be collected once, not erased twice. There is no exclusion for R equal to T or the linked creature, nor any deduplication of distinct world entries storing the same R pointer. An alias entry can therefore cause the same creature object to be visited again under a different K.

Evidence: native 0x196685–0x196824 and 0x196AD8–0x196B5F; `decompiled/client/functions/00583cb0.c` and `0043ede0.c`. The map's native key ordering compares upper 32-bit words as signed and lower words as unsigned when upper words tie. Structural corruption, mutation of gathered iterators by external callbacks and allocator failure remain outside the successful traversal contract.

## Membership-tested credit uses the world entry key

Only after R's contact cleanup and collected relationship erasures does the routine consider ordinary fatal credit for this entry. T's byte +0x60 must equal 1 and R's byte +0x60 must equal zero. It then searches the current T+0x13A4 map using K, the world-map entry key. It does not substitute the identifier stored at R+8, use the source identifier from the hit, or require a reciprocal surviving entry in R's map.

Presence is the test. The value of T's matching relationship entry is not read for this credit decision: zero, negative and unordered values do not fail a presence-only lookup. This differs from the strictly-positive contributor value needed for regional progress. There is no health, distance, owner, designated-pointer or K-nonzero gate here.

An admitted credit record is appended to O+0x48 with this 24-byte payload:

| Payload offset | Width | Value |
| --- | --- | --- |
| +0x00 | 8 bytes | K from the world-map entry |
| +0x08 | 8 bytes | Incoming record identifier I |
| +0x10 | 4 bytes | T's four-byte member +0x64 |
| +0x14 | 4-byte signed integer conversion | Prepared binary32 credit amount, truncated toward zero |

The append retains this record order and uses the native four-byte list count with limit 0x07FFFFFE. It is followed immediately by synchronous local consumption through 0x1A0BF0 at 0x1968E8, before this recipient's completion-reward branch and before visiting the next world entry. This audit identifies the call boundary without restating the callee's complete level/linked-actor policy.

The membership test occurs after cleanup, against the map as it then exists. It is not a snapshot of contributors taken before all recipients. If an earlier recipient alias is T itself, its cleanup can remove keys from T's own map before a later K is considered. Earlier local consumption can also mutate linked actors and their relationship state. The native order must not be replaced by an unordered batch of precomputed credit decisions. Equal identifiers, K zero and pointer aliases receive the rules above; they are not filtered by a separate anti-self rule at this producer site.

Evidence: native 0x196824–0x1968ED. Key address formation uses the world entry at +0x10; record construction and truncation are at 0x196862–0x19689D; the append and immediate consumer call are at 0x1968A0–0x1968E8. Full prepared-amount arithmetic, consumer behavior and transport are explicit dependencies outside this section.

## Completion rewards and remaining limits

For a T-byte-1/R-byte-0 entry, the local regional-completion byte is tested after the optional membership-tested credit. A missing K in T's relationship map skips ordinary credit but still reaches this completion test. Thus the later completion-reward route is not itself admitted by that membership test.

On the completed route, an item-event append to O+0x40 precedes an optional local application when R is W's designated pointer at +0xB8. A direct random draw at 0x196A1E then selects a nested item generator; a second item-event append and analogous designated-pointer application follow. These are per-entry ordering boundaries before the temporary cleanup list is freed and the next world entry is visited. The full reward payload, numeric amount, item-generator draw count, local application and runtime state are not specified here. No direct random draw occurs in the regional contributor/selector/event or relationship cleanup sites themselves. Earlier loot and the later completion generators can consume randomness and invoke unaudited nested work.

The byte at T+0x1D38 has a verified producer: creature constructor 0x03B690 initializes it to zero at 0x03BB66. In world descriptor activation, a newly constructed creature receives a raw byte copied from the current descriptor's +0x10E8 at 0x210179–0x210185. The copy does not normalize it to zero/one. Original native 0x20FFB4–0x2100FA identifies that destination as the constructed creature and the source as its current descriptor. This closes that producer identity, not every origin or mutation of descriptor+0x10E8, indirect copies of creature state, or the byte's full lifetime.

This contract leaves earlier hit admission, all protection predicates, full fatal amount/stat arithmetic, the earlier descriptor helper and respawn-marker work, complete world publication/receive paths, virtual tail-object lifetime, generator/local reward application and malformed-state behavior unresolved. It does not label the numeric regional selectors or mode bytes with unsupported gameplay meanings. All prior held artifacts remain unopened and unchanged.
