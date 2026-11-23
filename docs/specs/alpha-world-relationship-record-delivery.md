# Alpha world relationship record delivery

Status: FINAL / STABLE / HELD. This is a bounded original-evidence contract for the 24-byte records emitted into output aggregate member +0x50 by the Alpha client world routine at RVA 0x1A0970. It covers peer publication, the original server's outgoing packet, the original client's receive publication and the later client-frame relationship-map update. It does not specify a replacement implementation.

## Evidence and image identity

The evidence root is identified relatively as `disassembled alpha cube world`. All file references below are relative to that root. Native addresses are RVAs within the explicitly named image; both supported images have preferred image base 0x00400000.

| Original image | File size | SHA-256 |
| --- | ---: | --- |
| `binary/Cube.exe` | 3,885,568 bytes | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| `binary/Server.exe` | 1,718,784 bytes | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

The basis of this document is the native instructions of the two images, read through `decompiled/client/assembly.txt`, `decompiled/server/assembly.txt`, the individually named per-routine files below and, where the client listing has no decoded instructions (the kind-4 receive route), a direct disassembly of `binary/Cube.exe`. The client and server addresses are not interchangeable. The separately verified server sender closes the packet format; the client host publisher alone does not establish a complete socket-sending path. No reconstruction, runtime capture, earlier project contract or replacement implementation was used to establish any statement here.

## Record meaning and raw extent

The semantic direction is defined by the map being described. The first identifier selects the creature whose relationship map is to change. The second identifier is the key within that map. Neither identifier is a socket handle, a peer-vector index or a pointer.

| Payload offset | Width and representation | Established meaning |
| --- | --- | --- |
| +0x00 | 8 bytes, raw 64-bit creature identifier | Creature whose map is described |
| +0x08 | 8 bytes, raw 64-bit creature identifier | Relationship-map key |
| +0x10 | 4 bytes, IEEE binary32 | Value assigned to that map entry |
| +0x14 through +0x17 | 4 opaque bytes | No semantic use established in the inspected producer, transport or consumer |

The complete payload is 0x18 bytes. Its list-node links are separate from the payload and are not serialized. The final four bytes travel with the record even though only the preceding twenty bytes have an established meaning. No alignment bytes are inserted between successive serialized records.

The client routine at 0x1A0970 takes three explicit arguments: a first creature S, a second creature T and the output aggregate. Its first record carries T's identifier (read from T+8) at +0x00, S's identifier (read from S+8) at +0x08, and at +0x10 the current binary32 entry of T's relationship map at T+0x13A4 keyed by S's identifier. Reading that entry can insert an absent key with a zero value. This first record is appended to the aggregate's +0x50 list before any other creature is visited.

The routine then visits the creatures of a collection supplied as its implicit object. A visited creature C is skipped when its pointer is null or equals S or T. For the remaining creatures an upstream relationship predicate (client RVA 0x196CA0) and a squared-distance threshold must both pass; neither is closed here. If both pass and C's own map entry keyed by S's identifier compares equal to zero (an entry inserted by that lookup also qualifies; an unordered comparison does not), the routine assigns binary32 0.5 to that entry and appends a further record to the same +0x50 list: C's identifier, S's identifier and the entry value read back from C's map. Additional records therefore follow the first record in visiting order.

Both construction sites assign the two identifiers and the value but leave the trailing four bytes of their on-stack payloads unwritten. Their content is therefore not specified as zero and is not a fourth semantic field; the native list copies preserve them. The first construction site does not reject equal or zero identifiers; the receiver's later identity checks are a separate boundary. This does not make null creature arguments acceptable at the producer.

Evidence: client 0x1A098F–0x1A0A08 (first record) and 0x1A0A20–0x1A0B7A (visiting and later records) in `binary/Cube.exe`; `decompiled/client/functions/005a0970.c`; map access at client RVA 0x068AD0 (`00468ad0.c`); list append at client RVA 0x086150 (`00486150.c`); 24-byte node copy at client RVA 0x051D00 (`00451d00.c`), which copies the payload as three consecutive eight-byte portions, opaque bytes included.

## Publication into peer queues

The client host publisher at RVA 0x0DC8E0 operates on a host object whose +4 member supplies the world. Its peer vector spans the pointers stored at host+0x28 and host+0x2C, with four-byte peer pointers as elements. Peers are visited in ascending vector position, and a null peer pointer is skipped. For each peer reaching the relationship publication site, the complete forward list at output aggregate+0x50 is appended to the list at peer+0x68. Records already in that destination remain before the new records, and the source records keep their forward order.

There is no relationship-record identifier, amount, ownership, distance or designated-creature test at this copy site. Other aggregate members are handled with their own work and predicates in the same peer iteration. Earlier work in that iteration resolves and uses recipient state; this audit does not make a malformed or missing recipient safe. Apart from bounds-error exits, no ordinary branch between the null-peer test and the relationship copy skips the copy for a nonnull peer; the guarantee stated is the absence of any per-record filter once the site is reached.

The publisher enters the world critical section through client RVA 0x201CB0 at 0x0DC916–0x0DC919, using the world from host+4; that routine enters the critical section embedded at world+0x8000C0, and the publisher leaves it after the peer loop. The relationship append is at client 0x0DCDB2–0x0DCDD2, between the publication of aggregate+0x48 to peer+0x60 and of aggregate+0x58 to peer+0x70. Its range-copy routine at 0x0DC5B0 copies each payload through 0x051D00 without changing field contents or order.

The separately inspected original server publisher at RVA 0x0272D0 has the same relevant relationship: aggregate+0x50 is appended to peer+0x68 for each occupied peer-vector position, at server 0x0277A2–0x0277C2, through the 24-byte range-copy routine 0x021160 and its node copy 0x0203C0. It also uses the world at host+4 and the critical section at world+0x8000C0 (server 0x0D3DF0). This establishes the server-side queue producer used by the sender below without assuming that all surrounding client and server behavior is identical.

Evidence: `decompiled/client/functions/004dc8e0.c`, `004dc5b0.c`, `00601cb0.c`; `decompiled/server/functions/004272d0.c`, `00421160.c`, `004203c0.c`, `004d3df0.c`; native sites above. Ordinary list appends have the native 0x07FFFFFE element-count limit and take a length-error exit at that limit. Allocation and exception recovery are outside the successful-publication guarantee.

## Original server send boundary and wire location

The outgoing worker at server RVA 0x023DD0 serves one peer slot. Its context supplies a world pointer (+0x00), a pointer to the peer-vector storage (+0x04), a four-byte slot index (+0x08), the recipient's eight-byte identifier (+0x10) and a pointer to a running byte (+0x18). Each loop pass requires the running byte to be nonzero and the selected peer slot to be nonnull. Earlier preparation in the pass uses recipient and world state; complete connection validity and all of its failure paths are outside this audit. No per-relationship-record identity filter occurs when this worker copies, serializes or sends the queue.

Within one pass the worker sends, in order: a message-kind-0 stage (one kind-0 message per element of an earlier-prepared collection, each followed by a compressed length and compressed bytes), a bare four-byte kind-2 value at server 0x0247F4, then the world-event stage described next. If any kind-0 length or payload send returns -1, the worker records a failure flag and continues the pass.

At server 0x0248DB–0x024911 the worker enters the world critical section and makes a private copy of the peer's output aggregate beginning at peer+0x18. The aggregate copy routine at server RVA 0x023400 copies member +0x50 through 0x022380, so the relationship source is peer+0x68; the private copy holds the records in source order with all 24 payload bytes. While still holding the world lock the worker empties the peer's relationship queue at 0x024B18–0x024B4D, as part of clearing all consumed peer outputs, and releases the world lock at 0x024C7B. This happens before any part of the world-event packet is sent. The worker also releases a separate peer-associated critical section at peer+0x98 (server 0x024927) during this region; where that section is acquired and what it protects are not closed here, and it is not a substitute for the verified world lock.

A record can therefore have left the live peer queue while it exists only in the worker's private copy and before any successful network delivery. After the lock release the worker sends a kind-5 message followed by two four-byte values captured from the world (world+0x800160, then world+0x80015C) while the lock was held, and then the four-byte kind 4. Each of these sends, like the later kind-4 length and payload sends, treats a return of -1 as failure and leaves the send loop without requeuing. If the kind-0 failure flag was set, the worker serializes part of the aggregate, then discards the serialized buffer without sending the kind-4 length or payload and leaves its send loop; the cleared relationship records of that pass are never sent. The inspected send sites do not establish a retry policy for a positive short write.

The world-event packet is a four-byte message kind equal to 4, then a four-byte compressed byte count, then that many compressed bytes. These are native little-endian fields in the supported images, with no byte-order conversion at these sites. The kind is sent at server 0x024D22, the count at 0x024FDD and the compressed bytes at 0x025011. Compression of the serialized aggregate happens at 0x024FAC through server RVA 0x012C70 before the length is read.

Within the decompressed aggregate, relationship records are a counted list without a fixed absolute offset from the packet start. The serializer visits aggregate members in this order: +0x18, +0x00, +0x10, +0x08, +0x20, +0x30, +0x28, +0x38, +0x40, +0x48, +0x50, +0x58, +0x60. The +0x28 and +0x38 portions contain variable counted content. The relationship list at +0x50 therefore follows the complete +0x48 list and precedes the complete +0x58 list, regardless of earlier lengths.

At server 0x024F78, list serializer 0x021C60 writes the relationship list as its four-byte element count followed by exactly 24 bytes per element in forward list order: both identifiers, the binary32 value and the opaque trailing bytes. There is no per-element tag, field conversion, target lookup, equality test, sorting or duplicate coalescing. An empty list contributes a four-byte zero count and no records. The matching client decoder treats the count as signed 32-bit when deciding whether to iterate. The same serializer also writes members +0x08 and +0x48.

The kind-0, kind-2 and kind-5 stages precede the kind-4 packet in a successful pass. This is ordering within this worker, not a guarantee that every identifier in a later relationship record will resolve on the receiving client. Kind-4 records are world-event aggregate content and are not fields of any kind-0 message.

Evidence: `decompiled/server/functions/00423dd0.c` with the native sites above in `decompiled/server/assembly.txt` and `binary/Server.exe`; `00423400.c`, `00422380.c`, `00421c60.c`, `00412c70.c`, `00412d00.c`, `004d3df0.c`, `004d5c60.c`; the import table of `binary/Server.exe` for the peer+0x98 release. The compression routine reaches the bundled compression library import; compressor internals, compressed-byte reproducibility and compression failure are not specified here.

## Client receive decoding and guarded publication

The client receive worker at RVA 0x06B740 reads four-byte message kinds from the socket stored at client+0x8006CC while the running byte at client+0x800585 is nonzero and its local stop flag is clear. Kinds below 16 are dispatched through the index table at client 0x06D1A4 and the address table at 0x06D180: kind 4 goes to 0x06CC0E, and kind 0 goes to a separate destination at 0x06BAAD that is not this decoder.

The kind-4 route reads the four-byte compressed length, reads that many bytes, decompresses them through client RVA 0x0494B0 at 0x06CCC8 and decodes a temporary aggregate. Its member order matches the sender order above. The relationship list is decoded into temporary aggregate+0x50 by client RVA 0x057F70 at 0x06CEB2; the neighboring calls decode +0x48 (by the same routine, at 0x06CE9F) and +0x58 (at 0x06CEC5). Socket reads, decompression and the temporary-list allocations all happen before the world lock used for publication.

For a complete list, decoder 0x057F70 reads the four-byte count and appends each complete 24-byte payload to the temporary list in wire order. Its signed greater-than-zero test means zero and negative count bit patterns do not enter its record loop. There is no creature lookup, identifier normalization, self-identifier rejection, value clamp or duplicate removal while decoding, and the native copies preserve all payload bytes.

After the whole temporary aggregate is decoded, the worker enters the embedded world's critical section at client 0x06CEE8. The world object is at client+0x2E4, so the lock entered by 0x201CB0 is at client+0x8003A4. At 0x06D023–0x06D03D the temporary relationship list is appended to the queue at client+0x800680 through client RVA 0x0579C0 and the 24-byte range-copy path; existing queue entries stay ahead of the new entries. The worker releases this lock at 0x06D088 before destroying the temporary aggregate at 0x06D093. Publication does not change any creature's relationship map; it only makes the received list available to the later frame consumer.

Short or malformed data is outside the complete-record contract. The decoder's insufficient-record branch moves the cursor to the buffer end but still appends a payload that was not freshly read. Its insufficient-count branch does not produce a normal decoded count. While reading the compressed length, a receive call returning zero ends that read loop and the route continues with a partially filled length. This audit therefore does not specify clean rejection, zero filling, a valid prefix-only result or safe continuation for truncated data. A negative count is not a declared encoding of an empty list merely because the loop is skipped. Allocation limits, decompression errors, oversized counts and connection shutdown need further dependencies before a total malformed-input contract could be stated.

Evidence: client native 0x06B740–0x06B860 and 0x06CC0E–0x06D0EC, and the table bytes at 0x06D180–0x06D1B3, in `binary/Cube.exe`; `decompiled/client/functions/0046b740.c`, `00457f70.c`, `004579c0.c`, `004dc5b0.c`, `00451d00.c`, `004494b0.c`, `00449540.c`, `00601cb0.c`. The decompiled worker body is incomplete for the dispatched routes; the original native instructions were used for them, and no absent body was filled from elsewhere.

## Later frame map assignment and identity edge cases

When client frame RVA 0x088EE0 reaches 0x08FBB7–0x08FCB8, it skips the step for an empty queue and otherwise walks client+0x800680 in forward order. A record's first identifier must resolve to a nonnull creature in the frame's embedded world (client+0x2E4). Its second identifier must separately resolve to a nonnull creature in the same world. Both lookups use client RVA 0x02F000, which returns null for an absent key and also for a present key whose stored creature pointer is null.

Only after both lookups succeed does the consumer compare the two complete 64-bit identifiers; equal identifiers are ignored. For an accepted record it reads the binary32 value at +0x10, looks up the first identifier again, obtains that creature's map at +0x13A4 keyed by the second identifier through 0x068AD0 and assigns the value. The repeated first lookup is a real third lookup, not reuse of the earlier result.

Only the first creature's +0x13A4 map is written by this consumer. The map access can insert a previously absent key before the assignment. The consumer does not create a missing creature, update the second creature's map, create a reciprocal relation, touch the neighboring member at +0x13AC or apply a hit. The value is assigned directly, with no addition, interpolation, floating-point comparison or clamp; its four bytes pass through a scalar binary32 load and store.

| Ordinary identity or value case | Established receiver result |
| --- | --- |
| First identifier absent, or stored with null creature pointer | Record skipped before the second lookup |
| First identifier valid, second absent or stored null | Record skipped before the equality test and map write |
| Both identifiers resolve and are equal | Record skipped, including an equal nonzero identifier |
| Identifier zero | Used as an ordinary lookup key; there is no separate zero sentinel test here |
| Both identifiers zero | A missing or null lookup still rejects; if both resolve, equality rejects |
| Distinct identifiers resolve to the same creature pointer | Not rejected by the identifier comparison; the first creature's map can be assigned under the second identifier |
| Distinct valid identifiers, value 0 or an ordinary negative value | Value assigned without any sign or positivity gate |
| Several accepted records for the same ordered identifier pair | Each assignment occurs in queue order; the last one wins |

Neither record admission nor the map target depends on a comparison with a designated or controlled creature. This consumer has no health, class, owner remapping, relationship-predicate, world-role, local-player or source-authority check. A designated creature and any other creature follow the same rule when supplied as the first identifier. This describes this receiver only; it is not a claim that arbitrary peers are authorized upstream to create world events.

The first lookup, second lookup and equality test are at client 0x08FC22–0x08FC4D. The value read, repeated first lookup, map access and assignment are at 0x08FC4F–0x08FC76. Evidence: native `binary/Cube.exe`; `decompiled/client/functions/0042f000.c`, `00468ad0.c`, and the forward-iteration routines at client RVA 0x06F440, 0x068D30, 0x02C6C0, 0x20A9F0 and the empty test at 0x077230. The relationship map keys on the whole eight-byte identifier; its ordering compares the upper word as signed 32-bit and, when upper words tie, the lower word as unsigned 32-bit. This ordering detail does not narrow record equality to either word alone.

## Frame timing, queue consumption and remaining lifetime limits

The frame enters the embedded-world critical section at client 0x08902F and releases it at 0x09D2EA; no other release of that lock occurs in the frame between those sites. It is the same world+0x8000C0 lock used by receive publication. The separate frame lock operations at client 0x08D55B and 0x08D5EB use an object based at client+0x800D44 and are a different lock. Successful receive publication and this frame's map assignments are therefore serialized by the verified world lock, while socket reception and temporary decoding can proceed outside it.

The frame calls the embedded world's large update routine at client RVA 0x20C510 from a repeated site at 0x08CF2E (argument 20 per call) and once more at 0x08CF49 with the remainder; both calls precede the relationship consumer at 0x08FBB7. The relationship queue is cleared later at 0x090630–0x090636 through client RVA 0x06F870, which resets the list links and count and frees the old nodes. Records skipped for a missing, null or equal identifier are not kept for a later retry. The walk itself does not selectively remove or defer records.

This consumption and clearing precede later frame work at 0x09B58E and the outgoing host publication: when client+0x8006C8 is nonnull, the host publisher is called at 0x09C26D (test at 0x09C256) with the frame's output aggregate. The proved ordering is: the update-routine calls, then incoming relationship application and queue clearing, then later frame work and outgoing host publication. Incoming map assignment is not inside either update-routine call, and a packet arriving during a particular wall-clock interval is not guaranteed to be consumed in the same frame.

The producer may already have changed local relationship state before or while producing its outgoing record; receipt is still a direct assignment at a later client-frame boundary. No local or designated-creature shortcut was found in this record consumer.

Evidence: client frame native sites 0x08902F, 0x08CF2E, 0x08CF49, 0x08D55B, 0x08D5EB, 0x08FBB7–0x08FCB8, 0x090630–0x090636, 0x09B58E, 0x09C256–0x09C26D and 0x09D2EA in `decompiled/client/assembly.txt` and `binary/Cube.exe`; `decompiled/client/functions/0046f870.c`, `00601cb0.c`, `004dc8e0.c`.

## Unresolved

- The upstream relationship predicate at client RVA 0x196CA0, the distance threshold constant and the creature collection visited by the producer routine are not closed, so which creatures receive the later 0.5 records is not specified.
- Callers of the producer routine 0x1A0970 and the conditions under which it runs are not established.
- The content of the trailing four payload bytes is whatever the producer's stack held; it is not specified.
- The meaning of the kind-0, kind-2 and kind-5 messages, and of the two kind-5 values, is not established here beyond their order and widths.
- The acquisition and purpose of the peer+0x98 critical section released by the server sender are not closed.
- Characterizing client RVA 0x20C510 as the world's step routine rests on its call shape only, and the nature of the frame work at 0x09B58E is not established.
- Whether a particular host participant is represented by a socket peer, receives its own published record or only observes the producer's direct local state is not established by the publisher and consumer sites.
- Not covered: every caller gate of the frame, first construction of each aggregate and queue, map construction and destruction, creature removal and identifier reuse, lifetime or decay of relationship entries, all local producer mutations, reconnection replay, every other publisher or sender counterpart, fatal processing, and behavior after allocation, list-length, decompression or socket failure.
- No timer or expiry field is among the three established fields; that absence does not prove relationship state never expires elsewhere.
- No random draw occurs at the identified copy, serialization, decode, publication and assignment sites. Randomness in upstream relationship production or in neighboring aggregate members is outside this statement.

## Re-certification (2026-11-22)

A fresh source-only author re-checked every statement in this document against the original `binary/Cube.exe` and `binary/Server.exe` images (identities re-hashed and matching), using the native listings, the named per-routine files and direct disassembly of the client receive route. Record layout and widths, the publisher and serializer sites, member order, the client decode, publication and frame-consumer sites, and the lock identities were confirmed. Corrections: the kind-5 stage is no longer labeled a clock stage; the omitted kind-2 send is now listed; the server's kind-0 failure path, which clears the relationship queue and then drops the kind-4 payload unsent, is now stated; the producer's later-record conditions (exclusion of null, S and T, the zero-entry precondition and the shared +0x50 list) are now stated; the zero-return length read is added to the malformed-input limits; and the "world-step" and "action-input" labels were moved to the unresolved list. Decompiler-style wording was rewritten as plain prose. Everything listed under Unresolved remains open.
