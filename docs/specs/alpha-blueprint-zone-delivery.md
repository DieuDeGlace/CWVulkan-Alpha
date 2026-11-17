# Alpha blueprint zone delivery

Status **FINAL / STABLE / HELD**

Evidence comes from the original x86 Alpha images and their shipped asset store.
Routine addresses are RVAs. Both images have base 0x400000.

| Image | SHA-256 |
|---|---|
| C (Cube.exe) | 84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf |
| S (Server.exe) | 1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1 |

Game offsets start at the client game controller. The client world begins at
game +0x2e4. Creature offsets include the 16-byte object prefix. A zone is
256 × 256 horizontal blocks. A region contains 64 × 64 zones.

## 1. Build-mode and selection evidence

Startup allocates the game controller on the heap, calls its constructor,
then publishes its pointer at C 0x36b1c8. The base constructor's indexed
keyboard array covers game +0x19 through +0x118. The Win32 key-down and
key-up routes use the low eight bits of the key code when they update
that array. The focused DirectInput poll reads a 256-byte temporary and
writes individual gameplay flags within game +0x0b through
+0x17. Array entry 0x13 is the R key and supplies game +0x0f.
(C 0x0c8fb4–0x0c900a, 0x03b4c0–0x03b536, vtable 0x2ffdc8,
0x0c845b–0x0c84cd, 0x07e71f, 0x07e9ea, 0x03b5c0–0x03b5dd,
0x0c91da–0x0c92b5.)

The constructor clears game +0x800704 after initializing the neighboring
outgoing lists. The list aliases at +0x8006ec, +0x8006f4 and +0x8006fc
have an eight-byte header each. The constructor, append and sender paths
write their pointer and count fields or their separately allocated nodes.
The last neighboring header ends at +0x800703. The packet-4 receive bundle
starts at +0x800630 and its 13 eight-byte headers end at +0x800697.
The connection's packet and avatar copies target that bundle, child
allocations or the creature record. The embedded-world construction at
game +0x2e4 precedes the controller's own fields at +0x800580. The
placement byte would be at world +0x800420 if addressed from this world
pointer. The native text has no exact displacement at that world-relative
offset. (C 0x059f43–0x05a0a8, 0x18eb00, 0x066880–0x066a63,
0x0460a0, 0x0861a0, 0x09c0ae–0x09c216, 0x069e79–0x069ed4,
0x06cee8–0x06d088, 0x070211–0x07035b.)

The checked construction, input, list-header and record-copy routes
establish no nonzero write to game +0x800704. The older literal census
still identifies one zero store and five tests at that game offset.
The signed selection byte is creature +0x18d, next to rotation at +0x18c.
The constructor and reset clear them as a word. Member copies and
packet-0 fields 31 and 32 can carry their existing values into another
creature record. The connection setup copies the prior local record
into temporary storage and restores it to the bound creature, including
these two bytes. The ordinary recipe preview writes an item record at
widget +0x16c. A stock input that enters build mode or chooses a model
number for that in-world mode remains unresolved.
(C 0x03c276, 0x04633a, 0x04b22b–0x04b23f, 0x06623a–0x06624e,
0x070211–0x070221, 0x070333–0x070343, 0x0cd253–0x0cd27b,
0x07bbc7–0x07bbf0, 0x0a1db8–0x0a1e05.)
[Original placement input contract](alpha-packet6-placement-producers.md).

## 2. Resolved original models

The world initialization installs a map at world +0x800154 keyed by
blueprint number. The client in-world preview first tests the mode byte,
then sign-extends the live selection byte and resolves it through this
map. The preview draws only if the lookup produces a model. Both
placement handlers resolve the same selection against their world map
at consumption. (C 0x193409–0x1939cb, 0x0b3338–0x0b3364, 0x198840,
0x2112f7–0x21131a. S 0x0cce79–0x0cd43b, 0x0d1860,
0x1370b7–0x1370da.)

Each row was resolved against the stock data1.db, SHA-256
0e482f6d628414db017d983f62843e5dbdc52c5b54fd478f6f0e06c786639b9d.
The native database branch decodes the keyed blob, takes three
little-endian signed 32-bit dimensions and then copies three bytes
per voxel into model storage. In the decoded payload all twelve lengths
match 12 plus three times the dimension product. The native placement
color test treats the all-zero RGB triple as empty. The counts below
are the nonempty triples in those resolved payloads.
(C 0x0496a0–0x049719, 0x0e72fb–0x0e73dc, 0x0e7650–0x0e76e5,
0x211614–0x2116ba. S 0x012f80, 0x02fa0b–0x02fa1b.)

| Selection | Asset key | X × Y × Z | Nonempty voxels |
|---:|---|---:|---:|
| 0 | framework-floor-wood.cub | 10 × 10 × 6 | 600 |
| 1 | framework-wall-window.cub | 16 × 16 × 7 | 100 |
| 2 | framework-wall-door.cub | 18 × 18 × 7 | 99 |
| 3 | framework-roof1.cub | 16 × 16 × 8 | 520 |
| 4 | framework-entrance.cub | 10 × 10 × 5 | 464 |
| 5 | framework-wall.cub | 14 × 14 × 7 | 98 |
| 6 | framework-roof2.cub | 16 × 16 × 8 | 616 |
| 7 | framework-roof3.cub | 16 × 16 × 8 | 798 |
| 8 | framework-wall-balcony.cub | 20 × 20 × 7 | 152 |
| 9 | framework-roof-wall2.cub | 12 × 12 × 6 | 25 |
| 10 | building-stone-well.cub | 10 × 10 × 10 | 102 |
| 11 | building-stone-stairs2.cub | 10 × 10 × 10 | 550 |

Every assigned model has even horizontal dimensions. Model zero is filled
throughout its 10 × 10 × 6 volume. These are model properties before any
placement protection, unloaded-column handling or voxel writer effects.

## 3. Kind-7 consumption and zone snapshots

With build mode enabled, the frame's R rising edge appends a default
kind-7 request to the local creature's queue after the frame's world
pieces. On an offline client, a later authoritative world piece visits
it in insertion order at the beginning of that creature's turn and
clears the visited request list. The connected client world step skips
kind-7 effects and leaves the queue available to its network sender.
A stock offline input chain reaching the enabled state remains untraced.
(C 0x08cef4–0x08cf49, 0x09797d–0x0979c7, 0x2104ec–0x210546,
0x2112f7–0x21131a, 0x211a44–0x211adb.)

The authority uses the consuming creature's current position, aim,
signed rotation and signed selection. With a selected model and clear
world online flag, the voxel write branch runs. If the anchor's zone is
loaded, the placement appends a 16-byte record to zone +0x3c and marks
that zone for output. The four little-endian signed 32-bit fields at
offsets 0, 4, 8 and 12 are selection number, anchor block X, anchor
block Y and anchor block Z. Rotation is absent from the record. The
client's placement arm acquires the world lock at world +0x8000d8 for
the mutation and releases it after the zone-list handling.
(C 0x2112f7–0x2119c2, 0x211451, 0x211929–0x2119b5,
0x19c480, 0x201ea0. S 0x1370b7–0x137787.)

Both world steps take a copy of the whole resulting object list for each
changed zone at wrap-up, paired with its zone X and Y from zone +0x60 and
+0x64. The copy belongs to the result bundle at +0x38. Several additions
to the same zone in one piece are represented by its post-piece list.
An offline placement already mutates the live zone. Its world result
enters the frame's result bundle. The frame's incoming section-8 replacement
pass has a separate pending list.
(C 0x208f60–0x208fc8, 0x2212e0–0x221349, 0x08cf20–0x08cf49,
0x08f58d. S 0x12e950–0x12e9b8, 0x1470a0–0x1470e4.)

## 4. Packet-4 section 8 and client consumption

The server appends each changed-zone result to each connection's outgoing
section-8 list at connection +0x50. This path applies no distance or
request filter. The serializer writes a little-endian 32-bit count of
keyed entries, followed per entry by signed 32-bit zone X and Y, a 32-bit
record count and that many consecutive 16-byte records in list order.
The enclosing packet is the usual compressed packet 4. Repeated keys
can be carried as multiple snapshots.
(S 0x027712–0x027738, 0x024e82–0x024f42, 0x021a30–0x021a98.)

The client parses the zone key and inner list for each entry, then appends
the parsed entries to game +0x800668 while holding the primary world
lock at world +0x8000c0. The frame sees this pending list after its world
pieces. When the list is nonempty it acquires the world lock at
world +0x8000d8 and processes the entries in pending order.
(C 0x06cdd0–0x06ce7c, 0x058100–0x0581a9, 0x06cee8–0x06d088,
0x06cfcc–0x06cfe0, 0x08f58d–0x08f5e9, 0x19c480, 0x201cb0.)

For each entry, the zone lookup requires both coordinates in [0, 65535]
and an already loaded region and zone. If the zone exists, the client
clears its live list at +0x3c, inserts the received records in order,
then zeros zone byte +0x74. An empty inner list still clears the live
list and the byte. A zero outer count supplies no zone entry. An absent
zone causes the pending entry to be discarded. The frame erases each
entry after handling it, so successive snapshots for one loaded zone
leave the last applied list. The receiver makes no placement call
from this path. After the drain and release, the subsequent range-copy
call uses the pending list as its source and the frame section-8 result
as its destination. After an ordinary complete drain the source range
is empty.
(C 0x034d10–0x034d85, 0x08f5e2–0x08f710, 0x057a60,
0x057a90, 0x201ea0.)

## 5. Zone-request latch and sender snapshot

The client zone constructor sets zone +0x74 to 1 with a word store that
also zeros +0x75. Connection setup marks already loaded zones by setting
+0x74 to 1 again. The frame runs its pending section-7 item pass, section-6
static pass and section-8 placed-object pass in that order before it rebuilds
the zone candidate and request lists. For each candidate it resolves that
exact zone. It appends the candidate pair to the packet-11 list at game
+0x2cc only when the zone is loaded and +0x74 is nonzero. A latch cleared
by an earlier pass in this frame excludes the zone from this rebuild.
(C 0x1e4949–0x1e49d2, 0x2241fc, 0x07040e–0x070549, 0x08d5f0,
0x08f435, 0x08f58d, 0x09cfb3–0x09d1c6, 0x034d10.)

The candidate predicate is the 32-bit pointer at game +0x8006c8. When
zero, the only candidate is the local creature's current zone, using the
creature pointer at game +0x8006d0. When nonzero, the frame visits its
world creature table beginning at game +0x2e8 and appends one candidate
per non-null creature whose controller-kind byte at +0x60 is zero. The
test has no health condition. Both branches calculate X and Y from the
signed 64-bit fixed coordinates at creature +0x10 and +0x18. Divide each
by 65,536, keep the low 32 bits of the block quotient as a signed integer,
then divide by 256. Both divisions truncate toward zero. Each contributing
creature supplies just that one zone pair. The candidate and request
lists retain duplicates. The frame clears the outgoing request lists if
the socket at game +0x8006cc is zero.
(C 0x09cfc4–0x09d1c6, 0x0120b0, 0x02f570–0x02f581,
0x28d9e0–0x28da87, 0x069000–0x06901d, 0x086560,
0x09d2bc–0x09d2de.)

Construction zeros game +0x8006c8. A normal connection from that state
uses the local-creature branch. Its online byte is at game +0x398 and the
connection stores the socket separately at +0x8006cc. The non-null
+0x8006c8 branch also calls server-side routines around the frame's world
work, including distribution of world results to the object's
per-connection lists. The checked image has the constructor zero store
as its only identified direct write to +0x8006c8. A stock route that
initializes this pointer for hosted play remains untraced.
(C 0x059fc5, 0x06fc50–0x0709b5, 0x08ce48–0x08ce94,
0x09c256–0x09c26d, 0x0dc8e0–0x0dcd48. S 0x0272d0–0x027738.)

The sender initializes its request timestamp with the current WinMM
timeGetTime value at thread entry. On each network turn it takes copies
of the packet-11 list at +0x2cc and the packet-12 list at +0x2d4 under game
critical section +0x8005d0, then releases the lock before the rate test.
It subtracts the timestamp from the current timeGetTime value in 32 bits
and compares the result as signed. If the difference is strictly greater
than 1,000 ms, it stores the current sample and sends the packet-11
snapshot in order, followed by the packet-12 snapshot. It advances the
timestamp even if both snapshots are empty. A packet-11 entry is a
four-byte type 11 and two signed 32-bit zone coordinates. The sender
destroys its copies after the turn. Taking and sending the copy does not
consume the live request list or clear the zone latch. Continued
eligibility can produce a later request for the same zone.
(C 0x069c60–0x069c6e, 0x06a213–0x06a37a, 0x059250,
0x06a380 onward, frame lock 0x09cedb–0x09cee5 and 0x09d2d7–0x09d2de.)

The server queues received packet-11 pairs under the connection's request
lock, then copies and clears those request lists in distribution. Each
pair queries one zone. For each valid requested loaded zone, it adds stored
block edits to section 1, static states to section 6 and a complete item
snapshot to section 7. It adds the item snapshot even if the item list is
empty. The request handler supplies zero section-8 snapshots. An absent
zone gets no response from this request.
(S 0x0265e0–0x026645, 0x027823–0x02790d, 0x027913–0x027ca8.)

On the client, applying a section-7 item snapshot to a loaded zone
zeros +0x74. A section-6 update also zeros it after a valid static
index resolves and is updated. Section 8 clears it as described above.
The ordinary section-7 reply can therefore satisfy the latch even for
a zone with no items or placed-object update. The traced section-8
receive path can update a loaded zone regardless of the current latch.
(C 0x08d5f0–0x08d6d8, 0x08f435–0x08f4f6, 0x08f58d–0x08f677.)

Stock build-mode entry, stock model-selection input and live delivery
timing across concurrent frames and socket turns remain open.
