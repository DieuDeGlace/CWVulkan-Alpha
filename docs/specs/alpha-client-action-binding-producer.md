# Alpha client action-binding producer

Status: FINAL / STABLE / HELD.

## Evidence and boundary

This contract supports original 32-bit `binary/Cube.exe` from the evidence root `disassembled alpha cube world`, length 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x400000`. Every address below is an RVA. The original image, individually identified original function files and `decompiled/client/assembly.txt` were inspected. No reconstruction, other contract, replacement implementation or runtime material was used.

The scope is construction and frame refresh of the vector at client `+0x800814`, including all values produced by the identified refresh and its primary selector. It is not a description of keyboard keys, UI labels or full action execution. All binding values in tables and prose are **decimal**; hexadecimal offsets and masks retain the `0x` prefix.

The source creature is the pointer at client `+0x8006D0`. Its class byte is `+0x140`, specialization byte is `+0x141`, kind/discriminator is the four-byte field `+0x64`, action is the byte `+0x68`, and action clock is the signed four-byte field `+0x6C`. The two equipment-header pairs used here are creature bytes `+0xAA8/+0xAA9` and `+0x990/+0x991`. They are designated record A and record B in this document. This identifies their original storage without assigning an unproved hand, slot name or item-category label. The producer reads no item pointer to obtain these bytes: they are embedded in the current source creature.

## Construction, refresh and count

The native client constructor `0x059C40` initializes the three four-byte fields at client `+0x800814`, `+0x800818`, `+0x80081C` to zero. They are respectively the vector's begin, logical end and capacity end. Construction therefore starts with an empty vector, not a prepopulated action list. Evidence: `binary/Cube.exe`, RVAs `0x05A210–0x05A224`.

The identified populated-list producer is within frame `0x088EE0`, at `0x08B8AB–0x08BD8A`. On reaching this site, helper `0x04BE20` first resets logical end to begin; it does not release the buffer or change capacity. The frame then appends exactly two equipment/selector entries, followed by three class entries if and only if the class byte is 1, 2, 3 or 4. It subsequently replaces disallowed class entries with zero. The completed count is therefore five for those four classes and two for every other class byte, assuming normal completion and valid source/storage. Zero entries remain elements and do not reduce that count. Evidence: `binary/Cube.exe`, RVAs `0x08B8AB–0x08BD8A`, helper `0x04BE20–0x04BE25`.

Each append copies a complete four-byte binding into the next element and advances logical end by four. The native append helper is `0x26ADD0`; it requests capacity through `0x23DC90` when logical end reaches capacity end. The source arguments at these producer sites are frame-local four-byte values. Count helper `0x087F50` measures the byte extent between logical end and begin in four-byte elements. Exact capacity growth, allocation failures and partial state during an exception are outside the normal-completion count guarantee. Evidence: `binary/Cube.exe`, RVAs `0x26ADD0–0x26AE32`, `0x087F50–0x087F58`; individually identified `decompiled/client/functions/0063dc90.c`.

This is a rebuild on every normal passage through the identified refresh site. There is no equipment-change comparison, class-change comparison, dirty byte or elapsed-time interval around that rebuild. Prior logical entries are discarded even when all selected values will be identical. Capacity reuse is distinct from retaining prior logical contents. Whole-frame invocation, early returns, exceptional exits and external alias writers are not exhaustively closed; the guarantee concerns a frame route that reaches and completes this site with a valid controlled creature.

## The first two entries

Record A's second byte, creature `+0xAA9`, is checked before the general primary selector. These three cases determine **both** entries and bypass selector `0x045020`. Specialization is tested for equality with 1; every other byte takes the alternative column. No test of record A's first byte, class, health, resource, stun or action clock precedes these particular pair choices within the refresh.

| Record A byte `+0xAA9` | Specialization 1: entries 0, 1 | Other specialization: entries 0, 1 |
|---|---|---|
| 10 | 32, 33 | 30, 31 |
| 12 | 42, 43 | 40, 37 |
| 11 | 44, 45 | 38, 46 |

Evidence: `binary/Cube.exe`, RVAs `0x08B8C0–0x08BA12`.

For every other record-A second-byte value, entry 0 is the full four-byte result of selector `0x045020` called on the controlled creature at `0x08BA17`. Although the original decompiled signature describes a byte-like result, the native return paths assign full four-byte integer constants and this caller stores and appends that full result. The effective selector relation on this residual producer route is given below. Conditions are ordered: a later row applies only after all preceding rows failed.

For the timing alternatives, an **early continuation** means that the current action byte is the indicated old action and its signed four-byte clock is strictly less than helper `0x03D1A0`'s returned total boundary plus 200. Equality with that increased boundary takes the ordinary result. The addition and comparison are 32-bit native integer operations; overflowing boundary sums are not given an ordinary duration interpretation. The full total-boundary/stat calculation is an explicit dependency outside this contract.

| Ordered condition for residual entry 0 | Binding result |
|---|---|
| Class byte is 3, or four-byte creature kind is 117 or 86 | Specialization 1 yields 41 during early continuation from action 42, otherwise 42. Other specialization yields 39 during early continuation from action 40, otherwise 40. |
| Creature kind is 104; or record A bytes are first byte 3 and second byte 6 or 7; or record B bytes are first byte 3 and second byte 6 or 7 | 22. |
| Record A second byte is 3 | 18 during early continuation from action 19, otherwise 19. |
| Record A second byte is 4, or record A first byte is zero | 7 during early continuation from action 6, otherwise 6. This row precedes the following second-byte-5 row. |
| Record A second byte is 5 | 13 during early continuation from action 14, otherwise 14. |
| Record A first byte is 3 and second byte is 8 | 26. |
| Equipment predicate `0x044230` is true | 67 during early continuation from action 57; 58 during early continuation from action 67; otherwise 57. |
| Record B first byte is 3 and second byte is 13 | 9 during early continuation from action 10, otherwise 10. |
| Creature byte `+0x7E` has mask `0x10` set | 75. |
| All remaining cases | 2 during early continuation from action 1, otherwise 1. |

Evidence: `binary/Cube.exe`, selector RVAs `0x045020–0x04529C`, caller `0x08BA17–0x08BA2D`; `decompiled/client/functions/00445020.c`.

The selector itself has additional record-A-second-byte-10/11/12 magic-family admission, but this frame's earlier direct pair cases already intercept all such second-byte values. Those internal selector alternatives must not be substituted for the frame's pair table. In particular, the direct record-A-second-byte-12 pair uses entry 0 equal to 40 or 42 without the selector's current-action continuation test. In the residual table's first row, record A cannot have second byte 10, 11 or 12, so the shown 39/40/41/42 relation is the complete reachable result of that selector family for this producer.

Equipment predicate `0x044230` reads only record A's first two bytes. It is true when first byte `+0xAA8` is 3 and second byte `+0xAA9` belongs to `{5, 6, 7, 8, 10, 11, 15, 16, 17, 18}`; otherwise it is false. It does not consult record B, the creature class, action, clock or a resident buff. Its placement in the selector is later than the earlier subtype/empty-record alternatives in the table. Evidence: `binary/Cube.exe`, RVAs `0x044230–0x04426C`.

After appending the residual entry 0, the frame chooses residual **entry 1** independently from the following ordered cases. The record-B first byte is deliberately absent from these tests: a matching record-B second byte alone participates where stated.

| Ordered condition for residual entry 1 | Binding result |
|---|---|
| Record A second byte is 5 | 5. |
| Record A second byte is 3 | 17. |
| Record A second byte is 4 | 20. |
| Either record A or record B second byte is 6 | 25. |
| Either record A or record B second byte is 7 | 24. |
| Record A second byte is 8 | 27. |
| Equipment predicate `0x044230` is true | 59. |
| Record B second byte is 13 | 8. |
| All remaining cases | 63. |

Thus a record-A second byte of 3 wins over a record-B second byte of 6; a second-byte-6 case wins over a second-byte-7 case; and a second-byte-7 case wins over record A's second-byte-8 case. This precedence is part of the result, not an unordered set of equipment capabilities. The frame does not call secondary selector `0x0452A0` to construct this entry. Evidence: `binary/Cube.exe`, RVAs `0x08BA32–0x08BB51`.

## Class entries and zero replacement

The class-dependent suffix is appended after the two preceding entries, regardless of which equipment pair or selector result produced them. Values below are the entries **before** the subsequent zero-replacement pass. Specialization comparisons differ by class and must retain the stated equality cases.

| Creature class byte `+0x140` | Specialization condition | Entries 2, 3, 4 in order |
|---|---|---|
| 3 | Equal to 1 | 34, 103, 49 |
| 3 | Any other value | 88, 103, 49 |
| 4 | Equal to 0 | 48, 79, 97 |
| 4 | Any nonzero value | 48, 79, 96 |
| 2 | Equal to 1 | 21, 50, 100 |
| 2 | Any other value | 21, 50, 99 |
| 1 | Equal to 0 | 54, 86, 102 |
| 1 | Any nonzero value | 54, 86, 101 |
| Any other class byte | Any value | No suffix; the count remains two. |

Evidence: `binary/Cube.exe`, RVAs `0x08BB56–0x08BD39`.

The zero-replacement pass starts at element index 2. For the ordinary five-element result, entry 2 is retained only if controlled-creature signed four-byte `+0x1150` is strictly positive; entry 3 uses `+0x1154`; entry 4 uses `+0x1158`. A value equal to zero or less than zero replaces the respective complete four-byte binding with zero. The original value is not moved to another position, and subsequent entries retain their indices. Entries 0 and 1 are not tested by this pass. A class with no suffix has no entries to mask.

The native pass also contains an unsigned index-plus-four comparison against 10: indices greater than 6 would bypass this masking test. No normally completed output of the closed producer is longer than five, so that extra boundary does not alter any produced entry described here. The rank/skill naming, upstream ownership and mutation policy of the three `+0x1150` fields are not established; their exact positive-versus-nonpositive use is established. Evidence: `binary/Cube.exe`, RVAs `0x08BD3E–0x08BD8A`.

## Bindings 28 and 29

Neither decimal binding 28 nor decimal binding 29 is produced by any completed branch of this identified refresh. They are absent from the direct equipment-pair values, the complete reachable primary-selector results, the residual entry-1 values and all class suffixes. Zero replacement adds only zero. The constructor adds no bindings at all. This negative result covers the closed producers, rather than treating a later consumer case as proof that a producer must exist.

The notation matters: decimal 40 is hexadecimal `0x28`, and decimal 41 is hexadecimal `0x29`; both **are** produced under the conditions above, but neither is decimal action/binding 28 or 29. The native client has later consumers comparing vector entries with decimal 28 and 29 at `0x09BFDD–0x09C012`. This audit does not establish an upstream branch that supplies either of those decimal values. Unidentified indirect/alias writes, other caller-driven modification or exceptional/corrupt state remain unresolved, rather than being invented to satisfy the consumer.

The direct-offset inventory of the individually verified original client assembly found the constructor zeroing, the frame's refresh and consumers, and cleanup references at `0x2E3C95` and `0x2E51F3`. Those two cleanup references pass this vector to helper `0x1FB860`, which releases an allocated buffer; they do not populate it. A direct-offset inventory alone cannot prove that no untracked pointer alias can ever reach the same storage. Evidence: `decompiled/client/assembly.txt`, direct `+0x800814/+0x800818/+0x80081C` references; `binary/Cube.exe`, RVAs `0x2E3C8F–0x2E3C9B`, `0x2E51F0–0x2E51F9`; `decompiled/client/functions/005fb860.c`.

## Frame timing, storage lifetime and limits

The refresh and zero-replacement pass finish before the frame aggregate is constructed at `0x08CE2E` and before its two direct world-step call sites at `0x08CF2E` and `0x08CF49`. The later action-input loop reads this same client vector starting at `0x09B5FE`, visits indices from 1 upward, and checks a zero full-width binding before testing its corresponding input byte. Ordinary-primary fallback is later at `0x09BF70–0x09BF82`. Thus the binding entries are concrete values selected before these world calls, not selector callbacks evaluated afresh when each input byte is tested. A world update can change action clocks or equipment-related state after the refresh without the producer automatically running again at these later consumer sites. Full indirect effects inside world processing are not audited here. Evidence: `binary/Cube.exe`, RVAs `0x08B8AB–0x08BD8A`, `0x08CE2E–0x08CF49`, `0x09B5FE–0x09B678`, `0x09BF70–0x09BF82`; direct call sites in `decompiled/client/assembly.txt`.

The intermediate display/cooldown work after the rebuild reads binding values through the saved vector pointer. For example, `0x08BDF4–0x08BE05` passes an element's address as an exact four-byte key to helper `0x068910` on creature map `+0x139C`. That helper reads the supplied key and can insert a zero-valued map entry when absent; its insertion path copies the key into its own local storage and does not write the caller's binding element. It therefore is not an alternate action-binding producer, even though that display path may mutate the cooldown map. Complete UI refresh and map lifecycle are outside this contract. Evidence: `binary/Cube.exe`, RVAs `0x068910–0x06898F`, `0x08BDDC–0x08BE05`.

The closed producer directly mutates vector storage and its bookkeeping. Its selection predicates have no direct health, resource, stun, cooldown or resident-buff read. Current action and clock influence entry 0 only through the explicitly listed selector continuation cases; they do not delay the rebuild. No random-draw or event-append call appears in the producer, selector `0x045020` or equipment predicate `0x044230`. This is a statement about these audited bodies, not a claim that allocation, the total-boundary stat dependency or other frame callbacks have no side effects.

This contract does not establish operating-system input mappings, item display names, a server producer, complete enclosing frame/pause/device admission, all possible aliases of the vector, full stat/timing-helper mathematics, upstream class or specialization changes, concurrent publication guarantees, allocation/exception behavior, or consumer execution semantics. Valid creature and container pointers and ordinary nonoverflowing timer states are prerequisites for the normal results above. Earlier held artifacts were not opened or altered. The complete new document was read back after saving.
