# Alpha client and dedicated server world-call phase boundary

**FINAL / STABLE / HELD**

## Independent re-certification, 2026-11-17

This contract covers the original client and dedicated server elapsed producers,
the observed world-call sites, queue ownership on both sides of those calls, and
the ordered phases inside the world routine. The re-certification adds the
dedicated server, traces the client's preceding time sample, and identifies
the two zone collections exported at the end of the world call. It follows
control flow through loop joins and out-of-line bodies.

| Reference | Supported original image | SHA-256 |
|---|---|---|
| C | `binary/Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| S | `binary/Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

Both files have preferred image base `0x00400000`. All addresses below are
image-relative unless stated otherwise. Field offsets are relative to the
entity named in the text. The source evidence is the original image bytes,
PE imports, native data and direct instruction decoding. The scoped client
and server instruction exports were matched to their images, including
the alternate branches and calls cited here. C and S identify the image for
each site and establish correspondence only at the named sites.

## Elapsed production and caller order

### Client

The client driver calls `timeBeginPeriod` with one millisecond. The display
update at C `0x0C85F0` runs when both the device and controller pointers are
present. It samples `timeGetTime`, reads the previous sample from C
`0x36B234`, and substitutes the current sample for a zero previous sample.
It passes the low 32 bits of current minus previous through the controller's
virtual slot `+0x28`. This slot resolves to the frame update at C `0x088EE0`.
The frame retains that argument as its signed elapsed value. A zero previous
sample therefore produces zero elapsed. After update and any intervening
display-state work, the driver saves the current sample and calls the draw
slot at `+0x2C`.

The outer client loop separately measures its own iteration duration. It takes
one sample before the message/input work and another after the display update.
If the signed low-32-bit duration is less than controller `+0x19C`, it sleeps
for the difference. These samples belong to pacing. The display update supplies
the world frame's elapsed value.

Evidence at C `0x059CC3`, vtable cells `0x2FFDF0` and `0x2FFDF4`,
`0x0C85F0–0x0C86EC`, `0x088F31–0x088F34`, and
`0x0C903D–0x0C905F`, `0x0C9432–0x0C9463`. The timing and sleep imports
are at C `0x2FC5C8–0x2FC5D0` and `0x2FC084`.

The frame enters the world critical section before the scoped simulation
work. The world is embedded at controller `+0x2E4` and its critical section
is at world `+0x8000C0`. The frame constructs a single mutable aggregate,
with thirteen head/count pairs, every eight bytes from `+0x00` through
`+0x60`. It also stores the unpartitioned elapsed value at controller
`+0x8006E8`. If controller `+0x8006C8` holds a backend, the frame performs
the following three drains in order.

| Source backend list | Destination in the mutable aggregate | Client helper |
|---|---|---|
| `+0x34` | `+0x00` | C `0x0DD6A0` |
| `+0x3C` | `+0x58` | C `0x0DD5E0` |
| `+0x44` | `+0x20` | C `0x0DD640` |

Each drain enters and leaves the critical section in the world reached through
backend `+0x04`. It appends record copies to the destination in source
traversal order, resets the source head links and count, and frees the old
nodes. The frame's outer world hold surrounds the sequence.

After these drains, admission first compares the world seed word
`+0x800164` with controller `+0x800A50`. If they agree it compares the
world name at world `+0x94` with the byte string at controller `+0x800A54`.
The string comparison uses length and contents. Failure of either comparison
branches to later frame work at C `0x08D5F0` with the drains already performed.

For an admitted frame, elapsed values greater than 20 produce the integer
quotient of elapsed minus one divided by 20 calls of 20, followed by one
call containing the remaining 1 through 20. An elapsed value of 21 gives
20 then 1, and 40 gives 20 then 20. Values at most 20, including zero and
negative values, give one call with the original signed value. Both sites
use the same mutable aggregate and a null optional input pointer. Each
piece runs its own world invocation with the records left by earlier pieces.
The normal frame tail leaves the world critical section and subsequently
destroys the aggregate.

Evidence at C `0x089020–0x08902F`, `0x201CB0`, `0x066880`,
`0x08C138–0x08C13F`, `0x080DF0`, `0x0787F0`, `0x051440`,
`0x00C520`, `0x08CE2E–0x08CF49`, `0x0DD5E0–0x0DD6FC`, and
`0x09D2E4–0x09D365`. The repeated and final calls are at C `0x08CF2E`
and `0x08CF49`.

### Dedicated server

The dedicated server starts its timing period at one millisecond and takes a
baseline `timeGetTime` sample. On each admitted run-loop iteration it takes a
new sample at the head. It constructs a fresh optional input aggregate, drains
backend `+0x34` into input `+0x00`, then drains backend `+0x3C` into input
`+0x58`. Each copy-and-clear has its own acquisition and release of the world
critical section reached through backend `+0x04`. The acquisitions are
separate from the outer world hold that follows them. Each source list can
therefore have a different receipt cutoff.

The loop enters the world critical section at world `+0x8000C0`, constructs
a fresh result aggregate, and calls the world once with the signed low
32 bits of current sample minus previous sample, the result aggregate, and
the nonnull input aggregate. There is no value gate or subdivision between
the subtraction and call. Zero, a negative signed result, and an elapsed
value greater than 20 reach the same call site.

On return it commits the current sample as the next baseline. Still under
the outer world hold, it drains backend `+0x44` into result `+0x20` and
hands the result to the backend at S `0x0272D0`. The drain acquires the
same critical section recursively. With a separate critical section held,
the wrapper refreshes a list of sector coordinates from nonnull world
`+0x04` entries whose actor byte `+0x60` equals zero. Each signed 64-bit
horizontal position is divided by 65,536 with truncation toward zero.
The low 32-bit signed quotient is then divided by 256 with truncation
toward zero. The coordinate pairs are appended in traversal order.

After releasing both holds, the normal loop sleeps for 20 milliseconds,
destroys the result and input aggregates, and rechecks the run byte. Thus
the time between head samples includes any prior iteration work as well as
the sleep. The exit path calls `timeEndPeriod` with one millisecond.

Evidence at S `0x149FE4–0x14A0D0`, `0x14A0DC–0x14A2A0`,
`0x023510`, `0x028170–0x02828D`, `0x0D3DF0`, `0x0D5C60`,
and the imports at S `0x1583B8–0x1583C0` and `0x158068`.
The world call is at S `0x14A0AE`.

## Receipt and outgoing ownership

- The client socket receive worker stages decoded records in controller-owned
  lists while holding the world critical section. For the scoped decoded
  group, the receiver splices the terrain list into controller `+0x800648`
  and the placement snapshot list into `+0x800668`. The enclosing frame
  reaches its pending-receipt work after the world-call loop or from the
  skipped-world branch. The terrain list is visited at C `0x08F242`,
  followed by the `+0x800660` queue and then the placement snapshots.
  A placement snapshot for a loaded zone replaces its placement list
  `+0x3C` and clears zone byte `+0x74`. Missing zones skip that application.
  The frame later clears both controller lists.
  Evidence at C `0x06CEE8–0x06CFE0`, `0x08D5F0`,
  `0x08F242–0x08F719`, and `0x0905E3–0x090615`.

- With controller `+0x8006CC` nonzero, the late client frame visits aggregate
  `+0x00`, `+0x58`, and `+0x20` in that order. It copies a record into
  the matching outgoing queue only if its leading eight bytes match the
  local actor's eight-byte identity at actor `+0x08`. The outgoing queues
  are controller `+0x8006EC`, `+0x8006F4`, and `+0x8006FC`,
  respectively. A separate backend path at C `0x09C256–0x09C26D` hands
  the current aggregate to the backend when `+0x8006C8` is nonnull.
  Evidence at C `0x09C02F–0x09C26D` and `0x0DC8E0`.

- In the connected client sender, the first four-byte socket send of the
  scoped update must return a value other than `-1` before the snapshot
  and removal phase. Under the world lock, the sender copies the three
  outgoing lists, copies the oldest local actor request if present,
  unlinks that request and decrements actor `+0x1310`, and clears the
  outgoing event lists. The later payload sends occur after release.
  The request and event removal order is therefore tied to the early
  socket send and snapshot, before the later payload sends. On the dedicated
  server's matching request receive route, the fixed-size request record is
  added to the resolved actor's own `+0x130C` list under the world hold.
  This queue and the two backend event input lists have distinct owners.
  Evidence at C `0x069C88–0x069F26` and `0x069FB9` onward, and S
  `0x02627B–0x0262EC`.

- The dedicated server's peer receive worker appends hits, buffs and the
  third event type to backend lists `+0x34`, `+0x3C`, and `+0x44`,
  respectively. The completed-record paths read 72, 40, and 112 bytes,
  respectively, and take the world hold for the append. The receive
  worker captures the list addresses from the backend when launched.
  Evidence at S `0x025D9D–0x025E88`, `0x026B70–0x026B77`, and
  `0x026461–0x0265C4`.

- The dedicated server's post-call result handoff stages copies in per-peer
  queues. Observed mappings include result `+0x00` to peer `+0x18`,
  result `+0x18` to `+0x30`, result `+0x20` to `+0x38`, result
  `+0x28` to `+0x40`, result `+0x30` to `+0x48`, result `+0x38`
  to `+0x50`, and result `+0x58` to `+0x70`. Some other record paths
  have additional per-peer eligibility checks. The observed handoff boundary
  is insertion into the peer queues.
  Evidence at S `0x0272D0–0x02781E`.

## Ordered phases inside each world invocation

The world routine is C `0x20C510` and S `0x1322D0`. It takes a signed
32-bit elapsed value, a caller-owned mutable aggregate and a nullable optional
input aggregate, in that order. It retains the same mutable aggregate through
the invocation. The optional pointer is read after the initial passes.
The normal return consumes all three arguments at C `0x221462` and S
`0x147222`. The mode gate in the following phases is world byte `+0xB4`.
The dedicated server initializes it to zero. The client connection entry
can set its corresponding byte at controller `+0x398` to one, and the
socket failure/exit path can reset it to zero. Evidence at C
`0x20C541–0x20C564`, `0x20CD1F`, `0x06FCA7`, `0x06A869`, and
S `0x132301–0x132324`, `0x132ADF`, `0x149CB7–0x149CBD`,
`0x0C8679–0x0C867C`.

### Preliminary health and clocks

1. Scan the live creature collection at world `+0x04`. A nonnull actor
   with byte `+0x60` equal to zero, action byte `+0x68` equal to 84 and
   binary32 health at `+0x16C` ordered greater than zero sets the clock-rate
   flag. Within this scan, the qualifying actor identified by world `+0xB8`
   receives the health addition. First narrow the maximum-health helper's
   x87 return to binary32. Convert elapsed to binary32, then multiply by
   binary32 `0x3A83126F` (approximately 0.001) and then `0x3D4CCCCD`
   (approximately 0.05). Multiply the resulting factor by the rounded maximum,
   add current health and store to binary32. Query and narrow the maximum
   again for comparison. Ordered excess invokes a third maximum-health read,
   stored directly from x87 to actor health.
   Evidence at C `0x20C555–0x20C66C`, helper `0x044DB0`, and constants
   `0x2FD870`, `0x2FCDB4`. Corresponding S evidence is
   `0x132315–0x13242C`, helper `0x00FDA0`, and constants
   `0x158858`, `0x1586B8`.

2. Add elapsed to the world counter `+0x8000BC`. Add elapsed times 100
   with the flag set, or times 10 otherwise, to world clock `+0x80015C`.
   Arithmetic retains the low 32 bits. A zero world name length at
   `+0xA4` forces the clock to 32,400,000. While the signed clock is
   strictly greater than 86,400,000, increment day at `+0x800160`,
   subtract 86,400,000 from the clock and call the day helper with the
   same aggregate. Equality and negative clock values leave this wrap
   loop immediately.
   Evidence at C `0x20C672–0x20C6E2`, day helper `0x1F0C80`, and S
   `0x132432–0x1324A2`, day helper `0x124500`.

3. With the counter already incremented, compare its signed quotient by
   10,000 with the quotient obtained after adding elapsed to it once
   more with low-32-bit arithmetic. Both divisions truncate toward zero.
   A bucket change, an available storage handle, and a nonempty world
   name admit a write under the key `time`. Its eight-byte payload is
   the little-endian day word followed by the little-endian clock word.
   Evidence at C `0x20C6E4–0x20C838`, storage helpers `0x0497A0` and
   `0x0499C0`, key `0x315B40`. Corresponding S evidence is
   `0x1324A4–0x1325F8`, storage helpers `0x013000` and `0x013210`,
   key `0x16D08C`.

### Preparation and optional inputs

The world saves elapsed converted to binary32 and also saves the result of
multiplying that value by the binary32 0.001 constant. It creates local
collections and makes a separate preparatory creature traversal. This is
before optional hit or buff input and before the main creature traversal.
Preparation can call out with the shared aggregate and can append at aggregate
`+0x60`. The scoped preparation includes conditional spatial and relationship
work driven by live activity. Evidence at C `0x20C83D–0x20CD1F`, including
`0x20CBB7` and `0x20CCDD–0x20CCE0`, and S `0x1325FD–0x132ADF`.

A null optional aggregate skips both input traversals. With a nonnull input,
the world handles native forward-order hits from `+0x00` first. A nonempty
hit list with mode nonzero clears result `+0x00` before visiting the hits.
It supplies each hit to C `0x195A60` or S `0x0CEA80` along with the
mutable aggregate and an invocation-local collection.

The next list is at optional input `+0x58`. Its forty-byte records hold an
eight-byte source identity, an eight-byte target identity, and a resident
record starting at record `+0x10`. If the target lookup succeeds, the world
passes the resident record to the target at C `0x046AF0` or S `0x011740`.
This outer application depends on target presence. After a nonempty traversal,
mode nonzero clears result `+0x58`. Empty optional lists bypass the
corresponding result clear. Both outer loops advance over the input nodes,
whose lifetime remains the caller's. Evidence at C `0x20CD1F–0x20CEAA`
and S `0x132ADF–0x132C6A`.

### Intervening maintenance and RNG boundaries

Between optional input and the main creature traversal, another pass over a
locally prepared collection can call a generic hit helper. The mode branches
then perform different relationship and local-collection work. In the
mode-nonzero branch, a type-five creature can copy its identity to the linked
creature at `+0x11C8`. Later maintenance requires mode zero and a nonempty
world name. These regions precede the main creature traversal.
Evidence at C `0x20CEAA–0x210499`, including `0x20DD18`,
`0x20DDEB–0x20DEB6` and `0x20F942–0x20F962`, with corresponding
mode and name gates at S `0x135702–0x135722`.

- One earlier direct CRT `rand` draw is reached after the optional inputs,
  when a prepared entry's positive signed field `+0x13C` falls to zero
  or below after subtracting elapsed and the mode is zero. That field is
  set to zero before the draw. The returned value is converted to binary32,
  multiplied by the binary32 0.1 constant, divided by binary32 32,767 and
  added to binary32 1.0. This value goes into a record appended to the
  shared aggregate at `+0x08`. The record's selector is 58 for entry
  byte `+0x00` equal to 12, or 59 otherwise.
  Evidence at C `0x20D1ED–0x20D299` and S `0x132FAD–0x133059`.

- There is a conditional CRT `srand` in the later mode-zero,
  nonempty-name maintenance before the main actor traversal. The selected
  candidate first passes the branch's spatial comparison, the associated
  descriptor must be present with dword `+0x34` equal to five, and the
  candidate's word `+0x7A` must contain bit `0x1000`. The seed is the low
  32-bit sum of descriptor `+0x30` and candidate `+0x48`.
  The seed is set before the next per-candidate helper. The caller's piece
  loop does not reseed at its boundaries. Each actual world call can reach
  the conditional reseed through its own maintenance path.
  Evidence at C `0x20F942–0x20F9A0`, `0x20FC29–0x20FD4A` and S
  `0x135702–0x135760`, `0x1359E9–0x135B0A`.

- Separately, on the observed client startup path, a `timeGetTime` value
  supplied as the world seed is stored at world `+0x800164` and passed
  to `srand` by the setup helper. On the dedicated server the starting
  configuration path takes the remainder of an initial `rand` draw divided by
  1,000,000 before reading or writing the numeric seed in `server.cfg`.
  A successfully read value supersedes the default. The seed provided to
  the world setup is stored in world `+0x800164` and supplied to `srand`
  there. A direct draw follows the seed in each setup helper.
  Evidence at C `0x05A72D–0x05A741`, `0x1A5319–0x1A5339` and S
  `0x149CDB–0x149D5A`, `0x0D83D9–0x0D83F9`.

- In a later client-only frame effects path, after all world pieces and the
  scoped pending terrain and placement receipt, a record with signed count
  `+0x38` positive requests that many emissions. Each emission consumes
  two ordered groups of three direct CRT draws, with a seventh when record
  dword `+0x3C` is one. Each of the six draws is divided by binary32
  32,767 and reduced by binary32 0.5 before being used. The optional
  seventh is divided by the same divisor. This effects pass is part of
  the enclosing frame. Evidence at C `0x0952F0–0x0955AB`.

The native imports for these direct sites are C `0x2FC468`, `0x2FC4B4`
and S `0x1582F4`, `0x15832C`. The direct-site evidence establishes draw
and seed order on the respective paths. The complete draw stream of nested
helpers and interactions between different threads remain open.

### Main creature traversal

The main traversal uses the live world `+0x04` collection in native iterator
order. Null actor pointers advance immediately. Each nonnull actor first
visits its own request records at actor `+0x130C` in list order. Record byte
`+0x128` selects dispatch destinations for values one through eight. After
visiting the requests, the mode-zero path clears the list. The nonzero mode
bypasses this clear. This request work precedes the actor health admission.
Evidence at C `0x210499–0x210546`, `0x211A44–0x211A96`, `0x21FE21`
and S `0x136259–0x136306`, `0x137804–0x137856`, `0x145BE1`.

After request work the routine reads maximum health and narrows it from x87
to binary32. Only an ordered health value greater than the maximum triggers
a second maximum read and a store to current health. The subsequent comparison
against binary32 zero skips the remaining actor body for ordered health at
most zero. Positive or unordered health continues if floating-point execution
completes. Evidence at C `0x211A96–0x211ADB` and S `0x137856–0x13789B`.

For an admitted actor with byte `+0x60` nonzero and mode zero, the
signed accumulator at actor `+0x13C4` receives elapsed. A positive actor
`+0x12C` bypasses the callback threshold. Otherwise, at accumulated
elapsed of at least 32, a nonnull actor `+0x13E4` callback receives the
actor, world, accumulated elapsed and aggregate. That threshold path
resets the accumulator even if the callback pointer is null. Evidence at
C `0x211AE1–0x211B7B` and S `0x1378A1–0x13793B`.

Before the action switch, the routine subtracts elapsed from the signed
fields `+0x130`, `+0x134` and `+0x138`. Each resulting negative value
is clamped to zero. The `+0x130` path also performs a helper call and
actor `+0x6C` update when the reduced value is positive. The action-byte
router at actor `+0x68` has destinations for values one through 110, with
other byte values taking the common continuation. The common continuation
includes a visit to resident records at actor `+0x1178`. It subtracts
elapsed from each record's signed `+0x08` remaining time before the
positive-time check and subsequent removal processing. Other actor-state
and movement work precedes the iterator advance.
Evidence at C `0x211D0D–0x211E1B`, `0x2120E8–0x212163`,
`0x218310–0x218541`, `0x21FDD0–0x21FE5B` and S
`0x137ACD–0x137BDB`, `0x137EA8–0x137F23`, `0x145BE1–0x145C1B`.

### Late object passes and deferred world publication

After the main creature loop, the world visits collection `+0x0C`. The
first loop only advances the iterator. A second traversal performs object
work, then the routine enters the projectile collection at world `+0x14`.
Evidence at C `0x21FE61–0x220A70` and S `0x145C21–0x146830`.

At the projectile motion step the saved binary32 time factor is applied
to the velocity at record `+0x3C` and the elapsed value is added to
signed age at `+0x68`. At the ensuing gate, kind two at `+0x60` or
age at most 5,000 enters the collision body. Earlier kind-specific branches
can bypass this step. The collision body is located after the main return
in the image and rejoins the projectile loop. It can call a generic hit
helper and append events to the shared aggregate. Projectile entries
selected for removal are recorded locally and erased after traversal.
Evidence at C `0x220A70–0x220E4C`, `0x221465–0x222B92`, including
`0x222AB8`, and S `0x146830–0x146C0C`, `0x147225` onward.

A further creature relationship pass follows the projectile removals.
The four final publication steps are ordered as follows.

1. For each zone pointer in an invocation-local collection, set zone
   byte `+0x75` to one. Append an aggregate `+0x28` snapshot made
   from zone coordinates `+0x60` and `+0x64` and copies of the
   zone list at `+0x30`, in source order.
   Evidence at C `0x22113C–0x2211EE`, `0x0DC820`, and S
   `0x146EFC–0x146FAE`, `0x022C00`.

2. Visit the entire current aggregate `+0x30` list of coordinate pairs
   in list order. For each successful loaded-zone lookup set zone
   byte `+0x76` to one. Evidence at C `0x2211F0–0x221298`,
   lookup `0x034D10`, and S `0x146FB0–0x147058`, lookup `0x006290`.

3. For each zone pointer in the separate invocation-local placement
   collection, append an aggregate `+0x38` snapshot. This contains
   zone coordinates `+0x60` and `+0x64`, plus the full zone placement
   list at `+0x3C` in source order. Evidence at C
   `0x22129A–0x221349`, `0x208F60`, and S
   `0x14705A–0x147109`, `0x12E950`.

4. Visit the entire current aggregate `+0x18` list. Divide each
   signed horizontal coordinate by 256, truncating toward zero.
   On a successful loaded-zone lookup, append a copy of the 20-byte
   terrain edit to the zone's list at `+0x68`. The invocation-local
   collections are cleaned up afterward. Evidence at C
   `0x22134B–0x221462`, append helper `0x04D4B0`, and S
   `0x14710B–0x147222`, append helper `0x01F4D0`.

The suffix relationship pass starts at C `0x220E51` and S `0x146C11`.
Publication steps 2 and 4 start from the beginnings of the aggregate lists.
In the client, earlier pieces can leave records that these steps visit
again in a later piece. The invocation-local collections start afresh on
each call, while the mutable client aggregate lasts across the whole
subdivided frame. The dedicated server starts with fresh aggregates per
iteration and hands off the results only after the single world return.

## Boundaries left open

Zero elapsed still reaches the preliminary health queries, the unnamed
world clock assignment, and later applicable queue work. Negative elapsed
reaches the documented signed arithmetic and binary32 conversions. The
resulting behavior inside every nested helper, including for exceptional
clock discontinuities, remains open.

The client backend field is zeroed at C `0x059FC5`. The observed explicit
field references give the guarded paths above. Installation of a nonnull
backend and its lock identity relative to the frame's embedded world remain
open. Abnormal socket outcomes and partially received records remain open.

The full request/action bodies, callback
bodies, movement and collision, local maintenance membership, additional
world and frame draws, exceptional exits, packet timing and delivery,
peer eligibility, and the wider save/restore effects remain unresolved
here. Ambient SSE and x87 control states beyond the stated conversion and
store boundaries also remain open.
