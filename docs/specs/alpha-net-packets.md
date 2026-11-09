# Alpha multiplayer: packets, records and test vectors

Behavioural specification of every message exchanged by the Cube World **Alpha** client
(`Cube.exe`) and server (`Server.exe`) after the handshake. Read `alpha-net-transport.md`
first: it defines the framing (u32 packet id, then payload), byte order (little-endian),
the zlib usage and the per-tick message order. Routines are cited by address and binary
("client 0x46b740", "server 0x426020"). Record offsets are byte offsets within the record as
it appears on the wire.

Where a field's meaning is not yet known the field is still given with its exact offset,
size and type, and marked *unknown*; an implementation must carry such fields through
unchanged (relay them byte for byte) and send zero when it has to invent one.

---

## 0. Packet table

| Id | Direction | Name used here | Payload | Sent by Alpha? |
|---|---|---|---|---|
| 0 | both | creature update | u32 length + zlib(u64 id, creature delta) | yes (also in handshake, uncompressed form) |
| 1 | S→C | legacy creature batch (uncompressed) | see 3.2 | **no** (client still understands it) |
| 2 | S→C | end of creature batch | none | yes, every tick |
| 3 | S→C | static-object list | u32 count + count × 120 bytes | **no** (client understands it) |
| 4 | S→C | world update | u32 length + zlib(13 lists) | yes, every tick |
| 5 | S→C | clock | u32 day, u32 time of day (ms) | yes, every tick |
| 6 | C→S | item record (300 bytes) | 300 bytes | yes |
| 7 | C→S | hit | 72 bytes | yes |
| 8 | C→S | "event 40" | 40 bytes | yes |
| 9 | C→S | shoot | 112 bytes | yes |
| 10 | both | chat | C→S: u32 n + n UTF-16 units; S→C: u64 sender id + u32 n + n UTF-16 units | yes |
| 11 | C→S | zone request | i32 zone x, i32 zone y | yes, once a second per wanted zone |
| 12 | C→S | map region request | i32 region x, i32 region y | yes, once a second per wanted region |
| 15 | S→C | world seed | u32 seed | handshake only |
| 16, 17, 18, 3 | handshake | see `alpha-net-transport.md` §2 | | |

The client's receive switch (client 0x46b740, jump tables at 0x46d1a4 / 0x46d180) handles
ids 0, 1, 2, 3, 4, 5, 10 and 15; ids 6–9 and 11–14 and ≥16 fall through to "read the next
id" (which desynchronises the stream). The server's receive switch (server 0x426020)
handles 0 and 6–12; any other id is likewise not consumed.

---

## 1. Shared record types

### 1.1 Item (280 bytes, 0x118)

Used inside creature records, inventory, world items, packet 6 and the saves. Layout from
the item equality test (server 0x4078f0, which compares exactly these fields) and from saved
characters:

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 1 | u8 | item type (0 = empty slot). Seen: 3 (weapons), 11 (plants and food lying in the world), 9, 24 |
| 0x01 | 1 | u8 | item subtype within the type |
| 0x02 | 2 | — | padding (zero) |
| 0x04 | 4 | u32 | *unknown* (compared; a per-item value such as a modifier) |
| 0x08 | 4 | u32 | *unknown* (compared) |
| 0x0c | 1 | u8 | *unknown* (rarity is the likely meaning) |
| 0x0d | 1 | u8 | *unknown* (material is the likely meaning; 2 seen on starting weapons) |
| 0x0e | 1 | u8 | *unknown* flags |
| 0x0f | 1 | — | padding |
| 0x10 | 2 | u16 | *unknown* (item level is the likely meaning) |
| 0x12 | 2 | — | padding |
| 0x14 | 256 | 32 × 8 bytes | attached sub-block list; entry *k* is compared byte 3 first, then the rest |
| 0x114 | 4 | i32 | number of used entries in the list above (0–32) |

Two items are equal when every listed field and the first *count* list entries are equal.
The exact meaning of the 8-byte sub-block entries is *unknown* (**open question P-1**).

### 1.2 Creature appearance (172 bytes, 0xac)

Compared field by field by server 0x415750:

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 2 | 2 × u8 | *unknown* |
| 0x02 | 3 | 3 × u8 | *unknown* (a colour is likely) |
| 0x05 | 1 | — | padding |
| 0x06 | 2 | u16 | *unknown* |
| 0x08 | 12 | 3 × f32 | collision box size (server sets 1.2, 1.2, 2.7 for a new player: 0.8·1.5, 0.8·1.5, 1.8·1.5, constant 1.5 at server 0x5586e8) |
| 0x14 | 16 | 8 × u16 | body-part model indices (*which part is which is unknown*) |
| 0x24 | 40 | 10 × f32 | body-part scalars (*unknown individually*) |
| 0x4c | 12 | 3 × f32 | *unknown* vector |
| 0x58 | 12 | 3 × f32 | three scalars |
| 0x64 | 12 | 3 × f32 | body-part offset |
| 0x70 | 12 | 3 × f32 | body-part offset |
| 0x7c | 12 | 3 × f32 | body-part offset |
| 0x88 | 12 | 3 × f32 | body-part offset |
| 0x94 | 12 | 3 × f32 | body-part offset |
| 0xa0 | 12 | 3 × f32 | body-part offset |

The server fills a new player's appearance from its creature type (record offset 0x54) with
a generator routine (server 0x40a840). **Open question P-2**: names of the body-part fields.

---

## 2. Packet 0 and the creature record

### 2.1 The creature record (4456 bytes, 0x1168)

Every creature (player or NPC) is described by one 4456-byte record. It is the in-memory
creature starting 16 bytes into the creature object (the object's u64 id sits just before
it). The handshake sends this record whole; packet 0 sends a delta of it.

The record is cut into **48 fields**. Field *n* has change-mask bit *n*. The table gives each
field's record offset, wire size and type, and what is known of its meaning. Note that bits
41 and 42 are in the opposite order to their offsets.

| Bit | Off | Size | Type | Meaning |
|---|---|---|---|---|
| 0 | 0x000 | 24 | 3 × i64 | position (1/65536 block) |
| 1 | 0x018 | 12 | 3 × f32 | orientation (saved with the character; the third component looks like yaw in radians) |
| 2 | 0x024 | 12 | 3 × f32 | velocity (hit knock-back is added here, client 0x48ff5b) |
| 3 | 0x030 | 12 | 3 × f32 | *unknown* motion vector |
| 4 | 0x03c | 12 | 3 × f32 | *unknown* motion vector |
| 5 | 0x048 | 4 | f32 | *unknown* |
| 6 | 0x04c | 4 | i32 | *unknown* (0 for a new player) |
| 7 | 0x050 | 1 | u8 | controller kind: **0 = player**; non-zero = AI-controlled (the server applies the NPC visibility radius only when non-zero) |
| 8 | 0x054 | 4 | i32 | creature type / race (44 for the server's default new player) |
| 9 | 0x058 | 1 | u8 | *unknown* |
| 10 | 0x05c | 4 | i32 | *unknown* |
| 11 | 0x060 | 4 | i32 | *unknown* |
| 12 | 0x064 | 4 | i32 | *unknown* |
| 13 | 0x068 | 172 | appearance | §1.2 |
| 14 | 0x114 | 2 | u16 | *unknown* |
| 15 | 0x118 | 4 | i32 | *unknown* |
| 16 | 0x11c | 4 | i32 | a hit-driven counter: on a damaging hit the client sets it to max(current, hit field 0x18) |
| 17 | 0x120 | 4 | i32 | *unknown* |
| 18 | 0x124 | 4 | i32 | *unknown* |
| 19 | 0x128 | 4 | i32 | *unknown* |
| 20 | 0x12c | 4 | f32 | *unknown* |
| 21 | 0x130 | 1 | u8 | class (1 = warrior; the server's default new player is 1) |
| 22 | 0x131 | 1 | u8 | class specialization |
| 23 | 0x134 | 4 | f32 | a gauge raised by "type 1" hits and clamped to field 28 |
| 24 | 0x138 | 12 | 3 × f32 | *unknown* |
| 25 | 0x144 | 12 | 3 × f32 | *unknown* |
| 26 | 0x150 | 12 | 3 × f32 | *unknown* |
| 27 | 0x15c | 4 | f32 | **health** (damage is subtracted here, clamped to 0 and to the maximum) |
| 28 | 0x160 | 4 | f32 | a gauge in 0..1 (clamped to 1.0) |
| 29 | 0x164 | 4 | f32 | a gauge in −1..1 (clamped to −1.0) |
| 30 | 0x168 | 20 | 5 × f32 | *unknown* |
| 31 | 0x17c | 1 | u8 | *unknown* |
| 32 | 0x17d | 1 | u8 | *unknown* |
| 33 | 0x180 | 4 | i32 | **level** (1 for a new player) |
| 34 | 0x184 | 4 | i32 | **experience** |
| 35 | 0x188 | 8 | u64 | *unknown* 64-bit value (an entity id is likely) |
| 36 | 0x190 | 8 | u64 | *unknown* 64-bit value (an entity id is likely) |
| 37 | 0x198 | 1 | u8 | *unknown* |
| 38 | 0x19c | 4 | i32 | *unknown* |
| 39 | 0x1a0 | 12 | 3 × i32 | *unknown* (the 4 bytes 0x1ac–0x1af are never sent) |
| 40 | 0x1b0 | 24 | 3 × i64 | home / spawn position (the server copies the spawn position here) |
| 41 | 0x1cc | 12 | 3 × i32 | *unknown* |
| 42 | 0x1c8 | 1 | u8 | *unknown* (bytes 0x1c9–0x1cb are never sent) |
| 43 | 0x1d8 | 280 | item | an item slot outside the equipment (*which one is unknown*) |
| 44 | 0x2f0 | 3640 | 13 × item | equipment slots 0–12 (§1.1) |
| 45 | 0x1158 | 16 | char[16] | name, NUL-terminated (compared as a C string, always sent as 16 bytes) |
| 46 | 0x1128 | 44 | 11 × i32 | skill levels (11 skills) |
| 47 | 0x1154 | 4 | i32 | *unknown* (saved with the character) |

Bytes of the record not covered by a field (for example 0x1ac–0x1af) are never transmitted.
Mask bits 48–63 are always zero.

### 2.2 The delta encoding

Writer: server 0x416210 (client 0x4cdaa0 is the same algorithm). Reader: client 0x4ccfa0
(server 0x415dd0 is the same algorithm).

Plain payload:

    u64 entity id
    u64 change mask
    for n = 0 … 47, if mask bit n is set: field n's bytes (size from the table)

* A field is included when it differs from the previous state the **sender** sent to **this
  receiver**, comparing its bytes; floats are compared as floats, so +0.0 and −0.0 compare
  equal and a NaN always differs. The name is compared as a C string. "Full" mode (a
  creature the receiver has not been sent before, and a client's first update after
  joining) includes every field: mask = 0x0000_FFFF_FFFF_FFFF.
* The data of an included field is always the **new** value.
* The receiver starts from its stored copy of that creature's record (a fresh zeroed record
  for an unknown id) and overwrites the included fields.
* Framing: the plain payload is zlib-compressed and sent as u32 packet id 0, u32 compressed
  length, compressed bytes.

### 2.3 Who is authoritative

* **Client → server.** The id field of the client's packet is ignored: the server applies
  the delta to the creature that belongs to that connection (server 0x426020 case 0). The
  server trusts the client for **everything** in its own record: position, health, level,
  experience, equipment, name, skills. There is no validation.
* **Server → client.** The client applies updates to every creature **except its own
  player**: an update whose id is the local player's id is decoded and discarded (client
  0x46bcc5). A client is authoritative over itself and cannot be corrected by the server
  through packet 0.
* On the client, a creature first seen in a batch, or one that has moved far, has its
  display position snapped; otherwise the new position becomes an interpolation target over
  60 steps (client 0x46bd00–0x46bdf2). This is presentation only.

### 2.4 Packet 2: end of creature batch

No payload. On receipt the client deletes every creature it holds that was **not** updated
since the previous packet 2, except its own player (client 0x46be25). The server therefore
re-lists every visible creature in every batch, with an empty mask (an 8-byte zero) when
nothing changed. See `alpha-net-server.md` §4 for which creatures a server lists.

---

## 3. Other server → client packets

### 3.1 Packet 5: clock

    u32 day number
    u32 time of day in milliseconds (0 … 86,399,999)

Sent every tick. The client stores them in its game object (time at G+0x800440, day at
G+0x800444, client 0x46cb42); the renderer uses the time for daylight
(`alpha-rendering.md` §2). The values are the server world's own clock, which is also what
the world save stores under the key `time` (`alpha-saves.md`).

### 3.2 Packet 1: legacy uncompressed creature batch (not sent)

The client reads u32 *count* and then, for each creature, reads the creature delta straight
from the socket with recv-based field readers (client 0x4cd3e0) instead of from an inflated
buffer, then performs the same removal as packet 2. Alpha's server never sends id 1.
AlphaVulkan servers must not send it (**open question P-3** for its exact framing).

### 3.3 Packet 3: static-object list (not sent)

u32 *count*, then *count* × 120-byte records (client 0x46bfb0). The client keeps a map of
objects keyed by the record's first 8 bytes, replaces each listed object's state with the
record, and deletes objects not listed. Alpha's server builds such a list every tick (objects
within 512 blocks, squared distance ≤ 262,144, server 0x423dd0) but never sends it.
AlphaVulkan servers must not send id 3 unless they implement and verify the record
(**open question P-4**).

### 3.4 Packet 4: world update

    u32 compressed length, zlib stream; inflated content = 13 sections in this order

Every section starts with a u32 *count*. Sections 7 and 8 are keyed lists; all others are
plain arrays of fixed-size records. Record bytes are the server's in-memory structs copied
whole (padding included). The table also gives where the server takes each list from and
whether the server filters it by distance from the receiving player (positions compared in
block units; 200 blocks = squared distance < 40,000, constant at server 0x558924).

| # | Record size | Server source (see `alpha-net-server.md`) | Filter | Client action (client 0x46cc0e, then the frame update 0x488ee0) |
|---|---|---|---|---|
| 1 | 20 | block edits: world events, plus the stored edits of every zone the client requests | none | set or clear world blocks (§3.5) |
| 2 | 72 | hits (relayed player hits and other hits) | none | apply to the local player if targeted (§5.1) |
| 3 | 72 | hits (second list) | 200 blocks, using the record's first 24 bytes as a position (see the note below) | as section 2 |
| 4 | 24 | positional events (first 12 bytes are 3 × f32 position in blocks) | 200 blocks | *unknown* (sound effects are likely) |
| 5 | 112 | shoots (projectiles), relayed from packet 9 | none | spawn projectiles |
| 6 | 88 | static-entity states of requested zones (§3.6) | none | update doors, chests and similar |
| 7 | keyed: u64 zone key + u32 *n* + *n* × 328 | world items lying in requested zones (§3.7) | none | replace that zone's item list |
| 8 | keyed: u64 key + u32 *n* + *n* × 16 | *unknown* | none | *unknown* |
| 9 | 288 | *unknown* (item-carrying events; 8 + 280 is a likely split) | none | *unknown* |
| 10 | 24 | *unknown* | none | *unknown* |
| 11 | 24 | *unknown* | none | *unknown* |
| 12 | 40 | "event 40" records, relayed from packet 8 | none | *unknown* |
| 13 | 56 | map-cell (mission) states of requested map regions (§3.8) | none | update the world map and quest markers |

Note on section 3: the server's distance test for section 3 reads three i64 values from the
start of the record and compares them with the receiving player's position, i.e. the
record's first 24 bytes are taken as an i64 position. That contradicts the hit layout of
§5.1, where the first 16 bytes are two entity ids. **Open question P-5**: either section 3
carries a different 72-byte record than section 2, or its filter is meaningless in Alpha.
A capture of an Alpha server with NPC fights will settle it.

An empty world update (all thirteen counts zero) is 52 zero bytes before compression; the
server sends it every tick when nothing happened (test vector §6.6).

### 3.5 Section 1 record: block edit (20 bytes)

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 12 | 3 × i32 | block coordinates (world block units) |
| 0x0c | 3 | 3 × u8 | colour R, G, B |
| 0x0f | 1 | u8 | block kind; **0 = remove the block**, non-zero = place a block of this kind with the colour |
| 0x10 | 4 | — | *unknown*, possibly padding (send zero) |

After applying a section the client rebuilds the meshes of the bounding box of all edited
blocks (client 0x48f242–0x48f3d0). The same 20-byte records are stored per zone in world
saves.

### 3.6 Section 6 record: static-entity state (88 bytes)

Built by the server for each requested zone from its list of 392-byte (0x188) static
entities (server 0x4272d0). Field placement:

| Off | Size | Content |
|---|---|---|
| 0x00 | 8 | zone x, zone y (2 × i32) |
| 0x08 | 4 | index of the entity in the zone's static-entity list |
| 0x0c | 4 | padding |
| 0x10 | 4 | entity word 0 |
| 0x14 | 4 | padding |
| 0x18 | 28 | entity words 2–8 |
| 0x34 | 12 | entity words 9–11 |
| 0x40 | 1 | entity byte at 0x30 (the same byte the zone save stores per entity: 1 in all observed saves, an "intact / closed" state is likely) |
| 0x41 | 3 | padding |
| 0x44 | 8 | entity words 13–14 |
| 0x4c | 4 | padding |
| 0x50 | 8 | entity words 16–17 |

"Entity word *k*" is the *k*-th 4-byte word of the server's static-entity struct. Their
meaning (type, position, orientation, size) is not traced: **open question P-6**.
Because clients generate the same static entities from the seed, a server mainly needs to
echo the state byte; see `alpha-net-server.md` §5.

### 3.7 Section 7 record: world item (328 bytes)

Keyed by the zone: the key is two i32 (zone x, zone y), then u32 *n*, then *n* records:

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x000 | 280 | item | the item (§1.1) |
| 0x118 | 24 | 3 × i64 | position |
| 0x130 | 4 | f32 | *unknown* (an angle is likely: values like 287.0 seen) |
| 0x134 | 4 | f32 | *unknown* (0.07–0.10 seen: a scale is likely) |
| 0x138 | 1 | u8 | *unknown* (2 seen) |
| 0x139 | 3 | — | padding |
| 0x13c | 12 | 3 × i32 | *unknown* (0, 0, −1 seen) |

The zone save stores the same fields without the padding (`alpha-saves.md` §4.2).

### 3.8 Section 13 record: map cell state (56 bytes)

For a requested map region (*rx*, *ry*) the server walks its 8 × 8 map cells (cell
x = *rx*·8 + *i*, y = *ry*·8 + *j*) and emits one record per cell (server 0x4272d0, second
half). The record holds the cell coordinates (2 × i32), then words taken from the cell's
104-byte (0x68) state struct at offsets 0x2c, 0x30, 0x34, 0x38, 0x3c (5 × u32), bytes 0x40
and 0x41, and words 0x44, 0x48, 0x4c, 0x50, laid out in that order with in-memory padding;
the exact placement within the 56 bytes is **open question P-7** (confirm with a capture).
The same per-cell data is saved under the `mission` and `monster` keys (`alpha-saves.md`).

### 3.9 Packet 10 (S→C): chat, and packet 15: world seed

Packet 10: u64 sender entity id, u32 *n* (UTF-16 code units, no terminator), *n* × 2 bytes.
The client appends the line to its chat log (client 0x46ca1e). *n* ≤ 0 is ignored.

Packet 15: u32 seed. The client names the world `online_<seed>` (decimal) and, unless it is
already in that world, switches to it and generates it from the seed (client 0x46b8be; see
the world-generation specs). Sent once, right after the handshake.

---

## 4. Client → server packets

### 4.1 Packet 6: item record (300 bytes)

Sent when the local player's queue at creature object +0x130c is non-empty, one record per
tick, front of the queue first (client 0x469c10). The server stores the record in the
sending player's creature (server 0x426020 case 6) and its world simulation consumes it.

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x000 | 280 | item | the item |
| 0x118 | 16 | 4 × u32/f32 | *unknown* |
| 0x128 | 1 | u8 | *unknown* |
| 0x129 | 1 | — | padding |
| 0x12a | 2 | u16 | *unknown* |

What the record asks for (dropping an item into the world is the likely meaning) is
**open question P-8**.

### 4.2 Packets 7, 8, 9: events produced by the local player

The client's own simulation produces hit, "event 40" and shoot records. When online, every
record of those three kinds whose **first 8 bytes equal the local player's id** is sent to
the server, one packet per record (client 0x49c03c–0x49c256). So all three records start
with the u64 id of the creature that caused them. The server queues them for its world
simulation; shoots (9) and "event 40" (8) are passed on to every client in sections 5 and
12 of the world update, hits (7) feed sections 2/3.

* Packet 7, hit, 72 bytes: §5.1.
* Packet 8, 40 bytes: u64 source id, then 32 bytes *unknown* (**open question P-9**).
* Packet 9, shoot, 112 bytes: u64 shooter id, then the projectile state; the full layout
  is `alpha-projectiles.md` §1 (resolves this part of **P-9**).

### 4.3 Packet 10 (C→S): chat

u32 *n* (UTF-16 code units), *n* × 2 bytes. The server prefixes the sender's id and queues
the line for **every** connected client, the sender included (server 0x426020 case 10).
A line typed while offline, and lines starting with a command (`/connect`, `/disconnect`,
`/namepet`), are not sent.

### 4.4 Packets 11 and 12: data requests

* 11: i32 zone x, i32 zone y. The server accepts 0 ≤ x, y < 65,536, finds the zone (region =
  zone ÷ 64, cell within region = zone mod 64) and, **if that zone is loaded on the
  server**, answers in the next world update with the zone's block edits (section 1), its
  world items (section 7) and its static-entity states (section 6). If the zone is not
  loaded nothing is sent; the client asks again a second later.
* 12: i32 region x, i32 region y (one region = 8 × 8 map cells = 64 × 64 zones). The server
  answers with the 64 map-cell records of that region (section 13) for every cell whose
  region is loaded.

Requests are collected per connection and answered once per server tick; duplicates are
harmless.

---

## 5. Records shared by both directions

### 5.1 Hit (72 bytes)

Partly traced from the client's handling of incoming hits (client 0x48ff10–0x49030a):

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 8 | u64 | attacker id |
| 0x08 | 8 | u64 | target id; only hits whose target is the local player are applied |
| 0x10 | 4 | f32 | amount (damage, or the size of the effect for special kinds) |
| 0x14 | 4 | — | *unknown* |
| 0x18 | 4 | i32 | a counter/duration: the target's record field 16 is raised to at least this |
| 0x1c | 28 | — | *unknown* (a hit position is likely; **open question P-10**) |
| 0x38 | 12 | 3 × f32 | knock-back vector, added to the target's velocity |
| 0x44 | 1 | u8 | *unknown* |
| 0x45 | 1 | u8 | hit kind (below) |
| 0x46 | 2 | — | padding |

Client behaviour for a hit on the local player, by kind:

* Kind 1: no health loss. If the amount is positive the knock-back is added to the
  velocity and a client-only flag (creature object +0x1184) is set to 1.0; then the
  knock-back is added once more and the gauges at record fields 23, 28 and 29 are adjusted
  (field 28 is raised by field 29 and clamped at 1; field 23 is raised by field 29 and
  clamped to field 28; field 29 is lowered by amount ÷ a value derived from the player and
  clamped at −1). A "blocked" or "parried" meaning is likely.
* Kind 5: reduces an absorbing buff (an entry of kind 6 in the player's buff list) by the
  amount, removing it when it reaches zero.
* Any other kind (0, 2, 3, 4, …), if the player is alive: health (field 27) is reduced by
  the amount and
  clamped to 0 and to the player's maximum; a client-only resource of the player (creature
  object +0x1190, outside the record; mana or stamina is likely) is reduced by
  amount × a constant ÷ a level-based divisor and clamped at 0; record field 16 is raised to
  hit field 0x18.

Damage is decided by the **attacker's** client; the target's client applies it. Neither the
server nor the target validates it.

---

## 6. Worked examples (test vectors)

All bytes hexadecimal, in wire order. Compressed bytes were produced with zlib level 6;
other encoders produce different but equally valid streams.

### 6.1 Handshake

    C→S  11 00 00 00  03 00 00 00
    S→C  10 00 00 00
    S→C  00 00 00 00  01 00 00 00 00 00 00 00  <4456 bytes of creature record>
    S→C  0f 00 00 00  5b ce 24 00                      seed 2,412,123

### 6.2 Creature update: position and health of entity 1

Plain (44 bytes):

    01 00 00 00 00 00 00 00                            id 1
    01 00 00 08 00 00 00 00                            mask: bit 0 + bit 27
    00 00 80 42 26 80 00 00                            x = 550,397,706,240 (8,398,402.5 blocks)
    00 00 80 a0 22 80 00 00                            y = 550,336,757,760
    00 00 40 00 00 00 00 00                            z = 4,194,304 (64 blocks)
    00 80 08 43                                        health 136.5

On the wire (packet 0, 33 compressed bytes):

    00 00 00 00  21 00 00 00
    78 9c 63 64 80 00 46 06 06 0e 30 a3 c1 49 ad 01 4c 2f 50 02 d3 0c 0e 10 05 0d 1c ce
    00 48 c3 04 40

### 6.3 Creature update with no changes (keeps entity 1 alive in a batch)

    00 00 00 00  0b 00 00 00  78 9c 63 64 40 05 00 00 20 00 02

(plain: 01 00 00 00 00 00 00 00 followed by eight zero bytes.)

### 6.4 End of batch, clock

    02 00 00 00
    05 00 00 00  05 00 00 00  d6 03 df 00               day 5, 14,615,510 ms (04:03:35.5)

### 6.5 Chat "hi"

    C→S  0a 00 00 00  02 00 00 00  68 00 69 00
    S→C  0a 00 00 00  01 00 00 00 00 00 00 00  02 00 00 00  68 00 69 00   (from entity 1)

### 6.6 Empty world update

Plain: 52 zero bytes (13 counts of 0).

    04 00 00 00  0c 00 00 00  78 9c 63 60 20 1d 00 00 00 34 00 01

### 6.7 World update with one block edit

Plain: count 1, record (x 8,398,402, y 8,397,472, z 64, colour 80 40 20, kind 1, 4 zero
bytes), then 12 zero counts:

    01 00 00 00  42 26 80 00  a0 22 80 00  40 00 00 00  80 40 20 01  00 00 00 00
    00 00 00 00 … (48 zero bytes)

Wire:

    04 00 00 00  1f 00 00 00
    78 9c 63 64 60 60 70 52 6b 60 58 a0 d4 c0 e0 c0 c0 c0 d0 e0 a0 c0 c8 40 06 00 00 cc
    1d 03 4d

### 6.8 Requests

    0b 00 00 00  26 80 00 00  20 80 00 00              zone (32806, 32800)
    0c 00 00 00  00 02 00 00  00 02 00 00              map region (512, 512)

### 6.9 A complete idle server tick for a client alone on the server

    02 00 00 00
    05 00 00 00  <day>  <time>
    04 00 00 00  0c 00 00 00  78 9c 63 60 20 1d 00 00 00 34 00 01

The Alpha server never lists a client's own creature to that client (it skips the entity
whose id is the connection's id), so with nobody else around the batch is empty and the
tick starts directly with packet 2. With a second player (id 2) in the world, each tick to
client 1 starts with that player's packet 0 (6.3 with id 2 is
`00 00 00 00 0b 00 00 00 78 9c 63 62 40 05 00 00 30 00 03`; recompute the stream for other
ids) before the packet 2.

---

## 7. Open questions (and how to settle them)

Most of these are field meanings. The user's 32-bit Alpha mod DLLs can log every
`send`/`recv` of `Cube.exe` (hook the import thunks at client 0x62723a and 0x627234) and of
`Server.exe` (server 0x54a530 and 0x54a52a); inflating packet 0 and packet 4 payloads in the
log gives everything below.

* **P-1** Item sub-block entries (item 0x14–0x113) and fields 0x04, 0x08, 0x0c–0x0e, 0x10:
  craft or loot items in a recorded session and diff the item bytes.
* **P-2** Appearance field names: change hair, face, colour in character creation and diff.
* **P-3** Packet 1 framing (only matters if some Alpha build sends it).
* **P-4** Packet 3's 120-byte record.
* **P-5** Whether world-update section 3 carries hits; capture an Alpha server with NPCs
  fighting a player.
* **P-6** Static-entity words (section 6): open a door/chest online and diff.
* **P-7** Map-cell record placement (section 13): capture after opening the world map.
* **P-8** Packet 6 purpose: drop an item while online.
* **P-9** Packet 8 and 9 layouts: use abilities and ranged weapons while online.
* **P-10** Hit bytes 0x14 and 0x1c–0x37.
* **P-11** Unknown creature fields (bits 3–6, 9–12, 14, 15, 17–20, 24–26, 30–32, 35–39, 41–43,
  47): record a session and correlate with actions (jumping, gliding, attacking, mounting).
* **P-12** World-update sections 4 and 8–11: which world events they carry.
