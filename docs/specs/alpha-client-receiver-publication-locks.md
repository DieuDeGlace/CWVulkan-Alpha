# Alpha client receiver publication locks

Status: FINAL / STABLE / HELD. This is a bounded original-client synchronization contract. It describes ordinary completed-message paths for incoming kinds 0, 2, 4, 5, 10 and 15, their direct publication intervals, and the identity of the lock shared with the client frame and sender snapshot. It does not establish a server contract, a complete wire format, or synchronization for every user of these objects.

The evidence root is identified relatively as `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, with preferred image base `0x400000`. Every address described as an RVA is relative to that image. Object offsets below are byte offsets; native pointers are 32 bits. No reconstruction, replacement implementation, runtime capture, or earlier contract was used.

Evidence citations below are relative to that root. Native bytes take precedence over incomplete function exports. In particular, `decompiled/client/functions/0046b740.c` does not expand the complete receive dispatch, so its abbreviated body does not delimit the worker's actual message handling.

## Lock identity and dispatch identity

The receiver's recurring lock argument is the embedded world at client offset `+0x2E4`. The enter helper at RVA `0x201CB0` and leave helper at `0x201E90` operate on a critical section at their argument's `+0x8000C0`. These calls therefore address the concrete critical section at client `+0x8003A4`.

The client frame at RVA `0x088EE0` passes client `+0x2E4` at its acquisition at `0x08902F` and release at `0x09D2EA`. The sender at RVA `0x069C10` passes that same embedded-world address at acquisition `0x069CEA` and release `0x069F26`. Its outgoing-list snapshot/consumption work and controlled-creature identifier/state copy occur between those calls. Thus these receiver publication intervals, that frame interval and that sender snapshot use the same lock object. This identifies a common exclusion mechanism; it does not prove that every access to the associated data uses it.

A distinct critical section is directly addressed at client `+0x8005D0`. The kind-15 transition helper acquires it after acquiring the embedded-world lock, then releases it before releasing the world lock. The frame also addresses this second object, including `0x08B712` or `0x08B7BF` through `0x08B7FC`, and `0x09CEE5` through `0x09D2DE`. The sender separately uses it at `0x06A21B`–`0x06A253` for the lists at client `+0x2CC` and `+0x2D4`, after its world-snapshot interval has ended. Its wider ownership purpose is not established here; it must not be renamed a dedicated chat or clock lock.

Not every use of the enter/leave helpers identifies the first embedded world. For example, frame calls at `0x08D55B` and `0x08D5EB` pass client `+0x800D44`, which addresses a different critical section at client `+0x1000E04`. Those calls do not release or reacquire the critical section at client `+0x8003A4`. The subsidiary object's behavior is outside this contract.

Sources: `decompiled/client/functions/00601cb0.c`; `decompiled/client/functions/00601e90.c`; `decompiled/client/assembly.txt`, frame RVA `0x088EE0` and sender RVA `0x069C10`; `binary/Cube.exe`, RVAs `0x089020`–`0x089034`, `0x09D2D7`–`0x09D2EA`, `0x069CDB`–`0x069CFD`, `0x069EF6`–`0x069F26`, and `0x06A213`–`0x06A253`.

The receive worker begins at RVA `0x06B740`. Its four-byte message kind is dispatched using the byte-index table at image RVA `0x06D1A4` and target-address table at `0x06D180`. The native unsigned range check admits table indices for kinds through 15. The requested mapping is:

| Incoming kind | Native handler RVA | Principal publication in this scope |
| --- | --- | --- |
| 0 | `0x06BAAD` | Creature lookup/possible insertion, then later noncontrolled state assignment |
| 2 | `0x06BE25` | Creature-presence reconciliation; an earlier special case also publishes a timing difference |
| 4 | `0x06CC0E` | Appends decoded aggregate lists to 13 client queues |
| 5 | `0x06CB42` | Replaces two four-byte clock-related fields |
| 10 | `0x06CA1E` | Appends a text/message object to a client queue |
| 15 | `0x06B8BE` | Conditionally calls a world/selection transition helper |

This table is recovered from the original table bytes and native dispatch at `0x06B8A7`–`0x06B8B7`, not inferred from handler order or decompiler case labels. Other kinds are not characterized.

## Reads and temporary decoding

The worker tests its client activity byte at `+0x800585` and maintains a receiver-local stop/failure flag. It receives a four-byte kind using the socket at client `+0x8006CC`; partial header reads are accumulated. The normal header-read interval at `0x06B810`–`0x06B85A` precedes the handler's publication lock acquisition.

The payload helper at RVA `0x086B60` repeatedly receives into a caller-owned buffer until a positive requested byte count has been filled. A receive result below one returns immediately; a nonpositive requested size is returned without that accumulation loop. The helper itself does not acquire the critical sections identified above.

Kinds 0 and 4 receive their length and encoded bytes, and call the local decompression helper at RVA `0x0494B0`, before their publication intervals. That helper invokes `0x049540`, accumulates decoded bytes in temporary storage and replaces the local buffer. The inspected decompression helpers contain no acquisition of these world/client critical sections. Kind 4 also constructs and fills its entire local aggregate before acquiring the world lock. Kind 0 has an additional unlocked state-decoding interval between two world-lock intervals.

These facts distinguish potentially blocking socket input and temporary decoding from guarded publication. They do not establish that all transitively called allocation, stream, library or object methods are free of other internal locks. Short reads, corrupt compressed streams, allocation failure, exceptions and every stop/cleanup route are outside the normal completed-message claims below; no rollback or exception-safe publication guarantee is supplied.

Sources: `binary/Cube.exe`, receiver RVAs `0x06B810`–`0x06B85A`, `0x06BAEB`–`0x06BBB0`, `0x06CC0E`–`0x06CEE8`; `decompiled/client/functions/00486b60.c`; `decompiled/client/functions/004494b0.c`; `decompiled/client/functions/00449540.c`.

## Kind 0: two publication intervals

After reading and decompressing this message, the receiver extracts an eight-byte creature identifier into local storage. It then acquires the world lock at `0x06BBDC` and resolves that identifier. A missing creature causes allocation and construction of a native creature, followed by insertion in the world creature tree rooted at client `+0x2E8`. That insertion occurs before the incoming state has been decoded into the temporary state object.

Still in the first interval, the receiver copies the selected creature's existing state, beginning at creature `+0x10`, into a separate local state object. It releases the world lock at `0x06BC6F`. The state decoder at RVA `0x0CCFA0` then modifies that local copy outside the world lock. The receiver also records the identifier in its own accumulated presence collection outside the world lock. This collection is later consulted by kind 2.

The receiver reacquires the world lock at `0x06BCBE`. It compares the retained creature pointer with the controlled-creature pointer at client `+0x8006D0`. Equality skips the noncontrolled-creature state update in this interval. Inequality admits the interpolation/preparation changes and the assignment of decoded state to the actual creature's state at `+0x10`; that assignment calls RVA `0x04B040` at `0x06BDFF`. The receiver releases the lock at `0x06BE0C` and subsequently destroys local buffer storage.

The first insertion/baseline copy and the later state assignment are separate guarded publications. A whole kind-0 message must not be described as one indivisible world transaction. In particular, a newly inserted creature exists in the world before unlocked decoding finishes, and a decoder failure can occur after that insertion. The native caller retains a raw creature pointer across the unlocked interval; this audit does not establish the lifetime rule that makes that retention safe, or a rollback policy on decoding failure. The exact interpolation and complete snapshot-field contracts are outside this synchronization scope.

Source: `binary/Cube.exe`, RVAs `0x06BBBC`–`0x06BE0C`, especially resolve `0x06BBF0`, insertion `0x06BC44`, baseline copy `0x06BC62`, unlocked decoding `0x06BC82`, presence registration `0x06BCB1`, controlled comparison `0x06BCC5`, and state assignment `0x06BDFF`.

## Kind 2: timing publication and presence reconciliation

Kind 2 has a special operation before the dispatch table. The worker samples the native millisecond timer before acquiring the world lock at `0x06B875`. If its preceding receiver-local sample is nonzero, it stores the 32-bit current-minus-previous difference at image RVA `0x36B048` while holding that lock. A previous sample equal to zero suppresses that store. The world lock is released at `0x06B896`; only afterward does the worker replace its local previous sample with the already captured current sample. The subtraction is a four-byte native operation; no saturation or wrap correction is visible here. This establishes a timing difference between these received messages, not a complete round-trip-latency interpretation.

The actual kind-2 handler then allocates temporary removal storage and acquires the world lock at `0x06BE52`. It traverses the world creature tree at client `+0x2E8`. A nonnull creature pointer unequal to client `+0x8006D0` is a removal candidate when its tree entry's eight-byte identifier is absent from the receiver's accumulated presence collection. The receiver first collects qualifying entries, then destroys their nonnull creature objects and removes their world tree entries, all within this same guarded interval. The controlled creature is excluded by pointer equality.

The world lock is released at `0x06BF0D`. Clearing the receiver-private presence collection and destroying temporary removal storage occur afterward. No additional socket payload is read by this dispatch body. Thus this message has two distinct world-lock intervals: the earlier timing publication and the later membership reconciliation. The kind-0 registration/kind-2 reconciliation relationship is directly visible; broader server batching, identifier reuse and other producers of the presence collection remain unclosed.

Source: `binary/Cube.exe`, RVAs `0x06B860`–`0x06B8A1` and `0x06BE25`–`0x06BFA6`, especially removal calls `0x06BEE6` and `0x06BEFA`.

## Kind 4: decoded aggregate publication to queues

This handler receives its four-byte encoded length and encoded body, decompresses the local buffer at `0x06CCC8`, constructs a local aggregate at `0x06CCD3`, and decodes its constituent lists before acquiring the world lock at `0x06CEE8`. Its guarded work appends copies of the decoded ranges to persistent client queues in the following order. Aggregate offsets identify the temporary aggregate; client offsets identify destination queue objects.

| Publication order | Temporary aggregate offset | Client destination offset | Append call RVA |
| --- | --- | --- | --- |
| 1 | `+0x18` | `+0x800648` | `0x06CF07` |
| 2 | `+0x00` | `+0x800630` | `0x06CF26` |
| 3 | `+0x10` | `+0x800640` | `0x06CF45` |
| 4 | `+0x08` | `+0x800638` | `0x06CF64` |
| 5 | `+0x20` | `+0x800650` | `0x06CF83` |
| 6 | `+0x28` | `+0x800658` | `0x06CFA2` |
| 7 | `+0x30` | `+0x800660` | `0x06CFC1` |
| 8 | `+0x38` | `+0x800668` | `0x06CFE0` |
| 9 | `+0x40` | `+0x800670` | `0x06CFFF` |
| 10 | `+0x48` | `+0x800678` | `0x06D01E` |
| 11 | `+0x50` | `+0x800680` | `0x06D03D` |
| 12 | `+0x58` | `+0x800688` | `0x06D05C` |
| 13 | `+0x60` | `+0x800690` | `0x06D07B` |

These are tail insertions into existing queues rather than replacement of their existing contents or immediate application to creature health/state. The second destination is the incoming hit-record queue; its decoder admits positive record counts and reads records of 72 bytes. The remaining queue payload meanings and complete serialization dependencies are not assigned names here.

The lock remains held across this ordered group of append calls and is released at `0x06D088`. The temporary aggregate is destroyed at `0x06D093`, and buffer cleanup follows after unlock. For accesses that obey this same world lock, the ordinary completed group is protected together. This does not establish exception rollback, all-or-nothing allocation behavior, or protection against an unguarded observer. Message decoding and aggregate destruction are not part of this guarded publication interval.

Sources: `binary/Cube.exe`, RVAs `0x06CC0E`–`0x06D093`, including the aggregate hit-list decode call at `0x06CCFD` and publication `0x06CEE8`–`0x06D088`; native hit-list decoder RVA `0x0583F0`.

## Kind 5: two clock-related words

The receiver reads two consecutive four-byte values before acquiring the world lock at `0x06CBB9`. Within the lock it stores the first received word at client `+0x800444`, then the second at client `+0x800440`. These are embedded-world offsets `+0x800160` and `+0x80015C`, respectively. It releases the lock at `0x06CBE0`.

The copy does not establish signed arithmetic, units, validity ranges or a clamp. There is no separate clock critical-section acquisition in this inspected publication path: these two field writes use the common embedded-world lock. The earlier kind-2 timing word is a separate operation at a global image address, although that store also uses the same world lock.

Source: `binary/Cube.exe`, RVAs `0x06CB42`–`0x06CBE0`, especially stores at `0x06CBCC` and `0x06CBD2`.

## Kind 10: text/message queue

The receiver constructs a temporary message, reads an eight-byte identifier, then reads a four-byte length. The length admission uses signed comparison: zero and negative values skip ordinary queue publication. A positive length resizes a native wide-character string and requests twice that length in payload bytes. These reads and string preparation precede the publication lock. This establishes two-byte character storage in this route; identifier meaning and exceptional length/overflow behavior remain outside scope.

After the positive-length payload read, the receiver acquires the world lock at `0x06CB0E`, appends the temporary message to the queue object at client `+0x1000E58` through RVA `0x0861F0`, and releases the lock at `0x06CB2C`. The inspected append wrapper allocates/copies the node and updates its list membership/count; it introduces no separate chat critical section. Temporary message/string destruction follows unlock. The queue's distant numeric location in the client does not change which lock the receiver actually acquired.

There is no separate chat-lock acquisition in this inspected receiver path. This observation does not classify every helper or every text-related access elsewhere in the client, nor does it identify client `+0x8005D0` as a chat-specific lock.

Sources: `binary/Cube.exe`, RVAs `0x06CA1E`–`0x06CB38`, especially positive-length admission `0x06CAC5`–`0x06CACD`, payload call `0x06CAF9`, and append `0x06CB1F`; `decompiled/client/functions/004861f0.c`.

## Kind 15: transition helper and surrounding unguarded work

This handler reads a four-byte value and derives a local comparison string through local stream processing before its transition helper takes either lock. It compares the received value with client `+0x800A50`; if those values match, it also compares the string with the native string object at client `+0x800A54`. Equality of both comparisons bypasses the transition and its subsequent UI-property writes. A difference in either comparison invokes the helper at RVA `0x06F620`. These initial client comparisons are outside that helper's lock intervals, so this is not an established atomic compare-and-transition operation.

The helper first acquires the embedded-world lock at `0x06F65E`, then the distinct critical section at client `+0x8005D0` at `0x06F66D`. Under both locks it updates the numeric value at offset `+0x24` and native string at `+0x28` of the object reached through controlled creature `+0x1D28`. It also replaces client `+0x800A50` and the string object at client `+0x800A54` with the selected values. The numeric stores are four bytes; strings retain their native dynamic-object representation rather than being fixed-width scalar stores.

Still holding both locks, the helper collects world creature-tree entries whose creature pointer differs from the controlled-creature pointer. It then destroys each collected nonnull creature and removes its entry. Unlike the kind-2 collection admission, this comparison does not first exclude null creature pointers; the destruction step itself checks for null. No receiver-private presence-collection test is used by this transition helper.

The helper releases client `+0x8005D0` at `0x06F800`, then the world lock at `0x06F809`; temporary removal-storage cleanup follows both releases. This is the verified nested acquisition/release order for this route, not a universal lock hierarchy for every caller.

After the helper returns, the receive handler writes four-byte UI-property values through the objects rooted at client `+0x800884`, `+0x800880`, `+0x800888` and `+0x80088C`, in that order: first 1, then three zeros. These caller-side writes at `0x06BA23`, `0x06BA3E`, `0x06BA59` and `0x06BA74` occur after both helper locks have been released. Their user-interface meanings and any protection internal to other users of those objects are not established here. Consequently, the entire kind-15 handler must not be characterized as guarded by either one of these locks.

Sources: `binary/Cube.exe`, RVAs `0x06B8BE`–`0x06BA7B` and `0x06F653`–`0x06F80E`; `decompiled/client/functions/004514b0.c`; `decompiled/client/functions/0046f620.c`. Native bytes resolve the helper's actual lock argument and continuation where the function export misidentifies arguments or truncates paths after deletion calls.

## Limits of the synchronization conclusion

This audit establishes concrete lock identity and normal-path publication placement, including separate intervals within one message and caller-side work outside a helper's locks. The common world lock permits exclusion between the inspected receive publications, frame interval and sender snapshot when those paths acquire it. It does not serialize socket reads or temporary decompression against the frame, and it does not make an entire received packet a single world mutation.

Unresolved dependencies include complete worker shutdown/error publication, exception unwinding and lock release, creature lifetime across kind 0's unlocked decoding interval, all accesses to the global timing word, all queue payload formats and deep-copy helpers, the wider role of client `+0x8005D0`, subsidiary frame-lock object ownership, and all server-side receiver behavior. The native evidence here does not certify absence of data races, freedom from deadlock, thread affinity, lock-free reads, or cross-client/global synchronization.
