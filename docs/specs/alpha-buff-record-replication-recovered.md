# Alpha resident buff records and replication — independent recovery
**FINAL / STABLE / HELD**

Disposition: held. Authored 2026-11-08; re-certified against the original
images on 2026-11-20 (see the final section). This is a bounded,
code-free behavioral contract recovered independently from original Alpha
evidence. It is not a claim that all buff-producing abilities or every network
failure case have been recovered.

## Scope and evidence identity

The evidence root for every relative citation below is the owner's original
`disassembled alpha cube world` directory. The supported client is
`binary/Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
The companion original server inspected for transport is `binary/Server.exe`,
SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.
Both hashes were checked during this recovery. These images use preferred base
`0x00400000`; all routine and instruction addresses in this document are RVAs,
unless explicitly called file offsets.

Only the original images and their original decompilation/assembly exports
were used as behavioral evidence. No reconstruction, earlier buff draft,
replacement implementation, tests, captures, probes, logs, SDK, or handoff was
read. This document supplies native facts only; it specifies no engine
extensions.

## Resident storage and record identity

A creature owns an ordered, doubly linked resident list at creature offset
`+0x1178`, with a 32-bit entry count at `+0x117c`. Construction creates an empty
list. Each node has two 32-bit links followed by a **24-byte record**. Links
are container state, not part of the record or its wire bytes. Forward
iteration is insertion order. [C1, C2]

| Record offset | Width | Established meaning |
| --- | --- | --- |
| `+0x00` | 1 byte | Numeric buff type ID. |
| `+0x01` | 3 bytes | Alignment bytes; no semantic field established. |
| `+0x04` | 4 bytes | Binary32 value or magnitude, interpreted according to type. |
| `+0x08` | 4 bytes | Signed 32-bit remaining time, in the update clock's millisecond units. |
| `+0x0c` | 4 bytes | Alignment bytes; no semantic field established. |
| `+0x10` | 8 bytes | Embedded source creature ID, retained as a complete 64-bit identity. |

The record initializer zeroes the type, value, remaining time, and both halves
of the embedded source ID. It does not establish zero values for the alignment
bytes. Copy, list insertion, overwrite, and raw record/event serialization
retain the complete record, including alignment bytes. No semantic use or
portable canonical value for those alignment bytes is claimed. [C2, C3, C4]

The type lookup returns the **first** forward entry with the requested type,
or no result. It does not filter by embedded source ID, magnitude, or remaining
time. [C5]

## Applying, appending, overwriting, and removing

The ordinary record-application operation treats type IDs
**1, 2, 3, 6, 7, 9, 10, and 11** as first-match replacement types. For one of
these IDs, an existing first entry of that type has its entire 24-byte record
overwritten in place. Its list position and the entry count do not change.
The embedded source ID is overwritten along with the other record bytes;
the previous source is not retained. Any later same-type entries already
present remain untouched. [C4]

If no matching entry exists, application appends at the tail. For every other
numeric type ID, application appends without a same-type search. Consequently,
types **4, 5, 8, and 12**, and unrecognized byte IDs, are not made unique by this
operation. Appending preserves the supplied record bytes and increments the
resident count. There is no type-range validation or source-based merge in
this operation. [C2, C4]

The append path checks the resident count against **0x7fffffe** before
incrementing it; at that count the original raises a non-returning
"list too long" length failure instead of appending. The 40-byte event lists
use the corresponding limit **0x5555554**. These are container limits, not
buff semantics. [C4, C11]

Neither an incoming record with zero or negative remaining time nor a zero
magnitude is an immediate deletion instruction to this operation. The record
is first replaced or appended by the rules above. Its later lifetime follows
the resident update. The 40-byte event described below contains no separate
add, refresh, or remove opcode. [C4, C6]

During a creature update, the resident entries are visited in forward order.
The signed 32-bit update delta is subtracted from each record's full signed
32-bit remaining-time field. A post-subtraction value greater than zero is
still active. A value of zero or less is scheduled for removal. This is not a
16-bit duration, an unsigned countdown, or a floating-point countdown; the
record and event copy paths do not narrow it. Arithmetic is the original
32-bit integer operation, without a separate saturation step. [C6]

Expired entries are collected by their node identity during that traversal.
Actual unlinking and count reduction occur in a subsequent forward pass over
the collected entries, after the resident traversal completes. Thus a
same-type search performed by work during that traversal can still encounter
an entry already selected for later removal. The expiry collection and
removal path does not itself append a buff event to the outgoing event list.
[C6, C7]

A creature reset also clears the entire resident list and sets its count to
zero. The cited reset operation is broader than buff expiry and is not an
incoming buff-event operation. [C8]

The following type-specific update facts are established here because they use
the embedded source and the signed clock. Type-specific work is performed only
for entries that are still active after their subtraction; an entry scheduled
for removal receives no type work in that visit.

- An active type 1 entry sets the creature's signed 32-bit `+0x12c` timer to
  zero when that timer is greater than zero. After the resident traversal and
  removal pass, the same routine subtracts the same update delta from
  `+0x12c`; a timer cleared this way therefore ends that update at minus the
  delta.
- An active type 4 entry can cause periodic work only when world byte
  `+0xb4` is zero. The previous remaining time (the new value plus the delta,
  in 32-bit integer arithmetic) and the new remaining time are compared after
  signed integer division by **400**, truncating toward zero. A changed
  quotient causes one call to the combat-resolution path during that visit,
  even if the delta crosses several such boundaries. That call resolves the
  source using the record's embedded ID at `+0x10`, passes the owning creature
  as the affected creature, and uses the record's binary32 value at `+0x04`.
  A missing source lookup is passed as an absent source; it does not rewrite
  the stored ID or itself delete the record.
- The combat-resolution entry can consume C-runtime random draws. When it
  passes its entry gates with an absent source, it makes one random draw whose
  result is not used by that branch and sets the affected creature's binary32
  field `+0x1184` to `0.5` before continuing. Other branches of that entry make
  further draws; their complete sequence is not specified here. A type 4
  periodic call therefore participates in the global random-draw order.

These observations do not assign descriptive names to all type IDs or specify
the complete combat outcome. They establish clock, identity, and ownership
behavior at this boundary. [C6, C9]

## The 40-byte buff event

The update/event aggregate has a buff-event list at aggregate offset `+0x58`,
with its count at `+0x5c`. Event-list nodes have two links followed by a
**40-byte event payload**. Event enqueue appends the complete payload at the
tail; it does not coalesce events with matching source, target, or type.
[C10, C11]

| Event offset | Width | Established meaning |
| --- | --- | --- |
| `+0x00` | 8 bytes | Outer source creature ID. |
| `+0x08` | 8 bytes | Outer target creature ID. |
| `+0x10` | 24 bytes | Complete resident-record payload. |
| `+0x10` | 1 byte | Embedded record's type. |
| `+0x14` | 4 bytes | Embedded record's binary32 value. |
| `+0x18` | 4 bytes | Embedded record's signed 32-bit remaining time. |
| `+0x20` | 8 bytes | Embedded record's source ID. |

The outer source at `+0x00` and embedded source at `+0x20` are separate
identities. There is no general rule that one replaces or equals the other.
In particular, the generic self-application producer first applies its supplied
record, then emits an event whose two outer IDs both equal the receiving
creature's ID while the supplied embedded source is copied unchanged. A
self-applied record can therefore have a nonzero outer source and an embedded
source of zero. [C12]

The original type 11 self-application example runs only when the creature's
32-bit field `+0x128` is nonzero, its byte `+0x140` equals 4 and its byte
`+0x141` equals 1. It supplies a zero binary32 value, 30000 remaining time and
a zero embedded source, applies it to that creature through the ordinary
application operation, and appends the same record to the supplied
aggregate's buff-event list with both outer IDs set to that creature. Only the
type byte and the semantic fields are written; the record's alignment bytes
are not initialized by this producer. This independently demonstrates why
substituting the outer source would change resident state. [C13]

A distinct combat producer appends a type 4 resident record with 3000
remaining time, a value equal to the resolved magnitude at that point
multiplied by binary32 `0.1` (stored bits `0x3dcccccd`),
and an embedded source equal to the acting creature ID. Its accompanying
event has the actor as outer source and the affected creature as outer target.
This path appends the resident record directly. It does not first make type 4
unique. This is evidence of one producer, not a complete contract for the
ability that reaches it. [C14]

Two type 6 magnitude-depletion paths were traced. Both visit **every** type 6
entry of the affected creature's resident list in forward order (not only the
first). For each such entry they subtract an incoming hit magnitude (binary32
at hit `+0x10`) from the record's binary32 value and store the result. If the
result is zero or less, both the value at `+0x04` and the remaining time at
`+0x08` are then set to zero; a NaN result is stored unchanged and is not
treated as depleted. Each visited type 6 entry, depleted or not, is then
wrapped in a new 40-byte event carrying a copy of the complete updated record,
including its embedded source and alignment bytes. [C15]

- **Combat path.** Within the damage-application routine, this branch is
  reached only inside that routine's ownership gate (world byte `+0xb4` zero,
  or the affected creature is the designated local creature), when the hit's
  magnitude is greater than zero and the hit's kind byte at `+0x45` equals 5.
  The event's outer source and outer target are copied from the hit's own
  source and target IDs (hit `+0x00` and `+0x08`). It is appended to the
  routine's outgoing aggregate's buff-event list.
- **Client local-player path.** The client main update walks application list
  `+0x800630`. For an entry whose target ID (at `+0x08`) resolves to the
  client player and whose kind byte `+0x45` equals 5, it performs the
  depletion over the client player's resident list without a
  positive-magnitude test. Both outer IDs of each event equal the client
  player's ID. The events are appended to the main update's generated
  aggregate, whose buff list is the one later filtered for transmission, so
  they pass the outer-source filter described below.

On a receiver, each such event is applied by the ordinary operation: because
type 6 is a first-match replacement type, every one of several type 6 wrappers
for one target overwrites that target's **first** type 6 entry in turn. A
zero-time record is removed by the next resident traversal that reaches it
with a non-negative delta. [C4, C15]

## Applying received events and ownership

Both the world input-event loop and the client's pending-network-event loop
walk events in list order. They look up the **outer target ID** at event
`+0x08`. If the target is found, the complete 24-byte record starting at event
`+0x10` is submitted to the ordinary record-application operation. No lookup of
the outer source is required for this application, and the embedded source is
not synthesized from the outer source. If the target is absent, that event is
skipped; these loops do not retain it for a later target appearance. [C16]

The application loops do not gate resident application by whether the target
is the locally designated creature. They also do not perform a separate
positive-time, positive-value, or target-health test before ordinary record
application. The stated local/other distinction belongs to production,
combat resolution, and network forwarding, not to the target selection of
these application loops. [C4, C16]

The relevant world fields have these established relationships:

| World state | Established behavior at this boundary |
| --- | --- |
| Byte `+0xb4 == 0` | The active type 4 periodic path may run. |
| Byte `+0xb4 != 0` | That periodic type 4 path is suppressed. When the world update receives an input aggregate, a nonempty incoming hit list first causes the outgoing aggregate's hit list (`+0x00`) to be cleared before the hits are processed, and a nonempty incoming buff list causes the outgoing buff-event list (`+0x58`) to be cleared after all incoming buff events have been applied. That buff clear discards every outgoing buff event present at that moment, including type 6 wrappers emitted by the hit processing earlier in the same update. |
| Pointer `+0xb8` | The designated local creature; client setup and connection paths assign the client player here. |

The client's world begins at application `+0x2e4`, so world byte `+0xb4`
aliases application `+0x398`, the connection-state byte set by the connection
path. World pointer `+0xb8` aliases application `+0x39c`. These relationships
support the local-versus-connected interpretation without assuming that the
same byte globally controls every buff effect. [C17]

The combat-resolution entry used by the periodic path rejects work on a dead
target and also rejects the designated local target when world `+0xb4` is
nonzero. A later damage-application path permits health-changing work when
world `+0xb4` is zero or the target is the designated local creature. These
are two separate gates in different paths; they are not a universal rule that
remote creatures have no resident buffs. [C9, C14]

The resident remaining-time traversal itself contains no per-record local-ID
or world-mode exemption. For a creature reaching that update portion, all
resident entries are decremented and expired by the rules above, including
entries for other creatures. This statement does not override earlier
creature-level scheduling or lifecycle gates outside the bounded traversal.
[C6]

## List serialization and deserialization

The original contains list codecs for both record-sized 24-byte payloads and
40-byte event payloads. A list encoding starts with a **4-byte count**, then
the payloads in forward list order. The 24-byte writer copies all three
successive 8-byte record regions. The 40-byte writer copies all 40 bytes of
each event. Both are native little-endian encodings, without field-by-field
network byte-order conversion, duration conversion, or ID rewriting. [S1]

For a complete, well-formed input, each reader consumes the count as a signed
32-bit integer, then reads exactly that many payloads when the count is
positive. The reader **appends** each decoded payload to the supplied list.
It does not clear that destination first and does not call the creature's
first-match replacement operation. A zero or negative count adds no entries.
IDs, floating-point bits, remaining-time bits, order, duplicates, and opaque
alignment bytes survive a complete read. [S2]

The existence of the 24-byte codec does **not** establish that a creature's
resident list is included in creature snapshots. The server-to-client update
aggregate has two other 24-byte-list slots at `+0x48` and `+0x50`, immediately
before its 40-byte buff-event slot at `+0x58`. Their use of the same
record-sized codec is not evidence that they are resident buff snapshots.
The verified packet branch reads those two lists, then the buff-event list,
then the aggregate's `+0x60` list. [S3]

## Client-to-server transport

The client main update selects generated buff events whose **outer source ID**
equals the client player's ID. It copies those selected 40-byte events into
the outgoing network buff queue at application `+0x8006f4`, preserving their
order and every payload byte. The filter does not compare the embedded source
ID or require that the outer target be local. [N1]

The client sender snapshots that queue under the world protection scope and
clears the original pending queue. For each event in the copied queue, it sends
a 4-byte little-endian packet identifier **8**, then the raw **40-byte event**.
This single-event packet has no list-count prefix. Both IDs and the embedded
record are carried without modification. Within one send cycle, the buff
packets follow every packet-7 (72-byte hit) packet taken from the same snapshot
and precede the packet-9 (112-byte) packets. A send call returning the socket
error value skips the remaining event sends of that cycle and transfers to the
sender's error handling, which is not further specified here. [N2]

The original server's packet-8 receiver first zeroes the temporary event's
embedded type, value, remaining time and source (not its outer IDs or
alignment bytes), receives up to 40 payload bytes, and appends the event to the
server's incoming buff-event queue under world protection. A receive error
ends the connection's receive processing without appending; an end-of-stream
result before all 40 bytes arrive leaves the receive loop and still appends the
partially received event. That
branch does not rewrite the outer source or embedded source from the
connection's own ID. It does not itself apply the event to a creature. The
server update drains the incoming queue into an input aggregate and the world
event loop applies it by outer target as described above. [N3, C16]

Successful complete transfers preserve queue order. The sender's checks for a
send error are not a demonstrated guarantee against every short positive
socket send; failure behavior is bounded separately below. [N2]

## Server-to-client transport

The server distribution operation appends the outgoing aggregate's complete
buff-event list, in order, to every existing connection's buff-event list at
connection `+0x70` (aggregate `+0x58` within the aggregate beginning at
connection `+0x18`). Unlike some spatial event channels in that same operation,
this buff-list copy has no distance, source, target, or local-owner filter.
It copies events already present in the **outgoing** aggregate; this fact alone
does not establish automatic relay of every client-originated input event.
[N4]

The per-connection sender snapshots its aggregate and clears the pending lists
while holding the world protection (the snapshot itself also inside the
connection's own protection). It then sends the 4-byte packet identifier
**4**, serializes the snapshot in its fixed channel order, compresses the
resulting buffer, and sends a 4-byte compressed-byte length followed by that
many compressed bytes. The on-wire order is therefore identifier, length,
compressed bytes. The buff channel is encoded as the count and
40-byte event list described above. It follows the two 24-byte channels at
aggregate `+0x48` and `+0x50` and precedes the `+0x60` channel. [S1, S3, N5]

The client's packet-4 receiver obtains the byte length and compressed bytes,
expands the buffer, reads the aggregate channels in that order, and appends
the decoded buff events to application pending queue `+0x800688` under the
world protection scope. Existing pending events precede the new events. The
main update later applies that queue by outer target, then clears it. Received
events are not added back to the client sender queue merely by that pending
application loop. [N6, C16]

### Original-image confirmation of omitted packet branches

The client's assembly export does not contain the computed packet-4 body.
The following were decoded from the supported `binary/Cube.exe` bytes, not
inferred from a reconstruction or replacement implementation:

- The receiver begins at RVA `0x6b740`. Dispatch at RVA `0x6b8a7` bounds the
  packet ID to 15, uses the byte selector table at RVA `0x6d1a4`, then the
  destination table at RVA `0x6d180`.
- Packet ID 4 has selector index 4 at RVA `0x6d1a8`, file offset `0x6c5a8`.
  Destination entry RVA `0x6d190`, file offset `0x6c590`, contains preferred
  address `0x0046cc0e`: the packet-4 branch is RVA `0x6cc0e`.
- That branch reads the length, receives the buffer, and invokes the expansion
  wrapper at RVA `0x6ccc8`. The two 24-byte-list reader calls are at RVAs
  `0x6ce9f` and `0x6ceb2`; the 40-byte-list reader call is at `0x6cec5`.
- The buff-list append into application `+0x800688` is at RVA `0x6d05c`, after
  the world protection entry at RVA `0x6cee8` and before the corresponding
  protection exit at RVA `0x6d088`.

The PE `.text` mapping used here is section RVA `0x1000`, raw offset `0x400`;
the cited code/table bytes in that section have file offset equal to RVA minus
`0xc00`. The image-byte findings agree with the separately readable list
readers and the main update's pending-buff consumer. [N6]

## Timing and snapshots

The world input aggregate processes its hit-event list before its buff-event
list, then continues to creature simulation. An incoming buff applied through
that input path can therefore be decremented by the resident traversal later
in the same world update. In the client main update, the local world
simulation calls occur before the pending network buff consumer, so records
newly applied by that consumer are not decremented by those already completed
simulation calls. These are path-order facts, not a latency guarantee. [T1]

The client main update compares its signed frame interval `n` with 20. If
`n` is 20 or less (including zero or negative), it makes one world update with
delta `n`. Otherwise it makes `k = floor((n - 21) / 20) + 1` world updates of
delta 20 and then one final update of delta `n - 20k`, which always lies in
1 through 20; for example 40 yields 20 then 20, and 41 yields 20, 20, then 1.
These client simulation calls pass the main update's generated aggregate as
the outgoing aggregate and **no input aggregate**, so the world input hit and
buff loops do not run in them; received network buff events reach client
creatures only through the pending-network consumer. The buff timer consumes
the individual update delta it receives. No claim that all server
steps are fixed at 20 milliseconds follows: the standalone server loop passes
its measured elapsed interval to its world update. [T1, T2]

The ordinary creature core snapshot is **`0x1168` bytes**, corresponding to
the creature data beginning at creature `+0x10` and ending at creature
`+0x1178` exclusively. The resident list begins exactly at that excluded end.
The ordinary core copy operations and the core delta read/write operations
do not include the resident list or its count. The connection's initial
`0x1168`-byte core transfer likewise does not include them. [P1, P2]

Consequently, these traced core snapshots do not transmit the resident buff
records, their remaining times, or their embedded sources as a list. Their
ordinary application does not itself clear or replace an already existing
resident list. New creatures begin with the independently constructed empty
resident list. This is bounded to these identified core snapshot and delta
paths, not every save or persistence format in Alpha. [C1, P1, P2]

## Explicit limits and malformed input

The positive-count, complete-payload codec behavior above is established.
There is no transactional malformed-input contract in the original list
readers. Each reader requires the cursor plus 4 to be no greater than the
buffer size before reading the count. If that check fails, the cursor is set
to the buffer end and the loop bound is **not** a decoded count: it is an
unrelated machine value (the address of the reader's buffer-state object,
treated as a signed integer). When that value is positive, the reader performs
that many failed payload reads and appends one entry per iteration until the
loop ends or the container's length limit raises its non-returning length
failure; no stable result follows. If a declared payload is truncated, the
cursor is set to the buffer end and the entry is still appended. The
24-byte reader's temporary payload is not freshly initialized for each failed
read. The event reader initializes semantic embedded fields but does not
initialize both outer IDs or every alignment byte before a failed full copy.
The reader can therefore append incomplete or indeterminate data rather than
rejecting the whole list. No stable byte result, deterministic error, rollback,
safe count cap, or clean disconnect is specified for such input. [S2]

The socket receiver loops also distinguish a socket error from end-of-stream;
some zero-length receive exits can reach later processing with partial data.
Compression, allocation failure, partial positive sends, stream corruption,
extreme arithmetic overflow, and hostile sizes have not been established as a
complete end-to-end failure contract. They must not be represented as verified
successful normal events. [N2, N3, N6]

The following dependencies remain deliberately unclaimed:

- The complete producer and descriptive meaning of every buff type and every
  spell/ability path.
- Automatic server re-emission of every received client event. The traced
  server input loop applies the received record and does not itself enqueue
  an outgoing wrapper; the independently traced distribution routine sends
  the outgoing aggregate. No blanket input-to-output relay is established.
- Every creature scheduling/lifecycle condition that can prevent a creature
  from reaching the resident traversal, and every reconnect, late-join,
  destruction, or persistence interaction.
- A canonical semantic interpretation of record alignment bytes, a complete
  thread scheduling order across concurrent producers, or a native guarantee
  for malformed network traffic.
- Whether the client main update's pending-network buff consumer and its
  later clear of `+0x800688` run inside the world protection scope. The main
  update contains world protection entries and exits, but their control-flow
  pairing around that consumer was not established.
- The meaning of hit kind byte values 4 and 5 and of application list
  `+0x800630` beyond the field uses stated above.

## Original evidence index

Each citation names an original relative file and the relevant RVA. Export
filenames contain preferred addresses; the numeric identifiers below remain
RVAs. Assembly is used where the decompiler's control flow or return-type
inference is incomplete.

| Key | Original evidence and bounded subject |
| --- | --- |
| C1 | `decompiled/client/functions/0043b690.c`, RVA `0x3b690`: creature construction and empty resident list. `decompiled/client/assembly.txt`, RVAs `0x3b6da`–`0x3b6f9`: offsets and count. |
| C2 | `decompiled/client/functions/00439600.c`, RVA `0x39600`: 32-byte resident node. `decompiled/client/functions/00451d00.c`, RVA `0x51d00`: complete 24-byte node payload copy. |
| C3 | `decompiled/client/functions/0042c520.c`, RVA `0x2c520`: record semantic-field initialization. `decompiled/client/functions/0043b660.c`, RVA `0x3b660`: event embedded-field initialization. |
| C4 | `decompiled/client/functions/00446af0.c`, RVA `0x46af0`; `decompiled/client/assembly.txt`, RVAs `0x46af0`–`0x46ba7`: exact replacement-type comparisons, full overwrite, tail append and count. Companion server `decompiled/server/functions/00411740.c`, RVA `0x11740`. |
| C5 | `decompiled/client/functions/00443f10.c`, RVA `0x43f10`: first-type lookup. |
| C6 | `decompiled/client/assembly-functions/0060c510.asm`, routine RVA `0x20c510`, instructions RVAs `0x2120e8`–`0x212163` and `0x218310`–`0x2184c2`: signed countdown, active type work, deferred expiry; RVAs `0x2184c2`–`0x2184ce`: post-traversal `+0x12c` decrement. Companion server `decompiled/server/assembly-functions/005322d0.asm`, routine RVA `0x1322d0`. |
| C7 | `decompiled/client/assembly.txt`, RVA `0x77c60`: resident unlink; RVAs `0x218470`–`0x2184c0`: collected-node removal order. `binary/Cube.exe`, RVA `0x77c90`, file offset `0x77090`: directly checked resident count decrement after deletion. Both the C export and assembly export omit that post-deletion segment. |
| C8 | `decompiled/client/functions/00446330.c`, RVA `0x46330`; `decompiled/client/assembly.txt`, RVAs `0x4657f`–`0x465b9`: resident reset clear. |
| C9 | `decompiled/client/functions/00596d30.c`, RVA `0x196d30`: combat entry's target-health/local-mode gates and absent-source branch (one discarded random draw, `+0x1184` set to `0.5`). `decompiled/client/assembly.txt`, RVAs `0x218362`–`0x2183da`: periodic call arguments. `decompiled/client/functions/00595a60.c`, RVA `0x195a60`: separate damage-application ownership gate. |
| C10 | `decompiled/client/functions/00466880.c`, RVA `0x66880`: event aggregate slot `+0x58`. `decompiled/client/functions/0046d850.c`, RVA `0x6d850`: 48-byte event node. |
| C11 | `decompiled/client/functions/00451c70.c`, RVA `0x51c70`: full 40-byte copy. `decompiled/client/functions/004460a0.c`, RVA `0x460a0`: ordered event append. |
| C12 | `decompiled/client/functions/005957c0.c`, RVA `0x1957c0`: apply supplied record, then self-source/self-target wrapper without embedded-source replacement. |
| C13 | `decompiled/client/functions/0043cf20.c`, RVA `0x3cf20`: type 11 self example, 30000 time, zero value and embedded source; `decompiled/client/assembly.txt`, RVAs `0x3cf29`–`0x3cfd8`: gating bytes and the application target. |
| C14 | `decompiled/client/functions/00596d30.c`, RVA `0x196d30`; `decompiled/client/assembly.txt`, RVAs `0x1980c8`–`0x198197`: direct type 4 append and event copy. `binary/Cube.exe`, constant RVA `0x2fcb7c`, file offset `0x2fbb7c`: directly checked binary32 multiplier `0x3dcccccd`. |
| C15 | `decompiled/client/functions/00595a60.c`, RVA `0x195a60`, type 6 depletion portion; `decompiled/client/assembly.txt`, RVAs `0x195eed`–`0x195fc1` and `0x900d0`–`0x90182`: gates, all-type-6 iteration, value and time zeroing, preserved-record wrapper and outer IDs. `decompiled/client/assembly-functions/00488ee0.asm`, RVAs `0x8ff10`–`0x8ff7f` and `0x90081`–`0x90089`: client list `+0x800630`, player-target and kind-5 selection. `decompiled/client/assembly-functions/00488ee0.asm`, RVAs `0x8cf20`–`0x8cf49` and `0x9c0f4`–`0x9c103`: the same generated aggregate is simulated into and filtered. |
| C16 | `decompiled/client/assembly-functions/0060c510.asm`, RVAs `0x20cdd2`–`0x20ceaa`; `decompiled/client/assembly-functions/00488ee0.asm`, RVAs `0x8fcb8`–`0x8fd6c` and `0x9063b`–`0x90641`. Companion server `decompiled/server/assembly-functions/005322d0.asm`, RVAs `0x132b92`–`0x132c6a`: target lookup and record application. |
| C17 | `decompiled/client/functions/0046fc50.c`, RVA `0x6fc50`: application connection byte `+0x398`, world at `+0x2e4`, local player assignment `+0x39c`. `decompiled/client/assembly.txt`, RVA `0x87e90`: local-pointer setter. `decompiled/client/assembly.txt`, RVAs `0x20ce93`–`0x20cea5`: world-mode outgoing buff-list clear; RVAs `0x20cd1f`–`0x20cd4b`: absent input aggregate skip and world-mode outgoing hit-list clear. |
| S1 | `decompiled/server/functions/00421c60.c`, RVA `0x21c60`: count plus ordered 24-byte payloads. `decompiled/server/functions/004219c0.c`, RVA `0x219c0`: count plus ordered 40-byte events. |
| S2 | `decompiled/client/functions/00457f70.c`, RVA `0x57f70`; `decompiled/client/functions/00458030.c`, RVA `0x58030`; `decompiled/client/assembly.txt` at those RVAs, in particular `0x57f86`–`0x57fa2` and `0x58046`–`0x58062`: signed count, the non-data loop bound on a truncated count, append and truncation paths. |
| S3 | `decompiled/server/functions/00423dd0.c`, RVA `0x23dd0`: aggregate writer channel order. `binary/Cube.exe`, RVAs `0x6ce91`–`0x6ced8`: directly decoded matching reader tail. |
| N1 | `decompiled/client/assembly-functions/00488ee0.asm`, RVAs `0x9c0f4`–`0x9c1a3`: outer-source/player-ID filter and outgoing buff queue append. |
| N2 | `decompiled/client/functions/00469c10.c`, RVA `0x69c10`; `decompiled/client/assembly.txt`, RVAs `0x69cdb`–`0x69d14`: world protection and queue snapshot; RVAs `0x6a0ed`–`0x6a1b1`: packet 7 then packet 8 sends and error exits. |
| N3 | `decompiled/server/functions/00426020.c`, RVA `0x26020`, packet 8 (receive loop, error and end-of-stream exits); `decompiled/server/functions/004063d0.c`, RVA `0x63d0`: temporary event embedded-field initialization; `decompiled/server/functions/00411040.c`, RVA `0x11040`: complete event queue append. `decompiled/server/functions/00428170.c`, RVA `0x28170`: incoming event drain. |
| N4 | `decompiled/server/functions/004272d0.c`, RVA `0x272d0`, buff-list distribution portion; `decompiled/server/functions/00420bb0.c`, RVA `0x20bb0`: ordered complete event-list copying. |
| N5 | `decompiled/server/functions/00423dd0.c`, RVA `0x23dd0`: world protection around snapshot and clears, packet identifier 4 sent before serialization and compression, then length and bytes; `decompiled/server/functions/00423400.c`, RVA `0x23400`: per-connection aggregate snapshot. `decompiled/server/functions/00412c70.c`, RVA `0x12c70`; `decompiled/server/functions/00412d00.c`, RVA `0x12d00`: compression wrapper and compression call. |
| N6 | `binary/Cube.exe`, receiver RVA `0x6b740`, dispatch RVAs `0x6b8a7`–`0x6b8b7`, selector/table and packet body detailed above. `decompiled/client/functions/004494b0.c`, RVA `0x494b0`; `decompiled/client/functions/00449540.c`, RVA `0x49540`: expansion wrapper/body. `decompiled/client/functions/00486b60.c`, RVA `0x86b60`: receive helper. |
| T1 | `decompiled/client/assembly-functions/00488ee0.asm`, RVAs `0x8cef4`–`0x8cf49`, then `0x8fd00`–`0x8fd2f`: client simulation subdivision and later network buff application. `decompiled/client/assembly-functions/0060c510.asm`, RVAs `0x20cd1f`–`0x20ceaa`, then `0x2120e8`: hit input, buff input, later resident traversal. |
| T2 | `decompiled/server/functions/00549c50.c`, RVA `0x149c50`; `decompiled/server/assembly.txt`, RVAs `0x14a091`–`0x14a0ae`: measured server elapsed delta passed to world update. |
| P1 | `decompiled/client/functions/00466050.c`, RVA `0x66050`; `decompiled/client/functions/0044b040.c`, RVA `0x4b040`; `decompiled/server/functions/0041df70.c`, RVA `0x1df70`: core copy endpoints. `decompiled/client/functions/004ccfa0.c`, RVA `0xccfa0`; `decompiled/client/functions/004cdaa0.c`, RVA `0xcdaa0`; `decompiled/client/functions/004cd3e0.c`, RVA `0xcd3e0`: core delta field set, excluding resident list. |
| P2 | `decompiled/server/functions/004254a0.c`, RVA `0x254a0`: initial `0x1168`-byte core send beginning at creature `+0x10`. `decompiled/client/functions/0046fc50.c`, RVA `0x6fc50`: matching core receive. |

Author readback is required in full before the held handoff is reported. No
further edits are authorized by this document's disposition alone.

## Re-certification (2026-11-20)

Every claim above was re-checked against the supported `binary/Cube.exe` and
`binary/Server.exe` (both SHA-256 values re-verified unchanged) and their
original decompilation/assembly exports. No reconstruction source, other
specification, engine material or implementation history was consulted. This
specification cites no other held specification, so no cross-specification
comparison was required and no conflict is recorded.

Claims confirmed without change include: resident list offsets and node size,
the 24-byte record layout and initializer, first-type lookup, the exact
replacement-type set {1, 2, 3, 6, 7, 9, 10, 11} with full in-place overwrite,
tail append, signed 32-bit countdown with `> 0` active test, deferred removal
pass and post-unlink count decrement (image bytes at RVA `0x77c90`), reset
clear, signed division by 400, the 40-byte event layout, the self-application
wrapper, the type 4 producer (3000 time, `0x3dcccccd` multiplier, actor
embedded source), target-only lookup in both application loops, the client
outer-source filter, packet-8 framing, server distribution without filters,
the 24/40-byte codecs, the packet-4 dispatch and reader-call bytes, and the
`0x1168`-byte core snapshot boundary.

Corrections and additions:

1. **Type 6 depletion (reversal of a partial claim).** Depletion sets both
   the value and the remaining time to zero, not only the time; it iterates
   every type 6 entry rather than one; it emits a wrapper for every visited
   type 6 entry whether or not depleted; a NaN result is not treated as
   depleted. Gates and outer-ID sources for the two paths were added. Evidence:
   client RVAs `0x195eed`–`0x195fc1`, `0x900d0`–`0x90182`, `0x8ff10`–`0x8ff7f`.
2. **Receiver effect of multiple type 6 wrappers.** Each overwrites the
   target's first type 6 entry. Evidence: client RVA `0x46af0`.
3. **Random-draw order (omission).** The combat entry reached by the type 4
   periodic path consumes C-runtime random draws; its absent-source branch
   makes one discarded draw and sets `+0x1184` to 0.5. Evidence: client RVA
   `0x196d30`.
4. **Type 1 timer follow-up (omission).** After the traversal the same delta
   is subtracted from `+0x12c`, so a cleared timer ends at minus the delta.
   Evidence: client RVAs `0x2183e1`–`0x2183f0`, `0x2184c2`–`0x2184ce`.
5. **World-mode clears (omission).** With world `+0xb4` nonzero, a nonempty
   incoming hit list clears the outgoing hit list before hits are processed,
   and the post-application buff clear discards buff events already emitted in
   that update, including hit-produced type 6 wrappers. Evidence: client RVAs
   `0x20cd3d`–`0x20cd4b`, `0x20cdd2`–`0x20cea5`; `0x195a96`–`0x195a9c` for the
   shared outgoing aggregate.
6. **Client simulation subdivision (precision).** Exact count formula; the
   final update is always 1–20; an interval of 20 or less, including
   non-positive values, makes one update; the client simulation calls pass no
   input aggregate. Evidence: client RVAs `0x8cef4`–`0x8cf49`, `0x20cd1f`–
   `0x20cd28`.
7. **Packet-4 send ordering (correction).** The identifier 4 is sent before
   serialization and compression, not after compression; wire order is
   unchanged. Snapshot and clears are under world protection. Evidence: server
   RVA `0x23dd0` (identifier send preceding the channel writers and the
   compression call).
8. **Truncated count in list readers (correction of an understated limit).**
   The loop bound on a truncated count is a non-data machine value, not a
   cursor-derived count. Evidence: client RVAs `0x57f89`–`0x57f9d`,
   `0x58049`–`0x5805d`.
9. **Server packet-8 partial receive (precision).** Temporary event semantic
   fields are zeroed first; end-of-stream mid-payload still appends; a receive
   error does not. Evidence: server RVAs `0x63d0`, `0x26020` (packet-8 case).
10. **Client send-cycle ordering (omission).** Packet-8 events follow packet-7
    events and precede packet-9 events in each cycle; a socket-error send
    leaves the cycle. Evidence: client RVAs `0x6a0ed`–`0x6a1b1`.
11. **Container length limits (omission).** Resident list limit `0x7fffffe`,
    event list limit `0x5555554`, each raising a non-returning length failure.
    Evidence: client RVAs `0x46af0`, `0x460a0`, `0x58030`.
12. **Type 11 example details (precision).** Zero value, creature-state gates
    (`+0x128`, `+0x140`, `+0x141`) and uninitialized alignment bytes. Evidence:
    client RVAs `0x3cf29`–`0x3cfd8`.
13. **Type-specific work only for active entries; periodic call target
    (precision).** Evidence: client RVAs `0x212149`–`0x21214b`,
    `0x2183cc`–`0x2183da`.

Remaining uncertainties:

- Whether the client pending-network buff consumer runs inside world
  protection (entries/exits exist in the main update; pairing not traced).
- The identity of application list `+0x800630` and the meaning of hit kind
  byte values 4 and 5 beyond their stated uses.
- The full random-draw sequence of the combat-resolution entry and the
  sender's error-handling continuation after a failed send.
- Whether server world byte `+0xb4` is ever nonzero, and hence whether the
  world-mode clears in correction 5 occur on the original server.
- All previously listed unclaimed dependencies remain unclaimed.
