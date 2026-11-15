Status: FINAL / STABLE / HELD

# Alpha multiplayer: packets and records

Behavioural specification of the messages exchanged by the Cube World **Alpha** client
(`Cube.exe`) and server (`Server.exe`) after the handshake. Framing (u32 packet id, then
payload), byte order (little-endian), the zlib usage and the handshake itself are described
in `alpha-net-transport.md`. Routines are cited as relative virtual addresses (RVAs) of the
named image; both images have image base 0x400000. Record offsets are byte offsets within
the record as it appears on the wire. Where a field's meaning is not known the field is
still given with its exact offset, size and type and marked *unknown*.

---

## 0. Packet table

| Id | Direction | Name used here | Payload | Sent by Alpha? |
|---|---|---|---|---|
| 0 | both | creature update | u32 length + zlib(u64 id, creature delta) | yes (the handshake also uses id 0 with an uncompressed whole record) |
| 1 | S→C | uncompressed creature batch | see 3.2 | no (the client still understands it) |
| 2 | S→C | end of creature batch | none | yes, every server tick |
| 3 | S→C | static-object list | u32 count + count × 120 bytes | no in-game (the client understands it; id 3 is also a handshake reply) |
| 4 | S→C | world update | u32 length + zlib(13 sections) | yes, every server tick |
| 5 | S→C | clock | u32 day, u32 time of day (ms) | yes, every server tick |
| 6 | C→S | item record | 300 bytes | yes |
| 7 | C→S | hit | 72 bytes | yes |
| 8 | C→S | "event 40" | 40 bytes | yes |
| 9 | C→S | shoot | 112 bytes | yes |
| 10 | both | chat | C→S: i32 n + n UTF-16 units; S→C: u64 sender id + i32 n + n UTF-16 units | yes |
| 11 | C→S | zone request | i32 zone x, i32 zone y | yes |
| 12 | C→S | map region request | i32 region x, i32 region y | yes |
| 15 | S→C | world seed | u32 seed | handshake only |
| 16, 17, 18, 3 | handshake | see `alpha-net-transport.md` | | |

The client's receive loop (Cube.exe RVA 0x6b740; id-to-case byte table at RVA 0x6d1a4,
case address table at RVA 0x6d180) handles ids 0, 1, 2, 3, 4, 5, 10 and 15. Ids 6–9,
11–14 and every id ≥ 16 consume only the 4-byte id and the loop reads the next id, so any
payload bytes would then be interpreted as ids. The server's receive loop (Server.exe RVA
0x26020) handles ids 0 and 6–12; any other id likewise consumes only the 4-byte id.

For packet 0 in either direction, a compressed length of 0 makes the receiver treat the
connection as closed (Cube.exe RVA 0x6bb1c, Server.exe RVA 0x26020 case 0).

### 0.1 Per-tick order

Server, per connection (Server.exe RVA 0x23dd0): one packet 0 per listed creature, then
packet 2, then packet 5, then packet 4, then at most one packet 10 (only when a chat line
is queued for that connection; one queued line is taken per tick). The connection loop then
waits until at least 20 ms have passed since its previous tick (RVA 0x250f5).

Client (Cube.exe RVA 0x69c10): packet 0 (the local player), then at most one packet 10
(the front of the outgoing chat queue), then at most one packet 6 (front of the item
queue), then every queued packet 7, then every packet 8, then every packet 9; packets 11
and 12 follow only when more than 1000 ms have passed since they were last sent. The loop
then waits until at least 20 ms have passed since its previous tick.

---

## 1. Shared record types

### 1.1 Item (280 bytes, 0x118)

Used inside creature records (one extra slot and 13 equipment slots), world items,
section 9 records and packet 6. Layout from the item equality test (Server.exe RVA 0x78f0),
which compares exactly the fields listed below and nothing else:

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 1 | u8 | item type (0 = empty slot) |
| 0x01 | 1 | u8 | item subtype within the type |
| 0x02 | 2 | — | not compared |
| 0x04 | 4 | u32 | *unknown* (compared) |
| 0x08 | 4 | u32 | *unknown* (compared) |
| 0x0c | 1 | u8 | *unknown* (compared) |
| 0x0d | 1 | u8 | *unknown* (compared) |
| 0x0e | 1 | u8 | *unknown* (compared) |
| 0x0f | 1 | — | not compared |
| 0x10 | 2 | i16/u16 | *unknown* (compared; the client's world-item reader defaults it to 1; item level is the likely meaning) |
| 0x12 | 2 | — | not compared |
| 0x14 | 256 | 32 × 8 bytes | sub-entry list; entry *k* is compared on its byte 3 first, then on the rest of the entry |
| 0x114 | 4 | i32 | number of used sub-entries (0–32) |

Two items are equal when every compared field is equal, the counts are equal and the first
*count* sub-entries are equal. Bytes not compared, and sub-entries beyond *count*, never
make two items unequal.

Type values with client behaviour noted in this document: 13 and 25 (section 9). For a new
player the server writes type 3, subtype 2 and byte 0x0d = 1 into equipment slots 6 and 7
(Server.exe RVA 0x25a6a–0x25a81).

### 1.2 Creature appearance (172 bytes, 0xac)

Compared field by field by Server.exe RVA 0x15750:

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 2 | 2 × u8 | *unknown* |
| 0x02 | 3 | 3 × u8 | *unknown* (a colour is likely) |
| 0x05 | 1 | — | not compared |
| 0x06 | 2 | u16 | *unknown* |
| 0x08 | 12 | 3 × f32 | collision box size |
| 0x14 | 16 | 8 × u16 | body-part model indices (which part is which is unknown) |
| 0x24 | 40 | 10 × f32 | body-part scalars (unknown individually) |
| 0x4c | 12 | 3 × f32 | *unknown* vector |
| 0x58 | 12 | 3 × f32 | three scalars |
| 0x64 | 12 | 3 × f32 | body-part offset |
| 0x70 | 12 | 3 × f32 | body-part offset |
| 0x7c | 12 | 3 × f32 | body-part offset |
| 0x88 | 12 | 3 × f32 | body-part offset |
| 0x94 | 12 | 3 × f32 | body-part offset |
| 0xa0 | 12 | 3 × f32 | body-part offset |

For a new player the server sets the collision box to 0.8 × 1.5, 0.8 × 1.5, 1.8 × 1.5
(constant 1.5 at Server.exe RVA 0x1586e8) and then fills the appearance from the creature
type (record offset 0x54) with a generator routine (Server.exe RVA 0xa840).

---

## 2. Packet 0 and the creature record

### 2.1 The creature record (4456 bytes, 0x1168)

Every creature is described by one 4456-byte record. In both programs the record sits 16
bytes into the creature object, with the creature's u64 id at object offset 8. The handshake
sends this record whole; packet 0 sends a delta of it.

The record is cut into **48 fields**; field *n* has change-mask bit *n* (Server.exe RVA
0x16210, Cube.exe RVA 0xccfa0). Bits 41 and 42 are in the opposite order to their offsets.

| Bit | Off | Size | Type | Meaning |
|---|---|---|---|---|
| 0 | 0x000 | 24 | 3 × i64 | position (1/65536 block) |
| 1 | 0x018 | 12 | 3 × f32 | *unknown* (orientation is likely; zero for a new player) |
| 2 | 0x024 | 12 | 3 × f32 | velocity (hit knock-back is added here; zero for a new player) |
| 3 | 0x030 | 12 | 3 × f32 | *unknown* |
| 4 | 0x03c | 12 | 3 × f32 | *unknown* |
| 5 | 0x048 | 4 | f32 | *unknown* |
| 6 | 0x04c | 4 | i32 | *unknown* (0 for a new player) |
| 7 | 0x050 | 1 | u8 | controller kind: 0 for a player (the server sets 0 for a new player); the server applies the NPC visibility radius only when non-zero |
| 8 | 0x054 | 4 | i32 | creature type (44 for the server's new player; input to the appearance generator) |
| 9 | 0x058 | 1 | u8 | *unknown* (the client writes 0x6d here when a type-25 item arrives, §3.6) |
| 10 | 0x05c | 4 | i32 | *unknown* (the client writes 0 here in the same case) |
| 11 | 0x060 | 4 | i32 | *unknown* |
| 12 | 0x064 | 4 | i32 | *unknown* |
| 13 | 0x068 | 172 | appearance | §1.2 |
| 14 | 0x114 | 2 | u16 | *unknown* |
| 15 | 0x118 | 4 | i32 | *unknown* |
| 16 | 0x11c | 4 | i32 | raised by damaging hits to at least hit field 0x18 (§5.1) |
| 17 | 0x120 | 4 | i32 | *unknown* |
| 18 | 0x124 | 4 | i32 | *unknown* |
| 19 | 0x128 | 4 | i32 | *unknown* |
| 20 | 0x12c | 4 | f32 | *unknown* |
| 21 | 0x130 | 1 | u8 | *unknown* (class is the likely meaning; 1 for the server's new player) |
| 22 | 0x131 | 1 | u8 | *unknown* (class specialization is the likely meaning) |
| 23 | 0x134 | 4 | f32 | gauge raised by kind-1 hits, clamped to field 28 |
| 24 | 0x138 | 12 | 3 × f32 | *unknown* |
| 25 | 0x144 | 12 | 3 × f32 | *unknown* |
| 26 | 0x150 | 12 | 3 × f32 | *unknown* |
| 27 | 0x15c | 4 | f32 | **health** (damage is subtracted here, clamped to 0 and to the maximum; the new player gets the maximum) |
| 28 | 0x160 | 4 | f32 | gauge clamped to at most 1.0 by kind-1 hits |
| 29 | 0x164 | 4 | f32 | gauge clamped to at least −1.0 by kind-1 hits |
| 30 | 0x168 | 20 | 5 × f32 | *unknown* |
| 31 | 0x17c | 1 | u8 | *unknown* |
| 32 | 0x17d | 1 | u8 | *unknown* |
| 33 | 0x180 | 4 | i32 | **level** (1 for a new player; used by the damage formula, §5.1) |
| 34 | 0x184 | 4 | i32 | experience is the likely meaning (type-13 items add to it, §3.6) |
| 35 | 0x188 | 8 | u64 | *unknown* 64-bit value |
| 36 | 0x190 | 8 | u64 | *unknown* 64-bit value |
| 37 | 0x198 | 1 | u8 | *unknown* |
| 38 | 0x19c | 4 | i32 | *unknown* |
| 39 | 0x1a0 | 12 | 3 × i32 | *unknown* |
| 40 | 0x1b0 | 24 | 3 × i64 | a second position; the server copies the spawn position here for a new player, and a change of it makes the client snap the creature's display position (§2.3) |
| 41 | 0x1cc | 12 | 3 × i32 | *unknown* |
| 42 | 0x1c8 | 1 | u8 | *unknown* |
| 43 | 0x1d8 | 280 | item | an item slot outside the equipment (which one is unknown) |
| 44 | 0x2f0 | 3640 | 13 × item | equipment slots 0–12 (§1.1) |
| 45 | 0x1158 | 16 | char[16] | name, NUL-terminated (compared as a C string, always sent as 16 bytes; the client's `/name` command writes it) |
| 46 | 0x1128 | 44 | 11 × i32 | *unknown* (skill levels are the likely meaning) |
| 47 | 0x1154 | 4 | i32 | *unknown* |

Record bytes not covered by any field (0x051–0x053, 0x059–0x05b, 0x116–0x117,
0x132–0x133, 0x17e–0x17f, 0x199–0x19b, 0x1ac–0x1af, 0x1c9–0x1cb) are never transmitted by
packet 0. Mask bits 48–63 are always zero.

### 2.2 The delta encoding

Writer: Server.exe RVA 0x16210 and the same algorithm at Cube.exe RVA 0xcdaa0. Reader:
Cube.exe RVA 0xccfa0 and the same algorithm at Server.exe RVA 0x15dd0.

Plain payload:

    u64 entity id
    u64 change mask
    then, for n = 0 … 47 with mask bit n set, field n's bytes (size from the table)

* A field is included when its new value differs from the sender's reference value for it,
  using a typed comparison: integers and bytes by value; floats and float triples as floats
  (so +0.0 equals −0.0 and a NaN always differs); i64 triples and u64 values by value; the
  appearance by the field comparison of §1.2; the single item and the 13 equipment items by
  the item equality test of §1.1; the name as a C string; the 11 skill words by value.
* In full mode every field is included and the mask is 0x0000FFFFFFFFFFFF.
* The data of an included field is always the **new** value.
* The receiver starts from its stored copy of that creature's record and overwrites the
  included fields.
* Framing: the plain payload is zlib-compressed and sent as u32 packet id 0, u32 compressed
  length, compressed bytes.

Reference values:

* Server to a given client (Server.exe RVA 0x23dd0): the reference is the record of that
  creature as it stood when it was listed to this connection in the previous tick. A
  creature that was not listed to this connection in the previous tick is sent in full
  mode.
* Client to server (Cube.exe RVA 0x69c10): the client's first update after connecting is
  in full mode; each later update is a delta against the record it sent in the previous
  tick.

### 2.3 Who is authoritative

* **Client → server.** The server reads and discards the u64 id of the client's packet and
  applies the delta to the creature that belongs to that connection, writing the result
  back without further checks in the receive routine (Server.exe RVA 0x26020 case 0).
* **Server → client.** An update whose id is the local player's id is decoded and then
  discarded (Cube.exe RVA 0x6bcc5); every other update is committed to that creature,
  creating the creature if its id is unknown.
* On the client, a committed update either snaps the creature's display position or sets a
  60-step interpolation counter. It snaps when the creature was created by this update,
  when the field at record 0x1b0 (bit 40) changed, or when the new health is ≤ 0; otherwise
  it interpolates (Cube.exe RVA 0x6bcd1–0x6be04). This is presentation only.

### 2.4 Which creatures are listed, and packet 2

For each connection, every tick, the server lists creatures from its world map
(Server.exe RVA 0x23f00–0x240d2):

* the connection's own creature is never listed to it;
* a creature whose controller kind (bit 7) is non-zero is skipped when its squared distance
  to the connection's creature exceeds 40,000 blocks² (constant at Server.exe RVA
  0x158924; positions converted with 1/65536 at RVA 0x15869c); players are listed at any
  distance;
* among the creatures that pass the rules above, one that was listed to this connection in
  the previous tick is listed again; at most one creature not listed in the previous tick is
  added per tick, and none during the first 500 ms after the connection loop started.

Each listed creature produces one packet 0 per tick, with an empty mask (u64 id followed by
eight zero bytes) when nothing changed.

Packet 2 has no payload. On receipt the client deletes every creature it holds that was not
updated since the previous packet 2, except its own player (Cube.exe RVA 0x6be25).

---

## 3. Other server → client packets

### 3.1 Packet 5: clock

    u32 day number
    u32 time of day in milliseconds

Sent every tick. The first value is the server world's day counter and the second its
time of day (Server.exe RVA 0x248e4–0x248f0 and 0x24c80–0x24cf1). The server advances the
time of day and, while it exceeds 86,400,000, subtracts 86,400,000 and increments the day
(Server.exe RVA 0x13244c–0x1324a2), so the sent time lies in 0 … 86,400,000. The client
stores the day at game-object offset 0x800444 and the time at 0x800440 (Cube.exe RVA
0x6cb42).

### 3.2 Packet 1: uncompressed creature batch (not sent)

Client reader at Cube.exe RVA 0x6c296. Framing: u32 *count*, then *count* entries, each a
u64 entity id followed by an uncompressed u64 change mask and the included fields in the
order and sizes of §2.1, all read directly from the socket (Cube.exe RVA 0xcd3e0). The
entries are decoded onto copies of the current records; then every creature other than the
local player that was not in the batch is deleted (a *count* ≤ 0 therefore deletes all of
them); then the decoded records are committed, the local player's excluded, with the same
snap/interpolate choice as §2.3. Alpha's server never sends id 1.

### 3.3 Packet 3: static-object list (not sent in game)

Client reader at Cube.exe RVA 0x6bfb0. u32 *count*; when *count* ≤ 0 nothing else is read
and nothing changes. Otherwise *count* × 120-byte records follow. The client keeps a map of
objects keyed by a record's first 8 bytes. It zeroes the first 8 bytes of every object it
holds, creates or updates the object for each record from the record, then deletes every
object whose first 8 bytes are still zero. Alpha's server collects objects within a squared
distance of 262,144 blocks² (constant at Server.exe RVA 0x15b23c) of each connection's
creature every tick (Server.exe RVA 0x240d8–0x2440c) but never sends packet 3 after the
handshake.

### 3.4 Packet 4: world update

    u32 compressed length, zlib stream; inflated content = 13 sections in this order

Every section starts with a u32 *count*. Sections 7 and 8 are keyed lists (u32 count of
entries; each entry an 8-byte key, then u32 *n*, then *n* records); all others are arrays
of fixed-size records. Record bytes are the server's in-memory structs copied whole.

Server side: the world simulation produces 13 output lists each tick; a distribution
routine (Server.exe RVA 0x272d0) appends them to every connection's 13 outgoing lists,
filtering two of them by distance from that connection's creature (squared distance
< 40,000 blocks²), and adds answers to the connection's zone and region requests (§4.4).
The connection's next send serializes the lists in section order (Server.exe RVA
0x24d30–0x24fa6) and clears them. When every section is empty the inflated content is 52
zero bytes; packet 4 is sent every tick regardless.

Client side: the receive case (Cube.exe RVA 0x6cc0e) parses all 13 sections (readers at
Cube.exe RVA 0x58310, 0x583f0, 0x58860, 0x58940, 0x581d0, 0x58780, 0x58a20, 0x58100,
0x58500, 0x57f70, 0x57f70, 0x58030, 0x58650) and appends them to pending lists; the client
frame routine (Cube.exe RVA 0x88ee0) applies them.

| # | Record size | Server source | Filter | Client action |
|---|---|---|---|---|
| 1 | 20 | world output, plus the stored block edits of each requested zone | none | set or clear world blocks (§3.5) |
| 2 | 72 | world output; includes hits relayed from packet 7 (§4.2) | none | applied to the local player if it is the target (§5.1); other hits whose attacker is not the local player go to a client-side list |
| 3 | 72 | world output | squared distance of the i64 triple at record offset 0 | appended to a client-side list; not passed through the hit handling of §5.1 |
| 4 | 24 | world output | squared distance of the f32 triple (blocks) at record offset 0 | appended to a client-side list (use not traced) |
| 5 | 112 | shoot records received as packet 9, moved unchanged (Server.exe RVA 0x281d0) | none | records whose first 8 bytes are not the local player's id are added to the client's projectile list (Cube.exe RVA 0x9034a–0x903ff) |
| 6 | 88 | world output, plus the static-entity states of each requested zone (§3.7) | none | update the zone's static entities (§3.7) |
| 7 | keyed: u64 zone key + u32 *n* + *n* × 328 | world output, plus the world items of each requested zone (§3.8) | none | replace that zone's item list (Cube.exe RVA 0x8d5f0) |
| 8 | keyed: u64 zone key + u32 *n* + *n* × 16 | world output | none | replace the list at offset 0x3c of that zone with the records (Cube.exe RVA 0x8f58d) |
| 9 | 288 | world output | none | §3.6 |
| 10 | 24 | world output | none | each record is handed to a world routine (Cube.exe RVA 0x1a0bf0); meaning unknown |
| 11 | 24 | world output | none | u64 *A* at 0x00, u64 *B* at 0x08, f32 at 0x10, 4 bytes at 0x14 unknown: when creatures *A* and *B* both exist and differ, the f32 is stored in a per-creature map of *A* (object offset 0x13a4) under key *B* (Cube.exe RVA 0x8fbb7) |
| 12 | 40 | world output (whether packet-8 records are relayed here is unresolved) | none | §4.2: applied to the creature whose id is at record 0x08 (Cube.exe RVA 0x8fcb8) |
| 13 | 56 | world output, plus the map-cell states of each requested region (§3.9) | none | update the map cell (§3.9) |

The keyed sections use as key two i32 (zone x, zone y) and the client looks the zone up by
them.

### 3.5 Section 1 record: block edit (20 bytes)

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 12 | 3 × i32 | block coordinates (world block units) |
| 0x0c | 3 | 3 × u8 | colour R, G, B |
| 0x0f | 1 | u8 | block kind; **0 = remove the block**, non-zero = place a block of this kind with the colour |
| 0x10 | 4 | — | *unknown* |

The client applies every record, tracks the smallest and largest x and y coordinates among
them and then requests a rebuild of that horizontal x/y rectangle (Cube.exe RVA
0x8f242–0x8f3d5).

### 3.6 Section 9 record (288 bytes)

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 8 | u64 | creature id |
| 0x08 | 280 | item | an item (§1.1) |

Client behaviour (Cube.exe RVA 0x8f719–0x8fac6): when the creature is the local player and
the item type is not 25, the item is added to the local player's inventory. Afterwards,
for each record whose creature exists: when the type is 13 and the id is the local
player's, ten times the item's 16-bit field 0x10 is added to the local player's record
field 34 (record 0x184); when the type is 25, a per-player counter is incremented, record
offset 0x58 is set to 0x6d and 0x5c to 0, and a "mana cube" message is shown (with a
"skill point" message on every fourth); for other types a further client routine is
called with the item and the creature.

### 3.7 Section 6 record: static-entity state (88 bytes)

The server builds one record per entry of a requested zone's list of 392-byte (0x188)
static entities (Server.exe RVA 0x272d0). "Entity word *k*" is the *k*-th 4-byte word of
that 392-byte entry; record bytes 0x10–0x57 are the entry's first 0x48 bytes at the same
relative positions.

| Off | Size | Content |
|---|---|---|
| 0x00 | 8 | zone x, zone y (2 × i32, as requested) |
| 0x08 | 4 | index of the entity in the zone's static-entity list |
| 0x0c | 4 | not written by the server |
| 0x10 | 4 | entity word 0 |
| 0x14 | 4 | not written by the server (entity word 1 position) |
| 0x18 | 28 | entity words 2–8 |
| 0x34 | 12 | entity words 9–11 |
| 0x40 | 1 | entity byte 0x30 |
| 0x41 | 3 | not written by the server |
| 0x44 | 8 | entity words 13–14 |
| 0x4c | 4 | not written by the server (entity word 15 position) |
| 0x50 | 8 | entity words 16–17 |

Client (Cube.exe RVA 0x8f435): it finds the zone by (x, y); when the zone exists and
0 ≤ index < the zone's static-entity count, it copies exactly the written words and the
byte above into that entity (record offset − 0x10 = entity offset) and marks the zone for
update. The bytes marked "not written" are not read by the client.

### 3.8 Section 7 record: world item (328 bytes)

Keyed by the zone (two i32, zone x and zone y), then u32 *n*, then *n* records:

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x000 | 280 | item | the item (§1.1) |
| 0x118 | 24 | — | *unknown* (no client default; a 3 × i64 position is the likely meaning) |
| 0x130 | 4 | — | *unknown* (no client default) |
| 0x134 | 4 | f32 | *unknown* (client default 1/14) |
| 0x138 | 1 | u8 | *unknown* (client default 0) |
| 0x139 | 3 | — | *unknown* |
| 0x13c | 12 | 3 × i32 | *unknown* (client defaults 0, 0, −1) |

Defaults are those the client's reader (Cube.exe RVA 0x58a20) assigns before copying each
record; the copy overwrites all 328 bytes.

### 3.9 Section 13 record: map cell state (56 bytes)

For a requested map region (*rx*, *ry*) the server walks its 8 × 8 map cells (cell
x = 8·*rx* + *i*, cell y = 8·*ry* + *j*, each < 8192) and, for every cell whose region is
loaded, emits one record built from the cell's 104-byte (0x68) state struct (Server.exe RVA
0x272d0, second half):

| Off | Size | Content |
|---|---|---|
| 0x00 | 4 | cell x (i32) |
| 0x04 | 4 | cell y (i32) |
| 0x08 | 8 | zero when built by the server |
| 0x10 | 20 | cell words at 0x2c, 0x30, 0x34, 0x38, 0x3c |
| 0x24 | 1 | cell byte 0x40 |
| 0x25 | 1 | cell byte 0x41 |
| 0x26 | 2 | not written by the server |
| 0x28 | 16 | cell words at 0x44, 0x48, 0x4c, 0x50 |

So record offset = cell offset − 0x1c for bytes 0x10–0x37. The client (Cube.exe RVA
0x8fdc3) finds the cell by (x, y), copies exactly those words and bytes into the cell at
the same cell offsets, and marks the cell's region (x ÷ 8, y ÷ 8) for update. A later
client step reads byte 0x25 and the u64 at 0x08 of these records (Cube.exe RVA
0x90690–0x907dc); with byte 0x25 = 2 it shows an "objective" message.

### 3.10 Packet 10 (S→C): chat, and packet 15: world seed

Packet 10: u64 sender entity id, i32 *n* (UTF-16 code units, no terminator), *n* × 2
bytes. When *n* ≤ 0 nothing more is read; otherwise the client appends the line to its
chat log (Cube.exe RVA 0x6ca1e).

Packet 15: u32 seed. The client names the world `online_` followed by the seed in decimal
and, unless its current world already has that seed and name, switches to it (Cube.exe
RVA 0x6b8be).

---

## 4. Client → server packets

### 4.1 Packet 6: item record (300 bytes)

The client sends one record per tick while the local player's queue at creature-object
offset 0x130c is non-empty, taking the front entry and removing it (Cube.exe RVA 0x69c10).
The server appends the record to the queue at offset 0x130c of the sending connection's
creature (Server.exe RVA 0x26020 case 6); its consumer is not traced.

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x000 | 280 | item | the item |
| 0x118 | 16 | 4 × 4 bytes | *unknown* |
| 0x128 | 1 | u8 | *unknown* |
| 0x129 | 1 | — | *unknown* |
| 0x12a | 2 | u16 | *unknown* |

### 4.2 Packets 7, 8, 9: events produced by the local player

The client's simulation produces hit, "event 40" and shoot records. When a connection
exists, every record of those three kinds whose first 8 bytes equal the local player's id
is queued for sending (Cube.exe RVA 0x9c02f–0x9c256) and sent the next tick, one packet per
record. The server appends hits, event-40 records and shoots to three server-wide lists
(Server.exe RVA 0x26020 cases 7, 8, 9), which its main loop (Server.exe RVA 0x149c50)
drains once per world tick:

* Shoots are moved unchanged into world-update section 5 for every connection.
* Hits are given to the world simulation (Server.exe RVA 0xcea80). A hit whose target
  (record 0x08) exists with health > 0, is a player (controller kind 0) and arrives while a
  world flag (world object offset 0xb8) is zero is copied unchanged into section 2; other
  hits are processed by the server.
* Event-40 records are applied by the server to the creature whose id is at record 0x08,
  using the bytes from 0x10 (Server.exe RVA 0x132b92–0x132c4b).

Record layouts:

* Packet 7, hit, 72 bytes: §5.1.
* Packet 8, 40 bytes: u64 source id, u64 target id, then a 24-byte status entry of the
  kind kept in the creature-object list at offset 0x1178 (first byte: entry kind; f32 value
  at entry offset 4). The client builds such records itself (§5.1, kind 5) and applies
  section-12 records to the target creature.
* Packet 9, shoot, 112 bytes: u64 shooter id, then the projectile state
  (`alpha-projectiles.md` §1).

### 4.3 Packet 10 (C→S): chat

i32 *n* (UTF-16 code units), *n* × 2 bytes. For *n* > 0 the server prefixes the sending
connection's id and queues the line for **every** connection, the sender included
(Server.exe RVA 0x26020 case 10); *n* ≤ 0 is ignored. A typed line is queued for sending
only when a connection exists; the commands `/name`, `/connect`, `/disconnect` and
`/namepet` are handled locally and never sent (Cube.exe RVA 0x7e3a5–0x7e640).

### 4.4 Packets 11 and 12: data requests

The server appends each request to the connection's request list (no merging of
duplicates; each copy is answered) and answers the lists at the next distribution, then
clears them (Server.exe RVA 0x272d0).

* 11: i32 zone x, i32 zone y. The server accepts 0 ≤ x, y < 65,536, finds the region
  (zone ÷ 64, which must be < 1024 on both axes) and the zone within it (zone mod 64) and,
  if that zone exists on the server, answers with the zone's stored block edits
  (section 1), its world items (section 7) and its static-entity states (section 6). If the
  zone does not exist nothing is sent.
* 12: i32 region x, i32 region y (one region = 8 × 8 map cells = 64 × 64 zones). The server
  answers with the map-cell records of that region (section 13) for every cell whose region
  exists.

---

## 5. Records shared by both directions

### 5.1 Hit (72 bytes)

Fields used by the client's handling of incoming hits (Cube.exe RVA 0x8fecb–0x90344) and
by the sending filter:

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 8 | u64 | attacker id |
| 0x08 | 8 | u64 | target id; only hits whose target is the local player are applied |
| 0x10 | 4 | f32 | amount |
| 0x14 | 4 | — | *unknown* |
| 0x18 | 4 | i32 | when positive, the target's record field 16 is raised to at least this |
| 0x1c | 28 | — | *unknown* |
| 0x38 | 12 | 3 × f32 | knock-back vector, added to the target's velocity |
| 0x44 | 1 | u8 | *unknown* |
| 0x45 | 1 | u8 | hit kind (below) |
| 0x46 | 2 | — | *unknown* |

Client behaviour for a hit on the local player, by kind:

* Kind 1: no health loss. If the amount is positive the knock-back is added to the
  velocity and a client-only value at creature-object offset 0x1184 is set to 1.0. Then the
  knock-back is added once more; field 28 is raised by field 29 and clamped to at most 1.0;
  field 23 is raised by field 29 and clamped to at most field 28; field 29 is lowered by
  amount ÷ a value computed from the player (Cube.exe RVA 0x3e190) and clamped to at least
  −1.0 (constants at Cube.exe RVA 0x345dc0 and 0x345e8c).
* Kind 5: for every entry of kind 6 in the status list at creature-object offset 0x1178,
  the entry's value (entry offset 4) is reduced by the amount; when the result is ≤ 0 the
  value and the word at entry offset 8 are set to zero. For each such entry the client
  builds a 40-byte record (local player id, local player id, the 24-byte entry) in the
  frame-local list from which packet 8 records are taken.
* Any other kind (0, 2, 3, 4, …), only if the player's health is > 0: when the amount is
  positive, a client-only resource at creature-object offset 0x1190 is reduced by
  amount × 0.25 ÷ 2^(0.25 × (level − 1)) and clamped to at least 0 (constant 0.25 at
  Cube.exe RVA 0x345d98; power routine at Cube.exe RVA 0x11d10); when hit field 0x18 is
  positive, record field 16 is raised to it; then health (field 27) is reduced by the
  amount and clamped to at least 0 and at most the player's maximum.

The amount comes from the attacking client's record; the server copies relayed hits
unchanged (§4.2) and the target client applies them under the conditions above only.

---

## 6. Unresolved

* Meanings of the item fields 0x04, 0x08, 0x0c–0x0e, 0x10 and of the 8-byte sub-entries.
* Names of the appearance fields.
* Meanings of the creature fields marked *unknown* (bits 1, 3–6, 9–12, 14, 15, 17–22,
  24–26, 30–32, 34–39, 41–43, 46, 47), including whether bit 21 is the class, bit 34
  experience and bit 46 skill levels.
* The initial contents of a creature the client creates for a previously unknown id.
* Packet 3's 120-byte record fields.
* What world-update sections 3 and 4 carry and how the client uses them.
* Section 6 entity words and the entity byte 0x30.
* Section 7 world-item fields after the item (0x118–0x147).
* Section 8 16-byte record and section 10 and 11 meanings.
* Section 13 cell words and the use of bytes 0x08–0x0f.
* Packet 6 purpose and its server-side consumer.
* Whether, and how, packet-8 records reach section 12 for other clients; what else the
  world simulation places in sections 2, 12 and the other output lists; the meaning of the
  server world flags at offsets 0xb4 and 0xb8.
* Hit bytes 0x14, 0x1c–0x37, 0x44 and 0x46–0x47; the value computed at Cube.exe RVA
  0x3e190; hit kinds other than 1 and 5 beyond the default path.
* Cross-references to the world, zone and character save formats (`alpha-saves.md`) were
  not re-checked for this contract.

---

## Re-certification (2026-11-15)

Checked against the original Cube.exe and Server.exe (disassembly of the image bytes and
the decompiled listings).

**Confirmed:** packet ids and directions; client receive dispatch (handled ids, byte and
address tables) and server receive dispatch; packet 0 framing and the 48-field table
(offsets, sizes, bit order including the 41/42 inversion, full mask); delta writer/reader
pairing; server ignores the client's id; client discards updates for its own id; packet 2
removal; packet 5 order (day, then time) and client storage; packet 3 and packet 1 never
sent; world-update section order and all 13 record sizes; 200-block (40,000) and 512-block
(262,144) constants; static-entity record placement; block-edit layout; item equality
fields; appearance comparison layout; new-player values (type 44, level 1, controller 0,
class byte 1, field 6 zero, box 1.2/1.2/2.7, spawn copy at 0x1b0); packet 6 layout and
queue; packet 7/8/9 forwarding filter; chat formats and server fan-out including the
sender; request coordinate rules; hit fields 0x08, 0x10, 0x18, 0x38, 0x45 and kind-1
behaviour; seed handling and `online_` naming.

**Corrected (old → new):**
1. Delta inclusion "byte comparison" → typed per-field comparison; items and equipment use
   the item equality test and the appearance its field comparison, so uncompared bytes
   never set a bit. *Wire-affecting* (mask contents).
2. Reference value "previous state sent to this receiver" → the record as listed in the
   previous tick; a creature not listed in the previous tick is sent in full. *Wire-affecting.*
3. Creature listing: added the one-new-creature-per-tick limit, the initial 500 ms, players
   at any distance, NPC radius ≤ 40,000. *Wire-affecting* (which updates are sent).
4. Display snap condition "moved far" → created now, field at 0x1b0 changed, or health ≤ 0.
5. Packet 1 framing (formerly open): u32 count, then per creature u64 id, u64 mask and
   fields, uncompressed; count ≤ 0 deletes all other creatures. *Wire-affecting* (layout).
6. Packet 3: count ≤ 0 changes nothing; deletion works by zeroing and re-setting the first
   8 bytes.
7. Clock range 0 … 86,399,999 → 0 … 86,400,000. *Wire-affecting* (value range).
8. **Reversal:** section 3 client action "as section 2" → not passed through the hit
   handling; only appended to a client-side list.
9. **Reversal:** "event 40 passed on to every client in section 12" → applied by the server
   to the target creature; relay unresolved. Hits: relayed only for a living player target
   while world flag 0xb8 is zero (was: all hits feed sections 2/3). Shoots: confirmed moved
   unchanged into section 5.
10. Packet 8 layout "u64 source + 32 unknown bytes" → u64 source, u64 target, 24-byte status
    entry. *Wire-affecting* (layout).
11. Section 9 "unknown, 8 + 280 likely" → u64 creature id + item, with the client behaviour
    for types 13 and 25.
12. Section 13 placement (formerly open) → resolved: 0x08 eight zero bytes, 0x10–0x37 = cell
    0x2c–0x53, 0x26–0x27 not written. *Wire-affecting* (layout).
13. Section 6 "padding" at 0x0c, 0x14, 0x41–0x43, 0x4c → not written by the server and not
    read by the client; client copy mapping stated.
14. Sections 8 and 11 client actions (formerly unknown) stated; section 5 skip of the local
    player's own shoots added.
15. Block-edit rebuild "bounding box" → horizontal x/y rectangle.
16. Chat commands: added `/name`; "lines starting with" → recognised commands.
17. Server chat: at most one line per connection per tick, sent after packet 4; per-tick
    order of both programs added. *Wire-affecting* (pacing and order).
18. Kind-5 hit "removes the buff at zero" → zeroes the value and the next word, and builds a
    40-byte record; default path: resource reduced only for a positive amount with the
    stated formula, field 16 raised only for a positive hit field 0x18.
19. Packet 0 with compressed length 0 closes the connection on either side (added).
20. Request handling "duplicates are harmless" → duplicates are each answered.

**Removed:** advice addressed to a replacement (carrying unknown fields, which ids a
replacement may send); the open-questions procedures (send/recv hooks, captures, diffs);
the worked examples and test vectors (§6 of the old text); observed values taken from
saves or recordings (item types seen, "2 seen", "287.0", "0.07–0.10", "0, 0, −1 seen",
"1 in all observed saves"); the yaw interpretation of field 1; "saved with the character"
notes; the "warrior" class name.

**Unresolved:** see §6.
