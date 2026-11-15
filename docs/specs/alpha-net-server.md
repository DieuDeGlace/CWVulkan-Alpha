Status: FINAL / STABLE / HELD

# Alpha multiplayer: what the dedicated server does

Behavioural specification of Cube World **Alpha**'s `Server.exe` (SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, 32-bit x86, image base
0x00400000): its configuration, threads and main loop, joins and leaves, entity ids, what it sends
to whom, and what it accepts from clients without checking. Framing and packet layouts are in
`alpha-net-transport.md` and `alpha-net-packets.md`; the world save format is in `alpha-saves.md`;
the internals of the world simulation step are in `alpha-server-simulation.md`; world generation
itself is in the world-generation specs. Those names are pointers only; this document does not
depend on their contents.

Every address below is an RVA in `Server.exe` (add 0x400000 for the loaded address). The server
image carries its own copy of the world code (generation, zones, regions, saves); its addresses
have no relation to `Cube.exe`.

The world simulation step (RVA 0x1322D0) is the one large server routine without a recovered
decompilation in the evidence set; the few facts given here about it come from its disassembly,
and everything else about it is left to the simulation spec or listed as unresolved.

---

## 0. Conventions

* A creature is a 7,776-byte (0x1E60) object; its 64-bit id is at object offset 0x08 and the
  4,456-byte (0x1168) network record starts at object offset 0x10. "Record offset" means an
  offset into that network record.
* "Player" means a creature whose controller byte (record offset 0x50) is 0; "NPC" means any
  other creature.
* A creature position is three signed 64-bit values at record offset 0x00, in units of 1/65,536
  block. Distances are in blocks.
* A zone is 256 × 256 blocks; a region is 64 × 64 zones; a map cell is 8 × 8 zones, so a region
  holds 8 × 8 map cells. The world keeps a 1,024 × 1,024 table of regions; zone coordinates are
  valid in 0..65,535 and map-cell coordinates in 0..8,191.
* "The world lock" is the critical section at world object offset 0x8000C0, entered and left by
  the routines at RVA 0xD3DF0 and 0xD5C60. It is recursive (a Windows critical section).

---

## 1. Start-up and configuration (RVA 0x149C50)

### 1.1 Order of start-up

1. Winsock 2.2 is started.
2. The world object (0x80029C bytes) is constructed (RVA 0xC8570). The constructor sets the
   world clock to day 0 and time of day 43,200,000 ms (noon), and then runs the world set-up
   routine (RVA 0xD83A0) once with seed **123** and an empty name. That set-up seeds the C
   runtime generator with its seed and draws 77 values for the world's generation parameters.
3. The main routine draws one `rand()` value and keeps it modulo 1,000,000 as the fallback seed.
   Because of step 2 this draw is not the first draw of an unseeded generator; it follows
   `srand(123)` and the draws made inside the constructor (see unresolved U-1). Since the
   runtime's `rand()` never exceeds 32,767, the modulo has no effect.
4. `server.cfg` (working directory) is opened for reading:
   * if it opens, one decimal integer is extracted from it with the standard stream extraction
     operator, replacing the fallback (leading white space is skipped; nothing after the number
     is read);
   * if it cannot be opened, a new `server.cfg` is created holding the fallback seed as a decimal
     number with no line ending.
5. The world set-up routine runs again with the chosen seed and the name `server_<seed>`
   (decimal). It stores the seed (world offset 0x800164), calls `srand(seed)`, draws the
   generation parameters, discards any loaded regions (saving their zones first when saving is
   enabled), sets the default spawn point (§3.1), stores the name, and, when the name is not
   empty, opens the save database `Save/world_server_<seed>.db` and reads the key `time` (§1.2).
6. The server object is constructed (RVA 0x22E80). It copies the world's seed (server offset
   0x08) and name, and creates its three incoming queues (hits, "event 40" records, shoots).
7. The generation thread (RVA 0x149550) is started with its priority lowered by one step (§2.2).
8. The listening socket is created (RVA 0x28790): TCP, bound to port **12345** (0x3039), backlog
   10; the accept thread (RVA 0x254A0) is started.
9. The system timer resolution is raised to 1 ms, the loop clock is read, and the console thread
   (RVA 0x149B50) is started. Then the main loop (§2.1) begins.

There are no other settings: no configurable port, player limit, server name, password or
message of the day.

### 1.2 The world clock and the save database

* The world keeps the time of day in milliseconds at world offset 0x80015C and the day number at
  world offset 0x800160.
* At start-up (step 5 above) the key `time` is read from the database: its first u32 becomes the
  **day** and its second u32 the **time of day**. If the key is absent the constructor's values
  (day 0, 43,200,000 ms) remain.
* The database receives:
  * `zone…` records, written by the zone save routine (RVA 0xD81B0). A zone is written only when
    the world has a name, the zone exists, and the zone is marked modified or has stored block
    edits.
  * `mission…` and `monster…` records per region, written by the region save routine
    (RVA 0xD7C50).
  * `time`, written by the simulation step (§2.1, item 4).
* Zone and region saves happen when the generation thread unloads them (§2.2) and when the world
  object is destroyed at shutdown (§6). The simulation step does not call the zone or region save
  routines.
* The server writes no `map_…` database, no `worlds.db` and no `characters.db`. Player characters
  are not stored by the server.

### 1.3 Console

Messages printed by the server include "Waiting for connection." (before every accept),
"Connection failed.", "Client version: <n>", "New connection.", "Wrong client version. Closing
client connection.", "Player <slot> joined." (§3.1), "stopping generation thread..." and
"Stopping server..." (§6). The string "Server is full. Closing client connection." is present but
its branch cannot be taken (§3.1).

The console thread (RVA 0x149B50) repeatedly sleeps one second and then reads one character from
standard input; when that character is a lowercase **`q`** it clears the main loop's running flag
(§6). No other input is interpreted.

---

## 2. Threads and loops

### 2.1 The main loop (RVA 0x149C50)

Until its running flag is cleared, each pass does, in this order:

1. Read the millisecond clock (`timeGetTime`); dt is the unsigned difference from the previous
   pass's reading (the first pass measures from the reading taken just before the loop started).
2. Move all queued **hits** (packet 7 records from every client; RVA 0x28230) into the incoming
   event set, then all queued **"event 40"** records (packet 8; RVA 0x28170). Each transfer takes
   the world lock briefly and empties the server's queue.
3. Enter the world lock.
4. Run the world simulation step (RVA 0x1322D0) with dt, a fresh, empty outgoing event set (13
   lists, §4.2) and the incoming event set. Facts confirmed in its disassembly:
   * the time of day advances by 10 × dt, or by 100 × dt when a condition evaluated in the step's
     creature loop holds (U-5); while it exceeds 86,400,000 the day number increases by one and
     86,400,000 is subtracted, with a call to RVA 0x124500 after each wrap; for a world with an
     empty name the time of day is instead held at 32,400,000;
   * the step keeps a millisecond accumulator (world offset 0x8000BC) to which dt is added; when
     the accumulator plus dt and the accumulator fall into different multiples of 10,000 (signed
     division, truncated), the world has a name, and the database object's readiness check
     (RVA 0x13000) succeeds, the key `time` is
     written (day first, then time of day). In effect the clock is saved about every ten seconds
     of loop time.
   Everything else the step does is outside this document (`alpha-server-simulation.md`, U-5).
5. Move all queued **shoots** (packet 9 records; RVA 0x281D0) unchanged into the outgoing set's
   shoot list (the list that becomes packet 4 section 5).
6. Hand the outgoing set to every connection (RVA 0x272D0, §4.2), which also answers pending zone
   and map requests (§4.3).
7. Under the generation thread's own critical section, **replace** the generation request list
   with one entry per player creature, in entity-table order: the zone containing it, computed
   for x and y as the position divided by 65,536 and then by 256, each division truncating toward
   zero.
8. Leave the world lock, sleep **20 ms**, and discard both event sets.

The loop therefore runs a little under 50 passes per second with a variable dt.

### 2.2 The generation thread (RVA 0x149550)

The thread has no sleep in its loop. Each pass:

1. Copies the generation request list (§2.1 item 7) under its critical section.
2. If the server's seed and name equal the world's (the normal case), it releases that critical
   section and:
   * **Generates at most one zone**: among all zones whose squared distance in zone units from
     some requested zone is less than 9 (zone offsets up to ±2), with coordinates in 0..65,535,
     that are not loaded, it picks the one with the smallest squared distance (on equal distance
     the first one found wins: requests in list order, then x ascending, then y ascending) and
     generates it (RVA 0x118630).
   * **Unloads**, at most once per second (when more than 1,000 ms have passed since the last
     unload pass): for every loaded zone with no requested zone at squared zone distance below 9,
     the zone is removed from its region, saved (RVA 0xD81B0, subject to §1.2) and destroyed
     (RVA 0xD79F0). For every region with no requested zone's region within 2 regions on both
     axes (absolute difference below 3), the region's records are saved (RVA 0xD7C50) and the
     region is destroyed (RVA 0xD7960). A second per-region table entry is freed without saving
     when no requested zone's region is within 4 regions on both axes (RVA 0xD78E0). With no
     players connected every zone and region is unloaded.
3. If seed or name differ, it unloads every zone and region and re-runs the world set-up routine
   with the server's seed and name, under the world lock. This branch is not reached in the
   start-up order of §1.1, where the server copies both values from the world.

### 2.3 Accept thread and per-connection threads

The accept thread (RVA 0x254A0) loops while the server's running flag (server offset 0x4C) is set:
it sleeps 200 ms, prints "Waiting for connection." and blocks in `accept`. Each accepted client
goes through the join procedure (§3.1). For each joined client it starts a **receiver thread**
(RVA 0x26020, §5.1) and a **sender thread** (RVA 0x23DD0, §4), both with priority lowered by one
step.

---

## 3. Joining and leaving

### 3.1 Join (RVA 0x254A0)

Handshake framing is in `alpha-net-transport.md`. As observed in the accept thread:

1. Under the world lock, the server checks whether any of ids 1–10 is held by a player creature
   and keeps the answer as a 0-or-1 flag; the lock is released.
2. It reads a u32 from the client. If it is not 17, the socket is closed without reply.
3. It reads a u32 client version and prints "Client version: <n>".
4. The 0-or-1 flag is compared with 4; because it can never reach 4, the "server is full" reply
   (code 18) is never sent.
5. If the version is not 3, the server sends code 3, waits 500 ms, closes the socket and prints
   the wrong-version message.
6. Otherwise it sends code 16 and prints "New connection.".
7. It looks for the lowest id in **1–10** that is either absent from the world's entity table or
   mapped to no object. If there is none, the socket is closed without any further message, even
   though code 16 was already sent. This is the effective limit of ten players.
8. Under the world lock it creates the creature with that id, registers it in the entity table,
   and initialises it (below).
9. Outside the lock it fills the creature's inventory (below).
10. Under the world lock it stores the connection in the first empty connection slot (or appends
    a new slot) with its socket and id.
11. It prints "Player <slot> joined.", where `<slot>` is the zero-based connection slot index,
    not the entity id.
12. It sends the handshake reply: a u32 0, the id (8 bytes) and the full 4,456-byte record, then
    a u32 15 followed by the world seed as a u32 (server offset 0x08).
13. It starts the receiver and sender threads.

The new creature is initialised as follows (record offsets unless stated):

* position: the world's spawn point (three f32 values at world offset 0x8000F0, in blocks), each
  multiplied by 65,536 and converted to a 64-bit integer by truncation. The world set-up routine
  sets the spawn point to (8,396,928, 8,396,928, 0) blocks and no other routine changes it after
  set-up, so every player starts at (550,301,073,408; 550,301,073,408; 0). The constructor's own
  provisional value (same x and y, z taken from the terrain height there) is overwritten by the
  set-up routine.
* collision box (record 0x70 to 0x7B; appearance offset 0x08 when appearance starts at record
  0x68): 1.5 × 0.8, 1.5 × 0.8, 1.5 × 1.8, that is 1.2, 1.2, 2.7 (the factor 1.5 is the f32 at RVA
  0x1586E8).
* record 0x18 to 0x2F (two three-component vectors) and the u32 at record 0x4C: set to zero.
* level (record 0x180): 1.
* health (record 0x15C): the creature's maximum health as computed at this point (RVA 0xFDA0),
  i.e. before the type, class and equipment below are assigned.
* controller byte (record 0x50): 0 (player); creature type (record 0x54): 44; appearance
  (starting at record 0x68) generated from the type.
* home position (record 0x1B0 to 0x1C7): a copy of the position.
* two item records at record offsets 0x980 and 0xA98 (280-byte item stride): type 3, subtype 2,
  item byte 0x0D = 1 (slot indices: U-3).
* class (record 0x130): 1.
* inventory (outside the network record, object offset 0x11DC): fifty additions of an item of
  type 1, subtype 0, level 1, then one item of type 3, subtype 6, item byte 0x0D = 2, level 1.
* all other fields keep the record initialiser's defaults (U-4).

### 3.2 Leave

There is no leave message. The receiver thread ends when a receive fails (§5.1). The sender thread
ends when one of its checked sends fails (the packet 2 send is not checked), when the connection
slot is empty, or when the server's running flag is
cleared; it then waits for the receiver thread, and under the world lock closes the socket,
destroys the creature and removes it from the entity table, destroys the connection object and
empties the slot, which makes the id available again. Other clients stop being sent that creature
(§4.1). Nothing is saved for the leaving player.

### 3.3 Entity ids

* Ids are 64-bit; the handshake sends the id as 8 bytes.
* Players receive ids 1–10 (§3.1), reused after a player leaves.
* Creatures not created by the join procedure come from the simulation step; their numbering is
  outside this document (U-5, U-7).

---

## 4. What each client is sent (sender thread, RVA 0x23DD0)

The sender thread records the clock once when it starts and then repeats one tick per pass while
the server's running flag is set and its connection slot is occupied.

### 4.1 Creatures

The thread keeps, per connection, the set of creature records it sent last tick ("known"). At the
start of a tick, under the world lock, it sets a one-shot permission when more than **500 ms** have
passed since the thread started, and then visits every creature of the world's entity table in
table order:

1. Skip empty entries and the connection's own creature.
2. If the creature is an NPC, compute the difference between its position and the connection's
   own creature's position, convert each axis to single precision and scale by 1/65,536 (f32 at
   RVA 0x15869C); skip it if the squared length is **greater than 40,000** (200 blocks; f32 at
   RVA 0x158924). Players are never skipped by distance.
3. If its id is not in "known": skip it unless the one-shot permission is still available, and
   consume the permission if it is used. A newly joined client therefore sees no other creature
   for half a second, and afterwards at most one creature is newly introduced per tick.
4. Copy its record into this tick's "to send" set.

Under the same lock the thread also collects entries of a second world table (120-byte entries,
world offset 0x0C) lying within 512 blocks (squared distance at most 262,144, f32 at RVA
0x15B23C); that collection is discarded without being sent.

Still under the lock, "known" is reduced to the creatures that still exist in the entity table;
then the lock is released. For every creature in "to send", in id order, the thread sends a
packet 0 carrying the id and that record encoded against the "known" copy (complete when the id
was not known; encoding in `alpha-net-packets.md`). Then it sends packet 2. Finally "to send"
becomes "known". Consequently a creature that leaves the 200-block radius, despawns or
disconnects stops being listed, and every listed creature receives a packet 0 every tick.

### 4.2 Events and zone data (packet 4)

The simulation step fills the 13 lists of the outgoing set; the main loop adds the shoots (§2.1
item 5). The distributor (RVA 0x272D0), under the world lock, appends the lists to each
connection's pending lists; the sender thread later takes and empties them (§4.4) and writes them
in the wire section order of `alpha-net-packets.md`. The internal list order differs from the wire
order; the mapping and the filtering are:

| Wire section | Outgoing list (internal index) | Added per connection on request | Per-connection filter |
|---|---|---|---|
| 1 | 3 | stored block edits of each requested zone (§4.3) | none |
| 2 | 0 | none | none |
| 3 | 2 | none | kept when the squared distance is **below 40,000**, using the record's first 24 bytes as an i64 position (difference scaled by 1/65,536) |
| 4 | 1 | none | kept when the squared distance is **below 40,000**, using the record's first 12 bytes as an f32 position in blocks against the player's position converted to f32 blocks |
| 5 | 4 | none (packet 9 shoots arrive here unchanged through the main loop) | none |
| 6 | 6 | static-entity records of each requested zone | none |
| 7 | 5 | the item list of each requested zone, keyed by zone coordinates | none |
| 8 | 7 | none | none |
| 9–12 | 8–11 | none | none |
| 13 | 12 | the 8 × 8 map cells of each requested map region | none |

Filters are measured from the connection's own creature. Apart from sections 3 and 4, every
connection receives the same lists, including events that originated from its own client.
Incoming hits and "event 40" records are handed only to the simulation step; whether and how they
reach sections 2 and 12 is decided inside the step (U-5).

### 4.3 Requests (packets 11 and 12)

The receiver thread appends each requested zone (packet 11) or map region (packet 12) to the
connection's request lists under the connection's own critical section. Once per main-loop pass
the distributor copies and clears those lists and answers them:

* a zone request with coordinates in 0..65,535 whose region and zone are loaded yields the zone's
  stored block edits (section 1), its item list keyed by its coordinates (section 7) and one
  record per static entity (section 6);
* a map-region request yields, for each of the 8 × 8 cells of that region whose coordinates are
  in 0..8,191 and whose region is loaded, one cell record (section 13).

A request for something that is not loaded produces nothing and is forgotten. Zones near players
are loaded by the generation thread (§2.2).

### 4.4 Tick order, clock and chat

After packet 2 the thread enters the world lock, reads the time of day and the day number, takes a
copy of the connection's pending lists and empties them, and removes at most one queued chat line;
then it leaves the lock and sends:

1. packet 5: the **day** (u32) followed by the **time of day** (u32), every tick;
2. packet 4: the thirteen sections (§4.2);
3. packet 10, only when a chat line was taken: the sender's id (8 bytes), the length in UTF-16
   code units (u32) and the text.

Chat lines (packet 10 from any client) are queued by the receiving side for every connection
slot, including the sender's own, together with the sender's id; at most one line per connection
is sent per tick.

Pacing: after a tick the thread reads the clock and, when less than 20 ms have passed since a
**single timestamp shared by all sender threads** (RVA 0x183DCC), sleeps for the remainder; it then
stores its own reading in that shared timestamp. With one client this gives about 50 ticks per
second; with several clients the threads pace against each other's readings (U-8).

---

## 5. What the server accepts from clients

### 5.1 Receiver thread (RVA 0x26020)

The thread reads a u32 packet id and handles:

| Id | Payload read | Action |
|---|---|---|
| 0 | u32 size, then that many bytes | the payload is unpacked; its 8-byte id is read and **ignored**; the update is applied, under the world lock, to the connection's own creature |
| 6 | 300 bytes | appended, under the world lock, to a list kept on the connection's own creature object (object offset 0x130C) |
| 7 | 72 bytes | queued as a hit (server offset 0x34) |
| 8 | 40 bytes | queued as an "event 40" record (server offset 0x3C) |
| 9 | 112 bytes | queued as a shoot (server offset 0x44) |
| 10 | u32 length n, then 2n bytes when n > 0 | queued as a chat line, with the sender's id, for every connection slot |
| 11 | 8 bytes | appended to the connection's zone requests |
| 12 | 8 bytes | appended to the connection's map-region requests |

Any other id is ignored and its payload, if any, is not consumed. A packet 0 with size 0 ends the
thread, as does a receive error.

### 5.2 Trust

* The receiving side performs **no validation** of any field: creature updates are applied
  directly to the sender's own creature (position, movement, health, level, equipment and every
  other record field), and hits, "event 40" records, shoots, item records and chat are queued as
  received. What the simulation step does with hits and item records is outside this document
  (U-5).
* A client cannot address another creature through packet 0, because the id inside the packet is
  not used.
* The server never sends a connection its own creature (§4.1).
* The world clock, the zone, region and map-cell data it answers requests with, the save database,
  and the creatures created by the step are maintained by the server.
* The seed is sent to each client at join (packet 15). Clients generate terrain from it; the
  server sends only dynamic data (block edits, item lists, static-entity and map-cell records,
  creatures and events).

---

## 6. Shutdown

Typing a lowercase `q` (§1.3) clears the main loop's flag. The main loop finishes its current
pass and then:

1. restores the timer resolution and prints "stopping generation thread...";
2. clears the generation thread's flag and waits for that thread;
3. destroys the server object (RVA 0x23880): it prints "Stopping server...", clears the server's
   running flag (server offset 0x4C), closes the listening socket and waits for the accept thread;
   per-connection threads stop on their next check of that flag (§3.2);
4. destroys the world object (RVA 0xCE540, which calls RVA 0xCD940): every loaded zone is passed
   to the zone save routine (written when §1.2 allows) and every loaded region's records are saved
   (RVA 0xD7C50) when saving is enabled, before the objects are freed;
5. deletes the generation critical section and calls `WSACleanup`.

The key `time` is not written at shutdown; its last value is the most recent periodic save
(§2.1 item 4).

---

## 7. Unresolved

* **U-1** The exact fallback seed used when `server.cfg` is missing. It is the first `rand()`
  draw in the main thread after everything the world constructor (RVA 0xC8570) draws following its
  `srand(123)` (at least the 77 parameter draws); whether the constructor draws further values was
  not traced. A run on a copy without `server.cfg` would settle it.
* **U-2** What seed is used when `server.cfg` opens but does not start with a number (depends on
  the stream library's failed-extraction behaviour in this runtime).
* **U-3** Which equipment slot indices the item records at record offsets 0x980 and 0xA98 are
  (earlier text called them slots 6 and 7; the equipment array's base offset was not confirmed).
* **U-4** The defaults that the record initialiser gives to the fields not listed in §3.1.
* **U-5** Simulation step internals (pointer: `alpha-server-simulation.md`): the condition for the
  ×100 clock rate, the purpose of RVA 0x124500, whether incoming hits and "event 40" records are
  copied into sections 2 and 12, what fills each outgoing list, how packet 6 item lists are used,
  NPC spawning, AI and id numbering.
* **U-6** Client-side behaviour mentioned in earlier text was not re-examined here: how the client
  treats packet 0 data for its own id, events echoed back to their originator, a negative id in
  the handshake, how often it repeats zone and map requests, and what it sends right after
  joining.
* **U-7** Whether a non-player creature can hold an id in 1–10, which would reduce the join
  limit below ten (§3.1 step 7).
* **U-8** The resulting per-connection tick rate when several sender threads share one pacing
  timestamp (§4.4); needs a runtime measurement.

---

## Re-certification (2026-11-14)

Evidence: `Server.exe` (hash above), its disassembly by the installed dumpbin, and the
`decompiled/server` function files and call list for that image. The Infinite-Cubes alpha
reconstruction was not consulted; no Steam material was used.

**Confirmed:** server.cfg read/write behaviour (one decimal integer; new file with no line ending);
no settings besides the seed; port 12345; world name `server_<seed>`; database path
`Save/world_server_<seed>.db`; `time` key order (day, then time of day); default day 0 and time
43,200,000 (former S-2, now confirmed from the constructor); main loop order (time, hits, "event
40", lock, step, shoots, distribution, zone requests, unlock, 20 ms sleep); zone computation with
truncation toward zero; ids 1–10, lowest free; record and object sizes 4,456 and 7,776; join
collision box 1.2/1.2/2.7, controller 0, type 44, level 1, health from stats, class 1, home copy at
record 0x1B0, inventory items; no leave message and cleanup by the sender thread; NPC radius 200
blocks with players unfiltered; 500 ms initial delay and one new creature per tick; packet 0 per
listed creature then packet 2; sections 3 and 4 filters (below 40,000; i64 and f32 positions);
shoots appended unchanged; zone requests answer sections 1, 6, 7 and map requests section 13;
packet 5 every tick; one chat line per tick per connection with the sender's id; no input
validation; seed sent with packet 15; shutdown printing, thread wait, object deletion and
`WSACleanup`.

**Corrected** (old → new):
1. All addresses were loaded addresses → RVAs in `Server.exe`.
2. Fallback seed "first rand() of an unseeded runtime, 41" → drawn after the constructor's
   `srand(123)` and its parameter draws; exact value unresolved (U-1). **Reversal** of the earlier
   claim.
3. Quit key "`Q`" → lowercase `q`, read by a console thread after one-second sleeps.
4. "Player <n> joined." → `<n>` is the zero-based connection slot index.
5. Join limit "eleventh player refused with code 18" (via the transport pointer) → the code-18
   branch is unreachable; with ids 1–10 taken the socket is closed silently after code 16 was sent.
   **Reversal.**
6. Join spawn point "open question S-4" → (8,396,928, 8,396,928, 0) blocks from the world set-up
   routine, scaled by 65,536.
7. Join zeroed fields "record 0x18–0x47" → record 0x18–0x2F and the u32 at record 0x4C; health is
   computed before type and class are set.
8. "Equipment slots 6 and 7" → item records at record offsets 0x980/0xA98; slot indices
   unresolved (U-3).
9. "The simulation step saves and unloads zones nobody is near" → the generation thread unloads
   (and thereby saves) zones and regions once per second; the step writes only `time` (about every
   10 s). **Reversal.**
10. "Generation thread generates the zones the loop asks for" → it generates at most one missing
    zone per pass, the nearest with squared zone distance below 9 to any player's zone; the request
    list is replaced, not appended, each pass.
11. Section table: sections 2 and 12 "receive packet 7 hits / packet 8 records" → those lists are
    filled only by the step; pass-through unresolved (U-5). Record sizes for sections 8–12 removed
    (packet spec pointer); internal list indices added.
12. Section 4 filter: the player position is also reduced to f32 blocks; filters are strict
    (below 40,000) while the NPC creature filter skips only above 40,000.
13. Sender pacing "each connection's own ~20 ms loop" → a 20 ms pacing against one timestamp shared
    by all sender threads (U-8).
14. Packet 0 trust: the id inside a client's packet 0 is ignored; updates go to the sender's own
    creature.
15. Packet 6 "consumed by the step" → appended to a list on the sender's creature object (object
    offset 0x130C); use unresolved (U-5).
16. Shutdown "whether the world is saved, open question S-6" → the world destructor saves all
    loaded zones (subject to the modified/edits rule) and region records; `time` is not saved at
    shutdown.
17. Clock facts added: 10 × dt (or 100 × dt under a step condition), day wrap at 86,400,000.

**Removed:** all advice addressed to a replacement server or client (what to send in the
handshake, consequences to reproduce, optional validation, the whole "minimal compatible server"
section and its client-joining subsection); the verification paragraph about captures with mod
DLLs and the open-question "how to settle" procedures; the statement about the decompiler's
failure (reworded as an evidence note); unverified client-side statements (now U-6); the closing
of S-3/S-5 by reference to another spec (now U-5 as a pointer).

**Unresolved:** U-1 to U-8 above.
