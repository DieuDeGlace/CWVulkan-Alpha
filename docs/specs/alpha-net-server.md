# Alpha multiplayer: what the dedicated server does

Behavioural specification of Cube World **Alpha**'s `Server.exe` (SHA-256 `1ffbcbce…34fb1`):
its configuration, simulation loop, joins and leaves, entity ids, what it sends to whom, and
what it trusts clients with. Framing and packets are in `alpha-net-transport.md` and
`alpha-net-packets.md`; the world save it writes is in `alpha-saves.md`. World generation
itself (how the seed becomes terrain, NPCs and map cells) is in the world-generation specs.

Routines are cited by address in `Server.exe` unless marked "client". The server contains the
same world code as the client (generation, zones, saves, NPC logic), compiled separately;
its address space is unrelated to the client's.

The server's single biggest routine, the world simulation step (server 0x5322d0), is the one
server function the decompiler could not recover; its behaviour is described here only from
its inputs, outputs and the routines it calls, and several open questions remain there.

---

## 0. Conventions

As in `alpha-net-transport.md`. "Player" means a creature whose controller byte (record
offset 0x50) is 0; "NPC" means any other creature. Distances are in blocks.

---

## 1. Start-up and configuration

### 1.1 `server.cfg`: the world seed (server 0x549c50)

* `server.cfg` is a text file in the working directory holding **one decimal integer**, the
  world seed, read with the C++ stream `>>` operator (leading spaces and a trailing newline
  are fine; nothing else in the file is read).
* If the file cannot be opened, the server takes `rand() % 1000000` as the seed and **writes
  it** to a new `server.cfg` (just the number, no newline). Nothing seeds the C runtime's
  random generator before this call as far as the start-up code shows, so a fresh Alpha
  server would normally pick the same seed every time (with the Microsoft C runtime the first
  `rand()` value is 41). **Open question S-1**: confirm by deleting `server.cfg` on a copy of
  the server and starting it once.
* There are no other settings: no port, player limit, name, password or message of the day.
  The port is fixed at 12345 and the player limit at 10 (`alpha-net-transport.md` §2).

### 1.2 The world

* The world object is created and given the seed and the name `server_<seed>` (decimal)
  (server 0x4d83a0). From the seed the world derives its generation parameters (see the
  world-generation specs) and sets its time defaults.
* Its save database is `Save/world_server_<seed>.db` relative to the working directory, in
  the format of `alpha-saves.md` §4. At start-up the server reads the key `time` from it:
  the first u32 becomes the **day**, the second the **time of day** in milliseconds. If the
  key is missing the defaults stand (the client's new-world defaults are day 0, time
  43,200,000 = noon; the server's are assumed to match, **open question S-2**).
* The server writes zones, map-cell `mission`/`monster` records and `time` to that database
  through the same save routines as the client (server 0x4d81b0, 0x4d7c50 and the
  simulation step). It does not write a `map_…` database, `worlds.db` or `characters.db`;
  player characters are never stored on the server.
* A generation thread is started; it generates the zones the simulation loop asks for
  (§2 step 6).

### 1.3 Console

The console prints the messages listed in `alpha-net-transport.md` §1.1 and §2.1, plus
"Player <n> joined." and, at shutdown, "stopping generation thread...". Typing `Q` stops the
server (§6).

---

## 2. The simulation loop (server 0x549c50)

The main thread raises the system timer resolution to 1 ms (`timeBeginPeriod(1)`) and then
repeats until the running flag is cleared:

1. Read the millisecond clock (`timeGetTime`); dt = now − previous loop's time.
2. Take the queue of **hits** received from all clients (packet 7) and the queue of
   **"event 40"** records (packet 8); both queues are emptied.
3. Lock the world.
4. Run the **world simulation step** with dt, the incoming hits and "event 40" records, and
   an empty set of outgoing event lists (server 0x5322d0). The step advances the world
   clock, updates every NPC, applies the incoming events, consumes the item records that
   players sent with packet 6, saves and unloads zones nobody is near, and fills the
   outgoing lists described in §4.2. **Open question S-3** covers its internals.
5. Take the queue of **shoots** received from all clients (packet 9) and append it,
   unchanged, to the outgoing shoot list.
6. Hand the outgoing lists to every connection (§4.2), and for each **player** creature
   compute its zone (x and y position ÷ 65,536 ÷ 256, rounded toward zero) and queue that
   zone for the generation thread.
7. Unlock the world and sleep **20 ms**.

So the simulation runs at a little under 50 steps per second with a variable dt. Each
connection's sender thread runs its own ~20 ms loop (§4) and reads the world under the same
lock, so what a client receives is at most one or two steps old.

---

## 3. Joining and leaving

### 3.1 Join (server 0x4254a0)

The handshake order is in `alpha-net-transport.md` §2.1. When it accepts a player the server
creates a creature (a 7,776-byte object, of which the network sees the 4,456-byte record)
with:

* id: the lowest free id in **1–10**;
* position: a spawn point computed by the server (three i64 values; **open question S-4**
  for how it is chosen); the same value is copied into the home position (record 0x1b0);
* orientation, velocity and the other motion vectors (record 0x18–0x47): zero;
* collision box (appearance 0x08): 1.2, 1.2, 2.7;
* controller byte 0 (player), creature type 44, appearance generated from the type;
* level 1, health = the creature's maximum health as computed from its stats;
* class 1 (warrior);
* equipment slots 6 and 7: an item of type 3, subtype 2, with byte 0x0d = 1;
* inventory: 50 additions of an item of type 1, subtype 0, level 1, and one item of type 3,
  subtype 6, byte 0x0d = 2, level 1 (inventory is not part of the network record).

These values matter little in practice: the client keeps its own saved character and
immediately sends a full creature update, which overwrites everything in the record
(`alpha-net-packets.md` §2.3). They are what other players see during the first few
milliseconds, and what an AlphaVulkan server should send in the handshake record.

### 3.2 Leave

There is no leave message. When the connection's sender thread gets a send error it closes
the socket, removes the creature from the world and frees the slot and the id
(`alpha-net-transport.md` §5.2). Other clients stop receiving that creature, so their next
packet 2 deletes it. Nothing is saved for the leaving player on the server.

### 3.3 Entity ids

* Players: 1–10, reused after a player leaves.
* NPCs and other creatures are created by the world simulation step with ids from its own
  counter (**open question S-5**: start value and whether ids are reused). Clients do not
  care about the numbering as long as ids are unique 64-bit values and never collide with
  1–10.
* Ids are u64 on the wire; the client treats an id below 0 (as a signed value) in the
  handshake as "no id".

---

## 4. What each client is sent (sender thread, server 0x423dd0)

Every ~20 ms, per connection, under the world lock:

### 4.1 Creatures

The thread keeps, per connection, the set of creature records it sent last tick ("known").
For every creature in the world's entity table:

1. Skip the connection's own creature (its id equals the connection's id).
2. If the creature is an NPC (controller ≠ 0) and its squared distance from this player is
   greater than **40,000** (200 blocks; positions compared in block units after scaling the
   i64 difference by 1/65536), skip it.
3. If it is not in "known": skip it unless more than **500 ms** have passed since this
   connection's sender thread started **and** no other new creature has been introduced
   this tick. In other words a newly joined client sees no other creature for half a
   second, and after that at most one new creature appears per tick (about 50 per second).
4. Otherwise copy its record into this tick's "to send" set.

Then, for every creature in "to send": packet 0 with a delta against the "known" copy (all
48 fields if it was not known), and finally packet 2. "To send" becomes "known".

Consequences an AlphaVulkan server must reproduce for Alpha clients:

* players are visible to each other at **any** distance; NPCs only within 200 blocks;
* a creature that leaves the radius, despawns or disconnects simply stops being listed, and
  the client deletes it at the next packet 2;
* every visible creature is listed every tick (empty mask when unchanged).

### 4.2 Events and zone data (packet 4)

The simulation step and the request handling fill thirteen lists per connection; the sender
thread serialises them in the section order of `alpha-net-packets.md` §3.4 and empties
them. Where they come from (server 0x4272d0):

| Section | Filled from | Per-player filter |
|---|---|---|
| 1 block edits | simulation output, plus stored edits of each zone this client requested | none |
| 2 hits | simulation output (player hits from packet 7 end up here) | none |
| 3 hits | simulation output | squared distance < 40,000 using the record's first 24 bytes as an i64 position |
| 4 positional events | simulation output | squared distance < 40,000 using the record's first 12 bytes as an f32 position in blocks |
| 5 shoots | packet 9 records, unchanged | none |
| 6 static-entity states | each requested zone's static entities | none |
| 7 world items | each requested zone's item list, keyed by zone | none |
| 8 keyed 16-byte lists | simulation output | none |
| 9 288-byte records | simulation output | none |
| 10, 11 24-byte records | simulation output | none |
| 12 "event 40" | simulation output (packet 8 records pass through the step) | none |
| 13 map cells | the 8 × 8 cells of each requested map region | none |

The same event list is given to **every** connection; only sections 3 and 4 are filtered.
A client therefore also receives its own hits, shoots and events back; Alpha clients cope
with that (they apply hits only when they are the target, and the attacker's own client
simulated the attack already).

### 4.3 Requests (packets 11 and 12)

Per connection, the receiver thread records requested zones and map regions; once per
simulation loop the distributor answers them (§4.2 sections 1, 6, 7 and 13) and forgets
them. A request for a zone or region that the server has not generated yet produces
nothing; the client repeats its requests every second, and the zone will by then usually
have been generated because the server generates zones around every player (§2 step 6).

### 4.4 Clock and chat

* Packet 5 carries the world's day and time of day every tick.
* Chat lines (packet 10 from any client) are queued for every connection with the sender's
  id; the sender thread sends at most one queued line per tick per connection.

---

## 5. Trust model

Alpha's server is a relay with an NPC simulation, not an authority:

* **Trusted from clients without checks**: the player's whole creature record (position,
  movement, health, level, experience, equipment, skills, name, class), the hits they
  deal (damage amounts included), their shoots and "event 40" records, their item records
  (packet 6) and their chat.
* **Server-owned**: the world clock, NPCs (spawning, AI, their health), world items lying in
  zones, static-entity and map-cell state, block edits, and the world save.
* **Client-owned and never corrected**: the client ignores server packet 0 updates of its
  own creature, so the server cannot teleport or heal a player through the creature stream.
* **Both sides generate the world from the seed**: terrain, static entities and map cells
  are generated locally by every client (packet 15 tells them the seed); the server only
  sends dynamic changes (block edits, item lists, entity and cell states).

An AlphaVulkan server may add validation (speed, damage, level sanity) as long as it keeps
sending what Alpha clients expect; it cannot force corrections onto Alpha clients through
packet 0.

---

## 6. Shutdown

Typing `Q` in the console clears the running flag. The simulation loop ends, prints
"stopping generation thread...", waits for that thread, deletes the server and the world
objects and calls `WSACleanup`. The accept and connection threads end when they next notice
the flag. Whether the world is saved at shutdown, or only when zones unload during play, is
**open question S-6**.

---

## 7. A minimal compatible AlphaVulkan server

What an implementation needs, in order of importance, to host original Alpha clients:

1. Transport and handshake exactly as in `alpha-net-transport.md` (port 12345, 17/3 →
   16, id + record, 15 + seed); refuse an eleventh player with 18.
2. Per connection, every ~20 ms: packet 0 for each visible creature (rules of §4.1),
   packet 2, packet 5, packet 4 (thirteen counts, zeros when idle).
3. Relay: chat to everyone with the sender id; shoots (9) into section 5 and "event 40" (8)
   into section 12 for everyone; player hits (7) into section 2 for everyone.
4. Apply each client's packet 0 deltas to its creature and pass them on to the others.
5. A clock that advances (day and milliseconds of day) and is saved under `time`.
6. Later: NPC simulation (section 3 hits, NPC creatures), zone item lists, block edits,
   static-entity and map-cell state, answering packets 11 and 12, and saving the world in
   the Alpha format so `Save/world_server_<seed>.db` stays usable by `Server.exe`.

Steps 1–5 already give a working "players see each other, chat and fight" server for
Alpha clients; they also cover what an AlphaVulkan **client** must accept from an Alpha
server.

### 7.1 Joining an Alpha server from AlphaVulkan

The client side mirrors §3 of `alpha-net-transport.md`: keep the local character, adopt the
assigned id, switch to the world `online_<seed>`, send a full creature update first and
deltas afterwards every ~20 ms, send hits/shoots/"event 40" that the local player causes,
request the zones and map regions it needs every second, and obey packet 2 removals.

---

## 8. Open questions (and how to settle them)

Verification uses captures from the running game with the user's 32-bit mod DLLs (hooks on
the `send`/`recv` thunks listed in `alpha-net-packets.md` §7) and copies of `Server.exe`
run in a scratch folder.

* **S-1** Seed chosen when `server.cfg` is missing.
* **S-2** Server default day and time for a new world (read packet 5 from a fresh server).
* **S-3** The simulation step (server 0x5322d0): NPC spawning and AI, how incoming hits are
  applied to NPCs, how packet 6 item records are used, which events go to sections 3, 4 and
  8–11, how fast the clock runs, and when zones are saved and unloaded. Its assembly is in
  `decompiled/server/assembly-functions/005322d0.asm`; tracing it is the largest remaining
  spec task for a full server. A capture of packets 4 and 5 over a few minutes near NPCs
  gives the observable part. **Closed**: specified in `alpha-server-simulation.md` (which also settles S-5 and corrects §4.2 sections 2, 3 and 12).
* **S-4** How the join spawn point is computed (only visible to others for the first
  milliseconds; low priority).
* **S-5** NPC id numbering.
* **S-6** Whether the server saves on shutdown.
