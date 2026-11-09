# Alpha world relationship record delivery

Status: FINAL / STABLE / HELD. This is a bounded original-evidence contract for the 24-byte records emitted into output aggregate member +0x50 by the Alpha client world helper at RVA 0x1A0970. It covers peer publication, the original server's outgoing packet, the original client's receive publication and the later client-frame relationship-map update. It does not specify a replacement implementation.

## Evidence and image identity

The evidence root is identified relatively as `disassembled alpha cube world`. All file references below are relative to that root. Native addresses are RVAs within the explicitly named image; both supported images have preferred image base 0x00400000.

| Original image | File size | SHA-256 |
| --- | ---: | --- |
| `binary/Cube.exe` | 3,885,568 bytes | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| `binary/Server.exe` | 1,718,784 bytes | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

The individually inspected original function files and the corresponding native instructions in `decompiled/client/assembly.txt`, `decompiled/server/assembly.txt` and the two images are the basis of this document. The client and server addresses are not interchangeable. In particular, the separately verified server sender closes the packet format; the client host publisher alone does not establish a complete socket-sending path. No reconstruction, runtime capture, earlier project contract or replacement implementation was used.

## Record meaning and raw extent

The semantic direction is defined by the map being described. The first identifier selects the creature whose relationship map is to change. The second identifier is the key within that map. Neither identifier is a socket handle, a peer-vector index or a pointer.

| Payload offset | Width and representation | Established meaning |
| --- | --- | --- |
| +0x00 | 8 bytes, raw 64-bit creature identifier | Creature whose map is described |
| +0x08 | 8 bytes, raw 64-bit creature identifier | Relationship-map key |
| +0x10 | 4 bytes, IEEE binary32 | Value assigned to that map entry |
| +0x14 through +0x17 | 4 opaque bytes | No semantic use established in the inspected producer, transport or consumer |

The complete payload is 0x18 bytes. Its linked-list node links are separate from the payload and are not serialized. The final four bytes travel with the record even though only the preceding twenty bytes have an established semantic meaning. No additional payload alignment bytes are inserted between successive serialized records.

For the initial record in client helper 0x1A0970, call the first explicit creature argument S and the second T. The payload contains T's identifier from T+8, S's identifier from S+8, and the current binary32 entry in T's map at +0x13A4 keyed by S's identifier. That lookup can create an absent map entry with a zero binary32 value. The initial record is appended before the helper traverses other creatures. The helper's later qualifying records describe another creature C's map keyed by S's identifier, after its local entry has been assigned 0.5. This document does not close the upstream relationship and distance predicates deciding which C qualifies.

The identified construction sites assign the two identifiers and the value, but do not initialize the trailing four bytes of their stack payloads. Consequently their value is not specified as zero, and they cannot be treated as a fourth semantic field. The native list copies preserve them. The first construction site does not reject equal or zero identifiers before appending; the receiver's later identity checks are a separate boundary. This does not authorize null creature pointers at the producer.

Evidence: client RVA 0x1A098F–0x1A0A08 and the later append in 0x1A0970, `decompiled/client/functions/005a0970.c`; map access at client RVA 0x068AD0, `decompiled/client/functions/00468ad0.c`; 24-byte copying at client RVA 0x051D00, `decompiled/client/functions/00451d00.c`. The latter copies three consecutive eight-byte portions of the supplied payload, including the opaque bytes.

## Publication into peer queues

The client host publisher at RVA 0x0DC8E0 takes a host object whose +4 member supplies the world. Its peer vector spans the pointers at host+0x28 and host+0x2C; each occupied vector element is a four-byte peer pointer. Peers are visited in ascending vector position, and a null peer pointer is skipped. For a peer reaching the relationship publication site, the complete forward list at output aggregate+0x50 is appended to peer+0x68. Existing records in that destination remain before the new records. Source records retain their forward traversal order.

There is no relationship-record identifier, amount, ownership, distance or designated-creature test at this copy site. Other aggregate members have their own work and predicates. Earlier work in the publisher resolves the recipient creature and can use its state while processing those other members; this audit does not make a malformed or missing recipient creature safe or promise that it reaches the relationship copy. The precise guarantee is the absence of an additional per-record filter once this site is reached for an ordinary valid peer.

The publisher enters the world critical section through client RVA 0x201CB0, using the world from host+4 at 0x0DC916–0x0DC919. That helper enters the critical section embedded at world+0x8000C0. The relationship append itself is at client 0x0DCDB2–0x0DCDD2, between publication of aggregate+0x48 to peer+0x60 and aggregate+0x58 to peer+0x70. Its range-copy helper at 0x0DC5B0 copies the payloads through 0x051D00 without changing field contents or sorting.

The separately inspected original server publisher at RVA 0x0272D0 has the same relevant aggregate-to-peer relationship: aggregate+0x50 is appended to peer+0x68 for each occupied peer-vector position. Server 0x0277A2–0x0277C2 reaches the 24-byte range-copy helper 0x021160, whose record copy is 0x0203C0. It also uses the world at host+4 and the critical section at world+0x8000C0. This establishes the server-side queue producer used by the sender below without assuming that all surrounding client and server functionality is identical.

Evidence: `decompiled/client/functions/004dc8e0.c`, `004dc5b0.c`, `00601cb0.c`; `decompiled/server/functions/004272d0.c`, `00421160.c`, `004203c0.c`, `004d3df0.c`. Ordinary list appends have the native 0x07FFFFFE element-count limit and can take the length-error path at that limit. Allocation and exception recovery are outside the successful-publication guarantee.

## Original server send boundary and wire location

The outgoing worker at server RVA 0x023DD0 is associated with a peer slot. Its context supplies a world pointer, a pointer to the peer-vector storage, a four-byte slot index, a recipient identifier and a running-byte pointer. Its ordinary loop requires the running byte to remain nonzero and the selected peer slot to be nonnull. Earlier snapshot preparation uses recipient and world state; complete connection validity and all snapshot failure paths are outside this audit. No per-relationship-record identity filter occurs when this worker copies, serializes or sends the established queue.

At server 0x0248DB–0x024911 the worker enters the world critical section and copies the peer's aggregate beginning at peer+0x18 into a temporary outgoing aggregate. Copy-constructor RVA 0x023400 copies member +0x50 through 0x022380, so the relationship source is peer+0x68. The destination snapshot receives the records in source list order, with all 24 payload bytes preserved. The sender then empties the peer's relationship queue at 0x024B18–0x024B4D as part of clearing the consumed peer outputs. This happens before sending the world-event packet. The normal world-lock release is at server 0x024C7B, before packet serialization and socket sends of that packet. A separate peer-associated critical-section operation appears in the surrounding sender; its acquisition history and broader purpose are not closed here, and are not substituted for this verified world lock.

Thus a record can have left the live peer queue while it still exists in the sender's private snapshot and before any successful network delivery. This contract does not promise requeue on a send error. The observed send checks recognize a return value of -1; the inspected packet send sites do not establish a complete retry policy for a positive short write.

The world-event packet uses a four-byte message kind equal to 4, followed by a four-byte compressed-payload byte count and that many compressed bytes. These are native little-endian fields in the supported images, without byte-order conversion at these sites. The kind is sent at server 0x024D22, the compressed byte count at 0x024FDD, and the buffer at 0x025011. Compression is applied to the serialized aggregate at 0x024FAC through helper 0x012C70 before its length is obtained.

Within the decompressed aggregate, relationship records are a counted list; they do not have a fixed absolute byte offset from the packet start. The native serializer visits aggregate members in this order: +0x18, +0x00, +0x10, +0x08, +0x20, +0x30, +0x28, +0x38, +0x40, +0x48, +0x50, +0x58 and +0x60. The +0x28 and +0x38 portions include variable counted content. The relationship list at +0x50 therefore follows the complete +0x48 list and precedes the complete +0x58 list, regardless of the earlier lengths.

At server 0x024F78, serializer 0x021C60 writes the relationship list as a raw four-byte element count followed by exactly 24 bytes for each element in forward list order. It serializes both identifiers, the binary32 value and the opaque trailing bytes. There is no per-element type tag, field conversion, target lookup, equality test, sorting or duplicate coalescing in that serializer. An empty list contributes the four-byte zero count and no records. Its storage count is four bytes; the matching client decoder treats that count as signed 32-bit when deciding whether to iterate.

The sender has an earlier message-kind-0 creature-snapshot stage and a message-kind-5 clock stage before the kind-4 world-event packet. This establishes ordering in this worker's successful send route, not a guarantee that every identifier in a later relationship record will resolve on the receiving client. The kind-4 records are event-aggregate content and are not fields of the kind-0 creature snapshot.

Evidence: `decompiled/server/functions/00423dd0.c`, with the native sites above; `00423400.c`, `00422380.c`, `00421c60.c`, `00412c70.c`, `00412d00.c`, `004d3df0.c`, `004d5c60.c`. The native compression helper reaches the bundled compression DLL import; compressor internals, compressed-byte reproducibility and failure recovery are not specified here.

## Client receive decoding and guarded publication

Client receive worker RVA 0x06B740 reads message kinds from the socket stored at client+0x8006CC while its running byte at client+0x800585 admits the loop. The original dispatch maps kind 4 to 0x06CC0E. Kind 0 has a separate dispatch destination at 0x06BAAD and is not this relationship-list decoder.

The kind-4 route obtains the four-byte compressed length, reads the payload, decompresses it through client RVA 0x0494B0 at 0x06CCC8, and decodes a temporary aggregate. Its list order agrees with the sender order above. The relationship list is decoded into temporary aggregate+0x50 by client 0x057F70 at 0x06CEB2; the adjacent calls decode +0x48 and +0x58. Blocking socket reads, decompression and these temporary-list allocations occur before the world lock used for publication.

For a complete ordinary list, decoder 0x057F70 reads the four-byte count and appends each complete 24-byte payload to the temporary list in wire order. Its signed-positive count test means zero and negative count bit patterns do not enter its record loop. There is no actor lookup, ID normalization, self-ID rejection, amount clamp or removal of duplicates at decoding. Native copies preserve all payload bytes.

After the entire temporary aggregate has been decoded, the worker enters the embedded world's critical section at client 0x06CEE8. The world object is client+0x2E4, so the lock used by helper 0x201CB0 is at client+0x8003A4. At 0x06D023–0x06D03D the temporary relationship list is appended to client+0x800680 through helper 0x0579C0 and the 24-byte range-copy path. Existing queue entries precede newly appended entries. The worker releases this world lock at 0x06D088 before destroying the temporary aggregate at 0x06D093. Publication does not mutate a creature relationship map; it makes the received list available to the later frame consumer.

Short or malformed data is outside the complete-record contract. In particular, the record decoder's insufficient-record branch advances the cursor to the buffer end but does not supply a newly initialized payload before its append. The insufficient-count branch likewise does not establish a normal decoded count. Therefore this audit does not specify clean rejection, zero filling, a valid prefix-only result or safe continuation for truncated data. A negative count is not a declared valid wire encoding of an empty list merely because its local loop is skipped. Allocation limits, decompression errors, oversized counts and connection shutdown require further dependencies before a total malformed-input contract could be stated.

Evidence: client native 0x06CC0E–0x06D093 and dispatch tables at 0x06D180/0x06D1A4 in `binary/Cube.exe`; `decompiled/client/functions/0046b740.c`, `00457f70.c`, `004579c0.c`, `004dc5b0.c`, `00451d00.c`, `004494b0.c`, `00449540.c`, `00601cb0.c`. The incomplete high-level reconstruction of the worker is supplemented by the original native branches; it is not used to fill absent dispatch bodies.

## Later frame map assignment and identity edge cases

When client frame RVA 0x088EE0 reaches 0x08FBB7–0x08FCB8, it traverses client+0x800680 in forward queue order. A record's first identifier must resolve to a nonnull creature in this frame's embedded world. Its second identifier must separately resolve to a nonnull creature in that same world. Both lookups use client helper 0x02F000, which returns null for an absent key or a present key whose stored creature pointer is null.

Only after both successful lookups does the consumer compare the two complete 64-bit identifiers. Equal identifiers are ignored. For an accepted record it reads the binary32 payload at +0x10, looks up the first identifier again, obtains that creature's map at +0x13A4 keyed by the second identifier through helper 0x068AD0, and assigns the supplied value. The repeated first lookup is a real additional lookup, not use of the earlier returned pointer.

Exactly the first creature's +0x13A4 map is written by this consumer. Its map helper can insert a previously absent key before the assignment. It does not create a missing creature, update the second creature's map, create a reciprocal relation, alter the separate member at +0x13AC, or apply a hit record. The record value is an assignment, with no addition, interpolation, floating-point comparison or clamp. Its four bytes pass through scalar binary32 loads and stores; the consumer performs no arithmetic conversion of the supplied value.

| Ordinary identity or value case | Established receiver result |
| --- | --- |
| First identifier absent, or stored with null creature pointer | Record skipped before the second lookup |
| First identifier valid, second absent or stored null | Record skipped before the equality test and map write |
| Both identifiers resolve and are equal | Record skipped, including an equal nonzero ID |
| Identifier zero | Used as an ordinary complete lookup key; there is no separate zero-ID sentinel test here |
| Both identifiers zero | Missing/null lookup still rejects; if both resolve, equality rejects |
| Distinct identifiers resolve to the same creature pointer | The ID comparison does not reject this alias; the first creature's map can be assigned under the second ID |
| Distinct valid IDs, value 0 or an ordinary negative value | Value assigned without a sign or positivity gate |
| Several accepted records for the same ordered ID pair | Each assignment occurs in queue order; the last such assignment wins |

Neither successful record admission nor the map target depends on a comparison with the designated or controlled creature. There is no health, class, owner remapping, relationship-predicate, world-role-byte, local-player or source-authority check in this consumer. A designated creature and a noncontrolled creature take the same record rule when supplied as the first ID. This is a statement about this specific receiver, not a claim that arbitrary peers are authorized upstream to manufacture world events.

The first lookup, second lookup and equality test are at client 0x08FC22–0x08FC4D. The value read, repeated first lookup, map access and value assignment are at 0x08FC4F–0x08FC76. Evidence: native `binary/Cube.exe`; `decompiled/client/functions/0042f000.c` and `00468ad0.c`. The relationship map uses the whole eight-byte identifier as a key; its native ordering compares the upper word as signed 32-bit and the lower word as unsigned 32-bit when upper words tie. This signed comparison detail does not narrow record equality to either word alone.

## Frame timing, queue consumption and remaining lifetime limits

The frame enters the embedded-world critical section at client 0x08902F and releases it at 0x09D2EA. It is the same world+0x8000C0 lock used by receive publication. The separate frame lock operations involving client+0x800D44 are a different object. Consequently successful receive publication and this frame's map assignments are serialized by the verified world lock; socket reception and temporary decoding can proceed outside it.

The frame's world-step calls at 0x08CF2E and 0x08CF49 precede the relationship consumer at 0x08FBB7. The relationship queue is later cleared at 0x090630–0x090636 through helper 0x06F870, which resets the list links and count and removes its old nodes. Records skipped for a missing, null or equal identifier are not retained for a later retry by this clear operation. Iteration itself does not selectively remove or defer records.

This consumption and clearing also precede the later action-input region beginning at 0x09B58E. After that later frame work, a nonnull client+0x8006C8 admits the host publisher call at 0x09C26D with this frame's shared output aggregate. Thus the proved frame ordering places incoming relationship application after the earlier world-step sites and before later input and outgoing host publication. It does not move the incoming map assignment into either world-step call, and it does not guarantee that a packet arriving during a particular wall-clock interval is consumed in that same frame.

The producer may already have changed local relationship state before or while producing its outgoing record; receipt is still an assignment at a later client-frame boundary. No extra local or designated-creature shortcut in this record consumer was found. Whether a particular host participant is represented by a socket peer, receives its own published record or instead observes only the producer's direct local state is not established by the publisher and consumer sites alone.

Evidence: client frame native sites 0x08902F, 0x08CF2E, 0x08CF49, 0x08FBB7–0x08FCB8, 0x090630–0x090636, 0x09B58E, 0x09C256–0x09C26D and 0x09D2EA in `decompiled/client/assembly.txt` and `binary/Cube.exe`; `decompiled/client/functions/0046f870.c`, `00601cb0.c`, `004dc8e0.c`.

This contract stops at the described delivery and assignment. It does not close every caller gate of the frame, first construction of each aggregate or queue, map construction/destruction, creature removal and identifier reuse, the lifetime or decay of relationship entries, all local producer mutations, reconnection replay, every publisher/sender counterpart, fatal processing, or behavior after allocation, list-length, decompression or socket failure. No timer or expiry field is present among the three established semantic fields. That absence does not prove relationship state never expires elsewhere. No transport or receiver random draw is present at the identified relationship copy, serialization, decode, publication and assignment sites; randomness in upstream relationship production or other neighboring aggregate members is outside this statement.
