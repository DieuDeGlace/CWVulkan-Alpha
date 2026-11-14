Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha remote creature smoothing: the receive paths, the smoothing flag at +0x1398, the drawn pose and what the world update runs for creatures received over the network

## Scope and evidence

This contract describes how a connected Alpha client receives creatures from the server, how it keeps a separate drawn pose for them, and how that drawn pose approaches the live (received and locally advanced) pose inside the world creature update. The held contract `alpha-beam-height-leftovers.md` was read only to locate the interpolation block (its section 1.6) and for terms. Every statement below was checked against the original images.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were recomputed before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs. Evidence:

- The original instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`, and the call list `decompiled/client/calls.csv` (also used to identify runtime imports: cosine, sine, arcsine, 64-bit multiply and divide, and the time functions).
- The client network receive thread `0x06B740`. Its message-type cases are not decoded by the listing; they were decoded from the image bytes (range `0x06B8B0`–`0x06D180`) with an independent disassembler, and the dispatch tables were read from the image bytes at `0x06D180` and `0x06D1A4`.
- Byte-pattern searches of both whole images for the 32-bit displacements of the fields named below, for absolute references and for relative calls to the receive routines.
- The isolated Infinite-Cubes reconstruction (alpha part only) was consulted for naming leads. Such leads are marked "(reconstruction lead)". Nothing else from it is relied on.

Terms. S is the creature being updated. The **record** is the networked entity record at creature `+0x10`; record offset r is creature offset r + `0x10`. **Step** is the world update's first argument (signed 32-bit milliseconds, stored at `0x20C54F`). **Local creature** is world `+0xB8`; **client world** means world byte `+0xB4` nonzero. Fixed values are signed 64-bit with 65536 units per world unit. Creature fields used here:

| Field | Width | Meaning used here |
|---|---|---|
| `+0x10` | 3 × signed 64 | live position (record `+0x00`) |
| `+0x28`, `+0x2C`, `+0x30` | binary32 each | live rotation components 0, 1, 2 (record `+0x18`) |
| `+0x40` | 3 × binary32 | live velocity (record `+0x30`) |
| `+0x16C` | binary32 | health (record `+0x15C`) |
| `+0x160` | 3 × binary32 | target of the drawn aim (record `+0x150`; 2.4) |
| `+0x1C0` | 3 × signed 64 | the **snap field** (record `+0x1B0`); reconstruction lead: "origin" |
| `+0x1338` | 3 × signed 64 | saved drawn position |
| `+0x1350` | 3 × signed 64 | **drawn position** |
| `+0x1368` | 3 × binary32 | saved drawn rotation |
| `+0x1374` | 3 × binary32 | **drawn rotation** |
| `+0x1380` | 3 × binary32 | saved drawn aim |
| `+0x138C` | 3 × binary32 | **drawn aim** (reconstruction lead: "drawn aim") |
| `+0x1398` | signed 32 | **smoothing flag** (called a counter in the task) |
| `+0x11DC` | container of 12-byte elements | cleared on a snap (meaning unresolved) |

Helpers, all fully audited:

- **Approach** `0x011D40`: given a binary32 value, a binary32 target, a signed repeat count and a binary32 rate, repeats count times: value = ((target − value) × rate) + value, each operation rounded to binary32 and stored. A count of 0 or less does nothing. The body is unrolled by four with no change of operation order.
- **Fraction f** (used in Section 2): Approach applied to a value starting at +0.0, target 1.0, repeat count = step, rate 0.015 (binary32 `0x3C75C28F`).
- **Fraction g** (Section 2.4): the same with rate 0.025 (binary32 `0x3CCCCCCD`).
- **T(x)** `0x02C580`: x multiplied in binary32 by 65536.0, then converted to signed 64-bit by the runtime conversion `0x28D946`, which truncates toward zero.
- **Angle step** `0x023F70`, described in 2.3.

## 1. The smoothing flag +0x1398 and the fields it governs

### 1.1 Every access (complete by displacement)

A search of each whole image for the displacement `0x1398` gives these instruction sites and no others (the remaining pattern hits lie inside the displacement of unrelated call instructions: client `0x159823`, server `0x020513` and `0x09C173`):

| Image | Site | Routine | Access |
|---|---|---|---|
| client | `0x03B890` | creature constructor `0x03B690` | set to 0 |
| client | `0x04646B` | creature reset `0x046330` | set to 0 |
| client | `0x047198` | reset routine `0x047110` (called only from the client controller at `0x097395`) | set to 0 |
| client | `0x06BD0D` | receive thread, message type 0, smoothing path | set to 60 (`0x3C`) |
| client | `0x06BD6F` | receive thread, message type 0, snap path | set to 0 |
| client | `0x06C709` | receive thread, message type 1, smoothing path | set to 60 |
| client | `0x06C7B1` | receive thread, message type 1, snap path | set to 0 |
| client | `0x21FB80` | world creature update `0x20C510` | signed test: greater than 0 |
| server | `0x006600` | creature constructor `0x006400` | set to 0 |
| server | `0x01120B` | creature reset `0x0110D0` | set to 0 |
| server | `0x145940` | world creature update `0x1322D0` | signed test: greater than 0 |

That is seven client stores, one client test, two server stores and one server test.

**The value is never decremented.** No site adds to, subtracts from or decrements it. Once a receive sets it to 60, it stays 60 until a later receive for the same creature (or a reset) sets it to 0. The value 60 therefore acts as a flag: only "greater than 0" is ever tested. An access through a base register that points elsewhere in the creature (for example into the drawn position) was not excluded exhaustively. The reconstruction's comment that the value "counts down" is not supported by the original.

On the **server** the flag is only ever 0, so its world update always takes the copy branch of 2.1.

### 1.2 Write-only saved copies

The saved drawn position `+0x1338`, saved drawn rotation `+0x1368` and saved drawn aim `+0x1380` are written only by the receive thread (sites listed in 1.3 and 1.4). A displacement search of the client image finds no reader of any of them. In the server image the same search finds no instruction using any of the three displacements (its only hits lie in read-only data or inside an unrelated call). They have no observable effect in the original (an indirect reader was not excluded exhaustively).

### 1.3 Message type 0: one full creature (client `0x06BAAD`–`0x06BE20`)

The receive thread reads a 4-byte message type from the socket and dispatches it through the byte table at `0x06D1A4` and the address table at `0x06D180`: type 0 to `0x06BAAD`, type 1 to `0x06C296`, type 2 to `0x06BE25`; types 3, 4, 5, 10 and 15 go to cases outside this contract; every other value is ignored. The thread runs while controller byte `+0x800585` is nonzero and no disconnect has been flagged.

For type 0, in this order:

1. A 4-byte length L is read. If L is 0, or any socket read fails, a disconnect is flagged and nothing else happens.
2. L bytes are read into a buffer, which is then passed to `0x0494B0` (its callee `0x049540` calls the zlib import; treated as decompression, not audited).
3. An 8-byte creature identifier is taken from the start of the buffer.
4. With the world lock (`0x201CB0` on controller `+0x2E4`) held: the identifier is looked up in the world's creature map (`0x02F000`). If it is absent, a 0x1E60-byte creature is allocated, built by the constructor with that identifier, and stored in the map under the identifier; the creature is marked **new**. The creature's current record is copied (`0x066050`) into a temporary record. The lock is released (`0x201E90`).
5. The field reader `0x0CCFA0` applies the buffer to the temporary record (1.6). A failure flags a disconnect; a new creature created in step 4 is then left in the map.
6. The identifier is inserted into the thread's **seen set** (an ordered set built at `0x06B7C8`–`0x06B7E1`, persisting across messages).
7. With the lock held again: if the creature is the local creature (controller `+0x8006D0`), nothing is applied: **the received data for the local creature is discarded**. Otherwise:
   - **Snap** when the creature is new, or the 24-byte snap field differs in any 32-bit word between the creature (`+0x1C0`, still the old value) and the temporary record (helper `0x02C680`), or 0.0 ≥ the creature's current health `+0x16C` (the old value; a NaN health does not snap). The snap path (`0x06BD69`–`0x06BDEC`) sets the flag to 0, empties the container `+0x11DC` (`0x03E630`), sets the drawn position to the temporary record's position, sets the three drawn rotation components to the temporary record's rotation components, then copies the drawn position into `+0x1338` and the drawn rotation into `+0x1368`.
   - **Smooth** otherwise (`0x06BD00`–`0x06BD64`): the flag becomes 60; then the drawn position is copied into `+0x1338`, the drawn rotation into `+0x1368` and the drawn aim into `+0x1380`. The drawn pose itself is not changed.
   - In both cases the whole temporary record is then copied into the creature's record (`0x04B040`, at `0x06BDFF`). The lock is released.

### 1.4 Message type 1: a batch of delta records (client `0x06C296`–`0x06CA19`)

1. A 4-byte count N is read (a failure flags a disconnect).
2. With the lock held, a local map from identifier to record is filled with a copy of the record of every existing (non-empty) creature (`0x06C2F8`–`0x06C34F`). The lock is released.
3. N times: an 8-byte identifier is read; the stream receiver `0x0CD3E0` reads that creature's fields directly from the socket into the local map's record for the identifier (a default record built by `0x03C100` when the identifier was not present); the identifier is inserted into a per-message **received set**. A failure flags a disconnect and ends the message.
4. The local creature's identifier is inserted into the received set.
5. With the lock held: every non-empty creature in the world map whose identifier is not in the received set is **removed**: its virtual deleting destructor is called and its map entry erased (`0x03EDE0`). With N = 0 this removes every creature except the local one.
6. Still holding the lock, for each identifier of the received set in set order (`0x06C620`–`0x06C9BD`): the creature is looked up; if absent (or stored empty) it is created as in 1.3 step 4 and marked new. The local creature is skipped. Otherwise the same decision as 1.3 step 7 is made (the snap-field comparison is inline, word by word; the health test uses the creature's old health):
   - **Smooth** (`0x06C709`–`0x06C79D`): flag 60, then `+0x1338` ← drawn position, `+0x1368` ← drawn rotation, `+0x1380` ← drawn aim.
   - **Snap** (`0x06C7AB`–`0x06C954`): flag 0; the container `+0x11DC` is emptied inline (every element released, then emptied); the drawn position and drawn rotation are set from the map record; `+0x1338` ← drawn position. **Unlike type 0**, `+0x1368`, `+0x136C` and `+0x1370` receive the drawn position's x, y and z converted to float (integer load to the x87 unit, store to binary32, reload and store again, then binary32 multiply by 2⁻¹⁶, constant at image `0x2FCD94`), not the drawn rotation. Since 1.2 finds no reader, the difference is unobservable.
   - In both cases the map record is copied into the creature's record (`0x04B040`).
7. The lock is released and the local containers are freed.

### 1.5 Message type 2: end of a full update (client `0x06B85A`–`0x06B8A1` and `0x06BE25`–`0x06BFAB`)

Before dispatch, a type-2 message takes the system time (`timeGetTime`) and, under the lock, stores the difference from the previous type-2 time in the global at image `0x36B048` when a previous time exists; the global has no reader in the client image. Then, under the lock, every non-empty creature that is not the local creature and whose identifier is not in the seen set of 1.3 is removed (deleting destructor, then map erase), and the seen set is emptied. So creatures sent with type 0 are kept by a later type 2 only if they were sent since the previous type 2; creatures sent with type 1 are kept by membership in that same message.

### 1.6 What is received and where it goes

- Both readers use one scheme: a 64-bit presence mask, then the record fields in a fixed order, each read only when the mask bit at the running field index is set, the index advancing by one per field for every reader audited (the 24-byte reader `0x0CC5C0`/`0x0CC650` and the 12-byte reader `0x0CC540`/`0x0CC4C0`). The field reader takes its bytes from the buffer (a short buffer leaves the field unchanged); the stream receiver reads them from the socket. The first five fields are record `+0x00` (position), `+0x18` (rotation), `+0x24`, `+0x30` (velocity) and `+0x3C`. Record `+0x150` (aim vector), `+0x15C` (health) and `+0x1B0` (snap field) are among the later fields. The remaining field readers were bounded only by these call sites, not audited.
- **Every received field lands in the live record** through the whole-record copy. No received value goes to the drawn pose, except that a snap sets the drawn position and rotation from the received position and rotation. The smoothing targets of Section 2 are the live position `+0x10` and the live rotation `+0x28`…`+0x30`. The drawn aim is never set by a receive.
- Fields absent from a delta keep the value the creature had when the message was processed (type 1 starts from a copy of the creature's record; type 0 from a copy of the creature's record at step 4).
- New creatures start from the constructor's default record (`0x03C100`), with the drawn position and rotation zero; the first receive always snaps, so the drawn pose then equals the received pose. The constructor does not write the drawn aim or the saved copies by displacement (their initial contents are unresolved).

## 2. The world-update block 0x21FB80–0x21FDFC (client; server 0x145940 onward)

### 2.1 Branches

For every creature that reaches `0x21F968` (2.5), when byte `+0x60` is 5 and the creature with the identifier at `+0x198` exists and has action `0x6A`, the attachment copy runs and this block's two pose branches are skipped (the held contract, 1.6); that path also sets `+0x34` and the velocity `+0x40` to zero and rejoins at 2.4. Otherwise:

- **Flag ≤ 0** (`0x21FD36`–`0x21FD4F`): the drawn position becomes an exact copy of the live position, and the drawn rotation an exact copy of the three live rotation components. The drawn-height offset `+0x1180` is not touched.
- **Flag > 0** (`0x21FB8D`–`0x21FD34`): the smoothing of 2.2 and 2.3, then `+0x1180` is set to +0.0 (`0x21FD2A`). The flag is not changed.

The live pose used as the target is taken through pointers set earlier in the same creature's update: position at `0x21B44B` (creature `+0x10`), rotation component 0 base at `0x2191C7`/`0x2191E3` (creature `+0x28`), component 1 at `0x219204` (creature `+0x2C`) and component 2 at `0x21D7CC` (creature `+0x30`). These stores lie on every path through the movement span examined (two alternative arms join before `0x2191F6`; `0x21D7AC` is a join point).

### 2.2 Position smoothing (flag > 0)

f is computed once (helper at `0x21FBBD`) and reused for every component. Then:

1. k = T(f) (signed 64-bit; from 0 to 65536, the upper value only if f has reached 1.0).
2. For each axis x, y, z: d = live − drawn (64-bit, wrapping); e = (d × k, the low 64 bits of the signed product, runtime `__allmul`) divided by 65536 with signed truncation toward zero (runtime `__alldiv`); drawn = drawn + e (wrapping). Helpers `0x02C7A0`, `0x02C6E0`/`0x02C900`, `0x02C980`.
3. **The vertical axis then gets a second step:** d = live z − drawn z (the value just updated); d is converted to float (integer load to the x87 unit, store to binary32 under the ambient x87 rounding, reload and store again); multiplied in binary32 by f; converted to signed 64-bit by truncation toward zero (`0x068CF0`); added to drawn z (`0x02C9B0`).

So per update the horizontal axes close a fraction of about f of the gap and the vertical axis about 1 − (1 − f)², with the stated roundings.

### 2.3 Rotation smoothing (flag > 0)

For component 0, then 1, then 2, the drawn component becomes the angle step of (drawn component, live component, f). The angle step `0x023F70` (all operations in this order; "to binary32" is a rounding under the default SSE state):

1. Each angle in degrees is divided in binary32 by 180.0, widened to binary64, multiplied by π (binary64 `0x400921FB54442D18`) and rounded to binary32: ra for the drawn value, rb for the target.
2. cos ra, sin ra, cos rb and sin rb are each computed by the runtime's precise binary64 routines on the widened binary32 value and rounded to binary32.
3. q = (sin rb × cos ra) − (cos rb × sin ra), evaluated in binary64 on the rounded values, then rounded to binary32. This is sin(rb − ra).
4. If q > 1.0 it becomes 1.0; otherwise if −1.0 > q it becomes −1.0 (a NaN passes unchanged).
5. a = arcsine of q (precise binary64 routine) rounded to binary32.
6. m = (f widened to binary64) × 180.0, then ÷ π, in binary64.
7. The result is ((a widened) × m, rounded to binary32) + the drawn value, in binary32.

Consequences: the step is f times the signed angle whose sine equals sin(target − drawn), in degrees. A difference within ±90° is approached directly. A larger difference folds: for example 120° moves as if it were 60°, and exactly 180° does not move at all. No angle is wrapped into a range; drawn and live values can differ by whole turns, which this step ignores.

### 2.4 Drawn aim (every creature reaching 0x21FD54, both branches and the attachment path)

g is computed with the step as repeat count. Then for each component: the drawn aim becomes ((received aim `+0x160` − drawn aim) × g) + drawn aim, each operation in binary32 (helpers `0x0121C0`, `0x0252F0`, `0x012850`). Afterwards the drawn aim is replaced by an exact copy of the received aim when the action byte `+0x68` is 0, or the clock `+0x6C` is 0, or the clock is (signed) greater than the value returned by `0x03D1A0` for S (not audited; `0x21FDD0`–`0x21FDF7`). The reconstruction describes this as the aim being taken directly between attacks (reconstruction lead).

### 2.5 Reaching the block

The creature loop body runs from `0x2104D0` to `0x21FE21`. A search of the routine's direct jumps finds only two that skip from before `0x21F968` to the next creature: an empty map entry (`0x2104E6`) and health not greater than 0.0 (`0x211ADB`, after the maximum-health clamp). So a dead creature (health ≤ 0.0) runs none of its update, including this block: its drawn pose and drawn aim stay frozen. Every other creature reaches `0x21F968` (on the client directly from the gate at `0x21F6DA`; on the server after the server-only block of 3.1). Jump tables inside the body were not audited for further exits.

### 2.6 Later writers of the drawn pose in the same update (not audited)

- The attachment copy and its second pass (held contract, 1.6): the drawn position and rotation come from the partner.
- The pass over world objects after the creature loop (`0x21FE61` onward; the client branch from `0x22063F`, for example `0x220838`–`0x2208A0`) rewrites the drawn position and drawn rotation component 2 of creatures carried by an object.

The client draw routine `0x0AC260` reads the drawn position and drawn rotation (for example `0x0B3AAB`, `0x0B0020`). The full list of consumers was not audited.

## 3. What the world update runs for remote creatures

### 3.1 Gates in the creature loop (client world)

A remote creature is any creature other than the local creature on a client world. The loop body contains the following gates on world `+0xB4` and `+0xB8`. Where a span is named, its contents were bounded but **not audited**.

| Site | Runs when | Span skipped otherwise |
|---|---|---|
| `0x211A86` | server | call `0x06F870` on the effect list `+0x130C` after the effect loop |
| `0x211AE1`–`0x211AF8` | server, byte `+0x60` nonzero | behaviour timer: step added to `+0x13C4`; with `+0x12C` ≤ 0 and the timer ≥ 32, the first virtual function of the object at `+0x13E4` (when present) is called with (S, world, timer, the world update's second argument) and the timer reset; with `+0x12C` > 0 (tested before and after that call) a flag call on the record |
| `0x211B84` | server, race `+0x64` = `0x90` | health drain |
| `0x211BC7` | server | `0x211BD4`–`0x211D06` |
| `0x211F41` | server, or S is the local creature | the 4000 ms combo timeout block `0x211F56`–`0x212047` |
| `0x218329` | server | the effect kind-4 periodic hit through `0x196D30` |
| `0x218903` | S is the local creature, or server with byte `+0x60` nonzero | handling of actions `0x50`/`0x51`, `0x21891E`–`0x218BE2` |
| `0x218F08` | server, byte `+0x60` = 1 | health replaced by the value of `0x044DB0` under further conditions |
| `0x2192D9`, `0x2192FA`, `0x2193D2`, `0x21944F` | S is the local creature | local-player action handling |
| `0x219EE5` | S is the local creature or server, and a locally computed condition holds | `0x219EFA`–`0x21A1E9` |
| `0x21AC47` | S is the local creature | `0x21AC53`–`0x21AE3E` |
| `0x21B456` | server or local creature, and byte `+0x60` ≠ 6 | the unstuck routine `0x20AB30` |
| `0x21C279` | S is the local creature | `0x21C285`–`0x21C41C` (inside the vertical collision handling) |
| `0x21E8A8` | S is the local creature | `0x21E8B4`–`0x21E95B` |
| `0x21F6DA` | server | `0x21F6E7`–`0x21F967` |

The checks at `0x21C16D`, `0x21ED8F` and `0x21F63D` test only whether the world has a local creature and were not audited. The behaviour-object call at `0x211B45` is the only call site of a behaviour object found in the loop; it runs only on the server. Which classes the object can have was not traced.

### 3.2 Simulation between received updates

The movement span `0x218F44`–`0x21F081` has no gate that excludes remote creatures: the gate at `0x218F08` only adds a server-only step before it, and both of its outcomes continue at `0x218F44`. **A remote creature is therefore advanced locally**: its live position moves each update by its live velocity (`+0x40`, as last received) through the movement sub-steps with terrain, static-entity and world-object collision, step-up and step-down snaps (held contract, 1.2–1.3). The decays of `+0x1188`, `+0x1180` and `+0x1184` (held contract, 1.5) also run. Other ungated code also runs, for example the reduction of `+0x130` by the step at `0x211D07`, and the approach of live rotation component 1 toward a locally computed value at `0x2191F6`–`0x21920A` (repeat count = step, rate 0.001 or 0.005). This is extrapolation by local simulation: each received record then overwrites the live record, including position and velocity. The drawn pose follows the live pose by Section 2. The unstuck routine, AI behaviour, combo timeout and the local-player-only blocks above do not run for remote creatures.

### 3.3 Server

The server's block (`0x145940` onward) matches the client's `0x21FB80`–`0x21FE21` instruction for instruction after masking absolute addresses (163 instructions each). Its helpers were compared: the vector and approach helpers are byte-identical over their first 48 bytes; the angle step, T, the fixed-to-float scale and the index and copy helpers differ only in absolute addresses or padding; the constants 180.0, π and 65536.0 have the same values. The duration query `0x03D1A0` (server `0x0084B0`) and the cleanup routine `0x0DA480` (server `0x023710`) were not compared. Because the server flag is always 0, the server's drawn position and rotation are exact copies of the live pose after every update, and `+0x1180` is not zeroed by this block. No server receive path writes the flag. How the server applies data sent by clients was not traced.

## 4. Random draws

None. The interpolation block, its helpers, the type 0, 1 and 2 receive cases and the field readers make no call to `rand`. A transitive search of direct calls from the creature constructor and the record initialiser and copies finds none. Whether the rest of the world update draws for remote creatures was not audited.

## 5. Client/server differences

- Client only: the receive thread and its three creature messages, the flag stores of 60, the snap/smooth decision, the saved copies and the drawn-pose smoothing.
- Server: the same block, always on the copy branch; AI behaviour, the unstuck routine and the other server-gated spans of 3.1 run for all eligible creatures.

## 6. Unresolved

- The meaning of the snap field `+0x1C0` (reconstruction lead "origin") and why a change forces a snap; the meaning of the container `+0x11DC`.
- Whether any indirect access (a base register pointing elsewhere in the creature) modifies the flag or reads the saved copies. None was found by displacement search.
- The initial contents of the drawn aim and saved copies of a freshly constructed creature.
- The value returned by `0x03D1A0` in the drawn-aim copy condition.
- The contents of the gated spans of 3.1, the jump tables inside the loop body, and whether code outside them treats remote creatures differently.
- The full field list and mask indices beyond the first five fields of the receive readers; the decompression routine `0x0494B0`; message types 3, 4, 5, 10 and 15.
- How the world lock taken by the receive thread orders against the world update on the main thread.
- The full set of drawn-pose consumers and the later writers of 2.6.
- How the server receives and applies client-sent creature data.
