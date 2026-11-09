# Alpha multiplayer: transport, handshake and framing

Behavioural specification of how Cube World **Alpha** clients (`Cube.exe`, SHA-256
`84a7a132…30bf`) and the Alpha dedicated server (`Server.exe`, SHA-256 `1ffbcbce…34fb1`)
connect and frame their traffic. Written from the Alpha decompile
(`disassembled alpha cube world/decompiled/client` and `…/server`), their assembly and the
binaries' constants. Companion documents:

* `alpha-net-packets.md`: every packet and record layout, test vectors.
* `alpha-net-server.md`: what `Server.exe` simulates, interest rules, joins and leaves.
* `alpha-saves.md`: the save databases (the server writes the same world format).

Routines are cited by **address** and binary, e.g. "server 0x4254a0" or "client 0x46fc50"
(image base 0x400000 in both; client and server addresses are separate address spaces even
when the numbers are equal). Offsets into in-memory objects are given only so a reader can
cross-check; an implementation does not need them. Steam Cube World's protocol was **not**
consulted and must not be assumed: Alpha's protocol is a different, much simpler design.

---

## 0. Conventions

* All integers are **little-endian**, two's complement. "u32" is a 4-byte unsigned integer,
  "i32" signed, "u64"/"i64" 8-byte, "f32" an IEEE-754 single. Nothing is aligned on the wire:
  fields follow each other with no padding unless a record says so (records copied whole
  from memory keep their in-memory padding; such padding bytes are listed and should be sent
  as zero).
* **Packet id** means the u32 that starts every message. Ids are small integers and are
  written as decimals in this document (0x11 = 17, 0x10 = 16, 0x12 = 18, 0x0f = 15).
* World positions are i64 in 1/65536 of a block. One zone is 256 blocks (a position divided
  by 65536 and then by 256, rounding toward negative infinity for the server's interest
  code, gives the zone index). See the world-generation specs for zones, regions and seeds.
* "Block units" for distances: the server converts i64 position differences to float and
  multiplies by 1/65536 (constant at server 0x55869c = 1.52587890625e-05).
* "zlib stream" means an RFC 1950 stream (2-byte header, deflate data, Adler-32), as made by
  zlib's `compress()` and read by `inflate()`. Both binaries import `zlib1.dll` by ordinal:
  11 = `compress`, 58 = `inflateInit_` (version string "1.2.3"), 50 = `inflate`,
  55 = `inflateEnd`.

---

## 1. Transport

### 1.1 One TCP connection per player

* Protocol: **TCP over IPv4 only**. No UDP is used anywhere (neither binary imports
  `sendto`/`recvfrom`). Winsock 2.2 is started at launch (`WSAStartup(0x202)`: server
  0x549c50, client 0x4c8ae0).
* Port: **12345** (0x3039), fixed in both binaries; there is no setting for it.
  * Server (0x428790): `socket(AF_INET, SOCK_STREAM, 0)`, bind to 0.0.0.0:12345, `listen`
    with backlog 10. Console messages: "Starting server...", "Could not create server
    socket.", "Could not bind socket.", "Listen failed.", "Server started. Enter Q to quit."
  * Client (0x46fc50): the address string is first tried as a dotted IPv4 literal
    (`inet_addr`); if that fails it is resolved with `gethostbyname` and the first address
    is used. Then `connect` to that address, port 12345.
* Sockets are **blocking**, with no socket options set (no `setsockopt`, no `TCP_NODELAY`,
  no receive timeout, no keep-alive). Every read is a "read exactly N bytes" loop:
  `recv` is repeated until N bytes have arrived. A `recv` result of −1 (`SOCKET_ERROR`) is
  treated as a broken connection. A result of 0 (orderly close) only ends the inner loop;
  see 5.3 for the consequences.
* All sends are plain `send` calls of exactly the field sizes; a header and its payload are
  usually separate `send` calls, so a receiver must never assume one `recv` = one message.

### 1.2 How a player connects (client side)

* In-game chat command `/connect <address>` (client 0x47e1b0; `/disconnect` closes),
  or the main-menu "Multiplayer Worlds... / Connect to server" dialog ("Server address"
  field, stored with the menu key `serverName`).
* The client's command line: if the first argument is the word `server` a flag is set
  (purpose not traced); otherwise the first argument is converted to text and kept,
  apparently as an address to use (client 0x4c8ae0). **Open question N-1.**
* Status lines the client prints: "Connected.", "Connection error.", "Error: could not
  connect to server.", "Error: Server has different version (x) than client (y)",
  "Error: Server is full.", "Disconnected.".

---

## 2. Handshake

All values are u32 unless stated.

| Step | Direction | Bytes | Meaning |
|---|---|---|---|
| 1 | C→S | `17` | hello (0x11) |
| 2 | C→S | `3` | protocol version (client constant; shown to the user as version × 0.01 = "0.03") |
| 3a | S→C | `16` | accepted (0x10) |
| 3b | S→C | `3` | **server's reply to a wrong version** (see 2.2) |
| 3c | S→C | `18` | server full (0x12): coded but unreachable (see 2.3) |
| 4 | S→C | `0`, then u64 entity id, then 4456 (0x1168) raw bytes | your entity id and the server's initial creature record for you (**not** compressed) |
| 5 | S→C | `15`, then u32 world seed | the world to generate (0x0f) |

After step 5 the connection switches to the steady-state streams of section 3. There is no
login, no name exchange and no authentication: the player's name and everything else about
the character arrive in the client's first creature update (section 3.1).

### 2.1 Server order of operations (server 0x4254a0)

1. Wait 200 ms, print "Waiting for connection.", `accept`.
2. Read u32. If it is not 17, close the socket silently and go back to accepting.
3. Read u32 version, print "Client version: v".
4. (Full check, see 2.3.)
5. If version ≠ 3: send u32 `3`, wait 500 ms, close, print "Wrong client version. Closing
   client connection."
6. Send u32 `16`, print "New connection.".
7. Choose the entity id: the **lowest id from 1 to 10** that has no live entity in the
   world's entity table. If all ten are in use, close the socket silently (the client has
   already been told "accepted").
8. Create the player's creature (see `alpha-net-server.md` §3.1 for its default contents).
9. Put the connection in the first empty slot of the connection list (or append).
10. Print "Player <slot> joined.".
11. Send step 4 (u32 0, u64 id, the creature's 4456-byte record) and step 5 (u32 15, seed).
12. Start two threads for this connection: a **receiver** (server 0x426020) and a
    **sender** (server 0x423dd0), both at below-normal priority. Then go back to accepting.

The accept loop is single-threaded: a client that connects and sends nothing blocks every
later join until it disconnects. A reimplementation should not copy this.

### 2.2 Version mismatch (an Alpha quirk to reproduce)

The client (0x46fc50) expects a mismatch to be reported as u32 `17` followed by the server's
u32 version, and then prints "Error: Server has different version (s × 0.01) than client
(0.03)". The Alpha server never sends that: it sends a bare u32 `3`. A real Alpha client
receiving `3` falls into its "anything else" branch and prints "Connection error". For wire
compatibility:

* An AlphaVulkan **server** talking to an unknown client version should do what Alpha does
  (send `3`, wait about 500 ms, close). It may instead send `17` + its version to get the
  nicer client message; Alpha clients understand both.
* An AlphaVulkan **client** must treat `3`, `17`+version, and any other unexpected reply as a
  refused connection.

### 2.3 "Server is full" is unreachable in Alpha

Before the version test the server scans ids 1–10 and sets a flag to 1 as soon as it finds
an id whose entity is a player (the creature's "controller" byte, record offset 0x50, is 0);
it then sends `18` only if the flag is **4 or more**, which can never happen. The real
limit is therefore the id search in 2.1 step 7: at most **10 simultaneous players**, and the
eleventh is accepted and then dropped. An AlphaVulkan server should send `18` and close when
all ten ids are taken (Alpha clients print "Error: Server is full."), which is what the
Alpha code evidently intended.

### 2.4 Client order of operations (client 0x46fc50)

1. Mark "connecting", resolve and connect (1.1). Failure: "Error: could not connect…".
2. Send u32 17, u32 3.
3. Read u32 reply: 17 → read u32 version, print the version error, close; 18 → "Error:
   Server is full.", close; 16 → continue; anything else → "Connection error", close.
4. Print "Connected.". Read u32; if it is 0, read u64 id and 4456 bytes. If the id is
   non-negative the client rebuilds its local state: it saves its own player creature
   (record, inventory, the extra persistent data and two loose fields), deletes every
   creature it knows, creates its player again under the **server-assigned id**, and copies
   its own saved character data back in. In other words the client keeps its own character
   and adopts only the id; the server's 4456-byte record is read but does not appear to be
   used. **Open question N-2.** It also clears per-zone dynamic state in every loaded
   region so that it will be re-requested from the server.
5. Mark "online", start the **receiver** (client 0x46b740) and **sender** (client 0x469c10)
   threads at below-normal priority.

Packet 15 (the seed) is handled by the receiver thread like any other packet (see
`alpha-net-packets.md` §3.9).

---

## 3. Steady state

After the handshake each direction is a sequence of messages:

    u32 packet id, then that packet's payload (fixed or self-describing size)

There is **no length prefix on the message as a whole**, no sequence number, no checksum
and no message boundary marker. A receiver that does not know a packet id cannot skip it:
both Alpha receivers ignore an unknown id by simply reading the next u32 as a new id, which
desynchronises the stream. Never send an id the peer does not understand.

### 3.1 Client → server, one round per tick (client 0x469c10)

Each pass of the client sender loop, in this order:

1. Packet **0**, creature update for the local player: the u64 id plus a delta against the
   last state sent (the first update after joining carries every field), zlib-compressed.
2. Packet **10**, chat, if a chat line is queued (one line per pass).
3. Packet **6**, if the local player's 300-byte item record queue is non-empty (one per
   pass; see the packets document).
4. Packets **7** (hit, 72 bytes), **8** (40 bytes), **9** (shoot, 112 bytes): one packet per
   record produced this pass by the local simulation whose first 8 bytes are the local
   player's id.
5. If at least 1000 ms have passed since the last time this was done: packet **11** for
   every zone the client currently wants, and packet **12** for every map region it wants
   (8 bytes each). These lists are not cleared by sending; the client keeps asking once a
   second until it no longer wants the item.
6. Sleep so that passes are at least **20 ms** apart (a pass that took longer is not caught
   up).

### 3.2 Server → client, one round per tick (server 0x423dd0)

Each pass of a connection's sender thread, in this order:

1. Zero or more packet **0** messages: one per creature this client should see, each a zlib
   stream of u64 id + delta (section 4 of the packets document).
2. Packet **2**: "end of creature batch". The client removes every creature it did not
   receive in this batch (except itself).
3. Packet **5**: day number and time of day.
4. Packet **4**: one zlib stream holding thirteen lists of world events and requested
   zone/region data.
5. Packet **10**, if a chat line is queued for this client (one per pass).
6. Sleep so that passes are at least **20 ms** apart.

Packets 2, 5 and 4 are sent **every** pass even when they carry nothing, so the stream itself
is the keep-alive: an idle Alpha server still sends about 50 small messages per second to
each client. The client measures the interval between packet-2 arrivals (client
0x46b740, stored for its creature interpolation).

The 20 ms pacing timestamp is a single global per process (server 0x583dcc, client
0x76b07c), shared by all sender threads; with several clients the server's pacing is
therefore approximate. Treat "about 50 Hz per connection" as the contract.

---

## 4. Compression

* Used for exactly two payloads: creature updates (packet 0, both directions) and the
  world update (packet 4, server→client).
* The sender builds the plain payload, compresses it with zlib `compress()` (default level;
  the output buffer is sized 1.1 × input + 12, server 0x412d00), then sends u32 compressed
  length followed by the compressed bytes.
* The receiver reads the u32 length and that many bytes, then inflates in a loop with a
  128,000-byte (0x1f400) window buffer until `inflate` reports stream end (server 0x412e20,
  client 0x449540). There is no limit on the inflated size, and no uncompressed-length field.
* Any valid zlib stream is accepted; an implementation does not need to reproduce zlib
  1.2.3's exact bytes.
* A length of 0 is treated by the client as a fatal error for packet 0 (it disconnects).
  An AlphaVulkan sender must never send an empty stream.
* The handshake's 4456-byte creature record is **not** compressed.
* Nothing on the wire is scrambled: the byte-swap transform of `alpha-data-stores.md` applies
  only to the asset databases `data1.db`–`data4.db`, never to network data or saves.

---

## 5. Threads, disconnects and timeouts

### 5.1 Server

* Main thread (server 0x549c50): loads `server.cfg`, builds the world, starts a world
  generation thread, starts the listener (0x428790), starts an accept thread (0x4254a0) and
  a console thread that sets the "running" flag false when the operator types Q. Then it
  runs the simulation loop (see `alpha-net-server.md` §2) until running is false, and then
  prints "stopping generation thread..." and shuts down.
* Per connection: one receiver thread and one sender thread (2.1 step 12).

### 5.2 Leaving

* There is **no disconnect packet** in either direction. A player leaves by closing the TCP
  connection; the client's `/disconnect` just closes its socket.
* Server: when a `send` in the sender thread returns −1, or the connection's slot has been
  cleared, the sender thread leaves its loop, closes the socket, deletes the player's
  creature from the world, deletes the connection object and empties its slot. The receiver
  thread stops when `recv` returns −1, when it sees the slot emptied, or when the server
  stops. The freed id (1–10) is reused by the next join.
* Client: on a socket error either client thread stops and the client prints
  "Disconnected."/"Connection error." and returns to offline play.

### 5.3 Quirks of the read loops

* A `recv` result of 0 (peer closed cleanly) ends the "read exactly" loop early **without**
  reporting an error, so the half-filled buffer is processed and the thread loops, spinning
  on further zero-length reads until the sender side notices the dead socket. An AlphaVulkan
  implementation should treat 0 as a disconnect.
* There are no timeouts at all: a peer that stops sending but keeps the socket open is kept
  forever. AlphaVulkan may add a timeout (several seconds without a packet 2 from the server,
  or without a packet 0 from a client, is a safe signal because both are sent every 20 ms).

---

## 6. What an AlphaVulkan implementation must do to interoperate

1. Listen/connect on TCP 12345, IPv4.
2. Speak the handshake of section 2 exactly, including version 3 and the 4456-byte record.
3. Follow the per-tick message order of 3.1/3.2. The order matters to Alpha clients: packet 2
   ends the creature batch and deletes unlisted creatures, so a server must list every
   creature the client should keep in every batch (with an empty change mask if nothing
   changed).
4. Never send an unknown packet id; never send a zero-length compressed payload.
5. Keep the ~20 ms cadence (faster is tolerated by Alpha peers; much slower makes Alpha
   clients' remote creatures stutter).

---

## 7. Open questions (and how to settle them)

* **N-1** Client command-line `server` flag and address argument: purpose. Verify with a
  32-bit mod DLL hook on client 0x4c8ae0, or by launching `Cube.exe 127.0.0.1` against a test
  server and watching for an automatic connect.
* **N-2** Whether the server-sent 4456-byte record in the handshake is used by the client
  for anything. Verify by having a test server send a record with a distinctive name and
  position and checking whether the client shows them.
* **N-3** Whether Alpha's per-process 20 ms pacing makes multi-client servers send slower
  than 50 Hz per connection. Capture with two clients connected and time the packet-2
  messages per connection.
