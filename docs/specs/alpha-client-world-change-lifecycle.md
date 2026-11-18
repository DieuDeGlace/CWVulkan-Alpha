# Alpha client world change lifecycle

**FINAL / STABLE / HELD**

Source is the original x86 `Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses below are RVAs. The scope is a complete type-15 message and the
preparation it requests during normal connected play. Client offsets use the
outer client object. Its simulation world starts at client `+0x2e4`.

## Receive and request

The receive dispatch sends type 15 to `0x06b8be`. It reads one little-endian
signed 32-bit seed and formats the name as `online_` followed by that
integer in decimal.
It compares this pair with the **requested** seed and name at client
`+0x800a50` and `+0x800a54`. Full name equality uses byte contents and length.
An identical request skips the request call. A differing seed or name calls
`0x06f620` (compare at `0x06b9da`, call at `0x06ba0a`). This receive path
continues on the existing connection.

The request procedure enters the world critical section at world
`+0x8000c0`, then the requested-state critical section at client
`+0x8005d0`. It writes the seed and name in the local creature's profile
record first, then writes the client requested pair. The profile pointer
is at creature `+0x1d28`, with its seed at profile `+0x24` and its name at
profile `+0x28`. The request removes every entry in the world actor tree at
world `+0x04` whose actor pointer differs from the local creature pointer
at client `+0x8006d0`. It destroys each nonnull removed actor and preserves
the local creature object and its tree entry. It leaves the two critical
sections in reverse order. The writes and removal are at
`0x06f684` through `0x06f809`.

## Preparation and activation

The preparation worker at `0x06a8a0` loops while client byte
`+0x8005b0` is set. Under the requested-state lock it compares the requested
and active identities and copies its focus inputs. The active seed and
name are at client `+0x800448` and `+0x378`, aliasing world `+0x800164`
and `+0x94`. It carries the comparison result and focus snapshot out of
this lock (`0x06a8f1` through `0x06a9c3`).

On a mismatch the worker first resets the entries in its current wrapped
tile cache window. This uses the cache array at client `+0x2e0`, its side
length at `+0x2dc`, and the cache lock at `+0x800600`. The reset releases
the entries' owned objects and invalidates their coordinates
(`0x06a9c9` through `0x06aa7e`, reset routine `0x086ba0`). It then runs
the terrain sweep in eviction mode, removing loaded zones from their
regions and releasing region entries without the same-world nearby
retention tests (`0x06ad52` through `0x06af6e`; release routines
`0x1a4890`, `0x1a4800` and `0x1a4780`).

After the sweep, the worker enters the world critical sections at
`+0x8000c0` and `+0x8000d8`, in that order. It reads the requested seed and
name again for the activation call at `0x06afab`. A differing type-15
request that acquires the first world lock before this point replaces
the single requested pair. The ongoing pass activates the pair read at
the call. A differing request that waits behind the activation lock can
replace the pair after the worker releases that lock for a later pass.
An identical type-15 request during either phase follows the duplicate
rule. If a mismatch pass has already started and a later request returns
the requested pair to the current active identity before the call, the
pass still goes through activation with that pair. The worker has already
selected the mismatch branch at `0x06af74`.

The activation call at `0x1a52e0` performs these steps in order.

1. It stores the new active seed at `0x1a5324` and passes that seed's 32-bit
   value to the Microsoft CRT `srand` at `0x1a532a`. It takes 77 `rand`
   draws and stores each result's remainder on division by 100,000 in
   the world seed fields. The first draw feeds world `+0x800188`, the
   source seed used by later region and zone generation. The ordered
   destinations are given below.
2. It clears both 1,024 by 1,024 terrain slot tables at world `+0xbc` and
   `+0x4000bc`, releasing surviving region objects and auxiliary allocations.
   It closes the previous terrain archive at world `+0xac`. It resets the
   three binary32 terrain anchor values at world `+0x8000f0` to
   8,396,928, 8,396,928 and zero (`0x1a5543` through `0x1a560c`).
3. It copies the new active name at `0x1a561f`. The remote world byte
   at world `+0xb4` causes this path to skip saving evicted terrain and
   opening the local terrain archive. The connection path sets that byte at
   `0x06fca7`. The relevant terrain persistence guards are `0x1a4956`,
   `0x1a4b2b`, `0x1a5565` and `0x1a5624`.

The worker then resets the separate discovery map at client `+0x800d44`
through `0x1fbc90` (call at `0x06afb8`). This releases the map's
two resident tables. Its close routine writes the map's `discovered`
dword to the old map archive if open, then resets it and closes the
archive (`0x201f80`). For a nonempty new active name the map requests
`Save/map_<active name>.db` and reads the `discovered` dword when available
(`0x1fbdd1`, `0x1fbe70`). This map archive path also applies in connected
play.

For a nonempty name the worker selects the matching seed/name entry in
the client world catalog, appending an entry when none matches
(`0x06afbd` through `0x06b388`). It places the same local creature at its
saved fixed position for the requested pair if present. Otherwise it
converts the terrain anchor's three binary32 values to the creature's
signed 64-bit fixed coordinates with 65,536 units per block, truncating
toward zero. It copies the selected position into the creature's position
mirror at `+0x1350` and zeroes the three dwords at `+0x34` through
`+0x3c` (`0x06b3c2` through `0x06b686`). It releases the two world
locks in reverse order (`0x06b691`, `0x06b69e`).

The worker sleeps for a requested 20 milliseconds at the end of each
iteration. With matching identities in a later pass, it can choose a missing
zone near a focus and invoke the generation path at `0x1e4850` (call at
`0x06acd1`). Seed-derived initialization begins with the seed write while the
previous name is still active. Both new identity writes precede the later
zone generation call. The generation routines make further CRT
reseeds based on the derived seed and coordinates (`0x1da36a`, `0x1e49b5`).

### Seed draw order

Each group writes consecutive 32-bit fields, in ascending address order within
the group. Groups run in the order shown (`0x1a5339` through `0x1a5530`).

| First destination in world | Draws |
| --- | ---: |
| `+0x800188` | 1 |
| `+0x800168` | 4 |
| `+0x80018c` | 20 |
| `+0x8001dc` | 2 |
| `+0x8001e4` | 2 |
| `+0x8001ec` | 2 |
| `+0x8001f4` | 2 |
| `+0x8001fc` | 8 |
| `+0x80021c` | 6 |
| `+0x800234` | 6 |
| `+0x80024c` | 10 |
| `+0x800274` | 4 |
| `+0x800284` | 2 |
| `+0x800178` | 4 |
| `+0x80028c` | 2 |
| `+0x800294` | 2 |

## Projectile, queue and clock lifetime

- The active projectile list at world `+0x14` survives both the request and
  activation. The normal receive-to-frame path appends projectile records
  from the queue at client `+0x800650`, with local sender IDs filtered out
  (`0x090352` through `0x0903c4`). The world tick also appends to the
  active list (`0x21546d`, `0x2161c1`).
- Type 4 adds decoded records to thirteen client lists from `+0x800630`
  through `+0x800690` at eight-byte spacing (`0x06cee8` through
  `0x06d088`). Request and activation leave those lists in place. The
  frame's mismatch branch at `0x08cea8` through `0x08cec9` goes to
  `0x08d5f0` and can still process records, with ordinary recipient and
  loaded-terrain checks. The frame drains these lists on its ordinary
  processing path (`0x0905bd` through `0x09064c`). Clearing terrain slots
  can therefore affect where pending terrain events apply. Incoming actor
  updates continue to use the same actor tree after the request
  (`0x06bbdc` through `0x06bc6f`).
- Type 5 writes the day dword at world `+0x800160`, followed by the
  time-of-day dword at `+0x80015c` (`0x06cbcc`, `0x06cbd2`).
  The active setup leaves both connected clock values in place. The
  separate local-only terrain archive branch can read both from
  its `time` entry (`0x1a57c3`, `0x1a57db`).
- The receive worker also keeps its previous type-2 receipt timestamp.
  When its previous sample is nonzero, it stores the 32-bit millisecond
  difference from that sample at RVA `0x36b048`. Every type-2 receipt then
  replaces the previous sample (`0x06b865` through `0x06b8a1`). Type 15
  leaves this sampling state intact. Its dispatch leaves the receive worker
  and the type-2 actor tracking set on their ordinary lifecycle
  (`0x06be25` through `0x06bfa6`).

## Evidence limits

The trace establishes ordering in these routines and their traced connected
branches. Cross-thread visibility between the separate seed and name stores,
CRT state in other threads, failed or truncated receives, disconnect races,
runtime packet timing and live native acceptance remain open.
