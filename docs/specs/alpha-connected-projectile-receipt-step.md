# Alpha connected projectile receipt and resident step

Status: FINAL / STABLE / HELD

## Scope and image identity

This contract follows a well-formed connected-client projectile from
the outgoing queue through the dedicated relay, packet 4 receipt and
its entry into the receiving client's live world. The step evidence
covers types 0 through 4, finite representable motion, and the
connected client with a resolved nonlocal source. Missing-source
gates are included where traced. Client and dedicated addresses are
RVAs in the following original 32-bit images.

| Image | Size in bytes | SHA-256 |
| --- | ---: | --- |
| Cube.exe | 3,885,568 | 84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf |
| Server.exe | 1,718,784 | 1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1 |

## 1. Record and socket boundaries

The record is 112 bytes on the wire. Offsets below are relative to
the record, exclusive of list links. Numeric fields are little-endian.

| Offset | Width | Verified use |
| --- | ---: | --- |
| `+0x00` | 8 | Signed source creature key. Local echo filtering compares both halves for equality. |
| `+0x08`, `+0x10` | 8, 4 | Carried fields. |
| `+0x18` | 24 | Live fixed position, three signed 64-bit components at 65,536 units per block. |
| `+0x30` | 12 | Three carried 32-bit components. |
| `+0x3C` | 12 | Three binary32 velocity components. |
| `+0x48` | 4 | Binary32 base hit amount. |
| `+0x4C` | 4 | Binary32 direct contact half-width, reused across all three axes. |
| `+0x50` | 4 | Carried field. |
| `+0x54` | 4 | Binary32 strength used for the hit impulse, passed to the hit helper and used by type conversion. |
| `+0x58` | 4 | Carried field. |
| `+0x5C` | 1 | Charge byte, tested against zero. |
| `+0x60` | 4 | Projectile type. |
| `+0x64` | 1 | Variant. |
| `+0x68` | 4 | Signed age in milliseconds. |
| `+0x6C` | 4 | Signed value compared with the target's `+0x134`. |

The client reads full transmitted records into temporary storage.
Its field copies omit `+0x14–0x17`, `+0x5D–0x5F` and
`+0x65–0x67`. Those holes are included in the 112-byte wire
transfer without a verified behavioral use. The field copy is at
client `0x0595B0–0x05965B`.

### 1.1 Client packet 9 and dedicated collection

The late connected-client frame copies world output projectile
records to game `+0x8006FC` in collector order, admitting only
source IDs equal to the full current local creature ID. This is
the outgoing list and is distinct from the resident world list.
The sender at `0x069C10` snapshots that list while holding the world
lock, clears it, and releases the lock before writing the snapshot.
It walks the snapshot head to tail. Each record gets a 32-bit
message ID 9 followed by a single 112-byte payload. There is no
count or compressed length on this message
(`0x09C1E2–0x09C216`, `0x069CDB–0x069D2B`,
`0x069EB5–0x069F26`, `0x06A1B3–0x06A213`).

Dedicated ingress selects message 9 by directly indexing the
pointer table at `0x02671C` with the 32-bit message ID after an
unsigned maximum-12 check. Entry `0x026740` leads to `0x026560`.
The reader receives one 112-byte record and appends it under the
world lock. Its context `+0x20` points to host `+0x44`, from the
captured pointer at `0x025DBA–0x025E0E`, its copy at
`0x025E39–0x025E88` and the context adjustment at `0x026B70–0x026B73`.
The append `0x0265AC–0x0265C4` uses the record's carried source ID
without substituting the socket's creature ID.

The dedicated world can also put a newly produced projectile in
its resident list `world+0x14` and in its per-turn output list
`collector+0x20`. An example is `0x13D2E6–0x13D2FB`. Once the
dedicated world step returns, `0x14A0C2` calls `0x0281D0` to append
the host `+0x44` pending relay records to that collector's projectile
tail and clear the host pending list. The drain appends only to
the output list. Relayed records therefore follow the projectile
records already in that turn's collector. The subsequent fanout
at `0x14A0D0` calls `0x0272D0`. For each peer reaching the
projectile fanout site, `0x02768E–0x0276AE` appends the whole
ordered range to peer `+0x38` with no per-projectile filter.

### 1.2 Dedicated packet 4 and client decode

The dedicated sender copies the peer aggregate from `+0x18` into
a temporary aggregate while holding the world lock. The projectile
list lies at aggregate `+0x20`. The sender clears the peer list
before releasing the lock and writing the temporary snapshot
(`0x023466–0x023471`, `0x024907–0x024911`,
`0x024A13–0x024A40`, `0x024C79–0x024C7B`).

The dedicated writer sends the 32-bit message ID 4, then the 32-bit
compressed payload length and the zlib-compressed payload. The
projectile list is section 4, the fifth section starting at zero.
Its writer appends a signed 32-bit count followed by 112 bytes per
record in list order. Writer sites are `0x024D05–0x024D22`,
`0x024D6A–0x024DB6`, `0x021AA0–0x021B04` and
`0x024FAC–0x025011`.

The client reads the signed 32-bit message ID and uses it directly
as the byte index at `0x06D1A4` after the unsigned maximum-15
check. Byte index 4 selects pointer-table ordinal 4 at `0x06D190`,
which leads to `0x06CC0E`. The client reads the compressed
length and bytes, inflates and rewinds the temporary stream, then
initializes a fresh batch aggregate. Its four earlier section
readers take aggregate offsets `+0x18`, `+0x00`, `+0x10` and
`+0x08` in that order. The fifth reads the projectile section at
aggregate `+0x20`. Zero or negative signed count admits no entries.
With a well-formed positive count, the reader appends each complete
112-byte record in its original order. It makes no source,
position, type or duplicate check at this boundary
(`0x06B81C–0x06B8B7`, `0x06CC0E–0x06CD36`,
`0x0494B0`, `0x049540`, `0x0581D0–0x0582EF`).

After reading the batch, `0x06CEDD–0x06CEE8` takes the world
lock at game `+0x2E4`. The range append at `0x06CF69–0x06CF83`
copies the decoded list to the persistent receive tail at game
`+0x800650`. Successive packet arrivals accumulate there in append
order. The receive handler releases the lock and destroys its
temporary aggregate at `0x06D080–0x06D093`.

## 2. Frame merge and first resident visit

The frame at `0x088EE0` takes the same world lock at
`0x089020–0x08902F`. Its admitted world calls at `0x08CF2E`
and `0x08CF49` run the repeated 20 ms turns and the final world
turn for the remaining interval. The readiness branches at
`0x08CE9D–0x08CEC9` can bypass those calls.

Later in the frame, the receive-hit walk at `0x08FECB–0x090344`
finishes before the pending projectile walk at `0x09034A–0x090401`.
The projectile walk compares record `+0x00` against the ID at
`+0x08` of the current local creature pointed to by game
`+0x8006D0`. Equality skips the record. Inequality appends to the
resident tail at game `+0x2F8`, which is world `+0x14`.
The carried age, position and velocity survive the copy. This
site admits repeated remote IDs and repeated records, including
records whose source lookup would currently fail. All entries in
the pending list are then cleared at `0x0905EE–0x0905F4`.
The frame releases the lock at `0x09D2E4–0x09D2EA`.

A received projectile first steps on the next admitted world
turn after the merge. In each world turn at `0x20C510` the actor
work finishes before the resident projectile iteration begins.
The final actor walk returns to `0x220A70` from the loop ending
at `0x220A6A`. The projectile iteration visits existing residents
in list order and defers physical removal to the final collected
node walk at `0x220DB9–0x220E42`. An entry queued for removal
skips the rest of its current visit. These projectile outputs use
the current world's frame collector, with hit, sound and visual
effect appends shown below.

The receive-buff locator in §5 of
`alpha-connected-hit-consumption.md` needs this address correction.
The received buff list starts at game `+0x800688`. Its target
lookup and apply walk starts at `0x08FCB8`, with the buff apply
call at `0x08FD2F`. The later `0x09034A–0x090401` walk handles
projectiles as described here.

## 3. Start of each resident visit

At `0x20C83D–0x20C862` the signed step milliseconds are converted
to binary32 and multiplied by binary32 0.001. The type selector
at `0x220ACD–0x220ADC` reads the record's type directly.

- Type 1 enters common displacement with the stored velocity.
- Type 2 looks up its source by the record's full key, via
  `0x02F000`. There is no separate zero-key rejection at this
  lookup. A null result or pre-increment age strictly above 1,200
  removes the projectile immediately. At age at most 800 it
  steers toward the source's live position plus the source aim
  vector at `+0x160`, using the direction scaled to length five.
  For each signed step millisecond the velocity is moved toward
  that desired value by subtracting the current component,
  multiplying the difference by binary32 0.05, and adding it to
  the current component. Above 800 through 1,100 it assigns
  velocity directed toward the source's live position at length
  ten. Above 1,100 through 1,200 it moves the fixed position by
  the source-minus-projectile delta twice scaled by the same
  factor. Subtract 1,100 from signed age, convert to binary32,
  divide by binary32 100, and convert to fixed units. Each scale
  multiplies by this fixed factor and divides by 65,536, truncating
  toward zero before the next. The last branch keeps the velocity
  for common displacement
  (`0x220AE2–0x220C6D`, `0x205950`, `0x02C6E0`, `0x02C900`).
- Types 0, 3 and 4 subtract gravity from stored Z velocity.
  Multiply step seconds by binary32 30, multiply that result by
  binary32 0.25, then subtract the result
  (`0x220C6F–0x220C99`).

At `0x220C9D–0x220CC1` form the full binary32 displacement from
velocity times step seconds, then add signed step milliseconds to
the stored age. Type 2 bypasses the common age limit. For the other
covered types, post-increment age strictly above 5,000 removes the
entry. For types 0, 1 and 4 this first tries to resolve the source.
If found, read the source action speed at `0x047700`, zero signed
combo `+0x70`, and read that speed again. Divide the old speed by the
new in binary32, multiply by the source clock `+0x6C` converted to
binary32, and truncate toward zero back to that signed clock. The
connected nonlocal source follows these writes. Type 3 skips them.
This expiry adds no direct draw
(`0x220CCA–0x220D75`).

For an unexpired projectile, the full displacement length uses
binary32 component squares and sequential addition, square root
and a binary32 result. The substep count is the signed truncation
of binary32 two times that length plus one. Divide each displacement
component by the binary32 count to get the repeated substep
displacement. The hit impulse starts from a copy of velocity,
normalized if its binary32 squared length is positive, then scaled
by binary32 five times `+0x54`. Its Z is overwritten with binary32
three times `+0x54`. The full source is looked up for the contact
walk. A signed count at most zero bypasses the walk
(`0x221465–0x221590`, `0x023F20`, `0x069020`).

## 4. Direct contact walk and outputs

At each substep, reset the creature-stop and hit-success indicators.
Begin again at the first key in the world creature map at
`world+0x04`. The order is ascending signed 64-bit key, comparing
the high word signed and then the low word unsigned. The projectile
uses the stored live position and the actor's live `+0x10` position.
Drawn/interpolated copies have no role at these sites
(`0x077D90–0x077E03`, `0x221590–0x221659`).

1. Skip a null creature or an ordered health at most zero
   (`+0x16C`). When the source exists, reject the source pointer
   itself, require the live relationship result at `0x1988A0`,
   and reject an ID equal to the source's full linked-creature
   key at `+0x11C0/+0x11C4`. A missing source or type 3 with
   variant 2 bypasses these three source-based tests.
2. Form actor bounds from the three sizes at `+0x80` times
   binary32 0.5, and projectile bounds from `+0x4C`. Each bound
   offset is multiplied by binary32 65,536 and converted toward
   zero to signed fixed units. For all axes X, Y, Z, require the
   projectile upper bound at least the actor lower bound, then
   require the projectile lower bound strictly below the actor
   upper bound in the same axis order.
3. Test line of sight at `0x19EE90` from projectile to target
   with object flag one and limit 200. With target roll `+0x128`
   nonzero, insert its full ID into the source's deferred set
   `+0x11B4` if the source exists, then skip the contact.
   The missing-source roll route skips without inserting.

Rejections in these three stages add no projectile draw
(`0x2215E0–0x221971`, `0x012220`, `0x0122E0`,
`0x194660`, `0x194690`).

Types 0, 1 and 4 stop the direct candidate walk on its first
admitted contact, even if the hit helper returns false. With
charge byte nonzero they set the stop indicator without evaluating
the direct hit. Type 3 evaluates a direct hit only when the old
and new ages divided by 1,000 differ. Type 2 requires new age
strictly above 300 and a change of the 100 ms quotient. The bucket
tests use signed division truncated toward zero. Old age is new
age minus step milliseconds. Types 2 and 3 continue the candidate
walk after evaluation. Cadence rejection skips both the direct
draws and the direct target-clock write
(`0x221971–0x221A38`, `0x221EC1–0x221F24`).

### 4.1 Direct draws and record selection

For a resolved source and an uncharged type 0 or 2 that reaches
direct evaluation, draw at `0x221A71`. Convert the draw to binary32,
multiply by two, divide by 32,767, subtract from one, multiply by
0.05, and add 0.1. Type 2 then multiplies the result by 0.2.
Read the source duration at `0x03D1A0`, convert it to binary32
and divide by 300, then multiply it by the accumulated result.
Draw again at `0x221AEE` and double the result when the draw's
remainder by eight is zero. Only after these two draws does the
connected ownership gate at `0x221B12–0x221B27` bypass the
nonlocal source's mana credit. Types 1, 3 and 4 bypass the
two draws.

Next, a resolved source calls the critical helper at `0x0444A0`.
It draws at `0x0444E5` and compares the probability strictly
above the binary32 draw divided by 32,767. A resident type-11
buff takes the probability-one path and still reaches the draw.
Otherwise probability comes from `0x03EA40` plus binary32
0.15 times source `+0x1190`. This helper is reached at
`0x221B5E` in direct contact. A missing source provides false
without this draw. Double `+0x48` for a critical and floor
an ordered negative amount at zero. The separate owner-only
weapon proc at `0x221BC7`, with 0.15 passed to `0x0441A0`,
is bypassed before the call for a connected nonlocal source
(`0x221B96–0x221BB7`, `0x221CF3–0x221D03`).

Variant 2 uses the separate beneficial route. It requires a
resolved source, a false result from `0x196CA0` and target
controller byte `+0x60` other than six. Append a type-0 hit with
the record's full source ID, the target's live ID and position, the
critical flag, and the sign-negated amount. Source controller
kind one halves that amount. In connected mode the direct
application requires both creature pointers equal the local pointer.
A connected nonlocal source can therefore append the hit without
applying it at this site (`0x221D03–0x221E22`).

Other variants call `0x196D30` with the live target and source,
amount, critical result, charge byte, `+0x54`, current projectile
position, prepared impulse, the collector and scratch, type-1
flag, action argument zero, the projectile record and a final
enabled flag. A true result sets substep hit-success. After either
output route, even on a false helper result or a skipped variant-2
append, raise target signed `+0x134` to record signed `+0x6C`
if the latter is strictly greater
(`0x221E28–0x221EBB`).

### 4.2 Shared hit-helper boundary

The helper at `0x196D30` rejects ordered nonpositive health and,
in connected mode, the local target pointer before its own
draws or outputs. Direct draws already consumed by the
projectile and the caller's later target-clock comparison are
independent of that result. With a nonlocal target and resolved
source, the helper can append hit, sound and effect records.
Its ordinary terminal application at `0x1986B8` is bypassed in
connected mode. Earlier connected writes inside defensive and
equipment paths can still occur. Source live action 74 or 91
rejects a target before these paths if target byte `+0x5C`
has its low bit clear (`0x196DB4–0x196DD3`).

For a resolved source that reaches the ordinary defense sequence,
the kind-1 protection draw is `0x196EF9` before a type-3 record.
The active guard path mutates guard and mana, appends type 1,
then draws for sound at `0x1971C0`. Ward can append type 5 and
then draw for sound at `0x197478`. These are the same helper
sites traced in `alpha-connected-melee-candidates.md` §5.
Upstream checks on the source's live action still apply.

The continuing type-1 projectile helper call selects fixed sound
42 at target position with pitch and volume one if source
specialization `+0x141` is one. Otherwise it selects sound 39
with pitch 1.5 and volume one. This bypasses the ordinary
hit-sound draw at `0x197D1B`. Other covered projectile types
can reach that draw in the ordinary sound branch. The later
critical species-sound draw at `0x198258` requires critical
nonzero and target controller kind other than six. It precedes the
species selector and occurs even if that species adds no sound.
The stun draw at `0x198545` requires a resolved source, target
signed `+0x12C` strictly below minus 3,000, and target kind other
than six. Live source actions 12, 58, 68, 69 and 93 replace the
incoming strength with one before the probability calculations.
A probability of zero still reaches the draw once admitted.

If live source action is 28, the helper earlier draws at
`0x196E20` and appends an effect before the defense sequence.
The effect uses target live position, type 8, count one,
velocity (0, 0, 10), color (0.6, 0.6, 1, 1) and a binary32
offset-`0x34` value of 0.1 plus draw times 0.1 divided by 32,767.
A missing source passing the local-target and health gates has a
distinct early draw at `0x197E3D` and writes binary32 0.5 to target
`+0x1184`. The later branches specific to these two routes
remain partial
(`0x196DD1–0x196EC1`, `0x197AC1–0x197E5A`,
`0x1981CB–0x1983EC`, `0x1983EC–0x198563`).

## 5. Motion, terrain and retirement after direct contact

The motion branch follows the direct candidate loop, including
on a creature stop. Type 3 goes directly to the next substep
without advancing the position or probing terrain. Type 2 first
tests a change of the signed age quotient by 200. On a change,
it draws at `0x221F81` and appends sound 15 at the current
projectile position, volume one. Its pitch is binary32 one plus
draw times 0.25 divided by 32,767. This test and draw are per
substep, using the same old and new ages across that world step
(`0x221F1E–0x221FCD`).

For the other covered types, and for type 2 after its sound
branch, convert the substep displacement to fixed units and add
it to the resident position. Query the cell at that advanced
position. The low five bits of cell byte `+0x03` select passable
values 0 and 2 at `0x03B480`. A solid cell rolls back the
displacement and sets terrain contact. A type-0 solid-cell
contact also appends a deterministic cell-colored effect, type 4,
at the rolled-back position, with Z velocity ten and binary32
0.1 at effect offset `+0x34`. A passable cell goes to the ray at
`0x1A35D0` with the advanced position, the negated unit
velocity as the direction argument, flags zero and one, and a
maximum distance of velocity length times the full step seconds.
Returned distance strictly below that maximum likewise rolls back
and sets terrain contact. The direction helper at `0x027870`
divides by the binary32 length without a zero check
(`0x221FCD–0x2222D3`, `0x068E20`).

Terrain contact without creature stop retimes a resolved
source's action clock and zeroes combo using the same speed
sequence as common expiry. This runs for a connected nonlocal
source. Type 2 remains resident in this branch. A true substep hit
result with a resolved source increments its combo, calls
`0x0453B0` and clears `+0x74` regardless of terrain contact.
It then zeros velocity if terrain contact was set and proceeds
to the next substep (`0x2222D9–0x2223C8`).

For types 0, 1 and 4, creature stop or terrain contact takes
the impact branch. Type 1 or nonzero charge selects a second
creature walk. On terrain contact without a creature stop this
branch first draws at `0x222483` and appends sound 42 for type 1
variant 2, 39 for other type 1, or 20 for other charged records.
Volume is one and pitch is binary32 one plus draw times 0.4
divided by 32,767. The sound uses the current projectile position.

The second walk uses the same ordered world map, resolved-source
relationship and full linked-ID exclusion, target live position,
asymmetric bounds and line-of-sight limit, with a fixed projectile
half-width of five blocks. With a resolved source it rejects
nonpositive target health and defers a nonzero roll into the
source set. It takes the critical-helper draw per admitted
resolved-source target at the call `0x2228C7`. The 0.25
weapon-proc call at `0x222943` is bypassed for a connected
nonlocal source. There is no repeat of the direct mana draws.
It calls the shared hit helper at `0x222AB8` with the same
type-1 flag and current projectile state. The second walk uses
that helper even for variant 2. A true result joins the current
substep's success indicator. This walk has no direct counterpart
to the target `+0x134` comparison in §4.1
(`0x222457–0x222B10`).

At retirement, a true substep hit result and resolved source
increment combo, call `0x0453B0` and clear `+0x74`.
The combo sequence has no connected ownership exclusion here.
Type 1 variant 2 is converted in place to type 3. Set age to
3,000 for its old zero charge byte, or zero for its old nonzero
charge. Multiply `+0x48` by binary32 0.05 and set `+0x4C` to
binary32 five times the old `+0x54` plus five. Clear `+0x54`
and `+0x5C`. The variant, current position and velocity survive.
This ends its current visit. The other retiring types enter the
deferred removal walk (`0x222B10–0x222B92`).

## 6. Local output and outgoing hit gate

Projectile hit, sound and effect records use the same frame
collector as actor outputs. The late hit presentation follows
the shared target-resolution and record-type gates in
`alpha-connected-hit-consumption.md` §4. The sound walk at
`0x0955B1–0x09568B` consumes the collected position, channel,
pitch and volume. These paths are eligible for nonlocal shot
outputs. With a nonzero socket, the later hit-send collector
compares each hit's full source ID with the local creature ID
at `0x09C08C–0x09C0B4`. A hit carrying the nonlocal projectile's
source ID therefore stays out of the outgoing hit queue.
The helper can produce other records whose source fields are its
own branch's responsibility.

## Evidence limits

The normal connected relay and consumer are established above.
The optional embedded host drain at client `0x08CE94` takes a
different path into the frame's output collector before the client
world step. Its complete queue lifetime, dedicated projectile
collision authority, unsupported type values, partial socket
transfers, rendering, the collision-ray internals and meanings of
opaque record fields remain open. The shared hit helper's complete
live-action and equipment subbranches, the missing-source impact
variant, and stock reachability of sound 19 at `0x222406` likewise
remain open.
