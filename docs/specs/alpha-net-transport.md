Status: FINAL / STABLE / HELD

# Alpha multiplayer: transport, handshake and framing

Native behaviour of how the Cube World **Alpha** client and the Alpha dedicated server
connect to each other and frame their traffic. Reference images:

* client `Cube.exe`, SHA-256
  `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`
* server `Server.exe`, SHA-256
  `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`

Both images have image base 0x400000. Routines and data are cited as **RVAs with the
image named** ("server RVA 0x254a0", "client RVA 0x6fc50"); client and server RVAs are
separate address spaces even when the numbers coincide. Offsets into in-memory objects are
given only for cross-checking against the original images. Facts were taken from the
original images, their disassembly and the Alpha decompile. Steam Cube World material was
not consulted; Alpha's protocol is a separate, much simpler design.

Companion contracts (not re-checked by this document): `alpha-net-packets.md` (packet and
record layouts), `alpha-net-server.md` (server simulation, interest rules, joins and
leaves), `alpha-saves.md` (save databases).

---

## 0. Conventions

* Integers are **little-endian**, two's complement. "u32" is a 4-byte unsigned integer,
  "i32" signed, "u64"/"i64" 8-byte, "f32" an IEEE-754 single. Fields follow one another
  with no padding unless a record says so.
* **Packet id** is the u32 that begins every message. Ids are written as decimals here
  (0x0f = 15, 0x10 = 16, 0x11 = 17, 0x12 = 18).
* World positions are i64 in units of 1/65536 block; one zone is 256 blocks.
* Block-unit distances: the server converts i64 position differences to float and
  multiplies them by 2^-16 (the f32 constant 1.52587890625e-05 at server RVA 0x15869c).
* "zlib stream" means an RFC 1950 stream (2-byte header, deflate data, Adler-32). Both
  images import `zlib1.dll` by ordinal only: 11, 50, 55 and 58. From their argument
  patterns and the version string "1.2.3" passed to ordinal 58, these are respectively the
  one-call compressor (`compress`, default level), `inflate`, `inflateEnd` and
  `inflateInit_` (stream-structure size 56). The names are inferred; the images carry no
  import names for them.

---

## 1. Transport

### 1.1 One TCP connection per player

* **TCP over IPv4 only.** Neither image imports a datagram send or receive call, socket
  option call or non-blocking control call. Server Winsock imports: accept, bind,
  closesocket, htons, listen, recv, send, socket, WSAStartup, WSACleanup. Client Winsock
  imports: closesocket, connect, gethostbyname, htons, inet_addr, recv, send, socket,
  WSAStartup, WSACleanup.
* Winsock version 2.2 is requested at start-up (server RVA 0x149c50, client RVA 0xc8ae0).
* Port **12345** (0x3039) is fixed in both images; there is no setting for it.
  * Server listener (server RVA 0x28790): prints "Starting server...", creates an IPv4
    stream socket, binds it to 0.0.0.0:12345 and listens with backlog 10. Failures print
    "Could not create server socket.", "Could not bind socket." or "Listen failed." and
    are not otherwise acted on. It then marks the network layer running, prints "Server
    started. Enter Q to quit." and starts the accept thread (section 2.1).
  * Client (client RVA 0x6fc50): the address text is first parsed as a dotted IPv4
    literal; if that fails it is resolved by host name and the first returned address is
    used. If resolution also fails the all-ones address is left in place and the connect
    attempt fails. It then connects to that address on port 12345.
* Sockets stay in default blocking mode with no options, timeouts or keep-alive.
* Every read is a "read exactly N bytes" loop that repeats `recv` until N bytes have
  arrived. A result of −1 ends the loop as an error. A result of 0 (orderly close) ends the
  loop early **without** an error; see 5.3.
* Every send is a plain `send` of exactly one field or buffer; a packet id and its payload
  are separate `send` calls. Message boundaries are therefore not aligned with `recv`
  boundaries.

### 1.2 How a client connection is started

* In-game chat commands (client RVA 0x7e1b0): `/connect <address>` stores the address and
  calls the connect routine; `/disconnect` calls the disconnect routine (client RVA
  0x719f0, section 5.2).
* The main menu also calls the connect routine with the text of an edit field (client RVA
  0x815e0). The client contains the menu strings "Multiplayer Worlds...", "Connect to
  server", "Server address" and the key "serverName".
* The connect routine returns at once if the address text is empty.
* Client command line (client RVA 0xc8ae0): if a first argument exists and equals
  `server`, a flag is passed to the game object's constructor; otherwise the first
  argument is converted to narrow text and passed instead. Their effect is unresolved
  (N-1).
* Status lines printed by the client: "Connected.", "Connection error.", "Connection
  error" (no full stop), "Error: could not connect to server.", "Error: Server has
  different version (x) than client (y)", "Error: Server is full.", "Disconnected.".

---

## 2. Handshake

All values are u32 unless stated.

| Step | Direction | Bytes | Meaning |
|---|---|---|---|
| 1 | C→S | `17` | hello |
| 2 | C→S | `3` | protocol version (client constant; the client displays its own version as the constant 0.03) |
| 3a | S→C | `16` | accepted |
| 3b | S→C | `3` | the Alpha server's reply to a wrong version (2.2) |
| 3c | S→C | `18` | server full: present in the server but unreachable (2.3) |
| 4 | S→C | `0`, u64 entity id, 4456 (0x1168) bytes | the joining player's entity id and the server's creature record for it, **not** compressed |
| 5 | S→C | `15`, u32 world seed | the world to generate |

After step 5 both directions switch to the message streams of section 3. There is no
login, name exchange or authentication; everything about the character reaches the server
in the client's creature updates (3.1).

### 2.1 Server order of operations (accept thread, server RVA 0x254a0)

The accept thread repeats the following while the network layer is marked running:

1. Sleep 200 ms, print "Waiting for connection.", `accept`. On failure print "Connection
   failed." and repeat.
2. Under the world lock, scan entity ids 1 to 10; as soon as one has a live entity whose
   controller byte (creature record offset 0x50) is 0 (a player), set a flag to 1 and stop
   the scan. (Used in step 5.)
3. Read u32. A read error closes the socket. If the value is not 17 the socket is closed
   with no message.
4. Read u32 version (a read error closes the socket) and print "Client version: v".
5. If the step-2 flag is 4 or more: send u32 `18`, sleep 500 ms, close, print "Server is
   full. Closing client connection." (unreachable, 2.3).
6. Otherwise, if the version is not 3: send u32 `3`, sleep 500 ms, close, print "Wrong
   client version. Closing client connection.".
7. Otherwise send u32 `16` and print "New connection.".
8. Choose the entity id: the **lowest id from 1 to 10** with no live entity in the world's
   entity table. If all ten are taken, close the socket with no message (the client has
   already received "accepted").
9. Under the world lock, create the player's creature with that id (its default contents
   are described in `alpha-net-server.md`).
10. Place a new connection object in the first empty slot of the connection list, or
    append one if no slot is empty; record the socket and the entity id in it.
11. Print "Player <slot index> joined.".
12. Send step 4 (u32 0, u64 id, the creature's 4456-byte record) and step 5 (u32 15, the
    world seed), each field a separate `send`.
13. Start two threads for the connection, each set to below-normal priority: first the
    **receiver** (server RVA 0x26020), then the **sender** (server RVA 0x23dd0), which is
    given the receiver's thread handle. Then return to step 1.

Accepting is single-threaded: a client that connects and then sends nothing blocks all
later joins until it sends or disconnects.

### 2.2 Version mismatch

The client (client RVA 0x6fc50) treats a reply of u32 `17` as a version report: it reads a
further u32 and prints "Error: Server has different version (" followed by that value ×
0.01, then ") than client (" followed by 0.03 and ")". The Alpha server never sends this
reply; for a wrong version it sends a bare u32 `3` (2.1 step 6). An Alpha client receiving
`3` takes its "any other reply" branch (2.4 step 3) and prints "Connection error".

### 2.3 "Server is full" is unreachable

The flag tested in 2.1 step 5 is only ever 0 or 1, while the test requires 4 or more, so
`18` is never sent. The effective limit is the id search of 2.1 step 8: at most **10
simultaneous players**; an eleventh connection is told "accepted" and then closed. The
client nevertheless handles `18` (2.4 step 3).

### 2.4 Client order of operations (client RVA 0x6fc50)

1. Mark the client "connecting", create the socket, resolve and connect (1.1). On connect
   failure print "Error: could not connect to server.", close the socket, clear
   "connecting" and stop.
2. Send u32 17, then u32 3. A send error prints "Connection error.", closes and stops.
3. Read the u32 reply (a read error prints "Connection error." and closes):
   * 17: read u32 version, print the version error (2.2), close;
   * 18: print "Error: Server is full.", close;
   * 16: continue;
   * anything else: print "Connection error", close.
4. Print "Connected.". Read u32 (a read error prints "Connection error." and closes).
   * If it is not 0, the routine ends with nothing further: the socket stays open, the
     client is not marked online and no network threads are started.
   * If it is 0, read the u64 id, then the 4456-byte record. (A read error here ends the
     routine without a message and without closing the socket.) The record is read into a
     local buffer that is never read afterwards: the server's record is **discarded**.
   * If the id is non-negative the client rebuilds its local state: it saves its own
     player creature's data, deletes every creature it knows, finds or creates the player
     creature again under the **server-assigned id**, and restores its saved data into it.
     The client therefore keeps its own character and adopts only the id. It also visits
     every loaded region of its 1024 × 1024 region grid, clears per-zone state in all
     8 × 8 zones of each, and sets refresh markers on the region and its contents (the
     purpose is unresolved, N-4).
   * Mark the client "online" and start the **receiver** (client RVA 0x6b740) and then the
     **sender** (client RVA 0x69c10) threads, each at below-normal priority. These two
     steps are also taken when the id is negative.

Packet 15 (the seed) is not read by the connect routine; it arrives through the receiver
thread like any other packet.

---

## 3. Steady state

After the handshake each direction is a sequence of messages:

    u32 packet id, then that packet's payload (fixed size or self-describing)

There is **no length prefix for the message as a whole**, no sequence number, no checksum
and no boundary marker. Neither receiver can skip a packet it does not understand: the
server receiver ignores ids other than 0 and 6–12 and the client receiver ignores ids other
than 0–5, 10 and 15, in both cases by reading the next u32 as a new id. Any payload
following an unknown id is therefore misread as further ids and data.

### 3.1 Client → server, per pass (client sender, client RVA 0x69c10)

Each pass of the client sender loop, while the client is online and no send has failed:

1. Send u32 **0**. Then, under the client lock, take and empty the three outgoing record
   queues (for packets 7, 8 and 9), take the first queued chat line if any, take and
   remove the first entry of the local player's 300-byte item record list if any, and
   build the creature update: the u64 player id followed by a delta against the last state
   sent (the first update after joining carries every field). Compress it and send u32
   compressed length, then the compressed bytes. Record the state just sent.
2. If a chat line was taken: packet **10**, u32 character count, then the characters as
   2-byte units.
3. If an item record was taken: packet **6** with the 300 bytes.
4. One packet **7** (72 bytes) per record taken from its queue, then one packet **8**
   (40 bytes) per record, then one packet **9** (112 bytes) per record.
5. Under the client lock, copy the lists of zones and map regions the client currently
   wants. If more than 1000 ms have passed since the last time this step sent: one packet
   **11** (8 bytes) per wanted zone, then one packet **12** (8 bytes) per wanted region.
   The lists themselves are not emptied by this, so wanted items are re-sent about once a
   second until the client stops wanting them. A send failure in this step ends the loop
   at the start of the next pass; failures in steps 1–4 end it at once.
6. Pacing (3.3).

### 3.2 Server → client, per pass (server sender, server RVA 0x23dd0)

Each pass of a connection's sender thread, while the network layer is running and the
connection's slot is occupied:

1. Under the world lock, select the creatures this client should receive (interest rules
   in `alpha-net-server.md`). For each selected creature: packet **0**, then u32 length and
   a zlib stream of the u64 id plus a delta against what this connection last received
   (every field if the creature was not in the previous batch).
2. Packet **2**, "end of creature batch". The client receiver then removes every creature
   that was not received in this batch, except its own player.
3. Packet **5** followed by two u32 world-clock values (layout in `alpha-net-packets.md`).
4. Packet **4**, then u32 length and one zlib stream holding thirteen lists: world events
   and the zone/region data requested by this connection.
5. If this connection's chat queue is non-empty, its first line is removed and sent as
   packet **10**: u64 sender entity id, u32 character count, then the characters as 2-byte
   units (one line per pass).
6. Pacing (3.3).

Packets 2, 5 and 4 are sent on **every** pass even when they carry nothing new, so this
stream acts as the keep-alive. If a packet-0 send fails, the pass still sends packet 2,
packet 5 and the packet-4 id before the loop ends; other send failures end the loop at
once.

The client receiver (client RVA 0x6b740) reads the millisecond clock on each packet-2
arrival and stores the interval since the previous packet 2 in a global (client RVA
0x36b048). How that value is used is unresolved (N-5).

### 3.3 Sender pacing

The client sender and each server sender thread end every pass the same way: read the
millisecond clock (with 1 ms timer resolution requested by the thread); if fewer than 20 ms
have elapsed since a stored timestamp, sleep for the remainder of those 20 ms; then store
the clock value read **before** sleeping. The stored timestamp is a single global per
process (server RVA 0x183dcc, client RVA 0x36b07c), shared by every sender thread in that
process.

Because the stored value excludes the sleep, a lone sender thread whose own work is
negligible alternates between a pass that sleeps and a pass that does not, giving roughly
two passes per 20 ms rather than one. This is derived from the arithmetic of the rule and
has not been measured; with several server sender threads sharing the timestamp the
resulting rate is unresolved (N-3).

The server's simulation loop is paced separately: it sleeps a fixed 20 ms after each tick
(5.1).

---

## 4. Compression

* On the network path zlib is used for exactly two payloads: creature updates (packet 0,
  both directions) and the world update (packet 4, server → client). The client also uses
  zlib outside the network path; that use is not covered here.
* Sending: the plain payload is compressed with the one-call compressor at its default
  level and sent as u32 compressed length followed by the compressed bytes. In the server
  (server RVA 0x12d00) the output buffer is sized to 1.1 × input length, rounded to an
  integer, plus 12 bytes, and then trimmed to the compressed size.
* Receiving: the u32 length is read, then that many bytes, then the data is inflated in a
  loop into a 128,000-byte (0x1f400) window, appending each window's output (server RVA
  0x12e20, client RVA 0x49540). The loop ends with success at stream end. It ends with
  failure on the data-error, memory-error or need-dictionary results; the output produced
  so far still replaces the payload and the failure is not reported to the receiving
  thread. Other results, including the "no progress possible" buffer-error result, do not
  end the loop. There is no limit on the inflated size and no uncompressed-length field.
* A compressed length of 0 for packet 0 is treated as fatal by both receivers: the
  receiver thread ends (5.2, 5.3).
* The handshake's 4456-byte record is **not** compressed.
* No other transformation (encryption, scrambling, checksumming) is applied to network
  data.

---

## 5. Threads, disconnects and timeouts

### 5.1 Server

* Main thread (server RVA 0x149c50), in order: start Winsock; create the world; read the
  world seed from `server.cfg`, or, if that file cannot be opened, use a random value
  below 1,000,000 and write it to `server.cfg`; prepare the world; create the network
  layer; start a world-generation thread at below-normal priority; call the listener
  (server RVA 0x28790), which starts the accept thread (server RVA 0x254a0); start a
  console thread; then run the simulation loop, sleeping a fixed 20 ms after each tick,
  until the console thread clears the running flag. It then prints "stopping generation
  thread...", stops and waits for the generation thread, destroys the network layer and
  world, and shuts down Winsock.
* Console thread: once per second it reads one character from standard input and stops
  the server when that character is a lowercase `q`. An uppercase `Q`, despite the
  start-up message, is not accepted.
* Per connection: one receiver thread and one sender thread (2.1 step 13).

### 5.2 Leaving

* There is **no disconnect packet** in either direction. A connection ends when its socket
  is closed or fails.
* Server receiver (server RVA 0x26020): the thread ends when the network layer stops
  running, when the connection's slot is empty, when reading a packet id returns −1, when
  a packet payload read returns −1, or when a packet-0 compressed length is 0. It does not
  close the socket or release anything itself.
* Server sender (server RVA 0x23dd0): when its loop ends (a failed send, the network layer
  stopping or an empty slot), it **waits for the receiver thread to end**, then, under the
  world lock and only if the slot is still occupied: closes the socket, deletes the
  player's creature and removes it from the entity table, deletes the connection object
  and empties the slot. The freed id (1–10) is available to the next join.
* Client receiver (client RVA 0x6b740): ends when reading a packet id returns −1, when a
  packet-0 compressed length is 0 or its payload read fails, or when the client is no
  longer online. It does not close the socket, print a message or clear "online".
* Client sender (client RVA 0x69c10): when its loop ends, if the client is still marked
  online it closes the socket, forgets it and clears "connecting"; it prints nothing and
  does not clear "online".
* Explicit disconnect (client RVA 0x719f0, reached from `/disconnect` and from menu
  routines): only if the client is online, it clears "online", closes the socket if one is
  held, waits for both network threads to end, clears "connecting" and prints
  "Disconnected.".

### 5.3 Behaviour of the read loops

* A `recv` result of 0 ends a "read exactly" loop early without an error, and the
  partially filled field or buffer is used as it stands. Both receivers clear the packet-id
  field and the packet-0 length field before reading them, so an orderly close at a
  message boundary reads as packet id 0 followed by a length of 0, which ends the receiver
  thread (5.2). A close inside any other field leaves that field partially filled and it
  is processed as received.
* Payload reads for packet 0 report only −1 as an error, so a short payload caused by an
  orderly close is passed to the inflater.
* There are no timeouts: a peer that stops sending but keeps its socket open is kept
  indefinitely.

---

## 6. Unresolved

* **N-1** Effect of the client command-line `server` flag and of the first-argument text
  passed to the game object (client RVA 0xc8ae0).
* **N-3** Actual per-connection send rate of the server when several sender threads share
  the per-process pacing timestamp, and the measured cadence of a single sender (3.3).
* **N-4** Purpose of the per-zone state reset and refresh markers set by the client after
  adopting the server-assigned id (2.4 step 4), in particular whether they cause zone data
  to be requested again.
* **N-5** Use of the packet-2 interval stored by the client receiver (client RVA
  0x36b048).
* **N-6** Whether the client restricts its outgoing packet 7, 8 and 9 queues to records
  produced by the local player; the code that fills those queues was not traced.
* **N-7** Which shutdown step, if any, clears the network-layer running flag watched by the
  accept and per-connection threads.
* **N-8** Zone-index rounding. In the server main loop's per-player zone computation
  (server RVA 0x149c50) a position is divided by 65536 and then by 256, each step rounding
  toward zero. Rounding used by other interest code is not covered here.

---

## Re-certification (2026-11-14)

Re-checked against the reference images (hashes above), their disassembly, the PE import
tables (read with dumpbin) and the Alpha decompile. Every address is now an RVA with its
image named. No reconstruction source was consulted.

**Confirmed:** TCP/IPv4 only, with no datagram, socket-option or non-blocking imports;
Winsock 2.2 start-up; fixed port 12345; listen backlog 10 and the listener messages;
client parse-then-resolve address order; read-exactly loops; separate sends per field;
the handshake values 17/3/16/3/18/0/15, the 4456-byte uncompressed record and the seed;
version 3 and the client's 0.01 and 0.03 display constants; the 200 ms accept delay; the
500 ms wait before close; the lowest-free-id search over 1–10; first-free-slot placement;
receiver-then-sender thread start at below-normal priority (both images); the "Server is
full" flag that can never reach 4; the client's version, full, accepted and other reply
branches; the client-to-server pass order (0, 10, 6, 7, 8, 9, then 11 and 12 once a
second); the server-to-client pass order (0 per creature, 2, 5, 4, 10); packet sizes 72,
40, 112, 300 and 8; the per-process pacing globals; packet-2 interval timing; the
thirteen lists in packet 4; the compress-buffer rule 1.1 × n + 12; the 128,000-byte
inflate window; no uncompressed-length field; the fatal zero length; no disconnect packet;
no timeouts; ignoring unknown ids without resynchronising; packet 2 removing creatures that
were not received.

**Corrected** (old → new):
1. *Reversal.* "Passes are at least 20 ms apart" → the stored timestamp is the pre-sleep
   clock value, so passes alternate and a lone thread runs about two passes per 20 ms
   (derived, not measured) (3.3).
2. *Reversal.* A zero-length `recv` "spins until the sender notices" → both receivers
   pre-clear the id and length fields, so an orderly close at a message boundary reads as
   packet 0 with length 0 and ends the receiver thread (5.3).
3. *Reversal.* Client "prints Disconnected./Connection error. and returns to offline play"
   on a socket error → neither client thread prints anything or clears "online";
   "Disconnected." comes only from the explicit disconnect routine (5.2).
4. *Reversal.* N-2, "the server record does not appear to be used" (open) → confirmed
   discarded: it is read into a local buffer that is never read afterwards (2.4).
5. *Reversal.* Zone index "rounding toward negative infinity for the interest code" → the
   checked main-loop computation truncates toward zero at both steps; other code moved to
   N-8.
6. Server sender "closes the socket on leaving" → it first waits for the receiver thread,
   then closes the socket and frees everything, and only if the slot is still occupied
   (5.2).
7. The "Server is full" player scan runs after `accept` and before the hello is read, not
   after the version is read (2.1).
8. Console quit "Q" → only a lowercase `q`, polled once per second (5.1).
9. Main thread "starts the listener and an accept thread" → the listener routine itself
   starts the accept thread; added the `server.cfg` seed default and write-back, and the
   fixed 20 ms simulation sleep (5.1).
10. Client zero packet-0 length "disconnects" → only the receiver thread ends (4, 5.2).
11. Client packet-0 order clarified: the u32 0 is sent before the update is built under
    the client lock (3.1).
12. Added: the "full" branch's own message and timing; "Connection failed." on accept
    failure; the client's handling of a non-zero post-accept value and of read failures
    during the handshake; the all-ones address after a failed resolution; chat field
    layouts; the continuation of a pass after a failed packet-0 send.
13. Added: inflate termination rules (silent partial output on failure; buffer-error does
    not end the loop) and the server packet-0 payload read reporting only −1 (4, 5.3).
14. zlib ordinal names are now stated as inferred from argument patterns rather than as
    fact (0).
15. Section references into companion documents were removed; their names remain as
    pointers only.

**Removed:** every sentence addressed to a replacement implementation, including "an
implementation does not need them", "Never send an id the peer does not understand", "A
reimplementation should not copy this", the AlphaVulkan server/client advice in 2.2 and
2.3, "an implementation does not need to reproduce zlib 1.2.3's exact bytes", "An
AlphaVulkan sender must never send an empty stream", the AlphaVulkan advice in 5.3, the
whole section "What an AlphaVulkan implementation must do to interoperate", and the
verification procedures attached to the open questions. Also removed: the unverified
statement that the asset-store byte-swap transform does not apply to saves (outside this
document's evidence); the "creature interpolation" purpose of the packet-2 interval (now
N-5); the "about 50 Hz" contract figure (contradicted by item 1); the claim that packet 7,
8 and 9 records are filtered by the local player's id (now N-6).

**Unresolved:** N-1, N-3, N-4, N-5, N-6, N-7, N-8 (section 6).
